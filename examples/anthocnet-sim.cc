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

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

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

#include "ns3/aodv-module.h"
#include "ns3/anthocnet-module.h"

using namespace ns3;
using namespace ahn;

class RoutingExperiment {
public:
  RoutingExperiment();
  void Run();
  std::string CommandSetup(int argc, char** argv);
  
  
  
private:
  
  // State
  Ptr<SimDatabase> db;
  // Config
  
  // Simulation parameters
  Time total_time;
  uint32_t nWifis;
  uint32_t nSender;
  uint32_t nReceiver;
  
  uint32_t pWidth;
  uint32_t pHeight;
  
  uint32_t nodePause;
  uint32_t nodeMinSpeed;
  uint32_t nodeMaxSpeed;
  
  // Output parameters
  bool generate_pcap;
  
  // Phy Layer parameters
  uint32_t phyMode;
  uint32_t lossModel;
  
  double txpStart;
  double txpEnd;
  
  // Mac Layer parameters
  
  
  // IP Layer parameters
  uint32_t protocol;
  uint32_t packetSize;
  uint32_t packetRate;
  
};

RoutingExperiment::RoutingExperiment():
total_time(Seconds(900)),
nWifis(80),
nSender(20),
nReceiver(20),

pWidth(800),
pHeight(2400),

nodePause(30),
nodeMinSpeed(5),
nodeMaxSpeed(20),

generate_pcap(false),

phyMode(3),
lossModel(1),
txpStart(7.5),
txpEnd(7.5),


protocol(2),
packetSize(64),
packetRate(40)
{}

std::string RoutingExperiment::CommandSetup(int argc, char** argv) {
  CommandLine cmd;
  
  // Simulation parameters
  cmd.AddValue("Time", "The total time of the experiment", this->total_time);
  
  cmd.AddValue("nWifis", 
               "The total number of nodes in this simulation", this->nWifis);
  cmd.AddValue("nSender", "The total sender nodes", this->nSender);
  cmd.AddValue("nodeMaxSpeed", 
               "Maximum speed a node ", 
               this->nodeMaxSpeed);
  
  cmd.AddValue("nodePause",
               "Time in seconds a node rests after reaching waypoint", 
               this->nodePause);
  
  cmd.AddValue("nodeMinSpeed",
               "Minimal speed a node can move to next waypoint", 
               this->nodeMinSpeed);
  
  cmd.AddValue("nodeMaxSpeed",
               "Maximal speed a node can move to next waypoint", 
               this->nodeMaxSpeed);
  
  cmd.AddValue("pWidth", "Width of the simulated plane", this->pWidth);
  cmd.AddValue("pHeight", "Height of the simulated plane", this->pHeight);
  
  // Output parameters
  cmd.AddValue("generatePcap", 
               "Specify, whether Pcap output should be generated", 
               this->generate_pcap);
  
  // Phy layer parameters
  cmd.AddValue("phyMode", 
               "The physical Mode to use: 1=Dsss11Mbps; 2=Dsss1Mbps; 3=Dsss2Mbps",
               this->phyMode);
  cmd.AddValue("lossModel", "The loss model to simulate",
               this->lossModel);
  
  cmd.AddValue("txpStart", "Antenna gain at start of transmission", this->txpStart);
  cmd.AddValue("txpEnd", "Antenna gain at end of transmission", this->txpEnd);
  
  // Mac layer parameters
  
  // IP layer parametes
  cmd.AddValue("protocol", 
               "The protocol to use: 1=AODV; 2=ANTHOCNET", this->protocol);
  
  
  cmd.AddValue("packetSize", 
               "The size of the packets to be send", this->packetSize);
  cmd.AddValue("packetRate", 
               "The rate of the packets", this->packetRate);
  
  cmd.Parse(argc, argv);
  return "STUB";
}


void RoutingExperiment::Run() {
  
  Packet::EnablePrinting();
  
  // Create the nodes
  NodeContainer adhocNodes;
  adhocNodes.Create(this->nWifis);
  
  // Setting up the wifi
  
  // Set up the phy mode
  std::string phy_mode_string;
  switch (this->phyMode) {
    case 1:
      phy_mode_string = "DsssRate11Mbps";
      break;
    case 2:
      phy_mode_string = "DsssRate1Mbps";
      break;
    case 3:
      phy_mode_string = "DsssRate2Mbps";
      break;
    default:
      NS_FATAL_ERROR ("Phy mode not supported");
      break;
  }
  Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
                     StringValue (phy_mode_string));
  
  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
  
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  
  std::string loss_model_string;
  switch (this->lossModel){
    case 1:
      loss_model_string = "ns3::RangePropagationLossModel";
      break;
    case 2:
      loss_model_string = "ns3::FriisPropagationLossModel";
      break;
    default:
      NS_FATAL_ERROR ("Loss model not supported");
      break;
  }
  
  wifiChannel.AddPropagationLoss (loss_model_string);
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phy_mode_string),
                                "ControlMode", StringValue (phy_mode_string));

  wifiPhy.Set ("TxPowerStart", DoubleValue (this->txpStart));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (this->txpEnd));
  
  // Install wifi on the nodes
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);
  
  
  // Set up plane and mobility
  MobilityHelper mobilityAdhoc;
  // used to get consistent mobility across scenarios
  int64_t streamIndex = 0; 
  
  std::stringstream ssXpos, ssYpos;
  ssXpos << "ns3::UniformRandomVariable[Min=0.0|Max=" << this->pWidth << "]";
  ssYpos << "ns3::UniformRandomVariable[Min=0.0|Max=" << this->pHeight << "]";
  
  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue (ssXpos.str()));
  pos.Set ("Y", StringValue (ssYpos.str()));
  
  
  Ptr<PositionAllocator> taPositionAlloc = 
      pos.Create()->GetObject<PositionAllocator> ();
  streamIndex += taPositionAlloc->AssignStreams (streamIndex);

  std::stringstream ssSpeed;
  std::stringstream ssPause;
  
  ssSpeed << "ns3::UniformRandomVariable[Min=" << this->nodeMinSpeed 
      << "|Max=" << this->nodeMaxSpeed << "]";
  
  ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
  
  mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                  "Speed", StringValue (ssSpeed.str()),
                                  "Pause", StringValue (ssPause.str()),
                                  "PositionAllocator", PointerValue(taPositionAlloc));
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (adhocNodes);
  streamIndex += mobilityAdhoc.AssignStreams (adhocNodes, streamIndex);
  
  
  // Set up the IP layer routing protocol
  AodvHelper aodv;
  AntHocNetHelper ahn;
  
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;
  
  switch (this->protocol) {
    case 1:
      list.Add (aodv, 100);
      break;
    case 2:
      list.Add (ahn, 100);
      break;
    default:
      NS_FATAL_ERROR ("Loss model not supported");
      break;
  }
  
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);
  
  // Set up the application
  
  this->db = Create<SimDatabase>();
  
  // Set the default for the SimApllication
  Config::SetDefault("ns3::SimApplication::PacketSize", 
                     IntegerValue(this->packetSize));
  Config::SetDefault("ns3::SimApplication::PacketRate", 
                     IntegerValue(this->packetRate));
  
  Config::SetDefault("ns3::SimApplication::Database", PointerValue(this->db));
  
  SimHelper apphelper = SimHelper();
  
  apphelper.SetAttribute("SendMode", BooleanValue(false));
  
  // Install application in recevier mode
  for (uint32_t i = 0; i < this->nReceiver; i++) {
    
  }
  
  // Intall applicatoin in sender mode
  for (uint32_t i = this->nReceiver;
       i < this->nReceiver + this->nSender; i++) {
    
    
    
  }
}


int main (int argc, char* argv[]) {
  RoutingExperiment experiment;
  
  timeval start;
  gettimeofday(&start, NULL);
  
  time_t tim = time(0);
  struct tm* now = localtime(&tim);
  
  // Set up the experiment
  experiment.CommandSetup(argc, argv);
  
  // Create Folders
  std::stringstream dirss;
  dirss << "anthocnet_sim_" << (now->tm_year + 1900) << "-"
    << (now->tm_mon + 1) << "-" << (now->tm_mday) << "-"
    << (now->tm_hour) << "-" << (now->tm_min) << "-"
    << (now->tm_sec);
  
  std::string dir_string = dirss.str();
  
  if (mkdir(dir_string.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
    std::cerr << "Could no create directory to store the results. Aborting" << std::endl;
    return -1;
  }
  
  if (chdir(dir_string.c_str()) == -1) {
    std::cerr << "Could not cd into the directory to store results. Aborting" << std::endl;
    return -1;
  }
  
  std::cout << dir_string << std::endl;
  
  
  timeval stop;
  gettimeofday(&stop, NULL);
  
  int secs(stop.tv_sec - start.tv_sec);
  int usecs(stop.tv_usec - start.tv_usec);

  if(usecs < 0)
  {
      --secs;
      usecs += 1000000;
  }
  
  int total_time = static_cast<int>(secs * 1000 + usecs / 1000.0 + 0.5);
  std::cout << "Time: " << total_time << " milliseconds" << std::endl;
  
}