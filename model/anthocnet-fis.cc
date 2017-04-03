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

#include "anthocnet-fis.h"

using namespace fl;

AntHocNetFis::AntHocNetFis() {
  this->engine = FisImporter()
    .fromFile("src/anthocnet/fis/simple_control_analysis.fis");
  
  this->input1 = this->engine->getInputVariable("total-pheromone");
  this->input2 = this->engine->getInputVariable("pheromone-percentage");
  
  this->output = this->engine->getOutputVariable("trust");
}

double AntHocNetFis::Eval(double in1, double in2) {
  
  this->input1->setValue(in1);
  this->input2->setValue(in2);
  this->engine->process();
  
  return this->output->getValue();
}