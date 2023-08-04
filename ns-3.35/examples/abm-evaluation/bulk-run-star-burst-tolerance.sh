NS3="/u/az6922/COS513-Project/ns-3.35"
DIR="$NS3/examples/abm-evaluation"
DUMP_DIR="/u/az6922/data"

# TCP
DT=101
FAB=102
CS=103
IB=104
ABM=110

# ALG
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

cd $NS3

SERVER_LEAF_CAP=10
LEAF_SINK_CAP=1
SERVER_LEAF_LATENCY=10000
LEAF_SINK_LATENCY=10000

N_PRIO=3 # Changed this

ALPHAFILE="$DIR/alphas"
CDFFILE="$DIR/websearch.txt"
CDFNAME="WS"

NUMSINKS=2
NUMNODES=20

BUFFER=40000000

START_TIME=2
END_TIME=12
FLOW_END_TIME=10

TCP=$CUBIC
ALG=$DT

CONTA=8
BURSTA=1

NUMCONTFLOWS=10
NUMBURSTFLOWS=10

CONTIW=4


BTMODE=$BURSTWITHCONT

for SEED in {1..10}; do
    for BURSTIW in 5 50 100 500 1000; do
        for BURSTSTARTMS in 0 4500; do
            for BTMODE in $BURSTWITHCONT $BURSTONLY; do
                echo $SEED $BURSTIW $BURSTSTARTMS $BTMODE
                FLOWFILE="$DUMP_DIR/star-burst-tolerance-flow-$BURSTIW-$BURSTSTARTMS-$BTMODE-$SEED.data"
                TORFILE="$DUMP_DIR/star-burst-tolerance-buffer-$BURSTIW-$BURSTSTARTMS-$BTMODE-$SEED.data"
                PARAMFILE="$DUMP_DIR/star-burst-tolerance-parameters-$BURSTIW-$BURSTSTARTMS-$BTMODE-$SEED.data"
                ./waf --run "star-burst-tolerance --randomSeed=$SEED --StartTime=$START_TIME --EndTime=$END_TIME --FlowLaunchEndTime=$FLOW_END_TIME --numSinks=$NUMSINKS --numNodes=$NUMNODES --leafSinkCapacity=$LEAF_SINK_CAP --serverLeafCapacity=$SERVER_LEAF_CAP --leafSinkLinkLatency=$LEAF_SINK_LATENCY --serverLeafLinkLatency=$SERVER_LEAF_LATENCY --TcpProt=$TCP --BufferSize=$BUFFER --algorithm=$ALG --nPrior=$N_PRIO --alphasFile=$ALPHAFILE --cdfFileName=$CDFFILE --torOutFile=$TORFILE --fctOutFile=$FLOWFILE --paramOutFile=$PARAMFILE --continuousAlpha=$CONTA --burstyAlpha=$BURSTA --numContinuousFlows=$NUMCONTFLOWS --numBurstyFlows=$NUMBURSTFLOWS --btMode=$BTMODE --continuousInitialWindow=$CONTIW --burstyInitialWindow=$BURSTIW --burstyStartTime=$BURSTSTARTMS"
            done
        done
    done
done
