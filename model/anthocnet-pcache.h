/*
 * Copyright (c) 2017 Leon Tan
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
#include "anthocnet-config.h"

#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/output-stream-wrapper.h"

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
  PacketCache(Ptr<AntHocNetConfig> config);
  //dtor
  ~PacketCache();

void CachePacket(Ipv4Address dst, CacheEntry ce, Time expire);
void CachePacket(Ipv4Address dst, CacheEntry ce);

bool HasEntries(Ipv4Address dst);
std::pair<bool, CacheEntry> GetCacheEntry(Ipv4Address dst);

std::vector<Ipv4Address> GetDestinations();

void RemoveCache(Ipv4Address dst);

void SetConfig(Ptr<AntHocNetConfig>);
Ptr<AntHocNetConfig> GetConfig();

private:
  
  // Configuration of the packet cache
  Ptr<AntHocNetConfig> config;
  
  std::map<Ipv4Address, std::list<CacheEntry> > cache;
  
};


} 
}

#endif /* ANTHOCNETPCACHE_H */