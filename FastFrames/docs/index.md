# Fast Frames documentation

FastFrames is an histograming and ntuple reprocessing framework. The core code is written in C++ with a python interface around it.

## How to checkout and compile the code

#### Linux local (assuming you have ROOT and Boost libraries installed)

To run the code, you firstly need to check out the repository and compile the C++ part of the package. In order to do so:

The following structure is suggested:
```
cd YourFavoriteFolder
mkdir build install

git clone ssh://git@gitlab.cern.ch:7999/atlas-amglab/fastframes.git

cd build
cmake -DCMAKE_INSTALL_PREFIX=../install/ ../fastframes
# alternatively, point the path to the install folder

make # alternatively "make -jN", where N is number of CPUs you want to use for compilation

# install the library
make install
```

Then you will need to set the `LD_LIBRARY_PATH` to the installation path using the produced setup.sh script

```
source setup.sh
```

At this point the code should be compiled. Make sure you are using at least 6.28 version of the ROOT.

To install Boost libraries, do (or similarly on different linux platforms)
```
sudo apt-get install libboost-all-dev
```

If you do not have git large file storage (LFS) installed on your machine, you need to use the following steps:

```
sudo apt install git-lfs #or similar
git lfs install
git lfs pull
```

This should download the ROOT files that are used for the CI tests, but can also be used to run the code as an example

#### MacOS local

First, install ROOT, CMake and the BOOST libraries, you can use the package manager of your preference, in this tutorial we will use [homebrew](https://brew.sh)

```
brew install root
brew install cmake
brew install boost-python3
```
Now, create a folder to store the project files:
```
mkdir FF && cd FF # Pick your favourite name if you do not like FF :)
```
After this step, we clone the FastFrames repository:

```
git clone ssh://git@gitlab.cern.ch:7999/atlas-amglab/fastframes.git
```

We can now configure the build, compile and install:
```
cmake -S fastframes -B build -DCMAKE_INSTALL_PREFIX=install
# -S should point to the folder that contains the FastFrames source code and -B to the build directory.

cmake --build build -jN --target install
# N is the number of cores you want to use for the compilation.
```

Finally, you will need to set the `LD_LIBRARY_PATH` to the installation path using the produced setup.sh script

```
source build/setup.sh
```

To download the ROOT files that are used for the CI tests, and can also be used to run the code as an example do:

```
git lfs pull
```

#### Using lxplus(-like) machine
The instructions are the same as in the Local build, but you can setup ROOT and Boost using StatAnalysis using Alma Linux 9 (lxplus default)

```bash
setupATLAS
asetup StatAnalysis,0.5.0
```

This will setup an appropriate version of ROOT (you can check the ROOT version with `root --version`)



#### Docker Image and Continuous Integration (CI)

FastFrames provides pre-built Docker images to streamline the setup and usage of the framework. These images are based on the [ATLASOS](https://gitlab.cern.ch/atlas-sit/docker/) Docker images and rely on dependencies available through the CVMFS file system. As a result, the same conditions for running on lxplus-like machines apply when using these images.

These Docker images are particularly useful for automated CI testing of downstream software, as they ensure a consistent and reproducible environment aligned with lxplus-like setups.

##### Image Creation and Availability:

- Main Branch Updates: Any update to the main branch triggers the creation of a new Docker image tagged as latest. This tag is ideal for testing ongoing development.
- Tagged Releases: Each tagged release in the repository results in a Docker image tagged with the corresponding version number (e.g., v1.2.3). These tags represent stable releases for production or specific use cases.

##### Pulling the Docker Images

To pull the latest image:

```sh
docker pull gitlab.cern.ch:7999/atlas-amglab/fastframes:latest
```

To pull an image for a specific release (replace <version> with the desired tag):

```sh
docker pull gitlab.cern.ch:7999/atlas-amglab/fastframes:<version>
```

##### Using the Docker Images

These images are pre-configured to run FastFrames with dependencies set up via CVMFS. So, he host system must meet the following conditions for the images to work:

- Access to the CVMFS file system.
- A configuration compatible with lxplus-like environments.

Provided these conditions are met, you can easily use these images interactively for development and testing by mounting a working directory into the container (the > indicate that the commands are executed within the docker container):

```sh
docker run -it -v /path/to/your/workdir gitlab.cern.ch:7999/atlas-amglab/fastframes:latest /bin/bash
> export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase   # Sets up the ATLAS commands
> alias setupATLAS='source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh'  # Creates usual setupATLAS alias
> setupATLAS
> asetup StatAnalysis,0.5.0
```



## How to run the code:


#### Merging empty files and removing them

The RDataFrame cannot handle files with empty trees or no trees at all, but one can obtain such files from GRID production.

---
**NOTE**

From TopCPToolkit v2.12.0 one should never get empty trees as part of the output files. Please see [here](https://gitlab.cern.ch/atlasphys-top/reco/TopCPToolkit/-/issues/169).

Users of earlier TCT versions should use the legacy version of the merging script. See the note below.

---
In order to solve this problem, one can merge the empty files with one non-empty file and then remove the original files which have been merged (to avoid double counting). There is a script to do this in the ```FastFrames``` repository:

You can run the script using the following command:

```
python3 python/merge_empty_grid_files.py --root_files_folder <path to the folder with ROOT files>
```

it will go recursivelly over all subfolders, merge all empty files belonging to the same DSID, campaign and type (fastsim/fullsim/data) with one non-empty file. After that, it will remove the original files in order to avoid double counting in the later steps. If all all files from the same DSID+campaign+type are empty, it will merge all of them together (FastFrames can handle this case).

If one needs more detailed logging from this step, the following command line option can be used to set debug level (default is ```INFO```):

```
--log_level <LOG LEVEL, FOR EXAMPLE "DEBUG">
```

The script will by default check ```reco```, ```truth``` and ```particleLevel``` trees (if some of them is missing, it's fine, it will be ignored). If you want to use any other list of trees, you can use the following argument to specify it.

```
--trees_to_check <comma separated list of trees>
```

---
**TopCPToolkit < v2.12.0 users**

Earlier versions of TCT can produce files with empty trees. To use the merging script over this files please add the `--legacy-tct-output` flag to the merging command. Like:

```
python3 python/merge_empty_grid_files.py --root_files_folder <path to the folder with ROOT files> --legacy-tct-output
```

---

#### Obtaining metadata files:

In order to run the code, you will firstly need to produce text files with metadata. There are 2 of them, the first one contains the list of all ROOT files belonging to the same sample (same DSID, campaign and simulation type - fastsim/fullsim).
The other file contains sum of weights for each sample and each systematic variation.

In order to produce the metadata files:

```bash
python3 python/produce_metadata_files.py --root_files_folder <path_to_root_files_folder> --output_path <path_to_output_folder>
```

where you have to specify the path to the folder with the ROOT files that you are going to reprocess. The second argument is optional, it is the path to the folder where metadata files will be stored.
By default they will be in the same folder as input ROOT files. In the config file used in the next steps, you will have to specify the addresses of these metadata files.

Optionaly, one can add ```--check_duplicates true``` option. This will loop over all input files and print warning if there are duplicate events (the same DSID, data type (i.e. AF, FS, data) eventNumber and runNumber).

Checking duplicate events can be also run as a separate step:

```python3 python/check_duplicate_events.py --root_files_folder <root_files_folder>```

##### Building the metadata from files stored in the GRID:

---
**NOTE**

Using FastFrames in this way should be restricted to cases where the RSEs and the processing machines are at the same location. Otherwise, data reads will go through the internet and this will take a lot of bandwith and will make the processing very slow.

---

If the input files are stored in the GRID, in a particular Rucio Storage Element (RSE), FastFrames is also capable of building the metadata. Just use the same script from above but pass a different argument:

```bash
python3 produce_metadata_files.py --grid_datasets <path_to_text_files_with_grid_datasets> --output_path <path_to_output_folder>
```

The second argument is the path to a text file containing all the relevant GRID containers, one example is:

```
user.dbaronmo:user.dbaronmo.601229.PhPy8EG.DAOD_PHYS.e8514_s4162_r14622_p5980.Common-FTAG-NTuples-v0.3.0_output
user.dbaronmo:user.dbaronmo.601230.PhPy8EG.DAOD_PHYS.e8514_s4162_r14622_p5980.Common-FTAG-NTuples-v0.3.0_output
user.dbaronmo:user.dbaronmo.601348.PhPy8EG.DAOD_PHYS.e8514_s4162_r14622_p5980.Common-FTAG-NTuples-v0.3.0_output
```

The script will ask for the RSE where the files are stored. For the example containers you should provide:

```
UKI-NORTHGRID-MAN-HEP_LOCALGROUPDISK
```

---
**NOTE**

To get the list of all the GRID RSEs you can do:

```
rucio list-rses
```

---

#### Running histograming part:

To produce the histograms from your ROOT files, one has to set up a config. You can find example configs in ```test/configs/``` and you can find all available options also in the documentation webpage.
To run the histogramming part:

```bash
python3 python/FastFrames.py --config <path to your config> --step h
```

One can also use ```c``` instead of ```config```

If you do not want to run over all samples from your config, but just over a part of them, you can specify the list of these samples using optional argument ```samples```:

```bash
python3 FastFrames.py --config <path to your config> --step h --samples sample1,sample2,sample3
```

You can also specify ```max_event``` or ```min_event``` options from general part of the config using the command line.

#### Running ntuple part:

Running ntuple part is similar to running histogramming part, you just need to specify ```n``` step in terminal:

```bash
python3 FastFrames.py --config <path to your config> --step n
```

It is also possible to use ```-step nh``` to run both ntuples and histograms from one command.

#### Other command line options:

In order to split the sample into multiple jobs, you can use the following command line option:

```bash
--split_n_jobs <N jobs total> --job_index <current job index>
```

In order to specify a different metadata file directory (containing `filelist.txt` and `sum_of_weights.txt`), you can use the following option (or `-i` instead of `--input_path`):

```bash
--input_path <path to directory>
```

In order to merge the output ROOT files from all jobs into one file, one can use the following command:

```bash
python3 python/merge_jobs.py --c <config address>
```

One can use the following options to override paths to the output ntuples/histograms from the config file:

```--output_path_histograms <path>```

```--output_path_ntuples <path>```

In order to run only on given set of campaigns, you can use:

```--filter_campaigns <list of comma selarated list of campaigns>```

#### Submitting jobs to the HTCondor/Slurm batch service

FastFrames provides a useful script to submit jobs to the [CERN HTCondor batch service](https://batchdocs.web.cern.ch/concepts/index.html) or a local Slurm batch system. After loging into an Lxplus node go the directory you have FastFrames under:

```
cd <your_fastframes_path>
```
the relevant script, `batch_submit.py`, is under the `fastframes/python/` directory:
```
cd fastframes/python
```

To submit jobs run the script with the options that best adapt to your current workflow. If you do
```
python3 batch_submit.py --help
```
the following description will appear:
```
python3 batch_submit.py --help

usage: batch_submit.py [-h] [-c CONFIG] [--samples SAMPLES] [--split-n-jobs SPLIT_N_JOBS] [--step {h,n}] [--system {condor,slurm}] [--custom-class-path CUSTOM_CLASS_PATH] [--flavour FLAVOUR] [--slurm_time SLURM_TIME]
                       [--memory MEMORY] [--dry-run] [--chicago] [--local-data] [--metadata-path METADATA_PATH] [--remote-eos-access] [--kerberos-path KERBEROS_PATH]

options:
  -h, --help            show this help message and exit
  -c CONFIG, --config CONFIG
                        Path to the yml config file.
  --samples SAMPLES     A comma separated list of samples to run. One job is created per listed sample. Default: all samples listed in the yml config.
  --split-n-jobs SPLIT_N_JOBS
                        Number of jobs to split each sample into. Default: 1 (No splitting)
  --step {h,n}          Step to run: 'n' (ntuples) or 'h' (histograms). Default: 'h'
  --system {condor,slurm}
                        Batch system, either condor or slurm. Default: 'condor'
  --custom-class-path CUSTOM_CLASS_PATH
                        Path to the custom class used in the config file (if used). Default: None
  --flavour FLAVOUR     Job flavour which controls the max time of a job, CONDOR ONLY. Default: microcentury = 1h
  --slurm_time SLURM_TIME
                        Max time job can run before it is killed. Default: 1:00:00 = 1h
  --memory MEMORY       The amount of RAM to be request in GB. This option is only valid if you are running in Chicago or on Slurm. Memory in Lxplus scales as 2GB/CPU. Default: None
  --dry-run             Creates the execution and submission environment without sending the jobs to HTCondor. Useful for debugging.
  --chicago             Use this flag if you are running the jobs in the Chicago Analysis Facility.
  --local-data          Use this flag if you want to copy the data to the scratch directory where jobs run before running the jobs.
  --metadata-path METADATA_PATH
                        Path to directory containing the metadata of the input files.
  --remote-eos-access   Use this flag to run FastFrames on a remote machine while accessing files stored in eos.
  --kerberos-path KERBEROS_PATH
                        Add the path to your kerberos key on your remote machine e.g. '$HOME/krb5cc_12345'
```
Let's run an example containing a custom fastframes class - more details about the custom class in the next section. Here the custom class is located two levels up from the submission directory, i.e., at the same level than the fastframes source code.
```
python3 batch_submit.py -c ../../MyAnalysis/ConfigYML/sim_calib_config.yml --step h --custom-class-path ../../MyAnalysis
```

Now the scripts asks to confirm whether the following assumptions are true:
```
This script submits jobs to the HTCondor batch system from an lxplus-like machine...
Please read the following carefully... you are about to potentially submit a large number of jobs to the HTCondor batch system.
For a correct execution. This script assumes the following:
1.  The script is run from the fastframes/python directory
2.  The build and install directories of FastFrames are two levels up from the current directory, i.e, at the same level than the FastFrames source code.
3.  The metadata files are stored in a folder called metadata at the same level than the FastFrames source code.
4. (OPTIONAL) The path to the output files is an absolute path in EOS or AFS preferentially.
Are these assumptions correct? [y/n]
```

Essentially, the submission script expects the following directory hierarchy:
```
build (dir)
install (dir)
metadata (dir)
fastframes (dir)
    python (dir)
        batch_submit.py # You must be here when executing the script.
```
and your paths to the FastFrames outputs should be absolute paths in AFS or EOS. Therefore, in your general block your should have a config like:
```
general:
  input_filelist_path: "../../metadata/filelist.txt" # Notice how the metadata is stored two levels up from here inside the metadata directory.
  input_sumweights_path: "../../metadata/sum_of_weights.txt"
  output_path_histograms: "/afs/cern.ch/user/E/ExampleUser/public/results/" # Absolute path in AFS
  output_path_ntuples: "/eos/user/E/ExampleUser/results/" # Or, absolute path in EOS
```

After you acknowledge that the previous assumptions are true you will see the following message (for HTCondor):
```
Submitting job(s)..
X job(s) submitted to cluster 12776566.
```
where X is the number of jobs submitted. This number is dependent on the arguments you provided when executing `batch_submit.py`. If you did not provide a comma separated list of samples the script will submit one job for each sample listed in your yml config.

To monitor the status of your jobs you can do `condor_q`. You will see a similar output to:
```
condor_q


-- Schedd: bigbird14.cern.ch : <137.138.44.75:9618?... @ 04/26/24 10:08:55
OWNER    BATCH_NAME      SUBMITTED   DONE   RUN    IDLE  TOTAL JOB_IDS
dbaronmo ID: 12776566   4/26 10:05      _      _      2      2 12776566.0-1

Total for query: 8 jobs; 0 completed, 0 removed, 8 idle, 0 running, 0 held, 0 suspended
Total for dbaronmo: 8 jobs; 0 completed, 0 removed, 8 idle, 0 running, 0 held, 0 suspended
Total for all users: 22412 jobs; 2240 completed, 15 removed, 15542 idle, 4540 running, 75 held, 0 suspended
```

---
**NOTES (for HTCondor)**

* You can kill jobs if necessary by doing `condor_rm JOB_ID`.
* Be careful with how CPU cores you request per job. The more resources you request, the later your jobs will start executing.
* Make sure you compile the code to include new changes before submiting the jobs.
* If you are working in the Chicago AF, you need to provide the appropriate `--chicago` flag to produce the correct configuration for this cluster.
* You can specify with the `--flavour` argument an upper bound for how long individual jobs can last (afterwhich they may be killed.) Default is "microcentury" which is 1h. You can see other flavours [here](https://batchdocs.web.cern.ch/local/submit.html#job-flavours).

---

If using a local Slurm batch system (not on Lxplus!), you just need to pass the `--system slurm` argumet to the `batch_submit.py` script. Note the slurms jobs are submitted via an array. If using slurm you need to use the `--slurm_time` argument to pass an upper limit for how long jobs can take instead of the `--flavour` argument.

## Adding custom class for custom Define() call

When using RDataFrame, all variables ("columns" in RDataFrame language) used in the selection (Filter in RDataFrame language), variable definition or weights need to be defined.
RDataFrame allows to define variables using the `Define()` method on each RDataFrame node. There are two variations of the method:

```c++
auto newNode = node.Define("variable_GeV", "variable/1e3");
```

In the version above, the new variable, `variable_GeV` is created with a formula provided by a string. In the example, this takes a variable called `variable` and divides it by 1e3.
This version of Define() allows easy interface, but it has to be Just-In-Time (JIT) compiled.
Another version uses pure c++ code
```c++
auto newNode = node.Define("variable_GeV", [](const float value){return value/1e3}, {"variable"});
```

This version is functionally equivalent to the previous one, it also creates a new node called `variable_GeV`, by passing a c++ functor(lambda) and a list of varaibles it depends on as the third argument.
In this way, any new columns can be added. FastFrames allows users to only modify this part of the code while letting the rest of the code do the hard work.
Technically this is done by overriding the main class and leading the new class on runtime.

#### Example of custom class for custom define
An example of the custom class is provided [here](https://gitlab.cern.ch/atlas-amglab/fastframes/-/tree/main/examples/CustomVariables?ref_type=heads)

#### Compile the custom class
It is important that the new class needs to be compiled and linked against the base class.
This can be achieved by:
```
mkdir MyCustomFrame
cd MyCustomFrame
mkdir build install
```

And providing the new class in the new folder, e.g. you can copy the contents fo the linked folder:
```
cp -r * /your/path/MyCustomFrame/
```
or you can use the template for this [here](https://gitlab.cern.ch/atlas-amglab/FastFramesCustomClassTemplate)

If you want to use a different name than CustomFrame for the code, you need to change the CMakeLists.txt content appropriately (just renaming) and also the corresponding files and folder. Do not forget about Root/LinkDef.h!

Now you need to compile the code. For the cmake step, you need to tell the code where you want to install the library, so that ROOT can find it during run time and also where you installed FastFrames.
```
cd build
cmake -DCMAKE_PREFIX_PATH=~/Path/to/your/fastframes/install -DCMAKE_INSTALL_PREFIX=/where/you/want/to/install ../
```
E.g. if you installed FastFrames into /home/FastFranes/install and you want to install the custom library to /MyCustomFrame/install (default) do

```
cmake -DCMAKE_PREFIX_PATH=~/home/FastFrames/install -DCMAKE_INSTALL_PREFIX=../install ../
```

You can also adjust the CMakeLists.txt file to put the absolute path for the FastFrames install by adding

```
set (CMAKE_PREFIX_PATH "~/home/FastFrames/install" CACHE PATH )
```
And then you do not have to use -DCMAKE_PREFIX_PATH=~/home/FastFrames/install as an argument for the cmake call (you still need to use the -DCMAKE_INSTALL_PREFIX argument).

Now, compile and install the code

```
make
make install
```
Finally, you need to export set the `LD_LIBRARY_PATH` to tell ROOT where to look for the library:

```
source setup.sh
```

#### Using the custom class
To use the custom class, you need to set the name of the class in the config file in the general block: `custom_frame_name` to the name of the library, code will then search the `LD_LIBRARY_PATH` to find the appropriate library.
Have a look at how the new library can be used to add new columns: [here](https://gitlab.cern.ch/atlas-amglab/fastframes/-/blob/main/examples/CustomVariables/Root/CustomFrame.cc?ref_type=heads).
Note that there are many helper functions that only require you to provide the new columns for the nominal values and will be automatically copied for the systematic varaitions for you!
