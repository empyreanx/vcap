##==============================================================================
## MIT License
##
## Copyright 2022 James McLean
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in
## all copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.
##==============================================================================

set(V4L2_FOUND 0)

find_path(V4L2_INCLUDE_DIRS
    NAMES libv4l2.h libv4lconvert.h
    PATH_SUFFIXES v4l2 video4linux
    DOC "V4L2 userspace library include directory"
)

find_library(V4L2_LIBRARIES
    NAMES v4l2 v4lconvert
    DOC "V4L2 userspace library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(V4L2 DEFAULT_MSG V4L2_LIBRARIES V4L2_INCLUDE_DIRS)

if(NOT V4L2_FOUND)
    message(WARNING "libv4l2 userspace libraries not found!")
endif()

mark_as_advanced(V4L2_INCLUDE_DIRS V4L2_LIBRARIES)
