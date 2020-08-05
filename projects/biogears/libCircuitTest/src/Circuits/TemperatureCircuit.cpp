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
#include <biogears/cdm/circuit/SECircuit.h>
#include <biogears/cdm/circuit/SECircuitNode.h>
#include <biogears/cdm/circuit/SECircuitPath.h>
#include <biogears/cdm/circuit/thermal/SEThermalCircuit.h>
#include <biogears/cdm/circuit/thermal/SEThermalCircuitCalculator.h>
#include <biogears/cdm/properties/SEScalarArea.h>
#include <biogears/cdm/properties/SEScalarEnergy.h>
#include <biogears/cdm/properties/SEScalarFraction.h>
#include <biogears/cdm/properties/SEScalarFrequency.h>
#include <biogears/cdm/properties/SEScalarHeatCapacitancePerMass.h>
#include <biogears/cdm/properties/SEScalarHeatResistance.h>
#include <biogears/cdm/properties/SEScalarHeatResistanceArea.h>
#include <biogears/cdm/properties/SEScalarLengthPerTime.h>
#include <biogears/cdm/properties/SEScalarMassPerVolume.h>
#include <biogears/cdm/properties/SEScalarPower.h>
#include <biogears/cdm/properties/SEScalarTemperature.h>
#include <biogears/cdm/properties/SEScalarVolumePerTime.h>
#include <biogears/cdm/substance/SESubstance.h>
#include <biogears/cdm/substance/SESubstanceFraction.h>
#include <biogears/cdm/system/physiology/SERespiratorySystem.h>
#include <biogears/cdm/utils/DataTrack.h>
#include <biogears/engine/Systems/Energy.h>
#include <biogears/engine/test/BioGearsEngineTest.h>

namespace biogears {

//This test uses less elements for the temperature circuit than in engine to test "Lite" capabilities 
void BioGearsEngineTest::LiteThermalCircuitTest(const std::string& sTestDirectory)
{
  //Elements needed for test
  m_Logger->ResetLogFile(sTestDirectory + "/ThermalLite.log");
  std::string resultsFile = sTestDirectory + "/ThermalLite.csv";
  std::string resultsGraph = sTestDirectory + "/ThermalLiteGraph.csv";
  BioGears bg(m_Logger);
  SECircuitManager circuits(m_Logger);
  DataTrack circuitTrk;
  DataTrack graphTrk;
  SEThermalCircuit* thermLite = &circuits.CreateThermalCircuit("ThermLite");
  SEThermalCircuitCalculator calc(HeatCapacitanceUnit::J_Per_K, PowerUnit::W, HeatInductanceUnit::K_s_Per_W, TemperatureUnit::C, EnergyUnit::J, HeatResistanceUnit::K_Per_W, m_Logger);

  //Set up circuit constants
  //Capacitances
  double capCore_J_Per_K;
  double skinMassFraction = 0.09; //0.09 is fraction of mass that the skin takes up in a typical human /cite herman2006physics
  capCore_J_Per_K = ((1.0 - skinMassFraction) * 80.0 * 3490);
  double capSkin_J_per_K = 25011;
  //Resistances
  double skinBloodFlow_m3Persec = 4.97E-06;
  double bloodDensity_kgPerm3 = 1050;
  double bloodSpecificHeat_JPerkgK = 3617;
  double alphaScale = 0.79; //tuning parameter for scaling resistance
  double coreToSkinR_K_Per_W;
  coreToSkinR_K_Per_W = 1.0 / (alphaScale * bloodDensity_kgPerm3 * bloodSpecificHeat_JPerkgK * skinBloodFlow_m3Persec);
  //double skinToExternalR_K_Per_W = 5.0;
  double skinToExternalR_K_Per_W = std::pow(20,5)*std::abs(2.27 / (10.3+(1.67 * (10 ^ -7) * std::pow((297 + 307) / 2, 3)))); //check this first num
  
  std::cout << coreToSkinR_K_Per_W << std::endl;
  std::cout << skinToExternalR_K_Per_W << std::endl;

  //Sources
  double MetabolicHeat_W = 100.0;
  double RespirationHeat_W = 10.0; 
  double ExternalTemp_K = 299.0;

  
  //Circuit Nodes
  SEThermalCircuitNode& Ground1 = thermLite->CreateNode("Ground");
  Ground1.GetTemperature().SetValue(0.0, TemperatureUnit::K);
  Ground1.GetNextTemperature().SetValue(0.0, TemperatureUnit::K);
  thermLite->AddReferenceNode(Ground1);
  SEThermalCircuitNode& Ground2 = thermLite->CreateNode("Ground");
  Ground2.GetTemperature().SetValue(0.0, TemperatureUnit::K);
  Ground2.GetNextTemperature().SetValue(0.0, TemperatureUnit::K);
  thermLite->AddReferenceNode(Ground2);
  SEThermalCircuitNode& Ground3 = thermLite->CreateNode("Ground");
  Ground3.GetTemperature().SetValue(0.0, TemperatureUnit::K);
  Ground3.GetNextTemperature().SetValue(0.0, TemperatureUnit::K);
  thermLite->AddReferenceNode(Ground3);
  SEThermalCircuitNode& Core = thermLite->CreateNode("Core");
  Core.GetTemperature().SetValue(310.0, TemperatureUnit::K);
  SEThermalCircuitNode& Skin = thermLite->CreateNode("Skin");
  Skin.GetTemperature().SetValue(307.0, TemperatureUnit::K);
  SEThermalCircuitNode& Environment = thermLite->CreateNode("Environment");
  Environment.GetTemperature().SetValue(299, TemperatureUnit::K);
  SEThermalCircuitNode& Ref = thermLite->CreateNode("Ref");
  Ref.GetTemperature().SetValue(0.0, TemperatureUnit::K);
  Ref.GetNextTemperature().SetValue(0.0, TemperatureUnit::K);
  thermLite->AddReferenceNode(Ref);

  //Pathways
  SEThermalCircuitPath& ground1ToCore = thermLite->CreatePath(Ground1, Core, "Ground1ToCore");
  ground1ToCore.GetHeatSourceBaseline().SetValue(MetabolicHeat_W, PowerUnit::W);
  SEThermalCircuitPath& coreToGround2 = thermLite->CreatePath(Core, Ground2, "CoreToGround2");
  coreToGround2.GetCapacitanceBaseline().SetValue(capCore_J_Per_K, HeatCapacitanceUnit::J_Per_K);
  SEThermalCircuitPath& coreToSkin = thermLite->CreatePath(Core, Skin, "CoreToSkin");
  coreToSkin.GetResistanceBaseline().SetValue(coreToSkinR_K_Per_W, HeatResistanceUnit::K_Per_W);
  SEThermalCircuitPath& skinToGround3 = thermLite->CreatePath(Skin, Ground3, "SkinToGround3");
  skinToGround3.GetCapacitanceBaseline().SetValue(capSkin_J_per_K, HeatCapacitanceUnit::J_Per_K);
  SEThermalCircuitPath& refToExternal = thermLite->CreatePath(Ref, Environment, "ExternalToRef");
  refToExternal.GetTemperatureSourceBaseline().SetValue(ExternalTemp_K, TemperatureUnit::K);
  SEThermalCircuitPath& externalToSkin = thermLite->CreatePath(Environment, Skin, "ExternalToSkin");
  externalToSkin.GetResistanceBaseline().SetValue(skinToExternalR_K_Per_W, HeatResistanceUnit::K_Per_W);
  SEThermalCircuitPath& coreToRef = thermLite->CreatePath(Core, Ref, "CoreToRef");
  coreToRef.GetHeatSourceBaseline().SetValue(RespirationHeat_W, PowerUnit::W);
  //coreToRef.GetResistanceBaseline().SetValue(10, HeatResistanceUnit::K_Per_W);



  thermLite->SetNextAndCurrentFromBaselines();
  thermLite->StateChange();

  //Set up compartments
  /*
  SEThermalCompartmentGraph* liteGraph = &bg.GetCompartments().Create("LiteGraph");
  liteGraph->Clear();
  liteGraph->StateChange();
  */

  SEThermalCompartment& cGround1 = bg.GetCompartments().CreateThermalCompartment("cGround1");
  cGround1.MapNode(Ground1);
  SEThermalCompartment& cGround2 = bg.GetCompartments().CreateThermalCompartment("cGround2");
  cGround2.MapNode(Ground2);
  SEThermalCompartment& cGround3 = bg.GetCompartments().CreateThermalCompartment("cGround3");
  cGround3.MapNode(Ground3);
  SEThermalCompartment& cCore = bg.GetCompartments().CreateThermalCompartment("cCore");
  cCore.MapNode(Core);
  SEThermalCompartment& cSkin = bg.GetCompartments().CreateThermalCompartment("cSkin");
  cSkin.MapNode(Skin);
  SEThermalCompartment& cEnvironment = bg.GetCompartments().CreateThermalCompartment("cEnvironment");
  cEnvironment.MapNode(Environment);
  SEThermalCompartment& cRef = bg.GetCompartments().CreateThermalCompartment("cRef");
  cRef.MapNode(Ref);

  SEThermalCompartmentLink& lGround1ToCore = bg.GetCompartments().CreateThermalLink(cGround1, cCore, "lGround1ToCore");
  lGround1ToCore.MapPath(ground1ToCore);
  SEThermalCompartmentLink& lCoreToGround2 = bg.GetCompartments().CreateThermalLink(cCore, cGround2, "lCoreToGround2");
  lCoreToGround2.MapPath(coreToGround2);
  SEThermalCompartmentLink& lCoreToSkin = bg.GetCompartments().CreateThermalLink(cCore, cSkin, "lCoreToSkin");
  lCoreToSkin.MapPath(coreToSkin);
  SEThermalCompartmentLink& lSkinToGround3 = bg.GetCompartments().CreateThermalLink(cSkin, cGround3, "lSkinToGround3");
  lSkinToGround3.MapPath(skinToGround3);
  SEThermalCompartmentLink& lRefToEnvironment = bg.GetCompartments().CreateThermalLink(cRef, cEnvironment, "lEnvironmentToRef");
  lRefToEnvironment.MapPath(refToExternal);
  SEThermalCompartmentLink& lEnvironmentToSkin = bg.GetCompartments().CreateThermalLink(cEnvironment, cSkin, "lEnvironmentToSkin");
  lEnvironmentToSkin.MapPath(externalToSkin);
  SEThermalCompartmentLink& lCoreToRef = bg.GetCompartments().CreateThermalLink(cCore, cRef, "lCoreToRef");
  lCoreToRef.MapPath(coreToRef);

  double nextDriverTemperature_C = 23.85;
  double nextDriverTemperature_K = Convert(nextDriverTemperature_C, TemperatureUnit::C, TemperatureUnit::K);
  double nextDriverAdjustment = ((0.00001 * std::pow(nextDriverTemperature_C, 2.0)) - (0.0042 * nextDriverTemperature_C) + 2.0016); // Should not go above or below +/- 100 Celsius
  //std::cout << nextDriverAdjustment << std::endl;
  double nextDriverTemperature_adjusted = nextDriverAdjustment * nextDriverTemperature_K;
  double timeStep_s = 0.02;
  double simTime_min = 0.25;
  double currentTime_s = 0.0;

  for (int i = 0; i < simTime_min * 60.0 / timeStep_s; i++) {
    refToExternal.GetNextTemperatureSource().SetValue(nextDriverTemperature_adjusted, TemperatureUnit::K);
    calc.Process(*thermLite, timeStep_s);
    calc.PostProcess(*thermLite);
    circuitTrk.Track(currentTime_s, *thermLite);
    circuitTrk.Track("DriveInput", currentTime_s, nextDriverTemperature_adjusted);
    //graphTrk.Track(currentTime_s, *liteGraph);
    currentTime_s += timeStep_s;
  }

  circuitTrk.WriteTrackToFile(resultsFile.c_str());
  std::cout << "Success" << std::endl;
  
}

}