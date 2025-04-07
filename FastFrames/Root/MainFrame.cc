/**
 * @file MainFrame.cc
 * @brief Main class responsible for histogramming and ntupling
 *
 */

#include "FastFrames/MainFrame.h"

#include "FastFrames/Logger.h"
#include "FastFrames/ObjectCopier.h"
#include "FastFrames/Sample.h"
#include "FastFrames/UniqueSampleID.h"
#include "FastFrames/Utils.h"
#include "FastFrames/VariableMacros.h"
#include "FastFrames/SimpleONNXInference.h"

#include "TChain.h"
#include "TSystem.h"
#include "TTreeIndex.h"
#include "Math/Vector4D.h"
#include "ROOT/RDFHelpers.hxx"
#include "ROOT/RDF/RSampleInfo.hxx"

#include <algorithm>
#include <iostream>
#include <exception>
#include <regex>
#include <utility>

using ROOT::RDF::RSampleInfo;
using ROOT::VecOps::RVec;

void MainFrame::init() {
    TH1::AddDirectory(kFALSE);
    if (m_config->minEvent() >= 0 || m_config->maxEvent() >= 0 || m_config->numCPU()==1) {
        ROOT::DisableImplicitMT();
        if (m_config->numCPU() != 1) {
          LOG(WARNING) << "Disabling implicit multi-threading as it is not allowed for Range() call\n";
        }
    } else {
        ROOT::EnableImplicitMT(m_config->numCPU());
        LOG(INFO) << "Enabling implicit multi-threading with " << m_config->numCPU() << " threads\n";
    }
    m_metadataManager.readFileList(m_config->inputFilelistPath());
    m_metadataManager.readSumWeights(m_config->inputSumWeightsPath());

    // concatenate dsids from the file list and from the config file
    const std::vector<int> configDSIDs = m_config->uniqueDSIDs();

    m_metadataManager.readXSectionFiles(m_config->xSectionFiles(), configDSIDs);

    // propagate luminosity information from config
    for (const auto& ilumi : m_config->luminosityMap()) {
        m_metadataManager.addLuminosity(ilumi.first, ilumi.second);
    }

    // check systematics need to be cleared
    if (m_config->hasAutomaticSystematics()) {
        m_config->clearSystematics();
    }

    for (auto& isample : m_config->samples()) {
        if (isample->automaticSystematics() || isample->nominalOnly()) {
            this->readAutomaticSystematics(isample, isample->nominalOnly());
        }
    }

    this->prepareONNXwrapper();

}

void MainFrame::executeHistograms() {

    // run the check for metadata
    if (!m_metadataManager.checkSamplesMetadata(m_config->samples())) {
        LOG(ERROR) << "Metadata information missing or requested samples are not provided, please fix\n";
        throw std::invalid_argument("");
    }

    LOG(INFO) << "-------------------------------------\n";
    LOG(INFO) << "Started the main histogram processing\n";
    LOG(INFO) << "-------------------------------------\n";
    std::size_t sampleN(1);
    for (const auto& isample : m_config->samples()) {
        LOG(INFO) << "\n";
        LOG(INFO) << "Processing sample: " << isample->name() << ", sample " << sampleN << " out of " << m_config->samples().size() << " samples\n";

        if (isample->hasTruth() || m_config->splitProcessingPerUniqueSample()) {
            auto finalProduct = this->processHistogramsSplitPerUniqueSample(isample);
            auto&& finalSystHistos = std::get<0>(finalProduct);
            auto&& finalTruthHistos = std::get<1>(finalProduct);
            auto&& finalCutflowContainers = std::get<2>(finalProduct);

            this->writeHistosToFile(finalSystHistos, finalTruthHistos, finalCutflowContainers, isample, false);
        } else {
            LOG(DEBUG) << "Processing all unique samples in one go\n";
            auto finalProduct = this->processSampleWithAllUniqueSamples(isample);
            auto&& finalSystHistos = std::get<0>(finalProduct);
            auto&& finalCutflowContainers = std::get<1>(finalProduct);
            auto node = std::get<2>(finalProduct);

            this->writeHistosToFile(finalSystHistos, {}, finalCutflowContainers, isample, true);
        }

        ++sampleN;
    }
}

std::tuple<std::vector<SystematicHisto>,
           std::vector<VariableHisto>,
           std::vector<CutflowContainer> > MainFrame::processHistogramsSplitPerUniqueSample(const std::shared_ptr<Sample>& sample) {

    std::vector<SystematicHisto> finalSystHistos;
    std::vector<VariableHisto> finalTruthHistos;
    std::vector<CutflowContainer> finalCutflowContainers;

    std::size_t uniqueSampleN(1);
    for (const auto& iUniqueSampleID : sample->uniqueSampleIDs()) {
        LOG(INFO) << "\n";
        LOG(INFO) << "Processing unique sample: " << iUniqueSampleID << ", " << uniqueSampleN << " out of " << sample->uniqueSampleIDs().size() << " unique samples\n";

        auto currentHistos = this->processUniqueSample(sample, iUniqueSampleID);
        auto&& systematicHistos = std::get<0>(currentHistos);
        auto&& truthHistos      = std::get<1>(currentHistos);
        auto&& cutflows         = std::get<2>(currentHistos);
        auto node               = std::get<3>(currentHistos);
        auto recoChain          = std::move(std::get<4>(currentHistos));
        auto truthChains        = std::move(std::get<5>(currentHistos));
        // this happens when there are no files provided
        if (systematicHistos.empty()) {
            ++uniqueSampleN;
            continue;
        }

        // merge the histograms or take them if it is the first set
        if (finalSystHistos.empty())  {
            LOG(INFO) << "Triggering event loop for the reco tree\n";
            for (const auto& isystHist : systematicHistos) {
                finalSystHistos.emplace_back(isystHist.copy());
            }
        } else {
            if (systematicHistos.size() != finalSystHistos.size()) {
                LOG(ERROR) << "Number of the systematic histograms do not match\n";
                LOG(ERROR) << "Size of the current histograms: " << systematicHistos.size() << ", final histograms: " << finalSystHistos.size() << "\n";
                throw std::runtime_error("");
            }

            LOG(INFO) << "Merging samples, triggering the event loop for the reco tree!\n";
            for (std::size_t isyst = 0; isyst < finalSystHistos.size(); ++isyst) {
                finalSystHistos.at(isyst).merge(systematicHistos.at(isyst));
            }
        }
        if (!cutflows.empty()) {
            LOG(DEBUG) << "Processing cutflows\n";
            if (finalCutflowContainers.empty()) {
                for (auto& cutflow : cutflows) {
                    finalCutflowContainers.emplace_back(cutflow.name());
                    finalCutflowContainers.back().copyValues(cutflow);
                }
            } else {
                for (std::size_t i = 0; i < cutflows.size(); ++i) {
                    finalCutflowContainers.at(i).mergeValues(cutflows.at(i));
                }
            }
            LOG(DEBUG) << "Finished processing cutflows\n";
        }
        LOG(DEBUG) << "Number of event loops: " << node.GetNRuns() << ". For an optimal run, this number should be 1\n";
        if (!truthHistos.empty()) {
            if (finalTruthHistos.empty()) {
                LOG(INFO) << "Triggering the event loop for the truth tree\n";
                for (const auto& ivariable : truthHistos) {
                    finalTruthHistos.emplace_back(ivariable.name());
                    finalTruthHistos.back().copyHisto(ivariable.histo());
                }
            } else {
                LOG(INFO) << "Merging truth, triggering the event loop for the truth trees!\n";
                if (finalTruthHistos.size() != truthHistos.size()) {
                    LOG(ERROR) << "Sizes of truth histograms do not match!\n";
                    throw std::runtime_error("");
                }
                for (std::size_t ihist = 0; ihist < truthHistos.size(); ++ihist) {
                    finalTruthHistos.at(ihist).mergeHisto(truthHistos.at(ihist).histo());
                }
            }
        }
        ++uniqueSampleN;
        if (!truthChains.empty()) {
            LOG(DEBUG) << "Deleting truth chains and the TTree indices\n";
        }
        LOG(DEBUG) << "Deleting RDF objects (out of scope)\n";
    }

    return std::make_tuple(std::move(finalSystHistos), std::move(finalTruthHistos), std::move(finalCutflowContainers));
}


void MainFrame::executeNtuples() {

    // run the check for metadata
    if (!m_metadataManager.checkSamplesMetadata(m_config->samples())) {
        LOG(ERROR) << "Metadata information missing, please fix\n";
        throw std::invalid_argument("");
    }

    LOG(INFO) << "----------------------------------\n";
    LOG(INFO) << "Started the main ntuple processing\n";
    LOG(INFO) << "----------------------------------\n";
    std::size_t sampleN(1);
    for (const auto& isample : m_config->ntuple()->samples()) {
        LOG(INFO) << "\n";
        LOG(INFO) << "Processing sample: " << sampleN << " out of " << m_config->samples().size() << " samples\n";
        if (!m_config->ntuple()->copyTrees().empty()) {
            for (const auto& icopy : m_config->ntuple()->copyTrees()) {
                auto itr = std::find_if(isample->truths().begin(), isample->truths().end(), [&icopy](const auto& element){return icopy == element->truthTreeName();});
                if (itr != isample->truths().end()) {
                    LOG(WARNING) << "You are asking the code to copy a truth tree: " << icopy << " that is also being processed. This might overwrite the results!\n";
                }
            }
        }
        std::size_t uniqueSampleN(1);
        for (const auto& iUniqueSampleID : isample->uniqueSampleIDs()) {
            LOG(INFO) << "\n";
            LOG(INFO) << "Processing unique sample: " << iUniqueSampleID << ", " << uniqueSampleN << " out of " << isample->uniqueSampleIDs().size() << " unique samples\n";
            this->processUniqueSampleNtuple(isample, iUniqueSampleID);
            ++uniqueSampleN;
        }
        ++sampleN;
    }
}

std::tuple<std::vector<SystematicHisto>,
           std::vector<VariableHisto>,
           std::vector<CutflowContainer>,
           ROOT::RDF::RNode,
           std::unique_ptr<TChain>,
           std::vector<std::pair<std::unique_ptr<TChain> , std::unique_ptr<TTreeIndex> > > > MainFrame::processUniqueSample(const std::shared_ptr<Sample>& sample,
                                                                                                                            const UniqueSampleID& uniqueSampleID) {

    const std::vector<std::string>& filePaths = m_metadataManager.filePaths(uniqueSampleID);
    std::vector<std::string> selectedFilePaths(filePaths);
    std::vector<std::pair<std::unique_ptr<TChain>, std::unique_ptr<TTreeIndex> > > truthChains;
    if (m_config->totalJobSplits() > 0) {
        if (sample->hasUnfolding()) {
            LOG(WARNING) << "#############################################################################################\n";
            LOG(WARNING) << "Sample " << sample->name() << ", has unfolding histograms requested and split processing is used\n";
            LOG(WARNING) << "You will not be able to \"hadd\" the output to get the efficiency and acceptance histograms\n";
            LOG(WARNING) << "#############################################################################################\n";
        }
        selectedFilePaths = Utils::selectedFileList(filePaths, m_config->totalJobSplits(), m_config->currentJobIndex());
    }
    if (selectedFilePaths.empty()) {
        LOG(WARNING) << "UniqueSample: " << uniqueSampleID << " has no files, will not produce histograms\n";
        ROOT::RDataFrame tmp(1);
        return std::make_tuple(std::vector<SystematicHisto>{}, std::vector<VariableHisto>{}, std::vector<CutflowContainer>{}, tmp, nullptr, std::move(truthChains));
    }

    // we could use any file from the list, use the first one
    m_systReplacer.readSystematicMapFromFile(selectedFilePaths.at(0), sample->recoTreeName(), sample->systematics());

    std::vector<VariableHisto> truthHistos;

    if (sample->hasTruth()) {
        LOG(DEBUG) << "Processing standalone truth histograms\n";
        truthHistos = this->processTruthHistos(selectedFilePaths, sample, uniqueSampleID);
        LOG(DEBUG) << "Done processing standalone truth histograms\n";
    }

    std::unique_ptr<TChain> recoChain = Utils::chainFromFiles(sample->recoTreeName(), selectedFilePaths);
    const bool hasZeroEvents = recoChain->GetEntries() == 0;

    if (sample->hasTruth()) {
        truthChains = this->connectTruthTrees(recoChain, sample, selectedFilePaths);
    }

    ROOT::RDataFrame df(*recoChain);
    ROOT::RDF::RNode mainNode = df;

    if (hasZeroEvents) {
        LOG(WARNING) << "UniqueSampleID: " << uniqueSampleID << ", has 0 reco trees, skipping it\n";
        return std::make_tuple(std::vector<SystematicHisto>{}, std::move(truthHistos), std::vector<CutflowContainer>{}, std::move(mainNode), nullptr, std::move(truthChains));
    }

    #if ROOT_VERSION_CODE > ROOT_VERSION(6,29,0)
    ROOT::RDF::Experimental::AddProgressBar(mainNode);
    #endif

    mainNode = this->minMaxRange(mainNode);

    // add TLorentzVectors for objects
    mainNode = this->addTLorentzVectors(mainNode);

    if (!m_config->configDefineAfterCustomClass()) {
        mainNode = this->addCustomDefinesFromConfig(mainNode, sample);
    }

    // this is the method users will be able to override
    LOG(DEBUG) << "Adding custom reco variables from the code\n";
    mainNode = this->defineVariables(mainNode, sample, uniqueSampleID);
    LOG(DEBUG) << "Finished adding custom reco variables\n";

    if (m_config->configDefineAfterCustomClass()) {
        mainNode = this->addCustomDefinesFromConfig(mainNode, sample);
    }

    mainNode = this->prepareWeightMetadata(mainNode, sample, uniqueSampleID);
    mainNode = this->addWeightColumns(mainNode, sample, uniqueSampleID);

    // add truth variables if matching reco and truth
    mainNode = this->addTruthVariablesToReco(mainNode, sample, uniqueSampleID);

    // run models from simple_onnx_inference block
    mainNode = this->scheduleSimpleONNXInference(mainNode);

    // add columns for variables defined with a formula
    if (sample->hasTruth()) {
        mainNode = this->addVariablesWithFormulaReco(mainNode, sample, sample->uniqueTruthTreeNames());
    } else {
        mainNode = this->addVariablesWithFormulaReco(mainNode, sample, {});
    }

    LOG(DEBUG) << "Finished adding all columns to the reco tree\n";

    m_systReplacer.printMaps();

    // book cutflows
    std::vector<CutflowContainer> cutflows = this->bookCutflows(mainNode, sample);

    std::vector<std::vector<ROOT::RDF::RNode> > filterStore = this->applyFilters(mainNode, sample, uniqueSampleID);
    LOG(DEBUG) << "Finished booking filters\n";

    // retrieve the histograms;
    std::vector<SystematicHisto> histoContainer = this->processHistograms(filterStore, sample);
    LOG(DEBUG) << "Finished booking histograms\n";

    return std::make_tuple(std::move(histoContainer), std::move(truthHistos), std::move(cutflows), std::move(mainNode), std::move(recoChain), std::move(truthChains));
}

std::tuple<std::vector<SystematicHisto>,
           std::vector<CutflowContainer>,
           ROOT::RDF::RNode> MainFrame::processSampleWithAllUniqueSamples(const std::shared_ptr<Sample>& sample) {

    // we could use any file from the list, use the first one
    m_systReplacer.readSystematicMapFromFile(sample->oneFilePath(m_metadataManager), sample->recoTreeName(), sample->systematics());

    auto spec = m_metadataManager.dataSpec(sample, m_config);

    ROOT::RDataFrame df(spec);

    ROOT::RDF::RNode mainNode = df;

    ROOT::RDF::Experimental::AddProgressBar(mainNode);

    mainNode = this->minMaxRange(mainNode);

    // add TLorentzVectors for objects
    mainNode = this->addTLorentzVectors(mainNode);

    if (!m_config->configDefineAfterCustomClass()) {
        mainNode = this->addCustomDefinesFromConfig(mainNode, sample);
    }

    UniqueSampleID dummy(-1, "dummy", "dummy");

    // this is the method users will be able to override
    LOG(DEBUG) << "Adding custom reco variables from the code\n";
    mainNode = this->defineVariables(mainNode, sample, dummy);
    LOG(DEBUG) << "Finished adding custom reco variables\n";

    if (m_config->configDefineAfterCustomClass()) {
        mainNode = this->addCustomDefinesFromConfig(mainNode, sample);
    }

    mainNode = this->prepareWeightMetadataAllUniqueSamples(mainNode, sample);
    mainNode = this->addWeightColumns(mainNode, sample, dummy);

    // run models from simple_onnx_inference block
    mainNode = this->scheduleSimpleONNXInference(mainNode);

    // add columns for variables defined with a formula
    mainNode = this->addVariablesWithFormulaReco(mainNode, sample, {});

    LOG(DEBUG) << "Finished adding all columns to the reco tree\n";

    m_systReplacer.printMaps();

    // book cutflows
    std::vector<CutflowContainer> cutflows = this->bookCutflows(mainNode, sample);

    std::vector<std::vector<ROOT::RDF::RNode> > filterStore = this->applyFilters(mainNode, sample, dummy);
    LOG(DEBUG) << "Finished booking filters\n";

    // retrieve the histograms;
    std::vector<SystematicHisto> histoContainer = this->processHistograms(filterStore, sample);
    LOG(DEBUG) << "Finished booking histograms\n";

    return std::make_tuple(std::move(histoContainer), std::move(cutflows), std::move(mainNode));
}

void MainFrame::processUniqueSampleNtuple(const std::shared_ptr<Sample>& sample,
                                          const UniqueSampleID& id) {

    const std::vector<std::string>& filePaths = m_metadataManager.filePaths(id);
    std::vector<std::string> selectedFilePaths(filePaths);
    if (m_config->totalJobSplits() > 0) {
        selectedFilePaths = Utils::selectedFileList(filePaths, m_config->totalJobSplits(), m_config->currentJobIndex());
    }
    if (selectedFilePaths.empty()) {
        LOG(WARNING) << "UniqueSample: " << id << " has no files, will not produce output ntuple\n";
        return;
    }

    // we could use any file from the list, use the first one
    m_systReplacer.readSystematicMapFromFile(selectedFilePaths.at(0), sample->recoTreeName(), sample->systematics());

    // we still need to get the list of all systematics to be able to identify the nominal branches
    if (sample->nominalOnly()) {
        const auto systematics = this->automaticSystematicNames(selectedFilePaths);
        m_systReplacer.setSystematicNames(systematics);
    }

    auto chain = Utils::chainFromFiles(sample->recoTreeName(), selectedFilePaths);

    std::vector<std::pair<std::unique_ptr<TChain>, std::unique_ptr<TTreeIndex> > > truthChains;
    if (sample->hasTruth()) {
        truthChains = this->connectTruthTrees(chain, sample, selectedFilePaths);
    }

    const bool hasZeroEvents = chain->GetEntries() == 0;
    if (hasZeroEvents) LOG(WARNING) << "UniqueSampleID: " << id << ", has no events, skipping it\n";

    ROOT::RDataFrame df(*chain);

    ROOT::RDF::RNode mainNode = df;
    #if ROOT_VERSION_CODE > ROOT_VERSION(6,29,0)
    ROOT::RDF::Experimental::AddProgressBar(mainNode);
    #endif
    if (!hasZeroEvents) {
        mainNode = this->minMaxRange(mainNode);

        // add TLorentzVectors for objects
        mainNode = this->addTLorentzVectors(mainNode);

        if (!m_config->configDefineAfterCustomClass()) {
            mainNode = this->addCustomDefinesFromConfig(mainNode, sample);
        }

        // this is the method users will be able to override
        LOG(DEBUG) << "Adding custom reco variables from the code\n";
        mainNode = this->defineVariablesNtuple(mainNode, sample, id);
        LOG(DEBUG) << "Finished adding custom reco variables\n";

        if (m_config->configDefineAfterCustomClass()) {
            mainNode = this->addCustomDefinesFromConfig(mainNode, sample);
        }

        // run models from simple_onnx_inference block
        mainNode = this->scheduleSimpleONNXInference(mainNode);

        mainNode = this->prepareWeightMetadata(mainNode, sample, id);
        mainNode = this->addWeightColumns(mainNode, sample, id);

        m_systReplacer.printMaps();

        // apply filter
        if (!m_config->ntuple()->selection().empty()) {
            mainNode = mainNode.Filter(this->systematicOrFilter(sample));
        }
    }

    //store the file
    const std::string folder = m_config->outputPathNtuples().empty() ? "" : m_config->outputPathNtuples() + "/";
    std::string suffix("");
    if (m_config->totalJobSplits() > 0) {
        suffix = "_Njobs_" + std::to_string(m_config->totalJobSplits()) + "_jobIndex_" + std::to_string(m_config->currentJobIndex());
    }
    const std::string fileName = folder + sample->name() + "_" + std::to_string(id.dsid())+"_" + id.campaign() + "_"+id.simulation() + suffix + ".root";
    const bool nominalOnly = sample->nominalOnly();
    const std::vector<std::string> allBranches = mainNode.GetColumnNames();
    const std::vector<std::string> selectedBranches = m_config->ntuple()->listOfSelectedBranches(nominalOnly ? m_systReplacer.nominalBranches(allBranches) : allBranches);
    LOG(VERBOSE) << "List of selected branches:\n";
    for (const auto& iselected : selectedBranches) {
        LOG(VERBOSE) << "\t" << iselected << "\n";
    }
    LOG(INFO) << "Writing the ntuple to: " << fileName << "\n";
    LOG(INFO) << "Triggering the event loop for the reco tree!\n";
    ROOT::RDF::RSnapshotOptions opts;
    opts.fAutoFlush = m_config->ntupleAutoFlush();
    opts.fCompressionLevel = m_config->ntupleCompressionLevel();
    opts.fVector2RVec = m_config->convertVectorToRVec();
    mainNode.Snapshot(sample->recoTreeName(), fileName, selectedBranches, opts);
    LOG(DEBUG) << "Number of event loops: " << mainNode.GetNRuns() << ". For an optimal run, this number should be 1\n";

    auto nEntriesAfterCuts = mainNode.Count().GetValue();
    if (nEntriesAfterCuts==0) {
        LOG(WARNING) << "UniqueSampleID: " << id << ", has no events after cuts, generating an empty reco TTree\n";
    }
    // IF run in multi-threaded mode, we need to manually add an empty tree:
    // See ROOT::DataFrame::Snapshot for more details
    if (m_config->numCPU()!=1 && nEntriesAfterCuts==0){
        const char* emptyTreeName = sample->recoTreeName().c_str();
        TTree emptyTree(emptyTreeName, emptyTreeName);
        // Write to the file.
        TFile file(fileName.c_str(), "UPDATE");
        emptyTree.Write();
        file.Close();
    }

    for (const auto& itruth : sample->truths()) {
        this->processSingleTruthTreeNtuple(itruth, selectedFilePaths, fileName, sample, id);
    }

    ObjectCopier copier(selectedFilePaths);
    copier.readObjectInfo();
    if (!m_config->ntuple()->copyTrees().empty()) {
        copier.copyTreesTo(fileName, m_config->ntuple()->copyTrees(), m_config->convertVectorToRVec());
    }
    LOG(INFO) << "Copying metadata from the original files\n";
    copier.copyObjectsTo(fileName);
    LOG(INFO) << "Finished copying metadata from the original files\n";

    if (!truthChains.size()) {
        LOG(DEBUG) << "Deleting truth chains\n";
    }
}

std::string MainFrame::systematicFilter(const std::shared_ptr<Sample>& sample,
                                        const std::shared_ptr<Systematic>& systematic,
                                        const std::shared_ptr<Region>& region) const {

    std::string nominalSelection = region->selection();
    if (!sample->selectionSuffix().empty()) {
        nominalSelection = "(" + nominalSelection + ") && (" + sample->selectionSuffix() + ")";
    }
    const std::string systSelection = m_systReplacer.replaceString(nominalSelection, systematic);
    LOG(VERBOSE) << "Sample: " << sample->name() << ", region: " << region->name() << ", systematic: "
                 << systematic->name() << ", original selection: " << nominalSelection << ", systematic selection: " << systSelection << "\n";

    return systSelection;
}

std::string MainFrame::systematicOrFilter(const std::shared_ptr<Sample>& sample) const {
    const std::string& nominalSelection = m_config->ntuple()->selection();

    std::string result = "(" + nominalSelection + ")";
    if (!sample->nominalOnly()) {
        for (const auto& isyst : sample->systematics()) {
            const std::string systSelection = m_systReplacer.replaceString(nominalSelection, isyst);
            if (systSelection == nominalSelection) continue;

            result += "||(" + systSelection + ")";
        }
    }

    LOG(DEBUG) << "Final selection used for filtering ntuples: " << result << "\n";

    return result;
}

std::string MainFrame::systematicVariable(const Variable& variable,
                                          const std::shared_ptr<Systematic>& systematic) const {

    std::string nominalVariable = variable.definition();
    auto itr = m_variablesWithFormulaReco.find(nominalVariable);
    if (itr != m_variablesWithFormulaReco.end()) {
        LOG(VERBOSE) << "Booking variable: " << variable.name() << " with column: " << itr->second << "\n";
        nominalVariable = itr->second;
    }

    const std::string systVariable = m_systReplacer.replaceString(nominalVariable, systematic);

    return systVariable;
}

std::string MainFrame::systematicWeight(const std::shared_ptr<Systematic>& systematic) const {

    const std::string branchName = "weight_total_" + systematic->name();

    if (!m_systReplacer.branchExists(branchName)) {
        return "weight_total_NOSYS";
    }

    return branchName;
}

std::vector<std::vector<ROOT::RDF::RNode> > MainFrame::applyFilters(ROOT::RDF::RNode mainNode,
                                                                    const std::shared_ptr<Sample>& sample,
                                                                    const UniqueSampleID& id) {

    std::vector<std::vector<ROOT::RDF::RNode> > result;

    for (const auto& isyst : sample->systematics()) {
        std::vector<ROOT::RDF::RNode> perSystFilter;
        for (const auto& ireg : sample->regions()) {

            if (sample->skipSystematicRegionCombination(isyst, ireg)) {
                LOG(DEBUG) << "Skipping region: " << ireg->name() << ", systematic: " << isyst->name() << " combination for sample: " << sample->name() << " (filter)\n";
                continue;
            }

            ROOT::RDF::RNode filter = mainNode.Filter(this->systematicFilter(sample, isyst, ireg));
            filter = this->defineVariablesRegion(filter, sample, id, ireg->name());
            perSystFilter.emplace_back(std::move(filter));
        }
        result.emplace_back(std::move(perSystFilter));
    }

    return result;
}

ROOT::RDF::RNode MainFrame::addWeightColumns(ROOT::RDF::RNode node,
                                             const std::shared_ptr<Sample>& sample,
                                             const UniqueSampleID& id) {

    for (const auto& isyst : sample->systematics()) {
        node = this->addSingleWeightColumn(node, sample, isyst, id);
    }

    return node;
}

ROOT::RDF::RNode MainFrame::addSingleWeightColumn(ROOT::RDF::RNode mainNode,
                                                  const std::shared_ptr<Sample>& sample,
                                                  const std::shared_ptr<Systematic>& systematic,
                                                  const UniqueSampleID& id) {

    const std::string& nominalWeight = sample->weight();

    const std::string systName = "weight_total_" + systematic->name();
    const std::string nominalNorm = "lumi*xSection/sumWeights_NOSYS";
    const std::string nominalTotalWeight = "(" + nominalWeight + ")*("+nominalNorm+")";
    const std::string sumWeightsSyst = "sumWeights_"+systematic->sumWeights();
    const std::string normalisation = sample->isData() ? "1." : "lumi*xSection/" + sumWeightsSyst;
    std::string formula = m_systReplacer.replaceString("("+nominalWeight, systematic) + ")*("+normalisation+")";
    if (!systematic->weightSuffix().empty()) {
        formula = "(" + formula + ")*(" + systematic->weightSuffix() + ")";
    }
    if (!systematic->isNominal() && formula == nominalTotalWeight) {
        LOG(DEBUG) << "Sample: " << id << ", systematic: " << systematic->name() << ", does not impact the weight\n";
        return mainNode;
    }
    LOG(VERBOSE) << "Unique sample: " << id << ", systematic: " << systematic->name() << ", weight formula: " << formula << ", new weight name: " << systName << "\n";

    if (m_systReplacer.branchExists(systName)) {
        LOG(DEBUG) << "Branch: " << systName << " already exists, not adding it\n";
        return mainNode;
    }

    // add it to the list of branches
    m_systReplacer.addBranch(systName);

    m_systReplacer.addSingleSystematic("weight_total_NOSYS", systematic->name());

    auto node = mainNode.Define(systName, formula);
    return node;
}

ROOT::RDF::RNode MainFrame::addTLorentzVectors(ROOT::RDF::RNode mainNode) {
    const std::vector<std::string>& objects = m_config->tLorentzVectors();
    for (const auto& iobject : objects) {
        mainNode = this->addSingleTLorentzVector(mainNode, iobject);
    }

    return mainNode;
}

ROOT::RDF::RNode MainFrame::addSingleTLorentzVector(ROOT::RDF::RNode mainNode,
                                                    const std::string& object) {

    static const std::vector<std::string> kinematics = {"_pt_NOSYS", "_eta", "_phi", "_e_NOSYS"};

    auto createTLV_RVec = [](const RVec<float>& pt,
                             const RVec<float>& eta,
                             const RVec<float>& phi,
                             const RVec<float>& e) {

        RVec<ROOT::Math::PtEtaPhiEVector> result;
        for (std::size_t i = 0; i < pt.size(); ++i) {
            ROOT::Math::PtEtaPhiEVector vector(pt.at(i), eta.at(i), phi.at(i), e.at(i));
            result.emplace_back(vector);
        }

        return result;
    };

    auto createTLV = [](const std::vector<float>& pt,
                        const std::vector<float>& eta,
                        const std::vector<float>& phi,
                        const std::vector<float>& e) {

        std::vector<ROOT::Math::PtEtaPhiEVector> result;
        for (std::size_t i = 0; i < pt.size(); ++i) {
            ROOT::Math::PtEtaPhiEVector vector(pt.at(i), eta.at(i), phi.at(i), e.at(i));
            result.emplace_back(vector);
        }

        return result;
    };

    std::vector<std::string> objectColumns;
    for (const auto& ikinematics : kinematics) {
        const std::string variable = object + ikinematics;
        objectColumns.emplace_back(std::move(variable));
    }

    const std::string vectorName = object + "_TLV_NOSYS";
    if (m_systReplacer.branchExists(vectorName)) {
        LOG(DEBUG) << "Branch: " << vectorName << " already exists, not adding it (nor its uncertainty variations)\n";
        return mainNode;
    }
    if (m_config->useRVec())
        mainNode = this->systematicDefine(mainNode, vectorName, createTLV_RVec, objectColumns);
    else
        mainNode = this->systematicDefine(mainNode, vectorName, createTLV, objectColumns);

    return mainNode;
}

std::vector<SystematicHisto> MainFrame::processHistograms(std::vector<std::vector<ROOT::RDF::RNode> >& filters,
                                                          const std::shared_ptr<Sample>& sample) {

    std::vector<SystematicHisto> result;

    std::size_t systIndex(0);
    for (const auto& isyst : sample->systematics()) {
        SystematicHisto systematicHisto(isyst->name());

        std::size_t regIndex(0);
        for (const auto& ireg : sample->regions()) {
            if (sample->skipSystematicRegionCombination(isyst, ireg)) {
                LOG(DEBUG) << "Skipping region: " << ireg->name() << ", systematic: " << isyst->name() << " combination for sample: " << sample->name() << " (histogram)\n";
                ++regIndex;
                continue;
            }
            RegionHisto regionHisto(ireg->name());

            ROOT::RDF::RNode node = filters.at(systIndex).at(regIndex);

            this->processHistograms1D(&regionHisto, node, sample, ireg, isyst);

            this->processHistograms2D(&regionHisto, node, sample, ireg, isyst);

            this->processRecoVsTruthHistograms2D(&regionHisto, node, sample, ireg, isyst);

            this->processHistograms3D(&regionHisto, node, sample, ireg, isyst);

            systematicHisto.addRegionHisto(std::move(regionHisto));
            ++regIndex;
        }
        result.emplace_back(std::move(systematicHisto));
        ++systIndex;
    }

    return result;
}

void MainFrame::writeHistosToFile(const std::vector<SystematicHisto>& histos,
                                  const std::vector<VariableHisto>& truthHistos,
                                  std::vector<CutflowContainer>& cutflowHistos,
                                  const std::shared_ptr<Sample>& sample,
                                  const bool allUniqueSamples) const {

    if (histos.empty()) {
        LOG(WARNING) << "No histograms available for sample: " << sample->name() << "\n";
    }

    std::string suffix("");
    if (m_config->totalJobSplits() > 0) {
        suffix = "_Njobs_" + std::to_string(m_config->totalJobSplits()) + "_jobIndex_" + std::to_string(m_config->currentJobIndex());
    }

    std::string fileName = m_config->outputPathHistograms();
    fileName += fileName.empty() ? "" : "/";
    fileName += sample->name() + suffix + ".root";


    std::unique_ptr<TFile> out(TFile::Open(fileName.c_str(), "RECREATE"));
    if (!out) {
        LOG(ERROR) << "Cannot open ROOT file at: " << fileName << "\n";
        throw std::invalid_argument("");
    }

    LOG(INFO) << "Writing histograms to file: " << fileName << "\n";
    if (allUniqueSamples) {
        LOG(INFO) << "Triggering event loop!\n";
    }

    for (const auto& isystHist : histos) {
        if (isystHist.regionHistos().empty()) {
            LOG(WARNING) << "No histograms available for sample: " << sample->name() << ", systematic: " << isystHist.name() << "\n";
            continue;
        }
        if (!out->GetDirectory(isystHist.name().c_str())) {
            out->cd();
            out->mkdir(isystHist.name().c_str());
        }
        for (const auto& iregionHist : isystHist.regionHistos()) {

            const std::string subRegionName = isystHist.name() + "/" + iregionHist.name();

            if (m_config->useRegionSubfolders()) {
                out->cd();
                out->mkdir(subRegionName.c_str());
            }

            // 1D histograms
            for (const auto& ivariableHist : iregionHist.variableHistos()) {
                const std::string histoName = StringOperations::replaceString(ivariableHist.name(), "_NOSYS", "") + "_" + iregionHist.name();
                if (m_config->useRegionSubfolders()) {
                    out->cd(subRegionName.c_str());
                } else {
                    out->cd(isystHist.name().c_str());
                }
                if (allUniqueSamples) {
                    ivariableHist.histo()->Write(histoName.c_str());
                } else {
                    ivariableHist.histoUniquePtr()->Write(histoName.c_str());
                }
            }

            // 2D histograms
            for (const auto& ivariableHist2D : iregionHist.variableHistos2D()) {
                const std::string histo2DName = StringOperations::replaceString(ivariableHist2D.name(), "_NOSYS", "") + "_" + iregionHist.name();
                if (m_config->useRegionSubfolders()) {
                    out->cd(subRegionName.c_str());
                } else {
                    out->cd(isystHist.name().c_str());
                }
                if (allUniqueSamples) {
                    ivariableHist2D.histo()->Write(histo2DName.c_str());
                } else {
                    ivariableHist2D.histoUniquePtr()->Write(histo2DName.c_str());
                }
            }

            // 3D histograms
            for (const auto& ivariableHist3D : iregionHist.variableHistos3D()) {
                const std::string histo3DName = StringOperations::replaceString(ivariableHist3D.name(), "_NOSYS", "") + "_" + iregionHist.name();
                if (m_config->useRegionSubfolders()) {
                    out->cd(subRegionName.c_str());
                } else {
                    out->cd(isystHist.name().c_str());
                }
                if (allUniqueSamples) {
                    ivariableHist3D.histo()->Write(histo3DName.c_str());
                } else {
                    ivariableHist3D.histoUniquePtr()->Write(histo3DName.c_str());
                }
            }
        }
    }

    if (sample->hasCutflows()) {
        LOG(DEBUG) << "Writing cutflow histograms\n";
        for (auto& icutflow : cutflowHistos) {
            out->cd();
            std::unique_ptr<TH1D> hist(nullptr);
            if (allUniqueSamples) {
                hist = icutflow.cutflowHistoExecute();
            } else {
                hist = icutflow.cutflowHisto();
            }
            const std::string histoName = "Cutflow_" + icutflow.name();
            hist->Write(histoName.c_str());
        }
    }

    if (!truthHistos.empty()) {
        LOG(DEBUG) << "Writing truth histograms\n";
    }
    // Write truth histograms
    for (const auto& itruthHist : truthHistos) {
        const std::string truthHistoName = StringOperations::replaceString(itruthHist.name(), "_NOSYS", "");
        out->cd();
        itruthHist.histoUniquePtr()->Write(truthHistoName.c_str());
    }

    if (m_config->totalJobSplits() > 0 && sample->hasUnfolding()) {
        LOG(WARNING) << "Sample: " << sample->name() << " has unfolding, but split processing is requested. Will not produce acceptance and selection efficiency histograms\n";
    } else {
        this->writeUnfoldingHistos(out.get(), histos, truthHistos, sample);
    }

    out->Close();
}

void MainFrame::readAutomaticSystematics(std::shared_ptr<Sample>& sample, const bool isNominalOnly) const {

    if (isNominalOnly) {
        // clear current systematics
        sample->clearSystematics();

        // add nominal "systematic"
        auto nominal = std::make_shared<Systematic>("NOSYS");
        nominal->setSumWeights(sample->nominalSumWeights());
        for (const auto& ireg : m_config->regions()) {
            nominal->addRegion(ireg);
        }
        sample->addSystematic(nominal);

        m_config->addUniqueSystematic(nominal);

        return;
    }

    // add systematics now
    for (const auto& iuniqueSample : sample->uniqueSampleIDs()) {
        if (iuniqueSample.isData()) return; // nothing to add for data
        const auto fileList = m_metadataManager.filePaths(iuniqueSample);
        if (fileList.empty()) continue;

        const std::vector<std::string> listOfSystematics = this->automaticSystematicNames(fileList);
        // now add the systematics
        for (const auto& isyst : listOfSystematics) {
            if (sample->skipExcludedSystematic(isyst)) {
                LOG(VERBOSE) << "Sample: " << sample->name() << " skipping automatic systematic: " << isyst << " as it is excluded\n";
                continue;
            }

            const bool isSystematicAlreadyPresent = sample->hasSystematic(isyst);
            if (isSystematicAlreadyPresent) {
                LOG(DEBUG) << "Sample: " << sample->name() << ", systematic: " << isyst << " provided in the config file, will not automatically add it\n";
                continue;
            }

            LOG(VERBOSE) << "Sample: " << sample->name() << ", adding automatic systematic: " << isyst << "\n";

            auto syst = std::make_shared<Systematic>(isyst);
            if (m_metadataManager.sumWeightsExist(iuniqueSample, syst)) {
                syst->setSumWeights(isyst);
            } else {
                syst->setSumWeights(sample->nominalSumWeights());
            }
            for (const auto& ireg : m_config->regions()) {
                syst->addRegion(ireg);
            }
            sample->addSystematic(syst);
            m_config->addUniqueSystematic(syst);
        }

        break;
    }
}

std::vector<std::string> MainFrame::automaticSystematicNames(const std::vector<std::string>& paths) const {
    std::vector<std::string> result;
    std::size_t nPaths(0);
    for (const auto& ipath : paths) {
        std::unique_ptr<TFile> in(TFile::Open(ipath.c_str(), "READ"));
        if (!in) {
            LOG(ERROR) << "Cannot open ROOT file at: " << ipath << "\n";
            throw std::invalid_argument("");
        }

        std::unique_ptr<TH1> hist(in->Get<TH1>(m_config->listOfSystematicsName().c_str()));
        if (!hist) {
            ++nPaths;
            if (nPaths == paths.size()) {
                LOG(WARNING) << "None of the files have \"" << m_config->listOfSystematicsName() << "\" histogram. This can happen for cases of zero events passing, but please check the inputs\n";
            } else {
                LOG(WARNING) << "Cannot read histogram \"" << m_config->listOfSystematicsName() << "\". Will try the next file\n";
            }
            continue;
        }
        hist->SetDirectory(nullptr);

        for (int ibin = 1; ibin <= hist->GetNbinsX(); ++ibin) {
            const std::string name = hist->GetXaxis()->GetBinLabel(ibin);
            if (name == "NOSYS") continue;

            LOG(VERBOSE) << "Adding systematic from histogram: " << name << "\n";
            result.emplace_back(name);
        }
        in->Close();
        return result;
    }

    return result;
}

void MainFrame::processHistograms1D(RegionHisto* regionHisto,
                                    const ROOT::RDF::RNode& node,
                                    const std::shared_ptr<Sample>& sample,
                                    const std::shared_ptr<Region>& region,
                                    const std::shared_ptr<Systematic>& systematic) const {

    for (const auto& ivariable : region->variables()) {
        const std::vector<std::string>& variables = sample->variables();

        if (ivariable.isNominalOnly() && systematic->name() != "NOSYS") continue;

        auto itrVar = std::find(variables.begin(), variables.end(), ivariable.name());
        if (itrVar == variables.end()) {
            LOG(VERBOSE) << "Skipping variable: " << ivariable.name() << " for sample: " << sample->name() << ", systematic: " << systematic->name() << "\n";
            continue;
        }
        VariableHisto variableHisto(ivariable.name());

        ROOT::RDF::RResultPtr<TH1D> histogram = this->book1Dhisto(node, ivariable, systematic);

        if (!histogram) {
            LOG(ERROR) << "Histogram for sample: " << sample->name() << ", systematic: "
                       << systematic->name() << ", region: " << region->name() << " and variable: " << ivariable.name() << " is empty!\n";
            throw std::runtime_error("");

        }
        variableHisto.setHisto(histogram);

        regionHisto->addVariableHisto(std::move(variableHisto));
    }
}

void MainFrame::processHistograms2D(RegionHisto* regionHisto,
                                    const ROOT::RDF::RNode& node,
                                    const std::shared_ptr<Sample>& sample,
                                    const std::shared_ptr<Region>& region,
                                    const std::shared_ptr<Systematic>& systematic) const {

    for (const auto& combinations : region->variableCombinations()) {
        const Variable& v1 = region->variableByName(combinations.first);
        const Variable& v2 = region->variableByName(combinations.second);
        const std::string name = v1.name() + "_vs_" + v2.name();
        if ((v1.isNominalOnly() || v2.isNominalOnly()) && systematic->name() != "NOSYS") continue;

        const std::vector<std::string>& variables = sample->variables();
        auto itrVar1 = std::find(variables.begin(), variables.end(), v1.name());
        auto itrVar2 = std::find(variables.begin(), variables.end(), v2.name());
        if (itrVar1 == variables.end() || itrVar2 == variables.end()) {
            LOG(VERBOSE) << "Skipping variable (2D): " << name << " for sample: " << sample->name() << ", systematic" << systematic->name() << "\n";
            continue;
        }

        VariableHisto2D variableHisto2D(name);
        ROOT::RDF::RResultPtr<TH2D> histogram2D = this->book2Dhisto(node, v1, v2, systematic);

        if (!histogram2D) {
            LOG(ERROR) << "Histogram for sample: " << sample->name() << ", systematic: "
                       << systematic->name() << ", region: " << region->name() << " and variable combination: " << v1.name() << " & " << v2.name() << " is empty!\n";
            throw std::runtime_error("");

        }

        variableHisto2D.setHisto(histogram2D);

        regionHisto->addVariableHisto2D(std::move(variableHisto2D));
    }
}

void MainFrame::processRecoVsTruthHistograms2D(RegionHisto* regionHisto,
                                               ROOT::RDF::RNode node,
                                               const std::shared_ptr<Sample>& sample,
                                               const std::shared_ptr<Region>& region,
                                               const std::shared_ptr<Systematic>& systematic) {

    for (const auto& itruth : sample->truths()) {
        if (!itruth->matchRecoTruth()) continue;

        ROOT::RDF::RNode passedNode = node;
        if (!itruth->selection().empty()) {

            // the variables in the selection need to be protected
            // to make sure they can be read when the corresponding truth event does not exist
            const std::vector<std::string> columns = Utils::getColumnsFromString(itruth->selection(), passedNode);
            for (const auto& icolumn : columns) {
                passedNode = passedNode.FilterAvailable(icolumn);
            }

            passedNode = passedNode.Filter(itruth->selection());
        }
        for (const auto& imatch : itruth->matchedVariables()) {
            const Variable& recoVariable  = region->variableByName(imatch.first);
            Variable truthVariable = itruth->variableByName(imatch.second);
            if (recoVariable.isNominalOnly() && systematic->name() != "NOSYS") continue;

            auto itrTree = m_variablesWithFormulaTruth.find(itruth->truthTreeName());
            if (itrTree != m_variablesWithFormulaTruth.end()) {
                auto itr = itrTree->second.find(truthVariable.definition());
                if (itr != itrTree->second.end()) {
                    truthVariable.setDefinition(itr->second);
                    LOG(VERBOSE) << "Truth tree: " << itruth->truthTreeName() << ", setting truth variable: " << truthVariable.name() << " definition to: " << truthVariable.definition() << "\n";
                }
            }

            const std::string name = recoVariable.name() + "_vs_" + itruth->name() + "_" + truthVariable.name();
            VariableHisto2D variableHistoPassed(name);

            passedNode = passedNode.FilterAvailable(truthVariable.definition());

            ROOT::RDF::RResultPtr<TH2D> histogramPassed = this->book2Dhisto(passedNode, truthVariable, recoVariable, systematic);

            if (!histogramPassed) {
                LOG(ERROR) << "Histogram for sample: " << sample->name() << ", systematic: "
                           << systematic->name() << ", region: " << region->name() << " and variable: " << truthVariable.name() << " is empty!\n";
                throw std::runtime_error("");

            }
            variableHistoPassed.setHisto(histogramPassed);

            regionHisto->addVariableHisto2D(std::move(variableHistoPassed));
        }
    }
}

void MainFrame::processHistograms3D(RegionHisto* regionHisto,
                                    const ROOT::RDF::RNode& node,
                                    const std::shared_ptr<Sample>& sample,
                                    const std::shared_ptr<Region>& region,
                                    const std::shared_ptr<Systematic>& systematic) const {

    for (const auto& combinations : region->variableCombinations3D()) {
        const Variable& v1 = region->variableByName(std::get<0>(combinations));
        const Variable& v2 = region->variableByName(std::get<1>(combinations));
        const Variable& v3 = region->variableByName(std::get<2>(combinations));
        const std::string name = v1.name() + "_vs_" + v2.name() + "_vs_" + v3.name();
        if ((v1.isNominalOnly() || v2.isNominalOnly() || v3.isNominalOnly()) && systematic->name() != "NOSYS") continue;

        const std::vector<std::string>& variables = sample->variables();
        auto itrVar1 = std::find(variables.begin(), variables.end(), v1.name());
        auto itrVar2 = std::find(variables.begin(), variables.end(), v2.name());
        auto itrVar3 = std::find(variables.begin(), variables.end(), v3.name());
        if (itrVar1 == variables.end() || itrVar2 == variables.end() || itrVar3 == variables.end()) {
            LOG(VERBOSE) << "Skipping variable (3D): " << name << " for sample: " << sample->name() << ", systematic" << systematic->name() << "\n";
            continue;
        }

        VariableHisto3D variableHisto3D(name);
        ROOT::RDF::RResultPtr<TH3D> histogram3D = this->book3Dhisto(node, v1, v2, v3, systematic);

        if (!histogram3D) {
            LOG(ERROR) << "Histogram for sample: " << sample->name() << ", systematic: "
                       << systematic->name() << ", region: " << region->name() << " and variable combination: " << v1.name() << " & " << v2.name() << " & " << v3.name() << " is empty!\n";
            throw std::runtime_error("");

        }

        variableHisto3D.setHisto(histogram3D);

        regionHisto->addVariableHisto3D(std::move(variableHisto3D));
    }
}

std::vector<std::pair<std::unique_ptr<TChain>, std::unique_ptr<TTreeIndex> > > MainFrame::connectTruthTrees(std::unique_ptr<TChain>& chain,
                                                                                                            const std::shared_ptr<Sample>& sample,
                                                                                                            const std::vector<std::string>& filePaths) {

    std::vector<std::pair<std::unique_ptr<TChain>, std::unique_ptr<TTreeIndex> > > result;

    static const std::vector<std::string> patternToCheck = {"el_.*", "mu_.*", "tau_.*", "jet_.*", "ph_.*", "ljet_.*"};

    for (const auto& itruth : sample->uniqueTruthTreeNamesForMatching()) {

        const std::vector<std::string>& indexNames = sample->recoToTruthPairingIndices();
        if (indexNames.empty() || indexNames.size() > 2) {
            LOG(ERROR) << "Reco to truth index names for sample: " << sample->name() << " are 0 or > 2\n";
            throw std::invalid_argument("");
        }

        LOG(INFO) << "Attaching tree: " << itruth << " to the reco tree\n";
        std::unique_ptr<TChain> truthChain = Utils::chainFromFiles(itruth, filePaths);

        const std::vector<std::string> branchConflicts = Utils::matchingBranchesFromChains(chain, truthChain, patternToCheck);

        if (!branchConflicts.empty()) {
            LOG(WARNING) << "Truth tree: " << itruth << " has the following branches with the same name as the reco branch:\n";
            std::string tmp("\t");
            for (const auto& ibranch : branchConflicts) {
                tmp += ibranch + " ";
            }
            LOG(WARNING) << tmp << "\n";
            LOG(WARNING) << "This can cause issues when doing the reco and truth tree matching!\n";
        }

        // add the branches to the systematicReplacer
        std::vector<std::string> listOfTruthBranches;
        const TObjArray* const truthBranchList = truthChain->GetListOfBranches();
        for (int ibranch = 0; ibranch < truthBranchList->GetSize(); ++ibranch) {
            const std::string name = truthBranchList->At(ibranch)->GetName();
            listOfTruthBranches.emplace_back(name);
        }
        m_systReplacer.addTruthBranchesNominal(itruth, listOfTruthBranches);

        std::unique_ptr<TTreeIndex> t(nullptr);
        if (indexNames.size() == 1 ) {
            LOG(INFO) << "Building reco truth index with: " << indexNames.at(0) << "\n";
            t = std::make_unique<TTreeIndex>(truthChain.get(), indexNames.at(0).c_str(), "0");
            if (t->IsZombie()) {
                LOG(ERROR) << "The TTreeIndex is a zombie!\n";
                throw std::runtime_error("");
            }
            truthChain->SetTreeIndex(t.get());
        } else {
            LOG(INFO) << "Building reco truth index with: " << indexNames.at(0) << " and " << indexNames.at(1) << "\n";
            t = std::make_unique<TTreeIndex>(truthChain.get(), indexNames.at(0).c_str(), indexNames.at(1).c_str());
            if (t->IsZombie()) {
                LOG(ERROR) << "The TTreeIndex is a zombie!\n";
                throw std::runtime_error("");
            }
            truthChain->SetTreeIndex(t.get());
        }
        chain->AddFriend(truthChain.get());
        result.emplace_back(std::make_pair(std::move(truthChain), std::move(t)));
    }

    return result;
}

std::vector<VariableHisto> MainFrame::processTruthHistos(const std::vector<std::string>& filePaths,
                                                         const std::shared_ptr<Sample>& sample,
                                                         const UniqueSampleID& id) {

    std::vector<VariableHisto> result;

    // prepare truth nodes with weights and custom definitions
    std::map<std::string, ROOT::RDF::RNode> rdfNodes = this->prepareTruthNodes(filePaths, sample, id);

    // apply filters and book histograms
    for (const auto& itruth : sample->truths()) {
        auto itr = rdfNodes.find(itruth->truthTreeName());
        if (itr == rdfNodes.end()) {
            LOG(ERROR) << "Cannot find truth tree name: " << itruth->truthTreeName() << "in the map!\n";
            throw std::runtime_error("");
        }

        ROOT::RDF::RNode mainNode = itr->second;

        // apply truth filter
        if (!itruth->selection().empty()) {
            mainNode = mainNode.Filter(itruth->selection());
        }

        // add histograms
        for (const auto& ivariable : itruth->variables()) {
            // get histograms (will NOT trigger event loop)
            const std::string name = itruth->name() + "_" + ivariable.name();
            VariableHisto hist(name);
            auto rdfHist = this->book1DhistoTruth(mainNode, ivariable, itruth);

            hist.setHisto(rdfHist);

            result.emplace_back(std::move(hist));
        }
    }

    return result;
}

void MainFrame::writeUnfoldingHistos(TFile* outputFile,
                                     const std::vector<SystematicHisto>& histos,
                                     const std::vector<VariableHisto>& truthHistos,
                                     const std::shared_ptr<Sample>& sample) const {

    for (const auto& itruth : sample->truths()) {
        if (!itruth->produceUnfolding()) continue;
        for (const auto& imatch : itruth->matchedVariables()) {
            const std::string& truthName = itruth->name() + "_" + imatch.second;
            const std::string& recoName = imatch.first;
            const std::string& migrationName = recoName + "_vs_" + truthName;

            std::unique_ptr<TH1D> truth = Utils::copyHistoFromVariableHistos(truthHistos, truthName);
            for (const auto& isystHist : histos) {
                if (isystHist.regionHistos().empty()) {
                    LOG(WARNING) << "No histograms available for sample: " << sample->name() << ", systematic: " << isystHist.name() << "\n";
                    continue;
                }
                outputFile->cd();
                if (!outputFile->GetDirectory(isystHist.name().c_str())) {
                    outputFile->mkdir(isystHist.name().c_str());
                }
                for (const auto& iregionHist : isystHist.regionHistos()) {
                    const std::string regionSubfolder = isystHist.name() + "/" + iregionHist.name();

                    if (m_config->useRegionSubfolders()) {
                        outputFile->cd();
                        if (!outputFile->GetDirectory(regionSubfolder.c_str())) {
                            outputFile->mkdir(regionSubfolder.c_str());
                        }
                    }

                    // skip nominal only variables for systematics
                    const Variable& recoVar = Utils::getVariableByName(sample->regions(), iregionHist.name(), recoName);
                    if (recoVar.isNominalOnly() && isystHist.name() != "NOSYS") continue;

                    std::unique_ptr<TH1D> reco = Utils::copyHistoFromVariableHistos(iregionHist.variableHistos(), recoName);
                    std::unique_ptr<TH2D> migration = Utils::copyHistoFromVariableHistos2D(iregionHist.variableHistos2D(), migrationName);

                    std::unique_ptr<TH1D> selectionEff(migration->ProjectionX(""));
                    selectionEff->Divide(truth.get());
                    selectionEff->SetDirectory(nullptr);

                    std::unique_ptr<TH1D> acceptance(migration->ProjectionY(""));
                    acceptance->Divide(reco.get());
                    acceptance->SetDirectory(nullptr);

                    const std::string selectionEffName = "selection_eff_" + truthName + "_" + iregionHist.name();
                    const std::string acceptanceName   = "acceptance_"    + itruth->name() + "_" + StringOperations::replaceString(recoName, "_NOSYS", "") + "_" + iregionHist.name();

                    // correct acceptance and selection eff?
                    if (m_config->capAcceptanceSelection()) {
                        const std::string systematics_name = isystHist.name();
                        Utils::capHisto0And1(selectionEff.get(), systematics_name + "/" + selectionEffName);
                        Utils::capHisto0And1(acceptance.get(), systematics_name + "/" + acceptanceName);
                    }

                    if (m_config->useRegionSubfolders()) {
                        outputFile->cd(regionSubfolder.c_str());
                    } else {
                        outputFile->cd(isystHist.name().c_str());
                    }
                    selectionEff->Write(selectionEffName.c_str());
                    acceptance->Write(acceptanceName.c_str());
                }
            }
        }
    }
}

ROOT::RDF::RNode MainFrame::addCustomDefinesFromConfig(ROOT::RDF::RNode mainNode,
                                                       const std::shared_ptr<Sample>& sample) {

    if (sample->customRecoDefines().empty()) return mainNode;

    for (const auto& idefine : sample->customRecoDefines()) {
        const std::string& name = idefine->columnName();
        const std::string& expression = idefine->formula();
        LOG(DEBUG) << "Adding custom variable from config: " << name << ", formula: " << expression << "\n";
        mainNode = this->systematicStringDefine(mainNode, name, expression);
    }

    return mainNode;
}

ROOT::RDF::RNode MainFrame::systematicStringDefine(ROOT::RDF::RNode mainNode,
                                                   const std::string& name,
                                                   const std::string& formula) {

    if (name.find("NOSYS") == std::string::npos) {
        LOG(ERROR) << "The variable: " << name << ", does not contain \"NOSYS\"\n";
        throw std::invalid_argument("");
    }

    // add nominal
    mainNode = mainNode.Define(name, formula);

    // first find on which variables the formula depends that are affected by systematics
    const std::vector<std::string> affectedVariables = m_systReplacer.listOfVariablesAffected(formula);

    const std::vector<std::string> systematicList = m_systReplacer.getListOfEffectiveSystematics(affectedVariables);

    // now propagate
    for (const auto& isyst : systematicList) {
        if (isyst == "NOSYS") continue; // we already added nominal
        const std::string newName = StringOperations::replaceString(name, "NOSYS", isyst);
        const std::string newFormula = m_systReplacer.replaceString(formula, isyst);
        LOG(VERBOSE) << "Adding custom variable using strings: " << newName << ", formula: " << newFormula << "\n";

        mainNode = mainNode.Define(newName, newFormula);

    }
    m_systReplacer.addVariableAndEffectiveSystematics(name, systematicList);

    return mainNode;
}

ROOT::RDF::RNode MainFrame::systematicStringRedefine(ROOT::RDF::RNode mainNode,
                                                   const std::string& name,
                                                   const std::string& formula) {

    if (name.find("NOSYS") == std::string::npos) {
        LOG(ERROR) << "The variable: " << name << ", does not contain \"NOSYS\"\n";
        throw std::invalid_argument("");
    }

    // The usage of Define() vs. Redefine() just depends on whether or not the
    // variable has already been defined in the RDF. The result for
    // m_systReplacer is the same either way: it just keeps track of the nominal
    // and syst columns. So we dont do m_systReplacer.branchExists() here,
    // instead we look at mainNode.GetColumnNames() to tell us if the column
    // needs to be defined or redefined.
    auto columnNames = mainNode.GetColumnNames();
    if (std::find(columnNames.begin(), columnNames.end(), name) == columnNames.end()) {
        // we can assume that if the _NOSYS column doesn't exist,
        // then none of the systematic ones do either
        LOG(VERBOSE) << "No variable " << name << " exists to redefine, making new variable instead\n";
        return systematicStringDefine(mainNode, name, formula);
    }

    // redefine nominal
    mainNode = mainNode.Redefine(name, formula);

    // find systematics that could affect the result of this formula
    const std::vector<std::string> affectedVariables = m_systReplacer.listOfVariablesAffected(formula);
    const std::vector<std::string> systematicList = m_systReplacer.getListOfEffectiveSystematics(affectedVariables);

    // redefine each systematic
    for (const auto& isyst : systematicList) {
        if (isyst == "NOSYS") continue; // we already added nominal

        const std::string systName = StringOperations::replaceString(name, "NOSYS", isyst);
        const std::string systFormula = m_systReplacer.replaceString(formula, isyst);
        LOG(VERBOSE) << "Adding custom variable from config: " << systName << ", formula: " << systFormula << "\n";

        // it is possible that redefining the variable changes systematics, so
        // we have to check if we need Define() or Redefine() here too
        if (std::find(columnNames.begin(), columnNames.end(), systName) == columnNames.end()) {
            mainNode = mainNode.Define(systName, systFormula);
        } else {
            mainNode = mainNode.Redefine(systName, systFormula);
        }
    }

    // need to update the variable's affecting systematics in m_systReplacer
    m_systReplacer.updateVariableAndEffectiveSystematics(name, systematicList);

    return mainNode;
}

ROOT::RDF::RNode MainFrame::addCustomTruthDefinesFromConfig(ROOT::RDF::RNode mainNode,
                                                            const std::shared_ptr<Sample>& sample,
                                                            const std::string& treeName) const {

    for (const auto& idefine : sample->customTruthDefines()) {
        if (treeName != idefine->treeName()) continue;
        mainNode = mainNode.Define(idefine->columnName(), idefine->formula());
    }

    return mainNode;
}

ROOT::RDF::RNode MainFrame::minMaxRange(ROOT::RDF::RNode node) const {
    if (m_config->minEvent() >= 0 || m_config->maxEvent() >= 0) {
        long long int min = m_config->minEvent() >= 0 ? m_config->minEvent() : 0;
        long long int max = m_config->maxEvent() >= 0 ? m_config->maxEvent() : 0;
        LOG(INFO) << "Will only run for range: [" << min << "," << max << ")\n";
        return node.Range(min, max);
    }

    return node;
}

ROOT::RDF::RResultPtr<TH1D> MainFrame::book1Dhisto(ROOT::RDF::RNode node,
                                                   const Variable& variable,
                                                   const std::shared_ptr<Systematic>& systematic) const {

    switch (variable.type()) {
        case VariableType::UNDEFINED:
            return node.Histo1D(variable.histoModel1D(),
                                this->systematicVariable(variable, systematic),
                                this->systematicWeight(systematic));
            break;
        ADD_HISTO_1D_SUPPORT_VECTOR(CHAR,char)
        ADD_HISTO_1D_SUPPORT_VECTOR(UNSIGNED_CHAR,unsigned char)
        ADD_HISTO_1D_SUPPORT_SCALAR(BOOL,bool) // Special case: std::vector<bool> is not supported
        ADD_HISTO_1D_SUPPORT_VECTOR(INT,int)
        ADD_HISTO_1D_SUPPORT_VECTOR(UNSIGNED_INT,unsigned int)
        ADD_HISTO_1D_SUPPORT_VECTOR(LONG_INT,long long int)
        ADD_HISTO_1D_SUPPORT_VECTOR(UNSIGNED,unsigned long)
        ADD_HISTO_1D_SUPPORT_VECTOR(LONG_UNSIGNED,unsigned long long)
        ADD_HISTO_1D_SUPPORT_VECTOR(FLOAT,float)
        ADD_HISTO_1D_SUPPORT_VECTOR(DOUBLE,double)
    }
    return node.Histo1D(variable.histoModel1D(),
                        this->systematicVariable(variable, systematic),
                        this->systematicWeight(systematic));
}

ROOT::RDF::RResultPtr<TH1D> MainFrame::book1DhistoTruth(ROOT::RDF::RNode node,
                                                        const Variable& variable,
                                                        const std::shared_ptr<Truth>& truth) const {

    const std::string weightName = "weight_truth_TOTAL_" + truth->name();

    std::string definition = variable.definition();
    auto itrTree = m_variablesWithFormulaTruth.find(truth->truthTreeName());
    if (itrTree != m_variablesWithFormulaTruth.end()) {
        auto itr = itrTree->second.find(definition);
        if (itr != itrTree->second.end()) {
            definition = itr->second;
            LOG(VERBOSE) << "Truth tree: " << truth->truthTreeName() << ", variable: " << variable.name() << ", using definition: " << definition << "\n";
        }
    }

    if (StringOperations::stringStartsWith(definition, truth->truthTreeName()+".")) {
        const std::string prefix = truth->truthTreeName() + ".";
        LOG(DEBUG) << "Identified truth variable that contains \"treeName.\" prefix: " << definition << "\n";
        definition = definition.substr(prefix.length());
        LOG(DEBUG) << "Final definition: " << definition << "\n";
    }

    switch (variable.type()) {
        case VariableType::UNDEFINED:
            return node.Histo1D(variable.histoModel1D(),
                                definition,
                                weightName);
            break;
        ADD_HISTO_1D_SUPPORT_VECTOR_TRUTH(CHAR,char)
        ADD_HISTO_1D_SUPPORT_VECTOR_TRUTH(UNSIGNED_CHAR,unsigned char)
        ADD_HISTO_1D_SUPPORT_SCALAR_TRUTH(BOOL,bool) // Special case: std::vector<bool> is not supported
        ADD_HISTO_1D_SUPPORT_VECTOR_TRUTH(INT,int)
        ADD_HISTO_1D_SUPPORT_VECTOR_TRUTH(UNSIGNED_INT,unsigned int)
        ADD_HISTO_1D_SUPPORT_VECTOR_TRUTH(LONG_INT,long long int)
        ADD_HISTO_1D_SUPPORT_VECTOR_TRUTH(UNSIGNED,unsigned long)
        ADD_HISTO_1D_SUPPORT_VECTOR_TRUTH(LONG_UNSIGNED,unsigned long long)
        ADD_HISTO_1D_SUPPORT_VECTOR_TRUTH(FLOAT,float)
        ADD_HISTO_1D_SUPPORT_VECTOR_TRUTH(DOUBLE,double)
    }
    return node.Histo1D(variable.histoModel1D(),
                        variable.definition(),
                        weightName);
}

ROOT::RDF::RResultPtr<TH2D> MainFrame::book2Dhisto(ROOT::RDF::RNode node,
                                                   const Variable& variable1,
                                                   const Variable& variable2,
                                                   const std::shared_ptr<Systematic>& systematic) const {
    #ifndef NOT_JIT_2D_HISTOGRAMS
    const auto type1 = variable1.type();
    const auto type2 = variable2.type();

    if (type1 == VariableType::UNDEFINED || type2 == VariableType::UNDEFINED) {
        return node.Histo2D(Utils::histoModel2D(variable1, variable2),
                            this->systematicVariable(variable1, systematic),
                            this->systematicVariable(variable2, systematic),
                            this->systematicWeight(systematic));
    }
    ADD_HISTO_2D_SUPPORT_VECTOR(CHAR,char)
    ADD_HISTO_2D_SUPPORT_VECTOR(INT,int)
    ADD_HISTO_2D_SUPPORT_VECTOR(UNSIGNED_INT,unsigned int)
    ADD_HISTO_2D_SUPPORT_VECTOR(LONG_INT,long long int)
    ADD_HISTO_2D_SUPPORT_VECTOR(UNSIGNED,unsigned long)
    ADD_HISTO_2D_SUPPORT_VECTOR(LONG_UNSIGNED,unsigned long long)
    ADD_HISTO_2D_SUPPORT_VECTOR(FLOAT,float)
    ADD_HISTO_2D_SUPPORT_VECTOR(DOUBLE,double)
    ADD_HISTO_2D_PAIR_NO_VECTOR(BOOL,bool,BOOL,bool) // Special case: std::vector<bool> is not supported
    #endif

    // Rely on the JIT compiler to do 2D histograms.
    return node.Histo2D(Utils::histoModel2D(variable1, variable2),
                        this->systematicVariable(variable1, systematic),
                        this->systematicVariable(variable2, systematic),
                        this->systematicWeight(systematic));
}

ROOT::RDF::RResultPtr<TH3D> MainFrame::book3Dhisto(ROOT::RDF::RNode node,
                                                   const Variable& variable1,
                                                   const Variable& variable2,
                                                   const Variable& variable3,
                                                   const std::shared_ptr<Systematic>& systematic) const {
    // Rely on the JIT compiler to do 3D histograms.
    return node.Histo3D(Utils::histoModel3D(variable1, variable2, variable3),
                        this->systematicVariable(variable1, systematic),
                        this->systematicVariable(variable2, systematic),
                        this->systematicVariable(variable3, systematic),
                        this->systematicWeight(systematic));
}

std::vector<CutflowContainer> MainFrame::bookCutflows(ROOT::RDF::RNode node,
                                                      const std::shared_ptr<Sample>& sample) const {

    std::vector<CutflowContainer> result;

    if (!sample->hasCutflows()) return result;

    // add weight squared for stat uncertainty
    node = node.Define("weight_total_squared_NOSYS", [](const double weight){return weight*weight;}, {"weight_total_NOSYS"});

    for (const auto& cutflow : sample->cutflows()) {
        CutflowContainer container(cutflow->name());

        // add initial set
        auto initialWeight        = node.Sum("weight_total_NOSYS");
        auto initialWeightSquared = node.Sum("weight_total_squared_NOSYS");
        container.addBookedYields(initialWeight, initialWeightSquared, "Initial");

        auto filteredNode = node;

        // add sample suffix, if it exists
        if (!sample->selectionSuffix().empty()) {
            filteredNode             = filteredNode.Filter(sample->selectionSuffix());
            auto suffixWeight        = filteredNode.Sum("weight_total_NOSYS");
            auto suffixWeightSquared = filteredNode.Sum("weight_total_squared_NOSYS");
            container.addBookedYields(suffixWeight, suffixWeightSquared, "SuffixSelection");
        }

        for (const auto& iselection : cutflow->selections()) {
            const std::string& finalSelection = iselection.first;
            filteredNode       = filteredNode.Filter(finalSelection);
            auto weight        = filteredNode.Sum("weight_total_NOSYS");
            auto weightSquared = filteredNode.Sum("weight_total_squared_NOSYS");
            container.addBookedYields(weight, weightSquared, iselection.second);
        }

        result.emplace_back(std::move(container));
    }

    return result;
}

ROOT::RDF::RNode MainFrame::prepareWeightMetadata(ROOT::RDF::RNode node,
                                                  const std::shared_ptr<Sample>& sample,
                                                  const UniqueSampleID& id) const {

    if (sample->isData()) return node;

    const std::vector<std::string> allBranches = node.GetColumnNames();

    // add lumi
    if (std::find(allBranches.begin(), allBranches.end(), "lumi") == allBranches.end()) {
        node = node.Define("lumi", [this, &id](){return this->m_metadataManager.luminosity(id.campaign());}, {});
    }

    // add cross-section
    if (std::find(allBranches.begin(), allBranches.end(), "xSection") == allBranches.end()) {
        node = node.Define("xSection", [this, &id](){return this->m_metadataManager.crossSection(id);}, {});
    }

    std::vector<std::string> uniqueSumWeights;
    for (const auto& isyst : sample->systematics()) {
        const std::string sumWeightName = "sumWeights_" + isyst->sumWeights();
        if (std::find(uniqueSumWeights.begin(), uniqueSumWeights.end(), sumWeightName) != uniqueSumWeights.end()) continue;

        // is unique sumweights
        const double sumW = m_metadataManager.sumWeights(id, isyst);

        if (std::find(allBranches.begin(), allBranches.end(), sumWeightName) == allBranches.end()) {
            node = node.Define(sumWeightName, [sumW](){return sumW;}, {});
        }

        uniqueSumWeights.emplace_back(sumWeightName);
    }

    return node;
}

ROOT::RDF::RNode MainFrame::prepareWeightMetadataAllUniqueSamples(ROOT::RDF::RNode node,
                                                                  const std::shared_ptr<Sample>& sample) const {

    if (sample->isData()) return node;

    const std::vector<std::string> allBranches = node.GetColumnNames();

    // add lumi
    if (std::find(allBranches.begin(), allBranches.end(), "lumi") == allBranches.end()) {
        node = node.DefinePerSample("lumi", [](unsigned int /*slot*/, const RSampleInfo& id){return id.GetD("luminosity");});
    }

    // add lumi
    if (std::find(allBranches.begin(), allBranches.end(), "xSection") == allBranches.end()) {
        node = node.DefinePerSample("xSection", [](unsigned int /*slot*/, const RSampleInfo& id){return id.GetD("xSection");});
    }

    std::vector<std::string> uniqueSumWeights;
    for (const auto& isyst : sample->systematics()) {
        const std::string sumWeightName = "sumWeights_" + isyst->sumWeights();
        if (std::find(uniqueSumWeights.begin(), uniqueSumWeights.end(), sumWeightName) != uniqueSumWeights.end()) continue;

        const std::string weightName = "sumWeights_" + isyst->sumWeights();
        if (std::find(allBranches.begin(), allBranches.end(), sumWeightName) == allBranches.end()) {
            node = node.DefinePerSample(sumWeightName, [weightName](unsigned int /*slot*/, const RSampleInfo& id){return id.GetD(weightName);});
        }

        uniqueSumWeights.emplace_back(sumWeightName);
    }

    return node;
}

ROOT::RDF::RNode MainFrame::addTruthVariablesToReco(ROOT::RDF::RNode node,
                                                    const std::shared_ptr<Sample>& sample,
                                                    const UniqueSampleID& id) {

    std::vector<std::string> uniqueTrees;

    for (const auto& itruth : sample->truths()) {
        if (!itruth->matchRecoTruth()) continue;

        const std::string& treeName = itruth->truthTreeName();
        if (std::find(uniqueTrees.begin(), uniqueTrees.end(), treeName) != uniqueTrees.end()) continue;

        LOG(DEBUG) << "Adding custom truth variables from the code for tree: " << treeName << "\n";
        if (!m_config->configDefineAfterCustomClass()) {
            node = this->addCustomTruthDefinesFromConfig(node, sample, treeName);
        }
        node = this->defineVariablesTruth(node, treeName, sample, id);
        if (m_config->configDefineAfterCustomClass()) {
            node = this->addCustomTruthDefinesFromConfig(node, sample, treeName);
        }
        LOG(DEBUG) << "Finished adding custom truth variables\n";

        node = this->addVariablesWithFormulaTruth(node, sample, treeName);
        LOG(DEBUG) << "Finished adding all truth variables\n";
        uniqueTrees.emplace_back(treeName);
    }

    return node;
}

std::map<std::string, ROOT::RDF::RNode> MainFrame::prepareTruthNodes(const std::vector<std::string>& filePaths,
                                                                     const std::shared_ptr<Sample>& sample,
                                                                     const UniqueSampleID& id) {

    std::map<std::string, ROOT::RDF::RNode> result;

    const std::vector<std::string> uniqueTreeNames = sample->uniqueTruthTreeNames();
    for (const auto& iTree : uniqueTreeNames) {
        ROOT::RDataFrame rdf(iTree, filePaths);

        ROOT::RDF::RNode mainNode = rdf;

        #if ROOT_VERSION_CODE > ROOT_VERSION(6,29,0)
        ROOT::RDF::Experimental::AddProgressBar(mainNode);
        #endif

        mainNode = this->minMaxRange(mainNode);

        mainNode = this->prepareWeightMetadata(mainNode, sample, id);

        // add weight columns
        for (const auto& itruth : sample->truths()) {

            const std::string& truthTreeName = itruth->truthTreeName();
            if (truthTreeName != iTree) continue;

            const std::string normalisation = "lumi*xSection/sumWeights_NOSYS";
            const std::string totalWeight = "(" + itruth->eventWeight() + ")*(" + normalisation +")";
            const std::string weightName = "weight_truth_TOTAL_" + itruth->name();
            LOG(DEBUG) << "Adding column: " << weightName << " with formula " << totalWeight << "\n";
            mainNode = mainNode.Define(weightName, totalWeight);
        }

        LOG(DEBUG) << "Adding custom variables to truth tree: " << iTree << "\n";
        // add custom variables
        for (const auto& itruth : sample->truths()) {
            const std::string& truthTreeName = itruth->truthTreeName();
            if (truthTreeName != iTree) continue;

            if (!m_config->configDefineAfterCustomClass()) {
                mainNode = this->addCustomTruthDefinesFromConfig(mainNode, sample, truthTreeName);
            }
            mainNode = this->defineVariablesTruth(mainNode, truthTreeName, sample, id);
            if (m_config->configDefineAfterCustomClass()) {
                mainNode = this->addCustomTruthDefinesFromConfig(mainNode, sample, truthTreeName);
            }
            break;
        }
        LOG(DEBUG) << "Finished adding custom variables to truth tree: " << iTree << "\n";

        mainNode = this->addVariablesWithFormulaTruth(mainNode, sample, iTree);

        LOG(DEBUG) << "Finished adding all variables to truth tree: " << iTree << "\n";

        result.insert(std::make_pair(iTree, std::move(mainNode)));
    }

    return result;
}

void MainFrame::processSingleTruthTreeNtuple(const std::shared_ptr<Truth>& truth,
                                             const std::vector<std::string>& filePaths,
                                             const std::string& outputFilePath,
                                             const std::shared_ptr<Sample>& sample,
                                             const UniqueSampleID& id) {

    LOG(INFO) << "Processing truth ntuple: " << truth->name() << ", from TTree: " << truth->truthTreeName() << "\n";

    auto chain = Utils::chainFromFiles(truth->truthTreeName(), filePaths);

    ROOT::RDataFrame df(*chain);

    ROOT::RDF::RNode mainNode = df;
    #if ROOT_VERSION_CODE > ROOT_VERSION(6,29,0)
    ROOT::RDF::Experimental::AddProgressBar(mainNode);
    #endif

    LOG(DEBUG) << "Adding truth variables from the custom class\n";
    mainNode = this->defineVariablesNtupleTruth(mainNode, truth->truthTreeName(), sample, id);
    LOG(DEBUG) << "Finished adding truth variables from the custom class\n";

    if (!truth->selection().empty()) {
        mainNode = mainNode.Filter(truth->selection());
    }

    const std::vector<std::string> branches = Utils::selectedNotExcludedElements(mainNode.GetColumnNames(),
                                                                                 truth->branches(),
                                                                                 truth->excludedBranches());

    LOG(VERBOSE) << "Selected branches: \n";
    for (const auto& ibranch : branches) {
        LOG(VERBOSE) << "branch: " << ibranch << "\n";
    }

    if (branches.empty()) {
        LOG(INFO) << "No selected branches for truth tree: " << truth->truthTreeName() << " - will not produce the truth tree\n";
        return;
    }

    ROOT::RDF::RSnapshotOptions opts;
    opts.fMode = "UPDATE";
    opts.fAutoFlush = m_config->ntupleAutoFlush();
    opts.fCompressionLevel = m_config->ntupleCompressionLevel();
    opts.fVector2RVec = m_config->configDefineAfterCustomClass();
    mainNode.Snapshot(truth->name(), outputFilePath, branches, opts);
}

ROOT::RDF::RNode MainFrame::addVariablesWithFormulaReco(ROOT::RDF::RNode node,
                                                        const std::shared_ptr<Sample>& sample,
                                                        const std::vector<std::string>& truthTrees) {
    m_variablesWithFormulaReco = Utils::variablesWithFormulaReco(node, sample, truthTrees);

    ROOT::RDF::RNode outNode = node;

    for (const auto& [formula, name] : m_variablesWithFormulaReco) {
        LOG(DEBUG) << "Adding column: " << name << " with formula: " << formula << "\n";
        outNode = this->systematicStringDefine(outNode, name, formula);
    }

    return outNode;
}

ROOT::RDF::RNode MainFrame::addVariablesWithFormulaTruth(ROOT::RDF::RNode node,
                                                         const std::shared_ptr<Sample>& sample,
                                                         const std::string& treeName) {

    auto map = Utils::variablesWithFormulaTruth(node, sample, treeName);

    ROOT::RDF::RNode outNode = node;

    for (const auto& [formula, name] : map) {
        LOG(DEBUG) << "Adding column: " << name << " with formula: " << formula << " from truth tree: " << treeName << " to the reco tree\n";
        outNode = outNode.Define(name, formula);
    }

    m_variablesWithFormulaTruth.insert({treeName, map   });

    return outNode;
}

void MainFrame::prepareONNXwrapper() {
    for (const auto& infer : m_config->simpleONNXInferences()) {
        infer->initializeModels();
    }
}
