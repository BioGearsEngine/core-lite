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
#include <biogears/engine/Systems/Environment.h>

#include <biogears/cdm/circuit/fluid/SEFluidCircuit.h>
#include <biogears/cdm/circuit/thermal/SEThermalCircuit.h>
#include <biogears/cdm/properties/SEScalarArea.h>
#include <biogears/cdm/properties/SEScalarFraction.h>
#include <biogears/cdm/properties/SEScalarHeatCapacitancePerAmount.h>
#include <biogears/cdm/properties/SEScalarHeatCapacitancePerMass.h>
#include <biogears/cdm/properties/SEScalarHeatConductancePerArea.h>
#include <biogears/cdm/properties/SEScalarHeatResistance.h>
#include <biogears/cdm/properties/SEScalarHeatResistanceArea.h>
#include <biogears/cdm/properties/SEScalarLength.h>
#include <biogears/cdm/properties/SEScalarLengthPerTime.h>
#include <biogears/cdm/properties/SEScalarMass.h>
#include <biogears/cdm/properties/SEScalarMassPerAmount.h>
#include <biogears/cdm/properties/SEScalarMassPerTime.h>
#include <biogears/cdm/properties/SEScalarMassPerVolume.h>
#include <biogears/cdm/properties/SEScalarPower.h>
#include <biogears/cdm/properties/SEScalarPowerPerAreaTemperatureToTheFourth.h>
#include <biogears/cdm/properties/SEScalarPressure.h>
#include <biogears/cdm/properties/SEScalarTemperature.h>
#include <biogears/cdm/properties/SEScalarVolume.h>
#include <biogears/cdm/properties/SEScalarVolumePerTime.h>
#include <biogears/cdm/substance/SESubstanceConcentration.h>
#include <biogears/cdm/substance/SESubstanceFraction.h>
#include <biogears/cdm/system/environment/SEActiveCooling.h>
#include <biogears/cdm/system/environment/SEActiveHeating.h>
#include <biogears/cdm/system/environment/SEAppliedTemperature.h>
#include <biogears/cdm/utils/GeneralMath.h>

#include <biogears/engine/BioGearsPhysiologyEngine.h>
#include <biogears/engine/Controller/BioGears.h>
namespace BGE = mil::tatrc::physiology::biogears;

namespace biogears {
Environment::Environment(BioGears& bg)
  : SEEnvironment(bg.GetSubstances())
  , m_data(bg)
{
  Clear();
}

Environment::~Environment()
{
  Clear();
}

void Environment::Clear()
{
  SEEnvironment::Clear();
  m_Patient = nullptr;
  m_PatientActions = nullptr;
  m_EnvironmentActions = nullptr;
  m_AmbientGases = nullptr;
  m_AmbientAerosols = nullptr;
  m_EnvironmentCircuit = nullptr;
  m_ThermalEnvironment = nullptr;
  m_SkinNode = nullptr;
  m_ClothingNode = nullptr;
  m_EnclosureNode = nullptr;
  m_SkinToClothing = nullptr;
  m_ActiveHeatTransferRatePath = nullptr;
  m_ActiveTemperaturePath = nullptr;
  m_ActiveSwitchPath = nullptr;
  m_ClothingToEnclosurePath = nullptr;
  m_GroundToEnclosurePath = nullptr;
  m_ClothingToEnvironmentPath = nullptr;
  m_SkinToGroundPath = nullptr;
  m_GroundToEnvironmentPath = nullptr;
  m_EnvironmentSkinToGroundPath = nullptr;
  m_EnvironmentCoreToGroundPath = nullptr;
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Initializes system properties to valid homeostatic values.
//--------------------------------------------------------------------------------------------------
void Environment::Initialize()
{
  BioGearsSystem::Initialize();

  //Initialize all System Data outputs (inputs should be populated elsewhere)
  GetConvectiveHeatLoss().SetValue(0.0, PowerUnit::W);
  GetEvaporativeHeatLoss().SetValue(0.0, PowerUnit::W);
  GetRadiativeHeatLoss().SetValue(0.0, PowerUnit::W);
  GetRespirationHeatLoss().SetValue(0.0, PowerUnit::W);
  GetSkinHeatLoss().SetValue(0.0, PowerUnit::W);

  GetConvectiveHeatTranferCoefficient().SetValue(0.0, HeatConductancePerAreaUnit::W_Per_m2_K);
  GetEvaporativeHeatTranferCoefficient().SetValue(0.0, HeatConductancePerAreaUnit::W_Per_m2_K);
  GetRadiativeHeatTranferCoefficient().SetValue(0.0, HeatConductancePerAreaUnit::W_Per_m2_K);

  double patientDensity_g_Per_mL = m_Patient->GetBodyDensity(MassPerVolumeUnit::g_Per_mL);
  double patientMass_g = m_Patient->GetWeight(MassUnit::g);
  double patientHeight_m = m_Patient->GetHeight(LengthUnit::m);
  double pi = 3.14159;
  m_PatientEquivalentDiameter_m = std::pow(Convert(patientMass_g / patientDensity_g_Per_mL, VolumeUnit::mL, VolumeUnit::m3) / (pi * patientHeight_m), 0.5);

}

bool Environment::Load(const CDM::BioGearsEnvironmentData& in)
{
  if (!SEEnvironment::Load(in))
    return false;
  BioGearsSystem::LoadState();
  m_PatientEquivalentDiameter_m = in.PatientEquivalentDiameter_m();
  StateChange();
  return true;
}
CDM::BioGearsEnvironmentData* Environment::Unload() const
{
  CDM::BioGearsEnvironmentData* data = new CDM::BioGearsEnvironmentData();
  Unload(*data);
  return data;
}
void Environment::Unload(CDM::BioGearsEnvironmentData& data) const
{
  SEEnvironment::Unload(data);
  data.PatientEquivalentDiameter_m(m_PatientEquivalentDiameter_m);
}

void Environment::SetUp()
{
  // Patient and Actions
  m_Patient = &m_data.GetPatient();
  m_PatientActions = &m_data.GetActions().GetPatientActions();
  m_EnvironmentActions = &m_data.GetActions().GetEnvironmentActions();
  //Circuits
  m_EnvironmentCircuit = &m_data.GetCircuits().GetTemperatureCircuit();
  //Compartments
  m_AmbientGases = m_data.GetCompartments().GetGasCompartment(BGE::EnvironmentCompartment::Ambient);
  m_AmbientAerosols = m_data.GetCompartments().GetLiquidCompartment(BGE::EnvironmentCompartment::Ambient);
  //Nodes
  m_ThermalEnvironment = m_EnvironmentCircuit->GetNode(BGE::ThermalLiteNode::Environment);
  m_SkinNode = m_EnvironmentCircuit->GetNode(BGE::ThermalLiteNode::Skin);
  m_ClothingNode = m_SkinNode;
  m_EnclosureNode = m_ThermalEnvironment;
  //Paths
  m_EnvironmentCoreToGroundPath = m_EnvironmentCircuit->GetPath(BGE::ThermalLitePath::CoreToRef);
  m_GroundToEnvironmentPath = m_EnvironmentCircuit->GetPath(BGE::ThermalLitePath::RefToEnvironment);
  m_ClothingToEnvironmentPath = m_EnvironmentCircuit->GetPath(BGE::ThermalLitePath::EnvironmentToSkin); //RENAME ONCE ESTABLISHED...NO MORE CLOTHING DEFINED
  m_SkinToGroundPath = m_EnvironmentCircuit->GetPath(BGE::ThermalLitePath::SkinToGround);
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Resets environment parameters.
///
/// \details
/// This is called any time the environment change action/condition.  It sets the ambient node
/// values needed for the fluid systems.
//--------------------------------------------------------------------------------------------------
void Environment::StateChange()
{
  using namespace std::string_literals;
  if (m_AmbientGases == nullptr || m_AmbientAerosols == nullptr)
    return;

  // Add Gases to the environment
  //Check to make sure fractions sum to 1.0
  double totalFraction = 0.0;
  for (auto s : GetConditions().GetAmbientGases()) {
    SESubstance& sub = s->GetSubstance();
    totalFraction += s->GetFractionAmount().GetValue();
    m_data.GetSubstances().AddActiveSubstance(sub);
  }
  if (std::abs(1.0 - totalFraction) > 1e-6) //Allow for a little bit of numerical error
  {
    /// \error Fatal: Total ambient/environment gas volume fractions must sum to 1.0.
    std::stringstream ss;
    ss << "Total ambient/environment gas volume fractions must sum to 1.0. Current total fraction is " << totalFraction;
    Fatal(ss);
  }
  // Reset what we have
  for (SEGasSubstanceQuantity* subQ : m_AmbientGases->GetSubstanceQuantities())
    subQ->SetToZero();
  //Update the substance values on the Ambient Node based on the Action/File settings
  //We want to set an ambient volume fraction for all active gases
  for (SESubstanceFraction* subFrac : GetConditions().GetAmbientGases()) {
    SEGasSubstanceQuantity* subQ = m_AmbientGases->GetSubstanceQuantity(subFrac->GetSubstance());
    subQ->GetVolumeFraction().Set(subFrac->GetFractionAmount());
    //Set substance volumes to be infinite when compartment/node volume is also infinite
    subQ->GetVolume().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::L);
  }
  m_AmbientGases->GetPressure().Set(GetConditions().GetAtmosphericPressure());
  m_AmbientGases->Balance(BalanceGasBy::VolumeFraction);

  // Add aerosols to the environment
  for (auto s : GetConditions().GetAmbientAerosols()) {
    SESubstance& sub = s->GetSubstance();
    if (!sub.HasAerosolization()) {
      Error("Ignoring environment aerosol as it does not have any aerosol data : "s + sub.GetName());
      continue;
    }
    m_data.GetSubstances().AddActiveSubstance(sub);
    SELiquidSubstanceQuantity* subQ = m_AmbientAerosols->GetSubstanceQuantity(sub);
    subQ->GetConcentration().Set(s->GetConcentration());
  }
}

void Environment::AtSteadyState()
{
  if (m_data.GetState() == EngineState::AtInitialStableState) {
    if (m_data.GetConditions().HasInitialEnvironment())
      ProcessChange(*m_data.GetConditions().GetInitialEnvironment());
  }
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Preprocess prepares the cardiovascular system for the circuit solver
///
/// \details
/// This function uses feedback to calculate thermal properties and circuit element values for the
/// next engine state.
//--------------------------------------------------------------------------------------------------
void Environment::PreProcess()
{
  if (m_EnvironmentActions->HasChange()) {
    ProcessChange(*m_EnvironmentActions->GetChange());
    m_EnvironmentActions->RemoveChange();
  }

  //Calculate Clothing Effects
  //Set clothing resistor
  GetConditions().GetClothingResistance().SetValue(0.08, HeatResistanceAreaUnit::rsi);
  double dClothingResistance_rsi = GetConditions().GetClothingResistance(HeatResistanceAreaUnit::rsi); //1 rsi = 1 m^2-K/W, preset for LITE
  double dSurfaceArea_m2 = m_Patient->GetSkinSurfaceArea(AreaUnit::m2);
  double clothingResistAdd = (dClothingResistance_rsi / dSurfaceArea_m2);
  m_ClothingToEnvironmentPath->GetNextResistance().SetValue(clothingResistAdd, HeatResistanceUnit::K_Per_W);

  //Set the skin heat loss
  double dSkinHeatLoss_W = 0.0;
  if (m_ClothingToEnvironmentPath->HasHeatTransferRate()) {
    dSkinHeatLoss_W = m_ClothingToEnvironmentPath->GetHeatTransferRate().GetValue(PowerUnit::W);
  }
  GetSkinHeatLoss().SetValue(dSkinHeatLoss_W, PowerUnit::W);

  CalculateSupplementalValues();
  CalculateConvectionLite();
  CalculateEvaporationLite();
  CalculateRespirationLite();
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// There is nothing to do here.  Processing the combined Energy-Environment circuit is handled by
/// the Energy system.
//--------------------------------------------------------------------------------------------------
void Environment::Process()
{
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// There is nothing to do here.  Postprocessing the combined Energy-Environment circuit is handled by
/// the Energy system.
//--------------------------------------------------------------------------------------------------
void Environment::PostProcess()
{
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Calculate intermediate values to be used later.
///
/// \details
/// These computed values will be used in the other feedback methods.
//--------------------------------------------------------------------------------------------------
void Environment::CalculateSupplementalValues()
{
  //Use the Antoine Equation for to determine water vapor pressures
  //It's a piecewise function
  //Ambient Air
  double dAirTemperature_C = 0.0;
  if (!m_ThermalEnvironment->HasTemperature()) {
    dAirTemperature_C = GetConditions().GetAmbientTemperature(TemperatureUnit::C);
  } else {
    dAirTemperature_C = m_ThermalEnvironment->GetTemperature().GetValue(TemperatureUnit::C);
  }

  double dWaterVaporPressureInAmbientAir_mmHg = GeneralMath::AntoineEquation(dAirTemperature_C);
  m_dWaterVaporPressureInAmbientAir_Pa = Convert(dWaterVaporPressureInAmbientAir_mmHg, PressureUnit::mmHg, PressureUnit::Pa);
  //Skin
  double dSkinTemperature_C = m_SkinNode->GetTemperature().GetValue(TemperatureUnit::C);
  double dWaterVaporPressureAtSkin_mmHg = GeneralMath::AntoineEquation(dSkinTemperature_C);
  m_dWaterVaporPressureAtSkin_Pa = Convert(dWaterVaporPressureAtSkin_mmHg, PressureUnit::mmHg, PressureUnit::Pa);

  //Now use that to solve for the density of air
  double dUniversalGasConstant_JPerK_Mol = m_data.GetConfiguration().GetUniversalGasConstant(HeatCapacitancePerAmountUnit::J_Per_K_mol);
  double dMolarMassOfDryAir_KgPerMol = m_data.GetConfiguration().GetMolarMassOfDryAir(MassPerAmountUnit::kg_Per_mol);
  double dMolarMassOfWaterVapor_KgPerMol = m_data.GetConfiguration().GetMolarMassOfWaterVapor(MassPerAmountUnit::kg_Per_mol);

  double dAmbientTemperature_K = GetConditions().GetAmbientTemperature().GetValue(TemperatureUnit::K);

  double dPressureOfWaterVapor_Pa = GetConditions().GetRelativeHumidity().GetValue() * m_dWaterVaporPressureInAmbientAir_Pa;
  double dPartialPressureOfDryAir_Pa = GetConditions().GetAtmosphericPressure().GetValue(PressureUnit::Pa) - dPressureOfWaterVapor_Pa;

  double dAirDensity_kgPerm3 = (dPartialPressureOfDryAir_Pa * dMolarMassOfDryAir_KgPerMol + dPressureOfWaterVapor_Pa * dMolarMassOfWaterVapor_KgPerMol) / (dUniversalGasConstant_JPerK_Mol * dAmbientTemperature_K);
  GetConditions().GetAirDensity().SetValue(dAirDensity_kgPerm3, MassPerVolumeUnit::kg_Per_m3);

  //Now use that to determine the Lewis Relation
  double dAirSpecificHeat_JPerK_kg = m_data.GetConfiguration().GetAirSpecificHeat(HeatCapacitancePerMassUnit::J_Per_K_kg);
  m_dLewisRelation = 1 / (dAirDensity_kgPerm3 * dAirSpecificHeat_JPerK_kg);

  //Determine the heat vaporization of water
  //This equation was determined using a best fit from experimental data
  double dTemperature_K = Convert(dAirTemperature_C, TemperatureUnit::C, TemperatureUnit::K);
  double dHeatOfVaporizationOfWater_JPerMol = -0.1004 * std::pow(dTemperature_K, 2) + 22.173 * dTemperature_K + 46375.0;
  //Convert moles of water to kg
  m_dHeatOfVaporizationOfWater_J_Per_kg = dHeatOfVaporizationOfWater_JPerMol / 0.0180153; //1 mol of water = 0.0180153 kg

  //Water convective heat transfer properties
  if (GetConditions().GetSurroundingType() == CDM::enumSurroundingType::Water) {
    double dWaterTemperature_C = GetConditions().GetAmbientTemperature(TemperatureUnit::C);
    double dT = Convert(dWaterTemperature_C, TemperatureUnit::C, TemperatureUnit::K) / 298.15;

    m_WaterSpecificHeat_J_Per_kg_K = 0.001 * ((-1.0E-7) * std::pow(dWaterTemperature_C, 3.0) + (3.0E-5) * std::pow(dWaterTemperature_C, 2.0) - 0.0018 * dWaterTemperature_C + 4.2093);
    m_WaterViscosity_N_s_Per_m2 = 0.001 * ((-3.0E-6) * std::pow(dWaterTemperature_C, 3.0) + 0.0006 * std::pow(dWaterTemperature_C, 2.0) - 0.0462 * dWaterTemperature_C + 1.7412);
    m_WaterThermalConductivity_W_Per_m_K = 0.6065 * (-1.48446 + 4.12292 * dT + -1.63866 * std::pow(dT, 2));
    m_ThermalExpansion_Per_K = (6.0E-7) * std::pow(dWaterTemperature_C, 3.0) - 0.0001 * std::pow(dWaterTemperature_C, 2.0) + 0.016 * dWaterTemperature_C - 0.0632;
  }
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Calculate convection effects
///
/// \details
/// Uses forced and natural convection to determine heat effects on resistor from external to skin
//--------------------------------------------------------------------------------------------------
void Environment::CalculateConvectionLite()
{
  double dConvectiveHeatTransferCoefficient_WPerM2_K = 0.0;
  double dTotalHeatLoss_W = 0.0;
  //Calculate the coefficient
  //Velocity should take into account wind and patient movement combined
  double dAirVelocity_MPerS = GetConditions().GetAirVelocity(LengthPerTimeUnit::m_Per_s);

  // Natural Convection
  double dClothingTemperature_K = m_ClothingNode->GetTemperature().GetValue(TemperatureUnit::K);
  double dMeanRadiantTemperature_K = m_EnclosureNode->GetTemperature().GetValue(TemperatureUnit::K);
  dConvectiveHeatTransferCoefficient_WPerM2_K = 1.2 * std::pow(dClothingTemperature_K - dMeanRadiantTemperature_K, 0.347);

  //Set the coefficient
  GetConvectiveHeatTranferCoefficient().SetValue(dConvectiveHeatTransferCoefficient_WPerM2_K, HeatConductancePerAreaUnit::W_Per_m2_K);

  //Calculate the resistance
  double dSurfaceArea_m2 = m_Patient->GetSkinSurfaceArea(AreaUnit::m2);
  double dResistance_K_Per_W = 0.0;
  if (dConvectiveHeatTransferCoefficient_WPerM2_K == 0) {
    //Infinite resistance
    dResistance_K_Per_W = 0;
  } else {
    dResistance_K_Per_W = dSurfaceArea_m2 / dConvectiveHeatTransferCoefficient_WPerM2_K;
  }
  auto max_result = std::max(dResistance_K_Per_W, m_data.GetConfiguration().GetDefaultClosedHeatResistance(HeatResistanceUnit::K_Per_W));
  m_ClothingToEnvironmentPath->GetNextResistance().SetValue(max_result + (0.18 * m_ClothingToEnvironmentPath->GetNextResistance().GetValue(HeatResistanceUnit::K_Per_W)), HeatResistanceUnit::K_Per_W);

  //Set the source
  double dAmbientTemperature_K = GetConditions().GetAmbientTemperature(TemperatureUnit::K);
  m_GroundToEnvironmentPath->GetNextTemperatureSource().SetValue(dAmbientTemperature_K, TemperatureUnit::K);

  if (dAirVelocity_MPerS > ZERO_APPROX) {
    dConvectiveHeatTransferCoefficient_WPerM2_K = 10.3 * std::pow(dAirVelocity_MPerS, 0.6);

    //Set the coefficient
    GetConvectiveHeatTranferCoefficient().SetValue(dConvectiveHeatTransferCoefficient_WPerM2_K, HeatConductancePerAreaUnit::W_Per_m2_K);

    //Calculate the resistance
    dSurfaceArea_m2 = m_Patient->GetSkinSurfaceArea(AreaUnit::m2);
    double dResistance_K_Per_W = 0.0;
    if (dConvectiveHeatTransferCoefficient_WPerM2_K == 0) {
      //Infinite resistance
      dResistance_K_Per_W = 0;
    } else {
      dResistance_K_Per_W = dSurfaceArea_m2 / dConvectiveHeatTransferCoefficient_WPerM2_K;
    }
    auto max_result = std::max(dResistance_K_Per_W, m_data.GetConfiguration().GetDefaultClosedHeatResistance(HeatResistanceUnit::K_Per_W));
    double ndResistance_K_Per_W = (0.82 * max_result) + m_ClothingToEnvironmentPath->GetNextResistance().GetValue(HeatResistanceUnit::K_Per_W);
    m_ClothingToEnvironmentPath->GetNextResistance().SetValue(ndResistance_K_Per_W, HeatResistanceUnit::K_Per_W);

    //Set the source
    double dAmbientTemperature_K = GetConditions().GetAmbientTemperature(TemperatureUnit::K);
    m_GroundToEnvironmentPath->GetNextTemperatureSource().SetValue(dAmbientTemperature_K, TemperatureUnit::K);

    //Set the total heat lost
    if (m_ClothingToEnvironmentPath->HasHeatTransferRate()) {
      dTotalHeatLoss_W = m_ClothingToEnvironmentPath->GetHeatTransferRate().GetValue(PowerUnit::W);
    }
    GetConvectiveHeatLoss().SetValue(dTotalHeatLoss_W, PowerUnit::W);
    GetRadiativeHeatLoss().SetValue(dTotalHeatLoss_W, PowerUnit::W);
  }
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Calculate evaporation effects on capacitance.
///
/// \details
/// Uses sweat rate to alter skin capacitor and thermal effects on skin node
//--------------------------------------------------------------------------------------------------
void Environment::CalculateEvaporationLite()
{
  //Adjust for Evaporation
  //Calculate the coefficient
  double dConvectiveTransferCoefficient_WPerM2_K = GetConvectiveHeatTranferCoefficient(HeatConductancePerAreaUnit::W_Per_m2_K);
  double dEvaporativeHeatTransferCoefficient_WPerM2_K = m_dLewisRelation * dConvectiveTransferCoefficient_WPerM2_K;
  GetEvaporativeHeatTranferCoefficient().SetValue(dEvaporativeHeatTransferCoefficient_WPerM2_K, HeatConductancePerAreaUnit::W_Per_m2_K);
  double dTotalHeatLoss_W = 0.0;
  if (m_ClothingToEnvironmentPath->HasHeatTransferRate()) {
    double dTotalHeatLoss_W = m_ClothingToEnvironmentPath->GetHeatTransferRate().GetValue(PowerUnit::W);
  }

  //Calculate the source
  double dClothingResistance_rsi = GetConditions().GetClothingResistance(HeatResistanceAreaUnit::rsi); //1 rsi = 1 m^2-K/W
  double dFactorOfReduction = 1.0 / (1.0 + 2.22 * dConvectiveTransferCoefficient_WPerM2_K * dClothingResistance_rsi);
  double dMaxEvaporativePotential;
  if (m_dWaterVaporPressureAtSkin_Pa != m_dWaterVaporPressureInAmbientAir_Pa) {
    dMaxEvaporativePotential = dEvaporativeHeatTransferCoefficient_WPerM2_K * dFactorOfReduction * (m_dWaterVaporPressureAtSkin_Pa - m_dWaterVaporPressureInAmbientAir_Pa);
  } else {
    dMaxEvaporativePotential = dEvaporativeHeatTransferCoefficient_WPerM2_K * dFactorOfReduction * (0.01);
  }
  double dSurfaceArea_m2 = m_Patient->GetSkinSurfaceArea(AreaUnit::m2);
  double dSweatRate_kgPers = 0.0;
  if (m_data.GetEnergy().HasSweatRate()) {
    dSweatRate_kgPers = m_data.GetEnergy().GetSweatRate(MassPerTimeUnit::kg_Per_s) / dSurfaceArea_m2;
  }

  double dSweatingControlMechanisms = dSweatRate_kgPers * m_dHeatOfVaporizationOfWater_J_Per_kg;
  double dWettedPortion = 0.0;
  if (dMaxEvaporativePotential != 0) {
    dWettedPortion = dSweatingControlMechanisms / dMaxEvaporativePotential;
  }
  double dDiffusionOfWater = (1.0 - dWettedPortion) * 0.06 * dMaxEvaporativePotential;
  double EvaporativeHeatLossFromSkin_W = dSweatingControlMechanisms + dDiffusionOfWater;

  //Set the source
  double sweatHeatTransfer = dSurfaceArea_m2 * EvaporativeHeatLossFromSkin_W; //W
  double skinTemp_K = m_data.GetEnergy().GetSkinTemperature(TemperatureUnit::K);
  double coreTemp_C = m_data.GetEnergy().GetCoreTemperature(TemperatureUnit::C);
  double ambTemp_C = GetConditions().GetAmbientTemperature(TemperatureUnit::C);

  double currentSkinCapacitor = m_SkinToGroundPath->GetNextCapacitance().GetValue(HeatCapacitanceUnit::J_Per_K);
  double newCap = m_SkinToGroundPath->GetNextCapacitance().GetValue(HeatCapacitanceUnit::J_Per_K);
  double sweatCapacitorModifier = 0.0;
  double heatLeft = 0;
  if (coreTemp_C >= 37 && dSweatRate_kgPers > 0) {
    // Modification of capacitor based on percentage of sweat vs normal maximal sweating rate approximated based on hot/cold and gender variations \cite @Ergol1995Maximal
    sweatCapacitorModifier = (1 + (0.75 * m_data.GetEnergy().GetSweatRate(MassPerTimeUnit::kg_Per_s) / (0.00042)));
    if (sweatCapacitorModifier >= 1.025) {
      //If capacitor change gets too large, remove fraction of excess heat manually from node. Tuned to validate
      heatLeft = ((sweatCapacitorModifier - 1.025) / 1.025); // percent heat capacitance unaccounted for
      sweatCapacitorModifier = 1.025;
      double skinSweatHeatLost_K = 0.0000000572 * heatLeft * EvaporativeHeatLossFromSkin_W / (dSurfaceArea_m2 * dEvaporativeHeatTransferCoefficient_WPerM2_K); //Greatly scaled down (through tuning) to reduce effect directly on node
      m_data.GetCircuits().GetTemperatureCircuit().GetNode(BGE::ThermalLiteNode::Skin)->GetTemperature().SetReadOnly(false);
      m_data.GetCircuits().GetTemperatureCircuit().GetNode(BGE::ThermalLiteNode::Skin)->GetTemperature().SetValue(skinTemp_K - skinSweatHeatLost_K, TemperatureUnit::K);
      m_data.GetCircuits().GetTemperatureCircuit().GetNode(BGE::ThermalLiteNode::Skin)->GetTemperature().SetReadOnly(true);
    }
    newCap = sweatCapacitorModifier * (currentSkinCapacitor);
  } else {
    newCap = (m_SkinToGroundPath->GetCapacitanceBaseline(HeatCapacitanceUnit::J_Per_K) + currentSkinCapacitor) / 2;
  }

  m_SkinToGroundPath->GetNextCapacitance().SetValue(newCap, HeatCapacitanceUnit::J_Per_K);

  //Set the total heat lost
  double dTotalHeatLossE_W = 0.0;
  if (m_ClothingToEnvironmentPath->HasHeatTransferRate()) {
    dTotalHeatLossE_W = m_ClothingToEnvironmentPath->GetHeatTransferRate().GetValue(PowerUnit::W);
  }
  dTotalHeatLossE_W = dTotalHeatLossE_W - dTotalHeatLoss_W;
  if (dTotalHeatLossE_W < 0) {
    dTotalHeatLossE_W = 0;
  }
  GetEvaporativeHeatLoss().SetValue(dTotalHeatLossE_W, PowerUnit::W);
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Calculate effects of respiration.
///
/// \details
/// This determines the circuit element values and system data outputs associated with respiration
/// heat transfer based on feedback.
//--------------------------------------------------------------------------------------------------
void Environment::CalculateRespirationLite()
{
  double dTempOfRespAir_K = GetConditions().GetRespirationAmbientTemperature(TemperatureUnit::K);
  double dTempOfRespTract_K = 310.15; // = 37C = 98.6F
  if (m_data.GetEnergy().HasCoreTemperature()) {
    dTempOfRespTract_K = m_data.GetEnergy().GetCoreTemperature(TemperatureUnit::K);
  }
  double dPulmonaryVentilationRate_M3PerS = m_data.GetRespiratory().GetTotalPulmonaryVentilation(VolumePerTimeUnit::m3_Per_s);
  double dAirDensity_kgPerM3 = GetConditions().GetAirDensity(MassPerVolumeUnit::kg_Per_m3);
  double dAirSpecificHeat_JPerK_kg = m_data.GetConfiguration().GetAirSpecificHeat(HeatCapacitancePerMassUnit::J_Per_K_kg);
  double dSensibleHeatLoss_W = dPulmonaryVentilationRate_M3PerS * dAirDensity_kgPerM3 * dAirSpecificHeat_JPerK_kg * (dTempOfRespTract_K - dTempOfRespAir_K);

  //Evaporation
  double dTempOfRespAir_F = GetConditions().GetRespirationAmbientTemperature(TemperatureUnit::F);
  double dRelativeHumidity = GetConditions().GetRelativeHumidity().GetValue();
  double dPressure_Pa = GetConditions().GetAtmosphericPressure(PressureUnit::Pa);
  double dSpecificHumidity = (dRelativeHumidity * 100.0) / (0.263 * dPressure_Pa) * (std::exp(17.67 * (dTempOfRespAir_K - 273.16) / (dTempOfRespAir_K - 29.65)));
  double dHumidityDiff = 0.02645 + 0.0000361 * dTempOfRespAir_F - 0.798 * dSpecificHumidity;

  double dLatentHeatLoss_W = m_dHeatOfVaporizationOfWater_J_Per_kg * dPulmonaryVentilationRate_M3PerS * dAirDensity_kgPerM3 * dHumidityDiff;

  //Total
  double dTotalHeatLossResp_W = (dSensibleHeatLoss_W + dLatentHeatLoss_W);

  //Set the source
  m_EnvironmentCoreToGroundPath->GetNextHeatSource().SetValue(dTotalHeatLossResp_W, PowerUnit::W);

  //Set the total heat lost
  GetRespirationHeatLoss().SetValue(dTotalHeatLossResp_W, PowerUnit::W);
}
}