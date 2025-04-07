# Copyright (C) 2002-2019 CERN for the benefit of the ATLAS collaboration
#
# Defines:
#  - DAVIX_FOUND
#  - DAVIX_INCLUDE_DIRS
#  - DAVIX_LIBRARY_DIRS
#  - DAVIX_<component>_FOUND
#  - DAVIX_<component>_LIBRARY
#  - DAVIX_LIBRARIES
#
# Can be steered by DAVIX_LCGROOT.
#

# The LCG include(s):
include( LCGFunctions )

# Declare the external module:
lcg_external_module( NAME Davix
   INCLUDE_SUFFIXES include/davix INCLUDE_NAMES davix.hpp
   LIBRARY_SUFFIXES lib lib32 lib64
   COMPULSORY_COMPONENTS davix )

# Handle the standard find_package arguments:
include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Davix DEFAULT_MSG DAVIX_INCLUDE_DIR
   DAVIX_LIBRARIES )
mark_as_advanced( DAVIX_FOUND DAVIX_INCLUDE_DIR DAVIX_INCLUDE_DIRS
   DAVIX_LIBRARIES DAVIX_LIBRARY_DIRS  )

# Set up the RPM dependency:
lcg_need_rpm( Davix )
