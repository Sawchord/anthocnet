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
namespace anthocnet {
using namespace std;
    
RoutingTableEntry::RoutingTableEntry() {
    this->pheromone = NAN;
    this->send_pheromone = NAN;
    this->recv_pheromone = NAN;
    
}

RoutingTableEntry::~RoutingTableEntry() {
}


DestinationInfo::DestinationInfo(Time now) {
  
  this->last_time_used = now;
  
  
}
DestinationInfo::~DestinationInfo() {
}

NeighborInfo::NeighborInfo(unsigned int index, Time now) {
  
  this->index = index;
  this->last_lifesign = now;
  
}

NeighborInfo::~NeighborInfo() {
}

RoutingTable::RoutingTable() {
  this->n_dst = 0;
  this->n_nb = 0;
  
  for(unsigned int i = 0; i < MAX_NEIGHBORS; i++) {
    free_rows.push_back(i);
  }
  
  for(unsigned int i = 0; i < MAX_DESTINATIONS; i++) {
    free_collumns.push_back(i);
  }
  
}
RoutingTable::~RoutingTable() {
}

bool RoutingTable::AddNeighbor(Ipv4InterfaceAddress iface, Ipv4Address address, Time now) {
  
  // Check if NeighborInfo already exists
  map<Ipv4Address, NeighborInfo>::iterator it = 
    this->nbs.find(address);
  if (it != this->nbs.end()) {
    return false;
  }
  
  // Insert NeighborInfo into NeighborInfo map
  this->nbs.insert(pair<Ipv4Address, NeighborInfo> (address, NeighborInfo(free_rows.front())));
  this->free_rows.pop_front();
  
  // Increase number of neigbors
  this->n_nb++;
  return true;
}

bool RoutingTable::RemoveNeighbor(Ipv4Address address) {
  // Check, if Neighbor exists
  map<Ipv4Address, NeighborInfo>::iterator it = this->nbs.find(address);
  if (it == this->nbs.end()) {
    return false;
  }
  
  // First, reset the row in the array
  unsigned int delete_index = it->second.index;
  for (unsigned int i = 0; i < MAX_NEIGHBORS; i++) {
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




}
}