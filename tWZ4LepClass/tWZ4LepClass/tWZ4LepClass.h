#pragma once

#include "FastFrames/MainFrame.h"

#include "FastFrames/ConfigSetting.h"
#include "FastFrames/Sample.h"
#include "FastFrames/UniqueSampleID.h"

#include "Math/Vector4D.h"
#include "ROOT/RDataFrame.hxx"
#include "TClass.h"

#include <memory>

#include "Math/GenVector/Boost.h"

using ROOT::VecOps::RVec;
using TLV = ROOT::Math::PtEtaPhiEVector;
using LorentzBoost = ROOT::Math::Boost;

class UniqueSampleID;

class tWZ4LepClass : public MainFrame {
public:

  explicit tWZ4LepClass() = default;

  virtual ~tWZ4LepClass() = default;

  virtual void init() override final {MainFrame::init();}

  virtual ROOT::RDF::RNode defineVariables(ROOT::RDF::RNode mainNode,
                                           const std::shared_ptr<Sample>& sample,
                                           const UniqueSampleID& id) override final;
  
  virtual ROOT::RDF::RNode defineVariablesNtuple(ROOT::RDF::RNode mainNode,
                                                 const std::shared_ptr<Sample>& sample,
                                                 const UniqueSampleID& id) override final;

  virtual ROOT::RDF::RNode defineVariablesTruth(ROOT::RDF::RNode node,
                                                const std::string& truth,
                                                const std::shared_ptr<Sample>& sample,
                                                const UniqueSampleID& sampleID) override final;
  
  virtual ROOT::RDF::RNode defineVariablesNtupleTruth(ROOT::RDF::RNode node,
                                                      const std::string& treeName,
                                                      const std::shared_ptr<Sample>& sample,
                                                      const UniqueSampleID& sampleID) override final;
private:

  ClassDefOverride(tWZ4LepClass, 1);

};
