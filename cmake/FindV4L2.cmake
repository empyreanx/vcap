##==============================================================================
## Vcap - A Video4Linux2 capture library
##
## Copyright (C) 2018 James McLean
##
## This library is free software; you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public
## License as published by the Free Software Foundation; either
## version 2.1 of the License, or (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public
## License along with this library; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
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
