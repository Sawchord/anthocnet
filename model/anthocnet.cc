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
  nb_expire(Seconds(5)),
  dst_expire(Seconds(300)),
  rtable(RoutingTable(nb_expire, dst_expire))
  {}
  
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
    TimeValue (Seconds(300)),
    MakeTimeAccessor(&RoutingProtocol::dst_expire),
    MakeTimeChecker()
  )
  .AddAttribute("InitialTTL",
    "The TTL value of a newly generated Ant.",
    UintegerValue(16),
    MakeUintegerAccessor(&RoutingProtocol::initial_ttl),
    MakeUintegerChecker<uint8_t>()
  )
  .AddAttribute ("RTableUpdate",
    "The interval, in which the RoutingTable is updates.",
    TimeValue (MilliSeconds(1000)),
    MakeTimeAccessor(&RoutingProtocol::rtable_update_interval),
    MakeTimeChecker()
  )
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
Ptr<Ipv4Route> RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr) {
  // STUB
  return 0;
}


bool RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                 UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                 LocalDeliverCallback lcb, ErrorCallback ecb) {
  // STUB
  return false;
}

// Add an interface to an operational AntHocNet instance
void RoutingProtocol::NotifyInterfaceUp (uint32_t interface) {
  NS_LOG_FUNCTION (this << this->ipv4->GetAddress (interface,
    0).GetLocal ());
  
  if (interface >= MAX_INTERFACES) {
    NS_LOG_ERROR("Interfaceindex exceeds MAX_INTERFACES");
  }
  
  // Get the interface pointer
  Ptr<Ipv4L3Protocol> l3 =
    this->ipv4->GetObject<Ipv4L3Protocol>();
  
  if (l3->GetNAddresses (interface) > 1) {
    NS_LOG_ERROR ("AntHocNet does not support more than one\
    address per interface for now.");
  }
  
  Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    return;
  
  // Set up the socket to be able to receive
  Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
    UdpSocketFactory::GetTypeId ());
  NS_ASSERT(socket != 0);
  
  socket->SetRecvCallback(MakeCallback(
      &RoutingProtocol::Recv, this));
  
  socket->Bind(InetSocketAddress(iface.GetLocal(),
                                 ANTHOCNET_PORT));
  
  socket->SetAllowBroadcast(true);
  socket->SetIpRecvTtl(true);
  
  // Insert socket into the lists
  this->sockets[interface] = socket;
  
  this->socket_addresses.insert(std::make_pair(socket, iface));
  
  
  // TODO: Need broadcast address?
  // TODO: Support from MacLayer in detecting offline Neighbors
}

void RoutingProtocol::NotifyInterfaceDown (uint32_t interface) {
  
  // TODO: Support MacLayer?
  
  NS_LOG_FUNCTION (this << this->ipv4->GetAddress (interface, 0).GetLocal ());
  
  Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol> ();
  Ptr<NetDevice> dev = l3->GetNetDevice(interface);
  
  Ptr<Socket> socket = this->FindSocketWithInterfaceAddress(
    this->ipv4->GetAddress(interface, 0));
  
  NS_ASSERT(socket);
  
  socket->Close();
  this->sockets[interface] = 0;
  
  this->socket_addresses.erase(socket);
  this->rtable.PurgeInterface(interface);
  // TODO: Close Broadcast, if any
  
}

void RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {
  
  NS_LOG_FUNCTION(this << "interface " << interface 
    << " address " << address);
  
  Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol> ();
  if (!l3->IsUp(interface)) return;
  
  if (l3->GetNAddresses(interface) > 1) {
    NS_LOG_WARN("AntHocNet does not support more than one address per interface");
    
    return;
  }
  
  Ipv4InterfaceAddress iface = l3->GetAddress(interface, 0);
  Ptr<Socket> socket = this->FindSocketWithInterfaceAddress(iface);
  
  // Need to create socket, if it already exists, there is nothing to do
  if (socket) return;
  
  // Create and set up socket
  socket = Socket::CreateSocket(GetObject<Node>(), 
    UdpSocketFactory::GetTypeId());
  NS_ASSERT(socket != 0);
  
  socket->SetRecvCallback(MakeCallback(
    &RoutingProtocol::Recv, this));
  socket->Bind(InetSocketAddress(iface.GetLocal(), ANTHOCNET_PORT));
  socket->SetAllowBroadcast(true);
  socket->SetIpRecvTtl(true);
  
  // Insert socket into the lists
  this->sockets[interface] = socket;
  this->socket_addresses.insert(std::make_pair(socket, iface));
  
}

void RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {
  
  NS_LOG_FUNCTION(this << "interface " << interface 
  << " address " << address);
  
  Ptr<Socket> socket = FindSocketWithInterfaceAddress(address);
  Ptr<Ipv4L3Protocol> l3 = this->ipv4->GetObject<Ipv4L3Protocol>();
  
  if (!socket) {
    NS_LOG_WARN("Attempt to delete iface not participating in AHN ignored");
    return;
  }
  
  
  socket->Close();
  // If there is more than one address on this interface, close then
  // socket and reopen it again with new addess
  if (l3->GetNAddresses(interface)) {
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
    
  }
  // if this was the only address on the interface, close the socket
  else {
    
    NS_LOG_LOGIC("Address removed, closing socket");
    
    this->sockets[interface] = 0;
    this->socket_addresses.erase(socket);
    
    this->rtable.PurgeInterface(interface);
  }
  
  
}

void RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4) {
  
  NS_LOG_FUNCTION(this);
  
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (this->ipv4 == 0);
  
  this->ipv4 = ipv4;
  
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
  
  // Initialize all sockets as null pointers
  for (uint32_t i = 0; i < MAX_INTERFACES; i++) {
    this->sockets[i] = 0;
  }
  
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

uint32_t RoutingProtocol::FindSocketIndex(Ptr<Socket> s) const{
  uint32_t s_index = 0;
  for (s_index = 0; s_index < MAX_INTERFACES; s_index++) {
    if (this->sockets[s_index] == s) break;
  }
  return s_index;
}


// -------------------------------------------------------
// Callback functions used in receiving and timers
void RoutingProtocol::Recv(Ptr<Socket> socket) {
  // STUB
}

void RoutingProtocol::HelloTimerExpire() {
  //STUB
}

void RoutingProtocol::RTableTimerExpire() {
  // STUB
}



}
}
