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
NS_LOG_COMPONENT_DEFINE ("SimSenderApplication");
namespace ahn {

SimSenderApplication::SimSenderApplication(){}
SimSenderApplication::~SimSenderApplication(){}

NS_OBJECT_ENSURE_REGISTERED(SimSenderApplication);

TypeId SimSenderApplication::GetTypeId() {
  static TypeId tid = TypeId ("ns3::ahn::SimSenderApplication")
  .SetParent<Application>()
  .SetGroupName("AntHocNet")
  .AddConstructor<SimSenderApplication>()
  
  .AddAttribute ("PacketSize", 
    "The size of packets sent",
    UintegerValue (512),
    MakeUintegerAccessor (&SimSenderApplication::packet_size),
    MakeUintegerChecker<uint32_t> ()
  )  
  .AddAttribute ("PacketRate",
    "The number of packets send out per second",
    UintegerValue (512),
    MakeUintegerAccessor (&SimSenderApplication::packet_rate),
    MakeUintegerChecker<uint32_t> ()
  )
  .AddAttribute ("Remote",
    "The address of the destination",
    AddressValue (),
    MakeAddressAccessor (&SimSenderApplication::remote),
    MakeAddressChecker ()
  )
  .AddAttribute("Database",
    "Pointer to the statistics database",
    PointerValue(),
    MakePointerAccessor(&SimSenderApplication::db),
    MakePointerChecker<SimDatabase>()
  )
  ;
  return tid;
}

int64_t SimSenderApplication::AssignStreams (int64_t stream) {
  NS_LOG_FUNCTION(this << stream);
  
  this->random->SetStream(stream);
  return 1;
}

void SimSenderApplication::StartApplication() {
  NS_LOG_FUNCTION(this);
  
  if (!this->socket) {
    socket = Socket::CreateSocket(GetNode(), this->GetTypeId());
    
    if (this->send_mode) {
      // Set socket up to be able to send data out
      
      if (!InetSocketAddress::IsMatchingType (this->remote)) {
        NS_LOG_ERROR("Ipv6 not supported");
        return;
      }
      
      socket->Bind();
      socket->Connect(this->remote);
      socket->SetAllowBroadcast(true);
      
      this->tx_event = 
        Simulator::Schedule(Seconds(1) / this->packet_rate,
                            &SimSenderApplication::NextTxEvent, this);
      
    }
    else {
      
      
    }
    
    
  }
  
}

void SimSenderApplication::NextTxEvent() {
  
  
  // Schedule the next next event
  this->tx_event = 
    Simulator::Schedule(Seconds(1) / this->packet_rate,
                        &SimSenderApplication::NextTxEvent, this);
}

void SimSenderApplication::Send() {
  
}

void SimSenderApplication::Recv() {
  
}

void SimSenderApplication::StopApplication() {
  NS_LOG_FUNCTION(this);
}

void SimSenderApplication::DoDispose() {
  NS_LOG_FUNCTION (this);
  socket = 0;
  
  Application::DoDispose();
  
}

void SimSenderApplication::DoInitialize() {
  
}
// End of namespaces
}
}