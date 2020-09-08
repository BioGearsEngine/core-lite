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

#include <biogears/cdm/properties/SEScalarMass.h>
#include <biogears/cdm/system/physiology/SERenalSystem.h>
#include <biogears/cdm/utils/RunningAverage.h>
#include <biogears/engine/Controller/BioGearsSystem.h>
#include <biogears/schema/biogears/BioGearsPhysiology.hxx>

namespace biogears {
class SEUrinalysis;
class SEPatient;
class SEFluidCircuit;
class SEFluidCircuitNode;
class SEFluidCircuitPath;
class SESubstance;
class SELiquidCompartment;
class SELiquidSubstanceQuantity;
class BioGears;
class SETissueCompartment;
/**
 * @brief @copydoc Physiology_RenalSystemData
 */
class BIOGEARS_API Renal : public SERenalSystem, public BioGearsSystem {
  friend class BioGears;
  friend class BioGearsEngineTest;

protected:
  Renal(BioGears& bg);
  BioGears& m_data;
  double m_dt;

public:
  virtual ~Renal() override;

  static size_t TypeHash() { return reinterpret_cast<size_t>(&TypeHash); } //! Hopefully this returns a unique ID for every type
  static constexpr char const* const TypeTag() { return "Renal"; }
  const char* classname() const override { return TypeTag(); }
  size_t hash_code() const override { return TypeHash(); }

  void Clear() override;

  // Set members to a stable homeostatic state
  void Initialize() override;

  // Load a state
  virtual bool Load(const CDM::BioGearsRenalSystemData& in);
  virtual CDM::BioGearsRenalSystemData* Unload() const override;

protected:
  virtual void Unload(CDM::BioGearsRenalSystemData& data) const;

  // Set pointers and other member varialbes common to both homeostatic initialization and loading a state
  void SetUp() override;

public:
  void AtSteadyState() override;
  void PreProcess() override;
  void Process() override;
  void PostProcess() override;

  // Assessments
  bool CalculateUrinalysis(SEUrinalysis& u);

protected:
  struct ActiveTransport {
  public:
    double GlucoseReabsorptionMass_mg;
    double LactateExcretedMass_mg;
  };

  // Initialization
  void CalculateFilterability(SESubstance& sub);

  //Preprocess
  void CalculateUltrafiltrationFeedback();
  void CalculateColloidOsmoticPressure(SEScalarMassPerVolume& albuminConcentration, SEScalarPressure& osmoticPressure);
  void CalculateReabsorptionFeedback();
  void CalculateOsmoreceptorFeedback();
  void CalculateFluidPermeability();
  void CalculateTubuloglomerularFeedback();
  void UpdateBladderVolume();
  void ProcessActions();
  void Urinate();

  //Process
  void CalculateActiveTransport();
  void CalculateGlomerularTransport(SESubstance& sub);
  void CalculateReabsorptionTransport(SESubstance& sub);
  void CalculateSecretion();
  void CalculateExcretion(SESubstance& sub);
  void CalculateAutomaticClearance(SESubstance& sub);
  void CalculateGluconeogenesis();
  void CalculateVitalSigns();

  //Override
  void ProcessOverride();
  void OverrideControlLoop();

  // Serializable member variables (Set in Initialize and in schema)
  bool m_Urinating;
  //Tubuloglomerular Feedback
  double m_AfferentResistance_mmHg_s_Per_mL;
  double m_SodiumFlowSetPoint_mg_Per_s;

  //Events
  RunningAverage m_urineProductionRate_mL_Per_min_runningAvg;
  RunningAverage m_urineOsmolarity_mOsm_Per_L_runningAvg;
  RunningAverage m_sodiumConcentration_mg_Per_mL_runningAvg;
  RunningAverage m_sodiumExcretionRate_mg_Per_min_runningAvg;
  RunningAverage m_SodiumFlow_mg_Per_s_runningAvg;
  RunningAverage m_RenalArterialPressure_mmHg_runningAvg;

  // Stateless member variable (Set in SetUp())
  SEPatient* m_patient;
  //Circuits
  SEFluidCircuit* m_RenalCircuit;
  //Nodes
  SEFluidCircuitNode* m_GlomerularNode;
  SEFluidCircuitNode* m_BowmansNode;
  SEFluidCircuitNode* m_PeritubularNode;
  SEFluidCircuitNode* m_TubulesNode;
  SEFluidCircuitNode* m_RenalArteryNode;
  SEFluidCircuitNode* m_bladderNode;
  SEFluidCircuitNode* m_NetGlomerularCapillariesNode;
  SEFluidCircuitNode* m_NetBowmansCapsulesNode;
  SEFluidCircuitNode* m_NetPeritubularCapillariesNode;
  SEFluidCircuitNode* m_NetTubulesNode;

  //Paths
  SEFluidCircuitPath* m_GlomerularOsmoticSourcePath;
  SEFluidCircuitPath* m_BowmansOsmoticSourcePath;
  SEFluidCircuitPath* m_ReabsorptionResistancePath;
  SEFluidCircuitPath* m_TubulesOsmoticSourcePath;
  SEFluidCircuitPath* m_PeritubularOsmoticSourcePath;
  SEFluidCircuitPath* m_UreterPath;
  SEFluidCircuitPath* m_GlomerularFilterResistancePath;
  SEFluidCircuitPath* m_AfferentArteriolePath;
  SEFluidCircuitPath* m_bladderToGroundPressurePath;
  SEFluidCircuitPath* m_urethraPath;
  SEFluidCircuitPath* m_TubulesPath;
  SEFluidCircuitPath* m_EfferentArteriolePath;

  //Substances
  SESubstance* m_sodium;
  SESubstance* m_urea;
  SESubstance* m_glucose;
  SESubstance* m_lactate;
  SESubstance* m_potassium;
  //Compartments
  SELiquidCompartment* m_kidney;
  SELiquidCompartment* m_aorta;
  SELiquidCompartment* m_venaCava;
  SELiquidCompartment* m_bladder;
  SETissueCompartment* m_KidneyTissue;
  SELiquidCompartment* m_Ureter;
  SELiquidCompartment* m_Peritubular;
  SELiquidCompartment* m_Glomerular;
  SELiquidCompartment* m_Bowmans;
  SELiquidCompartment* m_Tubules;
  SELiquidCompartment* m_ground;
  // Compartment Substance Quantites
  SELiquidSubstanceQuantity* m_aortaLactate;

  SELiquidSubstanceQuantity* m_PeritubularGlucose;
  SELiquidSubstanceQuantity* m_PeritubularPotassium;
  SELiquidSubstanceQuantity* m_UreterPotassium;
  SELiquidSubstanceQuantity* m_UreterLactate;

  SELiquidSubstanceQuantity* m_bladderGlucose;
  SELiquidSubstanceQuantity* m_bladderPotassium;
  SELiquidSubstanceQuantity* m_bladderSodium;
  SELiquidSubstanceQuantity* m_bladderUrea;
  SELiquidSubstanceQuantity* m_TubulesSodium;

  SELiquidSubstanceQuantity* m_KidneyIntracellularLactate;

  // Configuration
  double m_defaultOpenResistance_mmHg_s_Per_mL;
  double m_defaultClosedResistance_mmHg_s_Per_mL;
  //   Ultrafiltration
  double m_CVOpenResistance_mmHg_s_Per_mL;
  //   Tubuloglomerular Feedback
  double m_maxAfferentResistance_mmHg_s_Per_mL;
  double m_minAfferentResistance_mmHg_s_Per_mL;
  //   Osmoreceptor Feedback
  double m_ReabsorptionPermeabilityModificationFactor;
  double m_sodiumPlasmaConcentrationSetpoint_mg_Per_mL;
  //   Secretion
  double m_baselinePotassiumConcentration_g_Per_dL;
  //   Reabsorption
  double m_ReabsorptionPermeabilitySetpoint_mL_Per_s_mmHg_m2;

  // Utility/ScratchPads
  SEScalarMass m_spCleared;
  ActiveTransport m_SubstanceTransport;
};
}
