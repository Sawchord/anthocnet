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
#ifndef ANTHOCNET_H
#define ANTHOCNET_H

#include "anthocnet-rtable.h"
#include "anthocnet-rqueue.h"
#include "anthocnet-packet.h"

#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"

#define MAX_SOCKETS 10

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
  
private:
  
  // All the network config stuff goes here as fields
  
  // The time to life a new born forward and
  uint8_t initial_ttl;
  
  // The frequency in which to send Hello Ants
  Time hello_interval;
  Timer hello_timer;
  
  
  
};
}
}

#endif /* ANTHOCNET_H */

