/**
 * @file ConfigReaderCpp.cxx
 * @brief Source file for ConfigReaderCpp
 *
 */
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <string>

#include "python_wrapper/headers/ConfigSettingWrapper.h"
#include "python_wrapper/headers/RegionWrapper.h"
#include "python_wrapper/headers/VariableWrapper.h"
#include "python_wrapper/headers/MainFrameWrapper.h"
#include "python_wrapper/headers/FastFramesExecutorWrapper.h"
#include "python_wrapper/headers/SampleWrapper.h"
#include "python_wrapper/headers/SystematicWrapper.h"
#include "python_wrapper/headers/NtupleWrapper.h"
#include "python_wrapper/headers/TruthWrapper.h"
#include "python_wrapper/headers/CutflowWrapper.h"
#include "python_wrapper/headers/ConfigDefineWrapper.h"
#include "python_wrapper/headers/SimpleONNXInferenceWrapper.h"

#include "python_wrapper/headers/DuplicateEventsChecker.h"
#include "python_wrapper/headers/SumWeightsGetter.h"

#include "FastFrames/Binning.h"

using namespace std;

/**
 * @brief Construct a new boost python module object containing all the classes that can be used from python
 *
 */
BOOST_PYTHON_MODULE(ConfigReaderCpp) {
    // An established convention for using boost.python.
    using namespace boost::python;

    /**
     * @brief python wrapper around std::vector<unsigned long long int>
     *
     */
    boost::python::class_<std::vector<unsigned long long int>>("ptrVector")
    .def(boost::python::vector_indexing_suite<std::vector<unsigned long long int>>());

    /**
     * @brief python wrapper around std::vector<std::string>
     *
     */
    boost::python::class_<std::vector<std::string>>("StringVector")
    .def(boost::python::vector_indexing_suite<std::vector<std::string>>());

    /**
     * @brief python wrapper around std::vector<unsigned int>
     *
     */
    boost::python::class_<std::vector<unsigned int>>("UIntVector")
    .def(boost::python::vector_indexing_suite<std::vector<unsigned int>>());

    /**
     * @brief python wrapper around std::vector<std::tuple<std::string, std::string>>
     *
     */
    boost::python::class_<std::vector<boost::python::tuple>>("StringPairVector")
    .def(boost::python::vector_indexing_suite<std::vector<boost::python::tuple>>());

    /**
     * @brief python wrapper around std::vector<double>
     *
     */
    boost::python::class_<std::vector<double>>("DoubleVector")
    .def(boost::python::vector_indexing_suite<std::vector<double>>());

    /**
     * @brief Python wrapper around FastFramesExecutor
     *
     */
    class_<FastFramesExecutorWrapper>("FastFramesExecutor",
        init<long long unsigned int>())
        // runFastFrames
        .def("runFastFrames",   &FastFramesExecutorWrapper::runFastFrames)

        // setRunNtuples
        .def("setRunNtuples",   &FastFramesExecutorWrapper::setRunNtuples)
        .def("runNtuples",      &FastFramesExecutorWrapper::runNtuples)
    ;

    /**
     * @brief Python wrapper around MainFrame
     *
     */
    class_<MainFrameWrapper>("MainFrame",
        init<>())
        // setConfig
        .def("setConfig",           &MainFrameWrapper::setConfig)

        // init
        .def("init",                &MainFrameWrapper::init)

        // executeHistograms
        .def("executeHistograms",   &MainFrameWrapper::executeHistograms)
    ;

    /**
     * @brief Python wrapper around ConfigSetting
     *
     */
    class_<ConfigSettingWrapper>("ConfigSettingWrapper",
        init<>())
        // getPtr
        .def("getPtr",          &ConfigSettingWrapper::getPtr)

        // outputPathHistograms
        .def("outputPathHistograms",      &ConfigSettingWrapper::outputPathHistograms)
        .def("setOutputPathHistograms",   &ConfigSettingWrapper::setOutputPathHistograms)

        // outputPathNtuples
        .def("outputPathNtuples",      &ConfigSettingWrapper::outputPathNtuples)
        .def("setOutputPathNtuples",   &ConfigSettingWrapper::setOutputPathNtuples)

        // inputSumWeightsPath
        .def("inputSumWeightsPath",    &ConfigSettingWrapper::inputSumWeightsPath)
        .def("setInputSumWeightsPath", &ConfigSettingWrapper::setInputSumWeightsPath)

        // inputFilelistPath
        .def("inputFilelistPath",   &ConfigSettingWrapper::inputFilelistPath)
        .def("setInputFilelistPath",&ConfigSettingWrapper::setInputFilelistPath)

        // customFrameName
        .def("customFrameName",     &ConfigSettingWrapper::customFrameName)
        .def("setCustomFrameName",  &ConfigSettingWrapper::setCustomFrameName)

        // numCPU
        .def("numCPU",      &ConfigSettingWrapper::numCPU)
        .def("setNumCPU",   &ConfigSettingWrapper::setNumCPU)

        // Luminosity
        .def("setLuminosity", &ConfigSettingWrapper::addLuminosityInformation)
        .def("getLuminosity", &ConfigSettingWrapper::getLuminosity)
        .def("campaignIsDefined", &ConfigSettingWrapper::campaignIsDefined)

        // x-section files
        .def("addXsectionFile",             &ConfigSettingWrapper::addXsectionFile)
        .def("xSectionFiles",               &ConfigSettingWrapper::xSectionFiles)

        // TLorentzVectors
        .def("addTLorentzVector",           &ConfigSettingWrapper::addTLorentzVector)
        .def("tLorentzVectors",             &ConfigSettingWrapper::tLorentzVectors)
        .def("setUseRVec",                  &ConfigSettingWrapper::setUseRVec)
        .def("useRVec",                     &ConfigSettingWrapper::useRVec)

        // addRegion
        .def("addRegion",           &ConfigSettingWrapper::addRegion)
        .def("getVariableNames",    &ConfigSettingWrapper::getVariableNames)
        .def("getRegionsSharedPtr", &ConfigSettingWrapper::getRegionsSharedPtr)

        // addSample
        .def("addSample",           &ConfigSettingWrapper::addSample)
        .def("getSamplesSharedPtr", &ConfigSettingWrapper::getSamplesSharedPtr)

        // addSystematic
        .def("addSystematic",           &ConfigSettingWrapper::addSystematic)
        .def("getSystematicsSharedPtr", &ConfigSettingWrapper::getSystematicsSharedPtr)

        .def("clearSystematics",  &ConfigSettingWrapper::clearSystematics)

        .def("setNtuple",           &ConfigSettingWrapper::setNtuple)
        .def("getNtupleSharedPtr",  &ConfigSettingWrapper::getNtupleSharedPtr)

        // min and max event
        .def("setMinEvent",         &ConfigSettingWrapper::setMinEvent)
        .def("minEvent",            &ConfigSettingWrapper::minEvent)
        .def("setMaxEvent",         &ConfigSettingWrapper::setMaxEvent)
        .def("maxEvent",            &ConfigSettingWrapper::maxEvent)

        // total job splits
        .def("setTotalJobSplits",   &ConfigSettingWrapper::setTotalJobSplits)
        .def("totalJobSplits",      &ConfigSettingWrapper::totalJobSplits)

        // current job index
        .def("setCurrentJobIndex",  &ConfigSettingWrapper::setCurrentJobIndex)
        .def("currentJobIndex",     &ConfigSettingWrapper::currentJobIndex)

        // capAcceptanceSelection
        .def("setCapAcceptanceSelection",  &ConfigSettingWrapper::setCapAcceptanceSelection)
        .def("capAcceptanceSelection",     &ConfigSettingWrapper::capAcceptanceSelection)

        // customOptions
        .def("addCustomOption",                   &ConfigSettingWrapper::addOption)
        .def("getCustomOption",                   &ConfigSettingWrapper::getOption)
        .def("getCustomOptionWithDefault",        &ConfigSettingWrapper::getOptionWithDefault)
        .def("hasCustomOption",                   &ConfigSettingWrapper::hasOption)
        .def("getCustomKeys",                     &ConfigSettingWrapper::getKeys)

        .def("setConfigDefineAfterCustomClass",   &ConfigSettingWrapper::setConfigDefineAfterCustomClass)
        .def("configDefineAfterCustomClass",      &ConfigSettingWrapper::configDefineAfterCustomClass)

        .def("setUseRegionSubfolders",          &ConfigSettingWrapper::setUseRegionSubfolders)
        .def("useRegionSubfolders",             &ConfigSettingWrapper::useRegionSubfolders)

        .def("simpleONNXInferencesSharedPtrs",  &ConfigSettingWrapper::simpleONNXInferencesSharedPtrs)
        .def("addSimpleONNXInference",          &ConfigSettingWrapper::addSimpleONNXInference)
        .def("setListOfSystematicsName",        &ConfigSettingWrapper::setListOfSystematicsName)
        .def("listOfSystematicsName",           &ConfigSettingWrapper::listOfSystematicsName)

        .def("setNtupleCompressionLevel",       &ConfigSettingWrapper::setNtupleCompressionLevel)
        .def("ntupleCompressionLevel",          &ConfigSettingWrapper::ntupleCompressionLevel)

        .def("setNtupleAutoFlush",              &ConfigSettingWrapper::setNtupleAutoFlush)
        .def("ntupleAutoFlush",                 &ConfigSettingWrapper::ntupleAutoFlush)

        .def("setSplitProcessingPerUniqueSample",&ConfigSettingWrapper::setSplitProcessingPerUniqueSample)
        .def("splitProcessingPerUniqueSample",   &ConfigSettingWrapper::splitProcessingPerUniqueSample)

        .def("setConvertVectorToRVec",          &ConfigSettingWrapper::setConvertVectorToRVec)
        .def("convertVectorToRVec",             &ConfigSettingWrapper::convertVectorToRVec)
    ;

    /**
     * @brief Python wrapper around Region
     *
     */
    class_<RegionWrapper>("RegionWrapper",
        init<std::string>())
        // getPtr
        .def("getPtr",          &RegionWrapper::getPtr)

        // constructFromSharedPtr
        .def("constructFromSharedPtr",  &RegionWrapper::constructFromSharedPtr)

        // name
        .def("name",            &RegionWrapper::name)

        // selection
        .def("selection",       &RegionWrapper::selection)
        .def("setSelection",    &RegionWrapper::setSelection)

        // addVariable
        .def("addVariable",         &RegionWrapper::addVariable)
        .def("getVariableRawPtrs",  &RegionWrapper::getVariableRawPtrs)
        .def("getVariableNames",    &RegionWrapper::getVariableNames)

        // addVariableCombination
        .def("addVariableCombination",   &RegionWrapper::addVariableCombination)
        .def("variableCombinations",    &RegionWrapper::variableCombinations)

        // addVariableCombination3D
        .def("addVariableCombination3D",   &RegionWrapper::addVariableCombination3D)
        .def("variableCombinations3D",    &RegionWrapper::variableCombinations3D)
    ;

    /**
     * @brief Python wrapper around Variable
     *
     */
    class_<VariableWrapper>("VariableWrapper",
        init<std::string>())
        // getPtr
        .def("getPtr",                  &VariableWrapper::getPtr)
        .def("constructFromSharedPtr",  &VariableWrapper::constructFromSharedPtr)
        .def("constructFromRawPtr",     &VariableWrapper::constructFromRawPtr)

        // name
        .def("name",            &VariableWrapper::name)

        // definition
        .def("definition",      &VariableWrapper::definition)
        .def("setDefinition",   &VariableWrapper::setDefinition)

        // title
        .def("title",               &VariableWrapper::title)
        .def("setTitle",            &VariableWrapper::setTitle)

        // binning
        .def("setBinningRegular",   &VariableWrapper::setBinningRegular)
        .def("setBinningIrregular", &VariableWrapper::setBinningIrregular)
        .def("hasRegularBinning",   &VariableWrapper::hasRegularBinning)

        .def("binEdges",            &VariableWrapper::binEdges)
        .def("binEdgesString",      &VariableWrapper::binEdgesString)
        .def("axisMin",             &VariableWrapper::axisMin)
        .def("axisMax",             &VariableWrapper::axisMax)
        .def("axisNbins",           &VariableWrapper::axisNbins)

        // isNominalOnly
        .def("setIsNominalOnly",    &VariableWrapper::setIsNominalOnly)
        .def("isNominalOnly",       &VariableWrapper::isNominalOnly)

        // type
        .def("type",               &VariableWrapper::type)
        .def("setType",            &VariableWrapper::setType)
    ;

    /**
     * @brief Python wrapper around Sample
     *
     */
    class_<SampleWrapper>("SampleWrapper",
        init<std::string>())

        // getPtr
        .def("getPtr",          &SampleWrapper::getPtr)

        // constructFromSharedPtr
        .def("constructFromSharedPtr",  &SampleWrapper::constructFromSharedPtr)

        // name
        .def("name",            &SampleWrapper::name)

        // recoTreeName
        .def("setRecoTreeName",     &SampleWrapper::setRecoTreeName)
        .def("recoTreeName",        &SampleWrapper::recoTreeName)

        // selection
        .def("setSelectionSuffix",  &SampleWrapper::setSelectionSuffix)
        .def("selectionSuffix",     &SampleWrapper::selectionSuffix)

        // uniqueSampleID
        .def("addUniqueSampleID",   &SampleWrapper::addUniqueSampleID)
        .def("nUniqueSampleIDs",    &SampleWrapper::nUniqueSampleIDs)
        .def("uniqueSampleIDstring",&SampleWrapper::uniqueSampleIDstring)

        // systematic
        .def("addSystematic",           &SampleWrapper::addSystematic)
        .def("nSystematics",            &SampleWrapper::nSystematics)
        .def("getSystematicPtr",        &SampleWrapper::getSystematicPtr)
        .def("systematicsNames",        &SampleWrapper::systematicsNames)
        .def("hasSystematics",          &SampleWrapper::hasSystematics)
        .def("getSystematicsSharedPtr", &SampleWrapper::getSystematicsSharedPtr)

        // addRegion
        .def("addRegion",           &SampleWrapper::addRegion)
        .def("regionsNames",        &SampleWrapper::regionsNames)
        .def("regions",             &SampleWrapper::regions)


        // RecoToTruthPairingIndex
        .def("setRecoToTruthPairingIndices",  &SampleWrapper::setRecoToTruthPairingIndices)
        .def("recoToTruthPairingIndices",     &SampleWrapper::recoToTruthPairingIndices)

        // setEventWeight
        .def("setEventWeight",      &SampleWrapper::setEventWeight)
        .def("weight",              &SampleWrapper::weight)

        // skipSystematicRegionCombination
        .def("skipSystematicRegionCombination", &SampleWrapper::skipSystematicRegionCombination)

        .def("addTruth",            &SampleWrapper::addTruth)
        .def("getTruthSharedPtrs",  &SampleWrapper::getTruthSharedPtrs)

        .def("addCustomDefine",     &SampleWrapper::addCustomDefine)
        .def("customRecoDefines",   &SampleWrapper::customRecoDefines)

        .def("customTruthDefines",  &SampleWrapper::customTruthDefines)
        .def("addCustomTruthDefine",&SampleWrapper::addCustomTruthDefine)

        .def("addVariable",         &SampleWrapper::addVariable)
        .def("variables",           &SampleWrapper::variables)

        .def("addExcludeAutomaticSystematic", &SampleWrapper::addExcludeAutomaticSystematic)
        .def("excludeAutomaticSystematics",   &SampleWrapper::excludeAutomaticSystematics)

        .def("setNominalOnly",  &SampleWrapper::setNominalOnly)
        .def("nominalOnly",     &SampleWrapper::nominalOnly)

        .def("setAutomaticSystematics",  &SampleWrapper::setAutomaticSystematics)
        .def("automaticSystematics",     &SampleWrapper::automaticSystematics)

        .def("addCutflow",          &SampleWrapper::addCutflow)
        .def("getCutflowSharedPtrs",&SampleWrapper::getCutflowSharedPtrs)
        .def("hasCutflows",         &SampleWrapper::hasCutflows)

        .def("setNominalSumWeights", &SampleWrapper::setNominalSumWeights)
        .def("nominalSumWeights",    &SampleWrapper::nominalSumWeights)
    ;

    /**
     * @brief Python wrapper around Systematic
     *
     */
    class_<SystematicWrapper>("SystematicWrapper",
        init<std::string>())

        // getPtr
        .def("getPtr",          &SystematicWrapper::getPtr)
        .def("getRawPtr",       &SystematicWrapper::getRawPtr)

        // constructFromSharedPtr
        .def("constructFromSharedPtr",  &SystematicWrapper::constructFromSharedPtr)

        // name
        .def("name",            &SystematicWrapper::name)

        // setSumWeights
        .def("setSumWeights",   &SystematicWrapper::setSumWeights)
        .def("sumWeights",      &SystematicWrapper::sumWeights)

        // setWeightSuffix
        .def("setWeightSuffix", &SystematicWrapper::setWeightSuffix)
        .def("weightSuffix",    &SystematicWrapper::weightSuffix)

        // addRegion
        .def("addRegion",       &SystematicWrapper::addRegion)
        .def("regionsNames",    &SystematicWrapper::regionsNames)

        // isNominal
        .def("isNominal",       &SystematicWrapper::isNominal)
    ;

    /**
     * @brief Python wrapper around Ntuple
     *
     */
    class_<NtupleWrapper>("NtupleWrapper",
        init<>())

        // getPtr
        .def("getPtr",          &NtupleWrapper::getPtr)

        // constructFromSharedPtr
        .def("constructFromSharedPtr",  &NtupleWrapper::constructFromSharedPtr)

        // addSample
        .def("addSample",       &NtupleWrapper::addSample)
        .def("nSamples",        &NtupleWrapper::nSamples)
        .def("sampleName",      &NtupleWrapper::sampleName)

        // setSelection
        .def("setSelection",    &NtupleWrapper::setSelection)
        .def("selection",       &NtupleWrapper::selection)

        // addBranch
        .def("addBranch",       &NtupleWrapper::addBranch)
        .def("nBranches",       &NtupleWrapper::nBranches)
        .def("branchName",      &NtupleWrapper::branchName)

        // addExcludedBranch
        .def("addExcludedBranch",   &NtupleWrapper::addExcludedBranch)
        .def("nExcludedBranches",   &NtupleWrapper::nExcludedBranches)
        .def("excludedBranchName",  &NtupleWrapper::excludedBranchName)

        // copyTree
        .def("addCopyTree",     &NtupleWrapper::addCopyTree)
        .def("copyTrees",       &NtupleWrapper::copyTrees)
    ;

    /**
     * @brief Python wrapper around Truth
     *
     */
    class_<TruthWrapper>("TruthWrapper",
        init<std::string> ())

        // getPtr
        .def("getPtr",          &TruthWrapper::getPtr)
        .def("constructFromSharedPtr", &TruthWrapper::constructFromSharedPtr)

        // name
        .def("name",            &TruthWrapper::name)

        // setTruthTreeName
        .def("setTruthTreeName",    &TruthWrapper::setTruthTreeName)
        .def("truthTreeName",       &TruthWrapper::truthTreeName)

        // setSelection
        .def("setSelection",        &TruthWrapper::setSelection)
        .def("selection",           &TruthWrapper::selection)

        // setEventWeight
        .def("setEventWeight",      &TruthWrapper::setEventWeight)
        .def("eventWeight",         &TruthWrapper::eventWeight)

        // addMatchVariables
        .def("addMatchVariables",   &TruthWrapper::addMatchVariables)
        .def("nMatchedVariables",   &TruthWrapper::nMatchedVariables)
        .def("matchedVariables",    &TruthWrapper::matchedVariables)

        // addVariable
        .def("addVariable",         &TruthWrapper::addVariable)
        .def("getVariableRawPtrs",  &TruthWrapper::getVariableRawPtrs)

        // produceUnfolding
        .def("setProduceUnfolding", &TruthWrapper::setProduceUnfolding)
        .def("produceUnfolding",    &TruthWrapper::produceUnfolding)

        .def("setMatchRecoTruth",   &TruthWrapper::setMatchRecoTruth)
        .def("matchRecoTruth",      &TruthWrapper::matchRecoTruth)

        .def("addBranch",           &TruthWrapper::addBranch)
        .def("branches",            &TruthWrapper::branches)

        .def("addExcludedBranch",   &TruthWrapper::addExcludedBranch)
        .def("excludedBranches",    &TruthWrapper::excludedBranches)
    ;


    /**
     * @brief Python wrapper around Truth
     *
     */
    class_<CutflowWrapper>("CutflowWrapper",
        init<std::string> ())

        // getPtr
        .def("getPtr",          &CutflowWrapper::getPtr)
        .def("constructFromSharedPtr", &CutflowWrapper::constructFromSharedPtr)

        // name
        .def("name",            &CutflowWrapper::name)

        // addSelection
        .def("addSelection",    &CutflowWrapper::addSelection)

        // selections
        .def("selectionsDefinition",      &CutflowWrapper::selectionsDefinition)
        .def("selectionsTitles",          &CutflowWrapper::selectionsTitles)
    ;

    /**
     * @brief Python wrapper around ConfigDefine
     *
     */
    class_<ConfigDefineWrapper>("ConfigDefineWrapper",
        init<std::string, std::string, std::string>())


        .def("getPtr",                  &ConfigDefineWrapper::getPtr)
        .def("constructFromSharedPtr",  &ConfigDefineWrapper::constructFromSharedPtr)
        .def("constructFromRawPointer", &ConfigDefineWrapper::constructFromRawPointer)


        // columnName
        .def("columnName",      &ConfigDefineWrapper::columnName)

        // formula
        .def("formula",        &ConfigDefineWrapper::formula)

        // treeName
        .def("treeName",       &ConfigDefineWrapper::treeName)
    ;

    /**
     * @brief Python wrapper SumWeightsGetter class
     *
     */
    class_<SumWeightsGetter>("SumWeightsGetter",
        init<const std::vector<std::string> &, const std::string>())

        .def("getSumWeightsNames",      &SumWeightsGetter::getSumWeightsNames)
        .def("getSumWeightsValues",     &SumWeightsGetter::getSumWeightsValues)
    ;

    /**
     * @brief Python wrapper around SimpleONNXInference class
     *
     */
    class_<SimpleONNXInferenceWrapper>("SimpleONNXInferenceWrapper",
        init<std::string>())
        // getPtr
        .def("getPtr",          &SimpleONNXInferenceWrapper::getPtr)

        // constructFromSharedPtr
        .def("constructFromSharedPtr",  &SimpleONNXInferenceWrapper::constructFromSharedPtr)

        // name
        .def("name",            &SimpleONNXInferenceWrapper::name)

        .def("setModelPaths",           &SimpleONNXInferenceWrapper::setModelPaths)
        .def("modelPaths",              &SimpleONNXInferenceWrapper::modelPaths)
        .def("setFoldFormula",          &SimpleONNXInferenceWrapper::setFoldFormula)

        .def("addInput",                &SimpleONNXInferenceWrapper::addInput)
        .def("getInputLayerNames",      &SimpleONNXInferenceWrapper::getInputLayerNames)
        .def("getInputLayerBranches",   &SimpleONNXInferenceWrapper::getInputLayerBranches)

        .def("addOutput",               &SimpleONNXInferenceWrapper::addOutput)
        .def("getOutputLayerNames",     &SimpleONNXInferenceWrapper::getOutputLayerNames)
        .def("getOutputLayerBranches",  &SimpleONNXInferenceWrapper::getOutputLayerBranches)
    ;

    /**
     * @brief Python wrapper DuplicateEventsChecker class
     *
     */
    class_<DuplicateEventChecker>("DuplicateEventChecker",
        init<const std::vector<std::string> &>())

        .def("checkDuplicateEntries", &DuplicateEventChecker::checkDuplicateEntries)
        .def("duplicateRunNumbers",   &DuplicateEventChecker::duplicateRunNumbers)
        .def("duplicateEventNumbers", &DuplicateEventChecker::duplicateEventNumbers)
    ;
}
