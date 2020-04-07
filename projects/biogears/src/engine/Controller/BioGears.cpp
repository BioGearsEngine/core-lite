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
#include <biogears/engine/Controller/BioGears.h>  

#include <biogears/cdm/compartment/fluid/SELiquidCompartment.h>
#include <biogears/cdm/patient/SEPatient.h>
#include <biogears/cdm/patient/assessments/SECompleteBloodCount.h>
#include <biogears/cdm/patient/assessments/SEComprehensiveMetabolicPanel.h>
#include <biogears/cdm/patient/assessments/SEPulmonaryFunctionTest.h>
#include <biogears/cdm/patient/assessments/SEUrinalysis.h>
#include <biogears/cdm/properties/SEScalarArea.h>
#include <biogears/cdm/properties/SEScalarFlowElastance.h>
#include <biogears/cdm/properties/SEScalarFraction.h>
#include <biogears/cdm/properties/SEScalarFrequency.h>
#include <biogears/cdm/properties/SEScalarHeatCapacitance.h>
#include <biogears/cdm/properties/SEScalarHeatCapacitancePerMass.h>
#include <biogears/cdm/properties/SEScalarLength.h>
#include <biogears/cdm/properties/SEScalarMass.h>
#include <biogears/cdm/properties/SEScalarMassPerMass.h>
#include <biogears/cdm/properties/SEScalarMassPerVolume.h>
#include <biogears/cdm/properties/SEScalarNeg1To1.h>
#include <biogears/cdm/substance/SESubstance.h>
#include <biogears/cdm/utils/DataTrack.h>
#include <biogears/cdm/utils/FileUtils.h>
#include <biogears/engine/Equipment/AnesthesiaMachine.h>
#include <biogears/engine/Equipment/ECG.h>
#include <biogears/engine/Systems/Environment.h>
#include <biogears/engine/Systems/Gastrointestinal.h>
#include <biogears/schema/cdm/EnvironmentConditions.hxx>
#include <biogears/schema/cdm/Patient.hxx>

#include <biogears/engine/BioGearsPhysiologyEngine.h>
namespace BGE = mil::tatrc::physiology::biogears;

namespace biogears {

BioGears::BioGears(const std::string& logFileName)
  : BioGears(new Logger(logFileName))
{
  myLogger = true;
  m_DataTrack = nullptr;
}

BioGears::BioGears(Logger* logger)
  : Loggable(logger)
{
  myLogger = false;
  m_DataTrack = nullptr;
  if (!m_Logger->HasForward()) { // Don't override a forwarder, if there already is one there
    m_Logger->SetForward(this);
  }

  m_CurrentTime = std::unique_ptr<SEScalarTime>(new SEScalarTime());
  m_SimulationTime = std::unique_ptr<SEScalarTime>(new SEScalarTime());
  m_CurrentTime->SetValue(0, TimeUnit::s);
  m_SimulationTime->SetValue(0, TimeUnit::s);
  m_Logger->SetLogTime(m_SimulationTime.get());

  m_Substances = std::unique_ptr<BioGearsSubstances>(new BioGearsSubstances(*this));
  m_Substances->LoadSubstanceDirectory();

  m_Patient = std::unique_ptr<SEPatient>(new SEPatient(GetLogger()));

  m_Config = std::unique_ptr<BioGearsConfiguration>(new BioGearsConfiguration(*m_Substances));
  m_Config->Initialize();

  m_SaturationCalculator = std::unique_ptr<SaturationCalculator>(new SaturationCalculator(*this));

  m_Actions = std::unique_ptr<SEActionManager>(new SEActionManager(*m_Substances));
  m_Conditions = std::unique_ptr<SEConditionManager>(new SEConditionManager(*m_Substances));

  m_Environment = std::unique_ptr<Environment>(new Environment(*this));

  m_BloodChemistrySystem = std::unique_ptr<BloodChemistry>(new BloodChemistry(*this));
  m_CardiovascularSystem = std::unique_ptr<Cardiovascular>(new Cardiovascular(*this));
  m_EndocrineSystem = std::unique_ptr<Endocrine>(new Endocrine(*this));
  m_EnergySystem = std::unique_ptr<Energy>(new Energy(*this));
  m_GastrointestinalSystem = std::unique_ptr<Gastrointestinal>(new Gastrointestinal(*this));
  m_HepaticSystem = std::unique_ptr<Hepatic>(new Hepatic(*this));
  m_NervousSystem = std::unique_ptr<Nervous>(new Nervous(*this));
  m_RenalSystem = std::unique_ptr<Renal>(new Renal(*this));
  m_RespiratorySystem = std::unique_ptr<Respiratory>(new Respiratory(*this));
  m_DrugSystem = std::unique_ptr<Drugs>(new Drugs(*this));
  m_TissueSystem = std::unique_ptr<Tissue>(new Tissue(*this));

  m_ECG = std::unique_ptr<ECG>(new ECG(*this));

  m_AnesthesiaMachine = std::unique_ptr<AnesthesiaMachine>(new AnesthesiaMachine(*this));

  m_Inhaler = std::unique_ptr<Inhaler>(new Inhaler(*this));

  m_Compartments = std::unique_ptr<BioGearsCompartments>(new BioGearsCompartments(*this));

  m_DiffusionCalculator = std::unique_ptr<DiffusionCalculator>(new DiffusionCalculator(*this));

  m_Circuits = std::unique_ptr<BioGearsCircuits>(new BioGearsCircuits(*this));
}

DataTrack& BioGears::GetDataTrack()
{
  if (m_DataTrack == nullptr) {
    m_DataTrack = new DataTrack();
  }
  return *m_DataTrack;
}

bool BioGears::Initialize(const PhysiologyEngineConfiguration* config)
{
  m_State = EngineState::NotReady;
  Info("Configuring patient");
  if (!SetupPatient()) {
    return false;
  }

  Info("Resetting Substances");
  m_Substances->Reset();

  // Clear all substances and reload the original data
  // This clears out all engine specific data stored in the substance
  // This will preserve the pointer to the substance, but not any pointers
  // to any substance child objects, those will need to be fixed up, if they exist

  Info("Initializing Configuration");
  m_Config->Initialize(); // Load up Defaults
  if (config != nullptr) {
    Info("Merging Provided Configuration");
    m_Config->Merge(*config);
  }
  // Now, Let's see if there is anything to merge into our base configuration
  Info("Merging OnDisk Configuration");
  BioGearsConfiguration cFile(*m_Substances);
  cFile.Load("BioGearsConfiguration.xml");
  m_Config->Merge(cFile);

  // Now we can check the config
  if (m_Config->WritePatientBaselineFile()) {
    std::string stableDir = "./stable/";
    MakeDirectory(stableDir);
    CDM::PatientData* pData = m_Patient->Unload();
    pData->contentVersion(BGE::Version);
    // Write out the stable patient state
    std::ofstream stream(stableDir + m_Patient->GetName() + ".xml");
    // Write out the xml file
    xml_schema::namespace_infomap map;
    map[""].name = "uri:/mil/tatrc/physiology/datamodel";
    Patient(stream, *pData, map);
    stream.close();
    SAFE_DELETE(pData);
  }

  m_SaturationCalculator->Initialize(*m_Substances);

  m_Actions->Clear();
  m_Conditions->Clear();

  // This will also Initialize the environment
  // Due to needing the initial environment values for circuits to construct properly
  Info("Creating Circuits and Compartments");
  CreateCircuitsAndCompartments();

  m_AirwayMode = CDM::enumBioGearsAirwayMode::Free;
  m_Intubation = CDM::enumOnOff::Off;
  m_CurrentTime->SetValue(0, TimeUnit::s);
  m_SimulationTime->SetValue(0, TimeUnit::s);
  m_Logger->SetLogTime(m_SimulationTime.get());

  Info("Initializing Substances");
  m_Substances->InitializeSubstances(); // Sets all concentrations and such of all substances for all compartments, need to do this after we figure out what's in the environment

  //Note:  Diffusion Calculator is initialized in Tissue::SetUp because it depends on so many Tissue parameters

  Info("Initializing Systems");
  m_CardiovascularSystem->Initialize();
  m_RespiratorySystem->Initialize();
  m_AnesthesiaMachine->Initialize();
  m_GastrointestinalSystem->Initialize();
  m_HepaticSystem->Initialize();
  m_RenalSystem->Initialize();
  m_NervousSystem->Initialize();
  m_EndocrineSystem->Initialize();
  m_DrugSystem->Initialize();
  m_EnergySystem->Initialize();
  m_BloodChemistrySystem->Initialize();
  m_TissueSystem->Initialize(); // Depends on some parameters that Blood Chemistry initializes,needs to be after
  m_ECG->Initialize();
  m_Inhaler->Initialize();

  return true;
}

void BioGears::SetAirwayMode(CDM::enumBioGearsAirwayMode::value mode)
{
  if (mode == m_AirwayMode) {
    return; // do nazing!
  }
  if (mode == CDM::enumBioGearsAirwayMode::Inhaler && m_AirwayMode != CDM::enumBioGearsAirwayMode::Free) {
    throw CommonDataModelException("Can only change airway mode to Inhaler from the Free mode, Disable other equipment first.");
  }
  if (mode == CDM::enumBioGearsAirwayMode::AnesthesiaMachine && m_AirwayMode != CDM::enumBioGearsAirwayMode::Free) {
    throw CommonDataModelException("Can only change airway mode to Anesthesia Machine from the Free mode, Disable other equipment first.");
  }
  if (mode == CDM::enumBioGearsAirwayMode::MechanicalVentilator && m_AirwayMode != CDM::enumBioGearsAirwayMode::Free) {
    throw CommonDataModelException("Can only change airway mode to Mechanical Ventilator from the Free mode, Disable other equipment first.");
  }
  if (mode != m_AirwayMode) {
    m_Compartments->UpdateAirwayGraph();
  }
  m_AirwayMode = mode;
  std::stringstream ss;
  ss << "Airway Mode : " << m_AirwayMode;
  Info(ss);
}
void BioGears::SetIntubation(CDM::enumOnOff::value s)
{
  if (m_Intubation == s) {
    return; // do nazing!
  }
  if (m_AirwayMode == CDM::enumBioGearsAirwayMode::Inhaler) {
    throw CommonDataModelException("Cannot intubate if the inhaler is active.");
  }
  m_Intubation = s;
}

bool BioGears::SetupPatient()
{
  bool err = false;
  std::stringstream ss;

  //Sex is the only thing we absolutely need to be defined
  //Everything else is either derived or assumed to be a "standard" value
  if (!m_Patient->HasSex()) {
    Error("Patient must provide sex.");
    err = true;
  }

  //AGE ---------------------------------------------------------------
  double age_yr;
  double ageMin_yr = 18.0;
  double ageMax_yr = 65.0;
  double ageStandard_yr = 44.0;
  if (!m_Patient->HasAge()) {
    m_Patient->GetAge().SetValue(ageStandard_yr, TimeUnit::yr);
    ss << "No patient age set. Using the standard value of " << ageStandard_yr << " years.";
    Info(ss);
  }
  age_yr = m_Patient->GetAge().GetValue(TimeUnit::yr);
  if (age_yr < ageMin_yr) {
    ss << "Patient age of " << age_yr << " years is too young. We do not model pediatrics. Minimum age allowed is " << ageMin_yr << " years.";
    Error(ss);
    err = true;
  } else if (age_yr > ageMax_yr) {
    ss << "Patient age of " << age_yr << " years is too old. We do not model geriatrics. Maximum age allowed is " << ageMax_yr << " years.";
    Error(ss);
    err = true;
  }

  //PAIN SUSCEPTIBILITY -----------------------------------------------------------------------------------
  double painStandard = 0.0;
  if (!m_Patient->HasPainSusceptibility()) {
    m_Patient->GetPainSusceptibility().SetValue(painStandard);
    ss << "No patient pain susceptibility set " << painStandard << " being used.";
    Info(ss);
  }

  //SWEAT SUSCEPTIBILITY -----------------------------------------------------------------------------------
  double sweatStandard = 0.0;
  if (!m_Patient->HasHyperhidrosis()) {
    m_Patient->GetHyperhidrosis().SetValue(sweatStandard);
    ss << "No patient sweat susceptibility set " << sweatStandard << " being used.";
    Info(ss);
  }

  //HEIGHT ---------------------------------------------------------------
  //From CDC values for 20 year olds
  //Minimums are 3rd percentile, Maximums are 97th percentile, and standard is 50th percentile
  /// \cite Centers2016clinical
  double heightMinMale_cm = 163.0;
  double heightMaxMale_cm = 190.0;
  double heightStandardMale_cm = 177.0;
  double heightMinFemale_cm = 151.0;
  double heightMaxFemale_cm = 175.5;
  double heightStandardFemale_cm = 163.0;
  //Male
  double heightMin_cm = heightMinMale_cm;
  double heightMax_cm = heightMaxMale_cm;
  double heightStandard_cm = heightStandardMale_cm;
  if (m_Patient->GetSex() == CDM::enumSex::Female) {
    //Female
    heightMin_cm = heightMinFemale_cm;
    heightMax_cm = heightMaxFemale_cm;
    heightStandard_cm = heightStandardFemale_cm;
  }
  if (!m_Patient->HasHeight()) {
    m_Patient->GetHeight().SetValue(heightStandard_cm, LengthUnit::cm);
    ss << "No patient height set. Using the standard value of " << heightStandard_cm << " cm.";
    Info(ss);
  }
  double height_cm = m_Patient->GetHeight().GetValue(LengthUnit::cm);
  double height_ft = Convert(height_cm, LengthUnit::cm, LengthUnit::ft);
  //Check for outrageous values
  if (height_ft < 4.5 || height_ft > 7.0) {
    Error("Patient height setting is outrageous. It must be between 4.5 and 7.0 ft");
    err = true;
  }
  if (height_cm < heightMin_cm) {
    ss << "Patient height of " << height_cm << " cm is outside of typical ranges - below 3rd percentile (" << heightMax_cm << " cm). No guarantees of model validity.";
    Warning(ss);
  } else if (height_cm > heightMax_cm) {
    ss << "Patient height of " << height_cm << " cm is outside of typical ranges - above 97th percentile(" << heightMin_cm << " cm). No guarantees of model validity.";
    Warning(ss);
  }

  //WEIGHT ---------------------------------------------------------------
  /// \cite World2006bmi
  double weight_kg;
  double BMI_kg_per_m2;
  double BMIStandard_kg_per_m2 = 21.75;
  double BMIObese_kg_per_m2 = 30.0;
  double BMIOverweight_kg_per_m2 = 25.0;
  double BMIUnderweight_kg_per_m2 = 18.5;
  double BMISeverelyUnderweight_kg_per_m2 = 16.0;
  if (!m_Patient->HasWeight()) {
    weight_kg = BMIStandard_kg_per_m2 * std::pow(m_Patient->GetHeight().GetValue(LengthUnit::m), 2);
    m_Patient->GetWeight().SetValue(weight_kg, MassUnit::kg);
    ss << "No patient weight set. Using the standard BMI value of 21.75 kg/m^2, resulting in a weight of " << weight_kg << " kg.";
    Info(ss);
  }
  weight_kg = m_Patient->GetWeight(MassUnit::kg);
  BMI_kg_per_m2 = weight_kg / std::pow(m_Patient->GetHeight().GetValue(LengthUnit::m), 2);
  if (BMI_kg_per_m2 > BMIObese_kg_per_m2) {
    ss << "Patient Body Mass Index (BMI) of " << BMI_kg_per_m2 << "  kg/m^2 is too high. Obese patients must be modeled by adding/using a condition. Maximum BMI allowed is " << BMIObese_kg_per_m2 << " kg/m^2.";
    Error(ss);
    err = true;
  }
  if (BMI_kg_per_m2 > BMIOverweight_kg_per_m2) {
    ss << "Patient Body Mass Index (BMI) of " << BMI_kg_per_m2 << " kg/m^2 is overweight. No guarantees of model validity.";
    Warning(ss);
  }
  if (BMI_kg_per_m2 < BMIUnderweight_kg_per_m2) {
    ss << "Patient Body Mass Index (BMI) of " << BMI_kg_per_m2 << " kg/m^2 is underweight. No guarantees of model validity.";
    Warning(ss);
  }
  if (BMI_kg_per_m2 < BMISeverelyUnderweight_kg_per_m2) {
    ss << "Patient Body Mass Index (BMI) of " << BMI_kg_per_m2 << " kg/m^2 is too low. Severly underweight patients must be modeled by adding/using a condition. Maximum BMI allowed is " << BMISeverelyUnderweight_kg_per_m2 << " kg/m^2.";
    Error(ss);
    err = true;
  }

  //BODY FAT FRACTION ---------------------------------------------------------------
  //From American Council on Exercise
  /// \cite muth2009what
  double fatFraction;
  double fatFractionStandardMale = 0.21;
  double fatFractionStandardFemale = 0.28;
  double fatFractionMaxMale = 0.25; //Obese
  double fatFractionMaxFemale = 0.32; //Obese
  double fatFractionMinMale = 0.02; //Essential fat
  double fatFractionMinFemale = 0.10; //Essential fat
  //Male
  double fatFractionMin = fatFractionMinMale;
  double fatFractionMax = fatFractionMaxMale;
  double fatFractionStandard = fatFractionStandardMale;
  if (m_Patient->GetSex() == CDM::enumSex::Female) {
    //Female
    fatFractionMin = fatFractionMinFemale;
    fatFractionMax = fatFractionMaxFemale;
    fatFractionStandard = fatFractionStandardFemale;
  }

  if (!m_Patient->HasBodyFatFraction()) {
    fatFraction = fatFractionStandard;
    m_Patient->GetBodyFatFraction().SetValue(fatFraction);
    ss << "No patient body fat fraction set. Using the standard value of " << fatFraction << ".";
    Info(ss);
  }
  fatFraction = m_Patient->GetBodyFatFraction().GetValue();
  if (fatFraction > fatFractionMax) {
    ss << "Patient body fat fraction of " << fatFraction << " is too high. Obese patients must be modeled by adding/using a condition. Maximum body fat fraction allowed is " << fatFractionMax << ".";
    Error(ss);
    err = true;
  } else if (fatFraction < fatFractionMin) {
    ss << "Patient body fat fraction  " << fatFraction << " is too low. Patients must have essential fat. Minimum body fat fraction allowed is " << fatFractionMin << ".";
    Error(ss);
    err = true;
  }

  //Lean Body Mass ---------------------------------------------------------------
  if (m_Patient->HasLeanBodyMass()) {
    ss << "Patient lean body mass cannot be set. It is determined by weight and body fat fraction.";
    Error(ss);
    err = true;
  }
  double leanBodyMass_kg = weight_kg * (1.0 - fatFraction);
  m_Patient->GetLeanBodyMass().SetValue(leanBodyMass_kg, MassUnit::kg);
  ss << "Patient lean body mass computed and set to " << leanBodyMass_kg << " kg.";
  Info(ss);

  //Muscle Mass ---------------------------------------------------------------
  // \cite janssen2000skeletal
  if (m_Patient->HasMuscleMass()) {
    ss << "Patient muscle mass cannot be set directly. It is determined by a percentage of weight.";
    Error(ss);
    err = true;
  }

  if (m_Patient->GetSex() == CDM::enumSex::Female) {
    m_Patient->GetMuscleMass().SetValue(weight_kg * .306, MassUnit::kg);
  } else {
    m_Patient->GetMuscleMass().SetValue(weight_kg * .384, MassUnit::kg);
  }

  ss << "Patient muscle mass computed and set to " << m_Patient->GetMuscleMass().GetValue(MassUnit::kg) << " kg.";
  Info(ss);

  //Body Density ---------------------------------------------------------------
  if (m_Patient->HasBodyDensity()) {
    ss << "Patient body density cannot be set. It is determined using body fat fraction.";
    Error(ss);
    err = true;
  }
  //Using the average of Siri and Brozek formulas
  /// \cite siri1961body
  /// \cite brovzek1963densitometric
  double SiriBodyDensity_g_Per_cm3 = 4.95 / (fatFraction + 4.50);
  double BrozekBodyDensity_g_Per_cm3 = 4.57 / (fatFraction + 4.142);
  double bodyDensity_g_Per_cm3 = (SiriBodyDensity_g_Per_cm3 + BrozekBodyDensity_g_Per_cm3) / 2.0;
  m_Patient->GetBodyDensity().SetValue(bodyDensity_g_Per_cm3, MassPerVolumeUnit::g_Per_cm3);
  ss << "Patient body density computed and set to " << bodyDensity_g_Per_cm3 << " g/cm^3.";
  Info(ss);

  //Heart Rate ---------------------------------------------------------------
  double heartRate_bpm;
  double heartStandard_bpm = 72.0;
  double heartRateMax_bpm = 109.0;
  double heartRateTachycardia_bpm = 100.0;
  double heartRateMin_bpm = 50.0;
  double heartRateBradycardia_bpm = 60.0;
  if (!m_Patient->HasHeartRateBaseline()) {
    heartRate_bpm = heartStandard_bpm;
    m_Patient->GetHeartRateBaseline().SetValue(heartRate_bpm, FrequencyUnit::Per_min);
    ss << "No patient heart rate baseline set. Using the standard value of " << heartRate_bpm << " bpm.";
    Info(ss);
  }
  heartRate_bpm = m_Patient->GetHeartRateBaseline(FrequencyUnit::Per_min);
  if (heartRateTachycardia_bpm < heartRate_bpm) {
    if (heartRate_bpm <= heartRateMax_bpm) {
      ss << "Patient heart rate baseline of " << heartRate_bpm << " bpm is tachycardic";
      Info(ss);
    } else {
      ss << "Patient heart rate baseline of " << heartRate_bpm << " exceeds maximum stable value of " << heartRateMax_bpm << " bpm.  Resetting heart rate baseline to " << heartRateMax_bpm;
      m_Patient->GetHeartRateBaseline().SetValue(heartRateMax_bpm, FrequencyUnit::Per_min);
      Info(ss);
    }
  } else if (heartRate_bpm < heartRateBradycardia_bpm) {
    if (heartRateMin_bpm <= heartRate_bpm) {
      ss << "Patient heart rate baseline of " << heartRate_bpm << " bpm is bradycardic";
      Info(ss);
    } else {
      ss << "Patient heart rate baseline of " << heartRate_bpm << " exceeds minimum stable value of " << heartRateMin_bpm << " bpm.  Resetting heart rate baseline to " << heartRateMin_bpm;
      m_Patient->GetHeartRateBaseline().SetValue(heartRateMin_bpm, FrequencyUnit::Per_min);
      Info(ss);
    }
  }

  //Tanaka H, Monahan KD, Seals DR (January 2001). "Age-predicted maximal heart rate revisited". J. Am. Coll. Cardiol. 37(1): 153–6. doi:10.1016/S0735-1097(00)01054-8.PMID 11153730.
  double computedHeartRateMaximum_bpm = 208.0 - (0.7 * m_Patient->GetAge(TimeUnit::yr));
  if (!m_Patient->HasHeartRateMaximum()) {
    m_Patient->GetHeartRateMaximum().SetValue(computedHeartRateMaximum_bpm, FrequencyUnit::Per_min);
    ss << "No patient heart rate maximum set. Using a computed value of " << computedHeartRateMaximum_bpm << " bpm.";
    Info(ss);
  } else {
    if (m_Patient->GetHeartRateMaximum(FrequencyUnit::Per_min) < heartRate_bpm) {
      ss << "Patient heart rate maximum must be greater than the baseline heart rate.";
      Error(ss);
      err = true;
    }
    ss << "Specified patient heart rate maximum of " << m_Patient->GetHeartRateMaximum(FrequencyUnit::Per_min) << " bpm differs from computed value of " << computedHeartRateMaximum_bpm << " bpm. No guarantees of model validity.";
    Warning(ss);
  }
  if (!m_Patient->HasHeartRateMinimum()) {
    m_Patient->GetHeartRateMinimum().SetValue(0.001, FrequencyUnit::Per_min);
    ss << "No patient heart rate minimum set. Using a default value of " << 0.001 << " bpm.";
    Info(ss);
  }
  if (m_Patient->GetHeartRateMinimum(FrequencyUnit::Per_min) > heartRate_bpm) {
    ss << "Patient heart rate minimum must be less than the baseline heart rate.";
    Error(ss);
    err = true;
  }

  //Arterial Pressures ---------------------------------------------------------------
  double systolic_mmHg;
  double diastolic_mmHg;
  double systolicStandard_mmHg = 114.0;
  double diastolicStandard_mmHg = 73.5;
  double systolicMax_mmHg = 120.0; //Hypertension
  double diastolicMax_mmHg = 80.0; //Hypertension
  double systolicMin_mmHg = 90.0; //Hypotension
  double diastolicMin_mmHg = 60.0; //Hypotension
  double narrowestPulseFactor = 0.75; //From Wikipedia: Pulse Pressure
  if (!m_Patient->HasSystolicArterialPressureBaseline()) {
    systolic_mmHg = systolicStandard_mmHg;
    m_Patient->GetSystolicArterialPressureBaseline().SetValue(systolic_mmHg, PressureUnit::mmHg);
    ss << "No patient systolic pressure baseline set. Using the standard value of " << systolic_mmHg << " mmHg.";
    Info(ss);
  }
  systolic_mmHg = m_Patient->GetSystolicArterialPressureBaseline(PressureUnit::mmHg);
  if (systolic_mmHg < systolicMin_mmHg) {
    ss << "Patient systolic pressure baseline of " << systolic_mmHg << " mmHg is too low. Hypotension must be modeled by adding/using a condition. Minimum systolic pressure baseline allowed is " << systolicMin_mmHg << " mmHg.";
    Error(ss);
    err = true;
  } else if (systolic_mmHg > systolicMax_mmHg) {
    ss << "Patient systolic pressure baseline of " << systolic_mmHg << " mmHg is too high. Hypertension must be modeled by adding/using a condition. Maximum systolic pressure baseline allowed is " << systolicMax_mmHg << " mmHg.";
    Error(ss);
    err = true;
  }

  if (!m_Patient->HasDiastolicArterialPressureBaseline()) {
    diastolic_mmHg = diastolicStandard_mmHg;
    m_Patient->GetDiastolicArterialPressureBaseline().SetValue(diastolic_mmHg, PressureUnit::mmHg);
    ss << "No patient diastolic pressure baseline set. Using the standard value of " << diastolic_mmHg << " mmHg.";
    Info(ss);
  }
  diastolic_mmHg = m_Patient->GetDiastolicArterialPressureBaseline(PressureUnit::mmHg);
  if (diastolic_mmHg < diastolicMin_mmHg) {
    ss << "Patient diastolic pressure baseline of " << diastolic_mmHg << " mmHg is too low. Hypotension must be modeled by adding/using a condition. Minimum diastolic pressure baseline allowed is " << diastolicMin_mmHg << " mmHg.";
    Error(ss);
    err = true;
  } else if (diastolic_mmHg > diastolicMax_mmHg) {
    ss << "Patient diastolic pressure baseline of " << diastolic_mmHg << " mmHg is too high. Hypertension must be modeled by adding/using a condition. Maximum diastolic pressure baseline allowed is " << diastolicMax_mmHg << " mmHg.";
    Error(ss);
    err = true;
  }

  if (diastolic_mmHg > 0.75 * systolic_mmHg) {
    ss << "Patient baseline pulse pressure (systolic vs. diastolic pressure fraction) of " << diastolic_mmHg / systolic_mmHg << " is abnormally narrow. Minimum fraction allowed is " << narrowestPulseFactor << " .";
    Error(ss);
    err = true;
  }

  if (m_Patient->HasMeanArterialPressureBaseline()) {
    ss << "Patient mean arterial pressure baseline cannot be set. It is determined through homeostatic simulation.";
    Error(ss);
    err = true;
  }
  double MAP_mmHg = 1.0 / 3.0 * systolic_mmHg + 2.0 / 3.0 * diastolic_mmHg;
  m_Patient->GetMeanArterialPressureBaseline().SetValue(MAP_mmHg, PressureUnit::mmHg);

  //Blood Volume ---------------------------------------------------------------
  /// \cite Morgan2006Clinical
  double bloodVolume_mL;
  double computedBloodVolume_mL = 65.6 * std::pow(weight_kg, 1.02);
  double bloodVolumeMin_mL = computedBloodVolume_mL * 0.85; //Stage 1 Hypovolemia
  double bloodVolumeMax_mL = computedBloodVolume_mL * 1.15; //Just go the same distance on the other side
  if (!m_Patient->HasBloodVolumeBaseline()) {
    bloodVolume_mL = computedBloodVolume_mL;
    m_Patient->GetBloodVolumeBaseline().SetValue(bloodVolume_mL, VolumeUnit::mL);
    ss << "No patient blood volume baseline set. Using a computed value of " << computedBloodVolume_mL << " mL.";
    Info(ss);
  }
  bloodVolume_mL = m_Patient->GetBloodVolumeBaseline(VolumeUnit::mL);
  if (bloodVolume_mL != computedBloodVolume_mL) {
    ss << "Specified patient blood volume baseline of " << bloodVolume_mL << " mL differs from computed value of " << computedBloodVolume_mL << " mL. No guarantees of model validity and there is a good chance the patient will not reach a starting homeostatic point.";
    Warning(ss);
  }
  if (bloodVolume_mL < bloodVolumeMin_mL) {
    ss << "Patient blood volume baseline of " << bloodVolume_mL << " mL is too low. Hypovolemia must be modeled by adding/using a condition. Minimum blood volume baseline allowed is " << bloodVolumeMin_mL << " mL.";
    Error(ss);
    err = true;
  } else if (bloodVolume_mL > bloodVolumeMax_mL) {
    ss << "Patient blood volume baseline of " << bloodVolume_mL << " mL is too high. Excessive volume must be modeled by adding/using a condition. Maximum blood volume baseline allowed is " << bloodVolumeMax_mL << " mL.";
    Error(ss);
    err = true;
  }

  //Respiration Rate ---------------------------------------------------------------
  //Note: This is overwritten after stabilization
  double respirationRate_bpm;
  double respirationRateStandard_bpm = 16.0;
  double respirationRateMax_bpm = 20.0;
  double respirationRateMin_bpm = 12.0;
  if (!m_Patient->HasRespirationRateBaseline()) {
    respirationRate_bpm = respirationRateStandard_bpm;
    m_Patient->GetRespirationRateBaseline().SetValue(respirationRate_bpm, FrequencyUnit::Per_min);
    ss << "No patient respiration rate baseline set. Using the standard value of " << respirationRate_bpm << " bpm.";
    Info(ss);
  }

  respirationRate_bpm = m_Patient->GetRespirationRateBaseline(FrequencyUnit::Per_min);

  if (respirationRate_bpm > respirationRateMax_bpm) {
    ss << "Patient respiration rate baseline of " << respirationRate_bpm << " bpm is too high. Non-healthy values must be modeled by adding/using a condition. Maximum respiration rate baseline allowed is " << respirationRateMax_bpm << " bpm.";
    Error(ss);
    err = true;
  } else if (respirationRate_bpm < respirationRateMin_bpm) {
    ss << "Patient respiration rate baseline of " << respirationRate_bpm << " bpm is too low. Non-healthy values must be modeled by adding/using a condition. Minimum respiration rate baseline allowed is " << respirationRateMin_bpm << " bpm.";
    Error(ss);
    err = true;
  }

  //Right Lung Ratio ---------------------------------------------------------------
  double rightLungRatio;
  double rightLungRatioStandard = 0.525;
  double rightLungRatioMax = 0.60;
  double rightLungRatioMin = 0.50;
  if (!m_Patient->HasRightLungRatio()) {
    rightLungRatio = rightLungRatioStandard;
    m_Patient->GetRightLungRatio().SetValue(rightLungRatio);
    ss << "No patient right lung ratio set. Using the standard value of " << rightLungRatio << ".";
    Info(ss);
  }
  rightLungRatio = m_Patient->GetRightLungRatio().GetValue();
  if (rightLungRatio > rightLungRatioMax) {
    ss << "Patient right lung ratio of " << rightLungRatio << " is too high. Non-healthy values must be modeled by adding/using a condition. Maximum right lung ratio allowed is " << rightLungRatioMax << ".";
    Error(ss);
    err = true;
  } else if (rightLungRatio < rightLungRatioMin) {
    ss << "Patient right lung ratio of " << rightLungRatio << " is too low. Non-healthy values must be modeled by adding/using a condition. Minimum right lung ratio allowed is " << rightLungRatioMin << ".";
    Error(ss);
    err = true;
  }

  //Respiratory Volumes ---------------------------------------------------------------
  //These are based on weight
  /// \cite ganong1995review
  double totalLungCapacity_L;
  double computedTotalLungCapacity_L = 80.0 * weight_kg / 1000.0;
  if (!m_Patient->HasTotalLungCapacity()) {
    totalLungCapacity_L = computedTotalLungCapacity_L;
    m_Patient->GetTotalLungCapacity().SetValue(totalLungCapacity_L, VolumeUnit::L);
    ss << "No patient total lung capacity set. Using a computed value of " << computedTotalLungCapacity_L << " L.";
    Info(ss);
  }
  totalLungCapacity_L = m_Patient->GetTotalLungCapacity(VolumeUnit::L);
  if (totalLungCapacity_L != computedTotalLungCapacity_L) {
    ss << "Specified total lung capacity of " << totalLungCapacity_L << " L differs from computed value of " << computedTotalLungCapacity_L << " L. No guarantees of model validity.";
    Warning(ss);
  }

  double functionalResidualCapacity_L;
  double computedFunctionalResidualCapacity_L = 30.0 * weight_kg / 1000.0;
  if (!m_Patient->HasFunctionalResidualCapacity()) {
    functionalResidualCapacity_L = computedFunctionalResidualCapacity_L;
    m_Patient->GetFunctionalResidualCapacity().SetValue(functionalResidualCapacity_L, VolumeUnit::L);
    ss << "No patient functional residual capacity set. Using a computed value of " << computedFunctionalResidualCapacity_L << " L.";
    Info(ss);
  }
  functionalResidualCapacity_L = m_Patient->GetFunctionalResidualCapacity(VolumeUnit::L);
  if (functionalResidualCapacity_L != computedFunctionalResidualCapacity_L) {
    ss << "Specified functional residual capacity of " << functionalResidualCapacity_L << " L differs from computed value of " << computedFunctionalResidualCapacity_L << " L. No guarantees of model validity.";
    Warning(ss);
  }

  double residualVolume_L;
  double computRedesidualVolume_L = 16.0 * weight_kg / 1000.0;
  if (!m_Patient->HasResidualVolume()) {
    residualVolume_L = computRedesidualVolume_L;
    m_Patient->GetResidualVolume().SetValue(residualVolume_L, VolumeUnit::L);
    ss << "No patient residual volume set. Using a computed value of " << computRedesidualVolume_L << " L.";
    Info(ss);
  }
  residualVolume_L = m_Patient->GetResidualVolume(VolumeUnit::L);
  if (residualVolume_L != computRedesidualVolume_L) {
    ss << "Specified residual volume of " << residualVolume_L << " L differs from computed value of " << computRedesidualVolume_L << " L. No guarantees of model validity.";
    Warning(ss);
  }

  if (m_Patient->HasTidalVolumeBaseline()) {
    ss << "Patient tidal volume baseline cannot be set. It is determined through homeostatic simulation.";
    Error(ss);
    err = true;
  }
  if (m_Patient->HasVitalCapacity()) {
    ss << "Patient vital capacity cannot be set. It is directly computed via other lung volume patient parameters.";
    Error(ss);
    err = true;
  }
  if (m_Patient->HasExpiratoryReserveVolume()) {
    ss << "Patient expiratory reserve volume cannot be set. It is directly computed via other lung volume patient parameters.";
    Error(ss);
    err = true;
  }
  if (m_Patient->HasInspiratoryReserveVolume()) {
    ss << "Patient inspiratory reserve volume cannot be set. It is directly computed via other lung volume patient parameters.";
    Error(ss);
    err = true;
  }
  if (m_Patient->HasInspiratoryCapacity()) {
    ss << "Patient inspiratory capacity cannot be set. It is directly computed via other lung volume patient parameters.";
    Error(ss);
    err = true;
  }

  double tidalVolume_L = 37.0 * weight_kg / 1000.0 - functionalResidualCapacity_L;
  double targetVent_L_Per_min = tidalVolume_L * respirationRate_bpm;
  m_Patient->GetTotalVentilationBaseline().SetValue(targetVent_L_Per_min, VolumePerTimeUnit::L_Per_min);
  //\ToDo:  Could probably optimze further by taking gender into account
  //Stabilization goes faster if we start the driver with a good amplitude that pushes blood gas levels to setpoint.
  //Based off testing, this relationship holds up well between RR = 12 and RR = 16 for Standard Male.
  double baselineDriverPressure_cmH2O = -5.8 + 0.25 * (respirationRate_bpm - 12.0);
  //Adjust driver pressure relationship for respiration rates > 16 (slope of driver - RR line decreases)
  if (respirationRate_bpm > 16.0) {
    //-4.8 = driver pressure at 16 bpm.
    baselineDriverPressure_cmH2O = -4.8 + 0.125 * (respirationRate_bpm - 16);
  }
  //Scale target pressure as ratio of calculated FRC to Standard Male FRC
  double standardFRC_L = 2.31332;
  baselineDriverPressure_cmH2O *= functionalResidualCapacity_L / standardFRC_L;

  BLIM(baselineDriverPressure_cmH2O, -5.8, -3.5);
  m_Patient->GetRespiratoryDriverAmplitudeBaseline().SetValue(baselineDriverPressure_cmH2O, PressureUnit::cmH2O);

  double vitalCapacity = totalLungCapacity_L - residualVolume_L;
  double expiratoryReserveVolume = functionalResidualCapacity_L - residualVolume_L;
  double inspiratoryReserveVolume = totalLungCapacity_L - functionalResidualCapacity_L - tidalVolume_L;
  double inspiratoryCapacity = totalLungCapacity_L - functionalResidualCapacity_L;
  //No negative volumes
  if (totalLungCapacity_L < 0.0 || functionalResidualCapacity_L < 0.0 || residualVolume_L < 0.0 || tidalVolume_L < 0.0 || vitalCapacity < 0.0 || expiratoryReserveVolume < 0.0 || inspiratoryReserveVolume < 0.0 || inspiratoryCapacity < 0.0) {
    ss << "All patient lung volumes must be positive.";
    Error(ss);
    err = true;
  }
  m_Patient->GetTidalVolumeBaseline().SetValue(tidalVolume_L, VolumeUnit::L); //This is overwritten after stabilization
  ss << "Patient tidal volume computed and set to " << tidalVolume_L << " L.";
  Info(ss);

  m_Patient->GetVitalCapacity().SetValue(vitalCapacity, VolumeUnit::L);
  ss << "Patient vital capacity computed and set to " << vitalCapacity << " L.";
  Info(ss);

  m_Patient->GetExpiratoryReserveVolume().SetValue(expiratoryReserveVolume, VolumeUnit::L);
  ss << "Patient expiratory reserve volume computed and set to " << expiratoryReserveVolume << " L.";
  Info(ss);

  m_Patient->GetInspiratoryReserveVolume().SetValue(inspiratoryReserveVolume, VolumeUnit::L);
  ss << "Patient inspiratory reserve volume computed and set to " << inspiratoryReserveVolume << " L.";
  Info(ss);

  m_Patient->GetInspiratoryCapacity().SetValue(inspiratoryCapacity, VolumeUnit::L);
  ss << "Patient inspiratory capacity computed and set to " << inspiratoryCapacity << " L.";
  Info(ss);

  //Alveoli Surface Area ---------------------------------------------------------------
  /// \cite roberts2000gaseous
  double standardAlveoliSurfaceArea_m2 = 70.0;
  double alveoliSurfaceArea_m2;
  //Scale the alveoli surface area based on the size of the patient’s lungs
  /// cite ganong1995review
  double standardTotalLungCapacity_L = 6.17; //This is the Total Lung Capacity of our standard patient
  double computedAlveoliSurfaceArea_m2 = totalLungCapacity_L / standardTotalLungCapacity_L * standardAlveoliSurfaceArea_m2;
  if (!m_Patient->HasAlveoliSurfaceArea()) {
    alveoliSurfaceArea_m2 = computedAlveoliSurfaceArea_m2;
    m_Patient->GetAlveoliSurfaceArea().SetValue(alveoliSurfaceArea_m2, AreaUnit::m2);
    ss << "No patient alveoli surface area set. Using a computed value of " << computedAlveoliSurfaceArea_m2 << " m^2.";
    Info(ss);
  }
  alveoliSurfaceArea_m2 = m_Patient->GetAlveoliSurfaceArea(AreaUnit::m2);
  if (alveoliSurfaceArea_m2 != computedAlveoliSurfaceArea_m2) {
    ss << "Specified alveoli surface area of " << alveoliSurfaceArea_m2 << " m^2 differs from computed value of " << computedAlveoliSurfaceArea_m2 << " m^2. No guarantees of model validity.";
    Warning(ss);
  }

  //Skin Surface Area ---------------------------------------------------------------
  /// \cite du1989formula
  double skinSurfaceArea_m2;
  double computSkinSurfaceArea_m2 = 0.20247 * std::pow(weight_kg, 0.425) * std::pow(Convert(height_cm, LengthUnit::cm, LengthUnit::m), 0.725);
  if (!m_Patient->HasSkinSurfaceArea()) {
    skinSurfaceArea_m2 = computSkinSurfaceArea_m2;
    m_Patient->GetSkinSurfaceArea().SetValue(skinSurfaceArea_m2, AreaUnit::m2);
    ss << "No patient skin surface area set. Using a computed value of " << computSkinSurfaceArea_m2 << " m^2.";
    Info(ss);
  }
  skinSurfaceArea_m2 = m_Patient->GetSkinSurfaceArea(AreaUnit::m2);
  if (skinSurfaceArea_m2 != computSkinSurfaceArea_m2) {
    ss << "Specified skin surface area of " << skinSurfaceArea_m2 << " cm differs from computed value of " << computSkinSurfaceArea_m2 << " cm. No guarantees of model validity.";
    Warning(ss);
  }

  //Basal Metabolic Rate ---------------------------------------------------------------
  //The basal metabolic rate is determined from the Harris-Benedict formula, with differences dependent on sex, age, height and mass
  /// \cite roza1984metabolic
  double BMR_kcal_Per_day;
  double computBMR_kcal_Per_day = 88.632 + 13.397 * weight_kg + 4.799 * height_cm - 5.677 * age_yr; //Male
  if (m_Patient->GetSex() == CDM::enumSex::Female) {
    computBMR_kcal_Per_day = 447.593 + 9.247 * weight_kg + 3.098 * height_cm - 4.330 * age_yr; //Female
  }
  if (!m_Patient->HasBasalMetabolicRate()) {
    BMR_kcal_Per_day = computBMR_kcal_Per_day;
    m_Patient->GetBasalMetabolicRate().SetValue(BMR_kcal_Per_day, PowerUnit::kcal_Per_day);

    ss << "No patient basal metabolic rate set. Using a computed value of " << computBMR_kcal_Per_day << " kcal/day.";
    Info(ss);
  }
  BMR_kcal_Per_day = m_Patient->GetBasalMetabolicRate(PowerUnit::kcal_Per_day);
  if (BMR_kcal_Per_day != computBMR_kcal_Per_day) {
    ss << "Specified basal metabolic rate of " << BMR_kcal_Per_day << " kcal/day differs from computed value of " << computBMR_kcal_Per_day << " kcal/day. No guarantees of model validity.";
    Warning(ss);
  }

  //Maximum Work Rate ---------------------------------------------------------------
  //The max work rate is determined from linear regressions of research by Plowman et al., with differences dependent on sex and age
  /// \cite plowman2013exercise
  double maxWorkRate_W;
  double computedMaxWorkRate_W;

  if (m_Patient->GetSex() == CDM::enumSex::Male) {
    if (age_yr >= 60.) {
      computedMaxWorkRate_W = ((-24.3 * 60.) + 2070.);
    } else {
      computedMaxWorkRate_W = ((-24.3 * age_yr) + 2070.);
    }
  } else {
    if (age_yr >= 60.) {
      computedMaxWorkRate_W = ((-20.7 * 60.) + 1673.);
    } else {
      computedMaxWorkRate_W = ((-20.7 * age_yr) + 1673.);
    }
  }

  if (!m_Patient->HasMaxWorkRate()) {
    maxWorkRate_W = computedMaxWorkRate_W;
    m_Patient->GetMaxWorkRate().SetValue(maxWorkRate_W, PowerUnit::W);

    ss << "No patient maximum work rate set. Using a computed value of " << computedMaxWorkRate_W << " Watts.";
    Info(ss);
  }
  maxWorkRate_W = m_Patient->GetMaxWorkRate(PowerUnit::W);
  if (maxWorkRate_W != computedMaxWorkRate_W) {
    ss << "Specified maximum work rate of " << maxWorkRate_W << " Watts differs from computed value of " << computedMaxWorkRate_W << " Watts. No guarantees of model validity.";
    Warning(ss);
  }

  if (err) {
    return false;
  }
  return true;
}

BioGears::~BioGears()
{
  if (myLogger) {
    SAFE_DELETE(m_Logger);
  } else { //Turn off forwarding for this logger
    m_Logger->SetForward(nullptr);
  }
}

EngineState BioGears::GetState() { return m_State; }
SaturationCalculator& BioGears::GetSaturationCalculator() { return *m_SaturationCalculator; }
DiffusionCalculator& BioGears::GetDiffusionCalculator() { return *m_DiffusionCalculator; }
BioGearsSubstances& BioGears::GetSubstances() { return *m_Substances; }
SEPatient& BioGears::GetPatient() { return *m_Patient; }
SEBloodChemistrySystem& BioGears::GetBloodChemistry() { return *m_BloodChemistrySystem; }
SECardiovascularSystem& BioGears::GetCardiovascular() { return *m_CardiovascularSystem; }
SEDrugSystem& BioGears::GetDrugs() { return *m_DrugSystem; }
SEEndocrineSystem& BioGears::GetEndocrine() { return *m_EndocrineSystem; }
SEEnergySystem& BioGears::GetEnergy() { return *m_EnergySystem; }
SEGastrointestinalSystem& BioGears::GetGastrointestinal() { return *m_GastrointestinalSystem; }
SEHepaticSystem& BioGears::GetHepatic() { return *m_HepaticSystem; }
SENervousSystem& BioGears::GetNervous() { return *m_NervousSystem; }
SERenalSystem& BioGears::GetRenal() { return *m_RenalSystem; }
SERespiratorySystem& BioGears::GetRespiratory() { return *m_RespiratorySystem; }
SETissueSystem& BioGears::GetTissue() { return *m_TissueSystem; }
SEEnvironment& BioGears::GetEnvironment() { return *m_Environment; }

const EngineState BioGears::GetState() const { return m_State; }
const SaturationCalculator& BioGears::GetSaturationCalculator() const { return *m_SaturationCalculator; }
const DiffusionCalculator& BioGears::GetDiffusionCalculator() const { return *m_DiffusionCalculator; }
const BioGearsSubstances& BioGears::GetSubstances() const { return *m_Substances; }
const SEPatient& BioGears::GetPatient() const { return *m_Patient; }
const SEBloodChemistrySystem& BioGears::GetBloodChemistry() const { return *m_BloodChemistrySystem; }
const SECardiovascularSystem& BioGears::GetCardiovascular() const { return *m_CardiovascularSystem; }
const SEDrugSystem& BioGears::GetDrugs() const { return *m_DrugSystem; }
const SEEndocrineSystem& BioGears::GetEndocrine() const { return *m_EndocrineSystem; }
const SEEnergySystem& BioGears::GetEnergy() const { return *m_EnergySystem; }
const SEGastrointestinalSystem& BioGears::GetGastrointestinal() const { return *m_GastrointestinalSystem; }
const SEHepaticSystem& BioGears::GetHepatic() const { return *m_HepaticSystem; }
const SENervousSystem& BioGears::GetNervous() const { return *m_NervousSystem; }
const SERenalSystem& BioGears::GetRenal() const { return *m_RenalSystem; }
const SERespiratorySystem& BioGears::GetRespiratory() const { return *m_RespiratorySystem; }
const SETissueSystem& BioGears::GetTissue() const { return *m_TissueSystem; }
const SEEnvironment& BioGears::GetEnvironment() const { return *m_Environment; }

SEAnesthesiaMachine& BioGears::GetAnesthesiaMachine() { return *m_AnesthesiaMachine; }
SEElectroCardioGram& BioGears::GetECG() { return *m_ECG; }
SEInhaler& BioGears::GetInhaler() { return *m_Inhaler; }
SEActionManager& BioGears::GetActions() { return *m_Actions; }
SEConditionManager& BioGears::GetConditions() { return *m_Conditions; }
BioGearsCircuits& BioGears::GetCircuits() { return *m_Circuits; }
BioGearsCompartments& BioGears::GetCompartments() { return *m_Compartments; }
const BioGearsConfiguration& BioGears::GetConfiguration() { return *m_Config; }
const SEScalarTime& BioGears::GetEngineTime() { return *m_CurrentTime; }
const SEScalarTime& BioGears::GetSimulationTime() { return *m_SimulationTime; }
const SEScalarTime& BioGears::GetTimeStep() { return m_Config->GetTimeStep(); }
CDM::enumBioGearsAirwayMode::value BioGears::GetAirwayMode() { return m_AirwayMode; }
CDM::enumOnOff::value BioGears::GetIntubation() { return m_Intubation; }

const SEAnesthesiaMachine& BioGears::GetAnesthesiaMachine() const { return *m_AnesthesiaMachine; }
const SEElectroCardioGram& BioGears::GetECG() const { return *m_ECG; }
const SEInhaler& BioGears::GetInhaler() const { return *m_Inhaler; }
const SEActionManager& BioGears::GetActions() const { return *m_Actions; }
const SEConditionManager& BioGears::GetConditions() const { return *m_Conditions; }
const BioGearsCircuits& BioGears::GetCircuits() const { return *m_Circuits; }
const BioGearsCompartments& BioGears::GetCompartments() const { return *m_Compartments; }
const BioGearsConfiguration& BioGears::GetConfiguration() const { return *m_Config; }
const SEScalarTime& BioGears::GetEngineTime() const { return *m_CurrentTime; }
const SEScalarTime& BioGears::GetSimulationTime() const { return *m_SimulationTime; }
const SEScalarTime& BioGears::GetTimeStep() const { return m_Config->GetTimeStep(); }
const CDM::enumBioGearsAirwayMode::value BioGears::GetAirwayMode() const { return m_AirwayMode; }
const CDM::enumOnOff::value BioGears::GetIntubation() const { return m_Intubation; }

void BioGears::AtSteadyState(EngineState state)
{
  m_State = state;
  m_Environment->AtSteadyState();
  m_NervousSystem->AtSteadyState();
  m_CardiovascularSystem->AtSteadyState();
  m_Inhaler->AtSteadyState();
  m_RespiratorySystem->AtSteadyState();
  m_AnesthesiaMachine->AtSteadyState();
  m_GastrointestinalSystem->AtSteadyState();
  m_HepaticSystem->AtSteadyState();
  m_RenalSystem->AtSteadyState();
  m_EnergySystem->AtSteadyState();
  m_EndocrineSystem->AtSteadyState();
  m_DrugSystem->AtSteadyState();
  m_TissueSystem->AtSteadyState();
  m_BloodChemistrySystem->AtSteadyState();
  m_ECG->AtSteadyState();
}

void BioGears::PreProcess()
{
  m_Environment->PreProcess();
  m_NervousSystem->PreProcess();
  m_CardiovascularSystem->PreProcess();
  m_Inhaler->PreProcess();
  m_RespiratorySystem->PreProcess();
  m_AnesthesiaMachine->PreProcess();
  m_GastrointestinalSystem->PreProcess();
  m_HepaticSystem->PreProcess();
  m_RenalSystem->PreProcess();
  m_EnergySystem->PreProcess();
  m_EndocrineSystem->PreProcess();
  m_DrugSystem->PreProcess();
  m_TissueSystem->PreProcess();
  m_BloodChemistrySystem->PreProcess();
  m_ECG->PreProcess();
}
void BioGears::Process()
{
  m_Environment->Process();
  m_NervousSystem->Process();
  m_CardiovascularSystem->Process();
  m_Inhaler->Process();
  m_RespiratorySystem->Process();
  m_AnesthesiaMachine->Process();
  m_GastrointestinalSystem->Process();
  m_HepaticSystem->Process();
  m_RenalSystem->Process();
  m_EnergySystem->Process();
  m_EndocrineSystem->Process();
  m_DrugSystem->Process();
  m_TissueSystem->Process();
  m_BloodChemistrySystem->Process();
  m_ECG->Process();
}
void BioGears::PostProcess()
{
  m_Environment->PostProcess();
  m_NervousSystem->PostProcess();
  m_CardiovascularSystem->PostProcess();
  m_Inhaler->PostProcess();
  m_RespiratorySystem->PostProcess();
  m_AnesthesiaMachine->PostProcess();
  m_GastrointestinalSystem->PostProcess();
  m_HepaticSystem->PostProcess();
  m_RenalSystem->PostProcess();
  m_EnergySystem->PostProcess();
  m_EndocrineSystem->PostProcess();
  m_DrugSystem->PostProcess();
  m_TissueSystem->PostProcess();
  m_BloodChemistrySystem->PostProcess();
  m_ECG->PostProcess();
}

bool BioGears::GetPatientAssessment(SEPatientAssessment& assessment)
{
  SEPulmonaryFunctionTest* pft = dynamic_cast<SEPulmonaryFunctionTest*>(&assessment);
  if (pft != nullptr) {
    return m_RespiratorySystem->CalculatePulmonaryFunctionTest(*pft);
  }

  SECompleteBloodCount* cbc = dynamic_cast<SECompleteBloodCount*>(&assessment);
  if (cbc != nullptr) {
    return m_BloodChemistrySystem->CalculateCompleteBloodCount(*cbc);
  }

  SEComprehensiveMetabolicPanel* cmp = dynamic_cast<SEComprehensiveMetabolicPanel*>(&assessment);
  if (cmp != nullptr) {
    return m_BloodChemistrySystem->CalculateComprehensiveMetabolicPanel(*cmp);
  }

  SEUrinalysis* u = dynamic_cast<SEUrinalysis*>(&assessment);
  if (u != nullptr) {
    return m_RenalSystem->CalculateUrinalysis(*u);
  }

  Error("Unsupported patient assessment");
  return false;
}

void BioGears::ForwardFatal(const std::string& msg, const std::string& origin)
{
  std::string err;
  err.append(msg);
  err.append(" ");
  err.append(origin);
  throw PhysiologyEngineException(err);
}

bool BioGears::CreateCircuitsAndCompartments()
{
  m_Circuits->Clear();
  m_Compartments->Clear();

  SetupCardiovascularLite();
  if (m_Config->IsRenalEnabled()) {
    SetupRenalLite();
  }
  if (m_Config->IsTissueEnabled()) {
    SetupTissue();
  }
  SetupGastrointestinal();

  ///////////////////////////////////////////////////////////////////
  // Create and Combine External and Internal Temperature Circuits //
  ///////////////////////////////////////////////////////////////////

  SetupLiteTemperature();

  // This node is shared between the respiratory, anesthesia, and inhaler circuits
  SEFluidCircuitNode& Ambient = m_Circuits->CreateFluidNode(BGE::EnvironmentNode::Ambient);
  Ambient.GetNextVolume().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::L);
  Ambient.GetVolumeBaseline().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::L);
  SEGasCompartment& gEnvironment = m_Compartments->CreateGasCompartment(BGE::EnvironmentCompartment::Ambient);
  gEnvironment.MapNode(Ambient);
  SELiquidCompartment& lEnvironment = m_Compartments->CreateLiquidCompartment(BGE::EnvironmentCompartment::Ambient);
  lEnvironment.MapNode(Ambient);

  m_Environment->Initialize();
  CDM_COPY((&m_Config->GetInitialEnvironmentalConditions()), (&m_Environment->GetConditions()));
  m_Environment->StateChange();
  // Update the environment pressures on all the 'air' circuits to match what the environment was set to
  gEnvironment.GetPressure().Set(m_Environment->GetConditions().GetAtmosphericPressure());

  SetupRespiratory();
  SetupAnesthesiaMachine();
  SetupInhaler();
  SetupMechanicalVentilator();

  m_Compartments->StateChange();
  return true;
}

void BioGears::SetupCardiovascularLite()
{
  Info("Setting Up Cardiovascular Lite");
  bool male = m_Patient->GetSex() == CDM::enumSex::Male ? true : false;
  double RightLungRatio = m_Patient->GetRightLungRatio().GetValue();
  double LeftLungRatio = 1 - RightLungRatio;
  double bloodVolume_mL = m_Patient->GetBloodVolumeBaseline(VolumeUnit::mL);

  double systolicPressureTarget_mmHg = m_Patient->GetSystolicArterialPressureBaseline(PressureUnit::mmHg);
  double heartRate_bpm = m_Patient->GetHeartRateBaseline(FrequencyUnit::Per_min);
  double strokeVolumeTarget_mL = 81.0; //Note:  Had set this to 87 in previous commit when vascular->interstitium resistances were different
  double cardiacOutputTarget_mL_Per_s = heartRate_bpm / 60.0 * strokeVolumeTarget_mL;
  double diastolicPressureTarget_mmHg = 80.0;
  double centralVenousPressureTarget_mmHg = 4.0;
  double pulmonaryShuntFractionFactor = 0.009; // Used to set the pulmonary shunt fraction. Actual shunt will be roughly double this value (two lungs).
  // The way this works is we compute resistances and compliances based on the hemodynamic variables above that are either in the patient
  // file or we use the defaults if nothing is there. Because the actual impedance depends on the frequency, the computations assume a resting heart rate.
  // So if a user needs to put pressures in the patient file assuming that the pts baseline hr is in the normal range (around 72).
  // If someone wants a patient with a high hr because s/he is exercising or something, then they need to use the action.
  // If a user wants a patient with a ridiculously high resting hr, then they will need to estimate what the pressures and CO would be if the pt had a normal resting hr.

  // We compute a tuning modifier to adjust some baseline resistances and compliances to get closer to the target systolic and diastolic pressures from the patient file
  // The tuning method in cardiovascular will do the fine tuning. This just speeds up the process.
  /// \todo Make these a function of the systolic and diastolic pressure by fitting a curve to the data from the variations test
  double systemicResistanceModifier = 0.849;
  double largeArteriesComplianceModifier = 0.4333;

  // Volume fractions and flow rates from \cite valtin1995renal
  // Pressure targets derived from information available in \cite guyton2006medical and \cite van2013davis
  double VolumeFractionAorta = 0.05, VascularPressureTargetAorta = 1.0 * systolicPressureTarget_mmHg, VascularFlowTargetAorta = 1.0 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionArmLeft = 0.01, VascularPressureTargetArmLeft = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetArmLeft = male ? 0.00724 * cardiacOutputTarget_mL_Per_s : 0.0083 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionArmRight = VolumeFractionArmLeft, VascularPressureTargetArmRight = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetArmRight = VascularFlowTargetArmLeft;
  double VolumeFractionBone = 0.07, VascularPressureTargetBone = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetBone = 0.05 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionBrain = 0.012, VascularPressureTargetBrain = 0.08 * systolicPressureTarget_mmHg, VascularFlowTargetBrain = 0.12 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionFat = male ? 0.05 : 0.085, VascularPressureTargetFat = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetFat = male ? 0.05 * cardiacOutputTarget_mL_Per_s : 0.085 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionHeartLeft = 0.0025, VascularPressureTargetHeartLeft = 1.06667 * systolicPressureTarget_mmHg; /*No flow targets heart right*/
  double VolumeFractionHeartRight = 0.0025, VascularPressureTargetHeartRight = 0.16667 * systolicPressureTarget_mmHg; /*No flow targets heart left*/
  double VolumeFractionKidney = 0.0202, VascularPressureTargetKidney = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetKidney = male ? 0.098 * cardiacOutputTarget_mL_Per_s : 0.088 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionLargeIntestine = 0.019, VascularPressureTargetLargeIntestine = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetLargeIntestine = male ? 0.04 * cardiacOutputTarget_mL_Per_s : 0.05 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionLegLeft = 0.0151, VascularPressureTargetLegLeft = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetLegLeft = male ? 0.01086 * cardiacOutputTarget_mL_Per_s : 0.01245 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionLegRight = VolumeFractionLegLeft, VascularPressureTargetLegRight = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetLegRight = VascularFlowTargetLegLeft;
  double VolumeFractionLiver = 0.106, VascularPressureTargetLiver = 0.25 * systolicPressureTarget_mmHg, VascularFlowTargetLiver = 0.075 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionMuscle = male ? 0.14 : 0.105, VascularPressureTargetMuscle = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetMuscle = male ? 0.17 * cardiacOutputTarget_mL_Per_s : 0.12 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionMyocardium = 0.007, VascularPressureTargetMyocardium = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetMyocardium = male ? 0.04 * cardiacOutputTarget_mL_Per_s : 0.05 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionPulmArtRight = 0.034 * RightLungRatio, VascularPressureTargetPulmArtRight = 0.13333 * systolicPressureTarget_mmHg, VascularFlowTargetPulmArtRight = RightLungRatio * cardiacOutputTarget_mL_Per_s * (1 - pulmonaryShuntFractionFactor);
  double VolumeFractionPulmCapRight = 0.023 * RightLungRatio, VascularPressureTargetPulmCapRight = 0.0650 * systolicPressureTarget_mmHg, VascularFlowTargetPulmCapRight = RightLungRatio * cardiacOutputTarget_mL_Per_s * (1 - pulmonaryShuntFractionFactor);
  double VolumeFractionPulmVeinsRight = 0.068 * RightLungRatio, VascularPressureTargetPulmVeinsRight = 0.03846 * systolicPressureTarget_mmHg, VascularFlowTargetPulmVeinsRight = RightLungRatio * cardiacOutputTarget_mL_Per_s * (1 - pulmonaryShuntFractionFactor);
  double VolumeFractionPulmArtLeft = 0.034 * LeftLungRatio, VascularPressureTargetPulmArtLeft = 0.13333 * systolicPressureTarget_mmHg, VascularFlowTargetPulmArtLeft = LeftLungRatio * cardiacOutputTarget_mL_Per_s * (1 - pulmonaryShuntFractionFactor);
  double VolumeFractionPulmCapLeft = 0.023 * LeftLungRatio, VascularPressureTargetPulmCapLeft = 0.0650 * systolicPressureTarget_mmHg, VascularFlowTargetPulmCapLeft = LeftLungRatio * cardiacOutputTarget_mL_Per_s * (1 - pulmonaryShuntFractionFactor);
  double VolumeFractionPulmVeinsLeft = 0.068 * LeftLungRatio, VascularPressureTargetPulmVeinsLeft = 0.03846 * systolicPressureTarget_mmHg, VascularFlowTargetPulmVeinsLeft = LeftLungRatio * cardiacOutputTarget_mL_Per_s * (1 - pulmonaryShuntFractionFactor);
  double VolumeFractionSkin = 0.032, VascularPressureTargetSkin = 0.0833 * systolicPressureTarget_mmHg, VascularFlowTargetSkin = 0.067 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionSmallIntestine = 0.038, VascularPressureTargetSmallIntestine = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetSmallIntestine = male ? 0.1 * cardiacOutputTarget_mL_Per_s : 0.11 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionSplanchnic = 0.0116, VascularPressureTargetSplanchnic = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetSplanchnic = male ? 0.0258 * cardiacOutputTarget_mL_Per_s : 0.0255 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionSpleen = 0.014, VascularPressureTargetSpleen = 0.33 * systolicPressureTarget_mmHg, VascularFlowTargetSpleen = 0.03 * cardiacOutputTarget_mL_Per_s;
  double VolumeFractionVenaCava = 0.247, VascularPressureTargetVenaCava = 0.0333 * systolicPressureTarget_mmHg, VascularFlowTargetVenaCava = 1.0 * cardiacOutputTarget_mL_Per_s;
  /*Portal Vein is path only*/ double VascularPressureTargetPortalVein = 0.25 * systolicPressureTarget_mmHg, VascularFlowTargetPortalVein = VascularFlowTargetLargeIntestine + VascularFlowTargetSmallIntestine + VascularFlowTargetSplanchnic + VascularFlowTargetSpleen;

  // Compute resistances from mean flow rates and pressure targets
  double ResistanceAorta = (VascularPressureTargetHeartLeft - systolicPressureTarget_mmHg) / VascularFlowTargetAorta; /*No Downstream Resistance Aorta*/
  double ResistanceArmLeft = (systolicPressureTarget_mmHg - VascularPressureTargetArmLeft) / VascularFlowTargetArmLeft, ResistanceArmLeftVenous = (VascularPressureTargetArmLeft - VascularPressureTargetVenaCava) / VascularFlowTargetArmLeft;
  double ResistanceArmRight = ResistanceArmLeft, ResistanceArmRightVenous = ResistanceArmLeftVenous;
  double ResistanceBone = (systolicPressureTarget_mmHg - VascularPressureTargetBone) / VascularFlowTargetBone, ResistanceBoneVenous = (VascularPressureTargetBone - VascularPressureTargetVenaCava) / VascularFlowTargetBone;
  double ResistanceBrain = 0.94 * ((systolicPressureTarget_mmHg - VascularPressureTargetBrain) / VascularFlowTargetBrain), ResistanceBrainVenous = (VascularPressureTargetBrain - VascularPressureTargetVenaCava) / VascularFlowTargetBrain;
  double ResistanceFat = (systolicPressureTarget_mmHg - VascularPressureTargetFat) / VascularFlowTargetFat, ResistanceFatVenous = (VascularPressureTargetFat - VascularPressureTargetVenaCava) / VascularFlowTargetFat;
  double ResistanceHeartLeft = 0.000002; /*No Downstream Resistance HeartLeft*/
  double ResistanceHeartRight = (0.04225 * systolicPressureTarget_mmHg - VascularPressureTargetVenaCava) / cardiacOutputTarget_mL_Per_s; // Describes the flow resistance between the systemic vasculature and the right atrium    /*No Downstream Resistance Heart Right*/
  double ResistanceKidney = (systolicPressureTarget_mmHg - VascularPressureTargetKidney) / VascularFlowTargetKidney, ResistanceKidneyVenous = (VascularPressureTargetKidney - VascularPressureTargetVenaCava) / VascularFlowTargetKidney;
  double ResistanceLargeIntestine = (systolicPressureTarget_mmHg - VascularPressureTargetLargeIntestine) / VascularFlowTargetLargeIntestine, ResistanceLargeIntestineVenous = (VascularPressureTargetLargeIntestine - VascularPressureTargetLiver) / VascularFlowTargetLargeIntestine;
  double ResistanceLegLeft = (systolicPressureTarget_mmHg - VascularPressureTargetLegLeft) / VascularFlowTargetLegLeft, ResistanceLegLeftVenous = (VascularPressureTargetLegLeft - VascularPressureTargetVenaCava) / VascularFlowTargetLegLeft;
  double ResistanceLegRight = ResistanceLegLeft, ResistanceLegRightVenous = ResistanceLegLeftVenous;
  double ResistanceLiver = (systolicPressureTarget_mmHg - VascularPressureTargetLiver) / VascularFlowTargetLiver, ResistanceLiverVenous = (VascularPressureTargetLiver - VascularPressureTargetVenaCava) / (VascularFlowTargetLiver + VascularFlowTargetPortalVein);
  double ResistanceMuscle = (systolicPressureTarget_mmHg - VascularPressureTargetMuscle) / VascularFlowTargetMuscle, ResistanceMuscleVenous = (VascularPressureTargetMuscle - VascularPressureTargetVenaCava) / VascularFlowTargetMuscle;
  double ResistanceMyocardium = (systolicPressureTarget_mmHg - VascularPressureTargetMyocardium) / VascularFlowTargetMyocardium, ResistanceMyocardiumVenous = (VascularPressureTargetMyocardium - VascularPressureTargetVenaCava) / VascularFlowTargetMyocardium;
  double ResistancePulmArtRight = (VascularPressureTargetHeartRight - VascularPressureTargetPulmArtRight) / VascularFlowTargetPulmArtRight; /*No Downstream Resistance PulmArt*/
  double ResistancePulmCapRight = (VascularPressureTargetPulmArtRight - VascularPressureTargetPulmCapRight) / VascularFlowTargetPulmCapRight; /*No Downstream Resistance PulmCap*/
  double ResistancePulmVeinsRight = (VascularPressureTargetPulmCapRight - VascularPressureTargetPulmVeinsRight) / VascularFlowTargetPulmVeinsRight; /*No Downstream Resistance PulmVeins*/
  double ResistancePulmArtLeft = (VascularPressureTargetHeartRight - VascularPressureTargetPulmArtLeft) / VascularFlowTargetPulmArtLeft; /*No Downstream Resistance PulmArt*/
  double ResistancePulmCapLeft = (VascularPressureTargetPulmArtLeft - VascularPressureTargetPulmCapLeft) / VascularFlowTargetPulmCapLeft; /*No Downstream Resistance PulmCap*/
  double ResistancePulmVeinsLeft = (VascularPressureTargetPulmCapLeft - VascularPressureTargetPulmVeinsLeft) / VascularFlowTargetPulmVeinsLeft; /*No Downstream Resistance PulmVeins*/
  double ResistanceSkin = (systolicPressureTarget_mmHg - VascularPressureTargetSkin) / VascularFlowTargetSkin, ResistanceSkinVenous = (VascularPressureTargetSkin - VascularPressureTargetVenaCava) / VascularFlowTargetSkin;
  double ResistanceSmallIntestine = (systolicPressureTarget_mmHg - VascularPressureTargetSmallIntestine) / VascularFlowTargetSmallIntestine, ResistanceSmallIntestineVenous = (VascularPressureTargetSmallIntestine - VascularPressureTargetLiver) / VascularFlowTargetSmallIntestine;
  double ResistanceSplanchnic = (systolicPressureTarget_mmHg - VascularPressureTargetSplanchnic) / VascularFlowTargetSplanchnic, ResistanceSplanchnicVenous = (VascularPressureTargetSplanchnic - VascularPressureTargetLiver) / VascularFlowTargetSplanchnic;
  double ResistanceSpleen = (systolicPressureTarget_mmHg - VascularPressureTargetSpleen) / VascularFlowTargetSpleen, ResistanceSpleenVenous = (VascularPressureTargetSpleen - VascularPressureTargetLiver) / VascularFlowTargetSpleen;

  // Portal vein and shunt are just paths - only have resistance
  double ResistancePortalVein = 0.001; // The portal vein is just a pathway in BioGears. The pressure across this path does not represent portal vein pressure (if it did our patient would always be portal hypertensive)
  double ResistanceShuntRight = (VascularPressureTargetPulmArtRight - VascularPressureTargetPulmCapRight) / (cardiacOutputTarget_mL_Per_s * pulmonaryShuntFractionFactor);
  double ResistanceShuntLeft = (VascularPressureTargetPulmArtLeft - VascularPressureTargetPulmCapLeft) / (cardiacOutputTarget_mL_Per_s * pulmonaryShuntFractionFactor);

  // Make a circuit
  SEFluidCircuit& cCardiovascular = m_Circuits->GetCardiovascularCircuit();
  SELiquidCompartmentGraph& gCardiovascular = m_Compartments->GetCardiovascularGraph();

  //Use baseline parameters defined for main circuit to derive values for Cardiovascular Lite circuit
  //Combined volume fractions
  double VolumeFractionArms = VolumeFractionArmLeft + VolumeFractionArmRight;
  double VolumeFractionLegs = VolumeFractionLegLeft + VolumeFractionLegRight;
  double VolumeFractionGut = VolumeFractionLargeIntestine + VolumeFractionSmallIntestine + VolumeFractionSpleen + VolumeFractionSplanchnic;
  //Combined pressure targets
  double VascularPressureTargetArms = VascularPressureTargetArmLeft; //Left and right arm have same target
  double VascularPressureTargetLegs = VascularPressureTargetLegLeft; //Left and right leg have same target
  double VascularPressureTargetGut = VascularPressureTargetSpleen; //Spleen, small intestine, large intestine, splanchnic have same pressure target
  //Combined resistances--resistors in parallel use (1/R_combined) = (1/R1) + (1/R2)
  double ResistanceArms = 1.0 / ((1.0 / ResistanceArmLeft) + (1.0 / ResistanceArmRight));
  double ResistanceArmsVenous = 1.0 / ((1.0 / ResistanceArmLeftVenous) + (1.0 / ResistanceArmRightVenous));
  double ResistanceLegs = 1.0 / ((1.0 / ResistanceLegLeft) + (1.0 / ResistanceLegRight));
  double ResistanceLegsVenous = 1.0 / ((1.0 / ResistanceLegLeftVenous) + (1.0 / ResistanceLegRightVenous));
  double ResistanceGut = 1.0 / ((1.0 / ResistanceLargeIntestine) + (1.0 / ResistanceSmallIntestine) + (1.0 / ResistanceSpleen) + (1.0 / ResistanceSplanchnic));
  double ResistanceGutVenous = 1.0 / ((1.0 / ResistanceLargeIntestineVenous) + (1.0 / ResistanceSmallIntestineVenous) + (1.0 / ResistanceSpleenVenous) + (1.0 / ResistanceSplanchnicVenous));

  SEFluidCircuitNode& RightHeart1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::RightHeart1);
  RightHeart1.GetPressure().SetValue(0.0, PressureUnit::mmHg);
  SEFluidCircuitNode& RightHeart2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::RightHeart2);
  SEFluidCircuitNode& RightHeart3 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::RightHeart3);
  RightHeart3.GetPressure().SetValue(0.0, PressureUnit::mmHg);
  RightHeart1.GetVolumeBaseline().SetValue(VolumeFractionHeartRight * bloodVolume_mL, VolumeUnit::mL);

  SEFluidCircuitNode& MainPulmonaryArteries = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::MainPulmonaryArteries);

  SEFluidCircuitNode& RightIntermediatePulmonaryArteries = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::RightIntermediatePulmonaryArteries);
  SEFluidCircuitNode& RightPulmonaryArteries = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::RightPulmonaryArteries);
  RightPulmonaryArteries.GetVolumeBaseline().SetValue(VolumeFractionPulmArtRight * bloodVolume_mL, VolumeUnit::mL);
  RightPulmonaryArteries.GetPressure().SetValue(VascularPressureTargetPulmArtRight, PressureUnit::mmHg);

  SEFluidCircuitNode& LeftIntermediatePulmonaryArteries = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::LeftIntermediatePulmonaryArteries);
  SEFluidCircuitNode& LeftPulmonaryArteries = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::LeftPulmonaryArteries);
  LeftPulmonaryArteries.GetVolumeBaseline().SetValue(VolumeFractionPulmArtLeft * bloodVolume_mL, VolumeUnit::mL);
  LeftPulmonaryArteries.GetPressure().SetValue(VascularPressureTargetPulmArtLeft, PressureUnit::mmHg);

  SEFluidCircuitNode& RightPulmonaryCapillaries = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::RightPulmonaryCapillaries);
  RightPulmonaryCapillaries.GetVolumeBaseline().SetValue(VolumeFractionPulmCapRight * bloodVolume_mL, VolumeUnit::mL);
  RightPulmonaryCapillaries.GetPressure().SetValue(VascularPressureTargetPulmCapRight, PressureUnit::mmHg);

  SEFluidCircuitNode& LeftPulmonaryCapillaries = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::LeftPulmonaryCapillaries);
  LeftPulmonaryCapillaries.GetVolumeBaseline().SetValue(VolumeFractionPulmCapLeft * bloodVolume_mL, VolumeUnit::mL);
  LeftPulmonaryCapillaries.GetPressure().SetValue(VascularPressureTargetPulmCapLeft, PressureUnit::mmHg);

  SEFluidCircuitNode& RightIntermediatePulmonaryVeins = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::RightIntermediatePulmonaryVeins);
  SEFluidCircuitNode& RightPulmonaryVeins = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::RightPulmonaryVeins);
  RightPulmonaryVeins.GetVolumeBaseline().SetValue(VolumeFractionPulmVeinsRight * bloodVolume_mL, VolumeUnit::mL);
  RightPulmonaryVeins.GetPressure().SetValue(VascularPressureTargetPulmVeinsRight, PressureUnit::mmHg);

  SEFluidCircuitNode& LeftIntermediatePulmonaryVeins = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::LeftIntermediatePulmonaryVeins);
  SEFluidCircuitNode& LeftPulmonaryVeins = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::LeftPulmonaryVeins);
  LeftPulmonaryVeins.GetVolumeBaseline().SetValue(VolumeFractionPulmVeinsLeft * bloodVolume_mL, VolumeUnit::mL);
  LeftPulmonaryVeins.GetPressure().SetValue(VascularPressureTargetPulmVeinsLeft, PressureUnit::mmHg);

  SEFluidCircuitNode& LeftHeart1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::LeftHeart1);
  LeftHeart1.GetPressure().SetValue(0.0, PressureUnit::mmHg);
  SEFluidCircuitNode& LeftHeart2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::LeftHeart2);
  SEFluidCircuitNode& LeftHeart3 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::LeftHeart3);
  LeftHeart3.GetPressure().SetValue(0.0, PressureUnit::mmHg);
  LeftHeart1.GetVolumeBaseline().SetValue(VolumeFractionHeartLeft * bloodVolume_mL, VolumeUnit::mL);

  SEFluidCircuitNode& Aorta1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Aorta1);
  SEFluidCircuitNode& Aorta2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Aorta2);
  SEFluidCircuitNode& Aorta3 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Aorta3);
  Aorta1.GetVolumeBaseline().SetValue(VolumeFractionAorta * bloodVolume_mL, VolumeUnit::mL);
  Aorta1.GetPressure().SetValue(VascularPressureTargetAorta, PressureUnit::mmHg);

  SEFluidCircuitNode& Arms1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Arms1);
  SEFluidCircuitNode& Arms2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Arms2);
  Arms1.GetVolumeBaseline().SetValue(VolumeFractionArms * bloodVolume_mL, VolumeUnit::mL);
  Arms1.GetPressure().SetValue(VascularPressureTargetArms, PressureUnit::mmHg);

  SEFluidCircuitNode& Brain1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Brain1);
  SEFluidCircuitNode& Brain2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Brain2);
  Brain1.GetVolumeBaseline().SetValue(VolumeFractionBrain * bloodVolume_mL, VolumeUnit::mL);
  Brain1.GetPressure().SetValue(0.0, PressureUnit::mmHg);
  Brain1.GetPressure().SetValue(VascularPressureTargetBrain, PressureUnit::mmHg);

  SEFluidCircuitNode& Bone1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Bone1);
  SEFluidCircuitNode& Bone2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Bone2);
  Bone1.GetVolumeBaseline().SetValue(VolumeFractionBone * bloodVolume_mL, VolumeUnit::mL);
  Bone1.GetPressure().SetValue(VascularPressureTargetBone, PressureUnit::mmHg);

  SEFluidCircuitNode& Fat1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Fat1);
  SEFluidCircuitNode& Fat2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Fat2);
  Fat1.GetVolumeBaseline().SetValue(VolumeFractionFat * bloodVolume_mL, VolumeUnit::mL);
  Fat1.GetPressure().SetValue(VascularPressureTargetFat, PressureUnit::mmHg);

  SEFluidCircuitNode& Gut = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Gut1);
  Gut.GetVolumeBaseline().SetValue(VolumeFractionGut * bloodVolume_mL, VolumeUnit::mL);
  Gut.GetPressure().SetValue(VascularPressureTargetGut, PressureUnit::mmHg);

  SEFluidCircuitNode& Liver1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Liver1);
  SEFluidCircuitNode& Liver2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Liver2);
  Liver1.GetVolumeBaseline().SetValue(VolumeFractionLiver * bloodVolume_mL, VolumeUnit::mL);
  Liver1.GetPressure().SetValue(VascularPressureTargetLiver, PressureUnit::mmHg);

  SEFluidCircuitNode& LeftKidney1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::LeftKidney1);
  SEFluidCircuitNode& LeftKidney2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::LeftKidney2);
  LeftKidney1.GetVolumeBaseline().SetValue(0.5 * VolumeFractionKidney * bloodVolume_mL, VolumeUnit::mL);
  LeftKidney1.GetPressure().SetValue(VascularPressureTargetKidney, PressureUnit::mmHg);

  SEFluidCircuitNode& Legs1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Legs1);
  SEFluidCircuitNode& Legs2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Legs2);
  Legs1.GetVolumeBaseline().SetValue(VolumeFractionLegs * bloodVolume_mL, VolumeUnit::mL);
  Legs1.GetPressure().SetValue(VascularPressureTargetLegs, PressureUnit::mmHg);

  SEFluidCircuitNode& Muscle1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Muscle1);
  SEFluidCircuitNode& Muscle2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Muscle2);
  Muscle1.GetVolumeBaseline().SetValue(VolumeFractionMuscle * bloodVolume_mL, VolumeUnit::mL);
  Muscle1.GetPressure().SetValue(VascularPressureTargetMuscle, PressureUnit::mmHg);

  SEFluidCircuitNode& Myocardium1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Myocardium1);
  SEFluidCircuitNode& Myocardium2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Myocardium2);
  Myocardium1.GetVolumeBaseline().SetValue(VolumeFractionMyocardium * bloodVolume_mL, VolumeUnit::mL);
  Myocardium1.GetPressure().SetValue(VascularPressureTargetMyocardium, PressureUnit::mmHg);

  SEFluidCircuitNode& PortalVein = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::PortalVein1);

  SEFluidCircuitNode& RightKidney1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::RightKidney1);
  SEFluidCircuitNode& RightKidney2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::RightKidney2);
  RightKidney1.GetVolumeBaseline().SetValue(0.5 * VolumeFractionKidney * bloodVolume_mL, VolumeUnit::mL);
  RightKidney1.GetPressure().SetValue(VascularPressureTargetKidney, PressureUnit::mmHg);

  SEFluidCircuitNode& Skin1 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Skin1);
  SEFluidCircuitNode& Skin2 = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Skin2);
  Skin1.GetVolumeBaseline().SetValue(VolumeFractionSkin * bloodVolume_mL, VolumeUnit::mL);
  Skin1.GetPressure().SetValue(VascularPressureTargetSkin, PressureUnit::mmHg);

  SEFluidCircuitNode& VenaCava = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::VenaCava);
  VenaCava.GetVolumeBaseline().SetValue(VolumeFractionVenaCava * bloodVolume_mL, VolumeUnit::mL);
  VenaCava.GetPressure().SetValue(VascularPressureTargetVenaCava, PressureUnit::mmHg);

  SEFluidCircuitNode& Ground = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Ground);
  cCardiovascular.AddReferenceNode(Ground);
  Ground.GetPressure().SetValue(0.0, PressureUnit::mmHg);

  double blood_mL = 0;
  for (SEFluidCircuitNode* n : cCardiovascular.GetNodes()) {
    if (n->HasVolumeBaseline()) {
      blood_mL += n->GetVolumeBaseline(VolumeUnit::mL);
    }
  }
  if (blood_mL > bloodVolume_mL) {
    Error("Blood volume greater than total blood volume");
  }

  //SEFluidCircuitNode& Pericardium = cCardiovascular.CreateNode(BGE::CardiovascularLiteNode::Pericardium1);
  //Pericardium.GetVolumeBaseline().SetValue(15.0, VolumeUnit::mL);
  //Pericardium.GetPressure().SetValue(1.0, PressureUnit::mmHg);

  // Create Paths, set switch (diodes), compliances, and resistances where appropriate
  SEFluidCircuitPath& VenaCavaToRightHeart2 = cCardiovascular.CreatePath(VenaCava, RightHeart2, BGE::CardiovascularLitePath::VenaCavaToRightHeart2);
  VenaCavaToRightHeart2.GetResistanceBaseline().SetValue(ResistanceHeartRight, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& RightHeart2ToRightHeart1 = cCardiovascular.CreatePath(RightHeart2, RightHeart1, BGE::CardiovascularLitePath::RightHeart2ToRightHeart1);
  RightHeart2ToRightHeart1.SetNextValve(CDM::enumOpenClosed::Closed);
  SEFluidCircuitPath& RightHeart1ToRightHeart3 = cCardiovascular.CreatePath(RightHeart1, RightHeart3, BGE::CardiovascularLitePath::RightHeart1ToRightHeart3);
  SEFluidCircuitPath& RightHeart3ToGround = cCardiovascular.CreatePath(Ground, RightHeart3, BGE::CardiovascularLitePath::RightHeart3ToGround);
  RightHeart3ToGround.GetPressureSourceBaseline().SetValue(0.0, PressureUnit::mmHg);

  SEFluidCircuitPath& RightHeart1ToMainPulmonaryArteries = cCardiovascular.CreatePath(RightHeart1, MainPulmonaryArteries, BGE::CardiovascularLitePath::RightHeart1ToMainPulmonaryArteries);
  RightHeart1ToMainPulmonaryArteries.SetNextValve(CDM::enumOpenClosed::Closed);

  SEFluidCircuitPath& MainPulmonaryArteriesToRightIntermediatePulmonaryArteries = cCardiovascular.CreatePath(MainPulmonaryArteries, RightIntermediatePulmonaryArteries, BGE::CardiovascularLitePath::MainPulmonaryArteriesToRightIntermediatePulmonaryArteries);
  //MainPulmonaryArteriesToRightIntermediatePulmonaryArteries.SetNextValve(CDM::enumOpenClosed::Closed);
  SEFluidCircuitPath& RightIntermediatePulmonaryArteriesToRightPulmonaryArteries = cCardiovascular.CreatePath(RightIntermediatePulmonaryArteries, RightPulmonaryArteries, BGE::CardiovascularLitePath::RightIntermediatePulmonaryArteriesToRightPulmonaryArteries);
  RightIntermediatePulmonaryArteriesToRightPulmonaryArteries.GetResistanceBaseline().SetValue(ResistancePulmArtLeft, FlowResistanceUnit::mmHg_s_Per_mL);

  SEFluidCircuitPath& RightPulmonaryArteriesToRightPulmonaryVeins = cCardiovascular.CreatePath(RightPulmonaryArteries, RightPulmonaryVeins, BGE::CardiovascularLitePath::RightPulmonaryArteriesToRightPulmonaryVeins);
  RightPulmonaryArteriesToRightPulmonaryVeins.GetResistanceBaseline().SetValue(ResistanceShuntRight, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& RightPulmonaryArteriesToRightPulmonaryCapillaries = cCardiovascular.CreatePath(RightPulmonaryArteries, RightPulmonaryCapillaries, BGE::CardiovascularLitePath::RightPulmonaryArteriesToRightPulmonaryCapillaries);
  RightPulmonaryArteriesToRightPulmonaryCapillaries.GetResistanceBaseline().SetValue(ResistancePulmCapRight, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& RightPulmonaryArteriesToGround = cCardiovascular.CreatePath(RightPulmonaryArteries, Ground, BGE::CardiovascularLitePath::RightPulmonaryArteriesToGround);
  RightPulmonaryArteriesToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& RightPulmonaryCapillariesToRightPulmonaryVeins = cCardiovascular.CreatePath(RightPulmonaryCapillaries, RightPulmonaryVeins, BGE::CardiovascularLitePath::RightPulmonaryCapillariesToRightPulmonaryVeins);
  RightPulmonaryCapillariesToRightPulmonaryVeins.GetResistanceBaseline().SetValue(ResistancePulmVeinsRight, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& RightPulmonaryCapillariesToGround = cCardiovascular.CreatePath(RightPulmonaryCapillaries, Ground, BGE::CardiovascularLitePath::RightPulmonaryCapillariesToGround);
  RightPulmonaryCapillariesToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& RightPulmonaryVeinsToRightIntermediatePulmonaryVeins = cCardiovascular.CreatePath(RightPulmonaryVeins, RightIntermediatePulmonaryVeins, BGE::CardiovascularLitePath::RightPulmonaryVeinsToRightIntermediatePulmonaryVeins);
  RightPulmonaryVeinsToRightIntermediatePulmonaryVeins.GetResistanceBaseline().SetValue(ResistanceHeartLeft, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& RightPulmonaryVeinsToGround = cCardiovascular.CreatePath(RightPulmonaryVeins, Ground, BGE::CardiovascularLitePath::RightPulmonaryVeinsToGround);
  RightPulmonaryVeinsToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& RightIntermediatePulmonaryVeinsToLeftHeart2 = cCardiovascular.CreatePath(RightIntermediatePulmonaryVeins, LeftHeart2, BGE::CardiovascularLitePath::RightIntermediatePulmonaryVeinsToLeftHeart2);
  //RightIntermediatePulmonaryVeinsToLeftHeart2.SetNextValve(CDM::enumOpenClosed::Closed);

  SEFluidCircuitPath& MainPulmonaryArteriesToLeftIntermediatePulmonaryArteries = cCardiovascular.CreatePath(MainPulmonaryArteries, LeftIntermediatePulmonaryArteries, BGE::CardiovascularLitePath::MainPulmonaryArteriesToLeftIntermediatePulmonaryArteries);
  //MainPulmonaryArteriesToLeftIntermediatePulmonaryArteries.SetNextValve(CDM::enumOpenClosed::Closed);
  SEFluidCircuitPath& LeftIntermediatePulmonaryArteriesToLeftPulmonaryArteries = cCardiovascular.CreatePath(LeftIntermediatePulmonaryArteries, LeftPulmonaryArteries, BGE::CardiovascularLitePath::LeftIntermediatePulmonaryArteriesToLeftPulmonaryArteries);
  LeftIntermediatePulmonaryArteriesToLeftPulmonaryArteries.GetResistanceBaseline().SetValue(ResistancePulmArtLeft, FlowResistanceUnit::mmHg_s_Per_mL);

  SEFluidCircuitPath& LeftPulmonaryArteriesToLeftPulmonaryVeins = cCardiovascular.CreatePath(LeftPulmonaryArteries, LeftPulmonaryVeins, BGE::CardiovascularLitePath::LeftPulmonaryArteriesToLeftPulmonaryVeins);
  LeftPulmonaryArteriesToLeftPulmonaryVeins.GetResistanceBaseline().SetValue(ResistanceShuntLeft, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& LeftPulmonaryArteriesToLeftPulmonaryCapillaries = cCardiovascular.CreatePath(LeftPulmonaryArteries, LeftPulmonaryCapillaries, BGE::CardiovascularLitePath::LeftPulmonaryArteriesToLeftPulmonaryCapillaries);
  LeftPulmonaryArteriesToLeftPulmonaryCapillaries.GetResistanceBaseline().SetValue(ResistancePulmCapLeft, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& LeftPulmonaryArteriesToGround = cCardiovascular.CreatePath(LeftPulmonaryArteries, Ground, BGE::CardiovascularLitePath::LeftPulmonaryArteriesToGround);
  LeftPulmonaryArteriesToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& LeftPulmonaryCapillariesToGround = cCardiovascular.CreatePath(LeftPulmonaryCapillaries, Ground, BGE::CardiovascularLitePath::LeftPulmonaryCapillariesToGround);
  LeftPulmonaryCapillariesToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& LeftPulmonaryCapillariesToLeftPulmonaryVeins = cCardiovascular.CreatePath(LeftPulmonaryCapillaries, LeftPulmonaryVeins, BGE::CardiovascularLitePath::LeftPulmonaryCapillariesToLeftPulmonaryVeins);
  LeftPulmonaryCapillariesToLeftPulmonaryVeins.GetResistanceBaseline().SetValue(ResistancePulmVeinsLeft, FlowResistanceUnit::mmHg_s_Per_mL);

  SEFluidCircuitPath& LeftPulmonaryVeinsToLeftIntermediatePulmonaryVeins = cCardiovascular.CreatePath(LeftPulmonaryVeins, LeftIntermediatePulmonaryVeins, BGE::CardiovascularLitePath::LeftPulmonaryVeinsToLeftIntermediatePulmonaryVeins);
  LeftPulmonaryVeinsToLeftIntermediatePulmonaryVeins.GetResistanceBaseline().SetValue(ResistanceHeartLeft, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& LeftPulmonaryVeinsToGround = cCardiovascular.CreatePath(LeftPulmonaryVeins, Ground, BGE::CardiovascularLitePath::LeftPulmonaryVeinsToGround);
  LeftPulmonaryVeinsToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& LeftIntermediatePulmonaryVeinsToLeftHeart2 = cCardiovascular.CreatePath(LeftIntermediatePulmonaryVeins, LeftHeart2, BGE::CardiovascularLitePath::LeftIntermediatePulmonaryVeinsToLeftHeart2);
  //LeftIntermediatePulmonaryVeinsToLeftHeart2.SetNextValve(CDM::enumOpenClosed::Closed);
  SEFluidCircuitPath& LeftHeart2ToLeftHeart1 = cCardiovascular.CreatePath(LeftHeart2, LeftHeart1, BGE::CardiovascularLitePath::LeftHeart2ToLeftHeart1);
  LeftHeart2ToLeftHeart1.SetNextValve(CDM::enumOpenClosed::Closed);
  SEFluidCircuitPath& LeftHeart1ToLeftHeart3 = cCardiovascular.CreatePath(LeftHeart1, LeftHeart3, BGE::CardiovascularLitePath::LeftHeart1ToLeftHeart3);

  SEFluidCircuitPath& LeftHeart3ToGround = cCardiovascular.CreatePath(Ground, LeftHeart3, BGE::CardiovascularLitePath::LeftHeart3ToGround);
  LeftHeart3ToGround.GetPressureSourceBaseline().SetValue(0.0, PressureUnit::mmHg);
  SEFluidCircuitPath& LeftHeart1ToAorta2 = cCardiovascular.CreatePath(LeftHeart1, Aorta2, BGE::CardiovascularLitePath::LeftHeart1ToAorta2);
  LeftHeart1ToAorta2.SetNextValve(CDM::enumOpenClosed::Closed);
  SEFluidCircuitPath& Aorta2ToAorta3 = cCardiovascular.CreatePath(Aorta2, Aorta3, BGE::CardiovascularLitePath::Aorta2ToAorta3);
  SEFluidCircuitPath& Aorta3ToAorta1 = cCardiovascular.CreatePath(Aorta3, Aorta1, BGE::CardiovascularLitePath::Aorta3ToAorta1);
  Aorta3ToAorta1.GetResistanceBaseline().SetValue(ResistanceAorta, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Aorta1ToGround = cCardiovascular.CreatePath(Aorta1, Ground, BGE::CardiovascularLitePath::Aorta1ToGround);
  Aorta1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& Aorta1ToArms1 = cCardiovascular.CreatePath(Aorta1, Arms1, BGE::CardiovascularLitePath::Aorta1ToArms1);
  Aorta1ToArms1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceArms, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Arms1ToGround = cCardiovascular.CreatePath(Arms1, Ground, BGE::CardiovascularLitePath::Arms1ToGround);
  Arms1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& Arms1ToArms2 = cCardiovascular.CreatePath(Arms1, Arms2, BGE::CardiovascularLitePath::Arms1ToArms2);
  Arms1ToArms2.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceArmsVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Arms2ToVenaCava = cCardiovascular.CreatePath(Arms2, VenaCava, BGE::CardiovascularLitePath::Arms2ToVenaCava);

  SEFluidCircuitPath& Aorta1ToBrain1 = cCardiovascular.CreatePath(Aorta1, Brain1, BGE::CardiovascularLitePath::Aorta1ToBrain1);
  Aorta1ToBrain1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceBrain, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Brain1ToGround = cCardiovascular.CreatePath(Brain1, Ground, BGE::CardiovascularLitePath::Brain1ToGround);
  Brain1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& Brain1ToBrain2 = cCardiovascular.CreatePath(Brain1, Brain2, BGE::CardiovascularLitePath::Brain1ToBrain2);
  Brain1ToBrain2.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceBrainVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Brain2ToVenaCava = cCardiovascular.CreatePath(Brain2, VenaCava, BGE::CardiovascularLitePath::Brain2ToVenaCava);

  SEFluidCircuitPath& Aorta1ToBone1 = cCardiovascular.CreatePath(Aorta1, Bone1, BGE::CardiovascularLitePath::Aorta1ToBone1);
  Aorta1ToBone1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceBone, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Bone1ToGround = cCardiovascular.CreatePath(Bone1, Ground, BGE::CardiovascularLitePath::Bone1ToGround);
  Bone1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& Bone1ToBone2 = cCardiovascular.CreatePath(Bone1, Bone2, BGE::CardiovascularLitePath::Bone1ToBone2);
  Bone1ToBone2.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceBoneVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Bone2ToVenaCava = cCardiovascular.CreatePath(Bone2, VenaCava, BGE::CardiovascularLitePath::Bone2ToVenaCava);

  SEFluidCircuitPath& Aorta1ToFat1 = cCardiovascular.CreatePath(Aorta1, Fat1, BGE::CardiovascularLitePath::Aorta1ToFat1);
  Aorta1ToFat1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceFat, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Fat1ToGround = cCardiovascular.CreatePath(Fat1, Ground, BGE::CardiovascularLitePath::Fat1ToGround);
  Fat1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& Fat1ToFat2 = cCardiovascular.CreatePath(Fat1, Fat2, BGE::CardiovascularLitePath::Fat1ToFat2);
  Fat1ToFat2.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceFatVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Fat2ToVenaCava = cCardiovascular.CreatePath(Fat2, VenaCava, BGE::CardiovascularLitePath::Fat2ToVenaCava);

  SEFluidCircuitPath& Aorta1ToGut = cCardiovascular.CreatePath(Aorta1, Gut, BGE::CardiovascularLitePath::Aorta1ToGut);
  Aorta1ToGut.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceGut, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& GutToGround = cCardiovascular.CreatePath(Gut, Ground, BGE::CardiovascularLitePath::GutToGround);
  GutToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& GutToPortalVein = cCardiovascular.CreatePath(Gut, PortalVein, BGE::CardiovascularLitePath::GutToPortalVein);
  GutToPortalVein.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceGutVenous, FlowResistanceUnit::mmHg_s_Per_mL);

  SEFluidCircuitPath& Aorta1ToLeftKidney1 = cCardiovascular.CreatePath(Aorta1, LeftKidney1, BGE::CardiovascularLitePath::Aorta1ToLeftKidney1);
  Aorta1ToLeftKidney1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceKidney, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& LeftKidney1ToGround = cCardiovascular.CreatePath(LeftKidney1, Ground, BGE::CardiovascularLitePath::LeftKidney1ToGround);
  LeftKidney1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& LeftKidney1ToLeftKidney2 = cCardiovascular.CreatePath(LeftKidney1, LeftKidney2, BGE::CardiovascularLitePath::LeftKidney1ToLeftKidney2);
  LeftKidney1ToLeftKidney2.GetResistanceBaseline().SetValue(ResistanceKidneyVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& LeftKidney2ToVenaCava = cCardiovascular.CreatePath(LeftKidney2, VenaCava, BGE::CardiovascularLitePath::LeftKidney2ToVenaCava);

  SEFluidCircuitPath& Aorta1ToLegs1 = cCardiovascular.CreatePath(Aorta1, Legs1, BGE::CardiovascularLitePath::Aorta1ToLegs1);
  Aorta1ToLegs1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceLegs, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Legs1ToGround = cCardiovascular.CreatePath(Legs1, Ground, BGE::CardiovascularLitePath::Legs1ToGround);
  Legs1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& Legs1ToLegs2 = cCardiovascular.CreatePath(Legs1, Legs2, BGE::CardiovascularLitePath::Legs1ToLegs2);
  Legs1ToLegs2.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceLegsVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Legs2ToVenaCava = cCardiovascular.CreatePath(Legs2, VenaCava, BGE::CardiovascularLitePath::Legs2ToVenaCava);

  SEFluidCircuitPath& Aorta1ToLiver1 = cCardiovascular.CreatePath(Aorta1, Liver1, BGE::CardiovascularLitePath::Aorta1ToLiver1);
  Aorta1ToLiver1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceLiver, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Liver1ToGround = cCardiovascular.CreatePath(Liver1, Ground, BGE::CardiovascularLitePath::Liver1ToGround);
  Liver1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& PortalVeinToLiver1 = cCardiovascular.CreatePath(PortalVein, Liver1, BGE::CardiovascularLitePath::PortalVeinToLiver1);
  PortalVeinToLiver1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistancePortalVein, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Liver1ToLiver2 = cCardiovascular.CreatePath(Liver1, Liver2, BGE::CardiovascularLitePath::Liver1ToLiver2);
  Liver1ToLiver2.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceLiverVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Liver2ToVenaCava = cCardiovascular.CreatePath(Liver2, VenaCava, BGE::CardiovascularLitePath::Liver2ToVenaCava);

  SEFluidCircuitPath& Aorta1ToMuscle1 = cCardiovascular.CreatePath(Aorta1, Muscle1, BGE::CardiovascularLitePath::Aorta1ToMuscle1);
  Aorta1ToMuscle1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceMuscle, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Muscle1ToGround = cCardiovascular.CreatePath(Muscle1, Ground, BGE::CardiovascularLitePath::Muscle1ToGround);
  Muscle1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& Muscle1ToMuscle2 = cCardiovascular.CreatePath(Muscle1, Muscle2, BGE::CardiovascularLitePath::Muscle1ToMuscle2);
  Muscle1ToMuscle2.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceMuscleVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Muscle2ToVenaCava = cCardiovascular.CreatePath(Muscle2, VenaCava, BGE::CardiovascularLitePath::Muscle2ToVenaCava);

  SEFluidCircuitPath& Aorta1ToMyocardium1 = cCardiovascular.CreatePath(Aorta1, Myocardium1, BGE::CardiovascularLitePath::Aorta1ToMyocardium1);
  Aorta1ToMyocardium1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceMyocardium, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Myocardium1ToGround = cCardiovascular.CreatePath(Myocardium1, Ground, BGE::CardiovascularLitePath::Myocardium1ToGround);
  Myocardium1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& Myocardium1ToMyocardium2 = cCardiovascular.CreatePath(Myocardium1, Myocardium2, BGE::CardiovascularLitePath::Myocardium1ToMyocardium2);
  Myocardium1ToMyocardium2.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceMyocardiumVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Myocardium2ToVenaCava = cCardiovascular.CreatePath(Myocardium2, VenaCava, BGE::CardiovascularLitePath::Myocardium2ToVenaCava);

  SEFluidCircuitPath& Aorta1ToRightKidney1 = cCardiovascular.CreatePath(Aorta1, RightKidney1, BGE::CardiovascularLitePath::Aorta1ToRightKidney1);
  Aorta1ToRightKidney1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceKidney, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& RightKidney1ToGround = cCardiovascular.CreatePath(RightKidney1, Ground, BGE::CardiovascularLitePath::RightKidney1ToGround);
  RightKidney1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& RightKidney1ToRightKidney2 = cCardiovascular.CreatePath(RightKidney1, RightKidney2, BGE::CardiovascularLitePath::RightKidney1ToRightKidney2);
  RightKidney1ToRightKidney2.GetResistanceBaseline().SetValue(ResistanceKidneyVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& RightKidney2ToVenaCava = cCardiovascular.CreatePath(RightKidney2, VenaCava, BGE::CardiovascularLitePath::RightKidney2ToVenaCava);

  SEFluidCircuitPath& Aorta1ToSkin1 = cCardiovascular.CreatePath(Aorta1, Skin1, BGE::CardiovascularLitePath::Aorta1ToSkin1);
  Aorta1ToSkin1.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceSkin, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Skin1ToGround = cCardiovascular.CreatePath(Skin1, Ground, BGE::CardiovascularLitePath::Skin1ToGround);
  Skin1ToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& Skin1ToSkin2 = cCardiovascular.CreatePath(Skin1, Skin2, BGE::CardiovascularLitePath::Skin1ToSkin2);
  Skin1ToSkin2.GetResistanceBaseline().SetValue(systemicResistanceModifier * ResistanceSkinVenous, FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& Skin2ToVenaCava = cCardiovascular.CreatePath(Skin2, VenaCava, BGE::CardiovascularLitePath::Skin2ToVenaCava);

  SEFluidCircuitPath& VenaCavaToGround = cCardiovascular.CreatePath(VenaCava, Ground, BGE::CardiovascularLitePath::VenaCavaToGround);
  VenaCavaToGround.GetComplianceBaseline().SetValue(0.0, FlowComplianceUnit::mL_Per_mmHg);
  SEFluidCircuitPath& VenaCavaBleed = cCardiovascular.CreatePath(VenaCava, Ground, BGE::CardiovascularLitePath::VenaCavaBleed);
  VenaCavaBleed.GetResistanceBaseline().SetValue(m_Config->GetDefaultOpenFlowResistance(FlowResistanceUnit::mmHg_s_Per_mL), FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& IVToVenaCava = cCardiovascular.CreatePath(Ground, VenaCava, BGE::CardiovascularLitePath::IVToVenaCava);
  IVToVenaCava.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);

  //Hemorrhage--Include major organs, if there is a right/left side just adjust the resistance of the right side
  SEFluidCircuitPath& AortaBleed = cCardiovascular.CreatePath(Aorta1, Ground, BGE::CardiovascularLitePath::AortaBleed);
  AortaBleed.GetResistanceBaseline().SetValue(m_Config->GetDefaultOpenFlowResistance(FlowResistanceUnit::mmHg_s_Per_mL), FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& ArmsBleed = cCardiovascular.CreatePath(Arms1, Ground, BGE::CardiovascularLitePath::ArmsBleed);
  ArmsBleed.GetResistanceBaseline().SetValue(m_Config->GetDefaultOpenFlowResistance(FlowResistanceUnit::mmHg_s_Per_mL), FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& GutBleed = cCardiovascular.CreatePath(Gut, Ground, BGE::CardiovascularLitePath::GutBleed);
  GutBleed.GetResistanceBaseline().SetValue(m_Config->GetDefaultOpenFlowResistance(FlowResistanceUnit::mmHg_s_Per_mL), FlowResistanceUnit::mmHg_s_Per_mL);
  SEFluidCircuitPath& LegsBleed = cCardiovascular.CreatePath(Legs1, Ground, BGE::CardiovascularLitePath::LegsBleed);
  LegsBleed.GetResistanceBaseline().SetValue(m_Config->GetDefaultOpenFlowResistance(FlowResistanceUnit::mmHg_s_Per_mL), FlowResistanceUnit::mmHg_s_Per_mL);

  // Compute compliances from target pressures and baseline volumes--assume that this method still holds valid for Lite
  for (SEFluidCircuitPath* p : cCardiovascular.GetPaths()) {
    if (p->HasCapacitanceBaseline()) {
      SEFluidCircuitNode& src = p->GetSourceNode();
      if (!src.HasVolumeBaseline()) {
        Fatal("Compliance paths must have a volume baseline.");
      }
      double pressure = src.GetPressure(PressureUnit::mmHg);
      double volume = src.GetVolumeBaseline(VolumeUnit::mL);
      p->GetComplianceBaseline().SetValue(volume / pressure, FlowComplianceUnit::mL_Per_mmHg);
    }
  }
  // The vena cava compliance needs to be decreased to ensure proper return
  double venaCavaComplianceTuning = 1.0;
  VenaCavaToGround.GetCapacitanceBaseline().SetValue(venaCavaComplianceTuning * VenaCavaToGround.GetComplianceBaseline().GetValue(FlowComplianceUnit::mL_Per_mmHg), FlowComplianceUnit::mL_Per_mmHg);

  // Hearts and pericardium have special compliance computations
  double InitialComplianceHeartRight = 1.0 / 0.0243;
  double InitialComplianceHeartLeft = 1.0 / 0.049;
  // Volumes are initialized from the volume baselines. The heart volume initialization is a little tricky. To much prime and the
  // initial pressure wave will be devastating to the rest of the CV system during the first contraction phase. Too little prime
  // and there will be issues with available flow as the elastance decreases during the first relaxation phase.
  // The 1/4 full initialization gives decent results.
  RightHeart1ToRightHeart3.GetComplianceBaseline().SetValue(InitialComplianceHeartRight, FlowComplianceUnit::mL_Per_mmHg);
  LeftHeart1ToLeftHeart3.GetComplianceBaseline().SetValue(InitialComplianceHeartLeft, FlowComplianceUnit::mL_Per_mmHg);
  //PericardiumToGround.GetComplianceBaseline().SetValue(100.0, FlowComplianceUnit::mL_Per_mmHg);

  double VolumeModifierAorta = 1.16722 * 1.018749, VolumeModifierBrain = 0.998011 * 1.038409, VolumeModifierBone = 1.175574 * 0.985629, VolumeModifierFat = 1.175573 * 0.986527;
  double VolumeModifierGut = 1.17528 * 0.985609, VolumeModifierArms = 1.175573 * 0.986529, VolumeModifierKidneyL = 0.737649 * 0.954339, VolumeModifierLegs = 1.175573 * 0.986529;
  double VolumeModifierPulmArtL = 0.855566 * 1.095697, VolumeModifierPulmCapL = 0.724704 * 1.079139, VolumeModifierPulmVeinL = 0.548452 * 1.056844 * 1.062, VolumeModifierLiver = 1.157475 * 0.991848;
  double VolumeModifierMuscle = 1.175573 * 0.986529, VolumeModifierMyocard = 1.175564 * 0.986531, VolumeModifierKidneyR = 0.737649 * 0.954339;
  double VolumeModifierPulmArtR = 0.756158 * 1.121167, VolumeModifierPulmCapR = 0.602545 * 1.118213, VolumeModifierPulmVeinR = 0.395656 * 1.11424 * 1.11;
  double VolumeModifierSkin = 1.007306 * 1.035695;
  double VolumeModifierVenaCava = 0.66932 * 1.134447;

  //And also modify the compliances
  Aorta1ToGround.GetComplianceBaseline().SetValue(largeArteriesComplianceModifier * Aorta1ToGround.GetComplianceBaseline(FlowComplianceUnit::mL_Per_mmHg), FlowComplianceUnit::mL_Per_mmHg);

  RightPulmonaryArteries.GetVolumeBaseline().SetValue(VolumeModifierPulmArtR * RightPulmonaryArteries.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  LeftPulmonaryArteries.GetVolumeBaseline().SetValue(VolumeModifierPulmArtL * LeftPulmonaryArteries.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  RightPulmonaryCapillaries.GetVolumeBaseline().SetValue(VolumeModifierPulmCapR * RightPulmonaryCapillaries.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  LeftPulmonaryCapillaries.GetVolumeBaseline().SetValue(VolumeModifierPulmCapL * LeftPulmonaryCapillaries.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  RightPulmonaryVeins.GetVolumeBaseline().SetValue(VolumeModifierPulmVeinR * RightPulmonaryVeins.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  LeftPulmonaryVeins.GetVolumeBaseline().SetValue(VolumeModifierPulmVeinL * LeftPulmonaryVeins.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Aorta1.GetVolumeBaseline().SetValue(VolumeModifierAorta * Aorta1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Arms1.GetVolumeBaseline().SetValue(VolumeModifierArms * Arms1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Brain1.GetVolumeBaseline().SetValue(VolumeModifierBrain * Brain1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Bone1.GetVolumeBaseline().SetValue(VolumeModifierBone * Bone1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Fat1.GetVolumeBaseline().SetValue(VolumeModifierFat * Fat1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Gut.GetVolumeBaseline().SetValue(VolumeModifierGut * Gut.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Liver1.GetVolumeBaseline().SetValue(VolumeModifierLiver * Liver1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  LeftKidney1.GetVolumeBaseline().SetValue(VolumeModifierKidneyL * LeftKidney1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Legs1.GetVolumeBaseline().SetValue(VolumeModifierLegs * Legs1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Muscle1.GetVolumeBaseline().SetValue(VolumeModifierMuscle * Muscle1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Myocardium1.GetVolumeBaseline().SetValue(VolumeModifierMyocard * Myocardium1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  RightKidney1.GetVolumeBaseline().SetValue(VolumeModifierKidneyR * RightKidney1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  Skin1.GetVolumeBaseline().SetValue(VolumeModifierSkin * Skin1.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);
  VenaCava.GetVolumeBaseline().SetValue(VolumeModifierVenaCava * VenaCava.GetVolumeBaseline(VolumeUnit::mL), VolumeUnit::mL);

  // Prepare circuit for compartment creation
  cCardiovascular.SetNextAndCurrentFromBaselines();
  cCardiovascular.StateChange();

  SEFluidCircuit& cCombinedCardiovascular = m_Circuits->GetActiveCardiovascularCircuit();
  cCombinedCardiovascular.AddCircuit(cCardiovascular);
  cCombinedCardiovascular.SetNextAndCurrentFromBaselines();
  cCombinedCardiovascular.StateChange();

  /////////////////////////
  // Create Compartments //
  /////////////////////////

  /////////////////
  // Right Heart //
  SELiquidCompartment& vRightHeart = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::RightHeart);
  vRightHeart.MapNode(RightHeart1);
  vRightHeart.MapNode(RightHeart2);
  vRightHeart.MapNode(RightHeart3);
  vRightHeart.MapNode(MainPulmonaryArteries);
  //////////////////////////////
  // Right Pulmonary Arteries //
  SELiquidCompartment& vRightPulmonaryArteries = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::RightPulmonaryArteries);
  vRightPulmonaryArteries.MapNode(RightPulmonaryArteries);
  vRightPulmonaryArteries.MapNode(RightIntermediatePulmonaryArteries);
  /////////////////////////////
  // Left Pulmonary Arteries //
  SELiquidCompartment& vLeftPulmonaryArteries = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::LeftPulmonaryArteries);
  vLeftPulmonaryArteries.MapNode(LeftPulmonaryArteries);
  vLeftPulmonaryArteries.MapNode(LeftIntermediatePulmonaryArteries);
  ////////////////////////
  // Pulmonary Arteries //
  SELiquidCompartment& vPulmonaryArteries = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::PulmonaryArteries);
  vPulmonaryArteries.AddChild(vRightPulmonaryArteries);
  vPulmonaryArteries.AddChild(vLeftPulmonaryArteries);
  /////////////////////////////////
  // Right Pulmonary Capillaries //
  SELiquidCompartment& vRightPulmonaryCapillaries = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::RightPulmonaryCapillaries);
  vRightPulmonaryCapillaries.MapNode(RightPulmonaryCapillaries);
  ////////////////////////////////
  // Left Pulmonary Capillaries //
  SELiquidCompartment& vLeftPulmonaryCapillaries = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::LeftPulmonaryCapillaries);
  vLeftPulmonaryCapillaries.MapNode(LeftPulmonaryCapillaries);
  ///////////////////////////
  // Pulmonary Capillaries //
  SELiquidCompartment& vPulmonaryCapillaries = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::PulmonaryCapillaries);
  vPulmonaryCapillaries.AddChild(vRightPulmonaryCapillaries);
  vPulmonaryCapillaries.AddChild(vLeftPulmonaryCapillaries);
  ///////////////////////////
  // Right Pulmonary Veins //
  SELiquidCompartment& vRightPulmonaryVeins = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::RightPulmonaryVeins);
  vRightPulmonaryVeins.MapNode(RightPulmonaryVeins);
  //////////////////////////
  // Left Pulmonary Veins //
  SELiquidCompartment& vLeftPulmonaryVeins = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::LeftPulmonaryVeins);
  vLeftPulmonaryVeins.MapNode(LeftPulmonaryVeins);
  /////////////////////
  // Pulmonary Veins //
  SELiquidCompartment& vPulmonaryVeins = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::PulmonaryVeins);
  vPulmonaryVeins.AddChild(vRightPulmonaryVeins);
  vPulmonaryVeins.AddChild(vLeftPulmonaryVeins);
  ////////////////
  // Left Heart //
  SELiquidCompartment& vLeftHeart = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::LeftHeart);
  vLeftHeart.MapNode(LeftHeart1);
  vLeftHeart.MapNode(LeftHeart2);
  vLeftHeart.MapNode(LeftHeart3);
  ///////////
  // Aorta //
  SELiquidCompartment& vAorta = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Aorta);
  vAorta.MapNode(Aorta1);
  vAorta.MapNode(Aorta2);
  vAorta.MapNode(Aorta3);
  ///////////
  // Left Arm //
  SELiquidCompartment& vArms = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Arms);
  vArms.MapNode(Arms1);
  vArms.MapNode(Arms2);
  //////////////
  // Brain //
  SELiquidCompartment& vBrain = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Brain);
  vBrain.MapNode(Brain1);
  vBrain.MapNode(Brain2);
  //////////
  // Bone //
  SELiquidCompartment& vBone = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Bone);
  vBone.MapNode(Bone1);
  vBone.MapNode(Bone2);
  /////////
  // Fat //
  SELiquidCompartment& vFat = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Fat);
  vFat.MapNode(Fat1);
  vFat.MapNode(Fat2);
  /////////////////////
  // Gut //
  SELiquidCompartment& vGut = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Gut);
  vGut.MapNode(Gut);
  ///////////
  // Liver //
  SELiquidCompartment& vLiver = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Liver);
  vLiver.MapNode(Liver1);
  vLiver.MapNode(Liver2);
  vLiver.MapNode(PortalVein);
  /////////////////
  // Left Kidney //
  SELiquidCompartment& vLeftKidney = m_Compartments->CreateLiquidCompartment(BGE::VascularCompartment::LeftKidney);
  vLeftKidney.MapNode(LeftKidney1);
  vLeftKidney.MapNode(LeftKidney2);
  //////////////
  // Legs //
  SELiquidCompartment& vLegs = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Legs);
  vLegs.MapNode(Legs1);
  vLegs.MapNode(Legs2);
  ////////////
  // Muscle //
  SELiquidCompartment& vMuscle = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Muscle);
  vMuscle.MapNode(Muscle1);
  vMuscle.MapNode(Muscle2);
  ////////////////
  // Myocardium //
  SELiquidCompartment& vMyocardium = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Myocardium);
  vMyocardium.MapNode(Myocardium1);
  vMyocardium.MapNode(Myocardium2);
  /////////////////
  // Pericardium //
  //SELiquidCompartment& vPericardium = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Pericardium);
  //vPericardium.MapNode(Pericardium);
  //////////////////
  // Right Kidney -- Leave as "Vascular Compartment" namespace since SetUpRenalLite will overwrite it //
  SELiquidCompartment& vRightKidney = m_Compartments->CreateLiquidCompartment(BGE::VascularCompartment::RightKidney);
  vRightKidney.MapNode(RightKidney1);
  vRightKidney.MapNode(RightKidney2);
  //////////
  // Skin //
  SELiquidCompartment& vSkin = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Skin);
  vSkin.MapNode(Skin1);
  vSkin.MapNode(Skin2);
  //////////////
  // VenaCava //
  SELiquidCompartment& vVenaCava = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::VenaCava);
  vVenaCava.MapNode(VenaCava);
  ////////////
  // Ground //
  SELiquidCompartment& vGround = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Ground);
  vGround.MapNode(Ground);

  //////////////////////////
  // Set up our hierarchy //
  //////////////////////////
  SELiquidCompartment& vKidneys = m_Compartments->CreateLiquidCompartment(BGE::VascularCompartment::Kidneys);
  vKidneys.AddChild(vLeftKidney);
  vKidneys.AddChild(vRightKidney);
  SELiquidCompartment& vHeart = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Heart);
  vHeart.AddChild(vMyocardium);
  vHeart.AddChild(vLeftHeart);
  vHeart.AddChild(vRightHeart);
  //vHeart.AddChild(vPericardium);
  SELiquidCompartment& vLeftLung = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::LeftLung);
  vLeftLung.AddChild(vLeftPulmonaryArteries);
  vLeftLung.AddChild(vLeftPulmonaryCapillaries);
  vLeftLung.AddChild(vLeftPulmonaryVeins);
  SELiquidCompartment& vRightLung = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::RightLung);
  vRightLung.AddChild(vRightPulmonaryArteries);
  vRightLung.AddChild(vRightPulmonaryCapillaries);
  vRightLung.AddChild(vRightPulmonaryVeins);
  SELiquidCompartment& vLungs = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Lungs);
  vLungs.AddChild(vLeftLung);
  vLungs.AddChild(vRightLung);

  //////////////////
  // Create Links //
  //////////////////

  /////////////////////
  // Heart and Lungs //
  SELiquidCompartmentLink& vVenaCavaToRightHeart = m_Compartments->CreateLiquidLink(vVenaCava, vRightHeart, BGE::VascularLiteLink::VenaCavaToRightHeart);
  vVenaCavaToRightHeart.MapPath(VenaCavaToRightHeart2);
  SELiquidCompartmentLink& vRightHeartToLeftPulmonaryArteries = m_Compartments->CreateLiquidLink(vRightHeart, vLeftPulmonaryArteries, BGE::VascularLiteLink::RightHeartToLeftPulmonaryArteries);
  vRightHeartToLeftPulmonaryArteries.MapPath(MainPulmonaryArteriesToLeftIntermediatePulmonaryArteries);
  SELiquidCompartmentLink& vLeftPulmonaryArteriesToCapillaries = m_Compartments->CreateLiquidLink(vLeftPulmonaryArteries, vLeftPulmonaryCapillaries, BGE::VascularLiteLink::LeftPulmonaryArteriesToCapillaries);
  vLeftPulmonaryArteriesToCapillaries.MapPath(LeftPulmonaryArteriesToLeftPulmonaryCapillaries);
  SELiquidCompartmentLink& vLeftPulmonaryArteriesToVeins = m_Compartments->CreateLiquidLink(vLeftPulmonaryArteries, vLeftPulmonaryVeins, BGE::VascularLiteLink::LeftPulmonaryArteriesToVeins);
  vLeftPulmonaryArteriesToVeins.MapPath(LeftPulmonaryArteriesToLeftPulmonaryVeins);
  SELiquidCompartmentLink& vLeftPulmonaryCapillariesToVeins = m_Compartments->CreateLiquidLink(vLeftPulmonaryCapillaries, vLeftPulmonaryVeins, BGE::VascularLiteLink::LeftPulmonaryCapillariesToVeins);
  vLeftPulmonaryCapillariesToVeins.MapPath(LeftPulmonaryCapillariesToLeftPulmonaryVeins);
  SELiquidCompartmentLink& vLeftPulmonaryVeinsToLeftHeart = m_Compartments->CreateLiquidLink(vLeftPulmonaryVeins, vLeftHeart, BGE::VascularLiteLink::LeftPulmonaryVeinsToLeftHeart);
  vLeftPulmonaryVeinsToLeftHeart.MapPath(LeftIntermediatePulmonaryVeinsToLeftHeart2);
  SELiquidCompartmentLink& vRightHeartToRightPulmonaryArteries = m_Compartments->CreateLiquidLink(vRightHeart, vRightPulmonaryArteries, BGE::VascularLiteLink::RightHeartToRightPulmonaryArteries);
  vRightHeartToRightPulmonaryArteries.MapPath(MainPulmonaryArteriesToRightIntermediatePulmonaryArteries);
  SELiquidCompartmentLink& vRightPulmonaryArteriesToCapillaries = m_Compartments->CreateLiquidLink(vRightPulmonaryArteries, vRightPulmonaryCapillaries, BGE::VascularLiteLink::RightPulmonaryArteriesToCapillaries);
  vRightPulmonaryArteriesToCapillaries.MapPath(RightPulmonaryArteriesToRightPulmonaryCapillaries);
  SELiquidCompartmentLink& vRightPulmonaryArteriesToVeins = m_Compartments->CreateLiquidLink(vRightPulmonaryArteries, vRightPulmonaryVeins, BGE::VascularLiteLink::RightPulmonaryArteriesToVeins);
  vRightPulmonaryArteriesToVeins.MapPath(RightPulmonaryArteriesToRightPulmonaryVeins);
  SELiquidCompartmentLink& vRightPulmonaryCapillariesToVeins = m_Compartments->CreateLiquidLink(vRightPulmonaryCapillaries, vRightPulmonaryVeins, BGE::VascularLiteLink::RightPulmonaryCapillariesToVeins);
  vRightPulmonaryCapillariesToVeins.MapPath(RightPulmonaryCapillariesToRightPulmonaryVeins);
  SELiquidCompartmentLink& vRightPulmonaryVeinsToLeftHeart = m_Compartments->CreateLiquidLink(vRightPulmonaryVeins, vLeftHeart, BGE::VascularLiteLink::RightPulmonaryVeinsToLeftHeart);
  vRightPulmonaryVeinsToLeftHeart.MapPath(RightIntermediatePulmonaryVeinsToLeftHeart2);
  SELiquidCompartmentLink& vLeftHeartToAorta = m_Compartments->CreateLiquidLink(vLeftHeart, vAorta, BGE::VascularLiteLink::LeftHeartToAorta);
  vLeftHeartToAorta.MapPath(LeftHeart1ToAorta2);
  //////////
  // Left Arm //
  SELiquidCompartmentLink& vAortaToArms = m_Compartments->CreateLiquidLink(vAorta, vArms, BGE::VascularLiteLink::AortaToArms);
  vAortaToArms.MapPath(Aorta1ToArms1);
  SELiquidCompartmentLink& vArmsToVenaCava = m_Compartments->CreateLiquidLink(vArms, vVenaCava, BGE::VascularLiteLink::ArmsToVenaCava);
  vArmsToVenaCava.MapPath(Arms2ToVenaCava);
  //////////////
  // Bone //
  SELiquidCompartmentLink& vAortaToBone = m_Compartments->CreateLiquidLink(vAorta, vBone, BGE::VascularLiteLink::AortaToBone);
  vAortaToBone.MapPath(Aorta1ToBone1);
  SELiquidCompartmentLink& vBoneToVenaCava = m_Compartments->CreateLiquidLink(vBone, vVenaCava, BGE::VascularLiteLink::BoneToVenaCava);
  vBoneToVenaCava.MapPath(Bone2ToVenaCava);
  ///////////
  // Brain //
  SELiquidCompartmentLink& vAortaToBrain = m_Compartments->CreateLiquidLink(vAorta, vBrain, BGE::VascularLiteLink::AortaToBrain);
  vAortaToBrain.MapPath(Aorta1ToBrain1);
  SELiquidCompartmentLink& vBrainToVenaCava = m_Compartments->CreateLiquidLink(vBrain, vVenaCava, BGE::VascularLiteLink::BrainToVenaCava);
  vBrainToVenaCava.MapPath(Brain2ToVenaCava);
  /////////
  // Fat //
  SELiquidCompartmentLink& vAortaToFat = m_Compartments->CreateLiquidLink(vAorta, vFat, BGE::VascularLiteLink::AortaToFat);
  vAortaToFat.MapPath(Aorta1ToFat1);
  SELiquidCompartmentLink& vFatToVenaCava = m_Compartments->CreateLiquidLink(vFat, vVenaCava, BGE::VascularLiteLink::FatToVenaCava);
  vFatToVenaCava.MapPath(Fat2ToVenaCava);
  /////////////////////
  // Gut //
  SELiquidCompartmentLink& vAortaToGut = m_Compartments->CreateLiquidLink(vAorta, vGut, BGE::VascularLiteLink::AortaToGut);
  vAortaToGut.MapPath(Aorta1ToGut);
  SELiquidCompartmentLink& vGutToLiver = m_Compartments->CreateLiquidLink(vGut, vLiver, BGE::VascularLiteLink::GutToLiver);
  vGutToLiver.MapPath(GutToPortalVein);
  ///////////
  // Liver //
  SELiquidCompartmentLink& vAortaToLiver = m_Compartments->CreateLiquidLink(vAorta, vLiver, BGE::VascularLiteLink::AortaToLiver);
  vAortaToLiver.MapPath(Aorta1ToLiver1);
  SELiquidCompartmentLink& vLiverToVenaCava = m_Compartments->CreateLiquidLink(vLiver, vVenaCava, BGE::VascularLiteLink::LiverToVenaCava);
  vLiverToVenaCava.MapPath(Liver2ToVenaCava);
  /////////////////
  // Left Kidney //
  SELiquidCompartmentLink& vAortaToLeftKidney = m_Compartments->CreateLiquidLink(vAorta, vLeftKidney, BGE::VascularLiteLink::AortaToLeftKidney);
  vAortaToLeftKidney.MapPath(Aorta1ToLeftKidney1);
  SELiquidCompartmentLink& vLeftKidneyToVenaCava = m_Compartments->CreateLiquidLink(vLeftKidney, vVenaCava, BGE::VascularLiteLink::LeftKidneyToVenaCava);
  vLeftKidneyToVenaCava.MapPath(LeftKidney2ToVenaCava);
  //////////////
  // Leg //
  SELiquidCompartmentLink& vAortaToLegs = m_Compartments->CreateLiquidLink(vAorta, vLegs, BGE::VascularLiteLink::AortaToLegs);
  vAortaToLegs.MapPath(Aorta1ToLegs1);
  SELiquidCompartmentLink& vLegsToVenaCava = m_Compartments->CreateLiquidLink(vLegs, vVenaCava, BGE::VascularLiteLink::LegsToVenaCava);
  vLegsToVenaCava.MapPath(Legs2ToVenaCava);
  ////////////
  // Muscle //
  SELiquidCompartmentLink& vAortaToMuscle = m_Compartments->CreateLiquidLink(vAorta, vMuscle, BGE::VascularLiteLink::AortaToMuscle);
  vAortaToMuscle.MapPath(Aorta1ToMuscle1);
  SELiquidCompartmentLink& vMuscleToVenaCava = m_Compartments->CreateLiquidLink(vMuscle, vVenaCava, BGE::VascularLiteLink::MuscleToVenaCava);
  vMuscleToVenaCava.MapPath(Muscle2ToVenaCava);
  ////////////////
  // Myocardium //
  SELiquidCompartmentLink& vAortaToMyocardium = m_Compartments->CreateLiquidLink(vAorta, vMyocardium, BGE::VascularLiteLink::AortaToMyocardium);
  vAortaToMyocardium.MapPath(Aorta1ToMyocardium1);
  SELiquidCompartmentLink& vMyocardiumToVenaCava = m_Compartments->CreateLiquidLink(vMyocardium, vVenaCava, BGE::VascularLiteLink::MyocardiumToVenaCava);
  vMyocardiumToVenaCava.MapPath(Myocardium2ToVenaCava);
  //////////////////
  // Right Kidney //
  SELiquidCompartmentLink& vAortaToRightKidney = m_Compartments->CreateLiquidLink(vAorta, vRightKidney, BGE::VascularLiteLink::AortaToRightKidney);
  vAortaToRightKidney.MapPath(Aorta1ToRightKidney1);
  SELiquidCompartmentLink& vRightKidneyToVenaCava = m_Compartments->CreateLiquidLink(vRightKidney, vVenaCava, BGE::VascularLiteLink::RightKidneyToVenaCava);
  vRightKidneyToVenaCava.MapPath(RightKidney2ToVenaCava);
  ///////////////
  // Skin //
  SELiquidCompartmentLink& vAortaToSkin = m_Compartments->CreateLiquidLink(vAorta, vSkin, BGE::VascularLiteLink::AortaToSkin);
  vAortaToSkin.MapPath(Aorta1ToSkin1);
  SELiquidCompartmentLink& vSkinToVenaCava = m_Compartments->CreateLiquidLink(vSkin, vVenaCava, BGE::VascularLiteLink::SkinToVenaCava);
  vSkinToVenaCava.MapPath(Skin2ToVenaCava);

  /////////////////////
  // Bleeds and IV's //
  SELiquidCompartmentLink& vVenaCavaIV = m_Compartments->CreateLiquidLink(vGround, vVenaCava, BGE::VascularLiteLink::VenaCavaIV);
  vVenaCavaIV.MapPath(IVToVenaCava);
  SELiquidCompartmentLink& vVenaCavaHemorrhage = m_Compartments->CreateLiquidLink(vVenaCava, vGround, BGE::VascularLiteLink::VenaCavaHemorrhage);
  vVenaCavaHemorrhage.MapPath(VenaCavaBleed);
  SELiquidCompartmentLink& vAortaHemorrhage = m_Compartments->CreateLiquidLink(vAorta, vGround, BGE::VascularLiteLink::AortaHemorrhage);
  vAortaHemorrhage.MapPath(AortaBleed);
  SELiquidCompartmentLink& vArmsHemorrhage = m_Compartments->CreateLiquidLink(vArms, vGround, BGE::VascularLiteLink::ArmsHemorrhage);
  vArmsHemorrhage.MapPath(ArmsBleed);
  SELiquidCompartmentLink& vGutHemorrhage = m_Compartments->CreateLiquidLink(vGut, vGround, BGE::VascularLiteLink::GutHemorrhage);
  vGutHemorrhage.MapPath(GutBleed);
  SELiquidCompartmentLink& vLegsHemorrhage = m_Compartments->CreateLiquidLink(vLegs, vGround, BGE::VascularLiteLink::LegsHemorrhage);
  vLegsHemorrhage.MapPath(LegsBleed);

  gCardiovascular.AddCompartment(vRightHeart);
  gCardiovascular.AddCompartment(vRightPulmonaryArteries);
  gCardiovascular.AddCompartment(vLeftPulmonaryArteries);
  gCardiovascular.AddCompartment(vRightPulmonaryCapillaries);
  gCardiovascular.AddCompartment(vLeftPulmonaryCapillaries);
  gCardiovascular.AddCompartment(vRightPulmonaryVeins);
  gCardiovascular.AddCompartment(vLeftPulmonaryVeins);
  gCardiovascular.AddCompartment(vLeftHeart);
  gCardiovascular.AddCompartment(vAorta);
  gCardiovascular.AddCompartment(vArms);
  gCardiovascular.AddCompartment(vBrain);
  gCardiovascular.AddCompartment(vBone);
  gCardiovascular.AddCompartment(vFat);
  gCardiovascular.AddCompartment(vGut);
  gCardiovascular.AddCompartment(vLiver);
  gCardiovascular.AddCompartment(vLeftKidney);
  gCardiovascular.AddCompartment(vLegs);
  gCardiovascular.AddCompartment(vMuscle);
  gCardiovascular.AddCompartment(vMyocardium);
  gCardiovascular.AddCompartment(vRightKidney);
  gCardiovascular.AddCompartment(vSkin);
  gCardiovascular.AddCompartment(vVenaCava);
  gCardiovascular.AddLink(vVenaCavaToRightHeart);
  gCardiovascular.AddLink(vRightHeartToLeftPulmonaryArteries);
  gCardiovascular.AddLink(vLeftPulmonaryArteriesToCapillaries);
  gCardiovascular.AddLink(vLeftPulmonaryArteriesToVeins);
  gCardiovascular.AddLink(vLeftPulmonaryCapillariesToVeins);
  gCardiovascular.AddLink(vLeftPulmonaryVeinsToLeftHeart);
  gCardiovascular.AddLink(vRightHeartToRightPulmonaryArteries);
  gCardiovascular.AddLink(vRightPulmonaryArteriesToCapillaries);
  gCardiovascular.AddLink(vRightPulmonaryArteriesToVeins);
  gCardiovascular.AddLink(vRightPulmonaryCapillariesToVeins);
  gCardiovascular.AddLink(vRightPulmonaryVeinsToLeftHeart);
  gCardiovascular.AddLink(vLeftHeartToAorta);
  gCardiovascular.AddLink(vAortaToArms);
  gCardiovascular.AddLink(vArmsToVenaCava);
  gCardiovascular.AddLink(vAortaToBone);
  gCardiovascular.AddLink(vBoneToVenaCava);
  gCardiovascular.AddLink(vAortaToBrain);
  gCardiovascular.AddLink(vBrainToVenaCava);
  gCardiovascular.AddLink(vAortaToFat);
  gCardiovascular.AddLink(vFatToVenaCava);
  gCardiovascular.AddLink(vAortaToGut);
  gCardiovascular.AddLink(vGutToLiver);
  gCardiovascular.AddLink(vAortaToLeftKidney);
  gCardiovascular.AddLink(vLeftKidneyToVenaCava);
  gCardiovascular.AddLink(vAortaToLegs);
  gCardiovascular.AddLink(vLegsToVenaCava);
  gCardiovascular.AddLink(vAortaToLiver);
  gCardiovascular.AddLink(vLiverToVenaCava);
  gCardiovascular.AddLink(vAortaToMuscle);
  gCardiovascular.AddLink(vMuscleToVenaCava);
  gCardiovascular.AddLink(vAortaToMyocardium);
  gCardiovascular.AddLink(vMyocardiumToVenaCava);
  gCardiovascular.AddLink(vAortaToRightKidney);
  gCardiovascular.AddLink(vRightKidneyToVenaCava);
  gCardiovascular.AddLink(vAortaToSkin);
  gCardiovascular.AddLink(vSkinToVenaCava);
  gCardiovascular.AddLink(vVenaCavaHemorrhage);
  gCardiovascular.AddLink(vVenaCavaIV);
  gCardiovascular.AddLink(vAortaHemorrhage);
  gCardiovascular.AddLink(vArmsHemorrhage);
  gCardiovascular.AddLink(vGutHemorrhage);
  gCardiovascular.AddLink(vLegsHemorrhage);
  gCardiovascular.StateChange();

  SELiquidCompartmentGraph& gCombinedCardiovascular = m_Compartments->GetActiveCardiovascularGraph();
  gCombinedCardiovascular.AddGraph(gCardiovascular);
  gCombinedCardiovascular.StateChange();
}

void BioGears::SetupRenalLite()
{
  Info("Setting Up Renal Lite");
  //////////////////////////
  // Circuit Interdependence
  SEFluidCircuit& cCardiovascular = m_Circuits->GetCardiovascularCircuit();

  //assuming there is a left and right kidney node in cardiovascular AND that a baseline volume is set (as a function of patient mass):
  double leftKidneyFluidVolume_mL = cCardiovascular.GetNode(BGE::CardiovascularNode::LeftKidney1)->GetVolumeBaseline(VolumeUnit::mL);
  double rightKidneyFluidVolume_mL = cCardiovascular.GetNode(BGE::CardiovascularNode::RightKidney1)->GetVolumeBaseline(VolumeUnit::mL);
  double totalKidneyFluidVolume_mL = leftKidneyFluidVolume_mL + rightKidneyFluidVolume_mL;
  double singleKidneyLargeVasculatureFluidVolume_mL = totalKidneyFluidVolume_mL / 2; //Total large vasculature fluid volume
  double singleKidneySmallVasculatureFluidVolume_mL = totalKidneyFluidVolume_mL / 2; //Total small vasculature fluid volume

  //////////////////////////
  ///// Circuit Parameters//////
  double openSwitch_mmHg_s_Per_mL = m_Config->GetDefaultOpenFlowResistance(FlowResistanceUnit::mmHg_s_Per_mL);
  //Resistances with some tuning multipliers
  double urineTuningMultiplier = 2.2*(0.40);
  double arteryTuningMultiplier = 0.05 * (0.8);
  double reabsorptionTuningMultiplier = 1.0*(0.8);

  double renalArteryResistance_mmHg_s_Per_mL = Convert(0.5 * (0.0250 * arteryTuningMultiplier), FlowResistanceUnit::mmHg_min_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  double afferentResistance_mmHg_s_Per_mL = Convert(0.5 * (0.0417)*arteryTuningMultiplier, FlowResistanceUnit::mmHg_min_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  double efferentResistance_mmHg_s_Per_mL = Convert(0.5 * (0.0763), FlowResistanceUnit::mmHg_min_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  double glomerularResistance_mmHg_s_Per_mL = Convert(0.5 * (0.0019), FlowResistanceUnit::mmHg_min_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  double peritubularResistance_mmHg_s_Per_mL = Convert(0.5 * (0.0167), FlowResistanceUnit::mmHg_min_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  double renalVeinResistance_mmHg_s_Per_mL = Convert(0.5 * (0.0066), FlowResistanceUnit::mmHg_min_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  double glomerularFilterResistance_mmHg_s_Per_mL = Convert(0.5 * (0.1600) * urineTuningMultiplier, FlowResistanceUnit::mmHg_min_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  double tubulesResistance_mmHg_s_Per_mL = Convert(0.5 * (0.1920) * urineTuningMultiplier, FlowResistanceUnit::mmHg_min_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  double reabsoprtionResistance_mmHg_s_Per_mL = Convert(0.5 * (0.1613) * reabsorptionTuningMultiplier, FlowResistanceUnit::mmHg_min_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  //This one is tuned
  double ureterTuningMultiplier = 0.50;
  double ureterResistance_mmHg_s_Per_mL = Convert(0.5 * (30.0) * ureterTuningMultiplier, FlowResistanceUnit::mmHg_min_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  double urethraResistance_mmHg_s_Per_mL = openSwitch_mmHg_s_Per_mL;
  //Compliances
  //0.5 * CapacitanceKidney is the per-kidney value from 3 element Windkessel
  double totalCompliance = 0.5 * (0.91 * 1.7560) * 0.02;
  //The fractions here should add to 1.0;
  double renalArteryCompliance_mL_Per_mmHg = totalCompliance * 0.11;
  double renalVeinCompliance_mL_Per_mmHg = totalCompliance * 0.78;
  double glomerularCompliance_mL_Per_mmHg = totalCompliance * 0.11;
  ///\todo The bladder is currently not being modeled as a compliance
  //double bladderCompliance_mL_Per_mmHg = Convert(38.3, FlowComplianceUnit::mL_Per_cmH2O, FlowComplianceUnit::mL_Per_mmHg);

  //Large Vasculature (divide total large vasculature fluid volume three ways)Need to double because we have a single lumped kidney:
  double tubulesVolume_mL = singleKidneyLargeVasculatureFluidVolume_mL / 3.0; //2.0 * (singleKidneyLargeVasculatureFluidVolume_mL / 3.0);
  double renalArteryVolume_mL = singleKidneyLargeVasculatureFluidVolume_mL / 3.0; //2.0 * (singleKidneyLargeVasculatureFluidVolume_mL / 3.0);
  double renalVeinVolume_mL = singleKidneyLargeVasculatureFluidVolume_mL / 3.0; //2.0 * (singleKidneyLargeVasculatureFluidVolume_mL / 3.0);

  //Small vasculature (divide total small vasculature fluid volume five ways):
  double peritubularVolume_mL = singleKidneySmallVasculatureFluidVolume_mL / 5.0; //2.0 * (singleKidneySmallVasculatureFluidVolume_mL / 5.0);
  double efferentVolume_mL = singleKidneySmallVasculatureFluidVolume_mL / 5.0; //2.0 * (singleKidneySmallVasculatureFluidVolume_mL / 5.0);
  double afferentVolume_mL = singleKidneySmallVasculatureFluidVolume_mL / 5.0; //2.0 * (singleKidneySmallVasculatureFluidVolume_mL / 5.0);
  double bowmansVolume_mL = singleKidneySmallVasculatureFluidVolume_mL / 5.0; //2.0 * (singleKidneySmallVasculatureFluidVolume_mL / 5.0);
  double glomerularVolume_mL = singleKidneySmallVasculatureFluidVolume_mL / 5.0; //2.0 * (singleKidneySmallVasculatureFluidVolume_mL / 5.0);

  //Using width = 1.8 mm and length = 11 inches => 710.6 mm^3, double for lumped lite model
  double ureterVolume_mL = 2.0 * (0.71);

  //Tuned constants
  double bladderVolume_mL = 1.0;
  //Unstressed Pressures - set to zero to use unstressed properly
  double renalArteryPressure_mmHg = 0.0;
  double renalVeinPressure_mmHg = 0.0;
  double glomerularPressure_mmHg = cCardiovascular.GetNode(BGE::CardiovascularNode::LeftKidney1)->GetPressure(PressureUnit::mmHg);
  //double bladderPressure_mmHg = bladderVolume_mL / bladderCompliance_mL_Per_mmHg;
  //Pressure Sources
  double glomerularOsmoticPressure_mmHg = -32.0;
  double bowmansOsmoticPressure_mmHg = 0.0;
  double tubulesOsmoticPressure_mmHg = -15.0;
  double peritubularOsmoticPressure_mmHg = -32.0;

  SEFluidCircuit& cRenal = m_Circuits->GetRenalCircuit();

  ////////////
  // Ground //
  SEFluidCircuitNode& Ground = cRenal.CreateNode(BGE::RenalLiteNode::Ground);
  Ground.GetPressure().SetValue(0.0, PressureUnit::mmHg);
  cRenal.AddReferenceNode(Ground);

  //////////////////
  // Create Nodes //
  //////////////////

  /////////////////
  //  Blood //
  /////////////////
  //////////////////////////
  // AortaConnection //
  SEFluidCircuitNode& AortaConnection = cRenal.CreateNode(BGE::RenalLiteNode::AortaConnection);
  //////////////////////
  // RenalArtery //
  SEFluidCircuitNode& RenalArtery = cRenal.CreateNode(BGE::RenalLiteNode::RenalArtery);
  RenalArtery.GetVolumeBaseline().SetValue(renalArteryVolume_mL, VolumeUnit::mL);
  RenalArtery.GetPressure().SetValue(renalArteryPressure_mmHg, PressureUnit::mmHg);
  RenalArtery.GetNextPressure().SetValue(renalArteryPressure_mmHg, PressureUnit::mmHg);
  ////////////////////////////
  // AfferentArteriole //
  SEFluidCircuitNode& AfferentArteriole = cRenal.CreateNode(BGE::RenalLiteNode::AfferentArteriole);
  AfferentArteriole.GetVolumeBaseline().SetValue(afferentVolume_mL, VolumeUnit::mL);
  ////////////////////////////////
  // GlomerularCapillaries //
  SEFluidCircuitNode& GlomerularCapillaries = cRenal.CreateNode(BGE::RenalLiteNode::GlomerularCapillaries);
  GlomerularCapillaries.GetVolumeBaseline().SetValue(glomerularVolume_mL, VolumeUnit::mL);
  GlomerularCapillaries.GetPressure().SetValue(glomerularPressure_mmHg, PressureUnit::mmHg);
  ////////////////////////////
  // EfferentArteriole //
  SEFluidCircuitNode& EfferentArteriole = cRenal.CreateNode(BGE::RenalLiteNode::EfferentArteriole);
  EfferentArteriole.GetVolumeBaseline().SetValue(efferentVolume_mL, VolumeUnit::mL);
  /////////////////////////////////
  // PeritubularCapillaries //
  SEFluidCircuitNode& PeritubularCapillaries = cRenal.CreateNode(BGE::RenalLiteNode::PeritubularCapillaries);
  PeritubularCapillaries.GetVolumeBaseline().SetValue(peritubularVolume_mL, VolumeUnit::mL);
  ////////////////////
  // RenalVein //
  SEFluidCircuitNode& RenalVein = cRenal.CreateNode(BGE::RenalLiteNode::RenalVein);
  RenalVein.GetVolumeBaseline().SetValue(renalVeinVolume_mL, VolumeUnit::mL);
  RenalVein.GetPressure().SetValue(renalVeinPressure_mmHg, PressureUnit::mmHg);
  /////////////////////////////
  // VenaCavaConnection //
  SEFluidCircuitNode& VenaCavaConnection = cRenal.CreateNode(BGE::RenalLiteNode::VenaCavaConnection);
  //////////////////////////
  // BowmansCapsules //
  SEFluidCircuitNode& BowmansCapsules = cRenal.CreateNode(BGE::RenalLiteNode::BowmansCapsules);
  BowmansCapsules.GetVolumeBaseline().SetValue(bowmansVolume_mL, VolumeUnit::mL);
  /////////////////////////////
  // NetBowmansCapsules //
  SEFluidCircuitNode& NetBowmansCapsules = cRenal.CreateNode(BGE::RenalLiteNode::NetBowmansCapsules);
  ///////////////////////////////////
  // NetGlomerularCapillaries //
  SEFluidCircuitNode& NetGlomerularCapillaries = cRenal.CreateNode(BGE::RenalLiteNode::NetGlomerularCapillaries);
  ////////////////////////////////////
  // NetPeritubularCapillaries //
  SEFluidCircuitNode& NetPeritubularCapillaries = cRenal.CreateNode(BGE::RenalLiteNode::NetPeritubularCapillaries);

  /////////////////
  //  Urine //
  /////////////////
  //////////////////
  // Tubules //
  SEFluidCircuitNode& Tubules = cRenal.CreateNode(BGE::RenalLiteNode::Tubules);
  Tubules.GetVolumeBaseline().SetValue(tubulesVolume_mL, VolumeUnit::mL);
  /////////////////////
  // NetTubules //
  SEFluidCircuitNode& NetTubules = cRenal.CreateNode(BGE::RenalLiteNode::NetTubules);
  /////////////////
  // Ureter //
  SEFluidCircuitNode& Ureter = cRenal.CreateNode(BGE::RenalLiteNode::Ureter);
  Ureter.GetVolumeBaseline().SetValue(ureterVolume_mL, VolumeUnit::mL);

  /////////////
  // Bladder //
  SEFluidCircuitNode& Bladder = cRenal.CreateNode(BGE::RenalLiteNode::Bladder);
  Bladder.GetVolumeBaseline().SetValue(bladderVolume_mL, VolumeUnit::mL);
  //Bladder.GetPressure().SetValue(0.0, PressureUnit::mmHg);

  //////////////////
  // Create Paths //
  //////////////////

  /////////////////
  //  Blood //
  /////////////////
  ///////////////////////////////////////
  // AortaConnectionToRenalArtery //
  SEFluidCircuitPath& AortaConnectionToRenalArtery = cRenal.CreatePath(AortaConnection, RenalArtery, BGE::RenalLitePath::AortaConnectionToRenalArtery);
  //////////////////////
  // RenalArteryToAfferentArteriole //
  SEFluidCircuitPath& RenalArteryToAfferentArteriole = cRenal.CreatePath(RenalArtery, AfferentArteriole, BGE::RenalLitePath::RenalArteryToAfferentArteriole);
  RenalArteryToAfferentArteriole.GetResistanceBaseline().SetValue(renalArteryResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  ////////////////////////////////
  // RenalArteryCompliance //
  SEFluidCircuitPath& RenalArteryCompliance = cRenal.CreatePath(RenalArtery, Ground, BGE::RenalLitePath::RenalArteryCompliance);
  RenalArteryCompliance.GetComplianceBaseline().SetValue(renalArteryCompliance_mL_Per_mmHg, FlowComplianceUnit::mL_Per_mmHg);
  ///////////////////////////////////////////////////
  // AfferentArterioleToGlomerularCapillaries //
  SEFluidCircuitPath& AfferentArterioleToGlomerularCapillaries = cRenal.CreatePath(AfferentArteriole, GlomerularCapillaries, BGE::RenalLitePath::AfferentArterioleToGlomerularCapillaries);
  AfferentArterioleToGlomerularCapillaries.GetResistanceBaseline().SetValue(afferentResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  ///////////////////////////////////////////////////
  // GlomerularCapillariesToEfferentArteriole //
  SEFluidCircuitPath& GlomerularCapillariesToEfferentArteriole = cRenal.CreatePath(GlomerularCapillaries, EfferentArteriole, BGE::RenalLitePath::GlomerularCapillariesToEfferentArteriole);
  GlomerularCapillariesToEfferentArteriole.GetResistanceBaseline().SetValue(glomerularResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  //////////////////////////////////////////
  // GlomerularCapillariesCompliance //
  SEFluidCircuitPath& GlomerularCapillariesCompliance = cRenal.CreatePath(GlomerularCapillaries, Ground, BGE::RenalLitePath::GlomerularCapillariesCompliance);
  GlomerularCapillariesCompliance.GetComplianceBaseline().SetValue(glomerularCompliance_mL_Per_mmHg, FlowComplianceUnit::mL_Per_mmHg);
  ////////////////////////////////////////////////////
  // EfferentArterioleToPeritubularCapillaries //
  SEFluidCircuitPath& EfferentArterioleToPeritubularCapillaries = cRenal.CreatePath(EfferentArteriole, PeritubularCapillaries, BGE::RenalLitePath::EfferentArterioleToPeritubularCapillaries);
  EfferentArterioleToPeritubularCapillaries.GetResistanceBaseline().SetValue(efferentResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  ////////////////////////////////////////////
  // PeritubularCapillariesToRenalVein //
  SEFluidCircuitPath& PeritubularCapillariesToRenalVein = cRenal.CreatePath(PeritubularCapillaries, RenalVein, BGE::RenalLitePath::PeritubularCapillariesToRenalVein);
  PeritubularCapillariesToRenalVein.GetResistanceBaseline().SetValue(peritubularResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  ////////////////////////////////////////
  // RenalVeinToVenaCavaConnection //
  SEFluidCircuitPath& RenalVeinToVenaCavaConnection = cRenal.CreatePath(RenalVein, VenaCavaConnection, BGE::RenalLitePath::RenalVeinToVenaCavaConnection);
  RenalVeinToVenaCavaConnection.GetResistanceBaseline().SetValue(renalVeinResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  //////////////////////////////
  // RenalVeinCompliance //
  SEFluidCircuitPath& RenalVeinCompliance = cRenal.CreatePath(RenalVein, Ground, BGE::RenalLitePath::RenalVeinCompliance);
  RenalVeinCompliance.GetComplianceBaseline().SetValue(renalVeinCompliance_mL_Per_mmHg, FlowComplianceUnit::mL_Per_mmHg);
  //////////////////////////////////////////////////////////
  // GlomerularCapillariesToNetGlomerularCapillaries //
  SEFluidCircuitPath& GlomerularCapillariesToNetGlomerularCapillaries = cRenal.CreatePath(GlomerularCapillaries, NetGlomerularCapillaries, BGE::RenalLitePath::GlomerularCapillariesToNetGlomerularCapillaries);
  GlomerularCapillariesToNetGlomerularCapillaries.GetPressureSourceBaseline().SetValue(glomerularOsmoticPressure_mmHg, PressureUnit::mmHg);
  ///////////////////////////////////////////////////////
  // NetGlomerularCapillariesToNetBowmansCapsules //
  SEFluidCircuitPath& NetGlomerularCapillariesToNetBowmansCapsules = cRenal.CreatePath(NetGlomerularCapillaries, NetBowmansCapsules, BGE::RenalLitePath::NetGlomerularCapillariesToNetBowmansCapsules);
  NetGlomerularCapillariesToNetBowmansCapsules.GetResistanceBaseline().SetValue(glomerularFilterResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  //////////////////////////////////////////////
  // BowmansCapsulesToNetBowmansCapsules //
  SEFluidCircuitPath& BowmansCapsulesToNetBowmansCapsules = cRenal.CreatePath(BowmansCapsules, NetBowmansCapsules, BGE::RenalLitePath::BowmansCapsulesToNetBowmansCapsules);
  BowmansCapsulesToNetBowmansCapsules.GetPressureSourceBaseline().SetValue(bowmansOsmoticPressure_mmHg, PressureUnit::mmHg);
  /////////////////
  // Hemorrhage from  kidney//
  SEFluidCircuitPath& KidneyBleed = cRenal.CreatePath(RenalVein, Ground, BGE::CardiovascularLitePath::KidneyBleed);
  KidneyBleed.GetResistanceBaseline().SetValue(m_Config->GetDefaultOpenFlowResistance(FlowResistanceUnit::mmHg_s_Per_mL), FlowResistanceUnit::mmHg_s_Per_mL);
  ///////////////////////////////////

  ///////////////////////////////////
  //  Urine //
  /////////////////
  ///////////////////////////////////
  // BowmansCapsulesToTubules //
  SEFluidCircuitPath& BowmansCapsulesToTubules = cRenal.CreatePath(BowmansCapsules, Tubules, BGE::RenalLitePath::BowmansCapsulesToTubules);
  BowmansCapsulesToTubules.GetResistanceBaseline().SetValue(tubulesResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  //////////////////////////
  // TubulesToUreter //
  SEFluidCircuitPath& TubulesToUreter = cRenal.CreatePath(Tubules, Ureter, BGE::RenalLitePath::TubulesToUreter);
  TubulesToUreter.GetResistanceBaseline().SetValue(ureterResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  /////////////////////////////
  // ubulesToNetTubules //
  SEFluidCircuitPath& TubulesToNetTubules = cRenal.CreatePath(Tubules, NetTubules, BGE::RenalLitePath::TubulesToNetTubules);
  TubulesToNetTubules.GetPressureSourceBaseline().SetValue(tubulesOsmoticPressure_mmHg, PressureUnit::mmHg);
  ////////////////////////////////////////////////
  // NetTubulesToNetPeritubularCapillaries //
  SEFluidCircuitPath& NetTubulesToNetPeritubularCapillaries = cRenal.CreatePath(NetTubules, NetPeritubularCapillaries, BGE::RenalLitePath::NetTubulesToNetPeritubularCapillaries);
  NetTubulesToNetPeritubularCapillaries.GetResistanceBaseline().SetValue(reabsoprtionResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);
  ////////////////////////////////////////////////////////////
  // PeritubularCapillariesToNetPeritubularCapillaries //
  SEFluidCircuitPath& PeritubularCapillariesToNetPeritubularCapillaries = cRenal.CreatePath(PeritubularCapillaries, NetPeritubularCapillaries, BGE::RenalLitePath::PeritubularCapillariesToNetPeritubularCapillaries);
  PeritubularCapillariesToNetPeritubularCapillaries.GetPressureSourceBaseline().SetValue(peritubularOsmoticPressure_mmHg, PressureUnit::mmHg);
  //////////////////////////
  // UreterToBladder //
  SEFluidCircuitPath& UreterToBladder = cRenal.CreatePath(Ureter, Bladder, BGE::RenalLitePath::UreterToBladder);
  UreterToBladder.SetNextValve(CDM::enumOpenClosed::Closed);

  ///////////////////////
  // BladderCompliance //
  SEFluidCircuitPath& BladderToGroundPressure = cRenal.CreatePath(Bladder, Ground, BGE::RenalPath::BladderToGroundPressure);
  /// \todo Use a compliance here - make sure you remove the current handling of bladder volume in the renal system as a pressure source
  //BladderCompliance.GetComplianceBaseline().SetValue(bladderCompliance_mL_Per_mmHg, FlowComplianceUnit::mL_Per_mmHg);
  BladderToGroundPressure.GetPressureSourceBaseline().SetValue(-4.0, PressureUnit::mmHg); //Negative because source-target is for compliance
  //////////////
  // BladderGround //
  SEFluidCircuitPath& BladderToGroundUrinate = cRenal.CreatePath(Bladder, Ground, BGE::RenalPath::BladderToGroundUrinate);
  BladderToGroundUrinate.GetResistanceBaseline().SetValue(urethraResistance_mmHg_s_Per_mL, FlowResistanceUnit::mmHg_s_Per_mL);

  cRenal.SetNextAndCurrentFromBaselines();
  cRenal.StateChange();

  // Delete the three-element Windkessel kidney model
  SEFluidCircuit& cCombinedCardiovascular = m_Circuits->GetActiveCardiovascularCircuit();
  m_Circuits->DeleteFluidNode(BGE::CardiovascularNode::LeftKidney1);
  m_Circuits->DeleteFluidNode(BGE::CardiovascularNode::LeftKidney2);
  m_Circuits->DeleteFluidNode(BGE::CardiovascularNode::RightKidney1);
  m_Circuits->DeleteFluidNode(BGE::CardiovascularNode::RightKidney2);
  m_Circuits->DeleteFluidPath(BGE::CardiovascularPath::Aorta1ToLeftKidney1);
  m_Circuits->DeleteFluidPath(BGE::CardiovascularPath::Aorta1ToRightKidney1);
  m_Circuits->DeleteFluidPath(BGE::CardiovascularPath::LeftKidney1ToLeftKidney2);
  m_Circuits->DeleteFluidPath(BGE::CardiovascularPath::RightKidney1ToRightKidney2);
  m_Circuits->DeleteFluidPath(BGE::CardiovascularPath::LeftKidney1ToGround);
  m_Circuits->DeleteFluidPath(BGE::CardiovascularPath::RightKidney1ToGround);
  m_Circuits->DeleteFluidPath(BGE::CardiovascularPath::LeftKidney2ToVenaCava);
  m_Circuits->DeleteFluidPath(BGE::CardiovascularPath::RightKidney2ToVenaCava);
  cCombinedCardiovascular.AddCircuit(cRenal);
  // Grab the nodes that we will be connecting between the 2 circuits
  SEFluidCircuitNode* Aorta1 = cCardiovascular.GetNode(BGE::CardiovascularNode::Aorta1);
  SEFluidCircuitNode* VenaCava = cCardiovascular.GetNode(BGE::CardiovascularNode::VenaCava);
  // Add the new connection paths
  SEFluidCircuitPath& NewAorta1ToKidney = cCombinedCardiovascular.CreatePath(*Aorta1, AortaConnection, BGE::CardiovascularPath::Aorta1ToKidney);
  SEFluidCircuitPath& NewKidneyToVenaCava = cCombinedCardiovascular.CreatePath(VenaCavaConnection, *VenaCava, BGE::CardiovascularPath::KidneyToVenaCava);

  // We need to move the resistances
  NewAorta1ToKidney.GetResistanceBaseline().Set(AortaConnectionToRenalArtery.GetResistanceBaseline());
  AortaConnectionToRenalArtery.GetResistanceBaseline().Invalidate();
  NewKidneyToVenaCava.GetResistanceBaseline().Set(RenalVeinToVenaCavaConnection.GetResistanceBaseline());
  RenalVeinToVenaCavaConnection.GetResistanceBaseline().Invalidate();

  // Update the circuit
  cCombinedCardiovascular.SetNextAndCurrentFromBaselines();
  cCombinedCardiovascular.StateChange();

  ////////////////////////
  // Renal Compartments //
  ////////////////////////

  ///////////
  // Blood //
  ///////////

  ///////////////////////
  // RightRenalArtery //
  SELiquidCompartment& vRenalArtery = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::RenalArtery);
  vRenalArtery.MapNode(RenalArtery);
  //////////////////////////////
  // AfferentArteriole //
  SELiquidCompartment& vAfferentArteriole = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::AfferentArteriole);
  vAfferentArteriole.MapNode(AfferentArteriole);
  ////////////////////////////////
  // GlomerularCapillaries //
  SELiquidCompartment& vGlomerularCapillaries = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::GlomerularCapillaries);
  vGlomerularCapillaries.MapNode(GlomerularCapillaries);
  vGlomerularCapillaries.MapNode(NetGlomerularCapillaries);
  ////////////////////////////
  // EfferentArteriole //
  SELiquidCompartment& vEfferentArteriole = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::EfferentArteriole);
  vEfferentArteriole.MapNode(EfferentArteriole);
  /////////////////////////////////
  // PeritubularCapillaries //
  SELiquidCompartment& vPeritubularCapillaries = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::PeritubularCapillaries);
  vPeritubularCapillaries.MapNode(PeritubularCapillaries);
  vPeritubularCapillaries.MapNode(NetPeritubularCapillaries);
  ///////////////////
  // RenalVein //
  SELiquidCompartment& vRenalVein = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::RenalVein);
  vRenalVein.MapNode(RenalVein);
  //////////////////////////
  // BowmansCapsules //
  SELiquidCompartment& vBowmansCapsules = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::BowmansCapsules);
  vBowmansCapsules.MapNode(BowmansCapsules);
  vBowmansCapsules.MapNode(NetBowmansCapsules);
  //////////////////
  // Tubules //
  SELiquidCompartment& vTubules = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Tubules);
  vTubules.MapNode(Tubules);
  vTubules.MapNode(NetTubules);

  // Let's build out the hierarchy
  // Grab these, as cardiovascular already made them
  SELiquidCompartment& vKidney = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Kidney);
  SELiquidCompartment& vNephron = m_Compartments->CreateLiquidCompartment(BGE::VascularLiteCompartment::Nephron);
  vKidney.AddChild(vRenalArtery);
  vKidney.AddChild(vNephron);
  vKidney.AddChild(vRenalVein);
  vNephron.AddChild(vAfferentArteriole);
  vNephron.AddChild(vGlomerularCapillaries);
  vNephron.AddChild(vEfferentArteriole);
  vNephron.AddChild(vPeritubularCapillaries);
  vNephron.AddChild(vBowmansCapsules);
  vNephron.AddChild(vTubules);

  ///////////
  // Urine //
  ///////////

  /////////////////
  // Ureter //
  SELiquidCompartment& uUreter = m_Compartments->CreateLiquidCompartment(BGE::UrineLiteCompartment::Ureter);
  uUreter.MapNode(Ureter);
  ////////////////

  /////////////
  // Bladder //
  SELiquidCompartment& uBladder = m_Compartments->CreateLiquidCompartment(BGE::UrineLiteCompartment::Bladder);
  uBladder.MapNode(Bladder);

  /////////////////////////////
  // Renal Compartment Links //
  /////////////////////////////

  // Graph Dependencies
  SELiquidCompartment& vAorta = *m_Compartments->GetLiquidCompartment(BGE::VascularLiteCompartment::Aorta);
  SELiquidCompartment& vVenaCava = *m_Compartments->GetLiquidCompartment(BGE::VascularLiteCompartment::VenaCava);
  SELiquidCompartment& vGround = *m_Compartments->GetLiquidCompartment(BGE::VascularLiteCompartment::Ground);

  ///////////
  // Blood //
  ///////////

  ////////////////////////////
  // AortaToRenalArtery, update left and right //
  m_Compartments->DeleteLiquidLink(BGE::VascularLiteLink::AortaToLeftKidney); // Replace this link
  m_Compartments->DeleteLiquidLink(BGE::VascularLiteLink::AortaToRightKidney); // Replace this link
  SELiquidCompartmentLink& vAortaToRenalArtery = m_Compartments->CreateLiquidLink(vAorta, vRenalArtery, BGE::VascularLiteLink::AortaToKidney);
  vAortaToRenalArtery.MapPath(AortaConnectionToRenalArtery);
  ////////////////////////////////////////
  // RenalArteryToAfferentArteriole //
  SELiquidCompartmentLink& vRenalArteryToAfferentArteriole = m_Compartments->CreateLiquidLink(vRenalArtery, vAfferentArteriole, BGE::VascularLiteLink::RenalArteryToAfferentArteriole);
  vRenalArteryToAfferentArteriole.MapPath(RenalArteryToAfferentArteriole);
  //////////////////////////////////////////////////
  // AfferentArterioleToGlomerularCapillaries //
  SELiquidCompartmentLink& vAfferentArterioleToGlomerularCapillaries = m_Compartments->CreateLiquidLink(vAfferentArteriole, vGlomerularCapillaries, BGE::VascularLiteLink::AfferentArterioleToGlomerularCapillaries);
  vAfferentArterioleToGlomerularCapillaries.MapPath(AfferentArterioleToGlomerularCapillaries);
  //////////////////////////////////////////////////
  // GlomerularCapillariesToEfferentArteriole //
  SELiquidCompartmentLink& vGlomerularCapillariesToEfferentArteriole = m_Compartments->CreateLiquidLink(vGlomerularCapillaries, vEfferentArteriole, BGE::VascularLiteLink::GlomerularCapillariesToEfferentArteriole);
  vGlomerularCapillariesToEfferentArteriole.MapPath(GlomerularCapillariesToEfferentArteriole);
  /////////////////////////////////////////////////
  // GlomerularCapillariesToBowmansCapsules //
  SELiquidCompartmentLink& vGlomerularCapillariesToBowmansCapsules = m_Compartments->CreateLiquidLink(vGlomerularCapillaries, vBowmansCapsules, BGE::VascularLiteLink::GlomerularCapillariesToBowmansCapsules);
  vGlomerularCapillariesToBowmansCapsules.MapPath(NetGlomerularCapillariesToNetBowmansCapsules);
  ///////////////////////////////////
  // BowmansCapsulesToTubules //
  SELiquidCompartmentLink& vBowmansCapsulesToTubules = m_Compartments->CreateLiquidLink(vBowmansCapsules, vTubules, BGE::VascularLiteLink::BowmansCapsulesToTubules);
  vBowmansCapsulesToTubules.MapPath(BowmansCapsulesToTubules);
  //////////////////////////////////////////
  // TubulesToPeritubularCapillaries //
  SELiquidCompartmentLink& vTubulesToPeritubularCapillaries = m_Compartments->CreateLiquidLink(vTubules, vPeritubularCapillaries, BGE::VascularLiteLink::TubulesToPeritubularCapillaries);
  vTubulesToPeritubularCapillaries.MapPath(NetTubulesToNetPeritubularCapillaries);
  ///////////////////////////////////////////////////
  // EfferentArterioleToPeritubularCapillaries //
  SELiquidCompartmentLink& vEfferentArterioleToPeritubularCapillaries = m_Compartments->CreateLiquidLink(vEfferentArteriole, vPeritubularCapillaries, BGE::VascularLiteLink::EfferentArterioleToPeritubularCapillaries);
  vEfferentArterioleToPeritubularCapillaries.MapPath(EfferentArterioleToPeritubularCapillaries);
  ////////////////////////////////////////////
  // PeritubularCapillariesToRenalVein //
  SELiquidCompartmentLink& vPeritubularCapillariesToRenalVein = m_Compartments->CreateLiquidLink(vPeritubularCapillaries, vRenalVein, BGE::VascularLiteLink::PeritubularCapillariesToRenalVein);
  vPeritubularCapillariesToRenalVein.MapPath(PeritubularCapillariesToRenalVein);
  /////////////////////////////
  // RenalVeinToVenaCava, delete right //
  m_Compartments->DeleteLiquidLink(BGE::VascularLiteLink::RightKidneyToVenaCava); // Replace this link
  m_Compartments->DeleteLiquidLink(BGE::VascularLiteLink::LeftKidneyToVenaCava); // Replace this link
  SELiquidCompartmentLink& vRenalVeinToVenaCava = m_Compartments->CreateLiquidLink(vRenalVein, vVenaCava, BGE::VascularLiteLink::KidneyToVenaCava);
  vRenalVeinToVenaCava.MapPath(RenalVeinToVenaCavaConnection);
  /////////////////////////////
  // Hemorrhage //
  SELiquidCompartmentLink& vKidneyHemorrhage = m_Compartments->CreateLiquidLink(vRenalVein, vGround, BGE::VascularLiteLink::KidneyHemorrhage);
  vKidneyHemorrhage.MapPath(KidneyBleed);


  ///////////
  // Urine //
  ///////////

  //////////////////////////
  // TubulesToUreter //
  SELiquidCompartmentLink& uTubulesToUreter = m_Compartments->CreateLiquidLink(vTubules, uUreter, BGE::UrineLiteLink::TubulesToUreter);
  uTubulesToUreter.MapPath(TubulesToUreter);
  //////////////////////////
  // UreterToBladder //
  SELiquidCompartmentLink& uUreterToBladder = m_Compartments->CreateLiquidLink(uUreter, uBladder, BGE::UrineLiteLink::UreterToBladder);
  uUreterToBladder.MapPath(UreterToBladder);

  /////////////////////
  // BladderToGround //
  SELiquidCompartmentLink& uBladderToGround = m_Compartments->CreateLiquidLink(uBladder, vGround, BGE::UrineLiteLink::BladderToGround);
  uBladderToGround.MapPath(BladderToGroundUrinate);
  SELiquidCompartmentLink& uBladderToGroundSource = m_Compartments->CreateLiquidLink(uBladder, vGround, BGE::UrineLiteLink::BladderToGroundSource);
  uBladderToGroundSource.MapPath(BladderToGroundPressure);

  SELiquidCompartmentGraph& gRenal = m_Compartments->GetRenalGraph();
  gRenal.AddCompartment(vAorta);
  gRenal.AddCompartment(vVenaCava);

  // Blood
  gRenal.AddCompartment(vRenalArtery);
  gRenal.AddCompartment(vAfferentArteriole);
  gRenal.AddCompartment(vGlomerularCapillaries);
  gRenal.AddCompartment(vEfferentArteriole);
  gRenal.AddCompartment(vPeritubularCapillaries);
  gRenal.AddCompartment(vBowmansCapsules);
  gRenal.AddCompartment(vTubules);
  gRenal.AddCompartment(vRenalVein);
  gRenal.AddLink(vAortaToRenalArtery);
  gRenal.AddLink(vRenalArteryToAfferentArteriole);
  gRenal.AddLink(vAfferentArterioleToGlomerularCapillaries);
  gRenal.AddLink(vGlomerularCapillariesToEfferentArteriole);
  gRenal.AddLink(vKidneyHemorrhage);
  //gRenal.AddLink(vGlomerularCapillariesToBowmansCapsules); //Active transport only
  gRenal.AddLink(vBowmansCapsulesToTubules);
  //gRenal.AddLink(vTubulesToPeritubularCapillaries); //Active transport only
  gRenal.AddLink(vEfferentArterioleToPeritubularCapillaries);
  gRenal.AddLink(vPeritubularCapillariesToRenalVein);
  gRenal.AddLink(vRenalVeinToVenaCava);
  //  Urine
  gRenal.AddCompartment(uUreter);
  gRenal.AddLink(uTubulesToUreter);
  gRenal.AddLink(uUreterToBladder);
  // Shared
  gRenal.AddCompartment(uBladder);
  gRenal.AddCompartment(vGround);
  gRenal.AddLink(uBladderToGround);
  gRenal.AddLink(uBladderToGroundSource);
  gRenal.StateChange();

  // We have discretized these compartments, so remove them
  SELiquidCompartmentGraph& gCombinedCardiovascular = m_Compartments->GetActiveCardiovascularGraph();
  SELiquidCompartment* vKidneys = m_Compartments->GetLiquidCompartment(BGE::VascularCompartment::Kidneys);
  SELiquidCompartment* vLeftKidney = m_Compartments->GetLiquidCompartment(BGE::VascularCompartment::LeftKidney);
  SELiquidCompartment* vRightKidney = m_Compartments->GetLiquidCompartment(BGE::VascularCompartment::RightKidney);
  gCombinedCardiovascular.RemoveCompartment(*vLeftKidney);
  gCombinedCardiovascular.RemoveCompartment(*vRightKidney);
  gCombinedCardiovascular.RemoveCompartment(*vKidneys);
  vKidneys->RemoveChild(BGE::VascularCompartment::LeftKidney);
  vKidneys->RemoveChild(BGE::VascularCompartment::RightKidney);
  m_Compartments->DeleteLiquidCompartment(BGE::VascularCompartment::LeftKidney);
  m_Compartments->DeleteLiquidCompartment(BGE::VascularCompartment::RightKidney);
  m_Compartments->DeleteLiquidCompartment(BGE::VascularCompartment::Kidneys);


  gCombinedCardiovascular.AddGraph(gRenal);
  gCombinedCardiovascular.StateChange();
}

void BioGears::SetupTissue()
{
  Info("Setting Up Tissue");
  SEFluidCircuit& cCardiovascular = m_Circuits->GetCardiovascularCircuit();
  SEFluidCircuit& cCombinedCardiovascular = m_Circuits->GetActiveCardiovascularCircuit();
  SELiquidCompartmentGraph& gCombinedCardiovascular = m_Compartments->GetActiveCardiovascularGraph();

  SEFluidCircuitNode* Ground = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::Ground);
  ///////////
  // Lymph //
  SEFluidCircuitNode& Lymph = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::Lymph);
  Lymph.GetPressure().SetValue(5.0, PressureUnit::mmHg);
  //If we break out Lymph, Table 8.5 in Nanomedicine: Volume 1 (found @ http://www.nanomedicine.com/NMI/8.2.1.3.htm) has a total capacity of ~2.0 L.
  Lymph.GetVolumeBaseline().SetValue(2.0, VolumeUnit::L);
  double lymphTotalBody_mL_Per_min = 3.5; //This corresponds to ~ 5 L/day of lymph flow in body

  SEFluidCircuitNode* VenaCava = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::VenaCava);
  SEFluidCircuitPath& LymphToVenaCava = cCombinedCardiovascular.CreatePath(Lymph, *VenaCava, BGE::TissueLitePath::LymphToVenaCava);
  LymphToVenaCava.GetResistanceBaseline().SetValue((Lymph.GetPressure(PressureUnit::mmHg) - VenaCava->GetPressure(PressureUnit::mmHg)) / lymphTotalBody_mL_Per_min, FlowResistanceUnit::mmHg_min_Per_mL);
  SEFluidCircuitPath& LymphToGround = cCombinedCardiovascular.CreatePath(Lymph, *Ground, BGE::TissueLitePath::LymphToGround);
  LymphToGround.GetComplianceBaseline().SetValue(Lymph.GetVolumeBaseline(VolumeUnit::mL) / Lymph.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);
  ///\ToDo:  Use P-V relationship in Himeno2015Mechanisms to get better initial compliance estimate

  SELiquidCompartment* cVenaCava = m_Compartments->GetLiquidCompartment(BGE::VascularLiteCompartment::VenaCava);
  SELiquidCompartment& cLymph = m_Compartments->CreateLiquidCompartment(BGE::LymphCompartment::Lymph);
  cLymph.MapNode(Lymph);

  gCombinedCardiovascular.AddCompartment(cLymph);

  /// \todo Put Initial Circuit/Compartment data values into the configuration file.

  //Density (kg/L)
  double AdiposeTissueDensity = 0.92;
  double BoneTissueDensity = 1.3;
  double BrainTissueDensity = 1.0;
  double GutTissueDensity = 1.0;
  double RKidneyTissueDensity = 1.0;
  double LKidneyTissueDensity = 1.0;
  double LiverTissueDensity = 1.0;
  double RLungTissueDensity = 1.0;
  double LLungTissueDensity = 1.0;
  double MuscleTissueDensity = 1.0;
  double MyocardiumTissueDensity = 1.0;
  double SkinTissueDensity = 1.0;
  double SpleenTissueDensity = 1.0;

  // ExtracellcularWaterFraction        IntracellularWaterFraction    NeutralLipid                   NeutralPhospolipid             AlbuminRatio              AlphaAcidGlycoprotein       PlasmaLipoprotein        AcidicPhospohlipidConcentration
  double AdiposeEWFraction = 0.135, AdiposeIWFraction = 0.017, AdiposeNLFraction = 0.79, AdiposeNPFraction = 0.002, AdiposeARatio = 0.049, AdiposeAAGRatio = 0.049, AdiposeLRatio = 0.068, AdiposeAPL = 0.4;
  double BoneEWFraction = 0.1, BoneIWFraction = 0.346, BoneNLFraction = 0.074, BoneNPFraction = 0.0011, BoneARatio = 0.1, BoneAAGRatio = 0.1, BoneLRatio = 0.05, BoneAPL = 0.67;
  double BrainEWFraction = 0.162, BrainIWFraction = 0.62, BrainNLFraction = 0.051, BrainNPFraction = 0.0565, BrainARatio = 0.048, BrainAAGRatio = 0.048, BrainLRatio = 0.041, BrainAPL = 0.4;
  double GutEWFraction = 0.282, GutIWFraction = 0.475, GutNLFraction = 0.0487, GutNPFraction = 0.0163, GutARatio = 0.158, GutAAGRatio = 0.158, GutLRatio = 0.0141, GutAPL = 2.41;
  double RKidneyEWFraction = 0.273, RKidneyIWFraction = 0.483, RKidneyNLFraction = 0.0207, RKidneyNPFraction = 0.0162, RKidneyARatio = 0.13, RKidneyAAGRatio = 0.13, RKidneyLRatio = 0.137, RKidneyAPL = 5.03;
  double LKidneyEWFraction = 0.273, LKidneyIWFraction = 0.483, LKidneyNLFraction = 0.0207, LKidneyNPFraction = 0.0162, LKidneyARatio = 0.13, LKidneyAAGRatio = 0.13, LKidneyLRatio = 0.137, LKidneyAPL = 5.03;
  double LiverEWFraction = 0.161, LiverIWFraction = 0.573, LiverNLFraction = 0.0348, LiverNPFraction = 0.0252, LiverARatio = 0.086, LiverAAGRatio = 0.086, LiverLRatio = 0.161, LiverAPL = 4.56;
  double RLungEWFraction = 0.336, RLungIWFraction = 0.446, RLungNLFraction = 0.003, RLungNPFraction = 0.009, RLungARatio = 0.212, RLungAAGRatio = 0.212, RLungLRatio = 0.168, RLungAPL = 3.91;
  double LLungEWFraction = 0.336, LLungIWFraction = 0.446, LLungNLFraction = 0.003, LLungNPFraction = 0.009, LLungARatio = 0.212, LLungAAGRatio = 0.212, LLungLRatio = 0.168, LLungAPL = 3.91;
  double MuscleEWFraction = 0.118, MuscleIWFraction = 0.63, MuscleNLFraction = 0.0238, MuscleNPFraction = 0.0072, MuscleARatio = 0.064, MuscleAAGRatio = 0.064, MuscleLRatio = 0.059, MuscleAPL = 1.53;
  double MyocardiumEWFraction = 0.32, MyocardiumIWFraction = 0.456, MyocardiumNLFraction = 0.0115, MyocardiumNPFraction = 0.0166, MyocardiumARatio = 0.157, MyocardiumAAGRatio = 0.157, MyocardiumLRatio = 0.16, MyocardiumAPL = 2.25;
  double SkinEWFraction = 0.382, SkinIWFraction = 0.291, SkinNLFraction = 0.0284, SkinNPFraction = 0.0111, SkinARatio = 0.277, SkinAAGRatio = 0.277, SkinLRatio = 0.096, SkinAPL = 1.32;
  double SpleenEWFraction = 0.207, SpleenIWFraction = 0.579, SpleenNLFraction = 0.0201, SpleenNPFraction = 0.0198, SpleenARatio = 0.277, SpleenAAGRatio = 0.277, SpleenLRatio = 0.096, SpleenAPL = 3.18;

  //Typical ICRP Male
  //Total Mass (kg)
  double AdiposeTissueMass = 14.5;
  double BoneTissueMass = 10.5;
  double BrainTissueMass = 1.45;
  double GutTissueMass = 1.02;
  double RKidneyTissueMass = 0.155;
  double LKidneyTissueMass = 0.155;
  double LiverTissueMass = 1.8;
  double RLungTissueMass = 0.25;
  double LLungTissueMass = 0.25;
  double MuscleTissueMass = 29.0;
  double MyocardiumTissueMass = 0.33;
  double SkinTissueMass = 3.3;
  double SpleenTissueMass = 0.15;

  //Typical ICRP Female - From ICRP
  //Total Mass (kg)
  if (m_Patient->GetSex() == CDM::enumSex::Female) {
    AdiposeTissueMass = 19.0;
    BoneTissueMass = 7.8;
    BrainTissueMass = 1.3;
    GutTissueMass = 0.96;
    RKidneyTissueMass = 0.1375;
    LKidneyTissueMass = 0.1375;
    LiverTissueMass = 1.4;
    RLungTissueMass = 0.21;
    LLungTissueMass = 0.21;
    MuscleTissueMass = 17.5;
    MyocardiumTissueMass = 0.25;
    SkinTissueMass = 2.3;
    SpleenTissueMass = 0.13;
  }

  //Scale things based on patient parameters -------------------------------

  //Modify adipose (i.e. fat) directly using the body fat fraction
  AdiposeTissueMass = m_Patient->GetBodyFatFraction().GetValue() * m_Patient->GetWeight().GetValue(MassUnit::kg);

  //Modify skin based on total surface area
  //Male
  double standardPatientWeight_lb = 170.0;
  double standardPatientHeight_in = 71.0;
  if (m_Patient->GetSex() == CDM::enumSex::Female) {
    //Female
    standardPatientWeight_lb = 130.0;
    standardPatientHeight_in = 64.0;
  }
  double typicalSkinSurfaceArea_m2 = 0.20247 * std::pow(Convert(standardPatientWeight_lb, MassUnit::lb, MassUnit::kg), 0.425) * std::pow(Convert(standardPatientHeight_in, LengthUnit::in, LengthUnit::m), 0.725);
  double patientSkinArea_m2 = m_Patient->GetSkinSurfaceArea(AreaUnit::m2);
  SkinTissueMass = SkinTissueMass * patientSkinArea_m2 / typicalSkinSurfaceArea_m2;

  //Modify most based on lean body mass
  //Hume, R (Jul 1966). "Prediction of lean body mass from height and weight." Journal of clinical pathology. 19 (4): 389–91. doi:10.1136/jcp.19.4.389. PMC 473290. PMID 5929341.
  //double typicalLeanBodyMass_kg = 0.32810 * Convert(standardPatientWeight_lb, MassUnit::lb, MassUnit::kg) + 0.33929 * Convert(standardPatientHeight_in, LengthUnit::in, LengthUnit::cm) - 29.5336; //Male
  //if (m_Patient->GetSex() == CDM::enumSex::Female)
  //{
  // typicalLeanBodyMass_kg = 0.29569 * Convert(standardPatientWeight_lb, MassUnit::lb, MassUnit::kg) + 0.41813 * Convert(standardPatientHeight_in, LengthUnit::in, LengthUnit::cm) - 43.2933; //Female
  //}

  //Male
  double standardFatFraction = 0.21;
  if (m_Patient->GetSex() == CDM::enumSex::Female) {
    //Female
    standardFatFraction = 0.28;
  }
  double standardLeanBodyMass_kg = Convert(standardPatientWeight_lb, MassUnit::lb, MassUnit::kg) * (1.0 - standardFatFraction);
  double patientLeanBodyMass_kg = m_Patient->GetLeanBodyMass(MassUnit::kg);
  double leanBodyMassFractionOfTypical = patientLeanBodyMass_kg / standardLeanBodyMass_kg;

  BoneTissueMass *= leanBodyMassFractionOfTypical;
  GutTissueMass *= leanBodyMassFractionOfTypical;
  RKidneyTissueMass *= leanBodyMassFractionOfTypical;
  LKidneyTissueMass *= leanBodyMassFractionOfTypical;
  LiverTissueMass *= leanBodyMassFractionOfTypical;
  RLungTissueMass *= leanBodyMassFractionOfTypical;
  LLungTissueMass *= leanBodyMassFractionOfTypical;
  MuscleTissueMass *= leanBodyMassFractionOfTypical;
  MyocardiumTissueMass *= leanBodyMassFractionOfTypical;
  SpleenTissueMass *= leanBodyMassFractionOfTypical;

  //Note: Brain doesn't change

  //Total Volume(L)
  double AdiposeTissueVolume = AdiposeTissueMass / AdiposeTissueDensity;
  double BoneTissueVolume = BoneTissueMass / BoneTissueDensity;
  double BrainTissueVolume = BrainTissueMass / BrainTissueDensity;
  double GutTissueVolume = GutTissueMass / GutTissueDensity;
  double RKidneyTissueVolume = RKidneyTissueMass / RKidneyTissueDensity;
  double LKidneyTissueVolume = LKidneyTissueMass / LKidneyTissueDensity;
  double LiverTissueVolume = LiverTissueMass / LiverTissueDensity;
  double RLungTissueVolume = RLungTissueMass / RLungTissueDensity;
  double LLungTissueVolume = LLungTissueMass / LLungTissueDensity;
  double MuscleTissueVolume = MuscleTissueMass / MuscleTissueDensity;
  double MyocardiumTissueVolume = MyocardiumTissueMass / MyocardiumTissueDensity;
  double SkinTissueVolume = SkinTissueMass / SkinTissueDensity;
  double SpleenTissueVolume = SpleenTissueMass / SpleenTissueDensity;

  double totalECWater_L = AdiposeEWFraction * AdiposeTissueVolume + BoneEWFraction * BoneTissueVolume + BrainEWFraction * BrainTissueVolume + GutEWFraction * GutTissueVolume
    + LiverEWFraction * LiverTissueVolume + RLungEWFraction * RLungTissueVolume + LLungEWFraction * LLungTissueVolume + MuscleEWFraction * MuscleTissueVolume + MyocardiumEWFraction * MyocardiumTissueVolume + SkinEWFraction * SkinTissueVolume
    + SpleenEWFraction * SpleenTissueVolume + LKidneyTissueVolume * LKidneyEWFraction + RKidneyTissueVolume * RKidneyEWFraction;

  // Colloid Osmotic Pressure--establish here so we can define pressure sources on tissue circuit (substances haven't been set up yet) and, if we need to, change them without jacking everything up
  /// \cite Mazonni1988Dynamic
  double albuminVascular_g_Per_dL = 4.5;
  double albuminExtracell_g_Per_dL = 2.0;
  double totalPlasamaProtein_g_Per_dL = 1.6 * albuminVascular_g_Per_dL; //Relationship between albumin and total plasma protein
  double totalInterstitialProtein_g_Per_dL = 1.6 * albuminExtracell_g_Per_dL;
  double copVascular_mmHg = 2.1 * totalPlasamaProtein_g_Per_dL + 0.16 * std::pow(totalPlasamaProtein_g_Per_dL, 2) + 0.009 * std::pow(totalPlasamaProtein_g_Per_dL, 3); //Use Landis-Pappenheimer equation to get plasma colloid oncotic pressure
  double copExtracell_mmHg = 2.1 * totalInterstitialProtein_g_Per_dL + 0.16 * std::pow(totalInterstitialProtein_g_Per_dL, 2) + 0.009 * std::pow(totalInterstitialProtein_g_Per_dL, 3); //If we assume only albumin leaks across membrame, use relationshp for albumin colloid pressure from Mazzoni1988Dynamic

  double targetPressureGradient_mmHg = 5.0;
  //Boron: Medical Physiology has total pressure gradient from capillary to interstitium as 12 mmHg at arterial end and -5 mmHg at venous end
  //So there should be a small net gradient towards interstitium.
  double targetHydrostaticGradient_mmHg = targetPressureGradient_mmHg + (copVascular_mmHg - copExtracell_mmHg); //COP gradient opposes flow into capillaries.  Thus, to get to target total gradient we need a hydrostatic gradient to oppose it

  //These modifiers are copied from the SetUp Cardiovascular function.  The volume modification helps tune the vascular node volumes, but it also causes pressure drops
  //on those nodes.  This is a problem because the tissue nodes below have their pressures set based on the pressures set on the CV nodes.  If the pressure in the vasculature
  //drops too much, the gradient favoring filtration to the tissue is lost.  Factoring in these modifiers prevents this.  Note: Did not include modifier for brain, lungs, or kidneys.
  double VolumeModifierBone = 1.175574 * 0.985629, VolumeModifierFat = 1.175573 * 0.986527;
  double VolumeModifierGut = 1.17528 * 0.985609, VolumeModifierLiver = 1.157475 * 0.991848;
  double VolumeModifierMuscle = 1.175573 * 0.986529, VolumeModifierMyocardium = 1.175564 * 0.986531;
  double VolumeModifierSpleen = 1.17528 * 0.986509, VolumeModifierSkin = 1.007306 * 1.035695;

  //Use these and keep recycling for each tissue to help define node and path baselines
  double vNodePressure = 0.0;
  double e1NodePressure = 0.0;
  double e2NodePressure = 0.0;
  double e3NodePressure = 0.0;
  double l1NodePressure = 0.0;
  double l2NodePressure = 0.0;
  double preLymphaticPressureMin_mmHg = 6.0;
  double filteredFlow_mL_Per_min = 0.0;
  double capillaryResistance_mmHg_min_Per_mL = 0.0;
  double lymphResistance_mmHg_min_Per_mL = 0.0;
  double lymphDrivePressure_mmHg = 0.0;

  //Circuit Set-Up
  /*
   V = vascular, E = tissue extracellular, I = tissue intracellular
  >> = pressure source (and direction), ~~~ = resistor, || = capacitor, ( = flow source
  
                              I---||----Ground
                              (
                              (
    V--<<---E1--~~~~-E2--->>--E3--||----Ground
                              v
                              v
                Vena cava-~~~-L
  
  */

  //Need to redefine some of the tuning constants created at the top of SetUpTissue.  Going to take a volume weighted approach
  //"Gut Lite" combines main engine gut (small intestine, large intestine, GutLite) with spleen
  double GutLiteTissueVolume = GutTissueVolume + SpleenTissueVolume;
  double GutLiteTissueMass = GutLiteTissueVolume; //All included compartments have density 1 kg/L defined above, so mass and volume are equal
  double VolumeModifierGutLite = VolumeModifierGut * (GutTissueVolume / GutLiteTissueVolume) + VolumeModifierSpleen * (SpleenTissueVolume / GutLiteTissueVolume);
  double GutLiteEWFraction = GutEWFraction * (GutTissueVolume / GutLiteTissueVolume) + SpleenEWFraction * (SpleenTissueVolume / GutLiteTissueVolume);
  double GutLiteIWFraction = GutIWFraction * (GutTissueVolume / GutLiteTissueVolume) + SpleenIWFraction * (SpleenTissueVolume / GutLiteTissueVolume);
  double GutLiteNLFraction = GutNLFraction * (GutTissueVolume / GutLiteTissueVolume) + SpleenNLFraction * (SpleenTissueVolume / GutLiteTissueVolume);
  double GutLiteNPFraction = GutNPFraction * (GutTissueVolume / GutLiteTissueVolume) + SpleenNPFraction * (SpleenTissueVolume / GutLiteTissueVolume);
  double GutLiteARatio = GutARatio * (GutTissueVolume / GutLiteTissueVolume) + SpleenARatio * (SpleenTissueVolume / GutLiteTissueVolume);
  double GutLiteAAGRatio = GutAAGRatio * (GutTissueVolume / GutLiteTissueVolume) + SpleenAAGRatio * (SpleenTissueVolume / GutLiteTissueVolume);
  double GutLiteLRatio = GutLRatio * (GutTissueVolume / GutLiteTissueVolume) + SpleenLRatio * (SpleenTissueVolume / GutLiteTissueVolume);
  double GutLiteAPL = GutAPL * (GutTissueVolume / GutLiteTissueVolume) + SpleenAPL * (SpleenTissueVolume / GutLiteTissueVolume);
  //Kidney Combined Variables--add mass/volume and just grab left kidney tissue parameters since these are identical for left/right
  double KidneyTissueVolume = LKidneyTissueVolume + RKidneyTissueVolume, KidneyTissueMass = LKidneyTissueMass + RKidneyTissueMass, KidneyEWFraction = LKidneyEWFraction, KidneyIWFraction = LKidneyIWFraction;
  double KidneyNLFraction = LKidneyNLFraction, KidneyNPFraction = LKidneyNPFraction, KidneyARatio = LKidneyARatio, KidneyAAGRatio = LKidneyAAGRatio, KidneyLRatio = LKidneyLRatio, KidneyAPL = LKidneyAPL;
  //Lung Combined Variables--add mas/volume and grab left lung parameters since these are identical for left/right
  double LungTissueVolume = LLungTissueVolume + RLungTissueVolume, LungTissueMass = LLungTissueMass + RLungTissueMass, LungEWFraction = LLungEWFraction, LungIWFraction = LLungIWFraction;
  double LungNLFraction = LLungNLFraction, LungNPFraction = LLungNPFraction, LungARatio = LLungARatio, LungAAGRatio = LLungAAGRatio, LungLRatio = LLungLRatio, LungAPL = LLungAPL;
  //Update total extracellular fluid volume in case we got off a litte bit when weighting GutLite compartment volumes
  totalECWater_L = AdiposeEWFraction * AdiposeTissueVolume + BoneEWFraction * BoneTissueVolume + BrainEWFraction * BrainTissueVolume + KidneyTissueVolume * KidneyEWFraction + LiverEWFraction * LiverTissueVolume
    + LungEWFraction * LungTissueVolume + MuscleEWFraction * MuscleTissueVolume + MyocardiumEWFraction * MyocardiumTissueVolume + SkinEWFraction * SkinTissueVolume + GutLiteTissueVolume * GutLiteEWFraction;

  /////////
  // Fat //
  SEFluidCircuitNode* FatV = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::Fat1);
  SEFluidCircuitNode& FatE1 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::FatE1);
  SEFluidCircuitNode& FatE2 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::FatE2);
  SEFluidCircuitNode& FatE3 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::FatE3);
  SEFluidCircuitNode& FatI = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::FatI);
  SEFluidCircuitNode& FatL = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::FatL); //Pre-lymphatic node

  vNodePressure = FatV->GetPressure(PressureUnit::mmHg) / VolumeModifierFat;
  e1NodePressure = vNodePressure - copVascular_mmHg; //Plasma colloid osmotic pressure opposes flow into tissue space (i.e. favor E1 to V)
  e3NodePressure = vNodePressure - targetHydrostaticGradient_mmHg;
  e2NodePressure = e3NodePressure - copExtracell_mmHg; //Extracellular colloid osmotic pressure promotes flow from E2 to E3
  if (e3NodePressure > preLymphaticPressureMin_mmHg) {
    l1NodePressure = e3NodePressure;
  } else {
    l1NodePressure = preLymphaticPressureMin_mmHg;
  }
  filteredFlow_mL_Per_min = (AdiposeTissueVolume * AdiposeEWFraction) / totalECWater_L * lymphTotalBody_mL_Per_min;
  capillaryResistance_mmHg_min_Per_mL = (e1NodePressure - e2NodePressure) / filteredFlow_mL_Per_min;
  lymphDrivePressure_mmHg = l1NodePressure - e3NodePressure;
  lymphResistance_mmHg_min_Per_mL = (l1NodePressure - l2NodePressure) / filteredFlow_mL_Per_min;

  FatE1.GetPressure().SetValue(e1NodePressure, PressureUnit::mmHg);
  FatE2.GetPressure().SetValue(e2NodePressure, PressureUnit::mmHg);
  FatE3.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg);
  FatE3.GetVolumeBaseline().SetValue(AdiposeEWFraction * AdiposeTissueVolume * 1000.0, VolumeUnit::mL);
  FatI.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg); //No hydrostatic pressure difference between intra/extra
  FatI.GetVolumeBaseline().SetValue(AdiposeIWFraction * AdiposeTissueVolume * 1000.0, VolumeUnit::mL); //intracellular node
  FatL.GetPressure().SetValue(l1NodePressure, PressureUnit::mmHg);

  SEFluidCircuitPath& FatVToFatE1 = cCombinedCardiovascular.CreatePath(*FatV, FatE1, BGE::TissueLitePath::FatVToFatE1);
  FatVToFatE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg); // < 0 because directed from extracellular to vascular
  SEFluidCircuitPath& FatE1ToFatE2 = cCombinedCardiovascular.CreatePath(FatE1, FatE2, BGE::TissueLitePath::FatE1ToFatE2);
  FatE1ToFatE2.GetResistanceBaseline().SetValue(capillaryResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  SEFluidCircuitPath& FatE2ToFatE3 = cCombinedCardiovascular.CreatePath(FatE2, FatE3, BGE::TissueLitePath::FatE2ToFatE3);
  FatE2ToFatE3.GetPressureSourceBaseline().SetValue(copExtracell_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& FatE3ToGround = cCombinedCardiovascular.CreatePath(FatE3, *Ground, BGE::TissueLitePath::FatE3ToGround);
  FatE3ToGround.GetComplianceBaseline().SetValue(FatE3.GetVolumeBaseline(VolumeUnit::mL) / vNodePressure, FlowComplianceUnit::mL_Per_mmHg); //Might need to change this
  SEFluidCircuitPath& FatE3ToFatI = cCombinedCardiovascular.CreatePath(FatE3, FatI, BGE::TissueLitePath::FatE3ToFatI);
  FatE3ToFatI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
  SEFluidCircuitPath& FatIToGround = cCombinedCardiovascular.CreatePath(FatI, *Ground, BGE::TissueLitePath::FatIToGround);
  FatIToGround.GetComplianceBaseline().SetValue(FatI.GetVolumeBaseline(VolumeUnit::mL) / FatI.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& FatE3ToFatL = cCombinedCardiovascular.CreatePath(FatE3, FatL, BGE::TissueLitePath::FatE3ToFatL);
  FatE3ToFatL.GetPressureSourceBaseline().SetValue(lymphDrivePressure_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& FatLToLymph = cCombinedCardiovascular.CreatePath(FatL, Lymph, BGE::TissueLitePath::FatLToLymph);
  FatLToLymph.GetResistanceBaseline().SetValue(lymphResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  FatLToLymph.SetNextPolarizedState(CDM::enumOpenClosed::Open);

  SETissueCompartment& FatTissue = m_Compartments->CreateTissueCompartment(BGE::TissueLiteCompartment::Fat);
  SELiquidCompartment& FatExtracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::FatExtracellular);
  SELiquidCompartment& FatIntracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::FatIntracellular);
  FatTissue.GetMatrixVolume().SetValue((1 - AdiposeEWFraction - AdiposeIWFraction) * AdiposeTissueVolume * 1000.0, VolumeUnit::mL);
  FatExtracellular.MapNode(FatE1);
  FatExtracellular.MapNode(FatE2);
  FatExtracellular.MapNode(FatE3);
  FatIntracellular.MapNode(FatI);
  FatExtracellular.MapNode(FatL);
  FatExtracellular.GetWaterVolumeFraction().SetValue(AdiposeEWFraction);
  FatIntracellular.GetWaterVolumeFraction().SetValue(AdiposeIWFraction);
  FatTissue.GetAcidicPhospohlipidConcentration().SetValue(AdiposeAPL, MassPerMassUnit::mg_Per_g);
  FatTissue.GetNeutralLipidsVolumeFraction().SetValue(AdiposeNLFraction);
  FatTissue.GetNeutralPhospholipidsVolumeFraction().SetValue(AdiposeNPFraction);
  FatTissue.GetTissueToPlasmaAlbuminRatio().SetValue(AdiposeARatio);
  FatTissue.GetTissueToPlasmaAlphaAcidGlycoproteinRatio().SetValue(AdiposeAAGRatio);
  FatTissue.GetTissueToPlasmaLipoproteinRatio().SetValue(AdiposeLRatio);
  FatTissue.GetTotalMass().SetValue(AdiposeTissueMass, MassUnit::kg);
  FatTissue.GetMembranePotential().SetValue(-84.8, ElectricPotentialUnit::mV);
  FatTissue.GetReflectionCoefficient().SetValue(1.0);

  //////////
  // Bone //
  SEFluidCircuitNode* BoneV = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::Bone1);
  SEFluidCircuitNode& BoneE1 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::BoneE1);
  SEFluidCircuitNode& BoneE2 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::BoneE2);
  SEFluidCircuitNode& BoneE3 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::BoneE3);
  SEFluidCircuitNode& BoneI = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::BoneI);
  SEFluidCircuitNode& BoneL = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::BoneL); //Pre-lymphatic node

  vNodePressure = BoneV->GetPressure(PressureUnit::mmHg) / VolumeModifierBone;
  e1NodePressure = vNodePressure - copVascular_mmHg; //Plasma colloid osmotic pressure opposes flow into tissue space (i.e. favor E1 to V)
  e3NodePressure = vNodePressure - targetHydrostaticGradient_mmHg;
  e2NodePressure = e3NodePressure - copExtracell_mmHg; //Extracellular colloid osmotic pressure promotes flow from E2 to E3
  if (e3NodePressure > preLymphaticPressureMin_mmHg) {
    l1NodePressure = e3NodePressure;
  } else {
    l1NodePressure = preLymphaticPressureMin_mmHg;
  }

  filteredFlow_mL_Per_min = (BoneTissueVolume * BoneEWFraction) / totalECWater_L * lymphTotalBody_mL_Per_min;
  capillaryResistance_mmHg_min_Per_mL = (e1NodePressure - e2NodePressure) / filteredFlow_mL_Per_min;
  lymphDrivePressure_mmHg = l1NodePressure - e3NodePressure;
  lymphResistance_mmHg_min_Per_mL = (l1NodePressure - l2NodePressure) / filteredFlow_mL_Per_min;

  BoneE1.GetPressure().SetValue(e1NodePressure, PressureUnit::mmHg);
  BoneE2.GetPressure().SetValue(e2NodePressure, PressureUnit::mmHg);
  BoneE3.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg);
  BoneE3.GetVolumeBaseline().SetValue(BoneEWFraction * BoneTissueVolume * 1000.0, VolumeUnit::mL);
  BoneI.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg); //No hydrostatic pressure difference between intra/extra
  BoneI.GetVolumeBaseline().SetValue(BoneIWFraction * BoneTissueVolume * 1000.0, VolumeUnit::mL); //intracellular node
  BoneL.GetPressure().SetValue(l1NodePressure, PressureUnit::mmHg);

  SEFluidCircuitPath& BoneVToBoneE1 = cCombinedCardiovascular.CreatePath(*BoneV, BoneE1, BGE::TissueLitePath::BoneVToBoneE1);
  BoneVToBoneE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& BoneE1ToBoneE2 = cCombinedCardiovascular.CreatePath(BoneE1, BoneE2, BGE::TissueLitePath::BoneE1ToBoneE2);
  BoneE1ToBoneE2.GetResistanceBaseline().SetValue(capillaryResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  SEFluidCircuitPath& BoneE2ToBoneE3 = cCombinedCardiovascular.CreatePath(BoneE2, BoneE3, BGE::TissueLitePath::BoneE2ToBoneE3);
  BoneE2ToBoneE3.GetPressureSourceBaseline().SetValue(copExtracell_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& BoneE3ToGround = cCombinedCardiovascular.CreatePath(BoneE3, *Ground, BGE::TissueLitePath::BoneE3ToGround);
  BoneE3ToGround.GetComplianceBaseline().SetValue(BoneE3.GetVolumeBaseline(VolumeUnit::mL) / vNodePressure, FlowComplianceUnit::mL_Per_mmHg); //Might need to change this
  SEFluidCircuitPath& BoneE3ToBoneI = cCombinedCardiovascular.CreatePath(BoneE3, BoneI, BGE::TissueLitePath::BoneE3ToBoneI);
  BoneE3ToBoneI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
  SEFluidCircuitPath& BoneIToGround = cCombinedCardiovascular.CreatePath(BoneI, *Ground, BGE::TissueLitePath::BoneIToGround);
  BoneIToGround.GetComplianceBaseline().SetValue(BoneI.GetVolumeBaseline(VolumeUnit::mL) / BoneI.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& BoneE3ToBoneL = cCombinedCardiovascular.CreatePath(BoneE3, BoneL, BGE::TissueLitePath::BoneE3ToBoneL);
  BoneE3ToBoneL.GetPressureSourceBaseline().SetValue(lymphDrivePressure_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& BoneLToLymph = cCombinedCardiovascular.CreatePath(BoneL, Lymph, BGE::TissueLitePath::BoneLToLymph);
  BoneLToLymph.GetResistanceBaseline().SetValue(lymphResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  BoneLToLymph.SetNextPolarizedState(CDM::enumOpenClosed::Open);

  SETissueCompartment& BoneTissue = m_Compartments->CreateTissueCompartment(BGE::TissueLiteCompartment::Bone);
  SELiquidCompartment& BoneExtracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::BoneExtracellular);
  SELiquidCompartment& BoneIntracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::BoneIntracellular);
  BoneTissue.GetMatrixVolume().SetValue((1 - BoneEWFraction - BoneIWFraction) * BoneTissueVolume * 1000.0, VolumeUnit::mL);
  BoneExtracellular.MapNode(BoneE1);
  BoneExtracellular.MapNode(BoneE2);
  BoneExtracellular.MapNode(BoneE3);
  BoneIntracellular.MapNode(BoneI);
  BoneExtracellular.MapNode(BoneL);
  BoneExtracellular.GetWaterVolumeFraction().SetValue(BoneEWFraction);
  BoneIntracellular.GetWaterVolumeFraction().SetValue(BoneIWFraction);
  BoneTissue.GetAcidicPhospohlipidConcentration().SetValue(BoneAPL, MassPerMassUnit::mg_Per_g);
  BoneTissue.GetNeutralLipidsVolumeFraction().SetValue(BoneNLFraction);
  BoneTissue.GetNeutralPhospholipidsVolumeFraction().SetValue(BoneNPFraction);
  BoneTissue.GetTissueToPlasmaAlbuminRatio().SetValue(BoneARatio);
  BoneTissue.GetTissueToPlasmaAlphaAcidGlycoproteinRatio().SetValue(BoneAAGRatio);
  BoneTissue.GetTissueToPlasmaLipoproteinRatio().SetValue(BoneLRatio);
  BoneTissue.GetTotalMass().SetValue(BoneTissueMass, MassUnit::kg);
  BoneTissue.GetMembranePotential().SetValue(-84.8, ElectricPotentialUnit::mV);
  BoneTissue.GetReflectionCoefficient().SetValue(1.0);

  ///////////
  // Brain //
  SEFluidCircuitNode* BrainV = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::Brain1);
  SEFluidCircuitNode& BrainE1 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::BrainE1);
  SEFluidCircuitNode& BrainE2 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::BrainE2);
  SEFluidCircuitNode& BrainE3 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::BrainE3);
  SEFluidCircuitNode& BrainI = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::BrainI);
  SEFluidCircuitNode& BrainL = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::BrainL); //Pre-lymphatic node 1

  vNodePressure = BrainV->GetPressure(PressureUnit::mmHg); // VolumeModifierBrain;
  e1NodePressure = vNodePressure - copVascular_mmHg; //Plasma colloid osmotic pressure opposes flow into tissue space (i.e. favor E1 to V)
  e3NodePressure = vNodePressure - targetHydrostaticGradient_mmHg;
  e2NodePressure = e3NodePressure - copExtracell_mmHg; //Extracellular colloid osmotic pressure promotes flow from E2 to E3
  if (e3NodePressure > preLymphaticPressureMin_mmHg) {
    l1NodePressure = e3NodePressure;
  } else {
    l1NodePressure = preLymphaticPressureMin_mmHg;
  }

  filteredFlow_mL_Per_min = (BrainTissueVolume * BrainEWFraction) / totalECWater_L * lymphTotalBody_mL_Per_min;
  capillaryResistance_mmHg_min_Per_mL = (e1NodePressure - e2NodePressure) / filteredFlow_mL_Per_min;
  lymphDrivePressure_mmHg = l1NodePressure - e3NodePressure;
  lymphResistance_mmHg_min_Per_mL = (l1NodePressure - l2NodePressure) / filteredFlow_mL_Per_min;

  BrainE1.GetPressure().SetValue(e1NodePressure, PressureUnit::mmHg);
  BrainE2.GetPressure().SetValue(e2NodePressure, PressureUnit::mmHg);
  BrainE3.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg);
  BrainE3.GetVolumeBaseline().SetValue(BrainEWFraction * BrainTissueVolume * 1000.0, VolumeUnit::mL);
  BrainI.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg); //No hydrostatic pressure difference between intra/extra
  BrainI.GetVolumeBaseline().SetValue(BrainIWFraction * BrainTissueVolume * 1000.0, VolumeUnit::mL); //intracellular node
  BrainL.GetPressure().SetValue(l1NodePressure, PressureUnit::mmHg);

  SEFluidCircuitPath& BrainVToBrainE1 = cCombinedCardiovascular.CreatePath(*BrainV, BrainE1, BGE::TissueLitePath::BrainVToBrainE1);
  BrainVToBrainE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& BrainE1ToBrainE2 = cCombinedCardiovascular.CreatePath(BrainE1, BrainE2, BGE::TissueLitePath::BrainE1ToBrainE2);
  BrainE1ToBrainE2.GetResistanceBaseline().SetValue(capillaryResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  SEFluidCircuitPath& BrainE2ToBrainE3 = cCombinedCardiovascular.CreatePath(BrainE2, BrainE3, BGE::TissueLitePath::BrainE2ToBrainE3);
  BrainE2ToBrainE3.GetPressureSourceBaseline().SetValue(copExtracell_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& BrainE3ToGround = cCombinedCardiovascular.CreatePath(BrainE3, *Ground, BGE::TissueLitePath::BrainE3ToGround);
  //Minus sign in compliance baselines are because the interstitial pressure is negative with respect to atmosphere and we don't want negative compliance
  BrainE3ToGround.GetComplianceBaseline().SetValue(BrainE3.GetVolumeBaseline(VolumeUnit::mL) / vNodePressure, FlowComplianceUnit::mL_Per_mmHg); //Might need to change this
  SEFluidCircuitPath& BrainE3ToBrainI = cCombinedCardiovascular.CreatePath(BrainE3, BrainI, BGE::TissueLitePath::BrainE3ToBrainI);
  BrainE3ToBrainI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
  SEFluidCircuitPath& BrainIToGround = cCombinedCardiovascular.CreatePath(BrainI, *Ground, BGE::TissueLitePath::BrainIToGround);
  BrainIToGround.GetComplianceBaseline().SetValue(-BrainI.GetVolumeBaseline(VolumeUnit::mL) / BrainI.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& BrainE3ToBrainL = cCombinedCardiovascular.CreatePath(BrainE3, BrainL, BGE::TissueLitePath::BrainE3ToBrainL);
  BrainE3ToBrainL.GetPressureSourceBaseline().SetValue(lymphDrivePressure_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& BrainLToLymph = cCombinedCardiovascular.CreatePath(BrainL, Lymph, BGE::TissueLitePath::BrainLToLymph);
  BrainLToLymph.GetResistanceBaseline().SetValue(lymphResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  BrainLToLymph.SetNextPolarizedState(CDM::enumOpenClosed::Open);

  SETissueCompartment& BrainTissue = m_Compartments->CreateTissueCompartment(BGE::TissueLiteCompartment::Brain);
  SELiquidCompartment& BrainExtracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::BrainExtracellular);
  SELiquidCompartment& BrainIntracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::BrainIntracellular);
  BrainTissue.GetMatrixVolume().SetValue((1 - BrainEWFraction - BrainIWFraction) * BrainTissueVolume * 1000.0, VolumeUnit::mL);
  BrainExtracellular.MapNode(BrainE1);
  BrainExtracellular.MapNode(BrainE2);
  BrainExtracellular.MapNode(BrainE3);
  BrainIntracellular.MapNode(BrainI);
  BrainExtracellular.MapNode(BrainL);
  BrainExtracellular.GetWaterVolumeFraction().SetValue(BrainEWFraction);
  BrainIntracellular.GetWaterVolumeFraction().SetValue(BrainIWFraction);
  BrainTissue.GetAcidicPhospohlipidConcentration().SetValue(BrainAPL, MassPerMassUnit::mg_Per_g);
  BrainTissue.GetNeutralLipidsVolumeFraction().SetValue(BrainNLFraction);
  BrainTissue.GetNeutralPhospholipidsVolumeFraction().SetValue(BrainNPFraction);
  BrainTissue.GetTissueToPlasmaAlbuminRatio().SetValue(BrainARatio);
  BrainTissue.GetTissueToPlasmaAlphaAcidGlycoproteinRatio().SetValue(BrainAAGRatio);
  BrainTissue.GetTissueToPlasmaLipoproteinRatio().SetValue(BrainLRatio);
  BrainTissue.GetTotalMass().SetValue(BrainTissueMass, MassUnit::kg);
  BrainTissue.GetMembranePotential().SetValue(-84.8, ElectricPotentialUnit::mV);
  BrainTissue.GetReflectionCoefficient().SetValue(1.0);

  /////////////////
  //Kidney--Left/Right combined //
  SEFluidCircuitNode* KidneyV;
  if (!m_Config->IsRenalEnabled()) {
    KidneyV = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::LeftKidney1);
  } else {
    KidneyV = cCombinedCardiovascular.GetNode(BGE::RenalLiteNode::GlomerularCapillaries);
  }

  SEFluidCircuitNode& KidneyE1 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::KidneyE1);
  SEFluidCircuitNode& KidneyE2 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::KidneyE2);
  SEFluidCircuitNode& KidneyE3 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::KidneyE3);
  SEFluidCircuitNode& KidneyI = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::KidneyI);
  SEFluidCircuitNode& KidneyL = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::KidneyL);

  //Kidneys are a little bit different because there is an oncotic pressure source set against glomerular capillaries that increases
  //effective pressure on node quite a bit.  This value is derived from average hydrostatic pressure on node and glomerular oncotic pressure source.
  vNodePressure = 65.474;
  e1NodePressure = vNodePressure - copVascular_mmHg; //Plasma colloid osmotic pressure opposes flow into tissue space (i.e. favor E1 to V)
  e3NodePressure = vNodePressure - targetHydrostaticGradient_mmHg;
  e2NodePressure = e3NodePressure - copExtracell_mmHg; //Extracellular colloid osmotic pressure promotes flow from E2 to E3
  if (e3NodePressure > preLymphaticPressureMin_mmHg) {
    l1NodePressure = e3NodePressure;
  } else {
    l1NodePressure = preLymphaticPressureMin_mmHg;
  }

  filteredFlow_mL_Per_min = (KidneyTissueVolume * KidneyEWFraction) / totalECWater_L * lymphTotalBody_mL_Per_min;
  capillaryResistance_mmHg_min_Per_mL = (e1NodePressure - e2NodePressure) / filteredFlow_mL_Per_min;
  lymphDrivePressure_mmHg = l1NodePressure - e3NodePressure;
  lymphResistance_mmHg_min_Per_mL = (l1NodePressure - l2NodePressure) / filteredFlow_mL_Per_min;

  KidneyE1.GetPressure().SetValue(e1NodePressure, PressureUnit::mmHg);
  KidneyE2.GetPressure().SetValue(e2NodePressure, PressureUnit::mmHg);
  KidneyE3.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg);
  KidneyE3.GetVolumeBaseline().SetValue(KidneyEWFraction * KidneyTissueVolume * 1000.0, VolumeUnit::mL);
  KidneyI.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg); //No hydrostatic pressure difference between intra/extra
  KidneyI.GetVolumeBaseline().SetValue(KidneyIWFraction * KidneyTissueVolume * 1000.0, VolumeUnit::mL); //intracellular node
  KidneyL.GetPressure().SetValue(l1NodePressure, PressureUnit::mmHg);

  SEFluidCircuitPath& KidneyVToKidneyE1 = cCombinedCardiovascular.CreatePath(*KidneyV, KidneyE1, BGE::TissueLitePath::KidneyVToKidneyE1);
  KidneyVToKidneyE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& KidneyE1ToKidneyE2 = cCombinedCardiovascular.CreatePath(KidneyE1, KidneyE2, BGE::TissueLitePath::KidneyE1ToKidneyE2);
  KidneyE1ToKidneyE2.GetResistanceBaseline().SetValue(capillaryResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  SEFluidCircuitPath& KidneyE2ToKidneyE3 = cCombinedCardiovascular.CreatePath(KidneyE2, KidneyE3, BGE::TissueLitePath::KidneyE2ToKidneyE3);
  KidneyE2ToKidneyE3.GetPressureSourceBaseline().SetValue(copExtracell_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& KidneyE3ToGround = cCombinedCardiovascular.CreatePath(KidneyE3, *Ground, BGE::TissueLitePath::KidneyE3ToGround);
  KidneyE3ToGround.GetComplianceBaseline().SetValue(KidneyE3.GetVolumeBaseline(VolumeUnit::mL) / vNodePressure, FlowComplianceUnit::mL_Per_mmHg); //Might need to change this
  SEFluidCircuitPath& KidneyE3ToLeftKidneyI = cCombinedCardiovascular.CreatePath(KidneyE3, KidneyI, BGE::TissueLitePath::KidneyE3ToKidneyI);
  KidneyE3ToLeftKidneyI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
  SEFluidCircuitPath& KidneyIToGround = cCombinedCardiovascular.CreatePath(KidneyI, *Ground, BGE::TissueLitePath::KidneyIToGround);
  KidneyIToGround.GetComplianceBaseline().SetValue(KidneyI.GetVolumeBaseline(VolumeUnit::mL) / KidneyI.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& KidneyE3ToLeftKidneyL = cCombinedCardiovascular.CreatePath(KidneyE3, KidneyL, BGE::TissueLitePath::KidneyE3ToKidneyL);
  KidneyE3ToLeftKidneyL.GetPressureSourceBaseline().SetValue(lymphDrivePressure_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& KidneyLToLymph = cCombinedCardiovascular.CreatePath(KidneyL, Lymph, BGE::TissueLitePath::KidneyLToLymph);
  KidneyLToLymph.GetResistanceBaseline().SetValue(lymphResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  KidneyLToLymph.SetNextPolarizedState(CDM::enumOpenClosed::Open);

  SETissueCompartment& KidneyTissue = m_Compartments->CreateTissueCompartment(BGE::TissueLiteCompartment::Kidney);
  SELiquidCompartment& KidneyExtracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::KidneyExtracellular);
  SELiquidCompartment& KidneyIntracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::KidneyIntracellular);
  KidneyTissue.GetMatrixVolume().SetValue((1 - KidneyEWFraction - KidneyIWFraction) * KidneyTissueVolume * 1000.0, VolumeUnit::mL);
  KidneyExtracellular.MapNode(KidneyE1);
  KidneyExtracellular.MapNode(KidneyE2);
  KidneyExtracellular.MapNode(KidneyE3);
  KidneyIntracellular.MapNode(KidneyI);
  KidneyExtracellular.MapNode(KidneyL);
  KidneyExtracellular.GetWaterVolumeFraction().SetValue(KidneyEWFraction);
  KidneyIntracellular.GetWaterVolumeFraction().SetValue(KidneyIWFraction);
  KidneyTissue.GetAcidicPhospohlipidConcentration().SetValue(KidneyAPL, MassPerMassUnit::mg_Per_g);
  KidneyTissue.GetNeutralLipidsVolumeFraction().SetValue(KidneyNLFraction);
  KidneyTissue.GetNeutralPhospholipidsVolumeFraction().SetValue(KidneyNPFraction);
  KidneyTissue.GetTissueToPlasmaAlbuminRatio().SetValue(KidneyARatio);
  KidneyTissue.GetTissueToPlasmaAlphaAcidGlycoproteinRatio().SetValue(KidneyAAGRatio);
  KidneyTissue.GetTissueToPlasmaLipoproteinRatio().SetValue(KidneyLRatio);
  KidneyTissue.GetTotalMass().SetValue(KidneyTissueMass, MassUnit::kg);
  KidneyTissue.GetMembranePotential().SetValue(-84.8, ElectricPotentialUnit::mV);
  KidneyTissue.GetReflectionCoefficient().SetValue(1.0);

  ///////////
  // Liver //
  SEFluidCircuitNode* LiverV = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::Liver1);
  SEFluidCircuitNode& LiverE1 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::LiverE1);
  SEFluidCircuitNode& LiverE2 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::LiverE2);
  SEFluidCircuitNode& LiverE3 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::LiverE3);
  SEFluidCircuitNode& LiverI = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::LiverI);
  SEFluidCircuitNode& LiverL = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::LiverL); //Pre-lymphatic node

  vNodePressure = LiverV->GetPressure(PressureUnit::mmHg) / VolumeModifierLiver;
  e1NodePressure = vNodePressure - copVascular_mmHg; //Plasma colloid osmotic pressure opposes flow into tissue space (i.e. favor E1 to V)
  e3NodePressure = vNodePressure - targetHydrostaticGradient_mmHg;
  e2NodePressure = e3NodePressure - copExtracell_mmHg; //Extracellular colloid osmotic pressure promotes flow from E2 to E3
  if (e3NodePressure > preLymphaticPressureMin_mmHg) {
    l1NodePressure = e3NodePressure;
  } else {
    l1NodePressure = preLymphaticPressureMin_mmHg;
  }

  filteredFlow_mL_Per_min = (LiverTissueVolume * LiverEWFraction) / totalECWater_L * lymphTotalBody_mL_Per_min;
  capillaryResistance_mmHg_min_Per_mL = (e1NodePressure - e2NodePressure) / filteredFlow_mL_Per_min;
  lymphDrivePressure_mmHg = l1NodePressure - e3NodePressure;
  lymphResistance_mmHg_min_Per_mL = (l1NodePressure - l2NodePressure) / filteredFlow_mL_Per_min;

  LiverE1.GetPressure().SetValue(e1NodePressure, PressureUnit::mmHg);
  LiverE2.GetPressure().SetValue(e2NodePressure, PressureUnit::mmHg);
  LiverE3.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg);
  LiverE3.GetVolumeBaseline().SetValue(LiverEWFraction * LiverTissueVolume * 1000.0, VolumeUnit::mL);
  LiverI.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg); //No hydrostatic pressure difference between intra/extra
  LiverI.GetVolumeBaseline().SetValue(LiverIWFraction * LiverTissueVolume * 1000.0, VolumeUnit::mL); //intracellular node
  LiverL.GetPressure().SetValue(l1NodePressure, PressureUnit::mmHg);

  SEFluidCircuitPath& LiverVToLiverE1 = cCombinedCardiovascular.CreatePath(*LiverV, LiverE1, BGE::TissueLitePath::LiverVToLiverE1);
  LiverVToLiverE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& LiverE1ToLiverE2 = cCombinedCardiovascular.CreatePath(LiverE1, LiverE2, BGE::TissueLitePath::LiverE1ToLiverE2);
  LiverE1ToLiverE2.GetResistanceBaseline().SetValue(capillaryResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  SEFluidCircuitPath& LiverE2ToLiverE3 = cCombinedCardiovascular.CreatePath(LiverE2, LiverE3, BGE::TissueLitePath::LiverE2ToLiverE3);
  LiverE2ToLiverE3.GetPressureSourceBaseline().SetValue(copExtracell_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& LiverE3ToGround = cCombinedCardiovascular.CreatePath(LiverE3, *Ground, BGE::TissueLitePath::LiverE3ToGround);
  LiverE3ToGround.GetComplianceBaseline().SetValue(LiverE3.GetVolumeBaseline(VolumeUnit::mL) / vNodePressure, FlowComplianceUnit::mL_Per_mmHg); //Might need to change this
  SEFluidCircuitPath& LiverE3ToLiverI = cCombinedCardiovascular.CreatePath(LiverE3, LiverI, BGE::TissueLitePath::LiverE3ToLiverI);
  LiverE3ToLiverI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
  SEFluidCircuitPath& LiverIToGround = cCombinedCardiovascular.CreatePath(LiverI, *Ground, BGE::TissueLitePath::LiverIToGround);
  LiverIToGround.GetComplianceBaseline().SetValue(LiverI.GetVolumeBaseline(VolumeUnit::mL) / LiverI.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& LiverE3ToLiverL = cCombinedCardiovascular.CreatePath(LiverE3, LiverL, BGE::TissueLitePath::LiverE3ToLiverL);
  LiverE3ToLiverL.GetPressureSourceBaseline().SetValue(lymphDrivePressure_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& LiverLToLymph = cCombinedCardiovascular.CreatePath(LiverL, Lymph, BGE::TissueLitePath::LiverLToLymph);
  LiverLToLymph.GetResistanceBaseline().SetValue(lymphResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  LiverLToLymph.SetNextPolarizedState(CDM::enumOpenClosed::Open);

  SETissueCompartment& LiverTissue = m_Compartments->CreateTissueCompartment(BGE::TissueLiteCompartment::Liver);
  SELiquidCompartment& LiverExtracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::LiverExtracellular);
  SELiquidCompartment& LiverIntracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::LiverIntracellular);
  LiverTissue.GetMatrixVolume().SetValue((1 - LiverEWFraction - LiverIWFraction) * LiverTissueVolume * 1000.0, VolumeUnit::mL);
  LiverExtracellular.MapNode(LiverE1);
  LiverExtracellular.MapNode(LiverE2);
  LiverExtracellular.MapNode(LiverE3);
  LiverIntracellular.MapNode(LiverI);
  LiverExtracellular.MapNode(LiverL);
  LiverExtracellular.GetWaterVolumeFraction().SetValue(LiverEWFraction);
  LiverIntracellular.GetWaterVolumeFraction().SetValue(LiverIWFraction);
  LiverTissue.GetAcidicPhospohlipidConcentration().SetValue(LiverAPL, MassPerMassUnit::mg_Per_g);
  LiverTissue.GetNeutralLipidsVolumeFraction().SetValue(LiverNLFraction);
  LiverTissue.GetNeutralPhospholipidsVolumeFraction().SetValue(LiverNPFraction);
  LiverTissue.GetTissueToPlasmaAlbuminRatio().SetValue(LiverARatio);
  LiverTissue.GetTissueToPlasmaAlphaAcidGlycoproteinRatio().SetValue(LiverAAGRatio);
  LiverTissue.GetTissueToPlasmaLipoproteinRatio().SetValue(LiverLRatio);
  LiverTissue.GetTotalMass().SetValue(LiverTissueMass, MassUnit::kg);
  LiverTissue.GetMembranePotential().SetValue(-84.8, ElectricPotentialUnit::mV);
  LiverTissue.GetReflectionCoefficient().SetValue(1.0);

  ///////////////
  // Lung-Left/Right combined //
  SEFluidCircuitNode* LeftLungV = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::LeftPulmonaryCapillaries);
  SEFluidCircuitNode* RightLungV = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::RightPulmonaryCapillaries);
  SEFluidCircuitNode& LungE1 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::LungE1);
  SEFluidCircuitNode& LungE2 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::LungE2);
  SEFluidCircuitNode& LungE3 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::LungE3);
  SEFluidCircuitNode& LungI = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::LungI);
  SEFluidCircuitNode& LungL = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::LungL); //Pre-lymphatic node

  //vNodePressure = LungV->GetPressure(PressureUnit::mmHg);
  //Using empirical value from previous iteration of tissue circuit because the extracellular lung volume was increasing too much
  //When we revisit oncotic pressure calculations, this might be attributate to different concentrations of albumin in lungs than rest of body
  vNodePressure = 9.339;
  e1NodePressure = vNodePressure - copVascular_mmHg; //Plasma colloid osmotic pressure opposes flow into tissue space (i.e. favor E1 to V)
  e3NodePressure = vNodePressure - targetHydrostaticGradient_mmHg;
  e2NodePressure = e3NodePressure - copExtracell_mmHg; //Extracellular colloid osmotic pressure promotes flow from E2 to E3
  if (e3NodePressure > preLymphaticPressureMin_mmHg) {
    l1NodePressure = e3NodePressure;
  } else {
    l1NodePressure = preLymphaticPressureMin_mmHg;
  }

  filteredFlow_mL_Per_min = (LungTissueVolume * LungEWFraction) / totalECWater_L * lymphTotalBody_mL_Per_min;
  capillaryResistance_mmHg_min_Per_mL = (e1NodePressure - e2NodePressure) / filteredFlow_mL_Per_min;
  lymphDrivePressure_mmHg = l1NodePressure - e3NodePressure;
  lymphResistance_mmHg_min_Per_mL = (l1NodePressure - l2NodePressure) / filteredFlow_mL_Per_min;

  LungE1.GetPressure().SetValue(e1NodePressure, PressureUnit::mmHg);
  LungE2.GetPressure().SetValue(e2NodePressure, PressureUnit::mmHg);
  LungE3.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg);
  LungE3.GetVolumeBaseline().SetValue(LungEWFraction * LungTissueVolume * 1000.0, VolumeUnit::mL);
  LungI.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg); //No hydrostatic pressure difference between intra/extra
  LungI.GetVolumeBaseline().SetValue(LungIWFraction * LungTissueVolume * 1000.0, VolumeUnit::mL); //intracellular node
  LungL.GetPressure().SetValue(l1NodePressure, PressureUnit::mmHg);

  SEFluidCircuitPath& LeftLungVToLungE1 = cCombinedCardiovascular.CreatePath(*LeftLungV, LungE1, BGE::TissueLitePath::LeftLungVToLungE1);
  LeftLungVToLungE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& RightLungVToLungE1 = cCombinedCardiovascular.CreatePath(*RightLungV, LungE1, BGE::TissueLitePath::RightLungVToLungE1);
  RightLungVToLungE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& LungE1ToLungE2 = cCombinedCardiovascular.CreatePath(LungE1, LungE2, BGE::TissueLitePath::LungE1ToLungE2);
  LungE1ToLungE2.GetResistanceBaseline().SetValue(capillaryResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  SEFluidCircuitPath& LungE2ToLungE3 = cCombinedCardiovascular.CreatePath(LungE2, LungE3, BGE::TissueLitePath::LungE2ToLungE3);
  LungE2ToLungE3.GetPressureSourceBaseline().SetValue(copExtracell_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& LungE3ToGround = cCombinedCardiovascular.CreatePath(LungE3, *Ground, BGE::TissueLitePath::LungE3ToGround);
  //Lung interstitial compliance based of value in Miserocchi1993Pulmonary--0.00544
  LungE3ToGround.GetComplianceBaseline().SetValue(0.001 * LungE3.GetVolumeBaseline(VolumeUnit::mL), FlowComplianceUnit::mL_Per_mmHg); //Might need to change this
  SEFluidCircuitPath& LungE3ToLungI = cCombinedCardiovascular.CreatePath(LungE3, LungI, BGE::TissueLitePath::LungE3ToLungI);
  LungE3ToLungI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
  SEFluidCircuitPath& LungIToGround = cCombinedCardiovascular.CreatePath(LungI, *Ground, BGE::TissueLitePath::LungIToGround);
  LungIToGround.GetComplianceBaseline().SetValue(-LungI.GetVolumeBaseline(VolumeUnit::mL) / LungI.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& LungE3ToLungL = cCombinedCardiovascular.CreatePath(LungE3, LungL, BGE::TissueLitePath::LungE3ToLungL);
  LungE3ToLungL.GetPressureSourceBaseline().SetValue(lymphDrivePressure_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& LungLToLymph = cCombinedCardiovascular.CreatePath(LungL, Lymph, BGE::TissueLitePath::LungLToLymph);
  LungLToLymph.GetResistanceBaseline().SetValue(lymphResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  LungLToLymph.SetNextPolarizedState(CDM::enumOpenClosed::Open);

  SETissueCompartment& LungTissue = m_Compartments->CreateTissueCompartment(BGE::TissueLiteCompartment::Lung);
  SELiquidCompartment& LungExtracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::LungExtracellular);
  SELiquidCompartment& LungIntracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::LungIntracellular);
  LungTissue.GetMatrixVolume().SetValue((1 - LungEWFraction - LungIWFraction) * LungTissueVolume * 1000.0, VolumeUnit::mL);
  LungExtracellular.MapNode(LungE1);
  LungExtracellular.MapNode(LungE2);
  LungExtracellular.MapNode(LungE3);
  LungIntracellular.MapNode(LungI);
  LungExtracellular.MapNode(LungL);
  LungExtracellular.GetWaterVolumeFraction().SetValue(LungEWFraction);
  LungIntracellular.GetWaterVolumeFraction().SetValue(LungIWFraction);
  LungTissue.GetAcidicPhospohlipidConcentration().SetValue(LungAPL, MassPerMassUnit::mg_Per_g);
  LungTissue.GetNeutralLipidsVolumeFraction().SetValue(LungNLFraction);
  LungTissue.GetNeutralPhospholipidsVolumeFraction().SetValue(LungNPFraction);
  LungTissue.GetTissueToPlasmaAlbuminRatio().SetValue(LungARatio);
  LungTissue.GetTissueToPlasmaAlphaAcidGlycoproteinRatio().SetValue(LungAAGRatio);
  LungTissue.GetTissueToPlasmaLipoproteinRatio().SetValue(LungLRatio);
  LungTissue.GetTotalMass().SetValue(LungTissueMass, MassUnit::kg);
  LungTissue.GetMembranePotential().SetValue(-84.8, ElectricPotentialUnit::mV);
  LungTissue.GetReflectionCoefficient().SetValue(1.0);

  ////////////
  // Muscle //
  SEFluidCircuitNode* MuscleV = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::Muscle1);
  SEFluidCircuitNode& MuscleE1 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::MuscleE1);
  SEFluidCircuitNode& MuscleE2 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::MuscleE2);
  SEFluidCircuitNode& MuscleE3 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::MuscleE3);
  SEFluidCircuitNode& MuscleI = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::MuscleI);
  SEFluidCircuitNode& MuscleL = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::MuscleL); //Pre-lymphatic node

  vNodePressure = MuscleV->GetPressure(PressureUnit::mmHg) / VolumeModifierMuscle;
  e1NodePressure = vNodePressure - copVascular_mmHg; //Plasma colloid osmotic pressure opposes flow into tissue space (i.e. favor E1 to V)
  e3NodePressure = vNodePressure - targetHydrostaticGradient_mmHg;
  e2NodePressure = e3NodePressure - copExtracell_mmHg; //Extracellular colloid osmotic pressure promotes flow from E2 to E3
  if (e3NodePressure > preLymphaticPressureMin_mmHg) {
    l1NodePressure = e3NodePressure;
  } else {
    l1NodePressure = preLymphaticPressureMin_mmHg;
  }

  filteredFlow_mL_Per_min = (MuscleTissueVolume * MuscleEWFraction) / totalECWater_L * lymphTotalBody_mL_Per_min;
  capillaryResistance_mmHg_min_Per_mL = (e1NodePressure - e2NodePressure) / filteredFlow_mL_Per_min;
  lymphDrivePressure_mmHg = l1NodePressure - e3NodePressure;
  lymphResistance_mmHg_min_Per_mL = (l1NodePressure - l2NodePressure) / filteredFlow_mL_Per_min;

  MuscleE1.GetPressure().SetValue(e1NodePressure, PressureUnit::mmHg);
  MuscleE2.GetPressure().SetValue(e2NodePressure, PressureUnit::mmHg);
  MuscleE3.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg);
  MuscleE3.GetVolumeBaseline().SetValue(MuscleEWFraction * MuscleTissueVolume * 1000.0, VolumeUnit::mL);
  MuscleI.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg); //No hydrostatic pressure difference between intra/extra
  MuscleI.GetVolumeBaseline().SetValue(MuscleIWFraction * MuscleTissueVolume * 1000.0, VolumeUnit::mL); //intracellular node
  MuscleL.GetPressure().SetValue(l1NodePressure, PressureUnit::mmHg);

  SEFluidCircuitPath& MuscleVToMuscleE1 = cCombinedCardiovascular.CreatePath(*MuscleV, MuscleE1, BGE::TissueLitePath::MuscleVToMuscleE1);
  MuscleVToMuscleE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& MuscleE1ToMuscleE2 = cCombinedCardiovascular.CreatePath(MuscleE1, MuscleE2, BGE::TissueLitePath::MuscleE1ToMuscleE2);
  MuscleE1ToMuscleE2.GetResistanceBaseline().SetValue(capillaryResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  SEFluidCircuitPath& MuscleE2ToMuscleE3 = cCombinedCardiovascular.CreatePath(MuscleE2, MuscleE3, BGE::TissueLitePath::MuscleE2ToMuscleE3);
  MuscleE2ToMuscleE3.GetPressureSourceBaseline().SetValue(copExtracell_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& MuscleE3ToGround = cCombinedCardiovascular.CreatePath(MuscleE3, *Ground, BGE::TissueLitePath::MuscleE3ToGround);
  MuscleE3ToGround.GetComplianceBaseline().SetValue(200.0, FlowComplianceUnit::mL_Per_mmHg); //From Reisner2012Computational
  SEFluidCircuitPath& MuscleE3ToMuscleI = cCombinedCardiovascular.CreatePath(MuscleE3, MuscleI, BGE::TissueLitePath::MuscleE3ToMuscleI);
  MuscleE3ToMuscleI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
  SEFluidCircuitPath& MuscleIToGround = cCombinedCardiovascular.CreatePath(MuscleI, *Ground, BGE::TissueLitePath::MuscleIToGround);
  MuscleIToGround.GetComplianceBaseline().SetValue(MuscleI.GetVolumeBaseline(VolumeUnit::mL) / MuscleI.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& MuscleE3ToMuscleL1 = cCombinedCardiovascular.CreatePath(MuscleE3, MuscleL, BGE::TissueLitePath::MuscleE3ToMuscleL);
  MuscleE3ToMuscleL1.GetPressureSourceBaseline().SetValue(lymphDrivePressure_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& MuscleLToLymph = cCombinedCardiovascular.CreatePath(MuscleL, Lymph, BGE::TissueLitePath::MuscleLToLymph);
  MuscleLToLymph.GetResistanceBaseline().SetValue(lymphResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  MuscleLToLymph.SetNextPolarizedState(CDM::enumOpenClosed::Open);

  SETissueCompartment& MuscleTissue = m_Compartments->CreateTissueCompartment(BGE::TissueLiteCompartment::Muscle);
  SELiquidCompartment& MuscleExtracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::MuscleExtracellular);
  SELiquidCompartment& MuscleIntracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::MuscleIntracellular);
  MuscleTissue.GetMatrixVolume().SetValue((1 - MuscleEWFraction - MuscleIWFraction) * MuscleTissueVolume * 1000.0, VolumeUnit::mL);
  MuscleExtracellular.MapNode(MuscleE1);
  MuscleExtracellular.MapNode(MuscleE2);
  MuscleExtracellular.MapNode(MuscleE3);
  MuscleIntracellular.MapNode(MuscleI);
  MuscleExtracellular.MapNode(MuscleL);
  MuscleExtracellular.GetWaterVolumeFraction().SetValue(MuscleEWFraction);
  MuscleIntracellular.GetWaterVolumeFraction().SetValue(MuscleIWFraction);
  MuscleTissue.GetAcidicPhospohlipidConcentration().SetValue(MuscleAPL, MassPerMassUnit::mg_Per_g);
  MuscleTissue.GetNeutralLipidsVolumeFraction().SetValue(MuscleNLFraction);
  MuscleTissue.GetNeutralPhospholipidsVolumeFraction().SetValue(MuscleNPFraction);
  MuscleTissue.GetTissueToPlasmaAlbuminRatio().SetValue(MuscleARatio);
  MuscleTissue.GetTissueToPlasmaAlphaAcidGlycoproteinRatio().SetValue(MuscleAAGRatio);
  MuscleTissue.GetTissueToPlasmaLipoproteinRatio().SetValue(MuscleLRatio);
  MuscleTissue.GetTotalMass().SetValue(MuscleTissueMass, MassUnit::kg);
  MuscleTissue.GetMembranePotential().SetValue(-84.8, ElectricPotentialUnit::mV);
  MuscleTissue.GetReflectionCoefficient().SetValue(1.0);

  ////////////////
  // Myocardium //
  SEFluidCircuitNode* MyocardiumV = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::Myocardium1);
  SEFluidCircuitNode& MyocardiumE1 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::MyocardiumE1);
  SEFluidCircuitNode& MyocardiumE2 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::MyocardiumE2);
  SEFluidCircuitNode& MyocardiumE3 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::MyocardiumE3);
  SEFluidCircuitNode& MyocardiumI = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::MyocardiumI);
  SEFluidCircuitNode& MyocardiumL = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::MyocardiumL); //Pre-lymphatic node

  vNodePressure = MyocardiumV->GetPressure(PressureUnit::mmHg) / VolumeModifierMyocardium;
  e1NodePressure = vNodePressure - copVascular_mmHg; //Plasma colloid osmotic pressure opposes flow into tissue space (i.e. favor E1 to V)
  e3NodePressure = vNodePressure - targetHydrostaticGradient_mmHg;
  e2NodePressure = e3NodePressure - copExtracell_mmHg; //Extracellular colloid osmotic pressure promotes flow from E2 to E3
  if (e3NodePressure > preLymphaticPressureMin_mmHg) {
    l1NodePressure = e3NodePressure;
  } else {
    l1NodePressure = preLymphaticPressureMin_mmHg;
  }

  filteredFlow_mL_Per_min = (MyocardiumTissueVolume * MyocardiumEWFraction) / totalECWater_L * lymphTotalBody_mL_Per_min;
  capillaryResistance_mmHg_min_Per_mL = (e1NodePressure - e2NodePressure) / filteredFlow_mL_Per_min;
  lymphDrivePressure_mmHg = l1NodePressure - e3NodePressure;
  lymphResistance_mmHg_min_Per_mL = (l1NodePressure - l2NodePressure) / filteredFlow_mL_Per_min;

  MyocardiumE1.GetPressure().SetValue(e1NodePressure, PressureUnit::mmHg);
  MyocardiumE2.GetPressure().SetValue(e2NodePressure, PressureUnit::mmHg);
  MyocardiumE3.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg);
  MyocardiumE3.GetVolumeBaseline().SetValue(MyocardiumEWFraction * MyocardiumTissueVolume * 1000.0, VolumeUnit::mL);
  MyocardiumI.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg); //No hydrostatic pressure difference between intra/extra
  MyocardiumI.GetVolumeBaseline().SetValue(MyocardiumIWFraction * MyocardiumTissueVolume * 1000.0, VolumeUnit::mL); //intracellular node
  MyocardiumL.GetPressure().SetValue(l1NodePressure, PressureUnit::mmHg);

  SEFluidCircuitPath& MyocardiumVToMyocardiumE1 = cCombinedCardiovascular.CreatePath(*MyocardiumV, MyocardiumE1, BGE::TissueLitePath::MyocardiumVToMyocardiumE1);
  MyocardiumVToMyocardiumE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& MyocardiumE1ToMyocardiumE2 = cCombinedCardiovascular.CreatePath(MyocardiumE1, MyocardiumE2, BGE::TissueLitePath::MyocardiumE1ToMyocardiumE2);
  MyocardiumE1ToMyocardiumE2.GetResistanceBaseline().SetValue(capillaryResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  SEFluidCircuitPath& MyocardiumE2ToMyocardiumE3 = cCombinedCardiovascular.CreatePath(MyocardiumE2, MyocardiumE3, BGE::TissueLitePath::MyocardiumE2ToMyocardiumE3);
  MyocardiumE2ToMyocardiumE3.GetPressureSourceBaseline().SetValue(copExtracell_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& MyocardiumE3ToGround = cCombinedCardiovascular.CreatePath(MyocardiumE3, *Ground, BGE::TissueLitePath::MyocardiumE3ToGround);
  MyocardiumE3ToGround.GetComplianceBaseline().SetValue(MyocardiumE3.GetVolumeBaseline(VolumeUnit::mL) / vNodePressure, FlowComplianceUnit::mL_Per_mmHg); //Might need to change this
  SEFluidCircuitPath& MyocardiumE3ToMyocardiumI = cCombinedCardiovascular.CreatePath(MyocardiumE3, MyocardiumI, BGE::TissueLitePath::MyocardiumE3ToMyocardiumI);
  MyocardiumE3ToMyocardiumI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
  SEFluidCircuitPath& MyocardiumIToGround = cCombinedCardiovascular.CreatePath(MyocardiumI, *Ground, BGE::TissueLitePath::MyocardiumIToGround);
  MyocardiumIToGround.GetComplianceBaseline().SetValue(MyocardiumI.GetVolumeBaseline(VolumeUnit::mL) / MyocardiumI.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& MyocardiumE3ToMyocardiumL = cCombinedCardiovascular.CreatePath(MyocardiumE3, MyocardiumL, BGE::TissueLitePath::MyocardiumE3ToMyocardiumL);
  MyocardiumE3ToMyocardiumL.GetPressureSourceBaseline().SetValue(lymphDrivePressure_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& MyocardiumLToLymph = cCombinedCardiovascular.CreatePath(MyocardiumL, Lymph, BGE::TissueLitePath::MyocardiumLToLymph);
  MyocardiumLToLymph.GetResistanceBaseline().SetValue(lymphResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  MyocardiumLToLymph.SetNextPolarizedState(CDM::enumOpenClosed::Open);

  SETissueCompartment& MyocardiumTissue = m_Compartments->CreateTissueCompartment(BGE::TissueLiteCompartment::Myocardium);
  SELiquidCompartment& MyocardiumExtracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::MyocardiumExtracellular);
  SELiquidCompartment& MyocardiumIntracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::MyocardiumIntracellular);
  MyocardiumTissue.GetMatrixVolume().SetValue((1 - MyocardiumEWFraction - MyocardiumIWFraction) * MyocardiumTissueVolume * 1000.0, VolumeUnit::mL);
  MyocardiumExtracellular.MapNode(MyocardiumE1);
  MyocardiumExtracellular.MapNode(MyocardiumE2);
  MyocardiumExtracellular.MapNode(MyocardiumE3);
  MyocardiumIntracellular.MapNode(MyocardiumI);
  MyocardiumExtracellular.MapNode(MyocardiumL);
  MyocardiumExtracellular.GetWaterVolumeFraction().SetValue(MyocardiumEWFraction);
  MyocardiumIntracellular.GetWaterVolumeFraction().SetValue(MyocardiumIWFraction);
  MyocardiumTissue.GetAcidicPhospohlipidConcentration().SetValue(MyocardiumAPL, MassPerMassUnit::mg_Per_g);
  MyocardiumTissue.GetNeutralLipidsVolumeFraction().SetValue(MyocardiumNLFraction);
  MyocardiumTissue.GetNeutralPhospholipidsVolumeFraction().SetValue(MyocardiumNPFraction);
  MyocardiumTissue.GetTissueToPlasmaAlbuminRatio().SetValue(MyocardiumARatio);
  MyocardiumTissue.GetTissueToPlasmaAlphaAcidGlycoproteinRatio().SetValue(MyocardiumAAGRatio);
  MyocardiumTissue.GetTissueToPlasmaLipoproteinRatio().SetValue(MyocardiumLRatio);
  MyocardiumTissue.GetTotalMass().SetValue(MyocardiumTissueMass, MassUnit::kg);
  MyocardiumTissue.GetMembranePotential().SetValue(-84.8, ElectricPotentialUnit::mV);
  MyocardiumTissue.GetReflectionCoefficient().SetValue(1.0);

  //////////
  // Skin //
  SEFluidCircuitNode* SkinV = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::Skin1);
  SEFluidCircuitNode& SkinE1 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::SkinE1);
  SEFluidCircuitNode& SkinE2 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::SkinE2);
  SEFluidCircuitNode& SkinE3 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::SkinE3);
  SEFluidCircuitNode& SkinI = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::SkinI);
  SEFluidCircuitNode& SkinL = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::SkinL); //Pre-lymphatic node

  vNodePressure = SkinV->GetPressure(PressureUnit::mmHg) / VolumeModifierSkin;
  e1NodePressure = vNodePressure - copVascular_mmHg; //Plasma colloid osmotic pressure opposes flow into tissue space (i.e. favor E1 to V)
  e3NodePressure = vNodePressure - targetHydrostaticGradient_mmHg;
  e2NodePressure = e3NodePressure - copExtracell_mmHg; //Extracellular colloid osmotic pressure promotes flow from E2 to E3
  if (e3NodePressure > preLymphaticPressureMin_mmHg) {
    l1NodePressure = e3NodePressure;
  } else {
    l1NodePressure = preLymphaticPressureMin_mmHg;
  }

  filteredFlow_mL_Per_min = (SkinTissueVolume * SkinEWFraction) / totalECWater_L * lymphTotalBody_mL_Per_min;
  capillaryResistance_mmHg_min_Per_mL = (e1NodePressure - e2NodePressure) / filteredFlow_mL_Per_min;
  lymphDrivePressure_mmHg = l1NodePressure - e3NodePressure;
  lymphResistance_mmHg_min_Per_mL = (l1NodePressure - l2NodePressure) / filteredFlow_mL_Per_min;

  SkinE1.GetPressure().SetValue(e1NodePressure, PressureUnit::mmHg);
  SkinE2.GetPressure().SetValue(e2NodePressure, PressureUnit::mmHg);
  SkinE3.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg);
  SkinE3.GetVolumeBaseline().SetValue(SkinEWFraction * SkinTissueVolume * 1000.0, VolumeUnit::mL);
  SkinI.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg); //No hydrostatic pressure difference between intra/extra
  SkinI.GetVolumeBaseline().SetValue(SkinIWFraction * SkinTissueVolume * 1000.0, VolumeUnit::mL); //intracellular node
  SkinL.GetPressure().SetValue(l1NodePressure, PressureUnit::mmHg);

  SEFluidCircuitPath& SkinVToSkinE1 = cCombinedCardiovascular.CreatePath(*SkinV, SkinE1, BGE::TissueLitePath::SkinVToSkinE1);
  SkinVToSkinE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& SkinE1ToSkinE2 = cCombinedCardiovascular.CreatePath(SkinE1, SkinE2, BGE::TissueLitePath::SkinE1ToSkinE2);
  SkinE1ToSkinE2.GetResistanceBaseline().SetValue(capillaryResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  SEFluidCircuitPath& SkinE2ToSkinE3 = cCombinedCardiovascular.CreatePath(SkinE2, SkinE3, BGE::TissueLitePath::SkinE2ToSkinE3);
  SkinE2ToSkinE3.GetPressureSourceBaseline().SetValue(copExtracell_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& SkinE3ToGround = cCombinedCardiovascular.CreatePath(SkinE3, *Ground, BGE::TissueLitePath::SkinE3ToGround);
  //Minus sign in compliance baselines are because the interstitial pressure is negative with respect to atmosphere and we don't want negative compliance
  SkinE3ToGround.GetComplianceBaseline().SetValue(SkinE3.GetVolumeBaseline(VolumeUnit::mL) / vNodePressure, FlowComplianceUnit::mL_Per_mmHg); //Might need to change this
  SEFluidCircuitPath& SkinE3ToSkinI = cCombinedCardiovascular.CreatePath(SkinE3, SkinI, BGE::TissueLitePath::SkinE3ToSkinI);
  SkinE3ToSkinI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
  SEFluidCircuitPath& SkinIToGround = cCombinedCardiovascular.CreatePath(SkinI, *Ground, BGE::TissueLitePath::SkinIToGround);
  SkinIToGround.GetComplianceBaseline().SetValue(-SkinI.GetVolumeBaseline(VolumeUnit::mL) / SkinI.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);

  SEFluidCircuitPath& SkinE3ToSkinL = cCombinedCardiovascular.CreatePath(SkinE3, SkinL, BGE::TissueLitePath::SkinE3ToSkinL);
  SkinE3ToSkinL.GetPressureSourceBaseline().SetValue(lymphDrivePressure_mmHg, PressureUnit::mmHg);
  SEFluidCircuitPath& SkinLToLymph = cCombinedCardiovascular.CreatePath(SkinL, Lymph, BGE::TissueLitePath::SkinLToLymph);
  SkinLToLymph.GetResistanceBaseline().SetValue(lymphResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
  SkinLToLymph.SetNextPolarizedState(CDM::enumOpenClosed::Open);

  SEFluidCircuitPath& SkinSweatLossToGround = cCombinedCardiovascular.CreatePath(SkinE3, *Ground, BGE::TissueLitePath::SkinSweating);
  SkinSweatLossToGround.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);

  SETissueCompartment& SkinTissue = m_Compartments->CreateTissueCompartment(BGE::TissueLiteCompartment::Skin);
  SELiquidCompartment& SkinExtracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::SkinExtracellular);
  SELiquidCompartment& SkinIntracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::SkinIntracellular);
  SkinTissue.GetMatrixVolume().SetValue((1 - SkinEWFraction - SkinIWFraction) * SkinTissueVolume * 1000.0, VolumeUnit::mL);
  SkinExtracellular.MapNode(SkinE1);
  SkinExtracellular.MapNode(SkinE2);
  SkinExtracellular.MapNode(SkinE3);
  SkinIntracellular.MapNode(SkinI);
  SkinExtracellular.MapNode(SkinL);
  SkinExtracellular.GetWaterVolumeFraction().SetValue(SkinEWFraction);
  SkinIntracellular.GetWaterVolumeFraction().SetValue(SkinIWFraction);
  SkinTissue.GetAcidicPhospohlipidConcentration().SetValue(SkinAPL, MassPerMassUnit::mg_Per_g);
  SkinTissue.GetNeutralLipidsVolumeFraction().SetValue(SkinNLFraction);
  SkinTissue.GetNeutralPhospholipidsVolumeFraction().SetValue(SkinNPFraction);
  SkinTissue.GetTissueToPlasmaAlbuminRatio().SetValue(SkinARatio);
  SkinTissue.GetTissueToPlasmaAlphaAcidGlycoproteinRatio().SetValue(SkinAAGRatio);
  SkinTissue.GetTissueToPlasmaLipoproteinRatio().SetValue(SkinLRatio);
  SkinTissue.GetTotalMass().SetValue(SkinTissueMass, MassUnit::kg);
  SkinTissue.GetMembranePotential().SetValue(-84.8, ElectricPotentialUnit::mV);
  SkinTissue.GetReflectionCoefficient().SetValue(1.0);

  //Cardiovascular lite needs to have small intestine, large intestine, spleen, splanchnic combined to GutV
  /////////
  // Gut //
    SEFluidCircuitNode* GutV = cCardiovascular.GetNode(BGE::CardiovascularLiteNode::Gut1);
    SEFluidCircuitNode& GutE1 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::GutE1);
    SEFluidCircuitNode& GutE2 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::GutE2);
    SEFluidCircuitNode& GutE3 = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::GutE3);
    SEFluidCircuitNode& GutI = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::GutI);
    SEFluidCircuitNode& GutL = cCombinedCardiovascular.CreateNode(BGE::TissueLiteNode::GutL); //Pre-lymphatic node

    //Splanchnic takes from both intestines, "splanchnic" vascular (pancreas), spleen, liver, and myocardium.
    //All of these compartments except liver have the same target vascular pressure.  We will need to account for
    //the lower target pressure in the liver when constructing vascular COP paths to avoid backflow into liver vascular.
    //For vNodePressure, we can take any of the values (except liver), so grab pancreas ("splanchnic vascular").

    vNodePressure = GutV->GetPressure(PressureUnit::mmHg) / VolumeModifierGutLite;
    e1NodePressure = vNodePressure - copVascular_mmHg; //Plasma colloid osmotic pressure opposes flow into tissue space (i.e. favor E1 to V)
    e3NodePressure = vNodePressure - targetHydrostaticGradient_mmHg;
    e2NodePressure = e3NodePressure - copExtracell_mmHg; //Extracellular colloid osmotic pressure promotes flow from E2 to E3
    if (e3NodePressure > preLymphaticPressureMin_mmHg) {
      l1NodePressure = e3NodePressure;
    } else {
      l1NodePressure = preLymphaticPressureMin_mmHg;
    }
    l2NodePressure = Lymph.GetPressure(PressureUnit::mmHg);

    filteredFlow_mL_Per_min = (GutLiteTissueVolume * GutLiteEWFraction) / totalECWater_L * lymphTotalBody_mL_Per_min;
    capillaryResistance_mmHg_min_Per_mL = (e1NodePressure - e2NodePressure) / filteredFlow_mL_Per_min;
    lymphDrivePressure_mmHg = l1NodePressure - e3NodePressure;
    lymphResistance_mmHg_min_Per_mL = (l1NodePressure - l2NodePressure) / filteredFlow_mL_Per_min;

    GutE1.GetPressure().SetValue(e1NodePressure, PressureUnit::mmHg);
    GutE2.GetPressure().SetValue(e2NodePressure, PressureUnit::mmHg);
    GutE3.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg);
    GutE3.GetVolumeBaseline().SetValue(GutLiteEWFraction * GutLiteTissueVolume * 1000.0, VolumeUnit::mL);
    GutI.GetPressure().SetValue(e3NodePressure, PressureUnit::mmHg); //No hydrostatic pressure difference between intra/extra
    GutI.GetVolumeBaseline().SetValue(GutLiteIWFraction * GutLiteTissueVolume * 1000.0, VolumeUnit::mL); //intracellular node
    GutL.GetPressure().SetValue(l1NodePressure, PressureUnit::mmHg);

    SEFluidCircuitPath& GutVToGutE1 = cCombinedCardiovascular.CreatePath(*GutV, GutE1, BGE::TissueLitePath::GutVToGutE1);
    GutVToGutE1.GetPressureSourceBaseline().SetValue(-copVascular_mmHg, PressureUnit::mmHg);
    SEFluidCircuitPath& GutE1ToGutE2 = cCombinedCardiovascular.CreatePath(GutE1, GutE2, BGE::TissueLitePath::GutE1ToGutE2);
    GutE1ToGutE2.GetResistanceBaseline().SetValue(capillaryResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
    SEFluidCircuitPath& GutE2ToGutE3 = cCombinedCardiovascular.CreatePath(GutE2, GutE3, BGE::TissueLitePath::GutE2ToGutE3);
    GutE2ToGutE3.GetPressureSourceBaseline().SetValue(copExtracell_mmHg, PressureUnit::mmHg);
    SEFluidCircuitPath& GutE3ToGround = cCombinedCardiovascular.CreatePath(GutE3, *Ground, BGE::TissueLitePath::GutE3ToGround);
    GutE3ToGround.GetComplianceBaseline().SetValue(GutE3.GetVolumeBaseline(VolumeUnit::mL) / vNodePressure, FlowComplianceUnit::mL_Per_mmHg); //Might need to change this
    SEFluidCircuitPath& GutE3ToGutI = cCombinedCardiovascular.CreatePath(GutE3, GutI, BGE::TissueLitePath::GutE3ToGutI);
    GutE3ToGutI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
    SEFluidCircuitPath& GutIToGround = cCombinedCardiovascular.CreatePath(GutI, *Ground, BGE::TissueLitePath::GutIToGround);
    GutIToGround.GetComplianceBaseline().SetValue(GutI.GetVolumeBaseline(VolumeUnit::mL) / GutI.GetPressure(PressureUnit::mmHg), FlowComplianceUnit::mL_Per_mmHg);

    SEFluidCircuitPath& GutE3ToGutL = cCombinedCardiovascular.CreatePath(GutE3, GutL, BGE::TissueLitePath::GutE3ToGutL);
    GutE3ToGutL.GetPressureSourceBaseline().SetValue(lymphDrivePressure_mmHg, PressureUnit::mmHg);
    SEFluidCircuitPath& GutLToLymph = cCombinedCardiovascular.CreatePath(GutL, Lymph, BGE::TissueLitePath::GutLToLymph);
    GutLToLymph.GetResistanceBaseline().SetValue(lymphResistance_mmHg_min_Per_mL, FlowResistanceUnit::mmHg_min_Per_mL);
    GutLToLymph.SetNextPolarizedState(CDM::enumOpenClosed::Open);

    SETissueCompartment& GutTissue = m_Compartments->CreateTissueCompartment(BGE::TissueLiteCompartment::Gut);
    SELiquidCompartment& GutExtracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::GutExtracellular);
    SELiquidCompartment& GutIntracellular = m_Compartments->CreateLiquidCompartment(BGE::ExtravascularLiteCompartment::GutIntracellular);
    GutTissue.GetMatrixVolume().SetValue((1 - GutLiteEWFraction - GutLiteIWFraction) * GutLiteTissueVolume * 1000.0, VolumeUnit::mL);
    GutExtracellular.MapNode(GutE1);
    GutExtracellular.MapNode(GutE2);
    GutExtracellular.MapNode(GutE3);
    GutIntracellular.MapNode(GutI);
    GutExtracellular.MapNode(GutL);
    GutExtracellular.GetWaterVolumeFraction().SetValue(GutLiteEWFraction);
    GutIntracellular.GetWaterVolumeFraction().SetValue(GutLiteIWFraction);
    GutTissue.GetAcidicPhospohlipidConcentration().SetValue(GutLiteAPL, MassPerMassUnit::mg_Per_g);
    GutTissue.GetNeutralLipidsVolumeFraction().SetValue(GutLiteNLFraction);
    GutTissue.GetNeutralPhospholipidsVolumeFraction().SetValue(GutLiteNPFraction);
    GutTissue.GetTissueToPlasmaAlbuminRatio().SetValue(GutLiteARatio);
    GutTissue.GetTissueToPlasmaAlphaAcidGlycoproteinRatio().SetValue(GutLiteAAGRatio);
    GutTissue.GetTissueToPlasmaLipoproteinRatio().SetValue(GutLiteLRatio);
    GutTissue.GetTotalMass().SetValue(GutLiteTissueMass, MassUnit::kg);
    GutTissue.GetMembranePotential().SetValue(-84.8, ElectricPotentialUnit::mV);
    GutTissue.GetReflectionCoefficient().SetValue(1.0);

    gCombinedCardiovascular.AddCompartment(GutExtracellular);
    gCombinedCardiovascular.AddCompartment(GutIntracellular);
  

  ////Finalize Circuit Changes
  cCombinedCardiovascular.SetNextAndCurrentFromBaselines();
  cCombinedCardiovascular.StateChange();

  ////Add compartments to graph, but not links because all substance transport involving extracellular is performed manually in Tissue::CalculateDiffusion
  gCombinedCardiovascular.AddCompartment(FatExtracellular);
  gCombinedCardiovascular.AddCompartment(FatIntracellular);
  gCombinedCardiovascular.AddCompartment(BoneExtracellular);
  gCombinedCardiovascular.AddCompartment(BoneIntracellular);
  gCombinedCardiovascular.AddCompartment(BrainExtracellular);
  gCombinedCardiovascular.AddCompartment(BrainIntracellular);
  gCombinedCardiovascular.AddCompartment(KidneyExtracellular);
  gCombinedCardiovascular.AddCompartment(KidneyIntracellular);
  gCombinedCardiovascular.AddCompartment(LiverExtracellular);
  gCombinedCardiovascular.AddCompartment(LiverIntracellular);
  gCombinedCardiovascular.AddCompartment(LungExtracellular);
  gCombinedCardiovascular.AddCompartment(LungIntracellular);
  gCombinedCardiovascular.AddCompartment(MuscleExtracellular);
  gCombinedCardiovascular.AddCompartment(MuscleIntracellular);
  gCombinedCardiovascular.AddCompartment(MyocardiumExtracellular);
  gCombinedCardiovascular.AddCompartment(MyocardiumIntracellular);
  gCombinedCardiovascular.AddCompartment(SkinIntracellular);
  gCombinedCardiovascular.AddCompartment(SkinExtracellular);

  gCombinedCardiovascular.StateChange();
}

void BioGears::SetupRespiratory()
{
  Info("Setting Up Respiratory");
  double RightLungRatio = m_Patient->GetRightLungRatio().GetValue();
  double LeftLungRatio = 1 - RightLungRatio;

  SEFluidCircuit& cRespiratory = m_Circuits->GetRespiratoryCircuit();
  SEFluidCircuitNode* Ambient = m_Circuits->GetFluidNode(BGE::EnvironmentNode::Ambient);
  cRespiratory.AddReferenceNode(*Ambient);
  SEGasCompartmentGraph& gRespiratory = m_Compartments->GetRespiratoryGraph();
  SEGasCompartment* gEnvironment = m_Compartments->GetGasCompartment(BGE::EnvironmentCompartment::Ambient);
  double AmbientPressure = 1033.23; // = 1 atm
  //Values from standard
  double FunctionalResidualCapacity_L = 2.313; //2.5;
  double LungResidualVolume_L = 1.234; //1.45;
  double OpenResistance_cmH2O_s_Per_L = m_Config->GetDefaultOpenFlowResistance(FlowResistanceUnit::cmH2O_s_Per_L);

  //We are using BioGears Lite.  This circuit combines left/right lung into single entity.  Based off circuit in Albanese2015Integrated
  //Compliances
  double throatCompliance_L_Per_cmH2O = 0.00238;
  double bronchiCompliance_L_Per_cmH2O = 0.03; //0.0131-->0.05;
  double alveoliCompliance_L_Per_cmH2O = 0.175; //0.2--0.155
  double chestWallCompliance_L_Per_cmH2O = 0.21; //0.2445
  //Resistances
  double totalPulmonaryResistance = 1.75;
  double TracheaResistancePercent = 0.6;
  double BronchiResistancePercent = 0.3;
  double AlveoliDuctResistancePercent = 0.1;
  double mouthToTracheaResistance_cmH2O_s_Per_L = TracheaResistancePercent * totalPulmonaryResistance;
  double tracheaToBronchiResistance_cmH2O_s_Per_L = BronchiResistancePercent * totalPulmonaryResistance;
  double bronchiToAlveoliResistance_cmH2O_s_Per_L = AlveoliDuctResistancePercent * totalPulmonaryResistance;
  //Target volumes are end-expiratory (i.e. bottom of breathing cycle, pressures = ambient pressure)
  double larynxVolume_mL = 34.4;
  double tracheaVolume_mL = 6.63;
  double bronchiVolume_mL = 20.0;
  double alveoliVolume_L = LungResidualVolume_L;
  //Circuit Nodes
  SEFluidCircuitNode& Mouth = cRespiratory.CreateNode(BGE::RespiratoryLiteNode::Mouth);
  Mouth.GetPressure().SetValue(AmbientPressure, PressureUnit::cmH2O);
  Mouth.GetVolumeBaseline().SetValue(20.6, VolumeUnit::mL);
  SEFluidCircuitNode& Trachea = cRespiratory.CreateNode(BGE::RespiratoryLiteNode::Trachea);
  Trachea.GetVolumeBaseline().SetValue(tracheaVolume_mL + larynxVolume_mL, VolumeUnit::mL);
  Trachea.GetPressure().SetValue(AmbientPressure, PressureUnit::cmH2O);
  SEFluidCircuitNode& Bronchi = cRespiratory.CreateNode(BGE::RespiratoryLiteNode::Bronchi);
  Bronchi.GetVolumeBaseline().SetValue(bronchiVolume_mL, VolumeUnit::mL);
  Bronchi.GetPressure().SetValue(AmbientPressure, PressureUnit::cmH2O);
  SEFluidCircuitNode& Alveoli = cRespiratory.CreateNode(BGE::RespiratoryLiteNode::Alveoli);
  Alveoli.GetVolumeBaseline().SetValue(alveoliVolume_L, VolumeUnit::L);
  Alveoli.GetPressure().SetValue(AmbientPressure, PressureUnit::cmH2O);
  SEFluidCircuitNode& PleuralConnection = cRespiratory.CreateNode(BGE::RespiratoryLiteNode::PleuralConnection);
  PleuralConnection.GetPressure().SetValue(AmbientPressure, PressureUnit::cmH2O);
  SEFluidCircuitNode& Pleural = cRespiratory.CreateNode(BGE::RespiratoryLiteNode::Pleural);
  Pleural.GetPressure().SetValue(AmbientPressure, PressureUnit::cmH2O);
  Pleural.GetVolumeBaseline().SetValue(0.017, VolumeUnit::L); //From BioGears.cpp
  SEFluidCircuitNode& RespiratoryMuscle = cRespiratory.CreateNode(BGE::RespiratoryLiteNode::RespiratoryMuscle);
  RespiratoryMuscle.GetPressure().SetValue(AmbientPressure, PressureUnit::cmH2O);
  SEFluidCircuitNode& AlveoliLeak = cRespiratory.CreateNode(BGE::RespiratoryLiteNode::AlveoliLeak);
  AlveoliLeak.GetPressure().SetValue(AmbientPressure, PressureUnit::cmH2O);

  //Pathways
  SEFluidCircuitPath& EnvironmentToMouth = cRespiratory.CreatePath(*Ambient, Mouth, BGE::RespiratoryLitePath::EnvironmentToMouth); //This could be ambient air or the Lite Anesthesia Machine
  EnvironmentToMouth.GetPressureSourceBaseline().SetValue(0.0, PressureUnit::cmH2O);
  SEFluidCircuitPath& MouthToTrachea = cRespiratory.CreatePath(Mouth, Trachea, BGE::RespiratoryLitePath::MouthToTrachea);
  MouthToTrachea.GetResistanceBaseline().SetValue(mouthToTracheaResistance_cmH2O_s_Per_L, FlowResistanceUnit::cmH2O_s_Per_L);
  SEFluidCircuitPath& TracheaToBronchi = cRespiratory.CreatePath(Trachea, Bronchi, BGE::RespiratoryLitePath::TracheaToBronchi);
  TracheaToBronchi.GetResistanceBaseline().SetValue(tracheaToBronchiResistance_cmH2O_s_Per_L, FlowResistanceUnit::cmH2O_s_Per_L);
  SEFluidCircuitPath& TracheaCompliance = cRespiratory.CreatePath(Trachea, PleuralConnection, BGE::RespiratoryLitePath::TracheaToPleuralConnection);
  TracheaCompliance.GetComplianceBaseline().SetValue(throatCompliance_L_Per_cmH2O, FlowComplianceUnit::L_Per_cmH2O);
  TracheaCompliance.SetNextPolarizedState(CDM::enumOpenClosed::Closed);
  SEFluidCircuitPath& BronchiToAlveoli = cRespiratory.CreatePath(Bronchi, Alveoli, BGE::RespiratoryLitePath::BronchiToAlveoli);
  BronchiToAlveoli.GetResistanceBaseline().SetValue(bronchiToAlveoliResistance_cmH2O_s_Per_L, FlowResistanceUnit::cmH2O_s_Per_L);
  SEFluidCircuitPath& BronchiCompliance = cRespiratory.CreatePath(Bronchi, PleuralConnection, BGE::RespiratoryLitePath::BronchiToPleuralConnection);
  BronchiCompliance.GetComplianceBaseline().SetValue(bronchiCompliance_L_Per_cmH2O, FlowComplianceUnit::L_Per_cmH2O);
  BronchiCompliance.SetNextPolarizedState(CDM::enumOpenClosed::Closed);
  SEFluidCircuitPath& AlveoliCompliance = cRespiratory.CreatePath(Alveoli, PleuralConnection, BGE::RespiratoryLitePath::AlveoliToPleuralConnection);
  AlveoliCompliance.GetComplianceBaseline().SetValue(alveoliCompliance_L_Per_cmH2O, FlowComplianceUnit::L_Per_cmH2O);
  AlveoliCompliance.SetNextPolarizedState(CDM::enumOpenClosed::Closed);
  //Need this pathway with no element so that we don't have capacitors on two capacitors between three nodes (hard for solver to address)
  SEFluidCircuitPath& PleuralConnectionToPleural = cRespiratory.CreatePath(PleuralConnection, Pleural, BGE::RespiratoryLitePath::PleuralConnectionToPleural);
  SEFluidCircuitPath& PleuralCompliance = cRespiratory.CreatePath(Pleural, RespiratoryMuscle, BGE::RespiratoryLitePath::PleuralToRespiratoryMuscle);
  PleuralCompliance.GetComplianceBaseline().SetValue(chestWallCompliance_L_Per_cmH2O, FlowComplianceUnit::L_Per_cmH2O);
  SEFluidCircuitPath& RespiratoryDriver = cRespiratory.CreatePath(*Ambient, RespiratoryMuscle, BGE::RespiratoryLitePath::RespiratoryMuscleDriver);
  RespiratoryDriver.GetPressureSourceBaseline().SetValue(0.0, PressureUnit::cmH2O);
  // Path between alveoli and pleural - for pleural leak
  SEFluidCircuitPath& AlveoliToAlveoliLeak = cRespiratory.CreatePath(Alveoli, AlveoliLeak, BGE::RespiratoryLitePath::AlveoliToAlveoliLeak);
  AlveoliToAlveoliLeak.SetNextValve(CDM::enumOpenClosed::Closed);
  SEFluidCircuitPath& AlveoliLeakToPleural = cRespiratory.CreatePath(AlveoliLeak, Pleural, BGE::RespiratoryLitePath::AlveoliLeakToPleural);
  AlveoliLeakToPleural.GetResistanceBaseline().SetValue(OpenResistance_cmH2O_s_Per_L, FlowResistanceUnit::cmH2O_s_Per_L);
  //Path for needle decompression
  SEFluidCircuitPath& PleuralToEnvironment = cRespiratory.CreatePath(Pleural, *Ambient, BGE::RespiratoryLitePath::PleuralToEnvironment);
  PleuralToEnvironment.GetResistanceBaseline().SetValue(OpenResistance_cmH2O_s_Per_L, FlowResistanceUnit::cmH2O_s_Per_L);

  cRespiratory.SetNextAndCurrentFromBaselines();
  cRespiratory.StateChange();

  SEGasCompartment& pMouth = m_Compartments->CreateGasCompartment(BGE::PulmonaryLiteCompartment::Mouth);
  pMouth.MapNode(Mouth);
  SEGasCompartment& pTrachea = m_Compartments->CreateGasCompartment(BGE::PulmonaryLiteCompartment::Trachea);
  pTrachea.MapNode(Trachea);
  SEGasCompartment& pBronchi = m_Compartments->CreateGasCompartment(BGE::PulmonaryLiteCompartment::Bronchi);
  pBronchi.MapNode(Bronchi);
  SEGasCompartment& pAlveoli = m_Compartments->CreateGasCompartment(BGE::PulmonaryLiteCompartment::Alveoli);
  pAlveoli.MapNode(Alveoli);
  SEGasCompartment& pPleural = m_Compartments->CreateGasCompartment(BGE::PulmonaryLiteCompartment::Pleural);
  pPleural.MapNode(PleuralConnection);
  pPleural.MapNode(Pleural);
  pPleural.MapNode(RespiratoryMuscle);
  SEGasCompartment& pDeadSpace = m_Compartments->CreateGasCompartment(BGE::PulmonaryLiteCompartment::DeadSpace);
  pDeadSpace.AddChild(pTrachea);
  pDeadSpace.AddChild(pBronchi);
  SEGasCompartment& pAlveoliLeak = m_Compartments->CreateGasCompartment(BGE::PulmonaryLiteCompartment::AlveoliLeak);
  pAlveoliLeak.MapNode(AlveoliLeak);

  SEGasCompartmentLink& pEnvironmentToMouth = m_Compartments->CreateGasLink(*gEnvironment, pMouth, BGE::PulmonaryLiteLink::EnvironmentToMouth);
  pEnvironmentToMouth.MapPath(EnvironmentToMouth);
  SEGasCompartmentLink& pMouthToTrachea = m_Compartments->CreateGasLink(pMouth, pTrachea, BGE::PulmonaryLiteLink::MouthToTrachea);
  pMouthToTrachea.MapPath(MouthToTrachea);
  SEGasCompartmentLink& pTracheaToBronchi = m_Compartments->CreateGasLink(pTrachea, pBronchi, BGE::PulmonaryLiteLink::TracheaToBronchi);
  pTracheaToBronchi.MapPath(TracheaToBronchi);
  SEGasCompartmentLink& pBronchiToAlveoli = m_Compartments->CreateGasLink(pBronchi, pAlveoli, BGE::PulmonaryLiteLink::BronchiToAlveoli);
  pBronchiToAlveoli.MapPath(BronchiToAlveoli);
  SEGasCompartmentLink& pAlveoliToAlveoliLeak = m_Compartments->CreateGasLink(pAlveoli, pAlveoliLeak, BGE::PulmonaryLiteLink::AlveoliToAlveoliLeak);
  pAlveoliToAlveoliLeak.MapPath(AlveoliToAlveoliLeak);
  SEGasCompartmentLink& pAlveoliLeakToPleural = m_Compartments->CreateGasLink(pAlveoliLeak, pPleural, BGE::PulmonaryLiteLink::AlveoliLeakToPleural);
  pAlveoliLeakToPleural.MapPath(AlveoliLeakToPleural);
  SEGasCompartmentLink& pPleuralToEnvironment = m_Compartments->CreateGasLink(pPleural, *gEnvironment, BGE::PulmonaryLiteLink::PleuralToEnvironment);
  pPleuralToEnvironment.MapPath(PleuralToEnvironment);

  gRespiratory.AddCompartment(*gEnvironment);
  gRespiratory.AddCompartment(pMouth);
  gRespiratory.AddCompartment(pTrachea);
  gRespiratory.AddCompartment(pBronchi);
  gRespiratory.AddCompartment(pAlveoli);
  gRespiratory.AddCompartment(pPleural);
  gRespiratory.AddCompartment(pAlveoliLeak);
  gRespiratory.AddLink(pEnvironmentToMouth);
  gRespiratory.AddLink(pMouthToTrachea);
  gRespiratory.AddLink(pTracheaToBronchi);
  gRespiratory.AddLink(pBronchiToAlveoli);
  gRespiratory.AddLink(pAlveoliToAlveoliLeak);
  gRespiratory.AddLink(pAlveoliLeakToPleural);
  gRespiratory.AddLink(pPleuralToEnvironment);
  gRespiratory.StateChange();

  // Generically set up the Aerosol Graph, this is a mirror of the Respiratory Gas Graph, only it's a liquid graph
  ////MCM--Unclear if we want to keep this for BgLite, but here it is if we want it
  SELiquidCompartmentGraph& lAerosol = m_Compartments->GetAerosolGraph();
  SELiquidCompartment* lEnvironment = m_Compartments->GetLiquidCompartment(BGE::EnvironmentCompartment::Ambient);
  lAerosol.AddCompartment(*lEnvironment);
  // First Create the compartments and map the same nodes
  for (auto name : BGE::PulmonaryLiteCompartment::GetValues()) {
    SEGasCompartment* gasCmpt = m_Compartments->GetGasCompartment(name);
    SELiquidCompartment& liquidCmpt = m_Compartments->CreateLiquidCompartment(name);
    if (gasCmpt->HasNodeMapping()) {
      for (auto node : gasCmpt->GetNodeMapping().GetNodes()) {
        liquidCmpt.MapNode(*node);
      }
    }
  }
  // Hook up any hierarchies
  for (auto name : BGE::PulmonaryLiteCompartment::GetValues()) {
    SEGasCompartment* gasCmpt = m_Compartments->GetGasCompartment(name);
    SELiquidCompartment* liquidCmpt = m_Compartments->GetLiquidCompartment(name);
    if (gasCmpt->HasChildren()) {
      for (auto child : gasCmpt->GetChildren()) {
        liquidCmpt->AddChild(*m_Compartments->GetLiquidCompartment(child->GetName()));
      }
    }
  }
  // Add leaf compartments to the graph
  for (auto name : BGE::PulmonaryLiteCompartment::GetValues()) {
    SELiquidCompartment* liquidCmpt = m_Compartments->GetLiquidCompartment(name);
    if (!liquidCmpt->HasChildren()) {
      lAerosol.AddCompartment(*liquidCmpt);
    }
  }
  // Create Links
  for (auto name : BGE::PulmonaryLiteLink::GetValues()) {
    SEGasCompartmentLink* gasLink = m_Compartments->GetGasLink(name);
    SELiquidCompartment* src = m_Compartments->GetLiquidCompartment(gasLink->GetSourceCompartment().GetName());
    SELiquidCompartment* tgt = m_Compartments->GetLiquidCompartment(gasLink->GetTargetCompartment().GetName());
    SELiquidCompartmentLink& liquidLink = m_Compartments->CreateLiquidLink(*src, *tgt, name);
    if (gasLink->HasPath()) {
      liquidLink.MapPath(*gasLink->GetPath());
    }
    lAerosol.AddLink(liquidLink);
  }
  lAerosol.StateChange();
}

void BioGears::SetupGastrointestinal()
{
  Info("Setting Up Gastrointestinal");
  // Circuit
  SEFluidCircuit& cCombinedCardiovascular = m_Circuits->GetActiveCardiovascularCircuit();

  SEFluidCircuitNode& SmallIntestineC1 = cCombinedCardiovascular.CreateNode(BGE::ChymeNode::SmallIntestineC1);
  SmallIntestineC1.GetPressure().SetValue(0, PressureUnit::mmHg);
  SmallIntestineC1.GetVolumeBaseline().SetValue(100, VolumeUnit::mL);

  SEFluidCircuitNode* SmallIntestine1 = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::SmallIntestine1);
  SEFluidCircuitNode* Ground = cCombinedCardiovascular.GetNode(BGE::CardiovascularNode::Ground);

  SEFluidCircuitPath& SmallIntestineC1ToSmallIntestine1 = cCombinedCardiovascular.CreatePath(SmallIntestineC1, *SmallIntestine1, BGE::ChymePath::SmallIntestineC1ToSmallIntestine1);
  SmallIntestineC1ToSmallIntestine1.GetFlowSourceBaseline().SetValue(0, VolumePerTimeUnit::mL_Per_min);
  SEFluidCircuitPath& GroundToSmallIntestineC1 = cCombinedCardiovascular.CreatePath(*Ground, SmallIntestineC1, BGE::ChymePath::GroundToSmallIntestineC1);

  if (m_Config->IsTissueEnabled()) {
    SEFluidCircuitNode* GutE3 = cCombinedCardiovascular.GetNode(BGE::TissueLiteNode::GutE3);
    SEFluidCircuitPath& SplanchnicE3ToGroundGI = cCombinedCardiovascular.CreatePath(*GutE3, *Ground, BGE::ChymePath::GutE3ToGroundGI);
    SplanchnicE3ToGroundGI.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::mL_Per_s);
  }

  cCombinedCardiovascular.SetNextAndCurrentFromBaselines();
  cCombinedCardiovascular.StateChange();

  // Compartment
  SELiquidCompartment& cSmallIntestine = m_Compartments->CreateLiquidCompartment(BGE::ChymeCompartment::SmallIntestine);
  cSmallIntestine.MapNode(SmallIntestineC1);

  //remove the link because substances are handled manually in model design
  SELiquidCompartment* vSmallIntestine = m_Compartments->GetLiquidCompartment(BGE::VascularLiteCompartment::Gut);
  //SELiquidCompartmentLink& lSmallIntestineChymeToVasculature = m_Compartments->CreateLiquidLink(cSmallIntestine, *vSmallIntestine, BGE::ChymeLink::SmallIntestineChymeToVasculature);
  //lSmallIntestineChymeToVasculature.MapPath(SmallIntestineC1ToSmallIntestine1);

  SELiquidCompartmentGraph& gCombinedCardiovascular = m_Compartments->GetActiveCardiovascularGraph();
  gCombinedCardiovascular.AddCompartment(cSmallIntestine);
  //gCombinedCardiovascular.AddLink(lSmallIntestineChymeToVasculature);
  gCombinedCardiovascular.StateChange();
}

void BioGears::SetupAnesthesiaMachine()
{
  Info("Setting Up Anesthesia Machine");
  /////////////////////// Circuit Interdependencies
  double AmbientPresure = 1033.23; // = 1 atm // Also defined in SetupRespiratoryCircuit
  SEFluidCircuit& cRespiratory = m_Circuits->GetRespiratoryCircuit();
  SEGasCompartmentGraph& gRespiratory = m_Compartments->GetRespiratoryGraph();
  ///////////////////////

  double ventilatorVolume_L = 1.0;
  double ventilatorCompliance_L_Per_cmH2O = 0.5;
  double dValveOpenResistance = m_Config->GetMachineOpenResistance(FlowResistanceUnit::cmH2O_s_Per_L);
  double dValveClosedResistance = m_Config->GetMachineClosedResistance(FlowResistanceUnit::cmH2O_s_Per_L);
  double dSwitchOpenResistance = m_Config->GetDefaultOpenFlowResistance(FlowResistanceUnit::cmH2O_s_Per_L);
  double dSwitchClosedResistance = m_Config->GetDefaultClosedFlowResistance(FlowResistanceUnit::cmH2O_s_Per_L);
  double dLowResistance = 0.01;

  SEFluidCircuit& cAnesthesia = m_Circuits->GetAnesthesiaMachineCircuit();
  SEFluidCircuitNode* Ambient = m_Circuits->GetFluidNode(BGE::EnvironmentNode::Ambient);
  cAnesthesia.AddReferenceNode(*Ambient);

  ////////////////
  // Ventilator //
  SEFluidCircuitNode& Ventilator = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::Ventilator);
  Ventilator.GetVolumeBaseline().SetValue(ventilatorVolume_L, VolumeUnit::L);
  Ventilator.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  //////////////////////////
  // VentilatorConnection //
  SEFluidCircuitNode& VentilatorConnection = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::VentilatorConnection);
  VentilatorConnection.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  /////////////////
  // ReliefValve //
  SEFluidCircuitNode& ReliefValve = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::ReliefValve);
  ReliefValve.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  //////////////
  // Selector //
  SEFluidCircuitNode& Selector = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::Selector);
  Selector.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  Selector.GetVolumeBaseline().SetValue(0.1, VolumeUnit::L);
  //////////////
  // Scrubber //
  SEFluidCircuitNode& Scrubber = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::Scrubber);
  Scrubber.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  Scrubber.GetVolumeBaseline().SetValue(0.1, VolumeUnit::L);
  ////////////
  // YPiece //
  SEFluidCircuitNode& Ypiece = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::YPiece);
  Ypiece.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  Ypiece.GetVolumeBaseline().SetValue(0.01, VolumeUnit::L);
  //////////////
  // GasInlet //
  SEFluidCircuitNode& GasInlet = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::GasInlet);
  GasInlet.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  GasInlet.GetVolumeBaseline().SetValue(0.1, VolumeUnit::L);
  ///////////////
  // GasSource //
  SEFluidCircuitNode& GasSource = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::GasSource);
  GasSource.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  GasSource.GetVolumeBaseline().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::mL);
  //////////////////////////
  // AnesthesiaConnection //
  SEFluidCircuitNode& AnesthesiaConnection = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::AnesthesiaConnection);
  AnesthesiaConnection.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  AnesthesiaConnection.GetVolumeBaseline().SetValue(0.01, VolumeUnit::L);
  /////////////////////
  // InspiratoryLimb //
  SEFluidCircuitNode& InspiratoryLimb = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::InspiratoryLimb);
  InspiratoryLimb.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  InspiratoryLimb.GetVolumeBaseline().SetValue(0.1, VolumeUnit::L);
  ////////////////////
  // ExpiratoryLimb //
  SEFluidCircuitNode& ExpiratoryLimb = cAnesthesia.CreateNode(BGE::AnesthesiaMachineNode::ExpiratoryLimb);
  ExpiratoryLimb.GetPressure().SetValue(AmbientPresure, PressureUnit::cmH2O);
  ExpiratoryLimb.GetVolumeBaseline().SetValue(0.1, VolumeUnit::L);

  /////////////////////////////
  // EnvironmentToVentilator //
  SEFluidCircuitPath& EnvironmentToVentilator = cAnesthesia.CreatePath(*Ambient, Ventilator, BGE::AnesthesiaMachinePath::EnvironmentToVentilator);
  EnvironmentToVentilator.GetPressureSourceBaseline().SetValue(0.0, PressureUnit::cmH2O);
  //////////////////////////////
  // EnvironmentToReliefValve //
  SEFluidCircuitPath& EnvironmentToReliefValve = cAnesthesia.CreatePath(*Ambient, ReliefValve, BGE::AnesthesiaMachinePath::EnvironmentToReliefValve);
  EnvironmentToReliefValve.GetPressureSourceBaseline().SetValue(100.0, PressureUnit::cmH2O);
  //////////////////////////////////////
  // VentilatorConnectionToVentilator //
  SEFluidCircuitPath& VentilatorToVentilatorConnection = cAnesthesia.CreatePath(Ventilator, VentilatorConnection, BGE::AnesthesiaMachinePath::VentilatorToVentilatorConnection);
  VentilatorToVentilatorConnection.GetComplianceBaseline().SetValue(ventilatorCompliance_L_Per_cmH2O, FlowComplianceUnit::L_Per_cmH2O);
  VentilatorToVentilatorConnection.SetNextPolarizedState(CDM::enumOpenClosed::Closed);
  ////////////////////////////////////
  // VentilatorConnectionToSelector //
  SEFluidCircuitPath& VentilatorConnectionToSelector = cAnesthesia.CreatePath(VentilatorConnection, Selector, BGE::AnesthesiaMachinePath::VentilatorConnectionToSelector);
  ///////////////////////////
  // SelectorToReliefValve //
  SEFluidCircuitPath& SelectorToReliefValve = cAnesthesia.CreatePath(Selector, ReliefValve, BGE::AnesthesiaMachinePath::SelectorToReliefValve);
  SelectorToReliefValve.SetNextValve(CDM::enumOpenClosed::Open);
  ////////////////////////
  // SelectorToScrubber //
  SEFluidCircuitPath& SelectorToScrubber = cAnesthesia.CreatePath(Selector, Scrubber, BGE::AnesthesiaMachinePath::SelectorToScrubber);
  SelectorToScrubber.GetResistanceBaseline().SetValue(dLowResistance, FlowResistanceUnit::cmH2O_s_Per_L);
  ////////////////////////
  // ScrubberToGasInlet //
  SEFluidCircuitPath& ScrubberToGasInlet = cAnesthesia.CreatePath(Scrubber, GasInlet, BGE::AnesthesiaMachinePath::ScrubberToGasInlet);
  ScrubberToGasInlet.GetResistanceBaseline().SetValue(dLowResistance, FlowResistanceUnit::cmH2O_s_Per_L);
  ////////////////////////////
  // EnvironmentToGasSource //
  SEFluidCircuitPath& EnvironmentToGasSource = cAnesthesia.CreatePath(*Ambient, GasSource, BGE::AnesthesiaMachinePath::EnvironmentToGasSource);
  ///////////////////////////
  // SelectorToEnvironment //
  SEFluidCircuitPath& SelectorToEnvironment = cAnesthesia.CreatePath(Selector, *Ambient, BGE::AnesthesiaMachinePath::SelectorToEnvironment);
  SelectorToEnvironment.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::L_Per_s); //Exhaust
  /////////////////////////
  // GasSourceToGasInlet //
  SEFluidCircuitPath& GasSourceToGasInlet = cAnesthesia.CreatePath(GasSource, GasInlet, BGE::AnesthesiaMachinePath::GasSourceToGasInlet);
  GasSourceToGasInlet.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::L_Per_s); //Fresh gas source
  ///////////////////////////////
  // GasInletToInspiratoryLimb //
  SEFluidCircuitPath& GasInletToInspiratoryLimb = cAnesthesia.CreatePath(GasInlet, InspiratoryLimb, BGE::AnesthesiaMachinePath::GasInletToInspiratoryLimb);
  GasInletToInspiratoryLimb.GetResistanceBaseline().SetValue(dLowResistance, FlowResistanceUnit::cmH2O_s_Per_L);
  /////////////////////////////
  // InspiratoryLimbToYPiece //
  SEFluidCircuitPath& InspiratoryLimbToYPiece = cAnesthesia.CreatePath(InspiratoryLimb, Ypiece, BGE::AnesthesiaMachinePath::InspiratoryLimbToYPiece);
  InspiratoryLimbToYPiece.GetResistanceBaseline().SetValue(dLowResistance, FlowResistanceUnit::cmH2O_s_Per_L);
  ////////////////////////////
  // YPieceToExpiratoryLimb //
  SEFluidCircuitPath& YPieceToExpiratoryLimb = cAnesthesia.CreatePath(Ypiece, ExpiratoryLimb, BGE::AnesthesiaMachinePath::YPieceToExpiratoryLimb);
  YPieceToExpiratoryLimb.GetResistanceBaseline().SetValue(dLowResistance, FlowResistanceUnit::cmH2O_s_Per_L);
  //////////////////////////////
  // ExpiratoryLimbToSelector //
  SEFluidCircuitPath& ExpiratoryLimbToSelector = cAnesthesia.CreatePath(ExpiratoryLimb, Selector, BGE::AnesthesiaMachinePath::ExpiratoryLimbToSelector);
  ExpiratoryLimbToSelector.GetResistanceBaseline().SetValue(dLowResistance, FlowResistanceUnit::cmH2O_s_Per_L);
  //////////////////////////////////
  // YPieceToAnesthesiaConnection //
  SEFluidCircuitPath& YPieceToAnesthesiaConnection = cAnesthesia.CreatePath(Ypiece, AnesthesiaConnection, BGE::AnesthesiaMachinePath::YPieceToAnesthesiaConnection);
  ///////////////////////////////////////
  // AnesthesiaConnectionToEnvironment //
  SEFluidCircuitPath& AnesthesiaConnectionToEnvironment = cAnesthesia.CreatePath(AnesthesiaConnection, *Ambient, BGE::AnesthesiaMachinePath::AnesthesiaConnectionToEnvironment);
  AnesthesiaConnectionToEnvironment.GetResistanceBaseline().SetValue(dSwitchOpenResistance, FlowResistanceUnit::cmH2O_s_Per_L);

  cAnesthesia.SetNextAndCurrentFromBaselines();
  cAnesthesia.StateChange();

  //Combined Respiratory and Anesthesia Machine Circuit
  SEFluidCircuit& cCombinedAnesthesia = m_Circuits->GetRespiratoryAndAnesthesiaMachineCircuit();
  cCombinedAnesthesia.AddCircuit(cRespiratory);
  cCombinedAnesthesia.AddCircuit(cAnesthesia);
  SEFluidCircuitNode& Mouth = *cCombinedAnesthesia.GetNode(BGE::RespiratoryNode::Mouth);
  SEFluidCircuitPath& AnesthesiaConnectionToMouth = cCombinedAnesthesia.CreatePath(AnesthesiaConnection, Mouth, "AnesthesiaConnectionToMouth");
  cCombinedAnesthesia.RemovePath(BGE::RespiratoryPath::EnvironmentToMouth);
  cCombinedAnesthesia.SetNextAndCurrentFromBaselines();
  cCombinedAnesthesia.StateChange();

  // Grab the Environment Compartment
  SEGasCompartment* eEnvironment = m_Compartments->GetGasCompartment(BGE::EnvironmentCompartment::Ambient);
  // Anesthesia Machine Compartments
  SEGasCompartment& aAnesthesiaConnection = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::AnesthesiaConnection);
  aAnesthesiaConnection.MapNode(AnesthesiaConnection);
  SEGasCompartment& aExpiratoryLimb = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::ExpiratoryLimb);
  aExpiratoryLimb.MapNode(ExpiratoryLimb);
  SEGasCompartment& aGasInlet = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::GasInlet);
  aGasInlet.MapNode(GasInlet);
  SEGasCompartment& aGasSource = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::GasSource);
  aGasSource.MapNode(GasSource);
  SEGasCompartment& aInspiratoryLimb = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::InspiratoryLimb);
  aInspiratoryLimb.MapNode(InspiratoryLimb);
  SEGasCompartment& aReliefValve = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::ReliefValve);
  aReliefValve.MapNode(ReliefValve);
  SEGasCompartment& aScrubber = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::Scrubber);
  aScrubber.MapNode(Scrubber);
  SEGasCompartment& aSelector = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::Selector);
  aSelector.MapNode(Selector);
  SEGasCompartment& aVentilator = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::Ventilator);
  aVentilator.MapNode(Ventilator);
  SEGasCompartment& aVentilatorConnection = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::VentilatorConnection);
  aVentilator.MapNode(VentilatorConnection);
  SEGasCompartment& aYPiece = m_Compartments->CreateGasCompartment(BGE::AnesthesiaMachineCompartment::YPiece);
  aYPiece.MapNode(Ypiece);

  // Setup Links //
  SEGasCompartmentLink& aVentilatorToSelector = m_Compartments->CreateGasLink(aVentilator, aSelector, BGE::AnesthesiaMachineLink::VentilatorToSelector);
  aVentilatorToSelector.MapPath(VentilatorConnectionToSelector);
  SEGasCompartmentLink& aSelectorToReliefValve = m_Compartments->CreateGasLink(aSelector, aReliefValve, BGE::AnesthesiaMachineLink::SelectorToReliefValve);
  aSelectorToReliefValve.MapPath(SelectorToReliefValve);
  SEGasCompartmentLink& aSelectorToScrubber = m_Compartments->CreateGasLink(aSelector, aScrubber, BGE::AnesthesiaMachineLink::SelectorToScrubber);
  aSelectorToScrubber.MapPath(SelectorToScrubber);
  SEGasCompartmentLink& aScrubberToGasInlet = m_Compartments->CreateGasLink(aScrubber, aGasInlet, BGE::AnesthesiaMachineLink::ScrubberToGasInlet);
  aScrubberToGasInlet.MapPath(ScrubberToGasInlet);
  SEGasCompartmentLink& aExhaust = m_Compartments->CreateGasLink(aSelector, *eEnvironment, BGE::AnesthesiaMachineLink::Exhaust);
  aExhaust.MapPath(SelectorToEnvironment);
  SEGasCompartmentLink& aGasSourceToGasInlet = m_Compartments->CreateGasLink(aGasSource, aGasInlet, BGE::AnesthesiaMachineLink::GasSourceToGasInlet);
  aGasSourceToGasInlet.MapPath(GasSourceToGasInlet);
  SEGasCompartmentLink& aGasInletToInspiratoryLimb = m_Compartments->CreateGasLink(aGasInlet, aInspiratoryLimb, BGE::AnesthesiaMachineLink::GasInletToInspiratoryLimb);
  aGasInletToInspiratoryLimb.MapPath(GasInletToInspiratoryLimb);
  SEGasCompartmentLink& aInspiratoryLimbToYPiece = m_Compartments->CreateGasLink(aInspiratoryLimb, aYPiece, BGE::AnesthesiaMachineLink::InspiratoryLimbToYPiece);
  aInspiratoryLimbToYPiece.MapPath(InspiratoryLimbToYPiece);
  SEGasCompartmentLink& aYPieceToExpiratoryLimb = m_Compartments->CreateGasLink(aYPiece, aExpiratoryLimb, BGE::AnesthesiaMachineLink::YPieceToExpiratoryLimb);
  aYPieceToExpiratoryLimb.MapPath(YPieceToExpiratoryLimb);
  SEGasCompartmentLink& aExpiratoryLimbToSelector = m_Compartments->CreateGasLink(aExpiratoryLimb, aSelector, BGE::AnesthesiaMachineLink::ExpiratoryLimbToSelector);
  aExpiratoryLimbToSelector.MapPath(ExpiratoryLimbToSelector);
  SEGasCompartmentLink& aYPieceToAnesthesiaConnection = m_Compartments->CreateGasLink(aYPiece, aAnesthesiaConnection, BGE::AnesthesiaMachineLink::YPieceToAnesthesiaConnection);
  aYPieceToAnesthesiaConnection.MapPath(YPieceToAnesthesiaConnection);
  SEGasCompartmentLink& aAnesthesiaConnectionLeak = m_Compartments->CreateGasLink(aAnesthesiaConnection, *eEnvironment, BGE::AnesthesiaMachineLink::AnesthesiaConnectionLeak);
  aAnesthesiaConnectionLeak.MapPath(AnesthesiaConnectionToEnvironment);

  SEGasCompartmentGraph& gAnesthesia = m_Compartments->GetAnesthesiaMachineGraph();
  gAnesthesia.AddCompartment(*eEnvironment);
  gAnesthesia.AddCompartment(aAnesthesiaConnection);
  gAnesthesia.AddCompartment(aExpiratoryLimb);
  gAnesthesia.AddCompartment(aGasInlet);
  gAnesthesia.AddCompartment(aGasSource);
  gAnesthesia.AddCompartment(aInspiratoryLimb);
  gAnesthesia.AddCompartment(aReliefValve);
  gAnesthesia.AddCompartment(aScrubber);
  gAnesthesia.AddCompartment(aSelector);
  gAnesthesia.AddCompartment(aVentilator);
  gAnesthesia.AddCompartment(aYPiece);
  gAnesthesia.AddLink(aVentilatorToSelector);
  gAnesthesia.AddLink(aSelectorToReliefValve);
  gAnesthesia.AddLink(aSelectorToScrubber);
  gAnesthesia.AddLink(aScrubberToGasInlet);
  gAnesthesia.AddLink(aExhaust);
  gAnesthesia.AddLink(aGasSourceToGasInlet);
  gAnesthesia.AddLink(aGasInletToInspiratoryLimb);
  gAnesthesia.AddLink(aInspiratoryLimbToYPiece);
  gAnesthesia.AddLink(aYPieceToExpiratoryLimb);
  gAnesthesia.AddLink(aExpiratoryLimbToSelector);
  gAnesthesia.AddLink(aYPieceToAnesthesiaConnection);
  gAnesthesia.AddLink(aAnesthesiaConnectionLeak);
  gAnesthesia.StateChange();

  //Now do the combined transport setup
  // Grab the mouth from pulmonary
  SEGasCompartment* pMouth = m_Compartments->GetGasCompartment(BGE::PulmonaryCompartment::Mouth);
  SEGasCompartmentLink& aMask = m_Compartments->CreateGasLink(aAnesthesiaConnection, *pMouth, BGE::AnesthesiaMachineLink::Mask);
  aMask.MapPath(AnesthesiaConnectionToMouth);

  SEGasCompartmentGraph& gCombinedRespiratoryAnesthesia = m_Compartments->GetRespiratoryAndAnesthesiaMachineGraph();
  gCombinedRespiratoryAnesthesia.AddGraph(gRespiratory);
  gCombinedRespiratoryAnesthesia.AddGraph(gAnesthesia);
  gCombinedRespiratoryAnesthesia.RemoveLink(BGE::PulmonaryLink::EnvironmentToMouth);
  gCombinedRespiratoryAnesthesia.AddLink(aMask);
  gCombinedRespiratoryAnesthesia.StateChange();
}

void BioGears::SetupInhaler()
{
  Info("Setting Up Inhaler");
  /////////////////////// Circuit Interdependencies
  double dLowResistance = 0.01; // Also defined in SetupRespiratoryCircuit
  SEFluidCircuit& cRespiratory = m_Circuits->GetRespiratoryCircuit();
  SEGasCompartmentGraph& gRespiratory = m_Compartments->GetRespiratoryGraph();
  SELiquidCompartmentGraph& lAerosol = m_Compartments->GetAerosolGraph();
  ///////////////////////

  //Combined Respiratory and Inhaler Circuit
  SEFluidCircuit& m_CombinedInhaler = m_Circuits->GetRespiratoryAndInhalerCircuit();
  m_CombinedInhaler.AddCircuit(cRespiratory);
  // Grab connection points/nodes
  SEFluidCircuitNode& Mouth = *cRespiratory.GetNode(BGE::RespiratoryNode::Mouth);
  SEFluidCircuitNode& Ambient = *cRespiratory.GetNode(BGE::EnvironmentNode::Ambient);
  // Define node on the combined graph, this is a simple circuit, no reason to make a independent circuit at this point
  SEFluidCircuitNode& Mouthpiece = m_CombinedInhaler.CreateNode(BGE::InhalerNode::Mouthpiece);
  Mouthpiece.GetPressure().SetValue(0.0, PressureUnit::cmH2O);
  Mouthpiece.GetNextPressure().SetValue(0.0, PressureUnit::cmH2O);
  double dInhalerBaseVolume_L = 0.030; // 30 milliliters
  Mouthpiece.GetVolumeBaseline().SetValue(dInhalerBaseVolume_L, VolumeUnit::L);
  // Define path on the combined graph, this is a simple circuit, no reason to make a independent circuit at this point
  SEFluidCircuitPath& EnvironmentToMouthpiece = m_CombinedInhaler.CreatePath(Ambient, Mouthpiece, BGE::InhalerPath::EnvironmentToMouthpiece);
  // Connect Path
  SEFluidCircuitPath& MouthpieceToMouth = m_CombinedInhaler.CreatePath(Mouthpiece, Mouth, BGE::InhalerPath::MouthpieceToMouth);
  MouthpieceToMouth.GetResistanceBaseline().SetValue(dLowResistance, FlowResistanceUnit::cmH2O_s_Per_L);
  m_CombinedInhaler.RemovePath(BGE::RespiratoryPath::EnvironmentToMouth);
  m_CombinedInhaler.SetNextAndCurrentFromBaselines();
  m_CombinedInhaler.StateChange();

  //////////////////////
  // GAS COMPARTMENTS //
  SEGasCompartment* gMouth = m_Compartments->GetGasCompartment(BGE::PulmonaryCompartment::Mouth);
  SEGasCompartment* gAmbient = m_Compartments->GetGasCompartment(BGE::EnvironmentCompartment::Ambient);
  //////////////////
  // Compartments //
  SEGasCompartment& gMouthpiece = m_Compartments->CreateGasCompartment(BGE::InhalerCompartment::Mouthpiece);
  gMouthpiece.MapNode(Mouthpiece);
  ///////////
  // Links //
  SEGasCompartmentLink& gEnvironmentToMouthpiece = m_Compartments->CreateGasLink(*gAmbient, gMouthpiece, BGE::InhalerLink::EnvironmentToMouthpiece);
  gEnvironmentToMouthpiece.MapPath(EnvironmentToMouthpiece);
  SEGasCompartmentLink& gMouthpieceToMouth = m_Compartments->CreateGasLink(gMouthpiece, *gMouth, BGE::InhalerLink::MouthpieceToMouth);
  gMouthpieceToMouth.MapPath(MouthpieceToMouth);
  ///////////
  // Graph //
  SEGasCompartmentGraph& gCombinedInhaler = m_Compartments->GetRespiratoryAndInhalerGraph();
  gCombinedInhaler.AddGraph(gRespiratory);
  gCombinedInhaler.RemoveLink(BGE::PulmonaryLink::EnvironmentToMouth);
  gCombinedInhaler.AddCompartment(gMouthpiece);
  gCombinedInhaler.AddLink(gEnvironmentToMouthpiece);
  gCombinedInhaler.AddLink(gMouthpieceToMouth);
  gCombinedInhaler.StateChange();

  // I could probably take the generic code I wrote in SetupRespiratory to clone the gas setup into a liquid setup
  // and then call that method here, but this is such a simple circuit... I will leave that as an exercise for somebody else...

  ///////////////////////////////////
  // LIQUID (AEROSOL) COMPARTMENTS //
  SELiquidCompartment* lMouth = m_Compartments->GetLiquidCompartment(BGE::PulmonaryCompartment::Mouth);
  SELiquidCompartment* lAmbient = m_Compartments->GetLiquidCompartment(BGE::EnvironmentCompartment::Ambient);
  //////////////////
  // Compartments //
  SELiquidCompartment& lMouthpiece = m_Compartments->CreateLiquidCompartment(BGE::InhalerCompartment::Mouthpiece);
  lMouthpiece.MapNode(Mouthpiece);
  ///////////
  // Links //
  SELiquidCompartmentLink& lEnvironmentToMouthpiece = m_Compartments->CreateLiquidLink(*lAmbient, lMouthpiece, BGE::InhalerLink::EnvironmentToMouthpiece);
  lEnvironmentToMouthpiece.MapPath(EnvironmentToMouthpiece);
  SELiquidCompartmentLink& lMouthpieceToMouth = m_Compartments->CreateLiquidLink(lMouthpiece, *lMouth, BGE::InhalerLink::MouthpieceToMouth);
  lMouthpieceToMouth.MapPath(MouthpieceToMouth);
  ///////////
  // Graph //
  SELiquidCompartmentGraph& lCombinedInhaler = m_Compartments->GetAerosolAndInhalerGraph();
  lCombinedInhaler.AddGraph(lAerosol);
  lCombinedInhaler.RemoveLink(BGE::PulmonaryLink::EnvironmentToMouth);
  lCombinedInhaler.AddCompartment(lMouthpiece);
  lCombinedInhaler.AddLink(lEnvironmentToMouthpiece);
  lCombinedInhaler.AddLink(lMouthpieceToMouth);
  lCombinedInhaler.StateChange();
}

void BioGears::SetupMechanicalVentilator()
{
  Info("Setting Up MechanicalVentilator");
  /////////////////////// Circuit Interdependencies
  SEFluidCircuit& cRespiratory = m_Circuits->GetRespiratoryCircuit();
  SEGasCompartmentGraph& gRespiratory = m_Compartments->GetRespiratoryGraph();
  ///////////////////////

  //Combined Respiratory and Mechanical Ventilator Circuit
  SEFluidCircuit& m_CombinedMechanicalVentilator = m_Circuits->GetRespiratoryAndMechanicalVentilatorCircuit();
  m_CombinedMechanicalVentilator.AddCircuit(cRespiratory);
  // Grab connection points/nodes
  SEFluidCircuitNode& Mouth = *cRespiratory.GetNode(BGE::RespiratoryNode::Mouth);
  SEFluidCircuitNode& Ambient = *cRespiratory.GetNode(BGE::EnvironmentNode::Ambient);
  // Define node on the combined graph, this is a simple circuit, no reason to make a independent circuit at this point
  SEFluidCircuitNode& Connection = m_CombinedMechanicalVentilator.CreateNode(BGE::MechanicalVentilatorNode::Connection);
  Connection.GetPressure().Set(Ambient.GetPressure());
  Connection.GetNextPressure().Set(Ambient.GetNextPressure());
  // No connection volume, so volume fractions work properly
  // Paths
  SEFluidCircuitPath& ConnectionToMouth = m_CombinedMechanicalVentilator.CreatePath(Connection, Mouth, BGE::MechanicalVentilatorPath::ConnectionToMouth);
  ConnectionToMouth.GetFlowSourceBaseline().SetValue(0.0, VolumePerTimeUnit::L_Per_s);
  SEFluidCircuitPath& GroundToConnection = m_CombinedMechanicalVentilator.CreatePath(Ambient, Connection, BGE::MechanicalVentilatorPath::GroundToConnection);
  GroundToConnection.GetPressureSourceBaseline().SetValue(0.0, PressureUnit::cmH2O);
  m_CombinedMechanicalVentilator.RemovePath(BGE::RespiratoryPath::EnvironmentToMouth);
  m_CombinedMechanicalVentilator.SetNextAndCurrentFromBaselines();
  m_CombinedMechanicalVentilator.StateChange();

  //////////////////////
  // GAS COMPARTMENTS //
  SEGasCompartment* gMouth = m_Compartments->GetGasCompartment(BGE::PulmonaryCompartment::Mouth);
  //////////////////
  // Compartments //
  SEGasCompartment& gConnection = m_Compartments->CreateGasCompartment(BGE::MechanicalVentilatorCompartment::Connection);
  gConnection.MapNode(Connection);
  ///////////
  // Links //
  SEGasCompartmentLink& gConnectionToMouth = m_Compartments->CreateGasLink(gConnection, *gMouth, BGE::MechanicalVentilatorLink::ConnectionToMouth);
  gConnectionToMouth.MapPath(ConnectionToMouth);
  ///////////
  // Graph //
  SEGasCompartmentGraph& gCombinedMechanicalVentilator = m_Compartments->GetRespiratoryAndMechanicalVentilatorGraph();
  gCombinedMechanicalVentilator.AddGraph(gRespiratory);
  gCombinedMechanicalVentilator.RemoveLink(BGE::PulmonaryLink::EnvironmentToMouth);
  gCombinedMechanicalVentilator.AddCompartment(gConnection);
  gCombinedMechanicalVentilator.AddLink(gConnectionToMouth);
  gCombinedMechanicalVentilator.StateChange();
}

void BioGears::SetupLiteTemperature()
{
  Info("Setting Up Thermal Lite");
  SEThermalCircuit& cThermal = m_Circuits->GetTemperatureCircuit();

  //Set up circuit constants
  //Capacitances
  double skinMassFraction = 0.09; //0.09 is fraction of mass that the skin takes up in a typical human /cite herman2006physics
  double capCore_J_Per_K = (1.0 - skinMassFraction) * m_Patient->GetWeight(MassUnit::kg) * GetConfiguration().GetBodySpecificHeat(HeatCapacitancePerMassUnit::J_Per_K_kg);
  double capSkin_J_per_K = skinMassFraction * m_Patient->GetWeight(MassUnit::kg) * GetConfiguration().GetBodySpecificHeat(HeatCapacitancePerMassUnit::J_Per_K_kg);
  //Resistances
  double skinBloodFlow_m3Persec = 4.97E-06;
  double bloodDensity_kgPerm3 = 1050;
  double bloodSpecificHeat_JPerkgK = 3617;
  double alphaScale = 0.95; //tuning parameter for scaling resistance
  double coreToSkinR_K_Per_W;
  //The heat transfer resistance from the core to the skin is inversely proportional to the skin blood flow.
  //When skin blood flow increases, then heat transfer resistance decreases leading to more heat transfer from core to skin.
  //The opposite occurs for skin blood flow decrease.
  coreToSkinR_K_Per_W = 1.0 / (alphaScale * bloodDensity_kgPerm3 * bloodSpecificHeat_JPerkgK * skinBloodFlow_m3Persec);
  //double skinToExternalR_K_Per_W = 5.0;
  double skinToExternalR_K_Per_W = std::pow(20, 5) * std::abs(2.27 / (10.3 + (1.67 * (10 ^ -7) * std::pow((297 + 307) / 2, 3)))); //check this first num

  //std::cout << coreToSkinR_K_Per_W << std::endl;
  //std::cout << skinToExternalR_K_Per_W << std::endl;

  //Sources
  double MetabolicHeat_W = 100.0;
  double RespirationHeat_W = 10.0;
  double ExternalTemp_K = 295.4; //~72F or ~22.25 degC

  //Circuit Nodes
  SEThermalCircuitNode& Ground = cThermal.CreateNode(BGE::ThermalLiteNode::Ground); //Reference Node for Internal thermal connection
  Ground.GetTemperature().SetValue(0.0, TemperatureUnit::K);
  Ground.GetNextTemperature().SetValue(0.0, TemperatureUnit::K);
  cThermal.AddReferenceNode(Ground);
  SEThermalCircuitNode& Core = cThermal.CreateNode(BGE::ThermalLiteNode::Core);
  Core.GetTemperature().SetValue(37.0, TemperatureUnit::C); //cite Herman2007Physics
  SEThermalCircuitNode& Skin = cThermal.CreateNode(BGE::ThermalLiteNode::Skin);
  Skin.GetTemperature().SetValue(33.0, TemperatureUnit::C); //cite Herman2007Physics
  SEThermalCircuitNode& Environment = cThermal.CreateNode(BGE::ThermalLiteNode::Environment);
  Environment.GetTemperature().SetValue(22, TemperatureUnit::C);
  SEThermalCircuitNode& Ref = cThermal.CreateNode(BGE::ThermalLiteNode::Ref); //Reference Node for External thermal connection
  Ref.GetTemperature().SetValue(0.0, TemperatureUnit::K);
  Ref.GetNextTemperature().SetValue(0.0, TemperatureUnit::K);
  cThermal.AddReferenceNode(Ref);

  //Pathways
  SEThermalCircuitPath& groundToCore = cThermal.CreatePath(Ground, Core, BGE::ThermalLitePath::GroundToCore);
  groundToCore.GetHeatSourceBaseline().SetValue(0.0, PowerUnit::W);
  SEThermalCircuitPath& coreToGround = cThermal.CreatePath(Core, Ground, BGE::ThermalLitePath::CoreToGround);
  //coreToGround.GetCapacitanceBaseline().SetValue(capCore_J_Per_K, HeatCapacitanceUnit::J_Per_K);
  coreToGround.GetCapacitanceBaseline().SetValue(capCore_J_Per_K, HeatCapacitanceUnit::J_Per_K);
  Core.GetHeatBaseline().SetValue(coreToGround.GetCapacitanceBaseline().GetValue(HeatCapacitanceUnit::J_Per_K) * Core.GetTemperature().GetValue(TemperatureUnit::K), EnergyUnit::J);
  SEThermalCircuitPath& coreToSkin = cThermal.CreatePath(Core, Skin, BGE::ThermalLitePath::CoreToSkin);
  coreToSkin.GetResistanceBaseline().SetValue(0.056, HeatResistanceUnit::K_Per_W);
  SEThermalCircuitPath& skinToGround = cThermal.CreatePath(Skin, Ground, BGE::ThermalLitePath::SkinToGround);
  //skinToGround.GetCapacitanceBaseline().SetValue(capSkin_J_per_K, HeatCapacitanceUnit::J_Per_K);
  skinToGround.GetCapacitanceBaseline().SetValue(capSkin_J_per_K, HeatCapacitanceUnit::J_Per_K);
  Skin.GetHeatBaseline().SetValue(skinToGround.GetCapacitanceBaseline().GetValue(HeatCapacitanceUnit::J_Per_K) * Skin.GetTemperature().GetValue(TemperatureUnit::K), EnergyUnit::J);

  SEThermalCircuitPath& refToExternal = cThermal.CreatePath(Ref, Environment, BGE::ThermalLitePath::RefToEnvironment);
  refToExternal.GetTemperatureSourceBaseline().SetValue(0.0, TemperatureUnit::K);
  SEThermalCircuitPath& externalToSkin = cThermal.CreatePath(Environment, Skin, BGE::ThermalLitePath::EnvironmentToSkin);
  externalToSkin.GetResistanceBaseline().SetValue(m_Config->GetDefaultClosedHeatResistance(HeatResistanceUnit::K_Per_W), HeatResistanceUnit::K_Per_W);
  SEThermalCircuitPath& coreToRef = cThermal.CreatePath(Core, Ref, BGE::ThermalLitePath::CoreToRef);
  coreToRef.GetHeatSourceBaseline().SetValue(0.0, PowerUnit::W);

  cThermal.SetNextAndCurrentFromBaselines();
  cThermal.StateChange();

  SEThermalCompartment& cGround = m_Compartments->CreateThermalCompartment(BGE::TemperatureLiteCompartment::Ground);
  cGround.MapNode(Ground);
  SEThermalCompartment& cCore = m_Compartments->CreateThermalCompartment(BGE::TemperatureLiteCompartment::Core);
  cCore.MapNode(Core);
  SEThermalCompartment& cSkin = m_Compartments->CreateThermalCompartment(BGE::TemperatureLiteCompartment::Skin);
  cSkin.MapNode(Skin);
  SEThermalCompartment& cEnvironment = m_Compartments->CreateThermalCompartment(BGE::TemperatureLiteCompartment::Environment);
  cEnvironment.MapNode(Environment);
  SEThermalCompartment& cRef = m_Compartments->CreateThermalCompartment(BGE::TemperatureLiteCompartment::Ref);
  cRef.MapNode(Ref);

  SEThermalCompartmentLink& lGroundToCore = m_Compartments->CreateThermalLink(cGround, cCore, BGE::TemperatureLiteLink::GroundToCore);
  lGroundToCore.MapPath(groundToCore);
  SEThermalCompartmentLink& lCoreToGround = m_Compartments->CreateThermalLink(cCore, cGround, BGE::TemperatureLiteLink::CoreToGround);
  lCoreToGround.MapPath(coreToGround);
  SEThermalCompartmentLink& lCoreToSkin = m_Compartments->CreateThermalLink(cCore, cSkin, BGE::TemperatureLiteLink::CoreToSkin);
  lCoreToSkin.MapPath(coreToSkin);
  SEThermalCompartmentLink& lSkinToGround = m_Compartments->CreateThermalLink(cSkin, cGround, BGE::TemperatureLiteLink::SkinToGround);
  lSkinToGround.MapPath(skinToGround);
  SEThermalCompartmentLink& lRefToEnvironment = m_Compartments->CreateThermalLink(cRef, cEnvironment, BGE::TemperatureLiteLink::RefToEnvironment);
  lRefToEnvironment.MapPath(refToExternal);
  SEThermalCompartmentLink& lEnvironmentToSkin = m_Compartments->CreateThermalLink(cEnvironment, cSkin, BGE::TemperatureLiteLink::EnvironmentToSkin);
  lEnvironmentToSkin.MapPath(externalToSkin);
  SEThermalCompartmentLink& lCoreToRef = m_Compartments->CreateThermalLink(cCore, cRef, BGE::TemperatureLiteLink::CoreToRef);
  lCoreToRef.MapPath(coreToRef);
}
}
