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

#define NS_LOG_APPEND_CONTEXT                                   \
  if (ipv4) { std::clog << "[node " << std::setfill('0') << std::setw(2) \
    << ipv4->GetObject<Node> ()->GetId () +1 << "] "; }


#include "anthocnet-rtable.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("AntHocNetRoutingTable");
namespace ahn {


RoutingTableEntry::RoutingTableEntry() {
    this->pheromone = 0;
    this->virtual_pheromone = 0;
    
}

RoutingTableEntry::~RoutingTableEntry() {}


// ------------------------------------------------------
DestinationInfo::DestinationInfo() :
  expires_in(Simulator::Now()),
  no_broadcast_time(Seconds(0)),
  session_time(Seconds(0)),
  session_active(false)
  {}

DestinationInfo::~DestinationInfo() {
}


// --------------------------------------------------------
NeighborInfo::NeighborInfo() :
  expires_in(Simulator::Now()),
  avr_T_send(Seconds(0))
  {}

NeighborInfo::~NeighborInfo() {
}



RoutingTable::RoutingTable(Ptr<AntHocNetConfig> config, Ptr<Ipv4> ipv4) :
  ipv4(ipv4),

  config(config)
  {}
  
RoutingTable::~RoutingTable() {
  
}

void RoutingTable::SetConfig(Ptr<AntHocNetConfig> config) {
  this->config = config;
}

Ptr<AntHocNetConfig> RoutingTable::GetConfig() const {
  return this->config;
}


void RoutingTable::AddNeighbor(Ipv4Address nb) {
  if (!this->IsNeighbor(nb)) {
    this->nbs.insert(std::make_pair(nb, NeighborInfo()));
    this->AddDestination(nb);
  }
}

bool RoutingTable::IsNeighbor(Ipv4Address nb) {
  return (this->nbs.find(nb) != this->nbs.end());
}

bool RoutingTable::IsNeighbor(NbIt nb_it) {
  return (nb_it != this->nbs.end());
}

void RoutingTable::RemoveNeighbor(uint32_t iface_index, Ipv4Address address) {
  for (dst_it = this->dsts.begin(); dst_it != this->dsts.end(); ++dst_it) {
    this->RemovePheromone(dst_it->first, nb);
  }
  this->nbs.erase(nb);
}



void RoutingTable::AddDestination(Ipv4Address dst) {
    if (!this->IsDestination(dst)) {
    this->dsts.insert(std::make_pair(dst, DestinationInfo()));

    // TODO: Add initialized pheromone tables here?
    
  }
}

bool RoutingTable::IsDestination(Ipv4Address dst) {
  return (this->dsts.find(dst) != this->dsts.end());
}

bool RoutingTable::IsDestination(DstIt dst_it) {
  return (dst_it != this->dsts.end());
}

void RoutingTable::RemoveDestination(Ipv4Address dst) {
  
  for (nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    this->RemovePheromone(dst, nb_it->first);
  }
  this->dsts.erase(dst);
  
  // TODO: Remove also as neighbor
  
}


void RoutingTable::AddPheromone(Ipv4Address dst, Ipv4Address nb, 
                                double pher, double virt_pher) {
  
  auto p_it = this->rtable.find(std::make_pair(dst, nb));
  
  if (p_it == this->rtable.end()) {
    if (!this->IsDestination(dst) || !this->IsNeighbor(nb))
      return;
    
    p_it = this->rtable.insert(std::make_pair(dst, nb));
    
  }
  
  p_it->second.pheromne = pher;
  p_it->second.virtual_pheromone = virt_pher;
  
}

void RoutingTable::RemovePheromone(Ipv4Address dst, Ipv4Address nb) {
  this->rtable.erase(std::make_pair(dst, nb));
}


void RoutingTable::SetPheromone(Ipv4Address dst, Ipv4Address nb, bool virt) {
  
  auto p_it = this->rtable.find(std::make_pair(dst, nb));
  
  if (p_it == this->rtable.end())
    return;
  
  if (!virt)
    p_it->second.pheromone = pher;
  else
    p_it->seconds.virtual_pheromone = virt_pher;
}

double RoutingTable::GetPheromone(Ipv4Address dst, Ipv4Address nb, bool virt) {
  
  auto p_it = this->rtable.find(std::make_pair(dst, nb));
  
  if (p_it == this->rtable.end())
    return;
  
  if (!virt)
    return p_it->second.pheromone;
  else
    return p_it->seconds.virtual_pheromone;
}


void RoutingTable::UpdatePheromone(Ipv4Address dst, Ipv4Address nb, 
                                   double update, bool virt) {
  
  
  
  
}

void RoutingTable::RegisterSession(Ipv4Address dst) {
  auto dst_it = this->dsts.find(dst);
  if (this->IsDestination(dst_it)) {
    dst_it->second.session_time = Simulator::Now();
    dst_it->second.session_active = true;
  }
}

std::list<Ipv4Address> RoutingTable::GetSessions() {
  
  std::list<Ipv4Address> ret;
  for (auto dst_it = this->dsts.begin(); 
       dst_it != this->dsts.end(); ++dst_it){
    
    if (dst_it->second.session_active &&
        Simulator::Now() - dst_it->second.session_time 
          < this->config->session_expire){
      
      ret.push_back(dst_it->first);
    
    }
    else {
      dst_it->second.session_active = false;
    }
  }
  
  return ret;
}

bool RoutingTable::IsBroadcastAllowed(Ipv4Address dst) {
  
  // Check if destination exists
  auto dst_it = this->dsts.find(dst);
  if (!this->IsDestination(dst_it)) {
    this->AddDestination(dst);
    return false;
  }
  
  if (Simulator::Now() <= dst_it->second.no_broadcast_time) {
    NS_LOG_FUNCTION(this << "no bcast to" 
      << dst << " for " 
      << (dst_it->second.no_broadcast_time - Simulator::Now()).GetMilliSeconds());
    return false;
  }
  
  return true;
}

void RoutingTable::ProcessAck(Ipv4Address nb, Time last_hello) {
  
  // If we get an ack without a Hello, what do do?
  // Be happy or suspicios?
  
  auto nb_it = this->nbs.find(nb);
  if (this->IsNeighbor(nb_it)) {
    return;
  }
  
  Time delta = Simulator::Now() - last_hello;
  Time avr = nb_it->second.avr_T_send;
  
  if (avr == Seconds(0)) {
    nb_it->second.avr_T_send = delta;
  }
  else {
    nb_it->second.avr_T_send = 
      NanoSeconds(this->config->eta_value * avr.GetNanoSeconds()) +
      NanoSeconds((1.0 - this->config->eta_value) * delta.GetNanoSeconds());
  }
  
  NS_LOG_FUNCTION(this << "nb" << nb << 
    << "new avr_T_send" << nb_it->second.avr_T_send.GetMicroSeconds());
  
}

Time RoutingTable::GetTSend(Ipv4Address nb, uint32_t iface) {
  
  auto nb_it = this->nbs.find(nb);
  if (this->IsNeighbor(nb_it)) {
    return Seconds(0);
  }
  
  return nb_it->second.avr_T_send;
}

void RoutingTable::NoBroadcast(Ipv4Address dst, Time duration) {
  
  auto dst_it = this->dsts.find(dst);
  if (this->IsDestination(dst_it)) {
    this->AddDestination(dst);
    dst_it = this->dsts.find(dst);
  }
  
  dst_it->second.no_broadcast_time = Simulator::Now() + duration;
  
  return;
}

void RoutingTable::UpdateNeighbor(Ipv4Address nb) {
  
  auto nb_it = this->nbs.find(nb);
  if (!this->IsNeighbor(nb_it))
    return;
  
  nb_it->second.last_active = Simulator::Now();
  
  
}

// TODO: Remove Interval parameter, use config interval thing instead??
// TODO: Handle ret table as refenerence for speadup
std::set<Ipv4Address> RoutingTable::Update(Time interval) {
  
  
  
  std::set<Ipv4Address> ret;
  
  for (dst_it = this->dsts.begin(); dst_it != this->dsts.end(); ++dst_it) {
    if (nb_it->second.last_active + interval) < Simulator::Now()) {
      NS_LOG_FUNCTION(this << "dst" << nb_it->first << "timed out");
      this->RemoveDestination(dst_it->first);
    }
  }
  
  
  for (nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    if (nb_it->second.last_active + interval) < Simulator::Now()) {
      NS_LOG_FUNCTION(this << "nb" << nb_it->first << "timed out");
      
      ret.insert(nb_it->first);
      
    }
  }
  
  for (p_it = this->rtable.begin(); p_it != this->rtable.end(); ++p_it) {
    // NOTE: If use time based evaportation, use it here
  }
  
  return ret;
}

bool RoutingTable::SelectRoute(Ipv4Address dst, double beta,
                               Ipv4Address& nb,  Ptr<UniformRandomVariable> vr,
                               bool virt){
  
  
  // Check if destination is a neighbor
  auto temp_nb_it = this->nbs.find(dst);
  if (temp_nb_it != this->nbs.end()) {
    NS_LOG_FUNCTION(this << "dst" << dst << "is nb" << nb 
     << "usevirt" << consider_virtual);
    return true;
  }
  
  
  //Get the destination index:
  auto dst_it = this->dsts.find(dst);
  
  // Fail, if there are no entries to that destination at all
  if (dst_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst does not exist" << dst);
    return false;
  }
  
  double total_pheromone = this->SumPropability(dst, beta, consider_virtual)
  
  // NOTE: Very low pheromone can lead to the total_pheromone 
  // beeing rounded down to Zero. (When used with a high power) 
  // This leads to the system acting as if
  // there is no pheromone at all. This is most likely not an intended
  // behaviour and there should be a case to handle that
  
  // Fail, if there are no initialized entries (same as no entires at all)
  if (total_pheromone < this->config->min_pheromone) {
    NS_LOG_FUNCTION(this << "no initialized nbs");
    return false;
  }
  
  // To select with right probability, a random uniform variable between 
  // 0 and 1 is generated, then it iterates over the neighbors, calculates their
  // probability and adds it to an aggregator. If the aggregator gets over the 
  // random value, the particular Neighbor is selected.
  double select = vr->GetValue(0.0, 1.0);
  double selected = 0.0;
  
  NS_LOG_FUNCTION(this << "total_pheromone" << total_pheromone);
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    auto p_it = this->rtable.find(std::make_pair(dst, nb_it->first));
    
    if (p_it == this->rtable.end())
      continue;
    
    // TODO: Consider virtual_malus
    if (virt && p_it->second.virtual_pheromone > p_it->second.pheromone) {
      selected += pow(p_it->second.virtual_pheromone, beta)/ total_pheromone);
    } else {
      selected += pow(p_it->second.pheromone, beta)/ total_pheromone);
    }
    
    if (selected > select) {
      nb = nb_it->first;
      break;
    }
    
  }
  
  // Never come here
  NS_LOG_FUNCTION(this << "never come here");
  return false;
}


bool RoutingTable::SelectRandomRoute(Ipv4Address& nb,
                                     Ptr<UniformRandomVariable> vr) {
  
  if (this->nbs.size() == 0) {
    return false;
  }
  
  uint32_t select = vr->GetInteger(0, this->nbs.size());
  uint32_t counter = 0;
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    if (counter == select) {
      nb = nb_it->first;
      NS_LOG_FUNCTION(this <<  "nb" << nb);
      return true;
    }
    
    counter++;
  }
  
  // Never come here
  return false;
}

/*void RoutingTable::ProcessNeighborTimeout(LinkFailureHeader& msg, 
                                          uint32_t iface, 
                                          Ipv4Address address) {
  
  auto brklink_it = this->dsts.find(address);
  auto brknb_it = brklink_it->second.nbs.find(iface);
  uint32_t brknb_index = brknb_it->second.index;
  
  // Iterate over all destinations
  for (auto dst_it = this->dsts.begin();
       dst_it != this->dsts.end(); ++ dst_it) {
    
    uint32_t dst_index = dst_it->second.index;
    
    // Find those with non nan pheromones over the broken link
    if (this->rtable[dst_index][brknb_index].pheromone !=
      this->rtable[dst_index][brknb_index].pheromone ) {
      continue;
    }
    
    // See, wether address iface was the only or the best route to dst
    auto other_inits = this->IsOnly(dst_it->first, address, iface);
    
    // If a connection over the broken link is found, it needs to be included
    // in the updates list
    double brk_pheromone = this->rtable[dst_index][brknb_index].pheromone;
    
    //NS_LOG_FUNCTION(this << "second" << other_inits.second << "broken" << brk_pheromone);
    
    // Now report our findings into the message
    if (!other_inits.first) {
      msg.AppendUpdate(dst_it->first, ONLY_VALUE, 0.0);
    }
    else if (other_inits.second < brk_pheromone) {
      msg.AppendUpdate(dst_it->first, NEW_BEST_VALUE, other_inits.second);
    }
    else {
     msg.AppendUpdate(dst_it->first, VALUE, 0.0); 
    }
  }
  
  NS_LOG_FUNCTION(this << "NB Timeout: " << msg);
  
  // After constructing the message, we can remove the neighbor
  this->RemoveNeighbor(iface, address);
  
  return;
}*/

/*void RoutingTable::ProcessLinkFailureMsg (LinkFailureHeader& msg,
                                          LinkFailureHeader& response,
                                          Ipv4Address origin,
                                          uint32_t iface){
  
  NS_ASSERT(msg.GetSrc() == origin);
  NS_ASSERT(msg.HasUpdates());
  
  NS_LOG_FUNCTION(this << "Processing::" << msg);
  
  // Need to check if we really have this neighbor as neighbor
  // If not just skip, since we do not have routes over this
  // node anyway
  auto linknb_it = this->dsts.find(origin);
  if (linknb_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << origin << "not neighbor");
    return; 
  }
    
  auto linkif_it = linknb_it->second.nbs.find(iface);
  if (linkif_it == linknb_it->second.nbs.end()) {
    NS_LOG_FUNCTION(this << iface << "iface not registered");
    return; 
  }
  
  uint32_t linkif_index = linkif_it->second.index;
  
  
  // Now we evaluate the update list from this message
  while (msg.HasUpdates()) {
    
    linkfailure_list_t l = msg.GetNextUpdate();
    auto dst_it = this->dsts.find(l.dst);
    if (dst_it == this->dsts.end()) {
      // Skip the destinations, we do not know about
      // since we do not have information that could be outdated anyway
      continue;
    }
    
    uint32_t dst_index = dst_it->second.index;
    auto other_inits = this->IsOnly(dst_it->first, origin, iface);
    
    double old_pheromone = 0.0;
    double new_pheromone = 0.0;
    
    switch (l.status) {
      case VALUE:
        
        // DO nothing ???
        
        break;
      case ONLY_VALUE:
        
        // The route via linknb to dst is now broken
        // If it was broken before, no need to update
        if (this->rtable[dst_index][linkif_index].pheromone !=
          this->rtable[dst_index][linkif_index].pheromone) {
          continue;
        }
        
        
        // Check for alternatives
        if (!other_inits.first) {
          // No alternatives, full breakage
          response.AppendUpdate(l.dst, ONLY_VALUE, 0.0);
        }
        else if (this->rtable[dst_index][linkif_index].pheromone 
          > other_inits.second){
          // Best pheromone become invalid, send new one
          response.AppendUpdate(l.dst, NEW_BEST_VALUE, other_inits.second);
        }
        
        this->rtable[dst_index][linkif_index].pheromone = NAN;
        this->rtable[dst_index][linkif_index].avr_hops = NAN;
        
        // TODO: set virt pheromone nan
        // Happens to be worse
        //this->rtable[dst_index][linkif_index].virtual_pheromone = NAN;
        break;
        
      case NEW_BEST_VALUE:
        
        // If the route reported to have a new best value did not have any value
        // before, can bootstrap it, but we must not publish our found, since it leads
        // to an infinite message loop
        if (this->rtable[dst_index][linkif_index].pheromone !=
          this->rtable[dst_index][linkif_index].pheromone) {
          new_pheromone = this->Bootstrap(l.dst, origin, iface, l.new_pheromone, false);
          continue;
        }
        
        old_pheromone = this->rtable[dst_index][linkif_index].pheromone;
        
        // Use Bootstrap algorithm
        new_pheromone = this->Bootstrap(l.dst, origin, iface, l.new_pheromone, false);
        
        // If the updates pheromone was not the best to begin with, there is no need to 
        // inform the other nodes
        if (old_pheromone < other_inits.second) {
          continue;
        }
        
        if (new_pheromone < other_inits.second) {
          response.AppendUpdate(l.dst, NEW_BEST_VALUE, other_inits.second);
        }
        
        break;
      default:
        break;
      
    }
    
  }
  
  NS_LOG_FUNCTION(this << "Response::" << response);
  return;
}*/

/*
void RoutingTable::ConstructHelloMsg(HelloMsgHeader& msg, uint32_t num_dsts, 
                                     Ptr<UniformRandomVariable> vr) {
  
  // NOTE: This is extremly inefficient.
  // Maybe consider to add additional data into
  // RoutingTable, to make a full iteration faster.
  
  bool use_random = true;
  
  // If there are less known destinations than requested,
  // there is no need to select some randomly
  if (this->n_dst <= num_dsts) {
    use_random = false;
  }
  
  std::list<std::pair<Ipv4Address, double> > selection;
  
  // Iterate over all possible connections
  for (auto dst_it = this->dsts.begin();
       dst_it != this->dsts.end(); ++dst_it) {
    
    Ipv4Address best_dst = dst_it->first;
    double best = NAN;
    
    // Iterate over all neighbors
    // to get all destinations best nb candidates
    for (auto nb_it1 = this->dsts.begin(); 
         nb_it1 != this->dsts.end(); ++nb_it1) {
      
      for (auto nb_it2 = nb_it1->second.nbs.begin();
           nb_it2 != nb_it1->second.nbs.end(); ++nb_it2) {
        
        uint32_t dst_idx = dst_it->second.index;
        uint32_t nb_idx = nb_it2->second.index;
	  
        // Check the real pheromone value
        if (std::abs(best) <= this->rtable[dst_idx][nb_idx].pheromone
            || best != best) {// check for nan
          best = this->rtable[dst_idx][nb_idx].pheromone;
        }
        
        // Check the virtual pheromone
        // We mark virtual pheromone by a negative value instead of flag
        if (std::abs(best) <= this->rtable[dst_idx][nb_idx].virtual_pheromone){
          best = -1.0 * this->rtable[dst_idx][nb_idx].virtual_pheromone;
        }
      } 
    }
    
    
    // Check if still NAN
    if (best == best) {
      selection.push_back(std::make_pair(best_dst, best));
    }
    
  }
  
  // Now select some of the pairs we found
  for (uint32_t i = 0; i < num_dsts; i++) {
    
    if (selection.empty()) {
      break;
    }
    
    uint32_t select;
    if (use_random) {
      select = std::floor(vr->GetValue(0.0, selection.size()));
    }
    else {
      select = 0;
    }
    
    //NS_LOG_FUNCTION(this << "select" << select << "ndst" << selection.size());
    
    // Get to the selection
    auto sel_it = selection.begin();
    for (uint32_t c = select; c > 0; c--) {
      sel_it++;
    }
    
    //double selected = sel_it->second;
    //NS_LOG_FUNCTION(this << "selected" << sel_it->first << selected);
    msg.PushDiffusion(sel_it->first, sel_it->second);
    selection.erase(sel_it);
    
  }
  
  
}*/

/*
void RoutingTable::HandleHelloMsg(HelloMsgHeader& msg, uint32_t iface) {
  
  if(!msg.IsValid()) {
    NS_LOG_FUNCTION(this << "Malformed HellMsg -> dropped");
    return;
  }
  
  // Bootstrap information for every possible destination
  while (msg.GetSize() != 0) {
    
    auto pos_dst = msg.PopDiffusion();
    
    // Get destination or add if not yet exist
    this->AddDestination(pos_dst.first);
    this->AddDestination(msg.GetSrc());
  
    // Get iterators to the destination and the neigbor
    auto dst_it = this->dsts.find(pos_dst.first);
    auto nb_it1 = this->dsts.find(msg.GetSrc());
    
    this->AddNeighbor(iface, msg.GetSrc());
    auto nb_it2 = nb_it1->second.nbs.find(iface);
    
    uint32_t dst_index = dst_it->second.index;
    uint32_t nb_index = nb_it2->second.index;
    
    bool is_virtual = false;
    
    if (pos_dst.second < 0) {
      is_virtual = true;
      pos_dst.second *= -1.0;
    }
    
    double T_id = this->GetTSend(pos_dst.first, iface).GetMilliSeconds();
    
    double bootstrap_info = (1.0/ (1.0/(pos_dst.second) + T_id));
    NS_LOG_FUNCTION(this << "bootstapped" 
      << bootstrap_info << "from" << pos_dst.second 
      << "dst" << pos_dst.first << "nb" << msg.GetSrc()
      << "dst_in" << dst_index << "nb_index" << nb_index
      << "virt" << is_virtual);
      
    RoutingTableEntry* ra = &this->rtable[dst_index][nb_index];
    
    // TODO: Add special case where real pheromone is used
    // TODO: Use bootstrap utility function here after testing it
    
    if (ra->virtual_pheromone != ra->virtual_pheromone) {
      ra->virtual_pheromone = bootstrap_info;
    }
    else {
      ra->virtual_pheromone = 
        this->config->alpha_pheromone*ra->virtual_pheromone +
        (1.0 - this->config->alpha_pheromone) * (bootstrap_info);
    }
    
  }
}*/
/*

bool RoutingTable::ProcessBackwardAnt(Ipv4Address dst, uint32_t iface,
  Ipv4Address nb, uint64_t T_sd, uint32_t hops) {
    
  
  NS_LOG_FUNCTION(this << "dst" << dst << "iface" 
    << iface << "nb" << nb << "T_sd" << T_sd << "hops" << hops);
  // First search the destination and add it if it did not exist.
   // Check if destination already exists
  auto dst_it = this->dsts.find(dst);
  if (dst_it == this->dsts.end()) {
    this->AddDestination(dst);
    dst_it = this->dsts.find(dst);
  }
  
  // Find the neighbors iterators
  auto nb_it = this->dsts.find(nb);
  if (nb_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "nb not in reach -> Ant dropped");
    return false;
  }
  
  auto nbif_it = nb_it->second.nbs.find(iface);
  if (nbif_it == nb_it->second.nbs.end()) {
    NS_LOG_FUNCTION(this << "interface not found -> Ant dropped");
    return false;
  }
  
  // Since both, the Neighbor and the Destination are found active,
  // reset their expiration dates.
  nb_it->second.expires_in = this->config->nb_expire;
  dst_it->second.expires_in = this->config->dst_expire;
  
  
  // Get the indexes into the pheromone of dst and nb table
  uint32_t nb_index = nbif_it->second.index;
  uint32_t dst_index = dst_it->second.index;
  
  // NOTE: This is the cost function.
  // One could get crazy here and have some 
  // really cool functions
  
  double T_id = (( ((double)T_sd / 1000000) + hops * this->config->T_hop) / 2);
  
  // Update the routing table
  RoutingTableEntry* ra = &this->rtable[dst_index][nb_index];
  
  // Check if hop count value is NAN
  if (ra->avr_hops != ra->avr_hops) {
    ra->avr_hops = hops;
  }
  else {
    ra->avr_hops = this->config->alpha_pheromone*ra->avr_hops +
      (1.0 - this->config->alpha_pheromone) * hops;
  }
  
  // Check if pheromone value is NAN
  if (ra->pheromone != ra->pheromone) {
    ra->pheromone = (1.0 / T_id);
  }
  else {
    ra->pheromone = 
      this->config->gamma_pheromone*ra->pheromone +
      (1.0 - this->config->gamma_pheromone) * (1.0 / T_id);
  }
  
  //this->UpdatePheromone(dst, nb, iface, T_id, hops);
  
  NS_LOG_FUNCTION(this << "updated pheromone" << ra->pheromone 
    << "average hops" << ra->avr_hops
    << "for" << dst_it->first << nb_it->first);
  
  return true;
}
*/


/*
double RoutingTable::Bootstrap(Ipv4Address dst,
                            Ipv4Address nb, uint32_t iface,
                            double pheromone, bool use_virtual) {
  
  NS_LOG_FUNCTION(this << dst << nb << iface << pheromone);
  
  auto dst_it = this->dsts.find(dst);
  auto nb1_it = this->dsts.find(nb);
  
  if (dst_it == this->dsts.end() || nb1_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst or nb not found");
    return NAN;
  }
  
  auto nb2_it = nb1_it->second.nbs.find(iface);
  if (nb2_it == nb1_it->second.nbs.end()) {
    NS_LOG_FUNCTION(this << "iface not found");
    return NAN;
  }
  
  uint32_t dst_index = dst_it->second.index;
  uint32_t nb_index = nb2_it->second.index;
  RoutingTableEntry* ra = &this->rtable[dst_index][nb_index];
  
  double T_id = this->GetTSend(nb, iface).GetMilliSeconds();
  double bootstrap = 1.0/(1.0/(pheromone) + T_id);
  
  double result;
  
  if (!use_virtual) {
    if (ra->pheromone != ra->pheromone) {
      result = ra->pheromone = bootstrap;
    }
    else {
      result = ra->pheromone = 
        this->config->gamma_pheromone*ra->pheromone +
        (1.0 - this->config->gamma_pheromone) * (bootstrap);
    }
  }
  else {
    if (ra->virtual_pheromone != ra->virtual_pheromone) {
      result = ra->virtual_pheromone = bootstrap;
    }
    else {
      result = ra->virtual_pheromone = 
        this->config->alpha_pheromone*ra->virtual_pheromone +
        (1.0 - this->config->alpha_pheromone) * (bootstrap);
    }
    // TODO: There is a special case where the real pheromones are used
  }
  
  return result;
}*/

/*
std::pair<bool, double> RoutingTable::IsOnly(Ipv4Address dst, 
                                             Ipv4Address nb, uint32_t iface) {
  
  auto dst_it = this->dsts.find(dst);
  
  if (dst_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst  not found");
    return std::make_pair(false, NAN);
  }
  
  uint32_t dst_index = dst_it->second.index;
  
  double best_pheromone = 0.0;
  bool other_inits = false;
  
  // Now iterate over all neighbors if there are other 
  // initilized pheromone values
  for (auto nb1_it = this->dsts.begin(); 
        nb1_it != this->dsts.end(); ++nb1_it) {
    
    // Skip the dst_address itself
    if (nb1_it->first == dst_it->first) {
      continue;
    }
    
    for (auto nb2_it = nb1_it->second.nbs.begin();
      nb2_it != nb1_it->second.nbs.end(); ++nb2_it) {
      
      if (nb1_it->first == nb && nb2_it->first == iface) {
        continue;
      }
    
      // Check if there are other initilized interfaces 
      // for this destination
      uint32_t nb_index = nb2_it->second.index;
      if (this->rtable[dst_index][nb_index].pheromone ==
        this->rtable[dst_index][nb_index].pheromone) {
        
        other_inits = true;
        // Check, if they are better
        if (this->rtable[dst_index][nb_index].pheromone > best_pheromone) {
          best_pheromone = this->rtable[dst_index][nb_index].pheromone;
        }  
      }
    }
  }
  
  NS_ASSERT((other_inits && best_pheromone != 0) || (!other_inits));
  
  return std::make_pair(other_inits, best_pheromone);
}*/


double RoutingTable::SumPropability(Ipv4Address dst, double beta, bool virt) {
  
  double Sum = 0;
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    auto p_it = this->rtable.find(std::make_pair(dst, nb_it->first));
    
    if (p_it == this->rtable.end())
      continue;
    
    // TODO: Add consideration value
    if (virt) {
      if (p_it->second.virtual_pheromone > p_it->second.pheromone)
        Sum += pow(p_it->second.virtual_pheromone, beta);
      else
        Sum += pow(p_it->second.pheromone, beta);
    }
    else {
      Sum += pow(p_it->second.pheromone, beta);
    }
    
  }
  
}

double RoutingTable::EvaporatePheromone(doube ph_value) {
  return ph_value - (1- this->config->alpha) * ph_value;
}

double RoutingTable::IncressPheromone(double ph_value, double update) {
  return (this->config->gamma * ph_value + (1 - this->config->gamma) + update);
}

/*void RoutingTable::Print(Ptr<OutputStreamWrapper> stream) const {
  this->Print(*stream->GetStream());
}

// FIXME: This function causes program to crash (when used in NS_LOG_FUNCTION)
void RoutingTable::Print(std::ostream& os) const{
  
  for (auto dst_it1 = this->dsts.begin();
       dst_it1 != this->dsts.end(); ++dst_it1) {
    
    // Output the destination info 
    //os << " DST:[" << dst_it1->first << " : " << dst_it1->second.index ;
    os << " DST:[" << dst_it1->first;
    
    
    if (dst_it1->second.nbs.size() != 0) {
      
      os << " NB:( ";
      
      for (auto nb_it = dst_it1->second.nbs.begin();
           nb_it != dst_it1->second.nbs.end(); ++nb_it) {
      
        os << nb_it->first << " ";
      }
      os << ")";
    }
    os << "]";
  }
  
  os << std::endl;
  
  // Print the pheromone table
  for (auto dst_it1 = this->dsts.begin();
       dst_it1 != this->dsts.end(); ++dst_it1) {
    
    os << dst_it1->first << ":";
    
    // Iterate over all neigbors
    for (auto dst_it2 = this->dsts.begin();
         dst_it2 != this->dsts.end(); ++dst_it2) {
      
      for (auto nb_it = dst_it2->second.nbs.begin();
           nb_it != dst_it2->second.nbs.end(); ++nb_it) {
        
        uint32_t dst_idx = dst_it1->second.index;
        uint32_t nb_idx = nb_it->second.index;
        
        os << "(" << dst_it2->first << ":" << nb_it->first << "):";
        os << this->rtable[dst_idx][nb_idx].pheromone << "|";  
        os << this->rtable[dst_idx][nb_idx].avr_hops << "|";
        os << this->rtable[dst_idx][nb_idx].virtual_pheromone;
        os << "->(" << dst_idx << ":" << nb_idx << ")\t";
        }
    }
    
    os << std::endl;
    
  }
  
}

std::ostream& operator<< (std::ostream& os, RoutingTable const& t) {
  t.Print(os);
  return os;
}*/

}
}