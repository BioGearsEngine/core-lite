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
*************************************************************************************/
#include <biogears/cdm/patient/conditions/SELobarPneumonia.h>

#include <biogears/cdm/properties/SEScalar0To1.h>

namespace biogears {
SELobarPneumonia::SELobarPneumonia()
  : SEPatientCondition()
{
  m_Severity = nullptr;
  m_LungAffectedFraction = nullptr;
}
//-----------------------------------------------------------------------------
SELobarPneumonia::~SELobarPneumonia()
{
  Clear();
}
//-----------------------------------------------------------------------------
void SELobarPneumonia::Clear()
{
  SEPatientCondition::Clear();
  SAFE_DELETE(m_Severity);
  SAFE_DELETE(m_LungAffectedFraction);
}
//-----------------------------------------------------------------------------
bool SELobarPneumonia::IsValid() const
{
  return SEPatientCondition::IsValid() && HasSeverity() && HasLungAffectedFraction();
}
//-----------------------------------------------------------------------------
bool SELobarPneumonia::Load(const CDM::LobarPneumoniaData& in)
{
  SEPatientCondition::Load(in);
  GetSeverity().Load(in.Severity());
  GetLungAffectedFraction().Load(in.LungAffectedFraction());
  return true;
}
//-----------------------------------------------------------------------------
CDM::LobarPneumoniaData* SELobarPneumonia::Unload() const
{
  CDM::LobarPneumoniaData* data(new CDM::LobarPneumoniaData());
  Unload(*data);
  return data;
}
//-----------------------------------------------------------------------------
void SELobarPneumonia::Unload(CDM::LobarPneumoniaData& data) const
{
  SEPatientCondition::Unload(data);
  if (m_Severity != nullptr)
    data.Severity(std::unique_ptr<CDM::Scalar0To1Data>(m_Severity->Unload()));
  if (m_LungAffectedFraction != nullptr)
    data.LungAffectedFraction(std::unique_ptr<CDM::Scalar0To1Data>(m_LungAffectedFraction->Unload()));
}
//-----------------------------------------------------------------------------
bool SELobarPneumonia::HasSeverity() const
{
  return m_Severity == nullptr ? false : m_Severity->IsValid();
}
//-----------------------------------------------------------------------------
SEScalar0To1& SELobarPneumonia::GetSeverity()
{
  if (m_Severity == nullptr)
    m_Severity = new SEScalar0To1();
  return *m_Severity;
}
//-----------------------------------------------------------------------------
bool SELobarPneumonia::HasLungAffectedFraction() const
{
  return m_LungAffectedFraction == nullptr ? false : m_LungAffectedFraction->IsValid();
}
//-----------------------------------------------------------------------------
SEScalar0To1& SELobarPneumonia::GetLungAffectedFraction()
{
  if (m_LungAffectedFraction == nullptr)
    m_LungAffectedFraction = new SEScalar0To1();
  return *m_LungAffectedFraction;
}
//-----------------------------------------------------------------------------
void SELobarPneumonia::ToString(std::ostream& str) const
{
  str << "Patient Condition : Lobar Pneumonia";
  if (HasComment())
    str << "\n\tComment: " << m_Comment;
  str << "\n\tSeverity: ";
  HasSeverity() ? str << *m_Severity : str << "NaN";
  str << "\n\tLungAffectedFraction: ";
  HasLungAffectedFraction() ? str << *m_LungAffectedFraction : str << "NaN";
  str << std::flush;
}
//-----------------------------------------------------------------------------
}