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
namespace ahn {
using namespace std;


RoutingTableEntry::RoutingTableEntry() {
    this->pheromone = NAN;
    this->send_pheromone = NAN;
    this->recv_pheromone = NAN;
    
}

RoutingTableEntry::~RoutingTableEntry() {
}


DestinationInfo::DestinationInfo(uint32_t index, Time now) :
  index(index),
  last_time_used(now)
  {}

DestinationInfo::~DestinationInfo() {
}

NeighborInfo::NeighborInfo(uint32_t id, Time now) :
  index(id),
  last_lifesign(now)
  {}

NeighborInfo::~NeighborInfo() {
}

RoutingTable::RoutingTable() {
  this->n_dst = 0;
  this->n_nb = 0;
  
  for(uint32_t i = 0; i < MAX_NEIGHBORS; i++) {
    free_rows.push_back(i);
  }
  
  for(uint32_t i = 0; i < MAX_DESTINATIONS; i++) {
    free_collumns.push_back(i);
  }
  
}
RoutingTable::~RoutingTable() {
}

bool RoutingTable::AddNeighbor(uint32_t iface_index, Ipv4Address address, Time now) {
  
  nb_t new_nb = nb_t(iface_index, address);
  
  // Check if NeighborInfo already exists
  map<nb_t, NeighborInfo>::iterator it = this->nbs.find(new_nb);
  if (it != this->nbs.end()) {
    return false;
  }
  
  // Insert Neighbor into map
  this->nbs.insert(pair<nb_t, NeighborInfo> (new_nb, NeighborInfo(free_rows.front())));
  this->free_rows.pop_front();
  
  // Increase number of neigbors
  this->n_nb++;
  return true;
}

bool RoutingTable::AddDestination(Ipv4Address address, Time now) {
  
  // Check if destination already exists
  map<Ipv4Address, DestinationInfo>::iterator it = this->dsts.find(address);
  if (it != this->dsts.end()) {
    return false;
  }
  
  // Insert Destination into map
  this->dsts.insert(pair<Ipv4Address, DestinationInfo>(address, 
    DestinationInfo(free_collumns.front())));
  this->free_collumns.pop_front();
  
  this->n_dst++;
  return true;
  
}

bool RoutingTable::RemoveNeighbor(uint32_t iface_index, Ipv4Address address) {
  
  nb_t rem_nb = nb_t(iface_index, address);
  
  // Check, if Neighbor exists
  map<nb_t, NeighborInfo>::iterator it = this->nbs.find(rem_nb);
  if (it == this->nbs.end()) {
    return false;
  }
  
  // First, reset the row in the array
  uint32_t delete_index = it->second.index;
  for (uint32_t i = 0; i < MAX_NEIGHBORS; i++) {
    this->rtable[delete_index][i] = RoutingTableEntry();
  }
  
  // Add index to freelist
  this->free_rows.push_front(delete_index);
  
  // Then remove the entry from the map of neighbors
  this->nbs.erase(it);
  
  // Decrease counter of neighbors
  this->n_nb--;
  return true;
}

bool RoutingTable::RemoveDestination(Ipv4Address address) {
  // Check, if the destination exists
  map<Ipv4Address, DestinationInfo>::iterator it = this->dsts.find(address);
  if (it == this->dsts.end()) {
    return false;
  }
  
  // Reset the collumn in the array
  uint32_t delete_index = it->second.index;
  for (uint32_t i = 0; i < MAX_DESTINATIONS; i++) {
    this->rtable[i][delete_index] = RoutingTableEntry();
  }
  
  // Add the index to the freelist
  this->free_collumns.push_front(delete_index);
  
  // Remove destination entry from the map
  this->dsts.erase(it);
  
  // Decrease the counter of destination
  this->n_dst--;
  return true;
}



}
}