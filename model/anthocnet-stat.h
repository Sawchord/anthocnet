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
#ifndef ANTHOCNET_STAT_H
#define ANTHOCNET_STAT_H

#include <list>
#include <map>
#include <cmath>

#include "ns3/simulator.h"
#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"

namespace ns3 {
namespace ahn {

typedef std::pair<Ipv4Address, Ipv4Address> SrcDstPair;

typedef struct DstStatType {
  Ipv4Address from;
  Ipv4Address to;
  Time time;
} dst_stat_t;

typedef struct NbRecvStatType {
  Ipv4Address via;
  Time time;
} nb_recv_stat_t;

typedef struct NbSendStatType {
  Ipv4Address via;
  Time time;
} nb_send_stat_t;



class AntHocNetStat {
public:
  
  AntHocNetStat();
  ~AntHocNetStat();
  
  void RegisterTx(Ipv4Address src, Ipv4Address dst, 
                  Ipv4Address nextHop);
  
  void RegisterRx(Ipv4Address prevHop);
  
  double GetTrafficSymmetry();
  
  double GetNbTrafficSymmetry(Ipv4Address nb);
  uint64_t GetNumTrafficEntries(Ipv4Address nb);
  
  
  
private:
  
  void RemoveOutdatedDstEntries();
  void RemoveOutdatedNbEntries();
  
  std::list<nb_recv_stat_t> nb_recv_traffic;
  std::list<nb_send_stat_t> nb_send_traffic;
  
  std::list<dst_stat_t> dst_traffic;
  
  Time nb_timewindow = Seconds(5);
  Time dst_timewindow = Seconds(40);
  
};


// End of namespaces
}
}



#endif /* ANTHOCNET_STAT_H */