/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
    case AHNTYPE_PRFW_ANT:
    case AHNTYPE_BW_ANT:
    case AHNTYPE_HELLO:
    case AHNTYPE_DATA:
    case AHNTYPE_RREP_ANT:
    case AHNTYPE_ERR:
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
    {
      os << "FORWARD_ANT";
      break;
    }
    case AHNTYPE_PRFW_ANT:
    {
      os << "PROACTIVE_FORWARD_ANT";
      break;
    }
    case AHNTYPE_BW_ANT:
    {
      os << "BACKWARD_ANT";
      break;
    }
    case AHNTYPE_HELLO:
    {
      os << "HELLO_MESSAGE";
      break;
    }
    case AHNTYPE_DATA:
    {
      os << "DATA";
      break;
    }
    case AHNTYPE_RREP_ANT:
    {
      os << "REPAIR_ANT";
      break;
    }
    case AHNTYPE_ERR:
    {
      os << "ERROR";
      break;
    }
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

//----------------------------------------------------------
// Ant Header
AntHeader::AntHeader (Ipv4Address src, Ipv4Address dst, 
  uint8_t ttl_or_max_hops, uint8_t hops, uint64_t T) : 
reserved(0), 
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


// ----------------------------------------------------
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
  i.WriteU8 (0x0); // Reserved is always 0 for now
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
  this->reserved = 0x0; i.ReadU8 (); // Skip reserved
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
  << " AntStack(" << this->ant_stack.size() << "): ";
  
  for (std::vector<Ipv4Address>::const_iterator it = this->ant_stack.begin(); 
       it != this->ant_stack.end(); ++it) {
    os << *it << " ";
  }
}

// --------------------------------------------------------
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


std::ostream &
operator<< (std::ostream & os, AntHeader const & h) {
  h.Print (os);
  return os;
}

bool AntHeader::IsValid() {
  
  NS_ASSERT(this->src != this->dst);
  
  return true;
}

// -------------------------------------------------
// HelloAnt stuff
HelloAntHeader::HelloAntHeader():
// NOTE: Nullpointers as Ipv4Addresses do not work
// the logger prints them on inialization and that raises a 
// NULL-Pointer induced segfault.
AntHeader(Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), 1, 0, 0.0)
{}

HelloAntHeader::HelloAntHeader (Ipv4Address src):
AntHeader(src, Ipv4Address("255.255.255.255"), 1, 0, 0.0)
{}

HelloAntHeader::~HelloAntHeader() {}

TypeId HelloAntHeader::GetTypeId () {
  static TypeId tid = TypeId ("ns3::ahn::HelloAntHeader")
  .SetParent<AntHeader> ()
  .SetGroupName("AntHocNet")
  .AddConstructor<HelloAntHeader> ();
  return tid;
}

bool HelloAntHeader::IsValid() {
  
  // Check the super method
  if (!AntHeader::IsValid()) {
    return false;
  }
  
  if(this->ttl_or_max_hops != 1 || this->hops != 0) {
    return false;
  }
  
  return true;
}


// ----------------------------------------------
// ForwardAnt stunff
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
  
  if (this->ttl_or_max_hops == 0) {
    return false;
  }
  
  this->hops++;
  this->ttl_or_max_hops--;
  
  this->ant_stack.push_back(this_node);
  
  this->CleanAntStack();
  
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
      
      this->hops = this->hops - (this->hops + i);
      
      break;
    }
  }
  
  NS_ASSERT(this->hops < 20);
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
  
  // Retrieve src
  Ipv4Address ans = this->ant_stack[this->hops];
  
  // Update ant
  this->hops--;
  this->T += T_ind;
  this->ant_stack.pop_back();
  
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