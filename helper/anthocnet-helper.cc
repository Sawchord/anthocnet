/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Leon Tan
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
#include "anthocnet-helper.h"

namespace ns3 {

AntHocNetHelper::AntHocNetHelper() : Ipv4RoutingHelper() {
  this->agent_factory.SetTypeId("ns3::ahn::RoutingProtocol");
  this->config_factory.SetTypeId("ns3::ahn::AntHocNetConfig");
}

AntHocNetHelper* AntHocNetHelper::Copy (void) const {
  return new AntHocNetHelper (*this);
}

Ptr<Ipv4RoutingProtocol> AntHocNetHelper::Create (Ptr<Node> node) const {  
  
  Ptr<ahn::RoutingProtocol> agent = 
    this->agent_factory.Create<ahn::RoutingProtocol> ();
  node->AggregateObject(agent);
  
  agent->SetConfig(this->config_factory.Create<ahn::AntHocNetConfig>());
  
  return agent;
}

void AntHocNetHelper::Set(std::string name, 
                           const AttributeValue &value)
{
  // NOTE: untested
  this->agent_factory.Set(name, value);
  std::string c = "Config";
  this->agent_factory.Set(c, 
     PointerValue(this->config_factory.Create<ahn::AntHocNetConfig>()));
}

int64_t
AntHocNetHelper::AssignStreams (NodeContainer c, int64_t stream) {
  
  int64_t current_stream = stream;
  
  for (NodeContainer::Iterator it = c.Begin();
    it != c.End(); ++it) {
    
    Ptr<Node> node = *it;
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    NS_ASSERT_MSG(ipv4, "Ipv4 not installed on node");
    Ptr<Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol();
    NS_ASSERT_MSG (proto, "Ipv4 routing not installed on node");
    Ptr<ahn::RoutingProtocol> ahn =
      DynamicCast<ahn::RoutingProtocol> (proto);
    if (ahn) {
      current_stream += ahn->AssignStreams(current_stream);
      continue;
    }
    
    Ptr<Ipv4ListRouting> list = 
      DynamicCast <Ipv4ListRouting> (proto);
    if (list) {
      int16_t priority;
      Ptr<Ipv4RoutingProtocol> list_proto;
      Ptr<ahn::RoutingProtocol> list_ahn;
      
      for (uint32_t i = 0; i< list->GetNRoutingProtocols(); i++) {
        list_proto = list->GetRoutingProtocol(i, priority);
        list_ahn = DynamicCast<ahn::RoutingProtocol>(list_proto);
        
        if (list_ahn) {
          current_stream += list_ahn->AssignStreams (current_stream);
            break;
        }
      }
    }
  }
  return (current_stream - stream);
}

}

