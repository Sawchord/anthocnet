/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
  if (ipv4) { std::clog << "[node " << ipv4->GetObject<Node> ()->GetId () << "] "; }

#include "anthocnet.h"

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

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("AntHocNetRoutingProtocol");
namespace ahn {

//ctor
RoutingProtocol::RoutingProtocol ():
  initial_ttl(16),
  hello_interval(Seconds(1)),
  hello_timer(Timer::CANCEL_ON_DESTROY),
  rtable_update_interval(MilliSeconds(1000)),
  rtable_update_timer(Timer::CANCEL_ON_DESTROY),
  rqueue_max_len(64),
  queue_expire(MilliSeconds(1000)),
  nb_expire(Seconds(5)),
  dst_expire(Seconds(30)),
  alpha_T_mac(0.7),
  T_hop(0.2),
  gamma_pheromone(0.7),
  avr_T_mac(Seconds(0)),
  rtable(RoutingTable(nb_expire, dst_expire, T_hop, gamma_pheromone)),
  packet_queue(IncomePacketQueue(rqueue_max_len, queue_expire)),
  vip_queue(IncomePacketQueue(rqueue_max_len, queue_expire))
  {
    // Initialize the sockets
    for (uint32_t i = 0; i < MAX_INTERFACES; i++) {
      this->sockets[i] = 0;
    }
    
  }
  
RoutingProtocol::~RoutingProtocol() {}

NS_OBJECT_ENSURE_REGISTERED(RoutingProtocol);


TypeId RoutingProtocol::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::ahn::RoutingProtocol")
  .SetParent<Ipv4RoutingProtocol> ()
  .SetGroupName("AntHocNet")
  .AddConstructor<RoutingProtocol>()
  .AddAttribute ("HelloInterval",
    "HELLO messages emission interval.",
    TimeValue (Seconds(1)),
    MakeTimeAccessor(&RoutingProtocol::hello_interval),
    MakeTimeChecker()
  )
  .AddAttribute ("NeighborExpire",
    "Time without HelloAnt, after which a neighbor is considered offline.",
    TimeValue (Seconds(5)),
    MakeTimeAccessor(&RoutingProtocol::nb_expire),
    MakeTimeChecker()
  )
  .AddAttribute ("DestinationExpire",
    "Time without traffic, after which a destination is considered offline.",
    TimeValue (Seconds(30)),
    MakeTimeAccessor(&RoutingProtocol::dst_expire),
    MakeTimeChecker()
  )
  .AddAttribute("InitialTTL",
    "The TTL value of a newly generated Ant.",
    UintegerValue(16),
    MakeUintegerAccessor(&RoutingProtocol::initial_ttl),
    MakeUintegerChecker<uint8_t>()
  )
  .AddAttribute("PacketQueueLength",
    "The length of the packet queues.",
    UintegerValue(64),
    MakeUintegerAccessor(&RoutingProtocol::rqueue_max_len),
    MakeUintegerChecker<uint32_t>()
  )
  .AddAttribute("AlphaTMac",
    "The alpha value of the running average of T_mac.",
    DoubleValue(0.7),
    MakeDoubleAccessor(&RoutingProtocol::alpha_T_mac),
    MakeDoubleChecker<double>()
  )
  
  .AddAttribute("THop",
    "The THop heuristic used to calculate initial pheromone values",
    DoubleValue(0.2),
    MakeDoubleAccessor(&RoutingProtocol::alpha_T_mac),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("GammaPheromone",
    "The pheromone uses a running average with a decline defined by gamma",
    DoubleValue(0.7),
    MakeDoubleAccessor(&RoutingProtocol::gamma_pheromone),
    MakeDoubleChecker<double>()
  )
  .AddAttribute ("PacketQueueExpire",
    "After this Time, a packet in the packet queue is considered outdated and dropped",
    TimeValue (MilliSeconds(1000)),
    MakeTimeAccessor(&RoutingProtocol::queue_expire),
    MakeTimeChecker()
  )
  .AddAttribute ("RTableUpdate",
    "The interval, in which the RoutingTable is updated.",
    TimeValue (MilliSeconds(1000)),
    MakeTimeAccessor(&RoutingProtocol::rtable_update_interval),
    MakeTimeChecker()
  )
  .AddAttribute ("UniformRv",
    "Access to the underlying UniformRandomVariable",
    StringValue ("ns3::UniformRandomVariable"),
    MakePointerAccessor (&RoutingProtocol::uniform_random),
    MakePointerChecker<UniformRandomVariable> ())
  // TODO: Add other attributes
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
Ptr<Ipv4Route> RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, 
  Ptr<NetDevice> oif, Socket::SocketErrno &sockerr) {
  NS_LOG_FUNCTION(this << "oif" << oif << "header" << header);
  
  if (!p) {
    NS_LOG_DEBUG("Empty packet");
    return this->LoopbackRoute(header, oif);
  }
  
  
  
  return 0;
}


bool RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                 UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                 LocalDeliverCallback lcb, ErrorCallback ecb) {
  // STUB
  
  NS_LOG_FUNCTION(this);
  return false;
}

// NOTE: This function is strongly relies on code from AODV.
// Copyright (c) 2009 IITP RAS
// It may contained changes
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
  std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = socket_addresses.begin ();
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
  NS_ASSERT_MSG (rt->GetSource () != Ipv4Address (), "Valid AntHocNet source address not found");
  rt->SetGateway (Ipv4Address ("127.0.0.1"));
  rt->SetOutputDevice (lo);
  return rt;
  
}



// Add an interface to an operational AntHocNet instance
void RoutingProtocol::NotifyInterfaceUp (uint32_t interface) {
  
  
  if (interface >= MAX_INTERFACES) {
    NS_LOG_ERROR("Interfaceindex exceeds MAX_INTERFACES");
  }
  
  // Get the interface pointer
  Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol>();
  
  if (l3->GetNAddresses (interface) > 1) {
    NS_LOG_WARN ("interface has more than one address. \
    Only first will be used.");
  }
  
  Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1")) {
    return;
  }
  
  // If there is not yet a socket in use, set one up
  if (this->sockets[interface] == 0) {
    Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(),
      UdpSocketFactory::GetTypeId());
    NS_ASSERT(socket != 0);
    
    socket->SetRecvCallback(MakeCallback(
      &RoutingProtocol::Recv, this));
    socket->Bind(InetSocketAddress(iface.GetLocal(), ANTHOCNET_PORT));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    socket->BindToNetDevice (l3->GetNetDevice (interface));
    
    // Insert socket into the lists
    this->sockets[interface] = socket;
    this->socket_addresses.insert(std::make_pair(socket, iface));
    
    // Need to add a broadcast socket
    socket = Socket::CreateSocket(GetObject<Node>(),
      UdpSocketFactory::GetTypeId());
    NS_ASSERT(socket != 0);
    
    socket->SetRecvCallback(MakeCallback(
      &RoutingProtocol::Recv, this));
    socket->Bind(InetSocketAddress(iface.GetBroadcast(), ANTHOCNET_PORT));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    socket->BindToNetDevice (l3->GetNetDevice (interface));
    this->bcast_addresses.insert(std::make_pair(socket, iface));
    
    //NS_LOG_FUNCTION (this << "interface" << interface << " local address" << 
    //  this->ipv4->GetAddress (interface, 0).GetLocal ());
    
    NS_LOG_FUNCTION(this << "interface" << interface 
      << " address" << this->ipv4->GetAddress (interface, 0) 
      << "broadcast" << iface.GetBroadcast()
      << "socket" << socket);
    
  }
  else {
    NS_LOG_FUNCTION(this << "Address was already set up" <<
      this->ipv4->GetAddress (interface, 0).GetLocal ());
  }
  
  
  // TODO: Support from MacLayer in detecting offline Neighbors
}

void RoutingProtocol::NotifyInterfaceDown (uint32_t interface) {
  
  // TODO: Support MacLayer?
  
  NS_LOG_FUNCTION (this << this->ipv4->GetAddress (interface, 0).GetLocal ());
  
  //Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol> ();
  //Ptr<NetDevice> dev = l3->GetNetDevice(interface);
  
  Ptr<Socket> socket = this->FindSocketWithInterfaceAddress(
    this->ipv4->GetAddress(interface, 0));
  
  NS_ASSERT(socket);
  
  socket->Close();
  
  this->sockets[interface] = 0;
  this->socket_addresses.erase(socket);
  this->rtable.PurgeInterface(interface);
  
}

void RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {
  
  Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol> ();
  
  if (!l3->IsUp(interface)) {
    NS_LOG_FUNCTION(this << "Added address");
    return;
  }
  
  if (l3->GetNAddresses(interface) > 1) {
    NS_LOG_WARN("AntHocNet does not support more than one address per interface");
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
    socket->Bind(InetSocketAddress(iface.GetLocal(), ANTHOCNET_PORT));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    socket->BindToNetDevice (l3->GetNetDevice (interface));
    
    // Insert socket into the lists
    this->sockets[interface] = socket;
    this->socket_addresses.insert(std::make_pair(socket, iface));
    
    // Need to add a broadcast socket
    socket = Socket::CreateSocket(GetObject<Node>(),
      UdpSocketFactory::GetTypeId());
    NS_ASSERT(socket != 0);
    
    socket->SetRecvCallback(MakeCallback(
      &RoutingProtocol::Recv, this));
    socket->Bind(InetSocketAddress(iface.GetBroadcast(), ANTHOCNET_PORT));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    socket->BindToNetDevice (l3->GetNetDevice (interface));
    this->bcast_addresses.insert(std::make_pair(socket, iface));
    
    
    NS_LOG_FUNCTION(this << "interface" << interface 
      << " address" << address << "broadcast" << iface.GetBroadcast() << "socket" << socket);
    return;
    
  }
  else {
    NS_LOG_FUNCTION(this << "Additional address not used for now");
  }
  
}

void RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {
  
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
    socket->Bind(InetSocketAddress(iface.GetLocal(), ANTHOCNET_PORT));
    socket->BindToNetDevice(l3->GetNetDevice(interface));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    
    this->sockets[interface] = socket;
    this->socket_addresses.insert(std::make_pair(socket, iface));
    
    // Need to add a broadcast socket
    socket = Socket::CreateSocket(GetObject<Node>(),
      UdpSocketFactory::GetTypeId());
    NS_ASSERT(socket != 0);
    
    socket->SetRecvCallback(MakeCallback(
      &RoutingProtocol::Recv, this));
    socket->Bind(InetSocketAddress(iface.GetBroadcast(), ANTHOCNET_PORT));
    socket->BindToNetDevice (l3->GetNetDevice (interface));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    this->bcast_addresses.insert(std::make_pair(socket, iface));
    
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
  Simulator::ScheduleNow (&RoutingProtocol::Start, this);
}

void RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const{
  
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
  this->hello_timer.Schedule(this->hello_interval);
  
  // Start the RTableUpdateTimer
  this->rtable_update_timer.SetFunction(
    &RoutingProtocol::RTableTimerExpire, this);
  this->rtable_update_timer.Schedule(this->rtable_update_interval);
}

Ptr<Socket> RoutingProtocol::FindSocketWithInterfaceAddress (
  Ipv4InterfaceAddress addr ) const
{
  NS_LOG_FUNCTION (this << addr);
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
      NS_LOG_FUNCTION(this << "index" << s_index << "socket" << s);
      return s_index;
    }
  }
  
  NS_LOG_FUNCTION(this << "failed to find socket" << s);
  return MAX_INTERFACES;
  
}


// -------------------------------------------------------
// Callback functions used in receiving and timers
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
  
  dst = this->socket_addresses[socket].GetLocal();
  NS_LOG_FUNCTION(this << "socket" << socket << "source_address" << source_address 
    << "src" << src << "dst" << dst);  
    
    iface = this->FindSocketIndex(socket);
    
  }
  else {
    dst = Ipv4Address("255.255.255.255");
  }
  
  NS_LOG_FUNCTION("Found interface with ID " << iface << " on destination " << dst);
  
  // Now enqueue the received packets
  switch (type.Get()) {
    case AHNTYPE_HELLO:
      this->vip_queue.Enqueue(AHNTYPE_HELLO, iface, packet);
      break;
    case AHNTYPE_FW_ANT:
      this->packet_queue.Enqueue(AHNTYPE_FW_ANT, iface, packet);
      break;
    case AHNTYPE_BW_ANT:
      this->vip_queue.Enqueue(AHNTYPE_BW_ANT, iface, packet);
      break;
    
    default:
      NS_LOG_WARN("Unimplemented Handlers.");
      return;
  }
  
  // TODO: Get better simulation of T_mac
  Time jitter = MilliSeconds (this->uniform_random->GetInteger (1, 10));
  // Schedule the handling of the queue
  Simulator::Schedule(jitter, &RoutingProtocol::HandleQueue, this);
  return;
}

// Callback function to send something in a deffered manner
void RoutingProtocol::Send(Ptr<Socket> socket,
  Ptr<Packet> packet, Ipv4Address destination) {
  NS_LOG_FUNCTION(this << "packet" << packet << "destination" << destination << "socket" << socket);
  socket->SendTo (packet, 0, InetSocketAddress (destination, ANTHOCNET_PORT));
}

void RoutingProtocol::HelloTimerExpire() {
    
  // send a hello over each socket
  for (std::map<Ptr<Socket> , Ipv4InterfaceAddress>::const_iterator
    it = this->socket_addresses.begin(); it != this->socket_addresses.end(); ++it) {
    
    
    Ptr<Socket> socket = it->first;
    Ipv4InterfaceAddress iface = it->second;
    
    Ipv4Address src = iface.GetLocal();
    
    HelloAntHeader hello_ant(src);
    TypeHeader type_header(AHNTYPE_HELLO);
    Ptr<Packet> packet = Create<Packet>();
    
    SocketIpTtlTag tag;
    tag.SetTtl(1);
    
    packet->AddPacketTag(tag);
    packet->AddHeader(hello_ant);
    packet->AddHeader(type_header);
    
    Ipv4Address destination;
    if (iface.GetMask () == Ipv4Mask::GetOnes ())
      {
        destination = Ipv4Address ("255.255.255.255");
      }
    else
      { 
        destination = iface.GetBroadcast ();
      }
    
    NS_LOG_FUNCTION(this << "iface" << iface << "packet" << *packet);
    
    // Jittery send simulates clock divergence
    Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
    
    // FIXME: The simulation does not work with jitter set to fixed value
    // Is this due to a bug, or is it due to all nodes sneding at once
    //Time jitter = MilliSeconds(10);
    Simulator::Schedule(jitter, &RoutingProtocol::Send, 
      this, socket, packet, destination);
  }
  
  this->hello_timer.Schedule(this->hello_interval);
}

void RoutingProtocol::RTableTimerExpire() {
  NS_LOG_FUNCTION(this);
  
  this->rtable.Update(this->rtable_update_interval);
  this->rtable_update_timer.Schedule(this->rtable_update_interval);
}


void RoutingProtocol::HandleQueue() {
  
  uint32_t iface;
  mtype_t type;
  Ptr<Packet> packet;
  Time new_T_mac;
  
  if (this->vip_queue.Dequeue(type, iface, packet, new_T_mac)) {
    NS_LOG_FUNCTION(this << "handle vip queue" << "type" << type << "iface" << iface
      << "packet" << packet << "new_T_mac" << new_T_mac);
    
    switch (type) {
      
      case AHNTYPE_HELLO:
        this->HandleHelloAnt(packet, iface);
        break;
      case AHNTYPE_BW_ANT:
        this->HandleBackwardAnt(packet, iface, new_T_mac);
        break;
      default:
        NS_LOG_WARN("type " << type << "should not be in vip queue"); 
    }
  } 
  else if (this->packet_queue.Dequeue(type, iface, packet, new_T_mac)) {
    NS_LOG_FUNCTION(this << "handle normie queue" << "type" << type << "iface" << iface
      << "packet" << packet << "new_T_mac" << new_T_mac);
    
    
    switch (type) {
      case AHNTYPE_FW_ANT:
        break;
      default:
        NS_LOG_WARN("type " << type << "should not be in normie queue"); 
    }
  }
  else {
    NS_LOG_FUNCTION(this << "nothing to do");
  }
}


// -------------------------------------------------------
// Handlers of the different Ants

void RoutingProtocol::HandleHelloAnt(Ptr<Packet> packet, uint32_t iface) {
  
  NS_LOG_FUNCTION (this << iface);
  
  HelloAntHeader ant;
  packet->RemoveHeader(ant);
  
  Ipv4Address src = ant.GetSrc();
  
  this->rtable.UpdateNeighbor(iface, src);
  return;

}

void RoutingProtocol::HandleForwardAnt(Ptr<Packet> packet, uint32_t iface) {
  //STUB
  
  ForwardAntHeader ant;
  packet->RemoveHeader(ant);
  
  if (!ant.IsValid()) {
    NS_LOG_WARN("Received invalid ForwardAnt ->Dropped");
    return;
  }
  
  // No need to update the avr_T_mac, since this is the non-priviledged
  // queue.
  
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
  
  Ipv4Address final_dst = ant.GetDst();
  
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
    
    Ptr<Socket> socket = this->sockets[iface];
    
    NS_LOG_FUNCTION(this << "received a fwant for this node. converting to bwant");
    NS_LOG_FUNCTION(this << "sending bwant" << "iface" << iface << "dst" << dst);
    
    Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
    Simulator::Schedule(jitter, &RoutingProtocol::Send, 
      this, socket, packet2, dst);
    
    return;
  }
  
  // TODO: Implement random broadcast
  Ipv4Address next_nb;
  uint32_t next_iface;
  this->rtable.SelectRoute(final_dst, false, next_iface, next_nb, this->uniform_random);
  
  ant.Update(this_node);
  Ptr<Packet> packet2 = Create<Packet>();
  TypeHeader type_header(AHNTYPE_FW_ANT);
  
  SocketIpTtlTag tag;
  tag.SetTtl(ant.GetTTL());
  
  packet2->AddPacketTag(tag);
  packet2->AddHeader(ant);
  packet2->AddHeader(type_header);
  
  
  
  Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
  Simulator::Schedule(jitter, &RoutingProtocol::Send, 
    this, socket, packet2, next_nb);
  
  NS_LOG_FUNCTION(this << "iface" << iface << "ant" << ant);
  
  return;
  
  
}

void RoutingProtocol::HandleBackwardAnt(Ptr<Packet> packet,  uint32_t iface, Time T_mac) {
  
  // Deserialize the ant
  BackwardAntHeader ant;
  packet->RemoveHeader(ant);
  
  if (!ant.IsValid()) {
    NS_LOG_WARN("Received invalid BackwardAnt ->Dropped");
    return;
  }
  
  // Update the running average on T_mac
  this->avr_T_mac = this->alpha_T_mac * this->avr_T_mac + (1 - this->alpha_T_mac) * T_mac;
  
  // Calculate the T_ind value used to update this ant
  uint64_t T_ind =  (this->vip_queue.GetNEntries() + 1) * this->avr_T_mac.GetMilliSeconds();
  Ipv4Address nb = ant.Update(T_ind);
  
  Ipv4Address dst = ant.PeekDst();
  
  // Now the RoutingTable needs an update
  this->rtable.ProcessBackwardAnt(dst, iface, nb, 
    ant.GetT(), ant.GetHops());
  
  // Now resend the backward ant
  Ptr<Packet> packet2 = Create<Packet>();
  TypeHeader type_header(AHNTYPE_BW_ANT);
  
  SocketIpTtlTag tag;
  tag.SetTtl(ant.GetMaxHops() - ant.GetHops() + 1);
  
  packet2->AddPacketTag(tag);
  packet2->AddHeader(ant);
  packet2->AddHeader(type_header);
  
  Ptr<Socket> socket = this->sockets[iface];
  
  Time jitter = MilliSeconds (uniform_random->GetInteger (0, 10));
  Simulator::Schedule(jitter, &RoutingProtocol::Send, 
    this, socket, packet2, dst);
  
  NS_LOG_FUNCTION(this << "iface" << iface << "ant" << ant);
  
  return;
}



// End of namespaces
}
}
