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

#define NS_LOG_APPEND_CONTEXT                                   \
  if (ipv4) { std::clog << "[node " << std::setfill('0') << std::setw(2) \
    << ipv4->GetObject<Node> ()->GetId () +1 << "] "; }

#include "anthocnet.h"


namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("AntHocNetRoutingProtocol");
namespace ahn {

//ctor
RoutingProtocol::RoutingProtocol ():
  hello_timer(Timer::CANCEL_ON_DESTROY),
  rtable_update_timer(Timer::CANCEL_ON_DESTROY),
  pr_ant_timer(Timer::CANCEL_ON_DESTROY),
  
  // These are no configs but rather inital values 
  last_hello(Seconds(0)),
  last_snr(0),
  
  rtable(RoutingTable(this->config, this->ipv4)),
  data_cache(PacketCache(this->config))
  {
    // Initialize the sockets
    for (uint32_t i = 0; i < MAX_INTERFACES; i++) {
      this->sockets[i] = 0;
    }
    
  }
  
RoutingProtocol::~RoutingProtocol() {}

void RoutingProtocol::SetConfig(Ptr<AntHocNetConfig> config) {
  this->config = config;
  this->rtable.SetConfig(config);
  this->data_cache.SetConfig(config);
}

Ptr<AntHocNetConfig> RoutingProtocol::GetConfig() const {
  return this->config;
}

NS_OBJECT_ENSURE_REGISTERED(RoutingProtocol);


TypeId RoutingProtocol::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::ahn::RoutingProtocol")
  .SetParent<Ipv4RoutingProtocol> ()
  .SetGroupName("AntHocNet")
  .AddConstructor<RoutingProtocol>()
  .AddAttribute("Config",
    "Pointer to the configuration of this module",
    PointerValue(),
    MakePointerAccessor(&RoutingProtocol::config),
    MakePointerChecker<AntHocNetConfig>()
  )
  
  .AddAttribute ("UniformRv",
    "Access to the underlying UniformRandomVariable",
    StringValue ("ns3::UniformRandomVariable"),
    MakePointerAccessor (&RoutingProtocol::uniform_random),
    MakePointerChecker<UniformRandomVariable> ())
  .AddTraceSource("AntDrop", "An ant is dropped.",
    MakeTraceSourceAccessor(&RoutingProtocol::ant_drop),
    "ns3::ahn::RoutingProtocol::DropReasonCallback")
  .AddTraceSource("DataDrop", "A data packet is dropped.",
    MakeTraceSourceAccessor(&RoutingProtocol::data_drop),
    "ns3::ahn::RoutingProtocol::DropReasonCallback")
  ;
  return tid;
}

void RoutingProtocol::DoInitialize() {
  NS_LOG_FUNCTION(this);
  Ipv4RoutingProtocol::DoInitialize();
}


void RoutingProtocol::DoDispose() {
    NS_LOG_FUNCTION(this);
    
    
    for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::iterator
      it = this->socket_addresses.begin();
      it != this->socket_addresses.end(); ++it) {
      it->first->Close();
    }
    
    Ipv4RoutingProtocol::DoDispose ();
    
}

// ------------------------------------------------------------------
// Implementation of Ipv4Protocol inherited functions
Ptr<Ipv4Route> RoutingProtocol::RouteOutput (Ptr<Packet> p, 
                                             const Ipv4Header &header, 
                                             Ptr<NetDevice> oif, 
                                             Socket::SocketErrno &sockerr) {
  
  if (!p) {
    NS_LOG_DEBUG("Empty packet");
    NS_LOG_FUNCTION(this << "Empty packet");
    return this->LoopbackRoute(header, oif);
  }
  
  // Fail if there are no interfaces
  if (this->socket_addresses.empty()) {
    sockerr = Socket::ERROR_NOROUTETOHOST;
    NS_LOG_LOGIC ("No valid interfaces");
    Ptr<Ipv4Route> route;
    return route;
  }
  
  // Try to find a destination in the rtable right away
  Ipv4Address dst = header.GetDestination();
  
  
  UdpHeader h;
  p->PeekHeader(h);
  if (h.GetSourcePort() != this->config->ant_port) {
    this->rtable.RegisterSession(dst);
  }
  
  NS_LOG_FUNCTION(this << "dst" << dst);
  
  uint32_t iface;
  Ipv4Address nb;
  
  //NS_LOG_UNCOND(this->rtable);
  // TODO: Maybe route with 2.0
  if (this->rtable.SelectRoute(dst, 20.0, iface, nb, this->uniform_random)) {
    Ptr<Ipv4Route> route(new Ipv4Route);
    
    Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol>();
    Ipv4Address this_node = l3->GetAddress(iface, 0).GetLocal();
    
    route->SetOutputDevice(this->ipv4->GetNetDevice(iface));
    route->SetSource(this_node);
    route->SetGateway(nb);
    route->SetDestination(dst);
    
    return route;
  }
  
  // If not found, send it to loopback to handle it in the packet cache.
   this->StartForwardAnt(dst, false);
  
  sockerr = Socket::ERROR_NOTERROR;
  NS_LOG_FUNCTION(this << "started FWAnt to " << dst);
  
  // TODO: This seems to be buggy and lead to data drop
  return this->LoopbackRoute(header, oif);
}


bool RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, 
  Ptr<const NetDevice> idev,
  UnicastForwardCallback ucb, MulticastForwardCallback mcb,
  LocalDeliverCallback lcb, ErrorCallback ecb) {
  
  //Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol>();
  //Ipv4Address this_node = l3->GetAddress(iface, 0).GetLocal();
  uint32_t recv_iface = this->ipv4->GetInterfaceForDevice(idev);
  Ipv4Address this_node = this->ipv4->GetAddress(recv_iface, 0).GetLocal();
  
  NS_LOG_FUNCTION (this << p->GetUid () 
    << header.GetDestination () << idev->GetAddress ());
  
  // TODO: Register activity for destination here
  
  // Fail if no interfaces
  if (this->socket_addresses.empty()) {
    NS_LOG_LOGIC("No active interfaces -> Data dropped");
    
    this->data_drop(p, "No active interfaces", this_node);
    
    Socket::SocketErrno sockerr = Socket::ERROR_NOROUTETOHOST;
    ecb(p, header, sockerr);
    return true;
  }
  
  // Get dst and origin
  Ipv4Address dst = header.GetDestination();
  Ipv4Address origin = header.GetSource();
  
  
  // Fail if Multicast 
  if (dst.IsMulticast()) {
    NS_LOG_LOGIC("AntHocNet does not support multicast");
    
    this->data_drop(p, "Was multicast message, which is not supported", 
                    this_node);
    
    Socket::SocketErrno sockerr = Socket::ERROR_NOROUTETOHOST;
    ecb(p, header, sockerr);
    return true;
  }
  
  // Get the socket and InterfaceAdress of the reciving net device
  Ptr<Socket> recv_socket = this->sockets[recv_iface];
  Ipv4InterfaceAddress recv_sockaddress = this->socket_addresses[recv_socket];
  
  NS_LOG_FUNCTION(this << "origin" << origin << "dst" << dst << "local" << recv_sockaddress.GetLocal());
  NS_LOG_FUNCTION(this << "iface_index" << recv_iface << "socket" 
    << recv_socket << "sockaddress" << recv_sockaddress);
  
  // Check if this is the node and local deliver
  if (recv_sockaddress.GetLocal() == dst) {
    NS_LOG_FUNCTION(this << "Local delivery");
    lcb(p, header, recv_iface);
    return true;
  }
  
  uint32_t iface;
  Ipv4Address nb;
  
  //Search for a route, 
  //NS_LOG_UNCOND(this->rtable);
  if (this->rtable.SelectRoute(dst, 2.0, iface, nb, this->uniform_random)) {
    Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
    // If a route was found:
    // create the route and call UnicastForwardCallback
    rt->SetSource(origin);
    rt->SetDestination(dst);
    rt->SetOutputDevice(this->ipv4->GetNetDevice(iface));
    rt->SetGateway(nb);
    
    NS_LOG_FUNCTION(this << "route to " << rt);
    ucb(rt, p, header);
    return true;
    
  }
  else {
    // If there is no route, cache the data to wait for a route
    
    // Also start a forward ant towards the destination 
    // NOTE: Repair forward ants are unimplemented
    // Reactive forward ants are started by routeoutput at this point already
    
    CacheEntry ce;
    //ce.type = AHNTYPE_DATA;
    // Why 0 here
    ce.iface = 0;
    ce.header = header;
    ce.packet = p;
    ce.ucb = ucb;
    ce.ecb = ecb;
    
    NS_LOG_FUNCTION(this << "cached data, send FWAnt");
    this->data_cache.CachePacket(dst, ce);
    
    // TODO: Only start FWAnt, if origin not 127.0.0.1
    //this->StartForwardAnt(dst, false);
    return true;
  }
  
  this->data_drop(p, "Unknow reason", this_node);
  NS_LOG_FUNCTION(this << "packet dropped");
  return false;
}

// NOTE: This function is strongly relies on code from AODV.
// Copyright (c) 2009 IITP RAS
// It may contain changes
Ptr<Ipv4Route> RoutingProtocol::LoopbackRoute(const Ipv4Header& hdr, 
  Ptr<NetDevice> oif) const{
  
  NS_LOG_FUNCTION (this << hdr);
  NS_ASSERT (lo != 0);
  Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
  rt->SetDestination (hdr.GetDestination ());
  //
  // Source address selection here is tricky.  The loopback route is
  // returned when AODV does not have a route; this causes the packet
  // to be looped back and handled (cached) in RouteInput() method
  // while a route is found. However, connection-oriented protocols
  // like TCP need to create an endpoint four-tuple (src, src port,
  // dst, dst port) and create a pseudo-header for checksumming.  So,
  // AODV needs to guess correctly what the eventual source address
  // will be.
  //
  // For single interface, single address nodes, this is not a problem.
  // When there are possibly multiple outgoing interfaces, the policy
  // implemented here is to pick the first available AODV interface.
  // If RouteOutput() caller specified an outgoing interface, that 
  // further constrains the selection of source address
  //
  auto j = socket_addresses.begin ();
  if (oif) {
      // Iterate to find an address on the oif device
      for (j = socket_addresses.begin (); j != socket_addresses.end (); ++j)
        {
          Ipv4Address addr = j->second.GetLocal ();
          int32_t interface = this->ipv4->GetInterfaceForAddress (addr);
          if (oif == this->ipv4->GetNetDevice (static_cast<uint32_t> (interface)))
            {
              rt->SetSource (addr);
              break;
            }
        }
    }
  else {
      rt->SetSource (j->second.GetLocal ());
    }
  NS_ASSERT_MSG (rt->GetSource () != Ipv4Address (),
    "Valid AntHocNet source address not found");
  
  rt->SetGateway (Ipv4Address ("127.0.0.1"));
  rt->SetOutputDevice (lo);
  return rt;
  
}


// This stuff is needed for handling link layer failures
void RoutingProtocol::AddArpCache(Ptr<ArpCache> a) {
  this->arp_cache.push_back (a);
}

void RoutingProtocol::DelArpCache(Ptr<ArpCache> a) {
  this->arp_cache.erase (std::remove (arp_cache.begin(), 
                                      arp_cache.end() , a), 
                                      arp_cache.end() );
}

std::vector<Ipv4Address> RoutingProtocol::LookupMacAddress(Mac48Address addr) {
  
  std::vector<Ipv4Address> ret;
  
  // Iterate over all interfaces arp cache
  for (std::vector<Ptr<ArpCache> >::const_iterator i = this->arp_cache.begin ();
      i != this->arp_cache.end (); ++i) {
    
    // Get all IpAddresses for a Mac
    std::list<ArpCache::Entry*> lookup = (*i)->LookupInverse(addr);
    
    // Check they are valid and include them into output
    for (std::list<ArpCache::Entry*>::const_iterator lit = lookup.begin();
      lit != lookup.end(); ++lit ) {
      
      ArpCache::Entry* entry = *lit;
      
      if (entry != 0 && (entry->IsAlive () 
          || entry->IsPermanent ()) && !entry->IsExpired ()) {
        ret.push_back(entry->GetIpv4Address());
      }
    }
    
  }
  
  return ret;
}

// Add an interface to an operational AntHocNet instance
void RoutingProtocol::NotifyInterfaceUp (uint32_t interface) {
  
  
  if (interface >= MAX_INTERFACES) {
    NS_LOG_ERROR("Interfaceindex exceeds MAX_INTERFACES");
  }
  
  // Get the interface pointer
  Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol>();
  
  if (l3->GetNAddresses(interface) > 1) {
    NS_LOG_WARN ("interface has more than one address. \
    Only first will be used.");
  }
  
  Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1")) {
    return;
  }
  
  // If there is not yet a socket in use, set one up
  if (this->sockets[interface] != 0) {
    NS_LOG_FUNCTION(this << "Address was already set up" <<
      this->ipv4->GetAddress (interface, 0).GetLocal ());
    return;
  }
  
  
  Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(),
    UdpSocketFactory::GetTypeId());
  NS_ASSERT(socket != 0);
  
  socket->SetRecvCallback(MakeCallback(&RoutingProtocol::Recv, this));
  socket->Bind(InetSocketAddress(iface.GetLocal(), this->config->ant_port));
  socket->SetAllowBroadcast(true);
  socket->SetIpRecvTtl(true);
  socket->BindToNetDevice (l3->GetNetDevice(interface));
  
  // Insert socket into the lists
  this->sockets[interface] = socket;
  this->socket_addresses.insert(std::make_pair(socket, iface));
  
  // Add the interfaces arp cache to the list of arpcaches
  if (l3->GetInterface (interface)->GetArpCache ()) {
    this->AddArpCache(l3->GetInterface (interface)->GetArpCache());
  }
  
  // Add layer2 support if possible
  Ptr<NetDevice> dev = this->ipv4->GetNetDevice(
    this->ipv4->GetInterfaceForAddress (iface.GetLocal ()));
  
  Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
  if (wifi != 0) {
    Ptr<WifiMac> mac = wifi->GetMac();
    Ptr<WifiPhy> phy = wifi->GetPhy();
    if (mac != 0) {
      
      mac->TraceConnectWithoutContext ("TxErrHeader",
        MakeCallback(&RoutingProtocol::ProcessTxError, this));
      
      phy->TraceConnectWithoutContext ("MonitorSnifferRx",
        MakeCallback(&RoutingProtocol::ProcessMonitorSnifferRx, this));
      
    }
    else {
      NS_LOG_FUNCTION(this << "MAC=0 to L2 support");
    }
  }
  else {
    NS_LOG_FUNCTION(this << "WIFI=0 to L2 support");
  }
  
  NS_LOG_FUNCTION(this << "interface" << interface 
    << " address" << this->ipv4->GetAddress (interface, 0) 
    << "broadcast" << iface.GetBroadcast()
    << "socket" << socket);
  
  return;
}

void RoutingProtocol::NotifyInterfaceDown (uint32_t interface) {
  
  NS_LOG_FUNCTION (this << this->ipv4->GetAddress (interface, 0).GetLocal ());
  
  //Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol> ();
  //Ptr<NetDevice> dev = l3->GetNetDevice(interface);
  
  Ptr<Socket> socket = this->FindSocketWithInterfaceAddress(
    this->ipv4->GetAddress(interface, 0));
  
  NS_ASSERT(socket);
  
  // Disable layer 2 link state monitoring (if possible)
  Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol> ();
  Ptr<NetDevice> dev = l3->GetNetDevice (interface);
  Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
  if (wifi != 0) {
    Ptr<WifiMac> mac = wifi->GetMac()->GetObject<AdhocWifiMac>();
    Ptr<WifiPhy> phy = wifi->GetPhy();
    if (mac != 0) {
      mac->TraceDisconnectWithoutContext ("TxErrHeader",
        MakeCallback(&RoutingProtocol::ProcessTxError, this));
      
      phy->TraceDisconnectWithoutContext ("MonitorSnifferRx",
        MakeCallback(&RoutingProtocol::ProcessMonitorSnifferRx, this));
      
      this->DelArpCache(l3->GetInterface(interface)->GetArpCache());
    }
  }
  
  socket->Close();
  
  this->sockets[interface] = 0;
  this->socket_addresses.erase(socket);
  this->rtable.PurgeInterface(interface);
  
}

void RoutingProtocol::NotifyAddAddress (uint32_t interface,
                                        Ipv4InterfaceAddress address) {
  
  Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol> ();
  
  if (!l3->IsUp(interface)) {
    NS_LOG_FUNCTION(this << "Added address");
    return;
  }
  
  if (l3->GetNAddresses(interface) > 1) {
    NS_LOG_WARN("AntHocNet does not support more than one addr per interface");
    return;
  }
  
  Ipv4InterfaceAddress iface = l3->GetAddress(interface, 0);
  Ptr<Socket> socket = this->FindSocketWithInterfaceAddress(iface);
  
  // Need to create socket, if it already exists, there is nothing to do
  if (socket) {
    NS_LOG_FUNCTION(this << "interface" << interface 
    << " address" << address << "socket" << socket << "nothing to do");
    return;
  }
  
  // If this is the first address on this interface
  // create a socket to operate on
  if (l3->GetNAddresses(interface) == 1) {
    socket = Socket::CreateSocket(GetObject<Node>(), 
      UdpSocketFactory::GetTypeId());
    NS_ASSERT(socket != 0);
    
    socket->SetRecvCallback(MakeCallback(
      &RoutingProtocol::Recv, this));
    socket->Bind(InetSocketAddress(iface.GetLocal(), this->config->ant_port));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    socket->BindToNetDevice (l3->GetNetDevice (interface));
    
    // Insert socket into the lists
    this->sockets[interface] = socket;
    this->socket_addresses.insert(std::make_pair(socket, iface));
    
    
    NS_LOG_FUNCTION(this << "interface" << interface 
      << " address" << address << "broadcast" 
      << iface.GetBroadcast() << "socket" << socket);
    return;
    
  }
  else {
    NS_LOG_FUNCTION(this << "Additional address not used for now");
  }
  
}

void RoutingProtocol::NotifyRemoveAddress (uint32_t interface, 
                                           Ipv4InterfaceAddress address) {
  
  Ptr<Socket> socket = FindSocketWithInterfaceAddress(address);
  Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol>();
  
  if (!socket) {
    NS_LOG_WARN("Attempt to delete iface not participating in AHN ignored");
    return;
  }
  
  // Remove all traces of this address from the routing table
  this->sockets[interface] = 0;
  this->socket_addresses.erase(socket);
  this->rtable.PurgeInterface(interface);
  
  socket->Close();
  // If there is more than one address on this interface, we reopen
  // a new socket on another interface to continue the work
  // However, since this node will be considered a new node, it will
  // have to restart the operation from the beginning.
  if (l3->GetNAddresses(interface) > 0) {
    NS_LOG_LOGIC("Address removed, reopen socket with new address");
    Ipv4InterfaceAddress iface = l3->GetAddress(interface, 0);
    
    socket = Socket::CreateSocket (GetObject<Node> (),
      UdpSocketFactory::GetTypeId ());
    NS_ASSERT(socket!= 0);
    
    socket->SetRecvCallback(MakeCallback(
      &RoutingProtocol::Recv, this));
    socket->Bind(InetSocketAddress(iface.GetLocal(), this->config->ant_port));
    socket->BindToNetDevice(l3->GetNetDevice(interface));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    
    this->sockets[interface] = socket;
    this->socket_addresses.insert(std::make_pair(socket, iface));
    
    NS_LOG_FUNCTION(this << "interface " << interface 
      << " address " << address << "reopened socket");
    
  }
  // if this was the only address on the interface, close the socket
  else {
    NS_LOG_FUNCTION(this << "interface " << interface 
      << " address " << address << "closed completely");
  }
  
  
}

void RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4) {
  
  //NS_LOG_FUNCTION(this);
  
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (this->ipv4 == 0);
  
  this->ipv4 = ipv4;
  this->rtable.SetIpv4(ipv4);
  
  // Initialize all sockets as null pointers
  for (uint32_t i = 0; i < MAX_INTERFACES; i++) {
    this->sockets[i] = 0;
  }
  
  // Check that loopback device is set up and the only one
  NS_ASSERT (ipv4->GetNInterfaces () == 1
    && ipv4->GetAddress (0, 0).GetLocal () == Ipv4Address ("127.0.0.1"));
  
  // Set the loopback device
  this->lo = ipv4->GetNetDevice(0);
  
  // Initiate the protocol and start operating
  Simulator::ScheduleNow(&RoutingProtocol::Start, this);
}

void RoutingProtocol::PrintRoutingTable 
  (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const{
  
  // Get the stream, print node id
  // then print time, then simply print rtables output
  *stream->GetStream() 
    << "Node: " << this->ipv4->GetObject<Node>()->GetId()
    << "Time: " << Now().As(unit)
    << "AntHocNet Routing Table: " << std::endl;
    this->rtable.Print(stream);
    *stream->GetStream () << std::endl;
}


// -----------------------------------------------------
// User defined private functions

void RoutingProtocol::Start() {
  NS_LOG_FUNCTION(this);
  
  // Start the HelloTimer
  this->hello_timer.SetFunction(&RoutingProtocol::HelloTimerExpire, this);
  this->hello_timer.Schedule(this->config->hello_interval);
  
  // Start the proacrive ant timer
  this->pr_ant_timer.SetFunction(&RoutingProtocol::PrAntTimerExpire, this);
  this->pr_ant_timer.Schedule(this->config->pr_ant_interval);
  
  // Start the RTableUpdateTimer
  this->rtable_update_timer.SetFunction(
    &RoutingProtocol::RTableTimerExpire, this);
  this->rtable_update_timer.Schedule(this->config->rtable_update_interval);
  
  // Open socket on the loopback
  Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(),
      UdpSocketFactory::GetTypeId());
  socket->Bind(InetSocketAddress(Ipv4Address ("127.0.0.1"), 
                                 this->config->ant_port));
  
  socket->BindToNetDevice(this->lo);
  socket->SetAllowBroadcast(true);
  socket->SetIpRecvTtl(true);
  
  this->sockets[0] = socket;
  this->socket_addresses.insert(std::make_pair(socket, 
                                               ipv4->GetAddress (0, 0)));
  
}

Ptr<Socket> RoutingProtocol::FindSocketWithInterfaceAddress (
  Ipv4InterfaceAddress addr ) const
{
  //NS_LOG_FUNCTION (this << addr);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
    this->socket_addresses.begin (); j != this->socket_addresses.end (); ++j)
  {
    Ptr<Socket> socket = j->first;
    Ipv4InterfaceAddress iface = j->second;
    if (iface == addr)
      return socket;
  }
  Ptr<Socket> socket;
  return socket;
}

int64_t RoutingProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  this->uniform_random->SetStream (stream);
  return 1;
}


uint32_t RoutingProtocol::FindSocketIndex(Ptr<Socket> s) const{
  uint32_t s_index = 0;
  for (s_index = 0; s_index < MAX_INTERFACES; s_index++) {
    if (this->sockets[s_index] == s) {
      
      return s_index;
    }
  }
  
  NS_LOG_FUNCTION(this << "failed to find socket" << s);
  return MAX_INTERFACES;
  
}


void RoutingProtocol::StartForwardAnt(Ipv4Address dst, bool is_proactive) {
  
  uint32_t iface;
  Ipv4Address nb;
  
  NS_LOG_FUNCTION(this);
  
  // Broadcast if no valid entries
  
  if (is_proactive) {
    if (!this->rtable.SelectRoute(dst, 2.0, iface, nb, this->uniform_random,
      true, 1.10)) {
      this->BroadcastForwardAnt(dst, true);
      return;
    }
  }
  else {
    if (!this->rtable.SelectRoute(dst, 20.0, iface, nb, this->uniform_random,
      false, 1.0)) {
      this->BroadcastForwardAnt(dst, false);
      return;
    }
  }
  // If destination was found, send an ant
  Ptr<Socket> socket = this->sockets[iface];
  std::map< Ptr<Socket>, Ipv4InterfaceAddress>::iterator it
    = this->socket_addresses.find(socket);
  
  Ipv4Address this_node = it->second.GetLocal();
  ForwardAntHeader ant (this_node, dst, this->config->initial_ttl);
  
  if (is_proactive) {
    ant.SetBCount(this->config->proactive_bcast_count); 
  }
  else {
    ant.SetBCount(this->config->reactive_bcast_count); 
  }
  this->UnicastForwardAnt(iface, nb, ant, is_proactive);
}


void RoutingProtocol::UnicastForwardAnt(uint32_t iface, 
                                        Ipv4Address dst,
                                        ForwardAntHeader ant, 
                                        bool is_proactive) {
  
  // Get the socket which runs on iface
  Ptr<Socket> socket = this->sockets[iface];
  std::map< Ptr<Socket>, Ipv4InterfaceAddress>::iterator it
    = this->socket_addresses.find(socket);
  
  // Create the packet and set it up correspondingly
  Ptr<Packet> packet = Create<Packet> ();
  TypeHeader type_header;
  
  if (is_proactive) {
    type_header = TypeHeader(AHNTYPE_PRFW_ANT);
  }
  else { 
    type_header = TypeHeader(AHNTYPE_FW_ANT);
  }
  
  SocketIpTtlTag tag;
  tag.SetTtl(ant.GetTTL());
  
  packet->AddPacketTag(tag);
  packet->AddHeader(ant);
  packet->AddHeader(type_header);
  
  NS_LOG_FUNCTION(this << "sending fwant" << *packet);
  
  Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
  Simulator::Schedule(jitter, &RoutingProtocol::Send, 
    this, socket, packet, dst);
  
}


void RoutingProtocol::UnicastBackwardAnt(uint32_t iface,
  Ipv4Address dst, BackwardAntHeader ant) {
  
  // Get the socket which runs on iface
  Ptr<Socket> socket = this->sockets[iface];
  std::map< Ptr<Socket>, Ipv4InterfaceAddress>::iterator it
    = this->socket_addresses.find(socket);
  
  // Create the packet and set it up correspondingly
  Ptr<Packet> packet = Create<Packet> ();
  TypeHeader type_header(AHNTYPE_BW_ANT);
  
  SocketIpTtlTag tag;
  tag.SetTtl(ant.GetMaxHops() - ant.GetHops() + 1);
  
  packet->AddPacketTag(tag);
  packet->AddHeader(ant);
  packet->AddHeader(type_header);
  
  NS_LOG_FUNCTION(this << "sending bwant" << ant << "dst" << dst);
  
  Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
  Simulator::Schedule(jitter, &RoutingProtocol::Send, 
    this, socket, packet, dst);
  
}

void RoutingProtocol::BroadcastForwardAnt(Ipv4Address dst, bool is_proactive) {
  
  
  for (auto sock_it = this->socket_addresses.begin();
      sock_it != this->socket_addresses.end(); ++sock_it) {
    
    Ptr<Socket> socket = sock_it->first;
    Ipv4InterfaceAddress iface = sock_it->second;
    
    // skip the loopback interface
    if (iface.GetLocal() == Ipv4Address("127.0.0.1")) {
      continue;
    }
    
    Ipv4Address this_node = iface.GetLocal();
    
    ForwardAntHeader ant (this_node, dst, this->config->initial_ttl);
    if (is_proactive) {
      ant.SetBCount(this->config->proactive_bcast_count);
    }
    else {
      ant.SetBCount(this->config->reactive_bcast_count);
    }
    
    this->BroadcastForwardAnt(dst, ant, is_proactive);
    
  }
}

void RoutingProtocol::BroadcastForwardAnt(Ipv4Address dst, 
                                          ForwardAntHeader ant,
                                          bool is_proactive) {
  
  if (!this->rtable.IsBroadcastAllowed(dst)) {
    NS_LOG_FUNCTION(this << "broadcast not allowed");
    return;
  }
  
  this->rtable.NoBroadcast(dst, this->config->no_broadcast);
  
  for (auto sock_it = this->socket_addresses.begin(); 
      sock_it != this->socket_addresses.end(); ++sock_it) {
    
    Ptr<Socket> socket = sock_it->first;
    Ipv4InterfaceAddress iface = sock_it->second;
    
    // skip the loopback interface
    if (iface.GetLocal() == Ipv4Address("127.0.0.1")) {
      NS_LOG_FUNCTION(this << "skip lo");
      continue;
    }
    
    NS_LOG_FUNCTION(this << "ant" << ant);
    
    TypeHeader type_header;
    if (is_proactive) {
      type_header = TypeHeader(AHNTYPE_PRFW_ANT);
    }
    else {
      type_header = TypeHeader(AHNTYPE_FW_ANT); 
    }
    
    Ptr<Packet> packet = Create<Packet> ();
    SocketIpTtlTag tag;
    tag.SetTtl(ant.GetTTL());
    
    packet->AddPacketTag(tag);
    packet->AddHeader(ant);
    
    packet->AddHeader(type_header);
    
    
    Ipv4Address destination;
    if (iface.GetMask () == Ipv4Mask::GetOnes ()) {
        destination = Ipv4Address ("255.255.255.255");
    } else { 
        destination = iface.GetBroadcast ();
    }
    
    NS_LOG_FUNCTION(this << "broadcast ant" << *packet << "dst" << dst);
    
    Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
    Simulator::Schedule(jitter, &RoutingProtocol::Send, 
      this, socket, packet, destination);
    
  }
  
}


// ---------------------------------------------------------------------
// Callbacks for lower levels
void RoutingProtocol::ProcessTxError(WifiMacHeader const& header) {

  NS_LOG_FUNCTION(this);
  
  Mac48Address addr[4];
  
  addr[0] = header.GetAddr1();
  addr[1] = header.GetAddr2();
  addr[2] = header.GetAddr3();
  addr[3] = header.GetAddr4();
  
  for (uint32_t i = 0; i < 4; i++) {
    
    std::vector<Ipv4Address> addresses = this->LookupMacAddress(addr[i]);
    
    for (std::vector<Ipv4Address>::const_iterator ad_it = addresses.begin();
      ad_it != addresses.end(); ++ad_it) {
      NS_LOG_FUNCTION(this << "Lost connections to" << *ad_it);
      this->rtable.RemoveNeighbor(this->ipv4->GetInterfaceForAddress(*ad_it), 
                                  *ad_it);
      
    }
  }
  
}

  
void RoutingProtocol::ProcessMonitorSnifferRx(Ptr<Packet const> packet, 
                              uint16_t frequency, uint16_t channel, 
                              uint32_t rate, WifiPreamble isShortPreable,
                              WifiTxVector tx_vector, mpduInfo mpdu,
                              signalNoiseDbm snr) {
  
  this->last_snr = snr.signal - snr.noise;
  
}

// -------------------------------------------------------
// Callback functions used in timers
void RoutingProtocol::HelloTimerExpire() {
  
  this->last_hello = Simulator::Now();
  
  // send a hello over each socket
  for (std::map<Ptr<Socket> , Ipv4InterfaceAddress>::const_iterator
      it = this->socket_addresses.begin();
      it != this->socket_addresses.end(); ++it) {
    
    Ptr<Socket> socket = it->first;
    Ipv4InterfaceAddress iface = it->second;
    
    Ipv4Address src = iface.GetLocal();
    
    if (src == Ipv4Address("127.0.0.1")) {
      continue;
    }
    
    HelloMsgHeader hello_msg(src);
    
    this->rtable.ConstructHelloMsg(hello_msg, 10, this->uniform_random);
    
    TypeHeader type_header(AHNTYPE_HELLO_MSG);
    Ptr<Packet> packet = Create<Packet>();
    
    SocketIpTtlTag tag;
    tag.SetTtl(1);
    
    packet->AddPacketTag(tag);
    packet->AddHeader(hello_msg);
    packet->AddHeader(type_header);
    
    Ipv4Address destination;
    if (iface.GetMask () == Ipv4Mask::GetOnes ()) {
        destination = Ipv4Address ("255.255.255.255");
    } else { 
        destination = iface.GetBroadcast ();
    }
    
    NS_LOG_FUNCTION(this << "iface" << iface << "packet" << *packet);
    
    // Jittery send simulates clock divergence
    Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
    
    // NOTE: The simulation does not work with jitter set to fixed value
    // Is this due to a bug, or is it due to all nodes sneding at once
    Simulator::Schedule(jitter, &RoutingProtocol::Send, 
      this, socket, packet, destination);
  }
  
  Time jitter = MilliSeconds (uniform_random->GetInteger (0, 100));
  this->hello_timer.Schedule(this->config->hello_interval + jitter);
}

void RoutingProtocol::PrAntTimerExpire() {
  
  NS_LOG_FUNCTION(this);
  
  std::list<Ipv4Address> dests = this->rtable.GetSessions();
  for (auto dst_it = dests.begin(); dst_it != dests.end(); ++dst_it) {
    NS_LOG_FUNCTION(this << "sampling" << *dst_it);
    this->StartForwardAnt(*dst_it, true);
  }
  
  Time jitter = MilliSeconds (uniform_random->GetInteger (0, 30));
  this->pr_ant_timer.Schedule(this->config->pr_ant_interval + jitter);
}

void RoutingProtocol::RTableTimerExpire() {
  
  std::list<std::pair<uint32_t, Ipv4Address> > nbs;
  
  // Update the routing table
  nbs = this->rtable.Update(this->config->rtable_update_interval);
  
  
  // Send for every broken link
  for (auto nb_it = nbs.begin(); nb_it != nbs.end(); ++nb_it) {
    
    // Over every active interface
    for (auto sock_it = this->socket_addresses.begin(); sock_it != this->socket_addresses.end(); ++sock_it) {
      
      Ptr<Socket> socket = sock_it->first;
      Ipv4InterfaceAddress iface = sock_it->second;
      
      if (iface.GetLocal() == Ipv4Address("127.0.0.1")) {
        NS_LOG_FUNCTION(this << "skip lo");
        continue;
      }
    
      
      LinkFailureHeader msg;
      msg.SetSrc(this->ipv4->GetInterfaceForAddress(iface.GetLocal()), 
                 iface.GetLocal());
      
      
      this->rtable.ProcessNeighborTimeout(msg, nb_it->first, nb_it->second);
      
      NS_LOG_FUNCTION(this << "Processed NB Timeout " << msg);
      
      TypeHeader type_header = TypeHeader(AHNTYPE_LINK_FAILURE);
      
      Ptr<Packet> packet = Create<Packet> ();
      
      packet->AddHeader(msg);
      packet->AddHeader(type_header);
      
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ()) {
          destination = Ipv4Address ("255.255.255.255");
      } else { 
          destination = iface.GetBroadcast ();
      }
      
      Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
      Simulator::Schedule(jitter, &RoutingProtocol::Send, 
        this, socket, packet, destination);
    }
  }
  
  
  this->rtable_update_timer.Schedule(this->config->rtable_update_interval);
}


// -----------------------------------------------------
// Receiving and Sending stuff
void RoutingProtocol::Recv(Ptr<Socket> socket) {
  
  // retrieve soucre
  Address source_address;
  Ptr<Packet>packet = socket->RecvFrom(source_address);
  InetSocketAddress inet_source = InetSocketAddress::ConvertFrom(source_address);
  
  Ipv4Address src = inet_source.GetIpv4();
  Ipv4Address dst;
  uint32_t iface;
  
  // Get the type of the ant
  TypeHeader type;
  packet->RemoveHeader(type);
  
  if (!type.IsValid()) {
    NS_LOG_WARN("Received ant of unknown type on " << this << ". -> Dropped");
    return;
  }
  
  if (this->socket_addresses.find(socket) != this->socket_addresses.end()) {
  
    iface = this->FindSocketIndex(socket);
    dst = this->socket_addresses[socket].GetLocal();
    
    this->rtable.UpdateNeighbor(iface, src);
    
    NS_LOG_FUNCTION(this << "src" << src << "dst" << dst);  
    
  }
  else {
    dst = Ipv4Address("255.255.255.255");
  }
  
  NS_LOG_FUNCTION("Found interface with ID " 
    << iface << " on destination " << dst
    << "type" << type
  );
  
  // Now enqueue the received packets
  switch (type.Get()) {
    case AHNTYPE_HELLO_MSG:
      this->HandleHelloMsg(packet, iface);
      break;
    case AHNTYPE_HELLO_ACK:
      this->rtable.ProcessAck(src, iface, this->last_hello);
      break;
    case AHNTYPE_FW_ANT:
      this->HandleForwardAnt(packet, iface, false);
      break;
    case AHNTYPE_PRFW_ANT:
      this->HandleForwardAnt(packet, iface, true);
      break;
    case AHNTYPE_BW_ANT:
      this->HandleBackwardAnt(packet, src, iface);
      break;
    case AHNTYPE_LINK_FAILURE:
      this->HandleLinkFailure(packet, iface);
      break;
    default:
      NS_LOG_WARN("Type not implemented.");
      return;
  }
  
  return;
}

// Callback function to send something in a deffered manner
void RoutingProtocol::Send(Ptr<Socket> socket,
  Ptr<Packet> packet, Ipv4Address destination) {
  NS_LOG_FUNCTION(this << "packet" << *packet 
    << "destination" << destination);
  socket->SendTo (packet, 0, InetSocketAddress (destination, 
                                                this->config->ant_port));
}
// -------------------------------------------------------
// Handlers of the different Ants

void RoutingProtocol::HandleHelloMsg(Ptr<Packet> packet, uint32_t iface) {
  
  HelloMsgHeader hello_msg;
  packet->RemoveHeader(hello_msg);
  this->rtable.HandleHelloMsg(hello_msg, iface);
  
  NS_LOG_FUNCTION (this << iface << "packet" << *packet);
  
  // Prepare ack
  Ptr<Packet> packet2 = Create<Packet>();
  TypeHeader type_header(AHNTYPE_HELLO_ACK);
  packet2->AddHeader(type_header);
  
  Ipv4Address dst = hello_msg.GetSrc();
  
  Ptr<Socket> socket = this->sockets[iface];
  
  Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
  Simulator::Schedule(jitter, &RoutingProtocol::Send, 
    this, socket, packet2, dst);
  
  //NS_LOG_UNCOND(this->rtable);
  
  return;

}

void RoutingProtocol::HandleLinkFailure(Ptr<Packet> packet, uint32_t iface) {
 
  LinkFailureHeader msg;
  packet->RemoveHeader(msg);
  
  this->rtable.ProcessLinkFailureMsg(msg, iface);
  
  //NS_LOG_UNCOND(this->rtable);
  
  // TODO: Resent LinkFailure if necessary
}

void RoutingProtocol::HandleForwardAnt(Ptr<Packet> packet, uint32_t iface,
                                       bool is_proactive) {
  
  ForwardAntHeader ant;
  packet->RemoveHeader(ant);
  
  if (!ant.IsValid()) {
    NS_LOG_WARN("Received invalid ForwardAnt ->Dropped");
    return;
  }
  
  // TODO: Cache FW ants and discard doubled ants.
  
  // Get the ip address of the interface, on which this ant
  // was received
  Ptr<Socket> socket = this->sockets[iface];
  std::map< Ptr<Socket>, Ipv4InterfaceAddress>::iterator it
    = this->socket_addresses.find(socket);
  
  Ipv4Address this_node = it->second.GetLocal();
  
  if (ant.GetTTL() == 0) {
    NS_LOG_FUNCTION(this << "Outlived ant" << ant << "->droped");
    return;
  }
  
  // NS_LOG_FUNCTION(this << "Before update" << ant);
  ant.Update(this_node);
  NS_LOG_FUNCTION(this << "After update" << ant);
  
  Ipv4Address final_dst = ant.GetDst();
  
  // Check if this is the destination and create a backward ant
  if (final_dst == this_node) {
    
    BackwardAntHeader bwant(ant);
    
    Ipv4Address dst = bwant.PeekDst();
    
    Ptr<Packet> packet2 = Create<Packet>();
    TypeHeader type_header(AHNTYPE_BW_ANT);
    
    SocketIpTtlTag tag;
    tag.SetTtl(bwant.GetMaxHops() - bwant.GetHops() + 1);
    
    packet2->AddPacketTag(tag);
    packet2->AddHeader(bwant);
    packet2->AddHeader(type_header);
    
    Ptr<Socket> socket2 = this->sockets[iface];
    
    NS_LOG_FUNCTION(this << "received fwant -> converting to bwant");
    NS_LOG_FUNCTION(this << "sending bwant" << "iface"
      << iface << "dst" << dst);
    
    Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
    Simulator::Schedule(jitter, &RoutingProtocol::Send, 
      this, socket2, packet2, dst);
    
    return;
  }
  
  NS_LOG_FUNCTION(this << "iface" << iface << "ant" << ant);
  
  Ipv4Address next_nb;
  uint32_t next_iface;
  //NS_LOG_UNCOND(this->rtable);
  if(
    (!is_proactive && !this->rtable.SelectRoute(final_dst, 20.0, 
      next_iface, next_nb, this->uniform_random, false, 1.0))
    ||
    (is_proactive && !this->rtable.SelectRoute(final_dst, 2.0, 
      next_iface, next_nb, this->uniform_random, true, 1.10))
  )
  {
    
    // This is the new Implementation using a 
    // counted amount of broadcasr
    if (ant.DecBCount()) {
      this->BroadcastForwardAnt(final_dst, ant, is_proactive);
      NS_LOG_FUNCTION(this << "nonrandom select");
      return;
    }
    else {
      if (!this->rtable.SelectRandomRoute(next_iface,
        next_nb, this->uniform_random)) {
        NS_LOG_FUNCTION(this << "no routes -> Ant dropped");
        return;
      }
      NS_LOG_FUNCTION(this << "random selected" << next_nb << next_iface);
      // Do not return, instead go on tu unicast
    }
    
  }
  
  this->UnicastForwardAnt(next_iface, next_nb, ant, is_proactive);
  return;
}

void RoutingProtocol::HandleBackwardAnt(Ptr<Packet> packet, 
                                        Ipv4Address orig_src, uint32_t iface) {
  
  // Deserialize the ant
  BackwardAntHeader ant;
  packet->RemoveHeader(ant);
  
  if (!ant.IsValid()) {
    NS_LOG_WARN("Received invalid BackwardAnt -> Dropped");
    return;
  }
  
  uint64_t T_ind = this->rtable.GetTSend(orig_src, iface).GetNanoSeconds();
  
  // Update the Ant
  Ipv4Address nb = ant.Update(T_ind);
  
  Ipv4Address next_dst = ant.PeekDst();
  Ipv4Address final_dst = ant.GetDst();
  Ipv4Address src = ant.GetSrc();
  
  NS_LOG_FUNCTION(this << "nb" << nb << "src" << src 
    << "final_dst" << final_dst << "next_dst" << next_dst);
  
  // Check if this Node is the destination and manage behaviour
  Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol>();
  Ipv4InterfaceAddress tmp_if = l3->GetAddress(iface, 0);
  
  if (ant.GetHops() == 0) {
    if (ant.PeekThis() == tmp_if.GetLocal()) {
      NS_LOG_FUNCTION(this << "bwant reached its origin.");
      
      
      // Now the RoutingTable needs an update
      if(this->rtable.ProcessBackwardAnt(src, iface, nb, 
        ant.GetT(),(ant.GetMaxHops() - ant.GetHops()) )) {
        this->SendCachedData(src);
      
      }
      
      //NS_LOG_UNCOND(this->rtable);
      
      return;
    }
    else {
      NS_LOG_WARN("Received BWant with hops == 0, but this != dst "
      << ant.PeekThis() << " and " << tmp_if.GetLocal() << "-> Dropped" );
      return;
    }
  }
  
  // Now the RoutingTable needs an update
  if(this->rtable.ProcessBackwardAnt(src, iface, nb, 
    ant.GetT(), (ant.GetMaxHops() - ant.GetHops()) )) {
    this->UnicastBackwardAnt(iface, next_dst, ant);
    //NS_LOG_UNCOND(this->rtable);
  }
  NS_LOG_FUNCTION(this << "iface" << iface << "ant" << ant);
  
  return;
}


void RoutingProtocol::SendCachedData(Ipv4Address dst) {
  
  //NS_LOG_FUNCTION(this << "dst" << dst);
  
  bool dst_found = false;
  
  while (this->data_cache.HasEntries(dst)) {
    
    std::pair<bool, CacheEntry> cv = this->data_cache.GetCacheEntry(dst);
    
    // check, if cache entry is expired
    if (cv.first == false) {
      NS_LOG_FUNCTION(this << "Data " << cv.second.packet << "expired");
      
      this->data_drop(cv.second.packet, 
                      "Cached and expired", cv.second.header.GetSource());
      
      continue;
    }
    
    
    uint32_t iface;
    Ipv4Address nb;
    
    //NS_LOG_UNCOND(this->rtable);
    if (this->rtable.SelectRoute(dst, 2.0, iface, nb, this->uniform_random)) {
      Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
      
      // Create the route and call UnicastForwardCallback
      rt->SetSource(cv.second.header.GetSource());
      rt->SetDestination(dst);
      rt->SetOutputDevice(this->ipv4->GetNetDevice(iface));
      rt->SetGateway(nb);
      
      NS_LOG_FUNCTION(this << "Data " << cv.second.packet << "send");
      cv.second.ucb(rt, cv.second.packet, cv.second.header);
      
      // If there was a route found, the destination must exist
      // in rtable. We can safely assume, that all data to that destination
      // will get routed out here
      dst_found = true;
    }
  
    // If this destination exists, all the data 
    // is routed out by now and can be discarded
    if (dst_found) {
        this->data_cache.RemoveCache(dst);
    }
  }
}

// End of namespaces
}
}
