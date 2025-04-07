# Changelog

### Upcoming release

### 4.2.0 <small>January 27, 2024</small>

- [isue #105](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/105): Do not produce the truth tree during ntupling if no branches are selected.
- [isue #104](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/104): Protect against non-matched events when doing the truth selection.
- [isue #103](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/103): Fix typo in samples from CustomBlocks.
- [isue #102](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/102): Addresses an issue with cling warnings appearing in the FastFrames Docker image.

### 4.1.0 <small>January 20, 2024</small>

- [issue #101](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/101): Adding a docker image configuration file and an image build step to the CI. These `latest` image gets created for every push to the main branch, tagged images for every git tag.
- [issue #99](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/99): Fixed split in N jobs and added this feature to the batch submission script.
- [issue #95](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/95) and [issue #100](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/100) : Properly propagate systematics on the "weight_total_NOSYS" variable in a custom code. Add support for matched truth branches with "NOSYS" in the name.
- [issue #97](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/97): Adding the possibility to specify trees to check when merging empty input files.
- [issue #96](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/96): Allowing user to override/use any keyword for sample, region and fit blocks in trex-fitter settings config.
- [issue #94](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/94): Fixing bug with regex support for regions in sample block.
- [issue #92](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/92): Adding CI test for testing histogram step on the ntuple output.

### 4.0.0 <small>December 4, 2024</small>

- [issue #91](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/91): Fix mysterious crash when pairing mroe than 1 truth tree to the reco tree.
- [issue #90](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/90): Fix memory leaks due to unreleased tchains.
- [issue #89](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/89): Allow support for treeName. prefit for the truth tree in case of identical branches in the truth and reco trees.
- [issue #88](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/88): Add error if user request reco vs. truth histograms, but disables truth vs. reco matching.
- [issue #43](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/43): Removed the workaround for the bug in ROOT for unmatched truth events. Using FilterAvailable() method of RDataFrame, see: https://root.cern/doc/v634/release-notes.html#rdataframe. This **breaks interface** as the `_IsMatched` available used in the workaround is removed.
- [issue #87](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/87): Add an option to control the std::vector to RVec conversion for ntupling. This **breaks interface** for ntupling, see the related issue.
- [issue #86](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/86): Update the ROOT version to 6.34.00 (using StatAnalysis 0.5.0)
- [issue #85](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/85): Changing the logic how the samples are processed. This **might break interface for the custom class**. The changes improve the running efficiency of the processing of the histograms. Please, check the issue for details!
- [issue #84](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/84): **Interface breaking change for the condor script.** Adding slurm support to `condor_submit.py` script and changing name of this script to `batch_submit.py` and some of the options there in to be batch instead of condor. Aside from name change condor script works as before. Also changing default memory option to be None so default options work on lxplus without chicago.
- [issue #83](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/83): Fix attempted redefinition of xSection and lumi columns when running over ntuples produced by FastFrames.
- [issue #82](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/82): Remove default Lumi values for Run 3 as there are multiple valid GRLs.

### 3.6.0 <small>November 14, 2024</small>

- [issue #81](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/81): Fix crash in RDF when no input files are provided.
- [issue #80](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/80): Allowing the user to specify excluded systematics or selected systematics lists in trex-fitter settings file.
- [issue #79](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/79): Adding support for custom settings in TRExFitter unfolding block (for example title migration matrix axes)
- [issue #78](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/78): Adding warning for the same branches in reco and truth trees when doing matching between the trees
- [issue #77](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/77): Fixing crash when producing unfolding inputs for config-defined truth variables
- [issue #76](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/76): Improving speed of truth to reco trees pairing.
- [issue #75](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/75): Fixing a crash, which occured if truth level selection was based on custom variable while reco to truth pairing was off.
- [issue #74](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/74): Define custom truth variables before selection for the truth ntupling.
- [issue #73](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/73): Allowing user to set list of regions via trex-fitter settings file.
- [issue #72](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/72): Fixing issue for regex matching in TRExFitter regions.
- [issue #71](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/71): Add support for 'unsigned char' branches.

### 3.5.0 <small>October 29, 2024</small>
- [issue #70](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/70):  Update to StatAnalysis 0.4.2.
- [issue #69](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/69):  Fix MiniAnalysis when using compounded selection strings.
- [issue #68](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/68):  Add support for TEfficiency histograms in the input files when running the ntupling step.
- [issue #67](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/67):  Ensuring propagation of ONNX related compiler flags from main FastFrames cmake to custom frame cmake.

### 3.4.0 <small>October 10, 2024</small>
- [issue #66](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/66):  Added capability to run condor jobs on a remote machine while accessing eos.
- [issue #61](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/61):  Added an option to configure the memory when submitting HTCondor jobs.
- [issue #64](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/64):  Add MiniAnalysis feature (an extension to the cutflows).
- [issue #63](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/63):  Fixing typo in trex_settings config + improving readibility of CI test output when comparing 2 ROOT files.
- [issue #62](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/62):  Fix mysterious crash when doing the reco to truth matching.
- [issue #65](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/65):  Removing TDP text file from the repository and adding warning for users who are still using it.

### 3.3.0 <small>September 18, 2024</small>

- [issue #60](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/60): Only check for missing cross-section for DSIDs defined in the config.
- [issue #59](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/59): Re-enable branches after doing the truth-reco pairing.
- [issue #58](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/58): Adding the possibility to use ```-step nh`` to run both ntuple and histogramming step in one command.
- [issue #57](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/57): Add 2023 luminosity default.
- [issue #56](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/56): Merging script for empty files updated for TCT v2.12.0 outputs.

### 3.2.0 <small>August 27, 2024</small>

- [issue #55](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/55): Updating condor submission script to use StatAnalysis 0.4.
- [issue #54](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/54): Fixing issue with trying to find matchable event for reco and truth trees that are not matched resulting in an ERROR.

### 3.1.0 <small>July 30, 2024</small>
- [issue #53](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/53): Add options to control the compression setting when creating ntuples.
- [issue #52](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/52): Skip not-defined variables for a sample when creating formula for a variable.
- [issue #51](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/51): Create log directories automatically when using condor submission. Check that paths in the config are relative to the submission position.
- [issue #43](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/43): Add a workaround for the bug in ROOT until it is fixed upstream.
- [issue #50](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/50): Read ONNX model only once at the beginning.
- [issue #49](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/49): Add an option to use simple formuale in a variable definition in the config.

### 3.0.0 <small>July 18, 2024</small>
- [issue #48](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/48): Fix condor_submit.py script to work without a custom FF class.
- [issue #47](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/47): Move order of TLV creation before the custom define calls.
- [issue #44](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/44): Adding an option to specify the name of the listOfSystematics histogram.
- [issue #46](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/46): Fix the problem of not being able to use custom-defined variables as weights when running the ntuple step.
- [issue #45](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/45): Pass Sample objects to the custom class defines. *THIS BREAKS USERS INTERFACE. CHECK THE ASSOCIATED MERGE REQUEST WITH DETAILS ON WHAT NEEDS TO BE UPDATED*.
- [issue #42](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/42): Make ntupling code recognise also variables added via Define() call.
- [issue #41](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/41): Adding script for checking duplicate events in input root files.
- [issue #40](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/40): Adding the possibility to select campaigns as command line input.
- [issue #39](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/39): Fixing issue with inconsistent set of branches in empty input files in merging script.
- [issue #38](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/38): Implementing a new config block for performing k-fold inference on simple ONNX models.
- [issue #19](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/19): Adding support for non-default sumweights histograms.

### 2.2.0 <small>July 1, 2024</small>
- [issue #37](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/37): Making ```MainFrame::defineVariablesNtupleTruth``` non const, for compatibility with ```MainFrame::defineVariablesTruth```. This is interface breaking change (this inteface was added quite recently in [issue #34](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/34)).
- [MR !330](https://gitlab.cern.ch/atlas-amglab/fastframes/-/merge_requests/330): Fixing compiler warning originating from onnxruntime library.
- [issue #36](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/36): Adding command line options for specifying path to the output ntuples/histograms.
- [issue #35](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/35): Fixed a problem with `merge_empty_grid_files.py` script which sometimes will generate empty trees with entries.
- [issue #34](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/34): Adding support for truth-level ntuples: selection, branch filtering and custom variables.
- [MR !313](https://gitlab.cern.ch/atlas-amglab/fastframes/-/merge_requests/313): Added wrapper for ONNX + updated tutorial.
- [issue #33](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/33): Separate subfolders can be used to individual regions. Please note that this is not supported by TRExFitter.
- [issue #31](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/31): Checking for duplicate DSIDs in sample block, making x-section manager less verbose, removing ```mc23c``` default lumi equal to 1.0 and fixing interface breaking change in StringOperations.
- [issue #30](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/30): Making x-section manager less verbose.
- [issue #29](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/29): Adding support for merging DSIDs from multiple samples into one sample (useful for fake lepton contribution).
- [issue #28](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/28): Adding support for job submission in the Chicago Analysis Facility.
- [issue #26](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/26): Adding support for regular expressions when using the '''exclude_regions''' and '''regions''' options in the samples block.
- [issue #25](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/25): Adding custom options to TRExFitter region block - all options defiend in region block of ```trex_settings``` text file will be copied to the output ```TRExFitter``` config file.

- [issue #27](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/27): Make ntuple processing respect `nominal_only flag`.

### 2.1.0 <small>June 12, 2024</small>
- [issue #24](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/24): Allowing select samples for TRExFitter using trex-fitter settings file. Allowing to override any variable-related option in TRExFitter config.
- [issue #23](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/23): Allowing a user to use also samples defined by ```numbering_sequence``` via CLI options.
- [issue #18](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/18): Allowing a user to specify more settings for TRExFitter config.
- [issue #17](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/17): Excluded regions were not propagated to the generated TRExFitter configuration file.
- Several improvemenets for PMG file reading: [issue #15](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/15), [issue #16](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/16), [issue #21](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/21).
- Several fixes for processing multiple truth selections on the same tree: [issue #20](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/20) [issue #22](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/22).

### 2.0.0 <small>May 22, 2024</small>
- [issue #10](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/10): Only read DSIDs from the PMG/TDP files that are actually needed.
- [issue #12](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/12): Fixing issues with processing multiple truth trees.
- [issue #14](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/14): Adding ```define_custom_columns_truth``` option under general block in config and removing ```define_custom_columns_truth``` from truth block. Interface breaking change affecting custom classes: method ```defineVariablesTruth``` takes string representing the truth tree as a second argument instead of taking the ```Truth``` object.
- [issue #15](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/15): Fixing the issue with duplicate x-section info in TDP file.

### 1.2.0 <small>May 15, 2024</small>
- Adjusting XSectionManager to be able to work with PMG x-section text files.
- Add the ability to modify automatic systematics in the TRexFitter config.
- When merging empty files, check if the trees are present in all files.
- [issue #8](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/8): add selection suffix to the Cutflow selections.
- Add the capability of building the metadata from files stored in the GRID.
- Add Sample and Region optional settings to trexfitter config.
- [issue #7](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/7): add a script to submit jobs to using an HTCondor batch system.

### 1.1.0 <small>April 25, 2024</small>
- [issue #6](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/6): add per-sample sum of weights option.
- Set AddDirectory to false.
- [issue #5](https://gitlab.cern.ch/atlas-amglab/fastframes/-/issues/5): add 3D histogram support.

### 1.0.0 <small>April 17, 2024</small>
- First release for TopWorkshop 2024 tutorial.
