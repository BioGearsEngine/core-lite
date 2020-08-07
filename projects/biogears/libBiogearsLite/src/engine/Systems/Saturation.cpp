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
#include <biogears/engine/Systems/Saturation.h>

#include <algorithm>
#include <cmath>
//Products Includes
#include <biogears/cdm/compartment/substances/SELiquidSubstanceQuantity.h>
#include <biogears/cdm/properties/SEScalarAmountPerVolume.h>
#include <biogears/cdm/properties/SEScalarFraction.h>
#include <biogears/cdm/properties/SEScalarInversePressure.h>
#include <biogears/cdm/properties/SEScalarMassPerAmount.h>
#include <biogears/cdm/properties/SEScalarMassPerVolume.h>
#include <biogears/engine/Controller/BioGears.h>

//#define VERBOSE
namespace biogears {

SaturationCalculator::SaturationCalculator(BioGears& bg)
  : Loggable(bg.GetLogger())
  , m_data(bg)
{
  Initialize(bg.GetSubstances());
}

void SaturationCalculator::Initialize(SESubstanceManager& substances)
{
  m_Logger = substances.GetLogger();
  m_O2 = substances.GetSubstance("Oxygen");
  m_CO2 = substances.GetSubstance("CarbonDioxide");
  m_CO = substances.GetSubstance("CarbonMonoxide");
  m_Hb = substances.GetSubstance("Hemoglobin");
  m_HbO2 = substances.GetSubstance("Oxyhemoglobin");
  m_HbCO2 = substances.GetSubstance("Carbaminohemoglobin");
  m_HbCO = substances.GetSubstance("Carboxyhemoglobin");
  m_HCO3 = substances.GetSubstance("Bicarbonate");

  if (m_O2 == nullptr)
    Fatal("Oxygen Definition not found");
  if (m_CO2 == nullptr)
    Fatal("CarbonDioxide Definition not found");
  if (m_CO == nullptr)
    Fatal("CarbonMonoxide Definition not found");
  if (m_Hb == nullptr)
    Fatal("Hemoglobin Definition not found");
  if (m_HbO2 == nullptr)
    Fatal("Oxyhemoglobin Definition not found");
  if (m_HbCO2 == nullptr)
    Fatal("Carbaminohemoglobin Definition not found");
  if (m_HbCO == nullptr)
    Fatal("Carboxyhemoglobin Definition not found");
  if (m_HCO3 == nullptr)
    Fatal("Bicarbonate Definition not found");

  m_O2_g_Per_mol = m_O2->GetMolarMass(MassPerAmountUnit::g_Per_mol);
  m_CO2_g_Per_mol = m_CO2->GetMolarMass(MassPerAmountUnit::g_Per_mol);
  m_HCO3_g_Per_mol = m_HCO3->GetMolarMass(MassPerAmountUnit::g_Per_mol);
  m_Hb_g_Per_mol = m_Hb->GetMolarMass(MassPerAmountUnit::g_Per_mol);
  m_HbO2_g_Per_mol = m_HbO2->GetMolarMass(MassPerAmountUnit::g_Per_mol);
  m_HbCO2_g_Per_mol = m_HbCO2->GetMolarMass(MassPerAmountUnit::g_Per_mol);
}

SaturationCalculator::~SaturationCalculator()
{
}

void SaturationCalculator::CalculateSaturation(SELiquidCompartment& cmpt)
{
  //Set up substance quantity variables
  m_subO2Q = nullptr;
  m_subCO2Q = nullptr;
  m_subHbQ = nullptr;
  m_subHbO2Q = nullptr; //Amount of O2 that is bound to hemoglobin  (i.e. 4 * amount of hemoglobin with O2 bound)
  m_subHbCO2Q = nullptr; //Amount of CO2 that is bound to hemoglobin (i.e. 4 * amount of hemoglobin with CO2 bound)
  m_subHCO3Q = nullptr;

  m_subO2Q = cmpt.GetSubstanceQuantity(*m_O2);
  m_subCO2Q = cmpt.GetSubstanceQuantity(*m_CO2);
  m_subHbQ = cmpt.GetSubstanceQuantity(*m_Hb);
  m_subHbO2Q = cmpt.GetSubstanceQuantity(*m_HbO2);
  m_subHbCO2Q = cmpt.GetSubstanceQuantity(*m_HbCO2);
  m_subHCO3Q = cmpt.GetSubstanceQuantity(*m_HCO3);

  //Blood parameters
  double hct = m_data.GetBloodChemistry().HasHematocrit() ? m_data.GetBloodChemistry().GetHematocrit().GetValue() : 0.45; //hematocrit
  double rbc = 0.69; //Gibbs-Donan ratio of HCO3 across red blood cell membrane (function of pH, but assumed standard value)
  double alphaO2_M_Per_mmHg = 1.37e-6; //O2 solubility in plasma
  double alphaCO2_M_Per_mmHg = 3.07e-5; //CO2 solubility in plasma
  double beta = 1.0 / 2.7; //exponent in [O2]-saturation relationship
  double p50_s = 26.8; //Standard O2 p50 in blood
  double pCO2_s = 40.0; // standard CO2 partial pressure in blood; mmHg
  double pH_s = 7.24; // standard pH in RBCs; unitless
  double temp_s = 37.0; // standard temperature in blood; degC

  //Only process compartments with hemoglobin
  double HbTotal_mM = m_subHbQ->GetMolarity(AmountPerVolumeUnit::mmol_Per_L);
  if (HbTotal_mM < ZERO_APPROX) {
    return;
  }

  //Get O2 and CO2 species and determine total amount of each gas
  double O2Dissolved_mM = m_subO2Q->GetMolarity(AmountPerVolumeUnit::mmol_Per_L);
  double O2Bound_mM = m_subHbO2Q->GetMolarity(AmountPerVolumeUnit::mmol_Per_L);
  double CO2Dissolved_mM = m_subCO2Q->GetMolarity(AmountPerVolumeUnit::mmol_Per_L);
  double CO2Bound_mM = m_subHbCO2Q->GetMolarity(AmountPerVolumeUnit::mmol_Per_L);
  double HCO3_Plasma_mM = m_subHCO3Q->GetMolarity(AmountPerVolumeUnit::mmol_Per_L); //Needs to be put on whole blood basis
  double O2Total_mM = O2Dissolved_mM + O2Bound_mM; //Assuming that O2 concentrations are whole blood
  double CO2Total_mM = CO2Dissolved_mM + HCO3_Plasma_mM; //Assuming that this is plasma value of total CO2 (if it were whole blood, we'd need to make more assumptions about distribution of HCO3 in RBCs)

  //--Step 1: Estimate pH using Chiari, 1994
  double NormalBB_mM = 46.2; //normal buffer base concentration
  double HPr0_mM = 39.8; //39.8; //total protein concentration, constant
  double KaCO2_mM = std::pow(10.0, -6.1) * 1000.0; //HCO3 / CO2 equilibrium constant
  double KaPr_mM = std::pow(10.0, -7.3) * 1000.0; //Hpr / Pr equilibrium constant

  //Adjust buffer base from normal--assume it is equivalent to change in SID from normal
  double strongIonDifference_mM = m_data.GetState() > EngineState::InitialStabilization ? CalculateStrongIonDifference(cmpt) : 42.0;
  double normalSID_mM = cmpt.GetStrongIonDifferenceBaseline().GetValue(AmountPerVolumeUnit::mmol_Per_L);
  double deltaBufferBase_mM = strongIonDifference_mM - normalSID_mM;
  double bufferBase_mM = NormalBB_mM + deltaBufferBase_mM;
  bufferBase_mM = std::max(bufferBase_mM, 0.0);   //Exercise can give extreme, unrealistic latcate (and very low SID values).  Make sure this stays positive

  //Constants for pH model
  double b2 = bufferBase_mM;
  double b1 = bufferBase_mM * (KaCO2_mM + KaPr_mM) - KaCO2_mM * CO2Total_mM - KaPr_mM * HPr0_mM;
  double b0 = KaCO2_mM * KaPr_mM * (bufferBase_mM - CO2Total_mM - HPr0_mM);

  //Use last [H+] as initial guess for Newton-Raphson solver and set up function and derivative required by solver
  double HpPlasma_mM = std::pow(10.0, -cmpt.GetPH().GetValue()) * 1000.0;
  auto hp_function = [&](double Hp_mM) { return b2 * Hp_mM * Hp_mM + b1 * Hp_mM + b0; };
  auto hp_derivative = [&](double Hp_mM) { return 2.0 * b2 * Hp_mM + b1; };
  HpPlasma_mM = NewtonRaphsonSolver(hp_function, hp_derivative, HpPlasma_mM, 1.0e-6, 50);

  //--Step 2:  Now that [H+] is known, find pH and use it to determine distribution of carbon species between dissolved CO2 and bicarbonate
  double pHPlasma = -std::log10(HpPlasma_mM / 1000.0);
  double pHRBC = 0.795 * pHPlasma + 1.357;
  double CO2Dissolved_mM_next = CO2Total_mM / (1.0 + std::pow(10.0, pHPlasma - 6.1));
  double HCO3Plasma_mM_next = CO2Total_mM - CO2Dissolved_mM_next;
  double pCO2_mmHg = (CO2Dissolved_mM_next / 1000.0) / alphaCO2_M_Per_mmHg;

  //--Step 3: Calculate effect of PCO2 and [H+] on O2 saturation midpoint
  double pHdiff = pHRBC - pH_s;
  double pCO2diff = pCO2_mmHg - pCO2_s;
  double coreTemp = m_data.GetEnergy().HasCoreTemperature() ? m_data.GetEnergy().GetCoreTemperature(TemperatureUnit::C) : 37.0;
  double tempDiff = coreTemp - temp_s;

  //Use relationships from Dash/Baissingwaithe, 2016
  double p50Delta_pH = p50_s - 25.535 * pHdiff + 10.646 * pHdiff * pHdiff - 1.764 * pHdiff * pHdiff * pHdiff;
  double p50Delta_CO2 = p50_s + 1.273e-1 * pCO2diff + 1.083e-4 * pCO2diff * pCO2diff;
  double p50Delta_T = p50_s + 1.435 * tempDiff + 4.163e-2 * tempDiff * tempDiff + 6.86e-4 * tempDiff * tempDiff * tempDiff;
  double P50 = p50_s * (p50Delta_pH / p50_s) * (p50Delta_CO2 / p50_s) * (p50Delta_T / p50_s);

  //--Step 4 : Solve for O2 saturation and dissolved concentration
  //Use last saturation as guess for solver, set up function to solve for O2 saturation (combines conservation of O2 mass and saturation relationship from Dash/BaissaingWaithe, 2016)
  double O2Sat = m_subO2Q->GetSaturation().GetValue();
  auto sat_function = [&](double sat) { return (alphaO2_M_Per_mmHg * 1000.0) * P50 * std::pow(sat / (1.0 - sat), beta) + 4.0 * HbTotal_mM * sat - O2Total_mM; };
  auto sat_derivative = [&](double sat) { return beta * (alphaO2_M_Per_mmHg * 1000.0) * P50 * std::pow(sat / (1.0 - sat), beta - 1) * (1.0 / ((1.0 - sat) * (1.0 - sat))) + 4.0 * HbTotal_mM; };
  O2Sat = NewtonRaphsonSolver(sat_function, sat_derivative, O2Sat, 1.0e-6, 50);

  //Calculate next O2 bound / dissolved plasma concentrations
  double O2Bound_mM_next = 4.0 * HbTotal_mM * O2Sat;
  double O2Dissolved_mM_next = O2Total_mM - O2Bound_mM_next;

  //Update and balance by molarity (not setting HbCO2 or CO2 saturation since we are assuming they stay constant)
  m_subCO2Q->GetMolarity().SetValue(CO2Dissolved_mM_next, AmountPerVolumeUnit::mmol_Per_L);
  m_subCO2Q->Balance(BalanceLiquidBy::Molarity);
  m_subO2Q->GetMolarity().SetValue(O2Dissolved_mM_next, AmountPerVolumeUnit::mmol_Per_L);
  m_subO2Q->Balance(BalanceLiquidBy::Molarity);
  m_subO2Q->GetSaturation().SetValue(O2Sat);
  m_subHCO3Q->GetMolarity().SetValue(HCO3Plasma_mM_next, AmountPerVolumeUnit::mmol_Per_L);
  m_subHCO3Q->Balance(BalanceLiquidBy::Molarity);
  m_subHbO2Q->GetMolarity().SetValue(O2Bound_mM_next, AmountPerVolumeUnit::mmol_Per_L);
  m_subHbO2Q->Balance(BalanceLiquidBy::Molarity);
  cmpt.GetPH().SetValue(pHPlasma);
}

double SaturationCalculator::CalculateStrongIonDifference(SELiquidCompartment& cmpt)
{
  double sodium_mM = cmpt.GetSubstanceQuantity(m_data.GetSubstances().GetSodium())->GetMolarity(AmountPerVolumeUnit::mmol_Per_L);
  double potassium_mM = cmpt.GetSubstanceQuantity(m_data.GetSubstances().GetPotassium())->GetMolarity(AmountPerVolumeUnit::mmol_Per_L);
  double chloride_mM = cmpt.GetSubstanceQuantity(m_data.GetSubstances().GetChloride())->GetMolarity(AmountPerVolumeUnit::mmol_Per_L);
  double lactate_mM = cmpt.GetSubstanceQuantity(m_data.GetSubstances().GetLactate())->GetMolarity(AmountPerVolumeUnit::mmol_Per_L);
  double otherIons_mM = -4.5;

  return sodium_mM + potassium_mM - chloride_mM - lactate_mM + otherIons_mM;
}

double SaturationCalculator::NewtonRaphsonSolver(std::function<double(double)> f, std::function<double(double)> fPrime, double x0, double tol, int maxIts)
{
  double x_n = x0;
  double x_n_1 = 10.0 * x_n; //Initialize "next" value to something way different from starting value
  int its = 0;

  while (its < maxIts) {
    x_n_1 = x_n - f(x_n) / fPrime(x_n);
    ++its;
    if (std::abs(x_n_1 - x_n) / x_n <= tol) {
      break;
    }
    x_n = x_n_1;
  }
  if (its == maxIts) {
    Info("Saturation::NewtonRaphsonSolver: Maximum iterations exceeded.  Setting to previous value.");
    x_n_1 = x0;
  }
  return x_n_1;
}

}