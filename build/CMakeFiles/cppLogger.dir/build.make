# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.27

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase/x86_64/Cmake/3.27.5/Linux-x86_64/bin/cmake

# The command to remove a file.
RM = /cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase/x86_64/Cmake/3.27.5/Linux-x86_64/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/FastFrames

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/build

# Include any dependencies generated for this target.
include CMakeFiles/cppLogger.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/cppLogger.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/cppLogger.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/cppLogger.dir/flags.make

CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.o: CMakeFiles/cppLogger.dir/flags.make
CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.o: /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/FastFrames/python_wrapper/utils/Logger.cxx
CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.o: CMakeFiles/cppLogger.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.o"
	/cvmfs/sft.cern.ch/lcg/releases/gcc/13.1.0-b3d18/x86_64-el9/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.o -MF CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.o.d -o CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.o -c /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/FastFrames/python_wrapper/utils/Logger.cxx

CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.i"
	/cvmfs/sft.cern.ch/lcg/releases/gcc/13.1.0-b3d18/x86_64-el9/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/FastFrames/python_wrapper/utils/Logger.cxx > CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.i

CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.s"
	/cvmfs/sft.cern.ch/lcg/releases/gcc/13.1.0-b3d18/x86_64-el9/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/FastFrames/python_wrapper/utils/Logger.cxx -o CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.s

# Object files for target cppLogger
cppLogger_OBJECTS = \
"CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.o"

# External object files for target cppLogger
cppLogger_EXTERNAL_OBJECTS =

lib/cppLogger.so: CMakeFiles/cppLogger.dir/python_wrapper/utils/Logger.cxx.o
lib/cppLogger.so: CMakeFiles/cppLogger.dir/build.make
lib/cppLogger.so: /usr/lib64/libpython3.11.so
lib/cppLogger.so: CMakeFiles/cppLogger.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library lib/cppLogger.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/cppLogger.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/cppLogger.dir/build: lib/cppLogger.so
.PHONY : CMakeFiles/cppLogger.dir/build

CMakeFiles/cppLogger.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/cppLogger.dir/cmake_clean.cmake
.PHONY : CMakeFiles/cppLogger.dir/clean

CMakeFiles/cppLogger.dir/depend:
	cd /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/FastFrames /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/FastFrames /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/build /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/build /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/build/CMakeFiles/cppLogger.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/cppLogger.dir/depend

