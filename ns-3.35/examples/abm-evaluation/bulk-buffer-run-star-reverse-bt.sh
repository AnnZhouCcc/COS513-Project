
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

# BTMODE
BURSTWITHCONT=0
BURSTONLY=1
CONTONLY=2

SERVER_LEAF_CAP=10
LEAF_SINK_CAP=1
SERVER_LEAF_LATENCY=10000
LEAF_SINK_LATENCY=10000

RED_MIN=10
RED_MAX=10

N_PRIO=3 # Changed this

ALPHAFILE="$DIR/alphas"
CDFFILE="$DIR/websearch.txt"
CDFNAME="WS"

NUMSINKS=2
NUMNODES=20

TCP=$CUBIC
ALG=$DT

CONTA=1
BURSTA=8

NUMCONTFLOWS=10
NUMBURSTFLOWS=10

BTMODE=$CONTONLY

BURSTIW=500
CONTIW=4

BURSTSTARTMS=0

START_TIME=2
END_TIME=10
FLOW_END_TIME=12

for B in {5..9}; do
    BUFFER=`expr $B \* 1000000` 
    echo $BUFFER

    FLOWFILE="$DUMP_DIR/fcts-single-$TCP-$ALG-$BUFFER.fct"
    TORFILE="$DUMP_DIR/tor-single-$TCP-$ALG-$BUFFER.stat"
    PARAMFILE="$DUMP_DIR/param-single-$TCP-$ALG-$BUFFER.txt"
    ./waf --run "star-reverse-bt --StartTime=$START_TIME --EndTime=$END_TIME --FlowLaunchEndTime=$FLOW_END_TIME --numSinks=$NUMSINKS --numNodes=$NUMNODES --leafSinkCapacity=$LEAF_SINK_CAP --serverLeafCapacity=$SERVER_LEAF_CAP --leafSinkLinkLatency=$LEAF_SINK_LATENCY --serverLeafLinkLatency=$SERVER_LEAF_LATENCY --TcpProt=$TCP --BufferSize=$BUFFER --algorithm=$ALG --RedMinTh=$RED_MIN --RedMaxTh=$RED_MAX --nPrior=$N_PRIO --alphasFile=$ALPHAFILE --cdfFileName=$CDFFILE --torOutFile=$TORFILE --fctOutFile=$FLOWFILE --paramOutFile=$PARAMFILE --continuousAlpha=$CONTA --burstyAlpha=$BURSTA --numContinuousFlows=$NUMCONTFLOWS --numBurstyFlows=$NUMBURSTFLOWS --btMode=$BTMODE --continuousInitialWindow=$CONTIW --burstyInitialWindow=$BURSTIW --burstyStartTime=$BURSTSTARTMS"
done
