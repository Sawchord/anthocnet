/*
 * Copyright (c) 2017 Leon Tan
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
  no_broadcast_time(Seconds(0)),
  session_time(Seconds(0)),
  session_active(false)
  {}

DestinationInfo::~DestinationInfo() {
}

// ---------------F-----------------------------------------
NeighborInfo::NeighborInfo() :
  avr_T_send(Seconds(0)),
  last_snr(0)
  {}

NeighborInfo::~NeighborInfo() {
}

RoutingTable::RoutingTable() :
seqno(0)
{}
  
RoutingTable::~RoutingTable() {}

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
    
    // Add timer
    this->nb_timers.insert(std::make_pair(nb, Timer(Timer::CANCEL_ON_DESTROY)));
    
  }
}

bool RoutingTable::IsNeighbor(Ipv4Address nb) {
  return (this->nbs.find(nb) != this->nbs.end());
}

bool RoutingTable::IsNeighbor(NbIt nb_it) {
  return (nb_it != this->nbs.end());
}

void RoutingTable::RemoveNeighbor(Ipv4Address nb) {
  for (auto dst_it = this->dsts.begin(); dst_it != this->dsts.end(); ++dst_it) {
    this->RemovePheromone(dst_it->first, nb);
  }
  
  // Remove timeout event
  auto nbt_it = this->nb_timers.find(nb);
  if (nbt_it != this->nb_timers.end())
    nbt_it->second.Remove();
  
  this->nbs.erase(nb);
}

void RoutingTable::AddDestination(Ipv4Address dst) {
    if (!this->IsDestination(dst)) {
    this->dsts.insert(std::make_pair(dst, DestinationInfo()));
  }
}

bool RoutingTable::IsDestination(Ipv4Address dst) {
  return (this->dsts.find(dst) != this->dsts.end());
}

bool RoutingTable::IsDestination(DstIt dst_it) {
  return (dst_it != this->dsts.end());
}

void RoutingTable::RemoveDestination(Ipv4Address dst) {
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    this->RemovePheromone(dst, nb_it->first);
  }
  this->RemoveNeighbor(dst);
  this->dsts.erase(dst);
}

void RoutingTable::AddPheromone(Ipv4Address dst, Ipv4Address nb, 
                                double pher, double virt_pher) {
  
  std::make_pair(dst, nb) == std::make_pair(nb, dst);
  
  auto key = std::make_pair(dst, nb);
  auto p_it = this->rtable.find(key);
  
  if (p_it == this->rtable.end()) {
    if (!this->IsDestination(dst) || !this->IsNeighbor(nb))
      return;
    
    this->rtable.insert(std::make_pair(key, RoutingTableEntry()));
    p_it = this->rtable.find(key);
  }
  
  p_it->second.pheromone = pher;
  p_it->second.virtual_pheromone = virt_pher;
  
}

void RoutingTable::RemovePheromone(Ipv4Address dst, Ipv4Address nb) {
  this->rtable.erase(std::make_pair(dst, nb));
}

bool RoutingTable::HasPheromone(Ipv4Address dst, Ipv4Address nb, bool virt) {
  auto p_it = this->rtable.find(std::make_pair(dst, nb));
  if (p_it == this->rtable.end())
    return false;
  
  if (!virt) {
    if (p_it->second.pheromone > this->config->min_pheromone)
      return true;
    else
      return false;
  } else {
    if (p_it->second.virtual_pheromone > this->config->min_pheromone)
      return true;
    else
      return false;
  }
  
}

void RoutingTable::SetPheromone(Ipv4Address dst, Ipv4Address nb,
                                double pher, bool virt) {
  
  //NS_ASSERT(pher <= 1);
  
  auto p_it = this->rtable.find(std::make_pair(dst, nb));
  
  if (p_it == this->rtable.end()) {
    this->AddPheromone(dst, nb, 0, 0);
    p_it = this->rtable.find(std::make_pair(dst, nb));
  }
  
  if (!virt)
    p_it->second.pheromone = pher;
  else
    p_it->second.virtual_pheromone = pher;
  
  if (p_it->second.pheromone < this->config->min_pheromone 
    && p_it->second.virtual_pheromone < this->config->min_pheromone) {
    this->rtable.erase(p_it);
  }
  
}

double RoutingTable::GetPheromone(Ipv4Address dst, Ipv4Address nb, bool virt) {
  
  auto p_it = this->rtable.find(std::make_pair(dst, nb));
  
  if (p_it == this->rtable.end())
    return 0;
  
  NS_ASSERT(p_it->second.pheromone > this->config->min_pheromone || p_it->second.virtual_pheromone > this->config->min_pheromone);
  
  if (!virt)
    return p_it->second.pheromone;
  else
    return p_it->second.virtual_pheromone;
}


void RoutingTable::UpdatePheromone(Ipv4Address dst, Ipv4Address nb, 
                                   double update, bool virt) {
  
  NS_LOG_FUNCTION(this << dst << nb << update << virt);
  auto target_nb_it = this->nbs.find(nb);
  if (target_nb_it == this->nbs.end()) {
    NS_LOG_FUNCTION(this << "nb not found" << nb);
    return;
  }
  
  // If destination not exists, create new
  // and set initial pheromone
  auto dst_it = this->dsts.find(dst);
  if (dst_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst does not exist");
    return;
  }
  
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    double old_phero = this->GetPheromone(dst, nb_it->first, virt);
    // This is the neigbor we are processing our data for
    if (nb_it == target_nb_it) {
      double new_phero;
      
      // Why does this make results worese
      //if (old_phero == 0)
      //  new_phero = update;
      //else
      new_phero = this->IncressPheromone(old_phero, update);
      
      this->SetPheromone(dst, nb_it->first, new_phero, virt);
    } else {
      // Evaporate the value
      //double new_phero = this->EvaporatePheromone(old_phero);
      //this->SetPheromone(dst, nb_it->first, new_phero, virt);
    } 
  }
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
    return true;
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
  if (!this->IsNeighbor(nb_it)) {
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
  
  NS_LOG_FUNCTION(this << "nb" << nb
    << "new avr_T_send" << nb_it->second.avr_T_send.GetMicroSeconds());
  
}

Time RoutingTable::GetTSend(Ipv4Address nb) {
  
  auto nb_it = this->nbs.find(nb);
  if (!this->IsNeighbor(nb_it)) {
    return Seconds(0);
  }
  
  return nb_it->second.avr_T_send;
}

void RoutingTable::SetLastSnr(Ipv4Address nb, double snr) {  
  auto nb_it = this->nbs.find(nb);
  if (nb_it == this->nbs.end()) {
    return;
  }
  
  nb_it->second.last_snr = snr;
}

double RoutingTable::GetLastSnr(Ipv4Address nb) {
  auto nb_it = this->nbs.find(nb);
  if (nb_it == this->nbs.end())
    return 0;
  
  return nb_it->second.last_snr;
}

double RoutingTable::GetQSend(Ipv4Address nb) {
  auto nb_it = this->nbs.find(nb);
  if (nb_it == this->nbs.end())
    return 0;
  
  if (nb_it->second.last_snr < this->config->snr_threshold)
    return this->config->snr_malus;
  else 
    return 1;
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
  
  auto nbt_it = this->nb_timers.find(nb);
  if(nbt_it == this->nb_timers.end())
    return;
  
  nbt_it->second.Remove();
  nbt_it->second.Schedule(this->config->nb_expire);
  
}

bool RoutingTable::SelectRoute(Ipv4Address dst, double beta,
                               Ipv4Address& nb,  Ptr<UniformRandomVariable> vr,
                               bool virt){
  
  if (this->config->fuzzy_mode)
    return this->SelectRouteFuzzy(dst, beta, nb, vr, virt);
  else
    return this->SelectRouteStandard(dst, beta, nb, vr, virt);
    
  
}


bool RoutingTable::SelectRouteStandard(Ipv4Address dst, double beta,
                               Ipv4Address& nb,  Ptr<UniformRandomVariable> vr,
                               bool virt){
  
  //Get the destination index:
  auto dst_it = this->dsts.find(dst);
  
  // Fail, if there are no entries to that destination at all
  if (dst_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst does not exist" << dst);
    return false;
  }
  
  // Fail, if there are no initialized entries (same as no entires at all)
  ProbVect pv;
  if (this->GetProbVector(pv, dst, beta, virt) == 0) {
    NS_LOG_FUNCTION(this << "no initialized nbs");
    return false;
  }
  
  double select = vr->GetValue(0.0, 1.0);
  double selected = 0.0;
  
  for (auto pv_it = pv.begin(); pv_it != pv.end(); pv_it++) {
    selected += pv_it->second;
    if (selected > select) {
      nb = pv_it->first;
      return true;
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

void RoutingTable::ProcessNeighborTimeout(LinkFailureHeader& msg, 
                                          Ipv4Address nb) {
  
  for (auto dst_it = this->dsts.begin(); dst_it != this->dsts.end(); ++dst_it) {
    
    auto p_it = this->rtable.find(std::make_pair(dst_it->first, nb));
    if (p_it == this->rtable.end() 
          || p_it->second.pheromone < this->config->min_pheromone) {
      continue;
    }
    
    auto other_inits = this->IsOnly(dst_it->first, nb);
    
    if (!other_inits.first) {
      msg.AppendUpdate(dst_it->first, ONLY_VALUE, 0.0);
    }
    else if (other_inits.second < p_it->second.pheromone) {
      msg.AppendUpdate(dst_it->first, NEW_BEST_VALUE, other_inits.second);
    }
    else {
     msg.AppendUpdate(dst_it->first, VALUE, 0.0); 
    }
  }
  NS_LOG_FUNCTION(this << "NB Timeout: " << msg);
  
  // After constructing the message, we can remove the neighbor
  this->RemoveNeighbor(nb);
  
  return;
}

void RoutingTable::ProcessLinkFailureMsg (LinkFailureHeader& msg,
                                          LinkFailureHeader& response,
                                          Ipv4Address origin){
  
  NS_ASSERT(msg.GetSrc() == origin);
  NS_ASSERT(msg.HasUpdates());
  
  NS_LOG_FUNCTION(this << "Processing" << msg);
  
  // Need to check if we really have this neighbor as neighbor
  // If not just skip, since we do not have routes over this
  // node anyway
  if (!this->IsNeighbor(origin)) {
    NS_LOG_FUNCTION(this << origin << "not neighbor");
    return; 
  }
  
  // Now we evaluate the update list from this message
  while (msg.HasUpdates()) {
    
    linkfailure_list_t l = msg.GetNextUpdate();
    
    // Skip the destinations, we do not know about
    // since we do not have information that could be outdated anyway
    if (!this->IsDestination(l.dst))
      continue;
    
    // Check for alternatives
    auto other_inits = this->IsOnly(l.dst, origin);
    
    double bs_phero = 0.0;
    double new_phero = 0.0;
    double T_id = 0.0;
    
    switch (l.status) {
      case VALUE:
        
        // DO nothing ???
        
        break;
      case ONLY_VALUE:
        
        // The route via linknb to dst is now broken
        // If it was broken before, no need to update
        if (!this->HasPheromone(l.dst, origin, false))
          continue;
        
        if (!other_inits.first) {
          // No alternatives, full breakage
          response.AppendUpdate(l.dst, ONLY_VALUE, 0.0);
        }
        else if (this->GetPheromone(l.dst, origin, false) > other_inits.second){
          // Best pheromone become invalid, send new one
          response.AppendUpdate(l.dst, NEW_BEST_VALUE, other_inits.second);
        }
        
        this->SetPheromone(l.dst, origin, 0, false);
        this->SetPheromone(l.dst, origin, 0, true);
        
        break;
        
      case NEW_BEST_VALUE:
        
        // NOTE: Experimental: Do not bootstrap if dst is unknown
        if (!this->HasPheromone(l.dst, origin, false))
          continue;
        
        if (this->config->snr_cost_metric)
          T_id = std::floor(this->GetQSend(origin));
        else
          T_id = this->GetTSend(origin).GetMilliSeconds();
        
        if (T_id == 0)
          continue;
        
        bs_phero = l.new_pheromone;
        new_phero = this->Bootstrap(bs_phero, T_id);
        this->UpdatePheromone(l.dst, origin, new_phero, false);
        this->UpdatePheromone(l.dst, origin, new_phero, true);
        
        break;
      default:
        break;
      
    }
  }
  
  
  NS_LOG_FUNCTION(this << "Response " << response);
  return;
}


void RoutingTable::ConstructHelloMsg(HelloMsgHeader& msg, uint32_t num_dsts, 
                                     Ptr<UniformRandomVariable> vr) {
  
  bool use_random = true;
  // If there are less known destinations than requested,
  // there is no need to select some randomly
  if (this->dsts.size() <= num_dsts) {
    use_random = false;
  }
  
  std::list<std::pair<Ipv4Address, double> > selection;
  
  for (auto dst_it = this->dsts.begin(); dst_it != this->dsts.end(); ++dst_it) {
    
    Ipv4Address temp_dst = dst_it->first;
    double best_phero = 0.0;
    
    for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
      
      auto p_it = this->rtable.find(std::make_pair(dst_it->first, nb_it->first));
      if (p_it == this->rtable.end())
        continue;
      
      if (std::abs(best_phero) < p_it->second.pheromone)
        best_phero = p_it->second.pheromone;
      
      
      if (std::abs(best_phero) < p_it->second.virtual_pheromone)
        best_phero = -1.0 * p_it->second.virtual_pheromone;
    }
   
   // Exclude neighbors from hello message
   if (this->IsNeighbor(temp_dst))
     continue;
   
   if (best_phero > this->config->min_pheromone)
     selection.push_back(std::make_pair(temp_dst, best_phero));
  }
  
  // Now select some of the pairs we found
  for (uint32_t i = 0; i < num_dsts; i++) {
    
    if (selection.empty()) 
      break;
    
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
    
    msg.PushDiffusion(sel_it->first, sel_it->second);
    selection.erase(sel_it); 
  }
  
  //NS_LOG_FUNCTION(this << "message" << msg);
}


void RoutingTable::HandleHelloMsg(HelloMsgHeader& msg) {
  
  if(!msg.IsValid()) {
    NS_LOG_FUNCTION(this << "Malformed HelloMsg -> dropped");
    return;
  }
  
  NS_ASSERT(this->IsNeighbor(msg.GetSrc()));
  
  // Bootstrap information for every possible destination
  while (msg.GetSize() != 0) {
    
    auto diff_val = msg.PopDiffusion();
    
    // Get destination or add if not yet exist
    if (!this->IsDestination(diff_val.first))
      this->AddDestination(diff_val.first);
    
    bool is_virt = false;
    double bs_phero = diff_val.second;
    if (bs_phero < 0) {
      bs_phero *= -1;
      is_virt = true;
    }
    
    uint64_t T_id;
    if (this->config->snr_cost_metric)
      T_id = std::floor(this->GetQSend(msg.GetSrc()));
    else
      T_id = this->GetTSend(msg.GetSrc()).GetMilliSeconds();
    
    if (T_id == 0)
      continue;
    
    double new_phero = this->Bootstrap(bs_phero, T_id);
    NS_LOG_FUNCTION(this << T_id << new_phero);
    
    
    if (!is_virt && this->HasPheromone(diff_val.first, msg.GetSrc(), false)) {
      // NOTE: Why does it makes it worse??
      this->UpdatePheromone(diff_val.first, msg.GetSrc(), new_phero, false);
      //return;
    }
    
    //this->UpdatePheromone(diff_val.first, msg.GetSrc(), new_phero, true);
    this->SetPheromone(diff_val.first, msg.GetSrc(), new_phero, true);
  }
}


bool RoutingTable::ProcessBackwardAnt(Ipv4Address dst, Ipv4Address nb, 
                                      uint64_t T_sd, uint32_t hops) {
    
  
  NS_LOG_FUNCTION(this << "dst" << dst 
    << "nb" << nb << "T_sd" << T_sd << "hops" << hops);
  
  if (!this->IsDestination(dst))
    this->AddDestination(dst);
  
  // NOTE: Maybe drop ant if nb is unknown
  NS_ASSERT(this->IsNeighbor(nb));
  
  // NOTE: This is the cost function.
  // One could get crazy here and have some 
  // really cool functions
   double T_id;
  if (this->config->snr_cost_metric)
    T_id = (double)T_sd;
  else
    T_id = (( ((double)T_sd / 1000000) + hops * this->config->T_hop) / 2);
  
  // Update the routing table
  this->UpdatePheromone(dst, nb, 1.0/T_id, false);
  
  
  NS_LOG_FUNCTION(this << "for" << dst << nb);
  
  return true;
}

uint64_t RoutingTable::NextSeqno() {
  uint64_t seqno = this->seqno;
  this->seqno++;
  return seqno;
}

bool RoutingTable::HasHistory(Ipv4Address dst, uint64_t seqno) {
  auto hist_it = this->history.find(std::make_pair(dst, seqno));
  return (hist_it != this->history.end());
}

void RoutingTable::AddHistory(Ipv4Address dst, uint64_t seqno) {
  this->history.insert(std::make_pair(dst, seqno));
}

// Private methods
double RoutingTable::Bootstrap(double ph_value, double update) {
  return 1.0/(1.0/(ph_value) + update);
}


std::pair<bool, double> RoutingTable::IsOnly(Ipv4Address dst, 
                                             Ipv4Address nb) {
  
  
  bool other_inits = false;
  double best_phero = 0;
  auto marked_nb_it = this->nbs.find(nb);
  
  for(auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    if (nb_it == marked_nb_it)
      continue;
    
    auto p_it = this->rtable.find(std::make_pair(dst, nb_it->first));
    if (p_it == this->rtable.end())
      continue;
    
    if (p_it->second.pheromone > this->config->min_pheromone) {
      other_inits = true;
      
      if(p_it->second.pheromone > best_phero) {
        best_phero = p_it->second.pheromone;
      }
    }
  }
  
  NS_ASSERT((other_inits && best_phero != 0) || (!other_inits));
  return std::make_pair(other_inits, best_phero);
}


double RoutingTable::SumPropability(Ipv4Address dst, double beta, bool virt) {
  
  double Sum = 0;
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    auto p_it = this->rtable.find(std::make_pair(dst, nb_it->first));
    
    if (p_it == this->rtable.end())
      continue;
    
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
  return Sum;
}

uint32_t RoutingTable::GetProbVector(ProbVect& pv, Ipv4Address dst, 
                                     double beta, bool virt) {
  
  double total_pheromone = this->SumPropability(dst, beta, virt);
  double cur_pheromone;
  uint32_t size = 0;
  
  // Fail, if there are no initialized entries (same as no entires at all)
  if (total_pheromone < pow(this->config->min_pheromone, beta)) {
    NS_LOG_FUNCTION(this << "total pheromone to low");
    return 0;
  }
  
  NS_LOG_FUNCTION("Total phero" << total_pheromone);
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    auto p_it = this->rtable.find(std::make_pair(dst, nb_it->first));
    if (p_it == this->rtable.end())
      continue;
    
    if (virt && p_it->second.virtual_pheromone > p_it->second.pheromone) {
      if (p_it->second.virtual_pheromone > this->config->min_pheromone)
        cur_pheromone = pow(p_it->second.virtual_pheromone, beta)/ total_pheromone;
    } else {
      if (p_it->second.pheromone > this->config->min_pheromone)
        cur_pheromone = pow(p_it->second.pheromone, beta)/ total_pheromone;
    }
    
    if (cur_pheromone > pow(this->config->min_pheromone, beta)) {
      NS_LOG_FUNCTION("Appending" << nb_it->first 
        << p_it->second.pheromone << cur_pheromone);
      
      pv.push_back(std::make_pair(nb_it->first, cur_pheromone));
      size++;
    }
  }
  return size;
}

double RoutingTable::EvaporatePheromone(double ph_value) {
  return ph_value - (1- this->config->alpha) * ph_value;
}

double RoutingTable::IncressPheromone(double ph_value, double update) {
  return (this->config->gamma * ph_value + (1 - this->config->gamma) * update);
}

void RoutingTable::Print(Ptr<OutputStreamWrapper> stream) const {
  this->Print(*stream->GetStream());
}

void RoutingTable::Print(std::ostream& os) const {
  
  for (auto dst_it = this->dsts.begin(); dst_it != this->dsts.end(); ++dst_it) {
    os << " DST:[" << dst_it->first;
    
    for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
      os << " NB:(" << nb_it->first << " ";
      
      auto p_it = this->rtable.find(std::make_pair(dst_it->first, nb_it->first));
      if (p_it != this->rtable.end())
        os << p_it->second.pheromone << "|" << p_it->second.virtual_pheromone;
      else 
        os << "None";
      
      os << ")";
    }
    os << "]" << std::endl;
  }
}

std::ostream& operator<< (std::ostream& os, RoutingTable const& t) {
  t.Print(os);
  return os;
}


// ----------------------- 
// Fuzzy experiments

double RoutingTable::GetNbTrust(Ipv4Address nb) {
  double fullfill = this->stat.GetFullfillmentRate(nb);
  double data_amount = this->stat.GetNumberOfData(nb);
  
  double trust = this->config->fis->Eval(fullfill, data_amount);
  
  NS_LOG_FUNCTION("NB FRate AmountData and Trust"
   << nb << fullfill << data_amount << trust);
  return trust;
}

bool RoutingTable::SelectRouteFuzzy(Ipv4Address dst, double beta,
                                    Ipv4Address& nb, Ptr<UniformRandomVariable> vr,
                                    bool virt) {
  
  auto dst_it = this->dsts.find(dst);
  
  // Fail, if there are no entries to that destination at all
  if (dst_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst does not exist" << dst);
    return false;
  }
  
  ProbVect tv;
  double total_pheromone = 0;
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    // If no pheromone at all, no need to evaulate further
    if (!this->HasPheromone(dst, nb_it->first, virt))
      continue;
    
    // Ignore neighbors, you do not trust at all
    double trust = this->GetNbTrust(nb_it->first);
    if (trust < this->config->trust_threshold)
      continue;
    
    double phero = this->GetPheromone(dst, nb_it->first, virt);
    
    // Calculate corrected pheromone
    phero = phero * trust;
    phero = pow(phero, beta);
    
    tv.push_back(std::make_pair(nb_it->first, phero));
    total_pheromone += phero;
  }
  
  // Normalize the vector
  for (auto tv_it = tv.begin(); tv_it != tv.end(); ++tv_it) {
    tv_it->second /= total_pheromone;
  }
  
  // Select as in normal select route
  double select = vr->GetValue(0.0, 1.0);
  double selected = 0.0;
  
  for (auto tv_it = tv.begin(); tv_it != tv.end(); ++tv_it) {
    selected += tv_it->second;
    if (selected > select) {
      nb = tv_it->first;
      return true;
    } 
  }
  
  return false;
  
}


// End of namespace
}
}