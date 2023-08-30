
NS3="/u/az6922/COS513-Project/ns-3.35"
DIR="$NS3/examples/abm-evaluation"
DUMP_DIR="/u/az6922/data"

cd $NS3

DT=101
FAB=102
CS=103
IB=104
ABM=110

RENO=0
CUBIC=1
DCTCP=2
# HPCC=3
# POWERTCP=4
# HOMA=5
# TIMELY=6
# THETAPOWERTCP=7

# FLOWSIZEMODE
CONTINUOUS=0
BURSTY=1
NONE=2

SERVER_LEAF_CAP=10
LEAF_SINK_CAP=1
SERVER_LEAF_LATENCY=1000
LEAF_SINK_LATENCY_LONG_RTT=20000
LEAF_SINK_LATENCY_SHORT_RTT=2000

RED_MIN=10
RED_MAX=10

N_PRIO=3 # Changed this

ALPHAFILE="$DIR/alphas"
CDFFILE="$DIR/websearch.txt"
CDFNAME="WS"

NUMSINKS=2
NUMNODES=20

BUFFER=2500000

START_TIME=1
END_TIME=25
FLOW_END_TIME=3

TCP=$CUBIC
ALG=$DT
# version=0

LONGA=1
SHORTA=1

NUMLONGFLOWS=10
NUMSHORTFLOWS=10

FSMODELONGRTT=$CONTINUOUS
FSMODESHORTRTT=$CONTINUOUS

SHORTIW=4
LONGIW=4

LONGSTARTMS=1000
SHORTSTARTMS=1000

for seed in {1..5}; do
    FLOWFILE="$DUMP_DIR/fcts-hetero-rtt-$TCP-$ALG-$seed.fct"
    TORFILE="$DUMP_DIR/tor-hetero-rtt-$TCP-$ALG-$seed.stat"
    PARAMFILE="$DUMP_DIR/param-hetero-rtt-$TCP-$ALG-$seed.txt"
    time ./waf --run "star-hetero-rtt --randomSeed=$seed --StartTime=$START_TIME --EndTime=$END_TIME --FlowLaunchEndTime=$FLOW_END_TIME --numSinks=$NUMSINKS --numNodes=$NUMNODES --leafSinkCapacity=$LEAF_SINK_CAP --serverLeafCapacity=$SERVER_LEAF_CAP --leafSinkLinkLatencyLongRTT=$LEAF_SINK_LATENCY_LONG_RTT --leafSinkLinkLatencyShortRTT=$LEAF_SINK_LATENCY_SHORT_RTT --serverLeafLinkLatency=$SERVER_LEAF_LATENCY --TcpProt=$TCP --BufferSize=$BUFFER --algorithm=$ALG --RedMinTh=$RED_MIN --RedMaxTh=$RED_MAX --nPrior=$N_PRIO --alphasFile=$ALPHAFILE --cdfFileName=$CDFFILE --torOutFile=$TORFILE --fctOutFile=$FLOWFILE --paramOutFile=$PARAMFILE --longRTTAlpha=$LONGA --shortRTTAlpha=$SHORTA --numLongRTTFlows=$NUMLONGFLOWS --numShortRTTFlows=$NUMSHORTFLOWS --fsModeLongRTT=$FSMODELONGRTT --fsModeShortRTT=$FSMODESHORTRTT --longRTTInitialWindow=$LONGIW --shortRTTInitialWindow=$SHORTIW --longRTTStartTime=$LONGSTARTMS --shortRTTStartTime=$SHORTSTARTMS" &
    sleep 20
done
wait