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

#include "anthocnet-stat.h"

namespace ns3 {
namespace ahn {
  
AntHocNetStat::AntHocNetStat() {}
AntHocNetStat::~AntHocNetStat() {}
  
  
void AntHocNetStat::RegisterTx(Ipv4Address src, Ipv4Address dst, 
                               Ipv4Address nextHop) {
  
  dst_stat_t dst_stat;
  dst_stat.from = src;
  dst_stat.to = dst;
  dst_stat.time = Simulator::Now();
  this->dst_traffic.push_back(dst_stat);
  
  nb_send_stat_t nbs_stat;
  nbs_stat.via = nextHop;
  nbs_stat.time = Simulator::Now();
  this->nb_send_traffic.push_back(nbs_stat);
}

void AntHocNetStat::RegisterRx(Ipv4Address prevHop) {
  nb_recv_stat_t nbr_stat;
  nbr_stat.via = prevHop;
  nbr_stat.time = Simulator::Now();
  this->nb_recv_traffic.push_back(nbr_stat);
}

double AntHocNetStat::GetTrafficSymmetry() {
  
  this->RemoveOutdatedDstEntries();
  
  // Count for every src dst pair the amount of registered packets
  std::map<SrcDstPair, uint64_t> count;
  
  for (auto dst_it = this->dst_traffic.begin(); 
       dst_it != this->dst_traffic.end(); dst_it++) {
    
    auto c_it = count.find(std::make_pair(dst_it->from, dst_it->to));
    if (c_it == count.end()) {
     count.insert(std::make_pair(
        std::make_pair(dst_it->from, dst_it->to) , 1)); 
      
    } else {
      c_it->second++;
    }
  }
  
  std::set<SrcDstPair> visited;
  std::list<double> sym_vals;
  
  for (auto c_it = count.begin(); c_it != count.end(); ++c_it) {
    
    // Skip if we already seen this pair
    if (visited.find(std::make_pair(c_it->first.first, c_it->first.second)) != visited.end())
      continue;
    if (visited.find(std::make_pair(c_it->first.second, c_it->first.first)) != visited.end())
      continue;
    
    // Get packet counts for both directions
    uint64_t val1 = c_it->second;
    uint64_t val2;
    
    auto c_it2 = count.find(std::make_pair(c_it->first.second, c_it->first.first));
    if (c_it2 == count.end())
      val2 = 0;
    else
      val2 = c_it2->second;
    
    // Divide smaller value by sum
    double newval1 = std::min(val1, val2);
    double newval2 = std::max(val1, val2);
    
    sym_vals.push_back(newval1/ (newval1 + newval2));
    
    // Remember, that we have visited this pair
    visited.insert(std::make_pair(c_it->first.first, c_it->first.second));
    visited.insert(std::make_pair(c_it->first.second, c_it->first.first));
    
  }
  
  // Now get the average of this
  double result = 0;  
  for (auto it = sym_vals.begin(); it != sym_vals.end(); ++it) {
    result += *it;
  }
  
  if (sym_vals.size() == 0)
    return 1.0;
  
  result /= sym_vals.size();
  return result * 2;
}

double AntHocNetStat::GetNbTrafficSymmetry(Ipv4Address nb) {
  
  this->RemoveOutdatedNbEntries();
  
  uint64_t total_packets = 0;
  uint64_t packets_recvd = 0;
  
  for (auto it = this->nb_recv_traffic.begin(); 
      it != this->nb_recv_traffic.end(); ++it) {
    if (it->via == nb) {
      total_packets++;
      packets_recvd++;
    }
  }
  
  for (auto it = this->nb_send_traffic.begin(); 
       it != this->nb_send_traffic.end(); ++it) {
    if (it->via == nb) {
      total_packets++;
    }
  }
  
  if (total_packets == 0)
    return 0.5;
  
  return ((double) packets_recvd) / total_packets;
}

uint64_t AntHocNetStat::GetNumTrafficEntries(Ipv4Address nb) {
  uint64_t entries = 0;  
  for (auto it = this->nb_recv_traffic.begin(); 
       it != this->nb_recv_traffic.end(); ++it) {
    if (it->via == nb) entries++;
  }
  for (auto it = this->nb_send_traffic.begin(); 
       it != this->nb_send_traffic.end(); ++it) {
    if (it->via == nb) entries++;
  }
  return entries;
}


void AntHocNetStat::RemoveOutdatedDstEntries() {
  
  for (auto it = this->dst_traffic.begin(); 
       it != this->dst_traffic.end(); /* no increment */) {
    if ((it->time + this->dst_timewindow) < Simulator::Now())
      this->dst_traffic.erase(it++);
    else
      it++;
  }
}

void AntHocNetStat::RemoveOutdatedNbEntries() {
  
  for (auto it = this->nb_recv_traffic.begin(); 
       it != this->nb_recv_traffic.end(); /* no increment */) {
    if ((it->time + this->nb_timewindow) < Simulator::Now())
      this->nb_recv_traffic.erase(it++);
    else
      it++;
  }
  
  for (auto it = this->nb_send_traffic.begin(); 
       it != this->nb_send_traffic.end(); /* no increment */) {
    if ((it->time + this->nb_timewindow) < Simulator::Now())
      this->nb_send_traffic.erase(it++);
    else
      it++;
  }
  
}


void AntHocNetStat::Expect(Ipv4Address nextHop, Ipv4Address src, 
                           Ipv4Address dst, Ptr<Packet const> packet) {
  
  // If the next hop the destination, we do not expect the packet to be replayed
  if (nextHop == dst) 
    return;
  
  auto nb_it = this->expecting.find(nextHop);
  if (nb_it == this->expecting.end()) {
    this->expecting.insert(std::make_pair(nextHop, std::list<expect_type_t>()));
    nb_it = this->expecting.find(nextHop);
  }
  
  size_t packet_size = packet->GetSerializedSize();
  
  uint8_t* uptr = this->buffer;
  char* cptr = (char*) uptr;
  
  if (!packet->Serialize(uptr, STAT_MAX_PKT_SIZE)) {
    return;
  }
  
  // Fill in the data including the hash
  expect_type_t et;
  
  et.included = Simulator::Now();
  et.src = src;
  et.dst = dst;
  et.hash = Hash32(cptr, packet_size);
  
  nb_it->second.push_back(et);
  
}

void AntHocNetStat::Fullfill(Ipv4Address nextHop, Ipv4Address src, 
                             Ipv4Address dst, Ptr<Packet const> packet) {
  
  auto nb_it = this->expecting.find(nextHop);
  
  for (auto ex_it = nb_it->second.begin(); ex_it != nb_it->second.end(); ++ex_it) {
    if (src != ex_it->src)
      continue;
      
    if (dst != ex_it->dst)
      continue;
    
    size_t packet_size = packet->GetSerializedSize();
  
    uint8_t* uptr = this->buffer;
    char* cptr = (char*) uptr;
    
    if (Hash32(cptr, packet_size) != ex_it->hash)
      continue;
    
    // If we passed the tree tests, we have found a replayed packet
    
    // Insert it into fullfilled list
    auto f_it = this->fullfilled.find(nextHop);
    if (f_it == this->fullfilled.end()) {
      this->fullfilled.insert(std::make_pair(nextHop, PacketList()));
      f_it = this->fullfilled.find(nextHop);
    }
    
    f_it->second.push_back(ex_it->included);
    
    // Remove from expecting list
    nb_it->second.erase(ex_it);
    return;
  }
  
}

void AntHocNetStat::GetExpectingNbs(std::set<Ipv4Address>& nbs) {
  
  for (auto nb_it = this->expecting.begin(); nb_it != this->expecting.end(); ++nb_it) {
    if (nb_it->second.size() != 0) {
      nbs.insert(nb_it->first);
    }
  }
  
  
}

double AntHocNetStat::GetFullfillmentRate(Ipv4Address nb) {
  
  auto nb_it = this->expecting.find(nb);
  auto f_it = this->fullfilled.find(nb);
  auto uf_it = this->unfullfilled.find(nb);
  
  // Return 1 if we do not have any data at all
  if (nb_it == this->expecting.end() 
    && f_it == this->fullfilled.end()
    && uf_it == this->unfullfilled.end()) {
    return 1;
  }
  
  // It is possible, that we do not have lists set up yet
  NS_ASSERT(nb_it != this->expecting.end());
  if (f_it == this->fullfilled.end()) {
    this->fullfilled.insert(std::make_pair(nb, PacketList()));
    f_it = this->fullfilled.find(nb);
  }
  if (uf_it == this->unfullfilled.end()) {
    this->unfullfilled.insert(std::make_pair(nb, PacketList()));
    uf_it = this->unfullfilled.find(nb);
  }
  
  // Put all unfullfilled packets from expecting into unfullfilled list
  for (auto ex_it = nb_it->second.begin(); ex_it != nb_it->second.end(); /* no increment */) {
    if (ex_it->included + this->expect_timeout < Simulator::Now()) {
      uf_it->second.push_back(ex_it->included);
      nb_it->second.erase(ex_it++);
    }
    else {
      ex_it++;
    }
  }
  
  // Remove packets
  uint32_t total_packets = f_it->second.size() + uf_it->second.size();
  while (total_packets > this->packets_considered) {
    
    if (f_it->second.empty()) {
      uf_it->second.pop_front();
      total_packets--;
      continue;
    }
    
    if (uf_it->second.empty()) {
      f_it->second.pop_front();
      total_packets--;
      continue;
    }
    
    if (f_it->second.front() < uf_it->second.front()) 
      f_it->second.pop_front();
    else
      uf_it->second.pop_front();
    
    total_packets--;
  }
  
  //NS_ASSERT((f_it->second.size() + uf_it->second.size()) != 0);
  if ((f_it->second.size() + uf_it->second.size()) == 0) {
    return 1;
  }
  
  return (double) f_it->second.size() / (f_it->second.size() + uf_it->second.size());
  
  
}

double AntHocNetStat::GetNumberOfData(Ipv4Address nb) {
  
  uint32_t pkts = 0;
  
  auto f_it = this->fullfilled.find(nb);
  if (f_it != this->fullfilled.end()) {
    for (auto t_it = f_it->second.begin(); t_it != f_it->second.end(); ++t_it) {
      if (*t_it + this->consider_old > Simulator::Now())
        pkts++;
    }
  }
  
  f_it = this->unfullfilled.find(nb);
  if (f_it != this->unfullfilled.end()) {
    for (auto t_it = f_it->second.begin(); t_it != f_it->second.end(); ++t_it) {
      if (*t_it + this->consider_old > Simulator::Now())
        pkts++;
    }
  }
  
  return (double) pkts;
}



// End of namespaces
}
}
