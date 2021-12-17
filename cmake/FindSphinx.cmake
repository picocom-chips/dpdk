# PICOCOM PROPRIETARY INFORMATION
#
# This software is supplied under the terms of a license agreement or
# nondisclosure agreement with PICOCOM and may not be copied
# or disclosed except in accordance with the terms of that agreement.
#
# Copyright PICOCOM.

# Try to find then sphinx executable
# Once done, this will define
#
# SPHINX_FOUND      - system has Sphinx
# SPHINX_EXECUTABLE - Sphinx executable

find_program(SPHINX_EXECUTABLE NAMES sphinx-build
    HINTS
    $ENV{SPHINX_DIR}
    PATH_SUFFIXES bin
    DOC "Sphinx documentation generator")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sphinx DEFAULT_MSG
    SPHINX_EXECUTABLE)

mark_as_advanced(SPHINX_EXECUTABLE)
