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
    
    this->down = NULL;
    this->right = NULL;
}

RoutingTableEntry::~RoutingTableEntry() {
}


DestinationInfo::DestinationInfo(Ipv4Address address, Time now) {
  
  this->dst.SetDestination(address);
  this->last_time_used = new Time(now);
  
  this->down = NULL;
  
}
DestinationInfo::~DestinationInfo() {
}

/*NeighborInfo::NeighborInfo(Ipv4Address address, Time now) {
  
  this->address = address;
  this->last_lifesign = now;
  
  this->right = NULL;
}*/
NeighborInfo::~NeighborInfo() {
}

RoutingTable::RoutingTable() {
    this->n_dst = 0;
    this->n_nb = 0;
    
    this->right = NULL;
    this->down = NULL;
}
RoutingTable::~RoutingTable() {
}

/*bool RoutingTable::AddNeighbor(Ipv4Address address, Time now) {
  
  // Check, that neighbor does not already exist
  NeighborInfo* check_nb = this->down;
  for (unsigned int i = 0; i < this->n_nb; i++) {
    if (check_nb->address == address) {
      return false;
    } else {
      check_nb = check_nb->down;
    }
  }
  
  // Create the new Neighbor
  NeighborInfo* new_nb = new NeighborInfo(address, now);
  
  // Insert Neighbor at the top of the list
  NeighborInfo* temp_np = this->down;
  this->down = new_nb;
  new_nb->down = temp_np;
  
  // Insert all empty RoutingTable entries.
  
  DestinationInfo* temp_dst = this->right;
  for (unsigned int i = 0; i < this->n_dst; i++) {
    RoutingTableEntry* temp_rte = temp_dst->down;
    temp_dst->down = new RoutingTableEntry();
    temp_dst->down->down = temp_rte;
  }
  
  // Increase the number if n_neighbors
  this->n_nb++;
  
  return true;
}*/



}
}