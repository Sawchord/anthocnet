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


#include <map>
#include <list>
#include <iomanip>

#include <set>

#include <cmath>

#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/timer.h"
#include "ns3/net-device.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/simulator.h"
#include "ns3/log.h"


#include "anthocnet-packet.h"
#include "anthocnet-config.h"

namespace ns3 {
namespace ahn {
using namespace std;
  


class RoutingTableEntry {
public:
  
  RoutingTableEntry();
  ~RoutingTableEntry();
  
  // The pheromone value of a connection
  double pheromone;
  
  // The virtual pheromone value aquired trough the dissimination part
  double virtual_pheromone;
  
};

class NeighborInfo {
public:
  NeighborInfo ();
  ~NeighborInfo();
  
  Time last_active;
  
  // Average time to send data to this neighbor
  Time avr_T_send;
  
};


class DestinationInfo {
public:
  
  DestinationInfo();
  
  ~DestinationInfo();
  
  Time last_active;
  
  // After a broadcast, a node cannot broadcast out ants to that destination for a certain amount of time
  Time no_broadcast_time;
  
  // Last Time a RouteOutput was called on this destination.
  // Important for the proactive ants
  Time session_time;
  bool session_active;
  
};

typedef std::map<Ipv4Address, DestinationInfo> DstMap;
typedef DstMap::iterator DstIt;

typedef std::map<Ipv4Address, NeighborInfo> NbMap;
typedef NbMap::iterator NbIt;

typedef std::map<std::pair<Ipv4Address, Ipv4Address> > PheromoneTable;
typedef PheromoneTable::iterator PheromoneIt;

class RoutingTable {
public:
  
  //ctor
  RoutingTable(Ptr<AntHocNetConfig> config, Ptr<Ipv4> ipv4);
  //dtor 
  ~RoutingTable();
  
  void AddNeighbor(Ipv4Address nb);
  bool IsNeighbor(Ipv4Address nb);
  bool IsNeighbor(NbIt nb_it);
  void RemoveNeighbor(Ipv4Address nb);
  
  void AddDestination(Ipv4Address dst);
  bool IsDestination(Ipv4Address dst);
  bool IsDestination(DstIt dst_it);
  void RemoveDestination(Ipv4Address dst);
  
  void AddPheromone(Ipv4Address dst, Ipv4Address nb, double pher, double virt_pher);
  void RemovePheromone(Ipv4Address dst, Ipv4Address nb);
  
  void SetPheromone(Ipv4Address dst, Ipv4Address nb, bool virt);
  double GetPheromone(Ipv4Address dst, Ipv4Address nb, bool virt);
  bool HasPheromone(Ipv4Address dst, Ipv4Address nb, bool virt);
  
  void UpdatePheromone(Ipv4Address dst, Ipv4Address nb, double update, bool virt);
  
  std::pair<bool, bool> HasPheromone(Ipv4Address dst, Ipv4Address nb);
  std::pair<bool, bool> HasPheromone(DstIt dst_it, NbIt nb_it);
  
  void RegisterSession(Ipv4Address dst);
  std::list<Ipv4Address> GetSessions();
  
  void ProcessAck(Ipv4Address nb, Time last_hello);
  Time GetTSend(Ipv4Address nb);
  
  bool IsBroadcastAllowed(Ipv4Address address);
  void NoBroadcast(Ipv4Address address, Time duration);
  
  bool SelectRandomRoute(Ipv4Address& nb,
    Ptr<UniformRandomVariable> vr);
  
  bool SelectRoute(Ipv4Address dst, double beta,
    Ipv4Address& nb, Ptr<UniformRandomVariable> vr,
    bool virt);
  
  
  std::set<Ipv4Address> Update(Time interval);
  
  void ProcessNeighborTimeout(LinkFailureHeader& msg 
                              Ipv4Address nb);
  
  
  void ProcessLinkFailureMsg(LinkFailureHeader& msg, 
                             LinkFailureHeader& response,
                             Ipv4Address origin);
  
  
  void ConstructHelloMsg(HelloMsgHeader& msg, uint32_t num_dsts,
                         Ptr<UniformRandomVariable> vr);
  
  
  void HandleHelloMsg(HelloMsgHeader& msg);
  
  
  bool ProcessBackwardAnt(Ipv4Address dst
    Ipv4Address nb, double pheromone, uint32_t hops);
  
  
  
  void SetIpv4(Ptr<Ipv4> ipv4) {
    this->ipv4 = ipv4;
  }
  
  
  //void Print(Ptr<OutputStreamWrapper> stream) const;
  //void Print(std::ostream& os) const;
  
  // Added for initialization
  void SetConfig(Ptr<AntHocNetConfig> config);
  Ptr<AntHocNetConfig> GetConfig() const;
  
private:
  
  // Util functions
  double Bootstrap(double ph_value, double update);
  
  std::pair<bool, double> IsOnly(Ipv4Address dst, Ipv4Address nb);
  
  double SumPropability(Ipv4Address dst, double beta, bool virt);
  
  double EvaporatePheromone(doube ph_value);
  double IncressPheromone(double ph_value, double update);
  
  DstMap dsts;
  NbMap nbs;
  
  PheromoneTable rtable;
  
  
  // The IP protocol
  Ptr<Ipv4> ipv4;
  Ptr<AntHocNetConfig> config;
  
  
};

//std::ostream& operator<< (std::ostream& os, RoutingTable const& t);

}
}
#endif /* ANTHOCNETRTABLE_H */