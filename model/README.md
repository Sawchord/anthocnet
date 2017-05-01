This is the model directory of this module.
The actual protocols logic is implemented here.
Description of the files:
anthocnet-packet.h
anthocnet-packet.cc
    These files implement all the packet types, that are used inside the protocol.
    It consists of a TypeHeader, which is treated as a seperate header and is just a single byte enumerate to indicate which type of ant it wraps.
    Then it consists of the AntHeader type, which is the genereic super class used by all other ants. 
    The ForwardAntHeader and BackwardAntHeader are derived from AntHeader, they share all the same members and serialization/deserialization but
    member access is handled differently, due to the respective logic of the forward and backward ant.
anthocnet-pcache.h
anthocnet-pcache.cc
    The AntHocNet protocol requires the capabillity to store packets for a later use.
    In this implementation, this is done via a packet cache mechanism, that saves the packets on a per destination basis.
    Thus, it is possible to cache packets, if the route to the destination is unknown.
    Once, this is known, it can send out all the data at once.
anthocnet-rtable.h
anthocnet-rtable.cc
    These files implement the actual routing table data structure.
    It can hold a map of all neighbors. A neighbor is a tuple of an address and an interface over which to reach an address.
    This is due to the fact, that a single neighboring node could be reached via multiple interfaces.
    Since these interfaces could have different capabillities and congestion states, it is necessary to threat them as two different
    possible routes to the destiation.
anthocnet.h
anthocnet.cc
    These are the main files implementing most of the packet handling logic.
    This consist of handling the interface and address changes, queueing and handling received packet, the Hello mechanism,
    routing forward and backward ants and answering the routing request.
anthocnet-fis.h
anthocnet-fis.cc
    Some glue code to integrate fuzzylite with this project.
    This could also be done directly in anthocnet-rtable, but it was decided to be kept separated.
anthocnet-stat.h
anthocnet-stat.cc
    The fuzzy anthocnets traffic monitor is implemented in these files.
anthocnet-config.h
anthocnet-config.cc
    Since lots of configuration parameters are needed by different parts of the program, the configuration was separated from the rest of the protocol.
    All important configuration is done via this module, and the other parts just keep a pointer to this object around.