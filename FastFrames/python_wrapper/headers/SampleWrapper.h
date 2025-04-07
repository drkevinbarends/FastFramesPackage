/**
 * @file SampleWrapper.h
 * @brief Header file for the SampleWrapper class
 *
 */
#pragma once

#include "FastFrames/Sample.h"
#include "FastFrames/ConfigDefine.h"

#include <memory>
#include <string>

/**
 * @brief A wrapper class for the Sample class, providing an interface for Python.
 */
class SampleWrapper {
    public:
        /**
         * @brief Construct a new Sample Wrapper object
         *
         * @param name
         */
        explicit SampleWrapper(const std::string& name) :
            m_sample(std::make_shared<Sample>(name))  {};

        /**
         * @brief Destroy the Sample Wrapper object
         *
         */
        ~SampleWrapper() = default;

        /**
         * @brief Get raw pointer to the shared_ptr<Sample>
         *
         * @return unsigned long long int
         */
        unsigned long long int getPtr() const   {return reinterpret_cast<unsigned long long int>(&m_sample);};

        /**
         * @brief Construct SampleWrapper object from shared_ptr
         *
         * @param unsigned long long int - shared_ptr
         */
        void constructFromSharedPtr(unsigned long long int shared_ptr)  {
            m_sample = *reinterpret_cast<std::shared_ptr<Sample> *>(shared_ptr);
        };

        /**
         * @brief Get name of the sample
         *
         * @return std::string
         */
        std::string name() const {return m_sample->name();};

        /**
         * @brief Set the name of the reco-level tree
         *
         * @param name
         */
        void setRecoTreeName(const std::string& treeName) {m_sample->setRecoTreeName(treeName);};

        /**
         * @brief Get the name of the reco-level tree
         *
         * @return std::string
         */
        std::string recoTreeName() const {return m_sample->recoTreeName();};

        /**
         * @brief Set additional selection for the sample
         *
         * @param selectionSuffix
         */
        void setSelectionSuffix(const std::string& selectionSuffix) {m_sample->setSelectionSuffix(selectionSuffix);};

        /**
         * @brief Get additional selection for the sample
         *
         * @return std::string
         */
        std::string selectionSuffix() const {return m_sample->selectionSuffix();};

        /**
         * @brief Add unique sample
         *
         * @param dsid
         * @param campaign
         * @param simulation
         */
        void addUniqueSampleID(const unsigned int dsid, const std::string& campaign, const std::string& simulation) {
            m_sample->addUniqueSampleID(UniqueSampleID(dsid, campaign, simulation));
        };

        /**
         * @brief Get number of unique samples
         *
         * @return unsigned int
         */
        unsigned int nUniqueSampleIDs() const {return (m_sample->uniqueSampleIDs()).size();};

        /**
         * @brief Get std::string with identifier of unique sample (dsid, campaign, simulation)
         *
         * @param i
         * @return std::string
         */
        std::string uniqueSampleIDstring(unsigned int i) const {
            const std::vector<UniqueSampleID> &uniqueSamples = m_sample->uniqueSampleIDs();
            const UniqueSampleID &id = uniqueSamples.at(i);
            return "(" + std::to_string(id.dsid()) + "," + id.campaign() + "," + id.simulation() + ")";
        };

        /**
         * @brief Add systematic uncertainty, given the raw pointer to the shared_ptr<Systematic>
         *
         * @param syst_shared_ptr_int - raw pointer to the shared_ptr<Systematic>
         */
        void addSystematic(unsigned long long int syst_shared_ptr_int) {
            const std::shared_ptr<Systematic> *syst = reinterpret_cast<std::shared_ptr<Systematic> *>(syst_shared_ptr_int);
            m_sample->addSystematic(*syst);
        };

        /**
         * @brief Get number of systematic variations defined for the sample
         *
         * @return unsigned int
         */
        unsigned int nSystematics() const {return (m_sample->systematics()).size();};

        /**
         * @brief Get raw pointer to the shared_ptr<Systematic> object
         *
         * @param i
         * @return unsigned long long int
         */
        unsigned long long getSystematicPtr(unsigned int i) const {
            const std::vector<std::shared_ptr<Systematic>> &systematics = m_sample->systematics();
            const std::shared_ptr<Systematic> &syst = systematics.at(i);
            return reinterpret_cast<unsigned long long int>(&syst);
        };

        /**
         * @brief Get std::vector<std::string> of systematic names defined for the sample
         *
         */
        std::vector<std::string> systematicsNames() const {
            const std::vector<std::shared_ptr<Systematic>> &systematics = m_sample->systematics();
            std::vector<std::string> syst_names(systematics.size());
            for (unsigned int i = 0; i < systematics.size(); ++i) {
                syst_names.at(i) = systematics.at(i)->name();
            }
            return syst_names;
        };


        /**
         * @brief Get vector of raw pointer to shared pointers to systematic objects
         *
         * @return std::vector<unsigned long long int>
         */
        std::vector<unsigned long long int> getSystematicsSharedPtr() const {
            const std::vector<std::shared_ptr<Systematic>> &systematics = m_sample->systematics();
            std::vector<unsigned long long int> systematics_ptr(systematics.size());
            for (unsigned int i = 0; i < systematics.size(); ++i) {
                systematics_ptr.at(i) = reinterpret_cast<unsigned long long int>(&systematics.at(i));
            }
            return systematics_ptr;
        };


        /**
         * @brief is the systematics defined for this sample?
        */
        bool hasSystematics(const std::string &syst_name) const {
            const std::vector<std::shared_ptr<Systematic>> &systematics = m_sample->systematics();
            for (const std::shared_ptr<Systematic> &syst : systematics) {
                if (syst_name == syst->name()) {
                    return true;
                };
            }
            return false;
        };

        /**
         * @brief Add region, given the raw pointer to the shared_ptr<Region>
         *
         * @param reg_shared_ptr_int
         */
        void addRegion(unsigned long long int reg_shared_ptr_int) {
            const std::shared_ptr<Region> *reg = reinterpret_cast<std::shared_ptr<Region> *>(reg_shared_ptr_int);
            m_sample->addRegion(*reg);
        };

        /**
         * @brief Get the names of the regions where the same is to be used
         *
         * @return std::vector<std::string>
         */
        std::vector<std::string> regionsNames()    const {
            const std::vector<std::shared_ptr<Region>> &regions_shared_ptrs = m_sample->regions();
            std::vector<std::string> region_names(regions_shared_ptrs.size());
            for (unsigned int i = 0; i < regions_shared_ptrs.size(); ++i) {
                region_names.at(i) = regions_shared_ptrs.at(i)->name();
            }
            return region_names;
        };

        std::vector<unsigned long long int> regions() const {
            const std::vector<std::shared_ptr<Region>> &regions_shared_ptrs = m_sample->regions();
            std::vector<unsigned long long int> region_ptrs;
            for (const std::shared_ptr<Region> &region : regions_shared_ptrs) {
                region_ptrs.push_back(reinterpret_cast<unsigned long long int>(&region));
            }
            return region_ptrs;
        };


        /**
         * @brief Set vector<string> of variables used for reco-to-truth pairing (e.g. {"mcChannelNumber", "eventNumber"})
         *
         * @param indices
         */
        void setRecoToTruthPairingIndices(const std::vector<std::string>& indices) {
            m_sample->setRecoToTruthPairingIndices(indices);
        };


        /**
         * @brief Get vector<string> of variables used for reco-to-truth pairing (e.g. {"mcChannelNumber", "eventNumber"})
         *
         * @param std::vector<std::string>
         */
        std::vector<std::string> recoToTruthPairingIndices() const {
            return m_sample->recoToTruthPairingIndices();
        };

        /**
         * @brief Set the event_weight for the sample
         *
         * @param string
         */
        void setEventWeight(const std::string& weight) {m_sample->setEventWeight(weight);};

        /**
         * @brief Get the event_weight for the sample
         *
         * @return std::string
         */
        std::string weight() const {return m_sample->weight();};

        /**
         * @brief Given the raw pointers to the shared_ptr<Systematic> and shared_ptr<Region> objects, should the systematic be applied to the region?
         *
         * @param unsigned long long int - syst_shared_ptr_int
         * @param unsigned long long int - reg_shared_ptr_int
         * @return bool
         */
        bool skipSystematicRegionCombination(unsigned long long int syst_shared_ptr_int, unsigned long long int reg_shared_ptr_int) const {
            const std::shared_ptr<Systematic> *syst = reinterpret_cast<std::shared_ptr<Systematic> *>(syst_shared_ptr_int);
            const std::shared_ptr<Region> *reg = reinterpret_cast<std::shared_ptr<Region> *>(reg_shared_ptr_int);
            return m_sample->skipSystematicRegionCombination(*syst, *reg);
        };

        /**
         * @brief Add truth object, given the raw pointers to the shared_ptr<Truth>
         *
         * @param truth_shared_ptr_int
         */
        void addTruth(unsigned long long int truth_shared_ptr_int) {
            const std::shared_ptr<Truth> *truth = reinterpret_cast<std::shared_ptr<Truth> *>(truth_shared_ptr_int);
            m_sample->addTruth(*truth);
        };

        /**
         * @brief Get std::vector<unsigned long long int> of raw pointers to shared_ptr<Truth> objects
         *
         * @return std::vector<unsigned long long int>
         */
        std::vector<unsigned long long int> getTruthSharedPtrs() const {
            const std::vector<std::shared_ptr<Truth>> &truths = m_sample->truths();
            std::vector<unsigned long long int> truth_ptrs;
            for (const std::shared_ptr<Truth> &truth : truths) {
                truth_ptrs.emplace_back(reinterpret_cast<unsigned long long int>(&truth));
            }
            return truth_ptrs;
        };

        /**
         * @brief Add custom column to the sample, given the name and formula
         *
         * @param newName
         * @param formula
         */
        void addCustomDefine(const std::string &newName, const std::string &formula) {
            m_sample->addCustomDefine(newName, formula);
        };


        /**
         * @brief Add custom new column to a truth tree from the config
         *
         * @param newName name of the new column
         * @param formula the formula for the new column
         * @param treeName the targer tree
         */
        inline void addCustomTruthDefine(const std::string& newName, const std::string& formula, const std::string& treeName) {
            m_sample->addCustomTruthDefine(newName, formula, treeName);
        };

        /**
         * @brief Get vector of pointers (as unsigned long long) to the shared_ptr<ConfigDefine> objects
         *
         * @return std::vector<unsigned long long>
         */
        std::vector<unsigned long long> customRecoDefines() const {
            const std::vector<std::shared_ptr<ConfigDefine>> &defines = m_sample->customRecoDefines();
            std::vector<unsigned long long> result(defines.size(),0);
            for (unsigned int i = 0; i < defines.size(); ++i) {
                result.at(i) = reinterpret_cast<unsigned long long int>(&defines.at(i));
            }
            return result;
        };

        /**
         * @brief Get vector of pointers (as unsigned long long) to the shared_ptr<ConfigDefine> objects
         *
         * @return std::vector<unsigned long long>
         */
        std::vector<unsigned long long> customTruthDefines() const {
            const std::vector<std::shared_ptr<ConfigDefine>> &defines = m_sample->customTruthDefines();
            std::vector<unsigned long long> result(defines.size(),0);
            for (unsigned int i = 0; i < defines.size(); ++i) {
                result.at(i) = reinterpret_cast<unsigned long long int>(&defines.at(i));
            }
            return result;
        };

        /**
         * @brief Add variable to the list of variables
         *
         * @param variable
         */
        void addVariable(const std::string& variable)   {
            m_sample->addVariable(variable);
        };

        /**
         * @brief Get the list of variables
         *
         * @return const std::vector<std::string>
         */
        std::vector<std::string> variables() const {
            return m_sample->variables();
        };


        /**
         * @brief Add regex for exclude systematics from automatic systematics
         *
         * @param name
         */
        void addExcludeAutomaticSystematic(const std::string& name) {
            m_sample->addExcludeAutomaticSystematic(name);
        };

        /**
         * @brief Get the vector of regexes for automatic systematics exclusion
         *
         * @return std::vector<std::string>
         */
        std::vector<std::string> excludeAutomaticSystematics() const {
            return m_sample->excludeAutomaticSystematics();
        };

        /**
         * @brief Set the Nominal Only object
         *
         * @param bool nominalOnly
         */
        void setNominalOnly(bool nominalOnly) {
            m_sample->setNominalOnly(nominalOnly);
        };

        /**
         * @brief Run nominal only?
         *
         * @return bool
         */
        bool nominalOnly() const {
            return m_sample->nominalOnly();
        };

        /**
         * @brief Set the Automatic Systematics flag
         *
         * @param automaticSystematics
         */
        void setAutomaticSystematics(bool automaticSystematics) {
            m_sample->setAutomaticSystematics(automaticSystematics);
        };

        /**
         * @brief Should the systematics be read automatically from the input ROOT file?
         *
         * @return true
         * @return false
         */
        bool automaticSystematics() const {
            return m_sample->automaticSystematics();
        };


        /**
         * @brief Add cutflow
         *
         * @param cutflow
         */
        inline void addCutflow(unsigned long long int cutflow_shared_ptr_int) {
            const std::shared_ptr<Cutflow> *cutflow = reinterpret_cast<std::shared_ptr<Cutflow> *>(cutflow_shared_ptr_int);
            m_sample->addCutflow(*cutflow);
        }

        /**
         * @brief Get the cutflows shared pointers
         *
         * @return const std::vector<std::shared_ptr<Cutflow> >&
         */
        inline const std::vector<unsigned long long int> getCutflowSharedPtrs() {
            const std::vector<std::shared_ptr<Cutflow>> &cutflows = m_sample->cutflows();
            std::vector<unsigned long long int> cutflow_ptrs;
            for (const std::shared_ptr<Cutflow> &cutflow : cutflows) {
                cutflow_ptrs.push_back(reinterpret_cast<unsigned long long int>(&cutflow));
            }
            return cutflow_ptrs;
        }

        /**
         * @brief Has cutflows?
         *
         * @return true
         * @return false
         */
        inline bool hasCutflows() const {return m_sample->hasCutflows();};

        /**
         * @brief Set the nominal sum weights
         *
         * @param sumw
         */
        inline void setNominalSumWeights(const std::string& sumw)   {
            m_sample->setNominalSumWeights(sumw);
        };


        /**
         * @brief Get nominal sum weights
         *
         * @return std::string
         */
        inline std::string nominalSumWeights() const {
            return m_sample->nominalSumWeights();
        };


    private:
        std::shared_ptr<Sample> m_sample;

};