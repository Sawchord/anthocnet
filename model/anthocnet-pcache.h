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

#ifndef ANTHOCNETPCACHE_H
#define ANTHOCNETPCACHE_H

#include "anthocnet-packet.h"

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"

#include <stdint.h>
#include <vector>
#include <map>

namespace ns3 {
namespace ahn {
  
struct CacheEntry {
  mtype_t type;
  uint32_t iface;
  
  Ipv4Header header;
  
  
  Ptr<const Packet> packet;
  
  Ipv4RoutingProtocol::UnicastForwardCallback ucb;
  Ipv4RoutingProtocol::ErrorCallback ecb;
  
  Time received_in;
  Time expire_in;
  
};
  
class PacketCache {
public:
  //ctor
  PacketCache(Time ie);
  //dtor
  ~PacketCache();

void CachePacket(Ipv4Address dst, CacheEntry ce, Time expire);
void CachePacket(Ipv4Address dst, CacheEntry ce);

  
std::vector<CacheEntry> GetCache(Ipv4Address dst, Time now = Simulator::Now());

std::vector<Ipv4Address> GetDestinations();

void RemoveCache(Ipv4Address dst);

private:
  
  // Standard expire time if no other specified
  Time initial_expire;
  std::map<Ipv4Address, std::vector<CacheEntry> > cache;
  
};


} 
}

#endif /* ANTHOCNETPCACHE_H */