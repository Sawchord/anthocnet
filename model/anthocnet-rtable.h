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

// TODO: Make these settable by GetTypeId()
//       instead of hardcoding them
#define MAX_NEIGHBORS 300
#define MAX_DESTINATIONS 1000


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
  DestinationInfo(uint32_t, Time);
  //dtor
  ~DestinationInfo();
  
  // The index into the rtable array
  uint32_t index;
  
  // After a certain amount of time without usage
  // the node consideres the destination useless and deletes it
  Time expires_in;
  
};

class NeighborInfo {
public:
  
  // ctor
  NeighborInfo (Time);
  //dtor
  ~NeighborInfo();
  
  // Neighbors are considered offline, after a certain amount of time without a lifesign and deleted
  Time expires_in;
  
};


class RoutingTable {
public:
  
  //ctor
  RoutingTable(Time, Time);
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
  bool AddNeighbor(uint32_t iface_index, Ipv4Address address);
  
  /**
   * @brief Adds a destination to the routing table
   * @arg address The address of the destination
   * @returns True if neighbor was added, false if neighbor already present.
   */
  bool AddDestination(Ipv4Address address, Time now = Simulator::Now());
  
  /**
   * \brief Removes a neighbor from the routing table
   * \param iface_index The interface index of
   *        the neighbor to be removed
   * \param address The address of the neighbor to be removed
   */
  void RemoveNeighbor(uint32_t iface_index, Ipv4Address address);
  
  /**
   * \brief Removes a destination from the routing table
   * \param address The address o fthe destination to be removed.
   */
  void RemoveDestination(Ipv4Address addess);
  
  /**
   * \brief Outputs a string representation of this RoutingTable.
   */
  void Print(Ptr<OutputStreamWrapper> stream) const;
  
  /**
   * \brief Removes all neighbors that are reachable via the interface
   *        specified.
   * \param interface_index The interface to purge
   */
  void PurgeInterface(uint32_t interface_index);
  
  /**
   * \brief Resets the initial expire times of neighbor and
   *        destination entries.
   * \param nb_expire The initial ttl of neighbor entries
   * \param dst_expire The initia ttl of destination entries
   */
  void SetExpireTimes(Time, Time);
  
  /**
   * \brief Updates a neighbor entry on receiving a HelloAnt
   *        either by adding the neighbors to the table or
   *        resetting its expire time, if it already exist.
   * \param iface_index The interface on which to reach the neighbor/
   * \param address The address of the neighbor.
   */
  void UpdateNeighbor(uint32_t , Ipv4Address);
  
  /**
   * \brief Updates the expire times of all neighbors and destinations.
   *        Removes the ones, that are expired.
   * \param interval The time interval, in which this function is called.
   */
  void Update(Time);
  
  
private:
  
  // Util functions
  
  
  
  // Network configurations affecting the routing table
  
  // Stores the number of destinations and neighbors
  uint32_t n_dst;
  uint32_t n_nb;
  
  map<Ipv4Address, DestinationInfo> dsts;
  
  // For neighbors, it is also important to know the interface
  map<nb_t, NeighborInfo> nbs;
  
  RoutingTableEntry rtable [MAX_DESTINATIONS][MAX_NEIGHBORS];
  
  // The neighbors and destinations get deleted after a while
  // Neighbors, if there are no HelloAnts incoming for a long time
  Time initial_lifetime_nb;
  // Destinations, if they are unused
  Time initial_lifetime_dst;
  
};

}
}
#endif /* ANTHOCNETRTABLE_H */