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

#include "sim-database.h"

namespace ns3 {
//NS_LOG_CONPONENT_DEFINE ("AntHocNetSimDatabase");
namespace ahn {

SimDatabase::SimDatabase():
packet_seqno(0),
transmission_seqno(0)
{}

SimDatabase::~SimDatabase() {
  
}

NS_OBJECT_ENSURE_REGISTERED(SimDatabase);

TypeId SimDatabase::GetTypeId() {
  static TypeId tid = TypeId("ns3::ahn::SimDatabase")
  .SetParent<Object>()
  .SetGroupName("AntHocNet")
  .AddConstructor<SimDatabase>()
  ;
  return tid;
}

void SimDatabase::DoDispose() {
}

void SimDatabase::DoInitialize() {
  
}
// End of namespaces
}
}