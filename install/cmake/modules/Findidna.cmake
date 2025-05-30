# Copyright (C) 2002-2019 CERN for the benefit of the ATLAS collaboration
#
# Sets:
#  IDNA_PYTHON_PATH
#
# Can be steered by IDNA_LCGROOT.
#

# The LCG include(s).
include( LCGFunctions )

# Find it.
lcg_python_external_module( NAME idna
   PYTHON_NAMES idna/__init__.py idna.py )

# Handle the standard find_package arguments.
include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( idna DEFAULT_MSG
   _IDNA_PYTHON_PATH )

# Set up the RPM dependency.
lcg_need_rpm( idna )
