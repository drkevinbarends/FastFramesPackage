"""
@file Source file with BlockReaderSample class.
"""
from BlockReaderCommon import set_paths
set_paths()

from python_wrapper.python.logger import Logger

from ConfigReaderCpp    import SampleWrapper, TruthWrapper, StringVector, RegionWrapper, SystematicWrapper
from BlockReaderGeneral import BlockReaderGeneral
from BlockReaderSystematic import BlockReaderSystematic
from BlockOptionsGetter import BlockOptionsGetter
from BlockReaderSampleTruth import BlockReaderSampleTruth
from CommandLineOptions import CommandLineOptions

import re

def regex_in_string_list(regex : str, string_list : list) -> bool:
    for string in string_list:
        if re.fullmatch(regex, string):
            return True
    return False

def string_in_regex_list(string : str, regex_list : list) -> bool:
    for regex in regex_list:
        if re.match(regex, string):
            return True
    return False

def get_matched_regions(regex : str, all_regions : list) -> list:
    result = []
    for region in all_regions:
        if re.fullmatch(regex, region):
            result.append(region)
    return result

def filter_only_selected_campaigns(campaigns_config : list[str]) -> list[str]:
    """!Filter only campaigns that are selected by the user
    @param campaigns_config: list of campaigns from the config file
    @return list of campaigns that are selected by the user
    """
    campaigns_cli = CommandLineOptions().get_campaigns()
    if campaigns_cli == None:
        return campaigns_config
    result = []
    for campaign in campaigns_config:
        if campaign in campaigns_cli:
            result.append(campaign)
    return result

class BlockReaderSample:
    """!Class for reading sample block of the config, equivalent of C++ class Sample
    """
    def __init__(self, input_dict : dict, block_reader_general : BlockReaderGeneral = None):
        """!Constructor of the BlockReaderSample class
        @param input_dict: dictionary with options from the config file
        @param block_reader_general: BlockReaderGeneral object with general options from the config file - this is there to get default values
        """
        self._options_getter = BlockOptionsGetter(input_dict)

        self._name = self._options_getter.get("name", None, [str])
        if self._name is None:
            Logger.log_message("ERROR", "No name specified for sample: " + str(self._options_getter))
            exit(1)

        self._is_data = self._options_getter.get("_is_data", False, [bool])
        self._unique_samples = self._options_getter.get("_internal_unique_samples", None, [list], [tuple])

        # check if all campaigns are defined in general block
        if not self._is_data and block_reader_general != None:
            for unique_sample in self._options_getter["_internal_unique_samples"]:
                campaign = unique_sample[1]
                if not block_reader_general.cpp_class.campaignIsDefined(campaign):
                    Logger.log_message("ERROR", "Unknown campaign {} specified for sample {}. Please define luminosity for this campaign in your config.".format(campaign, self._name))
                    Logger.log_message("ERROR", "Hint: For Run 3 data, check the Data Preparation twiki: https://twiki.cern.ch/twiki/bin/view/AtlasProtected/GoodRunListsForAnalysisRun3")
                    exit(1)

        self._selection_suffix = self._options_getter.get("selection","true", [str])

        self._regions = self._options_getter.get("regions",None, [list], [str])
        self._exclude_regions = self._options_getter.get("exclude_regions",None, [list], [str])
        if not self._regions is None and not self._exclude_regions is None:
            Logger.log_message("ERROR", "Both regions and exclude_regions specified for sample {}".format(self._name))
            exit(1)

        self._systematic = []

        self._event_weights = self._options_getter.get("event_weights", block_reader_general.default_event_weights if not self._is_data else "1.", [str])
        self._weight_suffix = self._options_getter.get("weight_suffix", None, [str])

        self._reco_tree_name = self._options_getter.get("reco_tree_name", block_reader_general.default_reco_tree_name, [str])

        self._selection_suffix = self._options_getter.get("selection_suffix", "", [str])

        self._reco_to_truth_pairing_indices = self._options_getter.get("reco_to_truth_pairing_indices", block_reader_general.reco_to_truth_pairing_indices, [list], [str])

        self._define_custom_columns = self._options_getter.get("define_custom_columns", block_reader_general.define_custom_columns, [list], [dict])

        self._define_custom_columns_truth = self._options_getter.get("define_custom_columns_truth", block_reader_general.define_custom_columns_truth, [list], [dict])

        self._exclude_systematics = self._options_getter.get("exclude_systematics", block_reader_general.default_exclude_systematics, [list], [str])

        self._automatic_systematics = self._options_getter.get("automatic_systematics", block_reader_general.automatic_systematics, [bool])

        self._nominal_only = self._options_getter.get("nominal_only", block_reader_general.nominal_only, [bool])

        if self._automatic_systematics and self._nominal_only:
            Logger.log_message("ERROR", "Both automatic_systematics and nominal_only specified for sample {}. Only one of these options can be True".format(self._name))
            exit(1)

        self._sum_weights = self._options_getter.get("sum_weights", block_reader_general.default_sumweights, [str])

        ## Instance of the SampleWrapper C++ class -> wrapper around C++ Sample class
        self.cpp_class = SampleWrapper(self._name)

        self._truth_dicts = self._options_getter.get("truth", None, [list], [dict])
        self._truths = []
        if self._truth_dicts is not None:
            reco_variables_from_regions = block_reader_general.cpp_class.getVariableNames()
            for truth_dict in self._truth_dicts:
                truth_object = BlockReaderSampleTruth(truth_dict)
                self._truths.append(truth_object)
                truth_object.check_reco_variables_existence(reco_variables_from_regions)
                self.cpp_class.addTruth(truth_object.cpp_class.getPtr())


        self.variables          = self._options_getter.get("variables", None, [list], [str])
        self.exclude_variables  = self._options_getter.get("exclude_variables", None, [list], [str])
        if self.variables is not None and self.exclude_variables is not None:
            Logger.log_message("ERROR", "Both variables and exclude_variables specified for sample {}".format(self._name))
            exit(1)


        self._set_unique_samples_IDs()

        self._set_cpp_class()

        self._check_unused_options()


    def _set_unique_samples_IDs(self):
        """!Set unique sample IDs for the sample. If no dsids are specified, take all dsids.
        """
        for unique_sample in self._unique_samples:
            self.cpp_class.addUniqueSampleID(*unique_sample)

    def adjust_regions(self, regions : dict) -> None:
        """!Set regions for the sample. If no regions are specified, take all regions, if exclude_regions are specified, remove them from the list of regions.
        @param regions: dictionary with all regions (keys are region names, values are BlockReaderRegion objects)
        """
        if self._exclude_regions is not None:
            for region_name in self._exclude_regions:
                if not regex_in_string_list(region_name, regions):
                    Logger.log_message("ERROR", "Region {} specified for sample {} does not exist".format(region_name, self._name))
                    exit(1)

        if self._regions is None: # if no regions are specified, take all regions
            self._regions = []
            for region_name in regions:
                if self._exclude_regions is not None and string_in_regex_list(region_name, self._exclude_regions):
                    continue
                self._regions.append(region_name)
        else:   # if regions are specified, check if they exist
            selected_regions = []
            for region in self._regions:
                matched_regions = get_matched_regions(region, regions)
                if len(matched_regions) == 0:
                    Logger.log_message("ERROR", "Region {} specified for sample {} does not exist".format(region, self._name))
                    exit(1)
                selected_regions.extend(matched_regions)
            self._regions = selected_regions


        for region_name in self._regions:
            region_object = regions[region_name]
            self.cpp_class.addRegion(region_object.cpp_class.getPtr())

    def adjust_cutflow_regions(self, regions : dict, included_regions : [str]) -> None:
        """!Set cutflow-regions for the sample. If no regions are specified, take all regions, if include_regions is specified, only add the regions that are in the list.
        @param regions: dictionary with all regions (keys are region names, values are BlockReaderRegion objects)
        """
        if included_regions is None:
            for region_name in regions:
                self._regions.append(region_name)
                region_object = regions[region_name]
                self.cpp_class.addRegion(region_object.cpp_class.getPtr())

        else :
            if self._name not in included_regions:
                return
            else :
                for region_name in regions:
                    self._regions.append(region_name)
                    region_object = regions[region_name]
                    self.cpp_class.addRegion(region_object.cpp_class.getPtr())

    def adjust_systematics(self, systematics_all : dict) -> None:
        """!Set systematics for the sample. For each systematics check if it has explicit list of samples (if not, add all). If sample_exclude is defined for it, check if this sample is not there.
        @param systematics_all: dictionary with all systematics (keys are systematic names, values are BlockReaderSystematic objects)
        """
        self._systematic = []
        for systematic_name, systematic in systematics_all.items():
            # check if systematics has explicit list of samples. If so, does it contain this sample?
            if systematic.samples is not None:
                if self._name not in systematic.samples:
                    continue

            # check if systematics has explicit list of exclude_samples. If so, does it contain this sample?
            if systematic.exclude_samples is not None:
                if self._name in systematic.exclude_samples:
                    continue

            # for data samples, we do not want to add systematics by default (other than nominal)
            if systematic.samples is None and (self._is_data or self._nominal_only) and not systematic.cpp_class.isNominal():
                continue

            self.cpp_class.addSystematic(systematic.cpp_class.getPtr())
            self._systematic.append(systematic_name)

    def resolve_variables(self) -> None:
        variables_defined_for_sample = []
        regions_ptrs = self.cpp_class.regions()
        for region_ptr in regions_ptrs:
            region_object = RegionWrapper("")
            region_object.constructFromSharedPtr(region_ptr)
            variables_in_region = region_object.getVariableNames()
            for variable in variables_in_region:
                if variable not in variables_defined_for_sample:
                    variables_defined_for_sample.append(variable)

        variables_to_keep = []

        if self.variables is None and self.exclude_variables is None:
            variables_to_keep = variables_defined_for_sample
        elif self.variables is not None:
            for variable in variables_defined_for_sample:
                if variable in self.variables:
                    variables_to_keep.append(variable)
                elif string_in_regex_list(variable, self.variables):
                    variables_to_keep.append(variable)
        elif self.exclude_variables is not None:
            for variable in variables_defined_for_sample:
                if variable not in self.exclude_variables and not string_in_regex_list(variable, self.exclude_variables):
                    variables_to_keep.append(variable)

        for variable in variables_to_keep:
            self.cpp_class.addVariable(variable)


    def _set_cpp_class(self) -> None:
        """!Set the cpp class for the sample.
        """
        total_weight = self._event_weights
        if self._weight_suffix is not None:
            total_weight =  "(" + total_weight + ")*(" + self._weight_suffix + ")"
        self.cpp_class.setEventWeight(total_weight)

        self.cpp_class.setRecoTreeName(self._reco_tree_name)
        self.cpp_class.setSelectionSuffix(self._selection_suffix)

        vector_pairing_indices = StringVector()
        if (self._reco_to_truth_pairing_indices != ["runNumber", "eventNumber"]):
            Logger.log_message("ERROR", "You have used non-default set of reco-to-truth pairing indices. Due to a bug in ROOT: https://github.com/root-project/root/issues/7713 this is currently not supported")
            exit(1)
        for reco_to_truth_pairing_index in self._reco_to_truth_pairing_indices:
            vector_pairing_indices.append(reco_to_truth_pairing_index)
        self.cpp_class.setRecoToTruthPairingIndices(vector_pairing_indices)

        if self._define_custom_columns:
            for custom_column_dict in self._define_custom_columns:
                custom_column_opts = BlockOptionsGetter(custom_column_dict)
                name        = custom_column_opts.get("name", None, [str])
                definition  = custom_column_opts.get("definition", None, [str])
                if name is None or definition is None:
                    Logger.log_message("ERROR", "Invalid custom column definition for sample {}".format(self._name))
                    exit(1)
                self.cpp_class.addCustomDefine(name, definition)
                if custom_column_opts.get_unused_options():
                    Logger.log_message("ERROR", "Invalid custom column definition for sample {}".format(self._name))
                    exit(1)

        if self._define_custom_columns_truth and self._truths:
            for custom_column_dict in self._define_custom_columns_truth:
                custom_column_opts = BlockOptionsGetter(custom_column_dict)
                name        = custom_column_opts.get("name", None, [str])
                definition  = custom_column_opts.get("definition", None, [str])
                truth_tree  = custom_column_opts.get("truth_tree", None, [str])
                if name is None or definition is None or truth_tree is None:
                    Logger.log_message("ERROR", "Invalid custom truth column definition for sample {}".format(self._name))
                    exit(1)
                self.cpp_class.addCustomTruthDefine(name, definition, truth_tree)

                if custom_column_opts.get_unused_options():
                    Logger.log_message("ERROR", "Invalid custom truth column definition for sample {}".format(self._name))
                    exit(1)

        for syst_regex in self._exclude_systematics:
            self.cpp_class.addExcludeAutomaticSystematic(syst_regex)


        self.cpp_class.setAutomaticSystematics(self._automatic_systematics)
        self.cpp_class.setNominalOnly(self._nominal_only)

        self.cpp_class.setNominalSumWeights(self._sum_weights)

    def _check_unused_options(self):
        unused = self._options_getter.get_unused_options()
        if len(unused) > 0:
            Logger.log_message("ERROR", "Key {} used in  sample block is not supported!".format(unused))
            exit(1)

    def get_truth_cpp_objects(vector_shared_ptr) -> list:
        """!Get list of truth cpp objects defined in the sample
        """
        result = []
        for ptr in vector_shared_ptr:
            truth_cpp_object = TruthWrapper("")
            truth_cpp_object.constructFromSharedPtr(ptr)
            result.append(truth_cpp_object)
        return result

    def is_data_sample(sample_cpp_object) -> bool:
        """!Check if the sample is data sample
        """
        n_unique_ids = sample_cpp_object.nUniqueSampleIDs()
        for i in range(n_unique_ids):
            id_string = sample_cpp_object.uniqueSampleIDstring(i)[1:-1]
            sample_type = id_string.split(",")[2].strip()
            if sample_type.lower() != "data":
                return False
        return True

    def get_unique_sample_list(sample_cpp_object) -> list:
        """!Get unique sample ID as a list of strings
        """
        id_list = []
        n_unique_ids = sample_cpp_object.nUniqueSampleIDs()
        for i in range(n_unique_ids):
            id_string = sample_cpp_object.uniqueSampleIDstring(i)
            id_list.append(id_string)
        return id_list

    def get_total_luminosity(general_block_cpp_object, samples : list[SampleWrapper]) -> float:
        """! Loop over all MC samples, indentify all MC campaign and sum up their luminosity
        """
        result = 0
        campaigns = []
        for sample in samples:
            if BlockReaderSample.is_data_sample(sample):
                continue
            n_unique_samples = sample.nUniqueSampleIDs()
            for i_unique_id in range(n_unique_samples):
                unique_sample_string = sample.uniqueSampleIDstring(i_unique_id).split(",")
                campaign = unique_sample_string[1].strip()
                if campaign not in campaigns:
                    campaigns.append(campaign)
                    result += general_block_cpp_object.getLuminosity(campaign)
        return result

    def get_systematics_objects(cpp_class) -> list:
        """!Get list of systematics cpp objects
        @return list of systematics
        """
        result = []
        vector_systematics = cpp_class.getSystematicsSharedPtr()
        for systematic_ptr in vector_systematics:
            systematic_cpp_object = SystematicWrapper("")
            systematic_cpp_object.constructFromSharedPtr(systematic_ptr)
            result.append(systematic_cpp_object)
        return result

    def resolve_unique_samples(samples : list[dict]) -> None:
        """!Add _internal_unique_samples to each sample. This is a list of tuples (dsid, campaign, simulation_type) that uniquely identifies the unique sample. It also resolves included_samples.
        @param samples: list of samples
        """
        sample_dict = {}
        for sample in samples:
            if "name" not in sample:
                Logger.log_message("ERROR", "No name specified for sample: " + str(sample))
                exit(1)
            sample_dict[sample["name"]] = sample

            def check_duplicate_option(option : str, sample : dict) -> None:
                if option in sample:
                    Logger.log_message("ERROR", "Both included_samples and {} specified for sample {}".format(option, sample["name"]))
                    exit(1)

            # check if included_samples is not used with other options
            if "included_samples" in sample:
                check_duplicate_option("simulation_type", sample)
                check_duplicate_option("campaigns", sample)
                check_duplicate_option("dsids", sample)

        def check_if_option_is_defined(option : str, sample : dict) -> None:
            if option not in sample:
                Logger.log_message("ERROR", "No {} specified for sample {}".format(option, sample["name"]))
                exit(1)

        for sample in samples:
            if "included_samples" in sample:
                continue
            check_if_option_is_defined("simulation_type", sample)
            check_if_option_is_defined("campaigns", sample)
            if sample["simulation_type"].upper() != "DATA":
                check_if_option_is_defined("dsids", sample)
                dsids = sample["dsids"]
                duplicates = set([x for x in dsids if dsids.count(x) > 1])
                if len(duplicates) > 0:
                    Logger.log_message("ERROR", "Duplicate dsids {} specified for sample {}".format(duplicates, sample["name"]))
                    exit(1)
                sample["_is_data"] = False
            else:
                dsids = [0]
                sample["_is_data"] = True
            unique_samples = []
            selected_campaigns = filter_only_selected_campaigns(sample["campaigns"])
            for campaign in selected_campaigns:
                for dsid in dsids:
                    unique_samples.append((dsid, campaign, sample["simulation_type"]))
            sample["_internal_unique_samples"] = unique_samples
            del sample["simulation_type"]
            del sample["campaigns"]
            if "dsids" in sample:
                del sample["dsids"]

        for sample in samples:
            if not "included_samples" in sample:
                continue
            unique_samples_merged = []
            n_included_data_samples = 0
            for included_sample_name in sample["included_samples"]:
                if included_sample_name not in sample_dict:
                    Logger.log_message("ERROR", "Included sample {} does not exist".format(included_sample_name))
                    exit(1)
                included_sample = sample_dict[included_sample_name]
                unique_samples = included_sample["_internal_unique_samples"]
                if (included_sample["_is_data"]):
                    n_included_data_samples += 1
                for unique_sample in unique_samples:
                    unique_samples_merged.append(unique_sample)
            del sample["included_samples"]
            if n_included_data_samples != 0 and n_included_data_samples != len(sample["included_samples"]):
                Logger.log_message("ERROR", "You cannot mix data and MC in one sample. Sample name " + sample["name"])
                exit(1)
            sample["_is_data"] = n_included_data_samples != 0
            sample["_internal_unique_samples"] = unique_samples_merged

            # check for duplicate unique samples
            unique_samples_set = set()
            for unique_sample in unique_samples_merged:
                if unique_sample in unique_samples_set:
                    Logger.log_message("ERROR", "Duplicate unique sample {} in sample {}".format(unique_sample, sample["name"]))
                    exit(1)
                unique_samples_set.add(unique_sample)
