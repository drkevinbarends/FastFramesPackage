
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was FastFramesConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

set(FastFrames_INCLUDE_DIRS /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/install/include)
set(ONNXRUNTIME_INCLUDE_DIRS "/cvmfs/atlas.cern.ch/repo/sw/software/0.5/StatAnalysis/0.5.0/InstallArea/x86_64-el9-gcc13-opt/include")
set(ONNXRUNTIME_LIBRARIES "/cvmfs/atlas.cern.ch/repo/sw/software/0.5/StatAnalysis/0.5.0/InstallArea/x86_64-el9-gcc13-opt/lib/libonnxruntime.so")
include ( "${CMAKE_CURRENT_LIST_DIR}/FastFramesTargets.cmake" )
