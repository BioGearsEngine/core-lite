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
#include <biogears/cdm/patient/conditions/SEChronicRenalStenosis.h>

#include <biogears/cdm/properties/SEScalar0To1.h>

namespace biogears {
SEChronicRenalStenosis::SEChronicRenalStenosis()
  : SEPatientCondition()
{
  m_KidneySeverity = nullptr;
}

SEChronicRenalStenosis::~SEChronicRenalStenosis()
{
  Clear();
}

void SEChronicRenalStenosis::Clear()
{
  SEPatientCondition::Clear();
  SAFE_DELETE(m_KidneySeverity);
}

bool SEChronicRenalStenosis::IsValid() const
{
  return SEPatientCondition::IsValid() && (HasKidneySeverity());
}

bool SEChronicRenalStenosis::Load(const CDM::ChronicRenalStenosisData& in)
{
  SEPatientCondition::Load(in);
  if (in.KidneySeverity().present())
    GetKidneySeverity().Load(in.KidneySeverity().get());
  return true;
}

CDM::ChronicRenalStenosisData* SEChronicRenalStenosis::Unload() const
{
  CDM::ChronicRenalStenosisData* data(new CDM::ChronicRenalStenosisData());
  Unload(*data);
  return data;
}

void SEChronicRenalStenosis::Unload(CDM::ChronicRenalStenosisData& data) const
{
  SEPatientCondition::Unload(data);
  if (HasKidneySeverity())
    data.KidneySeverity(std::unique_ptr<CDM::Scalar0To1Data>(m_KidneySeverity->Unload()));
}

bool SEChronicRenalStenosis::HasKidneySeverity() const
{
  return m_KidneySeverity == nullptr ? false : m_KidneySeverity->IsValid();
}
SEScalar0To1& SEChronicRenalStenosis::GetKidneySeverity()
{
  if (m_KidneySeverity == nullptr)
    m_KidneySeverity = new SEScalar0To1();
  return *m_KidneySeverity;
}

void SEChronicRenalStenosis::ToString(std::ostream& str) const
{
  str << "Patient Condition : Chronic Renal Stenosis";
  if (HasComment())
    str << "\n\tComment: " << m_Comment;
  str << "\n\ Kidney Occlusion 0To1: ";
  HasKidneySeverity() ? str << *m_KidneySeverity : str << "NaN";
  str << std::flush;
}
}