"""
@file Main config reader source file.
"""
import yaml
from sys import argv
import argparse
from copy import deepcopy

from BlockReaderGeneral import BlockReaderGeneral, vector_to_list
from BlockReaderRegion import BlockReaderRegion
from BlockReaderSample import BlockReaderSample
from BlockReaderNtuple import BlockReaderNtuple
from BlockOptionsGetter import BlockOptionsGetter
from BlockReaderSystematic import BlockReaderSystematic, read_systematics_variations
from CommandLineOptions import CommandLineOptions
from AutomaticRangeGenerator import AutomaticRangeGenerator
from BlockReaderCutflow import BlockReaderCutflow
from BlockReaderSimpleONNXInference import BlockReaderSimpleONNXInference

from python_wrapper.python.logger import Logger
from ConfigReaderCpp import VariableWrapper, ConfigDefineWrapper

class ConfigReader:
    """!Main class for reading the config file and connecting all its blocks.

    Reads the config file performs basic checks, create lists of the corresponding objects based on the objects in the config file, set all the necessary properties of the objects and then
    it propagates all the information to the C++ classes through BlockReaderGeneral class.
    """
    def __init__(self, config_file_path : str):
        with open(config_file_path, "r") as f:
            data_full = yaml.load(f, Loader=yaml.FullLoader)
            data = dict([(x, y) for x, y in data_full.items() if not x.startswith('.')])
            self.block_getter = BlockOptionsGetter(data)

            self.block_general = BlockReaderGeneral(self.block_getter.get("general"))

            self.regions = {}
            for region_dict in self.block_getter.get("regions"):
                region = BlockReaderRegion(region_dict, self.block_general)
                region_name = region.cpp_class.name()
                if region_name in self.regions:
                    Logger.log_message("ERROR", "Duplicate region name: {}".format(region_name))
                    exit(1)
                self.regions[region_name] = region
                self.block_general.add_region(region)

            self.samples = {}
            sample_blocks = self.block_getter.get("samples")
            sample_blocks = AutomaticRangeGenerator.unroll_sequence(sample_blocks)
            CommandLineOptions().check_samples_existence(sample_blocks)
            CommandLineOptions().keep_only_selected_samples(sample_blocks)
            BlockReaderSample.resolve_unique_samples(sample_blocks)
            for sample_dict in sample_blocks:
                sample = BlockReaderSample(sample_dict, self.block_general)
                sample.adjust_regions(self.regions)
                sample_name = sample.cpp_class.name()
                if sample_name in self.samples:
                    Logger.log_message("ERROR", "Duplicate sample name: {}".format(sample_name))
                    exit(1)
                self.samples[sample_name] = sample

            self.cutflows = []
            all_cutflow_regions = {}
            cutflows_config_blocks = self.block_getter.get("cutflows", [])
            for cutflow_dict in cutflows_config_blocks:
                cutflow = BlockReaderCutflow(cutflow_dict)
                cutflow.adjust_samples(self.samples)
                self.cutflows.append(cutflow)

                # Do the cutflow extend part if needed
                if cutflow.has_variables_to_track():
                    # Create new region dictionaries
                    # Cutflow regions are restricted to:
                    # - samples where the cutflow is defined
                    # - nominal only
                    cutflow_regions_dics = cutflow.build_MiniAnalysis_regions(self.block_getter.get("regions"))
                    cutflow_regions = {}
                    for region_dict in cutflow_regions_dics:
                        region = BlockReaderRegion(region_dict, self.block_general)
                        region_name = region.cpp_class.name()
                        # Check if the region name is not already used
                        if region_name in self.regions:
                            Logger.log_message("ERROR", "Duplicate region name coming from cutflow: {}".format(region_name))
                            exit(1)

                        # Add to the block regions
                        self.block_general.add_region(region)

                        # Add to the cutflow regions
                        cutflow_regions[region_name] = region
                        all_cutflow_regions[region_name] = region

                    # Add the regions only to the samples that are used in the cutflow
                    for sample_name, sample_object in self.samples.items():
                        sample_object.adjust_cutflow_regions(cutflow_regions, cutflow._sample_names)

            systematic_blocks_from_config = self.block_getter.get("systematics", [])
            systematic_blocks_from_config = AutomaticRangeGenerator.unroll_sequence(systematic_blocks_from_config)
            for sample_name,sample in self.samples.items():
                systematics = {}
                # nominal
                sum_weights = sample.cpp_class.nominalSumWeights()
                nominal_dict = {"variation": {"up": "NOSYS", "sum_weights_up" : sum_weights}, "samples": [sample_name]}
                nominal = BlockReaderSystematic(nominal_dict, "up", self.block_general)
                nominal.adjust_regions(self.regions | all_cutflow_regions) # The cutflow regions are only added to the nominal systematic.
                systematics[nominal.cpp_class.name()] = nominal

                self.systematics_dicts = [] # for creation of TRExFitter config
                for systematic_dict in systematic_blocks_from_config:
                    systematics_dict_modified = deepcopy(systematic_dict) # we have to adjust default sum of weights
                    def modify_sum_weights_for_variation(syst_dict : dict, variation_name : str, default_sum_weights : str) -> None:
                        if not variation_name in syst_dict["variation"]:
                            return
                        if "sum_weights_" + variation_name in syst_dict["variation"]:
                            return
                        syst_dict["variation"]["sum_weights_" + variation_name] = default_sum_weights
                    modify_sum_weights_for_variation(systematics_dict_modified, "up", sum_weights)
                    modify_sum_weights_for_variation(systematics_dict_modified, "down", sum_weights)

                    systematic_list = read_systematics_variations(systematics_dict_modified, self.block_general, self.systematics_dicts)
                    for systematic in systematic_list:
                        systematic.adjust_regions(self.regions)
                        systematic_name = systematic.cpp_class.name()
                        if systematic_name in systematics:
                            Logger.log_message("ERROR", "Duplicate systematic name: {}".format(systematic_name))
                            exit(1)
                        systematics[systematic_name] = systematic

                for systematic_name,systematic in systematics.items():
                    systematic.check_samples_existence(self.samples)
                    if systematic_name == 'NOSYS':
                        systematic.check_regions_existence(self.regions | all_cutflow_regions)
                    else:
                        systematic.check_regions_existence(self.regions)
                    self.block_general.cpp_class.addSystematic(systematic.cpp_class.getPtr())

                Logger.log_message("DEBUG", "Sample {} has {} regions".format(sample_name, len(sample.cpp_class.regionsNames())))
                sample.adjust_systematics(systematics)
                sample.resolve_variables()
                self.block_general.cpp_class.addSample(sample.cpp_class.getPtr())

            self.block_ntuple = BlockReaderNtuple([])
            self.has_ntuple_block = "ntuples" in self.block_getter
            if self.has_ntuple_block:
                self.block_ntuple = BlockReaderNtuple(self.block_getter.get("ntuples"))
                self.block_ntuple.adjust_regions(self.regions)
                self.block_ntuple.adjust_samples(self.samples)
            self.block_general.cpp_class.setNtuple(self.block_ntuple.cpp_class.getPtr())

            self.simple_onnx_inferences = {}
            self.has_simple_onnx_inference_block = "simple_onnx_inference" in self.block_getter
            if self.has_simple_onnx_inference_block:
                for simple_onnx_inference_dict in self.block_getter.get("simple_onnx_inference"):
                    inference = BlockReaderSimpleONNXInference(simple_onnx_inference_dict)
                    inferernce_name = inference.cpp_class.name()
                    if inferernce_name in self.simple_onnx_inferences:
                        Logger.log_message("ERROR", "Duplicate ONNX model name: {}".format(inferernce_name))
                        exit(1)
                    self.simple_onnx_inferences[inferernce_name] = inference
                    self.block_general.cpp_class.addSimpleONNXInference(inference.cpp_class.getPtr())

            unused_blocks = self.block_getter.get_unused_options()
            if unused_blocks:
                Logger.log_message("ERROR", "Unused blocks: {}".format(unused_blocks))
                exit(1)

if __name__ == "__main__":
    config_path = CommandLineOptions().get_config_path()

    config_reader = ConfigReader(config_path)
    block_general = config_reader.block_general

    print("\nGeneral block:")
    print("\tinputSumWeightsPath: ", block_general.cpp_class.inputSumWeightsPath())
    print("\toutputPathHistograms: ", block_general.cpp_class.outputPathHistograms())
    print("\toutputPathNtuples: ", block_general.cpp_class.outputPathNtuples())
    print("\tinputFilelistPath: ", block_general.cpp_class.inputFilelistPath())
    print("\tnumCPU: ", block_general.cpp_class.numCPU())
    print("\tcustomFrameName: ", block_general.cpp_class.customFrameName())
    print("\tmin_event: ", block_general.cpp_class.minEvent())
    print("\tmax_event: ", block_general.cpp_class.maxEvent())
    print("\t--split_n_jobs: ", block_general.cpp_class.totalJobSplits())
    print("\t--job_index: ", block_general.cpp_class.currentJobIndex())
    print("\txSectionFiles: ",  block_general.get_xsection_files())
    print("\tcap_acceptance_selection:", block_general.cpp_class.capAcceptanceSelection())
    print("\tconfig_define_after_custom_class:", block_general.cpp_class.configDefineAfterCustomClass())
    print("\tuse_region_subfolders:", block_general.cpp_class.useRegionSubfolders())
    print("\tlist_of_systematics_name:", block_general.cpp_class.listOfSystematicsName())
    print("\tntuple_compression_level:", block_general.cpp_class.ntupleCompressionLevel())
    print("\tntuple_auto_flush:", block_general.cpp_class.ntupleAutoFlush())
    print("\tsplit_processing_per_unique_samples:", block_general.cpp_class.splitProcessingPerUniqueSample())
    print("\tconvert_vector_to_rvec: ", block_general.cpp_class.convertVectorToRVec())
    print("\tluminosity, mc20a: ", block_general.cpp_class.getLuminosity("mc20a"))
    print("\tluminosity, mc20d: ", block_general.cpp_class.getLuminosity("mc20d"))

    print("\tcreate_tlorentz_vectors_for:")
    tlorentz_vectors = block_general.cpp_class.tLorentzVectors()
    for tlorentz_vector in tlorentz_vectors:
        print("\t\t", tlorentz_vector)

    print("\tuse_rvec", block_general.cpp_class.useRVec())

    print("custom_options:")
    custom_option_keys = block_general.get_list_of_custom_keys()
    for custom_option_key in custom_option_keys:
        print("\t", custom_option_key, ": ", block_general.cpp_class.getCustomOption(custom_option_key))

    onnx_interfaces = block_general.get_onnx_inferences()
    if len(onnx_interfaces) > 0:
        print("\nONNX interfaces:")
        for onnx_interface in onnx_interfaces:
            print("\tname: ", onnx_interface.name())
            print("\tmodel_paths: ")
            for model_path in onnx_interface.modelPaths():
                print("\t\t", model_path)

            # print input layers
            print("\tInput layers: (name, [branches]) ")
            for input_layer_name in onnx_interface.getInputLayerNames():
                print("\t\t" + input_layer_name, [x for x in onnx_interface.getInputLayerBranches(input_layer_name)])

            # print output layers
            print("\tOutput layers: (name, [branches]) ")
            for output_layer_name in onnx_interface.getOutputLayerNames():
                print("\t\t" + output_layer_name, [x for x in onnx_interface.getOutputLayerBranches(output_layer_name)])

            print("\n")

    ntuple_cpp_object = block_general.get_ntuple_object()
    if ntuple_cpp_object:
        print("\nNtuple block:")
        print("\tselection: ", ntuple_cpp_object.selection())
        n_samples = ntuple_cpp_object.nSamples()
        samples = [ntuple_cpp_object.sampleName(i_sample) for i_sample in range(n_samples)]
        print("\tsamples: [", ",".join(samples), "]")

        n_branches = ntuple_cpp_object.nBranches()
        branches = [ntuple_cpp_object.branchName(i_branch) for i_branch in range(n_branches)]
        print("\tbranches: [", ",".join(branches), "]")

        n_excluded_branches = ntuple_cpp_object.nExcludedBranches()
        excluded_branches = [ntuple_cpp_object.excludedBranchName(i_branch) for i_branch in range(n_excluded_branches)]
        print("\texcluded_branches: [", ",".join(excluded_branches), "]")

        print("\tcopy_trees: ", config_reader.block_ntuple.get_copy_trees())


    print("\n\nRegions block:\n")
    regions = config_reader.block_general.get_regions_cpp_objects()
    for region in regions:
        print("\tname: ", region.name())
        print("\tselection: ", region.selection())
        print("\tvariables:")
        variable_cpp_objects = BlockReaderRegion.get_variable_cpp_objects(region.getVariableRawPtrs())
        for variable_cpp_object in variable_cpp_objects:
            print("\t\tname: ", variable_cpp_object.name())
            print("\t\ttitle: ", variable_cpp_object.title())
            print("\t\ttype: ", variable_cpp_object.type())
            print("\t\tdefinition: ", variable_cpp_object.definition())
            print("\t\tis_nominal_only: ", variable_cpp_object.isNominalOnly())
            if variable_cpp_object.hasRegularBinning():
                print(  "\t\tbinning: ",
                        variable_cpp_object.axisNbins(), ", ",
                        variable_cpp_object.axisMin(), ", ",
                        variable_cpp_object.axisMax())
            else:
                print("\t\tbinning: ", variable_cpp_object.binEdgesString())
            print("\n")
        variable_combinations = BlockReaderRegion.get_2d_combinations(region.variableCombinations())
        if len(variable_combinations) > 0:
            print("\t2d combinations:")
            for variable_combination in variable_combinations:
                print("\t\t", variable_combination)
            print("\n")

        variable_combinations_3d = BlockReaderRegion.get_3d_combinations(region.variableCombinations3D())
        if len(variable_combinations_3d) > 0:
            print("\t3d combinations:")
            for variable_combination in variable_combinations_3d:
                print("\t\t", variable_combination)
            print("\n")

    print("\n\nSamples block:\n")
    samples = config_reader.block_general.get_samples_objects()
    for sample in samples:
        print("\tname: ", sample.name())
        print("\tregions: ", vector_to_list(sample.regionsNames()))
        print("\tweight: ", sample.weight())
        print("\tsystematic: ", vector_to_list(sample.systematicsNames()))
        print("\tselection_suffix: \"" + sample.selectionSuffix() + "\"")
        print("\treco_to_truth_pairing_indices: ", vector_to_list(sample.recoToTruthPairingIndices()))
        print("\tautomaticSystematics: ", sample.automaticSystematics())
        print("\tnominalOnly: ", sample.nominalOnly())
        has_cutflows = sample.hasCutflows()
        print("\thasCutflows: ", has_cutflows)
        if has_cutflows:
            cutflows = sample.getCutflowSharedPtrs()
            print("\tcutflows:")
            for cutflow in cutflows:
                BlockReaderCutflow.print_cutflow(cutflow, config_reader.cutflows, "\t\t")
        print("\tvariables:")
        variable_names = vector_to_list(sample.variables())
        for variable_name in variable_names:
            print("\t\t", variable_name)
        print("\tUnique samples:")
        n_unique_samples = sample.nUniqueSampleIDs()
        for i_unique_id in range(n_unique_samples):
            print("\t\t", sample.uniqueSampleIDstring(i_unique_id))
        truth_objects = BlockReaderSample.get_truth_cpp_objects(sample.getTruthSharedPtrs())
        if len(truth_objects) > 0:
            print("\tTruth objects:")
            for cpp_truth_object in truth_objects:
                print("\t\tname: ", cpp_truth_object.name())
                print("\t\tproduce_unfolding: ", cpp_truth_object.produceUnfolding())
                print("\t\ttruth_tree_name: ", cpp_truth_object.truthTreeName())
                print("\t\tselection: ", cpp_truth_object.selection())
                print("\t\tevent_weight: ", cpp_truth_object.eventWeight())
                print("\t\tpair_reco_and_truth_trees: ", cpp_truth_object.matchRecoTruth())
                print("\t\tbranches: ", vector_to_list(cpp_truth_object.branches()))
                print("\t\texcluded_branches: ", vector_to_list(cpp_truth_object.excludedBranches()))
                print("\t\tmatch_variables:")
                n_matched_variables = cpp_truth_object.nMatchedVariables()
                for i_match_variable in range(n_matched_variables):
                    print("\t\t\t", cpp_truth_object.matchedVariables(i_match_variable))
                variable_raw_ptrs = cpp_truth_object.getVariableRawPtrs()
                print("\t\tvariables:")
                for variable_ptr in variable_raw_ptrs:
                    variable = VariableWrapper("")
                    variable.constructFromRawPtr(variable_ptr)
                    print("\t\t\tname: ", variable.name())
                    print("\t\t\ttitle: ", variable.title())
                    print("\t\t\ttype: ", variable.type())
                    print("\t\t\tdefinition: ", variable.definition())
                    if variable.hasRegularBinning():
                        print(  "\t\t\tbinning: ",
                                variable.axisNbins(), ", ",
                                variable.axisMin(), ", ",
                                variable.axisMax())
                    else:
                        print("\t\t\tbinning: ", variable.binEdgesString())
        custom_defines_reco = vector_to_list(sample.customRecoDefines())
        if len(custom_defines_reco) > 0:
            print("\tCustom defines:")
            custom_define_cpp = ConfigDefineWrapper("","","")
            for custom_define_shared_ptr in custom_defines_reco:
                custom_define_cpp.constructFromSharedPtr(custom_define_shared_ptr)
                print("\t\t \"" + custom_define_cpp.columnName() + "\" -> \"" + custom_define_cpp.formula() + "\"")

        custom_defines_truth = vector_to_list(sample.customTruthDefines())
        if len(custom_defines_truth) > 0:
            print("\tCustom defines truth:")
            custom_define_cpp = ConfigDefineWrapper("","","")
            for custom_define_shared_ptr in custom_defines_truth:
                custom_define_cpp.constructFromSharedPtr(custom_define_shared_ptr)
                print("\t\t- name:", custom_define_cpp.columnName())
                print("\t\t  definition:", custom_define_cpp.formula())
                print("\t\t  truth_tree:", custom_define_cpp.treeName())

        excluded_systematics = vector_to_list(sample.excludeAutomaticSystematics())
        if len(excluded_systematics) > 0:
            print("\tExcluded systematics:")
            for excluded_systematic in excluded_systematics:
                print("\t\t", excluded_systematic)

        systematics = BlockReaderSample.get_systematics_objects(sample)
        print("\tSystematic uncertainties defined for this sample:\n")
        for systematic in systematics:
            print("\t\tname: ", systematic.name())
            print("\t\tregions: ", vector_to_list(systematic.regionsNames()))
            print("\t\tweight_suffix: ", systematic.weightSuffix())
            print("\t\tsum_weights: ", systematic.sumWeights())
            print("\n")

        print("\n\n\n")

