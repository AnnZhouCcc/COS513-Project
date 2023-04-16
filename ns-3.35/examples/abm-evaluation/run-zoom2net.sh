
NS3="/nfs/ns3-buffer-stats/ns-3.35"
DIR="$NS3/examples/abm-evaluation"
DUMP_DIR="/nfs/test_buf/"

DT=101
FAB=102
CS=103
IB=104
ABM=110

RENO=0
CUBIC=1
DCTCP=2
HPCC=3
POWERTCP=4
HOMA=5
TIMELY=6
THETAPOWERTCP=7

BUF_ALGS=($DT $FAB $CS $IB $ABM)
TCP_ALGS=($CUBIC $DCTCP $TIMELY $POWERTCP)

SERVERS=32
LEAVES=2
SPINES=2
LINKS=4
SERVER_LEAF_CAP=1
LEAF_SPINE_CAP=1
LATENCY=10

RED_MIN=65
RED_MAX=65

N_PRIO=2 # Changed this

ALPHAFILE="$DIR/alphas"
CDFFILE="$DIR/websearch.txt"
CDFNAME="WS"

ALPHA_UPDATE_INT=1 # 1 RTT


STATIC_BUFFER=0
# BUFFER=$(( 1000*1000*9  ))
BUFFER_PER_PORT_PER_GBPS=0.8 #9.6 # https://baiwei0427.github.io/papers/bcc-ton.pdf (Trident 2)
BUFFER=$(python3 -c "print(int($BUFFER_PER_PORT_PER_GBPS*1024*($SERVERS+$LINKS*$SPINES)*$SERVER_LEAF_CAP))")

TCP=$CUBIC

START_TIME=10
END_TIME=24
FLOW_END_TIME=20


cd $NS3


N=0

# 20 exps
BURST_SIZES=0.9
BURST_SIZE=$(python3 -c "print($BURST_SIZES*$BUFFER)")
BURST_FREQ=3
for ALG in $DT ;do
	for LOAD in 0.9 ;do
		FLOW_END_TIME=20 #$(python3 -c "print(10+3*0.8/$LOAD)")
		FLOWFILE="$DUMP_DIR/fcts-single-$TCP-$ALG-$LOAD-$BURST_SIZES-$BURST_FREQ.fct"
		TORFILE="$DUMP_DIR/tor-single-$TCP-$ALG-$LOAD-$BURST_SIZES-$BURST_FREQ.stat"
		./waf --run "evaluation --load=$LOAD --StartTime=$START_TIME --EndTime=$END_TIME --FlowLaunchEndTime=$FLOW_END_TIME --serverCount=$SERVERS --spineCount=$SPINES --leafCount=$LEAVES --linkCount=$LINKS --spineLeafCapacity=$LEAF_SPINE_CAP --leafServerCapacity=$SERVER_LEAF_CAP --linkLatency=$LATENCY --TcpProt=$TCP --BufferSize=$BUFFER --statBuf=$STATIC_BUFFER --algorithm=$ALG --RedMinTh=$RED_MIN --RedMaxTh=$RED_MAX --request=$BURST_SIZE --queryRequestRate=$BURST_FREQ --nPrior=$N_PRIO --alphasFile=$ALPHAFILE --cdfFileName=$CDFFILE --alphaUpdateInterval=$ALPHA_UPDATE_INT --fctOutFile=$FLOWFILE --torOutFile=$TORFILE"
	done
done

