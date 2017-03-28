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

#include "ns3/netanim-module.h"

using namespace ns3;
using namespace ahn;

class RoutingExperiment {
public:
  RoutingExperiment();
  void Run();
  std::string CommandSetup(int argc, char** argv);
  
private:
  
  void PhyTracer(Ptr<Packet const> packet);
  
  void IpTxTracer(Ptr<Packet const> packet, Ptr<Ipv4> ipv4, uint32_t interface);
  void IpRxTracer(Ptr<Packet const> packet, Ptr<Ipv4> ipv4, uint32_t interface);
  
  void DataDropTracer(Ptr<Packet const> packet, std::string reason, 
                      Ipv4Address address);
  
  void ProgressUpdate();
  
  void PrintSummary(std::ostream& os);
  
  // State
  Ptr<SimDatabase> db;
  std::ofstream data_drop_output;
  //std::vector<ApplicationContainer*> apps;
  
  uint64_t control_packets;
  uint64_t control_bytes;
  uint64_t data_packets;
  uint64_t data_bytes;
  
  results_t result;
  
  Ptr<UniformRandomVariable> random;
  
  void GenGnuplot (std::list<double>& values, 
                   std::string tr_name, std::string title,
                   std::string file_ext, 
                   std::string legendX, std::string legendY,
                   double gran ) const;
  
              
                   
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
  double output_granularity;
  
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
  
  uint32_t appStartBegin;
  uint32_t appStartEnd;
  
  
  std::string comment;
  
};

RoutingExperiment::RoutingExperiment():
total_time(Seconds(50)),
nWifis(20),
nSender(5),
nReceiver(5),

pWidth(400),
pHeight(1200),

nodePause(25),
nodeMinSpeed(5),
nodeMaxSpeed(20),

generate_pcap(false),
output_granularity(1.0),

phyMode(1),
lossModel(2),
txpStart(7.5),
txpEnd(7.5),


protocol(2),
packetSize(64),
packetRate(4),

appStartBegin(10),
appStartEnd(15)
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
  
  cmd.AddValue("outputGranularity",
               "How granualar the graphs generated should be (in seconds)",
               this->output_granularity);
  
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
  
  
  cmd.AddValue("appStartBegin", 
               "Time at which the first senders start", this->appStartBegin);
  cmd.AddValue("appStartEnd", 
               "Time at which the last senders start", this->appStartEnd);
  cmd.AddValue("Comment", 
               "Give a short description of this simulation", this->comment);
  
  cmd.Parse(argc, argv);
  return "STUB";
}


void RoutingExperiment::PrintSummary(std::ostream& os) {
  
  os << "Simulations summary: " << std::endl;
  os << "Comment: " << this->comment << std::endl;
  os << "Total average droprate: " 
    << this->result.droprate_total_avr << std::endl;
  os << "Total average delay: " 
    << this->result.end_to_end_delay_total_avr << "ms" << std::endl;
  
  os << "Data Packet rate: " << ((double)data_packets 
                / (data_packets+control_packets)) << std::endl;
  os << "Data Byte rate: " << ((double)data_bytes
                / (data_bytes+control_bytes)) << std::endl;
  
}

void RoutingExperiment::ProgressUpdate() {
  std::cout << Simulator::Now().GetSeconds()
    << "s/" << total_time.GetSeconds()
    << "s passed (" 
    << ((double)Simulator::Now().GetSeconds() / total_time.GetSeconds()) * 100
    << "%)" << std::endl; 
    
  
  Simulator::Schedule(Seconds(1), &RoutingExperiment::ProgressUpdate, this);
}

// All the tracers

void RoutingExperiment::DataDropTracer(Ptr<Packet const> packet, 
                                       std::string reason, 
                                       Ipv4Address address) {
  
  
  data_drop_output << Simulator::Now().GetSeconds() << " Data dropped at address: " << address << std::endl;
  data_drop_output << "\t Reason: " << reason << std::endl;
  data_drop_output << "\t Packet: " << packet << std::endl;
  
}

void RoutingExperiment::IpTxTracer(Ptr<Packet const> cpacket, Ptr<Ipv4> ipv4, 
                                   uint32_t interface) {
  
  //std::cout << "Packet txd: " << *cpacket << std::endl; 
  
  Ptr<Packet> packet = cpacket->CreateFragment(0, cpacket->GetSize());
  
  Ipv4Header ipheader;
  UdpHeader udpheader;
  SimPacketHeader simheader;
  
  packet->RemoveHeader(ipheader);
  packet->RemoveHeader(udpheader);
  
  if (udpheader.GetSourcePort() != 49153) {
    
    data_packets++;
    data_bytes += cpacket->GetSize();
    
  }
  else {
    packet->RemoveHeader(simheader);
    uint64_t seqno = this->db->CreateNewTransmission(
      ipv4->GetAddress(interface, 0).GetLocal());
    this->db->RegisterTx(seqno, simheader.GetSeqno(), packet->GetSize());
    
  }
  
  control_packets++;
  control_bytes += cpacket->GetSize();
  
}

void RoutingExperiment::IpRxTracer(Ptr<Packet const> cpacket, Ptr<Ipv4> ipv4, 
                                   uint32_t interface) {
  
  //std::cout << "Packet rxd: " << *cpacket << std::endl; 
  Ptr<Packet> packet = cpacket->CreateFragment(0, cpacket->GetSize());
  
  Ipv4Header ipheader;
  UdpHeader udpheader;
  SimPacketHeader simheader;
  
  packet->RemoveHeader(ipheader);
  packet->RemoveHeader(udpheader);
  
  
  if (udpheader.GetSourcePort() != 49153) {
    
    // How two get this tranmissions back?
  }
  else {
    packet->RemoveHeader(simheader);
    
    this->db->RegisterRx(simheader.GetSeqno(), 
                         ipv4->GetAddress(interface, 0).GetLocal());
    
  }
  
}

void RoutingExperiment::PhyTracer(Ptr<Packet const> packet) {
  
  std::cout << *packet << std::endl;
  
}

void RoutingExperiment::Run() {
    
  std::string tr_name = "anthocnet-sim";
  Packet::EnablePrinting();
  
  random = CreateObject<UniformRandomVariable>();
  
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
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  
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
  
  wifiChannel.AddPropagationLoss(loss_model_string);
  wifiPhy.SetChannel(wifiChannel.Create());
  
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phy_mode_string),
                                "ControlMode", StringValue (phy_mode_string));

  wifiPhy.Set ("TxPowerStart", DoubleValue (this->txpStart));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (this->txpEnd));
  
  // Install wifi on the nodes
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer adhocDevices = wifi.Install(wifiPhy, wifiMac, adhocNodes);
  
  
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
  mobilityAdhoc.SetPositionAllocator(taPositionAlloc);
  mobilityAdhoc.Install(adhocNodes);
  streamIndex += mobilityAdhoc.AssignStreams(adhocNodes, streamIndex);
  
  
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
      data_drop_output = std::ofstream(tr_name + "_data_drops.tr");
      break;
    default:
      NS_FATAL_ERROR ("Loss model not supported");
      break;
  }
  
  internet.SetRoutingHelper (list);
  internet.Install (adhocNodes);
  
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);
  
  // Set up the application
  
  this->db = Create<SimDatabase>();
  
  // Set the default for the SimApllication
  Config::SetDefault("ns3::ahn::SimApplication::PacketSize", 
                     UintegerValue(this->packetSize));
  Config::SetDefault("ns3::ahn::SimApplication::PacketRate", 
                     UintegerValue(this->packetRate));
  
  Config::SetDefault("ns3::ahn::SimApplication::Database", PointerValue(this->db));
  
  
  SimHelper apphelper("halp");
  
  // Install application in recevier mode
  apphelper.SetAttribute("SendMode", BooleanValue(false));
  for (uint32_t i = 0; i < this->nReceiver; i++) {
    
    
    apphelper.SetAttribute("Local",
                  AddressValue(
                    InetSocketAddress(adhocInterfaces.GetAddress(i))));
    
    apphelper.SetAttribute("StartTime", TimeValue(Seconds(0)));
    apphelper.SetAttribute("StopTime", TimeValue(this->total_time));
    
    apphelper.Install(adhocNodes.Get(i));
    
  }
  
  // Intall application in sender mode
  apphelper.SetAttribute("SendMode", BooleanValue(true));
  for (uint32_t i = this->nReceiver;
       i < this->nReceiver + this->nSender; i++) {
    
    apphelper.SetAttribute("Local",
                  AddressValue(
                    InetSocketAddress(adhocInterfaces.GetAddress(i))));
    
    apphelper.SetAttribute("Remote",
                  AddressValue(
                    InetSocketAddress(
                      adhocInterfaces.GetAddress(i % this->nReceiver))));
  
    Time start_time = Seconds(
      random->GetValue(this->appStartBegin, appStartEnd));
    
    std::cout << "App starts at " << start_time.GetSeconds() << std::endl;
    
    apphelper.SetAttribute("StartTime", TimeValue(start_time));
    apphelper.SetAttribute("StopTime", TimeValue(this->total_time));
    
    apphelper.Install(adhocNodes.Get(i));
    
    
  }
  
  streamIndex += apphelper.AssignStreams(adhocNodes, streamIndex);
  
  
  // Connect the tracers
  std::string IpTxPath = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";
  Config::ConnectWithoutContext (IpTxPath,
    MakeCallback(&RoutingExperiment::IpTxTracer, this));
  
  std::string IpRxPath = "/NodeList/*/$ns3::Ipv4L3Protocol/Rx";
  Config::ConnectWithoutContext (IpRxPath, 
    MakeCallback(&RoutingExperiment::IpRxTracer, this));
  
  std::string PhyPath = 
    "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop";
  Config::ConnectWithoutContext (PhyPath, 
    MakeCallback(&RoutingExperiment::PhyTracer, this));
  
  std::string DataDropPath = "/NodeList/*/$ns3::ahn::RoutingProtocol/DataDrop";
  Config::ConnectWithoutContext (DataDropPath,
    MakeCallback(&RoutingExperiment::DataDropTracer, this));
  
  this->data_packets = 0;
  this->data_bytes = 0;
  this->control_packets = 0;
  this->control_bytes = 0;
  
  // Start the net animator
  AnimationInterface anim (tr_name + "_animation.xml");
  
  //anim.EnablePacketMetadata ();
  //anim.EnableIpv4RouteTracking(tr_name + "_route.xml", Seconds(0), 
  //                             Seconds(this->total_time), MilliSeconds(100));
  
  anim.SkipPacketTracing();
  
  if (this->generate_pcap) {
    
    // phy level pcap
    //wifiPhy.EnablePcap((tr_name + ".pcap"), adhocNodes);
    
    // IP level pcap
    internet.EnablePcapIpv4((tr_name + ".pcap"), adhocNodes);
  }
  
  // Schedule initial events
  Simulator::Schedule(Seconds(1), &RoutingExperiment::ProgressUpdate, this);
  
  Simulator::Stop(this->total_time);
  Simulator::Run ();
  Simulator::Destroy ();
  
  // Get the result of the simulation and put them into graphs
  this->result = this->db->Evaluate(this->output_granularity);
  
  this->GenGnuplot(result.droprate, tr_name, "Droprate", 
                   "droprate", "Time [s]", "Droprate[%]", 
                   this->output_granularity);
  
  this->GenGnuplot(result.end_to_end_delay, tr_name, "End-To-End Delay", 
                   "delay", "Time [s]", "End-To-End Delay[ms]", 
                   this->output_granularity);
  
  
  std::ofstream packet_log(tr_name + "_packets.log");
  this->db->Print(packet_log);
  
  std::ofstream summary(tr_name + "_summary.txt");
  this->PrintSummary(summary);
  
}

void RoutingExperiment::GenGnuplot (std::list<double>& values, 
                                    std::string tr_name,
                                    std::string title,
                                    std::string file_ext,
                                    std::string legendX,
                                    std::string legendY,
                                    double gran
                                   ) const
{
  
  Time t = Seconds(0);
  
  Gnuplot plot (tr_name + "_" + file_ext  + ".eps");
  plot.SetTitle(title);
  plot.SetTerminal("eps");
  plot.SetLegend(legendX, legendY);
  
  Gnuplot2dDataset ds;
  ds.SetTitle (title);
  ds.SetStyle(Gnuplot2dDataset::LINES);
  
  for (auto it = values.begin(); it != values.end(); ++it) {
    
    ds.Add(t.GetSeconds(), *it);
    t += Seconds(gran);
  }
        
  plot.AddDataset(ds);
  
  std::ofstream f (tr_name + "_" + file_ext + ".plt");
  plot.GenerateOutput(f);
  
  popen( 
    ("gnuplot -c " + (tr_name + "_" + file_ext + ".plt")).c_str(), "r");
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
  
  experiment.Run();
  
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