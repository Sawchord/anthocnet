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

#include "ns3/hash.h"
#include "ns3/simulator.h"
#include "ns3/address.h"
#include "ns3/packet.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"

#define STAT_MAX_PKT_SIZE 2000

namespace ns3 {
namespace ahn {

typedef struct ExpectType {
  Time included;
  Ipv4Address src;
  Ipv4Address dst;
  uint32_t hash;
} expect_type_t;

typedef std::map <Ipv4Address, std::list<expect_type_t> > ExpectList;
typedef std::list<Time> PacketList;



class AntHocNetStat {
public:
  
  AntHocNetStat();
  ~AntHocNetStat();
  
  void Expect(Ipv4Address nextHop, Ipv4Address src, Ipv4Address dst, 
              Ptr<Packet const> packet);
  void Fullfill(Ipv4Address nextHop, Ipv4Address src, Ipv4Address dst, 
                Ptr<Packet const> packet);
  
  void GetExpectingNbs(std::set<Ipv4Address>& nbs);
  
  double GetFullfillmentRate(Ipv4Address nb);
  double GetNumberOfData(Ipv4Address nb);
  
private:
  
  uint8_t buffer[STAT_MAX_PKT_SIZE];
  
  ExpectList expecting;
  std::map<Ipv4Address, PacketList> fullfilled;
  std::map<Ipv4Address, PacketList> unfullfilled;
  
  Time expect_timeout = Seconds(1);
  Time consider_old = Seconds(30);
  uint32_t packets_considered = 30;
};


// End of namespaces
}
}



#endif /* ANTHOCNET_STAT_H */