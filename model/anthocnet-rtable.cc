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
    this->pheromone = NAN;
    this->avr_hops = NAN;
    this->virtual_pheromone = NAN;
    this->send_pheromone = NAN;
    this->recv_pheromone = NAN;
    
}

RoutingTableEntry::~RoutingTableEntry() {}


// ------------------------------------------------------
DestinationInfo::DestinationInfo(uint32_t index, Time expire) :
  index(index),
  expires_in(expire),
  no_broadcast_time(Seconds(0)),
  session_time(Seconds(0)),
  session_active(false)
  {}

DestinationInfo::~DestinationInfo() {
}


// --------------------------------------------------------
NeighborInfo::NeighborInfo(uint32_t index, Time expire) :
  index(index),
  expires_in(expire),
  avr_T_send(Seconds(0))
  {}

NeighborInfo::~NeighborInfo() {
}



RoutingTable::RoutingTable(Ptr<AntHocNetConfig> config, Ptr<Ipv4> ipv4) :
  ipv4(ipv4),
  n_dst(0),
  n_nb(0),
  config(config)
{
  // Initialize the usemaps
  for (uint32_t i = 0; i < MAX_DESTINATIONS; i++) this->dst_usemap[i] = false;
  for (uint32_t i = 0; i < MAX_NEIGHBORS; i++) this->nb_usemap[i] = false;
}
  
RoutingTable::~RoutingTable() {
  
}

void RoutingTable::SetConfig(Ptr<AntHocNetConfig> config) {
  this->config = config;
}

Ptr<AntHocNetConfig> RoutingTable::GetConfig() const {
  return this->config;
}


bool RoutingTable::AddNeighbor(uint32_t iface_index, Ipv4Address address) {
  return this->AddNeighbor(iface_index, 
                           address, this->config->nb_expire);
}

bool RoutingTable::AddNeighbor(uint32_t iface_index, 
                               Ipv4Address address, Time expire) {
  
  
  if (iface_index >= MAX_NEIGHBORS) {
    NS_LOG_ERROR("iface index to large index: " << iface_index);
    return false;
  }
  
  // Before the neighbor can be added, it needs a destination.
  // Check if destination already exists
  // NOTE: One could also simply add dst, since it checks the existance
  auto it = this->dsts.find(address);
  if (it == this->dsts.end()) {
    this->AddDestination(address);
    it = this->dsts.find(address);
  }
  
  // If interface already exist, just return
  auto it2 = it->second.nbs.find(iface_index);
  if (it2 != it->second.nbs.end()) {
	//NS_LOG_FUNCTION(this << "nb interface already exist");  
	return false;
  }
  
  for (uint32_t i = 0; i < MAX_NEIGHBORS; i++) {
    if (!this->nb_usemap[i]) {
      
      this->nb_usemap[i] = true;
      it->second.nbs.insert(std::make_pair(iface_index, 
                                           NeighborInfo(i, expire)));
      
      for (uint32_t j = 0; j < MAX_DESTINATIONS; j++) {
        this->rtable[j][i] = RoutingTableEntry();
      }
      
      //NS_LOG_FUNCTION(this << "index" << i);
      break;
    }
  }
  
  // Increase number of neigbors
  this->n_nb++;
  return true;
}

void RoutingTable::RemoveNeighbor(uint32_t iface_index, Ipv4Address address) {
  
  // Search for the destination
  auto it = this->dsts.find(address);
  if (it == this->dsts.end()) {
    //NS_LOG_FUNCTION(this << "nb address does not exist");
    return;
  }
  
  // Search for the interface
  auto nb_it = it->second.nbs.find(iface_index);
  if (nb_it == it->second.nbs.end()) {
    //NS_LOG_FUNCTION(this << "nb not on this interface");
    return;
  }
  
  
  // First, reset the row in the array
  uint32_t delete_index = nb_it->second.index;
  this->nb_usemap[delete_index] = false;
  
  NS_LOG_FUNCTION(this << "address" << address << "index" << iface_index);
  
  // Then remove the entry from the std::map of neighbors
  it->second.nbs.erase(nb_it);
  
  // Decrease counter of neighbors
  this->n_nb--;
  return;
}



bool RoutingTable::AddDestination(Ipv4Address address) {
    return this->AddDestination(address, this->config->dst_expire);
}

bool RoutingTable::AddDestination(Ipv4Address address, Time expire) {
  
  //NS_LOG_FUNCTION(this << "address" << address);
  
  if (this->n_dst == MAX_DESTINATIONS-1) {
    NS_LOG_ERROR(this << "Out of destination slots in rtable");
    return false;
  }
  
  // Check if destination already exists
  auto it = this->dsts.find(address);
  if (it != this->dsts.end()) {
    //NS_LOG_FUNCTION(this << "dst already exist");
    return false;
  }
  
  for (uint32_t i = 0; i < MAX_DESTINATIONS; i++) {
    if (!this->dst_usemap[i]) {
      this->dst_usemap[i] = true;
      this->dsts.insert(std::make_pair(address, DestinationInfo(i, expire)));
      
      // Reset the row in the array
      for (uint32_t j = 0; j < MAX_NEIGHBORS; j++) {
        this->rtable[i][j] = RoutingTableEntry();
      }
      
      
      break;
    }
  }
  
  this->n_dst++;
  return true;
  
}


void RoutingTable::RemoveDestination(Ipv4Address address) {
  
  NS_LOG_FUNCTION(this << "address" << address);
  
  // Check, if the destination exists
  auto it = this->dsts.find(address);
  if (it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst does not exist");
    return;
  }
  
  for (auto nb_it = it->second.nbs.begin();
    nb_it != it->second.nbs.end(); /* no increment*/) {
    this->RemoveNeighbor((nb_it++)->first, address);
  }
  
  // Set the usemap to indicate
  this->dst_usemap[it->second.index] = false;
  
  // Remove destination entry from the std::map
  this->dsts.erase(it);
  
  // Decrease the counter of destination
  this->n_dst--;
  return;
}

void RoutingTable::RegisterSession(Ipv4Address dst) {
  
  this->AddDestination(dst);
  auto dst_it = this->dsts.find(dst);
  
  dst_it->second.session_time = Simulator::Now();
  dst_it->second.session_active = true;
  
}

std::list<Ipv4Address> RoutingTable::GetSessions() {
  
  std::list<Ipv4Address> ret;
  for (auto dst_it = this->dsts.begin(); dst_it != this->dsts.end(); ++dst_it){
    if (dst_it->second.session_active &&
        Simulator::Now() - dst_it->second.session_time < this->config->session_expire){
      ret.push_back(dst_it->first);
    }
    else {
      dst_it->second.session_active = false;
    }
  }
  
  return ret;
}

bool RoutingTable::IsBroadcastAllowed(Ipv4Address address) {
  
  // Check if destination exists
  auto dst_it = this->dsts.find(address);
  if (dst_it == this->dsts.end()) {
    this->AddDestination(address);
    dst_it = this->dsts.find(address);
  }
  
  if (Simulator::Now() <= dst_it->second.no_broadcast_time) {
    NS_LOG_FUNCTION(this << "no bcast to" 
      << address << " for " 
      << (dst_it->second.no_broadcast_time - Simulator::Now()).GetMilliSeconds());
    return false;
  }
  
  return true;
}

void RoutingTable::ProcessAck(Ipv4Address address, uint32_t iface, 
                              Time last_hello) {
  
  // If we get an ack without a Hello, what do do?
  // Be happy or suspicios?
  
  auto dst_it = this->dsts.find(address);
  if (dst_it == this->dsts.end()) {
    return;
  }
  
  auto nb_it = dst_it->second.nbs.find(iface);
  if (nb_it == dst_it->second.nbs.end()) {
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
  
  NS_LOG_FUNCTION(this << "dst" << address << "iface" << iface
    << "new avr_T_send" << nb_it->second.avr_T_send.GetMicroSeconds());
  
}

Time RoutingTable::GetTSend(Ipv4Address address, uint32_t iface) {
  
  auto dst_it = this->dsts.find(address);
  if (dst_it == this->dsts.end()) {
    return Seconds(0);
  }
  
  auto nb_it = dst_it->second.nbs.find(iface);
  if (nb_it == dst_it->second.nbs.end()) {
    return Seconds(0);
  }
  return nb_it->second.avr_T_send;
}

void RoutingTable::NoBroadcast(Ipv4Address address, Time duration) {
  
  auto dst_it = this->dsts.find(address);
  if (dst_it == this->dsts.end()) {
    this->AddDestination(address);
    dst_it = this->dsts.find(address);
  }
  
  dst_it->second.no_broadcast_time = Simulator::Now() + duration;
  
  return;
}

void RoutingTable::PurgeInterface(uint32_t interface) {
  
  NS_LOG_FUNCTION(this << "interface" << interface);
  
  for (auto dst_it = this->dsts.begin();
    dst_it != this->dsts.end(); ++dst_it) {
    
    auto nb_it = dst_it->second.nbs.find(interface);
    if (nb_it != dst_it->second.nbs.end()) {
      dst_it->second.nbs.erase(nb_it);
    }
    
  }
}

void RoutingTable::UpdateNeighbor(uint32_t iface_index, Ipv4Address address) {
  
  //NS_LOG_FUNCTION(this << "address" << address);
  
  // Search destination, add if no exist
  auto dst_it = this->dsts.find(address);
  if (dst_it == this->dsts.end()) {
    this->AddNeighbor(iface_index, address);
    //NS_LOG_FUNCTION(this << "added nbs address");
    return;
  }
  
  auto nb_it = dst_it->second.nbs.find(iface_index);
  if (nb_it == dst_it->second.nbs.end()) {
    this->AddNeighbor(iface_index, address);
    //NS_LOG_FUNCTION(this << "added nbs interface");
    return;
  }
  
  // Update expire time. Make sure, expire time of dst is no
  // lower than the nb expire time
  nb_it->second.expires_in = this->config->nb_expire;
  if (dst_it->second.expires_in < nb_it->second.expires_in) {
    dst_it->second.expires_in = this->config->nb_expire;
  }
  
}

std::set<std::pair<uint32_t, Ipv4Address> > RoutingTable::Update(Time interval) {
  
  std::set<std::pair<uint32_t, Ipv4Address> > ret;
  
  // Iterate over the destinations
  for (auto nb1_it = this->dsts.begin();
    nb1_it != this->dsts.end(); /* no increment */) {
    
    
    // Calculate the time that ticked since the last occurence of update
    Time dst_dt = nb1_it->second.expires_in - interval;
    
    // Remove outdated destinations
    if (dst_dt <= Seconds(0)) {
      NS_LOG_FUNCTION(this << "dst" << nb1_it->first << "timed out");
      this->RemoveDestination((nb1_it++)->first);
      continue;
    }
      
    // Update dsts expire timer
    nb1_it->second.expires_in = dst_dt;
    
    // Iterate over all the neigbors
    for (auto nb2_it = nb1_it->second.nbs.begin();
    nb2_it != nb1_it->second.nbs.end(); ++nb2_it/* no increment */) {
      
      Time nb_dt = nb2_it->second.expires_in - interval;
      
      if (nb_dt <= Seconds(0)) {
        NS_LOG_FUNCTION(this << "nb" 
          << nb1_it->first << nb2_it->first << "timed out");
        
        ret.insert(std::make_pair(nb2_it->first , nb1_it->first));
        continue;
        
      }
      
      // Calculate the evaporation
      uint32_t nb_index = nb2_it->second.index;
      for (auto dst_it = this->dsts.begin(); 
           dst_it != this->dsts.end(); ++dst_it) {
        
        uint32_t dst_index = dst_it->second.index;
        
        RoutingTableEntry* ra = &(this->rtable[dst_index][nb_index]);
        /*ra->pheromone = ra->pheromone * (1.0 - this->config->evaporation);
        if (ra->pheromone < this->config->evap_threshold) {
          NS_LOG_FUNCTION(this << "pheromone evaporated" 
            << dst_it->first << nb1_it->first << nb2_it->first);
          //ra->pheromone = NAN;
          //ra->avr_hops = NAN;
          
          ret.insert(std::make_pair(nb2_it->first , nb1_it->first));
          
        }*/
      
        // FIXME: Also evaporate virtual pheromone?
        ra->virtual_pheromone = ra->virtual_pheromone 
          * (1.0 - this->config->evaporation);
        if (ra->virtual_pheromone < this->config->evap_threshold) {
          NS_LOG_FUNCTION(this << "virtual pheromone evaporated" 
            << dst_it->first << nb1_it->first << nb2_it->first);
          ra->virtual_pheromone = NAN;
        }
      
      }
       
      nb2_it->second.expires_in = nb_dt;
      if (dst_dt < nb_dt) {
        nb1_it->second.expires_in = nb_dt;
      }
      //++nb2_it;
      
    }
    ++nb1_it;
  }
  return ret;
}

void RoutingTable::ProcessNeighborTimeout(LinkFailureHeader& msg, 
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
}

void RoutingTable::ProcessLinkFailureMsg (LinkFailureHeader& msg,
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
}

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
  
  
}

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
}

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


bool RoutingTable::SelectRandomRoute(uint32_t& iface, Ipv4Address& nb,
  Ptr<UniformRandomVariable> vr) {
  
  if (this->n_nb == 0) {
    return false;
  }
  
  uint32_t select = vr->GetInteger(0, this->n_nb-1);
  uint32_t counter = 0;
  
  for (auto dst_it = this->dsts.begin();
       dst_it != this->dsts.end(); ++dst_it) {
    
    for (auto nb_it = dst_it->second.nbs.begin();
         nb_it != dst_it->second.nbs.end(); ++nb_it) {
      
      if (counter == select) {
        iface = nb_it->first;
        nb = dst_it->first;
        
        NS_LOG_FUNCTION(this << "iface" << iface << "nb" << nb);
        return true;
      }
      
      counter++;
    }
  }
  
  // Never come here
  return false;
}

bool RoutingTable::SelectRoute(Ipv4Address dst, double power,
                               uint32_t& iface, Ipv4Address& nb, 
                               Ptr<UniformRandomVariable> vr) {
  
  return this->SelectRoute(dst, power, iface, nb, vr, false, 1.0);
}


bool RoutingTable::SelectRoute(Ipv4Address dst, double power,
                               uint32_t& iface, Ipv4Address& nb, 
                               Ptr<UniformRandomVariable> vr,
                               bool consider_virtual, double virtual_malus){
  
  
  
  //Get the destination index:
  auto dst_it = this->dsts.find(dst);
  
  // Fail, if there are no entries to that destination at all
  if (dst_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst not exists" << dst);
    return false;
  }
  
  // Check, if the destination is a neighbor
  if (dst_it->second.nbs.size() != 0) {
    
    auto nb_it = dst_it->second.nbs.begin();
    iface = nb_it->first;
    nb = dst_it->first;
    
    NS_LOG_FUNCTION(this << "dst" << dst << "is nb" << nb 
      << "iface" << iface << "usevirt" << consider_virtual);
    
    return true;
  }
  
  uint32_t dst_index = dst_it->second.index;
  // Calculate the total pheromone value
  double total_pheromone = 0.0;
  uint32_t initialized = 0;
  
  for (auto dst2_it = this->dsts.begin();
       dst2_it != this->dsts.end(); ++dst2_it) {
    for (auto nb_it = dst2_it->second.nbs.begin();
         nb_it != dst2_it->second.nbs.end(); ++nb_it) {
      
      uint32_t nb_index = nb_it->second.index;
      
      double pheromone = this->rtable[dst_index][nb_index].pheromone;
      double virtual_pheromone = 
        this->rtable[dst_index][nb_index].virtual_pheromone;
      
      
      if (pheromone == pheromone) {
        initialized++;
        if (consider_virtual && virtual_pheromone == virtual_pheromone) {
          
          if (virtual_pheromone > virtual_malus * pheromone) {
            total_pheromone += pow(virtual_pheromone, power);
          }
          else {
            total_pheromone += pow(pheromone, power);
          }
        }
        else {
          total_pheromone += pow(pheromone, power);
        }
      }
      
      
    }
  }
  
  // NOTE: Very low pheromone can lead to the total_pheromone 
  // beeing rounded down to Zero. (When used with a high power) 
  // This leads to the system acting as if
  // there is no pheromone at all. This is most likely not an intended
  // behaviour and there should be a case to handle that
  
  // Fail, if there are no initialized entries (same as no entires at all)
  if (initialized == 0 || total_pheromone == 0.0) {
    NS_LOG_FUNCTION(this << "no initialized nbs");
    return false;
  }
  
  double select = vr->GetValue(0.0, 1.0);
  
  NS_LOG_FUNCTION(this << "total_pheromone");
    //<< total_pheromone << "select" << select);
  
  double selected = 0.0;
  
  // To select with right probability, a random uniform variable between 
  // 0 and 1 is generated, then it iterates over the neighbors, calculates their
  // probability and adds it to an aggregator. If the aggregator gets over the 
  // random value, the particular Neighbor is selected.
  
  for (auto dst2_it = this->dsts.begin();
       dst2_it != this->dsts.end(); ++dst2_it) {
    for (auto nb_it = dst2_it->second.nbs.begin();
         nb_it != dst2_it->second.nbs.end(); ++nb_it) {
      
      uint32_t nb_index = nb_it->second.index;
      double pheromone = this->rtable[dst_index][nb_index].pheromone;
      double virtual_pheromone = 
        this->rtable[dst_index][nb_index].virtual_pheromone;
      
      if (pheromone == pheromone) {
        if (consider_virtual && virtual_pheromone == virtual_pheromone) {
          
          if (virtual_pheromone > virtual_malus * pheromone) {
            selected += (pow(virtual_pheromone, power)/ total_pheromone);
          }
          else {
            selected += (pow(pheromone, power)/ total_pheromone);
          }
        }
        else {
          selected += (pow(pheromone, power)/ total_pheromone);
        }
      } 
      
      
      //NS_LOG_FUNCTION(this << "selected" << selected);
      
      if (selected > select) {
        iface = nb_it->first;
        nb = dst2_it->first;
        
        // Using this destination means it is relevant, update timeout
        dst2_it->second.expires_in = this->config->dst_expire;
        
        NS_LOG_FUNCTION(this << "dst" << dst 
          << "routed nb" << nb << "iface" << iface);
        return true;
      }
    }
  }
  
  // Never come here
  NS_LOG_FUNCTION(this << "never come here");
  return false;
}


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
}

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
}


void RoutingTable::UpdatePheromone(Ipv4Address dst, Ipv4Address nb, 
                      uint32_t iface,
                      double pheromone, uint32_t avr_hops) {
  //if (dst == nb) {
  //  return;
  //}
  
  auto dst_it = this->dsts.find(dst);
  auto nb_it = this->dsts.find(nb);
  
  this->UpdatePheromone(dst_it, nb_it, iface, pheromone, avr_hops);
}

void RoutingTable::UpdatePheromone(
                      std::map<Ipv4Address, DestinationInfo>::iterator dst_it, 
                      std::map<Ipv4Address, DestinationInfo>::iterator nb_it, 
                      uint32_t iface,
                      double pheromone, uint32_t hops) {
  
  RoutingTableEntry* ra = this->GetRa(dst_it, nb_it, iface);
  
  if (!ra) return;
  
  // If pheromone == NAN reset both values
  if (pheromone != pheromone) {
    ra->pheromone = NAN;
    ra->avr_hops = NAN;
  }
  
  // If only hops == NAN just ignore it
  if (hops == hops) {
    if (ra->avr_hops != ra->avr_hops) {
      ra->avr_hops = hops;
    }
    else {
      ra->avr_hops = this->config->alpha_pheromone*ra->avr_hops +
        (1.0 - this->config->alpha_pheromone) * hops;
    }
  }
  
  // Check if pheromone value is NAN
  if (ra->pheromone != ra->pheromone) {
    ra->pheromone = (1.0 / pheromone);
  }
  else {
    ra->pheromone = 
      this->config->gamma_pheromone*ra->pheromone +
      (1.0 - this->config->gamma_pheromone) * (1.0 / pheromone);
  }
  
}


void RoutingTable::UpdateVirtPheromone(Ipv4Address dst, Ipv4Address nb, 
                      uint32_t iface,
                      double pheromone) {
  //if (dst == nb) {
  //  return;
  //}
  
  auto dst_it = this->dsts.find(dst);
  auto nb_it = this->dsts.find(nb);
  
  this->UpdateVirtPheromone(dst_it, nb_it, iface, pheromone);
}


void RoutingTable::UpdateVirtPheromone(
                      std::map<Ipv4Address, DestinationInfo>::iterator dst_it, 
                      std::map<Ipv4Address, DestinationInfo>::iterator nb_it, 
                      uint32_t iface,
                      double pheromone) {
  RoutingTableEntry* ra = this->GetRa(dst_it, nb_it, iface);
  
  if (!ra) return;
  
  if (pheromone != pheromone) {
    ra->virtual_pheromone = NAN;
  }
  
  if (ra->virtual_pheromone != ra->virtual_pheromone) {
    ra->virtual_pheromone = pheromone;
  }
  else {
    ra->virtual_pheromone = 
      this->config->alpha_pheromone*ra->virtual_pheromone +
      (1.0 - this->config->alpha_pheromone) * (pheromone);
  }

}

RoutingTableEntry* RoutingTable::GetRa(
                       std::map<Ipv4Address, DestinationInfo>::iterator dst_it, 
                       std::map<Ipv4Address, DestinationInfo>::iterator nb_it, 
                       uint32_t iface) {
  // TODO: Add in later
  //if (dst_it->first == nb_it->first) {
  //  return 0;
  //}
  
  auto nb2_it = nb_it->second.nbs.find(iface);
  if (nb2_it == nb_it->second.nbs.end()) {
    return 0;
  }
  
  uint32_t dst_index = dst_it->second.index;
  uint32_t nb_index = nb2_it->second.index;
  
  return &(this->rtable[dst_index][nb_index]);
}


void RoutingTable::Print(Ptr<OutputStreamWrapper> stream) const {
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
}

}
}