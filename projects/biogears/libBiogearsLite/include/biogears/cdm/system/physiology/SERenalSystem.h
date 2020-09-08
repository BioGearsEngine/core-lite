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
#include <biogears/cdm/system/SESystem.h>
#include <biogears/schema/biogears/BioGearsPhysiology.hxx>
#include "biogears/cdm/properties/SEScalarVolumePerTimePressure.h"

namespace biogears {
class SEScalarPressure;
class PressureUnit;
class SEScalarFraction;
class SEScalarVolumePerTime;
class VolumePerTimeUnit;
class SEScalarFlowCompliance;
class FlowComplianceUnit;
class SEScalarFlowResistance;
class FlowResistanceUnit;
class SEScalarFrequency;
class FrequencyUnit;
class SEScalarVolume;
class VolumeUnit;
class SEScalarVolumePerTimePressure;
class VolumePerTimePressureUnit;
class SEScalarArea;
class AreaUnit;
class SEScalarVolumePerTimePressureArea;
class VolumePerTimePressureAreaUnit;
class OsmolarityUnit;
class OsmolalityUnit;
class MassPerVolumeUnit;

class BIOGEARS_API SERenalSystem : public SESystem {
public:
  SERenalSystem(Logger* logger);
  ~SERenalSystem() override;

  static size_t TypeHash() { return reinterpret_cast<size_t>(&TypeHash); }  //! Hopefully this returns a unique ID for every type
  static constexpr char const * const  TypeTag() { return "SERenalSystem"; }
  const char* classname() const override { return TypeTag(); }
  size_t hash_code() const override { return TypeHash(); }

  void Clear() override; // Deletes all members

  const SEScalar* GetScalar(const char* name) override;
  const SEScalar* GetScalar(const std::string& name) override;

  bool Load(const CDM::RenalSystemData& in);
  CDM::RenalSystemData* Unload() const override;

  Tree<const char*> GetPhysiologyRequestGraph() const override;
protected:
  void Unload(CDM::RenalSystemData& data) const;

public:
  bool HasGlomerularFiltrationRate() const;
  SEScalarVolumePerTime& GetGlomerularFiltrationRate();
  double GetGlomerularFiltrationRate(const VolumePerTimeUnit& unit) const;

  bool HasFiltrationFraction() const;
  SEScalarFraction& GetFiltrationFraction();
  double GetFiltrationFraction() const;

  bool HasAfferentArterioleResistance() const;
  SEScalarFlowResistance& GetAfferentArterioleResistance();
  double GetAfferentArterioleResistance(const FlowResistanceUnit& unit) const;

  bool HasBowmansCapsulesHydrostaticPressure() const;
  SEScalarPressure& GetBowmansCapsulesHydrostaticPressure();
  double GetBowmansCapsulesHydrostaticPressure(const PressureUnit& unit) const;

  bool HasBowmansCapsulesOsmoticPressure() const;
  SEScalarPressure& GetBowmansCapsulesOsmoticPressure();
  double GetBowmansCapsulesOsmoticPressure(const PressureUnit& unit) const;

  bool HasEfferentArterioleResistance() const;
  SEScalarFlowResistance& GetEfferentArterioleResistance();
  double GetEfferentArterioleResistance(const FlowResistanceUnit& unit) const;

  bool HasGlomerularCapillariesHydrostaticPressure() const;
  SEScalarPressure& GetGlomerularCapillariesHydrostaticPressure();
  double GetGlomerularCapillariesHydrostaticPressure(const PressureUnit& unit) const;

  bool HasGlomerularCapillariesOsmoticPressure() const;
  SEScalarPressure& GetGlomerularCapillariesOsmoticPressure();
  double GetGlomerularCapillariesOsmoticPressure(const PressureUnit& unit) const;

  bool HasGlomerularFiltrationCoefficient() const;
  SEScalarVolumePerTimePressure& GetGlomerularFiltrationCoefficient();
  double GetGlomerularFiltrationCoefficient(const VolumePerTimePressureUnit& unit) const;


  bool HasGlomerularFiltrationSurfaceArea() const;
  SEScalarArea& GetGlomerularFiltrationSurfaceArea();
  double GetGlomerularFiltrationSurfaceArea(const AreaUnit& unit) const;

  bool HasGlomerularFluidPermeability() const;
  SEScalarVolumePerTimePressureArea& GetGlomerularFluidPermeability();
  double GetGlomerularFluidPermeability(const VolumePerTimePressureAreaUnit& unit) const;


  bool HasNetFiltrationPressure() const;
  SEScalarPressure& GetNetFiltrationPressure();
  double GetNetFiltrationPressure(const PressureUnit& unit) const;

  bool HasNetReabsorptionPressure() const;
  SEScalarPressure& GetNetReabsorptionPressure();
  double GetNetReabsorptionPressure(const PressureUnit& unit) const;

  bool HasPeritubularCapillariesHydrostaticPressure() const;
  SEScalarPressure& GetPeritubularCapillariesHydrostaticPressure();
  double GetPeritubularCapillariesHydrostaticPressure(const PressureUnit& unit) const;

  bool HasPeritubularCapillariesOsmoticPressure() const;
  SEScalarPressure& GetPeritubularCapillariesOsmoticPressure();
  double GetPeritubularCapillariesOsmoticPressure(const PressureUnit& unit) const;

  bool HasReabsorptionFiltrationCoefficient() const;
  SEScalarVolumePerTimePressure& GetReabsorptionFiltrationCoefficient();
  double GetReabsorptionFiltrationCoefficient(const VolumePerTimePressureUnit& unit) const;

  bool HasReabsorptionRate() const;
  SEScalarVolumePerTime& GetReabsorptionRate();
  double GetReabsorptionRate(const VolumePerTimeUnit& unit) const;

  bool HasTubularReabsorptionFiltrationSurfaceArea() const;
  SEScalarArea& GetTubularReabsorptionFiltrationSurfaceArea();
  double GetTubularReabsorptionFiltrationSurfaceArea(const AreaUnit& unit) const;

  bool HasTubularReabsorptionFluidPermeability() const;
  SEScalarVolumePerTimePressureArea& GetTubularReabsorptionFluidPermeability();
  double GetTubularReabsorptionFluidPermeability(const VolumePerTimePressureAreaUnit& unit) const;

  bool HasTubularHydrostaticPressure() const;
  SEScalarPressure& GetTubularHydrostaticPressure();
  double GetTubularHydrostaticPressure(const PressureUnit& unit) const;

  bool HasTubularOsmoticPressure() const;
  SEScalarPressure& GetTubularOsmoticPressure();
  double GetTubularOsmoticPressure(const PressureUnit& unit) const;

  bool HasRenalBloodFlow() const;
  SEScalarVolumePerTime& GetRenalBloodFlow();
  double GetRenalBloodFlow(const VolumePerTimeUnit& unit) const;

  bool HasRenalPlasmaFlow() const;
  SEScalarVolumePerTime& GetRenalPlasmaFlow();
  double GetRenalPlasmaFlow(const VolumePerTimeUnit& unit) const;

  bool HasRenalVascularResistance() const;
  SEScalarFlowResistance& GetRenalVascularResistance();
  double GetRenalVascularResistance(const FlowResistanceUnit& unit) const;

  bool HasUrinationRate() const;
  SEScalarVolumePerTime& GetUrinationRate();
  double GetUrinationRate(const VolumePerTimeUnit& unit) const;

  bool HasUrineOsmolality() const;
  SEScalarOsmolality& GetUrineOsmolality();
  double GetUrineOsmolality(const OsmolalityUnit& unit) const;

  bool HasUrineOsmolarity() const;
  SEScalarOsmolarity& GetUrineOsmolarity();
  double GetUrineOsmolarity(const OsmolarityUnit& unit) const;

  bool HasUrineProductionRate() const;
  SEScalarVolumePerTime& GetUrineProductionRate();
  double GetUrineProductionRate(const VolumePerTimeUnit& unit) const;

  bool HasUrineSpecificGravity() const;
  SEScalar& GetUrineSpecificGravity();
  double GetUrineSpecificGravity() const;

  bool HasUrineVolume() const;
  SEScalarVolume& GetUrineVolume();
  double GetUrineVolume(const VolumeUnit& unit) const;

  bool HasUrineUreaNitrogenConcentration() const;
  SEScalarMassPerVolume& GetUrineUreaNitrogenConcentration();
  double GetUrineUreaNitrogenConcentration(const MassPerVolumeUnit& unit) const;

protected:
  SEScalarVolumePerTime* m_GlomerularFiltrationRate;
  SEScalarFraction* m_FiltrationFraction;

  SEScalarFlowResistance* m_AfferentArterioleResistance;
  SEScalarPressure* m_BowmansCapsulesHydrostaticPressure;
  SEScalarPressure* m_BowmansCapsulesOsmoticPressure;
  SEScalarFlowResistance* m_EfferentArterioleResistance;
  SEScalarPressure* m_GlomerularCapillariesHydrostaticPressure;
  SEScalarPressure* m_GlomerularCapillariesOsmoticPressure;
  SEScalarVolumePerTimePressure* m_GlomerularFiltrationCoefficient;
  SEScalarArea* m_GlomerularFiltrationSurfaceArea;
  SEScalarVolumePerTimePressureArea* m_GlomerularFluidPermeability;
  SEScalarPressure* m_NetFiltrationPressure;
  SEScalarPressure* m_NetReabsorptionPressure;
  SEScalarPressure* m_PeritubularCapillariesHydrostaticPressure;
  SEScalarPressure* m_PeritubularCapillariesOsmoticPressure;
  SEScalarVolumePerTimePressure* m_ReabsorptionFiltrationCoefficient;
  SEScalarVolumePerTime* m_ReabsorptionRate;
  SEScalarArea* m_TubularReabsorptionFiltrationSurfaceArea;
  SEScalarVolumePerTimePressureArea* m_TubularReabsorptionFluidPermeability;
  SEScalarPressure* m_TubularHydrostaticPressure;
  SEScalarPressure* m_TubularOsmoticPressure;

  SEScalarVolumePerTime* m_RenalBloodFlow;
  SEScalarVolumePerTime* m_RenalPlasmaFlow;
  SEScalarFlowResistance* m_RenalVascularResistance;

  SEScalarVolumePerTime* m_UrinationRate;
  SEScalarOsmolality* m_UrineOsmolality;
  SEScalarOsmolarity* m_UrineOsmolarity;
  SEScalarVolumePerTime* m_UrineProductionRate;
  SEScalar* m_UrineSpecificGravity;
  SEScalarVolume* m_UrineVolume;
  SEScalarMassPerVolume* m_UrineUreaNitrogenConcentration;
};
}