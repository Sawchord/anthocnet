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

#include <string>

#include "anthocnet-packet.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3 {
namespace ahn {  

// This function is needed for every class in ns3 that has a 
// GetTypeId function
NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (mtype_t t):
  type (t), valid(true) {}

TypeHeader::~TypeHeader() {}

TypeId TypeHeader::GetTypeId () {
  static TypeId tid = TypeId ("ns3::ahn::TypeHeader")
  .SetParent<Header> ()
  .SetGroupName("AntHocNet")
  .AddConstructor<TypeHeader> ();
  return tid;
}

TypeId TypeHeader::GetInstanceTypeId () const {
  return GetTypeId ();
}

uint32_t TypeHeader::GetSerializedSize () const {
  return 1;
}

void TypeHeader::Serialize (Buffer::Iterator i) const {
  i.WriteU8 ((uint8_t) type);
}

uint32_t TypeHeader::Deserialize (Buffer::Iterator start) {
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();
  valid = true;
  switch (type)
  {
    case AHNTYPE_FW_ANT:
    case AHNTYPE_BW_ANT:
    case AHNTYPE_PRFW_ANT:
    case AHNTYPE_HELLO_MSG:
    case AHNTYPE_HELLO_ACK:
    case AHNTYPE_LINK_FAILURE:
    case AHNTYPE_WARNING:
      this->type = (MessageType) type;
      break;
    default:
      valid = false;
  }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}


void TypeHeader::Print (std::ostream &os) const {
  switch (type)
  {
    case AHNTYPE_FW_ANT:
      os << "FORWARD_ANT";
      break;
    case AHNTYPE_PRFW_ANT:
      os << "PROACTIVE_FORWARD_ANT";
      break;
    case AHNTYPE_BW_ANT:
      os << "BACKWARD_ANT";
      break;
    case AHNTYPE_HELLO_MSG:
      os << "HELLO_MSG";
      break;
    case AHNTYPE_HELLO_ACK:
      os << "HELLO_ACK";
      break;
    case AHNTYPE_LINK_FAILURE:
      os << "LINK_FAILURE";
      break;
    case AHNTYPE_WARNING:
      os << "WARNING";
      break;
    default:
      os << "UNKNOWN_TYPE";
  }
}

bool TypeHeader::operator== (TypeHeader const & o) const {
  return (type == o.type && valid == o.valid);
}

std::ostream &
operator<< (std::ostream & os, TypeHeader const & h) {
  h.Print (os);
  return os;
}

// -------------------------------------------------
// LinkFailure Header
LinkFailureHeader::LinkFailureHeader() {}
LinkFailureHeader::~LinkFailureHeader() {}

TypeId LinkFailureHeader::GetTypeId () {
  static TypeId tid = TypeId ("ns3::ahn::LinkFailureHeader")
  .SetParent<Header> ()
  .SetGroupName("AntHocNet")
  .AddConstructor<LinkFailureHeader> ();
  return tid;
}

TypeId LinkFailureHeader::GetInstanceTypeId () const {
  return GetTypeId ();
}

uint32_t LinkFailureHeader::GetSerializedSize() const {
  
  // For src, link and iface as well as the size
  uint32_t size = 6;
  for (auto it = this->updates.begin(); it != this->updates.end(); ++it) {
    
    // Every entry has the destination and status
    // Some have an extra double value
    size += 5;
    if (it->status == NEW_BEST_VALUE) {
      size += 8;
    }
  }
  
  return size;
}

void LinkFailureHeader::Serialize (Buffer::Iterator i) const {
  
  WriteTo(i, this->src);
  i.WriteU16(this->updates.size());
  
  for (auto it = this->updates.begin(); it != this->updates.end(); ++it) {
    WriteTo(i, it->dst);
    i.WriteU8(it->status);
    
    if (it->status == NEW_BEST_VALUE) {
      char buf[sizeof(double)];
      memcpy(&buf, &it->new_pheromone, sizeof(double));
      for (uint32_t b = 0; b < sizeof(double); b++) {
        i.WriteU8(buf[b]);
      }
    }
  }
}

uint32_t LinkFailureHeader::Deserialize(Buffer::Iterator start) {
  
  Buffer::Iterator i = start;
  
  ReadFrom(i, this->src);
  uint32_t size = i.ReadU16();
  
  for (uint32_t c = 0; c < size; c++) {
    linkfailure_list_t l;
    ReadFrom(i, l.dst);
    l.status = (linkfailure_status_t) i.ReadU8();
    
    if (l.status == NEW_BEST_VALUE) {
      
      char buf[sizeof(double)];
      for (uint32_t b = 0; b < sizeof(double); b++) {
        buf[b] = i.ReadU8();
      }
      memcpy(&(l.new_pheromone), &buf, sizeof(double));
      
    }
    
    this->updates.push_back(l);
  }
  
  uint32_t dist = i.GetDistanceFrom(start);
  NS_ASSERT (dist == GetSerializedSize());
  
  return dist;
  
}

void LinkFailureHeader::Print (std::ostream &os) const {
  os << "Src: " << this->src;
  
  if (this->updates.size() != 0) {
    os << "\tDestinations broken: ";
    for (auto it = this->updates.begin(); it != this->updates.end(); ++it) {
      os << " " << it->dst << " Status: ";
      
      switch (it->status) {
        case VALUE:
          os << "Normal value";
          break;
        case ONLY_VALUE:
          os << "Only value";
          break;
        case NEW_BEST_VALUE:
          os << "New best value: " << it->new_pheromone;
            break;
        default:
          os << "Status unknown";
          break;
      }
    }
  }
  else {
    os << " No destinations broken";
  }
}

bool LinkFailureHeader::IsValid() const{
  return true;
}

bool LinkFailureHeader::operator== (LinkFailureHeader const &o) const {
  if (src == o.src) {
    // TODO: Compare the other stuff as well
  } 
  return false;
}

void LinkFailureHeader::SetSrc(Ipv4Address src) {
  this->src = src;
}

void LinkFailureHeader::AppendUpdate(Ipv4Address dst, 
                                     linkfailure_status_t status,
                                     double new_pheromone) {
  
  linkfailure_list_t l;
  l.dst = dst;
  l.status = status;
  l.new_pheromone  = new_pheromone;
  
  this->updates.push_back(l);
  
}

bool LinkFailureHeader::HasUpdates() {
  return this->updates.size() != 0;
}

linkfailure_list_t LinkFailureHeader::GetNextUpdate() {
  linkfailure_list_t l = this->updates[this->updates.size() - 1];
  this->updates.pop_back();
  return l;
}

Ipv4Address LinkFailureHeader::GetSrc() {
  return this->src;
}


// -------------------------------------------------
// Unicast warning Header
UnicastWarningHeader::UnicastWarningHeader() {}
UnicastWarningHeader::UnicastWarningHeader(Ipv4Address src, Ipv4Address sender,
                                           Ipv4Address dst) :
node(src),
sender(sender),
dst(dst)
{}
UnicastWarningHeader::~UnicastWarningHeader() {}

TypeId UnicastWarningHeader::GetTypeId () {
  static TypeId tid = TypeId ("ns3::ahn::UnicastWarningHeader")
  .SetParent<Header> ()
  .SetGroupName("AntHocNet")
  .AddConstructor<UnicastWarningHeader> ();
  return tid;
}

TypeId UnicastWarningHeader::GetInstanceTypeId () const {
  return GetTypeId ();
}

uint32_t UnicastWarningHeader::GetSerializedSize() const {
  return 3 * 4;
}


void UnicastWarningHeader::Serialize(Buffer::Iterator i) const{
  
  WriteTo(i, this->node);
  WriteTo(i, this->sender);
  WriteTo(i, this->dst);
}

uint32_t UnicastWarningHeader::Deserialize(Buffer::Iterator start) {
  
  Buffer::Iterator i = start;
  
  ReadFrom(i, this->node);
  ReadFrom(i, this->sender);
  ReadFrom(i, this->dst);
  
  uint32_t dist = i.GetDistanceFrom(start);
  NS_ASSERT (dist == GetSerializedSize());
  return dist;
  
}

void UnicastWarningHeader::Print (std::ostream &os) const {
  os << "Failure at: " << this->node
    << "Origin: " << this->sender
    << "Destination: " << this->dst;
}

bool UnicastWarningHeader::IsValid() const{
  return (node != sender && sender != dst && node != dst);
}

bool UnicastWarningHeader::operator== (UnicastWarningHeader const &o) const {
  return (node == o.node && sender == o.sender && dst == o.dst);
}

Ipv4Address UnicastWarningHeader::GetNode() {
  return this->node;
}

Ipv4Address UnicastWarningHeader::GetSender() {
  return this->sender;
}

Ipv4Address UnicastWarningHeader::GetDst() {
  return this->dst;
}

// -------------------------------------------------
// HelloMsg header
NS_OBJECT_ENSURE_REGISTERED (HelloMsgHeader);

HelloMsgHeader::HelloMsgHeader() {}
HelloMsgHeader::~HelloMsgHeader() {}

HelloMsgHeader::HelloMsgHeader(Ipv4Address src):
  src(src)
  {}

TypeId HelloMsgHeader::GetTypeId () {
  static TypeId tid = TypeId ("ns3::ahn::HelloMsgHeader")
  .SetParent<Header> ()
  .SetGroupName("AntHocNet")
  .AddConstructor<HelloMsgHeader> ();
  return tid;
}

TypeId HelloMsgHeader::GetInstanceTypeId () const {
  return GetTypeId ();
}

bool HelloMsgHeader::IsValid() {
  
  if (this->src == Ipv4Address("0.0.0.0") || 
    this->src == Ipv4Address("127.0.0.1") ) {
    return false;
  }
  
  if (this->diffusion.size() > 255) {
    return false;
  }
  
  return true;
}

Ipv4Address HelloMsgHeader::GetSrc() {
  return this->src;
}

uint32_t HelloMsgHeader::GetSize() {
  return this->diffusion.size();
}

void HelloMsgHeader::PushDiffusion(Ipv4Address dst, double pheromone) {
  this->diffusion.push_back(std::make_pair(dst, pheromone));
}

diffusion_t HelloMsgHeader::PopDiffusion() {
  diffusion_t t = this->diffusion.back();
  this->diffusion.pop_back();
  return t;
}

uint32_t HelloMsgHeader::GetSerializedSize() const {
  return 4 + 4 + this->diffusion.size() * 12;
}

void HelloMsgHeader::Serialize(Buffer::Iterator i) const {
  
  WriteTo(i, this->src);
  i.WriteU32(this->diffusion.size());
  
  for (uint32_t c = 0; c < this->diffusion.size(); c++) {
    WriteTo(i, this->diffusion[c].first);
    
    char buf[sizeof(double)];
    memcpy(&buf, &this->diffusion[c].second, sizeof(double));
    
    for (uint32_t b = 0; b < sizeof(double); b++) {
      i.WriteU8(buf[b]);
    }
    
    //i.WriteHtonU64((uint64_t) this->diffusion[c].second);
  }
  
}

uint32_t HelloMsgHeader::Deserialize(Buffer::Iterator start) {
    Buffer::Iterator i = start;
    
    char buf[sizeof(double)];
    double pheromone;
    
    NS_ASSERT(this->diffusion.size() == 0);
    
    ReadFrom(i, this->src);
    uint32_t diffsize = i.ReadU32();
    
    for(uint32_t c = 0; c < diffsize; c++) {
      Ipv4Address address;
      ReadFrom(i, address);
      
      for (uint32_t b = 0; b < sizeof(double); b++) {
        buf[b] = i.ReadU8();
      }
      
      memcpy(&pheromone, &buf, sizeof(double));
      
      this->diffusion.push_back(std::make_pair(address, pheromone));
    }
    
    uint32_t dist = i.GetDistanceFrom(start);
    NS_ASSERT(dist = this->GetSerializedSize());
    return dist;
}


void HelloMsgHeader::Print(std::ostream &os) const {
  os << "Source Address: " << this->src 
  << " Number of diffusion values: " << this->diffusion.size()
  << ": [";
  
  for (auto it = this->diffusion.begin(); it != this->diffusion.end(); ++it) {
    os << "(" << it->first << ":" << it->second << ")";
  }
  
  os << "]";
}

std::ostream& operator<< (std::ostream & os, HelloMsgHeader const & h) {
  h.Print (os);
  return os;
}

//----------------------------------------------------------
// Ant Header
AntHeader::AntHeader (Ipv4Address src, Ipv4Address dst, 
  uint8_t ttl_or_max_hops, uint8_t hops, uint64_t T) : 
flags(0), 
ttl_or_max_hops(ttl_or_max_hops), hops(hops),
src(src), dst(dst), T(T)
{}

AntHeader::~AntHeader() {}

NS_OBJECT_ENSURE_REGISTERED(AntHeader);

TypeId AntHeader::GetTypeId () {
  static TypeId tid = TypeId ("ns3::ahn::AntHeader")
  .SetParent<Header> ()
  .SetGroupName("AntHocNet")
  .AddConstructor<AntHeader> ();
  return tid;
}

TypeId AntHeader::GetInstanceTypeId () const {
  return GetTypeId ();
}

// Implementation of functions inherited from Header class
uint32_t AntHeader::GetSerializedSize () const {
  //Size: 1 Reserverd 1 TTL/MaxHops 1 Hops
  // 4 Src 4 Src 4 Time
  
  // The ant_stack.size if the number of Ip Addresses
  // The assumption is made, that an Ip Address gets serialized to 4 bytes
  return 19 + this->ant_stack.size() * 4;
}

void AntHeader::Serialize (Buffer::Iterator i) const {
  // Write the first line
  i.WriteU8 (this->flags); // Reserved is always 0 for now
  i.WriteU8 (this->ttl_or_max_hops);
  i.WriteU8 (this->hops);
  
  // Write src and dst
  WriteTo (i, this->src);
  WriteTo (i, this->dst);
  
  // Write the time value
  i.WriteHtonU64((uint64_t) this->T);
  
  // Serialize the AntStack
  for (std::vector<Ipv4Address>::const_iterator it = this->ant_stack.begin(); 
       it != this->ant_stack.end(); ++it) {
    WriteTo(i, *it);
  }
  
}

uint32_t AntHeader::Deserialize (Buffer::Iterator start) {
  Buffer::Iterator i = start;
  
  // Read the first line
  this->flags = i.ReadU8 ();
  this->ttl_or_max_hops = i.ReadU8 ();
  this->hops = i.ReadU8 ();
  
  // read src and dst
  ReadFrom(i, this->src);
  ReadFrom(i, this->dst);
  
  // Read the time value
  this->T = (double) i.ReadNtohU64();
  
  // Erase the existing antstack
  this->ant_stack.erase(this->ant_stack.begin(), this->ant_stack.end());
  // Read the antstack
  for (uint32_t c = 0; c <= this->hops; c++) {
    Ipv4Address temp;
    ReadFrom(i, temp);
    this->ant_stack.push_back(temp);
  }  
  
  uint32_t dist = i.GetDistanceFrom(start);
  NS_ASSERT (dist == GetSerializedSize());
  return dist;
}


void AntHeader::Print(std::ostream &os) const {
  os << "TTL/MaxHops: " << std::to_string(this->ttl_or_max_hops)
  << " Number of Hops: " << std::to_string(this->hops)
  << " Source Address: " << this->src
  << " Destination: " << this->dst
  << " AntStack(" << this->ant_stack.size() << "): [ ";
  
  for (std::vector<Ipv4Address>::const_iterator it = this->ant_stack.begin(); 
       it != this->ant_stack.end(); ++it) {
    os << *it << " ";
  }
  
  os << "]";
}

// Setters and Getters 
Ipv4Address AntHeader::GetSrc() {
    return this->src;
}
void AntHeader::SetSrc(Ipv4Address src) {
    this->src = src;
}
Ipv4Address AntHeader::GetDst() {
    return this->dst;
}
void AntHeader::SetDst(Ipv4Address dst) {
    this->dst = dst;
}
uint8_t AntHeader::GetHops () {
  return this->hops;
}
uint64_t AntHeader::GetT() {
  return this->T;
}

void AntHeader::SetBCount(uint8_t count) {
  if (count > 0x0F) count = 0x0F;
  this->flags = ((this->flags & 0xF0) | (count & 0x0F));
}

bool AntHeader::DecBCount() {
  if (this->flags & 0x0F) {
    this->flags--;
    return true;
  }
  else {
    return false;
  }
}

std::ostream& operator<< (std::ostream & os, AntHeader const & h) {
  h.Print (os);
  return os;
}

bool AntHeader::IsValid() {
  
  //NS_ASSERT(this->src != this->dst);
  //NS_ASSERT( ((size_t) this->hops + 1) == this->ant_stack.size() );
  if (this->src == this->dst) return false;
    
  return true;
}

// ----------------------------------------------
// ForwardAnt stuff
ForwardAntHeader::ForwardAntHeader() :
  AntHeader(Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), 1, 0, 0)
{}
ForwardAntHeader::ForwardAntHeader(
  Ipv4Address src, Ipv4Address dst, uint8_t ttl) :
  AntHeader(src, dst, ttl, 0, 0) {
    this->ant_stack.push_back(src);
    
  }
ForwardAntHeader::ForwardAntHeader(uint8_t ttl) :
  AntHeader(Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), ttl, 0, 0)
  {}
  
  
ForwardAntHeader::~ForwardAntHeader() {
}

bool ForwardAntHeader::IsValid() {
  
  // Check the super method
  if (!AntHeader::IsValid()) {
    return false;
  }
  
  // TODO: More sanity checks
  
  return true;
}

bool ForwardAntHeader::Update(Ipv4Address this_node) {
  
  this->IsValid();
    
  if (this->ttl_or_max_hops == 0) {
    return false;
  }
  
  this->hops++;
  this->ttl_or_max_hops--;
  
  this->ant_stack.push_back(this_node);
  
  this->CleanAntStack();
  
  this->IsValid();
  
  return true;
}

Ipv4Address ForwardAntHeader::PeekSrc() {
  return this->ant_stack[this->hops];
}


uint8_t ForwardAntHeader::GetTTL() {
  return this->ttl_or_max_hops;
}


void ForwardAntHeader::CleanAntStack() {
  
  // NOTE: We can only find a loop which
  // closes at the last address.
  // Since every node should call this function,
  // no other loops should be possible
  Ipv4Address now = this->ant_stack[this->hops];
  
  uint32_t i;
  for (i = 0; i < this->hops; i++) {
    
    if (now == this->ant_stack[i]) {
      this->ant_stack.erase(this->ant_stack.begin() + i + 1, 
                            this->ant_stack.end());
      
      this->hops = this->ant_stack.size()-1;
      
      break;
    }
  }
  
  //NS_ASSERT(this->hops < 20);
}

// ---------------------------------------------------
// Backward ant stuff
BackwardAntHeader::BackwardAntHeader() :
  AntHeader(Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), 1, 0, 0)
{}

BackwardAntHeader::BackwardAntHeader(ForwardAntHeader& ia) :
  AntHeader(ia.GetDst(), ia.GetSrc(), ia.GetHops(), ia.GetHops(), 0)
  {
    for (uint32_t i = 0; i <= ia.GetHops(); i++) {
      this->ant_stack.push_back(ia.ant_stack[i]);
    }
  }

BackwardAntHeader::~BackwardAntHeader() {
}

bool BackwardAntHeader::IsValid() {
  
  // Check the super method
  if (!AntHeader::IsValid()) {
    return false;
  }
  
  // Max_Hops must always be larger than hop count
  if (this->ttl_or_max_hops < this->hops) {
    return false;
  }
  
  // TODO: More sanity checks
  
  return true;
}

Ipv4Address BackwardAntHeader::Update(uint64_t T_ind) {
  
  this->IsValid();
  
  // Retrieve src
  Ipv4Address ans = this->ant_stack[this->hops];
  
  // Update ant
  this->hops--;
  this->T += T_ind;
  this->ant_stack.pop_back();
  
  this->IsValid();
  
  return ans;
}

Ipv4Address BackwardAntHeader::PeekThis() {
    return this->ant_stack[this->hops];
}

Ipv4Address BackwardAntHeader::PeekDst() {
  
  // The top of the stack is the address of this node
  // and it needs to stay that way. Thus, the destination is the 
  // entry right bellow that one.
  return this->ant_stack[this->hops-1];
  
}

uint8_t BackwardAntHeader::GetMaxHops() {
  return this->ttl_or_max_hops;
}


}
}