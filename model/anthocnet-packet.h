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
#include <map>
#include <vector>
#include <string>

#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3 {
namespace ahn {

typedef enum MessageType {
  AHNTYPE_UNKNOWN = 0,
  AHNTYPE_FW_ANT = 1, //!< Forward Ant
  AHNTYPE_PRFW_ANT= 2, //!< Proactive Forward Ant (reserved)
  AHNTYPE_BW_ANT = 3, //!< Backward Ant
  AHNTYPE_HELLO_MSG = 4, //!< Hello Packet
  AHNTYPE_HELLO_ACK = 5, //!< Hello Acknowledgement
  AHNTYPE_LINK_FAILURE = 6, //!< Link failure notification message
  AHNTYPE_WARNING = 7 //!< Unicast warning message
} mtype_t;

typedef std::pair<Ipv4Address, double> diffusion_t;

class TypeHeader : public Header {
public:
  
  //ctor
  TypeHeader(MessageType t = AHNTYPE_HELLO_MSG);
  //dtor
  ~TypeHeader();
  
  //Header serialization/deserialization
  static TypeId GetTypeId();
  TypeId GetInstanceTypeId() const;
  uint32_t GetSerializedSize() const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream& os) const;
  
  // Return type
  MessageType Get () const { return type; }
  // Check that type if valid
  bool IsValid () const { return valid; }
  bool operator== (TypeHeader const & o) const;
  
private:
  mtype_t type;
  bool valid;
};

typedef enum LinkFailureFlags {
  VALUE,
  NEW_BEST_VALUE,
  ONLY_VALUE
} linkfailure_status_t;

typedef struct LinkFailureList {
  Ipv4Address dst;
  linkfailure_status_t status;
  double new_pheromone;
} linkfailure_list_t;

class LinkFailureHeader : public Header {
public:
  //ctor
  LinkFailureHeader();
  //dtor
  ~LinkFailureHeader();
  
  //Header serialization/deserialization
  static TypeId GetTypeId();
  TypeId GetInstanceTypeId() const;
  uint32_t GetSerializedSize() const;
  void Serialize (Buffer::Iterator i) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  
  MessageType Get () const;
  // Check that type if valid
  bool IsValid () const;
  bool operator== (LinkFailureHeader const &o) const;
  
  void SetSrc(Ipv4Address src);
  
  void AppendUpdate(Ipv4Address dst, linkfailure_status_t status,
                    double new_pheromone);
  
  bool HasUpdates(); 
  linkfailure_list_t GetNextUpdate();
  
  Ipv4Address GetSrc();
  
  
  
private:
  
  Ipv4Address src;
  
  std::vector<linkfailure_list_t> updates;
  
};


class UnicastWarningHeader : public Header {
public:
  //ctor
  UnicastWarningHeader();
  UnicastWarningHeader(Ipv4Address src, Ipv4Address sender, Ipv4Address dst);
  //dtor
  ~UnicastWarningHeader();
  
  //Header serialization/deserialization
  static TypeId GetTypeId();
  TypeId GetInstanceTypeId() const;
  uint32_t GetSerializedSize() const;
  void Serialize (Buffer::Iterator i) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  
  MessageType Get () const;
  // Check that type if valid
  bool IsValid () const;
  bool operator== (UnicastWarningHeader const & o) const;
  
  Ipv4Address GetNode();
  Ipv4Address GetSender();
  Ipv4Address GetDst();
  
private:
  
  // The node which failed to resebd the data
  Ipv4Address node;
  
  // The sender of the data
  Ipv4Address sender;
  
  // The destionation of the data, which is unreachable
  Ipv4Address dst;
  
};


/**
 * \brief The hello packet. It is used to notify the neigbor 
 *        of the existance of the node as well as distribute the 
 *        pheromone values.
 */
class HelloMsgHeader : public Header {
public:
  // ctor
  HelloMsgHeader();
  HelloMsgHeader(Ipv4Address src);
  // dtor
  ~HelloMsgHeader();
  
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  
  virtual bool IsValid();
  
  Ipv4Address GetSrc();
  
  uint32_t GetSize();
  
  void PushDiffusion(Ipv4Address dst, double pheromone);
  diffusion_t PopDiffusion();
  
  uint32_t GetSerializedSize() const;
  
  void Serialize (Buffer::Iterator i) const;
  uint32_t Deserialize (Buffer::Iterator start);
  
  void Print (std::ostream&) const;
  
private:
  
  Ipv4Address src;
  std::vector<diffusion_t> diffusion;
  
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
  |     Type      |     Flags     |  TTL/Max Hops |   Hop Count   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Source IP Address                      |
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
    uint64_t T = 0
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
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream&) const;
  
  bool operator== (AntHeader const & o) const;
  
  // Checks, wether this Ant is valid
  virtual bool IsValid();
  
  uint8_t GetHops();
  uint64_t GetT();
  
  // Access to Src and Dst fields should always be granted
  Ipv4Address GetSrc();
  void SetSrc(Ipv4Address src);
  
  Ipv4Address GetDst();
  void SetDst(Ipv4Address dst);
  
  void SetBCount(uint8_t count);
  bool DecBCount();
  
  void SetSeqno(uint64_t seqno);
  uint64_t GetSeqno();
  
protected:
  
  uint8_t flags;
  
  // This field inidicates the time to life on a fwd ant
  // and the maximum number of hops from src to dst on a bw ant
  uint8_t ttl_or_max_hops;
  
  // The number of hops from src
  uint8_t hops;
  
  // source and destination addresses
  Ipv4Address src;
  Ipv4Address dst;
  
  uint64_t T_sd;
  
  uint64_t seqno;
  
  // All the ants travelled so far/ yet to travel
  std::vector<Ipv4Address> ant_stack;
  
};


class ForwardAntHeader : public AntHeader {
  friend class BackwardAntHeader;
public:
  
  //ctor
  ForwardAntHeader();
  ForwardAntHeader(Ipv4Address src, Ipv4Address dst, uint8_t ttl);
  ForwardAntHeader(uint8_t ttl);
  //dtor
  ~ForwardAntHeader();
  
  virtual bool IsValid();
  
  bool Update(Ipv4Address this_node);
  
  Ipv4Address PeekSrc();
  
  uint8_t GetTTL();
  
private:
  
  void CleanAntStack();
  
};
  

class BackwardAntHeader : public AntHeader {
  friend class ForwardAntHeader;
public:
  // ctor
  BackwardAntHeader();
  BackwardAntHeader(ForwardAntHeader& ia);
  //dtor
  ~BackwardAntHeader();
  
  virtual bool IsValid();
  
  
  Ipv4Address Update (uint64_t T_ind);
  
  Ipv4Address PeekThis();
  
  Ipv4Address PeekDst();
  
  uint8_t GetMaxHops();
  
};


}
}


#endif /* ANTHOCNETPACKET_H */