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

#ifndef ANTHOCNETRQUEUE_H
#define ANTHOCNETRQUEUE_H

#include "anthocnet-packet.h"

#include "ns3/simulator.h"
#include "ns3/node.h"

// TODO: which of these actually includes Packet
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"

#include <vector>
#include <queue>
#include <stdint.h>

namespace ns3 {
namespace ahn {

struct QueueEntry {
  
  mtype_t type;
  
  uint32_t iface;
  Ptr<Packet> packet;
  
  Time received_in;
  Time expire_in;
  
};


class IncomePacketQueue {
public:
  //ctor
  IncomePacketQueue(uint32_t max_len, Time initial_timeout);
  //dtor 
  ~IncomePacketQueue();
  
  /**
   * \brief Enqeues an Enrty, when possible.
   * \param type The type of the message
   * \param packet The packet to include
   * \param now The current Simulator time
   * \returns True if enqued, false if dropped
   */
  bool Enqueue(mtype_t type, uint32_t iface, Ptr<Packet> packet, Time now = Simulator::Now());
  
  /**
   * \brief Deques the first not expired entry
   * \param type Sets the type of the message
   * \param packet The pointer to the dequeued packet.
   * \param T_max The time this packet has been in the queue
   * \param now The current simulator time
   * \returns True if deqeued an entry false if empty
   */
  bool Dequeue(mtype_t& type, uint32_t& iface, Ptr<Packet>& packet, Time& T_max, Time now = Simulator::Now());
  
  /**
   * \brief Returns the number of entries currently in this queue.
   */
  uint32_t GetNEntries();
  
private:
  
  std::queue<QueueEntry> queue;
  
  uint32_t max_len;
  uint32_t len;
  
  Time timeout;
  
  
};

}
}
#endif /* ANTHOCNETRQUEUE_H */