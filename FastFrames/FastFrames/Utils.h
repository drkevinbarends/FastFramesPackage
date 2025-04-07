/**
 * @file Utils.h
 * @brief Helper functions
 *
 */

#pragma once

#include "FastFrames/HistoContainer.h"
#include "FastFrames/Region.h"
#include "FastFrames/Variable.h"

#include "ROOT/RDFHelpers.hxx"
#include "TChain.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class TChain;
class TTreeIndex;
class Sample;

/**
 * @brief Helper functions
 *
 */
namespace Utils {

  /**
   * @brief Get TChain from input file paths
   *
   * @param treeName
   * @param files
   * @return TChain
   */
  std::unique_ptr<TChain> chainFromFiles(const std::string& treeName,
                                         const std::vector<std::string>& files);

  /**
   * @brief Get 2D histo model (TH2D) from variables
   *
   * @param v1
   * @param v2
   * @return ROOT::RDF::TH2DModel
   */
  ROOT::RDF::TH2DModel histoModel2D(const Variable& v1, const Variable& v2);

  /**
   * @brief Get 3D histo model (TH3D) from variables
   *
   * @param v1
   * @param v2
   * @param v3
   * @return ROOT::RDF::TH3DModel
   */
  ROOT::RDF::TH3DModel histoModel3D(const Variable& v1, const Variable& v2, const Variable& v3);

  /**
   * @brief Copy a histogram from VariableHistos based on a name of the histogram
   * This triggers event loop!
   *
   * @param histos
   * @param name
   * @return std::unique_ptr<TH1D>
   */
  std::unique_ptr<TH1D> copyHistoFromVariableHistos(const std::vector<VariableHisto>& histos,
                                                    const std::string& name);

  /**
   * @brief
   * @brief Copy a histogram from VariableHistos2d based on a name of the histogram
   * This triggers event loop!
   *
   * @param histos
   * @param name
   * @return std::unique_ptr<TH2D>
   */
  std::unique_ptr<TH2D> copyHistoFromVariableHistos2D(const std::vector<VariableHisto2D>& histos,
                                                      const std::string& name);

  /**
   * @brief Take only N files from the input file list based on an approximate split and the current index
   *
   * @param fileList Input file list
   * @param split Approximate splitting
   * @param index Current index of a job
   * @return std::vector<std::string>
   */
  std::vector<std::string> selectedFileList(const std::vector<std::string>& fileList,
                                            const int split,
                                            const int index);

  /**
   * @brief Set histogram to be between 0 and 1 in each bin
   *
   * @param h
   */
  void capHisto0And1(TH1D* h, const std::string& name);

  /**
   * @brief Get variable using region and variable name
   *
   * @param regions list of regions
   * @param regionName region name for the variable
   * @param variableName variable name
   * @return const Variable&
   */
  const Variable& getVariableByName(const std::vector<std::shared_ptr<Region> >& regions,
                                    const std::string& regionName,
                                    const std::string& variableName);


  /**
   * @brief Compare 2 doubles with a given relative precision
   *
   * @param a
   * @param b
   * @param relative_precision
   * @return bool - are the doubles equal?
   */
  bool compareDoubles(const double a, const double b, const double relative_precision = 1e-6);

  /**
   * @brief Select elements that match selected RE and do not match ecludede RE
   *
   * @param all
   * @param selected
   * @param excluded
   * @return std::vector<std::string>
   */
  std::vector<std::string> selectedNotExcludedElements(const std::vector<std::string>& all,
                                                       const std::vector<std::string>& selected,
                                                       const std::vector<std::string>& excluded);

  /**
   * @brief Get the list of defined variables that use a formula and not the column name directly
   *
   * @param node Needed to get the list of the available columns
   * @param sample Sample
   * @param truthTrees names of the TruthTrees
   * @return std::map<std::string, std::string>
   */
  std::map<std::string, std::string> variablesWithFormulaReco(ROOT::RDF::RNode node,
                                                              const std::shared_ptr<Sample>& sample,
                                                              const std::vector<std::string>& truthTrees);

    /**
   * @brief Get the list of defined variables that use a formula and not the column name directly
     *
     * @param node Needed to get the list of all available columns
     * @param sample Sample
     * @param treeName name of the tree
     * @return std::map<std::string, std::string> formula | new name
     */
  std::map<std::string, std::string> variablesWithFormulaTruth(ROOT::RDF::RNode node,
                                                               const std::shared_ptr<Sample>& sample,
                                                               const std::string& treeName);

  /**
   * @brief Check if the same branches exist between truth and reco trees based on a list of regex
   *
   * @param reco Reco chain
   * @param truth Truth chain
   * @param toCheck List of regexes
   * @return std::vector<std::string>
   */
  std::vector<std::string> matchingBranchesFromChains(const std::unique_ptr<TChain>& reco,
                                                      const std::unique_ptr<TChain>& truth,
                                                      const std::vector<std::string>& toCheck);

  /**
   * @brief Split a formula definition to the individual columns
   *
   * @param formula
   * @param node
   * @return std::vector<std::string>
   */
  std::vector<std::string> getColumnsFromString(const std::string& formula,
                                                ROOT::RDF::RNode& node);
}
