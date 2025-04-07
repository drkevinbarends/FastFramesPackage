# Copyright (C) 2002-2019 CERN for the benefit of the ATLAS collaboration
#
# Defines:
#
#  EIGEN_FOUND
#  EIGEN_INCLUDE_DIR
#  EIGEN_INCLUDE_DIRS
#
# Can be steered by EIGEN_LCGROOT.
#

# The LCG include(s):
include( LCGFunctions )

# Declare the external module:
lcg_external_module( NAME Eigen
   INCLUDE_SUFFIXES include/eigen3 INCLUDE_NAMES Eigen/Eigen )

# Handle the standard find_package arguments:
include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Eigen DEFAULT_MSG EIGEN_INCLUDE_DIR )
mark_as_advanced( EIGEN_FOUND EIGEN_INCLUDE_DIR EIGEN_INCLUDE_DIRS )

# Set up the RPM dependency:
lcg_need_rpm( eigen )
