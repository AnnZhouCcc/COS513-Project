
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

NS_LOG_COMPONENT_DEFINE ("STAR_TEST_TIMELY");

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

	double START_TIME = 2;
	double FLOW_LAUNCH_END_TIME = 10;
	double END_TIME = 12;
	cmd.AddValue ("StartTime", "Start time of the simulation", START_TIME);
	cmd.AddValue ("EndTime", "End time of the simulation", END_TIME);
	cmd.AddValue ("FlowLaunchEndTime", "End time of the flow launch period", FLOW_LAUNCH_END_TIME);

	unsigned randomSeed = 8;
	cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);

	uint64_t serverLeafCapacity = 1;
	uint64_t leafSinkCapacity = 1;
	double serverLeafLinkLatencyLongRTT = 10;
	// double serverLeafLinkLatencyShortRTT = 10;
	double leafSinkLinkLatencyLongRTT = 10;
	// double leafSinkLinkLatencyShortRTT = 1;
	cmd.AddValue("serverLeafCapacity", "Server <-> Leaf capacity in Gbps", serverLeafCapacity);
	cmd.AddValue("leafSinkCapacity", "Leaf <-> Sink capacity in Gbps", leafSinkCapacity);
	cmd.AddValue("serverLeafLinkLatencyLongRTT", "Server <-> Leaf link latency in microseconds", serverLeafLinkLatencyLongRTT);
	// cmd.AddValue("serverLeafLinkLatencyShortRTT", "Server <-> Leaf link latency in microseconds", serverLeafLinkLatencyShortRTT);
	cmd.AddValue("leafSinkLinkLatencyLongRTT", "Leaf <-> Sink link latency in microseconds", leafSinkLinkLatencyLongRTT);
	// cmd.AddValue("leafSinkLinkLatencyShortRTT", "Leaf <-> Sink link latency in microseconds", leafSinkLinkLatencyShortRTT);

	uint32_t TcpProt=CUBIC;
	cmd.AddValue("TcpProt","Tcp protocol",TcpProt);

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

	uint32_t nPrior = 3; // number queues in switch ports
	cmd.AddValue ("nPrior", "number of priorities",nPrior);

	std::string alphasFile="/home/vamsi/ABM-ns3/ns-3.35/examples/abm-evaluation/alphas"; // On lakewood
	std::string cdfFileName = "/home/vamsi/FB_Simulations/ns-allinone-3.33/ns-3.33/examples/plasticine/DCTCP_CDF.txt";
	std::string cdfName = "WS";
	cmd.AddValue ("alphasFile", "alpha values file (should be exactly nPrior lines)", alphasFile);
	cmd.AddValue ("cdfFileName", "File name for flow distribution", cdfFileName);
	cmd.AddValue ("cdfName", "Name for flow distribution", cdfName);

	uint32_t printDelay= 1e7;
	cmd.AddValue("printDelay","printDelay in NanoSeconds", printDelay);

	std::string fctOutFile="./fcts.txt";
	cmd.AddValue ("fctOutFile", "File path for FCTs", fctOutFile);

	std::string torOutFile="./tor.txt";
	cmd.AddValue ("torOutFile", "File path for ToR statistic", torOutFile);

	std::string paramOutFile="./param.txt";
	cmd.AddValue ("paramOutFile", "File path for parameters", paramOutFile);

	uint32_t rto = 1000*1000; // in MicroSeconds, 5 milliseconds.
	cmd.AddValue ("rto", "min Retransmission timeout value in MicroSeconds", rto);

	uint32_t numSinks = 1;
	cmd.AddValue ("numSinks", "number of sinks", numSinks);

	uint32_t numNodes = 20;
	cmd.AddValue ("numNodes", "number of nodes", numNodes);

	double longRTTAlpha = 1;
	// double shortRTTAlpha = 1;
	cmd.AddValue("longRTTAlpha","the alpha value for long-RTT flows",longRTTAlpha);
	// cmd.AddValue("shortRTTAlpha","the alpha value for short-RTT flows",shortRTTAlpha);

	uint32_t numLongRTTFlows = 10;
	// uint32_t numShortRTTFlows = 10;
	cmd.AddValue("numLongRTTFlows","number of long-RTT flows",numLongRTTFlows);
	// cmd.AddValue("numShortRTTFlows","number of short-RTT flows",numShortRTTFlows);

	uint32_t fsModeLongRTT = 0;
	// uint32_t fsModeShortRTT = 0;
	cmd.AddValue("fsModeLongRTT","0:continuous flow, 1: bursty flow, 2: no flow",fsModeLongRTT);
	// cmd.AddValue("fsModeShortRTT","0:continuous flow, 1: bursty flow, 2: no flow",fsModeShortRTT);

	uint32_t longRTTIW = 4;
	// uint32_t shortRTTIW = 4;
	cmd.AddValue("longRTTInitialWindow","initial window size for long-RTT flows",longRTTIW);
	// cmd.AddValue("shortRTTInitialWindow","initial window size for short-RTT flows",shortRTTIW);

	double longRTTStartTime = 0;
	// double shortRTTStartTime = 0;
	cmd.AddValue("longRTTStartTime","start time of long-RTT flows in ms; 0 for random",longRTTStartTime);
	// cmd.AddValue("shortRTTStartTime","start time of short-RTT flows in ms; 0 for random",shortRTTStartTime);

	uint64_t minBurstSize = 100000000;
	double burstStartRange = 1000000;
	cmd.AddValue("minBurstSize","the approximate burst size in bytes",minBurstSize);
	cmd.AddValue("burstStartRange","the burst start range in ns",burstStartRange);
	
	/*Parse CMD*/
	cmd.Parse (argc,argv);


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
			*torStats->GetStream () << " port_" << i << "_queue_" << j <<"_throughput";
			*torStats->GetStream () << " port_" << i << "_queue_" << j <<"_sentBytes";
			*torStats->GetStream () << " port_" << i << "_queue_" << j <<"_droppedBytes";
			*torStats->GetStream () << " port_" << i << "_queue_" << j <<"_maxSize";
		}
	}
	*torStats->GetStream () << std::endl;


	// AnnC: [artemis-star-topology] Not supporting static buffer for now.
	// uint32_t staticBuffer = (double) BufferSize*statBuf/(SERVER_COUNT+SPINE_COUNT*LINK_COUNT);
	uint32_t staticBuffer = 0;
	// BufferSize = BufferSize - staticBuffer; // BufferSize is the buffer pool which is available for sharing
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
	alpha_values[0] = 8; //for ACK packets
	alpha_values[1] = longRTTAlpha; //for long-RTT flows
	// alpha_values[2] = shortRTTAlpha; //for short-RTT flows
	aFile.close();

	double RTTBytes = ((serverLeafCapacity*serverLeafLinkLatencyLongRTT+leafSinkCapacity*leafSinkLinkLatencyLongRTT)*2*GIGA*1e-6)/8;
	uint32_t RTTPackets = RTTBytes/PACKET_SIZE + 1;
	baseRTTNano = (serverLeafLinkLatencyLongRTT+leafSinkLinkLatencyLongRTT)*2*1e3;
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

	srand(randomSeed);
	NS_LOG_INFO ("Initialize random seed: " << randomSeed);

	NS_LOG_INFO ("Initialize CDF table");
	struct cdf_table* cdfTable = new cdf_table ();
	init_cdf (cdfTable);
	load_cdf (cdfTable, cdfFileName.c_str ());

	std::vector<uint64_t> bursts_list;
	uint64_t currBurstSize = 0;
	std::vector<double> add_burst_times;
	if (fsModeLongRTT == 1) {
		uint64_t flowSize = 0;
		while (currBurstSize < minBurstSize) {
			flowSize = gen_random_cdf(cdfTable);
			// AnnC: make sure all flow sizes are unique; just for the convenience of comparing FCTs
			while (flowSize == 0 || std::find(bursts_list.begin(), bursts_list.end(), flowSize)!=bursts_list.end()) { 
				flowSize = gen_random_cdf(cdfTable); 
			}
			bursts_list.push_back(flowSize);
			currBurstSize += flowSize;
		}
		uint32_t numBursts = bursts_list.size();
		numLongRTTFlows = numBursts;
		// numShortRTTFlows = numBursts;

		double addStartTimeS = 0;
		for (uint32_t i=0; i<numBursts; i++) {
			addStartTimeS = rand_range(0.0,burstStartRange)/1000000000;
			while (addStartTimeS == 0) { 
				addStartTimeS = rand_range(0.0,burstStartRange)/1000000000;
			}
			add_burst_times.push_back(addStartTimeS);
		}
	}

	// NodeContainer nodecontainers;
	// nodecontainers.Create(numNodes);
	NodeContainer longRTTnodecontainers;
	longRTTnodecontainers.Create(numLongRTTFlows);
	// NodeContainer shortRTTnodecontainers;
	// shortRTTnodecontainers.Create(numShortRTTFlows);
	NodeContainer nd;
	nd.Create (1);
	NodeContainer sinkcontainers; 
	sinkcontainers.Create(numSinks);

	InternetStackHelper stack;
	stack.InstallAll ();

	Ipv4GlobalRoutingHelper globalRoutingHelper;
	stack.SetRoutingHelper (globalRoutingHelper);


	TrafficControlHelper tc;
    uint16_t handle;
    TrafficControlHelper::ClassIdList cid;

    /*CC Configuration*/
    std::cout << "TcpProt=" << TcpProt << std::endl;
    switch (TcpProt){
    	case RENO:
			Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpNewReno::GetTypeId()));
			Config::SetDefault ("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
			Config::SetDefault("ns3::GenQueueDisc::nPrior", UintegerValue(nPrior));
			Config::SetDefault("ns3::GenQueueDisc::RoundRobin", UintegerValue(1));
			Config::SetDefault("ns3::GenQueueDisc::StrictPriority", UintegerValue(0));
			handle = tc.SetRootQueueDisc ("ns3::GenQueueDisc");
		    cid = tc.AddQueueDiscClasses (handle, nPrior , "ns3::QueueDiscClass");
		    for(uint32_t num=0;num<nPrior;num++){
				tc.AddChildQueueDisc (handle, cid[num], "ns3::FifoQueueDisc");
			}
			break;
    	case CUBIC:
        	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpCubic::GetTypeId()));
        	Config::SetDefault ("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
        	Config::SetDefault("ns3::GenQueueDisc::nPrior", UintegerValue(nPrior));
        	Config::SetDefault("ns3::GenQueueDisc::RoundRobin", UintegerValue(1));
			Config::SetDefault("ns3::GenQueueDisc::StrictPriority", UintegerValue(0));
			handle = tc.SetRootQueueDisc ("ns3::GenQueueDisc");
		    cid = tc.AddQueueDiscClasses (handle, nPrior , "ns3::QueueDiscClass");
		    for(uint32_t num=0;num<nPrior;num++){
				tc.AddChildQueueDisc (handle, cid[num], "ns3::FifoQueueDisc");
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
			handle = tc.SetRootQueueDisc ("ns3::GenQueueDisc");
		    cid = tc.AddQueueDiscClasses (handle, nPrior , "ns3::QueueDiscClass");
			for(uint32_t num=0;num<nPrior;num++){
				tc.AddChildQueueDisc (handle, cid[num], "ns3::RedQueueDisc", "MinTh", DoubleValue (RedMinTh*PACKET_SIZE), "MaxTh", DoubleValue (RedMaxTh*PACKET_SIZE));
			}
			break;
		case TIMELY:
			Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpWien::GetTypeId()));
			Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::initWienRate", DataRateValue(DataRate(leafSinkCapacity*GIGA)));
			Config::SetDefault("ns3::TcpSocketState::minWienRate", DataRateValue(DataRate("50Mbps")));
			Config::SetDefault("ns3::TcpSocketState::maxWienRate", DataRateValue(DataRate(leafSinkCapacity*GIGA)));
			Config::SetDefault("ns3::TcpSocketState::AIWien", DataRateValue(DataRate("10Mbps")));
			Config::SetDefault("ns3::TcpSocketState::HighAIWien", DataRateValue(DataRate("150Mbps")));
			Config::SetDefault("ns3::TcpSocketState::useTimely", BooleanValue(true));
			Config::SetDefault("ns3::TcpSocketState::baseRtt", TimeValue(MicroSeconds((serverLeafLinkLatencyLongRTT+leafSinkLinkLatencyLongRTT)*2 + double(PACKET_SIZE*8)/(serverLeafCapacity*1000) + double(PACKET_SIZE*8)/(leafSinkCapacity*1000))));
			// Config::SetDefault("ns3::TcpSocketState::TimelyTlow", UintegerValue(((serverLeafLinkLatencyLongRTT+leafSinkLinkLatencyLongRTT)*2*1.5)*1000)); // in nanoseconds
			// Config::SetDefault("ns3::TcpSocketState::TimelyThigh", UintegerValue(((serverLeafLinkLatencyLongRTT+leafSinkLinkLatencyLongRTT)*2*1.5)*1000)); // in nanoseconds
			// AnnC: ignore Tlow and Thigh for now
			Config::SetDefault("ns3::TcpSocketState::TimelyTlow", UintegerValue(0));
			Config::SetDefault("ns3::TcpSocketState::TimelyThigh", UintegerValue(GIGA));
			Config::SetDefault ("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
			Config::SetDefault("ns3::GenQueueDisc::nPrior", UintegerValue(nPrior));
			Config::SetDefault("ns3::GenQueueDisc::RoundRobin", UintegerValue(1));
			Config::SetDefault("ns3::GenQueueDisc::StrictPriority", UintegerValue(0));
			handle = tc.SetRootQueueDisc ("ns3::GenQueueDisc");
		    cid = tc.AddQueueDiscClasses (handle, nPrior , "ns3::QueueDiscClass");
			for(uint32_t num=0;num<nPrior;num++){
				tc.AddChildQueueDisc (handle, cid[num], "ns3::FifoQueueDisc");
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

	std::cout << "serverLeafCapacity=" << serverLeafCapacity << ", leafSinkCapacity=" << leafSinkCapacity << std::endl;

	//////////////////////////////////
	// An access link with long RTT //
	//////////////////////////////////
	PointToPointHelper accessLinkLongRTT;
	accessLinkLongRTT.SetDeviceAttribute ("DataRate", DataRateValue(DataRate(serverLeafCapacity*GIGA)));
	accessLinkLongRTT.SetChannelAttribute ("Delay", TimeValue(MicroSeconds(serverLeafLinkLatencyLongRTT)));
	// if (dropTail) {
	// 	accessLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));
	// }

	/* Server <--> Leaf */
	// std::cout << "nodecontainers.GetN()=" << nodecontainers.GetN() << std::endl;
	for (uint32_t server=0; server<longRTTnodecontainers.GetN(); server++) {
		NetDeviceContainer netDeviceContainer = accessLinkLongRTT.Install(longRTTnodecontainers.Get(server), nd.Get(0));
		QueueDiscContainer queuedisc = tc.Install(netDeviceContainer.Get(1));
		bottleneckQueueDiscs.Add(queuedisc.Get(0));
		Ptr<GenQueueDisc> genDisc = DynamicCast<GenQueueDisc> (queuedisc.Get(0));
		genDisc->SetPortId(portid++);
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
	}

	// ///////////////////////////////////
	// // An access link with short RTT //
	// ///////////////////////////////////
	// PointToPointHelper accessLink;
	// accessLink.SetDeviceAttribute ("DataRate", DataRateValue(DataRate(serverLeafCapacity*GIGA)));
	// accessLink.SetChannelAttribute ("Delay", TimeValue(MicroSeconds(serverLeafLinkLatencyShortRTT)));
	// // if (dropTail) {
	// // 	accessLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));
	// // }

	// /* Server <--> Leaf */
	// // std::cout << "nodecontainers.GetN()=" << nodecontainers.GetN() << std::endl;
	// for (uint32_t server=0; server<shortRTTnodecontainers.GetN(); server++) {
	// 	NetDeviceContainer netDeviceContainer = accessLink.Install(shortRTTnodecontainers.Get(server), nd.Get(0));
	// 	QueueDiscContainer queuedisc = tc.Install(netDeviceContainer.Get(1));
	// 	bottleneckQueueDiscs.Add(queuedisc.Get(0));
	// 	Ptr<GenQueueDisc> genDisc = DynamicCast<GenQueueDisc> (queuedisc.Get(0));
	// 	genDisc->SetPortId(portid++);
	// 	// AnnC: [artemis-star-topology] Assume to be DT.
	// 	genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	// 	genDisc->setPortBw(serverLeafCapacity);
	// 	genDisc->SetSharedMemory(sharedMemory);
	// 	genDisc->SetBufferAlgorithm(DT);
	// 	for(uint32_t n=0;n<nPrior;n++){
	// 		genDisc->alphas[n] = alpha_values[n];
	// 	}
	// 	address.NewNetwork ();
	// 	address.Assign (netDeviceContainer);
	// }

	std::cout << "done with server<->leaf links" << std::endl;


	///////////////////////////////
	// A bottleneck link with long RTT
	///////////////////////////////
	PointToPointHelper bottleneckLinkLongRTT;
	bottleneckLinkLongRTT.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (leafSinkCapacity*GIGA)));
	bottleneckLinkLongRTT.SetChannelAttribute ("Delay", TimeValue(MicroSeconds(leafSinkLinkLatencyLongRTT)));
	// if (dropTail) {
	// 	bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (std::to_string(queueDiscSize) + "p"));
	// }

	/* Leaf <--> Sink */
	Ipv4InterfaceContainer nsInterface;
	uint32_t sink=0;
	NetDeviceContainer devicesBottleneckLink = bottleneckLinkLongRTT.Install (nd.Get (0), sinkcontainers.Get (sink));
	QueueDiscContainer qdiscs = tc.Install (devicesBottleneckLink.Get(0));
	bottleneckQueueDiscs.Add(qdiscs.Get(0));
	Ptr<GenQueueDisc> genDisc = DynamicCast<GenQueueDisc> (qdiscs.Get(0));
	genDisc->SetPortId(portid++);
	genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	genDisc->setPortBw(leafSinkCapacity);
	genDisc->SetSharedMemory(sharedMemory);
	switch(algorithm){
		case DT:
			genDisc->SetBufferAlgorithm(DT);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc->alphas[n] = alpha_values[n];
			}
			break;
		case FAB:
			genDisc->SetBufferAlgorithm(FAB);
			genDisc->SetFabWindow(MicroSeconds(5000));
			genDisc->SetFabThreshold(15*PACKET_SIZE);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc->alphas[n] = alpha_values[n];
			}
			break;
		case CS:
			genDisc->SetBufferAlgorithm(CS);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc->alphas[n] = alpha_values[n];
			}
			break;
		case IB:
			genDisc->SetBufferAlgorithm(IB);
			genDisc->SetAfdWindow(MicroSeconds(50));
			genDisc->SetDppWindow(MicroSeconds(5000));
			genDisc->SetDppThreshold(RTTPackets);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc->alphas[n] = alpha_values[n];
				genDisc->SetQrefAfd(n,uint32_t(RTTBytes));
			}
			break;
		case ABM:
			genDisc->SetBufferAlgorithm(ABM);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc->alphas[n] = alpha_values[n];
			}
			break;
		default:
			std::cout << "Error in buffer management configuration. Exiting!";
			return 0;
	}

	address.NewNetwork ();
	Ipv4InterfaceContainer interfacesBottleneck = address.Assign (devicesBottleneckLink);
	nsInterface.Add (interfacesBottleneck.Get (1));


	// ///////////////////////////////
	// // A bottleneck link with short RTT
	// ///////////////////////////////
	// PointToPointHelper bottleneckLinkShortRTT;
	// bottleneckLinkShortRTT.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (leafSinkCapacity*GIGA)));
	// bottleneckLinkShortRTT.SetChannelAttribute ("Delay", TimeValue(MicroSeconds(leafSinkLinkLatencyShortRTT)));
	// // if (dropTail) {
	// // 	bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (std::to_string(queueDiscSize) + "p"));
	// // }

	// /* Leaf <--> Sink */
	// sink=1;
	// NetDeviceContainer devicesBottleneckLinkShortRTT = bottleneckLinkShortRTT.Install (nd.Get (0), sinkcontainers.Get (sink));
	// QueueDiscContainer qdiscsShortRTT = tc.Install (devicesBottleneckLinkShortRTT.Get(0));
	// bottleneckQueueDiscs.Add(qdiscsShortRTT.Get(0));
	// Ptr<GenQueueDisc> genDiscShortRTT = DynamicCast<GenQueueDisc> (qdiscsShortRTT.Get(0));
	// genDiscShortRTT->SetPortId(portid++);
	// genDiscShortRTT->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	// genDiscShortRTT->setPortBw(leafSinkCapacity);
	// genDiscShortRTT->SetSharedMemory(sharedMemory);
	// switch(algorithm){
	// 	case DT:
	// 		genDiscShortRTT->SetBufferAlgorithm(DT);
	// 		for(uint32_t n=0;n<nPrior;n++){
	// 			genDiscShortRTT->alphas[n] = alpha_values[n];
	// 		}
	// 		break;
	// 	case FAB:
	// 		genDiscShortRTT->SetBufferAlgorithm(FAB);
	// 		genDiscShortRTT->SetFabWindow(MicroSeconds(5000));
	// 		genDiscShortRTT->SetFabThreshold(15*PACKET_SIZE);
	// 		for(uint32_t n=0;n<nPrior;n++){
	// 			genDiscShortRTT->alphas[n] = alpha_values[n];
	// 		}
	// 		break;
	// 	case CS:
	// 		genDiscShortRTT->SetBufferAlgorithm(CS);
	// 		for(uint32_t n=0;n<nPrior;n++){
	// 			genDiscShortRTT->alphas[n] = alpha_values[n];
	// 		}
	// 		break;
	// 	case IB:
	// 		genDiscShortRTT->SetBufferAlgorithm(IB);
	// 		genDiscShortRTT->SetAfdWindow(MicroSeconds(50));
	// 		genDiscShortRTT->SetDppWindow(MicroSeconds(5000));
	// 		genDiscShortRTT->SetDppThreshold(RTTPackets);
	// 		for(uint32_t n=0;n<nPrior;n++){
	// 			genDiscShortRTT->alphas[n] = alpha_values[n];
	// 			genDiscShortRTT->SetQrefAfd(n,uint32_t(RTTBytes));
	// 		}
	// 		break;
	// 	case ABM:
	// 		genDiscShortRTT->SetBufferAlgorithm(ABM);
	// 		for(uint32_t n=0;n<nPrior;n++){
	// 			genDiscShortRTT->alphas[n] = alpha_values[n];
	// 		}
	// 		break;
	// 	default:
	// 		std::cout << "Error in buffer management configuration. Exiting!";
	// 		return 0;
	// }

	// address.NewNetwork ();
	// Ipv4InterfaceContainer interfacesBottleneckShortRTT = address.Assign (devicesBottleneckLinkShortRTT);
	// nsInterface.Add (interfacesBottleneckShortRTT.Get (1));


	std::cout << "start installing applications" << std::endl;

	// Install applications
	NS_LOG_INFO("Install applications");
	std::ofstream paramfile;
  	paramfile.open (paramOutFile);

	uint32_t numLongRTT = numLongRTTFlows;
	// uint32_t numShortRTT = numShortRTTFlows;
	// sink0 receives long-RTT flows, sink1 receives short-RTT flows
	// uint32_t nodetosink[numNodes] = {0};
	// for (uint32_t i=0; i<numLongRTT; i++) {
	// 	nodetosink[i] = 0;
	// }
	// for (uint32_t i=numLongRTT; i<numLongRTT+numShortRTT; i++) {
	// 	nodetosink[i] = 1;
	// }
	uint32_t portnumber = 9;
	uint32_t flowcount = 0;
	// Install long-RTT flows
	// double startTimeLongRTT = 0;
	// if (longRTTStartTime == 0) {
	// 	startTimeLongRTT = START_TIME + 0.1 + poission_gen_interval(0.2);
	// } else {
	// 	startTimeLongRTT = longRTTStartTime/1000;
	// }
	paramfile << "burst size = " << currBurstSize << "\n";
	paramfile << "number of long-RTT flows = " << numLongRTT << "\n";
	for (uint32_t node=0; node<numLongRTT; node++) {
		uint64_t flowSize = 0;
		double startTimeLongRTT = 0;
		if (fsModeLongRTT == 0) {
			flowSize = 1e9;
			startTimeLongRTT = rand_range(START_TIME,FLOW_LAUNCH_END_TIME);
		} else if (fsModeLongRTT == 1) {
			// flowSize = gen_random_cdf(cdfTable);
			// while (flowSize == 0) { 
			// 	flowSize = gen_random_cdf(cdfTable); 
			// }
			flowSize = bursts_list.at(node);
			// if (longRTTStartTime == 0) {
			// 	startTimeLongRTT = START_TIME + 0.1 + poission_gen_interval(0.2);
			// } else {
			// 	startTimeLongRTT = longRTTStartTime/1000;
			// }
			startTimeLongRTT = longRTTStartTime/1000 + add_burst_times.at(node);
		} else if (fsModeLongRTT == 2) {
			// AnnC: for random size alignment
			// flowSize = gen_random_cdf(cdfTable);
			// while (flowSize == 0) { 
			// 	flowSize = gen_random_cdf(cdfTable); 
			// }
			flowSize = 1;
			if (longRTTStartTime == 0) {
				startTimeLongRTT = START_TIME + 0.1 + poission_gen_interval(0.2);
			} else {
				startTimeLongRTT = longRTTStartTime/1000;
			}
		}
		// double startTime = START_TIME;
		//double startTime = START_TIME + node*0.1;
		// ACK packets are prioritized
		//uint64_t flowPriority = rand_range((u_int32_t)1,nPrior-1);
		uint64_t flowPriority = 1;
		
		// uint32_t sink = nodetosink[node];
		uint32_t sink = 0;
		InetSocketAddress ad(nsInterface.GetAddress(sink), portnumber);
		Address sinkAddress(ad);

		paramfile << "Sending from node " << node << " to sink " << sink << ": ";
		paramfile << "startTime=" << startTimeLongRTT << ", flowSize=" << flowSize << ", flowPriority=" << flowPriority << "\n";

		Ptr<BulkSendApplication> bulksend = CreateObject<BulkSendApplication>();
		bulksend->SetAttribute("Protocol",TypeIdValue(TcpSocketFactory::GetTypeId()));
		bulksend->SetAttribute("Remote",AddressValue(sinkAddress)); 
		bulksend->SetAttribute ("SendSize", UintegerValue (flowSize));
        bulksend->SetAttribute ("MaxBytes", UintegerValue(flowSize));
		bulksend->SetAttribute("FlowId", UintegerValue(flowcount++));
		bulksend->SetAttribute("InitialCwnd", UintegerValue (longRTTIW));
		bulksend->SetAttribute("priorityCustom",UintegerValue(flowPriority));
		bulksend->SetAttribute("priority",UintegerValue(flowPriority));
		bulksend->SetStartTime (Seconds(startTimeLongRTT));
		// std::cout << "node=" << node << ", start_time=" << Seconds(startTime) << std::endl;
        bulksend->SetStopTime (Seconds (END_TIME));
		longRTTnodecontainers.Get(node)->AddApplication(bulksend);

		PacketSinkHelper packetSink("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), portnumber));
		ApplicationContainer sinkApp = packetSink.Install(sinkcontainers.Get(sink));
		sinkApp.Get(0)->SetAttribute("Protocol",TypeIdValue(TcpSocketFactory::GetTypeId()));
		sinkApp.Get(0)->SetAttribute("TotalQueryBytes",UintegerValue(flowSize));
		sinkApp.Get(0)->SetAttribute("flowId", UintegerValue(flowcount++));
		sinkApp.Get(0)->SetAttribute("senderPriority",UintegerValue(flowPriority));
		// ACK packets are prioritized
		sinkApp.Get(0)->SetAttribute("priorityCustom",UintegerValue(0));
		sinkApp.Get(0)->SetAttribute("priority",UintegerValue(0));
		sinkApp.Start(Seconds(startTimeLongRTT));
		sinkApp.Stop(Seconds(END_TIME));
		sinkApp.Get(0)->TraceConnectWithoutContext("FlowFinish", MakeBoundCallback(&TraceMsgFinish, fctOutput));
		
		portnumber++;
	}

	// Install short-RTT flows
	// double startTime = 0;
	// if (shortRTTStartTime == 0) {
	// 	startTime = START_TIME + 0.1 + poission_gen_interval(0.2);
	// } else {
	// 	startTime = shortRTTStartTime/1000;
	// }
	// paramfile << "number of short-RTT flows = " << numShortRTT << "\n";
	// for (uint32_t node=0; node<numShortRTT; node++) {
	// 	uint64_t flowSize = 0;
	// 	double startTime = 0;
	// 	if (fsModeShortRTT == 0) {
	// 		flowSize = 1e9;
	// 		startTime = rand_range(START_TIME,FLOW_LAUNCH_END_TIME);
	// 	} else if (fsModeShortRTT == 1) {
	// 		// flowSize = gen_random_cdf(cdfTable);
	// 		// while (flowSize == 0) { 
	// 		// 	flowSize = gen_random_cdf(cdfTable); 
	// 		// }
	// 		flowSize = bursts_list.at(node);
	// 		// if (shortRTTStartTime == 0) {
	// 		// 	startTime = START_TIME + 0.1 + poission_gen_interval(0.2);
	// 		// } else {
	// 		// 	startTime = shortRTTStartTime/1000;
	// 		// }
	// 		startTime = shortRTTStartTime/1000 + add_burst_times.at(node);
	// 	} else if (fsModeShortRTT == 2) {
	// 		flowSize = 1;
	// 		if (shortRTTStartTime == 0) {
	// 			startTime = START_TIME + 0.1 + poission_gen_interval(0.2);
	// 		} else {
	// 			startTime = shortRTTStartTime/1000;
	// 		}
	// 	}
	// 	// AnnC: manually increase bursty flow size
	// 	//if (flowSize < 1e8) flowSize=flowSize*10;
	// 	//double startTime = START_TIME + 1 + poission_gen_interval(0.2);
	// 	//while (startTime >= FLOW_LAUNCH_END_TIME || startTime <= START_TIME) {
	// 	//	startTime = START_TIME + 1 + poission_gen_interval(0.2);
	// 	//}
	// 	// ACK packets are prioritized
	// 	//uint64_t flowPriority = rand_range((u_int32_t)1,nPrior-1);
	// 	uint64_t flowPriority = 2;

	// 	// uint32_t sink = nodetosink[numLongRTT+node];
	// 	uint32_t sink = 1;
	// 	InetSocketAddress ad(nsInterface.GetAddress(sink), portnumber);
	// 	Address sinkAddress(ad);

	// 	paramfile << "Sending from node " << node << " to sink " << sink << ": ";
	// 	paramfile << "startTime=" << startTime << ", flowSize=" << flowSize << ", flowPriority=" << flowPriority << "\n";

	// 	Ptr<BulkSendApplication> bulksend = CreateObject<BulkSendApplication>();
	// 	bulksend->SetAttribute("Protocol",TypeIdValue(TcpSocketFactory::GetTypeId()));
	// 	bulksend->SetAttribute("Remote",AddressValue(sinkAddress)); 
	// 	bulksend->SetAttribute ("SendSize", UintegerValue (flowSize));
    //     bulksend->SetAttribute ("MaxBytes", UintegerValue(flowSize));
	// 	bulksend->SetAttribute("FlowId", UintegerValue(flowcount++));
	// 	bulksend->SetAttribute("InitialCwnd", UintegerValue (shortRTTIW));
	// 	bulksend->SetAttribute("priorityCustom",UintegerValue(flowPriority));
	// 	bulksend->SetAttribute("priority",UintegerValue(flowPriority));
	// 	bulksend->SetStartTime (Seconds(startTime));
	// 	// std::cout << "node=" << node << ", start_time=" << Seconds(startTime) << std::endl;
    //     bulksend->SetStopTime (Seconds (END_TIME));
	// 	shortRTTnodecontainers.Get(node)->AddApplication(bulksend);

	// 	PacketSinkHelper packetSink("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), portnumber));
	// 	ApplicationContainer sinkApp = packetSink.Install(sinkcontainers.Get(sink));
	// 	sinkApp.Get(0)->SetAttribute("Protocol",TypeIdValue(TcpSocketFactory::GetTypeId()));
	// 	sinkApp.Get(0)->SetAttribute("TotalQueryBytes",UintegerValue(flowSize));
	// 	sinkApp.Get(0)->SetAttribute("flowId", UintegerValue(flowcount++));
	// 	sinkApp.Get(0)->SetAttribute("senderPriority",UintegerValue(flowPriority));
	// 	// ACK packets are prioritized
	// 	sinkApp.Get(0)->SetAttribute("priorityCustom",UintegerValue(0));
	// 	sinkApp.Get(0)->SetAttribute("priority",UintegerValue(0));
	// 	sinkApp.Start(Seconds(startTime));
	// 	sinkApp.Stop(Seconds(END_TIME));
	// 	sinkApp.Get(0)->TraceConnectWithoutContext("FlowFinish", MakeBoundCallback(&TraceMsgFinish, fctOutput));
		
	// 	portnumber++;
	// }

	paramfile.close();


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

