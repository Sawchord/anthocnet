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

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("AntHocNetRoutingTable");
namespace ahn {


RoutingTableEntry::RoutingTableEntry() {
    this->pheromone = NAN;
    this->send_pheromone = NAN;
    this->recv_pheromone = NAN;
    
}

RoutingTableEntry::~RoutingTableEntry() {
}



// ------------------------------------------------------
DestinationInfo::DestinationInfo(uint32_t index, Time expire) :
  index(index),
  expires_in(expire)
  {}

DestinationInfo::~DestinationInfo() {
}


// --------------------------------------------------------
NeighborInfo::NeighborInfo(Time expire) :
  expires_in(expire)
  {}

NeighborInfo::~NeighborInfo() {
}




RoutingTable::RoutingTable(Time nb_expire, Time dst_expire, double T_hop, double gamma) :
  n_dst(0),
  n_nb(0),
  T_hop(T_hop),
  gamma_pheromone(gamma),
  initial_lifetime_nb(nb_expire),
  initial_lifetime_dst(dst_expire)
{}
  
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
  
  
  nb_t new_nb = nb_t(iface_index, address);
  
  // Check if NeighborInfo already exists
  std::map<nb_t, NeighborInfo>::iterator it = this->nbs.find(new_nb);
  if (it != this->nbs.end()) {
    return false;
  }
  
  // Insert Neighbor into std::map
  this->nbs.insert(std::make_pair(new_nb,  
    NeighborInfo(expire)));
  
  // Increase number of neigbors
  this->n_nb++;
  return true;
}

void RoutingTable::RemoveNeighbor(uint32_t iface_index, Ipv4Address address) {
  
  if (iface_index >= MAX_NEIGHBORS) {
    NS_LOG_ERROR("iface index to large index: " << iface_index);
    return;
  }
  
  nb_t rem_nb = nb_t(iface_index, address);
  
  // Check, if Neighbor exists
  std::map<nb_t, NeighborInfo>::iterator it = this->nbs.find(rem_nb);
  if (it == this->nbs.end()) {
    return;
  }
  
  // First, reset the row in the array
  uint32_t delete_index = it->first.first;
  for (uint32_t i = 0; i < MAX_NEIGHBORS; i++) {
    this->rtable[delete_index][i] = RoutingTableEntry();
  }
  
  
  // Then remove the entry from the std::map of neighbors
  this->nbs.erase(it);
  
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
    return false;
  }
  
  // TODO: this is vastly inefficient and 
  // should be replaced in some sensible way
  bool usemap[MAX_DESTINATIONS] = {false};
  
  for (std::map<Ipv4Address, DestinationInfo>::iterator 
    it = this->dsts.begin(); it != this->dsts.end(); ++it) {
     usemap[it->second.index] = true;
  }
  
  for (uint32_t i = 0; i < MAX_DESTINATIONS; i++) {
    if (!usemap[i]) {
      
      this->dsts.insert(std::make_pair(address, DestinationInfo(i, expire)));
      
      break;
    }
  }
  
  this->n_dst++;
  return true;
  
}


void RoutingTable::RemoveDestination(Ipv4Address address) {
  // Check, if the destination exists
  std::map<Ipv4Address, DestinationInfo>::iterator it = this->dsts.find(address);
  if (it == this->dsts.end()) {
    return;
  }
  
  // Reset the collumn in the array
  uint32_t delete_index = it->second.index;
  for (uint32_t i = 0; i < MAX_DESTINATIONS; i++) {
    this->rtable[i][delete_index] = RoutingTableEntry();
  }
  
  // Remove destination entry from the std::map
  this->dsts.erase(it);
  
  // Decrease the counter of destination
  this->n_dst--;
  return;
}

void RoutingTable::Print(Ptr<OutputStreamWrapper> stream) const {
  
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
  }
}

void RoutingTable::PurgeInterface(uint32_t interface) {
  
  // Since you cannot delete a entries of a map while 
  // iterating over it, use mark and sweep
  std::list<nb_t> mark;
  
  // Iterate over neigbors and mark deletables
  for(std::map<nb_t, NeighborInfo>::iterator it =
    this->nbs.begin(); it != this->nbs.end(); ++it) {
    if (it->first.first == interface) {
      // Assign by value
      mark.push_back(it->first);
    }
  }
  
  
  // Sweep deletables
  for(std::list<nb_t>::iterator it = mark.begin(); it != mark.end(); ++it) {
    
    std::map<nb_t, NeighborInfo>::iterator it1 = this->nbs.find(*it);
    
    this->RemoveNeighbor(it1->first.first, it1->first.second);
    }
}

void RoutingTable::SetExpireTimes(Time nb_expire, Time dst_expire) {
  this->initial_lifetime_nb = nb_expire;
  this->initial_lifetime_dst = dst_expire;
}


void RoutingTable::UpdateNeighbor(uint32_t iface_index, Ipv4Address address) {
  
  nb_t new_nb = nb_t(iface_index, address);
  std::map<nb_t, NeighborInfo>::iterator it = this->nbs.find(new_nb);
  if (it == this->nbs.end()) {
    this->AddNeighbor(iface_index, address);
    return;
  }
  
  it->second.expires_in = this->initial_lifetime_nb;
  return;
  
}

void RoutingTable::Update(Time interval) {
  
  // Update neighbors
  std::list<nb_t> nb_mark;
  for (std::map<nb_t, NeighborInfo>::iterator it =
    this->nbs.begin(); it != this->nbs.end(); ++it) {
    
    Time dt = it->second.expires_in - interval;
    
    if (dt <= Seconds(0)) {
      nb_mark.push_back(it->first);
    } else {
      it->second.expires_in = dt;
    }
  }
  
  for (std::list<nb_t>::iterator it = nb_mark.begin();
    it != nb_mark.end(); ++it) {
    
    std::map<nb_t, NeighborInfo>::iterator it1 = this->nbs.find(*it);
    
    this->RemoveNeighbor(it1->first.first, it1->first.second);
  }
  
  // Update destinations
  std::list<Ipv4Address> dst_mark;
  for (std::map<Ipv4Address, DestinationInfo>::iterator
    it = this->dsts.begin(); it != this->dsts.end(); ++it) {
    
    Time dt = it->second.expires_in - interval;
    
    if (dt <= Seconds(0)) {
      dst_mark.push_back(it->first);
    } else {
      it->second.expires_in = dt;
    }
  }
  
  for (std::list<Ipv4Address>::iterator it = dst_mark.begin();
    it != dst_mark.end(); ++it) {
    
    std::map<Ipv4Address, DestinationInfo>::iterator it1 
      = this->dsts.find(*it);
    
    this->RemoveDestination(it1->first);
  }
  
}

void RoutingTable::ProcessBackwardAnt(Ipv4Address dst, uint32_t iface,
  Ipv4Address nb, uint64_t T_sd, uint32_t hops) {
    
  // First search the destination and add it if it did not exist.
   // Check if destination already exists
  std::map<Ipv4Address, DestinationInfo>::iterator dst_it = this->dsts.find(dst);
  if (dst_it == this->dsts.end()) {
    this->AddDestination(dst);
    dst_it = this->dsts.find(dst);
  }
  
  // Do the same for neigbors
  nb_t nbt = nb_t(iface, nb);
  std::map<nb_t, NeighborInfo>::iterator nb_it = this->nbs.find(nbt);
  if (nb_it == this->nbs.end()) {
    this->AddNeighbor(iface, nb);
    nb_it = this->nbs.find(nbt);
  }
  
  // Since both, the Neighbor and the Destination are found active,
  // reset their expiration dates.
  nb_it->second.expires_in = this->initial_lifetime_nb;
  dst_it->second.expires_in = this->initial_lifetime_dst;
  
  NS_ASSERT(nb_it->first.first  == iface);
  
  // Get the indexes of dst and nb into the pheromone table
  uint32_t nb_index = nb_it->first.first;
  uint32_t dst_index = dst_it->second.index;
  
  double T_id = 1.0 / ((T_sd + hops * this->T_hop) / 2);
  
  // Update the routing table
  RoutingTableEntry ra = this->rtable[dst_index][nb_index];
  if (ra.pheromone == NAN) {
    ra.pheromone = T_id;
  }
  else {
    ra.pheromone = this->gamma_pheromone*ra.pheromone +
      (1.0 - this->gamma_pheromone) * T_id;
  }
  
  this->rtable[dst_index][nb_index] = ra;
  return;
}


bool RoutingTable::SelectRandomRoute(uint32_t& iface, Ipv4Address& nb,
  Ptr<UniformRandomVariable> vr) {
  
  if (this->n_nb == 0) return false;
  
  uint32_t select = vr->GetInteger(0, this->n_nb);
  
  std::map<nb_t, NeighborInfo>::iterator it = this->nbs.begin();
  
  for (uint32_t i = 0; i < this->n_nb; i++) {
    
    if (i == select) {
      iface = it->first.first;
      nb = it->first.second;
      return true;
    }
    ++it;
  }
  
  // Never come here
  return false;
}

bool RoutingTable::SelectRoute(Ipv4Address dst, bool proactive,
  uint32_t& iface, Ipv4Address& nb, Ptr<UniformRandomVariable> vr) {
  
  // Set the power modifier.
  double power;
  if (proactive) {
    power = 1.0;
  }else {
    power = 2.0;
  }
  
  //Get the destination index:
  std::map<Ipv4Address, DestinationInfo>::iterator dst_it = this->dsts.find(dst);
  
  // Fail, if there are no entries to that destination at all
  if (dst_it == this->dsts.end()) {
    
    // This should be done by the RoutingRrotocol itslef, 
    // if and only if needed
    //this->AddDestination(dst);
    
    return false;
  }
  
  uint32_t dst_index = dst_it->second.index;
  // Calculate the total pheromone value
  double total_pheromone = 0.0;
  uint32_t initialized = 0;
  
  for (std::map<nb_t, NeighborInfo>::iterator nb_it = this->nbs.begin();
    nb_it != this->nbs.end(); ++nb_it) {
    
    uint32_t nb_index = nb_it->first.first;
    
    // Skip the unititialized entries
    if (this->rtable[dst_index][nb_index].pheromone == NAN) {
      continue;
    }
    initialized++;
    
    total_pheromone += pow(this->rtable[dst_index][nb_index].pheromone , power);
  }
  
  // Fail, if there are no initialized entries (same as no entires at all)
  if (initialized == 0) {
    return false;
  }
  
  
  double select = vr->GetValue(0.0, 1.0);
  double selected = 0.0;
  
  // To select with right probability, a random uniform variable between 
  // 0 and 1 is generated, then it iterates over the neighbors, calculates their
  // probability and adds it to an aggregator. If the aggregator gets over the 
  // random value, the particular Neighbor is selected.
  for (std::map<nb_t, NeighborInfo>::iterator nb_it = this->nbs.begin();
    nb_it != this->nbs.end(); ++nb_it) {
    
    uint32_t nb_index = nb_it->first.first;
    NeighborInfo nbi = nb_it->second;
    
    if (this->rtable[dst_index][nb_index].pheromone == NAN) {
      continue;
    }
    
    selected += pow(this->rtable[dst_index][nb_index].pheromone, power);
    
    if (selected > select) {
      iface = nb_it->first.first;
      nb = nb_it->first.second;
      return true;
    }
  }
  
  // Never come here
  return false;
}

}
}