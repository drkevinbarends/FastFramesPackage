# Tutorial

The following tutorial demonstrates how to process a ROOT ntuple produced by [TopCPToolkit](https://topcptoolkit.docs.cern.ch/).
The tutorial covers the initial setup, compilation, adding custom variables, config preparation, running the code as well as automatically generating configuration file for [TRExFitter](https://gitlab.cern.ch/TRExStats/TRExFitter).
For the code documentation, please see [doxygen](https://atlas-project-topreconstruction.web.cern.ch/fastframesdoxygen/).

## Setup
The tutorial setup assumes you are working on an Alma Linux 9 machine with an access to cvmfs, such as lxplus machines.

### Setting up ROOT and Boost
To setup ROOT and Boost, we can take advantage of the [StatAnalysis](https://gitlab.cern.ch/atlas/StatAnalysis) releases.
If you want to run ```fastframes``` on a machine without access to [StatAnalysis](https://gitlab.cern.ch/atlas/StatAnalysis),
you will need ROOT 6.34 or newer, as well as Boost.
After logging to the machine do

```
setupATLAS
lsetup git # to setup newer version of git
asetup StatAnalysis,0.5.0
```

To check the version of ROOT setup, use:
```
root --version
```

You can also run the code on your local machine as long as you have ROOT and Boost (dependency of FastFrames) installed on the machine.

### FastFrames setup
Create a new folder called FastFramesTutorial:

```
mkdir FastFramesTutorial
cd FastFramesTutorial
```

Now, clone the [repository](https://gitlab.cern.ch/atlas-amglab/fastframes) using ssh:
```
git clone ssh://git@gitlab.cern.ch:7999/atlas-amglab/fastframes.git FastFrames
```

And switch to release v4.2.0 using:
```
cd FastFrames
git checkout v4.2.0
cd ../
```

You can also use other communication protocol like html or kerberos.

And create the input, build and install folders
```
mkdir build install input
```

Now, download the ROOT file produced by TopCPToolkit:
```
cd input
cp /afs/cern.ch/work/r/ravinab/public/FastFrames/tutorial_file.root .
cd ../
```

The ROOT file is obtained by running a single-lepton ttbar selection on a ttbar non-allhadronic file using the 2022-like setup (mc23a campaign).

### Compilation
Run the cmake step in the `build` folder:
```
cd build
cmake -DCMAKE_INSTALL_PREFIX=../install/ ../FastFrames/
```

Compile the code
```
make -jN # where N is the number of CPUs you can use, e.g. -j4
```

Install the code
```
make install
```

And, finally, setup the paths with:
```
source setup.sh
```

## x-section text files
When processing the MC ntuples, FastFrames needs to know the corresponding x-sections. There are usually taken from PMG text files. You can find them here: ```/cvmfs/atlas.cern.ch/repo/sw/database/GroupData/dev/PMGTools/``` . Please make sure you have the path correctly set in your config file (option  ```xsection_files``` under ```general``` block).


## Generating file metadata
First we need to generate the metadata needed for further processing.
A set of python scripts is provided that will search a given folder and all of it subfolders for ROOT files, inspect them and then create two text files.
The first file contains the list of all the ROOT files, the paths to the individual files, the DSID (6 digit unique identifier for each MC sample) and the simulation type (full simulation or fast simulation).
The second file contains the total sumweights for each weight for a given DSID, campaign and the simulation type.
To generate these files, do
```
cd ../FastFrames
python3 python/produce_metadata_files.py --root_files_folder ../input/
```

Let us inspect the new txt files in the `../input/` folder.
The `filelist.txt` file contains only one line (only one input file):
```
601229  mc23a   fullsim /home/tomas/RDataFrame/tutorial/input/tutorial_file.root
```
which shows the DSID, campaign, simulation type and the absolute path to the file.

The `sum_of_weights.txt` file contains a lot of lines, one line per MC weight variation.
The beginning of the file looks like this:
```
601229  mc23a   fullsim GEN_AUX_bare_not_for_analyses -1.0
601229  mc23a   fullsim GEN_MUR05_MUF05_PDF260000 18378766.0
601229  mc23a   fullsim GEN_MUR05_MUF1_PDF260000 18059854.0
601229  mc23a   fullsim GEN_MUR05_MUF2_PDF260000 17862044.0
601229  mc23a   fullsim GEN_MUR1_MUF05_PDF260000 16642543.0
```

Where the first columns represent: DSID, campaign, simulation type, name of the variation and the corresponding sum weights.

The sum of weights is calculated from ```CutBookkeeper``` histograms produced by TopCPToolkit. However, if you want to use a different histograms to calculate the sum of weights, you can use command line option ```--sum_weights_histo <histo_name>```. In this case, histograms with the names ```<histo_name>_<systematic_name>``` will be used to get sum of weights.

## Adding custom variables
As the format of the ROOT file makes direct histogramming difficult, it is very likely you will need to use your own code to add more variables/columns to the input file.
FastFrames allows seamless extension of the code code by providing a short custom class that allows users to add new columns in a convenient way.

### Skeleton setup
Change directory to the folder where `build`and `install` folders are:
```
cd ../
```

A helper repository is provided that contain a skeleton code needed for the custom class.
Clone the [repository](https://gitlab.cern.ch/atlas-amglab/FastFramesCustomClassTemplate) with
```
git clone ssh://git@gitlab.cern.ch:7999/atlas-amglab/FastFramesCustomClassTemplate.git TutorialClass
```

Where the second argument is the name of the folder you want to generate (the name of the class).
Now do the renaming using the scripts provided:
```
cd TutorialClass
./renameFiles.sh TutorialClass
```

Now, you can remove the script as well as the `.git` folder
```
rm renameFiles.sh
rm -rf .git
```
To make it easier to turn the folder into your own repository (recommended) on gitlab.
You can also let us know if you have your own repository and we can add it to the list of custom classes so that the new users can have a look how other analyses use it, see [here](https://gitlab.cern.ch/atlas-amglab/fastframes/-/tree/main/examples?ref_type=heads).

Now, let us compile the custom class and link to the main FastFrames code.
**Please, adjust the absolute path to the install folder of the FastFrames code.**
```
mkdir build install
cd build
cmake -DCMAKE_PREFIX_PATH=/afs/cern.ch/user/t/tdado/FastFramesTutorial/install -DCMAKE_INSTALL_PREFIX=../install ../
```

Compile the code
```
make
```

Finally, set the path with
```
source setup.sh
```

and return to the previous folder
```
cd -
```

**The last step, setting the paths is needed in every new shell. The paths are needed for the FastFrames code to find your custom class.**

### Adding leading jet pT variable
Now we have everything prepared to start adding some new variables that we can use for plotting.
Note that in RDataFrame, every variable you want to plot needs to be added via `Define()`. This includes the variables for applying selection or weights.
In this example, we will add a leading jet pT variable so that we can plot it later on.

The changes relevant for histogramming go into `defineVariables` method of your class. This is the method we will modify in Root/TutorialClass.cxx.
There are two general ways in RDataFrame how to add variables via Define(). The first one uses strings to define a formula the second approach uses c++ pointer to functions (functors). The c++ version leads to a better performance as it will be compiled, while the string version will be only Just-In-Time(JIT) compiled.

The following conceptual steps are needed to add a leading jet pT variable:

1. Take the vector of jet pT (`jet_pt_<SYSTNAME>`) and a vector of jet selections (`jet_select_baselineJvt_<SYSTNAME>`) and take only the jets that pass the selection and overlap removal
2. Sort the vector based on jet pT - the vectors are never sorted - they cannot be as systematic variations could change the order
3. Take the first element and store this as a new variable

The steps needed could be implemented using RDataFrame's Define() functions, but this would have to be repeated for all systematic variation making it very inconvenient.
For this, FastFrames has a method that does this for you! You only need to define this for the nominal variables, everything else will be automatically provided for you. The relevant function is `MainFrame::systematicDefine`. Let us now add the leading jet pT variable. The following code can be added to the `defineVariables` method:
```c++
  auto SortedTLVs = [](const std::vector<ROOT::Math::PtEtaPhiEVector>& fourVec,
                       const std::vector<char>& selected) {
    return DefineHelpers::sortedPassedVector(fourVec,selected);
  };

  auto LeadingTLV = [](const std::vector<ROOT::Math::PtEtaPhiEVector>& fourVec) {
    return fourVec.empty() ? ROOT::Math::PtEtaPhiEVector{-999, -999, -999, -999} : fourVec.at(0);
  };

  auto tlvPtGEV = [](const ROOT::Math::PtEtaPhiEVector& tlv) {
    return tlv.pt()/1.e3;
  };

  // add sorted passed jet TLV vector
  mainNode = MainFrame::systematicDefine(mainNode,
                                         "sorted_jet_TLV_NOSYS",
                                         SortedTLVs,
                                         {"jet_TLV_NOSYS", "jet_select_baselineJvt_NOSYS"});

  // add leading jet TLV
  mainNode = MainFrame::systematicDefine(mainNode,
                                        "jet1_TLV_NOSYS",
                                        LeadingTLV,
                                        {"sorted_jet_TLV_NOSYS"});

  mainNode = MainFrame::systematicDefine(mainNode,
                                         "jet1_pt_GEV_NOSYS",
                                         tlvPtGEV,
                                         {"jet1_TLV_NOSYS"});
```

The code above does what we need, but in a slightly more convenient/efficient way. Let us go through the different parts

```c++
  auto SortedTLVs = [](const std::vector<ROOT::Math::PtEtaPhiEVector>& fourVec,
                       const std::vector<char>& selected) {
    return DefineHelpers::sortedPassedVector(fourVec,selected);
  };
```

The above code snippet defined a lambda (`std::function`) that takes a vector of lorentz vectors, a vector of chars and then returns a sorted vector (based on pT) for selected elements (where the char is == 1). The code uses a helper function `DefineHelpers::sortedPassedVector` defined in FastFrames, see [this](https://gitlab.cern.ch/atlas-amglab/fastframes/-/blob/main/Root/DefineHelpers.cc?ref_type=heads).

```c++
  auto LeadingTLV = [](const std::vector<ROOT::Math::PtEtaPhiEVector>& fourVec) {
    return fourVec.empty() ? ROOT::Math::PtEtaPhiEVector{-999, -999, -999, -999} : fourVec.at(0);
  };
```

The above code takes a vector of lorentz vectors and if it is not empty it returns the first element.

```c++
  auto tlvPtGEV = [this](const ROOT::Math::PtEtaPhiEVector& tlv) {
    return tlv.pt()/1.e3;
  };
```
The above code takes a lorentz vector and then returns the pT component divided by 1000.

```c++
  // add sorted passed jet TLV vector
  mainNode = MainFrame::systematicDefine(mainNode,
                                         "sorted_jet_TLV_NOSYS",
                                         SortedTLVs,
                                         {"jet_TLV_NOSYS", "jet_select_baselineJvt_NOSYS"});
```
The above code adds a new variable `sorted_jet_TLV_NOSYS` based on the `SortedTLVs` lambda (function) and it relies on the two columns(variables): `jet_TLV_NOSYS` and `jet_select_baselineJvt_NOSYS`, where the first variable is provided by FastFrames (configurable) for convenience.
Note here that we only require that the jets passed the kinematic selection (pT and |eta|), overlap-removal and JVT selection for a given systematic variation.
However, this is something which is still actively being developed in TopCPToolkit/CP algorithms.

```c++
  // add leading jet TLV
  mainNode = MainFrame::systematicDefine(mainNode,
                                        "jet1_TLV_NOSYS",
                                        LeadingTLV,
                                        {"sorted_jet_TLV_NOSYS"});
```
The above code adds a variable `jet1_TLV_NOSYS` based on lambda `LeadingTLV` and it relies on the variable we have just created, `sorted_jet_TLV_NOSYS`.

```c++
  mainNode = MainFrame::systematicDefine(mainNode,
                                         "jet1_pt_GEV_NOSYS",
                                         tlvPtGEV,
                                         {"jet1_TLV_NOSYS"});
```

The above code adds a variable `jet1_pt_GEV_NOSYS` based on lambda `tlvPtGEV` and as an input uses `jet1_TLV_NOSYS`.
This is the variable we wanted to plot.

You could add the variable directly in one lambda however, the presented approach has multiple advantages:

* If you want to also add leading jet eta, you can use the `jet1_TLV_NOSYS` without rerunning the other steps that would have added computation overhead (the filtering and sorting would have to be rerun)
* You can also easily look at the second leading jet using `sorted_jet_TLV_NOSYS`

Please check also other helper functions, they are quite useful!

Now you need to re-compile and install the code for the custom class again in the build directory of the custom class with
```
cd build
make
make install
cd ..
```

!!! tip "Sample"
    The `defineVariables` method of the class also provides `Sample` parameter (`sample`). You can use the poitner to get the Sample name as defined in the config file using `sample->name()`. See the class documentation [here](https://atlas-project-topreconstruction.web.cern.ch/fastframesdoxygen/classSample.html).

!!! tip "UniqueSampleID"
    The `defineVariables` method of the class also provides `UniqueSampleID` parameter (`id`). This is a very simple class that allows to do per-sample specific operations. It has methods such as `id.dsid()`, `id.isData()`, `id.campaign()` that you can use to provide special defines based on these properties. See the class documentation [here](https://atlas-project-topreconstruction.web.cern.ch/fastframesdoxygen/classUniqueSampleID.html).

!!! tip "On variable validity"
    The new variables/columns are added to the main node, i.e. before any selection. Make sure that the variables you define are always valid. For example if you access zeroth element of a vector, make sure it is not empty. For the undefined cases, you can put some dummy values.

!!! tip "Using standard Define()"
    You can still use the standard RDataFrame Define(). This will add the variables only for the nominal copy, but if you want to only plot the histogram for nominal only this is okay.

!!! tip "Debugging"
    Debugging in these custom functions can be tricky. Standard printout methods work, but due to multithreading, the outputs might be shuffled. It is recommended to switch to just one thread when debugging.

## Performing ML inference using ONNX runtime

The ONNX runtime library offers a framework-agnostic way to run inference on a large variety of machine learning (ML) models. It allows us to train and optimize our models using mainstream python based frameworks like `scikit-learn`, `XGBoost`, `PyTorch` and `TensorFlow/Keras`, and then evaluate those models from the `c++` code used to define a custom variable in `FastFrames`.

To spare the analyst from learning the nitty-gritty details of the ONNX runtime library, `FastFrames` provides a convenient wrapper that accepts the inputs to the ML model and returns the outputs as standard `c++` data types. This `ONNXWrapper` class also provides support for k-fold validation, which is a standard technique in HEP. The simplest form of this is the 2-fold validation, where a ML model is trained using a subset of MC events (e.g. the events with even `eventNumber`), and then used to evaluate the complementary subset of events (e.g. the events with odd `eventNumber`). Similarly, another model with identical architecture is trained using the events with odd `eventNumber`, and then used to evaluate the events with even `eventNumber`.

First, we need to include the header file for the `ONNXWrapper`, and define a pointer to the `ONNXWrapper` as a private member of our `CustomFrame` class in its header file:
```c++
#include "FastFrames/ONNXWrapper.h"            // newly added line
...
class CustomFrame : public MainFrame {
  ...
  private:

    ClassDefOverride(CustomFrame, 1);

    ONNXWrapper *m_onnx;                       // newly added line
    ...
}
```

Then we need to modify the `CustomFrame::init()` method to create the `ONNXWrapper` object:
```c++
void CustomFrame::init() {
  MainFrame::init();

  m_onnx = new ONNXWrapper("My ML model", {
    "/path/to/the/model/My_model_fold_0.onnx",
    "/path/to/the/model/My_model_fold_1.onnx"
  });
}
```
Here the `My_model_fold_0.onnx` model is supposed to be used to evaluate the events with even `eventNumber`, and the `My_model_fold_1.onnx` model is supposed to be used to evaluate the events with odd `eventNumber`.
In this simple example, we demonstrate a 2-fold cross-validation. However, cross-valiadtion with any number of folds is supported.

Finally, we need to define a new custom variable using `systematicDefine()` to store the model output, and in the function that actually calculates this variable, we can run inference using the following code:
```c++
ONNXWrapper::Inference infer = m_onnx->createInferenceInstance();
std::vector<float> X = {/* input variables separated by commas */};
std::vector<int64_t> shape = {1, static_cast<int64_t>(X.size())};
infer.addInputs(X, shape);
unsigned int fold = m_onnx->getSessionIndex(eventNumber);
m_onnx->evaluate(infer, fold);
const size_t nClasses = /* number of classes in the classification model */;
std::array<float, nClasses> *y = infer.getOutputs<std::array<float, nClasses>>(0);
```

Note that, for a binary classifier, generally class `0` corresponds to the background, and class `1` corresponds to signal. So, `y->at(1)` should be returned as the `signal_score` in that case.

### Config block for simple models
FastFrames lets you run inference on *simple enough* ONNX models directly from its [config](#preparing-the-config-file-and-running), without requiring the user to write any extra C++ code. Needless to say, the user still needs to write some code to caluclate the input variables for the ML model, unless they are already present in the output of TopCPToolkit. They can be defined either in the `CustomFrame` or in the config file itself.

So, what is *simple enough* model?

1. Each input layer must accept a 1-D tensor (excluding the batch dimension) of type `float32`.
2. The output layer that the user wants to access must produce a 1-D tensor (excluding the batch dimension) of type `float32`.
>There can be other output layers with different dimensions and data types. As long as the user doesn't access them, it's fine.

To check the model architecture, use a tool like [Netron](https://netron.app/) to visualize the model.
It also shows the names, dimensions and data types of the input and output layers of the model.

Here is an example:

<div style="display: flex; align-items: center;">
  <img src="../images/model_architecture.png" style="flex: 2; object-fit: contain; max-height: 640px">
  <img src="../images/model_properties.png" style="flex: 2; object-fit: contain; max-height: 480px">
</div>

As we can see, there is only one input layer `args_0` of length 4, and only one output layer `softmax_output` of length 3.
The symbols `unk__12` and `unk__12` are the batch dimensions, and should be ignored. In some cases, they might be represented by the symbol `?` or can have a constant value 1.
The output layer returns three scores for a multiclass classification problem, however we are interested in only the first and last ones.

To run inference on this model, add the following block in the config file:

```yaml
simple_onnx_inference:
  - name: "MVA_model_1"
    model_paths: [
      "/path/to/the/model_fold_0.onnx",
      "/path/to/the/model_fold_1.onnx"
    ]
    inputs:
      "args_0": ["only_el_pt_NOSYS", "only_el_eta_NOSYS", "met_met_NOSYS", "met_phi_NOSYS"]
    outputs:
      "softmax_output": ["signal_score_NOSYS", "", "bkg2_score_NOSYS"]
```

If some of the input variables are of type `double` or `int`, they can be converted to `float` like this, right in the config file:

```yaml
general:
  ...
  define_custom_columns:
    - name: "nJets_float_NOSYS"
      definition: "static_cast<float>(nJets_NOSYS)"
```

Details of this `simple_onnx_inference` config block is available [here](../config/#simple_onnx_inference-block-settings).

## Preparing the config file and running

We are now ready to write the configuration file that will steer the processing of the ntuple into histograms.
Go back to the main FastFrames repository
```
cd ../FastFrames/
```

And create the following `tutorial_config.yml` file:

```yaml
general:
  debug_level: DEBUG

  # paths
  input_filelist_path: "../input/filelist.txt"
  input_sumweights_path: "../input/sum_of_weights.txt"
  output_path_histograms: "../output/"

  # weights
  default_sumweights: "NOSYS"
  default_event_weights: "weight_mc_NOSYS * weight_pileup_NOSYS * weight_jvt_effSF_NOSYS * weight_btagSF_DL1dv01_Continuous_NOSYS * globalTriggerEffSF_NOSYS * weight_leptonSF_tight_NOSYS"

  default_reco_tree_name: "reco"

  # path to the file with cross-sections
  xsection_files: ["/cvmfs/atlas.cern.ch/repo/sw/database/GroupData/dev/PMGTools/PMGxsecDB_mc23.txt"]

  # name of the custom class
  custom_frame_name: "TutorialClass"

  # automatic systematics?
  automatic_systematics: True
  nominal_only: False

  # which objects to create lorentz vectors for
  create_tlorentz_vectors_for: ["jet"]

  number_of_cpus: 4

  luminosity:
    mc23a: 26071.4


# define regions
regions:
  - name: "Electron"
    selection: "pass_ejets_NOSYS"
    variables:
      - name: "jet1_pt_GEV"
        title : "Leading jet p_{T}; Leading jet p_{T} [GeV];Events"
        definition: "jet1_pt_GEV_NOSYS"
        type: "double"
        binning:
          min: 0
          max: 300
          number_of_bins: 10
      - name: "reco_met"
        title : "MET;E_{T}^{miss} [MeV];Events"
        definition: "met_met_NOSYS"
        type: "float"
        binning:
          bin_edges: [0,20000,60000,80000,140000,250000]

  - name: "Muon"
    selection: "pass_mujets_NOSYS"
    variables:
      - name: "jet1_pt_GEV"
        title : "Leading jet p_{T}; Leading jet p_{T} [GeV];Events"
        definition: "jet1_pt_GEV_NOSYS"
        type: "double"
        binning:
          min: 0
          max: 300
          number_of_bins: 10
      - name: "reco_met"
        title : "MET;E_{T}^{miss} [MeV];Events"
        definition: "met_met_NOSYS"
        type: "float"
        binning:
          bin_edges: [0,20000,60000,80000,140000,250000]


# define samples
samples:
  - name: "ttbar_FS"
    dsids: [601229]
    campaigns: ["mc23a"]
    simulation_type: "fullsim"
```

Let us now go through the some of the less intuitive configurations options.

```yaml
  # weights
  default_sumweights: "NOSYS"
  default_event_weights: "weight_mc_NOSYS * weight_pileup_NOSYS * weight_jvt_effSF_NOSYS * weight_btagSF_DL1dv01_Continuous_NOSYS * globalTriggerEffSF_NOSYS * weight_leptonSF_tight_NOSYS"
```
The above block sets the weights for normalisation. `default_sumweights` tell the code which sumweights to use for normalisation. `default_event_weights` tells the code what the formula for the event weights is. Note that the code will automatically add the luminosity, cross-section and sumweights values.

```yaml
  # path to the file with cross-sections
  xsection_files: ["/cvmfs/atlas.cern.ch/repo/sw/database/GroupData/dev/PMGTools/PMGxsecDB_mc23.txt"]
```
The above block tells the code where to look for the file that contains the cross-section for input MC samples.
These point to the standard PMG file, feel free to copy it an update locally!

```yaml
  # name of the custom class
  custom_frame_name: "TutorialClass"
```
The above block tells the code what is the name of the custom class.
The name needs to match the name of the class you chose during the re-naming step.
This is needed to find the correct libraries during runtime!

```yaml
  # automatic systematics?
  automatic_systematics: True
  nominal_only: False
```
The above block is optional and can be used to tell the code to process all systematics or only run nominal even when you provide systematics in the config file.

```yaml
  # which objects to create lorentz vectors for
  create_tlorentz_vectors_for: ["jet"]
```
The above block tells the code to generate the `TLV` lorentz vector variables that we used in the custom class. You can set this for different objects like electron `el` or muons `mu`.

```yaml
  luminosity:
    mc23a: 26071.4
```
The above block tells the code that campaign `mc32a` shouldbe scaled to luminosity of 26071.1 ipb.

The region block for one region:
```yaml
  - name: "Electron"
    selection: "pass_ejets_NOSYS"
    variables:
      - name: "jet1_pt_GEV"
        title : "Leading jet p_{T}; Leading jet p_{T} [GeV];Events"
        definition: "jet1_pt_GEV_NOSYS"
        type: "double"
        binning:
          min: 0
          max: 300
          number_of_bins: 10
      - name: "reco_met"
        title : "MET;E_{T}^{miss} [MeV];Events"
        definition: "met_met_NOSYS"
        type: "float"
        binning:
          bin_edges: [0,20000,60000,80000,140000,250000]
```
specifies the region definition. It has the name of the region (to be used in the output) and a selection.
The selection can be a formula but only simple formulae are encouraged (e.g. simple AND or OR).
Then, each region has a list of variables for which histograms will be created.
The variables have a name (to be used in the output), title (the format is `title; x axis title; y axis title`), definition which is a name of the column (also simple formulae, such as ultiplication by a constant, are supported) and binning.
The `type` of the variable tells the c++ code which c++ template type to pass.
This allows to reduce the need for JITing, thus reducing memory and the time before the event loop is being executed.
Only some types are supported: "int", "long long int", "unsigned long", "unsigned long long int", "float" and "double".
For other types, or if you do not care about this performance part, you can leave this empty and the code will JIT the correct type.

The sample definition:
```yaml
# define samples
samples:
  - name: "ttbar_FS"
    dsids: [601229]
    campaigns: ["mc23a"]
    simulation_type: "fullsim"
```

Defines the list of samples. Each sample requires a name (that will be used in the file name for each sample), list of DSIDs (these will be combined!), list of campaigns (these will be combined) and the simulation type.

Please, check the list of all config options [here](https://atlas-project-topreconstruction.web.cern.ch/fastframesdocumentation/config/).
As well as a config file used in our CI tests [here](https://gitlab.cern.ch/atlas-amglab/fastframes/-/blob/main/test/configs/config.yml?ref_type=heads).

The yaml format allows to use some "tricks". E.g. it is possible to copy a block of settings (such as variables):

```yaml
Defaults: &defaults
  Company: foo
  Item: 123

Computer:
  <<: *defaults
  Price: 3000
```

An anchor (&) is used to define the configuration block to be repeated and the alias (*) is used when referring to the repeated configuration elsewhere. FastFrames also supports a nice feature for extending and re-using variables in different analysis regions. To reproduce the following setup:
```yaml
Region1:
  Variables:
    - var1
    - var2

Region2:
  Variables:
    - var1
    - var2
    - var3

Region3:
  Variables:
    - var1
    - var2
    - var3
    - var4
```
We can write
```yaml
Region1:
  Variables: &alias1 # Use this anchor to refer to var1 and var2
    - var1
    - var2

Region2:
  Variables: &alias2 # Use this anchor to refer to var1, var2 and var3
    - *alias1 # Resolves to var1 and var2
    - var3    # Extend this region with var3

Region3:
  Variables:
    - *alias2 # Resolves to var1, var2 and var3
    - var4
```


### Manual systematic uncertainties in the config file
The previous setup showed how to define systematic uncertainties automatically (taking the metadata information from the input file).
However, it might be important/convenient to provide a list of systematic uncertainties manually in the config file.
For this, FastFrames provides an option block `systematics` that allows you to manually define systematic variations.
For example the following block:

```yaml
systematics:
  - campaigns: ["mc23a"]
    regions: ["Electron", "Muon"]
    variation:
      up: "JET_BJES_Response__1up"
      down: "JET_BJES_Response__1down"
```

represents a manual definition of `JET_BJES_Response__1up` and `JET_BJES_Response__1down` systematics.
The up and down variation is not important here, but might be important when producing the TRExFitter config file (if automatic systematics are not switched on).
The above definition will replace all occurrences of `NOSYS` with `JET_BJES_Response__1up` and `JET_BJES_Response__1down` to define two systematic variations.
This includes the event weights, sumWeights and selections.
You can restrict a given variation only to some regions, samples or campaigns.
For the list of provided options see the [documentation](https://atlas-project-topreconstruction.web.cern.ch/fastframesdocumentation/config/#systematics-block-settings).
When automatic systematics are turned on and some systematics are defined in the config file, the definition in the config file will be used in case of identical names as the automatic systematic.
If the systematic variation in the config file does not match any automatic systematic it will still be included as defined in the config file.


### Histogram processing (1D)

Now everything is ready to produce the histograms. First create the output folder (defined in the config with: `output_path_histograms`)
```
mkdir ../output
```

And then run the FastFrames code by providing the config file
```
python3 python/FastFrames.py -c tutorial_config.yml
```

After the code finishes, we can inspect the output files.
Open the produced root file
```
root -l ../output/ttbar_FS.root
```

And inspect it with `.ls` or `TBrowser`.
You will see that there is one folder for each systematic variation (the nominal variation is called `NOSYS`).
Inside each folder, there is a set of histograms with names of a form of `<VariableName>_<RegionName>`.

!!! tip "2D and 3D histograms"
    FastFrames allow to also produce 2D and 3D histograms. Have a look at the `histograms_2d` and `histograms_3d` option in the `regions` block. This is also demonstrated in the config file for CI tests [here](https://gitlab.cern.ch/atlas-amglab/fastframes/-/blob/main/test/configs/config.yml?ref_type=heads#L30). Note that for 3D histograms you need to use at least version v1.1.0.

### Histogram processing (unfolding-like)
In many analyses, inputs for unfolding need to be generated.
These are: truth level distributions, selection efficiency plots, acceptance plots and 2D migration matrices.
FastFrames can generate these distributions automatically.
As an example, we will show the setup for reco level missing transverse momentum (MET) and particle-level MET.
In order to do this, a new sub-block needs to be added to the `sample` block for the `ttbar_FS` sample.
Add the following lines to the configuration:

```yaml
    truth:
      - name: particle
        produce_unfolding: True
        truth_tree_name: "particleLevel"
        event_weight: "weight_mc_NOSYS"
        variables:
          - name: "particle_met"
            definition: "met_met"
            type: "float"
            binning:
              min: 0
              max: 500000
              number_of_bins: 10
        match_variables:
          - reco: "reco_met"
            truth: "particle_met"
```

The `truth` sub-block tells the code that some truth information will be processed.
Note that multiple truth selections can be defined (e.g. parton and particle).
Let us describe the settings.

```yaml
      - name: particle
        produce_unfolding: True
        truth_tree_name: "particleLevel"
        event_weight: "weight_mc_NOSYS"
```
The above block tells the code that a new truth configuration called `particle` should be set.
`produce_unfolding` tells the code to produce the histograms needed for unfolding (selection efficiency and acceptance).
`truth_tree_name` is the name of the truth level tree in the input ntuple.
`event_weight` is the event weight used for the truth level (the cross-section and luminosity will be the same as for the reco level).

```yaml
        variables:
          - name: "particle_met"
            definition: "met_met"
            type: "float"
            binning:
              min: 0
              max: 500000
              number_of_bins: 10
```
The above block tells the code which variables to plot on the truth level.

```yaml
        match_variables:
          - reco: "reco_met"
            truth: "particle_met"
```
The above block tells the code to create the 2D plots for reco and truth variables.
Since this requires matching reco and truth trees, the code needs to know how to merge the individual events.
This is done using the `BuildIndex` functionality in TTree and it requires a unique set of variables for event identification.
These can be set via the `reco_to_truth_pairing_indices` option. The default value is `[runNumber, eventNumber]`.
You can control if the reco to truth matching between the input trees should be applied or not with: `pair_reco_and_truth_trees`.
This option is set to `False` by default and is automatically turned on when `match_variables` are provided.

Similarly to the reco level, you can use the custom class to add new variables/columns to the truth level.
The relevant method of the custom class is 'defineVariablesTruth'.

!!! tip "Truth selection"
    You can apply selection on the truth level as well by setting the `selection` option in the given `truth` block.

### Ntuple processing

FastFrames also allows you to process the input ntuple into an output ntuple that is self-similar, i.e. it will have the same structure as the input ntuple so it can be used as an input to the histogram processing using FastFrames.
In this section, a simple ntupling step will be shown that just applies a selection on the input ntuple.
To do this, first provide the path to the output folder for ntuples in the `general` block

```yaml
output_path_ntuples: "../output/"
```

And then add the following block:
```yaml
ntuples:
  selection: "pass_ejets_NOSYS"
  copy_trees: ["particleLevel"]
```

The `ntuples` block is used only for the output ntuple processing.
The `selection` option is optional and allows you to apply a selection for this step.
The code will automatically create a logical OR between all systematic variations for the selection and this will be used for the selection.
`copy_trees` tells the code to copy some other trees that are not the reco trees to be copied (for the self-similarity).
You can also control which variables to be included in the output ntuple via `branches` option, e.g. `branches: ["weight_total_.*", "jet_pt_.*"]` will only select branches that match the regular-expressions.
Alternatively, you can also use `exclude_branches` options to select the branches to store.

Similarly to the histogramming part, you can define custom variables in your custom class for the ntupling step.
The relevant method in the custom class is 'defineVariablesNtuple'.

To run the ntupling step, do:
```
python3 python/FastFrames.py -c tutorial_config.yml --step n
```

Where the `--step n` argument tells the code to run the ntupling part instead of the histogramming part (the default `--step h`).
There will be some overhead in the processing due to the complex (many ORs) selections.
After the processing is done, you should find a new file called `ttbar_FS_601229_mc23a_fullsim.root` in the `../output` folder.
Note that the code will generate one output root file for each DSID, campaign and the simulation type.
This step takes significantly more time compared to histogramming option - it can take up to 10 minutes with the tutorial ROOT file (of course depending on the CPU of the machine that you are using).

## Producing cutflows
FastFrames allow to produce cutflow histograms in a convenient way.
To tell the code to produce the cutflow, you need to add a new block to the setting.
E.g.:

```yaml
cutflows:
  - name: "selection_jet_pt"
    samples: ["ttbar_FS"] # optional, default is all samples
    selections:
      - selection: "jet1_pt_GEV_NOSYS > 40"
        title: "jet_pt_40"
      - selection: "jet1_pt_GEV_NOSYS > 50"
        title: "jet_pt_50"
      - selection: "jet1_pt_GEV_NOSYS > 60"
        title: "jet_pt_60"
```

When running the standard histogram processing step an additional cutflow histogram will be created showing the weighted (with the nominal weight) events passing all specified selections.
The cutflow selection will follow the order of the selections in the config file.
You can provide multiple cutflows in the config file.
Note that you need to create at least one "standard" histogram if you want to run the cutflows.

## Distributed computing
RDataFrame supports multi-threading when processing the input files (both when the output is an ntuple or a set of histograms).
The number of threads used is controlled by the `number_of_cpus` parameter.
The multi-threading of RDataFrame is extremely efficient, see e.g. this [presentation](https://indico.fnal.gov/event/23628/contributions/241029/attachments/154864/201541/RDF%20%40%20ROOT%20workshop%202022.pdf).

However, you can still benefit from a distributed system (multiple physical/virtual machines).
The easiest way to parallelise the processing is to use a command line option to specify which samples (as defined in the config file) to process.
This can be achieved by passing the following parameter when running FastFrames:

```
--samples sample1,sample2,sample3
```

In some cases, even processing one sample can take a long time and it would be desirable to split it even further.
FastFrames provides an option for this.
Passing the following parameter:
```
--split_n_jobs <N jobs total> --job_index <current job index>
```

tells the code to split the processing of the individual input files into `<N jobs total>` where `<current job index>` can be used to control which set of the files is being processed.
The output of each of the jobs will contain these two parameter in the output name.

When unfolding plots are requested while the split processing is used, the selection efficiency and acceptance histograms will **not** be produced as they cannot be simply "hadd"-ed from the individual jobs.
Instead, a merging script is provided that allows to not only merge the output files in case of split processing but also to produce the selection efficiency and acceptange histograms if requested.
To use the script simply do
```
python3 python/merge_jobs.py -c <config_file>
```

where `<config_file>` is the path to the config file used for running FastFrames.

## Generating the TRExFitter config file

```fastframes``` offers a possibility to automatically generate a config file for TRExFitter. Please keep in mind that the config include all samples and systematics from the ```fastframes``` config, which does not have to be what you want to do in the fit. There might be also other analysis specific aspects that cannot be automatized. Thus the resulting TRExFitter config should be properly reviewed before the use.

### Generating the config file for inclusive fit

The following command will generate config file for inclusive cross-section profile-likelihood fit:

```
python3 python/produce_trexfitter_config.py --config test/configs/config.yml --output trex_config.yaml --trex_settings test/configs/trex_settings.yml
```

where:

```--config``` argument is the address of the config file used by ```FastFrames.py```. If you enabled automatic systematics, you have to run ```FastFrames.py``` first and keep the output ROOT files in the folder specified in the config. The script will need it to read the list of the systematic uncertainties.

```--output``` argument is the address of the output TRExFitter config file to be produced.

```--trex_settings``` argument is optional. It is the address of a yaml file providing additional settings for ```TRExFitter``` config, which cannot be automatically deduced from fastframes config, such as colors and titles used for individual samples, fit type, unfolding settings, etc. It can be also used to add new blocks to the config file, such as systematic uncertainties and norm. factors. The example of this file can be found in ```test/configs/trex_settings.yml```. The complete set of the available options can be found in the [FastFrames documentation](https://atlas-project-topreconstruction.web.cern.ch/fastframesdocumentation/trex_config/).

### Generating the config file for unfolding


The following command will generate config file for profile-likelihood unfolding in TRExFitter:

```
python3 python/produce_trexfitter_config.py --config test/configs/config.yml --output trex_config.yaml --trex_settings test/configs/trex_settings.yml --unfolding ttbar_FS:parton:Ttbar_MC_t_afterFSR_pt:jet_pt
```

Where first 3 arguments are the same as for inclusive fit. Additional argument ```--unfolding``` (or ```-u```) specifies the unfolding setup. There are 4 values separated by colon:

```ttbar_FS``` - Name of the signal sample - the name must match name in the ```fastframes``` config

```parton``` - Name of the truth block to be used. This is used, for example, to specify if you want to unfold to particle or parton level.

```Ttbar_MC_t_afterFSR_pt``` - Variable you want to unfold at the truth level.

```jet_pt``` - Detector level variable you want to use in the unfolding.

## Analysis prototyping with cutflows (Mini-Analysis)

This section explains how to use the `cutflows` block to perform quick analysis.

In many physics analyses one first defines a set of variables and selections (commonly also known as cuts). After this step, analysers often track the evolution of the kinematic variables as cuts are applied, and study how each selection changes the composition of their samples. In addition, the effect of changing the selection criteria is also studied.

For instance, let's assume that you want to select a sample of ttbar single electron + jets events. A list of kinematic variables to study might include:

- The number of jets in the event.
- The number of electrons in the event.
- The pT of the leading electron.
- The pT of some of the jets.

Furthermore, one might be interested in applying a set of selection cuts to isolate the ttbar single electron + jets events. Let's say:

- Requiring only one tight electron.
- Requiring four jets.
- Requiring that the leading jet has a pT greater than 50 GeV.
- Requiring that the electron has a pT greater than 40 GeV.

In FastFrames (FF), once you define the kinematic variables, you would need to create four regions - adding one additional cut each time you create a region, and adding all the variables you want to track in each region. The FF configuration woud look something like this:

```
Region: Cutflow1_Pass_Ne
 - Selection: nElectrons_tight_NOSYS == 1
 - Variables:
   - jet1_pt
   - jet2_pt
   - el_pt
   - nElectrons
   - nJets
   - el_eta

Region: Cutflow1_Pass_Ne_Pass_Nj
 - Selection: nElectrons_tight_NOSYS == 1 && nJets_good_NOSYS == 4
 - Variables:
   - jet1_pt
   - jet2_pt
   - el_pt
   - nElectrons
   - nJets
   - el_eta

Region: Cutflow1_Pass_Ne_Pass_Nj_Pass_j1pt
 - Selection: nElectrons_tight_NOSYS == 1 && nJets_good_NOSYS == 4 && jet1_pt_NOSYS >= 50
 - Variables:
   - jet1_pt
   - jet2_pt
   - el_pt
   - nElectrons
   - nJets
   - el_eta

Region: Cutflow1_Pass_Ne_Pass_Nj_Pass_j1pt_Pass_ept
 - Selection: nElectrons_tight_NOSYS == 1 && nJets_good_NOSYS == 4 && jet1_pt_NOSYS >= 50 && el1_pt_NOSYS >= 40
 - Variables:
   - jet1_pt
   - jet2_pt
   - el_pt
   - nElectrons
   - nJets
   - el_eta
```

Often, you would also be interested in knowing how did the variable used for a cut looked like before applying the cut. In this example since we have four cuts that means another 4 additional regions to be defined in FF:

```
Region: Cutflow1_Agnostic_Pass_Ne
 - Selection: nJets_good_NOSYS == 4 && jet1_pt_NOSYS >= 50 && el1_pt_NOSYS >= 40
 - Variables:
   - nElectrons

Region: Cutflow1_Agnostic_Pass_Nj
 - Selection: nElectrons_tight_NOSYS == 1 && jet1_pt_NOSYS >= 50 && el1_pt_NOSYS >= 40
 - Variables:
   - nJets

Region: Cutflow1_Agnostic_Pass_j1pt
 - Selection: nElectrons_tight_NOSYS == 1 && nJets_good_NOSYS == 4 && el1_pt_NOSYS >= 40
 - Variables:
   - jet1_pt

Region: Cutflow1_Agnostic_Pass_ept
 - Selection: nElectrons_tight_NOSYS == 1 && nJets_good_NOSYS == 4 && jet1_pt_NOSYS >= 50
 - Variables:
   - el_pt
```

Fortunately, FF has a great feature that allows to do the same with less effort! The `cutflow` block allows you to perform this type of analysis (we call it Mini-Analysis). To achieve the same behaviour (and more) you just need to write the following:

```
cutflows:
  - name: Cutflow1
    variables_to_track:
      - jet1_pt
      - jet2_pt
      - el_pt
      - nElectrons
      - nJets
      - el_eta
    selections:
      - selection: nElectrons_tight_NOSYS == 1
        title: "Pass_Ne"
        associated_var: nElectrons
        variables_to_exclude:
          - el_eta
      - selection: nJets_good_NOSYS == 4
        title: "Pass_Nj"
        associated_var: nJets
      - selection: jet1_pt_NOSYS >= 50
        title: "Pass_j1pt"
        associated_var: jet1_pt
        variables_to_exclude:
          - nElectrons
      - selection: el1_pt_NOSYS >= 40
        title: "Pass_ept"
        associated_var: el_pt
        variables_to_exclude:
          - nElectrons
          - nJets
```

This configuration allows you to easily define the variables you want to track, what cut to apply at each stage, which variables to skip at each stage of the cutflow, and which variable is the one associated with each cut. Cutflows allows you to define as many of these Mini-Analysis as you want and gives you an additional histogram that tracks the event yields across the cutflow. In our example, the cutflow looks like this:

<div style="display: flex; align-items: center;">
  <img src="../images/cutflow_histogram.png" style="flex: 2; object-fit: contain; max-height: 640px">
</div>

The histograms added to our output file in our example are the following:

<div style="display: flex; align-items: center;">
  <img src="../images/cutflow_file_content.png" style="flex: 2; object-fit: contain; max-height: 640px">
</div>

You can see that every tracked variable, except if excluded, was added at each stage of the cutflow. The naming scheme followed is: `varName_CutFlowName_cut1Name_cut2Name_..._cutLastName`. For example, if you have a very simple Mini-Analysis like:
```
cutflows:
  - name: Cutflow1
    variables_to_track:
      - var1
      - var2
      - var3
    selections:
      - title: "cut1"
        associated_var: CutVar1
      - title: "cut1"
        associated_var: CutVar2
```

you will have the following variables on your output:
```
# First cut
var1_Cutflow1_cut1
var2_Cutflow1_cut1
var3_Cutflow1_cut1

# Second cut
var1_Cutflow1_cut1_cut2
var2_Cutflow1_cut1_cut2
var3_Cutflow1_cut1_cut2

# Events agnostic of cut1 passing cut2
CutVar1_Cutflow1_Agnostic_cut1

# Events agnostic of cut2 passing cut1
CutVar2_Cutflow1_Agnostic_cut2
```

In our example, if we inspect the electron pT after each cut we see:

#### Requiring one electron

<div style="display: flex; align-items: center;">
  <img src="../images/cutflow_ept_1.png" style="flex: 2; object-fit: contain; max-height: 640px">
</div>

#### + Requiring four jets
we see less events,
<div style="display: flex; align-items: center;">
  <img src="../images/cutflow_ept_2.png" style="flex: 2; object-fit: contain; max-height: 640px">
</div>

#### + Requiring leading jet pT
we see less events,
<div style="display: flex; align-items: center;">
  <img src="../images/cutflow_ept_3.png" style="flex: 2; object-fit: contain; max-height: 640px">
</div>

#### + Requiring leading electron pT
we see less events and no electrons before 40 GeV.
<div style="display: flex; align-items: center;">
  <img src="../images/cutflow_ept_4.png" style="flex: 2; object-fit: contain; max-height: 640px">
</div>

#### Region agnostic of electron pT but all other cuts passed
We see all electron passing the other three selections before we cut at 40 GeV.
<div style="display: flex; align-items: center;">
  <img src="../images/cutflow_ept_agnostic.png" style="flex: 2; object-fit: contain; max-height: 640px">
</div>
