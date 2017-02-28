#ifndef ANTHOCNET_CONFIG_H
#define ANTHOCNET_CONFIG_H

#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/log.h"

namespace ns3 {
namespace ahn {

typedef struct ahn_config {
  uint32_t initial_ttl;
  Time hello_interval;
  Time rtable_update_interval;
  Time dcache_expire;
  Time nb_expire;
  Time dst_expire;
  Time no_broadcast;
  double alpha_T_mac;
  double T_hop;
  double alpha_pheromone;
  double gamma_pheromone;
  double avr_T_mac;
  
} ahn_config_t;

// End of namespaces
}
}
#endif /* ANTHOCNET_CONFIG_H */