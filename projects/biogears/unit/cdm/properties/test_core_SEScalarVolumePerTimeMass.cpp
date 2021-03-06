//-------------------------------------------------------------------------------------------
//- Copyright 2017 Applied Research Associates, Inc.
//- Licensed under the Apache License, Version 2.0 (the "License"); you may not use
//- this file except in compliance with the License. You may obtain a copy of the License
//- at:
//- http://www.apache.org/licenses/LICENSE-2.0
//- Unless required by applicable law or agreed to in writing, software distributed under
//- the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//- CONDITIONS OF ANY KIND, either express or implied. See the License for the
//-  specific language governing permissions and limitations under the License.
//-------------------------------------------------------------------------------------------
//!
//! @author David Lee
//! @date   2017 Aug 3rd
//!
//! Unit Test for Biogears-common Config
//!
#include <thread>

#include <gtest/gtest.h>

#include <biogears/cdm/properties/SEScalarVolumePerTimeMass.h>


#ifdef DISABLE_BIOGEARS_SEScalarVolumePerTimeMass_TEST
#define TEST_FIXTURE_NAME DISABLED_SEScalarVolumePerTimeMass_Fixture
#else
#define TEST_FIXTURE_NAME SEScalarVolumePerTimeMass_Fixture
#endif

// The fixture for testing class Foo.
class TEST_FIXTURE_NAME : public ::testing::Test {
protected:
  // You can do set-up work for each test here.
  TEST_FIXTURE_NAME() = default;

  // You can do clean-up work that doesn't throw exceptions here.
  virtual ~TEST_FIXTURE_NAME() = default;

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  // Code here will be called immediately after the constructor (right
  // before each test).
  virtual void SetUp() override;

  // Code here will be called immediately after each test (right
  // before the destructor).
  virtual void TearDown() override;
};

void TEST_FIXTURE_NAME::SetUp()
{
}

void TEST_FIXTURE_NAME::TearDown()
{
}

TEST_F(TEST_FIXTURE_NAME, Unload)
{
  biogears::SEScalarVolumePerTimeMass VolumePerTimeMass = biogears::SEScalarVolumePerTimeMass();
  auto ptr = VolumePerTimeMass.Unload();
  EXPECT_EQ(ptr, nullptr);
}

TEST_F(TEST_FIXTURE_NAME, IsValidUnit)
{
  bool unit0 = biogears::VolumePerTimeMassUnit::IsValidUnit("L/s g");
  bool unit1 = biogears::VolumePerTimeMassUnit::IsValidUnit("mL / s g");
  bool unit2 = biogears::VolumePerTimeMassUnit::IsValidUnit("mL/min kg");
  bool unit3 = biogears::VolumePerTimeMassUnit::IsValidUnit("mL/s kg");
  bool unit4 = biogears::VolumePerTimeMassUnit::IsValidUnit("uL/min kg");
  EXPECT_EQ(unit0, true);
  EXPECT_EQ(unit1, true);
  EXPECT_EQ(unit2, true);
  EXPECT_EQ(unit3, true);
  EXPECT_EQ(unit4, true);
  bool unit8 = biogears::VolumePerTimeMassUnit::IsValidUnit("DEADBEEF");
  EXPECT_EQ(unit8, false);
}

TEST_F(TEST_FIXTURE_NAME, GetCompoundUnit)
{
  biogears::VolumePerTimeMassUnit mu0 = biogears::VolumePerTimeMassUnit::GetCompoundUnit("L/s g");
  biogears::VolumePerTimeMassUnit mu1 = biogears::VolumePerTimeMassUnit::GetCompoundUnit("mL / s g");
  biogears::VolumePerTimeMassUnit mu2 = biogears::VolumePerTimeMassUnit::GetCompoundUnit("mL/min kg");
  biogears::VolumePerTimeMassUnit mu3 = biogears::VolumePerTimeMassUnit::GetCompoundUnit("mL/s kg");
  biogears::VolumePerTimeMassUnit mu4 = biogears::VolumePerTimeMassUnit::GetCompoundUnit("uL/min kg");
  EXPECT_EQ(mu0, biogears::VolumePerTimeMassUnit::L_Per_s_g);
  EXPECT_EQ(mu1, biogears::VolumePerTimeMassUnit::mL_Per_s_g);
  EXPECT_EQ(mu2, biogears::VolumePerTimeMassUnit::mL_Per_min_kg);
  EXPECT_EQ(mu3, biogears::VolumePerTimeMassUnit::mL_Per_s_kg);
  EXPECT_EQ(mu4, biogears::VolumePerTimeMassUnit::uL_Per_min_kg);
  EXPECT_THROW(biogears::VolumePerTimeMassUnit::GetCompoundUnit("DEADBEEF"),biogears::CommonDataModelException);
}