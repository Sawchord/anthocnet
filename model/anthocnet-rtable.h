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

#ifndef ANTHOCNETRTABLE_H
#define ANTHOCNETRTABLE_H

#include <vector>
#include <list>
#include <map>

#include <cmath>

#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/timer.h"
#include "ns3/net-device.h"
#include "ns3/output-stream-wrapper.h"

namespace anthocnet {
using namespace std;
  
class RoutingTableEntry {
public:
  
  // ctor
  RoutingTableEntry();
  // dtor
  ~RoutingTableEntry();
  
  // The pheromone value of a connection
  double pheromone;
  
  // Additional information is collected about active sending and receiving
  // Increases, whenever a packet is send trought this connection
  double send_pheromone;
  
  // Increases, whenever a packet is received throught this connection
  double recv_pheromone;
  
  // TODO: what else needs to be stored here?
  
  // For the 2d list, one need to store two links per entry
  //RoutingTableEntry down;
  //RoutingTableEntry right;
  
};

template <typename T_Address, typename T_Clock>
class DestinationInfo {
public:
  
  // The address of the destination
  T_Address destination;
  
  // After a certain amount of time without usage
  // the node consideres the destination useless and deletes it
  T_Clock last_time_used;
  
  // Pointer to the next routing table entry
  //RoutingTableEntry* down;
  
};

template <typename T_Address, typename T_Clock>
class NeighborInfo {
public:
  
  // ctor
  NeighborInfo (T_Address address, T_Clock now);
  
  // The address of the neighbor
  T_Address address;
  
  // Neighbors are considered offline, after a certain amount of time without a lifesign and deleted
  T_Clock last_lifesign;
  
  // Pointer to the next routing table enrty
  //RoutingTableEntry* right;
  
};

template <typename T_Address, typename T_Clock>
class RoutingTable {
public:
  
  // TODO: Develop plausible API for the routing table.
  
  /**
   * @brief Adds a neighbor to the routing table
   * @arg address The address of the neighbor
   * @arg now The time when this addition was made
   * @returns True, if neighbor was added, false if neighbor was already present
   */
  bool AddNeighbor(T_Address address, T_Clock now);
  
  /**
   * @brief Removes a neighbor from the routing table
   * @arg address The address of the neighbor
   * @returns True, if sucessfully removed, false, if not found
   */
  bool RemoveNeighbor(T_Address address);
  
  
  
private:
  
  // Util functions
  
  
  
  // Network configurations affecting the routing table
  
  // Stores the number of destinations and neighbors
  unsigned int n_dest;
  unsigned int n_neighbors;
  
  
  
  // The actual datastructures
  // Stores information about possible destinations
  //DestinationInfo <T_Address, T_Clock>* right;
  
  // Stores information about all neighbors
  //NeighborInfo <T_Address, T_Clock>* down;
  
};

}

#endif /* ANTHOCNETRTABLE_H */