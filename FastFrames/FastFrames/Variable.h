/**
 * @file Variable.h
 * @brief Variable
 *
 */

#pragma once

#include "FastFrames/Binning.h"

#include "ROOT/RDF/HistoModels.hxx"

#include <string>

/**
 * @brief Type of varaible needed to avoid JITing
 * 
 */
enum class VariableType {
  UNDEFINED = 0,
  CHAR,
  UNSIGNED_CHAR,
  BOOL,
  INT,
  UNSIGNED_INT,
  LONG_INT,
  UNSIGNED,
  LONG_UNSIGNED,
  FLOAT,
  DOUBLE,
  VECTOR_CHAR,
  VECTOR_UNSIGNED_CHAR,
  // VECTOR_BOOL, THIS IS NOT A C++ PROPER CONTAINER! DO NOT USE!
  VECTOR_INT,
  VECTOR_UNSIGNED_INT,
  VECTOR_LONG_INT,
  VECTOR_UNSIGNED,
  VECTOR_LONG_UNSIGNED,
  VECTOR_FLOAT,
  VECTOR_DOUBLE,
  RVEC_CHAR,
  RVEC_UNSIGNED_CHAR,
  RVEC_INT,
  RVEC_UNSIGNED_INT,
  RVEC_LONG_INT,
  RVEC_UNSIGNED,
  RVEC_LONG_UNSIGNED,
  RVEC_FLOAT,
  RVEC_DOUBLE,
};

/**
 * @brief Class responsible for the varaible definition
 *
 */
class Variable {
public:

  /**
   * @brief Construct a new Variable object
   *
   * @param name Name of the variable
   */
  explicit Variable(const std::string& name) noexcept;

  /**
   * @brief Deleted default constructor
   *
   */
  Variable()  = delete;

  /**
   * @brief Destroy the Variable object
   *
   */
  ~Variable() = default;

  /**
   * @brief Get the name of the variable
   *
   * @return const std::string&
   */
  inline const std::string& name() const {return m_name;}

  /**
   * @brief Set the Definition of the variable (the column name)
   *
   * @param definition
   */
  inline void setDefinition(const std::string& definition) {m_definition = definition;}

  /**
   * @brief Get the definition of the variable (column name)
   *
   * @return const std::string&
   */
  inline const std::string& definition() const {return m_definition;}

  /**
   * @brief Set the title of the axes (histo title;X axis title;Y axis title)
   *
   * @param title
   */
  inline void setTitle(const std::string& title) {m_title = title;}

  /**
   * @brief Get the title
   *
   * @return const std::string&
   */
  inline const std::string& title() const {return m_title;}

  /**
   * @brief Set the Binning object (constant)
   *
   * @param min axis min
   * @param max axis max
   * @param nbins number of bins
   */
  inline void setBinning(const double min, const double max, const int nbins) {
    m_binning.setBinning(min, max, nbins);
  }

  /**
   * @brief Set the Binning object (variable bin edges)
   *
   * @param edges Bin edges
   */
  inline void setBinning(const std::vector<double>& edges) {
    m_binning.setBinning(edges);
  }

  /**
   * @brief Tells if the binning is constant or not
   *
   * @return true
   * @return false
   */
  inline bool hasRegularBinning() const {return m_binning.hasRegularBinning();}

  /**
   * @brief Get the bin edges
   *
   * @return const std::vector<double>&
   */
  inline const std::vector<double>& binEdges() const {return m_binning.binEdges();}

  /**
   * @brief Get axis min
   *
   * @return double
   */
  inline double axisMin() const {return m_binning.min();}

  /**
   * @brief Get axis max
   *
   * @return double
   */
  inline double axisMax() const {return m_binning.max();}

  /**
   * @brief Get the number of bins
   *
   * @return int
   */
  inline int axisNbins() const {return m_binning.nbins();}

  /**
   * @brief Get the 1D histogram model that is needed by RDataFrame
   *
   * @return ROOT::RDF::TH1DModel
   */
  ROOT::RDF::TH1DModel histoModel1D() const;

  /**
   * @brief Set the Is Nominal Only flag
   *
   * @param flag
   */
  inline void setIsNominalOnly(const bool flag) {m_isNominalOnly = flag;}

  /**
   * @brief is nominal only?
   *
   * @return true
   * @return false
   */
  inline bool isNominalOnly() const {return m_isNominalOnly;}

  /**
   * @brief Set the Type object
   * 
   * @param type 
   */
  inline void setType(const VariableType type) {m_type = type;}

  /**
   * @brief variable type
   * 
   * @return VariableType 
   */
  inline VariableType type() const {return m_type;}

private:
  std::string m_name;
  std::string m_definition;
  std::string m_title;
  Binning m_binning;
  bool m_isNominalOnly;
  VariableType m_type;
};
