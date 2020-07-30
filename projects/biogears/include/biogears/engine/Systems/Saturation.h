/**************************************************************************************
Copyright 2015 Applied Research Associates, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use
this file except in compliance with the License. You may obtain a copy of the License
at:
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed under
the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
**************************************************************************************/

#pragma once

#include <functional>

#include <biogears/cdm/CommonDataModel.h>
#include <biogears/exports.h>
#include <biogears/cdm/system/physiology/SETissueSystem.h>
#include <biogears/engine/Controller/BioGearsSystem.h>
#include <biogears/chrono/stop_watch.tci.h>

namespace biogears {
class SESubstance;
class SELiquidCompartment;
class SELiquidSubstanceQuantity;
class BioGears;
class SEScalarFraction;
class SEScalarTemperature;

/**
* @brief
* The %SaturationCalculator class holds the blood gas distribution model.
*/
class BIOGEARS_API SaturationCalculator : public Loggable {
protected:
  friend class BioGears;
  friend class BioGearsEngineTest;

  biogears::StopWatch<std::chrono::nanoseconds> satWatch;
  double solverTime;
  double distributeTime;
  double setupTime;

  SaturationCalculator(BioGears& bg);
  BioGears& m_data;

public:
  virtual ~SaturationCalculator();

  void Initialize(SESubstanceManager& substances);
  void CalculateSaturation(SELiquidCompartment& cmpt);

protected:
  double CalculateStrongIonDifference(SELiquidCompartment& cmpt);
  double NewtonRaphsonSolver(std::function<double(double)> f, std::function<double(double)> fPrime, double x0, double tol, int maxIts);

  // All properties are stateless and are set by either the Initialize method or SetBodyState method
  SESubstance* m_O2;
  SESubstance* m_Hb;
  SESubstance* m_HbO2;
  SESubstance* m_CO2;
  SESubstance* m_CO;
  SESubstance* m_HbCO;
  SESubstance* m_HCO3;
  SESubstance* m_HbCO2;
  // Used for conversions
  double m_O2_g_Per_mol;
  double m_CO2_g_Per_mol;
  double m_HCO3_g_Per_mol;
  double m_Hb_g_Per_mol;
  double m_HbO2_g_Per_mol;
  double m_HbCO2_g_Per_mol;
  // This is the current compartment and the quantities we are balancing
  SELiquidSubstanceQuantity* m_subO2Q;
  SELiquidSubstanceQuantity* m_subCO2Q;
  SELiquidSubstanceQuantity* m_subCOQ;
  SELiquidSubstanceQuantity* m_subHbCOQ;
  SELiquidSubstanceQuantity* m_subHCO3Q;
  SELiquidSubstanceQuantity* m_subHbQ;
  SELiquidSubstanceQuantity* m_subHbO2Q;
  SELiquidSubstanceQuantity* m_subHbCO2Q;
};
}