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
#include <biogears/engine/Controller/BioGearsSubstances.h>

#include <biogears/cdm/compartment/fluid/SEGasCompartment.h>
#include <biogears/cdm/compartment/fluid/SEGasCompartmentLink.h>
#include <biogears/cdm/compartment/fluid/SELiquidCompartment.h>
#include <biogears/cdm/compartment/fluid/SELiquidCompartmentLink.h>
#include <biogears/cdm/compartment/substances/SEGasSubstanceQuantity.h>
#include <biogears/cdm/compartment/substances/SELiquidSubstanceQuantity.h>
#include <biogears/cdm/compartment/tissue/SETissueCompartment.h>
#include <biogears/cdm/properties/SEHistogramFractionVsLength.h>
#include <biogears/cdm/properties/SEScalarAmountPerVolume.h>
#include <biogears/cdm/properties/SEScalarFraction.h>
#include <biogears/cdm/properties/SEScalarMassPerAmount.h>
#include <biogears/cdm/properties/SEScalarMassPerVolume.h>
#include <biogears/cdm/system/environment/SEEnvironment.h>
#include <biogears/engine/BioGearsPhysiologyEngine.h>
#include <biogears/engine/Controller/BioGears.h>

namespace BGE = mil::tatrc::physiology::biogears;

namespace biogears {
BioGearsSubstances::BioGearsSubstances(BioGears& data)
  : SESubstanceManager(data.GetLogger())
  , m_data(data)
{
  m_isCOActive = false;
}

void BioGearsSubstances::Clear()
{
  SESubstanceManager::Clear();
  m_O2 = nullptr;
  m_CO = nullptr;
  m_CO2 = nullptr;
  m_N2 = nullptr;
  m_Hb = nullptr;
  m_HbO2 = nullptr;
  m_HbCO2 = nullptr;
  m_HbCO = nullptr;
  m_HCO3 = nullptr;
  m_epi = nullptr;

  m_albumin = nullptr;
  m_aminoAcids = nullptr;
  m_calcium = nullptr;
  m_chloride = nullptr;
  m_creatinine = nullptr;
  m_globulin = nullptr;
  m_glucagon = nullptr;
  m_glucose = nullptr;
  m_insulin = nullptr;
  m_ketones = nullptr;
  m_lactate = nullptr;
  m_potassium = nullptr;
  m_triacylglycerol = nullptr;
  m_sodium = nullptr;
  m_urea = nullptr;
}

void BioGearsSubstances::InitializeSubstances()
{
  // NOTE!!
  // The way BioGears sets up,
  // Substance initialization relies on environmental state,
  // so the environment will read the environment file associated
  // with this engine instance, and it will call AddActiveSubstance
  // and those gases will be added in the order they are read from the file
  // So your substance lists are not guaranteed to be in this order.
  // If this is ever a huge problem, or order is necessary for optimization or something
  // Just make an activate substances method that has to be called first,
  // That just makes an extra thing for unit tests and all that jazz to worry about
  // so I chose not to worry about it as I consider it a minor annoyance

  AddActiveSubstance(*m_O2);
  AddActiveSubstance(*m_CO2);
  AddActiveSubstance(*m_N2);

  AddActiveSubstance(*m_Hb);
  AddActiveSubstance(*m_HbO2);
  AddActiveSubstance(*m_HbCO2);
  AddActiveSubstance(*m_HCO3);
  AddActiveSubstance(*m_epi);

  AddActiveSubstance(*m_albumin);
  AddActiveSubstance(*m_aminoAcids);
  AddActiveSubstance(*m_calcium);
  AddActiveSubstance(*m_chloride);
  AddActiveSubstance(*m_creatinine);
  AddActiveSubstance(*m_glucagon);
  AddActiveSubstance(*m_glucose);
  AddActiveSubstance(*m_insulin);
  AddActiveSubstance(*m_ketones);
  AddActiveSubstance(*m_lactate);
  AddActiveSubstance(*m_potassium);
  AddActiveSubstance(*m_sodium);
  AddActiveSubstance(*m_triacylglycerol);
  AddActiveSubstance(*m_urea);

  InitializeGasCompartments();
  InitializeLiquidCompartmentGases();
  InitializeLiquidCompartmentNonGases();
}

void BioGearsSubstances::InitializeGasCompartments()
{
  SEGasCompartment* Ambient = m_data.GetCompartments().GetGasCompartment(BGE::EnvironmentCompartment::Ambient);
  double AmbientO2VF = Ambient->GetSubstanceQuantity(*m_O2)->GetVolumeFraction().GetValue();
  double AmbientCO2VF = Ambient->GetSubstanceQuantity(*m_CO2)->GetVolumeFraction().GetValue();
  double AmbientN2VF = Ambient->GetSubstanceQuantity(*m_N2)->GetVolumeFraction().GetValue();

  SEGasCompartment* Mouth = m_data.GetCompartments().GetGasCompartment(BGE::PulmonaryCompartment::Mouth);
  Mouth->GetSubstanceQuantity(*m_CO2)->GetVolumeFraction().SetValue(AmbientCO2VF);
  Mouth->GetSubstanceQuantity(*m_N2)->GetVolumeFraction().SetValue(AmbientN2VF);
  Mouth->GetSubstanceQuantity(*m_O2)->GetVolumeFraction().SetValue(AmbientO2VF);
  SEGasCompartment* Trachea = m_data.GetCompartments().GetGasCompartment(BGE::PulmonaryCompartment::Trachea);
  Trachea->GetSubstanceQuantity(*m_CO2)->GetVolumeFraction().SetValue(0.0413596);
  Trachea->GetSubstanceQuantity(*m_O2)->GetVolumeFraction().SetValue(0.161171);
  Trachea->GetSubstanceQuantity(*m_N2)->GetVolumeFraction().SetValue(1.0 - 0.0413596 - 0.161171);
  Trachea->Balance(BalanceGasBy::VolumeFraction);
  SEGasCompartment* Bronchi = m_data.GetCompartments().GetGasCompartment(BGE::PulmonaryCompartment::Bronchi);
  Bronchi->GetSubstanceQuantity(*m_CO2)->GetVolumeFraction().SetValue(0.0430653);
  Bronchi->GetSubstanceQuantity(*m_O2)->GetVolumeFraction().SetValue(0.159124);
  Bronchi->GetSubstanceQuantity(*m_N2)->GetVolumeFraction().SetValue(1.0 - 0.0430653 - 0.159124);
  Bronchi->Balance(BalanceGasBy::VolumeFraction);
  SEGasCompartment* Alveoli = m_data.GetCompartments().GetGasCompartment(BGE::PulmonaryCompartment::Alveoli);
  Alveoli->GetSubstanceQuantity(*m_CO2)->GetVolumeFraction().SetValue(0.049271);
  Alveoli->GetSubstanceQuantity(*m_O2)->GetVolumeFraction().SetValue(0.151463);
  Alveoli->GetSubstanceQuantity(*m_N2)->GetVolumeFraction().SetValue(1.0 - 0.049271 - 0.151463);
  Alveoli->Balance(BalanceGasBy::VolumeFraction);
  SEGasCompartment* PleuralCavity = m_data.GetCompartments().GetGasCompartment(BGE::PulmonaryCompartment::Pleural);
  PleuralCavity->GetSubstanceQuantity(*m_CO2)->GetVolumeFraction().SetValue(AmbientCO2VF);
  PleuralCavity->GetSubstanceQuantity(*m_N2)->GetVolumeFraction().SetValue(AmbientN2VF);
  PleuralCavity->GetSubstanceQuantity(*m_O2)->GetVolumeFraction().SetValue(AmbientO2VF);

  //Initialize the compartments to Ambient values
  for (SEGasCompartment* cmpt : m_data.GetCompartments().GetAnesthesiaMachineLeafCompartments()) {
    if (cmpt->HasVolume()) {
      cmpt->GetSubstanceQuantity(*m_O2)->GetVolumeFraction().SetValue(AmbientO2VF);
      cmpt->GetSubstanceQuantity(*m_CO2)->GetVolumeFraction().SetValue(AmbientCO2VF);
      cmpt->GetSubstanceQuantity(*m_N2)->GetVolumeFraction().SetValue(AmbientN2VF);
      cmpt->Balance(BalanceGasBy::VolumeFraction);
    }
  }

  for (SEGasCompartment* cmpt : m_data.GetCompartments().GetInhalerLeafCompartments()) {
    if (cmpt->HasVolume()) {
      cmpt->GetSubstanceQuantity(m_data.GetSubstances().GetO2())->GetVolumeFraction().SetValue(AmbientO2VF);
      cmpt->GetSubstanceQuantity(m_data.GetSubstances().GetCO2())->GetVolumeFraction().SetValue(AmbientCO2VF);
      cmpt->GetSubstanceQuantity(m_data.GetSubstances().GetN2())->GetVolumeFraction().SetValue(AmbientN2VF);
      cmpt->Balance(BalanceGasBy::VolumeFraction);
    }
  }
}

void BioGearsSubstances::InitializeLiquidCompartmentGases()
{
  BioGearsCompartments& cmpts = m_data.GetCompartments();

  SEScalarFraction hematocrit;
  hematocrit.SetValue(m_data.GetPatient().GetSex() == CDM::enumSex::Male ? 0.45 : 0.40);
  SELiquidCompartment* VenaCava = m_data.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::VenaCava);
  double volume = VenaCava->GetVolume().GetValue(VolumeUnit::mL);
  double Hb_total_g_Per_dL = hematocrit.GetValue() * 34.0;
  double Hb_total_mM = 2.33;

  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Aorta), Hb_total_mM, 0.96652, 0.141241, 0.114398, 1.34029, 24.2719, 7.35791);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Myocardium), Hb_total_mM, 0.80698, 0.074202, 0.134444, 1.57752, 25.4328, 7.30742);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::LeftHeart), Hb_total_mM, 0.966596, 0.141271, 0.114418, 1.33823, 24.261, 7.35838);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RightHeart), Hb_total_mM, 0.760915, 0.0686331, 0.136941, 1.65773, 25.7831, 7.29182);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::VenaCava), Hb_total_mM, 0.760899, 0.068628, 0.136938, 1.65758, 25.7824, 7.29185);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::LeftPulmonaryArteries), Hb_total_mM, 0.761068, 0.0686343, 0.136968, 1.65669, 25.7787, 7.29202);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::LeftPulmonaryCapillaries), Hb_total_mM, 0.972707, 0.151572, 0.114621, 1.34155, 24.8662, 7.368);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::LeftPulmonaryVeins), Hb_total_mM, 0.967472, 0.14146, 0.114261, 1.3343, 24.8082, 7.36934);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RightPulmonaryArteries), Hb_total_mM, 0.76106, 0.0686333, 0.13696, 1.65669, 25.7787, 7.29202);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RightPulmonaryCapillaries), Hb_total_mM, 0.971189, 0.150945, 0.114715, 1.34604, 23.7725, 7.34702);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RightPulmonaryVeins), Hb_total_mM, 0.966192, 0.141536, 0.114727, 1.33667, 23.7413, 7.34948);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RenalArtery), Hb_total_mM, 0.918642, 0.100813, 0.125117, 1.37348, 24.4459, 7.35038);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::AfferentArteriole), Hb_total_mM, 0.892735, 0.0905261, 0.130105, 1.39065, 24.5345, 7.34656);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::GlomerularCapillaries), Hb_total_mM, 0.86897, 0.0846112, 0.134476, 1.43764, 24.6164, 7.33357);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::EfferentArteriole), Hb_total_mM, 0.846224, 0.0790913, 0.136417, 1.45009, 24.8402, 7.33376);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::PeritubularCapillaries), Hb_total_mM, 0.82441, 0.0739954, 0.13999, 1.43292, 24.9495, 7.34084);

  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RenalVein), Hb_total_mM, 0.790757, 0.0691986, 0.147508, 1.46989, 24.9704, 7.33014);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Bone), Hb_total_mM, 0.806748, 0.0752983, 0.130419, 1.62973, 25.65, 7.29697);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Brain), Hb_total_mM, 0.650765, 0.0625731, 0.134891, 2.05578, 27.3382, 7.22379);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Fat), Hb_total_mM, 0.806963, 0.0756806, 0.130218, 1.64501, 25.7003, 7.29377);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Gut), Hb_total_mM, 0.807787, 0.0742538, 0.123887, 1.57641, 25.4981, 7.30884);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Liver), Hb_total_mM, 0.679376, 0.0605613, 0.142054, 1.75648, 26.3523, 7.27618);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Skin), Hb_total_mM, 0.806004, 0.0746394, 0.13355, 1.60525, 25.5431, 7.30173);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Muscle), Hb_total_mM, 0.824034, 0.0787811, 0.12534, 1.63263, 25.5187, 7.29397);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Arms), Hb_total_mM, 0.967025, 0.142751, 0.114319, 1.3559, 24.3638, 7.35452);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Legs), Hb_total_mM, 0.967028, 0.14276, 0.114318, 1.35601, 24.3645, 7.3545);

  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Aorta), Hb_total_mM, 0.969677, 0.132226, 0.114398, 1.21007, 24.1251, 7.39966);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Myocardium), Hb_total_mM, 0.769793, 0.0616152, 0.134444, 1.42943, 25.303, 7.34801);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::LeftHeart), Hb_total_mM, 0.969777, 0.132276, 0.114418, 1.20751, 24.1096, 7.4003);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RightHeart), Hb_total_mM, 0.72721, 0.0583052, 0.136941, 1.52172, 25.746, 7.32838);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::VenaCava), Hb_total_mM, 0.727209, 0.0583035, 0.136938, 1.52162, 25.7454, 7.32839);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::LeftPulmonaryArteries), Hb_total_mM, 0.727416, 0.0583138, 0.136968, 1.52094, 25.7426, 7.32854);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::LeftPulmonaryCapillaries), Hb_total_mM, 0.978565, 0.149565, 0.114621, 1.20819, 24.7057, 7.41066);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::LeftPulmonaryVeins), Hb_total_mM, 0.970427, 0.132223, 0.114261, 1.20463, 24.6632, 7.4112);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RightPulmonaryArteries), Hb_total_mM, 0.727403, 0.0583126, 0.13696, 1.52096, 25.7426, 7.32854);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RightPulmonaryCapillaries), Hb_total_mM, 0.977346, 0.148843, 0.114715, 1.21036, 23.5924, 7.38986);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RightPulmonaryVeins), Hb_total_mM, 0.969356, 0.132482, 0.114727, 1.20678, 23.5918, 7.39113);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RenalArtery), Hb_total_mM, 0.909501, 0.08681, 0.125117, 1.2331, 24.2615, 7.39392);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::AfferentArteriole), Hb_total_mM, 0.877789, 0.0770219, 0.130105, 1.24712, 24.3435, 7.39047);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::GlomerularCapillaries), Hb_total_mM, 0.848224, 0.0712855, 0.134476, 1.28688, 24.4103, 7.37804);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::EfferentArteriole), Hb_total_mM, 0.820122, 0.0661737, 0.136417, 1.29631, 24.6239, 7.37865);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::PeritubularCapillaries), Hb_total_mM, 0.793333, 0.0615554, 0.13999, 1.27901, 24.7409, 7.38654);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::BowmansCapsules), 0.0, 0.0, 0.0773075, 0.0, 1.07857, 0.0, 7.47375);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Tubules), 0.0, 0.0, 0.0773075, 0.0, 1.07857, 0.0, 7.47375);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::RenalVein), Hb_total_mM, 0.752183, 0.0571404, 0.147508, 1.31017, 24.7515, 7.37627);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Bone), Hb_total_mM, 0.773494, 0.0631747, 0.130419, 1.48209, 25.5038, 7.33573);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Brain), Hb_total_mM, 0.578376, 0.0503601, 0.134891, 1.86751, 27.2292, 7.26377);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Fat), Hb_total_mM, 0.771358, 0.0632703, 0.130218, 1.49941, 25.5349, 7.33121);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Gut), Hb_total_mM, 0.799015, 0.0665403, 0.123887, 1.4718, 25.5477, 7.3395);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Liver), Hb_total_mM, 0.665294, 0.054978, 0.142054, 1.69613, 26.6648, 7.29648);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Skin), Hb_total_mM, 0.768325, 0.0619879, 0.13355, 1.45632, 25.3868, 7.34135);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Muscle), Hb_total_mM, 0.79258, 0.0657546, 0.12534, 1.47388, 25.3909, 7.33622);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Arms), Hb_total_mM, 0.969757, 0.132484, 0.114319, 1.21424, 24.1862, 7.39926);
  InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::Legs), Hb_total_mM, 0.969756, 0.132485, 0.114318, 1.21427, 24.1867, 7.39926);

  InitializeBloodGases(*cmpts.GetTissueCompartment(BGE::TissueCompartment::Bone), *cmpts.GetLiquidCompartment(BGE::VascularCompartment::Bone));
  InitializeBloodGases(*cmpts.GetTissueCompartment(BGE::TissueCompartment::Brain), *cmpts.GetLiquidCompartment(BGE::VascularCompartment::Brain));
  InitializeBloodGases(*cmpts.GetTissueCompartment(BGE::TissueCompartment::Fat), *cmpts.GetLiquidCompartment(BGE::VascularCompartment::Fat));
  InitializeBloodGases(*cmpts.GetTissueCompartment(BGE::TissueCompartment::Gut), *cmpts.GetLiquidCompartment(BGE::VascularCompartment::Gut));
  InitializeBloodGases(*cmpts.GetTissueCompartment(BGE::TissueCompartment::Kidneys), *cmpts.GetLiquidCompartment(BGE::VascularCompartment::Kidneys));
  InitializeBloodGases(*cmpts.GetTissueCompartment(BGE::TissueCompartment::Liver), *cmpts.GetLiquidCompartment(BGE::VascularCompartment::Liver));
  InitializeBloodGases(*cmpts.GetTissueCompartment(BGE::TissueCompartment::Lungs), *cmpts.GetLiquidCompartment(BGE::VascularCompartment::Lungs));
  InitializeBloodGases(*cmpts.GetTissueCompartment(BGE::TissueCompartment::Muscle), *cmpts.GetLiquidCompartment(BGE::VascularCompartment::Muscle));
  InitializeBloodGases(*cmpts.GetTissueCompartment(BGE::TissueCompartment::Myocardium), *cmpts.GetLiquidCompartment(BGE::VascularCompartment::Myocardium));
  InitializeBloodGases(*cmpts.GetTissueCompartment(BGE::TissueCompartment::Skin), *cmpts.GetLiquidCompartment(BGE::VascularCompartment::Skin));

  SEScalarMassPerVolume concentration;
  concentration.SetValue(0.146448, MassPerVolumeUnit::g_Per_dL);
  SetSubstanceConcentration(*m_HCO3, cmpts.GetUrineLeafCompartments(), concentration);
}
void BioGearsSubstances::InitializeBloodGases(SELiquidCompartment& cmpt, double Hb_total_mM, double O2_sat, double O2_mmol_Per_L, double CO2_sat, double CO2_mmol_Per_L, double HCO3_mmol_Per_L, double pH, bool distribute)
{
  // N2 is inert
  SEGasCompartment* Ambient = m_data.GetCompartments().GetGasCompartment(BGE::EnvironmentCompartment::Ambient);
  double N2_mmHg = Ambient->GetPressure(PressureUnit::mmHg) * Ambient->GetSubstanceQuantity(*m_N2)->GetVolumeFraction().GetValue();
  SELiquidSubstanceQuantity* N2 = cmpt.GetSubstanceQuantity(*m_N2);
  N2->GetPartialPressure().SetValue(N2_mmHg, PressureUnit::mmHg);
  N2->Balance(BalanceLiquidBy::PartialPressure);

  SELiquidSubstanceQuantity* O2 = cmpt.GetSubstanceQuantity(*m_O2);
  SELiquidSubstanceQuantity* CO2 = cmpt.GetSubstanceQuantity(*m_CO2);
  SELiquidSubstanceQuantity* Hb = cmpt.GetSubstanceQuantity(*m_Hb);
  SELiquidSubstanceQuantity* HbO2 = cmpt.GetSubstanceQuantity(*m_HbO2);
  SELiquidSubstanceQuantity* HbCO2 = cmpt.GetSubstanceQuantity(*m_HbCO2);
  SELiquidSubstanceQuantity* HCO3 = cmpt.GetSubstanceQuantity(*m_HCO3);

  Hb->GetMolarity().SetValue(Hb_total_mM, AmountPerVolumeUnit::mmol_Per_L); //Interpretting Hb_total as all hemoglobin, regardless of state
  Hb->Balance(BalanceLiquidBy::Molarity);
  HbO2->GetMolarity().SetValue(4.0 * Hb_total_mM * O2_sat, AmountPerVolumeUnit::mmol_Per_L); //Interpretting HbO2 as concentration of bound O2 (not of Hb with bound O2)
  HbO2->Balance(BalanceLiquidBy::Molarity);
  HbCO2->GetMolarity().SetValue(4.0 * Hb_total_mM * CO2_sat, AmountPerVolumeUnit::mmol_Per_L); //Interpretting HbCO2 as concentration of bound CO2 (not of Hb with CO2 bound)
  HbCO2->Balance(BalanceLiquidBy::Molarity);
  CO2->GetMolarity().SetValue(CO2_mmol_Per_L, AmountPerVolumeUnit::mmol_Per_L);
  CO2->Balance(BalanceLiquidBy::Molarity);
  CO2->GetSaturation().SetValue(CO2_sat);
  HCO3->GetMolarity().SetValue(HCO3_mmol_Per_L, AmountPerVolumeUnit::mmol_Per_L);
  HCO3->Balance(BalanceLiquidBy::Molarity);
  O2->GetMolarity().SetValue(O2_mmol_Per_L, AmountPerVolumeUnit::mmol_Per_L);
  O2->Balance(BalanceLiquidBy::Molarity);
  O2->GetSaturation().SetValue(O2_sat);

  cmpt.GetPH().SetValue(pH);
  cmpt.GetStrongIonDifferenceBaseline().SetValue(42.0, AmountPerVolumeUnit::mmol_Per_L);    //Sat solver updates this for each compartment after initial stabilization

  if (distribute) {
    m_data.GetSaturationCalculator().CalculateSaturation(cmpt);
  }
}
void BioGearsSubstances::InitializeBloodGases(SETissueCompartment& tissue, SELiquidCompartment& vascular)
{
  SELiquidCompartment& extracellular = m_data.GetCompartments().GetExtracellularFluid(tissue);
  extracellular.GetSubstanceQuantity(*m_N2)->GetMolarity().Set(vascular.GetSubstanceQuantity(*m_N2)->GetMolarity());
  extracellular.GetSubstanceQuantity(*m_O2)->GetMolarity().Set(vascular.GetSubstanceQuantity(*m_O2)->GetMolarity());
  extracellular.GetSubstanceQuantity(*m_CO2)->GetMolarity().Set(vascular.GetSubstanceQuantity(*m_CO2)->GetMolarity());
  extracellular.Balance(BalanceLiquidBy::Molarity);

  SELiquidCompartment& intracellular = m_data.GetCompartments().GetIntracellularFluid(tissue);
  intracellular.GetSubstanceQuantity(*m_N2)->GetMolarity().Set(vascular.GetSubstanceQuantity(*m_N2)->GetMolarity());
  intracellular.GetSubstanceQuantity(*m_O2)->GetMolarity().Set(vascular.GetSubstanceQuantity(*m_O2)->GetMolarity());
  intracellular.GetSubstanceQuantity(*m_CO2)->GetMolarity().Set(vascular.GetSubstanceQuantity(*m_CO2)->GetMolarity());
  intracellular.Balance(BalanceLiquidBy::Molarity);
}

void BioGearsSubstances::WriteBloodGases()
{
  std::stringstream ss;
  for (SELiquidCompartment* cmpt : m_data.GetCompartments().GetVascularLeafCompartments()) {
    if (cmpt->HasVolume()) {
      SELiquidSubstanceQuantity* O2 = cmpt->GetSubstanceQuantity(*m_O2);
      SELiquidSubstanceQuantity* CO2 = cmpt->GetSubstanceQuantity(*m_CO2);
      SELiquidSubstanceQuantity* Hb = cmpt->GetSubstanceQuantity(*m_Hb);
      SELiquidSubstanceQuantity* HbO2 = cmpt->GetSubstanceQuantity(*m_HbO2);
      SELiquidSubstanceQuantity* HbCO2 = cmpt->GetSubstanceQuantity(*m_HbCO2);
      SELiquidSubstanceQuantity* HCO3 = cmpt->GetSubstanceQuantity(*m_HCO3);

      ss << "InitializeBloodGases(*cmpts.GetLiquidCompartment(BGE::VascularCompartment::" << cmpt->GetName() << "), Hb_total_mM, " << O2->GetSaturation() << ", " << O2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) << ", " << CO2->GetSaturation() << ", " << CO2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) << ", " << HCO3->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) << ", " << cmpt->GetPH().GetValue() << ");";
      Info(ss);
    }
  }
}
void BioGearsSubstances::WritePulmonaryGases()
{
  std::stringstream ss;
  std::vector<SEGasCompartment*> cmpts;
  cmpts.push_back(m_data.GetCompartments().GetGasCompartment(BGE::PulmonaryCompartment::Trachea));
  cmpts.push_back(m_data.GetCompartments().GetGasCompartment(BGE::PulmonaryCompartment::Bronchi));
  cmpts.push_back(m_data.GetCompartments().GetGasCompartment(BGE::PulmonaryCompartment::Alveoli));

  for (SEGasCompartment* cmpt : cmpts) {
    if (cmpt->HasVolume()) {
      SEGasSubstanceQuantity* O2 = cmpt->GetSubstanceQuantity(*m_O2);
      SEGasSubstanceQuantity* CO2 = cmpt->GetSubstanceQuantity(*m_CO2);
      SEGasSubstanceQuantity* N2 = cmpt->GetSubstanceQuantity(*m_N2);
      ss << cmpt->GetName() << "->GetSubstanceQuantity(*m_CO2)->GetVolumeFraction().SetValue(" << CO2->GetVolumeFraction() << ");" << std::endl;
      ss << cmpt->GetName() << "->GetSubstanceQuantity(*m_O2)->GetVolumeFraction().SetValue(" << O2->GetVolumeFraction() << ");" << std::endl;
      ss << cmpt->GetName() << "->GetSubstanceQuantity(*m_N2)->GetVolumeFraction().SetValue(" << N2->GetVolumeFraction() << ");" << std::endl;
      Info(ss);
    }
  }
}

void BioGearsSubstances::InitializeLiquidCompartmentNonGases()
{
  const std::vector<SELiquidCompartment*>& urine = m_data.GetCompartments().GetUrineLeafCompartments();
  const std::vector<SELiquidCompartment*>& vascular = m_data.GetCompartments().GetVascularLeafCompartments();
  const std::vector<SETissueCompartment*>& tissue = m_data.GetCompartments().GetTissueLeafCompartments();

  SELiquidSubstanceQuantity* subQ;
  // Initialize Substances throughout the body
  SEScalarMassPerVolume concentration;
  SEScalarAmountPerVolume molarity1;
  SEScalarAmountPerVolume molarity2;

  // Urine cmpts have to be explicitly set to values
  // Upper Tract
  SELiquidCompartment* BowmansCapsules = m_data.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::BowmansCapsules);
  SELiquidCompartment* Tubules = m_data.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::Tubules);
  SELiquidCompartment* Ureter = m_data.GetCompartments().GetLiquidCompartment(BGE::UrineCompartment::Ureter);

  // Lower Tract
  // Note I don't modify the urethra, it's just a flow pipe, with no volume, hence, no substance quantities (NaN)
  //SELiquidCompartment* urethra = m_data.GetCompartments().GetUrineCompartment("Urethra");
  SELiquidCompartment* bladder = m_data.GetCompartments().GetLiquidCompartment(BGE::UrineCompartment::Bladder);
  SELiquidCompartment* lymph = m_data.GetCompartments().GetLiquidCompartment(BGE::LymphCompartment::Lymph);
  //Right now the lymph is not used, but code is in place and commented out in case we revisit
  SETissueCompartment* brain = m_data.GetCompartments().GetTissueCompartment(BGE::TissueCompartment::Brain);

  // ALBUMIN //
  concentration.SetValue(4.5, MassPerVolumeUnit::g_Per_dL);
  SetSubstanceConcentration(*m_albumin, vascular, concentration);
  // Set Urine
  concentration.SetValue(0.155, MassPerVolumeUnit::mg_Per_dL);
  subQ = BowmansCapsules->GetSubstanceQuantity(*m_albumin);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  Tubules->GetSubstanceQuantity(*m_albumin)->SetToZero();
  Ureter->GetSubstanceQuantity(*m_albumin)->SetToZero();
  bladder->GetSubstanceQuantity(*m_albumin)->SetToZero();
  // Tissue
  molarity1.SetValue(20.0 / (m_albumin->GetMolarMass(MassPerAmountUnit::g_Per_mmol)), AmountPerVolumeUnit::mmol_Per_L);
  molarity2.SetValue(0.0, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_albumin, tissue, molarity1, molarity2);
  //Lymph - should be same as extracellular
  concentration.SetValue(2.0, MassPerVolumeUnit::g_Per_dL);
  lymph->GetSubstanceQuantity(*m_albumin)->GetConcentration().Set(concentration);
  lymph->GetSubstanceQuantity(*m_albumin)->Balance(BalanceLiquidBy::Concentration);

  // AMINOACIDS //
  concentration.SetValue(50.0, MassPerVolumeUnit::mg_Per_dL);
  SetSubstanceConcentration(*m_aminoAcids, vascular, concentration);
  // Tissue
  molarity1.SetValue(concentration.GetValue(MassPerVolumeUnit::g_Per_L) / m_aminoAcids->GetMolarMass(MassPerAmountUnit::g_Per_mol), AmountPerVolumeUnit::mol_Per_L);
  SetSubstanceMolarity(*m_aminoAcids, tissue, molarity1, molarity1);
  //Lymph
  lymph->GetSubstanceQuantity(*m_aminoAcids)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_aminoAcids)->Balance(BalanceLiquidBy::Molarity);

  // BICARBONATE IS SET IN GASES SECTION //

  // CALCIUM //
  concentration.SetValue(98.1, MassPerVolumeUnit::mg_Per_L);
  SetSubstanceConcentration(*m_calcium, vascular, concentration);
  // Tissue
  molarity1.SetValue(concentration.GetValue(MassPerVolumeUnit::g_Per_L) / m_calcium->GetMolarMass(MassPerAmountUnit::g_Per_mol), AmountPerVolumeUnit::mol_Per_L);
  molarity2.SetValue(0.0001, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_calcium, tissue, molarity1, molarity2);
  //Lymph
  lymph->GetSubstanceQuantity(*m_calcium)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_calcium)->Balance(BalanceLiquidBy::Molarity);
  // Set Urine
  concentration.SetValue(98.1, MassPerVolumeUnit::mg_Per_L);
  subQ = BowmansCapsules->GetSubstanceQuantity(*m_calcium);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = Tubules->GetSubstanceQuantity(*m_calcium);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = Ureter->GetSubstanceQuantity(*m_calcium);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = bladder->GetSubstanceQuantity(*m_calcium);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);

  // CHLORIDE //
  concentration.SetValue(0.362, MassPerVolumeUnit::g_Per_dL);
  SetSubstanceConcentration(*m_chloride, vascular, concentration);
  SetSubstanceConcentration(*m_chloride, urine, concentration);
  // Tissue
  molarity1.SetValue(102.0, AmountPerVolumeUnit::mmol_Per_L);
  molarity2.SetValue(4.5, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_chloride, tissue, molarity1, molarity2);
  //Lymph
  lymph->GetSubstanceQuantity(*m_chloride)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_chloride)->Balance(BalanceLiquidBy::Molarity);
  // CREATININE //
  concentration.SetValue(1.2, MassPerVolumeUnit::mg_Per_dL);
  SetSubstanceConcentration(*m_creatinine, vascular, concentration);
  // Set Urine
  concentration.SetValue(0.006, MassPerVolumeUnit::mg_Per_dL);
  subQ = BowmansCapsules->GetSubstanceQuantity(*m_creatinine);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  concentration.SetValue(1.25, MassPerVolumeUnit::g_Per_L);
  subQ = Tubules->GetSubstanceQuantity(*m_creatinine);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = Ureter->GetSubstanceQuantity(*m_creatinine);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = bladder->GetSubstanceQuantity(*m_creatinine);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  // Tissue
  //molarity1.SetValue(0.044, AmountPerVolumeUnit::mmol_Per_L);
  molarity1.SetValue(0.106, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_creatinine, tissue, molarity1);
  //Lymph
  lymph->GetSubstanceQuantity(*m_creatinine)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_creatinine)->Balance(BalanceLiquidBy::Molarity);

  // EPINEPHRINE //
  // Initializing to artificial plasma concentration because BG plasma is not totally correct
  //double hematocritGuess = 0.45;
  concentration.SetValue(0.034, MassPerVolumeUnit::ug_Per_L);
  SetSubstanceConcentration(*m_epi, vascular, concentration);
  // Tissue
  molarity1.SetValue(1.8558e-7, AmountPerVolumeUnit::mmol_Per_L); //epinephrine: 183.2044 g/mol
  //molarity1.SetValue(0, AmountPerVolumeUnit::mmol_Per_L); //epinephrine: 183.2044 g/mol
  SetSubstanceMolarity(*m_epi, tissue, molarity1);
  //Lymph
  lymph->GetSubstanceQuantity(*m_epi)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_epi)->Balance(BalanceLiquidBy::Molarity);

  // GLUCAGON //
  concentration.SetValue(0.079, MassPerVolumeUnit::ug_Per_L); //We want 70 pg/mL, but it dips in stabilization, so set it higher
  SetSubstanceConcentration(*m_glucagon, vascular, concentration);
  // Tissue
  molarity1.SetValue(0.0, AmountPerVolumeUnit::mmol_Per_L);
  molarity2.SetValue(0.0, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_glucagon, tissue, molarity1, molarity2);
  //Lymph
  //Don't set since none in tissue and it's too large to diffuse from bloodstream

  // GLUCOSE //
  concentration.SetValue(90, MassPerVolumeUnit::mg_Per_dL);
  SetSubstanceConcentration(*m_glucose, vascular, concentration);
  // Only Bowmans has it
  subQ = BowmansCapsules->GetSubstanceQuantity(*m_glucose);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  Tubules->GetSubstanceQuantity(*m_glucose)->SetToZero();
  Ureter->GetSubstanceQuantity(*m_glucose)->SetToZero();
  bladder->GetSubstanceQuantity(*m_glucose)->SetToZero();
  // Tissue
  molarity1.SetValue(concentration.GetValue(MassPerVolumeUnit::g_Per_L) / m_glucose->GetMolarMass(MassPerAmountUnit::g_Per_mol), AmountPerVolumeUnit::mol_Per_L);
  SetSubstanceMolarity(*m_glucose, tissue, molarity1, molarity1);
  lymph->GetSubstanceQuantity(*m_glucose)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_glucose)->Balance(BalanceLiquidBy::Molarity);

  // INSULIN //
  concentration.SetValue(0.85, MassPerVolumeUnit::ug_Per_L); //118.1 pmol/L is desired (.6859 ug/L), was .85 because of stabilization dip, but it seems okay now
  SetSubstanceConcentration(*m_insulin, vascular, concentration);
  // None in Urine
  BowmansCapsules->GetSubstanceQuantity(*m_insulin)->SetToZero();
  Tubules->GetSubstanceQuantity(*m_insulin)->SetToZero();
  Ureter->GetSubstanceQuantity(*m_insulin)->SetToZero();
  bladder->GetSubstanceQuantity(*m_insulin)->SetToZero();
  // Tissue
  molarity1.SetValue(0.0, AmountPerVolumeUnit::mmol_Per_L);
  molarity2.SetValue(0.0, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_insulin, tissue, molarity1, molarity2);
  //Lymph--Don't set since there is none in tissue and it's too large to diffuse from bloodstream

  // KETONES //
  concentration.SetValue(9.19, MassPerVolumeUnit::mg_Per_L);
  SetSubstanceConcentration(*m_ketones, vascular, concentration);
  // Set Urine
  subQ = BowmansCapsules->GetSubstanceQuantity(*m_ketones);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  Tubules->GetSubstanceQuantity(*m_ketones)->GetConcentration().SetValue(0.0, MassPerVolumeUnit::mg_Per_dL);
  Tubules->GetSubstanceQuantity(*m_ketones)->Balance(BalanceLiquidBy::Concentration);
  Ureter->GetSubstanceQuantity(*m_ketones)->GetConcentration().SetValue(0.0, MassPerVolumeUnit::mg_Per_dL);
  Ureter->GetSubstanceQuantity(*m_ketones)->Balance(BalanceLiquidBy::Concentration);
  bladder->GetSubstanceQuantity(*m_ketones)->GetConcentration().SetValue(0.0, MassPerVolumeUnit::mg_Per_dL);
  bladder->GetSubstanceQuantity(*m_ketones)->Balance(BalanceLiquidBy::Concentration);
  // Tissue
  molarity1.SetValue(0.09, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_ketones, tissue, molarity1);
  //Lymph
  lymph->GetSubstanceQuantity(*m_ketones)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_ketones)->Balance(BalanceLiquidBy::Molarity);

  // LACTATE //
  concentration.SetValue(142.5, MassPerVolumeUnit::mg_Per_L);
  molarity1.SetValue(concentration.GetValue(MassPerVolumeUnit::g_Per_L) / m_lactate->GetMolarMass(MassPerAmountUnit::g_Per_mol), AmountPerVolumeUnit::mol_Per_L);
  SetSubstanceConcentration(*m_lactate, vascular, concentration);
  //set in tubules zero in urine
  concentration.SetValue(1.5, MassPerVolumeUnit::g_Per_L);
  subQ = Tubules->GetSubstanceQuantity(*m_lactate);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  bladder->GetSubstanceQuantity(*m_lactate)->SetToZero();
  //Clear Lactate out of the Ureter to initialize for Gluconeogenesis
  Ureter->GetSubstanceQuantity(*m_lactate)->SetToZero();
  // Tissue
  SetSubstanceMolarity(*m_lactate, tissue, molarity1, molarity1);
  //Lymph
  lymph->GetSubstanceQuantity(*m_lactate)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_lactate)->Balance(BalanceLiquidBy::Molarity);

  // POTASSIUM //
  concentration.SetValue(175.5, MassPerVolumeUnit::mg_Per_L);
  SetSubstanceConcentration(*m_potassium, vascular, concentration);
  // Set Urine
  concentration.SetValue(175.5, MassPerVolumeUnit::mg_Per_L);
  subQ = BowmansCapsules->GetSubstanceQuantity(*m_potassium);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = Tubules->GetSubstanceQuantity(*m_potassium);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = Ureter->GetSubstanceQuantity(*m_potassium);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = bladder->GetSubstanceQuantity(*m_potassium);
  subQ->GetConcentration().Set(concentration);
  // Tissue
  molarity1.SetValue(4.5, AmountPerVolumeUnit::mmol_Per_L);
  molarity2.SetValue(120, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_potassium, tissue, molarity1, molarity2);
  //Lymph
  lymph->GetSubstanceQuantity(*m_potassium)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_potassium)->Balance(BalanceLiquidBy::Molarity);

  // SODIUM //
  concentration.SetValue(0.335, MassPerVolumeUnit::g_Per_dL);
  SetSubstanceConcentration(*m_sodium, vascular, concentration);
  // Set Urine
  subQ = BowmansCapsules->GetSubstanceQuantity(*m_sodium);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  concentration.SetValue(0.375, MassPerVolumeUnit::g_Per_dL);
  subQ = Tubules->GetSubstanceQuantity(*m_sodium);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = Ureter->GetSubstanceQuantity(*m_sodium);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = bladder->GetSubstanceQuantity(*m_sodium);
  subQ->GetConcentration().Set(concentration);
  // Tissue
  molarity1.SetValue(145, AmountPerVolumeUnit::mmol_Per_L);
  molarity2.SetValue(15, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_sodium, tissue, molarity1, molarity2);
  //Lymph
  lymph->GetSubstanceQuantity(*m_sodium)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_sodium)->Balance(BalanceLiquidBy::Molarity);

  // TRIACYLGLYCEROL //
  concentration.SetValue(75.0, MassPerVolumeUnit::mg_Per_dL);
  SetSubstanceConcentration(*m_triacylglycerol, vascular, concentration);
  // None in Urine
  BowmansCapsules->GetSubstanceQuantity(*m_triacylglycerol)->SetToZero();
  Tubules->GetSubstanceQuantity(*m_triacylglycerol)->SetToZero();
  Ureter->GetSubstanceQuantity(*m_triacylglycerol)->SetToZero();
  bladder->GetSubstanceQuantity(*m_triacylglycerol)->SetToZero();
  // Tissue
  molarity1.SetValue(concentration.GetValue(MassPerVolumeUnit::g_Per_L) / m_triacylglycerol->GetMolarMass(MassPerAmountUnit::g_Per_mol), AmountPerVolumeUnit::mol_Per_L);
  SetSubstanceMolarity(*m_triacylglycerol, tissue, molarity1, molarity1);
  //Lymph
  lymph->GetSubstanceQuantity(*m_triacylglycerol)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_triacylglycerol)->Balance(BalanceLiquidBy::Molarity);
  // TAG can't cross blood-brain barrier, so no TAG there
  molarity1.SetValue(0, AmountPerVolumeUnit::mmol_Per_L);
  m_data.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::BrainExtracellular)->GetSubstanceQuantity(*m_triacylglycerol)->GetMolarity().Set(molarity1);
  m_data.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::BrainExtracellular)->GetSubstanceQuantity(*m_triacylglycerol)->Balance(BalanceLiquidBy::Molarity);
  m_data.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::BrainIntracellular)->GetSubstanceQuantity(*m_triacylglycerol)->GetMolarity().Set(molarity1);
  m_data.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::BrainIntracellular)->GetSubstanceQuantity(*m_triacylglycerol)->Balance(BalanceLiquidBy::Molarity);

  // UREA //
  concentration.SetValue(270.0, MassPerVolumeUnit::mg_Per_L);
  SetSubstanceConcentration(*m_urea, vascular, concentration);
  // Set Urine
  concentration.SetValue(0.2, MassPerVolumeUnit::mg_Per_dL);
  subQ = BowmansCapsules->GetSubstanceQuantity(*m_urea);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  concentration.SetValue(20.0, MassPerVolumeUnit::g_Per_L);
  subQ = Tubules->GetSubstanceQuantity(*m_urea);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = Ureter->GetSubstanceQuantity(*m_urea);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  subQ = bladder->GetSubstanceQuantity(*m_urea);
  subQ->GetConcentration().Set(concentration);
  subQ->Balance(BalanceLiquidBy::Concentration);
  // Tissue
  molarity1.SetValue(4.8, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_urea, tissue, molarity1);
  //Lymph
  lymph->GetSubstanceQuantity(*m_urea)->GetMolarity().Set(molarity1);
  lymph->GetSubstanceQuantity(*m_urea)->Balance(BalanceLiquidBy::Molarity);
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Sets the status of blood concentrations to appropriate starved values
///
/// \details
/// The blood concentrations of glucose and ketones are set to match literature values. Insulin and
/// glucagon are not set because they react to set glucose quickly. Other metabolites are not set,
/// but they could be in the future if appropriate validation data is found.
//--------------------------------------------------------------------------------------------------
void BioGearsSubstances::SetLiquidCompartmentNonGasesForStarvation(double time_h)
{
  //This function copies InitializeLiquidCompartmentNonGases() in form and is called
  //from Tissue::SetStarvationState() to configure blood and tissue concentrations during
  //the Starvation condition (urine compartments are not currently considered)

  const std::vector<SELiquidCompartment*>& vascular = m_data.GetCompartments().GetVascularLeafCompartments();
  const std::vector<SETissueCompartment*>& tissue = m_data.GetCompartments().GetTissueLeafCompartments();

  // Initialize Substances throughout the body
  SEScalarMassPerVolume concentration;
  SEScalarAmountPerVolume molarity1;
  SEScalarAmountPerVolume molarity2;

  SETissueCompartment* brain = m_data.GetCompartments().GetTissueCompartment(BGE::TissueCompartment::Brain);

  // AMINOACIDS //
  //Probably sholdn't be messed with; see elia1984mineral that says total protein stays ~constant
  /*
  concentration.SetValue(50.0, MassPerVolumeUnit::mg_Per_dL);
  SetSubstanceConcentration(*m_aminoAcids, vascular, concentration);
  // Tissue
  molarity1.SetValue(concentration.GetValue(MassPerVolumeUnit::g_Per_L) / m_aminoAcids->GetMolarMass(MassPerAmountUnit::g_Per_mol), AmountPerVolumeUnit::mol_Per_L);
  SetSubstanceMolarity(*m_aminoAcids, tissue, molarity1, molarity1);
  */

  // GLUCAGON //
  //Not modified since it will react to glucose quickly
  /*
  concentration.SetValue(0.079, MassPerVolumeUnit::ug_Per_L);  //We want 70 pg/mL, but it dips in stabilization, so set it higher
  SetSubstanceConcentration(*m_glucagon, vascular, concentration);
  // Tissue
  molarity1.SetValue(0.0, AmountPerVolumeUnit::mmol_Per_L);
  molarity2.SetValue(0.0, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_glucagon, tissue, molarity1, molarity2);
  */

  // GLUCOSE //
  // \cite garber1974hepatic and \cite owen1971human show glucose concentrations from 0-3 days of fasting and then at 24 days
  // https://www.wolframalpha.com/input/?i=y%3D84.3105+-+.39147x+-+.00000434x%5E2+from+0+%3C+x+%3C+80
  // It's very nearly linear up to 3 days, where it stays hovering around 61 mg/dL
  double conc = 0;
  if (time_h < 72)
    conc = 84.3105 - .39147 * time_h - .00000434 * time_h * time_h;
  else
    conc = 61.25;
  concentration.SetValue(conc, MassPerVolumeUnit::mg_Per_dL);
  SetSubstanceConcentration(*m_glucose, vascular, concentration);

  // Tissue
  molarity1.SetValue(concentration.GetValue(MassPerVolumeUnit::g_Per_L) / m_glucose->GetMolarMass(MassPerAmountUnit::g_Per_mol), AmountPerVolumeUnit::mol_Per_L);
  SetSubstanceMolarity(*m_glucose, tissue, molarity1, molarity1);

  // INSULIN //
  //Not modified since it reacts to glucose quickly
  /*
  concentration.SetValue(0.85, MassPerVolumeUnit::ug_Per_L);  //118.1 pmol/L is desired (.6859 ug/L), but it dips during stabilization, so start higher
  SetSubstanceConcentration(*m_insulin, vascular, concentration);

  // Tissue
  molarity1.SetValue(0.0, AmountPerVolumeUnit::mmol_Per_L);
  molarity2.SetValue(0.0, AmountPerVolumeUnit::mmol_Per_L);
  SetSubstanceMolarity(*m_insulin, tissue, molarity1, molarity2);
  */

  // KETONES //
  // \cite garber1974hepatic shows ketone concentrations from 0-3 days, mentioning that the peak is around 3 days
  // https://www.wolframalpha.com/input/?i=y%3D2.705%2B.0276875x%2B.00398698x%5E2+from+0%3Cx%3C80
  // We'll hold constant after 3 days, though according to Garber, it might decrease a bit after that
  conc = 0;
  if (time_h < 72)
    conc = 2.705 + .0276875 * time_h + .00398698 * time_h * time_h;
  else
    conc = 25.52;
  concentration.SetValue(conc, MassPerVolumeUnit::mg_Per_dL);
  SetSubstanceConcentration(*m_ketones, vascular, concentration);

  // Tissue
  molarity1.SetValue(concentration.GetValue(MassPerVolumeUnit::g_Per_L) / m_ketones->GetMolarMass(MassPerAmountUnit::g_Per_mol), AmountPerVolumeUnit::mol_Per_L);
  SetSubstanceMolarity(*m_ketones, tissue, molarity1, molarity1);

  // LACTATE //
  //Modified to match engine state in order to provide adequate substrate for gluconeogenesis

  concentration.SetValue(32.5, MassPerVolumeUnit::mg_Per_dL);
  molarity1.SetValue(concentration.GetValue(MassPerVolumeUnit::g_Per_L) / m_lactate->GetMolarMass(MassPerAmountUnit::g_Per_mol), AmountPerVolumeUnit::mol_Per_L);
  SetSubstanceConcentration(*m_lactate, vascular, concentration);

  // Tissue
  SetSubstanceMolarity(*m_lactate, tissue, molarity1, molarity1);

  // TRIACYLGLYCEROL //
  //Not modified. \cite zauner2000resting shows it not changing much from basal levels, but since we don't model fatty acids, we'll see it rise over time.
  /*
  concentration.SetValue(75.0, MassPerVolumeUnit::mg_Per_dL);
  SetSubstanceConcentration(*m_triacylglycerol, vascular, concentration);

  // Tissue
  molarity1.SetValue(concentration.GetValue(MassPerVolumeUnit::g_Per_L) / m_triacylglycerol->GetMolarMass(MassPerAmountUnit::g_Per_mol), AmountPerVolumeUnit::mol_Per_L);
  SetSubstanceMolarity(*m_triacylglycerol, tissue, molarity1, molarity1);
  // TAG can't cross blood-brain barrier, so no TAG there
  molarity1.SetValue(0, AmountPerVolumeUnit::mmol_Per_L);
  m_data.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::BrainExtracellular)->GetSubstanceQuantity(*m_triacylglycerol)->GetMolarity().Set(molarity1);
  m_data.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::BrainExtracellular)->GetSubstanceQuantity(*m_triacylglycerol)->Balance(BalanceLiquidBy::Molarity);
  m_data.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::BrainIntracellular)->GetSubstanceQuantity(*m_triacylglycerol)->GetMolarity().Set(molarity1);
  m_data.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::BrainIntracellular)->GetSubstanceQuantity(*m_triacylglycerol)->Balance(BalanceLiquidBy::Molarity);
  */

  // IONS //
  //Not modified, but \cite elia1984mineral has good data for Na, K, Ca, and Cl (they don't change much during 4 day starvation)
}

bool BioGearsSubstances::LoadSubstanceDirectory()
{
  if (!SESubstanceManager::LoadSubstanceDirectory())
    return false;

  m_O2 = GetSubstance("Oxygen");
  m_CO = GetSubstance("CarbonMonoxide");
  m_CO2 = GetSubstance("CarbonDioxide");
  m_N2 = GetSubstance("Nitrogen");
  m_Hb = GetSubstance("Hemoglobin");
  m_HbO2 = GetSubstance("Oxyhemoglobin");
  m_HbCO2 = GetSubstance("Carbaminohemoglobin");
  m_HbCO = GetSubstance("Carboxyhemoglobin");
  m_HCO3 = GetSubstance("Bicarbonate");
  m_epi = GetSubstance("Epinephrine");

  if (m_O2 == nullptr)
    Error("Oxygen Definition not found");
  if (m_CO == nullptr)
    Error("CarbonMonoxide Definition not found");
  if (m_CO2 == nullptr)
    Error("CarbonDioxide Definition not found");
  if (m_N2 == nullptr)
    Error("Nitrogen Definition not found");
  if (m_Hb == nullptr)
    Error("Hemoglobin Definition not found");
  if (m_HbO2 == nullptr)
    Error("Oxyhemoglobin Definition not found");
  if (m_HbCO2 == nullptr)
    Error("Carbaminohemoglobin Definition not found");
  if (m_HbCO == nullptr)
    Error("Carboxyhemoglobin Definition not found");
  if (m_HCO3 == nullptr)
    Error("Bicarbonate Definition not found");
  if (m_epi == nullptr)
    Error("Epinephrine Definition not found");

  if (m_O2 == nullptr || m_CO == nullptr || m_CO2 == nullptr || m_N2 == nullptr || m_Hb == nullptr || m_HbO2 == nullptr || m_HbCO2 == nullptr || m_HbCO == nullptr || m_epi == nullptr || m_HCO3 == nullptr)
    return false;

  m_albumin = GetSubstance("Albumin");
  m_aminoAcids = GetSubstance("AminoAcids");
  m_calcium = GetSubstance("Calcium");
  m_chloride = GetSubstance("Chloride");
  m_creatinine = GetSubstance("Creatinine");
  m_globulin = GetSubstance("Globulin");
  m_glucagon = GetSubstance("Glucagon");
  m_glucose = GetSubstance("Glucose");
  m_insulin = GetSubstance("Insulin");
  m_ketones = GetSubstance("Ketones");
  m_lactate = GetSubstance("Lactate");
  m_potassium = GetSubstance("Potassium");
  m_sodium = GetSubstance("Sodium");
  m_triacylglycerol = GetSubstance("Triacylglycerol");
  m_urea = GetSubstance("Urea");

  if (m_albumin == nullptr)
    Error("Albumin Definition not found");
  if (m_aminoAcids == nullptr)
    Error("AminoAcids Definition not found");
  if (m_calcium == nullptr)
    Error("Calcium Definition not found");
  if (m_chloride == nullptr)
    Error("Chloride Definition not found");
  if (m_creatinine == nullptr)
    Error("Creatinine Definition not found");
  if (m_globulin == nullptr)
    Error("Globulin Definition not found");
  if (m_glucagon == nullptr)
    Error("Glucagon Definition not found");
  if (m_glucose == nullptr)
    Error("Glucose Definition not found");
  if (m_insulin == nullptr)
    Error("Insulin Definition not found");
  if (m_ketones == nullptr)
    Error("Ketones Definition not found");
  if (m_lactate == nullptr)
    Error("Lactate Definition not found");
  if (m_potassium == nullptr)
    Error("Potassium Definition not found");
  if (m_sodium == nullptr)
    Error("Sodium Definition not found");
  if (m_triacylglycerol == nullptr)
    Error("Triacylglycerol Definition not found");
  if (m_urea == nullptr)
    Error("Urea Definition not found");
  // These metabolites will be activated in initialization

  // Check that drugs have what we need
  for (SESubstance* sub : m_Substances) {
    if (sub->HasPD()) {
      if (sub->GetPD().GetEC50().IsZero() || sub->GetPD().GetEC50().IsNegative()) {
        std::stringstream ss;
        ss << sub->GetName() << " cannot have EC50 <= 0";
        Fatal(ss);
      }
    }
  }

  return true;
}

void BioGearsSubstances::AddActiveSubstance(SESubstance& substance)
{
  if (IsActive(substance))
    return; // If its already active, don't do anything

  SESubstanceManager::AddActiveSubstance(substance);
  if (substance.GetState() == CDM::enumSubstanceState::Gas)
    m_data.GetCompartments().AddGasCompartmentSubstance(substance);
  m_data.GetCompartments().AddLiquidCompartmentSubstance(substance);

  if (&substance == m_CO) // We need to put HbCO in the system if CO is in the system
  {
    m_isCOActive = true;
    AddActiveSubstance(*m_HbCO);
  }
}

bool BioGearsSubstances::IsActive(const SESubstance& sub) const
{
  if (&sub == m_CO)
    return m_isCOActive;
  return SESubstanceManager::IsActive(sub);
}

/// --------------------------------------------------------------------------------------------------
/// \brief
/// Calculates the substance mass cleared for a node
///
/// \details
/// The volume cleared, the compartment, and the substance are provided to clear the mass of the substance
/// from the node. This generic methodology can be used by other systems to calculate the mass cleared.
//--------------------------------------------------------------------------------------------------
void BioGearsSubstances::CalculateGenericClearance(double volumeCleared_mL, SELiquidCompartment& cmpt, SESubstance& sub, SEScalarMass* cleared)
{
  SELiquidSubstanceQuantity* subQ = cmpt.GetSubstanceQuantity(sub);
  if (subQ == nullptr)
    throw CommonDataModelException(std::string { "No Substance Quantity found for substance " } + sub.GetName());
  //GetMass and Concentration from the compartment
  double mass_ug = subQ->GetMass(MassUnit::ug);
  double concentration_ug_Per_mL = subQ->GetConcentration(MassPerVolumeUnit::ug_Per_mL);

  //Calculate Mass Cleared
  double MassCleared_ug = volumeCleared_mL * concentration_ug_Per_mL;
  //Ensure mass does not become negative
  mass_ug -= MassCleared_ug;
  if (mass_ug < 0) {
    mass_ug = 0;
  }

  MassCleared_ug = subQ->GetMass(MassUnit::ug) - mass_ug;
  subQ->GetMass().SetValue(mass_ug, MassUnit::ug);
  subQ->Balance(BalanceLiquidBy::Mass);

  sub.GetSystemicMassCleared().IncrementValue(MassCleared_ug, MassUnit::ug);
  if (cleared != nullptr)
    cleared->SetValue(MassCleared_ug, MassUnit::ug);
}

/// --------------------------------------------------------------------------------------------------
/// \brief
/// Calculates the substance mass cleared for a tissue compartment based on a volume
///
/// \param VolumeCleared_mL: the volume of fluid cleared of a substance by some process
/// \param tissue: a tissue compartment
/// \param sub: a substance
/// \param cleared: mass cleared
///
/// \details
/// The volume cleared, the compartment, and the substance are provided to clear the mass of the substance
/// from the node. This generic methodology can be used by other systems to calculate the mass cleared.
//--------------------------------------------------------------------------------------------------
void BioGearsSubstances::CalculateGenericClearance(double VolumeCleared_mL, SETissueCompartment& tissue, SESubstance& sub, SEScalarMass* cleared)
{
  SELiquidSubstanceQuantity* subQ = m_data.GetCompartments().GetIntracellularFluid(tissue).GetSubstanceQuantity(sub);
  if (subQ == nullptr)
    throw CommonDataModelException(std::string { "No Substance Quantity found for substance" } + sub.GetName());
  //GetMass and Concentration from the compartment
  double mass_ug = subQ->GetMass(MassUnit::ug);
  double concentration_ug_Per_mL;
  SEScalarMassPerVolume concentration;
  if (sub.HasPK()) {
    GeneralMath::CalculateConcentration(subQ->GetMass(), tissue.GetMatrixVolume(), concentration, m_Logger);
    concentration_ug_Per_mL = concentration.GetValue(MassPerVolumeUnit::ug_Per_mL);
  } else {
    concentration_ug_Per_mL = subQ->GetConcentration(MassPerVolumeUnit::ug_Per_mL);
  }

  //Calculate Mass Cleared
  double MassCleared_ug = VolumeCleared_mL * concentration_ug_Per_mL;
  //Ensure mass does not become negative
  mass_ug -= MassCleared_ug;
  if (mass_ug < 0) {
    mass_ug = 0;
  }

  MassCleared_ug = subQ->GetMass(MassUnit::ug) - mass_ug;
  subQ->GetMass().SetValue(mass_ug, MassUnit::ug);
  subQ->Balance(BalanceLiquidBy::Mass);
  subQ->GetMassCleared().IncrementValue(MassCleared_ug, MassUnit::ug);
  if (cleared != nullptr)
    cleared->SetValue(MassCleared_ug, MassUnit::ug);
}

/// --------------------------------------------------------------------------------------------------
/// \brief
/// Calculates the substance mass excreted for a compartment
///
/// \details
/// The volume cleared, the compartment, and the substance are provided to clear the mass of the substance
/// from the node. This generic methodology can be used by other systems to calculate the mass excreted.
//--------------------------------------------------------------------------------------------------
void BioGearsSubstances::CalculateGenericExcretion(double VascularFlow_mL_Per_s, SETissueCompartment& tissue, SESubstance& sub, double FractionExcreted, double timestep_s, SEScalarMass* excreted)
{
  SELiquidSubstanceQuantity* subQ = m_data.GetCompartments().GetIntracellularFluid(tissue).GetSubstanceQuantity(sub);
  if (subQ == nullptr)
    throw CommonDataModelException(std::string { "No Substance Quantity found for substance" } + sub.GetName());
  double concentration_ug_Per_mL;
  SEScalarMassPerVolume concentration;
  if (sub.HasPK()) {
    GeneralMath::CalculateConcentration(subQ->GetMass(), tissue.GetMatrixVolume(), concentration, m_Logger);
    concentration_ug_Per_mL = concentration.GetValue(MassPerVolumeUnit::ug_Per_mL);
  } else {
    concentration_ug_Per_mL = subQ->GetConcentration(MassPerVolumeUnit::ug_Per_mL);
  }

  double MassExcreted_ug = VascularFlow_mL_Per_s * concentration_ug_Per_mL * timestep_s * 0.5 * FractionExcreted; //0.5  is the tuning parameter to remove the correct percentage.

  double mass_ug = subQ->GetMass().GetValue(MassUnit::ug);
  mass_ug = subQ->GetMass().GetValue(MassUnit::ug) - MassExcreted_ug;
  if (mass_ug < 0)
    mass_ug = 0;

  MassExcreted_ug = subQ->GetMass().GetValue(MassUnit::ug) - mass_ug;
  subQ->GetMass().SetValue(mass_ug, MassUnit::ug);
  subQ->Balance(BalanceLiquidBy::Mass);
  subQ->GetMassExcreted().IncrementValue(MassExcreted_ug, MassUnit::ug);
  if (excreted != nullptr)
    excreted->SetValue(MassExcreted_ug, MassUnit::ug);
}

void BioGearsSubstances::ProbeBloodGases(SELiquidCompartment& cmpt, const std::string& prefix)
{
  SELiquidSubstanceQuantity* O2 = cmpt.GetSubstanceQuantity(*m_O2);
  SELiquidSubstanceQuantity* Hb = cmpt.GetSubstanceQuantity(*m_Hb);
  SELiquidSubstanceQuantity* HbO2 = cmpt.GetSubstanceQuantity(*m_HbO2);
  SELiquidSubstanceQuantity* CO2 = cmpt.GetSubstanceQuantity(*m_CO2);
  SELiquidSubstanceQuantity* HbCO2 = cmpt.GetSubstanceQuantity(*m_HbCO2);
  SELiquidSubstanceQuantity* HCO3 = cmpt.GetSubstanceQuantity(*m_HCO3);

  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_pH", cmpt.GetPH().GetValue());
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_O2_ug", O2->GetMass(MassUnit::ug));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_O2_mmol_Per_L", O2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_O2_mmHg", O2->GetPartialPressure(PressureUnit::mmHg));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_O2_sat", O2->GetSaturation().GetValue());
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_CO2_ug", CO2->GetMass(MassUnit::ug));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_CO2_mmol_Per_L", CO2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_CO2_mmHg", CO2->GetPartialPressure(PressureUnit::mmHg));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_CO2_sat", CO2->GetSaturation().GetValue());
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_Hb_ug", Hb->GetMass(MassUnit::ug));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_Hb_mmol_Per_L", Hb->GetMolarity(AmountPerVolumeUnit::mmol_Per_L));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_HbO2_ug", HbO2->GetMass(MassUnit::ug));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_HbO2_mmol_Per_L", HbO2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_HbCO2_ug", HbCO2->GetMass(MassUnit::ug));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_HbCO2_mmol_Per_L", HbCO2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_HCO3_ug", HCO3->GetMass(MassUnit::ug));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_HCO3_mmol_Per_L", HCO3->GetMolarity(AmountPerVolumeUnit::mmol_Per_L));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_TotalO2_mmol_per_L", O2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) + HbO2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_TotalCO2_mmol_per_L", CO2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) + HCO3->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) + HbCO2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_TotalHb_mmol_per_L", Hb->GetMolarity(AmountPerVolumeUnit::mmol_Per_L));
  m_data.GetDataTrack().Probe(cmpt.GetName() + prefix + "_TotalMass_mmol_per_L", O2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) + CO2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) + HCO3->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) + Hb->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) + HbO2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L) + HbCO2->GetMolarity(AmountPerVolumeUnit::mmol_Per_L));
}

double BioGearsSubstances::GetSubstanceMass(SESubstance& sub, const std::vector<SELiquidCompartment*>& cmpts, const MassUnit& unit)
{
  double mass = 0;
  SELiquidSubstanceQuantity* subQ;
  for (SELiquidCompartment* cmpt : cmpts) {
    subQ = cmpt->GetSubstanceQuantity(sub);
    mass += subQ->GetMass(unit);
  }
  return mass;
}
double BioGearsSubstances::GetSubstanceMass(SESubstance& sub, const std::vector<SETissueCompartment*>& cmpts, const MassUnit& unit)
{
  double mass = 0;
  SELiquidSubstanceQuantity* subQ;
  for (auto itr : m_data.GetCompartments().GetExtracellularFluid()) {
    subQ = itr.second->GetSubstanceQuantity(sub);
    ;
    mass += subQ->GetMass(unit);
  }
  for (auto itr : m_data.GetCompartments().GetIntracellularFluid()) {
    subQ = itr.second->GetSubstanceQuantity(sub);
    mass += subQ->GetMass(unit);
  }
  return mass;
}

void BioGearsSubstances::SetSubstanceConcentration(SESubstance& sub, const std::vector<SELiquidCompartment*>& cmpts, const SEScalarMassPerVolume& concentration)
{
  SELiquidSubstanceQuantity* subQ;
  for (SELiquidCompartment* cmpt : cmpts) {
    subQ = cmpt->GetSubstanceQuantity(sub);
    subQ->GetConcentration().Set(concentration);
    subQ->Balance(BalanceLiquidBy::Concentration);
  }
}
void BioGearsSubstances::SetSubstanceConcentration(SESubstance& sub, const std::vector<SETissueCompartment*>& cmpts, const SEScalarMassPerVolume& concentration)
{
  SELiquidSubstanceQuantity* subQ;
  for (auto itr : m_data.GetCompartments().GetExtracellularFluid()) {
    subQ = itr.second->GetSubstanceQuantity(sub);
    subQ->GetConcentration().Set(concentration);
    subQ->Balance(BalanceLiquidBy::Concentration);
  }
  for (auto itr : m_data.GetCompartments().GetIntracellularFluid()) {
    subQ = itr.second->GetSubstanceQuantity(sub);
    subQ->GetConcentration().Set(concentration);
    subQ->Balance(BalanceLiquidBy::Concentration);
  }
}
void BioGearsSubstances::SetSubstanceConcentration(SESubstance& sub, const std::vector<SETissueCompartment*>& cmpts, const SEScalarMassPerVolume& extracellular, const SEScalarMassPerVolume& intracellular)
{
  SELiquidSubstanceQuantity* subQ;
  for (auto itr : m_data.GetCompartments().GetExtracellularFluid()) {
    subQ = itr.second->GetSubstanceQuantity(sub);
    subQ->GetConcentration().Set(extracellular);
    subQ->Balance(BalanceLiquidBy::Concentration);
  }
  for (auto itr : m_data.GetCompartments().GetIntracellularFluid()) {
    subQ = itr.second->GetSubstanceQuantity(sub);
    subQ->GetConcentration().Set(intracellular);
    subQ->Balance(BalanceLiquidBy::Concentration);
  }
}

void BioGearsSubstances::SetSubstanceMolarity(SESubstance& sub, const std::vector<SELiquidCompartment*>& cmpts, const SEScalarAmountPerVolume& molarity)
{
  SELiquidSubstanceQuantity* subQ;
  for (SELiquidCompartment* cmpt : cmpts) {
    subQ = cmpt->GetSubstanceQuantity(sub);
    subQ->GetMolarity().Set(molarity);
    subQ->Balance(BalanceLiquidBy::Molarity);
  }
}
void BioGearsSubstances::SetSubstanceMolarity(SESubstance& sub, const std::vector<SETissueCompartment*>& cmpts, const SEScalarAmountPerVolume& molarity)
{
  SELiquidSubstanceQuantity* subQ;
  for (auto itr : m_data.GetCompartments().GetExtracellularFluid()) {
    subQ = itr.second->GetSubstanceQuantity(sub);
    subQ->GetMolarity().Set(molarity);
    subQ->Balance(BalanceLiquidBy::Molarity);
  }
  for (auto itr : m_data.GetCompartments().GetIntracellularFluid()) {
    subQ = itr.second->GetSubstanceQuantity(sub);
    subQ->GetMolarity().Set(molarity);
    subQ->Balance(BalanceLiquidBy::Molarity);
  }
}
void BioGearsSubstances::SetSubstanceMolarity(SESubstance& sub, const std::vector<SETissueCompartment*>& cmpts, const SEScalarAmountPerVolume& extracellularMolarity, const SEScalarAmountPerVolume& intracellularMolarity)
{
  SELiquidCompartment* intracellular = nullptr;
  SELiquidCompartment* extracellular = nullptr;

  for (SETissueCompartment* cmpt : cmpts) {
    intracellular = m_data.GetCompartments().GetLiquidCompartment(std::string { cmpt->GetName() } + "Intracellular");
    if (intracellular != nullptr) {
      intracellular->GetSubstanceQuantity(sub)->GetMolarity().Set(intracellularMolarity);
      intracellular->GetSubstanceQuantity(sub)->Balance(BalanceLiquidBy::Molarity);
    }

    extracellular = m_data.GetCompartments().GetLiquidCompartment(std::string { cmpt->GetName() } + "Extracellular");
    if (extracellular != nullptr) {
      extracellular->GetSubstanceQuantity(sub)->GetMolarity().Set(extracellularMolarity);
      extracellular->GetSubstanceQuantity(sub)->Balance(BalanceLiquidBy::Molarity);
    }
  }
}
void BioGearsSubstances::SetSubstanceMolarity(SESubstance& sub, const SEScalarAmountPerVolume& extracellular, const SEScalarAmountPerVolume& intracellular)
{
  SELiquidSubstanceQuantity* subQ;
  for (auto itr : m_data.GetCompartments().GetExtracellularFluid()) {
    subQ = itr.second->GetSubstanceQuantity(sub);
    subQ->GetMolarity().Set(extracellular);
    subQ->Balance(BalanceLiquidBy::Molarity);
  }
  for (auto itr : m_data.GetCompartments().GetIntracellularFluid()) {
    subQ = itr.second->GetSubstanceQuantity(sub);
    subQ->GetMolarity().Set(intracellular);
    subQ->Balance(BalanceLiquidBy::Molarity);
  }
}

const SizeIndependentDepositionEfficencyCoefficient& BioGearsSubstances::GetSizeIndependentDepositionEfficencyCoefficient(SESubstance& substance)
{
  auto itr = m_SIDECoefficients.find(&substance);
  if (itr != m_SIDECoefficients.end())
    return *itr->second;

  if (!substance.HasAerosolization())
    Fatal("Cannot generate a SIDE Coefficient if no aerosolization data is provided");
  if (!substance.GetAerosolization().HasParticulateSizeDistribution())
    Fatal("Cannot generate a SIDE Coefficient if no particulate distribution is provided");

  // This fraction vs. length histogram characterizes the size distribution for a polydispersed aerosol.
  // Length is the aerodynamic diameter of a particle, and Fraction is the fraction of the total
  // that are contained in bin i, which is bounded by boundary i and boundary i+1.
  // The deposition fraction for each bin is computed from the equations fit by Hinds to the ICRP
  // model data @cite Rostami2009computationsal with as much resolution as is provided by the histogram.
  // Regardless of the provided resolution, particles with aerodynamic diameter less than 10^-4 micrometers
  // all deposit in the head and particles greater than 100 micrometers deposit 50% in the head with 50% loss.
  // For more information see the [aerosol](@ref environment-aerosol) section in the @ref EnvironmentMethodology report.
  // A histogram with n bins must contain n+1 boundaries
  SEHistogramFractionVsLength& concentrations = substance.GetAerosolization().GetParticulateSizeDistribution();

  // Check to make sure everything is in the right form.
  if (!concentrations.IsVaild())
    Fatal("Particle distribution histogram is not valid");

  // First we need compartment-specific deposition fractions for each size
  SEHistogramFractionVsLength depositionsMouth;
  SEHistogramFractionVsLength depositionsCarina;
  SEHistogramFractionVsLength depositionsAnatomicalDeadspace;
  SEHistogramFractionVsLength depositionsAlveoli;
  // Copy sizes
  depositionsMouth.GetLength() = concentrations.GetLength();
  depositionsCarina.GetLength() = concentrations.GetLength();
  depositionsAnatomicalDeadspace.GetLength() = concentrations.GetLength();
  depositionsAlveoli.GetLength() = concentrations.GetLength();

  int numPerRegion = 1000; // This is how many times the equations are evaluated in a bin to get a mean deposition fraction
  for (size_t i = 0; i < concentrations.NumberOfBins(); i++) {
    double binLength = concentrations.GetLength()[i + 1] - concentrations.GetLength()[i];
    double stepSize = binLength / double(numPerRegion); //
    double sumHeadAirways = 0.;
    double sumAnatomicalDeadspace = 0.;
    double sumAlveoli = 0.;
    double aerodynamicDiameter;
    for (int j = 0; j < numPerRegion; j++) {
      aerodynamicDiameter = concentrations.GetLength()[i] + stepSize * j; //Start at the bottom of the bin and march towards the top minus one step
      if (aerodynamicDiameter == 0)
        continue;
      double inspirFrac = 1 - 0.5 * (1 - 1 / (1 + 0.00076 * std::pow(aerodynamicDiameter, 2.8)));
      sumHeadAirways += inspirFrac * (1 / (1 + exp(6.84 + 1.183 * log(aerodynamicDiameter))) + 1 / (1 + exp(0.924 - 1.885 * log(aerodynamicDiameter))));
      sumAnatomicalDeadspace += 0.00352 / aerodynamicDiameter * (exp(-0.23 * std::pow((log(aerodynamicDiameter) + 3.4), 2)) + 63.9 * exp(-0.819 * std::pow((log(aerodynamicDiameter) - 1.61), 2)));
      sumAlveoli += 0.0155 / aerodynamicDiameter * (exp(-0.416 * std::pow((log(aerodynamicDiameter) + 2.84), 2)) + 19.11 * exp(-0.482 * std::pow((log(aerodynamicDiameter) - 1.362), 2)));
    }
    // Mean this region
    depositionsMouth.GetFraction().push_back(sumHeadAirways / numPerRegion);
    depositionsAnatomicalDeadspace.GetFraction().push_back(sumAnatomicalDeadspace / numPerRegion);
    depositionsAlveoli.GetFraction().push_back(sumAlveoli / numPerRegion);
    // If any fractions are more than one, weight the error and distribute
    // More than 1.0 can happen with small particles, possibly due to
    // truncation in the equations in @cite Rostami2009computational.
    if (depositionsMouth.GetFraction()[i] + depositionsAnatomicalDeadspace.GetFraction()[i] + depositionsAlveoli.GetFraction()[i] > 1.0) {
      double mag = depositionsMouth.GetFraction()[i] + depositionsAnatomicalDeadspace.GetFraction()[i] + depositionsAlveoli.GetFraction()[i];
      double delta = 1.0 - mag;
      depositionsMouth.GetFraction()[i] = depositionsMouth.GetFraction()[i] * (1. + delta / mag);
      depositionsAnatomicalDeadspace.GetFraction()[i] = depositionsAnatomicalDeadspace.GetFraction()[i] * (1. + delta / mag);
      depositionsAlveoli.GetFraction()[i] = depositionsAlveoli.GetFraction()[i] * (1. + delta / mag);
    }
    // Head airways are split between mouth and carina
    double carinaFraction = 0.5;
    depositionsCarina.GetFraction().push_back(depositionsMouth.GetFraction()[i] * carinaFraction);
    depositionsMouth.GetFraction()[i] *= (1 - carinaFraction);
  }

  // Now we can compute the size-independent deposition efficiencies for each compartment
  // The fraction of the total that deposits in the mouth is equal to the sum of the
  // products of the fraction of each size times the deposition fractions of each size for the mouth.
  // The fraction of the total that deposits in each subsequent windward compartment is the sum of the
  // products of the fraction of each size times the deposition fractions of each size for the compartment
  // divided by the sum of (1-deposition fraction)*concentration fraction for the leeward compartment.
  double sumMouthProducts = 0.;
  double sumMouthOneMinusProducts = 0.;
  double sumCarinaProducts = 0.;
  double sumCarinaOneMinusProducts = 0.;
  double sumDeadspaceProducts = 0.;
  double sumDeadspaceOneMinusProducts = 0.;
  double sumAlveoliProducts = 0.;

  for (size_t i = 0; i < concentrations.NumberOfBins(); i++) {
    sumMouthProducts += concentrations.GetFraction()[i] * depositionsMouth.GetFraction()[i];
    sumMouthOneMinusProducts += concentrations.GetFraction()[i] * (1 - depositionsMouth.GetFraction()[i]);
    sumCarinaProducts += concentrations.GetFraction()[i] * depositionsCarina.GetFraction()[i];
    sumCarinaOneMinusProducts += concentrations.GetFraction()[i] * (1 - depositionsCarina.GetFraction()[i]);
    sumDeadspaceProducts += concentrations.GetFraction()[i] * depositionsAnatomicalDeadspace.GetFraction()[i];
    sumDeadspaceOneMinusProducts += concentrations.GetFraction()[i] * (1 - depositionsAnatomicalDeadspace.GetFraction()[i]);
    sumAlveoliProducts += concentrations.GetFraction()[i] * depositionsAlveoli.GetFraction()[i];
  }

  SizeIndependentDepositionEfficencyCoefficient* SIDECoefficients = new SizeIndependentDepositionEfficencyCoefficient();
  SIDECoefficients->m_mouth = sumMouthProducts;
  SIDECoefficients->m_carina = (sumMouthOneMinusProducts < 1.0e-12) ? 0 : sumCarinaProducts / sumMouthOneMinusProducts;
  SIDECoefficients->m_deadSpace = (sumCarinaOneMinusProducts < 1.0e-12) ? 0 : sumDeadspaceProducts / sumCarinaOneMinusProducts;
  SIDECoefficients->m_alveoli = (sumDeadspaceOneMinusProducts < 1.0e-12) ? 0 : sumAlveoliProducts / sumDeadspaceOneMinusProducts;
  m_SIDECoefficients[&substance] = SIDECoefficients;
  // Now we do not need to track size distributions, only mass (via concentration).

  return *SIDECoefficients;
}
}