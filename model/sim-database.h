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

#include <stdint.h>
#include <map>

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/attribute.h"
#include "ns3/log.h"

#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"

#include "ns3/ipv4-address.h"


namespace ns3 {
namespace ahn {

typedef enum PacketStatus {
  INIT = 1,
  IN_TRANSMISSION,
  RECEIVED,
  DROPED,
  UNKNOWN
} packet_status_t;

typedef enum TransmissionStatus {
  SUCCESS = 1,
  FAIL
} transmission_status_t;

typedef enum TransmissionType {
  CONTROL = 1,
  DATA
} transmission_type_t;

typedef struct PacketTrack {
  packet_status_t status;
  Ipv4Address src;
  Ipv4Address dst;
  Time creation;
  Time destruction;
  uint32_t size;
} packet_track_t;

typedef struct TransmissionTrack {
  transmission_status_t status;
  transmission_type_t type;
  Ipv4Address src;
  Ipv4Address dst;
  uint32_t size;
} transmission_track_t;
  
class SimDatabase : public Object {
public:
  //ctor
  SimDatabase();
  //dtor
  ~SimDatabase();
  
  static TypeId GetTypeId();
  
  virtual void DoDispose();
  
protected:
  virtual void DoInitialize();
  
  
public:
  
  
private:
  
  uint64_t packet_seqno;
  uint64_t transmission_seqno;
  
  std::map<uint64_t, packet_track_t*> packet_track;
  std::map<uint64_t, transmission_track_t*> transmission_track;
};

// End of namespaces
}
}
#endif /* SIM_DATABASE_H */