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

set(JANSSON_FOUND 0)

find_path(JANSSON_INCLUDE_DIR
    NAMES jansson.h
    DOC "Jansson include directory"
)

find_library(JANSSON_LIBRARY
    NAMES jansson
    DOC "Jansson library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JANSSON DEFAULT_MSG JANSSON_LIBRARY JANSSON_INCLUDE_DIR)

if(NOT JANSSON_FOUND)
    message(WARNING "Jansson not found!")
endif()

mark_as_advanced(JANSSON_INCLUDE_DIR JANSSON_LIBRARY)
