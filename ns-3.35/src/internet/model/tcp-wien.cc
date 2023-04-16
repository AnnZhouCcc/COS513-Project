/*
 * tcp-wien.cc
 *
 *  Created on: Jan 9, 2021
 *      Author: vamsi
 */

#include "tcp-socket-state.h"
#include "tcp-wien.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpWien");
NS_OBJECT_ENSURE_REGISTERED (TcpWien);

TypeId
TcpWien::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpWien")
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpWien> ()
    .SetGroupName ("Internet")
    .AddAttribute ("Alpha", "Lower bound of packets in network",
                   UintegerValue (2),
                   MakeUintegerAccessor (&TcpWien::m_alpha),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Beta", "Upper bound of packets in network",
                   UintegerValue (4),
                   MakeUintegerAccessor (&TcpWien::m_beta),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Gamma", "Limit on increase",
                   UintegerValue (1),
                   MakeUintegerAccessor (&TcpWien::m_gamma),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

TcpWien::TcpWien (void)
  : TcpNewReno (),
    m_baseRtt (MicroSeconds(60)),
    m_minRtt (MicroSeconds(60))
{
	setTcpWien();
  NS_LOG_FUNCTION (this);
}

TcpWien::TcpWien (const TcpWien& sock)
  : TcpNewReno (sock),
    m_baseRtt (sock.m_baseRtt),
    m_minRtt (sock.m_minRtt)
{
	setTcpWien();
  NS_LOG_FUNCTION (this);
}

TcpWien::~TcpWien (void)
{
  NS_LOG_FUNCTION (this);
}

void
TcpWien::Init(Ptr<TcpSocketState> tcb){
	NS_LOG_FUNCTION (this << tcb);
	NS_LOG_INFO (this << "TcpWien");

	// setCurrentRate(10*tcb->m_segmentSize);

//	tcb->wienRate = DataRate("1Gbps"); // temp
//	tcb->minWienRate=DataRate("50Mbps");
//	tcb->maxwienRate = DataRate ("30Gbps"); // temp
//	tcb->wienAddInc= DataRate("50Mbps");
//	tcb->m_miThresh = 5;
//	tcb->m_fast_react = true;
//	tcb->m_sampleFeedback = false;
//	tcb->m_multipleRate = false;
//	tcb->m_targetUtil = 0.95;
//
//	tcb->wienInitRtt = MicroSeconds(60);
	// tcb->m_initialCWnd = double(tcb->maxwienRate.GetBitRate())* (tcb->wienInitRtt.GetSeconds())/ ( 8 * tcb->m_segmentSize ) ;

	// tcb->m_setWienRateCallback(tcb,tcb->wienRate,tcb->wienRate, m_baseRtt);

	hpcc = tcb->useHpcc;

		if (!tcb->useTimely){
			tcb->m_initialCWnd = (tcb->maxwienRate.GetBitRate()*tcb->wienInitRtt.GetSeconds())/(8*tcb->m_segmentSize);
		}
		else{
			tcb->m_initialCWnd = (UINT32_MAX-1e3);	
		}
		tcb->wienRate=tcb->maxwienRate;

	m_baseRtt = tcb->wienInitRtt;

	if (!tcb->useTimely){
		tcb->m_setWienRateCallback(tcb,tcb->wienRate,tcb->wienRate, m_baseRtt,true);
	}
	else{
		tcb->m_setWienRateCallback(tcb,tcb->wienRate,tcb->wienRate, m_baseRtt,false);
	}

	// the below are used only by HPCC if enabled
	wienRateAgg = tcb->wienRate;
	for (int i =0; i < 10; i++)
		wienRateHop[i] = tcb->wienRate;

}

Ptr<TcpCongestionOps>
TcpWien::Fork (void)
{
  return CopyObject<TcpWien> (this);
}

void
TcpWien::SetWienRate(Ptr<TcpSocketState> tcb, DataRate rate, DataRate prevRate){

	tcb->wienRate = rate;
	if (wien_pacingTimer.IsRunning()){

		Time left = wien_pacingTimer.GetDelayLeft();

		Time prevRateTime = prevRate.CalculateBytesTxTime(tcb->m_segmentSize);
//		Time prevSched = Simulator::Now() - (prevRateTime - left);

		wien_pacingTimer.Suspend();

		Time toSend = tcb->wienRate.CalculateBytesTxTime(tcb->m_segmentSize);

		wien_pacingTimer.Schedule(toSend+left-prevRateTime);
		}
}






uint64_t GetTimeDelta(uint64_t a, uint64_t b){
	if(a>b){
		return a-b;
	}
	else{
		return b-a;
	}
}

uint64_t GetBytesDelta(uint64_t a, uint64_t b){
	if(a>b){
		return a-b;
	}
	else
		return b-a;
}

double GetUDelta (double a, double b){
	if (a>b)
		return a-b;
	if (b>a)
		return b-a;
}

void TcpWien::ProcessIntWien(Ptr<Packet> packet, const TcpHeader& tcpHeader, Ptr<TcpSocketState> tcb){

	bool found;
	IntHeader Int;
	found = packet->PeekPacketTag(Int);

	uint32_t ackNum = tcpHeader.GetAckNumber().GetValue();

	if (found){
		if (tcb->useHpcc){
			if (ackNum > lastUpdatedSeq){ 

				UpdateRateHp(packet,tcpHeader,tcb,Int,false);
			}else{ 
				FastReactHp(packet,tcpHeader,tcb,Int);
			}
		}
		else if (tcb->useTimely){
			// std::cout << tcb->m_cWnd.Get() << std::endl;
			if (ackNum > lastUpdatedSeq){ 

				UpdateRateTimely(packet,tcpHeader,tcb,Int,false);
			}else{ 
				FastReactTimely(packet,tcpHeader,tcb,Int);
			}
		}
		else if (tcb->useThetaPower){
			if (ackNum > lastUpdatedSeq){ 

				UpdateRateThetaPowertcp(packet,tcpHeader,tcb,Int,false);
			}else{ 
				FastReactThetaPowertcp(packet,tcpHeader,tcb,Int);
			}
		}
		else if (wienWait>=0){
			wienWait=0;
			if (ackNum > lastUpdatedSeq){
				UpdateRateWien(packet,tcpHeader,tcb,Int,false);
			}else{
				FastReactWien(packet,tcpHeader,tcb,Int);
			}
		}
		wienWait++;
	}
}



void TcpWien::UpdateRateWien(Ptr<Packet> packet, const TcpHeader& tcpHeader, Ptr<TcpSocketState> tcb, IntHeader ih, bool fast_react){
		uint32_t next_seq = tcb->m_nextTxSequence.Get().GetValue();
		uint32_t ackNum = tcpHeader.GetAckNumber().GetValue();


	if (lastUpdatedSeq == 0){
		lastUpdatedSeq = next_seq;
		lastAckedSeq = ackNum;

		NS_ASSERT(ih.getHopCount() <= ih.getMaxHops());
		IntPrev = ih;
	}else {
		
		if (ih.getHopCount()<= ih.getMaxHops()){
			double max_c = 0;
			bool inStable = false;

			double U = 0;
			uint64_t dt = 0;
			bool updated[ih.getMaxHops()] = {false}, updated_any = false;

			NS_ASSERT(ih.getHopCount()<= ih.getMaxHops());
			
			for (uint32_t i = 0; i < ih.getHopCount(); i++){
				IntHeader::telemetry tmt = ih.getFeedback(i);
				IntHeader::telemetry tmtPrev = IntPrev.getFeedback(i);
				if (tcb->m_sampleFeedback){
					if (tmt.qlenDeq == 0 and fast_react)
						continue;
				}
				updated[i] = updated_any = true;

				double tau = GetTimeDelta(tmt.tsDeq,tmtPrev.tsDeq);
				double duration = tau * 1e-9;
				double txRate = GetBytesDelta(tmt.txBytes, tmtPrev.txBytes)*8/duration; 
				double rxRate = txRate + (double(tmt.qlenDeq)-double(tmtPrev.qlenDeq))*8/duration;
				double u;
//

				double A=rxRate;
				if (A< 0.5*tmt.bandwidth && tmt.bandwidth==tmtPrev.bandwidth ){
					A = 0.5*tmt.bandwidth;
				}
				else
					A=rxRate;

				u = ( ( A ) * ( m_baseRtt.GetSeconds()*tmt.bandwidth + double(tmt.qlenDeq*8) ) ) / ( tmt.bandwidth*(tmt.bandwidth * m_baseRtt.GetSeconds()));

				if (u > U){
					U = u;
					dt = tau;
					if (tmt.bandwidth!=tmtPrev.bandwidth)
						uAggregate=U;
				}
			}
			IntPrev = ih;
			lastAckedSeq = ackNum;


			
			DataRate new_rate;
			int32_t new_incStage;
			DataRate new_rate_per_hop[ih.getMaxHops()];
			int32_t new_incStage_per_hop[ih.getMaxHops()];

			if (updated_any){
				if (dt > m_baseRtt.GetNanoSeconds())
					dt = m_baseRtt.GetNanoSeconds();


				uint32_t smaThresh = 10;


				uAggregate = (uAggregate * (m_baseRtt.GetNanoSeconds() - dt) + U * dt) / (double(m_baseRtt.GetNanoSeconds()));

					max_c = uAggregate / tcb->m_targetUtil;
					// std::cout << "U " << uAggregate << " time " << Simulator::Now().GetSeconds() << std::endl;

					if (max_c <=0.5 )
						new_rate = DataRate(wienRateAgg.GetBitRate()/max_c + tcb->wienAddInc.GetBitRate()) ;
					else
						new_rate =  DataRate( 0.9*(wienRateAgg.GetBitRate()/max_c + tcb->wienAddInc.GetBitRate()) + 0.1 *wienRateAgg.GetBitRate() ) ;
				
				if (new_rate < tcb->minWienRate)
					new_rate = tcb->minWienRate;
				if (new_rate > tcb->maxwienRate)
					new_rate = tcb->maxwienRate;
			}
			if (updated_any){
				tcb->m_setWienRateCallback(tcb,new_rate,tcb->wienRate, m_baseRtt, true);
			}
			if (!fast_react){
				if (updated_any){
					wienRateAgg = new_rate;
					incStageAgg = new_incStage;
				}
			}
		}
		if (!fast_react){
			if (next_seq > lastUpdatedSeq)
				lastUpdatedSeq = next_seq;
		}

	}
}



void TcpWien::FastReactWien(Ptr<Packet> packet, const TcpHeader& tcpHeader, Ptr<TcpSocketState> tcb, IntHeader ih){
	if (tcb->m_fast_react)
		UpdateRateWien(packet,tcpHeader, tcb,ih, true);
}


void TcpWien::UpdateRateThetaPowertcp(Ptr<Packet> packet, const TcpHeader& tcpHeader, Ptr<TcpSocketState> tcb, IntHeader ih, bool fast_react){
		uint32_t next_seq = tcb->m_nextTxSequence.Get().GetValue();
		uint32_t ackNum = tcpHeader.GetAckNumber().GetValue();

	if (lastUpdatedSeq == 0){
		lastUpdatedSeq = next_seq;
		lastAckedSeq = ackNum;
		double rtt = Simulator::Now().GetNanoSeconds() - ih.getPktTimestamp();
		lastRTT = rtt;
		lastReceivedTime = Simulator::Now().GetNanoSeconds();
	}else {
		
			double max_c = 0;

			bool updated_any = true;
			
			  uint64_t dt = Simulator::Now().GetNanoSeconds() - lastReceivedTime;
			  
			  lastReceivedTime = Simulator::Now().GetNanoSeconds();
				
				double rtt = Simulator::Now().GetNanoSeconds() - ih.getPktTimestamp();
				
				if (dt > m_baseRtt.GetNanoSeconds())
					dt = m_baseRtt.GetNanoSeconds();

				double A = double(rtt - lastRTT)/dt + 1;
				if (A<0.5)
					A=0.5;

				double U = double(rtt*(A))/(m_baseRtt.GetNanoSeconds());

			lastRTT = rtt;
			lastAckedSeq = ackNum;

			DataRate new_rate;

			if (updated_any){
				if (dt > m_baseRtt.GetNanoSeconds())
					dt = m_baseRtt.GetNanoSeconds();

				uAggregate = (uAggregate * (m_baseRtt.GetNanoSeconds() - dt) + U * dt) / (double(m_baseRtt.GetNanoSeconds()));

					max_c = uAggregate;
					// std::cout << "U " << uAggregate << " time " << Simulator::Now().GetSeconds() << std::endl;
				if (max_c < 0.5){
					new_rate = DataRate( 0.9*(wienRateAgg.GetBitRate()/max_c + tcb->wienAddInc.GetBitRate()) + 0.1*wienRateAgg.GetBitRate() ) ;
				}
				else{
					new_rate =  DataRate( 0.7*(wienRateAgg.GetBitRate()/max_c + tcb->wienAddInc.GetBitRate()) + 0.3 *wienRateAgg.GetBitRate() ) ;
				}
				
				if (new_rate < tcb->minWienRate)
					new_rate = tcb->minWienRate;
				if (new_rate > tcb->maxwienRate)
					new_rate = tcb->maxwienRate;
			}
			if (updated_any){
				tcb->m_setWienRateCallback(tcb,new_rate,tcb->wienRate, m_baseRtt, true);
			}
			if (!fast_react){
				if (updated_any){
					wienRateAgg = new_rate;
				}
			}

		if (!fast_react){
			if (next_seq > lastUpdatedSeq)
				lastUpdatedSeq = next_seq;
		}

	}
}



void TcpWien::FastReactThetaPowertcp(Ptr<Packet> packet, const TcpHeader& tcpHeader, Ptr<TcpSocketState> tcb, IntHeader ih){
	if (tcb->m_fast_react){
		// UpdateRateThetaPowertcp(packet,tcpHeader, tcb,ih, true);
	}
}


void TcpWien::UpdateRateHp(Ptr<Packet> packet, const TcpHeader& tcpHeader, Ptr<TcpSocketState> tcb, IntHeader ih, bool fast_react){

	uint32_t next_seq = tcb->m_nextTxSequence.Get().GetValue();


	if (lastUpdatedSeq == 0){
		lastUpdatedSeq = next_seq;

		NS_ASSERT(ih.getHopCount() <= ih.getMaxHops());
		IntPrev = ih;
	}else {

		if (ih.getHopCount()<= ih.getMaxHops()){
			double max_c = 0;
			// check each hop
			double U = 0;
			uint64_t dt = 0;
			bool updated[ih.getMaxHops()] = {false}, updated_any = false;
			NS_ASSERT(ih.getHopCount()<= ih.getMaxHops());

			for (uint32_t i = 0; i < ih.getHopCount(); i++){
				IntHeader::telemetry tmt = ih.getFeedback(i);
				IntHeader::telemetry tmtPrev = IntPrev.getFeedback(i);
				if (tcb->m_sampleFeedback){
					if (tmt.qlenDeq == 0 and fast_react)
						continue;
				}
				updated[i] = updated_any = true;

				double tau = GetTimeDelta(tmt.tsDeq,tmtPrev.tsDeq);
				double duration = tau * 1e-9;
				double txRate = GetBytesDelta(tmt.txBytes, tmtPrev.txBytes)*8/duration;

				double	u = txRate / tmt.bandwidth + (double)std::min(tmt.qlenDeq, tmtPrev.qlenDeq)*8.0/(tmt.bandwidth * m_baseRtt.GetSeconds() );


				if (!tcb->m_multipleRate){
					if (u > U){
						U = u;
						dt = tau;
					}
				}else {
					if (tau > m_baseRtt.GetNanoSeconds())
						tau = m_baseRtt.GetNanoSeconds();
					uOld[i] = (uOld[i]*(m_baseRtt.GetNanoSeconds()-tau) + u*tau)/double(m_baseRtt.GetNanoSeconds());
				}
			}
			IntPrev = ih;

			DataRate new_rate;
			int32_t new_incStage;
			DataRate new_rate_per_hop[ih.getMaxHops()];
			int32_t new_incStage_per_hop[ih.getMaxHops()];
			if (!tcb->m_multipleRate){
				if (updated_any){
					if (dt > m_baseRtt.GetNanoSeconds())
						dt = m_baseRtt.GetNanoSeconds();
					uAggregate = (uAggregate * (m_baseRtt.GetNanoSeconds() - dt) + U * dt) / double(m_baseRtt.GetNanoSeconds());
					max_c = uAggregate / tcb->m_targetUtil;
					if (max_c >= 1 || incStageAgg >= tcb->m_miThresh){
						new_rate = DataRate(wienRateAgg.GetBitRate() / max_c + tcb->wienAddInc.GetBitRate());
						new_incStage = 0;
					}else{
						new_rate = DataRate(wienRateAgg.GetBitRate() + tcb->wienAddInc.GetBitRate());
						new_incStage = incStageAgg + 1;
					}
					if (new_rate < tcb->minWienRate)
						new_rate = tcb->minWienRate;
					if (new_rate > tcb->maxwienRate)
						new_rate = tcb->maxwienRate;

				}
			}else{
				new_rate = tcb->maxwienRate;
				for (uint32_t i = 0; i < ih.getHopCount(); i++){
					if (updated[i]){
						double c = uOld[i]/tcb->m_targetUtil;
						if (c >= 1 || incStageHop[i] >= tcb->m_miThresh){
							new_rate_per_hop[i] = DataRate(wienRateHop[i].GetBitRate() / c + tcb->wienAddInc.GetBitRate());
							new_incStage_per_hop[i] = 0;
						}else{
							new_rate_per_hop[i] = DataRate(wienRateHop[i].GetBitRate() + tcb->wienAddInc.GetBitRate());
							new_incStage_per_hop[i] = incStageHop[i] + 1;
						}
						if (new_rate_per_hop[i] < tcb->minWienRate)
							new_rate_per_hop[i] = tcb->minWienRate;
						if (new_rate_per_hop[i] > tcb->maxwienRate)
							new_rate_per_hop[i] = tcb->maxwienRate;
						if (new_rate_per_hop[i] < new_rate)
							new_rate = new_rate_per_hop[i];

					}else{
						if (wienRateHop[i] < new_rate)
							new_rate = wienRateHop[i];
					}
				}

			}
			if (updated_any)
				tcb->m_setWienRateCallback(tcb,new_rate,tcb->wienRate, m_baseRtt, true);

			if (!fast_react){
				if (updated_any){
					wienRateAgg = new_rate;
					incStageAgg = new_incStage;
				}
				if (tcb->m_multipleRate){
					for (uint32_t i = 0; i < ih.getHopCount(); i++){
						if (updated[i]){
							wienRateHop[i] = new_rate_per_hop[i];
							incStageHop[i] = new_incStage_per_hop[i];
						}
					}
				}
			}
		}
		if (!fast_react){
			if (next_seq > lastUpdatedSeq)
				lastUpdatedSeq = next_seq;
		}
	}
}

void TcpWien::FastReactHp(Ptr<Packet> packet, const TcpHeader& tcpHeader, Ptr<TcpSocketState> tcb, IntHeader ih){
	if (tcb->m_fast_react)
		UpdateRateHp(packet,tcpHeader, tcb,ih, true);
}


/*Timely*/
// void TcpWien::HandleAckTimely(Ptr<Packet> packet, const TcpHeader& tcpHeader, Ptr<TcpSocketState> tcb, IntHeader ih, bool fast_react){
// 	uint32_t ack_seq = ch.ack.seq;
// 	// update rate
// 	if (ack_seq > qp->tmly.m_lastUpdateSeq){ // if full RTT feedback is ready, do full update
// 		UpdateRateTimely(qp, p, ch, false);
// 	}else{ // do fast react
// 		FastReactTimely(qp, p, ch);
// 	}
// }

void TcpWien::UpdateRateTimely(Ptr<Packet> packet, const TcpHeader& tcpHeader, Ptr<TcpSocketState> tcb, IntHeader ih, bool fast_react){
	uint32_t next_seq = tcb->m_nextTxSequence.Get().GetValue();

	uint64_t rtt = Simulator::Now().GetNanoSeconds() - ih.getPktTimestamp();
	if (lastUpdatedSeq != 0){
		int64_t new_rtt_diff = (int64_t)rtt - (int64_t)lastRTT;
		double rtt_diff = (1 - tcb->m_tmly_alpha) * lastRTTdiff + tcb->m_tmly_alpha * new_rtt_diff;
		double gradient = rtt_diff / m_baseRtt.GetNanoSeconds();
		bool inc = false;
		double c = 0;
		if (rtt < tcb->m_tmly_TLow){
			inc = true;
		}else if (rtt > tcb->m_tmly_THigh){
			c = 1 - tcb->m_tmly_beta * (1 - (double)tcb->m_tmly_THigh / rtt);
			inc = false;
		}else if (gradient <= 0){
			inc = true;
		}else{
			c = 1 - tcb->m_tmly_beta * gradient;
			if (c < 0)
				c = 0;
			inc = false;
		}
		DataRate new_rate;
		if (inc){
			if (incStageAgg < 5){
				new_rate = wienRateAgg + tcb->wienAddInc;
			}else{
				new_rate = wienRateAgg + tcb->wienAddIncHigh;
			}
			if (new_rate < tcb->minWienRate)
						new_rate = tcb->minWienRate;
			if (new_rate > tcb->maxwienRate)
				new_rate = tcb->maxwienRate;

			if (!fast_react){
				wienRateAgg = new_rate;
				incStageAgg++;
				lastRTTdiff = rtt_diff;
				tcb->m_setWienRateCallback(tcb,new_rate,tcb->wienRate, m_baseRtt, false);
			}
		}
		else{
			new_rate = std::max(tcb->minWienRate, wienRateAgg * c); 
			if (!fast_react){
				wienRateAgg = new_rate;
				incStageAgg = 0;
				lastRTTdiff = rtt_diff;
				tcb->m_setWienRateCallback(tcb,new_rate,tcb->wienRate, m_baseRtt,false);
			}
		}
	}
	if (!fast_react && next_seq > lastUpdatedSeq){
		lastUpdatedSeq = next_seq;
		lastRTT = rtt;
	}
}

void TcpWien::FastReactTimely(Ptr<Packet> packet, const TcpHeader& tcpHeader, Ptr<TcpSocketState> tcb, IntHeader ih){
}




void
TcpWien::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
}

void
TcpWien::ReduceCwnd (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
//  tcb->m_cWnd = std::max (tcb->m_cWnd.Get () / 2, tcb->m_segmentSize);

}



std::string
TcpWien::GetName () const
{
  return "TcpWien";
}

uint32_t
TcpWien::GetSsThresh (Ptr<const TcpSocketState> tcb,
                       uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << tcb << bytesInFlight);
  return UINT32_MAX;
//  return std::max (std::min (tcb->m_ssThresh.Get (), tcb->m_cWnd.Get () - tcb->m_segmentSize), 2 * tcb->m_segmentSize);
}

} // namespace ns3
