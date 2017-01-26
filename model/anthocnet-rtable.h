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

#include "ns3/random-variable-stream.h"
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
  
  // The average hop count
  double avr_hops;
  
  // The virtual pheromone value aquired trough the dissimination part
  double virtual_pheromone;
  
  // Additional information is collected about active sending and receiving
  // Increases, whenever a packet is send trought this connection
  double send_pheromone;
  
  // Increases, whenever a packet is received throught this connection
  double recv_pheromone;
  
};

class NeighborInfo {
public:
  
  // ctor
  NeighborInfo (uint32_t index, Time expire);
  //dtor
  ~NeighborInfo();
  
  // The index into the rtable array
  uint32_t index;
  
  // Neighbors are considered offline, after a certain amount of time without a lifesign and deleted
  Time expires_in;
  
};


class DestinationInfo {
public:
  
  //ctor
  DestinationInfo(uint32_t index, Time expire);
  //dtor
  ~DestinationInfo();
  
  // The index into the rtable array
  uint32_t index;
  
  // After a certain amount of time without usage
  // the node consideres the destination useless and deletes it
  Time expires_in;
  
  // A map of all interfaces on this node, trough that the 
  // Destination can be reached. If this map is not empty, it means the 
  // destination is a neigbor
  std::map<uint32_t, NeighborInfo> nbs;
  
};



class RoutingTable {
public:
  
  //ctor
  RoutingTable(Time nb_expire, Time dst_expire, double T_hop, double gamma);
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
  bool AddNeighbor(uint32_t iface_index, Ipv4Address address, Time expire);
  
  /**
   * \brief Removes a neighbor from the routing table
   * \param iface_index The interface index of
   *        the neighbor to be removed
   * \param address The address of the neighbor to be removed
   */
  void RemoveNeighbor(uint32_t iface_index, Ipv4Address address);
  
  
  
  
  /**
   * @brief Adds a destination to the routing table
   * @arg address The address of the destination
   * @returns True if neighbor was added, false if neighbor already present.
   */
  bool AddDestination(Ipv4Address address);
  bool AddDestination(Ipv4Address address, Time expire);
  
  
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
   * \brief Removes all neighbors that are reachable via the interface specified.
   * \param interface_index The interface to purge
   */
  void PurgeInterface(uint32_t interface_index);
  
  /**
   * \brief Resets the initial expire times of neighbor and
   *        destination entries.
   * \param nb_expire The initial ttl of neighbor entries
   * \param dst_expire The initia ttl of destination entries
   */
  void SetExpireTimes(Time nb_expire, Time dst_expire);
  
  /**
   * \brief Updates a neighbor entry on receiving a HelloAnt
   *        either by adding the neighbors to the table or
   *        resetting its expire time, if it already exist.
   * \param iface_index The interface on which to reach the neighbor/
   * \param address The address of the neighbor.
   */
  void UpdateNeighbor(uint32_t iface_index, Ipv4Address address);
  
  /**
   * \brief Updates the expire times of all neighbors and destinations.
   *        Removes the ones, that are expired.
   * \param interval The time interval, in which this function is called.
   */
  void Update(Time interval);
  
  /**
   * \brief Handles the RoutingTable side of receiving a Backward ant.
   * \param dst The destination of the path that is set up. 
   * \note The destination of the path is the source of the backward ant.
   * \param iface The interface index on which the neigbor sending this ant is.
   * \param nb The neighbor which send the ant to this node.
   * \param T_sd The time value of the and
   * \param hops The hop count of this ant
   * \returns true, if processing was successfull, fail otherwise
   */
  bool ProcessBackwardAnt(Ipv4Address dst, uint32_t iface,
    Ipv4Address nb, uint64_t T_sd, uint32_t hops);
  
  /**
   * \brief Returns a random route to a valid neighbor of this node.
   * \param iface Returns the interface index of this node.
   * \param nb The address of the neighbor
   * \param vr Give access to the uniform random variable
   * \returns true, if a random route was selected, false if no interfaces
   */
  bool SelectRandomRoute(uint32_t& iface, Ipv4Address& nb,
    Ptr<UniformRandomVariable> vr);
  
  /**
   * \brief Stocastically selects an interface and a neighbor from
   *        the routing table to route the packet towards the destination to.
   * \param dst The destination to route the packet to.
   * \param proactive Selects, whether proactive routing is done.
   *        This effects how the probabilities are chosen
   * \param iface References the interface index, to which to route the packet.
   * \param ns Gives the selected neighbor to route the packet towards.
   * \param vr Give the rtable access to the uniform random variable.
   * \return true, if a route was selcted, false, if no route could be 
   *         selected, since there where no enrties for dst.
   */
  bool SelectRoute(Ipv4Address dst, double power,
    uint32_t& iface, Ipv4Address& nb, Ptr<UniformRandomVariable> vr);
      
  void Print(std::ostream& os) const;
  
private:
  
  // Util functions
  
  
  
  // Network configurations affecting the routing table
  
  // Stores the number of destinations and neighbors
  uint32_t n_dst;
  uint32_t n_nb;
  
  double T_hop;
  double gamma_pheromone;
  
  map<Ipv4Address, DestinationInfo> dsts;
  
  // For neighbors, it is also important to know the interface
  //map<nb_t, NeighborInfo> nbs;
  
  RoutingTableEntry rtable [MAX_DESTINATIONS][MAX_NEIGHBORS];
  bool dst_usemap[MAX_DESTINATIONS];
  bool nb_usemap[MAX_NEIGHBORS];
  
  // The neighbors and destinations get deleted after a while
  // Neighbors, if there are no HelloAnts incoming for a long time
  Time initial_lifetime_nb;
  // Destinations, if they are unused
  Time initial_lifetime_dst;
  
  
};

std::ostream& operator<< (std::ostream& os, RoutingTable const& t);

}
}
#endif /* ANTHOCNETRTABLE_H */