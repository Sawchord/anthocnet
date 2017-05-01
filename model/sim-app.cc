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

#include "sim-app.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("SimApplication");
namespace ahn {

SimApplication::SimApplication(){
  
  this->random = Create<UniformRandomVariable>();
  
}
SimApplication::~SimApplication(){}

NS_OBJECT_ENSURE_REGISTERED(SimApplication);

TypeId SimApplication::GetTypeId() {
  static TypeId tid = TypeId ("ns3::ahn::SimApplication")
  .SetParent<Application>()
  .SetGroupName("AntHocNet")
  .AddConstructor<SimApplication>()
  
  .AddAttribute("SendStartTime",
    "The time, when this node shall start sending",
    TimeValue(Seconds(10)),
    MakeTimeAccessor(&SimApplication::send_start_time),
    MakeTimeChecker()
  )
  .AddAttribute ("SendMode",
    "Specify, whether this is a sender or a receiver",
    BooleanValue(false),
    MakeBooleanAccessor(&SimApplication::send_mode),
    MakeBooleanChecker()
  )
  .AddAttribute ("Port", 
    "The Port on which to operate",
    UintegerValue (49192),
    MakeUintegerAccessor (&SimApplication::port),
    MakeUintegerChecker<uint16_t> ()
  )  
  .AddAttribute ("PacketSize", 
    "The size of packets sent",
    UintegerValue (512),
    MakeUintegerAccessor (&SimApplication::packet_size),
    MakeUintegerChecker<uint32_t> ()
  )  
  .AddAttribute ("PacketRate",
    "The number of packets send out per second",
    UintegerValue (10),
    MakeUintegerAccessor (&SimApplication::packet_rate),
    MakeUintegerChecker<uint32_t> ()
  )
  .AddAttribute ("Local",
    "The address of this node",
    AddressValue (),
    MakeAddressAccessor (&SimApplication::local),
    MakeAddressChecker ()
  )
  .AddAttribute ("Remote",
    "The address of the destination",
    AddressValue (),
    MakeAddressAccessor (&SimApplication::remote),
    MakeAddressChecker ()
  )
  .AddAttribute("Database",
    "Pointer to the statistics database",
    PointerValue(),
    MakePointerAccessor(&SimApplication::db),
    MakePointerChecker<SimDatabase>()
  )
  ;
  return tid;
}

int64_t SimApplication::AssignStreams (int64_t stream) {
  NS_LOG_FUNCTION(this << stream);
  
  this->random->SetStream(stream);
  return 1;
}

void SimApplication::StartApplication() {
  NS_LOG_FUNCTION(this);
  
  if (!this->socket) {
    
    socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
    
    Ipv4Address t = InetSocketAddress::ConvertFrom(this->local).GetIpv4();
    socket->Bind(InetSocketAddress(t, this->port));
    socket->Listen();
    
    socket->SetRecvCallback(MakeCallback(&SimApplication::Recv, this));
    
    if (this->send_mode) {
      // Set socket up to be able to send data out
      
      if (!InetSocketAddress::IsMatchingType (this->remote)) {
        NS_LOG_ERROR("Ipv6 not supported");
        return;
      }
      
      socket->Connect(this->remote);
      //socket->SetAllowBroadcast(true);
      
      // Schedule first send event
      this->tx_event = 
        Simulator::Schedule(this->send_start_time,
                            &SimApplication::NextTxEvent, this);
      
    }
  }
  
}

void SimApplication::NextTxEvent() {
  
  // Register new packet in database create it and 
  // build a packet out of it, finally, schedule to send
  uint64_t seqno = this->db->CreateNewPacket(
    InetSocketAddress::ConvertFrom(this->local).GetIpv4(),
    InetSocketAddress::ConvertFrom(this->remote).GetIpv4());
  
  this->db->RegisterInTransmission(seqno);
  
  SimPacketHeader msg = SimPacketHeader(seqno, this->packet_size);
  Ptr<Packet> packet = Create<Packet>();
  
  packet->AddHeader(msg);
  
  Time jitter = MilliSeconds(random->GetInteger(0, 10));
  Simulator::Schedule(jitter, &SimApplication::Send, this,
                      socket, packet,
                      InetSocketAddress::ConvertFrom(this->remote).GetIpv4()
                     );
  
  // Schedule the next next event
  this->tx_event = 
    Simulator::Schedule(Seconds(1) / this->packet_rate,
                        &SimApplication::NextTxEvent, this);
}

void SimApplication::Send(Ptr<Socket> socket, Ptr<Packet> packet, 
                                Ipv4Address dst) {
  socket->SendTo(packet, 0, InetSocketAddress(dst, this->port));
}

void SimApplication::Recv(Ptr<Socket> socket) {
  
  Address src;
  Ptr<Packet> packet = socket->RecvFrom(src);
  
  SimPacketHeader msg;
  packet->RemoveHeader(msg);
  
  this->db->RegisterReceived(msg.GetSeqno());
  
}

void SimApplication::StopApplication() {
  NS_LOG_FUNCTION(this);
}

void SimApplication::DoDispose() {
  NS_LOG_FUNCTION (this);
  socket = 0;
  
  Application::DoDispose();
  
}

//void SimApplication::DoInitialize() {
  
//}
// End of namespaces
}
}