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

#include "anthocnet-rtable.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("AntHocNetRoutingTable");
namespace ahn {


RoutingTableEntry::RoutingTableEntry() {
    this->pheromone = NAN;
    this->avr_hops = NAN;
    this->virtual_pheromone = NAN;
    this->send_pheromone = NAN;
    this->recv_pheromone = NAN;
    
}

RoutingTableEntry::~RoutingTableEntry() {}



// ------------------------------------------------------
DestinationInfo::DestinationInfo(uint32_t index, Time expire) :
  index(index),
  expires_in(expire),
  no_broadcast_time(Seconds(0))
  {}

DestinationInfo::~DestinationInfo() {
}


// --------------------------------------------------------
NeighborInfo::NeighborInfo(uint32_t index, Time expire) :
  index(index),
  expires_in(expire)
  {}

NeighborInfo::~NeighborInfo() {
}



RoutingTable::RoutingTable(Time nb_expire, Time dst_expire,
                           double T_hop, double alpha, double gamma) :
  n_dst(0),
  n_nb(0),
  T_hop(T_hop),
  alpha_pheromone(alpha),
  gamma_pheromone(gamma),
  initial_lifetime_nb(nb_expire),
  initial_lifetime_dst(dst_expire)
{
  // Initialize the usemaps
  for (uint32_t i = 0; i < MAX_DESTINATIONS; i++) this->dst_usemap[i] = false;
  for (uint32_t i = 0; i < MAX_NEIGHBORS; i++) this->nb_usemap[i] = false;
}
  
RoutingTable::~RoutingTable() {
  
}


bool RoutingTable::AddNeighbor(uint32_t iface_index, Ipv4Address address) {
  return this->AddNeighbor(iface_index, address, this->initial_lifetime_nb);
}

bool RoutingTable::AddNeighbor(uint32_t iface_index, Ipv4Address address, Time expire) {
  
  NS_LOG_FUNCTION(this << "iface_index" << iface_index << "address" << address);
  
  if (iface_index >= MAX_NEIGHBORS) {
    NS_LOG_ERROR("iface index to large index: " << iface_index);
    return false;
  }
  
  // Before the neighbor can be added, it needs a destination.
  // Check if destination already exists
  std::map<Ipv4Address, DestinationInfo>::iterator it = this->dsts.find(address);
  if (it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "create dst");
    this->AddDestination(address);
    it = this->dsts.find(address);
  }
  
  for (uint32_t i = 0; i < MAX_NEIGHBORS; i++) {
    if (!this->nb_usemap[i]) {
      //this->nbs.insert(std::make_pair(new_nb, NeighborInfo(i, expire)));
      
      this->nb_usemap[i] = true;
      it->second.nbs.insert(std::make_pair(iface_index, NeighborInfo(i, expire)));
      
      for (uint32_t j = 0; j < MAX_DESTINATIONS; j++) {
        this->rtable[j][i] = RoutingTableEntry();
      }
      
      NS_LOG_FUNCTION(this << "index" << i);
      break;
    }
  }
  
  // Increase number of neigbors
  this->n_nb++;
  return true;
}

void RoutingTable::RemoveNeighbor(uint32_t iface_index, Ipv4Address address) {
  
  NS_LOG_FUNCTION(this << "iface_index" << iface_index << "address" << address);
  
  // Search for the destination
  std::map<Ipv4Address, DestinationInfo>::iterator it = this->dsts.find(address);
  if (it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "nb address does not exist");
    return;
  }
  
  // Search for the interface
  std::map<uint32_t, NeighborInfo>::iterator nb_it = it->second.nbs.find(iface_index);
  if (nb_it == it->second.nbs.end()) {
    NS_LOG_FUNCTION(this << "nb interface does not exist");
    return;
  }
  
  
  // First, reset the row in the array
  uint32_t delete_index = nb_it->second.index;
  this->nb_usemap[delete_index] = false;
  
  NS_LOG_FUNCTION(this << "index" << delete_index);
  
  // Then remove the entry from the std::map of neighbors
  it->second.nbs.erase(nb_it);
  
  // Decrease counter of neighbors
  this->n_nb--;
  return;
}



bool RoutingTable::AddDestination(Ipv4Address address) {
    return this->AddDestination(address, this->initial_lifetime_dst);
}

bool RoutingTable::AddDestination(Ipv4Address address, Time expire) {
  
  NS_LOG_FUNCTION(this << "address" << address);
  
  if (this->n_dst == MAX_DESTINATIONS-1) {
    NS_LOG_ERROR(this << "Out of destination slots in rtable");
    return false;
  }
  
  // Check if destination already exists
  std::map<Ipv4Address, DestinationInfo>::iterator it = this->dsts.find(address);
  if (it != this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst already exist");
    return false;
  }
  
  for (uint32_t i = 0; i < MAX_DESTINATIONS; i++) {
    if (!this->dst_usemap[i]) {
      this->dst_usemap[i] = true;
      this->dsts.insert(std::make_pair(address, DestinationInfo(i, expire)));
      
      // Reset the row in the array
      uint32_t delete_index = it->second.index;
      NS_LOG_FUNCTION(this << "index" << delete_index);
      for (uint32_t j = 0; j < MAX_NEIGHBORS; j++) {
        this->rtable[i][j] = RoutingTableEntry();
      }
      
      NS_LOG_FUNCTION(this << "index" << i);
      break;
    }
  }
  
  this->n_dst++;
  return true;
  
}


void RoutingTable::RemoveDestination(Ipv4Address address) {
  
  NS_LOG_FUNCTION(this << "address" << address);
  
  // Check, if the destination exists
  std::map<Ipv4Address, DestinationInfo>::iterator it = this->dsts.find(address);
  if (it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst does not exist");
    return;
  }
  
  // TODO: Delete the neighbors if any, before deleting the destination
  //it->second.nbs.erase(it->second.nbs.begin(), it->second.nbs.end());
  
  // FIXME: Very shady code, see, if it works
  // If so, remove the mark and sweep thing
  for (std::map<uint32_t, NeighborInfo>::iterator nb_it = it->second.nbs.begin();
    nb_it != it->second.nbs.end(); /* no increment*/) {
    this->RemoveNeighbor((nb_it++)->first, address);
  }
  
  // Set the usemap to indicate
  this->dst_usemap[it->second.index] = false;
  
  // Remove destination entry from the std::map
  this->dsts.erase(it);
  
  // Decrease the counter of destination
  this->n_dst--;
  return;
}

bool RoutingTable::IsBroadcastAllowed(Ipv4Address address) {
  
  std::map<Ipv4Address, DestinationInfo>::const_iterator dst_it = this->dsts.find(address);
  if (dst_it == this->dsts.end()) {
    return false;
  }
  
  if (dst_it->second.no_broadcast_time > Seconds(0)) {
    return false;
  }
  
  return true;
}

void RoutingTable::NoBroadcast(Ipv4Address address, Time duration) {
  
  std::map<Ipv4Address, DestinationInfo>::iterator dst_it = this->dsts.find(address);
  if (dst_it == this->dsts.end()) {
    return;
  }
  
  dst_it->second.no_broadcast_time = duration;
  
  return;
}

// TODO: Remove this function, since it does not function
void RoutingTable::Print(Ptr<OutputStreamWrapper> stream) const {
  /*
  *stream->GetStream() << "AntHocNet routing table \n\t";
  
  // Print the first line (all destination)
  for (std::map<Ipv4Address, DestinationInfo>::const_iterator
    it = this->dsts.begin(); it != this->dsts.end(); ++it) {
    *stream->GetStream() << it->first;
  }
  
  for (std::map<nb_t, NeighborInfo>::const_iterator 
    it = this->nbs.begin(); it != this->nbs.end(); ++it) {
    
    *stream->GetStream() << it->first.first << ":" << it->first.second;
    
    uint32_t nb_index = it->first.first;
    
    for (std::map<Ipv4Address, DestinationInfo>::const_iterator 
      it1 = this->dsts.begin(); it1 != this->dsts.end(); ++it1) {
      
      uint32_t dst_index = it1->second.index;
      
      *stream->GetStream() << this->rtable[dst_index][nb_index]
      .pheromone;
      
    }
    *stream->GetStream () << "\n";
  }*/
}

void RoutingTable::PurgeInterface(uint32_t interface) {
  
  NS_LOG_FUNCTION(this << "interface" << interface);
  
  for (std::map<Ipv4Address, DestinationInfo>::iterator dst_it = this->dsts.begin();
    dst_it != this->dsts.end(); ++dst_it) {
    
    std::map<uint32_t, NeighborInfo>::iterator nb_it = dst_it->second.nbs.find(interface);
    if (nb_it != dst_it->second.nbs.end()) {
      dst_it->second.nbs.erase(nb_it);
    }
    
  }
}

void RoutingTable::SetExpireTimes(Time nb_expire, Time dst_expire) {
  this->initial_lifetime_nb = nb_expire;
  this->initial_lifetime_dst = dst_expire;
}

void RoutingTable::UpdateNeighbor(uint32_t iface_index, Ipv4Address address) {
  
  // Search destination, add if no exist
  std::map<Ipv4Address, DestinationInfo>::iterator dst_it = this->dsts.find(address);
  if (dst_it == this->dsts.end()) {
    this->AddNeighbor(iface_index, address);
    NS_LOG_FUNCTION(this << "added nbs address");
    return;
  }
  
  std::map<uint32_t, NeighborInfo>::iterator nb_it = dst_it->second.nbs.find(iface_index);
  if (nb_it == dst_it->second.nbs.end()) {
    this->AddNeighbor(iface_index, address);
    NS_LOG_FUNCTION(this << "added nbs interface");
    return;
  }
  
  // Update expire time. Make sure, expire time of dst is no
  // lower than the nb expire time
  nb_it->second.expires_in = this->initial_lifetime_nb;
  if (dst_it->second.expires_in < nb_it->second.expires_in) {
    dst_it->second.expires_in = this->initial_lifetime_nb;
  }
  
}

void RoutingTable::Update(Time interval) {
  
  // Iterate over the destinations
  for (std::map<Ipv4Address, DestinationInfo>::iterator dst_it = this->dsts.begin();
    dst_it != this->dsts.end(); /* no increment */) {
    
    // Updatethe no_broadcast timers
    Time bc_dt = dst_it->second.no_broadcast_time - interval;
    if (bc_dt <= Seconds(0)) {
      dst_it->second.no_broadcast_time = Seconds(0);
    }
    else {
      dst_it->second.no_broadcast_time = bc_dt;
    }
  
    // Calculate the time that ticked since the last occurence of update
    Time dst_dt = dst_it->second.expires_in - interval;
    
    // Remove outdated destinations
    if (dst_dt <= Seconds(0)) {
      this->RemoveDestination((dst_it++)->first);
    }
    else {
      
      // Iterate over all the neigbors
      dst_it->second.expires_in = dst_dt;
      
      for (std::map<uint32_t, NeighborInfo>::iterator nb_it = dst_it->second.nbs.begin();
      nb_it != dst_it->second.nbs.end(); /* no increment */) {
        
        Time nb_dt = nb_it->second.expires_in - interval;
        
        if (nb_dt <= Seconds(0)) {
          this->RemoveNeighbor((nb_it++)->first , dst_it->first);
        }
        else {
          
          nb_it->second.expires_in = nb_dt;
          if (dst_dt < nb_dt) {
            dst_it->second.expires_in = nb_dt;
          }
          ++nb_it;
        }
      }
      ++dst_it;
    }
  }
}

bool RoutingTable::ProcessBackwardAnt(Ipv4Address dst, uint32_t iface,
  Ipv4Address nb, uint64_t T_sd, uint32_t hops) {
    
  
  NS_LOG_FUNCTION(this << "dst" << dst << "iface" 
    << iface << "nb" << nb << "T_sd" << T_sd << "hops" << hops);
  // First search the destination and add it if it did not exist.
   // Check if destination already exists
  std::map<Ipv4Address, DestinationInfo>::iterator dst_it = this->dsts.find(dst);
  if (dst_it == this->dsts.end()) {
    this->AddDestination(dst);
    dst_it = this->dsts.find(dst);
  }
  
  // Find the neighbors iterators
  std::map<Ipv4Address, DestinationInfo>::iterator nb_it = this->dsts.find(nb);
  if (nb_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "nb not in reach -> Ant dropped");
    return false;
  }
  
  std::map<uint32_t, NeighborInfo>::iterator nbif_it = nb_it->second.nbs.find(iface);
  if (nbif_it == nb_it->second.nbs.end()) {
    NS_LOG_FUNCTION(this << "interface not found -> Ant dropped");
    return false;
  }
  
  // Since both, the Neighbor and the Destination are found active,
  // reset their expiration dates.
  nb_it->second.expires_in = this->initial_lifetime_nb;
  dst_it->second.expires_in = this->initial_lifetime_dst;
  
  
  // Get the indexes into the pheromone of dst and nb table
  uint32_t nb_index = nb_it->second.index;
  uint32_t dst_index = dst_it->second.index;
  
  double T_id = 1.0 / ((T_sd + hops * this->T_hop) / 2);
  
  // Update the routing table
  RoutingTableEntry* ra = &this->rtable[dst_index][nb_index];
  
  // Check if hop count value is NAN
  if (ra->avr_hops != ra->avr_hops) {
    ra->avr_hops = hops;
  }
  else {
    // TODO: change gamma_pheromone to alpha_pheromone
    ra->avr_hops = this->alpha_pheromone*ra->avr_hops +
      (1.0 - this->alpha_pheromone) * hops;
  }
  
  // Check if pheromone value is NAN
  if (ra->pheromone != ra->pheromone) {
    ra->pheromone = T_id;
  }
  else {
    ra->pheromone = this->gamma_pheromone*ra->pheromone +
      (1.0 - this->gamma_pheromone) * T_id;
  }
  
  NS_LOG_FUNCTION(this << "updated pheromone" << ra->pheromone 
    << "average hops" << ra->avr_hops
    << "for" << dst_it->first << nb_it->first);
  
  return true;
}


bool RoutingTable::SelectRandomRoute(uint32_t& iface, Ipv4Address& nb,
  Ptr<UniformRandomVariable> vr) {
  
  if (this->n_nb == 0) {
    return false;
  }
  
  uint32_t select = vr->GetInteger(0, this->n_nb-1);
  uint32_t counter = 0;
  
  for (std::map<Ipv4Address, DestinationInfo>::iterator dst_it = this->dsts.begin();
    dst_it != this->dsts.end(); ++dst_it) {
    
    for (std::map<uint32_t, NeighborInfo>::iterator nb_it = dst_it->second.nbs.begin();
      nb_it != dst_it->second.nbs.end(); ++nb_it) {
      
      if (counter == select) {
        iface = nb_it->first;
        nb = dst_it->first;
        
        NS_LOG_FUNCTION(this << "iface" << iface << "nb" << nb);
        return true;
      }
      
      counter++;
    }
  }
  
  // Never come here
  return false;
}

bool RoutingTable::SelectRoute(Ipv4Address dst, double power,
  uint32_t& iface, Ipv4Address& nb, Ptr<UniformRandomVariable> vr) {
  
  NS_LOG_FUNCTION(this << "dst" << dst);
  
  // TODO: check, if deterministicly choosing the Neighbor (makes sense to me)
  // is really the way to go here 
  // check, if the destination is a neigbor
  // return Neighbor entry if so
  
  //Get the destination index:
  std::map<Ipv4Address, DestinationInfo>::iterator dst_it = this->dsts.find(dst);
  
  // Fail, if there are no entries to that destination at all
  if (dst_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "no dsts");
    return false;
  }
  
  // Check, if the destination is a neighbor
  if (dst_it->second.nbs.size() != 0) {
    
    std::map<uint32_t, NeighborInfo>::iterator nb_it = dst_it->second.nbs.begin();
    iface = nb_it->first;
    nb = dst_it->first;
    
    NS_LOG_FUNCTION(this << "dst" << dst << "is nb" << nb << "iface" << iface);
    
    return true;
  }
  
  uint32_t dst_index = dst_it->second.index;
  // Calculate the total pheromone value
  double total_pheromone = 0.0;
  uint32_t initialized = 0;
  
  for (std::map<Ipv4Address, DestinationInfo>::iterator dst2_it = this->dsts.begin();
    dst2_it != this->dsts.end(); ++dst2_it) {
    for (std::map<uint32_t, NeighborInfo>::iterator nb_it = dst2_it->second.nbs.begin();
      nb_it != dst2_it->second.nbs.end(); ++nb_it) {
      
      uint32_t nb_index = nb_it->second.index;
      
      // Skip the unititialized entries
      if (this->rtable[dst_index][nb_index].pheromone != 
        this->rtable[dst_index][nb_index].pheromone) {
        continue;
      }
      initialized++;
      
      total_pheromone += pow(this->rtable[dst_index][nb_index].pheromone , power);
    }
  }
  
  
  // Fail, if there are no initialized entries (same as no entires at all)
  if (initialized == 0) {
    NS_LOG_FUNCTION(this << "no initialized nbs");
    return false;
  }
  
  double select = vr->GetValue(0.0, 1.0);
  
  NS_LOG_FUNCTION(this << "total_pheromone" << total_pheromone << "select" << select);
  
  double selected = 0.0;
  
  // To select with right probability, a random uniform variable between 
  // 0 and 1 is generated, then it iterates over the neighbors, calculates their
  // probability and adds it to an aggregator. If the aggregator gets over the 
  // random value, the particular Neighbor is selected.
  
  for (std::map<Ipv4Address, DestinationInfo>::iterator dst2_it = this->dsts.begin();
    dst2_it != this->dsts.end(); ++dst2_it) {
    for (std::map<uint32_t, NeighborInfo>::iterator nb_it = dst2_it->second.nbs.begin();
      nb_it != dst2_it->second.nbs.end(); ++nb_it) {
      
      uint32_t nb_index = nb_it->second.index;
      
      if (this->rtable[dst_index][nb_index].pheromone != 
          this->rtable[dst_index][nb_index].pheromone) {
        continue;
      }
      
      selected += (pow(this->rtable[dst_index][nb_index].pheromone, power) / total_pheromone);
      
      NS_LOG_FUNCTION(this << "selected" << selected);
      
      if (selected > select) {
        iface = nb_it->first;
        nb = dst2_it->first;
        
        // Using this destination means it is relevant, update timeout
        dst2_it->second.expires_in = this->initial_lifetime_dst;
        
        NS_LOG_FUNCTION(this << "dst" << dst << "routed nb" << nb << "iface" << iface);
        return true;
      }
    }
  }
  
  // Never come here
  NS_LOG_FUNCTION(this << "never come here");
  return false;
}


// FIXME: This function causes program to crash
void RoutingTable::Print(std::ostream& os) const{
  
  for (std::map<Ipv4Address, DestinationInfo>::const_iterator dst_it = this->dsts.begin();
    dst_it != this->dsts.end(); ++dst_it) {
    
    os << "\nDST:(" << dst_it->first << ":" << dst_it->second.expires_in << "){";
    
    for (std::map<uint32_t, NeighborInfo>::const_iterator nb_it = dst_it->second.nbs.begin();
    nb_it != dst_it->second.nbs.end(); ++nb_it) {
      
      os << "NB:(" << nb_it->first << ":" << nb_it->second.expires_in << ")";
      
      // TODO: Output the pheromones
      
    }
    
    os << "}";
  }
}

std::ostream& operator<< (std::ostream& os, RoutingTable const& t) {
  t.Print(os);
  return os;
}

}
}