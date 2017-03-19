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

#include "sim-helper.h"

namespace ns3 {
  
SimHelper::SimHelper() {}

void SimHelper::SetAttribute(std::string name, const AttributeValue& value) {
  this->factory.Set(name, value);
}
  
ApplicationContainer SimHelper::Install (NodeContainer c) const {
  ApplicationContainer apps;
  
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i) {
    apps.Add (InstallPriv (*i));
  }
   
  return apps;
  
}
  
ApplicationContainer SimHelper::Install (Ptr<Node> node) const {
  return ApplicationContainer (this->InstallPriv(node));
}
  
ApplicationContainer SimHelper::Install (std::string nodeName) const {
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (this->InstallPriv (node));
}

Ptr<Application> SimHelper::InstallPriv (Ptr<Node> node) const {
  Ptr<Application> app = this->factory.Create<Application>();
  node->AddApplication(app);
  
  return app;
}

int64_t SimHelper::AssignStreams (NodeContainer c, int64_t stream) {
  
  int64_t current_stream = stream;
  
  Ptr<Node> node;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i) {
  node = (*i);
    for (uint32_t j = 0; j < node->GetNApplications (); j++) {
      Ptr<ahn::SimApplication> sim_app = 
        DynamicCast<ahn::SimApplication> (node->GetApplication (j));
        
        if (sim_app) {
          current_stream += sim_app->AssignStreams (current_stream);
        }
    }
  }
   
  return (current_stream - stream);
}
  
  

}