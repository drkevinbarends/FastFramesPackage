# FastFrames

FastFrames is a lightweight and efficient framework designed for high-performance data processing and analysis. It provides tools to streamline workflows and optimize computational tasks.

## Features

- **High Performance**: Optimized for speed and efficiency.
- **Modular Design**: Easily extendable and customizable.
- **User-Friendly**: Simple APIs for quick integration.

For more information, please visit the FastFrames documentation [link](https://atlas-project-topreconstruction.web.cern.ch/fastframesdocumentation/tutorial/)

## Getting Started

To start using the package, source the `setup.sh` script:

```bash
source setup.sh
```

This will set up the necessary environment variables and dependencies.

To run the pipeline, source the `runFastFramesPipeline.sh` script:

```bash
source runFastFramesPipeline.sh
```

This will apply the final selection cuts for the tWZ analysis over the various Signal and Control regions and store the output in an ntuple.

**Note:** Before running the scripts, ensure that the file paths inside `setup.sh`, `runFastFramesPipeline.sh`, and `runFastFramesTest.sh` are updated to reflect their correct locations on your machine.

## Creating another branch

To create a new branch, first ensure you are on the branch you want to base it on. Then run:

```bash
git checkout -b <new-branch-name>
```

Replace `<new-branch-name>` with the desired name for your branch. This will create and switch to the new branch.

## Compiling Changes

If you make any changes to the `tWZ4LepClass.h` or `tWZ4LepClass.cc` files, you must run the `setup.sh` script in the `tWZ4LepClass` directory to compile and link your changes:

```bash
cd tWZ4LepClass
source setup.sh
```

## Testing Changes

To test your changes, you can run the `runFastFramesTest.sh` script:

```bash
source runFastFramesTest.sh
```

**Note:** Before running the test script, ensure that the file paths inside `runFastFramesTest.sh` are updated to reflect their correct locations on your machine.
