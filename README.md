# ns3 buffer 

This repository provides an ns-3 implementation of several buffer management and congestion control algorithms.

# Build

```bash
cd ./ns-3.35
./waf clean
./waf distclean
CXXFLAGS=-Wno-error ./waf configure --build-profile=optimized --enable-examples --disable-tests --disable-python
./waf
````
