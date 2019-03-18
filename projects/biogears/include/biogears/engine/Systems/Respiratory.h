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
#include <biogears/cdm/CommonDataModel.h>
#include <biogears/exports.h>

#include <biogears/cdm/circuit/fluid/SEFluidCircuitCalculator.h>
#include <biogears/cdm/patient/SEPatient.h>
#include <biogears/cdm/substance/SESubstanceTransport.h>
#include <biogears/cdm/system/physiology/SERespiratorySystem.h>
#include <biogears/cdm/utils/RunningAverage.h>
#include <biogears/engine/Controller/BioGearsSystem.h>
#include <biogears/schema/biogears/BioGears.hxx>

namespace biogears {
class SEPulmonaryFunctionTest;
class SEPatient;
class SEPatientActionCollection;
class SEGasCompartment;
class SELiquidCompartment;
class SEGasSubstanceQuantity;
class SELiquidSubstanceQuantity;
class SEGasSubstanceQuantity;
class SELiquidCompartment;
class SEGasCompartment;
class SEFluidCircuit;
class SEFluidCircuitNode;
class SEConsciousRespirationCommand;
class BioGears;

/**
* @brief The %Respiratory System class handles the analysis and storage of
* data related the respiratory physiology and pathology.
* @details The goal of the Respiratory System class is to handle the analysis
* and storage of data representing the ventilation and gas exchange of the human
* respiratory system. The class handles the analysis of changes in the lungs volume
* and pressure data during the flow and transfer of gases between the lungs, the
* blood and the outside environment. The class updates the volume and volume fractions
* of gases in the lungs, and ensures the integration and flow of data between the
* respiratory system and the anesthesia machine during mechanical ventilation.
*/
class BIOGEARS_API Respiratory : public SERespiratorySystem, public BioGearsSystem {
  friend class BioGears;
  friend class BioGearsEngineTest;

protected:
  Respiratory(BioGears& bg);
  BioGears& m_data;

public:
  virtual ~Respiratory() override;

  static size_t TypeHash() { return reinterpret_cast<size_t>(&TypeHash); }  //! Hopefully this returns a unique ID for every type
  static constexpr char const * const  TypeTag() { return "Respiratory"; }
  const char* classname() const override { return TypeTag(); }
  size_t hash_code() const override { return TypeHash(); }

  void Clear() override;

  // Set members to a stable homeostatic state
  void Initialize() override;

  // Load a state
  virtual bool Load(const CDM::BioGearsRespiratorySystemData& in);
  virtual CDM::BioGearsRespiratorySystemData* Unload() const override;

protected:
  virtual void Unload(CDM::BioGearsRespiratorySystemData& data) const;

  // Set pointers and other member varialbes common to both homeostatic initialization and loading a state
  void SetUp() override;

public:
  void AtSteadyState() override;
  void PreProcess() override;
  void Process() override;
  void PostProcess() override;

  bool CalculatePulmonaryFunctionTest(SEPulmonaryFunctionTest& pft);

private:
  //Tuning
  void TuneCircuit();

  //PreProcess
  //Actions
  void AirwayObstruction();
  void BronchoConstriction();
  void BronchoDilation();
  void Pneumothorax();
  void ConsciousRespiration();
  /**/ void ProcessConsciousRespiration(SEConsciousRespirationCommand& cmd);
  void MechanicalVentilation();
  void Apnea();
  // Driver
  void RespiratoryDriver();
  void RespiratoryDriverLite();
  void ProcessDriverActions();

  // Shared Utility Methods for Actions/Driver
  /**/ void UpdateIERatio();
  // Aerosol Deposition and various Effects
  void ProcessAerosolSubstances();

  //Process
  void CalculateVitalSignsLite();

protected:
  // Override
  void ProcessOverride();
  void OverrideControlLoop();

private:
  std::string m_TuningFile; 
  // Serializable member variables (Set in Initialize and in schema)
  //   Initial patient values
  double m_InitialExpiratoryReserveVolume_L;
  double m_InitialFunctionalResidualCapacity_L;
  double m_InitialInspiratoryCapacity_L;
  double m_InitialResidualVolume_L;
  //   CalculateVitalSigns()
  bool m_bNotBreathing;
  double m_TopBreathTotalVolume_L;
  double m_TopBreathAlveoliVolume_L;
  double m_TopBreathDeadSpaceVolume_L;
  double m_TopBreathPleuralPressure_cmH2O;
  double m_LastCardiacCycleBloodPH;
  double m_PreviousTotalLungVolume_L;
  RunningAverage m_BloodPHRunningAverage;
  //   Respiratory Driver
  bool m_BreathingCycle;
  double m_ArterialO2PartialPressure_mmHg;
  double m_ArterialCO2PartialPressure_mmHg;
  RunningAverage m_ArterialO2Average_mmHg;
  RunningAverage m_ArterialCO2Average_mmHg;
  double m_BreathingCycleTime_s;
  double m_BreathTimeExhale_min;
  double m_DefaultDrivePressure_cmH2O;
  double m_DriverPressure_cmH2O;
  double m_DriverPressureMin_cmH2O;
  double m_ElapsedBreathingCycleTime_min;
  double m_IEscaleFactor;
  double m_InstantaneousFunctionalResidualCapacity_L;
  double m_MaxDriverPressure_cmH2O;
  double m_PeakRespiratoryDrivePressure_cmH2O;
  double m_TargetTidalVolume_L;
  double m_VentilationFrequency_Per_min;

  //   Conscious Breathing
  bool m_ConsciousBreathing;
  double m_ConsciousRespirationPeriod_s;
  double m_ConsciousRespirationRemainingPeriod_s;
  double m_ExpiratoryReserveVolumeFraction;
  double m_InspiratoryCapacityFraction;
  double m_ConsciousStartPressure_cmH2O;
  double m_ConsciousEndPressure_cmH2O;

  // Stateless member variable (Set in SetUp())
  double m_dt_s;
  double m_dt_min;
  bool m_hadApnea; 
  // Configuration parameters
  double m_dDefaultOpenResistance_cmH2O_s_Per_L;
  double m_dDefaultClosedResistance_cmH2O_s_Per_L;
  double m_dRespOpenResistance_cmH2O_s_Per_L;
  double m_dRespClosedResistance_cmH2O_s_Per_L;
  double m_VentilatoryOcclusionPressure_cmH2O;
  // State between functions (i.e. shared between methods in preprocess, set to a default value at the start of preprocess)
  double m_AverageLocalTissueBronchodilationEffects;
  // member to save off baseline PH in the patient 
  double m_arterialPHBaseline;

  // Patient
  SEPatient* m_Patient;
  SEPatientActionCollection* m_PatientActions;
  //Compartments
  SEGasCompartment* m_Environment;
  std::vector<SELiquidCompartment*> m_AerosolEffects;
  SEGasCompartment* m_MechanicalVentilatorConnection;
  //Circuits
  SEFluidCircuit* m_RespiratoryCircuit;
  //Nodes
  SEFluidCircuitNode* m_Ambient;
  //Paths
  SEFluidCircuitPath* m_DriverPressurePath;
  SEFluidCircuitPath* m_RightPulmonaryCapillary;
  SEFluidCircuitPath* m_LeftPulmonaryCapillary;
  SEFluidCircuitPath* m_ConnectionToMouth;
  SEFluidCircuitPath* m_GroundToConnection;
  //Substances
  SELiquidSubstanceQuantity* m_AortaO2;
  SELiquidSubstanceQuantity* m_AortaCO2;

  SEFluidCircuitCalculator m_Calculator;
  SEGasTransporter m_GasTransporter;
  SELiquidTransporter m_AerosolTransporter;
};
}
