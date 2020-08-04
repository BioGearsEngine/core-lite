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
#include <biogears/exports.h>

#include <biogears/cdm/CommonDataModel.h>
// CDM Features in use
#include <biogears/cdm/engine/PhysiologyEngine.h>
#include <biogears/cdm/utils/FileUtils.h>

#include <memory>

namespace biogears {
BIOGEARS_API std::unique_ptr<biogears::PhysiologyEngine> CreateBioGearsEngine(const std::string& logfile = "");
BIOGEARS_API std::unique_ptr<biogears::PhysiologyEngine> CreateBioGearsEngine(biogears::Logger* logger = nullptr);
}

namespace mil {
namespace tatrc {
  namespace physiology {
    namespace biogears {

      DEFINE_STATIC_STRING_EX(Version, BioGears_6 .1.1_beta);

      namespace Graph {

        DEFINE_STATIC_STRING(ActiveCardiovascular);
        DEFINE_STATIC_STRING(Cardiovascular);
        DEFINE_STATIC_STRING(Renal);
        DEFINE_STATIC_STRING(Respiratory);
        DEFINE_STATIC_STRING(RespiratoryAndAnesthesiaMachine);
        DEFINE_STATIC_STRING(RespiratoryAndInhaler);
        DEFINE_STATIC_STRING(RespiratoryAndMechanicalVentilator);
        DEFINE_STATIC_STRING(Aerosol);
        DEFINE_STATIC_STRING(AerosolAndInhaler);
        DEFINE_STATIC_STRING(AnesthesiaMachine);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            ActiveCardiovascular, Cardiovascular, Renal, Respiratory, RespiratoryAndAnesthesiaMachine, RespiratoryAndInhaler, RespiratoryAndMechanicalVentilator, Aerosol, AerosolAndInhaler, AnesthesiaMachine
          };
          return _values;
        }
      };

      namespace ChymeCompartment {

        DEFINE_STATIC_STRING_EX(SmallIntestine, SmallIntestineChyme);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            SmallIntestine
          };
          return _values;
        }
      };

      namespace ChymeLink {

        DEFINE_STATIC_STRING(SmallIntestineChymeToVasculature);
        static const std::vector<std::string>& GetValues()
        {

          static std::vector<std::string> _values = {
            SmallIntestineChymeToVasculature
          };
          return _values;
        }
      };

      namespace PulmonaryCompartment {
        DEFINE_STATIC_STRING(Mouth);
        DEFINE_STATIC_STRING(Trachea);
        DEFINE_STATIC_STRING(Bronchi);
        DEFINE_STATIC_STRING(Alveoli);
        DEFINE_STATIC_STRING(Pleural);
        DEFINE_STATIC_STRING(DeadSpace);
        DEFINE_STATIC_STRING(AlveoliLeak);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Mouth, Trachea, Bronchi, Alveoli, Pleural, DeadSpace, AlveoliLeak
          };
          return _values;
        }
      };

      namespace PulmonaryLink {
        DEFINE_STATIC_STRING(EnvironmentToMouth);
        DEFINE_STATIC_STRING(MouthToTrachea);
        DEFINE_STATIC_STRING(TracheaToBronchi);
        DEFINE_STATIC_STRING(BronchiToAlveoli);
        DEFINE_STATIC_STRING(AlveoliToAlveoliLeak);
        DEFINE_STATIC_STRING(AlveoliLeakToPleural);
        DEFINE_STATIC_STRING(PleuralToEnvironment);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            EnvironmentToMouth, MouthToTrachea, TracheaToBronchi, BronchiToAlveoli, AlveoliToAlveoliLeak, AlveoliLeakToPleural, PleuralToEnvironment
          };
          return _values;
        }
      };

      namespace TissueLiteCompartment {
        DEFINE_STATIC_STRING_EX(Bone, BoneTissue);
        DEFINE_STATIC_STRING_EX(Brain, BrainTissue);
        DEFINE_STATIC_STRING_EX(Fat, FatTissue);
        DEFINE_STATIC_STRING_EX(Gut, GutTissue);
        DEFINE_STATIC_STRING_EX(Kidney, KidneyTissue);
        DEFINE_STATIC_STRING_EX(Liver, LiverTissue);
        DEFINE_STATIC_STRING_EX(Lung, LungTissue);
        DEFINE_STATIC_STRING_EX(Muscle, MuscleTissue);
        DEFINE_STATIC_STRING_EX(Myocardium, MyocardiumTissue);
        DEFINE_STATIC_STRING_EX(Skin, SkinTissue);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Bone, Brain, Fat, Gut, Kidney, Liver, Lung, Muscle, Myocardium, Skin
          };
          return _values;
        }
      };

      namespace ExtravascularLiteCompartment {

        DEFINE_STATIC_STRING_EX(BoneExtracellular, BoneTissueExtracellular);
        DEFINE_STATIC_STRING_EX(BrainExtracellular, BrainTissueExtracellular);
        DEFINE_STATIC_STRING_EX(FatExtracellular, FatTissueExtracellular);
        DEFINE_STATIC_STRING_EX(GutExtracellular, GutTissueExtracellular);
        DEFINE_STATIC_STRING_EX(KidneyExtracellular, KidneyTissueExtracellular);
        DEFINE_STATIC_STRING_EX(LiverExtracellular, LiverTissueExtracellular);
        DEFINE_STATIC_STRING_EX(LungExtracellular, LungTissueExtracellular);
        DEFINE_STATIC_STRING_EX(MuscleExtracellular, MuscleTissueExtracellular);
        DEFINE_STATIC_STRING_EX(MyocardiumExtracellular, MyocardiumTissueExtracellular);
        DEFINE_STATIC_STRING_EX(SkinExtracellular, SkinTissueExtracellular);

        DEFINE_STATIC_STRING_EX(BoneIntracellular, BoneTissueIntracellular);
        DEFINE_STATIC_STRING_EX(BrainIntracellular, BrainTissueIntracellular);
        DEFINE_STATIC_STRING_EX(FatIntracellular, FatTissueIntracellular);
        DEFINE_STATIC_STRING_EX(GutIntracellular, GutTissueIntracellular);
        DEFINE_STATIC_STRING_EX(KidneyIntracellular, KidneyTissueIntracellular);
        DEFINE_STATIC_STRING_EX(LiverIntracellular, LiverTissueIntracellular);
        DEFINE_STATIC_STRING_EX(LungIntracellular, LungTissueIntracellular);
        DEFINE_STATIC_STRING_EX(MuscleIntracellular, MuscleTissueIntracellular);
        DEFINE_STATIC_STRING_EX(MyocardiumIntracellular, MyocardiumTissueIntracellular);
        DEFINE_STATIC_STRING_EX(SkinIntracellular, SkinTissueIntracellular);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            BoneExtracellular, BrainExtracellular, FatExtracellular, GutExtracellular, KidneyExtracellular, LiverExtracellular, LungExtracellular, MuscleExtracellular, MyocardiumExtracellular, SkinExtracellular, BoneIntracellular, BrainIntracellular, FatIntracellular, GutIntracellular, KidneyIntracellular, LiverIntracellular, LungIntracellular, MuscleIntracellular, MyocardiumIntracellular, SkinIntracellular
          };
          return _values;
        }
      };

      namespace VascularCompartment {
        // Renal
        DEFINE_STATIC_STRING_EX(Kidneys, KidneysVasculature);
        DEFINE_STATIC_STRING_EX(LeftKidney, LeftKidneyVasculature);
        DEFINE_STATIC_STRING(LeftRenalArtery);
        DEFINE_STATIC_STRING(LeftAfferentArteriole);
        DEFINE_STATIC_STRING(LeftPeritubularCapillaries);
        DEFINE_STATIC_STRING_EX(RightKidney, RightKidneyVasculature);
        DEFINE_STATIC_STRING(RightRenalArtery);
        DEFINE_STATIC_STRING(RightAfferentArteriole);
        DEFINE_STATIC_STRING(RightPeritubularCapillaries);
       

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Kidneys, LeftKidney, RightKidney
            //,Ground
          };
          return _values;
        }
      };

      namespace VascularLiteCompartment {
        // Cardio
        DEFINE_STATIC_STRING(Aorta);
        DEFINE_STATIC_STRING(Heart);
        DEFINE_STATIC_STRING_EX(Myocardium, MyocardiumVasculature);
        DEFINE_STATIC_STRING(LeftHeart);
        DEFINE_STATIC_STRING(RightHeart);
        DEFINE_STATIC_STRING(VenaCava);
        // Pulmonary
        DEFINE_STATIC_STRING(PulmonaryArteries);
        DEFINE_STATIC_STRING(PulmonaryCapillaries);
        DEFINE_STATIC_STRING(PulmonaryVeins);
        DEFINE_STATIC_STRING_EX(Lungs, LungVasculature);
        DEFINE_STATIC_STRING_EX(LeftLung, LeftLungVasculature);
        DEFINE_STATIC_STRING(LeftPulmonaryArteries);
        DEFINE_STATIC_STRING(LeftPulmonaryCapillaries);
        DEFINE_STATIC_STRING(LeftPulmonaryVeins);
        DEFINE_STATIC_STRING_EX(RightLung, RightLungVasculature);
        DEFINE_STATIC_STRING(RightPulmonaryArteries);
        DEFINE_STATIC_STRING(RightPulmonaryCapillaries);
        DEFINE_STATIC_STRING(RightPulmonaryVeins);
        // RenalLite
        DEFINE_STATIC_STRING_EX(Kidney, KidneyVasculature);
        DEFINE_STATIC_STRING(RenalArtery);
        DEFINE_STATIC_STRING(Nephron);
        DEFINE_STATIC_STRING(AfferentArteriole);
        DEFINE_STATIC_STRING(GlomerularCapillaries);
        DEFINE_STATIC_STRING(EfferentArteriole);
        DEFINE_STATIC_STRING(PeritubularCapillaries);
        DEFINE_STATIC_STRING(BowmansCapsules);
        DEFINE_STATIC_STRING(Tubules);
        DEFINE_STATIC_STRING(RenalVein);
        // General Organs and Periphery
        DEFINE_STATIC_STRING_EX(Bone, BoneVasculature);
        DEFINE_STATIC_STRING_EX(Brain, BrainVasculature);
        DEFINE_STATIC_STRING_EX(Fat, FatVasculature);
        DEFINE_STATIC_STRING_EX(Gut, GutVasculature);
        DEFINE_STATIC_STRING_EX(Liver, LiverVasculature);
        DEFINE_STATIC_STRING_EX(Skin, SkinVasculature);
        DEFINE_STATIC_STRING_EX(Muscle, MuscleVasculature);
        DEFINE_STATIC_STRING_EX(Arms, ArmsVasculature);
        DEFINE_STATIC_STRING_EX(Legs, LegsVasculature);

        DEFINE_STATIC_STRING(Ground);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Aorta, Heart, Myocardium, LeftHeart, RightHeart, VenaCava, PulmonaryArteries, PulmonaryCapillaries, 
            PulmonaryVeins, Lungs, LeftLung, LeftPulmonaryArteries, LeftPulmonaryCapillaries, LeftPulmonaryVeins, RightLung, 
            RightPulmonaryArteries, RightPulmonaryCapillaries, RightPulmonaryVeins, Kidney, RenalArtery, Nephron, 
            AfferentArteriole, GlomerularCapillaries, EfferentArteriole, PeritubularCapillaries, BowmansCapsules, Tubules, RenalVein, 
            Bone, Brain, Fat, Gut, Liver, Skin, Muscle, Arms, Legs
          };
          return _values;
        }
      };


    namespace VascularLiteLink {
        // Heart and Lungs
        DEFINE_STATIC_STRING(VenaCavaToRightHeart);
        DEFINE_STATIC_STRING(RightHeartToLeftPulmonaryArteries);
        DEFINE_STATIC_STRING(LeftPulmonaryArteriesToCapillaries);
        DEFINE_STATIC_STRING(LeftPulmonaryArteriesToVeins);
        DEFINE_STATIC_STRING(LeftPulmonaryCapillariesToVeins);
        DEFINE_STATIC_STRING(LeftPulmonaryVeinsToLeftHeart);
        DEFINE_STATIC_STRING(RightHeartToRightPulmonaryArteries);
        DEFINE_STATIC_STRING(RightPulmonaryArteriesToCapillaries);
        DEFINE_STATIC_STRING(RightPulmonaryArteriesToVeins);
        DEFINE_STATIC_STRING(RightPulmonaryCapillariesToVeins);
        DEFINE_STATIC_STRING(RightPulmonaryVeinsToLeftHeart);
        DEFINE_STATIC_STRING(LeftHeartToAorta);
        // Arm
        DEFINE_STATIC_STRING(AortaToArms);
        DEFINE_STATIC_STRING(ArmsToVenaCava);
        // Bone
        DEFINE_STATIC_STRING(AortaToBone);
        DEFINE_STATIC_STRING(BoneToVenaCava);
        // Brain
        DEFINE_STATIC_STRING(AortaToBrain);
        DEFINE_STATIC_STRING(BrainToVenaCava);
        // Fat
        DEFINE_STATIC_STRING(AortaToFat);
        DEFINE_STATIC_STRING(FatToVenaCava);
        //  Kidney
        DEFINE_STATIC_STRING(AortaToKidney);
        DEFINE_STATIC_STRING(KidneyToVenaCava);
        //  Left Kidney
        DEFINE_STATIC_STRING(AortaToLeftKidney);
        DEFINE_STATIC_STRING(LeftKidneyToVenaCava);
        //  Right Kidney
        DEFINE_STATIC_STRING(AortaToRightKidney);
        DEFINE_STATIC_STRING(RightKidneyToVenaCava);
        // Gut
        DEFINE_STATIC_STRING(AortaToGut);
        DEFINE_STATIC_STRING(GutToLiver);
        // Leg
        DEFINE_STATIC_STRING(AortaToLegs);
        DEFINE_STATIC_STRING(LegsToVenaCava);
        // Liver
        DEFINE_STATIC_STRING(AortaToLiver);
        DEFINE_STATIC_STRING(LiverToVenaCava);
        // Muscle
        DEFINE_STATIC_STRING(AortaToMuscle);
        DEFINE_STATIC_STRING(MuscleToVenaCava);
        // Myocardium
        DEFINE_STATIC_STRING(AortaToMyocardium);
        DEFINE_STATIC_STRING(MyocardiumToVenaCava);
        // Skin
        DEFINE_STATIC_STRING(AortaToSkin);
        DEFINE_STATIC_STRING(SkinToVenaCava);

        // Hemorrhage and IV
        DEFINE_STATIC_STRING(VenaCavaIV);
        DEFINE_STATIC_STRING(VenaCavaHemorrhage);
        DEFINE_STATIC_STRING(AortaHemorrhage);
        DEFINE_STATIC_STRING(ArmsHemorrhage);
        DEFINE_STATIC_STRING(GutHemorrhage);
        DEFINE_STATIC_STRING(KidneyHemorrhage);
        DEFINE_STATIC_STRING(LegsHemorrhage);

        // Vascular To Tissue Links
        DEFINE_STATIC_STRING(BoneVascularToTissue);
        DEFINE_STATIC_STRING(BrainVascularToTissue);
        DEFINE_STATIC_STRING(FatVascularToTissue);
        DEFINE_STATIC_STRING(GutVascularToTissue);
        DEFINE_STATIC_STRING(KidneyVascularToTissue);
        DEFINE_STATIC_STRING(LeftLungVascularToTissue);
        DEFINE_STATIC_STRING(LiverVascularToTissue);
        DEFINE_STATIC_STRING(MuscleVascularToTissue);
        DEFINE_STATIC_STRING(MyocardiumVascularToTissue);
        DEFINE_STATIC_STRING(RightLungVascularToTissue);
        DEFINE_STATIC_STRING(SkinVascularToTissue);

        // Renal Lite Links
        // Reusing 'AortaToKidney' to connect the Aorta To  Renal Artery
        DEFINE_STATIC_STRING(RenalArteryToAfferentArteriole);
        DEFINE_STATIC_STRING(AfferentArterioleToGlomerularCapillaries);
        DEFINE_STATIC_STRING(GlomerularCapillariesToEfferentArteriole);
        DEFINE_STATIC_STRING(GlomerularCapillariesToBowmansCapsules);
        DEFINE_STATIC_STRING(BowmansCapsulesToTubules);
        DEFINE_STATIC_STRING(TubulesToPeritubularCapillaries);
        DEFINE_STATIC_STRING(EfferentArterioleToPeritubularCapillaries);
        DEFINE_STATIC_STRING(PeritubularCapillariesToRenalVein);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            VenaCavaToRightHeart, RightHeartToLeftPulmonaryArteries, LeftPulmonaryArteriesToCapillaries, LeftPulmonaryArteriesToVeins, LeftPulmonaryCapillariesToVeins, LeftPulmonaryVeinsToLeftHeart, RightHeartToRightPulmonaryArteries, RightPulmonaryArteriesToCapillaries, RightPulmonaryArteriesToVeins, RightPulmonaryCapillariesToVeins, RightPulmonaryVeinsToLeftHeart, LeftHeartToAorta, AortaToBone, BoneToVenaCava, AortaToBrain, BrainToVenaCava, AortaToFat, FatToVenaCava, AortaToGut, GutToLiver,  AortaToArms, ArmsToVenaCava, AortaToKidney, KidneyToVenaCava, AortaToLegs, LegsToVenaCava, AortaToLiver, LiverToVenaCava, AortaToMuscle, MuscleToVenaCava, AortaToMyocardium, MyocardiumToVenaCava, AortaToSkin, SkinToVenaCava
            ,
            BoneVascularToTissue, BrainVascularToTissue, FatVascularToTissue, GutVascularToTissue, KidneyVascularToTissue, LeftLungVascularToTissue, LiverVascularToTissue, MuscleVascularToTissue, MyocardiumVascularToTissue, RightLungVascularToTissue, SkinVascularToTissue
            ,
            VenaCavaHemorrhage, ArmsHemorrhage, LegsHemorrhage, GutHemorrhage, AortaHemorrhage
          };
          return _values;
        }
      };

       namespace UrineCompartment {

        DEFINE_STATIC_STRING(Ureters);
        DEFINE_STATIC_STRING(LeftUreter);
        DEFINE_STATIC_STRING(RightUreter);
        DEFINE_STATIC_STRING(Bladder);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Ureters, LeftUreter, RightUreter, Bladder
          };
          return _values;
        }
      };

       namespace UrineLink {

         DEFINE_STATIC_STRING(RightTubulesToUreter);
         DEFINE_STATIC_STRING(LeftTubulesToUreter);
         DEFINE_STATIC_STRING(RightUreterToBladder);
         DEFINE_STATIC_STRING(LeftUreterToBladder);
         DEFINE_STATIC_STRING(BladderToGround);
         DEFINE_STATIC_STRING(BladderToGroundSource);

         static const std::vector<std::string>& GetValues()
         {
           static std::vector<std::string> _values = {
             RightTubulesToUreter, LeftTubulesToUreter, RightUreterToBladder, LeftUreterToBladder, BladderToGround, BladderToGroundSource
           };
           return _values;
         }
       }

      namespace UrineLiteCompartment {

        DEFINE_STATIC_STRING(Ureter);
        DEFINE_STATIC_STRING(Bladder);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Ureter, Bladder
          };
          return _values;
        }
      };

      namespace UrineLiteLink {

        DEFINE_STATIC_STRING(TubulesToUreter);
        DEFINE_STATIC_STRING(UreterToBladder);
        DEFINE_STATIC_STRING(BladderToGround);
        DEFINE_STATIC_STRING(BladderToGroundSource);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            TubulesToUreter, UreterToBladder, BladderToGround, BladderToGroundSource
          };
          return _values;
        }
      };

      namespace LymphCompartment {

        DEFINE_STATIC_STRING(Lymph);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Lymph
          };
          return _values;
        }
      };

      namespace LymphLink {

        DEFINE_STATIC_STRING(BoneTissueToLymph);
        DEFINE_STATIC_STRING(BrainTissueToLymph);
        DEFINE_STATIC_STRING(FatTissueToLymph);
        DEFINE_STATIC_STRING(GutTissueToLymph);
        DEFINE_STATIC_STRING(KidneyTissueToLymph);
        DEFINE_STATIC_STRING(LungTissueToLymph);
        DEFINE_STATIC_STRING(LiverTissueToLymph);
        DEFINE_STATIC_STRING(MuscleTissueToLymph);
        DEFINE_STATIC_STRING(MyocardiumTissueToLymph);
        DEFINE_STATIC_STRING(RightLungTissueToLymph);
        DEFINE_STATIC_STRING(SkinTissueToLymph);
        DEFINE_STATIC_STRING(SpleenTissueToLymph);

        DEFINE_STATIC_STRING(LymphToVenaCava);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            BoneTissueToLymph, BrainTissueToLymph, FatTissueToLymph, GutTissueToLymph, KidneyTissueToLymph, LungTissueToLymph, LiverTissueToLymph, MuscleTissueToLymph, MyocardiumTissueToLymph, RightLungTissueToLymph, SkinTissueToLymph, SpleenTissueToLymph, LymphToVenaCava
          };
          return _values;
        }
      };

      namespace TemperatureCompartment {

        DEFINE_STATIC_STRING(Active);
        DEFINE_STATIC_STRING(Ambient);
        DEFINE_STATIC_STRING(Clothing);
        DEFINE_STATIC_STRING(Enclosure);
        DEFINE_STATIC_STRING(ExternalCore);
        DEFINE_STATIC_STRING(ExternalSkin);
        DEFINE_STATIC_STRING(ExternalGround);
        DEFINE_STATIC_STRING(InternalCore);
        DEFINE_STATIC_STRING(InternalSkin);
        DEFINE_STATIC_STRING(InternalGround);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Active, Ambient, Clothing, Enclosure, ExternalCore, ExternalSkin, ExternalGround, InternalCore, InternalSkin, InternalGround
          };
          return _values;
        }
      };

      namespace TemperatureLink {

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
        DEFINE_STATIC_STRING(GroundToInternalCore);
        DEFINE_STATIC_STRING(InternalCoreToInternalSkin);
        DEFINE_STATIC_STRING(InternalCoreToGround);
        DEFINE_STATIC_STRING(InternalSkinToGround);
        DEFINE_STATIC_STRING(InternalCoreToExternalCore);
        DEFINE_STATIC_STRING(InternalSkinToExternalSkin);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            ActiveToClothing, ClothingToEnclosure, ClothingToEnvironment, ExternalCoreToGround, GroundToActive, GroundToClothing, GroundToEnclosure, GroundToEnvironment, ExternalSkinToGround, ExternalSkinToClothing, GroundToInternalCore, InternalCoreToInternalSkin, InternalCoreToGround, InternalSkinToGround, InternalCoreToExternalCore, InternalSkinToExternalSkin
          };
          return _values;
        }
      };

      namespace EnvironmentCompartment {

        DEFINE_STATIC_STRING(Ambient);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Ambient
          };
          return _values;
        }
      };

      namespace TemperatureLiteCompartment {

        DEFINE_STATIC_STRING(Core);
        DEFINE_STATIC_STRING(Environment);
        DEFINE_STATIC_STRING(Skin);
        DEFINE_STATIC_STRING(Ground);
        DEFINE_STATIC_STRING(Ref);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Core, Environment, Skin, Ground, Ref
          };
          return _values;
        }
      };

      namespace TemperatureLiteLink {

        DEFINE_STATIC_STRING(CoreToRef); //Respiration
        DEFINE_STATIC_STRING(RefToEnvironment); //TempSource
        DEFINE_STATIC_STRING(EnvironmentToSkin); //Resistor
        DEFINE_STATIC_STRING(GroundToCore); //Metabolism
        DEFINE_STATIC_STRING(CoreToSkin); //Resistor
        DEFINE_STATIC_STRING(CoreToGround); //Capacitor
        DEFINE_STATIC_STRING(SkinToGround); //Capacitor

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            CoreToRef, RefToEnvironment, EnvironmentToSkin, GroundToCore, CoreToSkin, CoreToGround, SkinToGround
          };
          return _values;
        }
      };

      namespace AnesthesiaMachineCompartment {

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

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            AnesthesiaConnection, ExpiratoryLimb, GasInlet, GasSource, InspiratoryLimb, ReliefValve, Scrubber, Selector, Ventilator, VentilatorConnection, YPiece
          };
          return _values;
        }
      };

      namespace AnesthesiaMachineLink {

        DEFINE_STATIC_STRING(VentilatorToSelector);
        DEFINE_STATIC_STRING(SelectorToReliefValve);
        DEFINE_STATIC_STRING(SelectorToScrubber);
        DEFINE_STATIC_STRING(ScrubberToGasInlet);
        DEFINE_STATIC_STRING(Exhaust);
        DEFINE_STATIC_STRING(GasSourceToGasInlet);
        DEFINE_STATIC_STRING(GasInletToInspiratoryLimb);
        DEFINE_STATIC_STRING(InspiratoryLimbToYPiece);
        DEFINE_STATIC_STRING(YPieceToExpiratoryLimb);
        DEFINE_STATIC_STRING(ExpiratoryLimbToSelector);
        DEFINE_STATIC_STRING(YPieceToAnesthesiaConnection);
        DEFINE_STATIC_STRING(AnesthesiaConnectionLeak);
        DEFINE_STATIC_STRING(Mask);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            VentilatorToSelector, SelectorToReliefValve, SelectorToScrubber, ScrubberToGasInlet, Exhaust, GasSourceToGasInlet, GasInletToInspiratoryLimb, InspiratoryLimbToYPiece, YPieceToExpiratoryLimb, ExpiratoryLimbToSelector, YPieceToAnesthesiaConnection, AnesthesiaConnectionLeak, Mask
          };
          return _values;
        }
      };

      namespace InhalerCompartment {

        DEFINE_STATIC_STRING(Mouthpiece);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Mouthpiece
          };
          return _values;
        }
      };

      namespace InhalerLink {

        DEFINE_STATIC_STRING(EnvironmentToMouthpiece);
        DEFINE_STATIC_STRING(MouthpieceToMouth);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            EnvironmentToMouthpiece, MouthpieceToMouth
          };
          return _values;
        }
      };

      namespace MechanicalVentilatorCompartment {

        DEFINE_STATIC_STRING(Connection);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            Connection
          };
          return _values;
        }
      };

      namespace MechanicalVentilatorLink {

        DEFINE_STATIC_STRING(ConnectionToMouth);

        static const std::vector<std::string>& GetValues()
        {
          static std::vector<std::string> _values = {
            ConnectionToMouth
          };
          return _values;
        }
      };

    } //namespace biogears
  } //namespace physiology
} //namespace tatrc
} //namespace mil

namespace BGE = mil::tatrc::physiology::biogears;
