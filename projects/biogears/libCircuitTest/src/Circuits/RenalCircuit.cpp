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

#define _USE_MATH_DEFINES
#include <cmath>

#include <biogears/cdm/circuit/fluid/SEFluidCircuit.h>
#include <biogears/cdm/compartment/fluid/SELiquidCompartmentGraph.h>
#include <biogears/cdm/properties/SEScalarAmountPerVolume.h>
#include <biogears/cdm/properties/SEScalarFlowResistance.h>
#include <biogears/cdm/properties/SEScalarFraction.h>
#include <biogears/cdm/properties/SEScalarMassPerTime.h>
#include <biogears/cdm/properties/SEScalarMassPerVolume.h>
#include <biogears/cdm/properties/SEScalarVolumePerTimePressureArea.h>
#include <biogears/cdm/substance/SESubstanceFraction.h>
#include <biogears/cdm/system/physiology/SEBloodChemistrySystem.h>
#include <biogears/engine/Systems/Renal.h>
#include <biogears/engine/test/BioGearsEngineTest.h>
#include <biogears/schema/cdm/Properties.hxx>

namespace biogears {
void BioGearsEngineTest::RenalCircuitAndTransportTest(const std::string& sTestDirectory)
{
  TimingProfile tmr;
  tmr.Start("Test");
  //Output files
  DataTrack circiutTrk;
  std::ofstream circuitFile;
  DataTrack graphTrk;
  std::ofstream graphFile;

  BioGears bg(sTestDirectory + "/RenalCircuitAndTransportTest.log");
  bg.GetPatient().Load("./patients/StandardMale.xml");
  bg.SetupPatient();
  bg.m_Config->EnableRenal(CDM::enumOnOff::On);
  bg.m_Config->EnableTissue(CDM::enumOnOff::Off);
  bg.CreateCircuitsAndCompartments();
  // Renal needs these tissue compartments
  // Let's make them manually, without the tissue circuit
  bg.GetCompartments().CreateTissueCompartment(BGE::TissueCompartment::Kidneys);
  bg.GetCompartments().CreateLiquidCompartment(BGE::ExtravascularCompartment::KidneysExtracellular);
  bg.GetCompartments().CreateLiquidCompartment(BGE::ExtravascularCompartment::KidneysExtracellular);

  bg.m_Config->EnableTissue(CDM::enumOnOff::On); // This needs to be on for making the tissue to extravascular mapping
  bg.GetCompartments().StateChange();
  //Add just N2
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetN2());
  SEScalarMassPerVolume N2_ug_per_mL;
  N2_ug_per_mL.SetValue(0.5, MassPerVolumeUnit::ug_Per_mL);
  bg.GetSubstances().SetSubstanceConcentration(bg.GetSubstances().GetN2(), bg.GetCompartments().GetUrineLeafCompartments(), N2_ug_per_mL);
  bg.GetSubstances().SetSubstanceConcentration(bg.GetSubstances().GetN2(), bg.GetCompartments().GetVascularLeafCompartments(), N2_ug_per_mL);

  //Get the renal stuff
  SEFluidCircuit& rCircuit = bg.GetCircuits().GetRenalCircuit();
  SELiquidCompartmentGraph& rGraph = bg.GetCompartments().GetRenalGraph();

  SEFluidCircuitNode* Ground = rCircuit.GetNode(BGE::RenalNode::Ground);

  SEFluidCircuitNode* RenalArtery = rCircuit.GetNode(BGE::RenalNode::RenalArtery);;
  SEFluidCircuitNode* RenalVein = rCircuit.GetNode(BGE::RenalNode::RenalVein);

  SELiquidCompartment* cRenalArtery = bg.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::RenalArtery);

  //Set up the N2 source to keep a constant concentrations to supply the system
  RenalArtery->GetVolumeBaseline().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::mL);
  cRenalArtery->GetVolume().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::mL);
  cRenalArtery->GetSubstanceQuantity(bg.GetSubstances().GetN2())->GetMass().SetValue(std::numeric_limits<double>::infinity(), MassUnit::mg);
  cRenalArtery->Balance(BalanceLiquidBy::Concentration);

  SEFluidCircuitPath& AortaSourcePath = rCircuit.CreatePath(*Ground, *RenalArtery, "AortaSource");
  SEFluidCircuitPath& VenaCavaSourcePath = rCircuit.CreatePath(*Ground, *RenalVein, "VenaCavaSource");

  //set baseline values, if it's anything other than 0, the sinusoid will not be used and the circuit will just be driven with this static pressure, for debugging or something
  double aortaPressure_mmHg = 0.0;
  AortaSourcePath.GetPressureSourceBaseline().SetValue(aortaPressure_mmHg, PressureUnit::mmHg);
  AortaSourcePath.GetNextPressureSource().SetValue(aortaPressure_mmHg, PressureUnit::mmHg);

  VenaCavaSourcePath.GetPressureSourceBaseline().SetValue(4.0, PressureUnit::mmHg); // Set to give cv equivalent pressure
  VenaCavaSourcePath.GetNextPressureSource().SetValue(4.0, PressureUnit::mmHg); // Set to give cv equivalent pressure

  rCircuit.SetNextAndCurrentFromBaselines();
  rCircuit.StateChange();

  //Execution parameters
  double time_s = 0;
  double deltaT_s = 1.0 / 90.0;
  double runTime_min = 10.0;
  //Drive waveform parameters
  double period = 1.0;
  double alpha = (2 * M_PI) / (period);
  double driverPressure_mmHg = 0.0;
  double amplitude_cmH2O = 6.0;
  double yOffset = 75.0;

  SELiquidTransporter txpt(VolumePerTimeUnit::mL_Per_s, VolumeUnit::mL, MassUnit::ug, MassPerVolumeUnit::ug_Per_mL, bg.GetLogger());
  SEFluidCircuitCalculator calc(FlowComplianceUnit::mL_Per_mmHg, VolumePerTimeUnit::mL_Per_s, FlowInertanceUnit::mmHg_s2_Per_mL, PressureUnit::mmHg, VolumeUnit::mL, FlowResistanceUnit::mmHg_s_Per_mL, bg.GetLogger());
  for (unsigned int i = 0; i < runTime_min * 60.0 / deltaT_s; i++) {
    // Drive the circuit
    driverPressure_mmHg = yOffset + amplitude_cmH2O * sin(alpha * time_s); //compute new pressure
    AortaSourcePath.GetNextPressureSource().SetValue(driverPressure_mmHg, PressureUnit::mmHg);

    //Process - Execute the circuit
    calc.Process(rCircuit, deltaT_s);
    //There is a compliance on the renal arteries, so the infinite volume is getting overwritten
    //Just keep it infinite
    RenalArtery->GetNextVolume().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::mL);
    //Execute the substance transport function
    txpt.Transport(rGraph, deltaT_s);
    //convert 'Next' values to current
    calc.PostProcess(rCircuit);

    circiutTrk.Track(time_s, rCircuit);
    graphTrk.Track(time_s, rGraph);
    time_s += deltaT_s;

    if (i == 0) {
      graphTrk.CreateFile(std::string(sTestDirectory + "/RenalTransportOutput.csv").c_str(), graphFile);
      circiutTrk.CreateFile(std::string(sTestDirectory + "/RenalCircuitOutput.csv").c_str(), circuitFile);
    }
    graphTrk.StreamTrackToFile(graphFile);
    circiutTrk.StreamTrackToFile(circuitFile);
  }
  graphFile.close();
  circuitFile.close();
  std::stringstream ss;
  ss << "It took " << tmr.GetElapsedTime_s("Test") << "s to run";
  bg.GetLogger()->Info(ss.str(), "RenalCircuitAndTransportTest");
}

// runs renal system at constant MAP to test TGF feedback function
// Resistance values at 80 & 180 mmHg are used for the resistance bounds in BioGears Circuits
void BioGearsEngineTest::RenalFeedbackTest(RenalFeedback feedback, const std::string& sTestDirectory, const std::string& sTestName)
{
  TimingProfile tmr;
  tmr.Start("Test");
  BioGears bg(sTestDirectory + "/RenalFeedbackTest.log");
  bg.GetPatient().Load("./patients/StandardMale.xml");
  bg.SetupPatient();
  bg.m_Config->EnableRenal(CDM::enumOnOff::On);
  bg.m_Config->EnableTissue(CDM::enumOnOff::Off);
  bg.CreateCircuitsAndCompartments();
  // Renal needs these tissue compartments
  // Let's make them manually, without the tissue circuit
  bg.GetCompartments().CreateTissueCompartment(BGE::TissueCompartment::Kidneys);
  bg.GetCompartments().CreateLiquidCompartment(BGE::ExtravascularCompartment::KidneysExtracellular);
  bg.GetCompartments().CreateLiquidCompartment(BGE::ExtravascularCompartment::KidneysExtracellular);
  bg.m_Config->EnableTissue(CDM::enumOnOff::On); // This needs to be on for making the tissue to extravascular mapping
  bg.GetCompartments().StateChange();
  SEPatient* patient = (SEPatient*)&bg.GetPatient();

  SEFluidCircuit& rCircuit = bg.GetCircuits().GetRenalCircuit();
  SELiquidCompartmentGraph& rGraph = bg.GetCompartments().GetRenalGraph();

  // Renal needs these present for Gluconeogenesis
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetLactate());
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetGlucose());
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetPotassium());
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetUrea());
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetSodium());

  SEFluidCircuitNode* Ground = rCircuit.GetNode(BGE::RenalNode::Ground);
  SEFluidCircuitNode* RenalArtery = rCircuit.GetNode(BGE::RenalNode::RenalArtery);
  SEFluidCircuitNode* RenalVein = rCircuit.GetNode(BGE::RenalNode::RenalVein);

  SELiquidCompartment* cRenalArtery = bg.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::RenalArtery);

  bg.GetSubstances().GetSodium().GetBloodConcentration().SetValue(bg.GetConfiguration().GetPlasmaSodiumConcentrationSetPoint(MassPerVolumeUnit::g_Per_L), MassPerVolumeUnit::g_Per_L);
  bg.GetSubstances().SetSubstanceConcentration(bg.GetSubstances().GetSodium(), bg.GetCompartments().GetUrineLeafCompartments(), bg.GetSubstances().GetSodium().GetBloodConcentration());
  bg.GetSubstances().SetSubstanceConcentration(bg.GetSubstances().GetSodium(), bg.GetCompartments().GetVascularLeafCompartments(), bg.GetSubstances().GetSodium().GetBloodConcentration());

  //Set up the sodium concentration on the source to keep a constant concentrations to supply the system
  RenalArtery->GetVolumeBaseline().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::mL);
  RenalArtery->GetNextVolume().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::mL);
  cRenalArtery->GetVolume().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::mL);
  cRenalArtery->GetSubstanceQuantity(bg.GetSubstances().GetSodium())->GetConcentration().SetValue(4.5, MassPerVolumeUnit::g_Per_L); //tubules sodium concentration in engine
  cRenalArtery->Balance(BalanceLiquidBy::Concentration);

  SEFluidCircuitPath& AortaSourcePath = rCircuit.CreatePath(*Ground, *RenalArtery, "AortaSource");
  SEFluidCircuitPath& VenaCavaSourcePath = rCircuit.CreatePath(*Ground, *RenalVein, "VenaCavaSource");

  SEFluidCircuitPath* AfferentArterioleToGlomerularCapillaries = rCircuit.GetPath(BGE::RenalPath::AfferentArterioleToGlomerularCapillaries);
  SEFluidCircuitPath* GlomerularCapillariesToNetGlomerularCapillaries = rCircuit.GetPath(BGE::RenalPath::GlomerularCapillariesToNetGlomerularCapillaries);
  SEFluidCircuitPath* BowmansCapsulesToNetBowmansCapsules = rCircuit.GetPath(BGE::RenalPath::BowmansCapsulesToNetBowmansCapsules);
  SEFluidCircuitPath* TubulesToNetTubules = rCircuit.GetPath(BGE::RenalPath::TubulesToNetTubules);
  SEFluidCircuitPath* NetTubulesToNetPeritubularCapillaries = rCircuit.GetPath(BGE::RenalPath::NetTubulesToNetPeritubularCapillaries);
  SEFluidCircuitPath* PeritubularCapillariesToNetPeritubularCapillaries = rCircuit.GetPath(BGE::RenalPath::PeritubularCapillariesToNetPeritubularCapillaries);


  SELiquidTransporter txpt(VolumePerTimeUnit::mL_Per_s, VolumeUnit::mL, MassUnit::ug, MassPerVolumeUnit::ug_Per_mL, bg.GetLogger());
  SEFluidCircuitCalculator calc(FlowComplianceUnit::mL_Per_mmHg, VolumePerTimeUnit::mL_Per_s, FlowInertanceUnit::mmHg_s2_Per_mL, PressureUnit::mmHg, VolumeUnit::mL, FlowResistanceUnit::mmHg_s_Per_mL, bg.GetLogger());

  DataTrack trk;
  double time_s = 0.0;
  double deltaT_s = 1.0 / 90.0;
  double aortaPressure_mmHg = 100.0;
  double venaCavaPressure_mmHg = 4.0;
  double maxSteadyCycles = 1.0 / deltaT_s * 30.0; // must be steady for 30 second
  double convergencePercentage = 0.01; // within 1%
  double AffResistance_mmHg_Per_mL_Per_s = 0.0;
  //fit parameters for upr curve:
  double a = 2.9e-4;
  double b = -0.017;
  double c = 0.219;
  SEScalarTime eventTime;
  eventTime.SetValue(0, TimeUnit::s);

  //initialize pressure
  AortaSourcePath.GetPressureSourceBaseline().SetValue(aortaPressure_mmHg, PressureUnit::mmHg);
  VenaCavaSourcePath.GetPressureSourceBaseline().SetValue(venaCavaPressure_mmHg, PressureUnit::mmHg);

  // Simple system setup
  Renal& bgRenal = (Renal&)bg.GetRenal();
  bgRenal.Initialize();
  // Renal needs this
  bg.GetBloodChemistry().GetHematocrit().SetValue(0.45817);

  //Update the circuit
  rCircuit.SetNextAndCurrentFromBaselines();
  rCircuit.StateChange();
  calc.Process(rCircuit, deltaT_s); //Preprocess wants a circuit full of data, need to calc it once

  //This can't think it doing resting stabilization, or it will just keep overwriting the TGF setpoint
  bg.m_State = EngineState::Active;

  //Do it every 10 mmHg between 80 and 200
  for (double MAP = 70.0; MAP <= 200.0; MAP += 10.0) {
    std::cout << "MAP = " << MAP << std::endl;

    double AffResistance_mmHg_s_Per_mL = 0.0;
    double ReabsorptionResistance_mmHg_s_Per_mL = 0.0;
    double ReabsorptionFlow_mL_Per_min = 0.0;
    //Loop until the GFR and RBF are steady
    double steadyGFR_L_Per_min = 0.0;
    double steadyRBF_L_Per_min = 0.0;
    double steadyUPR_mL_Per_min = 0.0;
    double currentGFR_L_Per_min = 0.0;
    double currentRBF_L_Per_min = 0.0;
    double currentUPR_mL_Per_min = 0.0;
    double steadyCycles = 0.0;
    bool GFRSteady = false;
    bool RBFSteady = false;
    bool UPRSteady = false;
    bool convergedCheck = false;
    //validation data of urine production:
    double urineValidation = (a * std::pow(MAP, 2) + b * MAP + c);
    //get the permeability and resistance for output:
    double permeabilityCurrent_mL_Per_s_Per_mmHg_Per_m2 = 0.0;
    double permeabilitySteady_mL_Per_s_Per_mmHg_Per_m2 = 0.0;

    // Stabilize the circuit
    for (unsigned int i = 0; i < 3e6; i++) {
      //Flag beginning of cardiac cycle - this will make it just use the current value instead of a running average
      patient->SetEvent(CDM::enumPatientEvent::StartOfCardiacCycle, true, eventTime);

      GFRSteady = false;
      RBFSteady = false;
      UPRSteady = false;

      //Set the MAP as a static value
      AortaSourcePath.GetNextPressureSource().SetValue(MAP, PressureUnit::mmHg);

      //Do all normal preprocessing for feedback for all tests but the urine production curve
      bgRenal.PreProcess();

      //Don't do any Albumin feedback by overwriting
      GlomerularCapillariesToNetGlomerularCapillaries->GetNextPressureSource().Set(GlomerularCapillariesToNetGlomerularCapillaries->GetPressureSourceBaseline());
      BowmansCapsulesToNetBowmansCapsules->GetNextPressureSource().Set(BowmansCapsulesToNetBowmansCapsules->GetPressureSourceBaseline());
      TubulesToNetTubules->GetNextPressureSource().Set(TubulesToNetTubules->GetPressureSourceBaseline());
      PeritubularCapillariesToNetPeritubularCapillaries->GetNextPressureSource().Set(PeritubularCapillariesToNetPeritubularCapillaries->GetPressureSourceBaseline());

      

      //Overwrite unwanted feedback
      switch (feedback) {
      case TGF: {
        //Turn off UPR feedback to just test tubuloglomerular feedback methodology
        NetTubulesToNetPeritubularCapillaries->GetNextResistance().Set(NetTubulesToNetPeritubularCapillaries->GetResistance());
        break;
      }
      case TGFandUPR: {
        break;
      }
      }

      //These are normally calculated in the cardiovascular system as part of the combined circulatory circuit
      calc.Process(rCircuit, deltaT_s);
      //We only care about sodium for this test
      txpt.Transport(rGraph, deltaT_s);
      //Do the normal processing to do active transport
      bgRenal.Process();

      //Get flows and resistances to output and test:
      currentGFR_L_Per_min = bgRenal.GetGlomerularFiltrationRate(VolumePerTimeUnit::L_Per_min);
      currentRBF_L_Per_min = bgRenal.GetRenalBloodFlow(VolumePerTimeUnit::L_Per_min);
      currentUPR_mL_Per_min = bgRenal.GetUrineProductionRate(VolumePerTimeUnit::mL_Per_min);
      ReabsorptionFlow_mL_Per_min = NetTubulesToNetPeritubularCapillaries->GetNextFlow(VolumePerTimeUnit::mL_Per_min);
      AffResistance_mmHg_s_Per_mL = AfferentArterioleToGlomerularCapillaries->GetNextResistance(FlowResistanceUnit::mmHg_s_Per_mL);
      ReabsorptionResistance_mmHg_s_Per_mL = NetTubulesToNetPeritubularCapillaries->GetNextResistance(FlowResistanceUnit::mmHg_s_Per_mL);
      permeabilityCurrent_mL_Per_s_Per_mmHg_Per_m2 = bgRenal.GetTubularReabsorptionFluidPermeability().GetValue(VolumePerTimePressureAreaUnit::mL_Per_s_mmHg_m2);

      //These is normally calculated in the cardiovascular system as part of the combined circulatory circuit
      calc.PostProcess(rCircuit);

      //steady conditions on the flows:
      if (feedback == TGF) {
        steadyGFR_L_Per_min = currentGFR_L_Per_min;
        steadyRBF_L_Per_min = currentRBF_L_Per_min;
        steadyUPR_mL_Per_min = currentUPR_mL_Per_min;
      }

      // all must be steady to satisfy criteria
      if (currentGFR_L_Per_min <= (steadyGFR_L_Per_min * (1 + convergencePercentage)) && currentGFR_L_Per_min >= (steadyGFR_L_Per_min * (1 - convergencePercentage)))
        GFRSteady = true;
      else
        steadyGFR_L_Per_min = currentGFR_L_Per_min;

      if (currentRBF_L_Per_min <= (steadyRBF_L_Per_min * (1 + convergencePercentage)) && currentRBF_L_Per_min >= (steadyRBF_L_Per_min * (1 - convergencePercentage)))
        RBFSteady = true;
      else
        steadyRBF_L_Per_min = currentRBF_L_Per_min;

      //set upr to steady to mitigate low map fluctuations around zero
      UPRSteady = true;
      steadyUPR_mL_Per_min = currentUPR_mL_Per_min;

      permeabilitySteady_mL_Per_s_Per_mmHg_Per_m2 = permeabilityCurrent_mL_Per_s_Per_mmHg_Per_m2;

      if (GFRSteady && RBFSteady && UPRSteady)
        steadyCycles += 1;
      else
        steadyCycles = 0;

      if (steadyCycles == maxSteadyCycles) {
        convergedCheck = true;
        break;
      }
    }

    //Output the data
    if (convergedCheck) {
      trk.Track("MeanArterialPressure(mmHg)", time_s, MAP);
      trk.Track("GlomerularFiltrationRate(L/min)", time_s, steadyGFR_L_Per_min);
      trk.Track("RenalBloodFlow(L/min)", time_s, steadyRBF_L_Per_min);
      trk.Track("UrineProductionRate(mL/min)", time_s, steadyUPR_mL_Per_min);
      trk.Track("ReabsorptionFlow(mL/min)", time_s, ReabsorptionFlow_mL_Per_min);
      trk.Track("AfferentResistance(mmHg-s/mL)", time_s, AffResistance_mmHg_s_Per_mL);
      trk.Track("ReabsorptionResistance(mmHg-s/mL)", time_s,ReabsorptionResistance_mmHg_s_Per_mL);
      trk.Track("TubulesPermeability_mL_Per_s_Per_mmHg_Per_m2", time_s, permeabilitySteady_mL_Per_s_Per_mmHg_Per_m2);
    } else {
      std::cerr << "Could not converge flows, check criteria" << std::endl;
      return;
    }
    time_s += 1.0;
    switch (feedback) {
    case TGF: {
      std::cout << "GFR: " << currentGFR_L_Per_min << std::endl;
      std::cout << "RBF: " << currentRBF_L_Per_min << std::endl;
      break;
    }
    case TGFandUPR: {
      std::cout << "UPR" << steadyUPR_mL_Per_min << std::endl;
      std::cout << "GFR: " << currentGFR_L_Per_min << std::endl;
      std::cout << "RBF: " << currentRBF_L_Per_min << std::endl;
      break;
    }
    }
  }
  trk.WriteTrackToFile(std::string(sTestDirectory + "/" + sTestName + ".csv").c_str());
  std::stringstream ss;
  ss << "It took " << tmr.GetElapsedTime_s("Test") << "s to run " << sTestName << "CircuitAndTransportTest";
  bg.GetLogger()->Info(ss.str(), "RenalFeedbackTest");
}

void BioGearsEngineTest::RenalTGFFeedbackTest(const std::string& sTestDirectory)
{
  RenalFeedbackTest(TGF, sTestDirectory, "RenalTGFFeedbackOutput");
}
void BioGearsEngineTest::RenalTGFandUPRFeedbackTest(const std::string& sTestDirectory)
{
  RenalFeedbackTest(TGFandUPR, sTestDirectory, "RenalTGFandUPRFeedbackOutput");
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// checks secretion function and osmolarity/lality calculations as a function of substance quantities
///
/// \details
/// increment postassium values above average blood concentration levels and let run for a number of
/// seconds. After stabilization time has run compute osmolarity/lality
/// output potassium blood concentration levels and osmolarity/lality, check against data.
//--------------------------------------------------------------------------------------------------
void BioGearsEngineTest::RenalSystemTest(RenalSystems systemtest, const std::string& sTestDirectory, const std::string& sTestName)
{

  TimingProfile tmr;
  tmr.Start("Test");
  BioGears bg(sTestDirectory + "/RenalSystemTest.log");
  bg.GetPatient().Load("./patients/StandardMale.xml");
  bg.SetupPatient();
  bg.m_Config->EnableRenal(CDM::enumOnOff::On);
  bg.m_Config->EnableTissue(CDM::enumOnOff::Off);
  bg.CreateCircuitsAndCompartments();
  // Renal needs these tissue compartments
  // Let's make them manually, without the tissue circuit
  bg.GetCompartments().CreateTissueCompartment(BGE::TissueCompartment::Kidneys);
  bg.GetCompartments().CreateLiquidCompartment(BGE::ExtravascularCompartment::KidneysExtracellular);
  bg.GetCompartments().CreateLiquidCompartment(BGE::ExtravascularCompartment::KidneysExtracellular);
  bg.m_Config->EnableTissue(CDM::enumOnOff::On); // This needs to be on for making the tissue to extravascular mapping
  bg.GetCompartments().StateChange();
  SEPatient* patient = (SEPatient*)&bg.GetPatient();
  SESubstance& potassium = bg.GetSubstances().GetPotassium();

  // Renal needs these present for Gluconeogenesis
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetPotassium());
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetSodium());
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetLactate());
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetGlucose());
  bg.GetSubstances().AddActiveSubstance(bg.GetSubstances().GetUrea());

  // Removing const in order to fill out and test
  //SERenalSystem &RenalSystem = bg.GetRenal();
  Renal& bgRenal = (Renal&)bg.GetRenal();
  bgRenal.Initialize();

  // VIPs only
  SEFluidCircuit& RenalCircuit = bg.GetCircuits().GetRenalCircuit();
  SELiquidCompartmentGraph& rGraph = bg.GetCompartments().GetRenalGraph();

  //Initialize potassium to baseline:
  double baselinePotassiumConcentration_g_Per_dl = 0.0185;
  SEScalarMassPerVolume K_g_Per_dL;
  K_g_Per_dL.SetValue(baselinePotassiumConcentration_g_Per_dl, MassPerVolumeUnit::g_Per_dL); //set to normal concentration values
  bg.GetSubstances().SetSubstanceConcentration(bg.GetSubstances().GetPotassium(), bg.GetCompartments().GetUrineLeafCompartments(), K_g_Per_dL);
  bg.GetSubstances().SetSubstanceConcentration(bg.GetSubstances().GetPotassium(), bg.GetCompartments().GetVascularLeafCompartments(), K_g_Per_dL);

  //Initialize sodium
  SEScalarMassPerVolume Na_g_Per_dL;
  Na_g_Per_dL.SetValue(bg.GetConfiguration().GetPlasmaSodiumConcentrationSetPoint(MassPerVolumeUnit::g_Per_dL), MassPerVolumeUnit::g_Per_dL);
  bg.GetSubstances().SetSubstanceConcentration(bg.GetSubstances().GetSodium(), bg.GetCompartments().GetUrineLeafCompartments(), Na_g_Per_dL);
  bg.GetSubstances().SetSubstanceConcentration(bg.GetSubstances().GetSodium(), bg.GetCompartments().GetVascularLeafCompartments(), Na_g_Per_dL);

  //Initialize the things in BloodChemistry Renal needs
  bg.GetBloodChemistry().GetHematocrit().SetValue(0.45817);
  bg.GetSubstances().GetSodium().GetBloodConcentration().SetValue(bg.GetConfiguration().GetPlasmaSodiumConcentrationSetPoint(MassPerVolumeUnit::g_Per_L), MassPerVolumeUnit::g_Per_L);

  //Renal nodes
  SEFluidCircuitNode* ReferenceNode = RenalCircuit.GetNode(BGE::RenalNode::Ground);
  SEFluidCircuitNode* RenalArteryNode = RenalCircuit.GetNode(BGE::RenalNode::RenalArtery);
  SEFluidCircuitNode* RenalVenaCavaConnectionNode = RenalCircuit.GetNode(BGE::RenalNode::VenaCavaConnection);
  SEFluidCircuitNode* UreterNode = RenalCircuit.GetNode(BGE::RenalNode::Ureter);
  SEFluidCircuitNode* Bladder = RenalCircuit.GetNode(BGE::RenalNode::Bladder);
  SEFluidCircuitNode* PeritubularCapillariesNode = RenalCircuit.GetNode(BGE::RenalNode::PeritubularCapillaries);

  //Renal/vascular compartments
  SELiquidCompartment* BladderCompartment = bg.GetCompartments().GetLiquidCompartment(BGE::UrineCompartment::Bladder);
  SELiquidCompartment* UreterCompartment = bg.GetCompartments().GetLiquidCompartment(BGE::UrineCompartment::Ureter);
  SELiquidCompartment* ArteryCompartment = bg.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::RenalArtery);
  SELiquidCompartment* AfferentArterioleCompartment = bg.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::AfferentArteriole);
  SELiquidCompartment* PeritubularCapillariesCompartment = bg.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::PeritubularCapillaries);

  //Renal paths
  SEFluidCircuitPath& AortaSourcePath = RenalCircuit.CreatePath(*ReferenceNode, *RenalArteryNode, "AortaSourcePath");
  SEFluidCircuitPath& VenaCavaSourcePath = RenalCircuit.CreatePath(*ReferenceNode, *RenalVenaCavaConnectionNode, "VenaCavaSourcePath");

  SEFluidCircuitCalculator calc(bg.GetLogger());
  SELiquidTransporter txpt(VolumePerTimeUnit::mL_Per_s, VolumeUnit::mL, MassUnit::ug, MassPerVolumeUnit::ug_Per_mL, bg.GetLogger());

  double deltaT_s = 1.0 / 90.0;
  double time_s = 0.0;

  DataTrack trk;

  double initialPotassiumConcentration_g_Per_dL = 0.0;
  double aortaPressure_mmHg = 100.0;
  double venaCavaPressure_mmHg = 4.0;
  //double convergencePercentage = 0.01; // within 1%
  double percentIncrease = 0.0; //increment potassium concentration
  double peritubularCapillariesPotassium_g_Per_dL = 0.0;
  double bladderPotassium_g_Per_dL = 0.0;
  double ureterPotassium_g_Per_dL = 0.0;
  double runTime_min = 5.0; //run system for 5000 iterations before checking data
  int halftime = (int)(runTime_min * 60 / deltaT_s * 0.5);

  //initialize pressure on sources:
  AortaSourcePath.GetPressureSourceBaseline().SetValue(aortaPressure_mmHg, PressureUnit::mmHg);
  VenaCavaSourcePath.GetPressureSourceBaseline().SetValue(venaCavaPressure_mmHg, PressureUnit::mmHg);
  AortaSourcePath.GetNextPressureSource().SetValue(aortaPressure_mmHg, PressureUnit::mmHg);
  VenaCavaSourcePath.GetNextPressureSource().SetValue(venaCavaPressure_mmHg, PressureUnit::mmHg);

  //Update the circuit
  RenalCircuit.SetNextAndCurrentFromBaselines();
  RenalCircuit.StateChange();
  calc.Process(RenalCircuit, deltaT_s); //Preprocess wants a circuit full of data, need to calc it once

  SEScalarTime time;
  time.SetValue(0, TimeUnit::s);

  //initialize concentrations to zero:
  double currentPotassium_g_dl = 0.0;

  switch (systemtest) {
  case Urinating: {
    break;
  }
  case Secretion: {
    percentIncrease = 0.1; //10 percent increase

    //adjust initial potassium concentrations:
    initialPotassiumConcentration_g_Per_dL = baselinePotassiumConcentration_g_Per_dl * (1 + percentIncrease);

    //set concentrations
    ArteryCompartment->GetSubstanceQuantity(bg.GetSubstances().GetPotassium())->GetConcentration().SetValue(initialPotassiumConcentration_g_Per_dL, MassPerVolumeUnit::g_Per_dL);
    ArteryCompartment->Balance(BalanceLiquidBy::Concentration);
    break;
  }
  }

  //begin renal process:
  for (int j = 0; j < runTime_min * 60.0 / deltaT_s; j++) {
    //Do all normal preprocessing for feedback for all tests but the urine production curve
    bgRenal.PreProcess();

    //call urinate at half simulation time:
    if (j == halftime) {
      switch (systemtest) {
      case Urinating: {
        bg.GetActions().GetPatientActions().HasUrinate();
        bgRenal.Urinate();
        break;
      }
      case Secretion: {
        break;
      }
      }
    }

    //These are normally calculated in the cardiovascular system as part of the combined circulatory circuit
    calc.Process(RenalCircuit, deltaT_s);

    txpt.Transport(rGraph, deltaT_s); //not sure if this is the same as the above commented out line

    bgRenal.Process();

    //These is normally calculated in the cardiovascular system as part of the combined circulatory circuit
    calc.PostProcess(RenalCircuit);

    //data call:
    peritubularCapillariesPotassium_g_Per_dL = PeritubularCapillariesCompartment->GetSubstanceQuantity(potassium)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_dL);
    currentPotassium_g_dl = ArteryCompartment->GetSubstanceQuantity(potassium)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_dL);
    bladderPotassium_g_Per_dL = BladderCompartment->GetSubstanceQuantity(potassium)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_dL);
    ureterPotassium_g_Per_dL = UreterCompartment->GetSubstanceQuantity(potassium)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_dL);

    time_s += deltaT_s;

    switch (systemtest) {
    case Urinating: {
      trk.Track(time_s, rGraph);
      break;
    }
    case Secretion: {
      //track the potassium value in the peritubular capillaries:
      trk.Track("PeritubularPotassium(g/dl)", time_s, peritubularCapillariesPotassium_g_Per_dL);
      trk.Track("RenalArteryPotassium(g/dl)", time_s, currentPotassium_g_dl);
      trk.Track("BladderPotassium(g/dl)", time_s, bladderPotassium_g_Per_dL);
      trk.Track("UreterPotassium(g/dl)", time_s, ureterPotassium_g_Per_dL);
      break;
    }
    }
  }
  trk.WriteTrackToFile(std::string(sTestDirectory + "/" + sTestName + ".csv").c_str());
  std::stringstream ss;
  ss << "It took " << tmr.GetElapsedTime_s("Test") << "s to run " << sTestName << "SecretionandUrinatingTest";
  bg.GetLogger()->Info(ss.str(), "RenalSystemTest");
}

void BioGearsEngineTest::RenalSecretionTest(const std::string& sTestDirectory)
{
  RenalSystemTest(Secretion, sTestDirectory, "RenalSecretionOutput");
}
void BioGearsEngineTest::RenalUrinateTest(const std::string& sTestDirectory)
{
  RenalSystemTest(Urinating, sTestDirectory, "RenalUrinateOutput");
}
}