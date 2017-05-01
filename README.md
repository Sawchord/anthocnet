This is the codebase for my BachelorThesis.

To run this code, a recent version of ns3 is required.
To Install ns3, please follow the instructions on:
https://www.nsnam.org/wiki/Installation

To install the module, clone this repo into your ns-3-dev/src folder, then rerun 
./waf configure [--enable-tests] [--enable-examples] [--disable-python]

The module requires fuzzylite 6.
To install fuzzylite run 
git submodule update --init
Then run "build.sh all" inside src/anthocnet/fuzzylite/fuzzylite.

Build the module with
./waf build

To run the simulations, you must have the examples activated in the configure part, then you can run it via
./waf --run "anthocnet-sim"

Run 
./waf --run "anthocnet-sim --PrintHelp"
to get an overview over the settable parameters.

To activate the logging output of the main anthocnet.cc file use
export 'NS_LOG=AntHocNetRoutingProtocol=level_all|prefix_func|prefix_time:AodvRoutingProtocol=level_all|prefix_func|prefix_time'

or
export 'NS_LOG=AntHocNetRoutingProtocol=level_all|prefix_func|prefix_time:AntHocNetRoutingTable=level_all|prefix_func|prefix_time:AodvRoutingProtocol=level_all|prefix_func|prefix_time'

export 'NS_LOG=AntHocNetPCache=level_all|prefix_func|prefix_time:SimDatabase=level_all|prefix_func|prefix_time:AntHocNetRoutingProtocol=level_all|prefix_func|prefix_time:AntHocNetRoutingTable=level_all|prefix_func|prefix_time'

To activate GDB:
./waf --run anthocnet-compare --command-template="gdb %s"

To activate callgrind:
./waf --run anthocnet-compare --command-template="valgrind --tool=callgrind %s"
./waf --run anthocnet-compare --command-template="valgrind --tool=callgrind --time-stamp=yes %s"