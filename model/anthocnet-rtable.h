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

#define MAX_NEIGHBORS 30
#define MAX_DESTINATIONS 100

#include <map>
#include <list>
#include <iomanip>

#include <cmath>

#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/timer.h"
#include "ns3/net-device.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {
namespace ahn {
using namespace std;
  
typedef pair<uint32_t, Ipv4Address> nb_t;

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
  
};

class DestinationInfo {
public:
  
  //ctor
  DestinationInfo(uint32_t index, Time now = Simulator::Now());
  //dtor
  ~DestinationInfo();
  
  // The index into the rtable array
  uint32_t index;
  
  // After a certain amount of time without usage
  // the node consideres the destination useless and deletes it
  Time last_time_used;
  
};

class NeighborInfo {
public:
  
  // ctor
  NeighborInfo (uint32_t id, Time now = Simulator::Now());
  //dtor
  ~NeighborInfo();
  
  // The index into the rtable array
  uint32_t index;
  
  // Neighbors are considered offline, after a certain amount of time without a lifesign and deleted
  Time last_lifesign;
  
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
   * @arg iface_index The index of the interface on which to
   *  reach this neighbor
   * @arg address The address of the neighbor
   * @arg now The time when this addition was made
   * @returns True if neighbor was added, false if neighbor was already present
   */
  bool AddNeighbor(uint32_t iface_index, Ipv4Address address, Time now = Simulator::Now());
  
  /**
   * @brief Adds a destination to the routing table
   * @arg address The address of the destination
   * @arg now The time, when this addition was made.
   * @returns True if neighbor was added, false if neighbor already present.
   */
  bool AddDestination(Ipv4Address address, Time now = Simulator::Now());
  
  /**
   * \brief Removes a neighbor from the routing table
   * \param iface_index The interface index of
   *        the neighbor to be removed
   * \param address The address of the neighbor to be removed
   * \returns True if sucessfully removed, false if not found
   */
  bool RemoveNeighbor(uint32_t iface_index, Ipv4Address address);
  
  /**
   * \brief Removes a destination from the routing table
   * \param address The address o fthe destination to be removed.
   * \returns True if successfully removed, false if not found.
   */
  bool RemoveDestination(Ipv4Address addess);
  
  /**
   * \brief Outputs a string representation of this RoutingTable.
   */
  void Print(Ptr<OutputStreamWrapper> stream) const;
  
  /**
   * \brief Removes all neighbors that use the interface
   *        specified.
   * \param interface_index The interface to purge
   */
  void PurgeInterface(uint32_t interface_index);
  
  
private:
  
  // Util functions
  
  
  
  // Network configurations affecting the routing table
  
  // Stores the number of destinations and neighbors
  uint32_t n_dst;
  uint32_t n_nb;
  
  list<Ipv4InterfaceAddress*> interfaces;
  uint32_t n_ifaces;
  
  map<Ipv4Address, DestinationInfo> dsts;
  
  // For neighbors, it is also important to know the interface
  map<nb_t, NeighborInfo> nbs;
  
  list<uint32_t> free_rows;
  list<uint32_t> free_collumns;
  
  RoutingTableEntry rtable [MAX_DESTINATIONS][MAX_NEIGHBORS];
  
};

}
}
#endif /* ANTHOCNETRTABLE_H */