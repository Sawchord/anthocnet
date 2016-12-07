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


#include "anthocnet-rqueue.h"
#include "anthocnet-packet.h"

namespace ns3 {
namespace ahn {

IncomePacketQueue::IncomePacketQueue(uint32_t max_len, Time initial_timeout) :
max_len(max_len), timeout(initial_timeout)
{}

IncomePacketQueue::~IncomePacketQueue() {}


bool IncomePacketQueue::Enqueue(mtype_t type, Ptr<Packet> packet, Time now) {
  
  if (this->len == this->max_len-1) {
    return false;
  }
  
  QueueEntry qe;
  qe.type = type;
  qe.received_in = now;
  qe.expire_in = this->timeout;
  qe.packet = packet;
  
  this->queue.push(qe);
  
  this->len++;
  
  return true;
}

bool IncomePacketQueue::Dequeue(mtype_t& type, Ptr<Packet>& packet,
  Time& T_max, Time now) {
  
  while (true) {
    
    if (this->queue.empty()) {
      return false;
    }
    
    // Get next element from Queue
    QueueEntry qe = this->queue.front();
    this->queue.pop();
    
    Time T = now - qe.received_in;
    
    // Check if it is expired
    if (T > qe.expire_in) {
      continue;
    }
    
    type = qe.type;
    packet = qe.packet;
    T_max = T;
    
    
  }
  return true;
}


// End of namespaces
}
}