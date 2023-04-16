/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Stanford University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Serhat Arslan <sarslan@stanford.edu>
 */

#include "msg-generator-app.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/callback.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/double.h"

#include "ns3/udp-socket-factory.h"
#include "ns3/homa-socket-factory.h"
#include "ns3/point-to-point-net-device.h"

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MsgGeneratorApp");

NS_OBJECT_ENSURE_REGISTERED (MsgGeneratorApp);

TypeId
MsgGeneratorApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MsgGeneratorApp")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddAttribute ("Protocol", "The type of protocol to use. This should be "
                    "a subclass of ns3::SocketFactory",
                    TypeIdValue (HomaSocketFactory::GetTypeId ()),
                    MakeTypeIdAccessor (&MsgGeneratorApp::m_tid),
                    // This should check for SocketFactory as a parent
                    MakeTypeIdChecker ())
    .AddAttribute ("MaxMsg",
                   "The total number of messages to send. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&MsgGeneratorApp::m_maxMsgs),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("MaxSize",
					"The total number of messages to send. The value zero means "
					"that there is no limit.",
					UintegerValue (0),
					MakeUintegerAccessor (&MsgGeneratorApp::m_maxSize),
					MakeUintegerChecker<uint64_t> ())
	.AddAttribute ("Sink", "The total number of messages to send. The value zero means "
										"that there is no limit.",
										BooleanValue (false),
										MakeBooleanAccessor (&MsgGeneratorApp::m_sink),
										MakeBooleanChecker ())
    .AddAttribute ("PayloadSize",
                   "MTU for the network interface excluding the header sizes",
                   UintegerValue (1400),
                   MakeUintegerAccessor (&MsgGeneratorApp::m_maxPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("cdfFile",
                   "path to the cdf file",
                   StringValue ("/home/vamsi/HomaL4Protocol-ns-3/scratch/websearch.txt"),
                   MakeStringAccessor (&MsgGeneratorApp::cdfFileName),
                   MakeStringChecker())
  ;
  return tid;
}

MsgGeneratorApp::MsgGeneratorApp(Ipv4Address localIp, uint16_t localPort)
  : m_socket (0),
    m_interMsgTime (0),
    m_msgSizePkts (0),
    m_remoteClient (0),
    m_totMsgCnt (0)
{
  NS_LOG_FUNCTION (this << localIp << localPort);

  m_localIp = localIp;
  m_localPort = localPort;
}

MsgGeneratorApp::~MsgGeneratorApp()
{
  NS_LOG_FUNCTION (this);
  free_cdf_msg (cdfTable);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* initialize a CDF distribution */
void 
MsgGeneratorApp::init_cdf_msg(struct cdf_table_msg *table)
{
    if(!table)
        return;

    table->entries = (struct cdf_entry_msg*)malloc(TG_CDF_TABLE_ENTRY * sizeof(struct cdf_entry_msg));
    table->num_entry = 0;
    table->max_entry = TG_CDF_TABLE_ENTRY;
    table->min_cdf = 0;
    table->max_cdf = 1;

    if (!(table->entries))
        perror("Error: malloc entries in init_cdf()");
}

/* free resources of a CDF distribution */
void 
MsgGeneratorApp::free_cdf_msg(struct cdf_table_msg *table)
{
    if (table)
        free(table->entries);
}

/* get CDF distribution from a given file */
void 
MsgGeneratorApp::load_cdf_msg(struct cdf_table_msg *table, const char *file_name)
{
    FILE *fd = NULL;
    char line[256] = {0};
    struct cdf_entry_msg *e = NULL;
    int i = 0;

    if (!table)
        return;

    fd = fopen(file_name, "r");
    if (!fd)
        perror("Error: open the CDF file in load_cdf()");

    while (fgets(line, sizeof(line), fd))
    {
        /* resize entries */
        if (table->num_entry >= table->max_entry)
        {
            table->max_entry *= 2;
            e = (struct cdf_entry_msg*)malloc(table->max_entry * sizeof(struct cdf_entry_msg));
            if (!e)
                perror("Error: malloc entries in load_cdf()");
            for (i = 0; i < table->num_entry; i++)
                e[i] = table->entries[i];
            free(table->entries);
            table->entries = e;
        }

        sscanf(line, "%lf %lf", &(table->entries[table->num_entry].value), &(table->entries[table->num_entry].cdf));

        if (table->min_cdf > table->entries[table->num_entry].cdf)
            table->min_cdf = table->entries[table->num_entry].cdf;
        if (table->max_cdf < table->entries[table->num_entry].cdf)
            table->max_cdf = table->entries[table->num_entry].cdf;

        table->num_entry++;
    }
    fclose(fd);
}

/* print CDF distribution information */
void 
MsgGeneratorApp::print_cdf_msg(struct cdf_table_msg *table)
{
    int i = 0;

    if (!table)
        return;

    for (i = 0; i < table->num_entry; i++)
        printf("%.2f %.2f\n", table->entries[i].value, table->entries[i].cdf);
}

/* get average value of CDF distribution */
double 
MsgGeneratorApp::avg_cdf_msg(struct cdf_table_msg *table)
{
    int i = 0;
    double avg = 0;
    double value, prob;

    if (!table)
        return 0;

    for (i = 0; i < table->num_entry; i++)
    {
        if (i == 0)
        {
            value = table->entries[i].value / 2;
            prob = table->entries[i].cdf;
        }
        else
        {
            value = (table->entries[i].value + table->entries[i-1].value) / 2;
            prob = table->entries[i].cdf - table->entries[i-1].cdf;
        }
        avg += (value * prob);
    }

    return avg;
}

double 
MsgGeneratorApp::interpolate_msg(double x, double x1, double y1, double x2, double y2)
{
    if (x1 == x2)
        return (y1 + y2) / 2;
    else
        return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
}

/* generate a random floating point number from min to max */
double 
MsgGeneratorApp::rand_range_msg(double min, double max)
{
    return min + rand() * (max - min) / RAND_MAX;
}

/* generate a random value based on CDF distribution */
double 
MsgGeneratorApp::gen_random_cdf_msg(struct cdf_table_msg *table)
{
    int i = 0;
    double x = rand_range_msg(table->min_cdf, table->max_cdf);
    /* printf("%f %f %f\n", x, table->min_cdf, table->max_cdf); */

    if (!table)
        return 0;

    for (i = 0; i < table->num_entry; i++)
    {
        if (x <= table->entries[i].cdf)
        {
            if (i == 0)
                return interpolate_msg(x, 0, 0, table->entries[i].cdf, table->entries[i].value);
            else
                return interpolate_msg(x, table->entries[i-1].cdf, table->entries[i-1].value, table->entries[i].cdf, table->entries[i].value);
        }
    }

    return table->entries[table->num_entry-1].value;
}

double 
MsgGeneratorApp::poission_gen_interval_msg(double avg_rate)
{
    if (avg_rate > 0)
       return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
    else
       return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void MsgGeneratorApp::Install (Ptr<Node> node, 
                               std::vector<InetSocketAddress> remoteClients)
{
  NS_LOG_FUNCTION(this << node);

  node->AddApplication (this);

  if (m_maxMsgs==0){
    NS_ASSERT(cdfFileName!="");
    cdfTable = new cdf_table_msg ();
    init_cdf_msg (cdfTable);
    load_cdf_msg (cdfTable, cdfFileName.c_str ());
    srand (7);
  }

  m_socket = Socket::CreateSocket (node, m_tid);
  m_socket->Bind (InetSocketAddress(m_localIp, m_localPort));
  m_socket->SetRecvCallback (MakeCallback (&MsgGeneratorApp::ReceiveMessage, this));

  for (std::size_t i = 0; i < remoteClients.size(); i++)
  {
    if (remoteClients[i].GetIpv4() != m_localIp ||
        remoteClients[i].GetPort() != m_localPort)
    {
      m_remoteClients.push_back(remoteClients[i]);
    }
    else
    {
      // Remove the local address from the client addresses list
      NS_LOG_LOGIC("MsgGeneratorApp (" << this <<
                   ") removes address " << remoteClients[i].GetIpv4() <<
                   ":" << remoteClients[i].GetPort() <<
                   " from remote clients because it is the local address.");
    }
  }

  m_remoteClient = CreateObject<UniformRandomVariable> ();
  m_remoteClient->SetAttribute ("Min", DoubleValue (0));
  m_remoteClient->SetAttribute ("Max", DoubleValue (m_remoteClients.size()));
}


void
MsgGeneratorApp::setReqRate(double reqrate){
  requestRate = reqrate;
}



void MsgGeneratorApp::SetWorkload (double load, 
                                   std::map<double,int> msgSizeCDF,
                                   double avgMsgSizePkts)
{
  NS_LOG_FUNCTION(this << avgMsgSizePkts);

  load = std::max(0.0, std::min(load, 1.0));

  Ptr<NetDevice> netDevice = GetNode ()->GetDevice (0);
  uint32_t mtu = netDevice->GetMtu ();

  PointToPointNetDevice* p2pNetDevice = dynamic_cast<PointToPointNetDevice*>(&(*(netDevice)));
  uint64_t txRate = p2pNetDevice->GetDataRate ().GetBitRate ();

  double avgPktLoadBytes = (double)(mtu + 64); // Account for the ctrl pkts each data pkt induce
  double avgInterMsgTime = (avgMsgSizePkts * avgPktLoadBytes * 8.0 ) / (((double)txRate) * load);

  m_interMsgTime = CreateObject<ExponentialRandomVariable> ();
  m_interMsgTime->SetAttribute ("Mean", DoubleValue (avgInterMsgTime));

  m_msgSizeCDF = msgSizeCDF;

  m_msgSizePkts = CreateObject<UniformRandomVariable> ();
  m_msgSizePkts->SetAttribute ("Min", DoubleValue (0));
  m_msgSizePkts->SetAttribute ("Max", DoubleValue (1));
}

void MsgGeneratorApp::Start (Time start)
{
  NS_LOG_FUNCTION (this);

  SetStartTime(start);
  DoInitialize();
}

void MsgGeneratorApp::Stop (Time stop)
{
  NS_LOG_FUNCTION (this);

  SetStopTime(stop);
}

void MsgGeneratorApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  CancelNextEvent ();
  // chain up
  Application::DoDispose ();
}

void MsgGeneratorApp::StartApplication ()
{
  NS_LOG_FUNCTION (Simulator::Now ().GetNanoSeconds () << this);

//  if(m_maxMsgs==0)
//	  NS_ASSERT_MSG(m_remoteClient && m_interMsgTime && m_msgSizePkts,
//                "MsgGeneratorApp should be installed on a node and "
//                "the workload should be set before starting the application!");
  if (m_sink)
	  return;

  // SendMessage ();
  ScheduleNextMessage ();
}

void MsgGeneratorApp::StopApplication ()
{
  NS_LOG_FUNCTION (Simulator::Now ().GetNanoSeconds () << this);

  CancelNextEvent();
  // m_socket->Close();
}

void MsgGeneratorApp::CancelNextEvent()
{
  NS_LOG_FUNCTION (this);

  if (!Simulator::IsExpired(m_nextSendEvent))
    Simulator::Cancel (m_nextSendEvent);
}

void MsgGeneratorApp::ScheduleNextMessage ()
{
  NS_LOG_FUNCTION (Simulator::Now ().GetNanoSeconds () << this);

  if (Simulator::IsExpired(m_nextSendEvent))
  {
    // std::cout << poission_gen_interval (requestRate) << std::endl;
	  if (m_maxMsgs==0)
      m_nextSendEvent = Simulator::Schedule(Seconds(poission_gen_interval_msg (requestRate)), &MsgGeneratorApp::SendMessage, this);
		  // m_nextSendEvent = Simulator::Schedule (Seconds (m_interMsgTime->GetValue ()), &MsgGeneratorApp::SendMessage, this);
	  else{
      if (m_totMsgCnt==0)
        m_nextSendEvent = Simulator::Schedule (Seconds (0), &MsgGeneratorApp::SendMessage, this);  
		  else
        m_nextSendEvent = Simulator::Schedule (Seconds (m_maxSize/double(25*1e9/8)), &MsgGeneratorApp::SendMessage, this);
    }
  }
  // else
  // {
  //   NS_LOG_WARN("MsgGeneratorApp (" << this <<
  //               ") tries to schedule the next msg before the previous one is sent!");
  // }
}

uint32_t MsgGeneratorApp::GetNextMsgSizeFromDist ()
{
  NS_LOG_FUNCTION(this);

  int msgSizePkts = -1;
  double rndValue = m_msgSizePkts->GetValue();
  for (auto it = m_msgSizeCDF.begin(); it != m_msgSizeCDF.end(); it++)
  {
    if (rndValue <= it->first)
    {
      msgSizePkts = it->second;
      break;
    }
  }

  NS_ASSERT(msgSizePkts >= 0);
  // Homa header can't handle msgs larger than 0xffff pkts
  msgSizePkts = std::min(0xffff, msgSizePkts);

  if (m_maxPayloadSize > 0)
    return m_maxPayloadSize * (uint32_t)msgSizePkts;
  else
    return GetNode ()->GetDevice (0)->GetMtu () * (uint32_t)msgSizePkts;

  // NOTE: If maxPayloadSize is not set, the generated messages will be
  //       slightly larger than the intended number of packets due to
  //       the addition of the protocol headers.
}

void MsgGeneratorApp::SendMessage ()
{
  NS_LOG_FUNCTION (Simulator::Now ().GetNanoSeconds () << this);

  /* Decide which remote client to send to */
  double rndValue = m_remoteClient->GetValue ();
  int remoteClientIdx = (int) std::floor(rndValue);

  InetSocketAddress receiverAddr = m_remoteClients[remoteClientIdx];

  /* Decide on the message size to send */
  uint64_t msgSizeBytes;
  if (m_maxSize>0){
	  msgSizeBytes = m_maxSize;
  }
  else{
	  msgSizeBytes = gen_random_cdf_msg (cdfTable);  //GetNextMsgSizeFromDist ();
  }

  /* Create the message to send */
  Ptr<Packet> msg = Create<Packet> (msgSizeBytes);
  NS_LOG_LOGIC ("MsgGeneratorApp {" << this << ") generates a message of size: "
                << msgSizeBytes << " Bytes.");

  uint32_t sentBytes = m_socket->SendTo (msg, 0, receiverAddr);

  if (sentBytes > 0)
  {
    NS_LOG_INFO(sentBytes << " Bytes sent to " << receiverAddr);
    m_totMsgCnt++;
  }
  // std::cout << "sentMsgs " << m_totMsgCnt << " sentBytes "<< sentBytes << std::endl;
  if (m_maxMsgs == 0 || m_totMsgCnt < m_maxMsgs)
  {
    ScheduleNextMessage ();
  }
}

void MsgGeneratorApp::ReceiveMessage (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);

  Ptr<Packet> message;
  Address from;
  while ((message = socket->RecvFrom (from)))
  {
    NS_LOG_DEBUG (Simulator::Now ().GetNanoSeconds () <<
                 " client received " << message->GetSize () << " bytes from " <<
                 InetSocketAddress::ConvertFrom (from).GetIpv4 () << ":" <<
                 InetSocketAddress::ConvertFrom (from).GetPort ());
  }
}

} // Namespace ns3
