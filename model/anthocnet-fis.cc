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

#include <string>
#include "anthocnet-fis.h"

using namespace fl;

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("AntHocNetRoutingFis");
namespace ahn{  

AntHocNetFis::AntHocNetFis() {
}

AntHocNetFis::~AntHocNetFis() {}

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
  .AddAttribute ("Input1Name",
    "The name of the first input",
    StringValue("total-pheromone"),
    MakeStringAccessor(&AntHocNetFis::in1_name),
    MakeStringChecker()
  )
  .AddAttribute ("Input2Name",
    "The name of the second input",
    StringValue("pheromone-percentage"),
    MakeStringAccessor(&AntHocNetFis::in2_name),
    MakeStringChecker()
  )
  .AddAttribute ("OutputName",
    "The name of the output",
    StringValue("trust"),
    MakeStringAccessor(&AntHocNetFis::out_name),
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
  
  this->input1 = this->engine->getInputVariable(this->in1_name);
  this->input2 = this->engine->getInputVariable(this->in2_name);
  
  this->output = this->engine->getOutputVariable(this->out_name);
}

double AntHocNetFis::Eval(double in1, double in2) {
  
  this->input1->setValue(in1);
  this->input2->setValue(in2);
  this->engine->process();
  
  return this->output->getValue();
}

void AntHocNetFis::DoDispose() {
}

void AntHocNetFis::DoInitialize() {
}

// End of namespaces
}
}