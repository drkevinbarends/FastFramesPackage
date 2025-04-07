# Copyright (C) 2002-2019 CERN for the benefit of the ATLAS collaboration
#
# Defines:
#
#  FFTW_FOUND
#  FFTW_INCLUDE_DIR
#  FFTW_INCLUDE_DIRS
#  FFTW_<component>_FOUND
#  FFTW_<component>_LIBRARY
#  FFTW_LIBRARIES
#  FFTW_LIBRARY_DIRS
#
# Can be steered by FFTW_LCGROOT.
#

# The LCG include(s):
include( LCGFunctions )

# Declare the external module:
lcg_external_module( NAME FFTW
   INCLUDE_SUFFIXES include INCLUDE_NAMES fftw3.h
   LIBRARY_SUFFIXES lib DEFAULT_COMPONENTS fftw3 )

# Handle the standard find_package arguments:
include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( FFTW DEFAULT_MSG FFTW_INCLUDE_DIR
   FFTW_LIBRARIES FFTW_LIBRARY_DIRS )
mark_as_advanced( FFTW_FOUND FFTW_INCLUDE_DIR FFTW_INCLUDE_DIRS
   FFTW_LIBRARIES FFTW_LIBRARY_DIR FFTW_LIBRARI_DIRS )

# Set up the RPM dependency:
lcg_need_rpm( fftw )
