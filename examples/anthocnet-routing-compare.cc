/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of Kansas
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
 *
 * Author: Leon Tan adapted from manet-routing-compare
 * Author of original manet-routing-compare: Justin Rohrer <rohrej@ittc.ku.edu>
 *  
 * 
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#include <fstream>
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
//#include "ns3/pcap-writer.h"


#include "ns3/aodv-module.h"
#include "ns3/anthocnet-module.h"
#include "ns3/trace-helper.h"
#include "ns3/applications-module.h"

#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("manet-routing-compare");

// All the important output handlers can be global
//std::ofstream out;
std::ofstream ant_drop_output;
std::ofstream data_drop_output;


class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (double txp);
  std::string CommandSetup (int argc, char **argv);

private:
  // The tracer callbacks for the applications
  void OnOffTxTracer(Ptr<Packet const> packet);
  void SinkRxTracer(Ptr<Packet const> packet, const Address& address);
  
  // The tracer callbacks for the Ip layer
  void IpTxTracer(Ptr<Packet const> packet, Ptr<Ipv4> ipv4, uint32_t interface);
  void IpRxTracer(Ptr<Packet const> packet, Ptr<Ipv4> ipv4, uint32_t interface);
  void IpDropTracer(const Ipv4Header& header, Ptr<Packet const> packet, Ipv4L3Protocol::DropReason reason, Ptr<Ipv4> ipv4, uint32_t interface);
  
  void PhyTxDropTracer(Ptr<Packet const> packet);
  void PhyRxDropTracer(Ptr<Packet const> packet);
  
  void AntDropTracer(Ptr<Packet const> packet, std::string reason, Ipv4Address address);
  void DataDropTracer(Ptr<Packet const> packet, std::string reason, Ipv4Address address);
  
  
  uint32_t m_nWifis;
  uint32_t m_nSinks;
  
  uint32_t m_total_time;
  
  std::vector<ApplicationContainer*> apps;
  
  void Evaluate();
  
  // Plots
  Gnuplot* plot_droprate_simple;
  Gnuplot2dDataset* dataset_droprate_simple;
  Gnuplot2dDataset* dataset_droprate_counted;
  
  uint32_t port;
  uint32_t bytesTotal;
  
  uint32_t data_dropped;
  uint32_t packets_received;
  uint32_t packets_sent;
  
  std::string m_protocolName;
  double m_txp;
  bool m_traceMobility;
  
  uint32_t m_protocol;
  uint32_t m_experiment;
  
  uint32_t m_app_start;
  
  bool m_output_iptxrx;
  
  bool m_generate_pcap;
  bool m_generate_asciitrace;
  bool m_generate_flowmon;
  
  
};

RoutingExperiment::RoutingExperiment () : 
    m_nWifis(10),
    m_nSinks(3),
    m_total_time(40),
    
    port(9),
    bytesTotal(0),
    
    data_dropped(0),
    packets_received(0),
    packets_sent(0),
    
    m_traceMobility (false),
    m_protocol (2), // ANTHOCNET
    m_experiment (2), // UDPEcho
    m_app_start(20),
    
    
    m_output_iptxrx(false),
    m_generate_pcap(false),
    m_generate_asciitrace(false),
    m_generate_flowmon(false)
    
    {}


std::string RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd;
  //cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
  cmd.AddValue("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue("appStart", "Start the OnnOff Application after that many seconds", m_app_start);
  
  cmd.AddValue("protocol", "1=AODV; 2=ANTHOCNET", m_protocol);
  cmd.AddValue("Time", "The total time of the experiment", m_total_time);
  cmd.AddValue("Experiment", "1=OnOff<--->Sink; 2=UDPEchoClient<--->UDPEchoServer", m_experiment);
  
  cmd.AddValue("nWifis", "The total number of nodes in this simulation", m_nWifis);
  cmd.AddValue("nSinks", "The total number of sinks/servers in this simulation", m_nSinks);
  
  cmd.AddValue("outputIpTxRx", "Specify, whether events on the Ip tracer should be printed", m_output_iptxrx);
  
  cmd.AddValue("generatePcap", "Specify, whether Pcap output should be generated", m_generate_pcap);
  cmd.AddValue("generateAsciitrace", "Specify, whether Asciitrace output should be generated", m_generate_asciitrace);
  cmd.AddValue("generateFlowmon", "Specify, whether Flow monitor output should be generated", m_generate_flowmon);
  
  cmd.Parse(argc, argv);
  return "blob";
}


// Tracers 
void RoutingExperiment::OnOffTxTracer(Ptr<Packet const> packet) {
  
  packets_sent++;
  
  std::ostringstream oss;
  oss << Simulator::Now().GetSeconds () << "s Send Packet: " << *packet;
  NS_LOG_UNCOND(oss.str());
  
}

void RoutingExperiment::SinkRxTracer(Ptr<Packet const> packet, const Address& address) {
  
  packets_received++;
  
  if (InetSocketAddress::IsMatchingType (address)) {
    InetSocketAddress addr = InetSocketAddress::ConvertFrom (address);
    std::ostringstream oss;
    oss << Simulator::Now().GetSeconds() << "s Recv Packet: " << *packet << " Address: " << addr.GetIpv4();
    NS_LOG_UNCOND(oss.str());
  }
  
}

void RoutingExperiment::IpTxTracer(Ptr<Packet const> packet, Ptr<Ipv4> ipv4, uint32_t interface) {
  
  if (m_output_iptxrx) {
    std::ostringstream oss;
    oss << Simulator::Now().GetSeconds() << "s IP Layer send " << *packet << " Address: " << ipv4->GetAddress(interface, 0) << " Interface: " << interface;
    NS_LOG_UNCOND(oss.str());
  }
  
}

void RoutingExperiment::IpRxTracer(Ptr<Packet const> packet, Ptr<Ipv4> ipv4, uint32_t interface) {
  
  if (m_output_iptxrx) {
    std::ostringstream oss;
    oss << Simulator::Now().GetSeconds() << "s IP Layer recv " << *packet << " Address: " << ipv4->GetAddress(interface, 0) << " Interface: " << interface;
    NS_LOG_UNCOND(oss.str());
  }
  
}

void RoutingExperiment::IpDropTracer(const Ipv4Header& header, Ptr<Packet const> packet, Ipv4L3Protocol::DropReason reason, Ptr<Ipv4> ipv4, uint32_t interface) {
  
  data_dropped++;
  
  //std::ostringstream oss;
  //oss << Simulator::Now().GetSeconds() << "s IP Layer dropped " << *packet << " Address: " << ipv4->GetAddress(interface, 0) << " Interface: " << interface << " Reason: " << reason;
  //NS_LOG_UNCOND(oss.str());
  
  data_drop_output << Simulator::Now().GetSeconds() << " IPLayer dropped at interface: " << interface << std::endl;
  data_drop_output << "\t Reason: " << reason << std::endl;
  data_drop_output << "\t Header: " << header << std::endl;
  data_drop_output << "\t Packet: " << packet << std::endl;
  
  
}

void RoutingExperiment::AntDropTracer(Ptr<Packet const> packet, std::string reason, Ipv4Address address) {
  
  ant_drop_output << Simulator::Now().GetSeconds() << " Ant dropped at address: " << address << std::endl;
  ant_drop_output << "\t Reason: " << reason << std::endl;
  ant_drop_output << "\t Packet: " << packet << std::endl;
  
}


void RoutingExperiment::DataDropTracer(Ptr<Packet const> packet, std::string reason, Ipv4Address address) {
  
  data_dropped++;
  
  
  data_drop_output << Simulator::Now().GetSeconds() << " Data dropped at address: " << address << std::endl;
  data_drop_output << "\t Reason: " << reason << std::endl;
  data_drop_output << "\t Packet: " << packet << std::endl;
  
}

void RoutingExperiment::PhyTxDropTracer(Ptr<Packet const> packet) {
  
  data_dropped++;
  data_drop_output << Simulator::Now().GetSeconds() << " PHYLayer Tx drop" << std::endl;
  data_drop_output << "\t Packet: " << packet << std::endl;
    
}

void RoutingExperiment::PhyRxDropTracer(Ptr<Packet const> packet) {
    
  data_dropped++;
  data_drop_output << Simulator::Now().GetSeconds() << " PHYLayer Rx drop" << std::endl;
  data_drop_output << "\t Packet: " << packet << std::endl;  
}

void RoutingExperiment::Evaluate () {
  
  double drop_rate_simple = 1.0 - ((double) packets_received / packets_sent);
  
  if (drop_rate_simple < 0) {
    drop_rate_simple = 0;
  }
  
  double drop_rate_counted = (double )data_dropped / packets_sent;
  
  
  // Do not record data, if 
  if (!(packets_sent == 0 || packets_received == 0)) {
    this->dataset_droprate_simple->Add(Simulator::Now().GetSeconds(), drop_rate_simple);
    
    if (m_protocol == 2) {
      this->dataset_droprate_counted->Add(Simulator::Now().GetSeconds(), drop_rate_counted);
    }
    
  }
  
  data_dropped = 0;
  packets_received = 0;
  packets_sent = 0;
  
  // Change the fill data
  if (m_experiment == 2) {
    for (uint32_t i = m_nSinks; i < m_nWifis; i++) { 
      
      Ptr<Application> appclient = apps[i]->Get(0);
      UdpEchoClient* client = dynamic_cast<UdpEchoClient*>(PeekPointer(appclient));
      
      std::stringstream css;
      css << "Source: " << i << " Destination: " << (i % m_nSinks) << " Time: " << Simulator::Now();
      
      client->SetFill(css.str());
      
    }
    
  }
  
  
  Simulator::Schedule (Seconds (1.0), &RoutingExperiment::Evaluate, this);
}

int main (int argc, char *argv[]) {
  RoutingExperiment experiment;
  
  timeval start;
  gettimeofday(&start, NULL);
  
  time_t tim = time(0);
  struct tm* now = localtime(&tim);
  
  experiment.CommandSetup(argc, argv);
  
  // Every experiments output is stored in its own folder
  // such that experiments can be compared later to see, how changes in the code effect
  // the simulation
  std::string dir_string("anthocnet_compare_" + std::to_string(now->tm_year + 1900) + "-"
    + std::to_string(now->tm_mon + 1) + "-" + std::to_string(now->tm_mday) + "-"
    + std::to_string(now->tm_hour) + "-" + std::to_string(now->tm_min) + "-"
    + std::to_string(now->tm_sec)
  );
  
  if (mkdir(dir_string.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
    std::cerr << "Could no create directory to store the results. Aborting" << std::endl;
    return -1;
  }
  
  if (chdir(dir_string.c_str()) == -1) {
    std::cerr << "Could not cd into the directory to store results. Aborting" << std::endl;
    return -1;
  }
  
  std::cout << dir_string << std::endl;
  double txp = 7.5;
  //double txp = 3.5;

  experiment.Run (txp);
  
  // Calculate the time
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

void RoutingExperiment::Run (double txp) {
  Packet::EnablePrinting ();
  m_txp = txp;

  double TotalTime = m_total_time;
  std::string rate ("2048bps");
  std::string phyMode ("DsssRate11Mbps");
  std::string tr_name ("anthocnet-routing-compare");
  int nodeSpeed = 20; //in m/s
  int nodePause = 0; //in s
  m_protocolName = "Protocol";
  
  Config::SetDefault("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault("ns3::OnOffApplication::DataRate",  StringValue (rate));

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  NodeContainer adhocNodes;
  adhocNodes.Create(m_nWifis);

  // setting up wifi phy and channel using helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  //wifiChannel.SetPropagationDelay ("ns3::RandomPropagationDelayModel");
  //wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  wifiPhy.Set ("TxPowerStart", DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);

  MobilityHelper mobilityAdhoc;
  int64_t streamIndex = 0; // used to get consistent mobility across scenarios

  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

  Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator> ();
  streamIndex += taPositionAlloc->AssignStreams (streamIndex);

  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
  std::stringstream ssPause;
  ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
  mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                  "Speed", StringValue (ssSpeed.str()),
                                  "Pause", StringValue (ssPause.str()),
                                  "PositionAllocator", PointerValue(taPositionAlloc));
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (adhocNodes);
  streamIndex += mobilityAdhoc.AssignStreams (adhocNodes, streamIndex);
  NS_UNUSED (streamIndex); // From this point, streamIndex is unused

  AodvHelper aodv;
  AntHocNetHelper ahn;
  
  if (m_protocol == 2) {
    // Open the drop tracers
    ant_drop_output = std::ofstream(tr_name + "_ant_drops.tr");
    data_drop_output = std::ofstream(tr_name + "_data_drops.tr");
  }
  
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  switch (m_protocol)
    {
    case 1:
      list.Add (aodv, 100);
      m_protocolName = "AODV";
      break;
    case 2:
      list.Add (ahn, 100);
      m_protocolName = "ANTHOCNET";
      break;
    default:
      NS_FATAL_ERROR ("No such protocol:" << m_protocol);
    }

  if (m_protocol < 3)
    {
      internet.SetRoutingHelper (list);
      internet.Install (adhocNodes);
    }

  NS_LOG_INFO ("assigning ip address");
  
  
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);
  
  // For the first experiment
  OnOffHelper onoff1 ("ns3::UdpSocketFactory", Address());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
  
  PacketSinkHelper sink1 ("ns3::UdpSocketFactory", Address());
  
  // For the second experiment
  UdpEchoServerHelper echoserver1(9);
  
  // Start the Receiver application
  for (uint32_t i = 0; i < m_nSinks; i++) {
    
    
    AddressValue localAddress (InetSocketAddress(adhocInterfaces.GetAddress (i), port));
    sink1.SetAttribute("Local", localAddress);
    
    ApplicationContainer* temp = new ApplicationContainer();
    apps.push_back(temp);
    
    //ApplicationContainer* temp = apps[0]*;
    if (m_experiment == 1) {
      *temp = sink1.Install(adhocNodes.Get(i));
    }
    else if (m_experiment == 2) {
      *temp = echoserver1.Install(adhocNodes.Get(i));
    }
    else {
      NS_LOG_UNCOND("Unknown experiment.");
      exit(-1);
    }
    
    temp->Start(Seconds(20.0));
    temp->Stop(Seconds(TotalTime));
    
    
    
  }
  
  // Start the Sender Applications
  for (uint32_t i = m_nSinks; i < m_nWifis; i++) {
    
    ApplicationContainer* temp = new ApplicationContainer();
    apps.push_back(temp);
    
    // Get the addresses of the Nodes to send to
    AddressValue remoteAddress (InetSocketAddress(adhocInterfaces.GetAddress (i % m_nSinks), port));
    
    if (m_experiment == 1) {
      onoff1.SetAttribute ("Remote", remoteAddress);
      // Start the OnOffApplication at a random time between the set papams
      *temp = onoff1.Install(adhocNodes.Get(i));
    }
    else if (m_experiment == 2) {
      
      UdpEchoClientHelper client(adhocInterfaces.GetAddress (i % m_nSinks), 9);
      client.SetAttribute ("MaxPackets", UintegerValue (1000));
      client.SetAttribute ("Interval", TimeValue (Seconds(1)));
      //client.SetAttribute ("PacketSize", UintegerValue (packetSize));
      *temp = client.Install(adhocNodes.Get(i));
      
    }
    else {
      NS_LOG_UNCOND("Unknown experiment.");
      exit(-1);
    }
    
    Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>();
    temp->Start (Seconds (var->GetValue (m_app_start, m_app_start + 1)));
    temp->Stop (Seconds (TotalTime));
  }
  
  // Prepare Plots
  
  // Prepare the simple droprate plot
  this->plot_droprate_simple = new Gnuplot(tr_name + "_droprate_simple.eps");
  this->plot_droprate_simple->SetTitle("Droprate simple of " + m_protocolName);
  this->plot_droprate_simple->SetTerminal("eps");
  this->plot_droprate_simple->SetLegend("Time [s]", "Droprate");
  
  this->dataset_droprate_simple = new Gnuplot2dDataset();
  this->dataset_droprate_simple->SetTitle("Droprate simple");
  this->dataset_droprate_simple->SetStyle(Gnuplot2dDataset::LINES);
  
  this->dataset_droprate_counted = new Gnuplot2dDataset();
  this->dataset_droprate_counted->SetTitle("Droprate counted");
  this->dataset_droprate_counted->SetStyle(Gnuplot2dDataset::LINES);
  
  if (m_experiment == 1) {
    // Enable Rx and Tx tracers to measure the packets, that are sent
    std::string OnOffTracePath = "/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx";
    Config::ConnectWithoutContext (OnOffTracePath, MakeCallback(&RoutingExperiment::OnOffTxTracer, this));
    
    std::string SinkTracePath = "/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx";
    Config::ConnectWithoutContext (SinkTracePath, MakeCallback(&RoutingExperiment::SinkRxTracer, this));
  }
  else if (m_experiment ==2 ) {
    
    std::string OnOffTracePath = "/NodeList/*/ApplicationList/*/$ns3::UdpEchoClient/Tx";
    Config::ConnectWithoutContext (OnOffTracePath, MakeCallback(&RoutingExperiment::OnOffTxTracer, this));
    
  }
    
  // Enable Tracers on the IP level
  //std::string IpTxPath = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";
  //Config::ConnectWithoutContext (IpTxPath, MakeCallback(&RoutingExperiment::IpTxTracer, this));
  
  //std::string IpRxPath = "/NodeList/*/$ns3::Ipv4L3Protocol/Rx";
  //Config::ConnectWithoutContext (IpRxPath, MakeCallback(&RoutingExperiment::IpRxTracer, this));
  
  std::string IpDropPath = "/NodeList/*/$ns3::Ipv4L3Protocol/Drop";
  Config::ConnectWithoutContext (IpDropPath, MakeCallback(&RoutingExperiment::IpDropTracer, this));
  
  std::string PhyTxDropPath = "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/TxDrop";
  Config::ConnectWithoutContext (PhyTxDropPath, MakeCallback(&RoutingExperiment::PhyTxDropTracer, this));
  
  std::string PhyRxDropPath = "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/RxDrop";
  Config::ConnectWithoutContext (PhyRxDropPath, MakeCallback(&RoutingExperiment::PhyRxDropTracer, this));
  
  if (m_protocol == 2) {
    std::string AntDropPath = "/NodeList/*/$ns3::ahn::RoutingProtocol/AntDrop";
    Config::ConnectWithoutContext (AntDropPath, MakeCallback(&RoutingExperiment::AntDropTracer, this));
    
    std::string DataDropPath = "/NodeList/*/$ns3::ahn::RoutingProtocol/DataDrop";
    Config::ConnectWithoutContext (DataDropPath, MakeCallback(&RoutingExperiment::DataDropTracer, this));
  }
  
  if (m_generate_pcap) {
    // Enable PCAP on physical level, to allow in detail offline debugging of the protocol
    //wifiPhy.EnablePcap((tr_name + ".pcap"), adhocNodes);
    internet.EnablePcapIpv4((tr_name + ".pcap"), adhocNodes);
    
  }
  
  
  if (m_generate_asciitrace) {
    // Read up what these output files mean, because I do not know
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream ((tr_name + ".tr").c_str());
    wifiPhy.EnableAsciiAll (osw);
    AsciiTraceHelper ascii1;
    MobilityHelper::EnableAsciiAll (ascii1.CreateFileStream (tr_name + ".mob")); 
  }
  
  
  // Start the net animator
  AnimationInterface anim (tr_name + "_animation.xml");
  
  anim.EnablePacketMetadata ();
  anim.EnableIpv4RouteTracking(tr_name + "_route.xml", Seconds(0), Seconds(TotalTime), MilliSeconds(100));
  
  // reduce anim file, if only positions is needed
  anim.SkipPacketTracing();
  
  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  if (m_generate_flowmon) {
    
    flowmon = flowmonHelper.InstallAll ();
  }

  NS_LOG_INFO ("Run Simulation.");
  
  // Trigger the first evaluate loop
  Evaluate();

  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();
  Simulator::Destroy ();
  
  // Output the plotfiles
  
  // Output the simple droprate plot
  this->plot_droprate_simple->AddDataset(*this->dataset_droprate_simple);
  this->plot_droprate_simple->AddDataset(*this->dataset_droprate_counted);
  
  std::ofstream file_droprate_simple(tr_name + "_droprate_simple.plt");
  this->plot_droprate_simple->GenerateOutput(file_droprate_simple);
  file_droprate_simple.close();
  // Quick and dirty way to get the eps generated
  popen( ("gnuplot -c " + (tr_name + "_droprate_simple.plt")).c_str(), "r");
  
  if (m_generate_flowmon) {
    flowmon->SerializeToXmlFile ((tr_name + ".flowmon").c_str(), false, false);
  }
}

