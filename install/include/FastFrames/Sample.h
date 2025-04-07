/**
 * @file Sample.h
 * @brief Sample
 *
 */

#pragma once

#include "FastFrames/ConfigDefine.h"
#include "FastFrames/Cutflow.h"
#include "FastFrames/Truth.h"
#include "FastFrames/UniqueSampleID.h"

#include <memory>
#include <string>
#include <vector>

class MetadataManager;
class Region;
class Systematic;

/**
 * @brief Class containing all information for a given Sample
 *
 */
class Sample {
public:

  /**
   * @brief Construct a new Sample object
   *
   * @param name Name of the sample
   */
  explicit Sample(const std::string& name) noexcept;

  /**
   * @brief Destroy the Sample object
   *
   */
  ~Sample() = default;

  /**
   * @brief Get the name of the Sample
   *
   * @return const std::string&
   */
  inline const std::string& name() const {return m_name;}

  /**
   * @brief Set the Reco Tree Name object
   *
   * @param treeName
   */
  inline void setRecoTreeName(const std::string& treeName) {m_recoTreeName = treeName;}

  /**
   * @brief Get the name of the reco tree
   *
   * @return const std::string&
   */
  inline const std::string& recoTreeName() const {return m_recoTreeName;}

  /**
   * @brief Set the selectionSuffix
   *
   * @param selectionSuffix
   */
  inline void setSelectionSuffix(const std::string& selectionSuffix) {m_selectionSuffix = selectionSuffix;};

  /**
   * @brief Get the selectionSuffix
   *
   * @return const std::string&
   */
  inline const std::string& selectionSuffix() const {return m_selectionSuffix;};

  /**
   * @brief Get all UniqueSampleIDs for this Sample
   *
   * @return const std::vector<UniqueSampleID>&
   */
  inline const std::vector<UniqueSampleID>& uniqueSampleIDs() const {return m_uniqueSampleIDs;}

  /**
   * @brief Add UniqueSampleID to this Sample
   *
   * @param id
   */
  inline void addUniqueSampleID(const UniqueSampleID& id) {m_uniqueSampleIDs.emplace_back(id);}

  /**
   * @brief Add Systemtic to this sample
   *
   * @param syst
   */
  inline void addSystematic(const std::shared_ptr<Systematic>& syst) {m_systematics.emplace_back(syst);}

  /**
   * @brief Get list of all Systematics
   *
   * @return const std::vector<std::shared_ptr<Systematic> >&
   */
  inline const std::vector<std::shared_ptr<Systematic> >& systematics() const {return m_systematics;}

  /**
   * @brief Add Region to this sample
   *
   * @param region
   */
  inline void addRegion(const std::shared_ptr<Region>& region) {m_regions.emplace_back(region);}

  /**
   * @brief Get all regions for this sample
   *
   * @return const std::vector<std::shared_ptr<Region> >&
   */
  inline const std::vector<std::shared_ptr<Region> >& regions() const {return m_regions;}

  /**
   * @brief Set vector of indices used to pair reco level and truth level trees.
   *
   * @param reco_to_truth_pairing_indices
   */
  void setRecoToTruthPairingIndices(const std::vector<std::string>& reco_to_truth_pairing_indices) {m_reco_to_truth_pairing_indices = reco_to_truth_pairing_indices;};

  /**
   * @brief Get vector of indices used to pair reco level and truth level trees.
   *
   * @return const std::vector<std::string>&
   */
  const std::vector<std::string>& recoToTruthPairingIndices() const {return m_reco_to_truth_pairing_indices;};

  /**
   * @brief Set the event weight
   *
   * @param weight
   */
  inline void setEventWeight(const std::string& weight) {m_eventWeight = weight;}

  /**
   * @brief Get the event weight
   *
   * @return const std::string&
   */
  inline const std::string& weight() const {return m_eventWeight;}

  /**
   * @brief Skip setup where a given region and a systematic are not set
   *
   * @param syst Systematic
   * @param reg Region
   * @return true
   * @return false
   */
  bool skipSystematicRegionCombination(const std::shared_ptr<Systematic>& syst,
                                       const std::shared_ptr<Region>& reg) const;

  /**
   * @brief Remove all systematics
   *
   */
  inline void clearSystematics() {m_systematics.clear();}

  /**
   * @brief Add Truth object
   *
   * @param truth
   */
  inline void addTruth(const std::shared_ptr<Truth>& truth) {m_truths.emplace_back(truth);}

  /**
   * @brief Get Truth objects
   *
   * @return const std::vector<std::shared_ptr<Truth> >&
   */
  inline const std::vector<std::shared_ptr<Truth> >& truths() const {return m_truths;}

  /**
   * @brief Has truths defined?
   *
   * @return true
   * @return false
   */
  inline bool hasTruth() const {return !m_truths.empty();}

  /**
   * @brief Get the list of unqiue truth tree names for this sample
   *
   * @return std::vector<std::string>
   */
  std::vector<std::string> uniqueTruthTreeNames() const;

  /**
   * @brief Get the list of truth tree names that are used for matching reco and truth
   *
   * @return std::vector<std::string>
   */
  std::vector<std::string> uniqueTruthTreeNamesForMatching() const;

  /**
   * @brief Get nominal (i.e. name == "NOSYS") systematic
   *
   * @return const std::shared_ptr<Systematic>&
   */
  const std::shared_ptr<Systematic>& nominalSystematic() const;

  /**
   * @brief Add custom new column from the config
   *
   * @param newName name of the new column
   * @param formula the formula for the new column
   */
  inline void addCustomDefine(const std::string& newName,
                              const std::string& formula) {
                                m_customRecoDefines.emplace_back(std::make_shared<ConfigDefine>(newName, formula));
                              }

  /**
   * @brief Add custom new column to a truth tree from the config
   *
   * @param newName name of the new column
   * @param formula the formula for the new column
   * @param treeName the targer tree
   */
  inline void addCustomTruthDefine(const std::string& newName,
                                   const std::string& formula,
                                   const std::string& treeName) {
                                    m_customTruthDefines.emplace_back(std::make_shared<ConfigDefine>(newName, formula, treeName));
                                   }
  /**
   * @brief Get custom reco defines
   *
   * @return const std::vector<std::pair<std::string, std::string> >&
   */
  inline const std::vector<std::shared_ptr<ConfigDefine>>& customRecoDefines() const {return m_customRecoDefines;}

  /**
   * @brief Get custom truth defines
   *
   * @return const std::vector<std::pair<std::string, std::string> >&
   */
  inline const std::vector<std::shared_ptr<ConfigDefine>>& customTruthDefines() const {return m_customTruthDefines;}

  /**
   * @brief Add variable to the list of variables
   *
   * @param variable
   */
  inline void addVariable(const std::string& variable) {m_variables.emplace_back(variable);}

  /**
   * @brief Get the list of variables
   *
   * @return const std::vector<std::string>&
   */
  inline const std::vector<std::string>& variables() const {return m_variables;}

  /**
   * @brief Add regex for exclude systematics from automatic systematics
   *
   * @param name
   */
  inline void addExcludeAutomaticSystematic(const std::string& name) {m_excludeAutomaticSystematics.emplace_back(name);}

  /**
   * @brief Get the vector of regexes for automatic systematics exclusion
   *
   * @return const std::vector<std::string>&
   */
  inline const std::vector<std::string>& excludeAutomaticSystematics() const {return m_excludeAutomaticSystematics;}

  /**
   * @brief Set to true to only use nominal systematics
   *
   * @param flag
   */
  void setNominalOnly(const bool flag) {m_nominalOnly = flag;}

  /**
   * @brief Get is nominal only
   *
   * @return true
   * @return false
   */
  bool nominalOnly() const {return m_nominalOnly;}

  /**
   * @brief Set to true if all systematic uncertainties should be read from the file
   *
   * @param flag
   */
  void setAutomaticSystematics(const bool flag) {m_automaticSystematics = flag;}

  /**
   * @brief Get automaticSystematics
   *
   * @return true
   * @return false
   */
  bool automaticSystematics() const {return m_automaticSystematics;}

  /**
   * @brief Does the sample have the systematic already included?
   *
   * @param systematicName Name of the systematic to check
   * @return true
   * @return false
   */
  bool hasSystematic(const std::string& systematicName) const;

  /**
   * @brief Should the given systematic be skipped as it is excluded?
   *
   * @param systematicName Name of the systematic to check
   * @return true
   * @return false
   */
  bool skipExcludedSystematic(const std::string& systematicName) const;

  /**
   * @brief At least one truth object that uses a given tree name matched to the reco tree?
   *
   * @param treeName Name of the tree
   * @return true
   * @return false
   */
  bool matchTruthTree(const std::string& treeName) const;

  /**
   * @brief Does sample have unfolding?
   *
   * @return true
   * @return false
   */
  bool hasUnfolding() const;

  /**
   * @brief Sample is data?
   *
   * @return true
   * @return false
   */
  bool isData() const;

  /**
   * @brief Add cutflow
   *
   * @param cutflow
   */
  inline void addCutflow(const std::shared_ptr<Cutflow>& cutflow) {m_cutflows.emplace_back(cutflow);}

  /**
   * @brief Get the cutflows
   *
   * @return const std::vector<std::shared_ptr<Cutflow> >&
   */
  inline const std::vector<std::shared_ptr<Cutflow> >& cutflows() {return m_cutflows;}

  /**
   * @brief Has cutflows?
   *
   * @return true
   * @return false
   */
  inline bool hasCutflows() const {return !m_cutflows.empty();}

  /**
   * @brief Set the nominal sum weights
   *
   * @param sumw
   */
  inline void setNominalSumWeights(const std::string& sumw) {m_nominalSumWeights = sumw;}

  /**
   * @brief Get nominal sum weights
   *
   * @return const std::string&
   */
  inline const std::string& nominalSumWeights() const {return m_nominalSumWeights;}

  /**
   * @brief Get one file path
   *
   * @param manager
   * @return std::string
   */
  std::string oneFilePath(const MetadataManager& manager) const;

private:
  std::string m_name;

  std::string m_recoTreeName;

  std::string m_selectionSuffix;

  std::vector<std::shared_ptr<Region> > m_regions;

  std::vector<std::shared_ptr<Systematic> > m_systematics;

  std::vector<UniqueSampleID> m_uniqueSampleIDs;

  std::vector<std::shared_ptr<Truth> > m_truths;

  std::vector<std::string> m_reco_to_truth_pairing_indices;

  std::string m_eventWeight;

  std::vector<std::shared_ptr<ConfigDefine>> m_customRecoDefines;

  std::vector<std::shared_ptr<ConfigDefine>> m_customTruthDefines;

  std::vector<std::string> m_variables;

  std::vector<std::string> m_excludeAutomaticSystematics;

  bool m_nominalOnly = false;

  bool m_automaticSystematics = false;

  std::vector<std::shared_ptr<Cutflow> > m_cutflows;

  std::string m_nominalSumWeights;
};
