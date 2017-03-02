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
#include "anthocnet-packet.h"
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
  AHNTYPE_HELLO_ACK = 5 //!< Hello Acknowledgement
} mtype_t;

typedef std::pair<Ipv4Address, double> diffusion_t;

/**
 * \brief This is a class used to read 
 *        out any Header Type that the AntHocNet 
 *        protocol knows.
 */
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
  void Print (std::ostream &os) const;
  
  // Return type
  MessageType Get () const { return type; }
  // Check that type if valid
  bool IsValid () const { return valid; }
  bool operator== (TypeHeader const & o) const;
  
private:
  mtype_t type;
  bool valid;
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
  
  void Serialize (Buffer::Iterator start) const;
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
  
  /**
   * \brief
   * \returns Hops count of this Ant.
   */
  uint8_t GetHops();
  uint64_t GetT();
  
  // Access to Src and Dst fields should always be granted
  Ipv4Address GetSrc();
  void SetSrc(Ipv4Address src);
  
  Ipv4Address GetDst();
  void SetDst(Ipv4Address dst);
  
  void SetBCount(uint8_t count);
  bool DecBCount();
  
protected:
  
  // Not used for now, set 0 on send, ignored on recv
  uint8_t flags;
  
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
  std::vector<Ipv4Address> ant_stack;
  
};


/**
 * \brief The ForwardAnt is created to find a new route to
 *        a destination. Every node including the originator
 *        push their address on the stack. The next node can
 *        peek at the top element on the stack if it needs to.
 * \note  Strictly, the originator does not need to push its address
 *        on the Stack, and the receiving nodes do not need to be
 *        able to peek on the stack, since all of this information 
 *        can be retrieved by looking at the IP header.
 *        However beeing implemented on top of IP, AntHocNet cannot
 *        make the general assumption of beeing implemented on top
 *        of an IP protocol. It can also be implemented directly on top
 *        of a MAC layer.
 */
class ForwardAntHeader : public AntHeader {
  friend class BackwardAntHeader;
public:
  
  //ctor
  ForwardAntHeader();
  ForwardAntHeader(Ipv4Address src, Ipv4Address dst, uint8_t ttl);
  ForwardAntHeader(uint8_t ttl);
  //dtor
  ~ForwardAntHeader();
  
  /**
   * \brief Checks, wether this Ant is well formed.
   * \returns True, if ant is well formed, false otherwise.
   */
  virtual bool IsValid();
  
  /**
   * \brief Does all the processing on the ant, that is required
   *        to forward it. Updates Stack, hop count and ttl.
   * \note The Ant should be updated, after all important information for
   *       this node is retrieved. (Contrary to the BackwardAnt)
   * \param this_node The Address of this node.
   * \returns true if the Ant was updated sucessfully, and can be resend.
   *          false if the Ant cannot be resend. (Reached end of life or 
   *          this node is the destination)
   */
  bool Update(Ipv4Address this_node);
  
  /**
   * \brief Looks at the address on top of stack.
   * \returns The address on top of the stack
   */
  Ipv4Address PeekSrc();
  
  /**
   * \brief
   * \returns TTL of this Ant.
   */
  uint8_t GetTTL();
  
private:
  /**
   * \brief Checks the AntStack for cyclic routes and removes them, such that
   *        the backward ants go the direct way
   */
  void CleanAntStack();
  
};
  


/**
 * \brief The BackwardAnt. It can only be created out of a ForwardAnt.
 * \note If a node wants to generate a Backward and, it has to Update the Forward,
 *       then generate the BackwardAnt. It does not need to be updated
 *       again after that.
 */
class BackwardAntHeader : public AntHeader {
  friend class ForwardAntHeader;
public:
  // ctor
  BackwardAntHeader();
  BackwardAntHeader(ForwardAntHeader& ia);
  //dtor
  ~BackwardAntHeader();
  
  /**
   * \brief Checks, whether this Ant is well formed.
   * \returns True, if ant is well formed, false otherwise.
   */
  virtual bool IsValid();
  
  /** 
   * \brief Updates the node to be used by this node.
   *        Updates stack, hop count and T_ind value.
   * \note Update must occur before using this node, but src address
   *       must be retrieved before. (Contrary to the ForwardAnt)
   * \param T_ind The T_mac value of this node. (See paper)
   * \returns The neighbor, which send the ant.
   */
  Ipv4Address Update (uint64_t T_ind);
  
  Ipv4Address PeekThis();
  /**
   * \brief
   * \returns The address to which to send this Ant along its path
   */
  Ipv4Address PeekDst();
  
  /**
   * \brief
   * \returns MaxHops of this Ant.
   */
  uint8_t GetMaxHops();
  
};


}
}


#endif /* ANTHOCNETPACKET_H */