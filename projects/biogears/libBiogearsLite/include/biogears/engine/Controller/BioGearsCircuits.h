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

#include <biogears/cdm/circuit/SECircuitManager.h>

#include <biogears/schema/cdm/Properties.hxx>

namespace biogears {
class BioGears;
/**
* @brief Manages all circuits associated with all %BioGears systems/equipement
*/

class BIOGEARS_API BioGearsCircuits : public SECircuitManager {
public:
  BioGearsCircuits(BioGears& data);
  virtual ~BioGearsCircuits();
  void Clear();

  virtual bool Load(const CDM::CircuitManagerData& in);

  virtual void SetReadOnlyFluid(bool b);

  // Active methods return the circuit configuration to be calculated
  SEFluidCircuit& GetActiveCardiovascularCircuit();
  SEFluidCircuit& GetCardiovascularCircuit();
  SEFluidCircuit& GetRenalCircuit();

  SEFluidCircuit& GetActiveRespiratoryCircuit();
  SEFluidCircuit& GetRespiratoryCircuit();
  SEFluidCircuit& GetRespiratoryAndAnesthesiaMachineCircuit();
  SEFluidCircuit& GetRespiratoryAndInhalerCircuit();
  SEFluidCircuit& GetRespiratoryAndMechanicalVentilatorCircuit();

  SEThermalCircuit& GetTemperatureCircuit();
  SEThermalCircuit& GetExternalTemperatureCircuit();
  SEThermalCircuit& GetInternalTemperatureCircuit();

  SEFluidCircuit& GetAnesthesiaMachineCircuit();

protected:
  BioGears& m_data;

  // Note, I am not making a tissue or inhaler standalone circuit
  // this are super simple circuits and can't be tested by themselves

  // The combined cardiovascular circuit only needs to be built once,
  // but we can build it different ways, with or without renal and tissue subcircuits.
  // It depends on how the engine is configured, but this configuration does not change
  // once it's set. Tests can also configure how it's built and test appropriately.

  SEFluidCircuit* m_CombinedCardiovascularCircuit;

  SEFluidCircuit* m_CardiovascularCircuit;
  SEFluidCircuit* m_RenalCircuit;

  SEFluidCircuit* m_RespiratoryCircuit;

  SEFluidCircuit* m_AnesthesiaMachineCircuit;

  // Based on if equipment is hooked up, we have to build out the combination of
  // these 2 circuits and graphs as we don't want to dynamically modify circuits
  // It's quicker and easier to test these combiniation circuits
  SEFluidCircuit* m_CombinedRespiratoryAnesthesiaCircuit;
  SEFluidCircuit* m_CombinedRespiratoryInhalerCircuit;
  SEFluidCircuit* m_CombinedRespiratoryMechanicalVentilatorCircuit;

  SEThermalCircuit* m_TemperatureCircuit;
  SEThermalCircuit* m_InternalTemperatureCircuit; // Attempt at circuit independence
  SEThermalCircuit* m_ExternalTemperatureCircuit; // Attempt at circuit independence
};
}
///////////////////////////////
// Respiratory Circuit Enums //
///////////////////////////////

namespace mil {
namespace tatrc {
  namespace physiology {
    namespace biogears {

      namespace Circuits {

        DEFINE_STATIC_STRING(FullCardiovascular);
        DEFINE_STATIC_STRING(Cardiovascular);
        DEFINE_STATIC_STRING(Renal);
        DEFINE_STATIC_STRING(Respiratory);
        DEFINE_STATIC_STRING(AnesthesiaMachine);
        DEFINE_STATIC_STRING(RespiratoryAnesthesia);
        DEFINE_STATIC_STRING(RespiratoryInhaler);
        DEFINE_STATIC_STRING(RespiratoryMechanicalVentilator);
        DEFINE_STATIC_STRING(Temperature);
        DEFINE_STATIC_STRING(InternalTemperature);
        DEFINE_STATIC_STRING(ExternalTemperature);
      };

      ///////////////////////////
      // Chyme Circuit Enums //
      ///////////////////////////

      namespace ChymeNode {

        DEFINE_STATIC_STRING(SmallIntestineC1);
      };

      namespace ChymePath {

        DEFINE_STATIC_STRING(SmallIntestineC1ToSmallIntestine1);
        DEFINE_STATIC_STRING(GroundToSmallIntestineC1);
        DEFINE_STATIC_STRING(GutE3ToGroundGI);
      };

      namespace RespiratoryNode {
        DEFINE_STATIC_STRING(Mouth);
        DEFINE_STATIC_STRING(Trachea);
        DEFINE_STATIC_STRING(Bronchi);
        DEFINE_STATIC_STRING(Alveoli);
        DEFINE_STATIC_STRING(AlveoliLeak);
        DEFINE_STATIC_STRING(PleuralConnection);
        DEFINE_STATIC_STRING(Pleural);
        DEFINE_STATIC_STRING(RespiratoryMuscle);
      };

      namespace RespiratoryPath {
        DEFINE_STATIC_STRING(EnvironmentToMouth);
        DEFINE_STATIC_STRING(MouthToTrachea);
        DEFINE_STATIC_STRING(TracheaToBronchi);
        DEFINE_STATIC_STRING(TracheaToPleuralConnection);
        DEFINE_STATIC_STRING(BronchiToAlveoli);
        DEFINE_STATIC_STRING(BronchiToPleuralConnection);
        DEFINE_STATIC_STRING(AlveoliToPleuralConnection);
        DEFINE_STATIC_STRING(PleuralConnectionToPleural);
        DEFINE_STATIC_STRING(PleuralToRespiratoryMuscle);
        DEFINE_STATIC_STRING(RespiratoryMuscleDriver);
        DEFINE_STATIC_STRING(AlveoliToAlveoliLeak);
        DEFINE_STATIC_STRING(AlveoliLeakToPleural);
        DEFINE_STATIC_STRING(PleuralToEnvironment);
      };

      //////////////////////////////////////
      // Anesthesia Machine Circuit Enums //
      //////////////////////////////////////

      namespace AnesthesiaMachineNode {

        DEFINE_STATIC_STRING(AnesthesiaConnection);
        DEFINE_STATIC_STRING(ExpiratoryLimb);
        DEFINE_STATIC_STRING(GasInlet);
        DEFINE_STATIC_STRING(GasSource);
        DEFINE_STATIC_STRING(InspiratoryLimb);
        DEFINE_STATIC_STRING(ReliefValve);
        DEFINE_STATIC_STRING(Scrubber);
        DEFINE_STATIC_STRING(Selector);
        DEFINE_STATIC_STRING(Ventilator);
        DEFINE_STATIC_STRING(VentilatorConnection);
        DEFINE_STATIC_STRING(YPiece);
      };

      namespace AnesthesiaMachinePath {

        DEFINE_STATIC_STRING(EnvironmentToVentilator);
        DEFINE_STATIC_STRING(EnvironmentToReliefValve);
        DEFINE_STATIC_STRING(VentilatorToVentilatorConnection);
        DEFINE_STATIC_STRING(VentilatorConnectionToSelector);
        DEFINE_STATIC_STRING(SelectorToReliefValve);
        DEFINE_STATIC_STRING(SelectorToScrubber);
        DEFINE_STATIC_STRING(ScrubberToGasInlet);
        DEFINE_STATIC_STRING(EnvironmentToGasSource);
        DEFINE_STATIC_STRING(GasSourceToGasInlet);
        DEFINE_STATIC_STRING(GasInletToInspiratoryLimb);
        DEFINE_STATIC_STRING(InspiratoryLimbToYPiece);
        DEFINE_STATIC_STRING(YPieceToExpiratoryLimb);
        DEFINE_STATIC_STRING(ExpiratoryLimbToSelector);
        DEFINE_STATIC_STRING(YPieceToAnesthesiaConnection);
        DEFINE_STATIC_STRING(AnesthesiaConnectionToEnvironment);
        DEFINE_STATIC_STRING(SelectorToEnvironment);
      };

      namespace CombinedAnesthesiaMachinePath {

        DEFINE_STATIC_STRING(AnesthesiaConnectionToMouth);
        DEFINE_STATIC_STRING(GroundConnection);
      };

      ///////////////////////////
      // Inhaler Circuit Enums //
      ///////////////////////////

      namespace InhalerNode {

        DEFINE_STATIC_STRING(Mouthpiece);
      };

      namespace InhalerPath {

        DEFINE_STATIC_STRING(EnvironmentToMouthpiece);
        DEFINE_STATIC_STRING(MouthpieceToMouth);
      };

      //////////////////////////////////////////
      // Mechanical Ventilator Circuit Enums //
      ////////////////////////////////////////

      namespace MechanicalVentilatorNode {

        DEFINE_STATIC_STRING_EX(Connection, MechanicalVentilatorConnection);
      };

      namespace MechanicalVentilatorPath {

        DEFINE_STATIC_STRING_EX(ConnectionToMouth, MechanicalVentilatorConnectionToMouth);
        DEFINE_STATIC_STRING_EX(GroundToConnection, MechanicalVentilatorGroundToConnection);
      };

      ///////////////////////////////////
      // Environment Gas Circuit Enums //
      ///////////////////////////////////

      namespace EnvironmentNode {

        DEFINE_STATIC_STRING(Ambient);
      };

      ///////////////////////////////////////////
      // External Temperature Circuit Enums //
      ///////////////////////////////////////////

      namespace ExternalTemperatureNode {

        DEFINE_STATIC_STRING(Active);
        DEFINE_STATIC_STRING(Clothing);
        DEFINE_STATIC_STRING(Enclosure);
        DEFINE_STATIC_STRING(ExternalCore);
        DEFINE_STATIC_STRING(ExternalSkin);
        DEFINE_STATIC_STRING(ExternalGround);
        DEFINE_STATIC_STRING(Ambient);
      };

      namespace ExternalTemperaturePath {

        DEFINE_STATIC_STRING(ActiveToClothing);
        DEFINE_STATIC_STRING(ClothingToEnclosure);
        DEFINE_STATIC_STRING(ClothingToEnvironment);
        DEFINE_STATIC_STRING(ExternalCoreToGround);
        DEFINE_STATIC_STRING(GroundToActive);
        DEFINE_STATIC_STRING(GroundToClothing);
        DEFINE_STATIC_STRING(GroundToEnclosure);
        DEFINE_STATIC_STRING(GroundToEnvironment);
        DEFINE_STATIC_STRING(ExternalSkinToGround);
        DEFINE_STATIC_STRING(ExternalSkinToClothing);
      };

      ////////////////////////////////////////
      // Internal Temperature Circuit Enums //
      ////////////////////////////////////////

      namespace InternalTemperatureNode {

        DEFINE_STATIC_STRING(InternalCore);
        DEFINE_STATIC_STRING(InternalSkin);
        DEFINE_STATIC_STRING(InternalGround);
      };

      namespace InternalTemperaturePath {

        DEFINE_STATIC_STRING(GroundToInternalCore);
        DEFINE_STATIC_STRING(InternalCoreToInternalSkin);
        DEFINE_STATIC_STRING(InternalCoreToGround);
        DEFINE_STATIC_STRING(InternalSkinToGround);
      };

      ////////////////////////////////////////
      // Thermal Lite Circuit Enums //
      ////////////////////////////////////////

      namespace ThermalLiteNode {

        DEFINE_STATIC_STRING(Core);
        DEFINE_STATIC_STRING(Skin);
        DEFINE_STATIC_STRING(Environment);
        DEFINE_STATIC_STRING(Ground);
        DEFINE_STATIC_STRING(Ref);
      };

      namespace ThermalLitePath {

        DEFINE_STATIC_STRING(CoreToRef); //Respiration
        DEFINE_STATIC_STRING(RefToEnvironment); //TempSource
        DEFINE_STATIC_STRING(EnvironmentToSkin); //Resistor
        DEFINE_STATIC_STRING(GroundToCore); //Metabolism
        DEFINE_STATIC_STRING(CoreToSkin); //Resistor
        DEFINE_STATIC_STRING(CoreToGround); //Capacitor
        DEFINE_STATIC_STRING(SkinToGround); //Capacitor
      };

      ////////////////////////////////////////
      // Combined Temperature Circuit Enums //
      ////////////////////////////////////////

      namespace CombinedTemperaturePath {

        DEFINE_STATIC_STRING(InternalCoreToExternalCore);
        DEFINE_STATIC_STRING(InternalSkinToExternalSkin);
      };

      namespace CardiovascularNode {

        DEFINE_STATIC_STRING(RightHeart1);
        DEFINE_STATIC_STRING(RightHeart2);
        DEFINE_STATIC_STRING(RightHeart3);

        DEFINE_STATIC_STRING(MainPulmonaryArteries);
        DEFINE_STATIC_STRING(LeftIntermediatePulmonaryArteries);
        DEFINE_STATIC_STRING(LeftPulmonaryArteries);
        DEFINE_STATIC_STRING(RightIntermediatePulmonaryArteries);
        DEFINE_STATIC_STRING(RightPulmonaryArteries);

        DEFINE_STATIC_STRING(LeftPulmonaryCapillaries);
        DEFINE_STATIC_STRING(RightPulmonaryCapillaries);

        DEFINE_STATIC_STRING(LeftIntermediatePulmonaryVeins);
        DEFINE_STATIC_STRING(LeftPulmonaryVeins);
        DEFINE_STATIC_STRING(RightIntermediatePulmonaryVeins);
        DEFINE_STATIC_STRING(RightPulmonaryVeins);

        DEFINE_STATIC_STRING(LeftHeart1);
        DEFINE_STATIC_STRING(LeftHeart2);
        DEFINE_STATIC_STRING(LeftHeart3);

        DEFINE_STATIC_STRING(Aorta1);
        DEFINE_STATIC_STRING(Aorta2);
        DEFINE_STATIC_STRING(Aorta3);

        DEFINE_STATIC_STRING(Bone1);
        DEFINE_STATIC_STRING(Bone2);

        DEFINE_STATIC_STRING(Brain1);
        DEFINE_STATIC_STRING(Brain2);

        DEFINE_STATIC_STRING(Fat1);
        DEFINE_STATIC_STRING(Fat2);

        DEFINE_STATIC_STRING(LargeIntestine1);

        DEFINE_STATIC_STRING(Liver1);
        DEFINE_STATIC_STRING(Liver2);
        DEFINE_STATIC_STRING(PortalVein1);

        DEFINE_STATIC_STRING(LeftArm1);
        DEFINE_STATIC_STRING(LeftArm2);

        DEFINE_STATIC_STRING(LeftKidney1);
        DEFINE_STATIC_STRING(LeftKidney2);

        DEFINE_STATIC_STRING(LeftLeg1);
        DEFINE_STATIC_STRING(LeftLeg2);

        DEFINE_STATIC_STRING(Muscle1);
        DEFINE_STATIC_STRING(Muscle2);

        DEFINE_STATIC_STRING(Myocardium1);
        DEFINE_STATIC_STRING(Myocardium2);

        DEFINE_STATIC_STRING(Pericardium1);

        DEFINE_STATIC_STRING(RightArm1);
        DEFINE_STATIC_STRING(RightArm2);

        DEFINE_STATIC_STRING(RightKidney1);
        DEFINE_STATIC_STRING(RightKidney2);

        DEFINE_STATIC_STRING(RightLeg1);
        DEFINE_STATIC_STRING(RightLeg2);

        DEFINE_STATIC_STRING(Skin1);
        DEFINE_STATIC_STRING(Skin2);

        DEFINE_STATIC_STRING(SmallIntestine1);

        DEFINE_STATIC_STRING(Splanchnic1);

        DEFINE_STATIC_STRING(Spleen1);

        DEFINE_STATIC_STRING(VenaCava);

        DEFINE_STATIC_STRING(Ground);
      };

      namespace CardiovascularLiteNode {
        DEFINE_STATIC_STRING(RightHeart1);
        DEFINE_STATIC_STRING(RightHeart2);
        DEFINE_STATIC_STRING(RightHeart3);

        DEFINE_STATIC_STRING(MainPulmonaryArteries);
        DEFINE_STATIC_STRING(LeftIntermediatePulmonaryArteries);
        DEFINE_STATIC_STRING(LeftPulmonaryArteries);
        DEFINE_STATIC_STRING(RightIntermediatePulmonaryArteries);
        DEFINE_STATIC_STRING(RightPulmonaryArteries);

        DEFINE_STATIC_STRING(LeftPulmonaryCapillaries);
        DEFINE_STATIC_STRING(RightPulmonaryCapillaries);

        DEFINE_STATIC_STRING(LeftIntermediatePulmonaryVeins);
        DEFINE_STATIC_STRING(LeftPulmonaryVeins);
        DEFINE_STATIC_STRING(RightIntermediatePulmonaryVeins);
        DEFINE_STATIC_STRING(RightPulmonaryVeins);

        DEFINE_STATIC_STRING(LeftHeart1);
        DEFINE_STATIC_STRING(LeftHeart2);
        DEFINE_STATIC_STRING(LeftHeart3);

        DEFINE_STATIC_STRING(Aorta1);
        DEFINE_STATIC_STRING(Aorta2);
        DEFINE_STATIC_STRING(Aorta3);

        DEFINE_STATIC_STRING(Arms1);
        DEFINE_STATIC_STRING(Arms2);

        DEFINE_STATIC_STRING(Bone1);
        DEFINE_STATIC_STRING(Bone2);

        DEFINE_STATIC_STRING(Brain1);
        DEFINE_STATIC_STRING(Brain2);

        DEFINE_STATIC_STRING(Fat1);
        DEFINE_STATIC_STRING(Fat2);

        DEFINE_STATIC_STRING(Gut1);

        DEFINE_STATIC_STRING(Liver1);
        DEFINE_STATIC_STRING(Liver2);
        DEFINE_STATIC_STRING(PortalVein1);

        DEFINE_STATIC_STRING(LeftKidney1);
        DEFINE_STATIC_STRING(LeftKidney2);

        DEFINE_STATIC_STRING(Legs1);
        DEFINE_STATIC_STRING(Legs2);

        DEFINE_STATIC_STRING(Muscle1);
        DEFINE_STATIC_STRING(Muscle2);

        DEFINE_STATIC_STRING(Myocardium1);
        DEFINE_STATIC_STRING(Myocardium2);

        DEFINE_STATIC_STRING(Pericardium1);

        DEFINE_STATIC_STRING(RightKidney1);
        DEFINE_STATIC_STRING(RightKidney2);

        DEFINE_STATIC_STRING(Skin1);
        DEFINE_STATIC_STRING(Skin2);

        DEFINE_STATIC_STRING(VenaCava);

        DEFINE_STATIC_STRING(Ground);
      };

      namespace CardiovascularPath {

        // Heart and Lungs
        DEFINE_STATIC_STRING(VenaCavaToRightHeart2);
        DEFINE_STATIC_STRING(RightHeart2ToRightHeart1);
        DEFINE_STATIC_STRING(RightHeart1ToRightHeart3);
        DEFINE_STATIC_STRING(RightHeart3ToGround);
        DEFINE_STATIC_STRING(RightHeart1ToMainPulmonaryArteries);
        DEFINE_STATIC_STRING(MainPulmonaryArteriesToRightIntermediatePulmonaryArteries);
        DEFINE_STATIC_STRING(RightIntermediatePulmonaryArteriesToRightPulmonaryArteries);
        DEFINE_STATIC_STRING(RightPulmonaryArteriesToRightPulmonaryVeins);
        DEFINE_STATIC_STRING(RightPulmonaryArteriesToRightPulmonaryCapillaries);
        DEFINE_STATIC_STRING(RightPulmonaryArteriesToGround);
        DEFINE_STATIC_STRING(RightPulmonaryCapillariesToRightPulmonaryVeins);
        DEFINE_STATIC_STRING(RightPulmonaryCapillariesToGround);
        DEFINE_STATIC_STRING(RightPulmonaryVeinsToRightIntermediatePulmonaryVeins);
        DEFINE_STATIC_STRING(RightPulmonaryVeinsToGround);
        DEFINE_STATIC_STRING(RightIntermediatePulmonaryVeinsToLeftHeart2);
        DEFINE_STATIC_STRING(MainPulmonaryArteriesToLeftIntermediatePulmonaryArteries);
        DEFINE_STATIC_STRING(LeftIntermediatePulmonaryArteriesToLeftPulmonaryArteries);
        DEFINE_STATIC_STRING(LeftPulmonaryArteriesToLeftPulmonaryVeins);
        DEFINE_STATIC_STRING(LeftPulmonaryArteriesToLeftPulmonaryCapillaries);
        DEFINE_STATIC_STRING(LeftPulmonaryArteriesToGround);
        DEFINE_STATIC_STRING(LeftPulmonaryCapillariesToGround);
        DEFINE_STATIC_STRING(LeftPulmonaryCapillariesToLeftPulmonaryVeins);
        DEFINE_STATIC_STRING(LeftPulmonaryVeinsToLeftIntermediatePulmonaryVeins);
        DEFINE_STATIC_STRING(LeftPulmonaryVeinsToGround);
        DEFINE_STATIC_STRING(LeftIntermediatePulmonaryVeinsToLeftHeart2)
        DEFINE_STATIC_STRING(LeftHeart2ToLeftHeart1);
        DEFINE_STATIC_STRING(LeftHeart1ToLeftHeart3);
        DEFINE_STATIC_STRING(LeftHeart3ToGround);
        DEFINE_STATIC_STRING(LeftHeart1ToAorta2);
        DEFINE_STATIC_STRING(Aorta2ToAorta3);
        DEFINE_STATIC_STRING(Aorta3ToAorta1);
        DEFINE_STATIC_STRING(Aorta1ToGround);
        // Brain
        DEFINE_STATIC_STRING(Aorta1ToBrain1);
        DEFINE_STATIC_STRING(Brain1ToGround);
        DEFINE_STATIC_STRING(Brain1ToBrain2);
        DEFINE_STATIC_STRING(Brain2ToVenaCava);
        // Bone
        DEFINE_STATIC_STRING(Aorta1ToBone1);
        DEFINE_STATIC_STRING(Bone1ToGround);
        DEFINE_STATIC_STRING(Bone1ToBone2);
        DEFINE_STATIC_STRING(Bone2ToVenaCava);
        // Fat
        DEFINE_STATIC_STRING(Aorta1ToFat1);
        DEFINE_STATIC_STRING(Fat1ToGround);
        DEFINE_STATIC_STRING(Fat1ToFat2);
        DEFINE_STATIC_STRING(Fat2ToVenaCava);
        // Large Intestine
        DEFINE_STATIC_STRING(Aorta1ToLargeIntestine);
        DEFINE_STATIC_STRING(LargeIntestineToGround);
        DEFINE_STATIC_STRING(LargeIntestineToPortalVein);
        // Left Arm
        DEFINE_STATIC_STRING(Aorta1ToLeftArm1);
        DEFINE_STATIC_STRING(LeftArm1ToGround);
        DEFINE_STATIC_STRING(LeftArm1ToLeftArm2);
        DEFINE_STATIC_STRING(LeftArm2ToVenaCava);
        // Left Kidney
        DEFINE_STATIC_STRING(Aorta1ToLeftKidney1);
        DEFINE_STATIC_STRING(LeftKidney1ToGround);
        DEFINE_STATIC_STRING(LeftKidney1ToLeftKidney2);
        DEFINE_STATIC_STRING(LeftKidney2ToVenaCava);
        // Left Leg
        DEFINE_STATIC_STRING(Aorta1ToLeftLeg1);
        DEFINE_STATIC_STRING(LeftLeg1ToGround);
        DEFINE_STATIC_STRING(LeftLeg1ToLeftLeg2);
        DEFINE_STATIC_STRING(LeftLeg2ToVenaCava);
        // Liver
        DEFINE_STATIC_STRING(Aorta1ToLiver1);
        DEFINE_STATIC_STRING(Liver1ToGround);
        DEFINE_STATIC_STRING(PortalVeinToLiver1);
        DEFINE_STATIC_STRING(Liver1ToLiver2);
        DEFINE_STATIC_STRING(Liver2ToVenaCava);
        // Muscle
        DEFINE_STATIC_STRING(Aorta1ToMuscle1);
        DEFINE_STATIC_STRING(Muscle1ToGround);
        DEFINE_STATIC_STRING(Muscle1ToMuscle2);
        DEFINE_STATIC_STRING(Muscle2ToVenaCava);
        // Myocardium
        DEFINE_STATIC_STRING(Aorta1ToMyocardium1);
        DEFINE_STATIC_STRING(Myocardium1ToGround);
        DEFINE_STATIC_STRING(Myocardium1ToMyocardium2);
        DEFINE_STATIC_STRING(Myocardium2ToVenaCava);
        // Pericardium
        DEFINE_STATIC_STRING(Pericardium1ToGround);
        DEFINE_STATIC_STRING(GroundToPericardium1);
        // Right Arm
        DEFINE_STATIC_STRING(Aorta1ToRightArm1);
        DEFINE_STATIC_STRING(RightArm1ToGround);
        DEFINE_STATIC_STRING(RightArm1ToRightArm2);
        DEFINE_STATIC_STRING(RightArm2ToVenaCava);
        // Right Kidney
        DEFINE_STATIC_STRING(Aorta1ToRightKidney1);
        DEFINE_STATIC_STRING(RightKidney1ToGround);
        DEFINE_STATIC_STRING(RightKidney1ToRightKidney2);
        DEFINE_STATIC_STRING(RightKidney2ToVenaCava);
        // Kidney
        DEFINE_STATIC_STRING(Aorta1ToKidney);
        DEFINE_STATIC_STRING(KidneyToVenaCava);
        // Right Leg
        DEFINE_STATIC_STRING(Aorta1ToRightLeg1);
        DEFINE_STATIC_STRING(RightLeg1ToGround);
        DEFINE_STATIC_STRING(RightLeg1ToRightLeg2);
        DEFINE_STATIC_STRING(RightLeg2ToVenaCava);
        // Skin
        DEFINE_STATIC_STRING(Aorta1ToSkin1);
        DEFINE_STATIC_STRING(Skin1ToGround);
        DEFINE_STATIC_STRING(Skin1ToSkin2);
        DEFINE_STATIC_STRING(Skin2ToVenaCava);
        // Small Intestine
        DEFINE_STATIC_STRING(Aorta1ToSmallIntestine);
        DEFINE_STATIC_STRING(SmallIntestineToGround);
        DEFINE_STATIC_STRING(SmallIntestineToPortalVein);
        // Splanchnic
        DEFINE_STATIC_STRING(Aorta1ToSplanchnic);
        DEFINE_STATIC_STRING(SplanchnicToGround);
        DEFINE_STATIC_STRING(SplanchnicToPortalVein);
        // Spleen
        DEFINE_STATIC_STRING(Aorta1ToSpleen);
        DEFINE_STATIC_STRING(SpleenToGround);
        DEFINE_STATIC_STRING(SpleenToPortalVein);
        // Vena Cava
        DEFINE_STATIC_STRING(VenaCavaToGround);
        DEFINE_STATIC_STRING(IVToVenaCava);
        //Hemorrhage
        DEFINE_STATIC_STRING(AortaBleed);
        DEFINE_STATIC_STRING(BrainBleed);
        DEFINE_STATIC_STRING(MyocardiumBleed);
        DEFINE_STATIC_STRING(LeftLungBleed);
        DEFINE_STATIC_STRING(RightLungBleed);
        DEFINE_STATIC_STRING(LeftArmBleed);
        DEFINE_STATIC_STRING(RightArmBleed);
        DEFINE_STATIC_STRING(SpleenBleed);
        DEFINE_STATIC_STRING(SmallIntestineBleed);
        DEFINE_STATIC_STRING(LargeIntestineBleed);
        DEFINE_STATIC_STRING(SplanchnicBleed);
        DEFINE_STATIC_STRING(RightKidneyBleed);
        DEFINE_STATIC_STRING(LeftKidneyBleed);
        DEFINE_STATIC_STRING(LiverBleed);
        DEFINE_STATIC_STRING(PortalBleed);
        DEFINE_STATIC_STRING(LeftLegBleed);
        DEFINE_STATIC_STRING(RightLegBleed);
        DEFINE_STATIC_STRING(VenaCavaBleed);
      };

      namespace CardiovascularLitePath {

        DEFINE_STATIC_STRING(KidneyBleed)
        // Heart and Lungs
        DEFINE_STATIC_STRING(VenaCavaToRightHeart2);
        DEFINE_STATIC_STRING(RightHeart2ToRightHeart1);
        DEFINE_STATIC_STRING(RightHeart1ToRightHeart3);
        DEFINE_STATIC_STRING(RightHeart3ToGround);
        DEFINE_STATIC_STRING(RightHeart1ToMainPulmonaryArteries);
        DEFINE_STATIC_STRING(MainPulmonaryArteriesToRightIntermediatePulmonaryArteries);
        DEFINE_STATIC_STRING(RightIntermediatePulmonaryArteriesToRightPulmonaryArteries);
        DEFINE_STATIC_STRING(RightPulmonaryArteriesToRightPulmonaryVeins);
        DEFINE_STATIC_STRING(RightPulmonaryArteriesToRightPulmonaryCapillaries);
        DEFINE_STATIC_STRING(RightPulmonaryArteriesToGround);
        DEFINE_STATIC_STRING(RightPulmonaryCapillariesToRightPulmonaryVeins);
        DEFINE_STATIC_STRING(RightPulmonaryCapillariesToGround);
        DEFINE_STATIC_STRING(RightPulmonaryVeinsToRightIntermediatePulmonaryVeins);
        DEFINE_STATIC_STRING(RightPulmonaryVeinsToGround);
        DEFINE_STATIC_STRING(RightIntermediatePulmonaryVeinsToLeftHeart2);
        DEFINE_STATIC_STRING(MainPulmonaryArteriesToLeftIntermediatePulmonaryArteries);
        DEFINE_STATIC_STRING(LeftIntermediatePulmonaryArteriesToLeftPulmonaryArteries);
        DEFINE_STATIC_STRING(LeftPulmonaryArteriesToLeftPulmonaryVeins);
        DEFINE_STATIC_STRING(LeftPulmonaryArteriesToLeftPulmonaryCapillaries);
        DEFINE_STATIC_STRING(LeftPulmonaryArteriesToGround);
        DEFINE_STATIC_STRING(LeftPulmonaryCapillariesToGround);
        DEFINE_STATIC_STRING(LeftPulmonaryCapillariesToLeftPulmonaryVeins);
        DEFINE_STATIC_STRING(LeftPulmonaryVeinsToLeftIntermediatePulmonaryVeins);
        DEFINE_STATIC_STRING(LeftPulmonaryVeinsToGround);
        DEFINE_STATIC_STRING(LeftIntermediatePulmonaryVeinsToLeftHeart2)
        DEFINE_STATIC_STRING(LeftHeart2ToLeftHeart1);
        DEFINE_STATIC_STRING(LeftHeart1ToLeftHeart3);
        DEFINE_STATIC_STRING(LeftHeart3ToGround);
        DEFINE_STATIC_STRING(LeftHeart1ToAorta2);
        DEFINE_STATIC_STRING(Aorta2ToAorta3);
        DEFINE_STATIC_STRING(Aorta3ToAorta1);
        DEFINE_STATIC_STRING(Aorta1ToGround);
        //Arms
        DEFINE_STATIC_STRING(Aorta1ToArms1);
        DEFINE_STATIC_STRING(Arms1ToGround);
        DEFINE_STATIC_STRING(Arms1ToArms2);
        DEFINE_STATIC_STRING(Arms2ToVenaCava);
        // Brain
        DEFINE_STATIC_STRING(Aorta1ToBrain1);
        DEFINE_STATIC_STRING(Brain1ToGround);
        DEFINE_STATIC_STRING(Brain1ToBrain2);
        DEFINE_STATIC_STRING(Brain2ToVenaCava);
        // Bone
        DEFINE_STATIC_STRING(Aorta1ToBone1);
        DEFINE_STATIC_STRING(Bone1ToGround);
        DEFINE_STATIC_STRING(Bone1ToBone2);
        DEFINE_STATIC_STRING(Bone2ToVenaCava);
        // Fat
        DEFINE_STATIC_STRING(Aorta1ToFat1);
        DEFINE_STATIC_STRING(Fat1ToGround);
        DEFINE_STATIC_STRING(Fat1ToFat2);
        DEFINE_STATIC_STRING(Fat2ToVenaCava);
        // Gut
        DEFINE_STATIC_STRING(Aorta1ToGut);
        DEFINE_STATIC_STRING(GutToGround);
        DEFINE_STATIC_STRING(GutToPortalVein);
        // Left Kidney
        DEFINE_STATIC_STRING(Aorta1ToLeftKidney1);
        DEFINE_STATIC_STRING(LeftKidney1ToGround);
        DEFINE_STATIC_STRING(LeftKidney1ToLeftKidney2);
        DEFINE_STATIC_STRING(LeftKidney2ToVenaCava);
        //Legs
        DEFINE_STATIC_STRING(Aorta1ToLegs1);
        DEFINE_STATIC_STRING(Legs1ToGround);
        DEFINE_STATIC_STRING(Legs1ToLegs2);
        DEFINE_STATIC_STRING(Legs2ToVenaCava);
        // Liver
        DEFINE_STATIC_STRING(Aorta1ToLiver1);
        DEFINE_STATIC_STRING(Liver1ToGround);
        DEFINE_STATIC_STRING(PortalVeinToLiver1);
        DEFINE_STATIC_STRING(Liver1ToLiver2);
        DEFINE_STATIC_STRING(Liver2ToVenaCava);
        // Muscle
        DEFINE_STATIC_STRING(Aorta1ToMuscle1);
        DEFINE_STATIC_STRING(Muscle1ToGround);
        DEFINE_STATIC_STRING(Muscle1ToMuscle2);
        DEFINE_STATIC_STRING(Muscle2ToVenaCava);
        // Myocardium
        DEFINE_STATIC_STRING(Aorta1ToMyocardium1);
        DEFINE_STATIC_STRING(Myocardium1ToGround);
        DEFINE_STATIC_STRING(Myocardium1ToMyocardium2);
        DEFINE_STATIC_STRING(Myocardium2ToVenaCava);
        // Pericardium
        DEFINE_STATIC_STRING(Pericardium1ToGround);
        DEFINE_STATIC_STRING(GroundToPericardium1);
        // Right Kidney
        DEFINE_STATIC_STRING(Aorta1ToRightKidney1);
        DEFINE_STATIC_STRING(RightKidney1ToGround);
        DEFINE_STATIC_STRING(RightKidney1ToRightKidney2);
        DEFINE_STATIC_STRING(RightKidney2ToVenaCava);
        // Skin
        DEFINE_STATIC_STRING(Aorta1ToSkin1);
        DEFINE_STATIC_STRING(Skin1ToGround);
        DEFINE_STATIC_STRING(Skin1ToSkin2);
        DEFINE_STATIC_STRING(Skin2ToVenaCava);
        // Vena Cava
        DEFINE_STATIC_STRING(VenaCavaToGround);
        DEFINE_STATIC_STRING(IVToVenaCava);
        //Hemorrhage
        DEFINE_STATIC_STRING(AortaBleed);
        DEFINE_STATIC_STRING(ArmsBleed);
        DEFINE_STATIC_STRING(LegsBleed);
        DEFINE_STATIC_STRING(GutBleed);
        DEFINE_STATIC_STRING(VenaCavaBleed);

      };

      namespace TissueNode {
        DEFINE_STATIC_STRING(BoneE1);
        DEFINE_STATIC_STRING(BoneE2);
        DEFINE_STATIC_STRING(BoneE3);
        DEFINE_STATIC_STRING(BoneI);
        DEFINE_STATIC_STRING(BoneL);
        DEFINE_STATIC_STRING(BrainE1);
        DEFINE_STATIC_STRING(BrainE2);
        DEFINE_STATIC_STRING(BrainE3);
        DEFINE_STATIC_STRING(BrainI);
        DEFINE_STATIC_STRING(BrainL);
        DEFINE_STATIC_STRING(FatE1);
        DEFINE_STATIC_STRING(FatE2);
        DEFINE_STATIC_STRING(FatE3);
        DEFINE_STATIC_STRING(FatI);
        DEFINE_STATIC_STRING(FatL);
        DEFINE_STATIC_STRING(KidneyE1);
        DEFINE_STATIC_STRING(KidneyE2);
        DEFINE_STATIC_STRING(KidneyE3);
        DEFINE_STATIC_STRING(KidneyI);
        DEFINE_STATIC_STRING(KidneyL);
        DEFINE_STATIC_STRING(LiverE1);
        DEFINE_STATIC_STRING(LiverE2);
        DEFINE_STATIC_STRING(LiverE3);
        DEFINE_STATIC_STRING(LiverI);
        DEFINE_STATIC_STRING(LiverL);
        DEFINE_STATIC_STRING(LungE1);
        DEFINE_STATIC_STRING(LungE2);
        DEFINE_STATIC_STRING(LungE3);
        DEFINE_STATIC_STRING(LungI);
        DEFINE_STATIC_STRING(LungL);
        DEFINE_STATIC_STRING(Lymph)
        DEFINE_STATIC_STRING(MuscleE1);
        DEFINE_STATIC_STRING(MuscleE2);
        DEFINE_STATIC_STRING(MuscleE3)
        DEFINE_STATIC_STRING(MuscleI);
        DEFINE_STATIC_STRING(MuscleL);
        DEFINE_STATIC_STRING(MyocardiumE1);
        DEFINE_STATIC_STRING(MyocardiumE2);
        DEFINE_STATIC_STRING(MyocardiumE3);
        DEFINE_STATIC_STRING(MyocardiumI);
        DEFINE_STATIC_STRING(MyocardiumL);
        //Using Gut to refer to intestines, spleen, and "vascular splanchnic", which is basically the pancreas.
        DEFINE_STATIC_STRING(GutE1);
        DEFINE_STATIC_STRING(GutE2);
        DEFINE_STATIC_STRING(GutE3);
        DEFINE_STATIC_STRING(GutI);
        DEFINE_STATIC_STRING(GutL);
        DEFINE_STATIC_STRING(SkinE1);
        DEFINE_STATIC_STRING(SkinE2);
        DEFINE_STATIC_STRING(SkinE3);
        DEFINE_STATIC_STRING(SkinI);
        DEFINE_STATIC_STRING(SkinL);
      };

      namespace TissuePath {
        DEFINE_STATIC_STRING(BoneVToBoneE1);
        DEFINE_STATIC_STRING(BoneE1ToBoneE2);
        DEFINE_STATIC_STRING(BoneE2ToBoneE3);
        DEFINE_STATIC_STRING(BoneE3ToBoneI);
        DEFINE_STATIC_STRING(BoneE3ToGround);
        DEFINE_STATIC_STRING(BoneIToGround);
        DEFINE_STATIC_STRING(BoneE3ToBoneL);
        DEFINE_STATIC_STRING(BoneLToLymph);

        DEFINE_STATIC_STRING(BrainVToBrainE1);
        DEFINE_STATIC_STRING(BrainE1ToBrainE2);
        DEFINE_STATIC_STRING(BrainE2ToBrainE3);
        DEFINE_STATIC_STRING(BrainE3ToBrainI);
        DEFINE_STATIC_STRING(BrainE3ToGround);
        DEFINE_STATIC_STRING(BrainIToGround);
        DEFINE_STATIC_STRING(BrainE3ToBrainL);
        DEFINE_STATIC_STRING(BrainLToLymph);

        DEFINE_STATIC_STRING(FatVToFatE1);
        DEFINE_STATIC_STRING(FatE1ToFatE2);
        DEFINE_STATIC_STRING(FatE2ToFatE3);
        DEFINE_STATIC_STRING(FatE3ToFatI);
        DEFINE_STATIC_STRING(FatE3ToGround);
        DEFINE_STATIC_STRING(FatIToGround);
        DEFINE_STATIC_STRING(FatE3ToFatL);
        DEFINE_STATIC_STRING(FatLToLymph);

        DEFINE_STATIC_STRING(KidneyVToKidneyE1);
        DEFINE_STATIC_STRING(KidneyE1ToKidneyE2);
        DEFINE_STATIC_STRING(KidneyE2ToKidneyE3);
        DEFINE_STATIC_STRING(KidneyE3ToKidneyI);
        DEFINE_STATIC_STRING(KidneyE3ToGround);
        DEFINE_STATIC_STRING(KidneyIToGround);
        DEFINE_STATIC_STRING(KidneyE3ToKidneyL);
        DEFINE_STATIC_STRING(KidneyLToLymph);

        DEFINE_STATIC_STRING(LiverVToLiverE1);
        DEFINE_STATIC_STRING(LiverE1ToLiverE2);
        DEFINE_STATIC_STRING(LiverE2ToLiverE3);
        DEFINE_STATIC_STRING(LiverE3ToLiverI);
        DEFINE_STATIC_STRING(LiverE3ToGround);
        DEFINE_STATIC_STRING(LiverIToGround);
        DEFINE_STATIC_STRING(LiverE3ToLiverL);
        DEFINE_STATIC_STRING(LiverLToLymph);

        DEFINE_STATIC_STRING(LeftLungVToLungE1);
        DEFINE_STATIC_STRING(RightLungVToLungE1);
        DEFINE_STATIC_STRING(LungE1ToLungE2);
        DEFINE_STATIC_STRING(LungE2ToLungE3);
        DEFINE_STATIC_STRING(LungE3ToLungI);
        DEFINE_STATIC_STRING(LungE3ToGround);
        DEFINE_STATIC_STRING(LungIToGround);
        DEFINE_STATIC_STRING(LungE3ToLungL);
        DEFINE_STATIC_STRING(LungLToLymph);

        DEFINE_STATIC_STRING(LymphToVenaCava);
        DEFINE_STATIC_STRING(LymphToGround);

        DEFINE_STATIC_STRING(MuscleVToMuscleE1);
        DEFINE_STATIC_STRING(MuscleE1ToMuscleE2);
        DEFINE_STATIC_STRING(MuscleE2ToMuscleE3);
        DEFINE_STATIC_STRING(MuscleE3ToMuscleI);
        DEFINE_STATIC_STRING(MuscleE3ToGround);
        DEFINE_STATIC_STRING(MuscleIToGround);
        DEFINE_STATIC_STRING(MuscleE3ToMuscleL);
        DEFINE_STATIC_STRING(MuscleLToLymph);

        DEFINE_STATIC_STRING(MyocardiumVToMyocardiumE1);
        DEFINE_STATIC_STRING(MyocardiumE1ToMyocardiumE2);
        DEFINE_STATIC_STRING(MyocardiumE2ToMyocardiumE3);
        DEFINE_STATIC_STRING(MyocardiumE3ToMyocardiumI);
        DEFINE_STATIC_STRING(MyocardiumE3ToGround);
        DEFINE_STATIC_STRING(MyocardiumIToGround);
        DEFINE_STATIC_STRING(MyocardiumE3ToMyocardiumL);
        DEFINE_STATIC_STRING(MyocardiumLToLymph)

        DEFINE_STATIC_STRING(SkinVToSkinE1);
        DEFINE_STATIC_STRING(SkinE1ToSkinE2);
        DEFINE_STATIC_STRING(SkinE2ToSkinE3);
        DEFINE_STATIC_STRING(SkinE3ToSkinI);
        DEFINE_STATIC_STRING(SkinE3ToGround);
        DEFINE_STATIC_STRING(SkinIToGround);
        DEFINE_STATIC_STRING(SkinE3ToSkinL);
        DEFINE_STATIC_STRING(SkinLToLymph);
        DEFINE_STATIC_STRING(SkinSweating);

        //GutV will replace small, large, spleen, splanchnic due to CV lite
        DEFINE_STATIC_STRING(GutVToGutE1);
        DEFINE_STATIC_STRING(SmallIntestineVToGutE1);
        DEFINE_STATIC_STRING(LargeIntestineVToGutE1);
        DEFINE_STATIC_STRING(SpleenVToGutE1);
        DEFINE_STATIC_STRING(SplanchnicVToGutE1);
        DEFINE_STATIC_STRING(GutE1ToGutE2);
        DEFINE_STATIC_STRING(GutE2ToGutE3);
        DEFINE_STATIC_STRING(GutE3ToGutI);
        DEFINE_STATIC_STRING(GutE3ToGround);
        DEFINE_STATIC_STRING(GutIToGround);
        DEFINE_STATIC_STRING(GutE3ToGutL);
        DEFINE_STATIC_STRING(GutLToLymph)
      }

      namespace RenalNode {

        // Blood
        DEFINE_STATIC_STRING(RightAortaConnection);
        DEFINE_STATIC_STRING(RightRenalArtery);
        DEFINE_STATIC_STRING(RightAfferentArteriole);
        DEFINE_STATIC_STRING(RightGlomerularCapillaries);
        DEFINE_STATIC_STRING(RightNetGlomerularCapillaries);
        DEFINE_STATIC_STRING(RightEfferentArteriole);
        DEFINE_STATIC_STRING(RightPeritubularCapillaries);
        DEFINE_STATIC_STRING(RightNetPeritubularCapillaries);
        DEFINE_STATIC_STRING(RightRenalVein);
        DEFINE_STATIC_STRING(RightVenaCavaConnection);
        // Urine
        DEFINE_STATIC_STRING(RightBowmansCapsules);
        DEFINE_STATIC_STRING(RightNetBowmansCapsules);
        DEFINE_STATIC_STRING(RightTubules);
        DEFINE_STATIC_STRING(RightNetTubules);
        DEFINE_STATIC_STRING(RightUreter);

        // Blood
        DEFINE_STATIC_STRING(LeftAortaConnection);
        DEFINE_STATIC_STRING(LeftRenalArtery);
        DEFINE_STATIC_STRING(LeftAfferentArteriole);
        DEFINE_STATIC_STRING(LeftGlomerularCapillaries);
        DEFINE_STATIC_STRING(LeftNetGlomerularCapillaries);
        DEFINE_STATIC_STRING(LeftEfferentArteriole);
        DEFINE_STATIC_STRING(LeftPeritubularCapillaries);
        DEFINE_STATIC_STRING(LeftNetPeritubularCapillaries);
        DEFINE_STATIC_STRING(LeftRenalVein);
        DEFINE_STATIC_STRING(LeftVenaCavaConnection);
        // Urine
        DEFINE_STATIC_STRING(LeftNetBowmansCapsules);
        DEFINE_STATIC_STRING(LeftBowmansCapsules);
        DEFINE_STATIC_STRING(LeftTubules);
        DEFINE_STATIC_STRING(LeftNetTubules);
        DEFINE_STATIC_STRING(LeftUreter);

        DEFINE_STATIC_STRING(Bladder);

        DEFINE_STATIC_STRING(Ground);
      };

      namespace RenalPath {

        DEFINE_STATIC_STRING(RightAortaConnectionToRenalArtery);
        DEFINE_STATIC_STRING(RightRenalArteryToAfferentArteriole);
        DEFINE_STATIC_STRING(RightRenalArteryCompliance);
        DEFINE_STATIC_STRING(RightAfferentArterioleToGlomerularCapillaries);
        DEFINE_STATIC_STRING(RightGlomerularCapillariesToEfferentArteriole);
        DEFINE_STATIC_STRING(RightGlomerularCapillariesCompliance);
        DEFINE_STATIC_STRING(RightEfferentArterioleToPeritubularCapillaries);
        DEFINE_STATIC_STRING(RightPeritubularCapillariesToRenalVein);
        DEFINE_STATIC_STRING(RightRenalVeinToVenaCavaConnection);
        DEFINE_STATIC_STRING(RightRenalVeinCompliance);

        DEFINE_STATIC_STRING(RightGlomerularCapillariesToNetGlomerularCapillaries);
        DEFINE_STATIC_STRING(RightNetGlomerularCapillariesToNetBowmansCapsules);
        DEFINE_STATIC_STRING(RightBowmansCapsulesToNetBowmansCapsules);
        DEFINE_STATIC_STRING(RightBowmansCapsulesToTubules);
        DEFINE_STATIC_STRING(RightTubulesToUreter);
        DEFINE_STATIC_STRING(RightTubulesToNetTubules);
        DEFINE_STATIC_STRING(RightNetTubulesToNetPeritubularCapillaries);
        DEFINE_STATIC_STRING(RightPeritubularCapillariesToNetPeritubularCapillaries);
        DEFINE_STATIC_STRING(RightUreterToBladder);

        DEFINE_STATIC_STRING(LeftAortaConnectionToRenalArtery);
        DEFINE_STATIC_STRING(LeftRenalArteryToAfferentArteriole);
        DEFINE_STATIC_STRING(LeftRenalArteryCompliance);
        DEFINE_STATIC_STRING(LeftAfferentArterioleToGlomerularCapillaries);
        DEFINE_STATIC_STRING(LeftGlomerularCapillariesToEfferentArteriole);
        DEFINE_STATIC_STRING(LeftGlomerularCapillariesCompliance);
        DEFINE_STATIC_STRING(LeftEfferentArterioleToPeritubularCapillaries);
        DEFINE_STATIC_STRING(LeftPeritubularCapillariesToRenalVein);
        DEFINE_STATIC_STRING(LeftRenalVeinToVenaCavaConnection);
        DEFINE_STATIC_STRING(LeftRenalVeinCompliance);

        DEFINE_STATIC_STRING(LeftGlomerularCapillariesToNetGlomerularCapillaries);
        DEFINE_STATIC_STRING(LeftNetGlomerularCapillariesToNetBowmansCapsules);
        DEFINE_STATIC_STRING(LeftBowmansCapsulesToNetBowmansCapsules);
        DEFINE_STATIC_STRING(LeftBowmansCapsulesToTubules);
        DEFINE_STATIC_STRING(LeftTubulesToUreter);
        DEFINE_STATIC_STRING(LeftTubulesToNetTubules);
        DEFINE_STATIC_STRING(LeftNetTubulesToNetPeritubularCapillaries);
        DEFINE_STATIC_STRING(LeftPeritubularCapillariesToNetPeritubularCapillaries);
        DEFINE_STATIC_STRING(LeftUreterToBladder);

        DEFINE_STATIC_STRING(BladderToGroundPressure);
        DEFINE_STATIC_STRING(BladderToGroundUrinate);
      };

      namespace RenalLiteNode {

        // Blood
        DEFINE_STATIC_STRING(AortaConnection);
        DEFINE_STATIC_STRING(RenalArtery);
        DEFINE_STATIC_STRING(AfferentArteriole);
        DEFINE_STATIC_STRING(GlomerularCapillaries);
        DEFINE_STATIC_STRING(NetGlomerularCapillaries);
        DEFINE_STATIC_STRING(EfferentArteriole);
        DEFINE_STATIC_STRING(PeritubularCapillaries);
        DEFINE_STATIC_STRING(NetPeritubularCapillaries);
        DEFINE_STATIC_STRING(RenalVein);
        DEFINE_STATIC_STRING(VenaCavaConnection);
        // Urine
        DEFINE_STATIC_STRING(BowmansCapsules);
        DEFINE_STATIC_STRING(NetBowmansCapsules);
        DEFINE_STATIC_STRING(Tubules);
        DEFINE_STATIC_STRING(NetTubules);
        DEFINE_STATIC_STRING(Ureter);

        DEFINE_STATIC_STRING(Bladder);

        DEFINE_STATIC_STRING(Ground);
      };

      namespace RenalLitePath {

        DEFINE_STATIC_STRING(AortaConnectionToRenalArtery);
        DEFINE_STATIC_STRING(RenalArteryToAfferentArteriole);
        DEFINE_STATIC_STRING(RenalArteryCompliance);
        DEFINE_STATIC_STRING(AfferentArterioleToGlomerularCapillaries);
        DEFINE_STATIC_STRING(GlomerularCapillariesToEfferentArteriole);
        DEFINE_STATIC_STRING(GlomerularCapillariesCompliance);
        DEFINE_STATIC_STRING(EfferentArterioleToPeritubularCapillaries);
        DEFINE_STATIC_STRING(PeritubularCapillariesToRenalVein);
        DEFINE_STATIC_STRING(RenalVeinToVenaCavaConnection);
        DEFINE_STATIC_STRING(RenalVeinCompliance);

        DEFINE_STATIC_STRING(GlomerularCapillariesToNetGlomerularCapillaries);
        DEFINE_STATIC_STRING(NetGlomerularCapillariesToNetBowmansCapsules);
        DEFINE_STATIC_STRING(BowmansCapsulesToNetBowmansCapsules);
        DEFINE_STATIC_STRING(BowmansCapsulesToTubules);
        DEFINE_STATIC_STRING(TubulesToUreter);
        DEFINE_STATIC_STRING(TubulesToNetTubules);
        DEFINE_STATIC_STRING(NetTubulesToNetPeritubularCapillaries);
        DEFINE_STATIC_STRING(PeritubularCapillariesToNetPeritubularCapillaries);
        DEFINE_STATIC_STRING(UreterToBladder);

        DEFINE_STATIC_STRING(BladderToGroundPressure);
        DEFINE_STATIC_STRING(BladderToGroundUrinate);
      };

      namespace DigestionNode {
        DEFINE_STATIC_STRING(GutChyme);
      };

      namespace DigestionPath {

        DEFINE_STATIC_STRING(GutChymeToSmallIntestineVascular);
      };
    } //namespace biogears
  } //namespace physiology
} //namespace tatric
} //namespace mil
