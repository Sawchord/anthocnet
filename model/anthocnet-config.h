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

#include "ns3/simulator.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

#include "anthocnet-fis.h"

namespace ns3 {
namespace ahn {

class AntHocNetConfig : public Object {
public:
  // ctor
  AntHocNetConfig();
  //dtor
  ~AntHocNetConfig();
  
  
  static TypeId GetTypeId();
  void Print(std::ostream& os) const;
  
  virtual void DoDispose();
protected:
  virtual void DoInitialize();
  
public:
  // Here go all the config members
  // These values will be used troughout the whole module
  
  // ---------------------------------
  // General
  bool snr_cost_metric;
  uint16_t ant_port;
  
  // ---------------------------------
  // Timings
  
  // TODO: Make jitters settable
  // TODO: Make hello counter timeout settable
  
  // Intervals of Timer events
  Time hello_interval;
  Time rtable_update_interval;
  Time pr_ant_interval;
  
  // Timeout of Rtable information
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
  
  double alpha;
  double gamma;
  
  double prog_beta;
  double cons_beta;
  
  double min_pheromone;
  
  double eta_value;
  
  // TODO: Exploration power (settable)
  
  double snr_threshold;
  double snr_malus;
  
  // TODO: Consideration values
  
  // ---------------------------------
  // Misc
  uint8_t initial_ttl;
  
  uint8_t reactive_bcast_count;
  uint8_t proactive_bcast_count;
  
  // -------------------------------------
  // Blackhole mode
  bool blackhole_mode;
  Time blackhole_activation;
  double blackhole_amount;
  bool IsBlackhole();
  
  
  // ----------------------------------------
  // Fuzzy inference system
  Ptr<AntHocNetFis> fis;
  bool fuzzy_mode;
};

}  
}

#endif /* ANTHOCNETCONFIG_H */