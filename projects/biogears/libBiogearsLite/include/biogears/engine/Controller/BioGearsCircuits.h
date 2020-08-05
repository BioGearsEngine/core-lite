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
        DEFINE_STATIC_STRING(ChestLeak);
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
        DEFINE_STATIC_STRING(EnvironmentToChestLeak);
        DEFINE_STATIC_STRING(ChestLeakToPleural);

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

      ////////////////////////////////////////
      // ThermalCircuit Enums //
      ////////////////////////////////////////

      namespace ThermalNode {

        DEFINE_STATIC_STRING(Core);
        DEFINE_STATIC_STRING(Skin);
        DEFINE_STATIC_STRING(Environment);
        DEFINE_STATIC_STRING(Ground);
        DEFINE_STATIC_STRING(Ref);
      };

      namespace ThermalPath {

        DEFINE_STATIC_STRING(CoreToRef); //Respiration
        DEFINE_STATIC_STRING(RefToEnvironment); //TempSource
        DEFINE_STATIC_STRING(EnvironmentToSkin); //Resistor
        DEFINE_STATIC_STRING(GroundToCore); //Metabolism
        DEFINE_STATIC_STRING(CoreToSkin); //Resistor
        DEFINE_STATIC_STRING(CoreToGround); //Capacitor
        DEFINE_STATIC_STRING(SkinToGround); //Capacitor
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

        DEFINE_STATIC_STRING(Arms1);
        DEFINE_STATIC_STRING(Arms2);

        DEFINE_STATIC_STRING(Bone1);
        DEFINE_STATIC_STRING(Bone2);

        DEFINE_STATIC_STRING(Brain1);
        DEFINE_STATIC_STRING(Brain2);

        DEFINE_STATIC_STRING(Fat1);
        DEFINE_STATIC_STRING(Fat2);

        DEFINE_STATIC_STRING(Gut1);

        DEFINE_STATIC_STRING(Kidneys1);
        DEFINE_STATIC_STRING(Kidneys2);

        DEFINE_STATIC_STRING(Liver1);
        DEFINE_STATIC_STRING(Liver2);
        DEFINE_STATIC_STRING(PortalVein1);

        DEFINE_STATIC_STRING(Legs1);
        DEFINE_STATIC_STRING(Legs2);

        DEFINE_STATIC_STRING(Muscle1);
        DEFINE_STATIC_STRING(Muscle2);

        DEFINE_STATIC_STRING(Myocardium1);
        DEFINE_STATIC_STRING(Myocardium2);

        DEFINE_STATIC_STRING(Pericardium1);

        DEFINE_STATIC_STRING(Skin1);
        DEFINE_STATIC_STRING(Skin2);

        DEFINE_STATIC_STRING(VenaCava);

        DEFINE_STATIC_STRING(Ground);
      };

      namespace CardiovascularPath {

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
        // Kidney
        DEFINE_STATIC_STRING(Aorta1ToKidneys1);
        DEFINE_STATIC_STRING(Kidneys1ToGround);
        DEFINE_STATIC_STRING(Kidneys1ToKidneys2);
        DEFINE_STATIC_STRING(Kidneys2ToVenaCava);
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
        DEFINE_STATIC_STRING(KidneysE1);
        DEFINE_STATIC_STRING(KidneysE2);
        DEFINE_STATIC_STRING(KidneysE3);
        DEFINE_STATIC_STRING(KidneysI);
        DEFINE_STATIC_STRING(KidneysL);
        DEFINE_STATIC_STRING(LiverE1);
        DEFINE_STATIC_STRING(LiverE2);
        DEFINE_STATIC_STRING(LiverE3);
        DEFINE_STATIC_STRING(LiverI);
        DEFINE_STATIC_STRING(LiverL);
        DEFINE_STATIC_STRING(LungsE1);
        DEFINE_STATIC_STRING(LungsE2);
        DEFINE_STATIC_STRING(LungsE3);
        DEFINE_STATIC_STRING(LungsI);
        DEFINE_STATIC_STRING(LungsL);
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

        DEFINE_STATIC_STRING(KidneysVToKidneysE1);
        DEFINE_STATIC_STRING(KidneysE1ToKidneysE2);
        DEFINE_STATIC_STRING(KidneysE2ToKidneysE3);
        DEFINE_STATIC_STRING(KidneysE3ToKidneysI);
        DEFINE_STATIC_STRING(KidneysE3ToGround);
        DEFINE_STATIC_STRING(KidneysIToGround);
        DEFINE_STATIC_STRING(KidneysE3ToKidneysL);
        DEFINE_STATIC_STRING(KidneysLToLymph);

        DEFINE_STATIC_STRING(LiverVToLiverE1);
        DEFINE_STATIC_STRING(LiverE1ToLiverE2);
        DEFINE_STATIC_STRING(LiverE2ToLiverE3);
        DEFINE_STATIC_STRING(LiverE3ToLiverI);
        DEFINE_STATIC_STRING(LiverE3ToGround);
        DEFINE_STATIC_STRING(LiverIToGround);
        DEFINE_STATIC_STRING(LiverE3ToLiverL);
        DEFINE_STATIC_STRING(LiverLToLymph);

        DEFINE_STATIC_STRING(LeftLungVToLungsE1);
        DEFINE_STATIC_STRING(RightLungVToLungsE1);
        DEFINE_STATIC_STRING(LungsE1ToLungsE2);
        DEFINE_STATIC_STRING(LungsE2ToLungsE3);
        DEFINE_STATIC_STRING(LungsE3ToLungsI);
        DEFINE_STATIC_STRING(LungsE3ToGround);
        DEFINE_STATIC_STRING(LungsIToGround);
        DEFINE_STATIC_STRING(LungsE3ToLungsL);
        DEFINE_STATIC_STRING(LungsLToLymph);

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
      };

      namespace RenalNode {

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

      namespace RenalPath {

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
