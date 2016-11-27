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
#include "anthocnet-helper.h"

namespace ns3 {

AntHocNetHelper::AntHocNetHelper() : Ipv4RoutingHelper() {
  this->agent_factory.SetTypeId(
    "ns3::ahn::RoutingProtocol");
}

AntHocNetHelper* AntHocNetHelper::Copy (void) const {
  return new AntHocNetHelper (*this);
}

Ptr<Ipv4RoutingProtocol> AntHocNetHelper::Create
  (Ptr<Node> node) const {
  Ptr<ahn::RoutingProtocol> agent = 
    this->agent_factory.Create<ahn::RoutingProtocol> ();
  node->AggregateObject(agent);
  
  return agent;
}

void AntHocNetHelper::Set (std::string name, 
  const AttributeValue &value)
{
  this->agent_factory.Set (name, value);
}

int64_t
AntHocNetHelper::AssignStreams (NodeContainer c, int64_t stream) {
  
  int64_t current_stream = stream;
  
  
  // TODO: Implement Assignstreams
  
  
  return (current_stream - stream);
}

}

