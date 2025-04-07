# Copyright (C) 2002-2019 CERN for the benefit of the ATLAS collaboration
#
# Module finding Pyquen in the LCG release. Defines:
#  - PYQUEN_FOUND
#  - PYQUEN_LIBRARIES
#  - PYQUEN_LIBRARY_DIRS
#
# Can be steered by PYQUEN_LCGROOT.
#

# The LCG include(s):
include( LCGFunctions )

# Declare the external module:
lcg_external_module( NAME Pyquen
   LIBRARY_SUFFIXES lib
   COMPULSORY_COMPONENTS pyquen )

# Handle the standard find_package arguments:
include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Pyquen DEFAULT_MSG PYQUEN_LIBRARY_DIRS
   PYQUEN_LIBRARIES )
mark_as_advanced( PYQUEN_FOUND PYQUEN_LIBRARIES PYQUEN_LIBRARY_DIRS )

# Set up the RPM dependency:
lcg_need_rpm( pyquen )
