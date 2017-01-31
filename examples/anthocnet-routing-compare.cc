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
 * Author: Leon Tan
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

/*
 * This example program allows one to run ns-3 DSDV, AODV, or OLSR under
 * a typical random waypoint mobility model.
 *
 * By default, the simulation runs for 200 simulated seconds, of which
 * the first 50 are used for start-up time.  The number of nodes is 50.
 * Nodes move according to RandomWaypointMobilityModel with a speed of
 * 20 m/s and no pause time within a 300x1500 m region.  The WiFi is
 * in ad hoc mode with a 2 Mb/s rate (802.11b) and a Friis loss model.
 * The transmit power is set to 7.5 dBm.
 *
 * It is possible to change the mobility and density of the network by
 * directly modifying the speed and the number of nodes.  It is also
 * possible to change the characteristics of the network by changing
 * the transmit power (as power increases, the impact of mobility
 * decreases and the effective density increases).
 *
 * By default, there are 10 source/sink data pairs sending UDP data
 * at an application rate of 2.048 Kb/s each.    This is typically done
 * at a rate of 4 64-byte packets per second.  Application data is
 * started at a random time between 50 and 51 seconds and continues
 * to the end of the simulation.
 *
 * The program outputs a few items:
 * - packet receptions are notified to stdout such as:
 *   <timestamp> <node-id> received one packet from <src-address>
 * - each second, the data reception statistics are tabulated and output
 *   to a comma-separated value (csv) file
 * - some tracing and flow monitor configuration that used to work is
 *   left commented inline in the program
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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("manet-routing-compare");

// All the important output handlers can be global
std::ofstream out;



class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (int nSinks, double txp);
  std::string CommandSetup (int argc, char **argv);

private:
  Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node);
  void ReceivePacket (Ptr<Socket> socket);
  
  void OnOffTxTracer(Ptr<Packet const> packet);
  void SinkRxTracer(Ptr<Packet const> packet, const Address& address);
  
  void CheckThroughput ();

  uint32_t port;
  uint32_t bytesTotal;
  uint32_t packetsReceived;

  int m_nSinks;
  std::string m_protocolName;
  double m_txp;
  bool m_traceMobility;
  uint32_t m_protocol;
};

RoutingExperiment::RoutingExperiment ()
  : port (9),
    bytesTotal (0),
    packetsReceived (0),
    m_traceMobility (false),
    m_protocol (2) // ANTHOCNET
{
}

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (senderAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void RoutingExperiment::ReceivePacket (Ptr<Socket> socket) {
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
      bytesTotal += packet->GetSize ();
      packetsReceived += 1;
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}

void RoutingExperiment::OnOffTxTracer(Ptr<Packet const> packet) {
  
  std::ostringstream oss;
  oss << Simulator::Now ().GetSeconds () << "s Send Packet: " << *packet;
  NS_LOG_UNCOND(oss.str());
  
}

void RoutingExperiment::SinkRxTracer(Ptr<Packet const> packet, const Address& address) {
  
  std::ostringstream oss;
  oss << Simulator::Now ().GetSeconds () << "s Recv Packet: " << *packet << " Address: " << address;
  NS_LOG_UNCOND(oss.str());
  
}

void RoutingExperiment::CheckThroughput () {
  double kbs = (bytesTotal * 8.0) / 1000;
  bytesTotal = 0;

  out << (Simulator::Now ()).GetSeconds () << ","
      << kbs << ","
      << packetsReceived << ","
      << m_nSinks << ","
      << m_protocolName << ","
      << m_txp << ""
      << std::endl;

  packetsReceived = 0;
  Simulator::Schedule (Seconds (1.0), &RoutingExperiment::CheckThroughput, this);
}

Ptr<Socket>
RoutingExperiment::SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));

  return sink;
}

std::string RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd;
  //cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  
  //cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR", m_protocol);
  cmd.AddValue ("protocol", "1=AODV;2=ANTHOCNET", m_protocol);
  cmd.Parse (argc, argv);
  return "blob";
}

int main (int argc, char *argv[]) {
  RoutingExperiment experiment;
  
  timeval start;
  gettimeofday(&start, NULL);
  
  time_t tim = time(0);
  struct tm* now = localtime(&tim);
  
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

  int nSinks = 20;
  double txp = 7.5;

  experiment.Run (nSinks, txp);
  
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


void RoutingExperiment::Run (int nSinks, double txp) {
  Packet::EnablePrinting ();
  m_nSinks = nSinks;
  m_txp = txp;

  int nWifis = 50;

  double TotalTime = 200.0;
  std::string rate ("2048bps");
  std::string phyMode ("DsssRate11Mbps");
  std::string tr_name ("anthocnet-routing-compare");
  int nodeSpeed = 20; //in m/s
  int nodePause = 0; //in s
  m_protocolName = "protocol";

  
  out = std::ofstream(tr_name + ".csv");
  out << "SimulationSecond," <<
    "ReceiveRate," <<
    "PacketsReceived," <<
    "NumberOfSinks," <<
    "RoutingProtocol," <<
    "TransmissionPower" <<
  std::endl;
  
  
  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  NodeContainer adhocNodes;
  adhocNodes.Create (nWifis);

  // setting up wifi phy and channel using helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
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

  OnOffHelper onoff1 ("ns3::UdpSocketFactory", Address());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
  
  PacketSinkHelper sink1 ("ns3::UdpSocketFactory", Address());
  
  // Start the Sinks
  for (int i = 0; i < nSinks; i++) {
    
    // TODO: Set the Local address on sink1
    
    AddressValue localAddress (InetSocketAddress(adhocInterfaces.GetAddress (i), port));
    sink1.SetAttribute("Local", localAddress);
    
    ApplicationContainer temp = sink1.Install(adhocNodes.Get(i));
    temp.Start(Seconds(20.0));
    temp.Stop(Seconds(TotalTime));
  }
  
  // Start the OnnOff Applications
  for (int i = nSinks; i < nWifis; i++) {
    
    // Get the addresses of the Nodes to send to
    AddressValue remoteAddress (InetSocketAddress(adhocInterfaces.GetAddress (i % nSinks), port));
    onoff1.SetAttribute ("Remote", remoteAddress);
    
    // Start the OnOffApplication at a random time between the set papams
    Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>();
    ApplicationContainer temp = onoff1.Install(adhocNodes.Get(i));
    // TODO: Make starttime settable
    temp.Start (Seconds (var->GetValue (20.0,21.0)));
    temp.Stop (Seconds (TotalTime));
  }
  
  // Enable Rx and Tx tracers to measure the packets, that are sent
  std::string OnOffTracePath = "/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx";
  std::string SinkTracePath = "/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx" ;
  Config::ConnectWithoutContext (OnOffTracePath, MakeCallback(&RoutingExperiment::OnOffTxTracer, this));
  Config::ConnectWithoutContext (SinkTracePath, MakeCallback(&RoutingExperiment::SinkRxTracer, this));
  
  // Enable PCAP on physical level, to allow in detail offline debugging of the protocol
  wifiPhy.EnablePcap((tr_name + ".pcap"), adhocNodes);
  
  // Set the Gnuplot output
  // Set GnuplotHelper to plot packet byte count
  std::string packetProbe = "ns3::Ipv4PacketProbe";
  std::string packetPath = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";
  
  GnuplotHelper packetPlotHelper;
  packetPlotHelper.ConfigurePlot((tr_name + "_bytecount"),
    "Packet Byte Count vs. Time",
    "Time (Seconds)",
    "Packet Byte Count",
    "eps");

  packetPlotHelper.PlotProbe(packetProbe,
    packetPath,
    "OutputBytes",
    "Packet Byte Count",
    GnuplotAggregator::KEY_BELOW);
  
  // TODO: What are those?
  /*FileHelper bytefileHelper;
  bytefileHelper.ConfigureFile(tr_name, FileAggregator::FORMATTED);
  
  bytefileHelper.Set2dFormat ("Time (Seconds) = %.3e\tPacket Byte Count = %.0f");
  bytefileHelper.WriteProbe (packetProbe, packetPath, "OutputBytes");*/
  
  // I don't know, what these are good for. 
  // But free output is free
  // TODO: Read up what these output files mean
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream ((tr_name + ".tr").c_str());
  wifiPhy.EnableAsciiAll (osw);
  AsciiTraceHelper ascii1;
  MobilityHelper::EnableAsciiAll (ascii1.CreateFileStream (tr_name + ".mob"));

  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();
  

  NS_LOG_INFO ("Run Simulation.");

  CheckThroughput ();

  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();
  Simulator::Destroy ();
  
  flowmon->SerializeToXmlFile ((tr_name + ".flowmon").c_str(), false, false);
}

