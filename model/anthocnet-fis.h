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
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/attribute.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"

using namespace fl;

namespace ns3 {
namespace ahn {
  
typedef std::map <std::pair<double, double>, double> FisCache;
  
class AntHocNetFis : public Object{
public:
  AntHocNetFis();
  ~AntHocNetFis();
  
  void Init();
  double Eval(double in1, double in2);
  
  static TypeId GetTypeId();
  //void Print(std::ostream& os) const;
  
  virtual void DoDispose();
protected:
  virtual void DoInitialize();
  
private:
  
  std::string fis_file;
  
  Engine* engine;
  InputVariable* input1;
  InputVariable* input2;
  OutputVariable* output;
  
  FisCache cache;
  
};

// End of namespaces
}
}

#endif /* ANTHOCNET_FIS_H */