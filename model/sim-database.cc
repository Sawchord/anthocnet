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

#include "sim-database.h"

namespace ns3 {
//NS_LOG_CONPONENT_DEFINE ("AntHocNetSimDatabase");
namespace ahn {

SimPacketHeader::SimPacketHeader() {}

SimPacketHeader::SimPacketHeader(uint64_t seqno, uint64_t size):
seqno(seqno),
size(size)
{}
  
SimPacketHeader::~SimPacketHeader() {}

NS_OBJECT_ENSURE_REGISTERED(SimPacketHeader);

TypeId SimPacketHeader::GetTypeId() {
  static TypeId tid = TypeId("ns3::ahn::SimPacketHeader")
  .SetParent<Header>()
  .SetGroupName("AntHocNet")
  .AddConstructor<SimPacketHeader>()
  ;
  return tid;
  
}

TypeId SimPacketHeader::GetInstanceTypeId () const {
  return GetTypeId ();
}

uint32_t SimPacketHeader::GetSerializedSize () const {
  return this->size;
}

void SimPacketHeader::Serialize(Buffer::Iterator i) const {
  i.WriteHtonU64(this->seqno);
  i.WriteHtonU64(this->size);
  
  // Fill the rest to macht specified size
  for (uint64_t u = 0; u < this->size - 2*sizeof(uint64_t) ; u++) {
    i.WriteU8(0);
  }
  
}

uint32_t SimPacketHeader::Deserialize(Buffer::Iterator start) {
  
  Buffer::Iterator  i = start;
  
  this->seqno = i.ReadNtohU64();
  this->size = i.ReadNtohU64();
  
  // Read and discard the rest
  for (uint64_t u = 0; u < this->size - 2*sizeof(uint64_t); u++) {
    i.ReadU8();
  }
  
  uint32_t dist = i.GetDistanceFrom(start);
  NS_ASSERT(dist == this->GetSerializedSize());
  
  return dist;
}

void SimPacketHeader::Print(std::ostream& os) const {
  os << "SimPacket(Seqno: " << this->seqno 
    << " Size:" << this->size << ")" ;
}

bool SimPacketHeader::IsValid() const {
  if (this->size < 2* sizeof(uint64_t)) return false;
  return true;
}
  
bool SimPacketHeader::operator==(SimPacketHeader const & o) const {
  return (seqno == o.seqno && size == o.size);
}


uint64_t SimPacketHeader::GetSeqno() {
  return this->seqno;
}


// --------------------------------------------
// Database 
SimDatabase::SimDatabase():
packet_seqno(0),
transmission_seqno(0)
{}

SimDatabase::~SimDatabase() {
  
}

NS_OBJECT_ENSURE_REGISTERED(SimDatabase);

TypeId SimDatabase::GetTypeId() {
  static TypeId tid = TypeId("ns3::ahn::SimDatabase")
  .SetParent<Object>()
  .SetGroupName("AntHocNet")
  .AddConstructor<SimDatabase>()
  ;
  return tid;
}

void SimDatabase::DoDispose() {
}

void SimDatabase::DoInitialize() {
  
}

uint64_t SimDatabase::CreateNewPacket(Ipv4Address src, Ipv4Address dst) {
  
  packet_track_t track = PacketTrack();
  
  //track->seqno = this->seqno++;
  track.status = INIT;
  track.src = src;
  track.dst = dst;
  
  uint64_t s = this->packet_seqno;
  this->packet_seqno++;
  this->packet_track.insert(std::make_pair(s, track));
  
  return s;
}

void SimDatabase::RegisterInTransmission(uint64_t seqno) {
  
  auto track = this->packet_track.find(seqno);
  
  track->second.status = IN_TRANSMISSION;
  track->second.creation = Simulator::Now();
  
}

void SimDatabase::RegisterReceived(uint64_t seqno) {
  
  auto track = this->packet_track.find(seqno);
  
  track->second.status = RECEIVED;
  track->second.destruction = Simulator::Now();
  
}

void SimDatabase::RegisterDropped(uint64_t seqno) {
  
  auto track = this->packet_track.find(seqno);
  
  track->second.status = DROPPED;
  track->second.destruction = Simulator::Now();
  
}

// End of namespaces
}
}