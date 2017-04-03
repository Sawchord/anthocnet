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
#ifndef ANTHOCNET_FIS_H
#define ANTHOCNET_FIS_H

#include "fl/Headers.h"
using namespace fl;

class AntHocNetFis {
public:
  AntHocNetFis();
  double Eval(double in1, double in2);
  
private:
  Engine* engine;
  InputVariable* input1;
  InputVariable* input2;
  OutputVariable* output;
  
};



#endif /* ANTHOCNET_FIS_H */