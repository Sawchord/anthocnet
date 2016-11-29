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
  AHNTYPE_FW_UNKNOWN = 0,
  AHNTYPE_FW_ANT = 1, //!< Forward Ant
  AHNTYPE_PRFW_ANT= 2, //!< Proactive Forward Ant (reserved)
  AHNTYPE_BW_ANT = 3, //!< Backward Ant
  AHNTYPE_HELLO = 4, //!< Hello Packet
  AHNTYPE_DATA = 5, //!< Data packet (NEEDED?)
  AHNTYPE_RREP_ANT = 6, //!< RouteRepair Ant
  AHNTYPE_ERR = 7 //!< Error Ant
};

/**
 * \brief This is a class used to read 
 *        out any Header Type that the AntHocNet 
 *        protocol knows.
 */
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


/**
 * \brief Base class for all Ant types used
 *        in this protocol. The field types do 
 *        not change, only the way, these are 
 *        accesses, interpreted and calculated
 * \verbatim
0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |     Type      |    Reserved   |  TTL/Max Hops |   Hop Count   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Originator IP Address                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Destination IP Address                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  +                  Aggregated Time Value                        +
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Ant Stack                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \endverbatim
 */
class AntHeader : public Header {
public:
  //ctor
  AntHeader (
    Ipv4Address src = Ipv4Address(),
    Ipv4Address dst = Ipv4Address(),
    uint8_t ttl_or_max_hops = 20,
    uint8_t hops = 0,
    double T = 0.0
  );
  
  //dtor
  ~AntHeader();
  
  // Header serialization/deserialization
  // These functions do not need to be overwritten
  // by inheriting AntTypes, thus there is no need
  // for them to be virtual
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator) const;
  uint32_t Deserialize (Buffer::Iterator);
  void Print (std::ostream&) const;
  
  bool operator== (AntHeader const & o) const;
  
  // Checks, wether this Ant is valid
  virtual bool IsValid();
  
  // Access to Src and Dst fields should always be granted
  Ipv4Address GetSrc();
  void SetSrc(Ipv4Address);
  
  Ipv4Address GetDst();
  void SetDst(Ipv4Address);
  
  // The way of accessing 
  
protected:
  
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
  
  double T;
  
  // All the ants travelled so far/ yet to travel
  std::list<Ipv4Address> ant_stack;
  
};

/**
 * \brief A HelloAnt is basically a ForwardAnt, 
 *        but without a Destination Address.
 *        Its TTL should always be 1 and its HopCount 0.
 *        All HelloAnts not following this convention need 
 *        to be discarded.
 * \note Since a lot of fields in the Ant are unused, on 
 *       might consider introducing a special HelloPacket,
 *       which is not an Ant. However, since this is a proof
 *       of concept, readability goes over conciseness.
 */
class HelloAntHeader : public AntHeader {
public:
  // ctor
  HelloAntHeader(Ipv4Address src);
  // dtor
  ~HelloAntHeader();
  
  virtual bool IsValid();
};


}
}


#endif /* ANTHOCNETPACKET_H */