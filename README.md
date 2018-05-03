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

## Running the scenario on a fresh Ubuntu installation

    # Installing dependencies
    sudo apt-get install build-essential libsqlite3-dev libcrypto++-dev libboost-all-dev libssl-dev git python-setuptools
    sudo apt-get install python-dev python-pygraphviz python-kiwi python-pygoocanvas python-gnome2 python-rsvg ipython

    # Installing ndnSIM
    mkdir ndnSIM_2.3
    cd ndnSIM_2.3/
    git clone https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
    git clone https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
    git clone --recursive https://github.com/named-data-ndnSIM/ndnSIM.git ns-3/src/ndnSIM


    # BRITE intermezzo
    sudo apt-get install mercurial
    hg clone http://code.nsnam.org/BRITE
    cd BRITE
    make
    sudo cp libbrite.so /usr/lib/

    # /usr/local/include/ns3-dev not existing
    # created /usr/local/include/ns3-dev/

    sudo cp *.h /usr/local/include/ns3-dev/ns3


    sudo mkdir /usr/local/include/ns3-dev/ns3/Models
    cd Models/
    sudo cp *.h /usr/local/include/ns3-dev/ns3/Models

    # installing ndnSIM with BRITE
    # cd ../..
    cd ns-3
    ./waf configure --with-brite=../BRITE
    #Checking BRITE location   : ../BRITE (given)
    #BRITE Integration: enabled
    ./waf
    sudo ./waf install


    # running the scenario
    git clone https://github.com/phylib/brite-scenario.git brite-scenario
    cd brite-scenario
    git checkout without-queuing
    ./waf configure --logging --debug
    ./waf

    # build/brite-topo: error while loading shared libraries: libns3-dev-core-debug.so: cannot open shared object file: No such file or directory
    sudo ldconfig
    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
    export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

    ./waf --run=brite-topo --vis
