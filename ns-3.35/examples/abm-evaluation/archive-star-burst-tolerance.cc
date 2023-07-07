
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <fstream>
#include <iomanip>
#include <map>
#include <ctime>
#include <set>
#include <unordered_map>

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/timestamp-tag.h"
#include "ns3/gen-queue-disc.h"
#include "ns3/red-queue-disc.h"
#include "ns3/fq-pie-queue-disc.h"
#include "ns3/fq-codel-queue-disc.h"
#include "ns3/shared-memory.h"

# define PACKET_SIZE 1400
# define GIGA 1000000000

/*Buffer Management Algorithms*/
# define DT 101
# define FAB 102
# define CS 103
# define IB 104
# define ABM 110


/*Congestion Control Algorithms*/
# define RENO 0
# define CUBIC 1
# define DCTCP 2
# define HPCC 3
# define POWERTCP 4
# define HOMA 5
# define TIMELY 6
# define THETAPOWERTCP 7

// #define PORT_END 65530

extern "C"
{
	#include "cdf.h"
}


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("STAR_BURST_TOLERANCE");

double alpha_values[8]={1};

Ptr<OutputStreamWrapper> fctOutput;
AsciiTraceHelper asciiTraceHelper;

Ptr<OutputStreamWrapper> torStats;
AsciiTraceHelper torTraceHelper;

Ptr<SharedMemoryBuffer> sharedMemory;
QueueDiscContainer bottleneckQueueDiscs;


double poission_gen_interval(double avg_rate)
{
    if (avg_rate > 0)
       return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
    else
       return 0;
}


template<typename T>
T rand_range (T min, T max)
{
    return min + ((double)max - min) * rand () / RAND_MAX;
}


void StarTopologyInvokeToRStats(Ptr<OutputStreamWrapper> stream, uint32_t BufferSize, double nanodelay, uint32_t nPrior){

	int64_t currentNanoSeconds = Simulator::Now().GetNanoSeconds();

	Ptr<SharedMemoryBuffer> pbuffer = sharedMemory;
	QueueDiscContainer pqueues = bottleneckQueueDiscs;

	*stream->GetStream() << currentNanoSeconds << " " << double(BufferSize)/1e6 << " " << 100 * double(pbuffer->GetOccupiedBuffer())/BufferSize;
	for (uint32_t port = 0; port<pqueues.GetN(); port++) {
		Ptr<GenQueueDisc> genDisc = DynamicCast<GenQueueDisc>(pqueues.Get(port));
		double remaining = genDisc->GetRemainingBuffer();
		for (uint32_t priority = 0; priority<nPrior; priority++)  {
			uint32_t qSize = genDisc->GetQueueDiscClass(priority)->GetQueueDisc()->GetNBytes();
			auto [th, sentBytes] = genDisc->GetThroughputQueue(priority, nanodelay);  // only with c++17
			uint64_t droppedBytes = genDisc->GetDroppedBytes(priority);
			uint64_t maxSize = genDisc->GetAlpha(priority)*remaining;
			*stream->GetStream() << " " << qSize << " " << th << " " << sentBytes << " " << droppedBytes << " " << maxSize;
		}
	}
	*stream->GetStream() << std::endl;

	Simulator::Schedule(NanoSeconds(nanodelay), StarTopologyInvokeToRStats, stream, BufferSize, nanodelay, nPrior);
}


double baseRTTNano;
// double nicBw;
void TraceMsgFinish (Ptr<OutputStreamWrapper> stream, double size, double start, bool incast,uint32_t prior )
{
  double fct, standalone_fct,slowdown;
	fct = Simulator::Now().GetNanoSeconds()- start;
	// standalone_fct = baseRTTNano + size*8.0/ nicBw;
	// slowdown = fct/standalone_fct;

	*stream->GetStream ()
	<< Simulator::Now().GetNanoSeconds()
  	<< " " << size
  	<< " " << fct
  	// << " " << standalone_fct
  	// << " " << slowdown
  	<< " " << baseRTTNano
  	<< " " << start
	<< " " << prior
	<< " " << incast
	<< std::endl;
}


int
main (int argc, char *argv[])
{
	CommandLine cmd;

	double START_TIME = 10;
	double FLOW_LAUNCH_END_TIME = 13;
	double END_TIME = 20;
	cmd.AddValue ("StartTime", "Start time of the simulation", START_TIME);
	cmd.AddValue ("EndTime", "End time of the simulation", END_TIME);
	cmd.AddValue ("FlowLaunchEndTime", "End time of the flow launch period", FLOW_LAUNCH_END_TIME);

	unsigned randomSeed = 8;
	cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);

	uint64_t serverLeafCapacity = 1;
	uint64_t leafSinkCapacity = 1;
	double serverLeafLinkLatency = 10;
	double leafSinkLinkLatency = 10;
	cmd.AddValue("serverLeafCapacity", "Server <-> Leaf capacity in Gbps", serverLeafCapacity);
	cmd.AddValue("leafSinkCapacity", "Leaf <-> Sink capacity in Gbps", leafSinkCapacity);
	cmd.AddValue("serverLeafLinkLatency", "Server <-> Leaf link latency in microseconds", serverLeafLinkLatency);
	cmd.AddValue("leafSinkLinkLatency", "Leaf <-> Sink link latency in microseconds", leafSinkLinkLatency);

	uint32_t TcpProt0=CUBIC;
	cmd.AddValue("TcpProt0","Tcp protocol 0",TcpProt0);

	uint32_t TcpProt1=DCTCP;
	cmd.AddValue("TcpProt1","Tcp protocol 1",TcpProt1);

	uint32_t BufferSize = 1000*1000*1;//Maria§
	double statBuf = 0; // fraction of buffer that is reserved
	cmd.AddValue ("BufferSize", "BufferSize in Bytes",BufferSize);
	cmd.AddValue ("statBuf", "staticBuffer in fraction of Total buffersize",statBuf);

	uint32_t algorithm = DT;
	cmd.AddValue ("algorithm", "Buffer Management algorithm", algorithm);

	/*RED Parameters*/
	uint32_t RedMinTh = 65;
	uint32_t RedMaxTh = 65;
	uint32_t UseEcn = 0;
	std::string ecnEnabled = "EcnDisabled";
	cmd.AddValue("RedMinTh", "Min Threshold for RED in packets", RedMinTh);
	cmd.AddValue("RedMaxTh", "Max Threshold for RED in packets", RedMaxTh);
	cmd.AddValue("UseEcn","Ecn Enabled",UseEcn);

	std::string sched = "roundRobin";
	cmd.AddValue ("sched", "scheduling", sched);

	uint32_t nPrior = 1; // number queues in switch ports
	cmd.AddValue ("nPrior", "number of priorities",nPrior);

	std::string alphasFile="/home/vamsi/ABM-ns3/ns-3.35/examples/abm-evaluation/alphas"; // On lakewood
	std::string cdfFileName = "/home/vamsi/FB_Simulations/ns-allinone-3.33/ns-3.33/examples/plasticine/DCTCP_CDF.txt";
	std::string cdfName = "WS";
	cmd.AddValue ("alphasFile", "alpha values file (should be exactly nPrior lines)", alphasFile);
	cmd.AddValue ("cdfFileName", "File name for flow distribution", cdfFileName);
	cmd.AddValue ("cdfName", "Name for flow distribution", cdfName);

	uint32_t printDelay= 1e3;
	cmd.AddValue("printDelay","printDelay in NanoSeconds", printDelay);

	std::string fctOutFile="./fcts.txt";
	cmd.AddValue ("fctOutFile", "File path for FCTs", fctOutFile);

	std::string torOutFile="./tor.txt";
	cmd.AddValue ("torOutFile", "File path for ToR statistic", torOutFile);

	uint32_t rto = 10*1000; // in MicroSeconds, 5 milliseconds.
	cmd.AddValue ("rto", "min Retransmission timeout value in MicroSeconds", rto);

	uint32_t numSinks = 2;
	// cmd.AddValue ("numSinks", "number of sinks", numSinks);

	/*Parse CMD*/
	cmd.Parse (argc,argv);

	uint32_t numNodes = 2;

	fctOutput = asciiTraceHelper.CreateFileStream (fctOutFile);

	*fctOutput->GetStream ()
	<< "time "
  	<< "flowsize "
  	<< "fct "
  	// << "basefct "
  	// << "slowdown "
  	<< "basertt "
  	<< "flowstart "
	<< "priority "
	<< "incast "
	<< std::endl;

	torStats = torTraceHelper.CreateFileStream (torOutFile);
	*torStats->GetStream ()
	<< "time "
	<< "bufferSizeMB "
	<< "occupiedBufferPct";
	for (int i=0; i<(numSinks+numNodes); i++) {
		for (int j=0; j<nPrior; j++) {
			*torStats->GetStream () << " port_" << i << "_queue_" << j <<"_qSize";
			// AnnC: [artemis-star-topology] How is this threshold not throughput?
			*torStats->GetStream () << " port_" << i << "_queue_" << j <<"_threshold";
			*torStats->GetStream () << " port_" << i << "_queue_" << j <<"_sentBytes";
			*torStats->GetStream () << " port_" << i << "_queue_" << j <<"_droppedBytes";
			*torStats->GetStream () << " port_" << i << "_queue_" << j <<"_maxSize";
		}
	}
	*torStats->GetStream () << std::endl;


	// AnnC: [artemis-star-topology] Not supporting static buffer for now.
	std::cout << "total buffer size = " << BufferSize << std::endl;
	// uint32_t staticBuffer = (double) BufferSize*statBuf/(SERVER_COUNT+SPINE_COUNT*LINK_COUNT);
	uint32_t staticBuffer = 0;
	BufferSize = BufferSize - staticBuffer; // BufferSize is the buffer pool which is available for sharing
	if(UseEcn){
		ecnEnabled = "EcnEnabled";
	}
	else{
		ecnEnabled = "EcnDisabled";
	}


	/*Reading alpha values from file*/
	std::string line;
	std::fstream aFile;
	aFile.open(alphasFile);
	uint32_t p=0;
	while( getline( aFile, line ) && p < 8 ){ // hard coded to read only 8 alpha values.
		std::istringstream iss( line );
		double a;
		iss >> a;
		alpha_values[p]=a;
		// std::cout << "Alpha-"<< p << " = "<< a << " " << alpha_values[p]<< std::endl;
		p++;
	}
	// AnnC: hard-code alpha for the first queue to be 8
	alpha_values[0] = 8;
	aFile.close();

	// AnnC: [artemis-star-topology] Uncertain what the calculation is for.
	double RTTBytes = (serverLeafCapacity*serverLeafLinkLatency+leafSinkCapacity*leafSinkLinkLatency)*2*1e3/8;
	uint32_t RTTPackets = RTTBytes/PACKET_SIZE + 1;
	baseRTTNano = (serverLeafLinkLatency+leafSinkLinkLatency)*2*1e3;
	// nicBw = serverLeafCapacity+leafSinkCapacity;

    // Config::SetDefault("ns3::GenQueueDisc::updateInterval", UintegerValue(alphaUpdateInterval*linkLatency*8*1000));
    Config::SetDefault("ns3::GenQueueDisc::staticBuffer", UintegerValue(staticBuffer));
    Config::SetDefault("ns3::GenQueueDisc::BufferAlgorithm", UintegerValue(algorithm));
    Config::SetDefault("ns3::SharedMemoryBuffer::BufferSize", UintegerValue(BufferSize));
    Config::SetDefault ("ns3::FifoQueueDisc::MaxSize", QueueSizeValue (QueueSize ("100MB")));

    /*General TCP Socket settings. Mostly used by various congestion control algorithms in common*/
	Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds (10))); // syn retry interval
    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MicroSeconds (rto)) );  //(MilliSeconds (5))
    Config::SetDefault ("ns3::TcpSocketBase::RTTBytes", UintegerValue ( RTTBytes ));  //(MilliSeconds (5))
    Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", TimeValue (NanoSeconds (10))); //(MicroSeconds (100))
    Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue (MicroSeconds (200))); //TimeValue (MicroSeconds (80))
	Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1073725440)); //1073725440
	Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1073725440));
    Config::SetDefault ("ns3::TcpSocket::ConnCount", UintegerValue (6));  // Syn retry count
    Config::SetDefault ("ns3::TcpSocketBase::Timestamp", BooleanValue (true));
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (PACKET_SIZE));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
    Config::SetDefault ("ns3::TcpSocket::PersistTimeout", TimeValue (Seconds (20)));

	/****************************************************************/
	/* Swap to a star topology                                      */
	/* Reference: https://github.com/netsyn-princeton/cc-aqm-bm-ns3 */
	/****************************************************************/

	NodeContainer nodecontainers;
	nodecontainers.Create(numNodes);
	NodeContainer nd;
	nd.Create (1);
	NodeContainer sinkcontainers; 
	sinkcontainers.Create(numSinks);

	InternetStackHelper stack;
	stack.InstallAll ();

	Ipv4GlobalRoutingHelper globalRoutingHelper;
	stack.SetRoutingHelper (globalRoutingHelper);


	/****************************************************************/
	/* Server0 <-> Leaf, Leaf <-> Sink0                             */
	/* Hard-coded                                                   */
	/****************************************************************/

	TrafficControlHelper tc0;
    uint16_t handle0;
    TrafficControlHelper::ClassIdList cid0;

    /*CC Configuration*/
    std::cout << "TcpProt0=" << TcpProt0 << std::endl;
    switch (TcpProt0){
    	case RENO:
			Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpNewReno::GetTypeId()));
			Config::SetDefault ("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
			Config::SetDefault("ns3::GenQueueDisc::nPrior", UintegerValue(nPrior));
			Config::SetDefault("ns3::GenQueueDisc::RoundRobin", UintegerValue(1));
			Config::SetDefault("ns3::GenQueueDisc::StrictPriority", UintegerValue(0));
			handle0 = tc0.SetRootQueueDisc ("ns3::GenQueueDisc");
		    cid0 = tc0.AddQueueDiscClasses (handle0, nPrior , "ns3::QueueDiscClass");
		    for(uint32_t num=0;num<nPrior;num++){
				tc0.AddChildQueueDisc (handle0, cid0[num], "ns3::FifoQueueDisc");
			}
			break;
    	case CUBIC:
        	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpCubic::GetTypeId()));
        	Config::SetDefault ("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
        	Config::SetDefault("ns3::GenQueueDisc::nPrior", UintegerValue(nPrior));
        	Config::SetDefault("ns3::GenQueueDisc::RoundRobin", UintegerValue(1));
			Config::SetDefault("ns3::GenQueueDisc::StrictPriority", UintegerValue(0));
			handle0 = tc0.SetRootQueueDisc ("ns3::GenQueueDisc");
			cid0 = tc0.AddQueueDiscClasses (handle0, nPrior , "ns3::QueueDiscClass");
		    for(uint32_t num=0;num<nPrior;num++){
				tc0.AddChildQueueDisc (handle0, cid0[num], "ns3::FifoQueueDisc");
			}
        	break;
        case DCTCP:
        	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpDctcp::GetTypeId()));
			Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));
			Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1.0));
			Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (RedMinTh*PACKET_SIZE));
			Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (RedMaxTh*PACKET_SIZE));
			Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("100MB"))); // This is just for initialization. The buffer management algorithm will take care of the rest.
			Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("On"));
			Config::SetDefault ("ns3::RedQueueDisc::LInterm", DoubleValue (0.0));
			Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
			Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
			Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
			Config::SetDefault ("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
			UseEcn =1;
			ecnEnabled = "EcnEnabled";
			Config::SetDefault("ns3::GenQueueDisc::nPrior", UintegerValue(nPrior));
			Config::SetDefault("ns3::GenQueueDisc::RoundRobin", UintegerValue(1));
			Config::SetDefault("ns3::GenQueueDisc::StrictPriority", UintegerValue(0));
			handle0 = tc0.SetRootQueueDisc ("ns3::GenQueueDisc");
		    cid0 = tc0.AddQueueDiscClasses (handle0, nPrior , "ns3::QueueDiscClass");
			for(uint32_t num=0;num<nPrior;num++){
				tc0.AddChildQueueDisc (handle0, cid0[num], "ns3::RedQueueDisc", "MinTh", DoubleValue (RedMinTh*PACKET_SIZE), "MaxTh", DoubleValue (RedMaxTh*PACKET_SIZE));
			}
			break;
		default:
			std::cout << "Error in CC configuration" << std::endl;
			return 0;
    }
    std::cout << "done configuring CC" << std::endl;

	// AnnC: [artemis-star-topology] Add AQM later; the CC above has already SetRootQueueDisc to GenQueueDisc
    // if (queueDiscType.compare ("PfifoFast") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize",
    //                                   QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscSize)));
    // }
    // else if (queueDiscType.compare ("ARED") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::RedQueueDisc");
	// 	Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
	// 	Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
    //                       QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscSize)));
    // }
    // else if (queueDiscType.compare ("CoDel") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::CoDelQueueDisc");
	// 	Config::SetDefault ("ns3::CoDelQueueDisc::MaxSize",
    //                       QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscSize)));
    // }
    // else if (queueDiscType.compare ("FqCoDel") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::FqCoDelQueueDisc");
	// 	Config::SetDefault ("ns3::FqCoDelQueueDisc::MaxSize",
    //                       QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscSize)));
    // }
    // else if (queueDiscType.compare ("PIE") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::PieQueueDisc");
	// 	Config::SetDefault ("ns3::PieQueueDisc::MaxSize",
    //                       QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscSize)));
    // }
    // else if (queueDiscType.compare ("prio") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::PrioQueueDisc", "Priomap",
    //                                                     StringValue ("0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1"));
	// 	TrafficControlHelper::ClassIdList cid = tchBottleneck.AddQueueDiscClasses (handle, 2, "ns3::QueueDiscClass");
	// 	tchBottleneck.AddChildQueueDisc (handle, cid[0], "ns3::FifoQueueDisc");
	// 	tchBottleneck.AddChildQueueDisc (handle, cid[1], "ns3::RedQueueDisc");
    // }

	// bool dropTail = (queueDiscType == "droptail");
	// uint32_t queueDiscSize = 10;


	Ipv4AddressHelper address;
	address.SetBase ("10.1.0.0", "255.255.252.0");

	sharedMemory = CreateObject<SharedMemoryBuffer>();
	sharedMemory->SetAttribute("BufferSize",UintegerValue(BufferSize));
	sharedMemory->SetSharedBufferSize(BufferSize);

	int portid = 0;

	std::cout << "checkpoint0" << std::endl;

	PointToPointHelper accessLink;
	accessLink.SetDeviceAttribute ("DataRate", DataRateValue(DataRate(serverLeafCapacity*GIGA)));
	accessLink.SetChannelAttribute ("Delay", TimeValue(MicroSeconds(serverLeafLinkLatency)));
	// if (dropTail) {
	// 	accessLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));
	// }

	/* Server0 <--> Leaf */
	uint32_t server=0;
	NetDeviceContainer netDeviceContainer = accessLink.Install(nodecontainers.Get(server), nd.Get(0));
	QueueDiscContainer queuedisc = tc0.Install(netDeviceContainer.Get(1));
	bottleneckQueueDiscs.Add(queuedisc.Get(0));
	Ptr<GenQueueDisc> genDisc = DynamicCast<GenQueueDisc> (queuedisc.Get(0));
	genDisc->SetPortId(portid++);
	std::cout << "after" << std::endl;
	std::cout << portid << "after" << std::endl;
	// AnnC: [artemis-star-topology] Assume to be DT.
	genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	genDisc->setPortBw(serverLeafCapacity);
	genDisc->SetSharedMemory(sharedMemory);
	genDisc->SetBufferAlgorithm(DT);
	for(uint32_t n=0;n<nPrior;n++){
		genDisc->alphas[n] = alpha_values[n];
	}
	address.NewNetwork ();
	address.Assign (netDeviceContainer);

	std::cout << "done with server0<->leaf links" << std::endl;


	PointToPointHelper bottleneckLink;
	bottleneckLink.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (leafSinkCapacity*GIGA)));
	bottleneckLink.SetChannelAttribute ("Delay", TimeValue(MicroSeconds(leafSinkLinkLatency)));
	// if (dropTail) {
	// 	bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (std::to_string(queueDiscSize) + "p"));
	// }

	/* Leaf <--> Sink0 */
	Ipv4InterfaceContainer nsInterface;
	uint32_t sink = 0;
	NetDeviceContainer devicesBottleneckLink = bottleneckLink.Install (nd.Get (0), sinkcontainers.Get (sink));
	QueueDiscContainer qdiscs = tc0.Install (devicesBottleneckLink.Get(0));
	bottleneckQueueDiscs.Add(qdiscs.Get(0));
	Ptr<GenQueueDisc> genDisc0 = DynamicCast<GenQueueDisc> (qdiscs.Get(0));
	genDisc0->SetPortId(portid++);
	genDisc0->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	genDisc0->setPortBw(leafSinkCapacity);
	genDisc0->SetSharedMemory(sharedMemory);
	switch(algorithm){
		case DT:
			genDisc0->SetBufferAlgorithm(DT);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc0->alphas[n] = alpha_values[n];
			}
			break;
		case FAB:
			genDisc0->SetBufferAlgorithm(FAB);
			genDisc0->SetFabWindow(MicroSeconds(5000));
			genDisc0->SetFabThreshold(15*PACKET_SIZE);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc0->alphas[n] = alpha_values[n];
			}
			break;
		case CS:
			genDisc0->SetBufferAlgorithm(CS);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc0->alphas[n] = alpha_values[n];
			}
			break;
		case IB:
			genDisc0->SetBufferAlgorithm(IB);
			genDisc0->SetAfdWindow(MicroSeconds(50));
			genDisc0->SetDppWindow(MicroSeconds(5000));
			genDisc0->SetDppThreshold(RTTPackets);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc0->alphas[n] = alpha_values[n];
				genDisc0->SetQrefAfd(n,uint32_t(RTTBytes));
			}
			break;
		case ABM:
			genDisc0->SetBufferAlgorithm(ABM);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc0->alphas[n] = alpha_values[n];
			}
			break;
		default:
			std::cout << "Error in buffer management configuration. Exiting!";
			return 0;
	}

	address.NewNetwork ();
	Ipv4InterfaceContainer interfacesBottleneck = address.Assign (devicesBottleneckLink);
	nsInterface.Add (interfacesBottleneck.Get (1));


	std::cout << "start installing applications" << std::endl;

	// Application0
	// Install applications
	NS_LOG_INFO ("Initialize CDF table");
	struct cdf_table* cdfTable = new cdf_table ();
	init_cdf (cdfTable);
	load_cdf (cdfTable, cdfFileName.c_str ());

	uint32_t portnumber = 9;
	uint32_t flowcount = 0;
	srand(randomSeed);
	NS_LOG_INFO ("Initialize random seed: " << randomSeed);
	// uint32_t node = 0;
	// uint32_t sink = 0;

	double startTime = START_TIME + 1;
	// double startTime = START_TIME + poission_gen_interval(0.2);
	// while (startTime >= FLOW_LAUNCH_END_TIME || startTime <= START_TIME) {
	// 	startTime = START_TIME + poission_gen_interval(0.2);
	// }

	uint64_t flowSize = 1e9;
	// uint64_t flowSize = gen_random_cdf(cdfTable);
	// while (flowSize == 0) { 
	// 	flowSize = gen_random_cdf(cdfTable); 
	// }

	// ACK packets are prioritized
	uint64_t flowPriority = rand_range((u_int32_t)1,nPrior-1);

	InetSocketAddress ad(nsInterface.GetAddress(0), portnumber);
	Address sinkAddress(ad);

	std::cout << "Sending from server " << server << " to sink " << sink << ": ";
	std::cout << "startTime=" << startTime << ", flowSize=" << flowSize << ", flowPriority=" << flowPriority << std::endl;

	Ptr<BulkSendApplication> bulksend = CreateObject<BulkSendApplication>();
	bulksend->SetAttribute("Protocol",TypeIdValue(TcpSocketFactory::GetTypeId()));
	bulksend->SetAttribute("Remote",AddressValue(sinkAddress)); 
	bulksend->SetAttribute ("SendSize", UintegerValue (flowSize));
	bulksend->SetAttribute ("MaxBytes", UintegerValue(flowSize));
	bulksend->SetAttribute("FlowId", UintegerValue(flowcount++));
	bulksend->SetAttribute("InitialCwnd", UintegerValue (4));
	bulksend->SetAttribute("priorityCustom",UintegerValue(flowPriority));
	bulksend->SetAttribute("priority",UintegerValue(flowPriority));
	bulksend->SetStartTime (Seconds(startTime));
	bulksend->SetStopTime (Seconds (END_TIME));
	nodecontainers.Get(server)->AddApplication(bulksend);

	PacketSinkHelper packetSink("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), portnumber));
	ApplicationContainer sinkApp = packetSink.Install(sinkcontainers.Get(sink));
	sinkApp.Get(0)->SetAttribute("Protocol",TypeIdValue(TcpSocketFactory::GetTypeId()));
	sinkApp.Get(0)->SetAttribute("TotalQueryBytes",UintegerValue(flowSize));
	sinkApp.Get(0)->SetAttribute("flowId", UintegerValue(flowcount++));
	sinkApp.Get(0)->SetAttribute("senderPriority",UintegerValue(flowPriority));
	// ACK packets are prioritized
	sinkApp.Get(0)->SetAttribute("priorityCustom",UintegerValue(0));
	sinkApp.Get(0)->SetAttribute("priority",UintegerValue(0));
	sinkApp.Start(Seconds(0));
	sinkApp.Stop(Seconds(END_TIME));
	sinkApp.Get(0)->TraceConnectWithoutContext("FlowFinish", MakeBoundCallback(&TraceMsgFinish, fctOutput));

	portnumber++;


	/****************************************************************/
	/* Server1 <-> Leaf, Leaf <-> Sink1                             */
	/* Hard-coded                                                   */
	/****************************************************************/

	TrafficControlHelper tc1;
    uint16_t handle1;
    TrafficControlHelper::ClassIdList cid1;

    /*CC Configuration*/
    std::cout << "TcpProt1=" << TcpProt1 << std::endl;
	switch (TcpProt1){
    	case RENO:
			Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpNewReno::GetTypeId()));
			Config::SetDefault ("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
			Config::SetDefault("ns3::GenQueueDisc::nPrior", UintegerValue(nPrior));
			Config::SetDefault("ns3::GenQueueDisc::RoundRobin", UintegerValue(1));
			Config::SetDefault("ns3::GenQueueDisc::StrictPriority", UintegerValue(0));
			handle1 = tc1.SetRootQueueDisc ("ns3::GenQueueDisc");
		    cid1 = tc1.AddQueueDiscClasses (handle1, nPrior , "ns3::QueueDiscClass");
		    for(uint32_t num=0;num<nPrior;num++){
				tc1.AddChildQueueDisc (handle1, cid1[num], "ns3::FifoQueueDisc");
			}
			break;
    	case CUBIC:
        	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpCubic::GetTypeId()));
        	Config::SetDefault ("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
        	Config::SetDefault("ns3::GenQueueDisc::nPrior", UintegerValue(nPrior));
        	Config::SetDefault("ns3::GenQueueDisc::RoundRobin", UintegerValue(1));
			Config::SetDefault("ns3::GenQueueDisc::StrictPriority", UintegerValue(0));
			handle1 = tc1.SetRootQueueDisc ("ns3::GenQueueDisc");
			cid1 = tc1.AddQueueDiscClasses (handle1, nPrior , "ns3::QueueDiscClass");
		    for(uint32_t num=0;num<nPrior;num++){
				tc1.AddChildQueueDisc (handle1, cid1[num], "ns3::FifoQueueDisc");
			}
        	break;
        case DCTCP:
        	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpDctcp::GetTypeId()));
			Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));
			Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1.0));
			Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (RedMinTh*PACKET_SIZE));
			Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (RedMaxTh*PACKET_SIZE));
			Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("100MB"))); // This is just for initialization. The buffer management algorithm will take care of the rest.
			Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("On"));
			Config::SetDefault ("ns3::RedQueueDisc::LInterm", DoubleValue (0.0));
			Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
			Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
			Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
			Config::SetDefault ("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
			UseEcn =1;
			ecnEnabled = "EcnEnabled";
			Config::SetDefault("ns3::GenQueueDisc::nPrior", UintegerValue(nPrior));
			Config::SetDefault("ns3::GenQueueDisc::RoundRobin", UintegerValue(1));
			Config::SetDefault("ns3::GenQueueDisc::StrictPriority", UintegerValue(0));
			handle1 = tc1.SetRootQueueDisc ("ns3::GenQueueDisc");
		    cid1 = tc1.AddQueueDiscClasses (handle1, nPrior , "ns3::QueueDiscClass");
			for(uint32_t num=0;num<nPrior;num++){
				tc1.AddChildQueueDisc (handle1, cid1[num], "ns3::RedQueueDisc", "MinTh", DoubleValue (RedMinTh*PACKET_SIZE), "MaxTh", DoubleValue (RedMaxTh*PACKET_SIZE));
			}
			break;
		default:
			std::cout << "Error in CC configuration" << std::endl;
			return 0;
    }
    std::cout << "done configuring CC" << std::endl;

	// AnnC: [artemis-star-topology] Add AQM later; the CC above has already SetRootQueueDisc to GenQueueDisc
    // if (queueDiscType.compare ("PfifoFast") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize",
    //                                   QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscSize)));
    // }
    // else if (queueDiscType.compare ("ARED") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::RedQueueDisc");
	// 	Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
	// 	Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
    //                       QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscSize)));
    // }
    // else if (queueDiscType.compare ("CoDel") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::CoDelQueueDisc");
	// 	Config::SetDefault ("ns3::CoDelQueueDisc::MaxSize",
    //                       QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscSize)));
    // }
    // else if (queueDiscType.compare ("FqCoDel") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::FqCoDelQueueDisc");
	// 	Config::SetDefault ("ns3::FqCoDelQueueDisc::MaxSize",
    //                       QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscSize)));
    // }
    // else if (queueDiscType.compare ("PIE") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::PieQueueDisc");
	// 	Config::SetDefault ("ns3::PieQueueDisc::MaxSize",
    //                       QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscSize)));
    // }
    // else if (queueDiscType.compare ("prio") == 0)
    // {
	// 	handle = tc.SetRootQueueDisc ("ns3::PrioQueueDisc", "Priomap",
    //                                                     StringValue ("0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1"));
	// 	TrafficControlHelper::ClassIdList cid = tchBottleneck.AddQueueDiscClasses (handle, 2, "ns3::QueueDiscClass");
	// 	tchBottleneck.AddChildQueueDisc (handle, cid[0], "ns3::FifoQueueDisc");
	// 	tchBottleneck.AddChildQueueDisc (handle, cid[1], "ns3::RedQueueDisc");
    // }

	// bool dropTail = (queueDiscType == "droptail");
	// uint32_t queueDiscSize = 10;


	// Ipv4AddressHelper address;
	// address.SetBase ("10.1.0.0", "255.255.252.0");

	// sharedMemory = CreateObject<SharedMemoryBuffer>();
	// sharedMemory->SetAttribute("BufferSize",UintegerValue(BufferSize));
	// sharedMemory->SetSharedBufferSize(BufferSize);

	// int portid = 0;

	std::cout << "checkpoint1" << std::endl;

	// PointToPointHelper accessLink;
	// accessLink.SetDeviceAttribute ("DataRate", DataRateValue(DataRate(serverLeafCapacity*GIGA)));
	// accessLink.SetChannelAttribute ("Delay", TimeValue(MicroSeconds(serverLeafLinkLatency)));
	// if (dropTail) {
	// 	accessLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));
	// }

	/* Server1 <--> Leaf */
	server=1;
	NetDeviceContainer netDeviceContainer1 = accessLink.Install(nodecontainers.Get(server), nd.Get(0));
	QueueDiscContainer queuedisc1 = tc1.Install(netDeviceContainer1.Get(1));
	bottleneckQueueDiscs.Add(queuedisc1.Get(0));
	Ptr<GenQueueDisc> genDisc1 = DynamicCast<GenQueueDisc> (queuedisc1.Get(0));
	genDisc1->SetPortId(portid++);
	std::cout << "after" << std::endl;
	std::cout << portid << "after" << std::endl;
	// AnnC: [artemis-star-topology] Assume to be DT.
	genDisc1->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	genDisc1->setPortBw(serverLeafCapacity);
	genDisc1->SetSharedMemory(sharedMemory);
	genDisc1->SetBufferAlgorithm(DT);
	for(uint32_t n=0;n<nPrior;n++){
		genDisc1->alphas[n] = alpha_values[n];
	}
	address.NewNetwork ();
	address.Assign (netDeviceContainer1);

	std::cout << "done with server1<->leaf links" << std::endl;


	// PointToPointHelper bottleneckLink;
	// bottleneckLink.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (leafSinkCapacity*GIGA)));
	// bottleneckLink.SetChannelAttribute ("Delay", TimeValue(MicroSeconds(leafSinkLinkLatency)));
	// if (dropTail) {
	// 	bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (std::to_string(queueDiscSize) + "p"));
	// }

	/* Leaf <--> Sink1 */
	Ipv4InterfaceContainer nsInterface1;
	sink = 1;
	NetDeviceContainer devicesBottleneckLink1 = bottleneckLink.Install (nd.Get (0), sinkcontainers.Get (sink));
	QueueDiscContainer qdiscs1 = tc1.Install (devicesBottleneckLink1.Get(0));
	bottleneckQueueDiscs.Add(qdiscs1.Get(0));
	Ptr<GenQueueDisc> genDisc11 = DynamicCast<GenQueueDisc> (qdiscs1.Get(0));
	genDisc11->SetPortId(portid++);
	genDisc11->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	genDisc11->setPortBw(leafSinkCapacity);
	genDisc11->SetSharedMemory(sharedMemory);
	switch(algorithm){
		case DT:
			genDisc11->SetBufferAlgorithm(DT);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc11->alphas[n] = alpha_values[n];
			}
			break;
		case FAB:
			genDisc11->SetBufferAlgorithm(FAB);
			genDisc11->SetFabWindow(MicroSeconds(5000));
			genDisc11->SetFabThreshold(15*PACKET_SIZE);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc11->alphas[n] = alpha_values[n];
			}
			break;
		case CS:
			genDisc11->SetBufferAlgorithm(CS);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc11->alphas[n] = alpha_values[n];
			}
			break;
		case IB:
			genDisc11->SetBufferAlgorithm(IB);
			genDisc11->SetAfdWindow(MicroSeconds(50));
			genDisc11->SetDppWindow(MicroSeconds(5000));
			genDisc11->SetDppThreshold(RTTPackets);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc11->alphas[n] = alpha_values[n];
				genDisc11->SetQrefAfd(n,uint32_t(RTTBytes));
			}
			break;
		case ABM:
			genDisc11->SetBufferAlgorithm(ABM);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc11->alphas[n] = alpha_values[n];
			}
			break;
		default:
			std::cout << "Error in buffer management configuration. Exiting!";
			return 0;
	}

	address.NewNetwork ();
	Ipv4InterfaceContainer interfacesBottleneck1 = address.Assign (devicesBottleneckLink1);
	nsInterface1.Add (interfacesBottleneck1.Get (1));


	std::cout << "start installing applications" << std::endl;

	// Application1
	// Install applications
	// NS_LOG_INFO ("Initialize CDF table");
	// struct cdf_table* cdfTable = new cdf_table ();
	// init_cdf (cdfTable);
	// load_cdf (cdfTable, cdfFileName.c_str ());

	// uint32_t portnumber = 9;
	// uint32_t flowcount = 0;
	// srand(randomSeed);
	// NS_LOG_INFO ("Initialize random seed: " << randomSeed);
	// node = 1;
	// sink = 1;
	startTime = START_TIME + 4;
	// double startTime = START_TIME + poission_gen_interval(0.2);
	// while (startTime >= FLOW_LAUNCH_END_TIME || startTime <= START_TIME) {
	// 	startTime = START_TIME + poission_gen_interval(0.2);
	// }

	flowSize = 1e9;
	//flowSize = gen_random_cdf(cdfTable);
	while (flowSize == 0) { 
		flowSize = gen_random_cdf(cdfTable); 
	}

	// ACK packets are prioritized
	flowPriority = rand_range((u_int32_t)1,nPrior-1);

	InetSocketAddress ad1(nsInterface1.GetAddress(0), portnumber);
	Address sinkAddress1(ad1);

	std::cout << "Sending from server " << server << " to sink " << sink << ": ";
	std::cout << "startTime=" << startTime << ", flowSize=" << flowSize << ", flowPriority=" << flowPriority << std::endl;

	Ptr<BulkSendApplication> bulksend1 = CreateObject<BulkSendApplication>();
	bulksend1->SetAttribute("Protocol",TypeIdValue(TcpSocketFactory::GetTypeId()));
	bulksend1->SetAttribute("Remote",AddressValue(sinkAddress1)); 
	bulksend1->SetAttribute ("SendSize", UintegerValue (flowSize));
	bulksend1->SetAttribute ("MaxBytes", UintegerValue(flowSize));
	bulksend1->SetAttribute("FlowId", UintegerValue(flowcount++));
	bulksend1->SetAttribute("InitialCwnd", UintegerValue (4));
	bulksend1->SetAttribute("priorityCustom",UintegerValue(flowPriority));
	bulksend1->SetAttribute("priority",UintegerValue(flowPriority));
	bulksend1->SetStartTime (Seconds(startTime));
	bulksend1->SetStopTime (Seconds (END_TIME));
	nodecontainers.Get(server)->AddApplication(bulksend1);

	PacketSinkHelper packetSink1("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), portnumber));
	ApplicationContainer sinkApp1 = packetSink1.Install(sinkcontainers.Get(sink));
	sinkApp1.Get(0)->SetAttribute("Protocol",TypeIdValue(TcpSocketFactory::GetTypeId()));
	sinkApp1.Get(0)->SetAttribute("TotalQueryBytes",UintegerValue(flowSize));
	sinkApp1.Get(0)->SetAttribute("flowId", UintegerValue(flowcount++));
	sinkApp1.Get(0)->SetAttribute("senderPriority",UintegerValue(flowPriority));
	// ACK packets are prioritized
	sinkApp1.Get(0)->SetAttribute("priorityCustom",UintegerValue(0));
	sinkApp1.Get(0)->SetAttribute("priority",UintegerValue(0));
	sinkApp1.Start(Seconds(0));
	sinkApp1.Stop(Seconds(END_TIME));
	sinkApp1.Get(0)->TraceConnectWithoutContext("FlowFinish", MakeBoundCallback(&TraceMsgFinish, fctOutput));

	portnumber++;


	Simulator::Schedule(Seconds(START_TIME),StarTopologyInvokeToRStats,torStats, BufferSize, printDelay, nPrior);


	// AsciiTraceHelper ascii;
 //    p2p.EnableAsciiAll (ascii.CreateFileStream ("eval.tr"));
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	// NS_LOG_UNCOND("Running the Simulation...!");
	std::cout << "Running the Simulation...!" << std::endl;
	Simulator::Stop (Seconds (END_TIME));
	Simulator::Run ();
	Simulator::Destroy ();
	free_cdf (cdfTable);
	return 0;
}

