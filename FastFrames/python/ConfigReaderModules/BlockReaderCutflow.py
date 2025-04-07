"""
@file Source file with BlockReaderCutflow class.
"""
from BlockReaderCommon import set_paths
set_paths()

from ConfigReaderCpp import CutflowWrapper

from BlockReaderSample import BlockReaderSample
from BlockOptionsGetter import BlockOptionsGetter
from python_wrapper.python.logger import Logger
from CommandLineOptions import CommandLineOptions


class BlockReaderCutflow:
    """!Class for reading Cutflow block of the config, equivalent of C++ class Cutflow
    """

    class CutInCutflow:
        """ This class holds the information relevant to MiniAnalysis for each cut in a cutflow.
        """
        def __init__(self, associated_variable : str, variables_to_exclude : list[str], cut_title : str, cut_expression : str):
            self.associated_variable = associated_variable
            self.variables_to_exclude = variables_to_exclude
            self.cut_expression = '(' + cut_expression + ')' # Add parenthesis to the cut expression to avoid problems with the '&&' operator in composed selections.
            self.cut_title = cut_title

    def __init__(self, input_dict : dict):
        """!Constructor of the BlockReaderCutflow class
        @param input_dict: dictionary with options from the config file
        """
        self._options_getter = BlockOptionsGetter(input_dict)

        self._name = self._options_getter.get("name", None)
        if self._name is None:
            Logger.log_message("ERROR", "Name not specified in cutflow block")
            exit(1)

        self._selections_list = self._options_getter.get("selections", None, [list], [dict])
        if self._selections_list is None:
            Logger.log_message("ERROR", "No selections specified in cutflow block " + self._name)
            exit(1)

        self._variables_to_track = self._options_getter.get("variables_to_track", None, [list])
        if self._variables_to_track is None:
            Logger.log_message("INFO", "No variables to track in cutflow named " + self._name + " doing simple cutflow histogram.")

        # Do the cutflow extend part if needed
        if self._variables_to_track is not None:
            self._cuts_vector = []

            for selection_tuple in self._selections_list:
                options_getter = BlockOptionsGetter(selection_tuple)
                title = options_getter.get("title", None, [str])
                selection = options_getter.get("selection", None, [str])

                associated_variable = options_getter.get("associated_var", None, [str])
                if associated_variable is None:
                    Logger.log_message("INFO", "No variable associated with cut named " + title + " in cutflow block " + self._name)
                    associated_variable = ""

                variables_to_exclude = options_getter.get("variables_to_exclude",[], [list])
                if variables_to_exclude is None:
                    Logger.log_message("INFO", "Tracking all variables for cut named "+title+" in cutflow block: " + self._name)
                else:
                    for excluded_variable in variables_to_exclude:
                        Logger.log_message("INFO", "Excluding variable " + str(excluded_variable) + " for cut named " + title + " in cutflow block " + self._name)

                self._cuts_vector.append(BlockReaderCutflow.CutInCutflow(associated_variable, variables_to_exclude, title, selection))

        self._sample_names = self._options_getter.get("samples", None, [list], [str])
        CommandLineOptions().keep_only_selected_samples(self._sample_names)

        unused_options = self._options_getter.get_unused_options()
        if len(unused_options) > 0:
            Logger.log_message("ERROR", "Unused options in cutflow block: " + str(unused_options))
            exit(1)

        self.cpp_class = CutflowWrapper(self._name)

        self.__set_config_reader_cpp()

    def has_variables_to_track(self) -> bool:
        return self._variables_to_track is not None

    def build_MiniAnalysis_sequential_regions(self, relevant_variables : list[dict]) -> dict:
        regions_to_add = []

        region_name = self._name
        selection = '( '

        # Go cut by cut adding selections sequentially.
        for cut in self._cuts_vector:
            region_name += "_" + cut.cut_title
            selection += cut.cut_expression # Add each cut everytime.
            variables_to_add = []
            for var_to_track in self._variables_to_track:
                if var_to_track in cut.variables_to_exclude: # Drop excluded variables.
                    continue
                else:
                    for var in relevant_variables:
                        if var["name"] == var_to_track:
                            variables_to_add.append(var)

            # Build the region dictionary
            region = {}
            region['name'] = region_name
            region['selection'] = selection + ' )'
            region['variables'] = variables_to_add

            # Add the region to the list
            regions_to_add.append(region)

            # Postprocessing to add the '&&' between cuts
            selection += ' && '

        return regions_to_add

    def build_MiniAnalysis_cut_agnostic_regions(self, relevant_variables : list[dict]) -> dict:
        regions_to_add = []
        region_name_base = self._name

        # Build a vector with all the cuts. We will drop one cut at a time to build each cut-agnostic region.
        cuts_expressions_vector = [cut.cut_expression for cut in self._cuts_vector]

        # Go cut by cut adding the agonistic regions
        for cut in self._cuts_vector:
            if cut.associated_variable == '': # Nothing to do if a variable is not associated with the cut
                continue

            # First build the region name and selection
            region_name = region_name_base + "_Agnostic_" + cut.cut_title
            all_cuts_but_this = [cut_expression for cut_expression in cuts_expressions_vector if cut_expression != cut.cut_expression]
            selection = '( ' + (' && ').join(all_cuts_but_this) + ' )'

            # Now, find the variable to add.
            variable = None
            for var in relevant_variables:
                if var["name"] == cut.associated_variable:
                    variable = var
                    break

            if variable is None: # This should never happen because we checked this before. But just in case.
                Logger.log_message("ERROR", "Variable to track: %s, declared in cutflow %s as an associated variable for cut %s was not found." % (cut.associated_variable, self._name, cut.cut_title))
                exit(1)

            # Build the region dictionary
            region = {}
            region['name'] = region_name
            region['selection'] = selection
            region['variables'] = [variable] # This needs to be a list.

            # Add the region to the list
            regions_to_add.append(region)

        return regions_to_add

    def build_MiniAnalysis_regions(self, existent_regions : dict) -> dict:
        if existent_regions is None:
            Logger.log_message("ERROR", "You cannot create a MiniAnalysis without at least one basic region.")
            exit(1)

        # Look for the variables the user wants to track and the associated variables for each cut in the defined regions.
        variable_names_to_find = self._variables_to_track + [cut.associated_variable for cut in self._cuts_vector if(cut.associated_variable not in self._variables_to_track and cut.associated_variable!='')]
        relevant_variables = []

        # This loop will search for the variables in the regions
        for var_to_find in variable_names_to_find: # Seach for each variable
            vars_found = []
            for region in existent_regions: # Search in each region
                variables = region["variables"]
                for var in variables: # Search all variables
                    if type(var) == list: # This indicates that in this region anchors are being used. In esence we have checked those variables already.
                        continue
                    if var["name"] == var_to_find:
                        if var not in vars_found: # Check if it was already found with the same definitions
                            vars_found.append(var)
                            break # Found in this region

            # Nothing found then exit
            if len(vars_found) == 0:
                Logger.log_message("ERROR", "Variable to track: %s, declared in cutflow %s was not found in any region." % (var_to_find, self._name))
                exit(1)

            # If only one definition was found, add it to the relevant variables
            elif len(vars_found) == 1:
                relevant_variables.append(vars_found[0])

            # More than one option found so ask the user to choose between them
            else:
                Logger.log_message("ERROR", "Variable to track: %s, declared in cutflow %s was found with more than one different definitions. The following definitions were found:" % (var_to_find, self._name))
                counter = 0
                for var in vars_found:
                    Logger.log_message("ERROR", " %s ) " % (counter+1))
                    for key, value in var.items():
                        Logger.log_message("ERROR", "     %s : %s" % (key, value))

                    counter += 1
                Logger.log_message("ERROR", "Please give this variables a unique name or use the same definition.")
                exit(1)

        # Build the region dictionaries
        sequential_regions = self.build_MiniAnalysis_sequential_regions(relevant_variables)
        cut_agnostic_regions = self.build_MiniAnalysis_cut_agnostic_regions(relevant_variables)

        # Merge the two dictionaries
        regions = sequential_regions + cut_agnostic_regions
        return regions

    def __set_config_reader_cpp(self) -> None:
        """!Set the options in the C++ class
        """
        for selection_tuple in self._selections_list:
            options_getter = BlockOptionsGetter(selection_tuple)
            selection = options_getter.get("selection", None, [str])
            if selection is None:
                Logger.log_message("ERROR", "No selection specified in cutflow block " + self._name)
                exit(1)

            title = options_getter.get("title", None, [str])
            if title is None:
                Logger.log_message("ERROR", "No title specified in cutflow block " + self._name)
                exit(1)

            self.cpp_class.addSelection(selection, title)

    def adjust_samples(self, samples : dict[str, BlockReaderSample]) -> None:
        # if list of samples is not defined, add all samples
        if self._sample_names is None:
            for sample_name in samples:
                sample_cpp = samples[sample_name].cpp_class
                sample_cpp.addCutflow(self.cpp_class.getPtr())
        else:
            for sample_name in self._sample_names:
                if sample_name not in samples:
                    Logger.log_message("ERROR", "Sample " + sample_name + " not found in cutflow block " + self._name)
                    exit(1)
                sample_cpp = samples[sample_name].cpp_class
                sample_cpp.addCutflow(self.cpp_class.getPtr())

    @staticmethod
    def print_cutflow(cutflow_shared_ptr : int, cutflows_objects : list['BlockReaderCutflow'] ,indent : str = "") -> None:
        """!Print cutflow
        @param cutflow_shared_ptr: shared pointer to the Cutflow object
        @param indent: indentation
        """
        cpp_class = CutflowWrapper("")
        cpp_class.constructFromSharedPtr(cutflow_shared_ptr)

        # Find the correct cutflow object, search by name.
        cutflow_object = None
        for i_cutflow_object in cutflows_objects:
            if (cpp_class.name() == i_cutflow_object._name):
                cutflow_object = i_cutflow_object
                break
        if cutflow_object is None:
            Logger.log_message("ERROR", "Cutflow object not found in the list of cutflows objects")
            exit(1)

        variables_to_track = cutflow_object._variables_to_track
        selections_definitions = cpp_class.selectionsDefinition()
        selections_titles = cpp_class.selectionsTitles()

        print(indent + cpp_class.name())
        if cutflow_object.has_variables_to_track():
            print(indent + "variables_to_track: ")
            for i in range(len(variables_to_track)):
                print(indent+ indent + "- " + variables_to_track[i])
        for i in range(len(selections_definitions)):
            print(indent + "- selection:" + selections_definitions[i])
            print(indent + "  title:" + selections_titles[i])
            if cutflow_object.has_variables_to_track():
                cuts_vector = cutflow_object._cuts_vector
                if cuts_vector[i].associated_variable != None:
                    print(indent + "  associated_variable:" + cuts_vector[i].associated_variable)
                if cuts_vector[i].variables_to_exclude != None:
                    print(indent + "  variables_to_exclude:")
                    for excluded_variable in cuts_vector[i].variables_to_exclude:
                        print(indent + indent + "- " + excluded_variable)