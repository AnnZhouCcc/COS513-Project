
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

BUFFER=10000000

START_TIME=2
END_TIME=12
FLOW_END_TIME=10

TCP=$CUBIC
ALG=$DT
version=0

CONTA=1
BURSTA=8

NUMCONTFLOWS=10
NUMBURSTFLOWS=10

BTMODE=$CONTONLY

BURSTIW=500
CONTIW=4

BURSTSTARTMS=0
CONTSTARTMS=0

FLOWFILE="$DUMP_DIR/fcts-single-$TCP-$ALG-$version.fct"
TORFILE="$DUMP_DIR/tor-single-$TCP-$ALG-$version.stat"
PARAMFILE="$DUMP_DIR/param-single-$TCP-$ALG-$version.txt"
./waf --run "star-reverse-bt --StartTime=$START_TIME --EndTime=$END_TIME --FlowLaunchEndTime=$FLOW_END_TIME --numSinks=$NUMSINKS --numNodes=$NUMNODES --leafSinkCapacity=$LEAF_SINK_CAP --serverLeafCapacity=$SERVER_LEAF_CAP --leafSinkLinkLatency=$LEAF_SINK_LATENCY --serverLeafLinkLatency=$SERVER_LEAF_LATENCY --TcpProt=$TCP --BufferSize=$BUFFER --algorithm=$ALG --RedMinTh=$RED_MIN --RedMaxTh=$RED_MAX --nPrior=$N_PRIO --alphasFile=$ALPHAFILE --cdfFileName=$CDFFILE --torOutFile=$TORFILE --fctOutFile=$FLOWFILE --paramOutFile=$PARAMFILE --continuousAlpha=$CONTA --burstyAlpha=$BURSTA --numContinuousFlows=$NUMCONTFLOWS --numBurstyFlows=$NUMBURSTFLOWS --btMode=$BTMODE --continuousInitialWindow=$CONTIW --burstyInitialWindow=$BURSTIW --burstyStartTime=$BURSTSTARTMS --continuousStartTime=$CONTSTARTMS"
