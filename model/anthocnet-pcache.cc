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

#include "anthocnet-pcache.h"

namespace ns3 {
namespace ahn {
  
PacketCache::PacketCache(Time ie):
  initial_expire(ie)
  {}
  
PacketCache::~PacketCache() {}

void PacketCache::CachePacket(Ipv4Address dst, CacheEntry ce, Time expire) {
  
  ce.received_in = Simulator::Now();
  ce.expire_in = expire;
  
  std::map<Ipv4Address, std::vector<CacheEntry> >::iterator it = this->cache.find(dst);
  
  if (it == this->cache.end()) {
    std::vector<CacheEntry> v;
    v.push_back(ce);
    this->cache.insert(std::make_pair(dst, v));
  }
  else {
    it->second.push_back(ce);
  }
}

void PacketCache::CachePacket(Ipv4Address dst, CacheEntry ce) {
  this->CachePacket(dst, ce, this->initial_expire);
}

std::vector<CacheEntry> PacketCache::GetCache(Ipv4Address dst, Time now) {
  
  std::vector<CacheEntry> retv;
  
  std::map<Ipv4Address, std::vector<CacheEntry> >::iterator it = this->cache.find(dst);
  
  if (it == this->cache.end()) {
    // Return empty vector
    std::vector<CacheEntry> v;
    return v;
  }
  
  for (uint32_t i = 0; i < it->second.size(); i++) {
    
    Time T = now - it->second[i].received_in;
    
    // Only include not expired packets.
    if (T < it->second[i].expire_in) {
      retv.push_back(it->second[i]);
    }
  }
  return retv;
}


std::vector<Ipv4Address> PacketCache::GetDestinations() {
    std::vector<Ipv4Address> retv;
    
    for (std::map<Ipv4Address, std::vector<CacheEntry> >::iterator it = this->cache.begin();
      it != this->cache.end(); ++it) {
      
      retv.push_back(it->first);
    }
    
    return retv;
}

void PacketCache::RemoveCache(Ipv4Address dst) {
    
    std::map<Ipv4Address, std::vector<CacheEntry> >::iterator it = this->cache.find(dst);
    it->second.clear();
    
}


// End of namespaces
}
}