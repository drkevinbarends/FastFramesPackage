/**
 * @file MainFrame.h
 * @brief Main class responsible for histogramming and ntupling
 *
 */

#pragma once

#include "FastFrames/ConfigSetting.h"
#include "FastFrames/CutflowContainer.h"
#include "FastFrames/HistoContainer.h"
#include "FastFrames/MetadataManager.h"
#include "FastFrames/StringOperations.h"
#include "FastFrames/SystematicReplacer.h"
#include "FastFrames/Truth.h"

#include "ROOT/RDataFrame.hxx"
#include "TClass.h"

#include <memory>
#include <string>
#include <tuple>

class Variable;
class TTreeIndex;

/**
 * @brief Main class that does all the hard work
 *
 */
class MainFrame {
public:

  /**
   * @brief Construct a new Main Frame object
   *
   */
  explicit MainFrame() : m_config(std::make_shared<ConfigSetting>()) {};

  /**
   * @brief Destroy the Main Frame object
   *
   */
  virtual ~MainFrame() = default;

  /**
   * @brief Set the Config object
   *
   * @param config
   */
  virtual void setConfig(const std::shared_ptr<ConfigSetting>& config) {m_config = config;}

  /**
   * @brief Run all the steps needed at the beggining of the code
   *
   */
  virtual void init();

  /**
   * @brief Method to process histograms
   *
   */
  virtual void executeHistograms();

  /**
   * @brief Method to produce ntuples
   *
   */
  virtual void executeNtuples();

  /**
   * @brief Allows to define new observables for ntupling
   * Users can override this and add their own variables
   *
   * @param node The input RDF node
   * @return ROOT::RDF::RNode the output node containg the new columns
   */
  virtual ROOT::RDF::RNode defineVariablesNtuple(ROOT::RDF::RNode node,
                                                 const std::shared_ptr<Sample>& /*sample*/,
                                                 const UniqueSampleID& /*sampleID*/) {return node;}

  /**
   * @brief Allows to define new observables
   * Users can override this and add their own variables
   *
   * @param node The input RDF node
   * @return ROOT::RDF::RNode the output node containg the new columns
   */
  virtual ROOT::RDF::RNode defineVariables(ROOT::RDF::RNode node,
                                           const std::shared_ptr<Sample>& /*sample*/,
                                           const UniqueSampleID& /*sampleID*/) {return node;}

  /**
   * @brief Allows to define new columns for specific regions
   * These columns will be attached only after the filters
   *
   * @param node The input RDF node (after the filter)
   * @return ROOT::RDF::RNode the output node containing the new columns
   */
  virtual ROOT::RDF::RNode defineVariablesRegion(ROOT::RDF::RNode node,
                                                 const std::shared_ptr<Sample>&,
                                                 const UniqueSampleID&,
                                                 const std::string&) {return node;}

  /**
   * @brief Allows to define new obserbables for truth trees
   *
   * @param node The input RDF node
   *
   * @return ROOT::RDF::RNode the output node containing the new columns
   */
  virtual ROOT::RDF::RNode defineVariablesTruth(ROOT::RDF::RNode node,
                                                const std::string& /*treeName*/,
                                                const std::shared_ptr<Sample>& /*sample*/,
                                                const UniqueSampleID& /*sampleID*/) {return node;}


  /**
   * @brief Allows to define a custom variable used for ntupling for truth trees
   *
   * @param node The input RDF node
   *
   * @return ROOT::RDF::RNode the output node containing the new columns
   */
  virtual ROOT::RDF::RNode defineVariablesNtupleTruth(ROOT::RDF::RNode node,
                                                      const std::string& /*treeName*/,
                                                      const std::shared_ptr<Sample>& /*sample*/,
                                                      const UniqueSampleID& /*sampleID*/) {return node;}

  /**
   * @brief A helper method that make systematic copies of a provided nominal column
   * Name of the new variable has to contain _NOSYS
   *
   * @tparam F
   * @param node Input node
   * @param newVariable Name of the new variable
   * @param defineFunction Actual function to be used for the variable definition
   * @param branches List of branch names that the function processes
   * @return ROOT::RDF::RNode Output node with the new columns
   */
  template<typename F>
  ROOT::RDF::RNode systematicDefine(ROOT::RDF::RNode node,
                                    const std::string& newVariable,
                                    F defineFunction,
                                    const std::vector<std::string>& branches) {

    if (newVariable.find("NOSYS") == std::string::npos) {
      LOG(ERROR) << "The new variable name: \"" << newVariable << "\" does not contain \"NOSYS\"\n";
      throw std::invalid_argument("");
    }

    bool variableExists(false);
    if (m_systReplacer.branchExists(newVariable)) {
      LOG(VERBOSE) << "Variable: " << newVariable << " is already in the input, will not add it to the map (but adding it to the node)\n";
      variableExists = true;
    }

    // first add the nominal define
    node = node.Define(newVariable, defineFunction, branches);

    // add systematics
    // get list of all systeamtics affecting the inputs
    std::vector<std::string> effectiveSystematics = m_systReplacer.getListOfEffectiveSystematics(branches);

    for (const auto& isystematic : effectiveSystematics) {
      if (isystematic == "NOSYS") continue;
      const std::string systName = StringOperations::replaceString(newVariable, "NOSYS", isystematic);
      const std::vector<std::string> systBranches = m_systReplacer.replaceVector(branches, isystematic);
      node = node.Define(systName, defineFunction, systBranches);
    }

    // tell the replacer about the new columns
    if (!variableExists) {
      m_systReplacer.addVariableAndEffectiveSystematics(newVariable, effectiveSystematics);
    }

    return node;
  }

  /**
   * @brief A helper method that make systematic copies of a provided nominal column
   * Name of the new variable has to contain _NOSYS
   * This method uses `DefineSlot` instead of `Define` for implementing thread-safe operations.
   *
   * @tparam F
   * @param node Input node
   * @param newVariable Name of the new variable
   * @param defineFunction Actual function to be used for the variable definition (the first argument has to be `unsigned int` slot)
   * @param branches List of branch names that the function processes
   * @return ROOT::RDF::RNode Output node with the new columns
   */
  template<typename F>
  ROOT::RDF::RNode systematicDefineSlot(ROOT::RDF::RNode node,
                                    const std::string& newVariable,
                                    F defineFunction,
                                    const std::vector<std::string>& branches) {

    if (newVariable.find("NOSYS") == std::string::npos) {
      LOG(ERROR) << "The new variable name: \"" << newVariable << "\" does not contain \"NOSYS\"\n";
      throw std::invalid_argument("");
    }

    bool variableExists(false);
    if (m_systReplacer.branchExists(newVariable)) {
      LOG(VERBOSE) << "Variable: " << newVariable << " is already in the input, will not add it to the map (but adding it to the node)\n";
      variableExists = true;
    }

    // first add the nominal define
    node = node.DefineSlot(newVariable, defineFunction, branches);

    // add systematics
    // get list of all systeamtics affecting the inputs
    std::vector<std::string> effectiveSystematics = m_systReplacer.getListOfEffectiveSystematics(branches);

    for (const auto& isystematic : effectiveSystematics) {
      if (isystematic == "NOSYS") continue;
      const std::string systName = StringOperations::replaceString(newVariable, "NOSYS", isystematic);
      const std::vector<std::string> systBranches = m_systReplacer.replaceVector(branches, isystematic);
      node = node.DefineSlot(systName, defineFunction, systBranches);
    }

    // tell the replacer about the new columns
    if (!variableExists) {
      m_systReplacer.addVariableAndEffectiveSystematics(newVariable, effectiveSystematics);
    }

    return node;
  }
  /**
   * @brief Helper function for systematic define that only adds columns if the input columns exist
   *
   * @tparam F
   * @param node Input node
   * @param newVariable Name of the new column
   * @param defineFunction Functor to be used for the column
   * @param branches Branches the functor depends on
   * @return ROOT::RDF::RNode Output node
   */
  template<typename F>
  ROOT::RDF::RNode systematicDefineNoCheck(ROOT::RDF::RNode node,
                                           const std::string& newVariable,
                                           F defineFunction,
                                           const std::vector<std::string>& branches) {

    // check of the branches exist, if not then do not do anything
    bool missing(false);
    for (const auto& ibranch : branches) {
      if (!m_systReplacer.branchExists(ibranch)) {
        LOG(WARNING) << "Branch: " << ibranch << " used in the custom Define() does not exist for this sample, will not add the new column: " << newVariable << "!\n";
        missing = true;
        break;
      }
    }

    if (missing) return node;

    return this->systematicDefine(node, newVariable, defineFunction, branches);
  }

  /**
   * @brief Define new variable (column) using a string. The code will create a replica
   * for every systematic variation that affecgts the formula
   *
   * @param mainNode The input RDF node
   * @param newName Name of the new column, has to contain "NOSYS"
   * @param formula The formula (using nominal branches)
   * @return ROOT::RDF::RNode modified node
   */
  ROOT::RDF::RNode systematicStringDefine(ROOT::RDF::RNode mainNode,
                                          const std::string& newName,
                                          const std::string& formula);

  /**
   * @brief Define an existing variable (column) and systematic copies.
   * NOTE: Name of the new variable has to contain _NOSYS.
   * NOTE: If you try to redefine a variable which does not exist in the input, it will create
   * a new variable using systematicDefine.
   *
   * @tparam F
   * @param node Input node
   * @param variable Name of the variable
   * @param defineFunction Actual function to be used for the variable definition
   * @param branches List of branch names that the function processes
   * @return ROOT::RDF::RNode Output node with the new columns
   */
  template<typename F>
  ROOT::RDF::RNode systematicRedefine(ROOT::RDF::RNode node,
                                      const std::string& variable,
                                      F defineFunction,
                                      const std::vector<std::string>& branches) {

    if (variable.find("NOSYS") == std::string::npos) {
      LOG(ERROR) << "The new variable name: \"" << variable << "\" does not contain \"NOSYS\"\n";
      throw std::invalid_argument("");
    }

    // The usage of Define() vs. Redefine() just depends on whether or not the
    // variable has already been defined in the RDF. The result for
    // m_systReplacer is the same either way: it just keeps track of the nominal
    // and syst columns. So we dont do m_systReplacer.branchExists() here,
    // instead we look at mainNode.GetColumnNames() to tell us if the column
    // needs to be defined or redefined.
    auto columnNames = node.GetColumnNames();
    if (std::find(columnNames.begin(), columnNames.end(), variable) == columnNames.end()) {
      LOG(VERBOSE) << "No variable " << variable << " exists to redefine, making new variable instead\n";
      return systematicDefine(node, variable, defineFunction, branches);
    }

    // first add the nominal define
    node = node.Redefine(variable, defineFunction, branches);

    // add systematics
    // get list of all systematics affecting the inputs
    std::vector<std::string> effectiveSystematics = m_systReplacer.getListOfEffectiveSystematics(branches);

    for (const auto& isystematic : effectiveSystematics) {
      if (isystematic == "NOSYS") continue;
      const std::string systName = StringOperations::replaceString(variable, "NOSYS", isystematic);
      const std::vector<std::string> systBranches = m_systReplacer.replaceVector(branches, isystematic);

      // it is possible that redefining the variable changes systematics, so
      // we have to check if we need Define() or Redefine() here too
      if (std::find(columnNames.begin(), columnNames.end(), systName) == columnNames.end()) {
        node = node.Define(systName, defineFunction, systBranches);
      } else {
        node = node.Redefine(systName, defineFunction, systBranches);
      }
    }

    // tell the replacer about the new columns
    m_systReplacer.updateVariableAndEffectiveSystematics(variable, effectiveSystematics);

    return node;
  }

  /**
   * @brief Redefine an existing variable (column) using a string. The code will create a replica
   * for every systematic variation that affects the formula
   * NOTE: Name of the new variable has to contain _NOSYS.
   * NOTE: If you try to redefine a variable which does not exist in the input, it will create
   * a new variable using systematicStringDefine
   *
   * @param mainNode The input RDF node
   * @param newName Name of the column, has to contain "NOSYS"
   * @param formula The formula (using nominal branches)
   * @return ROOT::RDF::RNode modified node
   */
  ROOT::RDF::RNode systematicStringRedefine(ROOT::RDF::RNode mainNode,
                                            const std::string& newName,
                                            const std::string& formula);

private:

  /**
   * @brief Process one UniqueSample (dsid, campaign, simulation type)
   * This stil ldoes not trigger the event loop as the histograms just contain the pointers.
   *
   * @param sample
   * @param uniqueSampleID
   * @return std::tuple<std::vector<SystematicHisto>, std::vector<VariableHisto>, std::vector<CutflowContainer>, ROOT::RDF::RNode, std::vector<std::pair<TChain*, TTreeIndex*> > >
   * The histograms, truth histograms, the main RDF node for logging, and truth tchain pointers for memory management
   */
  std::tuple<std::vector<SystematicHisto>,
             std::vector<VariableHisto>,
             std::vector<CutflowContainer>,
             ROOT::RDF::RNode,
             std::unique_ptr<TChain>,
             std::vector<std::pair<std::unique_ptr<TChain>, std::unique_ptr<TTreeIndex> > > > processUniqueSample(const std::shared_ptr<Sample>& sample,
                                                                                                                  const UniqueSampleID& uniqueSampleID);

  /**
   * @brief Process histograms when the sample is split based on UniqueSamples
   *
   * @param sample
   * @return std::tuple<std::vector<SystematicHisto>,
   * std::vector<VariableHisto>,
   * std::vector<CutflowContainer> >
   */
  std::tuple<std::vector<SystematicHisto>,
             std::vector<VariableHisto>,
             std::vector<CutflowContainer> > processHistogramsSplitPerUniqueSample(const std::shared_ptr<Sample>& sample);

  /**
   * @brief Process histograms for a single sample in one go
   *
   * @param sample
   * @return std::tuple<std::vector<SystematicHisto>,
   * std::vector<CutflowContainer> >
   * ROOT::RDF::RNode
   */
  std::tuple<std::vector<SystematicHisto>,
             std::vector<CutflowContainer>,
             ROOT::RDF::RNode> processSampleWithAllUniqueSamples(const std::shared_ptr<Sample>& sample);

  /**
   * @brief Main processing function for ntuples
   *
   * @param sample
   * @param id
   */
  void processUniqueSampleNtuple(const std::shared_ptr<Sample>& sample,
                                 const UniqueSampleID& id);
  /**
   * @brief Get name of a filter after applying the systematic replacements
   *
   * @param sample Sample for which the replacement happens (needed for selection suffix)
   * @param systematic Systematic to be used for the replacement
   * @param region Region to be used for the replacement
   * @return std::string
   */
  std::string systematicFilter(const std::shared_ptr<Sample>& sample,
                               const std::shared_ptr<Systematic>& systematic,
                               const std::shared_ptr<Region>& region) const;

  /**
   * @brief Returns OR for all systematic variation for a given nominal selection
   * This is needed for apply filters on ntuples
   *
   * @param sample
   * @return std::string
   */
  std::string systematicOrFilter(const std::shared_ptr<Sample>& sample) const;

  /**
   * @brief Get name of a variable after applying the systematic replacements
   *
   * @param systematic Variable to be used for the replacement
   * @return std::string
   */
  std::string systematicVariable(const Variable& variable,
                                 const std::shared_ptr<Systematic>& systematic) const;

  /**
   * @brief Get name of a systematic weight after applying the systematic replacements
   * The name is "weight_total_<SUFFIX>". If this does not exist for a given systematic,
   * uses "weigt_total_NOSYS"
   *
   * @param systematic Systematic to be used for the replacement
   * @return std::string
   */
  std::string systematicWeight(const std::shared_ptr<Systematic>& systematic) const;

  /**
   * @brief apply RDF filters (selections)
   *
   * @param mainNode current ROOT node
   * @param sample current Sample
   * @param id unique sample ID
   * @return std::vector<std::vector<ROOT::RDF::RNode> > Filter stored per region, per systematic
   */
  std::vector<std::vector<ROOT::RDF::RNode> > applyFilters(ROOT::RDF::RNode mainNode,
                                                           const std::shared_ptr<Sample>& sample,
                                                           const UniqueSampleID& id);

  /**
   * @brief Add columns representing the systematic event weights
   *
   * @param mainNode current ROOT note
   * @param sample current sample
   * @param id current sample identificatotr
   * @return ROOT::RDF::RNode  node with weights added
   */
  ROOT::RDF::RNode addWeightColumns(ROOT::RDF::RNode mainNode,
                                    const std::shared_ptr<Sample>& sample,
                                    const UniqueSampleID& id);

  /**
   * @brief Add one systematic weight to the RDF nodes
   * The weight contains all event weights as well as normalisation
   * (luminosity * cross_section / sumWeights)
   * The new column is called "weight_total_<SYSTSUFFIX>"
   * Adds the new colum to the list of the available variables/columns
   *
   * @param mainNode current ROOT node
   * @param sample current sample
   * @param systematic current systematic
   * @param id current sample identificator
   * @return ROOT::RDF::RNode node iwth added weight
   */
  ROOT::RDF::RNode addSingleWeightColumn(ROOT::RDF::RNode mainNode,
                                         const std::shared_ptr<Sample>& sample,
                                         const std::shared_ptr<Systematic>& systematic,
                                         const UniqueSampleID& id);

  /**
   * @brief Adds ROOT::Math::PtEtaPhiEVector for provided objects to RDF
   *
   * @param mainNode current ROOT node
   * @return ROOT::RDF::RNode node with the addedd vectors
   */
  ROOT::RDF::RNode addTLorentzVectors(ROOT::RDF::RNode mainNode);

  /**
   * @brief Adds a single ROOT::Math::PtEtaPhiEVector for provided object
   *
   * @param mainNode current ROOT node
   * @param object object name, e.g. "jet" or "el_tight"
   * @return ROOT::RDF::RNode node with the added vector for the object
   */
  ROOT::RDF::RNode addSingleTLorentzVector(ROOT::RDF::RNode mainNode,
                                           const std::string& object);

  /**
   * @brief Main code that calls the event loop
   *
   * @param filters List of nodes, each node represents per region, per systematic filter
   * @param sample current sample
   * @return std::vector<SystematicHisto> container of the histograms
   */
  std::vector<SystematicHisto> processHistograms(std::vector<std::vector<ROOT::RDF::RNode> >& filters,
                                                 const std::shared_ptr<Sample>& sample);

  /**
   * @brief Define 1D histograms with variables and systematics
   *
   * @param regionHisto RegionHisto to be filled
   * @param node Filtered node
   * @param sample Sample
   * @param region Region
   * @param systematic Systematic
   */
  void processHistograms1D(RegionHisto* regionHisto,
                           const ROOT::RDF::RNode& node,
                           const std::shared_ptr<Sample>& sample,
                           const std::shared_ptr<Region>& region,
                           const std::shared_ptr<Systematic>& systematic) const;

  /**
   * @brief Define 2D histograms with variables and systematics
   *
   * @param regionHisto RegionHisto to be filled
   * @param node Filtered node
   * @param sample Sample
   * @param region Region
   * @param systematic Systematic
   */
  void processHistograms2D(RegionHisto* regionHisto,
                           const ROOT::RDF::RNode& node,
                           const std::shared_ptr<Sample>& sample,
                           const std::shared_ptr<Region>& region,
                           const std::shared_ptr<Systematic>& systematic) const;

  /**
   * @brief Define 2D histograms with variables and systematics for unfolding
   *
   * @param regionHisto RegionHisto to be filled
   * @param node Filtered node
   * @param sample Sample
   * @param region Region
   * @param systematic Systematic
   */
  void processRecoVsTruthHistograms2D(RegionHisto* regionHisto,
                                      ROOT::RDF::RNode node,
                                      const std::shared_ptr<Sample>& sample,
                                      const std::shared_ptr<Region>& region,
                                      const std::shared_ptr<Systematic>& systematic);

  /**
   * @brief Define 3D histograms with variables and systematics
   *
   * @param regionHisto RegionHisto to be filled
   * @param node Filtered node
   * @param sample Sample
   * @param region Region
   * @param systematic Systematic
   */
  void processHistograms3D(RegionHisto* regionHisto,
                           const ROOT::RDF::RNode& node,
                           const std::shared_ptr<Sample>& sample,
                           const std::shared_ptr<Region>& region,
                           const std::shared_ptr<Systematic>& systematic) const;

  /**
   * @brief Write histogram container to a ROOT file
   *
   * @param histos histogram container
   * @param truthHistos truth histogram container
   * @param cutflowHistos cutflow container
   * @param sample current sample
   * @param allUniqueSample flag whether all unique samples are merged together
   */
  void writeHistosToFile(const std::vector<SystematicHisto>& histos,
                         const std::vector<VariableHisto>& truthHistos,
                         std::vector<CutflowContainer>& cutflowHistos,
                         const std::shared_ptr<Sample>& sample,
                         bool allUniqueSamples) const;

  /**
   * @brief Store efficiency and acceptance histograms
   *
   * @param outputFile
   * @param histos
   * @param truthHistos
   * @param sample
   */
  void writeUnfoldingHistos(TFile* outputFile,
                            const std::vector<SystematicHisto>& histos,
                            const std::vector<VariableHisto>& truthHistos,
                            const std::shared_ptr<Sample>& sample) const;

  /**
   * @brief Add systematics from a file
   *
   * @param sample Sample to be added
   * @param isNominalOnly Flag to tell the code to only add nominal or all systematics
   */
  void readAutomaticSystematics(std::shared_ptr<Sample>& sample, const bool isNominalOnly) const;

  /**
   * @brief Read all systematics from a ROOT file (stored in listOfSystematics histogram)
   *
   * @param filePath Paths to the ROOT files
   * @return std::vector<std::string>
   */
  std::vector<std::string> automaticSystematicNames(const std::vector<std::string>& filePath) const;

  /**
   * @brief Connect truth trees to the reco tree also add the truth branch names to the SystematicReplacer tracking
   *
   * @param chain The reco chain
   * @param sample Current Sample
   * @param filePaths Paths to the files
   * @result std::vector<std::pair<std::unique_ptr<TChain>, std::unique_ptr<TTreeIndex> > > the pointers for deleting
   */
  std::vector<std::pair<std::unique_ptr<TChain>, std::unique_ptr<TTreeIndex> > > connectTruthTrees(std::unique_ptr<TChain>& chain,
                                                                                                   const std::shared_ptr<Sample>& sample,
                                                                                                   const std::vector<std::string>& filePaths);

  /**
   * @brief Process truth histograms
   *
   * @param filePaths Paths to the input files
   * @param sample Current sample
   * @param id current UniqueSampleID
   * @return std::vector<VariableHisto>
   */
  std::vector<VariableHisto> processTruthHistos(const std::vector<std::string>& filePaths,
                                                const std::shared_ptr<Sample>& sample,
                                                const UniqueSampleID& id);

  /**
   * @brief Add custom variables (columns) from the config via string
   *
   * @param mainNode
   * @param sample
   * @return ROOT::RDF::RNode
   */
  ROOT::RDF::RNode addCustomDefinesFromConfig(ROOT::RDF::RNode mainNode,
                                              const std::shared_ptr<Sample>& sample);

  /**
   * @brief Add custom variables (columns) to the truth tree from the config via string
   *
   * @param mainNode
   * @param sample
   * @param treeName
   * @return ROOT::RDF::RNode
   */
  ROOT::RDF::RNode addCustomTruthDefinesFromConfig(ROOT::RDF::RNode mainNode,
                                                   const std::shared_ptr<Sample>& sample,
                                                   const std::string& treeName) const;

  /**
   * @brief Take a node and apply Range criteria if applicable
   *
   * @param node Input node
   * @return ROOT::RDF::RNode
   */
  ROOT::RDF::RNode minMaxRange(ROOT::RDF::RNode node) const;

  /**
   * @brief Book 1D histogram with proper templates
   *
   * @param node
   * @param variable
   * @param systematic
   * @return ROOT::RDF::RResultPtr<TH1D>
   */
  ROOT::RDF::RResultPtr<TH1D> book1Dhisto(ROOT::RDF::RNode node,
                                          const Variable& variable,
                                          const std::shared_ptr<Systematic>& systematic) const;

  /**
   * @brief Book 1d histograms with proper templates
   *
   * @param node
   * @param variable
   * @param truth
   * @return ROOT::RDF::RResultPtr<TH1D>
   */
  ROOT::RDF::RResultPtr<TH1D> book1DhistoTruth(ROOT::RDF::RNode node,
                                               const Variable& variable,
                                               const std::shared_ptr<Truth>& truth) const;

  /**
   * @brief Book 2D histograms using the JIT compiler
   *
   * @param node
   * @param variable1
   * @param variable2
   * @param systematic
   * @return ROOT::RDF::RResultPtr<TH2D>
   */
  ROOT::RDF::RResultPtr<TH2D> book2Dhisto(ROOT::RDF::RNode node,
                                          const Variable& variable1,
                                          const Variable& variable2,
                                          const std::shared_ptr<Systematic>& systematic) const;

  /**
   * @brief Book 3D histograms using the JIT compiler
   *
   * @param node
   * @param variable1
   * @param variable2
   * @param variable3
   * @param systematic
   * @return ROOT::RDF::RResultPtr<TH3D>
   */
  ROOT::RDF::RResultPtr<TH3D> book3Dhisto(ROOT::RDF::RNode node,
                                          const Variable& variable1,
                                          const Variable& variable2,
                                          const Variable& variable3,
                                          const std::shared_ptr<Systematic>& systematic) const;

  /**
   * @brief Book results ptr for cutflows
   *
   * @param node
   * @param sample
   * @return std::vector<CutflowContainer>
   */
  std::vector<CutflowContainer> bookCutflows(ROOT::RDF::RNode node,
                                             const std::shared_ptr<Sample>& sample) const;

  /**
   * @brief Prepare varaibles needed for the MC weight definitions (lumi, xsec, sumweights)
   *
   * @param node
   * @param sample
   * @param id
   * @return ROOT::RDF::RNode
   */
  ROOT::RDF::RNode prepareWeightMetadata(ROOT::RDF::RNode node,
                                         const std::shared_ptr<Sample>& sample,
                                         const UniqueSampleID& id) const;

  /**
   * @brief Prepare xSec, sumweights and lumi for the whole Sample
   *
   * @param node
   * @param sample
   * @return ROOT::RDF::RNode
   */
  ROOT::RDF::RNode prepareWeightMetadataAllUniqueSamples(ROOT::RDF::RNode node,
                                                         const std::shared_ptr<Sample>& sample) const;

  /**
   * @brief Add truth variables to reco tree if needed (when matching reco and truth)
   *
   * @param node Input node
   * @param sample Sample
   * @param id UniqueSampleID
   * @return ROOT::RDF::RNode
   */
  ROOT::RDF::RNode addTruthVariablesToReco(ROOT::RDF::RNode node,
                                           const std::shared_ptr<Sample>& sample,
                                           const UniqueSampleID& id);


  /**
   * @brief Add weights and custom columns to the truth nodes
   *
   * @param filePaths
   * @param sample
   * @param id
   * @return std::map<std::string, ROOT::RDF::RNode>
   */
  std::map<std::string, ROOT::RDF::RNode> prepareTruthNodes(const std::vector<std::string>& filePaths,
                                                            const std::shared_ptr<Sample>& sample,
                                                            const UniqueSampleID& id);


  /**
   * @brief Process single truth tree when running the ntupling step
   *
   * @param truth
   * @param filePaths
   * @param outputPath
   * @param sample
   * @param id
   */
  void processSingleTruthTreeNtuple(const std::shared_ptr<Truth>& truth,
                                    const std::vector<std::string>& filePaths,
                                    const std::string& outputPath,
                                    const std::shared_ptr<Sample>& sample,
                                    const UniqueSampleID& id);

  /**
   * @brief Run inference on models from simple_onnx_inference block
   *
   * @param node Input node
   * @return ROOT::RDF::RNode
   */
  ROOT::RDF::RNode scheduleSimpleONNXInference(ROOT::RDF::RNode node);

  /**
   * @brief Add columns for variables with a formula
   *
   * @param node
   * @param sample
   * @param truthTrees
   * @return ROOT::RDF::RNode
   */
  ROOT::RDF::RNode addVariablesWithFormulaReco(ROOT::RDF::RNode node,
                                               const std::shared_ptr<Sample>& sample,
                                               const std::vector<std::string>& truthTrees);

  /**
   * @brief Add columns for variables in a truth tree
   *
   * @param node
   * @param sample
   * @param treeName
   * @return ROOT::RDF::RNode
   */
  ROOT::RDF::RNode addVariablesWithFormulaTruth(ROOT::RDF::RNode node,
                                                const std::shared_ptr<Sample>& sample,
                                                const std::string& treeName);

  /**
   * @brief Load the ONNX model
   *
   */
  void prepareONNXwrapper();

protected:

  /**
   * @brief class responsible for managing metadata (lumminosity, cross-section, sumWeights, ...)
   *
   */
  MetadataManager m_metadataManager;

  /**
   * @brief holds all configuration settings
   *
   */
  std::shared_ptr<ConfigSetting> m_config;

  /**
   * @brief main tool to do the string operations for systematic variations
   *
   */
  SystematicReplacer m_systReplacer;

  /**
   * @brief Map of variables with formula for the reco tree, key = formula, value = new name
   *
   */
  std::map<std::string, std::string> m_variablesWithFormulaReco;

  /**
   * @brief Map of variables with formula for the truth tree: tree name | formula | new name
   *
   */
  std::map<std::string, std::map<std::string, std::string> > m_variablesWithFormulaTruth;

  /**
   * @brief Needed for ROOT to generate the dictionary
   *
   */
  ClassDef(MainFrame, 1);
};
