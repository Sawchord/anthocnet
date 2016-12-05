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
#ifndef ANTHOCNET_H
#define ANTHOCNET_H

#include "anthocnet-rtable.h"
#include "anthocnet-rqueue.h"
#include "anthocnet-packet.h"

#include <list>

#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"

// TODO: Make these settable by GetTypeId()
//       instead of hardcoding them
#define MAX_INTERFACES 10
#define ANTHOCNET_PORT 5555

namespace ns3 {
namespace ahn {

class RoutingProtocol : public Ipv4RoutingProtocol {
public:
  //ctor
  RoutingProtocol();
  //dtor
  ~RoutingProtocol();
  
  static TypeId GetTypeId (void);
  static const uint32_t AODV_PORT;
  
  // Inherited from Ipv4RoutingProtocol
  // Look up their documentation in ns3 reference manual
  Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                   LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  virtual void DoDispose();
  
protected:
    virtual void DoInitialize();
  
private:
  
  // All the utiliy and callback functions of the protocol go here
  // Sets up the operation of the protocol
  void Start();
  
  Ptr<Socket> FindSocketWithInterfaceAddress
    (Ipv4InterfaceAddress addr) const;
  
  /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model.  Return the number of streams (possibly zero) that
  * have been assigned.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);
    
  /**
   * \brief Finds the index of the Socket, used in the rtable.
   * \param socket The pointer to the sockeT;
   * \returns The index into the socket table.
   */  
  uint32_t FindSocketIndex(Ptr<Socket>)const;
  
  // ----------------------------------------------
  // Callback function for receiving a packet
  void Recv(Ptr<Socket> socket);
  
  // Callback to do a deferred send
  void Send(Ptr<Socket>, Ptr<Packet>, Ipv4Address);
  
  // Send a HelloAnt every other second
  void HelloTimerExpire();
  
  // Update the RoutingTable (mainly throw out old data)
  void RTableTimerExpire();
  
  // -------------------------------------------------
  // Ant Handers
  
  // Handles receiving of a HelloAnt
  void HandleHelloAnt(Ptr<Packet>, Ipv4Address, Ipv4Address, uint32_t);
  
  // Handles receiving of a ForwardAnt
  void HandleForwardAnt(Ptr<Packet>, Ipv4Address, Ipv4Address);
  
  // Handles receiving of a BackwardAnt
  void HandleBackwardAnt(Ptr<Packet>, Ipv4Address, Ipv4Address);
  
  //-----------------------------------------------
  // All the network config stuff go here 
  
  // The time to live of a new born forward and
  uint8_t initial_ttl;
  
  // The frequency in which to send Hello Ants
  Time hello_interval;
  Timer hello_timer;
  
  // Frequency, in which to update the RoutingTable
  Time rtable_update_interval;
  Timer rtable_update_timer;
  
  // Time until a neighbor expires
  Time nb_expire;
  
  // Time until a destination expires
  Time dst_expire;
  
  Ptr<UniformRandomVariable> uniform_random;
  
  //----------------------------------------------
  // All the global state of the protocol go here
  RoutingTable rtable;
  
  // The IP protocol
  Ptr<Ipv4> ipv4;
  
  // Holds information about the interfaces
  Ptr<Socket> sockets[MAX_INTERFACES];
  //list<uint32_t>* free_sockets;
  std::map< Ptr<Socket>, Ipv4InterfaceAddress> socket_addresses;
  
  
};
}
}

#endif /* ANTHOCNET_H */

