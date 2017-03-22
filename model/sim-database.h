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

#ifndef SIM_DATABASE_H
#define SIM_DATABASE_H

#include <algorithm>
#include <stdint.h>
#include <map>

#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/attribute.h"
#include "ns3/log.h"

#include "ns3/header.h"

#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"

#include "ns3/ipv4-address.h"


namespace ns3 {
namespace ahn {

class SimPacketHeader : public Header {
public:
  
  SimPacketHeader();
  SimPacketHeader(uint64_t seqno, uint64_t size);
  ~SimPacketHeader();
  
  static TypeId GetTypeId();
  TypeId GetInstanceTypeId() const;
  
  uint32_t GetSerializedSize() const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream& os) const;
  
  bool IsValid() const;
  bool operator== (SimPacketHeader const & o) const;
  
  uint64_t GetSeqno();
  
private:
  
  uint64_t seqno;
  uint64_t size;
};
  

// Database stuff
typedef enum PacketStatus {
  UNKNOWN = 1,
  IN_TRANSMISSION,
  RECEIVED,
  DROPPED
} packet_status_t;

typedef enum TransmissionStatus {
  T_UNKNOWN = 1,
  SUCCESS,
  FAIL
} transmission_status_t;


typedef struct PacketTrack {
  packet_status_t status;
  Ipv4Address src;
  Ipv4Address dst;
  Time creation;
  Time destruction;
  uint32_t size;
  uint64_t last_transmission_seqno;
} packet_track_t;

typedef struct TransmissionTrack {
  transmission_status_t status;
  Ipv4Address src;
  Ipv4Address dst;
  Time tx_time;
  Time rx_time;
  uint64_t packet_seqno;
  uint32_t size;
} transmission_track_t;
  
typedef struct Results {
  std::list<double> droprate;
  std::list<double> end_to_end_delay;
  std::list<double> average_delay_jitter;
  std::list<double> rate_control_package;
  std::list<double> rate_control_bytes;
  
} results_t;

class SimDatabase : public Object {
public:
  //ctor
  SimDatabase();
  //dtor
  ~SimDatabase();
  
  static TypeId GetTypeId();
  
  virtual void DoDispose();
  
  uint64_t CreateNewPacket(Ipv4Address src, Ipv4Address dst);
  void RegisterInTransmission(uint64_t seqno);
  void RegisterReceived(uint64_t seqno);
  void RegisterDropped(uint64_t seqno);
  
  uint64_t CreateNewTransmission(Ipv4Address src, Ipv4Address dst);
  void RegisterTx(uint64_t seqno, uint64_t packet_seqno, uint32_t size);
  
  void RegisterRx(uint64_t packet_seqno);
  void Print(std::ostream& os) const;
  results_t Evaluate(double granularity) const;
  
protected:
  virtual void DoInitialize();
  
  
private:
  
  uint64_t packet_seqno;
  uint64_t transmission_seqno;
  
  std::map<uint64_t, packet_track_t> packet_track;
  std::map<uint64_t, transmission_track_t> transmission_track;
};

// End of namespaces
}
}
#endif /* SIM_DATABASE_H */