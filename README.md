Prerequisites
=============

## Installing BRITE

    hg clone http://code.nsnam.org/BRITE
    cd BRITE
    make
    sudo cp libbrite.so /usr/lib/
    sudo cp *.h /usr/local/include/ns3-dev/ns3

    sudo mkdir /usr/local/include/ns3-dev/ns3/Models
    cd Models/
    sudo cp *.h /usr/local/include/ns3-dev/ns3/Models

## Installing ndnSIM with BRITE

    cd ns-3
    ./waf configure --with-brite=../BRITE
    ./waf

## Running the demo scenario with generated topology

    git clone git@github.com:phylib/brite-scenario.git brite-scenario
    cd brite-scenario
    ./waf configure --logging --debug
    ./waf
    ./waf --run=brite-topo --vis