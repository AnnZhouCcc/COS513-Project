
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

#define PORT_END 65530

extern "C"
{
	#include "cdf.h"
}


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ABM_EVALUATION");

const uint32_t nPrior = 1;

uint32_t PORT_START[512]={4444};

double alpha_values[8]={1};

Ptr<OutputStreamWrapper> fctOutput;
AsciiTraceHelper asciiTraceHelper;

Ptr<OutputStreamWrapper> torStats;
AsciiTraceHelper torTraceHelper;

// Ptr<SharedMemoryBuffer> sharedMemoryLeaf[10];
Ptr<SharedMemoryBuffer> sharedMemory;

// QueueDiscContainer northQueues[10];
// QueueDiscContainer ToRQueueDiscs[10];//MA
QueueDiscContainer bottleneckQueueDiscs;

double poission_gen_interval(double avg_rate)
{
    if (avg_rate > 0)
       return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
    else
       return 0;
}

// template<typename T>
// T rand_range (T min, T max)
// {
//     return min + ((double)max - min) * rand () / RAND_MAX;
// }

double baseRTTNano;
double nicBw;
void TraceMsgFinish (Ptr<OutputStreamWrapper> stream, double size, double start, bool incast,uint32_t prior )
{
  double fct, standalone_fct,slowdown;
	fct = Simulator::Now().GetNanoSeconds()- start;
	standalone_fct = baseRTTNano + size*8.0/ nicBw;
	slowdown = fct/standalone_fct;

	*stream->GetStream ()
	<< Simulator::Now().GetNanoSeconds()
  	<< " " << size
  	<< " " << fct
  	<< " " << standalone_fct
  	<< " " << slowdown
  	<< " " << baseRTTNano/1e3
  	<< " " << (start/1e3- Seconds(10).GetMicroSeconds())
	<< " " << prior
	<< " " << incast
	<< std::endl;
}


void StarTopologyInvokeToRStats(Ptr<OutputStreamWrapper> stream, uint32_t BufferSize, int LEAF_COUNT, double nanodelay){

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

	// for(int leafId = 0; leafId < LEAF_COUNT; leafId += 1) {
	// 	Ptr<SharedMemoryBuffer> sm = sharedMemoryLeaf[leafId];
	// 	QueueDiscContainer queues = ToRQueueDiscs[leafId];

	// 	*stream->GetStream()
	// 	<< currentNanoSeconds
	// 	<< " " << leafId
	// 	<< " " << double(BufferSize)/1e6
	// 	<< " " << 100 * double(sm->GetOccupiedBuffer())/BufferSize;

	// 	 for (uint32_t port = 0; port<queues.GetN(); port++){
	// 	 	Ptr<GenQueueDisc> genDisc = DynamicCast<GenQueueDisc>(queues.Get(port));
	// 	 	double remaining = genDisc->GetRemainingBuffer();
	// 	 	for (uint32_t priority = 0; priority<nPrior; priority++)  {
	// 	 		uint32_t qSize = genDisc->GetQueueDiscClass(priority)->GetQueueDisc()->GetNBytes();
	// 	 		auto [th, sentBytes] = genDisc->GetThroughputQueue(priority, nanodelay);  // only with c++17
	// 	 		uint64_t droppedBytes = genDisc->GetDroppedBytes(priority);
	// 	 		uint64_t maxSize = genDisc->GetAlpha(priority)*remaining;
	// 	 		*stream->GetStream() << " " << qSize << " " << th << " " << sentBytes << " " << droppedBytes << " " << maxSize;
	// 	 	}
	// 	 }
	// 	*stream->GetStream() << std::endl;
	// }

	Simulator::Schedule(NanoSeconds(nanodelay), StarTopologyInvokeToRStats, stream, BufferSize, LEAF_COUNT, nanodelay);
}


// void 
// InvokeToRStats(Ptr<OutputStreamWrapper> stream, uint32_t BufferSize, int LEAF_COUNT, double nanodelay){

// 	int64_t currentNanoSeconds = Simulator::Now().GetNanoSeconds();

// 	for(int leafId = 0; leafId < LEAF_COUNT; leafId += 1) {
// 		Ptr<SharedMemoryBuffer> sm = sharedMemoryLeaf[leafId];
// 		QueueDiscContainer queues = ToRQueueDiscs[leafId];

// 		*stream->GetStream()
// 		<< currentNanoSeconds
// 		<< " " << leafId
// 		<< " " << double(BufferSize)/1e6
// 		<< " " << 100 * double(sm->GetOccupiedBuffer())/BufferSize;

// /*
// 		 for (uint32_t port = 0; port<queues.GetN(); port++){
// 		 	Ptr<GenQueueDisc> genDisc = DynamicCast<GenQueueDisc>(queues.Get(port));
// 		 	double remaining = genDisc->GetRemainingBuffer();
// 		 	for (uint32_t priority = 0; priority<nPrior; priority++)  {
// 		 		uint32_t qSize = genDisc->GetQueueDiscClass(priority)->GetQueueDisc()->GetNBytes();
// 		 		auto [th, sentBytes] = genDisc->GetThroughputQueue(priority, nanodelay);  // only with c++17
// 		 		uint64_t droppedBytes = genDisc->GetDroppedBytes(priority);
// 		 		uint64_t maxSize = genDisc->GetAlpha(priority)*remaining;
// 		 		*stream->GetStream() << " " << qSize << " " << th << " " << sentBytes << " " << droppedBytes << " " << maxSize;
// 		 	}
// 		 }
// */
// 		*stream->GetStream() << std::endl;
// 	}

// 	Simulator::Schedule(NanoSeconds(nanodelay), InvokeToRStats, stream, BufferSize, LEAF_COUNT, nanodelay);
// }


// int tar=0;
// int get_target_leaf(int leafCount){
// tar+=1;
//     if(tar==leafCount){
//         tar=0;
//         return tar;
//     }
//     return tar;
// }

// void install_applications_incast (int incastLeaf, NodeContainer* servers, double requestRate,uint32_t requestSize, struct cdf_table *cdfTable,
//         long &flowCount, int SERVER_COUNT, int LEAF_COUNT, double START_TIME, double END_TIME, double FLOW_LAUNCH_END_TIME, int numPrior)
// {
// 	int fan = SERVER_COUNT;
//     uint64_t flowSize = double(requestSize) / double(fan);

//     uint32_t prior = rand_range(1,numPrior-1);

//     for (int incastServer = 0; incastServer < SERVER_COUNT; incastServer++)
//     {
//     	double startTime = START_TIME + poission_gen_interval (requestRate);
//         while (startTime < FLOW_LAUNCH_END_TIME && startTime > START_TIME)
//         {
//             // Permutation demand matrix
//         	int txLeaf=incastLeaf+1;
//             if (txLeaf==LEAF_COUNT){
//             	txLeaf = 0;
//             }
//             // int txLeaf=incastLeaf;
//             // while (txLeaf==incastLeaf){
//             //     txLeaf = get_target_leaf(LEAF_COUNT);
//             // }

//             for (uint32_t txServer = 0; txServer<fan; txServer++){

//             	uint16_t port = PORT_START[ incastLeaf*SERVER_COUNT+ incastServer]++;
// 	            if (port>PORT_END){
// 	                port=4444;
// 	                PORT_START[incastLeaf*SERVER_COUNT+ incastServer]=4444;
// 	            }
//             	Time startApp = (NanoSeconds (150)+MilliSeconds(rand_range(50,1000)));
//             	Ptr<Node> rxNode = servers[incastLeaf].Get (incastServer);
//             	Ptr<Ipv4> ipv4 = rxNode->GetObject<Ipv4> ();
//             	Ipv4InterfaceAddress rxInterface = ipv4->GetAddress (1,0);
// 	            Ipv4Address rxAddress = rxInterface.GetLocal ();

// 	            InetSocketAddress ad (rxAddress, port);
// 	            Address sinkAddress(ad);
// 	            Ptr<BulkSendApplication> bulksend = CreateObject<BulkSendApplication>();
// 	            bulksend->SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
// 	            bulksend->SetAttribute ("SendSize", UintegerValue (flowSize));
// 	            bulksend->SetAttribute ("MaxBytes", UintegerValue(flowSize));
// 	            bulksend->SetAttribute("FlowId", UintegerValue(flowCount++));
// 	            bulksend->SetAttribute("priorityCustom",UintegerValue(prior));
// 	            bulksend->SetAttribute("Remote", AddressValue(sinkAddress));
// 	            bulksend->SetAttribute("InitialCwnd", UintegerValue (flowSize/PACKET_SIZE+1));
// 				bulksend->SetAttribute("priority",UintegerValue(prior));
// 	            bulksend->SetAttribute("sendAt", TimeValue(Seconds (startTime)));
// 	            bulksend->SetStartTime (startApp);
// 	            bulksend->SetStopTime (Seconds (END_TIME));
// 	            servers[txLeaf].Get (txServer)->AddApplication(bulksend);

// 	            PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
//                 ApplicationContainer sinkApp = sink.Install (servers[incastLeaf].Get(incastServer));
//                 sinkApp.Get(0)->SetAttribute("TotalQueryBytes",UintegerValue(flowSize));
//                 sinkApp.Get(0)->SetAttribute("recvAt",TimeValue(Seconds(startTime)));
//                 sinkApp.Get(0)->SetAttribute("priority",UintegerValue(0)); // ack packets are prioritized
//                 sinkApp.Get(0)->SetAttribute("priorityCustom",UintegerValue(0)); // ack packets are prioritized
//                 sinkApp.Get(0)->SetAttribute("senderPriority",UintegerValue(prior));
//                 sinkApp.Get(0)->SetAttribute("flowId",UintegerValue(flowCount));
//                 flowCount+=1;
//                 sinkApp.Start (startApp);
//                 sinkApp.Stop (Seconds (END_TIME));
//                 sinkApp.Get(0)->TraceConnectWithoutContext("FlowFinish", MakeBoundCallback(&TraceMsgFinish, fctOutput));
//             }
//             startTime += poission_gen_interval (requestRate);
//         }
//     }
// }

// void install_applications (int txLeaf, NodeContainer* servers, double requestRate, struct cdf_table *cdfTable,
//         long &flowCount, int SERVER_COUNT, int LEAF_COUNT, double START_TIME, double END_TIME, double FLOW_LAUNCH_END_TIME, int numPrior)
// {
//     uint64_t flowSize;

//     uint32_t prior = rand_range(1,numPrior-1);

//     for (int txServer = 0; txServer < SERVER_COUNT; txServer++)
//     {
//     	double startTime = START_TIME + poission_gen_interval (requestRate);
//         while (startTime < FLOW_LAUNCH_END_TIME && startTime > START_TIME)
//         {
//         	// Permutation demand matrix
//         	int rxLeaf=txLeaf+1;
//             if (rxLeaf==LEAF_COUNT){
//             	rxLeaf = 0;
//             }
//             // int rxLeaf=txLeaf;
//             // while (txLeaf==rxLeaf){
//             //     rxLeaf = get_target_leaf(LEAF_COUNT);
//             // }

//             uint32_t rxServer = rand_range(0,SERVER_COUNT);

//         	uint16_t port = PORT_START[rxLeaf*SERVER_COUNT + rxServer]++;
//             if (port>PORT_END){
//                 port=4444;
//                 PORT_START[rxLeaf*SERVER_COUNT + rxServer]=4444;
//             }

//             uint64_t flowSize = gen_random_cdf (cdfTable);
//             while(flowSize==0){
//                 flowSize=gen_random_cdf (cdfTable);
//             }

//         	Ptr<Node> rxNode = servers[rxLeaf].Get (rxServer);
//         	Ptr<Ipv4> ipv4 = rxNode->GetObject<Ipv4> ();
//         	Ipv4InterfaceAddress rxInterface = ipv4->GetAddress (1,0);
//             Ipv4Address rxAddress = rxInterface.GetLocal ();

//             InetSocketAddress ad (rxAddress, port);
//             Address sinkAddress(ad);
//             Ptr<BulkSendApplication> bulksend = CreateObject<BulkSendApplication>();
//             bulksend->SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
//             bulksend->SetAttribute ("SendSize", UintegerValue (flowSize));
//             bulksend->SetAttribute ("MaxBytes", UintegerValue(flowSize));
//             bulksend->SetAttribute("FlowId", UintegerValue(flowCount++));
//             bulksend->SetAttribute("priorityCustom",UintegerValue(prior));
//             bulksend->SetAttribute("Remote", AddressValue(sinkAddress));
//             bulksend->SetAttribute("InitialCwnd", UintegerValue (4));
// 			bulksend->SetAttribute("priority",UintegerValue(prior));
//             bulksend->SetStartTime (Seconds(startTime));
//             bulksend->SetStopTime (Seconds (END_TIME));
//             servers[txLeaf].Get (txServer)->AddApplication(bulksend);

//             PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
//             ApplicationContainer sinkApp = sink.Install (servers[rxLeaf].Get(rxServer));
//             sinkApp.Get(0)->SetAttribute("TotalQueryBytes",UintegerValue(flowSize));
//             sinkApp.Get(0)->SetAttribute("priority",UintegerValue(0)); // ack packets are prioritized
//             sinkApp.Get(0)->SetAttribute("priorityCustom",UintegerValue(0)); // ack packets are prioritized
//             sinkApp.Get(0)->SetAttribute("flowId",UintegerValue(flowCount));
//             sinkApp.Get(0)->SetAttribute("senderPriority",UintegerValue(prior));
//             flowCount+=1;
//             sinkApp.Start (Seconds(startTime));
//             sinkApp.Stop (Seconds (END_TIME));
//             sinkApp.Get(0)->TraceConnectWithoutContext("FlowFinish", MakeBoundCallback(&TraceMsgFinish, fctOutput));
//             startTime += poission_gen_interval (requestRate);
//         }
//     }
//     // std::cout << "Finished installation of applications from leaf-"<< fromLeafId << std::endl;
// }



// void install_applications_simple_noincast_bursty (int txLeaf, NodeContainer* servers, double requestRate, struct cdf_table *cdfTable,
//         long &flowCount, int SERVER_COUNT, int LEAF_COUNT, double START_TIME, double END_TIME, double FLOW_LAUNCH_END_TIME, int numPrior)
// {
//     std::cout << "install applications simple-noincast-bursty" << std::endl;
//     std::cout << "requestRate=" << requestRate << std::endl;

//     uint64_t flowSize;

//     uint32_t prior = rand_range(1,numPrior-1);

//     for (int txServer = 0; txServer < 2; txServer++)
//     {
//         std::cout << "txLeaf=" << txLeaf << ", txServer=" << txServer << std::endl;
//     	double startTime = START_TIME + poission_gen_interval (requestRate);
//         while (startTime < FLOW_LAUNCH_END_TIME && startTime > START_TIME)
//         {
// 		std::cout << "startTime=" << startTime << std::endl;
//         	// Permutation demand matrix
//         	int rxLeaf=txLeaf+1;
//             if (rxLeaf==LEAF_COUNT){
//             	rxLeaf = 0;
//             }
//             // int rxLeaf=txLeaf;
//             // while (txLeaf==rxLeaf){
//             //     rxLeaf = get_target_leaf(LEAF_COUNT);
//             // }

//             //uint32_t rxServer = rand_range(0,SERVER_COUNT);
//             uint32_t rxServer = 0;
//             std::cout << "rxLeaf=" << rxLeaf << ", rxServer=" << rxServer << std::endl;

//         	uint16_t port = PORT_START[rxLeaf*SERVER_COUNT + rxServer]++;
//             if (port>PORT_END){
//                 port=4444;
//                 PORT_START[rxLeaf*SERVER_COUNT + rxServer]=4444;
//             }
//             //std::cout << "port=" << port << std::endl;

//             uint64_t flowSize = gen_random_cdf (cdfTable);
//             while(flowSize==0){
//                 flowSize=gen_random_cdf (cdfTable);
//             }

//         	Ptr<Node> rxNode = servers[rxLeaf].Get (rxServer);
//         	Ptr<Ipv4> ipv4 = rxNode->GetObject<Ipv4> ();
//         	Ipv4InterfaceAddress rxInterface = ipv4->GetAddress (1,0);
//             Ipv4Address rxAddress = rxInterface.GetLocal ();

//             InetSocketAddress ad (rxAddress, port);
//             Address sinkAddress(ad);
//             Ptr<BulkSendApplication> bulksend = CreateObject<BulkSendApplication>();
//             bulksend->SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
//             bulksend->SetAttribute ("SendSize", UintegerValue (flowSize));
//             bulksend->SetAttribute ("MaxBytes", UintegerValue(flowSize));
//             bulksend->SetAttribute("FlowId", UintegerValue(flowCount++));
//             bulksend->SetAttribute("priorityCustom",UintegerValue(prior));
//             bulksend->SetAttribute("Remote", AddressValue(sinkAddress));
//             bulksend->SetAttribute("InitialCwnd", UintegerValue (4));
// 			bulksend->SetAttribute("priority",UintegerValue(prior));
//             bulksend->SetStartTime (Seconds(startTime));
//             bulksend->SetStopTime (Seconds (END_TIME));
//             servers[txLeaf].Get (txServer)->AddApplication(bulksend);

//             PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
//             ApplicationContainer sinkApp = sink.Install (servers[rxLeaf].Get(rxServer));
//             sinkApp.Get(0)->SetAttribute("TotalQueryBytes",UintegerValue(flowSize));
//             sinkApp.Get(0)->SetAttribute("priority",UintegerValue(0)); // ack packets are prioritized
//             sinkApp.Get(0)->SetAttribute("priorityCustom",UintegerValue(0)); // ack packets are prioritized
//             sinkApp.Get(0)->SetAttribute("flowId",UintegerValue(flowCount));
//             sinkApp.Get(0)->SetAttribute("senderPriority",UintegerValue(prior));
//             flowCount+=1;
//             sinkApp.Start (Seconds(startTime));
//             sinkApp.Stop (Seconds (END_TIME));
//             sinkApp.Get(0)->TraceConnectWithoutContext("FlowFinish", MakeBoundCallback(&TraceMsgFinish, fctOutput));
//             startTime += poission_gen_interval (requestRate);
//         }
//     }
//     // std::cout << "Finished installation of applications from leaf-"<< fromLeafId << std::endl;
// }



// void install_applications_simple_noincast_continuous (int txLeaf, NodeContainer* servers, double requestRate, struct cdf_table *cdfTable,
//         long &flowCount, int SERVER_COUNT, int LEAF_COUNT, double START_TIME, double END_TIME, double FLOW_LAUNCH_END_TIME, int numPrior)
// {
//     std::cout << "install applications simple-noincast-continuous" << std::endl;

//     uint64_t flowSize = 1e9;

//     uint32_t prior = rand_range(1,numPrior-1);

//     for (int txServer = 0; txServer < 2; txServer++)
//     {
//         std::cout << "txLeaf=" << txLeaf << ", txServer=" << txServer << std::endl;
//     	double startTime = START_TIME + poission_gen_interval (requestRate);
//         // while (startTime < FLOW_LAUNCH_END_TIME && startTime > START_TIME)
//         // {
//         	// Permutation demand matrix
//         	int rxLeaf=txLeaf+1;
//             if (rxLeaf==LEAF_COUNT){
//             	rxLeaf = 0;
//             }
//             // int rxLeaf=txLeaf;
//             // while (txLeaf==rxLeaf){
//             //     rxLeaf = get_target_leaf(LEAF_COUNT);
//             // }

//             //uint32_t rxServer = rand_range(0,SERVER_COUNT);
//             uint32_t rxServer = 0;
//             std::cout << "rxLeaf=" << rxLeaf << ", rxServer=" << rxServer << std::endl;

//         	uint16_t port = PORT_START[rxLeaf*SERVER_COUNT + rxServer]++;
//             if (port>PORT_END){
//                 port=4444;
//                 PORT_START[rxLeaf*SERVER_COUNT + rxServer]=4444;
//             }
//             //std::cout << "port=" << port << std::endl;

//             // uint64_t flowSize = gen_random_cdf (cdfTable);
//             // while(flowSize==0){
//             //     flowSize=gen_random_cdf (cdfTable);
//             // }

//         	Ptr<Node> rxNode = servers[rxLeaf].Get (rxServer);
//         	Ptr<Ipv4> ipv4 = rxNode->GetObject<Ipv4> ();
//         	Ipv4InterfaceAddress rxInterface = ipv4->GetAddress (1,0);
//             Ipv4Address rxAddress = rxInterface.GetLocal ();

//             InetSocketAddress ad (rxAddress, port);
//             Address sinkAddress(ad);
//             Ptr<BulkSendApplication> bulksend = CreateObject<BulkSendApplication>();
//             bulksend->SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
//             bulksend->SetAttribute ("SendSize", UintegerValue (flowSize));
//             bulksend->SetAttribute ("MaxBytes", UintegerValue(flowSize));
//             bulksend->SetAttribute("FlowId", UintegerValue(flowCount++));
//             bulksend->SetAttribute("priorityCustom",UintegerValue(prior));
//             bulksend->SetAttribute("Remote", AddressValue(sinkAddress));
//             bulksend->SetAttribute("InitialCwnd", UintegerValue (4));
// 			bulksend->SetAttribute("priority",UintegerValue(prior));
//             bulksend->SetStartTime (Seconds(startTime));
//             bulksend->SetStopTime (Seconds (END_TIME));
//             servers[txLeaf].Get (txServer)->AddApplication(bulksend);

//             PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
//             ApplicationContainer sinkApp = sink.Install (servers[rxLeaf].Get(rxServer));
//             sinkApp.Get(0)->SetAttribute("TotalQueryBytes",UintegerValue(flowSize));
//             sinkApp.Get(0)->SetAttribute("priority",UintegerValue(0)); // ack packets are prioritized
//             sinkApp.Get(0)->SetAttribute("priorityCustom",UintegerValue(0)); // ack packets are prioritized
//             sinkApp.Get(0)->SetAttribute("flowId",UintegerValue(flowCount));
//             sinkApp.Get(0)->SetAttribute("senderPriority",UintegerValue(prior));
//             flowCount+=1;
//             sinkApp.Start (Seconds(startTime));
//             sinkApp.Stop (Seconds (END_TIME));
//             sinkApp.Get(0)->TraceConnectWithoutContext("FlowFinish", MakeBoundCallback(&TraceMsgFinish, fctOutput));
//             // startTime += poission_gen_interval (requestRate);
//         // }
//     }
//     // std::cout << "Finished installation of applications from leaf-"<< fromLeafId << std::endl;
// }



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

	double load = 0.6;
	cmd.AddValue ("load", "Load of the network, 0.0 - 1.0", load);

	uint32_t SERVER_COUNT = 32;
	uint32_t SPINE_COUNT = 2;
	uint32_t LEAF_COUNT = 2;
	uint32_t LINK_COUNT = 4;
	uint64_t spineLeafCapacity = 1; // Maria Gbps
	uint64_t leafServerCapacity = 1; //Maria Gbps
	double linkLatency = 10;
	cmd.AddValue ("serverCount", "The Server count", SERVER_COUNT);
	cmd.AddValue ("spineCount", "The Spine count", SPINE_COUNT);
	cmd.AddValue ("leafCount", "The Leaf count", LEAF_COUNT);
	cmd.AddValue ("linkCount", "The Link count", LINK_COUNT);
	cmd.AddValue ("spineLeafCapacity", "Spine <-> Leaf capacity in Gbps", spineLeafCapacity);
	cmd.AddValue ("leafServerCapacity", "Leaf <-> Server capacity in Gbps", leafServerCapacity);
	cmd.AddValue ("linkLatency", "linkLatency in microseconds", linkLatency);

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

	uint32_t requestSize = 0.2*BufferSize;
	double queryRequestRate = 0; // at each server (per second)
	cmd.AddValue ("request", "Query Size in Bytes", requestSize);
	cmd.AddValue("queryRequestRate","Query request rate (poisson arrivals)",queryRequestRate);

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

	double alphaUpdateInterval=1;
	cmd.AddValue("alphaUpdateInterval","(Number of Rtts) update interval for alpha values in ABM",alphaUpdateInterval);


	std::string fctOutFile="./fcts.txt";
	cmd.AddValue ("fctOutFile", "File path for FCTs", fctOutFile);

	std::string torOutFile="./tor.txt";
	cmd.AddValue ("torOutFile", "File path for ToR statistic", torOutFile);

	uint32_t rto = 10*1000; // in MicroSeconds, 5 milliseconds.
	cmd.AddValue ("rto", "min Retransmission timeout value in MicroSeconds", rto);

	/*Parse CMD*/
	cmd.Parse (argc,argv);

	fctOutput = asciiTraceHelper.CreateFileStream (fctOutFile);

	*fctOutput->GetStream ()
	<< "time "
  	<< "flowsize "
  	<< "fct "
  	<< "basefct "
  	<< "slowdown "
  	<< "basertt "
  	<<  "flowstart "
	<< "priority "
	<< "incast "
	<< std::endl;

	torStats = torTraceHelper.CreateFileStream (torOutFile);

	*torStats->GetStream ()
	<< "time "
	// << "tor "
	<< "bufferSizeMB "
	<< "occupiedBufferPct";

	// AnnC: [artemis-star-topoloy] numSinks hard-coded for now
	int numSinks = 1;
	for (int i=0; i<numSinks; i++) {
		for (int j=0; j<nPrior; j++) {
			*torStats->GetStream () << " sink_" << i << "_queue_" << j <<"_qSize";
			// AnnC: [artemis-star-topology] how is this threshold not throughput?
			*torStats->GetStream () << " sink_" << i << "_queue_" << j <<"_threshold";
			*torStats->GetStream () << " sink_" << i << "_queue_" << j <<"_sentBytes";
			*torStats->GetStream () << " sink_" << i << "_queue_" << j <<"_droppedBytes";
			*torStats->GetStream () << " sink_" << i << "_queue_" << j <<"_maxSize";
		}
	}

	/* Links from leaf to servers, lower port IDs. */
	// int csv_header_pcount = 0;
	// for(int i=0; i<SERVER_COUNT; i+=1) {
	// 	for(int j=0; j<nPrior; j+=1) {
	// 		*torStats->GetStream () << " port_" << csv_header_pcount << "_q_" << j <<"_qSize";
	// 		*torStats->GetStream () << " port_" << csv_header_pcount << "_q_" << j <<"_threshold";
	// 		*torStats->GetStream () << " port_" << csv_header_pcount << "_q_" << j <<"_sentBytes";
	// 		*torStats->GetStream () << " port_" << csv_header_pcount << "_q_" << j <<"_droppedBytes";
	// 		*torStats->GetStream () << " port_" << csv_header_pcount << "_q_" << j <<"_maxSize";
	// 	}
	// 	csv_header_pcount++;
	// }

	// /* Links from leaf to spine, higher port IDs. */
	// for(int i=0; i<SPINE_COUNT; i+=1) {
	// 	for(int j=0; j<LINK_COUNT; j+=1) {
	// 		for(int k=0; k<nPrior; k+=1) {
	// 			*torStats->GetStream () << " port_" << csv_header_pcount << "_q_" << k <<"_qSize";
	// 			*torStats->GetStream () << " port_" << csv_header_pcount << "_q_" << k <<"_threshold";
	// 			*torStats->GetStream () << " port_" << csv_header_pcount << "_q_" << k <<"_sentBytes";
	// 			*torStats->GetStream () << " port_" << csv_header_pcount << "_q_" << k <<"_droppedBytes";
	// 			*torStats->GetStream () << " port_" << csv_header_pcount << "_q_" << k <<"_maxSize";
	// 		}
	// 		csv_header_pcount++;
	// 	}
	// }
	
	*torStats->GetStream () << std::endl;
	// << "q1_1,th1_1,q1_2,th1_2," // q1_3 th1_3 q1_4 th1_4 q1_5 th1_5 q1_6 th1_6 q1_7 th1_7 q1_8 th1_8 "
	// << "q2_1,th2_1,q2_2,th2_2," // q2_3 th2_3 q2_4 th2_4 q2_5 th2_5 q2_6 th2_6 q2_7 th2_7 q2_8 th2_8 "
	// << "q3_1,th3_1,q3_2,th3_2," // q3_3 th3_3 q3_4 th3_4 q3_5 th3_5 q3_6 th3_6 q3_7 th3_7 q3_8 th3_8 "
	// << "q4_1,th4_1,q4_2,th4_2," // q4_3 th4_3 q4_4 th4_4 q4_5 th4_5 q4_6 th4_6 q4_7 th4_7 q4_8 th4_8 "
	// << "q5_1,th5_1,q5_2,th5_2," // q5_3 th5_3 q5_4 th5_4 q5_5 th5_5 q5_6 th5_6 q5_7 th5_7 q5_8 th5_8 "
	// << "q6_1,th6_1,q6_2,th6_2," // q6_3 th6_3 q6_4 th6_4 q6_5 th6_5 q6_6 th6_6 q6_7 th6_7 q6_8 th6_8 "
	// << "q7_1,th7_1,q7_2,th7_2," // q7_3 th7_3 q7_4 th7_4 q7_5 th7_5 q7_6 th7_6 q7_7 th7_7 q7_8 th7_8 "
	// << "q8_1,th8_1,q8_2,th8_2" // q8_3 th8_3 q8_4 th8_4 q8_5 th8_5 q8_6 th8_6 q8_7 th8_7 q8_8 th8_8 "
	// << std::endl;

	// AnnC: [artemis-star-topology] double check the buffer calculation here
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


	uint64_t SPINE_LEAF_CAPACITY = spineLeafCapacity * GIGA;
    uint64_t LEAF_SERVER_CAPACITY = leafServerCapacity * GIGA;
    Time LINK_LATENCY = MicroSeconds(linkLatency);

	// AnnC: [artemis-star-topology] the calculation is wrong for sure; leave it as this for now; will come back and change later
	double RTTBytes = (LEAF_SERVER_CAPACITY*1e-6)*linkLatency; // 8 links visited in roundtrip according to the topology, divided by 8 for bytes
	uint32_t RTTPackets = RTTBytes/PACKET_SIZE + 1;
	baseRTTNano = linkLatency*8*1e3;
	nicBw = leafServerCapacity;
    // std::cout << "bandwidth " << spineLeafCapacity << "gbps" <<  " rtt " << linkLatency*8 << "us" << " BDP " << bdp/1e6 << std::endl;

    if (load < 0.0)
    {
    	NS_LOG_ERROR ("Illegal Load value");
    	return 0;
    }

    Config::SetDefault("ns3::GenQueueDisc::updateInterval", UintegerValue(alphaUpdateInterval*linkLatency*8*1000));
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
	// AnnC: [artemis-star-topology] PACKET_SIZE
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (PACKET_SIZE));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
    Config::SetDefault ("ns3::TcpSocket::PersistTimeout", TimeValue (Seconds (20)));

	/****************************************************************/
	/* Swap to a star topology                                      */
	/* Reference: https://github.com/netsyn-princeton/cc-aqm-bm-ns3 */
	/****************************************************************/

	int numNodes = 10;
	NodeContainer nodecontainers;
	// NodeContainer n0, n1, n2, n3, n4, n5, n6, n7, n8, n9, nd, ns; 
	nodecontainers.Create(numNodes);
	// n0.Create (1);
	// n1.Create (1);
	// n2.Create (1);
	// n3.Create (1);
	// n4.Create (1);
	// n5.Create (1);
	// n6.Create (1);
	// n7.Create (1);
	// n8.Create (1);
	// n9.Create (1);
	NodeContainer nd, ns; 
	nd.Create (1);
	ns.Create (1);
	// NodeContainer nodecontainers[numNodes] = {n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};


	InternetStackHelper stack;
	stack.InstallAll ();

	// AnnC: [artemis-star-topology] maybe not necessary; only in ABM, not in Artemis'
	Ipv4GlobalRoutingHelper globalRoutingHelper;
	stack.SetRoutingHelper (globalRoutingHelper);


	TrafficControlHelper tchPfifoFastAccess;
	tchPfifoFastAccess.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize", StringValue ("1000p"));

	TrafficControlHelper tc;
	// AnnC: [artemis-star-topology] declared above
	// uint16_t handle;
	// TrafficControlHelper tc;
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
		case HPCC:
			Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpWien::GetTypeId()));
			// Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::initWienRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
			Config::SetDefault("ns3::TcpSocketState::minWienRate", DataRateValue(DataRate("100Mbps")));
			Config::SetDefault("ns3::TcpSocketState::maxWienRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
			Config::SetDefault("ns3::TcpSocketState::AIWien", DataRateValue(DataRate("100Mbps")));
			Config::SetDefault("ns3::TcpSocketState::mThreshHpcc", UintegerValue(5));
			Config::SetDefault("ns3::TcpSocketState::fastReactHpcc", BooleanValue(true));
			Config::SetDefault("ns3::TcpSocketState::sampleFeedbackHpcc", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::useHpcc", BooleanValue(true));
			Config::SetDefault("ns3::TcpSocketState::multipleRateHpcc", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::targetUtil", DoubleValue(0.95));
			Config::SetDefault("ns3::TcpSocketState::baseRtt", TimeValue(MicroSeconds(linkLatency*4*2 + 2*double(PACKET_SIZE*8)/(LEAF_SERVER_CAPACITY))));
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
		case TIMELY:
			Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpWien::GetTypeId()));
			// Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::initWienRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
			Config::SetDefault("ns3::TcpSocketState::minWienRate", DataRateValue(DataRate("100Mbps")));
			Config::SetDefault("ns3::TcpSocketState::maxWienRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
			Config::SetDefault("ns3::TcpSocketState::AIWien", DataRateValue(DataRate("100Mbps")));
			Config::SetDefault("ns3::TcpSocketState::HighAIWien", DataRateValue(DataRate("150Mbps")));
			Config::SetDefault("ns3::TcpSocketState::useTimely", BooleanValue(true));
			Config::SetDefault("ns3::TcpSocketState::baseRtt", TimeValue(MicroSeconds(linkLatency*4*2 + 2*double(PACKET_SIZE*8)/(LEAF_SERVER_CAPACITY))));
			// Config::SetDefault("ns3::TcpSocketState::TimelyTlow", UintegerValue((linkLatency*4*2*1.5)*1000)); // in nanoseconds
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
		case POWERTCP:
			Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpWien::GetTypeId()));
			// Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::initWienRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
			Config::SetDefault("ns3::TcpSocketState::minWienRate", DataRateValue(DataRate("100Mbps")));
			Config::SetDefault("ns3::TcpSocketState::maxWienRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
			Config::SetDefault("ns3::TcpSocketState::AIWien", DataRateValue(DataRate("100Mbps")));
			Config::SetDefault("ns3::TcpSocketState::mThreshHpcc", UintegerValue(5));
			Config::SetDefault("ns3::TcpSocketState::fastReactHpcc", BooleanValue(true));
			Config::SetDefault("ns3::TcpSocketState::sampleFeedbackHpcc", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::useHpcc", BooleanValue(false)); // This parameter is critical. Pay attention. Both HPCC and PowerTCP are implemented in the same file tcp-wien.
			Config::SetDefault("ns3::TcpSocketState::multipleRateHpcc", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::targetUtil", DoubleValue(0.95));
			Config::SetDefault("ns3::TcpSocketState::baseRtt", TimeValue(MicroSeconds(linkLatency*4*2 + 2*double(PACKET_SIZE*8)/(LEAF_SERVER_CAPACITY))));
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
		case THETAPOWERTCP:
			Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpWien::GetTypeId()));
			// Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::initWienRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
			Config::SetDefault("ns3::TcpSocketState::minWienRate", DataRateValue(DataRate("100Mbps")));
			Config::SetDefault("ns3::TcpSocketState::maxWienRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
			Config::SetDefault("ns3::TcpSocketState::AIWien", DataRateValue(DataRate("100Mbps")));
			Config::SetDefault("ns3::TcpSocketState::mThreshHpcc", UintegerValue(5));
			Config::SetDefault("ns3::TcpSocketState::fastReactHpcc", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::sampleFeedbackHpcc", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::useThetaPower", BooleanValue(true)); // This parameter is critical. Pay attention. Both HPCC and PowerTCP are implemented in the same file tcp-wien.
			Config::SetDefault("ns3::TcpSocketState::multipleRateHpcc", BooleanValue(false));
			Config::SetDefault("ns3::TcpSocketState::targetUtil", DoubleValue(0.95));
			// AnnC: [artemis-star-topology] this part may become a problem when I shift to the star topology
			Config::SetDefault("ns3::TcpSocketState::baseRtt", TimeValue(MicroSeconds(linkLatency*4*2 + 2*double(PACKET_SIZE*8)/(LEAF_SERVER_CAPACITY))));
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
		// case HOMA:
		// 	Config::SetDefault("ns3::HomaL4Protocol::RttPackets", UintegerValue(RTTPackets));
		// 	Config::SetDefault("ns3::HomaL4Protocol::NumTotalPrioBands", UintegerValue(8));
		// 	Config::SetDefault("ns3::HomaL4Protocol::NumUnschedPrioBands", UintegerValue(2));
		// 	Config::SetDefault("ns3::HomaL4Protocol::InbndRtxTimeout", TimeValue (MicroSeconds (1000)));
		// 	Config::SetDefault("ns3::HomaL4Protocol::OutbndRtxTimeout", TimeValue (MicroSeconds (10000)));
		// 	Config::SetDefault("ns3::HomaL4Protocol::OvercommitLevel", UintegerValue(6));
		// 	Config::SetDefault("ns3::HomaL4Protocol::m_linkRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY )));
		// 	Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
		// 	Config::SetDefault("ns3::Ipv4L3Protocol::MayFragment", BooleanValue(false));
		// 	Config::SetDefault("ns3::GenQueueDisc::HomaQueue", BooleanValue(true));
		// 	Config::SetDefault("ns3::GenQueueDisc::nPrior", UintegerValue(8));
		// 	Config::SetDefault("ns3::GenQueueDisc::RoundRobin", UintegerValue(0));
		// 	Config::SetDefault("ns3::GenQueueDisc::StrictPriority", UintegerValue(1));
		// 	handle = tc.SetRootQueueDisc ("ns3::GenQueueDisc");
		// 	nPrior = 8;
		//     cid = tc.AddQueueDiscClasses (handle, nPrior , "ns3::QueueDiscClass");
		// 	for(uint32_t num=0;num<nPrior;num++){
		// 		tc.AddChildQueueDisc (handle, cid[num], "ns3::FifoQueueDisc");
		// 	}
		// 	break;
		default:
			std::cout << "Error in CC configuration" << std::endl;
			return 0;
    }

	// AnnC: [artemis-star-topology] add in this part later; the CC part above has already SetRootQueueDisc to GenQueueDisc
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


	// AnnC: [artemis-star-topology] add in proper queue management; for now, assume droptail
	// std::string queueDiscType = "droptail";
	// bool dropTail = (queueDiscType == "droptail");
	// // AnnC: [artemis-star-topology] add in max queue size for droptail
	// uint32_t queueDiscSize = 10;
	Ipv4AddressHelper address;
	// address.SetBase ("192.168.0.0", "255.255.255.0");
	address.SetBase ("10.1.0.0", "255.255.252.0");

	sharedMemory = CreateObject<SharedMemoryBuffer>();
	sharedMemory->SetAttribute("BufferSize",UintegerValue(BufferSize));
	sharedMemory->SetSharedBufferSize(BufferSize);

	int portid = 0;

	PointToPointHelper accessLink;
	accessLink.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
	accessLink.SetChannelAttribute ("Delay", StringValue ("0.1ms"));
	// AnnC: [artemis-star-topology] maybe no need to explicitly change Queue if we also have QueueDisc
	// if (dropTail) {
	// 	accessLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));
	// }
	/* Server <--> Leaf */
	for (uint32_t server=0; server<nodecontainers.GetN(); server++) {
		NetDeviceContainer netDeviceContainer = accessLink.Install(nodecontainers.Get(server), nd.Get(0));
		QueueDiscContainer queuedisc = tchPfifoFastAccess.Install(netDeviceContainer.Get(1));
		bottleneckQueueDiscs.Add(queuedisc.Get(0));
		Ptr<GenQueueDisc> genDisc = DynamicCast<GenQueueDisc> (queuedisc.Get(0));
		genDisc->SetPortId(portid++);
		// assume to be DT
		// AnnC: [artemis-star-topology] maybe this part is not even necessary
		genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
		// AnnC: [artemis-star-topology] check whether it is right to set it to leafServerCapacity
		genDisc->setPortBw(leafServerCapacity);
		genDisc->SetSharedMemory(sharedMemory);
		genDisc->SetBufferAlgorithm(DT);
		for(uint32_t n=0;n<nPrior;n++){
			genDisc->alphas[n] = alpha_values[n];
		}
		address.NewNetwork ();
		address.Assign (netDeviceContainer);
	}

	PointToPointHelper bottleneckLink;
	bottleneckLink.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));
	bottleneckLink.SetChannelAttribute ("Delay", TimeValue(LINK_LATENCY));
	// if (dropTail) {
	// 	bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (std::to_string(queueDiscSize) + "p"));
	// }
	/* Leaf <--> Sink */
	NetDeviceContainer devicesBottleneckLink = bottleneckLink.Install (nd.Get (0), ns.Get (0));
	// AnnC: [artemis-star-topology] check size here; maybe 1 or 2; could change how we install later
	// std::cout << devicesBottleneckLink.GetN() << std::endl;
	address.NewNetwork ();
	Ipv4InterfaceContainer interfacesBottleneck = address.Assign (devicesBottleneckLink);
	Ipv4InterfaceContainer nsInterface;
  	nsInterface.Add (interfacesBottleneck.Get (1));


	// NetDeviceContainer devicesn0nd = accessLink.Install (n0.Get (0), nd.Get (0));
	// tchPfifoFastAccess.Install (devicesn0nd);
  	// NetDeviceContainer devicesn1nd = accessLink.Install (n1.Get (0), nd.Get (0));
	// tchPfifoFastAccess.Install (devicesn1nd);
	// NetDeviceContainer devicesn2nd = accessLink.Install (n2.Get (0), nd.Get (0));
	// tchPfifoFastAccess.Install (devicesn2nd);
	// NetDeviceContainer devicesn3nd = accessLink.Install (n3.Get (0), nd.Get (0));
	// tchPfifoFastAccess.Install (devicesn3nd);
	// NetDeviceContainer devicesn4nd = accessLink.Install (n4.Get (0), nd.Get (0));
	// tchPfifoFastAccess.Install (devicesn4nd);
	// NetDeviceContainer devicesn5nd = accessLink.Install (n5.Get (0), nd.Get (0));
	// tchPfifoFastAccess.Install (devicesn5nd);
	// NetDeviceContainer devicesn6nd = accessLink.Install (n6.Get (0), nd.Get (0));
	// tchPfifoFastAccess.Install (devicesn6nd);
	// NetDeviceContainer devicesn7nd = accessLink.Install (n7.Get (0), nd.Get (0));
	// tchPfifoFastAccess.Install (devicesn7nd);
	// NetDeviceContainer devicesn8nd = accessLink.Install (n8.Get (0), nd.Get (0));
	// tchPfifoFastAccess.Install (devicesn8nd);
	// NetDeviceContainer devicesn9nd = accessLink.Install (n9.Get (0), nd.Get (0));
	// tchPfifoFastAccess.Install (devicesn9nd);
  
  	
	// address.NewNetwork ();
	// Ipv4InterfaceContainer interfacesn0nd = address.Assign (devicesn0nd);
	// address.NewNetwork();
	// Ipv4InterfaceContainer interfacesn1nd = address.Assign (devicesn1nd);
	// address.NewNetwork();
	// Ipv4InterfaceContainer interfacesn2nd = address.Assign (devicesn2nd);
	// address.NewNetwork();
	// Ipv4InterfaceContainer interfacesn3nd = address.Assign (devicesn3nd);
	// address.NewNetwork();
	// Ipv4InterfaceContainer interfacesn4nd = address.Assign (devicesn4nd);
	// address.NewNetwork();
	// Ipv4InterfaceContainer interfacesn5nd = address.Assign (devicesn5nd);
	// address.NewNetwork();
	// Ipv4InterfaceContainer interfacesn6nd = address.Assign (devicesn6nd);
	// address.NewNetwork();
	// Ipv4InterfaceContainer interfacesn7nd = address.Assign (devicesn7nd);
	// address.NewNetwork();
	// Ipv4InterfaceContainer interfacesn8nd = address.Assign (devicesn8nd);
	// address.NewNetwork();
	// Ipv4InterfaceContainer interfacesn9nd = address.Assign (devicesn9nd);


	// Ipv4InterfaceContainer n0Interface, n1Interface, n2Interface, n3Interface, n4Interface, n5Interface, n6Interface, n7Interface, n8Interface, n9Interface; 
	// n0Interface.Add (interfacesn0nd.Get (0));
	// n1Interface.Add (interfacesn1nd.Get (0));
	// n2Interface.Add (interfacesn2nd.Get (0));
	// n3Interface.Add (interfacesn3nd.Get (0));
	// n4Interface.Add (interfacesn4nd.Get (0));
	// n5Interface.Add (interfacesn5nd.Get (0));
	// n6Interface.Add (interfacesn6nd.Get (0));
	// n7Interface.Add (interfacesn7nd.Get (0));
	// n8Interface.Add (interfacesn8nd.Get (0));
	// n9Interface.Add (interfacesn9nd.Get (0));


	// AnnC: [artemis-star-topology] why is it such that we do not need to install when dropTail?
	//                               ohhh maybe if it's droptail, we dont need to install; its naturally droptail?
	// if (!dropTail) {
	// 	qdiscs = tc.Install (devicesBottleneckLink);
	// }

	
	QueueDiscContainer qdiscs = tc.Install (devicesBottleneckLink.Get(0));
	bottleneckQueueDiscs.Add(qdiscs.Get(0));
	Ptr<GenQueueDisc> genDisc = DynamicCast<GenQueueDisc> (qdiscs.Get(0));
	genDisc->SetPortId(portid++);
	switch(algorithm){
		case DT:
			genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
			// AnnC: [artemis-star-topology] check whether it is right to set it to leafServerCapacity
			genDisc->setPortBw(leafServerCapacity);
			genDisc->SetSharedMemory(sharedMemory);
			genDisc->SetBufferAlgorithm(DT);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc->alphas[n] = alpha_values[n];
			}
			break;
		case FAB:
			genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
			genDisc->setPortBw(leafServerCapacity);
			genDisc->SetSharedMemory(sharedMemory);
			genDisc->SetBufferAlgorithm(FAB);
			genDisc->SetFabWindow(MicroSeconds(5000));
			// AnnC: [artemis-star-topology] PACKET_SIZE should be replaced
			genDisc->SetFabThreshold(15*PACKET_SIZE);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc->alphas[n] = alpha_values[n];
			}
			break;
		case CS:
			genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
			genDisc->setPortBw(leafServerCapacity);
			genDisc->SetSharedMemory(sharedMemory);
			genDisc->SetBufferAlgorithm(CS);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc->alphas[n] = alpha_values[n];
			}
			break;
		case IB:
			genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
			genDisc->setPortBw(leafServerCapacity);
			genDisc->SetSharedMemory(sharedMemory);
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
			genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
			genDisc->setPortBw(leafServerCapacity);
			genDisc->SetSharedMemory(sharedMemory);
			genDisc->SetBufferAlgorithm(ABM);
			for(uint32_t n=0;n<nPrior;n++){
				genDisc->alphas[n] = alpha_values[n];
			}
			break;
		default:
			std::cout << "Error in buffer management configuration. Exiting!";
			return 0;
	}


	// AnnC: [artemis-star-topology] pass in flowsPacketsSize as a parameter
	uint32_t flowsPacketsSize = 1000;
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	// AnnC: [artemis-star-topology] look duplicated from above
	// Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (flowsPacketsSize));
	// Set TCP Protocol for all instantiated TCP Sockets
	// AnnC: [artemis-star-topology] should swap to TCP type defined in macro (or maybe not?)
	// AnnC: [artemis-star-topology] double check: SocketType may have been defined before
	// AnnC: [artemis-star-topology] is this part necessary at all?
	// std::string tcpProtocol = "TcpNewReno";
	// if (tcpProtocol != "TcpBasic") { 
	// 	Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue (TypeId::LookupByName ("ns3::" + tcpProtocol)));
	// }


	// Install applications
	// AnnC: [artemis-star-topology] moved up from below
	NS_LOG_INFO ("Initialize CDF table");
	struct cdf_table* cdfTable = new cdf_table ();
	init_cdf (cdfTable);
	load_cdf (cdfTable, cdfFileName.c_str ());

	uint16_t port = 9;
  	ApplicationContainer sinkApps, sourceApps;
  	// Configure and install upload flow
  	Address addUp (InetSocketAddress (Ipv4Address::GetAny (), port));
  	PacketSinkHelper sinkHelperUp ("ns3::TcpSocketFactory", addUp);
  	sinkHelperUp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
  	sinkApps.Add (sinkHelperUp.Install (ns));

  	InetSocketAddress socketAddressUp = InetSocketAddress (nsInterface.GetAddress (0), port);
  	BulkSendHelper bulkSendHelperUp ("ns3::TcpSocketFactory", Address ());
  	bulkSendHelperUp.SetAttribute ("Remote", AddressValue (socketAddressUp));

	//AnnC: [artemis-star-topology] priority not set

  	uint64_t allflows = 0;
  	srand(randomSeed);
	NS_LOG_INFO ("Initialize random seed: " << randomSeed);
  	for (int i = 0; i < numNodes; i++) {
		//uint64_t flowSize = 1000000000000;
		uint64_t flowSize = gen_random_cdf(cdfTable);
		while (flowSize == 0) { flowSize = gen_random_cdf(cdfTable); }
		allflows += flowSize;
		bulkSendHelperUp.SetAttribute ("MaxBytes", UintegerValue (flowSize));
		sourceApps.Add (bulkSendHelperUp.Install (nodecontainers.Get(i)));
		// AnnC: [artemis-star-topology] in ABM, the request rate (0.2 here) is calculated instead
		double startTime = poission_gen_interval(0.2);
		std::cout << startTime << std::endl;
		sourceApps.Get(i)->SetStartTime (Seconds (startTime));
	}

	// AnnC: [artemis-star-topology] need to check whether it is right to set 0 & END_TIME; we also have a startTime
	sinkApps.Start (Seconds (0));
  	sinkApps.Stop  (Seconds (END_TIME));
  	sourceApps.Stop (Seconds (END_TIME - 0.1));




    // NodeContainer spines;
	// spines.Create (SPINE_COUNT);
	// NodeContainer leaves;
	// leaves.Create (LEAF_COUNT);
	// NodeContainer servers[LEAF_COUNT];
	// Ipv4InterfaceContainer serverIpv4[LEAF_COUNT];

	// std::string TcpMixNames[3] = {"ns3::TcpCubic", "ns3::TcpDctcp", "ns3::TcpWien"};
	// uint32_t mixRatio[2]={cubicMix,dctcpMix};
	// for (uint32_t i = 0; i< LEAF_COUNT; i++){
	// 	servers[i].Create (SERVER_COUNT);
	// 	// if (multiQueueExp){
	// 	// 	for (uint32_t k = 0; k < 3; k++){
	// 	// 		for (uint32_t j = 0; j < SERVER_COUNT/3; j++){
	// 	// 			TypeId tid = TypeId::LookupByName (TcpMixNames[k]);
	// 	// 			std::stringstream nodeId;
	// 	// 			nodeId << servers[i].Get (j)->GetId ();
	// 	// 			std::string specificNode = "/NodeList/" + nodeId.str () + "/$ns3::TcpL4Protocol/SocketType";
	// 	// 			Config::Set (specificNode, TypeIdValue (tid));
	// 	// 		}
	// 	// 	}
	// 	// }
	// }

	// InternetStackHelper internet;
    // Ipv4GlobalRoutingHelper globalRoutingHelper;
	// internet.SetRoutingHelper (globalRoutingHelper);

	// internet.Install(spines);
	// internet.Install(leaves);
	// for (uint32_t i = 0; i<LEAF_COUNT; i++){
	// 	internet.Install(servers[i]);
	// }

	// PointToPointHelper p2p;
    // Ipv4AddressHelper ipv4;
	// Ipv4InterfaceContainer serverNics[LEAF_COUNT][SERVER_COUNT];



    // for (uint32_t leaf = 0; leaf<LEAF_COUNT; leaf++){
    // 	sharedMemoryLeaf[leaf]=CreateObject<SharedMemoryBuffer>();
    // 	sharedMemoryLeaf[leaf]->SetAttribute("BufferSize",UintegerValue(BufferSize));
    // 	sharedMemoryLeaf[leaf]->SetSharedBufferSize(BufferSize);
    // }

    // Ptr<SharedMemoryBuffer> sharedMemorySpine[SPINE_COUNT];
    // for (uint32_t spine = 0; spine<SPINE_COUNT; spine++){
    // 	sharedMemorySpine[spine]=CreateObject<SharedMemoryBuffer>();
    // 	sharedMemorySpine[spine]->SetAttribute("BufferSize",UintegerValue(BufferSize));
    // 	sharedMemorySpine[spine]->SetSharedBufferSize(BufferSize);
    // }

    // uint32_t leafPortId[LEAF_COUNT]={0};
    // uint32_t spinePortId[SPINE_COUNT]={0};
    /*Server <--> Leaf*/
    // ipv4.SetBase ("10.1.0.0", "255.255.252.0");
    // p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));
    // p2p.SetChannelAttribute ("Delay", TimeValue(LINK_LATENCY));
    // p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("10p"));

    // for (uint32_t leaf = 0; leaf<LEAF_COUNT; leaf++){
    // 	ipv4.NewNetwork ();
    // 	for (uint32_t server = 0; server<SERVER_COUNT; server++){
    // 		NodeContainer nodeContainer = NodeContainer (leaves.Get (leaf), servers[leaf].Get (server));
    // 		NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
    // 		QueueDiscContainer queueDiscs;
    // 		queueDiscs = tc.Install(netDeviceContainer.Get(0));

	// 		ToRQueueDiscs[leaf].Add(queueDiscs.Get(0)); //MA
    // 		Ptr<GenQueueDisc> genDisc = DynamicCast<GenQueueDisc> (queueDiscs.Get (0));
    // 		genDisc->SetPortId(leafPortId[leaf]++);
    // 		switch(algorithm){
    // 			case DT:
    // 				genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
    // 				genDisc->setPortBw(leafServerCapacity);
    // 				genDisc->SetSharedMemory(sharedMemoryLeaf[leaf]);
    // 				genDisc->SetBufferAlgorithm(DT);
    // 				for(uint32_t n=0;n<nPrior;n++){
    //                     genDisc->alphas[n] = alpha_values[n];
	// 		std::cout << "n=" << n << ", alpha=" << alpha_values[n] << std::endl;
    //                 }
    // 				break;
    // 			case FAB:
	//     			genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
    // 				genDisc->setPortBw(leafServerCapacity);
    // 				genDisc->SetSharedMemory(sharedMemoryLeaf[leaf]);
    // 				genDisc->SetBufferAlgorithm(FAB);
    // 				genDisc->SetFabWindow(MicroSeconds(5000));
    // 				genDisc->SetFabThreshold(15*PACKET_SIZE);
    // 				for(uint32_t n=0;n<nPrior;n++){
    //                     genDisc->alphas[n] = alpha_values[n];
    //                 }
    // 				break;
    // 			case CS:
    // 				genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
    // 				genDisc->setPortBw(leafServerCapacity);
    // 				genDisc->SetSharedMemory(sharedMemoryLeaf[leaf]);
    // 				genDisc->SetBufferAlgorithm(CS);
    // 				for(uint32_t n=0;n<nPrior;n++){
    //                     genDisc->alphas[n] = alpha_values[n];
    //                 }
    // 				break;
    // 			case IB:
	//     			genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
    // 				genDisc->setPortBw(leafServerCapacity);
    // 				genDisc->SetSharedMemory(sharedMemoryLeaf[leaf]);
    // 				genDisc->SetBufferAlgorithm(IB);
    //                 genDisc->SetAfdWindow(MicroSeconds(50));
    //                 genDisc->SetDppWindow(MicroSeconds(5000));
    //                 genDisc->SetDppThreshold(RTTPackets);
    // 				for(uint32_t n=0;n<nPrior;n++){
    //                     genDisc->alphas[n] = alpha_values[n];
    //                     genDisc->SetQrefAfd(n,uint32_t(RTTBytes));
    //                 }
    // 				break;
    // 			case ABM:
    // 				genDisc->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
    // 				genDisc->setPortBw(leafServerCapacity);
    // 				genDisc->SetSharedMemory(sharedMemoryLeaf[leaf]);
    // 				genDisc->SetBufferAlgorithm(ABM);
    // 				for(uint32_t n=0;n<nPrior;n++){
    //                     genDisc->alphas[n] = alpha_values[n];
    //                 }
    // 				break;
    // 			default:
    // 				std::cout << "Error in buffer management configuration. Exiting!";
    // 				return 0;
    // 		}
    // 		serverNics[leaf][server] = ipv4.Assign(netDeviceContainer.Get(1));
    // 		ipv4.Assign(netDeviceContainer.Get(0));
    // 	}
    // }

	// AnnC: [artemis-star-topology] this part seems not used at all in ABM; just commented out for now
    // std::vector<InetSocketAddress> clients[LEAF_COUNT];
    // for (uint32_t leaf = 0; leaf<LEAF_COUNT; leaf++){
    // 	for (uint32_t leafnext = 0; leafnext < LEAF_COUNT ; leafnext++){
    // 		if (leaf==leafnext){
    // 			continue;
    // 		}
    // 		for (uint32_t server = 0; server < SERVER_COUNT; server++){
    // 			clients[leaf].push_back(InetSocketAddress (serverNics[leafnext][server].GetAddress (0), 1000+leafnext*LEAF_COUNT + server));
    // 		}
    // 	}
    // }

   /*Leaf <--> Spine*/
    // p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (SPINE_LEAF_CAPACITY)));
    // p2p.SetChannelAttribute ("Delay", TimeValue(LINK_LATENCY));
    // // p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("10p"));
    // for (uint32_t leaf = 0; leaf<LEAF_COUNT; leaf++){
    // 	for (uint32_t spine = 0; spine< SPINE_COUNT; spine++){
    // 		for (uint32_t link = 0; link< LINK_COUNT; link++){
    // 			ipv4.NewNetwork ();
    // 			NodeContainer nodeContainer = NodeContainer (leaves.Get (leaf), spines.Get (spine));
    //             NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
    //             QueueDiscContainer queueDiscs = tc.Install(netDeviceContainer);
    //             northQueues[leaf].Add(queueDiscs.Get(0));
	// 	ToRQueueDiscs[leaf].Add(queueDiscs.Get(0));// MA
    //             Ptr<GenQueueDisc> genDisc[2];
    //             genDisc[0] = DynamicCast<GenQueueDisc> (queueDiscs.Get (0));
    //             genDisc[0]->SetSharedMemory(sharedMemoryLeaf[leaf]);
    //             genDisc[1] = DynamicCast<GenQueueDisc> (queueDiscs.Get (1));
    //             genDisc[1]->SetSharedMemory(sharedMemorySpine[spine]);
    //             genDisc[0]->SetPortId(leafPortId[leaf]++);
    //             genDisc[1]->SetPortId(spinePortId[spine]++);
    //             for (uint32_t i = 0; i<2; i++){
	//                 switch(algorithm){
	// 	    			case DT:
	// 	    				genDisc[i]->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	// 	    				genDisc[i]->setPortBw(leafServerCapacity);
	// 	    				genDisc[i]->SetSharedMemory(sharedMemoryLeaf[leaf]);
	// 	    				genDisc[i]->SetBufferAlgorithm(DT);
	// 	    				for(uint32_t n=0;n<nPrior;n++){
	// 	                        genDisc[i]->alphas[n] = alpha_values[n];
	// 						std::cout << "n=" << n << ", alpha=" << alpha_values[n] << std::endl;
	// 	                    }
	// 	    				break;
	// 	    			case FAB:
	// 		    			genDisc[i]->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	// 	    				genDisc[i]->setPortBw(leafServerCapacity);
	// 	    				genDisc[i]->SetSharedMemory(sharedMemoryLeaf[leaf]);
	// 	    				genDisc[i]->SetBufferAlgorithm(FAB);
	// 	    				genDisc[i]->SetFabWindow(MicroSeconds(5000));
	// 	    				genDisc[i]->SetFabThreshold(15*PACKET_SIZE);
	// 	    				for(uint32_t n=0;n<nPrior;n++){
	// 	                        genDisc[i]->alphas[n] = alpha_values[n];
	// 	                    }
	// 	    				break;
	// 	    			case CS:
	// 	    				genDisc[i]->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	// 	    				genDisc[i]->setPortBw(leafServerCapacity);
	// 	    				genDisc[i]->SetSharedMemory(sharedMemoryLeaf[leaf]);
	// 	    				genDisc[i]->SetBufferAlgorithm(CS);
	// 	    				for(uint32_t n=0;n<nPrior;n++){
	// 	                        genDisc[i]->alphas[n] = alpha_values[n];
	// 	                    }
	// 	    				break;
	// 	    			case IB:
	// 		    			genDisc[i]->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	// 	    				genDisc[i]->setPortBw(leafServerCapacity);
	// 	    				genDisc[i]->SetSharedMemory(sharedMemoryLeaf[leaf]);
	// 	    				genDisc[i]->SetBufferAlgorithm(IB);
	// 	                    genDisc[i]->SetAfdWindow(MicroSeconds(50));
	// 	                    genDisc[i]->SetDppWindow(MicroSeconds(5000));
	// 	                    genDisc[i]->SetDppThreshold(RTTPackets);
	// 	    				for(uint32_t n=0;n<nPrior;n++){
	// 	                        genDisc[i]->alphas[n] = alpha_values[n];
	// 	                        genDisc[i]->SetQrefAfd(n,uint32_t(RTTBytes));
	// 	                    }
	// 	    				break;
	// 	    			case ABM:
	// 	    				genDisc[i]->setNPrior(nPrior); // IMPORTANT. This will also trigger "alphas = new ..."
	// 	    				genDisc[i]->setPortBw(leafServerCapacity);
	// 	    				genDisc[i]->SetSharedMemory(sharedMemoryLeaf[leaf]);
	// 	    				genDisc[i]->SetBufferAlgorithm(ABM);
	// 	    				for(uint32_t n=0;n<nPrior;n++){
	// 	                        genDisc[i]->alphas[n] = alpha_values[n];
	// 	                    }
	// 	    				break;
	// 	    			default:
	// 	    				std::cout << "Error in buffer management configuration. Exiting!";
	// 	    				return 0;
	// 	    		}
	// 	    	}
	//     		Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);
    // 		}
    // 	}
    // }

	// double oversubRatio = static_cast<double>(SERVER_COUNT * LEAF_SERVER_CAPACITY) / (SPINE_LEAF_CAPACITY * SPINE_COUNT * LINK_COUNT);
	// NS_LOG_INFO ("Over-subscription ratio: " << oversubRatio);
	// NS_LOG_INFO ("Initialize CDF table");
	// struct cdf_table* cdfTable = new cdf_table ();
	// init_cdf (cdfTable);
	// load_cdf (cdfTable, cdfFileName.c_str ());
	// NS_LOG_INFO ("Calculating request rate");
	// double requestRate = load * LEAF_SERVER_CAPACITY * SERVER_COUNT / oversubRatio / (8 * avg_cdf (cdfTable)) / SERVER_COUNT;
	// NS_LOG_INFO ("Average request rate: " << requestRate << " per second");
	// NS_LOG_INFO ("Initialize random seed: " << randomSeed);
	// if (randomSeed == 0)
	// {
	// srand ((unsigned)time (NULL));
	// }
	// else
	// {
	// srand (randomSeed);
	// }
	// double QUERY_START_TIME = START_TIME;

	// long flowCount=0;

	// if (TcpProt != HOMA){
	// 	for (int fromLeafId = 1; fromLeafId < LEAF_COUNT; fromLeafId ++)
	//     {
    //                     std::cout << "fromLeafId=" << fromLeafId << std::endl;
	// 		//install_applications_simple_noincast_continuous(fromLeafId, servers, requestRate, cdfTable, flowCount, SERVER_COUNT, LEAF_COUNT, START_TIME, END_TIME, FLOW_LAUNCH_END_TIME,nPrior);
	// 		// install_applications_simple_noincast_bursty(fromLeafId, servers, requestRate, cdfTable, flowCount, SERVER_COUNT, LEAF_COUNT, START_TIME, END_TIME, FLOW_LAUNCH_END_TIME,nPrior);
	// 		install_applications(fromLeafId, servers, requestRate, cdfTable, flowCount, SERVER_COUNT, LEAF_COUNT, START_TIME, END_TIME, FLOW_LAUNCH_END_TIME,nPrior);
	// 		if (queryRequestRate>0 && requestSize>0){
	// 			install_applications_incast(fromLeafId, servers, queryRequestRate,requestSize, cdfTable, flowCount, SERVER_COUNT, LEAF_COUNT, QUERY_START_TIME, END_TIME, FLOW_LAUNCH_END_TIME,nPrior);
	// 		}
	//     }
	// 	// std::cout << "hi" << std::endl;
	//  //    Config::ConnectWithoutContext("/NodeList/*/$ns3::PacketSink/FlowFinish", MakeBoundCallback(&TraceMsgFinish, msgStream));
	//  //    std::cout << "bye" << std::endl;
	// }
	// else{
	// 	// Currently, incast workload is not supported here. We only use HOMA with a given flowsize cdf file.
	// 	HomaHeader homah;
	// 	Ipv4Header ipv4h;
	// 	uint32_t payloadSize = PACKET_SIZE;
	// 	Config::SetDefault("ns3::MsgGeneratorApp::PayloadSize", UintegerValue(payloadSize));
	// 	Config::SetDefault("ns3::MsgGeneratorApp::cdfFile", StringValue(cdfFileName));
	// 	for (int fromLeafId = 0; fromLeafId < LEAF_COUNT; fromLeafId++){
	// 		for (int txServer = 0; txServer < SERVER_COUNT; txServer++){
	// 			Ptr<MsgGeneratorApp> app = CreateObject<MsgGeneratorApp>(serverNics[fromLeafId][txServer].GetAddress (0), 1000 + fromLeafId*SERVER_COUNT + txServer);
	// 		    app->Install (servers[fromLeafId].Get (txServer), clients[fromLeafId]);
	// 		    app->setReqRate(requestRate);
	// 		    // app->SetWorkload (networkLoad, msgSizeCDF, avgMsgSizePkts);
	// 		    app->Start(Seconds (START_TIME));
	// 		    app->Stop(Seconds (FLOW_LAUNCH_END_TIME));
	// 		}
	// 	}

	// 	Config::ConnectWithoutContext("/NodeList/*/$ns3::HomaL4Protocol/MsgFinish", MakeBoundCallback(&TraceMsgFinish, fctOutput));
	// }


	// Simulator::Schedule(Seconds(START_TIME),InvokeToRStats,torStats, BufferSize, LEAF_COUNT, printDelay);
	Simulator::Schedule(Seconds(START_TIME),StarTopologyInvokeToRStats,torStats, BufferSize, LEAF_COUNT, printDelay);


	// AsciiTraceHelper ascii;
 //    p2p.EnableAsciiAll (ascii.CreateFileStream ("eval.tr"));
	// Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	// NS_LOG_UNCOND("Running the Simulation...!");
	std::cout << "Running the Simulation...!" << std::endl;
	Simulator::Stop (Seconds (END_TIME));
	Simulator::Run ();
	Simulator::Destroy ();
	free_cdf (cdfTable);
	return 0;
}

