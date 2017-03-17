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

#include "ns3/object.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/traced-value.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/stats-module.h"

using namespace ns3;

class RoutingExperiment {
public:
  RoutingExperiment();
  void Run();
  std::string CommandSetup(int argc, char** argv);
  
  
  
private:
  
  Time total_time;
  
  uint32_t nNodes;
  uint32_t nSender;
  uint32_t nReceiver;
  
  uint32_t plane_width;
  uint32_t plane_height;
  
  bool generate_pcap;
  
};

RoutingExperiment::RoutingExperiment() {
  
}

std::string RoutingExperiment::CommandSetup(int argc, char** argv) {
  CommandLine cmd;
  
  cmd.AddValue("Time", "The total time of the experiment", this->total_time);
  
  
  
  return "STUB";
}


void RoutingExperiment::Run() {
  
}


int main (int argc, char** argv) {
  RoutingExperiment experiment;
  
}