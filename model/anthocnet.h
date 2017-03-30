/*
 * Copyright (c) 2016 Leon Tan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef ANTHOCNET_H
#define ANTHOCNET_H


#include "anthocnet-config.h"
#include "anthocnet-rtable.h"
#include "anthocnet-packet.h"
#include "anthocnet-pcache.h"

#include <list>

#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/arp-cache.h"
#include "ns3/traced-callback.h"
#include "ns3/wifi-phy.h"


#include "ns3/object-ptr-container.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/pointer.h"


// TODO: Make these settable by GetTypeId()
//       instead of hardcoding them
#define MAX_INTERFACES 30

namespace ns3 {
namespace ahn {
  
class RoutingProtocol : public Ipv4RoutingProtocol {
public:
  
  
  //ctor
  RoutingProtocol();
  //dtor
  ~RoutingProtocol();
  
  static TypeId GetTypeId (void);
  static const uint32_t AODV_PORT;
  
  // Inherited from Ipv4RoutingProtocol
  // Look up their documentation in ns3 reference manual
  Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                   LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  virtual void DoDispose();
  
  // Added for initialization
  void SetConfig(Ptr<AntHocNetConfig> config);
  Ptr<AntHocNetConfig> GetConfig() const;
  
protected:
    virtual void DoInitialize();
  
private:
  
  // All the utiliy and callback functions of the protocol go here
  // Sets up the operation of the protocol
  void Start();
  
  Ptr<Socket> FindSocketWithInterfaceAddress
    (Ipv4InterfaceAddress addr) const;
  
  Ptr<Ipv4Route> LoopbackRoute(const Ipv4Header& hdr, 
    Ptr<NetDevice> oif) const;
  
  /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model.  Return the number of streams (possibly zero) that
  * have been assigned.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);
    
  /**
   * \brief Finds the index of the Socket, used in the rtable.
   * \param socket The pointer to the sockeT;
   * \returns The index into the socket table.
   */  
  uint32_t FindSocketIndex(Ptr<Socket>)const;
  
  TracedCallback<Ptr<Packet const>, std::string, Ipv4Address> ant_drop;
  TracedCallback<Ptr<Packet const>, std::string, Ipv4Address> data_drop;
  
  void StartForwardAnt(Ipv4Address dst, bool is_proactive);
  
  void UnicastForwardAnt(uint32_t iface, Ipv4Address dst, ForwardAntHeader ant,
                         bool is_proactive);
  
  void BroadcastForwardAnt(Ipv4Address dst, bool is_proactive);
  void BroadcastForwardAnt(Ipv4Address dst, ForwardAntHeader ant,\
                           bool is_proactive);
  
  void UnicastBackwardAnt(uint32_t iface, Ipv4Address dst, 
                          BackwardAntHeader ant);
  
  void SendCachedData(Ipv4Address dst);
  
  // Add ARP cache to be used to allow layer 2 notifications processing
  void AddArpCache (Ptr<ArpCache>);
  // Don't use given ARP cache any more (interface is down)
  void DelArpCache (Ptr<ArpCache>);
  
  // Search all interfaces arpcaches for that mac address
  std::vector<Ipv4Address> LookupMacAddress (Mac48Address addr);
  
  // Holds pointers to all arp caches, such that it can
  // look up IP addresses from MACs. Needed for L2 support
  std::vector<Ptr<ArpCache> > arp_cache;
  
  // ----------------------------------------------
  // Called when there is an error in the Layer 2 link
  void ProcessTxError (WifiMacHeader const& header);
  
  // Used by RouteInput, is the other side to ProcessRxTrace
  void UpdateAvrTMac();
  
  void ProcessMonitorSnifferRx(Ptr<Packet const> packet, 
                              uint16_t frequency, uint16_t channel, 
                              uint32_t rate, WifiPreamble isShortPreable,
                              WifiTxVector tx_vector, mpduInfo mpdu,
                              signalNoiseDbm snr);
  
  // ----------------------------------------------
  // Callback function for receiving a packet
  void Recv(Ptr<Socket> socket);
  
  // Callback to do a deferred send
  void Send(Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address dst);
  void SendDirect(Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address dst);
  
  // Send a HelloAnt every other second
  void HelloTimerExpire();
  
  // Send sampler ants to all destinations having active sessions
  void PrAntTimerExpire();
  
  // Update the RoutingTable (mainly throw out old data)
  void RTableTimerExpire();
  
  void NBExpire(Ipv4Address nb);
  
  // -------------------------------------------------
  // Ant Handlers
  
  // Handles receiving of a HelloAnt
  void HandleHelloMsg(Ptr<Packet> packet, uint32_t iface);
  
  // Handles receiving of a LinkFailure Message
  void HandleLinkFailure(Ptr<Packet> packet, Ipv4Address src, 
                         uint32_t iface);
  
  // Handles receiving of a ForwardAnt
  void HandleForwardAnt(Ptr<Packet> packet, uint32_t iface, bool is_proactive);
  
  // Handles receiving of a BackwardAnt
  void HandleBackwardAnt(Ptr<Packet> packet,Ipv4Address orig_src, uint32_t iface);
  
  //-----------------------------------------------
  // All the network config stuff go here 
  Ptr<AntHocNetConfig> config;
  
  // -------------------------------------------------
  // Timers and their intervals
  
  Timer hello_timer;
  Timer pr_ant_timer;
  
  Ptr<UniformRandomVariable> uniform_random;
  
  //----------------------------------------------
  // All the global state of the protocol go here
  
  // Last time the Hello Timer expired
  Time last_hello;
  
  // The last measured snr
  double last_snr; 
  
  // Holds the loopback device
  Ptr<NetDevice> lo;
  
  // The routing table
  RoutingTable rtable;
  
  // The Cache for the data that has no route yet
  PacketCache data_cache;
  
  // The IP protocol
  Ptr<Ipv4> ipv4;
  
  // Holds information about the interfaces
  Ptr<Socket> sockets[MAX_INTERFACES];
  
  std::map< Ptr<Socket>, Ipv4InterfaceAddress> socket_addresses;
  
};

// End of namespace
}
}

#endif /* ANTHOCNET_H */

