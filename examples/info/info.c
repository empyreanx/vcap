//==============================================================================
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>
//==============================================================================

#include <vcap.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    int index = 0;

    // First argument is device index
    if (argc == 2)
        index = atoi(argv[1]);

    vcap_dev_info info = { 0 };

    int result = vcap_enum_devices(index, &info);

    if (result == VCAP_ENUM_ERROR)
    {
        printf("Error while enumerating devices\n");
        return -1;
    }

    if (result == VCAP_ENUM_INVALID)
    {
        printf("Error: Unable to find video capture device\n");
        return -1;
    }

    vcap_dev* vd = vcap_create_device(info.path, true, 0);

    // Open device
    result = vcap_open(vd);

    if (result == VCAP_ERROR)
    {
        printf("%s\n", vcap_get_error(vd));
        return -1;
    }

    // Dump info
    if (vcap_dump_info(vd, stdout) == VCAP_ERROR)
    {
        printf("%s\n", vcap_get_error(vd));
        return -1;
    }

    vcap_destroy_device(vd);

    return 0;
}
