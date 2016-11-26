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

#ifndef ANTHOCNETPACKET_H
#define ANTHOCNETPACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include <list>
#include "ns3/nstime.h"

namespace ns3 {
namespace ahn {

enum MessageType {
  AHNTYPE_FW_ANT = 1, //!< Forward Ant
  AHNTYPE_PRFW_ANT= 2, //!< Proactive Forward Ant (reserved)
  AHNTYPE_BW_ANT = 3, //!< Backward Ant
  AHNTYPE_HELLO = 4, //!< Hello Packet
  AHNTYPE_DATA = 5, //!< Data packet (NEEDED?)
  AHNTYPE_RREP_ANT = 6, //!< RouteRepair Ant
  AHNTYPE_ERR = 7 //!< Error Ant
};

class TypeHeader : public Header {
public:
  
  //ctor
  TypeHeader(MessageType t = AHNTYPE_HELLO);
  //dtor
  ~TypeHeader();
  
  //Header serialization/deserialization
  static TypeId GetTypeId();
  TypeId GetInstanceTypeId() const;
  uint32_t GetSerializedSize() const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  
  // Return type
  MessageType Get () const { return type; }
  // Check that type if valid
  bool IsValid () const { return valid; }
  bool operator== (TypeHeader const & o) const;
  
private:
  MessageType type;
  bool valid;
};

class AntHeader : public Header {
public:
  //ctor
  AntHeader (
    Ipv4Address src = Ipv4Address(),
    Ipv4Address dst = Ipv4Address()
  );
  
  //dtor
  ~AntHeader();
  
  // Header serialization/deserialization
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  
  bool operator== (AntHeader const & o) const;
  
private:
  
  // Needed to identyfy the type of ant
  MessageType type; 
  
  // Not used for now, set 0 on send, ignored on recv
  uint8_t reserved;
  
  // This field inidicates the time to life on a fwd ant
  // and the maximum number of hops from src to dst on a bw ant
  uint8_t ttl_or_max_hops;
  
  // The number of hops from src
  uint8_t hops;
  
  // source and destination addresses
  Ipv4Address src;
  Ipv4Address dst;
  
  // All the ants travelled so far/ yet to travel
  std::list<Ipv4Address> ant_stack;
  
};


}
}


#endif /* ANTHOCNETPACKET_H */