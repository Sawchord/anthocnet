This is the codebase for my BachelorThesis.
Some more in detail descritpion will follow soon.


To install the module, clone this repo into your ns-3-dev/src folder, then rerun 
./waf configure [--enable-tests] [--enable-examples] [--disable-python]

The rebuild with
./waf build

To run the examples, you must have the examples activated in the configure part, then you can run it via
./waf --run anthocnet-complare

The module comes with a hacked version of manet-routing-compare

To activate the logging output of the main anthocnet.cc file use
export 'NS_LOG=AntHocNetRoutingProtocol=level_all|prefix_func|prefix_time:AodvRoutingProtocol=level_all|prefix_func|prefix_time'

or
export 'NS_LOG=AntHocNetRoutingProtocol=level_all|prefix_func|prefix_time:AntHocNetRoutingTable=level_all|prefix_func|prefix_time:AodvRoutingProtocol=level_all|prefix_func|prefix_time'

To activate insight:
./waf --run anthocnet-compare --command-template="insight %s"

To activate callgrind:
./waf --run anthocnet-compare --command-template="valgrind --tool=callgrind %s"