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

#ifndef SIM_APP_H
#define SIM_APP_H

#include "ns3/simulator.h"
#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable-stream.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/socket.h"
#include "ns3/boolean.h"
#include "ns3/pointer.h"
#include "ns3/packet.h"


#include "sim-database.h"

namespace ns3 {
namespace ahn {

class SimApplication : public Application {
public:
  
  static TypeId GetTypeId();
  
  SimApplication();
  virtual ~SimApplication();
  
  Ptr<Socket> GetSocket() const;
  
  int64_t AssignStreams (int64_t stream);
  
  virtual void DoDispose(void);
  
  virtual void StartApplication(void);
  virtual void StopApplication(void);
  
protected:
  //virtual void DoInitialize(void);
  
private:
  
  bool send_mode;
  
  void NextTxEvent();
  
  void Send(Ptr<Socket> socket, Ptr<Packet> packet, 
                                Ipv4Address dst);
  void Recv(Ptr<Socket> socket);
  // config
  
  uint16_t port;
  
  uint32_t packet_size;
  uint32_t packet_rate;
  
  Address local;
  Address remote;
  
  Ptr<SimDatabase> db;
  
  // state
  Ptr<Socket> socket;
  Ptr<UniformRandomVariable> random;
  EventId tx_event;
  
};

// End of namespaces
}
}

#endif /* SIM_APP_H */