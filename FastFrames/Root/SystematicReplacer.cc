/**
 * @file SystematicReplacer.cc
 * @brief Management of the string replacements for systematics
 *
 */

#include "FastFrames/SystematicReplacer.h"

#include "FastFrames/Logger.h"
#include "FastFrames/Systematic.h"
#include "FastFrames/StringOperations.h"

#include "TFile.h"
#include "TTree.h"

#include <algorithm>
#include <exception>

SystematicReplacer::SystematicReplacer() noexcept
{
}

void SystematicReplacer::readSystematicMapFromFile(const std::string& path,
                                                   const std::string& treeName,
                                                   const std::vector<std::shared_ptr<Systematic> >& systematics) {
    m_systImpactsBranches.clear();
    m_branchesAffectedBySyst.clear();
    m_allBranches.clear();
    std::unique_ptr<TFile> file(TFile::Open(path.c_str(), "read"));
    if (!file) {
        LOG(ERROR) << "Cannot open ROOT file at: " << path << "\n";
        throw std::invalid_argument("");
    }

    this->getBranchesFromFile(file, treeName);
    this->matchSystematicVariables(m_allBranches, systematics);

    file->Close();
}

void SystematicReplacer::getBranchesFromFile(const std::unique_ptr<TFile>& file,
                                             const std::string& treeName) {

    TTree* tree = file->Get<TTree>(treeName.c_str());
    if (!tree) {
        LOG(ERROR) << "Cannot read TTree: " << treeName << "\n";
        throw std::invalid_argument("");
    }

    const TObjArray* const branchList = tree->GetListOfBranches();
    std::size_t branchSize = tree->GetNbranches();
    for (std::size_t ibranch = 0; ibranch < branchSize; ++ibranch) {
        const std::string name = branchList->At(ibranch)->GetName();
        m_allBranches.emplace_back(std::move(name));
    }
}

void SystematicReplacer::matchSystematicVariables(const std::vector<std::string>& variables,
                                                  const std::vector<std::shared_ptr<Systematic> >& systematics) {

    for (const auto& isyst : systematics) {
        const std::string& systName = isyst->name();
        std::vector<std::string> affectedBranches;
        for (const auto& ivariable : variables) {
            // the systematic substring should be the suffix
            if (!StringOperations::stringEndsWith(ivariable, systName)) continue;

            // if it is found, we need to replace the systematic suffix with "NOSYS:
            const std::string nominalBranch = StringOperations::replaceString(ivariable, systName, "NOSYS");
            affectedBranches.emplace_back(nominalBranch);
        }

        m_systImpactsBranches.insert({systName, affectedBranches});
    }

    // now do it the other way around
    for (const auto& ielement : m_systImpactsBranches) {
        for (const auto& ivariable : ielement.second) {
            auto itr = m_branchesAffectedBySyst.find(ivariable);
            if (itr == m_branchesAffectedBySyst.end()) {
                m_branchesAffectedBySyst.insert({ivariable, {ielement.first}});
            } else {
                itr->second.emplace_back(ielement.first);
            }
        }
    }
}

std::string SystematicReplacer::replaceString(const std::string& original, const std::shared_ptr<Systematic>& systematic) const {
    return this->replaceString(original, systematic->name());
}

std::string SystematicReplacer::replaceString(const std::string& original, const std::string& systematicName) const {
    auto itr = m_systImpactsBranches.find(systematicName);
    if (itr == m_systImpactsBranches.end()) {
        LOG(ERROR) << "Cannot find systematic: " << systematicName << " in the systematic map. Please, fix!\n";
        throw std::invalid_argument("");
    }

    std::string result(original);

    // loop over all affected branches and replace all of them
    for (const std::string& ibranch : itr->second) {
        const std::string replacer = StringOperations::replaceString(ibranch, "NOSYS", systematicName);
        result = StringOperations::replaceString(result, ibranch, replacer);
    }

    return result;
}

std::vector<std::string> SystematicReplacer::replaceVector(const std::vector<std::string>& originalVector, const std::string& systematicName) const {
    std::vector<std::string> result;
    for (const auto& ielement : originalVector) {
        result.emplace_back(this->replaceString(ielement, systematicName));
    }

    return result;
}

bool SystematicReplacer::branchExists(const std::string& name) const {
    auto itr = std::find(m_allBranches.begin(), m_allBranches.end(), name);

    return itr != m_allBranches.end();
}

std::vector<std::string> SystematicReplacer::getListOfEffectiveSystematics(const std::vector<std::string>& columns) const {

    std::vector<std::string> result;

    for (const auto& icolumn : columns) {
        if (icolumn.find("NOSYS") == std::string::npos) continue;

        // get the variables from the map and then take the unique ones
        auto itr = m_branchesAffectedBySyst.find(icolumn);
        if (itr == m_branchesAffectedBySyst.end()) {
            LOG(ERROR) << "Cannot find branch: " << icolumn << ", in the branch map\n";
            throw std::invalid_argument("");
        }

        // loop over all systematics and add the unique ones
        for (const auto& isyst : itr->second) {
            auto itrResult = std::find(result.begin(), result.end(), isyst);
            if (itrResult == result.end()) {
                result.emplace_back(isyst);
            }
        }
    }

    return result;
}

void SystematicReplacer::addVariableAndEffectiveSystematics(const std::string& variable, const std::vector<std::string>& systematics) {
    if (this->branchExists(variable)) {
        LOG(DEBUG) << "Variable " << variable << " already exists, not adding it\n";
        return;
    }

    if (variable.find("NOSYS") == std::string::npos) {
        LOG(ERROR) << "Variable " << variable << " does not contain \"NOSYS\"\n";
        throw std::invalid_argument("");
    }

    // add to the list of branches
    m_allBranches.emplace_back(variable);

    // add to the maps of systematics
    m_branchesAffectedBySyst.insert({variable, systematics});

    for (const auto& isystematic : systematics) {
        auto itr = m_systImpactsBranches.find(isystematic);
        if (itr == m_systImpactsBranches.end()) {
            LOG(ERROR) << "Unknown systematic: " << isystematic << "\n";
            throw std::invalid_argument("");
        }
        itr->second.emplace_back(variable);

        const std::string systName = this->replaceString(variable, isystematic);
        if (systName == variable) continue;
        m_allBranches.emplace_back(systName);
    }
}

void SystematicReplacer::addSingleSystematic(const std::string& variable, const std::string& systematic) {
    if (variable.find("NOSYS") == std::string::npos) {
        LOG(ERROR) << "Variable " << variable << " does not contain \"NOSYS\"\n";
        throw std::invalid_argument("");
    }

    // check if the systematics is already added or not
    auto itr = m_branchesAffectedBySyst.find(variable);
    if (itr == m_branchesAffectedBySyst.end()) {
        m_branchesAffectedBySyst.insert({variable, {systematic}});
    } else {
        auto itrBranch = std::find(itr->second.begin(), itr->second.end(), systematic);
        if (itrBranch == itr->second.end()) {
            itr->second.emplace_back(systematic);
        }
    }

    auto itrSyst = m_systImpactsBranches.find(systematic);
    if (itrSyst == m_systImpactsBranches.end()) {
        m_systImpactsBranches.insert({systematic, {variable}});
    } else {
        auto vec = std::find(itrSyst->second.begin(), itrSyst->second.end(), variable);
        if (vec == itrSyst->second.end()) {
            itrSyst->second.emplace_back(variable);
        }
    }

    if (!this->branchExists(variable)) {
        m_allBranches.emplace_back(variable);
    }
}

void SystematicReplacer::addTruthBranchesNominal(const std::string& truthTreeName, const std::vector<std::string>& branches) {
    for (const auto& ibranch : branches) {
        if (ibranch.find("NOSYS") == std::string::npos) continue;

        const std::string branch = truthTreeName+"."+ibranch;

        bool isAlreadyAdded(false);
        if (this->branchExists(ibranch)) {
            // was added in the reco tree
            isAlreadyAdded = true;
        }

        // add it with the tree.branch name
        m_branchesAffectedBySyst.insert({branch, {"NOSYS"}});
        auto itrSyst = m_systImpactsBranches.find("NOSYS");
        if (itrSyst == m_systImpactsBranches.end()) {
            LOG(ERROR) << "Cannot find NOSYS systematic!\n";
            throw std::runtime_error("");
        }

        itrSyst->second.emplace_back(branch);

        if (!isAlreadyAdded) {
            // if it is unique, add it also without the "."
            m_branchesAffectedBySyst.insert({ibranch, {"NOSYS"}});
            itrSyst->second.emplace_back(ibranch);
        }
    }
}

void SystematicReplacer::updateVariableAndEffectiveSystematics(const std::string& variable, const std::vector<std::string>& systematics) {
    if (!this->branchExists(variable)) {
        LOG(DEBUG) << "Variable " << variable << " does not exist, adding it\n";
        this->addVariableAndEffectiveSystematics(variable, systematics);
        return;
    }

    if (variable.find("NOSYS") == std::string::npos) {
        LOG(ERROR) << "Variable " << variable << " does not contain \"NOSYS\"\n";
        throw std::invalid_argument("");
    }

    // remove variable from any systematics that no longer affect it
    for (const auto& isyst : m_branchesAffectedBySyst[variable]) {
        if (std::find(systematics.begin(), systematics.end(), isyst) == systematics.end()) {
            LOG(DEBUG) << "Systematic " << isyst << " no longer affects variable " << variable << "\n";
            m_systImpactsBranches[isyst].erase(std::remove(m_systImpactsBranches[isyst].begin(), m_systImpactsBranches[isyst].end(), variable), m_systImpactsBranches[isyst].end());
        }
    }

    // add variable to any systematics that now affect it
    for (const auto& isyst : systematics) {
        if (std::find(m_branchesAffectedBySyst[variable].begin(), m_branchesAffectedBySyst[variable].end(), isyst) == m_branchesAffectedBySyst[variable].end()) {
            LOG(DEBUG) << "Systematic " << isyst << " now affects variable " << variable << "\n";
            m_systImpactsBranches[isyst].emplace_back(variable);
        }
    }

    // overwrite old systematics list with new one
    m_branchesAffectedBySyst[variable] = systematics;
}

void SystematicReplacer::printMaps() const {

    LOG(VERBOSE) << "List of branches and systematics that affect them\n";
    for (const auto& ibranch : m_branchesAffectedBySyst) {
        LOG(VERBOSE) << "branch: " << ibranch.first << "\n";
        for (const auto& isyst : ibranch.second) {
            LOG(VERBOSE) << "\t systematic: " << isyst << "\n";
        }
    }

    LOG(VERBOSE) << "\n";
    LOG(VERBOSE) << "List of systematics and which branches they affect\n";
    for (const auto& isyst : m_systImpactsBranches) {
        LOG(VERBOSE) << "systematic: " << isyst.first << "\n";
        for (const auto& ibranch : isyst.second) {
            LOG(VERBOSE) << "\t branch: " << ibranch << "\n";
        }
    }
}

std::vector<std::string> SystematicReplacer::listOfVariablesAffected(const std::string& input) const {

    std::vector<std::string> result;

    for (const auto& ivariables : m_branchesAffectedBySyst) {
        if (input.find(ivariables.first) != std::string::npos) {
            result.emplace_back(ivariables.first);
        }
    }

    return result;
}

std::vector<std::string> SystematicReplacer::nominalBranches(const std::vector<std::string>& branches) const {
    std::vector<std::string> result;

    for (const auto& ibranch : branches) {
        if (!this->isNominalBranch(ibranch)) continue;

        result.emplace_back(ibranch);
    }

    return result;
}

bool SystematicReplacer::isNominalBranch(const std::string& branch) const {
    if (branch.find("NOSYS") != std::string::npos) return true;

    // if it does not contain NOSYS, it might either be directly a systematic or something which is not affected by systematics
    if (this->isSystematicVariation(branch)) return false;

    return true;
}

bool SystematicReplacer::isSystematicVariation(const std::string& branch) const {
    // check all systematics and see if the branch contains the substring
    auto itr = std::find_if(m_systematics.begin(), m_systematics.end(), [&branch](const auto& element){return branch.find(element) != std::string::npos;});
    return itr != m_systematics.end();
}