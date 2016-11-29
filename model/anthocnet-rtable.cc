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
  
  this->free_rows = new list<uint32_t>();
  this->free_collumns = new list<uint32_t>();
  
  for(uint32_t i = 0; i < MAX_NEIGHBORS; i++) {
    this->free_rows->push_back(i);
  }
  
  for(uint32_t i = 0; i < MAX_DESTINATIONS; i++) {
    this->free_collumns->push_back(i);
  }
  
}
RoutingTable::~RoutingTable() {
    delete(this->free_rows);
    delete(this->free_collumns);
}

bool RoutingTable::AddNeighbor(uint32_t iface_index, Ipv4Address address, Time now) {
  
  if (this->free_rows->empty()) {
    NS_LOG_ERROR(this << "rtable full->no rows");
  }
  
  nb_t new_nb = nb_t(iface_index, address);
  
  // Check if NeighborInfo already exists
  std::map<nb_t, NeighborInfo>::iterator it = this->nbs.find(new_nb);
  if (it != this->nbs.end()) {
    return false;
  }
  
  // Insert Neighbor into std::map
  this->nbs.insert(std::make_pair(new_nb,  
    NeighborInfo(free_rows->front())));
  this->free_rows->pop_front();
  
  // Increase number of neigbors
  this->n_nb++;
  return true;
}

bool RoutingTable::AddDestination(Ipv4Address address, Time now) {
  
  if (this->free_collumns->empty()) {
    NS_LOG_ERROR(this << "rtable full->no collumns");
  }
  
  // Check if destination already exists
  std::map<Ipv4Address, DestinationInfo>::iterator it = this->dsts.find(address);
  if (it != this->dsts.end()) {
    return false;
  }
  
  // Insert Destination into std::map
  this->dsts.insert(std::make_pair(address, 
    DestinationInfo(free_collumns->front())));
  this->free_collumns->pop_front();
  
  this->n_dst++;
  return true;
  
}

bool RoutingTable::RemoveNeighbor(uint32_t iface_index, Ipv4Address address) {
  
  nb_t rem_nb = nb_t(iface_index, address);
  
  // Check, if Neighbor exists
  std::map<nb_t, NeighborInfo>::iterator it = this->nbs.find(rem_nb);
  if (it == this->nbs.end()) {
    return false;
  }
  
  // First, reset the row in the array
  uint32_t delete_index = it->second.index;
  for (uint32_t i = 0; i < MAX_NEIGHBORS; i++) {
    this->rtable[delete_index][i] = RoutingTableEntry();
  }
  
  // Add index to freestd::list
  this->free_rows->push_front(delete_index);
  
  // Then remove the entry from the std::map of neighbors
  this->nbs.erase(it);
  
  // Decrease counter of neighbors
  this->n_nb--;
  return true;
}

bool RoutingTable::RemoveDestination(Ipv4Address address) {
  // Check, if the destination exists
  std::map<Ipv4Address, DestinationInfo>::iterator it = this->dsts.find(address);
  if (it == this->dsts.end()) {
    return false;
  }
  
  // Reset the collumn in the array
  uint32_t delete_index = it->second.index;
  for (uint32_t i = 0; i < MAX_DESTINATIONS; i++) {
    this->rtable[i][delete_index] = RoutingTableEntry();
  }
  
  // Add the index to the freestd::list
  this->free_collumns->push_front(delete_index);
  
  // Remove destination entry from the std::map
  this->dsts.erase(it);
  
  // Decrease the counter of destination
  this->n_dst--;
  return true;
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
    
    uint32_t nb_index = it->second.index;
    
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
  std::list<std::map<nb_t, NeighborInfo>::iterator > mark;
  
  // Iterate over neigbors and mark deletables
  for(std::map<nb_t, NeighborInfo>::iterator it =
    this->nbs.begin(); it != this->nbs.end(); ++it) {
    if (it->first.first == interface) {
      // Assign by value
      mark.push_back(it);
    }
  }
  
  
  // Sweep deletables
  for( 
    std::list<std::map<nb_t, NeighborInfo>::iterator >::iterator
    it = mark.begin(); it != mark.end(); ++it) {
    this->RemoveNeighbor((**it).first.first,
                         (**it).first.second);
    }
}

}
}