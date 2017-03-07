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

#ifndef ANTHOCNETCONFIG_H
#define ANTHOCNETCONFIG_H

#include <stdint.h>

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/attribute.h"
#include "ns3/log.h"

#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"

namespace ns3 {
namespace ahn {

class AntHocNetConfig : public Object {
public:
  // ctor
  AntHocNetConfig();
  //dtor
  ~AntHocNetConfig();
  
  
  static TypeId GetTypeId();
  
  virtual void DoDispose();
protected:
  virtual void DoInitialize();
  
public:
  // TODO: Add descriptions
  // Here go all the config members
  
  // ---------------------------------
  // General
  uint16_t ant_port;
  
  
  // ---------------------------------
  // Timings
  
  // Intervals of Timer events
  Time hello_interval;
  Time rtable_update_interval;
  Time pr_ant_interval;
  
  // Timeout of Rtable information
  Time dst_expire;
  Time nb_expire;
  Time session_expire;
  Time dcache_expire;
  // Time after a broadcast, in which no other broadcast to 
  // same destination is allowed.
  Time no_broadcast;
  
  
  // ---------------------------------
  // Pheromone calculation
  double alpha_T_mac;
  double T_hop;
  double alpha_pheromone;
  double gamma_pheromone;
  double eta_value;
  
  double snr_threshold;
  double bad_snr_cost;
  
  // TODO: Consideration values
  
  // ---------------------------------
  // Misc
  uint8_t initial_ttl;
  
  // TODO: Broadcast ttls
  
};

}  
}

#endif /* ANTHOCNETCONFIG_H */