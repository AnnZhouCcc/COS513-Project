| Experiment | Flow | Alpha | Link Latency(ms) | Bandwidth(Gbps) | WTS |
| ----------- | ----------- | ----------- | ----------- | ----------- | ----------- |
| burst-tolerance | 10 sync continuous, 10 bursty | 8,1 | 10,10;10,10 | 10:1 | FCT(b)&uarr; |
| reverse-bt | 100 desync continuous, burst (50 bursty) | 1,8 | 10,10;10,10 | 10:1 | throughput(c)&darr; |
| hetero-rtt-cc | 10 long, 10 short | 1,1 | 1,1;20,2 | 10:1 | throughput(l)&darr;&darr; > throughput(s)&darr; |