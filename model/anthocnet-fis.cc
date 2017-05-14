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

#include <string>
#include "anthocnet-fis.h"

using namespace fl;

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("AntHocNetRoutingFis");
namespace ahn{  

AntHocNetFis::AntHocNetFis() {
}

AntHocNetFis::~AntHocNetFis() {}

NS_OBJECT_ENSURE_REGISTERED (AntHocNetFis);

TypeId AntHocNetFis::GetTypeId() {
  static TypeId tid = TypeId("ns3::ahn::AntHocNetFis")
  .SetParent<Object> ()
  .SetGroupName("AntHocNet")
  .AddConstructor<AntHocNetFis>()
  
  .AddAttribute ("FisFile",
    "The name of the .fis file to use as fussy inference system",
    StringValue("src/anthocnet/fis/simple_control_analysis.fis"),
    MakeStringAccessor(&AntHocNetFis::fis_file),
    MakeStringChecker()
  )
  ;
  return tid;
}



void AntHocNetFis::Init() {
  this->engine = FisImporter().fromFile(this->fis_file);
  
  std::string status;
  if (!engine->isReady(&status)) {
    NS_LOG_ERROR("Fis file could not be loaded");
  }
  
  this->input1 = this->engine->getInputVariable(0);
  this->input2 = this->engine->getInputVariable(1);
  
  this->output = this->engine->getOutputVariable(0);
}

double AntHocNetFis::Eval(double in1, double in2) {
  
  this->input1->setValue(in1);
  this->input2->setValue(in2);
  this->engine->process();
  
  return this->output->getValue();;
}

void AntHocNetFis::DoDispose() {
}

void AntHocNetFis::DoInitialize() {
}

// End of namespaces
}
}