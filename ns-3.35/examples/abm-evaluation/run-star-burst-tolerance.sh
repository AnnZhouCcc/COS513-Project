
NS3="/u/az6922/COS513-Project/ns-3.35"
DIR="$NS3/examples/abm-evaluation"
DUMP_DIR="/u/az6922/data"

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

# BUF_ALGS=($DT $FAB $CS $IB $ABM)
# TCP_ALGS=($CUBIC $DCTCP $TIMELY $POWERTCP)

# SERVERS=32
# LEAVES=2
# SPINES=2
# LINKS=4
# SERVER_LEAF_CAP=1
# LEAF_SPINE_CAP=1
# LATENCY=10

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

# ALPHA_UPDATE_INT=1 # 1 RTT

NUMSINKS=2
NUMNODES=20

# STATIC_BUFFER=0
# BUFFER=$(( 1000*1000*9  ))
#BUFFER_PER_PORT_PER_GBPS=0.5 #9.6 # https://baiwei0427.github.io/papers/bcc-ton.pdf (Trident 2)
# BUFFER=$(python3 -c "print(int($BUFFER_PER_PORT_PER_GBPS*1024*($SERVERS+$LINKS*$SPINES)*$SERVER_LEAF_CAP))")
# BUFFER=$(python3 -c "print(int($BUFFER_PER_PORT_PER_GBPS*1024*($NUMNODES*$SERVER_LEAF_CAP+$NUMSINKS*$LEAF_SINK_CAP)))")
BUFFER=40000000

START_TIME=2
END_TIME=12
FLOW_END_TIME=10


cd $NS3


# 20 exps
# BURST_SIZES=0.9
# BURST_SIZE=$(python3 -c "print($BURST_SIZES*$BUFFER)")
# BURST_FREQ=3

TCP=$CUBIC
ALG=$DT
version=30

CONTA=1
BURSTA=1

NUMCONTFLOWS=10
NUMBURSTFLOWS=10

# BTMODE
BURSTWITHCONT=0
BURSTONLY=1
BTMODE=$BURSTWITHCONT

BURSTIW=4
CONTIW=4

BURSTSTARTMS=0

# for LOAD in 0.9 ;do
# 	FLOWFILE="$DUMP_DIR/fcts-single-$TCP-$ALG-$LOAD-$BURST_SIZES-$BURST_FREQ.fct"
# 	TORFILE="$DUMP_DIR/tor-single-$TCP-$ALG-$LOAD-$BURST_SIZES-$BURST_FREQ.stat"
# 	./waf --run "simple-noincast-continuous --load=$LOAD --StartTime=$START_TIME --EndTime=$END_TIME --FlowLaunchEndTime=$FLOW_END_TIME --serverCount=$SERVERS --spineCount=$SPINES --leafCount=$LEAVES --linkCount=$LINKS --spineLeafCapacity=$LEAF_SPINE_CAP --leafServerCapacity=$SERVER_LEAF_CAP --linkLatency=$LATENCY --TcpProt=$TCP --BufferSize=$BUFFER --statBuf=$STATIC_BUFFER --algorithm=$ALG --RedMinTh=$RED_MIN --RedMaxTh=$RED_MAX --request=$BURST_SIZE --queryRequestRate=$BURST_FREQ --nPrior=$N_PRIO --alphasFile=$ALPHAFILE --cdfFileName=$CDFFILE --alphaUpdateInterval=$ALPHA_UPDATE_INT --fctOutFile=$FLOWFILE --torOutFile=$TORFILE"
# done

FLOWFILE="$DUMP_DIR/fcts-single-$TCP-$ALG-$version.fct"
TORFILE="$DUMP_DIR/tor-single-$TCP-$ALG-$version.stat"
./waf --run "star-burst-tolerance --StartTime=$START_TIME --EndTime=$END_TIME --FlowLaunchEndTime=$FLOW_END_TIME --numSinks=$NUMSINKS --numNodes=$NUMNODES --leafSinkCapacity=$LEAF_SINK_CAP --serverLeafCapacity=$SERVER_LEAF_CAP --leafSinkLinkLatency=$LEAF_SINK_LATENCY --serverLeafLinkLatency=$SERVER_LEAF_LATENCY --TcpProt=$TCP --BufferSize=$BUFFER --algorithm=$ALG --RedMinTh=$RED_MIN --RedMaxTh=$RED_MAX --nPrior=$N_PRIO --alphasFile=$ALPHAFILE --cdfFileName=$CDFFILE --torOutFile=$TORFILE --fctOutFile=$FLOWFILE --continuousAlpha=$CONTA --burstyAlpha=$BURSTA --numContinuousFlows=$NUMCONTFLOWS --numBurstyFlows=$NUMBURSTFLOWS --btMode=$BTMODE --continuousInitialWindow=$CONTIW --burstyInitialWindow=$BURSTIW --burstyStartTime=$BURSTSTARTMS"
