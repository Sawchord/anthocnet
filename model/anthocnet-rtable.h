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

namespace ns3 {
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
  
  
  // For the 2d list, one need to store two links per entry
  RoutingTableEntry* down;
  RoutingTableEntry* right;
  
};

class DestinationInfo {
public:
  
  //ctor
  DestinationInfo(Ipv4Address address = Ipv4Address(), Time now = Simulator::Now());
  //dtor
  ~DestinationInfo();
  
  // The address of the destination
  Ptr<Ipv4Address> dst;
  
  // After a certain amount of time without usage
  // the node consideres the destination useless and deletes it
  Time last_time_used;
  
  // Pointer to the next Destination info structure
  DestinationInfo* right;
  // Pointer to the next routing table entry
  RoutingTableEntry* down;
  
};

class NeighborInfo {
public:
  
  // ctor
  NeighborInfo (Ipv4InterfaceAddress iface = Ipv4InterfaceAddress(),
                  Ipv4Address address = Ipv4Address(), Time now = Simulator::Now());
  //dtor
  ~NeighborInfo();
  
  // The interface behind which this neighbor is
  Ipv4InterfaceAddress iface;
  
  // The address of the neighbor
  Ipv4Address address;
  
  // Neighbors are considered offline, after a certain amount of time without a lifesign and deleted
  Time last_lifesign;
  
  // Pointer to the next routing table enrty
  RoutingTableEntry* right;
  
  // Pointer to the nex NeighborInfo structure
  NeighborInfo* down;
  
};


class RoutingTable {
public:
  
  //ctor
  RoutingTable();
  //dtor 
  ~RoutingTable();
    
  // TODO: Develop plausible API for the routing table.
  
  /**
   * @brief Adds a neighbor to the routing table
   * @arg address The address of the neighbor
   * @arg now The time when this addition was made
   * @returns True, if neighbor was added, false if neighbor was already present
   */
  bool AddNeighbor(Ipv4Address address, Time now);
  
  /**
   * @brief Removes a neighbor from the routing table
   * @arg address The address of the neighbor
   * @returns True, if sucessfully removed, false, if not found
   */
  bool RemoveNeighbor(Ipv4Address address);
  
  
  
private:
  
  // Util functions
  
  
  
  // Network configurations affecting the routing table
  
  // Stores the number of destinations and neighbors
  unsigned int n_dst;
  unsigned int n_nb;
  
  
  
  // The actual datastructures
  // Stores information about possible destinations
  DestinationInfo* right;
  
  // Stores information about all neighbors
  NeighborInfo* down;
  
};

}
}
#endif /* ANTHOCNETRTABLE_H */