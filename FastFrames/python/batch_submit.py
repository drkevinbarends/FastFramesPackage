import argparse
import os
import yaml

# Define the colors for the output
class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
def DEBUG(text):
    return bcolors.OKGREEN+text+bcolors.ENDC
def TITLE(text):
    return bcolors.OKBLUE+bcolors.BOLD+text+bcolors.ENDC
def HEADER(text):
    return bcolors.HEADER+bcolors.BOLD+bcolors.OKBLUE+bcolors.UNDERLINE+text+bcolors.ENDC
def WARNING(text):
    return bcolors.WARNING+text+bcolors.ENDC
def ERROR(text):
    return bcolors.FAIL+text+bcolors.ENDC

def createParser():
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config",  help="Path to the yml config file.")
    parser.add_argument("--samples", help="A comma separated list of samples to run. One job is created per listed sample. Default: all samples listed in the yml config.",default="all")
    parser.add_argument("--split-n-jobs", help="Number of jobs to split each sample into. Default: 1 (No splitting)", default=1, type=int)
    parser.add_argument("--step",    help="Step to run: 'n' (ntuples) or 'h' (histograms). Default: 'h'", choices=["h","n"],  default="h")
    parser.add_argument("--system",    help="Batch system, either condor or slurm. Default: 'condor'", choices=["condor","slurm"],  default="condor")
    parser.add_argument("--custom-class-path", help= "Path to the custom class used in the config file (if used). Default: None", default=None)
    parser.add_argument("--flavour", help="Job flavour which controls the max time of a job, CONDOR ONLY. Default: microcentury = 1h", default="microcentury")
    parser.add_argument("--slurm_time", help="Max time job can run before it is killed. Default: 1:00:00 = 1h", default="1:00:00")
    parser.add_argument("--memory", help="The amount of RAM to be request in GB. This option is only valid if you are running in Chicago or on Slurm. Memory in Lxplus scales as 2GB/CPU. Default: None", default=None)
    parser.add_argument("--dry-run", help="Creates the execution and submission environment without sending the jobs to HTCondor. Useful for debugging.", action="store_true")
    parser.add_argument("--chicago", help="Use this flag if you are running the jobs in the Chicago Analysis Facility.", action="store_true")
    parser.add_argument("--local-data", help="Use this flag if you want to copy the data to the scratch directory where jobs run before running the jobs.", action="store_true")
    parser.add_argument("--metadata-path",help="Path to directory containing the metadata of the input files.")
    parser.add_argument("--remote-eos-access",help="Use this flag to run FastFrames on a remote machine while accessing files stored in eos.", action="store_true")
    parser.add_argument("--kerberos-path", help="Add the path to your kerberos key on your remote machine e.g. '$HOME/krb5cc_12345'")
    return parser

def checkParser(cmdLineArguments):
    # Check that a correct configuration for the memory is passed.
    if cmdLineArguments.memory is not None and not (cmdLineArguments.chicago or cmdLineArguments.system == 'slurm'):
        print(ERROR("The memory option can only be used on lxplus HTCondor if you are running in Chicago."))
        print(ERROR("The memory in Lxplus scales as 2GB/CPU."))
        print(ERROR("Please see: https://batchdocs.web.cern.ch/tutorial/exercise6b.html"))
        exit(1)
    if cmdLineArguments.memory is not None:
        if "GB" not in cmdLineArguments.memory:
            print(ERROR("Please provide the memory in GB."))
            exit(1)
        if cmdLineArguments.memory.replace("GB","").isdigit() == False or cmdLineArguments.memory.replace("GB","")=="0":
            print(ERROR("Please provide a valid number for the requested memory."))
            exit(1)
    if cmdLineArguments.memory is None and cmdLineArguments.chicago:
        print(WARNING("The amount of memory is not being set. The default memory in Chicago is 2GB and this can be too low, causing your jobs to go on hold status."))

    # Check that job splitting makes sense
    if cmdLineArguments.split_n_jobs != 1:
        if cmdLineArguments.split_n_jobs < 1:
            print(ERROR("The number of jobs to split the samples into must be greater than 1."))
            exit(1)

# The params dictionary contains the parameters that are calculated given the user input.
jobParamatersDict = {
    "initialdir" : "",
    "RequestCpus" : "",
    "transfer_input_files" : "fastframes/,build/,install/,metadata/",
    "+JobFlavour" : ""
}

# Get the path to the FastFrames directory
def getFFPath():
    submissionPath = os.getcwd()
    twoLevelsUp = os.path.abspath(os.path.join(submissionPath, os.pardir, os.pardir))
    return twoLevelsUp

def createSubmissionFile(paramsDictionary):  
    executableCMD = "executable              = runFF.sh\n"
    argumentsCMD = "arguments               = $(ClusterId)$(ProcId)\n"
    outputCMD = "output                  = output/runFF.$(ClusterId).$(ProcId).out\n"
    errorCMD = "error                   = error/runFF.$(ClusterId).$(ProcId).err\n"
    logCMD = "log                     = log/runFF.$(ClusterId).log\n"
    getenvCMD = "getenv                  = True\n"
    preserveRelativePathsCMD = "preserve_relative_paths = True\n"
    with open("condor_submit.sub","w") as f:
        f.write(executableCMD)
        f.write(argumentsCMD)
        f.write(outputCMD)
        f.write(errorCMD)
        f.write(logCMD)
        f.write(getenvCMD) # Get the environment variables from the submission machine (lxplus)
        f.write(preserveRelativePathsCMD)
        for key,value in paramsDictionary.items():
            f.write(key + " = " + str(value) + "\n")
        f.write("\n")
        f.write("queue arguments from inputSamples.txt\n")

def createSlurmArraySubmissionFile(params, num_samples, split_n_jobs):
    """Creates a Slurm submission script for a job array."""
    with open("slurm_submit_array.sh", "w") as f:
        f.write("#!/bin/bash\n")
        f.write("#SBATCH --job-name=FastFramesArray\n")
        f.write("#SBATCH --output=output/runFF.%A_%a.out\n")
        f.write("#SBATCH --error=error/runFF.%A_%a.err\n")
        f.write(f"#SBATCH --time={params['time']}\n")
        f.write(f"#SBATCH --cpus-per-task={params['cpus']}\n")
        if params['memory'] is not None: 
            f.write(f"#SBATCH --mem={params['memory']}\n")
        n_totaljobs = num_samples * split_n_jobs
        f.write(f"#SBATCH --array=0-{n_totaljobs - 1}\n")  # Array indices
        f.write("\n")

        f.write("# Get the corresponding line from inputSamples.txt\n")
        f.write("line=$(sed -n \"$((SLURM_ARRAY_TASK_ID + 1))p\" inputSamples.txt)\n")

        f.write("# Parse the line to extract details\n")
        f.write("sample=$(echo $line | awk '{print $1}')       # Extract sample name\n")
        if split_n_jobs > 1:
            f.write("total_jobs=$(echo $line | awk '{print $2}')   # Extract total jobs\n")
            f.write("job_index=$(echo $line | awk '{print $3}')    # Extract job index\n")
        
        f.write("\n")
        f.write("# Run script with appropriate arguments\n")
        if split_n_jobs > 1:
            f.write("echo \"Processing sample: $sample, total jobs: $total_jobs, job index: $job_index\"\n")
            f.write("srun ./runFF.sh $sample $total_jobs $job_index\n")
        else: 
            f.write("echo \"Processing sample: $sample\"\n")
            f.write("srun ./runFF.sh $sample\n")  

def createExecutable(configYMLPath,step,copyDataToScratch,metadataPath, split_n_jobs,system):
    # Copy the config file to the submission directory
    os.system('cp ' +configYMLPath+ ' BatchConfig.yml')
    with open("runFF.sh","w") as f:
        f.write("#!/bin/bash\n")
        if(commandLineArguments.remote_eos_access):
            f.write("export KRB5CCNAME="+commandLineArguments.kerberos_path+"\n")
        if system == 'condor':
            f.write("shopt -s expand_aliases\n") # Enable aliases in the remote machine
            f.write("alias setupATLAS='source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh'\n") # Set up the ATLAS environment
            f.write("setupATLAS\n")
            f.write("asetup StatAnalysis,0.5.0\n")
            f.write("source build/setup.sh\n") # Load the FastFrames environment
            f.write("source CustomClassForBatch/build/setup.sh\n") # Load the custom class environment
        if copyDataToScratch and system == 'condor':
            if metadataPath is None:
                print(ERROR("ERROR: Please provide the path to the permanet data directory that will be used to send data from."))
                exit(1)
            f.write("python3 fastframes/python/copyDataToScratch.py $_CONDOR_SCRATCH_DIR $1 "+metadataPath+"\n") # Copy the data to the scratch directory if requested
        if system == "condor":
            f.write("cd fastframes/python\n")
        if split_n_jobs == 1: # Run
            f.write("python3 FastFrames.py " + "--config BatchConfig.yml --step " +step+ " --samples $1\n")
        else:
            f.write("python3 FastFrames.py " + "--config BatchConfig.yml --step " +step+ " --samples $1 --split_n_jobs $2 --job_index $3\n")
        
    # Give the file executable rights.
    os.system("chmod +x runFF.sh")

def loadYMLConfig(pathToConfig):
    with open(pathToConfig, 'r') as stream:
        try:
            return yaml.safe_load(stream)
        except yaml.YAMLError as exc:
            print(exc)

def copyCustomClassFilesForSubmission(customClassPath):
    # Do nothing if no custom class is used
    if customClassPath is None:
        return

    # First, remove any previous custom class used for condor jobs.
    try:
        os.remove('../../CustomClassForBatch/')
    except OSError:
        pass

    # Then, copy the custom class files to the submission directory
    try:
        os.system('rsync -r --exclude .git '+customClassPath+'/*'+' '+'../../CustomClassForBatch') # Avoid copying the .git directory if present
    except FileNotFoundError:
        print("Error copying the custom class files to the submission directory.")
        print(customClassPath,' path to custom class not found.')
        exit()

def setupJobParamsDict(generalBlock,cmdLineArguments):
     # Set to the FastFrames path
    jobParamatersDict["initialdir"] = getFFPath()
    # Set number of cpus as requested in the config file
    jobParamatersDict["RequestCpus"] = generalBlock["number_of_cpus"]
    # Set up the wall-time
    jobParamatersDict["+JobFlavour"] = f"\"{cmdLineArguments.flavour}\""
    # Add the custom class path to the transfer_input_files
    if cmdLineArguments.custom_class_path is not None:
        jobParamatersDict["transfer_input_files"] += ",CustomClassForBatch/"

    # Configuration change for Chicago Analysis Facility
    if cmdLineArguments.chicago:
        # First remove the options not supported in the Chicago Analysis Facility
        jobParamatersDict.pop("RequestCpus")
        jobParamatersDict.pop("+JobFlavour")
        # Add the Chicago specific options
        jobParamatersDict["request_cpus"] = generalBlock["number_of_cpus"]
        if cmdLineArguments.memory is not None:
            jobParamatersDict["request_memory"] = " %s" % cmdLineArguments.memory
        jobParamatersDict["+queue"] = '"short"'

def getListOfSamples(listOfSamplesFromInput,samplesBlock):
    listOfSamples = listOfSamplesFromInput
    if listOfSamplesFromInput == ["all"]: # If no samples are given, use all samples in the config file
        listOfSamples = []
        for sample in samplesBlock:
            listOfSamples.append(sample["name"])
    return listOfSamples

def createInputSamplesFile(listOfSamples, nJobs):
    with open("inputSamples.txt","w") as f:
        if nJobs == 1:
            for sample in listOfSamples:
                f.write(sample + "\n")
        else:
            for iJob in range(nJobs):
                for sample in listOfSamples:
                    f.write("%s %d %d \n" % (sample, nJobs, iJob))

def checkIsAFSorEOSPath(path):
    if not path.startswith("/eos/") and not path.startswith("/afs/"):
        print(WARNING("The path to the output files is recommended to be an absolute path in EOS or AFS."))
        print(path," is neither an AFS or EOS path.")

def checkIsRemoteEosAccess(args):
    if(args.remote_eos_access):
        print(WARNING("--remote-eos-access flag is switched ON!!! Accessing eos from a remote machine will potentially produce a lot of traffic between the machine running the jobs and the EOS instance!!! Preferably use lxplus for accessing the eos files."))
        print(WARNING("For accessing eos files from a remote machine make sure the metadata files used have the URL: 'root://eosatlas.cern.ch/'. e.g. 'root://eosatlas.cern.ch//eos/atlas/atlascerngroupdisk/......'. You should run produce_metadata_files.py with --remote-eos-access flag to get the output filelist.txt with the correct URL."))
    
def checkFileExistsFromSubmissionPath(filePath):
    if not os.path.exists(filePath):
        print(ERROR("The file "+filePath+" does not exist or its position is not defined properly relative to /fastframes/python/ in the YML config file."))
        exit()

def checkAssumptions(geeneralBlock):
    # 1. The script is run from the fastframes/python directory
    currentPath = os.getcwd()
    twoLastLevels = currentPath.split("/")[-2:]
    if twoLastLevels != ["fastframes","python"]:
        print(ERROR("Please run the script from the fastframes/python directory."))
        exit()

    # 2. The build and install directories of FastFrames are two levels up from the current directory, i.e, at the same level than the FastFrames source code.
    fastframesPath = getFFPath()
    directoriesAtSameLevel = os.listdir(fastframesPath)
    if "build" not in directoriesAtSameLevel or "install" not in directoriesAtSameLevel:
        print(ERROR("The build and install directories of FastFrames are not at the same level than the FastFrames source code."))
        exit()

    # 3. The metadata files are stored in a folder called metadata at the same level than the FastFrames source code.
    if "metadata" not in directoriesAtSameLevel:
        print(ERROR("The metadata files are not stored in a folder called metadata at the same level than the FastFrames source code."))
        exit()
    
    # 4. The path to the output files is an absolute path in EOS or AFS.
    pathToOutputHisotograms = geeneralBlock["output_path_histograms"]
    pathToOutputNtuples = geeneralBlock["output_path_ntuples"]
    checkIsAFSorEOSPath(pathToOutputHisotograms)
    checkIsAFSorEOSPath(pathToOutputNtuples)

    # 5. Check the that the paths to the metadata and the xSec files are set up correctly in the config file.
    # Check the metadata file
    pathToInputFiles = geeneralBlock['input_filelist_path']
    pathToInputSumW = geeneralBlock['input_sumweights_path']
    checkFileExistsFromSubmissionPath(pathToInputFiles)
    checkFileExistsFromSubmissionPath(pathToInputSumW)
    # Check the xSec file
    pathsToXSecFile = geeneralBlock['xsection_files']
    for path in pathsToXSecFile:
        checkFileExistsFromSubmissionPath(path)

def askUserForConfirmation():
    print(TITLE("This script submits jobs to the HTCondor/Slurm batch system from an lxplus-like machine..."))
    print(DEBUG("Please read the following carefully... you are about to potentially submit a large number of jobs to the HTCondor/Slurm batch system."))
    print("For a correct execution. This script assumes the following:")
    print(TITLE("1. "), "The script is run from the fastframes/python directory")
    print(TITLE("2. "), "The build and install directories of FastFrames are two levels up from the current directory, i.e, at the same level than the FastFrames source code.")
    print(TITLE("3. "), "The metadata files are stored in a folder called metadata at the same level than the FastFrames source code.")
    print(TITLE("4. (OPTIONAL)"), "The path to the output files is an absolute path in EOS or AFS preferentially.")
    print(TITLE("5. "), "The paths defined in the general block of the FastFrames configuration file are relative to /fastframes/python or absolute paths.")
    print("Are these assumptions correct? [y/n]")
    userConfirmation = input()
    if userConfirmation != "y":
        print(ERROR("Please make sure you fulfill the assumptions before running the script. Or input a valid answer."))
        exit()

if __name__ == "__main__":
    # Parse all the arguments
    commandLineArguments = createParser()
    commandLineArguments = commandLineArguments.parse_args()
    checkParser(commandLineArguments)

    checkIsRemoteEosAccess(commandLineArguments)
    
    # Load the config file
    config = loadYMLConfig(commandLineArguments.config)
    # Get the the general block and set up all the parameters
    generalBlockSettings = config["general"]

    # Inform the user what assumptions are made by the script and ask to confirm, then check them.
    askUserForConfirmation()
    checkAssumptions(generalBlockSettings)

    #Create the directories for the logs
    os.system("mkdir -p ../../output ../../log ../../error")

    # Create the executable file
    createExecutable(commandLineArguments.config,commandLineArguments.step,commandLineArguments.local_data,commandLineArguments.metadata_path,commandLineArguments.split_n_jobs,commandLineArguments.system)

    # Copy the necessary files to send with the job
    copyCustomClassFilesForSubmission(commandLineArguments.custom_class_path)
    
    # Create input samples files
    commaSeparatedSamples = commandLineArguments.samples.split(",")
    samplesBlock = config["samples"]
    samplesList = getListOfSamples(commaSeparatedSamples,samplesBlock)
    createInputSamplesFile(samplesList, commandLineArguments.split_n_jobs)
    
    # System specific setup
    if commandLineArguments.system == 'condor':

        # Set up job parameters and create the submission file
        setupJobParamsDict(generalBlockSettings,commandLineArguments)
        createSubmissionFile(jobParamatersDict)

        # Submit the jobs
        if not commandLineArguments.dry_run:
            os.system("condor_submit condor_submit.sub")
        else:
            print(DEBUG("Dry run. The submission files have been created in this directory. But the jobs have not been submitted."))
            print("To submit the jobs, run 'condor_submit condor_submit.sub'")

    elif commandLineArguments.system == 'slurm':

        # Set up SLURM parameters
        slurmParams = {
            "time": commandLineArguments.slurm_time,
            "cpus": generalBlockSettings.get("number_of_cpus", 1),
            "memory": commandLineArguments.memory
        }

        # Create slurm job array submission script
        createSlurmArraySubmissionFile(slurmParams, len(samplesList), commandLineArguments.split_n_jobs)

        # Submit the job array
        if not commandLineArguments.dry_run:
            os.system("sbatch slurm_submit_array.sh")
        else:
            print(DEBUG("Dry run: Created job array submission script slurm_submit_array.sh."))
            print("To submit the jobs, run 'sbatch slurm_submit_array.sh'")

