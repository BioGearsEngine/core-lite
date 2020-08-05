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

#include <biogears/engine/Systems/Tissue.h>
#include <biogears/engine/test/BioGearsEngineTest.h>

#include <biogears/cdm/Serializer.h>
#include <biogears/cdm/compartment/SECompartmentManager.h>
#include <biogears/cdm/compartment/fluid/SEGasCompartment.h>
#include <biogears/cdm/compartment/tissue/SETissueCompartment.h>
#include <biogears/cdm/properties/SEScalarAmountPerVolume.h>
#include <biogears/cdm/properties/SEScalarFraction.h>
#include <biogears/cdm/properties/SEScalarMass.h>
#include <biogears/cdm/properties/SEScalarMassPerAmount.h>
#include <biogears/cdm/properties/SEScalarMassPerAreaTime.h>
#include <biogears/cdm/properties/SEScalarMassPerVolume.h>
#include <biogears/cdm/substance/SESubstance.h>
#include <biogears/cdm/substance/SESubstanceManager.h>
#include <biogears/cdm/utils/TimingProfile.h>
#include <biogears/cdm/utils/testing/SETestCase.h>
#include <biogears/cdm/utils/testing/SETestReport.h>
#include <biogears/cdm/utils/testing/SETestSuite.h>

#include <Eigen/Core>
#include <biogears/cdm/utils/DataTrack.h>
#include <biogears/chrono/stop_watch.tci.h>

namespace biogears {
void BioGearsEngineTest::DistributeMass(SETestSuite& testSuite)
{
  TimingProfile timer;
  BioGears bg(testSuite.GetLogger());
  Tissue& tsu = (Tissue&)bg.GetTissue();
  // First test case
  SETestCase& testCase = testSuite.CreateTestCase();
  testCase.SetName("DistributeMassToHierarchy");
  timer.Start("Test");

  //                L0C0
  //               /    \
  //           L1C0      L1C1
  //           /  \      /  \
  //        L2C0  L2C1 L2C2 L2C3 <-- Only these cmpts have data

  SELiquidCompartment& L0C0 = bg.GetCompartments().CreateLiquidCompartment("L0C0");
  SELiquidCompartment& L1C0 = bg.GetCompartments().CreateLiquidCompartment("L1C0");
  SELiquidCompartment& L1C1 = bg.GetCompartments().CreateLiquidCompartment("L1C1");
  SELiquidCompartment& L2C0 = bg.GetCompartments().CreateLiquidCompartment("L2C0");
  SELiquidCompartment& L2C1 = bg.GetCompartments().CreateLiquidCompartment("L2C1");
  SELiquidCompartment& L2C2 = bg.GetCompartments().CreateLiquidCompartment("L2C2");
  SELiquidCompartment& L2C3 = bg.GetCompartments().CreateLiquidCompartment("L2C3");

  // Build up the hierarchy
  L0C0.AddChild(L1C0);
  L0C0.AddChild(L1C1);
  L1C0.AddChild(L2C0);
  L1C0.AddChild(L2C1);
  L1C1.AddChild(L2C2);
  L1C1.AddChild(L2C3);
  bg.GetCompartments().StateChange(); // Call this, AFTER YOU SET UP YOUR HIERARCHY, to ensure all parent compartments have their link data

  SESubstance* sub = bg.GetSubstances().GetSubstance("Oxygen");
  bg.GetSubstances().AddActiveSubstance(*sub);

  double L2C0_mL = 10;
  double L2C0_g = 10;
  double L2C1_mL = 20;
  double L2C1_g = 20;
  double L2C2_mL = 40;
  double L2C2_g = 40;
  double L2C3_mL = 80;
  double L2C3_g = 80;
  double total_mL = L2C0_mL + L2C1_mL + L2C2_mL + L2C3_mL;

  L2C0.GetVolume().SetValue(L2C0_mL, VolumeUnit::mL);
  L2C0.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C0_g, MassUnit::g);
  L2C1.GetVolume().SetValue(L2C1_mL, VolumeUnit::mL);
  L2C1.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C1_g, MassUnit::g);
  L2C2.GetVolume().SetValue(L2C2_mL, VolumeUnit::mL);
  L2C2.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C2_g, MassUnit::g);
  L2C3.GetVolume().SetValue(L2C3_mL, VolumeUnit::mL);
  L2C3.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C3_g, MassUnit::g);

  double delta_g = -20;

  double expected_L2C0_g = L2C0_g + (delta_g * L2C0_mL / total_mL);
  double expected_L2C1_g = L2C1_g + (delta_g * L2C1_mL / total_mL);
  double expected_L2C2_g = L2C2_g + (delta_g * L2C2_mL / total_mL);
  double expected_L2C3_g = L2C3_g + (delta_g * L2C3_mL / total_mL);

  tsu.DistributeMassbyVolumeWeighted(L0C0, *sub, delta_g, MassUnit::g);

  double L2C0_calc = L2C0.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C0_calc - expected_L2C0_g) > ZERO_APPROX)
    testCase.AddFailure("DistributeMassbyVolumeWeighted L2C0 mass is not expected mass");

  double L2C1_calc = L2C1.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C1_calc - expected_L2C1_g) > ZERO_APPROX)
    testCase.AddFailure("DistributeMassbyVolumeWeighted L2C1 mass is not expected mass");

  double L2C2_calc = L2C2.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C2_calc - expected_L2C2_g) > ZERO_APPROX)
    testCase.AddFailure("DistributeMassbyVolumeWeighted L2C2 mass is not expected mass");

  double L2C3_calc = L2C3.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C3_calc - expected_L2C3_g) > ZERO_APPROX)
    testCase.AddFailure("DistributeMassbyVolumeWeighted L2C3 mass is not expected mass");

  // Log how long our test took
  testCase.GetDuration().SetValue(timer.GetElapsedTime_s("Test"), TimeUnit::s);

  // Second test case
  SETestCase& testCase2 = testSuite.CreateTestCase();
  testCase2.SetName("DistributeMassbyMassToHierarchy");
  timer.Start("Test");

  L2C0_mL = 10;
  L2C0_g = 10;
  L2C1_mL = 20;
  L2C1_g = 20;
  L2C2_mL = 40;
  L2C2_g = 40;
  L2C3_mL = 80;
  L2C3_g = 80;
  double total_g = L2C0_g + L2C1_g + L2C2_g + L2C3_g;

  L2C0.GetVolume().SetValue(L2C0_mL, VolumeUnit::mL);
  L2C0.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C0_g, MassUnit::g);
  L2C1.GetVolume().SetValue(L2C1_mL, VolumeUnit::mL);
  L2C1.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C1_g, MassUnit::g);
  L2C2.GetVolume().SetValue(L2C2_mL, VolumeUnit::mL);
  L2C2.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C2_g, MassUnit::g);
  L2C3.GetVolume().SetValue(L2C3_mL, VolumeUnit::mL);
  L2C3.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C3_g, MassUnit::g);

  delta_g = -20;

  expected_L2C0_g = L2C0_g + (delta_g * L2C0_g / total_g);
  expected_L2C1_g = L2C1_g + (delta_g * L2C1_g / total_g);
  expected_L2C2_g = L2C2_g + (delta_g * L2C2_g / total_g);
  expected_L2C3_g = L2C3_g + (delta_g * L2C3_g / total_g);

  tsu.DistributeMassbyMassWeighted(L0C0, *sub, delta_g, MassUnit::g);

  L2C0_calc = L2C0.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C0_calc - expected_L2C0_g) > ZERO_APPROX)
    testCase2.AddFailure("DistributeMassbyMassWeighted L2C0 mass is not expected mass");

  L2C1_calc = L2C1.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C1_calc - expected_L2C1_g) > ZERO_APPROX)
    testCase2.AddFailure("DistributeMassbyMassWeighted L2C1 mass is not expected mass");

  L2C2_calc = L2C2.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C2_calc - expected_L2C2_g) > ZERO_APPROX)
    testCase2.AddFailure("DistributeMassbyMassWeighted L2C2 mass is not expected mass");

  L2C3_calc = L2C3.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C3_calc - expected_L2C3_g) > ZERO_APPROX)
    testCase2.AddFailure("DistributeMassbyMassWeighted L2C3 mass is not expected mass");

  // Third test case
  // Second test case
  SETestCase& testCase3 = testSuite.CreateTestCase();
  testCase3.SetName("DistributeMassbyMassToHierarchy_tooMuch");
  timer.Start("Test");

  L2C0_mL = 10;
  L2C0_g = 10;
  L2C1_mL = 20;
  L2C1_g = 20;
  L2C2_mL = 40;
  L2C2_g = 40;
  L2C3_mL = 80;
  L2C3_g = 80;
  total_g = L2C0_g + L2C1_g + L2C2_g + L2C3_g;

  L2C0.GetVolume().SetValue(L2C0_mL, VolumeUnit::mL);
  L2C0.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C0_g, MassUnit::g);
  L2C1.GetVolume().SetValue(L2C1_mL, VolumeUnit::mL);
  L2C1.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C1_g, MassUnit::g);
  L2C2.GetVolume().SetValue(L2C2_mL, VolumeUnit::mL);
  L2C2.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C2_g, MassUnit::g);
  L2C3.GetVolume().SetValue(L2C3_mL, VolumeUnit::mL);
  L2C3.GetSubstanceQuantity(*sub)->GetMass().SetValue(L2C3_g, MassUnit::g);

  delta_g = -2000;

  expected_L2C0_g = 0.;
  expected_L2C1_g = 0.;
  expected_L2C2_g = 0.;
  expected_L2C3_g = 0.;

  tsu.DistributeMassbyMassWeighted(L0C0, *sub, delta_g, MassUnit::g);

  L2C0_calc = L2C0.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C0_calc - expected_L2C0_g) > ZERO_APPROX)
    testCase3.AddFailure("L2C0 mass is not expected mass - Expecting zero");

  L2C1_calc = L2C1.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C1_calc - expected_L2C1_g) > ZERO_APPROX)
    testCase3.AddFailure("L2C1 mass is not expected mass - Expecting zero");

  L2C2_calc = L2C2.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C2_calc - expected_L2C2_g) > ZERO_APPROX)
    testCase3.AddFailure("L2C2 mass is not expected mass - Expecting zero");

  L2C3_calc = L2C3.GetSubstanceQuantity(*sub)->GetMass(MassUnit::g);
  if (std::abs(L2C3_calc - expected_L2C3_g) > ZERO_APPROX)
    testCase3.AddFailure("L2C3 mass is not expected mass - Expecting zero");
}

void BioGearsEngineTest::PerfusionLimitedDiffusionTest(SETestSuite& testSuite)
{
  BioGears bg(testSuite.GetLogger());
  Tissue& tsu = (Tissue&)bg.GetTissue();
  TimingProfile timer;
  double timestep_s = 1. / 90.;
  SESubstance* sub = bg.GetSubstances().GetSubstance("Ketamine");
  double bFlow_mL_Per_s = 2.0;
  double PartitionCoeff = 1.52201;
  double matrixVolume_mL = 2.0;
  double tissueMass_ug = 3.;
  double bVol_mL = 10.;
  double bConc_ug_Per_mL = 10.;

  bg.GetSubstances().AddActiveSubstance(*sub);

  // First test case
  SETestCase& testCase1 = testSuite.CreateTestCase();
  testCase1.SetName("PerfusionLimitedDiffusionTest");
  timer.Start("Test");

  SETissueCompartment& tissue = bg.GetCompartments().CreateTissueCompartment(BGE::TissueCompartment::Bone);
  SELiquidCompartment& extracellular = bg.GetCompartments().CreateLiquidCompartment(BGE::ExtravascularCompartment::BoneExtracellular);
  SELiquidCompartment& intracellular = bg.GetCompartments().CreateLiquidCompartment(BGE::ExtravascularCompartment::BoneIntracellular);
  tissue.GetMatrixVolume().SetValue(matrixVolume_mL, VolumeUnit::mL);
  intracellular.GetSubstanceQuantity(*sub)->GetMass().SetValue(tissueMass_ug, MassUnit::ug);
  intracellular.GetVolume().SetValue(1.0, VolumeUnit::mL); //Need fluid volume to balance.
  extracellular.GetVolume().SetValue(1.0, VolumeUnit::mL); //Should be fine, right? We don't validate and tissue concentrations.

  SELiquidCompartment& vascular = bg.GetCompartments().CreateLiquidCompartment(BGE::VascularCompartment::Bone);
  vascular.GetVolume().SetValue(bVol_mL, VolumeUnit::mL);
  vascular.GetSubstanceQuantity(*sub)->GetConcentration().SetValue(bConc_ug_Per_mL, MassPerVolumeUnit::ug_Per_mL);
  vascular.GetSubstanceQuantity(*sub)->Balance(BalanceLiquidBy::Concentration);
  intracellular.GetSubstanceQuantity(*sub)->Balance(BalanceLiquidBy::Mass);

  SELiquidCompartmentLink& flow = bg.GetCompartments().CreateLiquidLink(intracellular, vascular, "ExtravascularExchange");
  flow.GetFlow().SetValue(bFlow_mL_Per_s, VolumePerTimeUnit::mL_Per_s);
  extracellular.AddLink(flow);
  vascular.AddLink(flow);

  bg.GetCompartments().StateChange();

  double rtnMassInc_ug = tsu.PerfusionLimitedDiffusion(tissue, vascular, *sub, PartitionCoeff, timestep_s);

  // Check
  double MassIncrement_ug = bFlow_mL_Per_s * timestep_s * ((bConc_ug_Per_mL) - ((tissueMass_ug / matrixVolume_mL) / PartitionCoeff));
  if (std::abs(rtnMassInc_ug - MassIncrement_ug) > 1.e-10) {
    testCase1.AddFailure("PerfusionLimitedDiffusion returned unexpected mass increment.");
  }

  // Log how long our test took
  testCase1.GetDuration().SetValue(timer.GetElapsedTime_s("Test"), TimeUnit::s);

  // Subsequent test cases
  //timer.Start("Test");
}

void BioGearsEngineTest::AlveolarOxygenDiffusionTest(const std::string& rptDirectory)
{
  //This test examines diffusion of O2 from a gas compartment (like lungs) to a liquid compartment
  //The gas compartment is set to resemble atmospheric conditions, and the liquid compartment is oxygen-poor
  //We should expect to see oxygen diffuse into the liquid compartment
  BioGears bg(m_Logger);
  Tissue& tsu = (Tissue&)bg.GetTissue();

  double timestep_s = 1.0 / 90;
  SESubstance* O2 = bg.GetSubstances().GetSubstance("Oxygen");
  SESubstance* N2 = bg.GetSubstances().GetSubstance("Nitrogen");
  bg.GetSubstances().AddActiveSubstance(*O2);
  bg.GetSubstances().AddActiveSubstance(*N2);

  SELiquidCompartment& lcmpt = bg.GetCompartments().CreateLiquidCompartment("lcmpt");
  SEGasCompartment& gcmpt = bg.GetCompartments().CreateGasCompartment("gcmpt");

  DataTrack trk1;
  std::string rptFile = rptDirectory + "/AlveolarOxygenDiffusionTest.csv";
  double time = 0.0;

  double liquidVol_mL = 1000.0;
  double lMass_g = .001; //oxygen-poor

  gcmpt.GetVolume().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::mL);
  lcmpt.GetVolume().SetValue(liquidVol_mL, VolumeUnit::mL);

  gcmpt.GetSubstanceQuantity(*O2)->GetVolumeFraction().SetValue(.2);
  gcmpt.GetSubstanceQuantity(*N2)->GetVolumeFraction().SetValue(.8);
  gcmpt.GetPressure().SetValue(760, PressureUnit::mmHg);
  lcmpt.GetSubstanceQuantity(*O2)->GetMass().SetValue(lMass_g, MassUnit::g);
  lcmpt.GetSubstanceQuantity(*N2)->GetMass().SetValue(lMass_g, MassUnit::g);
  gcmpt.Balance(BalanceGasBy::VolumeFraction);
  lcmpt.Balance(BalanceLiquidBy::Mass);

  double diffusingCapacityO2_mL_Per_s_mmHg = .33;

  double l_O2_ug = lcmpt.GetSubstanceQuantity(*O2)->GetMass().GetValue(MassUnit::ug);
  double lconc = lcmpt.GetSubstanceQuantity(*O2)->GetConcentration().GetValue(MassPerVolumeUnit::ug_Per_L);
  double lppO2_mmHg = lcmpt.GetSubstanceQuantity(*O2)->GetPartialPressure(PressureUnit::mmHg);
  double gppO2_mmHg = gcmpt.GetSubstanceQuantity(*O2)->GetPartialPressure(PressureUnit::mmHg);

  trk1.Track("LiquidCompartmentOxygenMass_ug", time, l_O2_ug);
  trk1.Track("LiquidCompartmentOxygenConc_ug_Per_L", time, lconc);
  trk1.Track("LiquidCompartmentPO2_mmHg", time, lcmpt.GetSubstanceQuantity(*O2)->GetPartialPressure(PressureUnit::mmHg));
  trk1.Track("GasCompartmentPO2_mmHg", time, gcmpt.GetSubstanceQuantity(*O2)->GetPartialPressure(PressureUnit::mmHg));

  for (int i = 0; i < 180; i++) {
    tsu.AlveolarPartialPressureGradientDiffusion(gcmpt, lcmpt, *O2, diffusingCapacityO2_mL_Per_s_mmHg, timestep_s);
    gcmpt.Balance(BalanceGasBy::VolumeFraction);
    time += timestep_s;
    l_O2_ug = lcmpt.GetSubstanceQuantity(*O2)->GetMass().GetValue(MassUnit::ug);
    lconc = lcmpt.GetSubstanceQuantity(*O2)->GetConcentration().GetValue(MassPerVolumeUnit::ug_Per_L);
    lppO2_mmHg = lcmpt.GetSubstanceQuantity(*O2)->GetPartialPressure(PressureUnit::mmHg);
    gppO2_mmHg = gcmpt.GetSubstanceQuantity(*O2)->GetPartialPressure(PressureUnit::mmHg);
    trk1.Track("LiquidCompartmentOxygenMass_ug", time, l_O2_ug);
    trk1.Track("LiquidCompartmentOxygenConc_ug_Per_L", time, lconc);
    trk1.Track("LiquidCompartmentPO2_mmHg", time, lcmpt.GetSubstanceQuantity(*O2)->GetPartialPressure(PressureUnit::mmHg));
    trk1.Track("GasCompartmentPO2_mmHg", time, gcmpt.GetSubstanceQuantity(*O2)->GetPartialPressure(PressureUnit::mmHg));
  }

  // Write data to file
  trk1.WriteTrackToFile(rptFile.c_str());
}

void BioGearsEngineTest::AlveolarCarbonDioxideDiffusionTest(const std::string& rptDirectory)
{
  //Second test case: High liquid compartment CO2 concentration with a lower gas compartment CO2 concentration
  //We should expect CO2 to diffuse out of the liquid compartment into the gas compartment
  BioGears bg(m_Logger);
  Tissue& tsu = (Tissue&)bg.GetTissue();

  DataTrack trk2;
  std::string rptFile = rptDirectory + "/AlveolarCarbonDioxideDiffusionTest.csv";

  double timestep_s = 1.0 / 90;

  SESubstance* CO2 = bg.GetSubstances().GetSubstance("CarbonDioxide");
  SESubstance* N2 = bg.GetSubstances().GetSubstance("Nitrogen");
  bg.GetSubstances().AddActiveSubstance(*CO2);
  bg.GetSubstances().AddActiveSubstance(*N2);

  SELiquidCompartment& lcmpt2 = bg.GetCompartments().CreateLiquidCompartment("lcmpt2");
  SEGasCompartment& gcmpt2 = bg.GetCompartments().CreateGasCompartment("gcmpt2");

  double time = 0.0;

  double liquidVol_mL = 1000.0;
  double lMass_g = .04; //high amount of CO2

  gcmpt2.GetVolume().SetValue(std::numeric_limits<double>::infinity(), VolumeUnit::mL);
  lcmpt2.GetVolume().SetValue(liquidVol_mL, VolumeUnit::mL);

  gcmpt2.GetSubstanceQuantity(*CO2)->GetVolumeFraction().SetValue(.0004); //atmospheric CO2 is .04%
  gcmpt2.GetSubstanceQuantity(*N2)->GetVolumeFraction().SetValue(1 - .0004);
  gcmpt2.GetPressure().SetValue(760, PressureUnit::mmHg);
  lcmpt2.GetSubstanceQuantity(*CO2)->GetMass().SetValue(lMass_g, MassUnit::g);
  lcmpt2.GetSubstanceQuantity(*N2)->GetMass().SetValue(lMass_g, MassUnit::g);
  gcmpt2.Balance(BalanceGasBy::VolumeFraction);
  lcmpt2.Balance(BalanceLiquidBy::Mass);

  double l_CO2_ug = lcmpt2.GetSubstanceQuantity(*CO2)->GetMass().GetValue(MassUnit::ug);
  double lconc = lcmpt2.GetSubstanceQuantity(*CO2)->GetConcentration().GetValue(MassPerVolumeUnit::ug_Per_L);
  double lppO2_mmHg = lcmpt2.GetSubstanceQuantity(*CO2)->GetPartialPressure(PressureUnit::mmHg);
  double gppO2_mmHg = gcmpt2.GetSubstanceQuantity(*CO2)->GetPartialPressure(PressureUnit::mmHg);
  double diffusingCapacityO2_mL_Per_s_mmHg = .33;

  trk2.Track("LiquidCompartmentCO2Mass_ug", time, l_CO2_ug);
  trk2.Track("LiquidCompartmentCO2Conc_ug_Per_L", time, lconc);
  trk2.Track("LiquidCompartmentPCO2_mmHg", time, lcmpt2.GetSubstanceQuantity(*CO2)->GetPartialPressure(PressureUnit::mmHg));
  trk2.Track("GasCompartmentPCO2_mmHg", time, gcmpt2.GetSubstanceQuantity(*CO2)->GetPartialPressure(PressureUnit::mmHg));

  for (int i = 0; i < 180; i++) {
    tsu.AlveolarPartialPressureGradientDiffusion(gcmpt2, lcmpt2, *CO2, diffusingCapacityO2_mL_Per_s_mmHg, timestep_s);
    gcmpt2.Balance(BalanceGasBy::VolumeFraction);
    time += timestep_s;
    l_CO2_ug = lcmpt2.GetSubstanceQuantity(*CO2)->GetMass().GetValue(MassUnit::ug);
    lconc = lcmpt2.GetSubstanceQuantity(*CO2)->GetConcentration().GetValue(MassPerVolumeUnit::ug_Per_L);
    lppO2_mmHg = lcmpt2.GetSubstanceQuantity(*CO2)->GetPartialPressure(PressureUnit::mmHg);
    gppO2_mmHg = gcmpt2.GetSubstanceQuantity(*CO2)->GetPartialPressure(PressureUnit::mmHg);
    trk2.Track("LiquidCompartmentCO2Mass_ug", time, l_CO2_ug);
    trk2.Track("LiquidCompartmentCO2Conc_ug_Per_L", time, lconc);
    trk2.Track("LiquidCompartmentPCO2_mmHg", time, lcmpt2.GetSubstanceQuantity(*CO2)->GetPartialPressure(PressureUnit::mmHg));
    trk2.Track("GasCompartmentPCO2_mmHg", time, gcmpt2.GetSubstanceQuantity(*CO2)->GetPartialPressure(PressureUnit::mmHg));
  }

  // Write data to file
  trk2.WriteTrackToFile(rptFile.c_str());
}

void BioGearsEngineTest::SimpleDiffusionTwoCompartmentTest(const std::string& rptDirectory)
{
  // Second test case --
  // Tests the diffusion between two tissue liquid compartments
  // Calculated permeability coefficient
  BioGears bg(m_Logger);
  Tissue& tsu = (Tissue&)bg.GetTissue();
  double timestep_s = 1.0 / 90;
  SESubstance* o2 = bg.GetSubstances().GetSubstance("Oxygen");

  bg.GetSubstances().AddActiveSubstance(*o2);
  SETissueCompartment& tissue = bg.GetCompartments().CreateTissueCompartment("Tissue");
  SELiquidCompartment& cmpt1_IC = bg.GetCompartments().CreateLiquidCompartment("cmpt1_IC");
  SELiquidCompartment& cmpt2_EC = bg.GetCompartments().CreateLiquidCompartment("cmpt2_EC");

  DataTrack trk2;
  std::string rptFile = rptDirectory + "/SimpleDiffusionTwoCompartmentTest.csv";
  double time = 0.0;

  double ecVol_mL = 40.0;
  double icVol_mL = 30.0;
  double ecMass_g = 1.0;
  double icMass_g = 20.0;

  cmpt2_EC.GetVolume().SetValue(ecVol_mL, VolumeUnit::mL);
  cmpt1_IC.GetVolume().SetValue(icVol_mL, VolumeUnit::mL);
  cmpt2_EC.GetSubstanceQuantity(*o2)->GetMass().SetValue(ecMass_g, MassUnit::g);
  cmpt1_IC.GetSubstanceQuantity(*o2)->GetMass().SetValue(icMass_g, MassUnit::g);
  cmpt2_EC.Balance(BalanceLiquidBy::Mass);
  cmpt1_IC.Balance(BalanceLiquidBy::Mass);

  tissue.GetTotalMass().SetValue(500.0, MassUnit::g);

  double molarMass = o2->GetMolarMass().GetValue(MassPerAmountUnit::g_Per_mol);
  double molecularRadius = 0.0348 * std::pow(molarMass, 0.4175);
  double permeabilityCoefficient_mL_Per_s_hg;
  if (molecularRadius > 1.0) {
    permeabilityCoefficient_mL_Per_s_hg = 0.0287 * std::pow(molecularRadius, -2.920);
  } else {
    permeabilityCoefficient_mL_Per_s_hg = 0.0184 * std::pow(molecularRadius, -1.223);
  }

  // The tissue mass baseline is a constant property of the tissue - values can be found in the ICRP and other sources
  // We use the tissue mass as a stand-in for surface area, follow the lead of Renkin and Curry
  // Here are the rules for the different types of compartments
  // Vascular to tissue (in BioGears it is always extracellular, but it doesn't matter)
  // The mass is the tissue compartment mass
  // Tissue to tissue
  // The mass is always the smaller of the two (the smaller area will be the max area for diffusion)
  double permeabilityCoefficient_mL_Per_s = permeabilityCoefficient_mL_Per_s_hg / 100.0 * tissue.GetTotalMass(MassUnit::g);

  trk2.Track("ExtracellularMass_g", time, cmpt2_EC.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g));
  trk2.Track("IntracellularMass_g", time, cmpt1_IC.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g));
  trk2.Track("ExtracellularConc_g_Per_mL", time, cmpt2_EC.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk2.Track("IntracellularConc_g_Per_mL", time, cmpt1_IC.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));

  for (int i = 0; i < 3600; i++) {
    tsu.MoveMassBySimpleDiffusion(cmpt1_IC, cmpt2_EC, *o2, permeabilityCoefficient_mL_Per_s, timestep_s);
    cmpt2_EC.Balance(BalanceLiquidBy::Mass);
    cmpt1_IC.Balance(BalanceLiquidBy::Mass);
    time += timestep_s;
    ecMass_g = cmpt2_EC.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g);
    icMass_g = cmpt1_IC.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g);
    double ecConc_g_Per_mL = cmpt2_EC.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL);
    double icConc_g_Per_mL = cmpt1_IC.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL);
    trk2.Track("ExtracellularMass_g", time, ecMass_g);
    trk2.Track("IntracellularMass_g", time, icMass_g);
    trk2.Track("ExtracellularConc_g_Per_mL", time, ecConc_g_Per_mL);
    trk2.Track("IntracellularConc_g_Per_mL", time, icConc_g_Per_mL);
  }

  trk2.WriteTrackToFile(rptFile.c_str());
}

void BioGearsEngineTest::SimpleDiffusionFourCompartmentTest(const std::string& rptDirectory)
{
  // Now we will add two more compartments and test two things
  // First, that the concentration does not change within a time slice
  // Second, that the concentrations equilibrate appropriately
  //              / cmpt1
  //     cmpt4 <->  cmpt2
  //              \ cmpt3
  // Artificial permeability coefficient
  BioGears bg(m_Logger);
  Tissue& tsu = (Tissue&)bg.GetTissue();
  double timestep_s = 1.0 / 90;
  SESubstance* o2 = bg.GetSubstances().GetSubstance("Oxygen");
  bg.GetSubstances().AddActiveSubstance(*o2);
  SETissueCompartment& tissue = bg.GetCompartments().CreateTissueCompartment("Tissue");
  SELiquidCompartment& cmpt1_IC = bg.GetCompartments().CreateLiquidCompartment("cmpt1_IC");
  SELiquidCompartment& cmpt2_EC = bg.GetCompartments().CreateLiquidCompartment("cmpt2_EC");
  SELiquidCompartment& cmpt3_LQ = bg.GetCompartments().CreateLiquidCompartment("cmpt3_LQ");
  SELiquidCompartment& cmpt4_LQ = bg.GetCompartments().CreateLiquidCompartment("cmpt4_LQ");

  DataTrack trk3;
  std::string rptFile = rptDirectory + "/SimpleDiffusionFourCompartmentTest.csv";
  double time = 0.0;

  // Initialize
  double v3Mass_g = 10.0;
  double v4Mass_g = 0.0;
  double v3Vol_mL = 20.0;
  double v4Vol_mL = 50.0;
  double ecVol_mL = 10.0;
  double icVol_mL = 50.0;
  double ecMass_g = 2.5;
  double icMass_g = 2.0;
  cmpt2_EC.GetVolume().SetValue(ecVol_mL, VolumeUnit::mL);
  cmpt1_IC.GetVolume().SetValue(icVol_mL, VolumeUnit::mL);
  cmpt3_LQ.GetVolume().SetValue(v3Vol_mL, VolumeUnit::mL);
  cmpt4_LQ.GetVolume().SetValue(v4Vol_mL, VolumeUnit::mL);
  cmpt2_EC.GetSubstanceQuantity(*o2)->GetMass().SetValue(ecMass_g, MassUnit::g);
  cmpt1_IC.GetSubstanceQuantity(*o2)->GetMass().SetValue(icMass_g, MassUnit::g);
  cmpt3_LQ.GetSubstanceQuantity(*o2)->GetMass().SetValue(v3Mass_g, MassUnit::g);
  cmpt4_LQ.GetSubstanceQuantity(*o2)->GetMass().SetValue(v4Mass_g, MassUnit::g);
  cmpt4_LQ.Balance(BalanceLiquidBy::Mass);
  cmpt3_LQ.Balance(BalanceLiquidBy::Mass);
  cmpt2_EC.Balance(BalanceLiquidBy::Mass);
  cmpt1_IC.Balance(BalanceLiquidBy::Mass);

  // Make it a little faster for this test
  double permeabilityCoefficient_mL_Per_s = 20.0;

  tsu.MoveMassBySimpleDiffusion(cmpt1_IC, cmpt4_LQ, *o2, permeabilityCoefficient_mL_Per_s, timestep_s);
  tsu.MoveMassBySimpleDiffusion(cmpt2_EC, cmpt4_LQ, *o2, permeabilityCoefficient_mL_Per_s, timestep_s);
  tsu.MoveMassBySimpleDiffusion(cmpt3_LQ, cmpt4_LQ, *o2, permeabilityCoefficient_mL_Per_s, timestep_s);

  double v3CalculatedConcentration_g_Per_mL = cmpt3_LQ.GetSubstanceQuantity(*o2)->GetConcentration(MassPerVolumeUnit::g_Per_mL);
  double v4CalculatedConcentration_g_Per_mL = cmpt4_LQ.GetSubstanceQuantity(*o2)->GetConcentration(MassPerVolumeUnit::g_Per_mL);
  double v3ExpectedConcentration_g_Per_mL = (v3Mass_g / v3Vol_mL);
  double v4ExpectedConcentration_g_Per_mL = (v4Mass_g / v4Vol_mL);

  // Ok, now balance
  cmpt4_LQ.Balance(BalanceLiquidBy::Mass);
  cmpt3_LQ.Balance(BalanceLiquidBy::Mass);
  cmpt2_EC.Balance(BalanceLiquidBy::Mass);
  cmpt1_IC.Balance(BalanceLiquidBy::Mass);
  trk3.Track("cmpt1mass_g", time, cmpt1_IC.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g));
  trk3.Track("cmpt2mass_g", time, cmpt2_EC.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g));
  trk3.Track("cmpt3mass_g", time, cmpt3_LQ.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g));
  trk3.Track("cmpt4mass_g", time, cmpt4_LQ.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g));
  trk3.Track("cmpt1conc_g_Per_mL", time, cmpt1_IC.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk3.Track("cmpt2conc_g_Per_mL", time, cmpt2_EC.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk3.Track("cmpt3conc_g_Per_mL", time, cmpt3_LQ.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk3.Track("cmpt4conc_g_Per_mL", time, cmpt4_LQ.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));

  for (int i = 0; i < 3600; i++) {
    tsu.MoveMassBySimpleDiffusion(cmpt1_IC, cmpt4_LQ, *o2, permeabilityCoefficient_mL_Per_s, timestep_s);
    tsu.MoveMassBySimpleDiffusion(cmpt2_EC, cmpt4_LQ, *o2, permeabilityCoefficient_mL_Per_s, timestep_s);
    tsu.MoveMassBySimpleDiffusion(cmpt3_LQ, cmpt4_LQ, *o2, permeabilityCoefficient_mL_Per_s, timestep_s);
    cmpt4_LQ.Balance(BalanceLiquidBy::Mass);
    cmpt3_LQ.Balance(BalanceLiquidBy::Mass);
    cmpt2_EC.Balance(BalanceLiquidBy::Mass);
    cmpt1_IC.Balance(BalanceLiquidBy::Mass);
    time += timestep_s;
    trk3.Track("cmpt1mass_g", time, cmpt1_IC.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g));
    trk3.Track("cmpt2mass_g", time, cmpt2_EC.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g));
    trk3.Track("cmpt3mass_g", time, cmpt3_LQ.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g));
    trk3.Track("cmpt4mass_g", time, cmpt4_LQ.GetSubstanceQuantity(*o2)->GetMass().GetValue(MassUnit::g));
    trk3.Track("cmpt1conc_g_Per_mL", time, cmpt1_IC.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk3.Track("cmpt2conc_g_Per_mL", time, cmpt2_EC.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk3.Track("cmpt3conc_g_Per_mL", time, cmpt3_LQ.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk3.Track("cmpt4conc_g_Per_mL", time, cmpt4_LQ.GetSubstanceQuantity(*o2)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  }

  // Write data to file
  trk3.WriteTrackToFile(rptFile.c_str());
}

void BioGearsEngineTest::SimpleDiffusionHierarchyTest(const std::string& rptDirectory)
{
  // Tests diffusion with distribution for hierarchical compartments
  //                                       L0C0        <---->        M0C0
  //                                      /    \                    /  |  \
  //                                  L1C0      L1C1            M1C0  M1C1 M1C2  <--Only these cmpts have data
  //                                  /  \      /  \
  // Only these cmpts have data--> L2C0  L2C1 L2C2 L2C3
  // Artificial permeability coefficient

  BioGears bg(m_Logger);
  Tissue& tsu = (Tissue&)bg.GetTissue();
  double timestep_s = 1.0 / 90;
  SESubstance* sub = bg.GetSubstances().GetSubstance("Desflurane");
  bg.GetSubstances().AddActiveSubstance(*sub);

  DataTrack trk4;
  std::string rptFile = rptDirectory + "/SimpleDiffusionHierarchyTest.csv";
  double time = 0.0;

  SELiquidCompartment& L0C0 = bg.GetCompartments().CreateLiquidCompartment("L0C0");
  SELiquidCompartment& L1C0 = bg.GetCompartments().CreateLiquidCompartment("L1C0");
  SELiquidCompartment& L1C1 = bg.GetCompartments().CreateLiquidCompartment("L1C1");
  SELiquidCompartment& L2C0 = bg.GetCompartments().CreateLiquidCompartment("L2C0");
  SELiquidCompartment& L2C1 = bg.GetCompartments().CreateLiquidCompartment("L2C1");
  SELiquidCompartment& L2C2 = bg.GetCompartments().CreateLiquidCompartment("L2C2");
  SELiquidCompartment& L2C3 = bg.GetCompartments().CreateLiquidCompartment("L2C3");
  SELiquidCompartment& M0C0 = bg.GetCompartments().CreateLiquidCompartment("M0C0");
  SELiquidCompartment& M1C0 = bg.GetCompartments().CreateLiquidCompartment("M1C0");
  SELiquidCompartment& M1C1 = bg.GetCompartments().CreateLiquidCompartment("M1C1");
  SELiquidCompartment& M1C2 = bg.GetCompartments().CreateLiquidCompartment("M1C2");

  // Build up the hierarchy
  L0C0.AddChild(L1C0);
  L0C0.AddChild(L1C1);
  L1C0.AddChild(L2C0);
  L1C0.AddChild(L2C1);
  L1C1.AddChild(L2C2);
  L1C1.AddChild(L2C3);
  M0C0.AddChild(M1C0);
  M0C0.AddChild(M1C1);
  M0C0.AddChild(M1C2);
  bg.GetCompartments().StateChange(); // Call this, AFTER YOU SET UP YOUR HIERARCHY, to ensure all parent compartments have their link data

  double Lvol_mL = 10.0;
  double Mvol_mL = 8.0;
  double M1C2mol_fraction = 0.01;
  L2C0.GetVolume().SetValue(Lvol_mL, VolumeUnit::mL);
  L2C1.GetVolume().SetValue(Lvol_mL, VolumeUnit::mL);
  L2C2.GetVolume().SetValue(Lvol_mL, VolumeUnit::mL);
  L2C3.GetVolume().SetValue(Lvol_mL, VolumeUnit::mL);
  M1C0.GetVolume().SetValue(Mvol_mL, VolumeUnit::mL);
  M1C1.GetVolume().SetValue(Mvol_mL, VolumeUnit::mL);
  M1C2.GetVolume().SetValue(Mvol_mL, VolumeUnit::mL);

  double Lmolarity_mmol_Per_mL = 10.0;
  double Mmolarity_mmol_Per_mL = 0.0;
  L2C0.GetSubstanceQuantity(*sub)->GetMolarity().SetValue(Lmolarity_mmol_Per_mL, AmountPerVolumeUnit::mmol_Per_mL);
  L2C1.GetSubstanceQuantity(*sub)->GetMolarity().SetValue(0.5 * Lmolarity_mmol_Per_mL, AmountPerVolumeUnit::mmol_Per_mL);
  L2C2.GetSubstanceQuantity(*sub)->GetMolarity().SetValue(2.0 * Lmolarity_mmol_Per_mL, AmountPerVolumeUnit::mmol_Per_mL);
  L2C3.GetSubstanceQuantity(*sub)->GetMolarity().SetValue(1.5 * Lmolarity_mmol_Per_mL, AmountPerVolumeUnit::mmol_Per_mL);
  M1C0.GetSubstanceQuantity(*sub)->GetMolarity().SetValue(Mmolarity_mmol_Per_mL, AmountPerVolumeUnit::mmol_Per_mL);
  M1C1.GetSubstanceQuantity(*sub)->GetMolarity().SetValue(Mmolarity_mmol_Per_mL, AmountPerVolumeUnit::mmol_Per_mL);
  M1C2.GetSubstanceQuantity(*sub)->GetMolarity().SetValue(Mmolarity_mmol_Per_mL * M1C2mol_fraction, AmountPerVolumeUnit::mmol_Per_mL); // He has less moles

  L2C0.Balance(BalanceLiquidBy::Molarity);
  L2C1.Balance(BalanceLiquidBy::Molarity);
  L2C2.Balance(BalanceLiquidBy::Molarity);
  L2C3.Balance(BalanceLiquidBy::Molarity);
  M1C0.Balance(BalanceLiquidBy::Molarity);
  M1C1.Balance(BalanceLiquidBy::Molarity);
  M1C2.Balance(BalanceLiquidBy::Molarity);

  double permeabilityCoefficient_mL_Per_s = 2.0;

  trk4.Track("L0C0_mass_g", time, L0C0.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("L1C0_mass_g", time, L1C0.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("L1C1_mass_g", time, L1C1.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("L2C0_mass_g", time, L2C0.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("L2C1_mass_g", time, L2C1.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("L2C2_mass_g", time, L2C2.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("L2C3_mass_g", time, L2C3.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("M0C0_mass_g", time, M0C0.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("M1C0_mass_g", time, M1C0.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("M1C1_mass_g", time, M1C1.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("M1C2_mass_g", time, M1C2.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
  trk4.Track("L0C0_conc_g_Per_mL", time, L0C0.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk4.Track("L1C0_conc_g_Per_mL", time, L1C0.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk4.Track("L1C1_conc_g_Per_mL", time, L1C1.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk4.Track("L2C0_conc_g_Per_mL", time, L2C0.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk4.Track("L2C1_conc_g_Per_mL", time, L2C1.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk4.Track("L2C2_conc_g_Per_mL", time, L2C2.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk4.Track("L2C3_conc_g_Per_mL", time, L2C3.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk4.Track("M0C0_conc_g_Per_mL", time, M0C0.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk4.Track("M1C0_conc_g_Per_mL", time, M1C0.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk4.Track("M1C1_conc_g_Per_mL", time, M1C1.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  trk4.Track("M1C2_conc_g_Per_mL", time, M1C2.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));

  for (int i = 0; i < 3600; i++) {
    tsu.MoveMassBySimpleDiffusion(L0C0, M0C0, *sub, permeabilityCoefficient_mL_Per_s, timestep_s);
    L2C0.Balance(BalanceLiquidBy::Mass);
    L2C1.Balance(BalanceLiquidBy::Mass);
    L2C2.Balance(BalanceLiquidBy::Mass);
    L2C3.Balance(BalanceLiquidBy::Mass);
    M1C0.Balance(BalanceLiquidBy::Mass);
    M1C1.Balance(BalanceLiquidBy::Mass);
    M1C2.Balance(BalanceLiquidBy::Mass);
    time += timestep_s;
    trk4.Track("L0C0_mass_g", time, L0C0.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("L1C0_mass_g", time, L1C0.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("L1C1_mass_g", time, L1C1.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("L2C0_mass_g", time, L2C0.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("L2C1_mass_g", time, L2C1.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("L2C2_mass_g", time, L2C2.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("L2C3_mass_g", time, L2C3.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("M0C0_mass_g", time, M0C0.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("M1C0_mass_g", time, M1C0.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("M1C1_mass_g", time, M1C1.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("M1C2_mass_g", time, M1C2.GetSubstanceQuantity(*sub)->GetMass().GetValue(MassUnit::g));
    trk4.Track("L0C0_conc_g_Per_mL", time, L0C0.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk4.Track("L1C0_conc_g_Per_mL", time, L1C0.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk4.Track("L1C1_conc_g_Per_mL", time, L1C1.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk4.Track("L2C0_conc_g_Per_mL", time, L2C0.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk4.Track("L2C1_conc_g_Per_mL", time, L2C1.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk4.Track("L2C2_conc_g_Per_mL", time, L2C2.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk4.Track("L2C3_conc_g_Per_mL", time, L2C3.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk4.Track("M0C0_conc_g_Per_mL", time, M0C0.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk4.Track("M1C0_conc_g_Per_mL", time, M1C0.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk4.Track("M1C1_conc_g_Per_mL", time, M1C1.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
    trk4.Track("M1C2_conc_g_Per_mL", time, M1C2.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL));
  }

  trk4.WriteTrackToFile(rptFile.c_str());
}

void BioGearsEngineTest::FacilitatedDiffusionTest(const std::string& rptDirectory)
{
  BioGears bg(m_Logger);
  Tissue& tsu = (Tissue&)bg.GetTissue();
  double timestep_s = 1.0 / 90;
  SESubstance* sub = bg.GetSubstances().GetSubstance("Glucose");
  bg.GetSubstances().AddActiveSubstance(*sub);
  SETissueCompartment& tissue = bg.GetCompartments().CreateTissueCompartment("Tissue");
  SELiquidCompartment& tissueExtracellular = bg.GetCompartments().CreateLiquidCompartment("Extracellular");
  SELiquidCompartment& vascular = bg.GetCompartments().CreateLiquidCompartment("Vascular");

  DataTrack trk1;
  std::string rptFile = rptDirectory + "/FacilitatedDiffusionTest.csv";
  double time = 0.0;

  tissueExtracellular.GetVolume().SetValue(45.0, VolumeUnit::mL);
  vascular.GetVolume().SetValue(100.0, VolumeUnit::mL);
  tissueExtracellular.GetSubstanceQuantity(*sub)->GetMass().SetValue(2.9, MassUnit::g);
  vascular.GetSubstanceQuantity(*sub)->GetMass().SetValue(5.0, MassUnit::g);
  tissueExtracellular.Balance(BalanceLiquidBy::Mass);
  vascular.Balance(BalanceLiquidBy::Mass);

  double capCoverage_cm2 = 90.0; /// \todo Relate capillary coverage to tissue mass.
  double maximumMassFlux = sub->GetMaximumDiffusionFlux(MassPerAreaTimeUnit::g_Per_cm2_s);
  double combinedCoefficient_g_Per_s = maximumMassFlux * capCoverage_cm2;

  double vascularConc_g_Per_mL = vascular.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL);
  double ECconc_g_Per_mL = tissueExtracellular.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL);
  trk1.Track("VascularConc_g_Per_mL", time, vascularConc_g_Per_mL);
  trk1.Track("ExtracellularConc_g_Per_mL", time, ECconc_g_Per_mL);

  for (int i = 0; i < 180; i++) {
    tsu.MoveMassByFacilitatedDiffusion(vascular, tissueExtracellular, *sub, combinedCoefficient_g_Per_s, timestep_s);
    vascular.Balance(BalanceLiquidBy::Mass);
    tissueExtracellular.Balance(BalanceLiquidBy::Mass);
    time += timestep_s;
    vascularConc_g_Per_mL = vascular.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL);
    ECconc_g_Per_mL = tissueExtracellular.GetSubstanceQuantity(*sub)->GetConcentration().GetValue(MassPerVolumeUnit::g_Per_mL);
    trk1.Track("VascularConc_g_Per_mL", time, vascularConc_g_Per_mL);
    trk1.Track("ExtracellularConc_g_Per_mL", time, ECconc_g_Per_mL);
  }

  trk1.WriteTrackToFile(rptFile.c_str());
}

void BioGearsEngineTest::GenericClearanceTest(SETestSuite& testSuite)
{
  TimingProfile timer;
  BioGears bg(testSuite.GetLogger());
  Tissue& tsu = (Tissue&)bg.GetTissue();

  // First test case
  SETestCase& testCase1 = testSuite.CreateTestCase();
  testCase1.SetName("GenericClearanceTestLiquid");
  // You at least need 2 test cases here (liquid and tissue)
  SETestCase& testCase = testSuite.CreateTestCase();
  timer.Start("Test");
  // Test Logic
  //testCase.GetFailures().push_back("Report any errors like this");
  // Log how long our test took
  testCase1.GetDuration().SetValue(timer.GetElapsedTime_s("Test"), TimeUnit::s);

  // Second test case
  SETestCase& testCase2 = testSuite.CreateTestCase();
  testCase2.SetName("GenericClearanceTestTissue");
  timer.Start("Test");

  testCase2.GetDuration().SetValue(timer.GetElapsedTime_s("Test"), TimeUnit::s);
  // Subsequent test cases
}

void BioGearsEngineTest::GenericExcretionTest(SETestSuite& testSuite)
{
  TimingProfile timer;
  BioGears bg(testSuite.GetLogger());
  Tissue& tsu = (Tissue&)bg.GetTissue();

  // First test case
  SETestCase& testCase1 = testSuite.CreateTestCase();
  testCase1.SetName("GenericExcretionTest");
  timer.Start("Test");
  // Test Logic
  //testCase.GetFailures().push_back("Report any errors like this");
  // Log how long our test took
  testCase1.GetDuration().SetValue(timer.GetElapsedTime_s("Test"), TimeUnit::s);
}

//Set-up the test suite
void BioGearsEngineTest::DiffusionClearanceExcretionTests(const std::string& rptDirectory)
{

  m_Logger->ResetLogFile(rptDirectory + "/DiffusionClearanceExcretionTests.log");
  // Set up our test report

  SETestReport testReport = SETestReport(m_Logger);

  SETestSuite& tsDistMass = testReport.CreateTestSuite();
  tsDistMass.SetName("DistributeMass");
  DistributeMass(tsDistMass);

  SETestSuite& ts1 = testReport.CreateTestSuite();
  ts1.SetName("PerfusionLimitedDiffusionTest");
  PerfusionLimitedDiffusionTest(ts1);

  //SETestSuite& ts5 = testReport.CreateTestSuite();
  //ts5.SetName("ActiveTransportTest");
  //ActiveTransportTest(ts5, rptDirectory);


  //SETestSuite& ts7 = testReport.CreateTestSuite();
  //ts6.SetName("GenericClearanceTest");
  //GenericClearanceTest(ts7);

  //SETestSuite& ts8 = testReport.CreateTestSuite();
  //ts7.SetName("GenericExcretionTest");
  //GenericExcretionTest(ts8);

  //testReport.WriteFile(rptDirectory + "/GasCompartmentTest.xml");
}

//Set up three substances and three pairs of vascular -> extracellular compartments
//Determine whether looping over cmpt pairs and substances is more efficient than
//transporting by setting up substance/cmpt matrices and using Eigen solvers
void BioGearsEngineTest::DiffusionMatrixMathTest(const std::string& rptDirectory)
{
  m_Logger->ResetLogFile(rptDirectory + "/DiffusionMatrixMathTest.log");
  BioGears bg(m_Logger);
  Tissue& tsu = (Tissue&)bg.GetTissue();
  bg.GetPatient().Load("./patients/StandardMale.xml");
  bg.SetupPatient();
  bg.m_Config->EnableRenal(CDM::enumOnOff::Off);
  bg.m_Config->EnableTissue(CDM::enumOnOff::On);
  bg.m_Config->EnableBioGearsLite(CDM::enumOnOff::On);
  bg.CreateCircuitsAndCompartments();

  std::string standardFile = rptDirectory + "/StandardDiffusion.csv";
  std::string matrixFile = rptDirectory + "/MatrixDiffusion.csv";
  DataTrack standardTrk;
  DataTrack matrixTrk;

  //Use sodium, potassium, and chloride as model substances
  SESubstance* Na = &bg.GetSubstances().GetSodium();
  SESubstance* K = &bg.GetSubstances().GetPotassium();
  SESubstance* Cl = &bg.GetSubstances().GetChloride();

  bg.GetSubstances().AddActiveSubstance(*Na);
  bg.GetSubstances().AddActiveSubstance(*K);
  bg.GetSubstances().AddActiveSubstance(*Cl);

  std::vector<SESubstance*> subs = { Na, K, Cl };

  //Grab some existing compartments--notice that this test was done using BioGears Lite
  SELiquidCompartment* liverVas = bg.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::Liver);
  SELiquidCompartment* liverTis = bg.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::LiverExtracellular);
  SELiquidCompartment* skinVas = bg.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::Skin);
  SELiquidCompartment* skinTis = bg.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::SkinExtracellular);
  SELiquidCompartment* muscleVas = bg.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::Muscle);
  SELiquidCompartment* muscleTis = bg.GetCompartments().GetLiquidCompartment(BGE::ExtravascularCompartment::MuscleExtracellular);

  //Set concentrations.  Liver uses vascular and intra values in engine and other cmpts mix the values so that we can see
  //different progressions towards equilibrium
  liverVas->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(145.0, MassPerVolumeUnit::mg_Per_L);
  liverVas->GetSubstanceQuantity(*K)->GetConcentration().SetValue(4.5, MassPerVolumeUnit::mg_Per_L);
  liverVas->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(105.0, MassPerVolumeUnit::mg_Per_L);
  liverTis->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(20.0, MassPerVolumeUnit::mg_Per_L);
  liverTis->GetSubstanceQuantity(*K)->GetConcentration().SetValue(120.0, MassPerVolumeUnit::mg_Per_L);
  liverTis->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(15.0, MassPerVolumeUnit::mg_Per_L);

  skinVas->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(50.0, MassPerVolumeUnit::mg_Per_L);
  skinVas->GetSubstanceQuantity(*K)->GetConcentration().SetValue(40.0, MassPerVolumeUnit::mg_Per_L);
  skinVas->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(10.0, MassPerVolumeUnit::mg_Per_L);
  skinTis->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(100.0, MassPerVolumeUnit::mg_Per_L);
  skinTis->GetSubstanceQuantity(*K)->GetConcentration().SetValue(20.0, MassPerVolumeUnit::mg_Per_L);
  skinTis->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(200.0, MassPerVolumeUnit::mg_Per_L);

  muscleVas->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(5.0, MassPerVolumeUnit::mg_Per_L);
  muscleVas->GetSubstanceQuantity(*K)->GetConcentration().SetValue(80.0, MassPerVolumeUnit::mg_Per_L);
  muscleVas->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(500.0, MassPerVolumeUnit::mg_Per_L);
  muscleTis->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(75.0, MassPerVolumeUnit::mg_Per_L);
  muscleTis->GetSubstanceQuantity(*K)->GetConcentration().SetValue(70.0, MassPerVolumeUnit::mg_Per_L);
  muscleTis->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(1.0, MassPerVolumeUnit::mg_Per_L);

  liverVas->Balance(BalanceLiquidBy::Concentration);
  liverTis->Balance(BalanceLiquidBy::Concentration);
  skinVas->Balance(BalanceLiquidBy::Concentration);
  skinTis->Balance(BalanceLiquidBy::Concentration);
  muscleVas->Balance(BalanceLiquidBy::Concentration);
  muscleTis->Balance(BalanceLiquidBy::Concentration);

  //Let's simulate 3- minutes
  double simTime_s = 60.0 * 60.0;
  double timestep_s = 1.0 / 50.0;
  double timeStandard_s = 0.0;
  double timeMatrix_s = 0.0;
  double watchStandard = 0.0;
  double watchMatrix = 0.0;
  biogears::StopWatch<std::chrono::nanoseconds> timerStandard;
  biogears::StopWatch<std::chrono::nanoseconds> timerMatrix;

  //Make a vascular-tissue map just like "CalcDiffusion" does
  std::map<SELiquidCompartment*, SELiquidCompartment*> cmptMap;
  cmptMap[liverVas] = liverTis;
  cmptMap[skinVas] = skinTis;
  cmptMap[muscleVas] = muscleTis;

  //This loop is how we calculate diffusion now
  timerStandard.reset();
  while (timeStandard_s <= simTime_s) {
    timerStandard.lap();
    for (auto vtPair : cmptMap) {
      SELiquidCompartment* vascular = vtPair.first;
      SELiquidCompartment* tissue = vtPair.second;
      for (auto sub : subs) {
        double molarMass_g_Per_mol = sub->GetMolarMass(MassPerAmountUnit::g_Per_mol);
        double molecularRadius_nm = 0.0348 * std::pow(molarMass_g_Per_mol, 0.4175);
        double vToECpermeabilityCoefficient_mL_Per_s_g = 0.0287 * std::pow(molecularRadius_nm, -2.920) / 100.0; // This is only valid if the molecular radius is > 1.0 nm.
        if (molecularRadius_nm < 1.0)
          vToECpermeabilityCoefficient_mL_Per_s_g = 0.0184 * std::pow(molecularRadius_nm, -1.223) / 100.0;

        // Multiply by tissue mass to get the tissue-dependent coefficient.
        double tissueMass = 100.0;   //Just assume this is the same for all of them--this is 1000x greater than in matrix test below because MoveMassBySimpleDiffusion function
                                      //increments by ug automatically and I'd rather be working with mg.  So this is acting like our conversion factor.
        double vToECpermeabilityCoefficient_mL_Per_s = vToECpermeabilityCoefficient_mL_Per_s_g * tissueMass;

        //Vascular to Extracellular
        double moved_ug = tsu.MoveMassBySimpleDiffusion(*vascular, *tissue, *sub, vToECpermeabilityCoefficient_mL_Per_s, timestep_s);
      }
      vascular->Balance(BalanceLiquidBy::Mass);
      tissue->Balance(BalanceLiquidBy::Mass);
    }
    watchStandard += timerStandard.lap();

    standardTrk.Track("CumulativeTime_ms", timeStandard_s, watchStandard / 1e6);
    standardTrk.Track("LiverVascular_Na", timeStandard_s, liverVas->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("LiverVascular_K", timeStandard_s, liverVas->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("LiverVascular_Cl", timeStandard_s, liverVas->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("LiverTissue_Na", timeStandard_s, liverTis->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("LiverTissue_K", timeStandard_s, liverTis->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("LiverTissue_Cl", timeStandard_s, liverTis->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("MuscleVascular_Na", timeStandard_s, muscleVas->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("MuscleVascular_K", timeStandard_s, muscleVas->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("MuscleVascular_Cl", timeStandard_s, muscleVas->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("MuscleTissue_Na", timeStandard_s, muscleTis->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("MuscleTissue_K", timeStandard_s, muscleTis->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("MuscleTissue_Cl", timeStandard_s, muscleTis->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("SkinVascular_Na", timeStandard_s, skinVas->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("SkinVascular_K", timeStandard_s, skinVas->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("SkinVascular_Cl", timeStandard_s, skinVas->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("SkinTissue_Na", timeStandard_s, skinTis->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("SkinTissue_K", timeStandard_s, skinTis->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    standardTrk.Track("SkinTissue_Cl", timeStandard_s, skinTis->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));

    timeStandard_s += timestep_s;

  }

  //Now reset the compartment concentrations for matrix test
  liverVas->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(145.0, MassPerVolumeUnit::mg_Per_L);
  liverVas->GetSubstanceQuantity(*K)->GetConcentration().SetValue(4.5, MassPerVolumeUnit::mg_Per_L);
  liverVas->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(105.0, MassPerVolumeUnit::mg_Per_L);
  liverTis->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(20.0, MassPerVolumeUnit::mg_Per_L);
  liverTis->GetSubstanceQuantity(*K)->GetConcentration().SetValue(120.0, MassPerVolumeUnit::mg_Per_L);
  liverTis->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(15.0, MassPerVolumeUnit::mg_Per_L);

  skinVas->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(50.0, MassPerVolumeUnit::mg_Per_L);
  skinVas->GetSubstanceQuantity(*K)->GetConcentration().SetValue(40.0, MassPerVolumeUnit::mg_Per_L);
  skinVas->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(10.0, MassPerVolumeUnit::mg_Per_L);
  skinTis->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(100.0, MassPerVolumeUnit::mg_Per_L);
  skinTis->GetSubstanceQuantity(*K)->GetConcentration().SetValue(20.0, MassPerVolumeUnit::mg_Per_L);
  skinTis->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(200.0, MassPerVolumeUnit::mg_Per_L);

  muscleVas->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(5.0, MassPerVolumeUnit::mg_Per_L);
  muscleVas->GetSubstanceQuantity(*K)->GetConcentration().SetValue(80.0, MassPerVolumeUnit::mg_Per_L);
  muscleVas->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(500.0, MassPerVolumeUnit::mg_Per_L);
  muscleTis->GetSubstanceQuantity(*Na)->GetConcentration().SetValue(75.0, MassPerVolumeUnit::mg_Per_L);
  muscleTis->GetSubstanceQuantity(*K)->GetConcentration().SetValue(70.0, MassPerVolumeUnit::mg_Per_L);
  muscleTis->GetSubstanceQuantity(*Cl)->GetConcentration().SetValue(1.0, MassPerVolumeUnit::mg_Per_L);

  liverVas->Balance(BalanceLiquidBy::Concentration);
  liverTis->Balance(BalanceLiquidBy::Concentration);
  skinVas->Balance(BalanceLiquidBy::Concentration);
  skinTis->Balance(BalanceLiquidBy::Concentration);
  muscleVas->Balance(BalanceLiquidBy::Concentration);
  muscleTis->Balance(BalanceLiquidBy::Concentration);

  //Make matrices to hold concentrations--3x3 structered [Na, K, Cl] x [Liver, Skin, Muscle]
  //This probably is not the most efficient way to initialize these matrices but we'll worry about that later
  Eigen::Matrix3d vascular_mM;
  Eigen::Matrix3d tissue_mM;

  vascular_mM << 145.0, 50.0, 5.0, 4.5, 40.0, 80.0, 105.0, 10.0, 500.0;
  tissue_mM << 20.0, 100.0, 75.0, 120.0, 20.0, 70.0, 15.0, 200.0, 1.0;

  //Make a matrix to cache permeability coefficients and populate it so that pCoeff for Na, K, Cl are on diagonal in that order
  Eigen::Matrix3d pCoeff = Eigen::Matrix3d::Zero();
  for (int i = 0; i < subs.size(); i++) {
    double molarMass_g_Per_mol = subs[i]->GetMolarMass(MassPerAmountUnit::g_Per_mol);
    double molecularRadius_nm = 0.0348 * std::pow(molarMass_g_Per_mol, 0.4175);
    double vToECpermeabilityCoefficient_mL_Per_s_g = 0.0287 * std::pow(molecularRadius_nm, -2.920) / 100.0; // This is only valid if the molecular radius is > 1.0 nm.
    if (molecularRadius_nm < 1.0)
      vToECpermeabilityCoefficient_mL_Per_s_g = 0.0184 * std::pow(molecularRadius_nm, -1.223) / 100.0;

    // Multiply by tissue mass to get the tissue-dependent coefficient.
    double tissueMass = 0.1; //Just assume this is the same for all of them--1000x less than assumed in standard loop above to account for mg to ug conversion
    double vToECpermeabilityCoefficient_mL_Per_s = vToECpermeabilityCoefficient_mL_Per_s_g * tissueMass;

    pCoeff(i, i) = vToECpermeabilityCoefficient_mL_Per_s;
  }

  std::vector<std::pair<SELiquidCompartment*, SELiquidCompartment*>> vascularTissuePair; //Need to store like this to preserve order
  vascularTissuePair.emplace_back(std::make_pair(liverVas, liverTis));
  vascularTissuePair.emplace_back(std::make_pair(skinVas, skinTis));
  vascularTissuePair.emplace_back(std::make_pair(muscleVas, muscleTis));

  timerMatrix.reset();
  Eigen::Matrix3d deltaM;
  while (timeMatrix_s < simTime_s) {
    timerMatrix.lap();
    deltaM = timestep_s * pCoeff * (vascular_mM - tissue_mM);
    for (int i = 0; i < vascularTissuePair.size(); i++) {
      for (int j = 0; j < subs.size(); j++) {
        vascularTissuePair[i].first->GetSubstanceQuantity(*subs[j])->GetMass().IncrementValue(-deltaM(j, i), MassUnit::mg);
        vascularTissuePair[i].first->GetSubstanceQuantity(*subs[j])->Balance(BalanceLiquidBy::Mass);
        vascular_mM(j, i) = vascularTissuePair[i].first->GetSubstanceQuantity(*subs[j])->GetConcentration(MassPerVolumeUnit::mg_Per_L);
        vascularTissuePair[i].second->GetSubstanceQuantity(*subs[j])->GetMass().IncrementValue(deltaM(j, i), MassUnit::mg);
        vascularTissuePair[i].second->GetSubstanceQuantity(*subs[j])->Balance(BalanceLiquidBy::Mass);
        tissue_mM(j, i) = vascularTissuePair[i].second->GetSubstanceQuantity(*subs[j])->GetConcentration(MassPerVolumeUnit::mg_Per_L);
      }
    }
    watchMatrix += timerMatrix.lap();

    matrixTrk.Track("CumulativeTime_ms", timeMatrix_s, watchMatrix / 1e6);
    matrixTrk.Track("LiverVascular_Na", timeMatrix_s, liverVas->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("LiverVascular_K", timeMatrix_s, liverVas->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("LiverVascular_Cl", timeMatrix_s, liverVas->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("LiverTissue_Na", timeMatrix_s, liverTis->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("LiverTissue_K", timeMatrix_s, liverTis->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("LiverTissue_Cl", timeMatrix_s, liverTis->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("MuscleVascular_Na", timeMatrix_s, muscleVas->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("MuscleVascular_K", timeMatrix_s, muscleVas->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("MuscleVascular_Cl", timeMatrix_s, muscleVas->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("MuscleTissue_Na", timeMatrix_s, muscleTis->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("MuscleTissue_K", timeMatrix_s, muscleTis->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("MuscleTissue_Cl", timeMatrix_s, muscleTis->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("SkinVascular_Na", timeMatrix_s, skinVas->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("SkinVascular_K", timeMatrix_s, skinVas->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("SkinVascular_Cl", timeMatrix_s, skinVas->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("SkinTissue_Na", timeMatrix_s, skinTis->GetSubstanceQuantity(*Na)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("SkinTissue_K", timeMatrix_s, skinTis->GetSubstanceQuantity(*K)->GetConcentration(MassPerVolumeUnit::mg_Per_L));
    matrixTrk.Track("SkinTissue_Cl", timeMatrix_s, skinTis->GetSubstanceQuantity(*Cl)->GetConcentration(MassPerVolumeUnit::mg_Per_L));

    timeMatrix_s += timestep_s;
  }

  standardTrk.WriteTrackToFile(standardFile.c_str());
  matrixTrk.WriteTrackToFile(matrixFile.c_str());
}
}
