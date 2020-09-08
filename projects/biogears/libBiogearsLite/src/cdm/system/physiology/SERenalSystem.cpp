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
#include <biogears/cdm/system/physiology/SERenalSystem.h>

#include <biogears/cdm/properties/SEScalarAmountPerVolume.h>
#include <biogears/cdm/properties/SEScalarArea.h>
#include <biogears/cdm/properties/SEScalarFlowResistance.h>
#include <biogears/cdm/properties/SEScalarFraction.h>

#include <biogears/cdm/properties/SEScalarMassPerVolume.h>
#include <biogears/cdm/properties/SEScalarOsmolality.h>
#include <biogears/cdm/properties/SEScalarOsmolarity.h>
#include <biogears/cdm/properties/SEScalarPressure.h>
#include <biogears/cdm/properties/SEScalarVolume.h>
#include <biogears/cdm/properties/SEScalarVolumePerTime.h>

#include <biogears/cdm/properties/SEScalarVolumePerTimePressure.h>
#include <biogears/cdm/properties/SEScalarVolumePerTimePressureArea.h>
#include <biogears/cdm/substance/SESubstanceManager.h>
#include <biogears/container/Tree.tci.h>

namespace biogears {
  constexpr char idGlomerularFiltrationRate[] = "GlomerularFiltrationRate";
  constexpr char idFiltrationFraction[] = "FiltrationFraction";
  constexpr char idAfferentArterioleResistance[] = "AfferentArterioleResistance";
  constexpr char idBowmansCapsulesHydrostaticPressure[] = "BowmansCapsulesHydrostaticPressure";
  constexpr char idBowmansCapsulesOsmoticPressure[] = "BowmansCapsulesOsmoticPressure";
  constexpr char idEfferentArterioleResistance[] = "EfferentArterioleResistance";
  constexpr char idGlomerularCapillariesHydrostaticPressure[] = "GlomerularCapillariesHydrostaticPressure";
  constexpr char idGlomerularCapillariesOsmoticPressure[] = "GlomerularCapillariesOsmoticPressure";
  constexpr char idGlomerularFiltrationCoefficient[] = "GlomerularFiltrationCoefficient";
  constexpr char idGlomerularFiltrationSurfaceArea[] = "GlomerularFiltrationSurfaceArea";
  constexpr char idGlomerularFluidPermeability[] = "GlomerularFluidPermeability";
  constexpr char idNetFiltrationPressure[] = "NetFiltrationPressure";
  constexpr char idNetReabsorptionPressure[] = "NetReabsorptionPressure";
  constexpr char idPeritubularCapillariesHydrostaticPressure[] = "PeritubularCapillariesHydrostaticPressure";
  constexpr char idPeritubularCapillariesOsmoticPressure[] = "PeritubularCapillariesOsmoticPressure";
  constexpr char idReabsorptionFiltrationCoefficient[] = "ReabsorptionFiltrationCoefficient";
  constexpr char idReabsorptionRate[] = "ReabsorptionRate";
  constexpr char idTubularReabsorptionFiltrationSurfaceArea[] = "TubularReabsorptionFiltrationSurfaceArea";
  constexpr char idTubularReabsorptionFluidPermeability[] = "TubularReabsorptionFluidPermeability";
  constexpr char idTubularHydrostaticPressure[] = "TubularHydrostaticPressure";
  constexpr char idTubularOsmoticPressure[] = "TubularOsmoticPressure";
  constexpr char idRenalBloodFlow[] = "RenalBloodFlow";
  constexpr char idRenalPlasmaFlow[] = "RenalPlasmaFlow";
  constexpr char idRenalVascularResistance[] = "RenalVascularResistance";
  constexpr char idUrinationRate[] = "UrinationRate";
  constexpr char idUrineOsmolality[] = "UrineOsmolality";
  constexpr char idUrineOsmolarity[] = "UrineOsmolarity";
  constexpr char idUrineProductionRate[] = "UrineProductionRate";
  constexpr char idUrineSpecificGravity[] = "UrineSpecificGravity";
  constexpr char idUrineVolume[] = "UrineVolume";
  constexpr char idUrineUreaNitrogenConcentration[] = "UrineUreaNitrogenConcentration";

SERenalSystem::SERenalSystem(Logger* logger)
  : SESystem(logger)
{
  m_GlomerularFiltrationRate = nullptr;
  m_FiltrationFraction = nullptr;

  m_AfferentArterioleResistance = nullptr;
  m_BowmansCapsulesHydrostaticPressure = nullptr;
  m_BowmansCapsulesOsmoticPressure = nullptr;
  m_EfferentArterioleResistance = nullptr;
  m_GlomerularCapillariesHydrostaticPressure = nullptr;
  m_GlomerularCapillariesOsmoticPressure = nullptr;
  m_GlomerularFiltrationCoefficient = nullptr;
  m_GlomerularFiltrationSurfaceArea = nullptr;
  m_GlomerularFluidPermeability = nullptr;
  m_NetFiltrationPressure = nullptr;
  m_NetReabsorptionPressure = nullptr;
  m_PeritubularCapillariesHydrostaticPressure = nullptr;
  m_PeritubularCapillariesOsmoticPressure = nullptr;
  m_ReabsorptionFiltrationCoefficient = nullptr;
  m_ReabsorptionRate = nullptr;
  m_TubularReabsorptionFiltrationSurfaceArea = nullptr;
  m_TubularReabsorptionFluidPermeability = nullptr;
  m_TubularHydrostaticPressure = nullptr;
  m_TubularOsmoticPressure = nullptr;

  m_RenalBloodFlow = nullptr;
  m_RenalPlasmaFlow = nullptr;
  m_RenalVascularResistance = nullptr;

  m_UrinationRate = nullptr;
  m_UrineOsmolality = nullptr;
  m_UrineOsmolarity = nullptr;
  m_UrineProductionRate = nullptr;
  m_UrineSpecificGravity = nullptr;
  m_UrineVolume = nullptr;
  m_UrineUreaNitrogenConcentration = nullptr;
}
//-------------------------------------------------------------------------------

SERenalSystem::~SERenalSystem()
{
  Clear();
}
//-------------------------------------------------------------------------------

void SERenalSystem::Clear()
{
  SESystem::Clear();

  SAFE_DELETE(m_GlomerularFiltrationRate);
  SAFE_DELETE(m_FiltrationFraction);

  SAFE_DELETE(m_AfferentArterioleResistance);
  SAFE_DELETE(m_BowmansCapsulesHydrostaticPressure);
  SAFE_DELETE(m_BowmansCapsulesOsmoticPressure);
  SAFE_DELETE(m_EfferentArterioleResistance);
  SAFE_DELETE(m_GlomerularCapillariesHydrostaticPressure);
  SAFE_DELETE(m_GlomerularCapillariesOsmoticPressure);
  SAFE_DELETE(m_GlomerularFiltrationCoefficient);
  SAFE_DELETE(m_GlomerularFiltrationSurfaceArea);
  SAFE_DELETE(m_GlomerularFluidPermeability);
  SAFE_DELETE(m_NetFiltrationPressure);
  SAFE_DELETE(m_NetReabsorptionPressure);
  SAFE_DELETE(m_PeritubularCapillariesHydrostaticPressure);
  SAFE_DELETE(m_PeritubularCapillariesOsmoticPressure);
  SAFE_DELETE(m_ReabsorptionFiltrationCoefficient);
  SAFE_DELETE(m_ReabsorptionRate);
  SAFE_DELETE(m_TubularReabsorptionFiltrationSurfaceArea);
  SAFE_DELETE(m_TubularReabsorptionFluidPermeability);
  SAFE_DELETE(m_TubularHydrostaticPressure);
  SAFE_DELETE(m_TubularOsmoticPressure);

  SAFE_DELETE(m_RenalBloodFlow);
  SAFE_DELETE(m_RenalPlasmaFlow);
  SAFE_DELETE(m_RenalVascularResistance);

  SAFE_DELETE(m_UrinationRate);
  SAFE_DELETE(m_UrineOsmolality);
  SAFE_DELETE(m_UrineOsmolarity);
  SAFE_DELETE(m_UrineProductionRate);
  SAFE_DELETE(m_UrineSpecificGravity);
  SAFE_DELETE(m_UrineVolume);
  SAFE_DELETE(m_UrineUreaNitrogenConcentration);
}
//-------------------------------------------------------------------------------
const SEScalar* SERenalSystem::GetScalar(const char* name)
{
  return GetScalar(std::string{ name });
}
//-------------------------------------------------------------------------------
const SEScalar* SERenalSystem::GetScalar(const std::string& name)
{
  if (name == idGlomerularFiltrationRate)
    return &GetGlomerularFiltrationRate();
  if (name == idFiltrationFraction)
    return &GetFiltrationFraction();

  if (name == idAfferentArterioleResistance)
    return &GetAfferentArterioleResistance();
  if (name == idBowmansCapsulesHydrostaticPressure)
    return &GetBowmansCapsulesHydrostaticPressure();
  if (name == idBowmansCapsulesOsmoticPressure)
    return &GetBowmansCapsulesOsmoticPressure();
  if (name == idEfferentArterioleResistance)
    return &GetEfferentArterioleResistance();
  if (name == idGlomerularCapillariesHydrostaticPressure)
    return &GetGlomerularCapillariesHydrostaticPressure();
  if (name == idGlomerularCapillariesOsmoticPressure)
    return &GetGlomerularCapillariesOsmoticPressure();
  if (name == idGlomerularFiltrationCoefficient)
    return &GetGlomerularFiltrationCoefficient();
  if (name == idGlomerularFiltrationRate)
    return &GetGlomerularFiltrationRate();
  if (name == idGlomerularFiltrationSurfaceArea)
    return &GetGlomerularFiltrationSurfaceArea();
  if (name == idGlomerularFluidPermeability)
    return &GetGlomerularFluidPermeability();
  if (name == idFiltrationFraction)
    return &GetFiltrationFraction();
  if (name == idNetFiltrationPressure)
    return &GetNetFiltrationPressure();
  if (name == idNetReabsorptionPressure)
    return &GetNetReabsorptionPressure();
  if (name == idPeritubularCapillariesHydrostaticPressure)
    return &GetPeritubularCapillariesHydrostaticPressure();
  if (name == idPeritubularCapillariesOsmoticPressure)
    return &GetPeritubularCapillariesOsmoticPressure();
  if (name == idReabsorptionFiltrationCoefficient)
    return &GetReabsorptionFiltrationCoefficient();
  if (name == idReabsorptionRate)
    return &GetReabsorptionRate();
  if (name == idTubularReabsorptionFiltrationSurfaceArea)
    return &GetTubularReabsorptionFiltrationSurfaceArea();
  if (name == idTubularReabsorptionFluidPermeability)
    return &GetTubularReabsorptionFluidPermeability();
  if (name == idTubularHydrostaticPressure)
    return &GetTubularHydrostaticPressure();
  if (name == idTubularOsmoticPressure)
    return &GetTubularOsmoticPressure();

  if (name == idRenalBloodFlow)
    return &GetRenalBloodFlow();
  if (name == idRenalPlasmaFlow)
    return &GetRenalPlasmaFlow();
  if (name == idRenalVascularResistance)
    return &GetRenalVascularResistance();

  if (name == idUrinationRate)
    return &GetUrinationRate();
  if (name == idUrineOsmolality)
    return &GetUrineOsmolality();
  if (name == idUrineOsmolarity)
    return &GetUrineOsmolarity();
  if (name == idUrineProductionRate)
    return &GetUrineProductionRate();
  if (name == idUrineSpecificGravity)
    return &GetUrineSpecificGravity();
  if (name == idUrineVolume)
    return &GetUrineVolume();
  if (name == idUrineUreaNitrogenConcentration)
    return &GetUrineUreaNitrogenConcentration();

  return nullptr;
}
//-------------------------------------------------------------------------------

bool SERenalSystem::Load(const CDM::RenalSystemData& in)
{
  SESystem::Load(in);

  if (in.GlomerularFiltrationRate().present())
    GetGlomerularFiltrationRate().Load(in.GlomerularFiltrationRate().get());
  if (in.FiltrationFraction().present())
    GetFiltrationFraction().Load(in.FiltrationFraction().get());

  if (in.AfferentArterioleResistance().present())
    GetAfferentArterioleResistance().Load(in.AfferentArterioleResistance().get());
  if (in.BowmansCapsulesHydrostaticPressure().present())
    GetBowmansCapsulesHydrostaticPressure().Load(in.BowmansCapsulesHydrostaticPressure().get());
  if (in.BowmansCapsulesOsmoticPressure().present())
    GetBowmansCapsulesOsmoticPressure().Load(in.BowmansCapsulesOsmoticPressure().get());
  if (in.EfferentArterioleResistance().present())
    GetEfferentArterioleResistance().Load(in.EfferentArterioleResistance().get());
  if (in.GlomerularCapillariesHydrostaticPressure().present())
    GetGlomerularCapillariesHydrostaticPressure().Load(in.GlomerularCapillariesHydrostaticPressure().get());
  if (in.GlomerularCapillariesOsmoticPressure().present())
    GetGlomerularCapillariesOsmoticPressure().Load(in.GlomerularCapillariesOsmoticPressure().get());
  if (in.GlomerularFiltrationCoefficient().present())
    GetGlomerularFiltrationCoefficient().Load(in.GlomerularFiltrationCoefficient().get());
  if (in.GlomerularFiltrationSurfaceArea().present())
    GetGlomerularFiltrationSurfaceArea().Load(in.GlomerularFiltrationSurfaceArea().get());
  if (in.GlomerularFluidPermeability().present())
    GetGlomerularFluidPermeability().Load(in.GlomerularFluidPermeability().get());
  if (in.NetFiltrationPressure().present())
    GetNetFiltrationPressure().Load(in.NetFiltrationPressure().get());
  if (in.NetReabsorptionPressure().present())
    GetNetReabsorptionPressure().Load(in.NetReabsorptionPressure().get());
  if (in.PeritubularCapillariesHydrostaticPressure().present())
    GetPeritubularCapillariesHydrostaticPressure().Load(in.PeritubularCapillariesHydrostaticPressure().get());
  if (in.PeritubularCapillariesOsmoticPressure().present())
    GetPeritubularCapillariesOsmoticPressure().Load(in.PeritubularCapillariesOsmoticPressure().get());
  if (in.ReabsorptionFiltrationCoefficient().present())
    GetReabsorptionFiltrationCoefficient().Load(in.ReabsorptionFiltrationCoefficient().get());
  if (in.ReabsorptionRate().present())
    GetReabsorptionRate().Load(in.ReabsorptionRate().get());
  if (in.TubularReabsorptionFiltrationSurfaceArea().present())
    GetTubularReabsorptionFiltrationSurfaceArea().Load(in.TubularReabsorptionFiltrationSurfaceArea().get());
  if (in.TubularReabsorptionFluidPermeability().present())
    GetTubularReabsorptionFluidPermeability().Load(in.TubularReabsorptionFluidPermeability().get());
  if (in.TubularHydrostaticPressure().present())
    GetTubularHydrostaticPressure().Load(in.TubularHydrostaticPressure().get());
  if (in.TubularOsmoticPressure().present())
    GetTubularOsmoticPressure().Load(in.TubularOsmoticPressure().get());

  if (in.RenalBloodFlow().present())
    GetRenalBloodFlow().Load(in.RenalBloodFlow().get());
  if (in.RenalPlasmaFlow().present())
    GetRenalPlasmaFlow().Load(in.RenalPlasmaFlow().get());
  if (in.RenalVascularResistance().present())
    GetRenalVascularResistance().Load(in.RenalVascularResistance().get());

  if (in.UrinationRate().present())
    GetUrinationRate().Load(in.UrinationRate().get());
  if (in.UrineOsmolality().present())
    GetUrineOsmolality().Load(in.UrineOsmolality().get());
  if (in.UrineOsmolarity().present())
    GetUrineOsmolarity().Load(in.UrineOsmolarity().get());
  if (in.UrineProductionRate().present())
    GetUrineProductionRate().Load(in.UrineProductionRate().get());
  if (in.UrineSpecificGravity().present())
    GetUrineSpecificGravity().Load(in.UrineSpecificGravity().get());
  if (in.UrineVolume().present())
    GetUrineVolume().Load(in.UrineVolume().get());
  if (in.UrineUreaNitrogenConcentration().present())
    GetUrineUreaNitrogenConcentration().Load(in.UrineUreaNitrogenConcentration().get());

  return true;
}
//-------------------------------------------------------------------------------

CDM::RenalSystemData* SERenalSystem::Unload() const
{
  CDM::RenalSystemData* data = new CDM::RenalSystemData();
  Unload(*data);
  return data;
}
//-------------------------------------------------------------------------------

void SERenalSystem::Unload(CDM::RenalSystemData& data) const
{
  SESystem::Unload(data);
  if (m_GlomerularFiltrationRate != nullptr)
    data.GlomerularFiltrationRate(std::unique_ptr<CDM::ScalarVolumePerTimeData>(m_GlomerularFiltrationRate->Unload()));
  if (m_FiltrationFraction != nullptr)
    data.FiltrationFraction(std::unique_ptr<CDM::ScalarFractionData>(m_FiltrationFraction->Unload()));

  if (m_AfferentArterioleResistance != nullptr)
    data.AfferentArterioleResistance(std::unique_ptr<CDM::ScalarFlowResistanceData>(m_AfferentArterioleResistance->Unload()));
  if (m_BowmansCapsulesHydrostaticPressure != nullptr)
    data.BowmansCapsulesHydrostaticPressure(std::unique_ptr<CDM::ScalarPressureData>(m_BowmansCapsulesHydrostaticPressure->Unload()));
  if (m_BowmansCapsulesOsmoticPressure != nullptr)
    data.BowmansCapsulesOsmoticPressure(std::unique_ptr<CDM::ScalarPressureData>(m_BowmansCapsulesOsmoticPressure->Unload()));
  if (m_EfferentArterioleResistance != nullptr)
    data.EfferentArterioleResistance(std::unique_ptr<CDM::ScalarFlowResistanceData>(m_EfferentArterioleResistance->Unload()));
  if (m_GlomerularCapillariesHydrostaticPressure != nullptr)
    data.GlomerularCapillariesHydrostaticPressure(std::unique_ptr<CDM::ScalarPressureData>(m_GlomerularCapillariesHydrostaticPressure->Unload()));
  if (m_GlomerularCapillariesOsmoticPressure != nullptr)
    data.GlomerularCapillariesOsmoticPressure(std::unique_ptr<CDM::ScalarPressureData>(m_GlomerularCapillariesOsmoticPressure->Unload()));
  if (m_GlomerularFiltrationCoefficient != nullptr)
    data.GlomerularFiltrationCoefficient(std::unique_ptr<CDM::ScalarVolumePerTimePressureData>(m_GlomerularFiltrationCoefficient->Unload()));
  if (m_GlomerularFiltrationSurfaceArea != nullptr)
    data.GlomerularFiltrationSurfaceArea(std::unique_ptr<CDM::ScalarAreaData>(m_GlomerularFiltrationSurfaceArea->Unload()));
  if (m_GlomerularFluidPermeability != nullptr)
    data.GlomerularFluidPermeability(std::unique_ptr<CDM::ScalarVolumePerTimePressureAreaData>(m_GlomerularFluidPermeability->Unload()));
  if (m_NetFiltrationPressure != nullptr)
    data.NetFiltrationPressure(std::unique_ptr<CDM::ScalarPressureData>(m_NetFiltrationPressure->Unload()));
  if (m_NetReabsorptionPressure != nullptr)
    data.NetReabsorptionPressure(std::unique_ptr<CDM::ScalarPressureData>(m_NetReabsorptionPressure->Unload()));
  if (m_PeritubularCapillariesHydrostaticPressure != nullptr)
    data.PeritubularCapillariesHydrostaticPressure(std::unique_ptr<CDM::ScalarPressureData>(m_PeritubularCapillariesHydrostaticPressure->Unload()));
  if (m_PeritubularCapillariesOsmoticPressure != nullptr)
    data.PeritubularCapillariesOsmoticPressure(std::unique_ptr<CDM::ScalarPressureData>(m_PeritubularCapillariesOsmoticPressure->Unload()));
  if (m_ReabsorptionFiltrationCoefficient != nullptr)
    data.ReabsorptionFiltrationCoefficient(std::unique_ptr<CDM::ScalarVolumePerTimePressureData>(m_ReabsorptionFiltrationCoefficient->Unload()));
  if (m_ReabsorptionRate != nullptr)
    data.ReabsorptionRate(std::unique_ptr<CDM::ScalarVolumePerTimeData>(m_ReabsorptionRate->Unload()));
  if (m_TubularReabsorptionFiltrationSurfaceArea != nullptr)
    data.TubularReabsorptionFiltrationSurfaceArea(std::unique_ptr<CDM::ScalarAreaData>(m_TubularReabsorptionFiltrationSurfaceArea->Unload()));
  if (m_TubularReabsorptionFluidPermeability != nullptr)
    data.TubularReabsorptionFluidPermeability(std::unique_ptr<CDM::ScalarVolumePerTimePressureAreaData>(m_TubularReabsorptionFluidPermeability->Unload()));
  if (m_TubularHydrostaticPressure != nullptr)
    data.TubularHydrostaticPressure(std::unique_ptr<CDM::ScalarPressureData>(m_TubularHydrostaticPressure->Unload()));
  if (m_TubularOsmoticPressure != nullptr)
    data.TubularOsmoticPressure(std::unique_ptr<CDM::ScalarPressureData>(m_TubularOsmoticPressure->Unload()));

  if (m_RenalBloodFlow != nullptr)
    data.RenalBloodFlow(std::unique_ptr<CDM::ScalarVolumePerTimeData>(m_RenalBloodFlow->Unload()));
  if (m_RenalPlasmaFlow != nullptr)
    data.RenalPlasmaFlow(std::unique_ptr<CDM::ScalarVolumePerTimeData>(m_RenalPlasmaFlow->Unload()));
  if (m_RenalVascularResistance != nullptr)
    data.RenalVascularResistance(std::unique_ptr<CDM::ScalarFlowResistanceData>(m_RenalVascularResistance->Unload()));

  if (m_UrinationRate != nullptr)
    data.UrinationRate(std::unique_ptr<CDM::ScalarVolumePerTimeData>(m_UrinationRate->Unload()));
  if (m_UrineOsmolality != nullptr)
    data.UrineOsmolality(std::unique_ptr<CDM::ScalarOsmolalityData>(m_UrineOsmolality->Unload()));
  if (m_UrineOsmolarity != nullptr)
    data.UrineOsmolarity(std::unique_ptr<CDM::ScalarOsmolarityData>(m_UrineOsmolarity->Unload()));
  if (m_UrineProductionRate != nullptr)
    data.UrineProductionRate(std::unique_ptr<CDM::ScalarVolumePerTimeData>(m_UrineProductionRate->Unload()));
  if (m_UrineSpecificGravity != nullptr)
    data.UrineSpecificGravity(std::unique_ptr<CDM::ScalarData>(m_UrineSpecificGravity->Unload()));
  if (m_UrineVolume != nullptr)
    data.UrineVolume(std::unique_ptr<CDM::ScalarVolumeData>(m_UrineVolume->Unload()));
  if (m_UrineUreaNitrogenConcentration != nullptr)
    data.UrineUreaNitrogenConcentration(std::unique_ptr<CDM::ScalarMassPerVolumeData>(m_UrineUreaNitrogenConcentration->Unload()));
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasGlomerularFiltrationRate() const
{
  return m_GlomerularFiltrationRate == nullptr ? false : m_GlomerularFiltrationRate->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolumePerTime& SERenalSystem::GetGlomerularFiltrationRate()
{
  if (m_GlomerularFiltrationRate == nullptr)
    m_GlomerularFiltrationRate = new SEScalarVolumePerTime();
  return *m_GlomerularFiltrationRate;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetGlomerularFiltrationRate(const VolumePerTimeUnit& unit) const
{
  if (m_GlomerularFiltrationRate == nullptr)
    return SEScalar::dNaN();
  return m_GlomerularFiltrationRate->GetValue(unit);
}

//-------------------------------------------------------------------------------
bool SERenalSystem::HasFiltrationFraction() const
{
  return m_FiltrationFraction == nullptr ? false : m_FiltrationFraction->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarFraction& SERenalSystem::GetFiltrationFraction()
{
  if (m_FiltrationFraction == nullptr)
    m_FiltrationFraction = new SEScalarFraction();
  return *m_FiltrationFraction;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetFiltrationFraction() const
{
  if (m_FiltrationFraction == nullptr)
    return SEScalar::dNaN();
  return m_FiltrationFraction->GetValue();
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasAfferentArterioleResistance() const
{
  return m_AfferentArterioleResistance == nullptr ? false : m_AfferentArterioleResistance->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarFlowResistance& SERenalSystem::GetAfferentArterioleResistance()
{
  if (m_AfferentArterioleResistance == nullptr)
    m_AfferentArterioleResistance = new SEScalarFlowResistance();
  return *m_AfferentArterioleResistance;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetAfferentArterioleResistance(const FlowResistanceUnit& unit) const
{
  if (m_AfferentArterioleResistance == nullptr)
    return SEScalar::dNaN();
  return m_AfferentArterioleResistance->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasBowmansCapsulesHydrostaticPressure() const
{
  return m_BowmansCapsulesHydrostaticPressure == nullptr ? false : m_BowmansCapsulesHydrostaticPressure->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarPressure& SERenalSystem::GetBowmansCapsulesHydrostaticPressure()
{
  if (m_BowmansCapsulesHydrostaticPressure == nullptr)
    m_BowmansCapsulesHydrostaticPressure = new SEScalarPressure();
  return *m_BowmansCapsulesHydrostaticPressure;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetBowmansCapsulesHydrostaticPressure(const PressureUnit& unit) const
{
  if (m_BowmansCapsulesHydrostaticPressure == nullptr)
    return SEScalar::dNaN();
  return m_BowmansCapsulesHydrostaticPressure->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasBowmansCapsulesOsmoticPressure() const
{
  return m_BowmansCapsulesOsmoticPressure == nullptr ? false : m_BowmansCapsulesOsmoticPressure->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarPressure& SERenalSystem::GetBowmansCapsulesOsmoticPressure()
{
  if (m_BowmansCapsulesOsmoticPressure == nullptr)
    m_BowmansCapsulesOsmoticPressure = new SEScalarPressure();
  return *m_BowmansCapsulesOsmoticPressure;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetBowmansCapsulesOsmoticPressure(const PressureUnit& unit) const
{
  if (m_BowmansCapsulesOsmoticPressure == nullptr)
    return SEScalar::dNaN();
  return m_BowmansCapsulesOsmoticPressure->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasEfferentArterioleResistance() const
{
  return m_EfferentArterioleResistance == nullptr ? false : m_EfferentArterioleResistance->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarFlowResistance& SERenalSystem::GetEfferentArterioleResistance()
{
  if (m_EfferentArterioleResistance == nullptr)
    m_EfferentArterioleResistance = new SEScalarFlowResistance();
  return *m_EfferentArterioleResistance;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetEfferentArterioleResistance(const FlowResistanceUnit& unit) const
{
  if (m_EfferentArterioleResistance == nullptr)
    return SEScalar::dNaN();
  return m_EfferentArterioleResistance->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasGlomerularCapillariesHydrostaticPressure() const
{
  return m_GlomerularCapillariesHydrostaticPressure == nullptr ? false : m_GlomerularCapillariesHydrostaticPressure->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarPressure& SERenalSystem::GetGlomerularCapillariesHydrostaticPressure()
{
  if (m_GlomerularCapillariesHydrostaticPressure == nullptr)
    m_GlomerularCapillariesHydrostaticPressure = new SEScalarPressure();
  return *m_GlomerularCapillariesHydrostaticPressure;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetGlomerularCapillariesHydrostaticPressure(const PressureUnit& unit) const
{
  if (m_GlomerularCapillariesHydrostaticPressure == nullptr)
    return SEScalar::dNaN();
  return m_GlomerularCapillariesHydrostaticPressure->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasGlomerularCapillariesOsmoticPressure() const
{
  return m_GlomerularCapillariesOsmoticPressure == nullptr ? false : m_GlomerularCapillariesOsmoticPressure->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarPressure& SERenalSystem::GetGlomerularCapillariesOsmoticPressure()
{
  if (m_GlomerularCapillariesOsmoticPressure == nullptr)
    m_GlomerularCapillariesOsmoticPressure = new SEScalarPressure();
  return *m_GlomerularCapillariesOsmoticPressure;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetGlomerularCapillariesOsmoticPressure(const PressureUnit& unit) const
{
  if (m_GlomerularCapillariesOsmoticPressure == nullptr)
    return SEScalar::dNaN();
  return m_GlomerularCapillariesOsmoticPressure->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasGlomerularFiltrationCoefficient() const
{
  return m_GlomerularFiltrationCoefficient == nullptr ? false : m_GlomerularFiltrationCoefficient->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolumePerTimePressure& SERenalSystem::GetGlomerularFiltrationCoefficient()
{
  if (m_GlomerularFiltrationCoefficient == nullptr)
    m_GlomerularFiltrationCoefficient = new SEScalarVolumePerTimePressure();
  return *m_GlomerularFiltrationCoefficient;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetGlomerularFiltrationCoefficient(const VolumePerTimePressureUnit& unit) const
{
  if (m_GlomerularFiltrationCoefficient == nullptr)
    return SEScalar::dNaN();
  return m_GlomerularFiltrationCoefficient->GetValue(unit);
}
//-------------------------------------------------------------------------------
bool SERenalSystem::HasGlomerularFiltrationSurfaceArea() const
{
  return m_GlomerularFiltrationSurfaceArea == nullptr ? false : m_GlomerularFiltrationSurfaceArea->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarArea& SERenalSystem::GetGlomerularFiltrationSurfaceArea()
{
  if (m_GlomerularFiltrationSurfaceArea == nullptr)
    m_GlomerularFiltrationSurfaceArea = new SEScalarArea();
  return *m_GlomerularFiltrationSurfaceArea;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetGlomerularFiltrationSurfaceArea(const AreaUnit& unit) const
{
  if (m_GlomerularFiltrationSurfaceArea == nullptr)
    return SEScalar::dNaN();
  return m_GlomerularFiltrationSurfaceArea->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasGlomerularFluidPermeability() const
{
  return m_GlomerularFluidPermeability == nullptr ? false : m_GlomerularFluidPermeability->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolumePerTimePressureArea& SERenalSystem::GetGlomerularFluidPermeability()
{
  if (m_GlomerularFluidPermeability == nullptr)
    m_GlomerularFluidPermeability = new SEScalarVolumePerTimePressureArea();
  return *m_GlomerularFluidPermeability;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetGlomerularFluidPermeability(const VolumePerTimePressureAreaUnit& unit) const
{
  if (m_GlomerularFluidPermeability == nullptr)
    return SEScalar::dNaN();
  return m_GlomerularFluidPermeability->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasNetFiltrationPressure() const
{
  return m_NetFiltrationPressure == nullptr ? false : m_NetFiltrationPressure->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarPressure& SERenalSystem::GetNetFiltrationPressure()
{
  if (m_NetFiltrationPressure == nullptr)
    m_NetFiltrationPressure = new SEScalarPressure();
  return *m_NetFiltrationPressure;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetNetFiltrationPressure(const PressureUnit& unit) const
{
  if (m_NetFiltrationPressure == nullptr)
    return SEScalar::dNaN();
  return m_NetFiltrationPressure->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasNetReabsorptionPressure() const
{
  return m_NetReabsorptionPressure == nullptr ? false : m_NetReabsorptionPressure->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarPressure& SERenalSystem::GetNetReabsorptionPressure()
{
  if (m_NetReabsorptionPressure == nullptr)
    m_NetReabsorptionPressure = new SEScalarPressure();
  return *m_NetReabsorptionPressure;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetNetReabsorptionPressure(const PressureUnit& unit) const
{
  if (m_NetReabsorptionPressure == nullptr)
    return SEScalar::dNaN();
  return m_NetReabsorptionPressure->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasPeritubularCapillariesHydrostaticPressure() const
{
  return m_PeritubularCapillariesHydrostaticPressure == nullptr ? false : m_PeritubularCapillariesHydrostaticPressure->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarPressure& SERenalSystem::GetPeritubularCapillariesHydrostaticPressure()
{
  if (m_PeritubularCapillariesHydrostaticPressure == nullptr)
    m_PeritubularCapillariesHydrostaticPressure = new SEScalarPressure();
  return *m_PeritubularCapillariesHydrostaticPressure;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetPeritubularCapillariesHydrostaticPressure(const PressureUnit& unit) const
{
  if (m_PeritubularCapillariesHydrostaticPressure == nullptr)
    return SEScalar::dNaN();
  return m_PeritubularCapillariesHydrostaticPressure->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasPeritubularCapillariesOsmoticPressure() const
{
  return m_PeritubularCapillariesOsmoticPressure == nullptr ? false : m_PeritubularCapillariesOsmoticPressure->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarPressure& SERenalSystem::GetPeritubularCapillariesOsmoticPressure()
{
  if (m_PeritubularCapillariesOsmoticPressure == nullptr)
    m_PeritubularCapillariesOsmoticPressure = new SEScalarPressure();
  return *m_PeritubularCapillariesOsmoticPressure;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetPeritubularCapillariesOsmoticPressure(const PressureUnit& unit) const
{
  if (m_PeritubularCapillariesOsmoticPressure == nullptr)
    return SEScalar::dNaN();
  return m_PeritubularCapillariesOsmoticPressure->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasReabsorptionFiltrationCoefficient() const
{
  return m_ReabsorptionFiltrationCoefficient == nullptr ? false : m_ReabsorptionFiltrationCoefficient->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolumePerTimePressure& SERenalSystem::GetReabsorptionFiltrationCoefficient()
{
  if (m_ReabsorptionFiltrationCoefficient == nullptr)
    m_ReabsorptionFiltrationCoefficient = new SEScalarVolumePerTimePressure();
  return *m_ReabsorptionFiltrationCoefficient;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetReabsorptionFiltrationCoefficient(const VolumePerTimePressureUnit& unit) const
{
  if (m_ReabsorptionFiltrationCoefficient == nullptr)
    return SEScalar::dNaN();
  return m_ReabsorptionFiltrationCoefficient->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasReabsorptionRate() const
{
  return m_ReabsorptionRate == nullptr ? false : m_ReabsorptionRate->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolumePerTime& SERenalSystem::GetReabsorptionRate()
{
  if (m_ReabsorptionRate == nullptr)
    m_ReabsorptionRate = new SEScalarVolumePerTime();
  return *m_ReabsorptionRate;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetReabsorptionRate(const VolumePerTimeUnit& unit) const
{
  if (m_ReabsorptionRate == nullptr)
    return SEScalar::dNaN();
  return m_ReabsorptionRate->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasTubularReabsorptionFiltrationSurfaceArea() const
{
  return m_TubularReabsorptionFiltrationSurfaceArea == nullptr ? false : m_TubularReabsorptionFiltrationSurfaceArea->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarArea& SERenalSystem::GetTubularReabsorptionFiltrationSurfaceArea()
{
  if (m_TubularReabsorptionFiltrationSurfaceArea == nullptr)
    m_TubularReabsorptionFiltrationSurfaceArea = new SEScalarArea();
  return *m_TubularReabsorptionFiltrationSurfaceArea;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetTubularReabsorptionFiltrationSurfaceArea(const AreaUnit& unit) const
{
  if (m_TubularReabsorptionFiltrationSurfaceArea == nullptr)
    return SEScalar::dNaN();
  return m_TubularReabsorptionFiltrationSurfaceArea->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasTubularReabsorptionFluidPermeability() const
{
  return m_TubularReabsorptionFluidPermeability == nullptr ? false : m_TubularReabsorptionFluidPermeability->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolumePerTimePressureArea& SERenalSystem::GetTubularReabsorptionFluidPermeability()
{
  if (m_TubularReabsorptionFluidPermeability == nullptr)
    m_TubularReabsorptionFluidPermeability = new SEScalarVolumePerTimePressureArea();
  return *m_TubularReabsorptionFluidPermeability;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetTubularReabsorptionFluidPermeability(const VolumePerTimePressureAreaUnit& unit) const
{
  if (m_TubularReabsorptionFluidPermeability == nullptr)
    return SEScalar::dNaN();
  return m_TubularReabsorptionFluidPermeability->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasTubularHydrostaticPressure() const
{
  return m_TubularHydrostaticPressure == nullptr ? false : m_TubularHydrostaticPressure->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarPressure& SERenalSystem::GetTubularHydrostaticPressure()
{
  if (m_TubularHydrostaticPressure == nullptr)
    m_TubularHydrostaticPressure = new SEScalarPressure();
  return *m_TubularHydrostaticPressure;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetTubularHydrostaticPressure(const PressureUnit& unit) const
{
  if (m_TubularHydrostaticPressure == nullptr)
    return SEScalar::dNaN();
  return m_TubularHydrostaticPressure->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasTubularOsmoticPressure() const
{
  return m_TubularOsmoticPressure == nullptr ? false : m_TubularOsmoticPressure->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarPressure& SERenalSystem::GetTubularOsmoticPressure()
{
  if (m_TubularOsmoticPressure == nullptr)
    m_TubularOsmoticPressure = new SEScalarPressure();
  return *m_TubularOsmoticPressure;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetTubularOsmoticPressure(const PressureUnit& unit) const
{
  if (m_TubularOsmoticPressure == nullptr)
    return SEScalar::dNaN();
  return m_TubularOsmoticPressure->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasRenalBloodFlow() const
{
  return m_RenalBloodFlow == nullptr ? false : m_RenalBloodFlow->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolumePerTime& SERenalSystem::GetRenalBloodFlow()
{
  if (m_RenalBloodFlow == nullptr)
    m_RenalBloodFlow = new SEScalarVolumePerTime();
  return *m_RenalBloodFlow;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetRenalBloodFlow(const VolumePerTimeUnit& unit) const
{
  if (m_RenalBloodFlow == nullptr)
    return SEScalar::dNaN();
  return m_RenalBloodFlow->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasRenalPlasmaFlow() const
{
  return m_RenalPlasmaFlow == nullptr ? false : m_RenalPlasmaFlow->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolumePerTime& SERenalSystem::GetRenalPlasmaFlow()
{
  if (m_RenalPlasmaFlow == nullptr)
    m_RenalPlasmaFlow = new SEScalarVolumePerTime();
  return *m_RenalPlasmaFlow;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetRenalPlasmaFlow(const VolumePerTimeUnit& unit) const
{
  if (m_RenalPlasmaFlow == nullptr)
    return SEScalar::dNaN();
  return m_RenalPlasmaFlow->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasRenalVascularResistance() const
{
  return m_RenalVascularResistance == nullptr ? false : m_RenalVascularResistance->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarFlowResistance& SERenalSystem::GetRenalVascularResistance()
{
  if (m_RenalVascularResistance == nullptr)
    m_RenalVascularResistance = new SEScalarFlowResistance();
  return *m_RenalVascularResistance;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetRenalVascularResistance(const FlowResistanceUnit& unit) const
{
  if (m_RenalVascularResistance == nullptr)
    return SEScalar::dNaN();
  return m_RenalVascularResistance->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasUrinationRate() const
{
  return m_UrinationRate == nullptr ? false : m_UrinationRate->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolumePerTime& SERenalSystem::GetUrinationRate()
{
  if (m_UrinationRate == nullptr)
    m_UrinationRate = new SEScalarVolumePerTime();
  return *m_UrinationRate;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetUrinationRate(const VolumePerTimeUnit& unit) const
{
  if (m_UrinationRate == nullptr)
    return SEScalar::dNaN();
  return m_UrinationRate->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasUrineOsmolality() const
{
  return m_UrineOsmolality == nullptr ? false : m_UrineOsmolality->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarOsmolality& SERenalSystem::GetUrineOsmolality()
{
  if (m_UrineOsmolality == nullptr)
    m_UrineOsmolality = new SEScalarOsmolality();
  return *m_UrineOsmolality;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetUrineOsmolality(const OsmolalityUnit& unit) const
{
  if (m_UrineOsmolality == nullptr)
    return SEScalar::dNaN();
  return m_UrineOsmolality->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasUrineOsmolarity() const
{
  return m_UrineOsmolarity == nullptr ? false : m_UrineOsmolarity->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarOsmolarity& SERenalSystem::GetUrineOsmolarity()
{
  if (m_UrineOsmolarity == nullptr)
    m_UrineOsmolarity = new SEScalarOsmolarity();
  return *m_UrineOsmolarity;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetUrineOsmolarity(const OsmolarityUnit& unit) const
{
  if (m_UrineOsmolarity == nullptr)
    return SEScalar::dNaN();
  return m_UrineOsmolarity->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasUrineProductionRate() const
{
  return m_UrineProductionRate == nullptr ? false : m_UrineProductionRate->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolumePerTime& SERenalSystem::GetUrineProductionRate()
{
  if (m_UrineProductionRate == nullptr)
    m_UrineProductionRate = new SEScalarVolumePerTime();
  return *m_UrineProductionRate;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetUrineProductionRate(const VolumePerTimeUnit& unit) const
{
  if (m_UrineProductionRate == nullptr)
    return SEScalar::dNaN();
  return m_UrineProductionRate->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasUrineSpecificGravity() const
{
  return m_UrineSpecificGravity == nullptr ? false : m_UrineSpecificGravity->IsValid();
}
//-------------------------------------------------------------------------------
SEScalar& SERenalSystem::GetUrineSpecificGravity()
{
  if (m_UrineSpecificGravity == nullptr)
    m_UrineSpecificGravity = new SEScalar();
  return *m_UrineSpecificGravity;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetUrineSpecificGravity() const
{
  if (m_UrineSpecificGravity == nullptr)
    return SEScalar::dNaN();
  return m_UrineSpecificGravity->GetValue();
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasUrineVolume() const
{
  return m_UrineVolume == nullptr ? false : m_UrineVolume->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarVolume& SERenalSystem::GetUrineVolume()
{
  if (m_UrineVolume == nullptr)
    m_UrineVolume = new SEScalarVolume();
  return *m_UrineVolume;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetUrineVolume(const VolumeUnit& unit) const
{
  if (m_UrineVolume == nullptr)
    return SEScalar::dNaN();
  return m_UrineVolume->GetValue(unit);
}
//-------------------------------------------------------------------------------

bool SERenalSystem::HasUrineUreaNitrogenConcentration() const
{
  return m_UrineUreaNitrogenConcentration == nullptr ? false : m_UrineUreaNitrogenConcentration->IsValid();
}
//-------------------------------------------------------------------------------
SEScalarMassPerVolume& SERenalSystem::GetUrineUreaNitrogenConcentration()
{
  if (m_UrineUreaNitrogenConcentration == nullptr)
    m_UrineUreaNitrogenConcentration = new SEScalarMassPerVolume();
  return *m_UrineUreaNitrogenConcentration;
}
//-------------------------------------------------------------------------------
double SERenalSystem::GetUrineUreaNitrogenConcentration(const MassPerVolumeUnit& unit) const
{
  if (m_UrineUreaNitrogenConcentration == nullptr)
    return SEScalar::dNaN();
  return m_UrineUreaNitrogenConcentration->GetValue(unit);
}
//-------------------------------------------------------------------------------

Tree<const char*> SERenalSystem::GetPhysiologyRequestGraph() const
{
  return Tree<const char*>{classname()}
    .emplace_back(idGlomerularFiltrationRate)
    .emplace_back(idFiltrationFraction)
    .emplace_back(idAfferentArterioleResistance)
    .emplace_back(idBowmansCapsulesHydrostaticPressure)
    .emplace_back(idBowmansCapsulesOsmoticPressure)
    .emplace_back(idEfferentArterioleResistance)
    .emplace_back(idGlomerularCapillariesHydrostaticPressure)
    .emplace_back(idGlomerularCapillariesOsmoticPressure)
    .emplace_back(idGlomerularFiltrationCoefficient)
    .emplace_back(idGlomerularFiltrationSurfaceArea)
    .emplace_back(idGlomerularFluidPermeability)
    .emplace_back(idNetFiltrationPressure)
    .emplace_back(idNetReabsorptionPressure)
    .emplace_back(idPeritubularCapillariesHydrostaticPressure)
    .emplace_back(idPeritubularCapillariesOsmoticPressure)
    .emplace_back(idReabsorptionFiltrationCoefficient)
    .emplace_back(idReabsorptionRate)
    .emplace_back(idTubularReabsorptionFiltrationSurfaceArea)
    .emplace_back(idTubularReabsorptionFluidPermeability)
    .emplace_back(idTubularHydrostaticPressure)
    .emplace_back(idTubularOsmoticPressure)
    .emplace_back(idRenalBloodFlow)
    .emplace_back(idRenalPlasmaFlow)
    .emplace_back(idRenalVascularResistance)
    .emplace_back(idUrinationRate)
    .emplace_back(idUrineOsmolality)
    .emplace_back(idUrineOsmolarity)
    .emplace_back(idUrineProductionRate)
    .emplace_back(idUrineSpecificGravity)
    .emplace_back(idUrineVolume)
    .emplace_back(idUrineUreaNitrogenConcentration)
  ;
}
}