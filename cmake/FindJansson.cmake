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

set(JANSSON_FOUND 0)

find_path(JANSSON_INCLUDE_DIR
    NAMES jansson.h
    DOC "Jansson include directory"
)

find_library(JANSSON_LIBRARY
    NAMES jansson
    DOC "Jansson library"
)

# handle the QUIETLY and REQUIRED arguments and set JANSSON_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JANSSON DEFAULT_MSG JANSSON_LIBRARY JANSSON_INCLUDE_DIR)

if(NOT JANSSON_FOUND)
    message(WARNING "Jansson not found!")
endif()

mark_as_advanced(JANSSON_INCLUDE_DIR JANSSON_LIBRARY)
