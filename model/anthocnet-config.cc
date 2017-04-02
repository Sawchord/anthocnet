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

#include "anthocnet-config.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("AntHocNetConfig");
namespace ahn {

AntHocNetConfig::AntHocNetConfig() {
}
AntHocNetConfig::~AntHocNetConfig() {
}


NS_OBJECT_ENSURE_REGISTERED(AntHocNetConfig);
TypeId AntHocNetConfig::GetTypeId() {
  static TypeId tid = TypeId("ns3::ahn::AntHocNetConfig")
  .SetParent<Object> ()
  .SetGroupName("AntHocNet")
  .AddConstructor<AntHocNetConfig>()
  
  .AddAttribute ("SnrCostMetric",
    "Specify, wether SNR or Turnaround-Time should be used as cost metric",
    BooleanValue(true),
    MakeBooleanAccessor(&AntHocNetConfig::snr_cost_metric),
    MakeBooleanChecker()
  )
  .AddAttribute ("Port",
    "The port, the protocol uses to exchange control messages.",
    UintegerValue(5555),
    MakeUintegerAccessor(&AntHocNetConfig::ant_port),
    MakeUintegerChecker<uint16_t>()
  )
  .AddAttribute ("HelloInterval",
    "HELLO messages emission interval.",
    TimeValue (Seconds(1)),
    MakeTimeAccessor(&AntHocNetConfig::hello_interval),
    MakeTimeChecker()
  )
  .AddAttribute ("RTableUpdate",
    "The interval, in which the RoutingTable is updated.",
    TimeValue (MilliSeconds(1000)),
    MakeTimeAccessor(&AntHocNetConfig::rtable_update_interval),
    MakeTimeChecker()
  )
  .AddAttribute ("ProactiveAntTimer",
    "The interval, in which an active session sends out proactive ants",
    TimeValue (MilliSeconds(1000)),
    MakeTimeAccessor(&AntHocNetConfig::pr_ant_interval),
    MakeTimeChecker()
  )
  .AddAttribute ("NeighborExpire",
    "Time without HelloAnt, after which a neighbor is considered offline.",
    TimeValue (MilliSeconds(2500)),
    MakeTimeAccessor(&AntHocNetConfig::nb_expire),
    MakeTimeChecker()
  )
  .AddAttribute ("SessionExpire",
    "Time without outbound traffic, after a session is considered over",
    TimeValue (Seconds(10)),
    MakeTimeAccessor(&AntHocNetConfig::session_expire),
    MakeTimeChecker()
  )
  .AddAttribute ("DataCacheExpire",
    "Time data packets wait for route to be found. Dropped if expires",
    TimeValue (MilliSeconds(5000)),
    MakeTimeAccessor(&AntHocNetConfig::dcache_expire),
    MakeTimeChecker()
  )
  .AddAttribute ("NoBroadcast",
    "Time after broadcast,where no broadcast is allowed to same destination",
    TimeValue (MilliSeconds(100)),
    MakeTimeAccessor(&AntHocNetConfig::no_broadcast),
    MakeTimeChecker()
  )
  .AddAttribute("AlphaTMac",
    "The alpha value of the running average of T_mac.",
    DoubleValue(0.7),
    MakeDoubleAccessor(&AntHocNetConfig::alpha_T_mac),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("THop",
    "The THop heuristic used to calculate initial pheromone values",
    DoubleValue(0.2),
    MakeDoubleAccessor(&AntHocNetConfig::T_hop),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("Alpha",
    "The hop count uses a running average with a decay defined by alpha",
    DoubleValue(0.7),
    MakeDoubleAccessor(&AntHocNetConfig::alpha),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("Gamma",
    "The pheromone uses a running average with a decay defined by gamma",
    DoubleValue(0.7),
    MakeDoubleAccessor(&AntHocNetConfig::gamma),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("ProgressiveBeta",
    "Beta value used by proactive ants",
    DoubleValue(2),
    MakeDoubleAccessor(&AntHocNetConfig::prog_beta),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("ConservativeBeta",
    "BetaValue used by data and reactive ants",
    DoubleValue(20),
    MakeDoubleAccessor(&AntHocNetConfig::cons_beta),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("MinPheroone",
    "The pheromone below this value are consiered zero",
    DoubleValue(0.0001),
    MakeDoubleAccessor(&AntHocNetConfig::min_pheromone),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("EtaValue",
    "The Avr Rx Time (used in bootstrap algorithm) is a running average with a decay defined by eta",
    DoubleValue(0.7),
    MakeDoubleAccessor(&AntHocNetConfig::eta_value),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("SnrThreshold",
    "Connections with a SNR below this value are considered bad connections",
    DoubleValue(17.0),
    MakeDoubleAccessor(&AntHocNetConfig::snr_threshold),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("SnrMalus",
    "The malus of a bad connection",
    DoubleValue(3.0),
    MakeDoubleAccessor(&AntHocNetConfig::snr_malus),
    MakeDoubleChecker<double>()
  )
  .AddAttribute("InitialTTL",
    "The TTL value of a newly generated Ant.",
    UintegerValue(16),
    MakeUintegerAccessor(&AntHocNetConfig::initial_ttl),
    MakeUintegerChecker<uint8_t>()
  )
  .AddAttribute("ReactiveBroadcastTTL",
    "The number of times, a reactive ant can be broadcasted",
    UintegerValue(10),
    MakeUintegerAccessor(&AntHocNetConfig::reactive_bcast_count),
    MakeUintegerChecker<uint8_t>()
  )
  .AddAttribute("ProactiveBroadcastTTL",
    "The number of times, a proactive ant can be broadcasted",
    UintegerValue(1),
    MakeUintegerAccessor(&AntHocNetConfig::proactive_bcast_count),
    MakeUintegerChecker<uint8_t>()
  )
  // Blackhole mode configuration
  .AddAttribute ("BlackholeMode",
    "If set true, blackhole mode is activaed",
    BooleanValue(false),
    MakeBooleanAccessor(&AntHocNetConfig::blackhole_mode),
    MakeBooleanChecker()
  )
  .AddAttribute ("BlackholeActivation",
    "Time at which the blackhole shall become active",
    TimeValue (Seconds(35)),
    MakeTimeAccessor(&AntHocNetConfig::blackhole_activation),
    MakeTimeChecker()
  )
  .AddAttribute("BlackholeAmount",
    "Percentage of Data drop to conduct.",
    DoubleValue(1.0),
    MakeDoubleAccessor(&AntHocNetConfig::blackhole_amount),
    MakeDoubleChecker<double>()
  )
  ;
  return tid;
}

bool AntHocNetConfig::IsBlackhole() {
  return (this->blackhole_mode 
    && (this->blackhole_activation < Simulator::Now()));
}

void AntHocNetConfig::DoDispose() {
  
}

void AntHocNetConfig::DoInitialize() {
  
}

void AntHocNetConfig::Print(std::ostream& os) const {
  
  //os << "value: " << value << std::endl;
  
  os << "hello_interval: " << hello_interval << std::endl;
  os << "rtable_update_interval: " << rtable_update_interval << std::endl;
  os << "pr_ant_interval: " << pr_ant_interval << std::endl;
  
  os << "nb_expire: " << nb_expire << std::endl;
  os << "session_expire: " << session_expire << std::endl;
  os << "dcache_expire: " << dcache_expire << std::endl;
  
  os << "no_broadcast: " << no_broadcast << std::endl;
  
  os << "alpha_T_mac: " << alpha_T_mac << std::endl;
  os << "T_hop: " << T_hop << std::endl;
  
  os << "alpha: " << alpha << std::endl;
  os << "gamma: " << gamma << std::endl;
  
  os << "prog_beta: " << prog_beta << std::endl;
  os << "cons_beta: " << cons_beta << std::endl;
  
  os << "min_pheromone: " << min_pheromone << std::endl;
  
  os << "eta_value: " << eta_value << std::endl;
  
  os << "initial_ttl: " << initial_ttl << std::endl;
  
  os << "reactive_bcast_count: " << reactive_bcast_count << std::endl;
  os << "proactive_bcast_count: " << proactive_bcast_count << std::endl;
  
}





// End of namespaces
}
}