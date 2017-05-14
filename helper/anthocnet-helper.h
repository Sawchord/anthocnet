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
#ifndef ANTHOCNET_HELPER_H
#define ANTHOCNET_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/object-ptr-container.h"
#include "ns3/ipv4-routing-helper.h"


#include "ns3/anthocnet.h"

namespace ns3 {

/** 
 * \ingroup anthocnet
 * \brief Helper class that adds ANTHOCNET routing to nodes
 */
class AntHocNetHelper : public Ipv4RoutingHelper {
public:
  AntHocNetHelper();
  
  /**
   * \returns Pointer to a clone of thos Helper.
   */
  AntHocNetHelper* Copy(void) const;
  
  /**
   * \param node The node on which the routing protocol will run
   * \returns A newly created routing protocol
   */
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;
  
  /**
   * \param name The name of the attribute to set
   * \param value The value of the Attribute to set
   */
  void Set(std::string name, const AttributeValue &value);
  
  /**
   * \param stream First stream index to use.
   * \param c NodeContainer of the set of Nodes on wich AntHocNet
   *          should use the fixed stream
   * \returns The number of stream indices assigned by this helper
   */
  int64_t AssignStreams(NodeContainer c, int64_t stream);
  
private:
  ObjectFactory agent_factory;
  ObjectFactory config_factory;
};

}

#endif /* ANTHOCNET_HELPER_H */

