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

#include "anthocnet-packet.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3 {
namespace ahn {  

// This function is needed for every class in ns3 that has a 
// GetTypeId function
NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t):
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
    {
      type = (MessageType) type;
      break;
    }
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
      os << "HELLO_MSG";
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

AntHeader::AntHeader (Ipv4Address src, Ipv4Address dst) : 
src(src), dst(dst){}

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

uint32_t AntHeader::GetSerializedSize () const {
  //Size: 1 TypeTag 1 Reserverd 1 TTL/MaxHops 1 Hops
  // 4 Src 4 Src 4 Time
  return 32 + this->ant_stack.size();
}

void AntHeader::Serialize (Buffer::Iterator i) const {
  // Write the first line
  i.WriteU8 ((uint8_t) this->type);
  i.WriteU8 (0x0); // Reserved is always 0 for now
  i.WriteU8 (this->ttl_or_max_hops);
  i.WriteU8 (this->hops);
  
  // Write src and dst
  WriteTo (i, this->src);
  WriteTo (i, this->dst);
  
  // Serialize the AntStack
  for (std::list<Ipv4Address>::const_iterator it = this->ant_stack.begin(); 
       it != this->ant_stack.end(); ++it) {
    WriteTo(i, *it);
  }
  
}

uint32_t AntHeader::Deserialize (Buffer::Iterator start) {
  Buffer::Iterator i = start;
  
  // Read the first line
  this->type = (MessageType) i.ReadU8 ();
  this->reserved = 0x0; i.ReadU8 (); // Skip reserved
  this->ttl_or_max_hops = i.ReadU8 ();
  this->hops = i.ReadU8 ();
  
  // read src and dst
  ReadFrom(i, this->src);
  ReadFrom(i, this->dst);
  
  
  // Erase the existing antstack
  this->ant_stack.erase(this->ant_stack.begin(), this->ant_stack.end());
  // Read the antstack
  for (uint32_t c = 0; c<= this->hops; c++) {
    Ipv4Address temp;
    ReadFrom(i, temp);
    this->ant_stack.push_back(temp);
  }  
  
  uint32_t dist = i.GetDistanceFrom(start);
  NS_ASSERT (dist == GetSerializedSize());
  return dist;
}

void AntHeader::Print(std::ostream &os) const {
  os << "TypeTag: (unimplemented)"
  << "TTL/MaxHops: " << this->ttl_or_max_hops
  << "Number of Hops: " << this->hops
  << "Source Address: " << this->src
  << "Destination: " << this->dst
  << "AntStack: ";
  
  for (std::list<Ipv4Address>::const_iterator it = this->ant_stack.begin(); 
       it != this->ant_stack.end(); ++it) {
    os << *it;
  }
}

std::ostream &
operator<< (std::ostream & os, AntHeader const & h) {
  h.Print (os);
  return os;
}



}
}