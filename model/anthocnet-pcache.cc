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

#include "anthocnet-pcache.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("AntHocNetPCache");
namespace ahn {
  
PacketCache::PacketCache(Ptr<AntHocNetConfig> config):
  config(config)
  {}
  
PacketCache::~PacketCache() {}

void PacketCache::SetConfig(Ptr<AntHocNetConfig> config) {
  this->config = config;
}

Ptr<AntHocNetConfig> PacketCache::GetConfig() {
  return this->config;
}

void PacketCache::CachePacket(Ipv4Address dst, CacheEntry ce, Time expire) {
  
  ce.received_in = Simulator::Now();
  ce.expire_in = Simulator::Now() + expire;
  
  std::map<Ipv4Address, std::list<CacheEntry> >::iterator it = this->cache.find(dst);
  
  if (it == this->cache.end()) {
    std::list<CacheEntry> v;
    v.push_back(ce);
    this->cache.insert(std::make_pair(dst, v));
    
    NS_LOG_FUNCTION(this << "to dst" << dst
      << "expires at:" << ce.expire_in.GetSeconds() << "new size" << 1);
  }
  else {
    it->second.push_back(ce);
    NS_LOG_FUNCTION(this << "to dst" << dst
    << "expires at:" << ce.expire_in.GetSeconds() << "new size" << it->second.size());
  }
  
}


void PacketCache::CachePacket(Ipv4Address dst, CacheEntry ce) {
  this->CachePacket(dst, ce, this->config->dcache_expire);
}


bool PacketCache::HasEntries(Ipv4Address dst) {
  
  std::map<Ipv4Address, std::list<CacheEntry> >::iterator it = this->cache.find(dst);
  if (it == this->cache.end()) {
    // Destination does not exist
    return false;
  }
  
  if (it->second.size() == 0) {
      return false;
  }
  
  NS_LOG_FUNCTION(this << "entries" << it->second.size());
  
  return true;
}

std::pair<bool, CacheEntry> PacketCache::GetCacheEntry(Ipv4Address dst) {
  
  
  std::map<Ipv4Address, std::list<CacheEntry> >::iterator it = this->cache.find(dst);
  
  if (it == this->cache.end()) {  
    return std::make_pair(false, CacheEntry());
  }
  
  
  CacheEntry ce = it->second.front();
  it->second.pop_front();
  
  //Time T = now - ce.received_in;
  
  // Only include not expired packets.
  if (ce.expire_in < Simulator::Now()) {
    NS_LOG_FUNCTION(this << "expired packet to" << dst
      << "expired at" << ce.expire_in.GetSeconds());
    return std::make_pair(false, ce);
  }
  else {
    NS_LOG_FUNCTION(this << "packet to" << dst);
    return std::make_pair(true, ce);
  }
}


std::vector<Ipv4Address> PacketCache::GetDestinations() {
    std::vector<Ipv4Address> retv;
    
    for (std::map<Ipv4Address, std::list<CacheEntry> >::iterator it = this->cache.begin();
      it != this->cache.end(); ++it) {
      
      retv.push_back(it->first);
    }
    
    return retv;
}

void PacketCache::RemoveCache(Ipv4Address dst) {
    
    std::map<Ipv4Address, std::list<CacheEntry> >::iterator it = this->cache.find(dst);
    it->second.clear();
    
}


// End of namespaces
}
}