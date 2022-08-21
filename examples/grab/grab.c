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

    vcap_dev_info dev_info;

    // Find first video capture device
    int result = vcap_enum_devices(index, &dev_info);

    if (result == VCAP_ENUM_ERROR)
    {
        printf("Error while enumerating devices\n");
        return -1;
    }

    if (result == VCAP_ENUM_INVALID)
    {
        printf("Unable to find capture device\n");
        return -1;
    }

    // Create device
    vcap_dev* vd = vcap_create_device(dev_info.path, true, 0); // force read

    // Open device
    result = vcap_open(vd);

    if (result == VCAP_ERROR)
    {
        printf("%s\n", vcap_get_error(vd));
        vcap_destroy_device(vd);
        return -1;
    }

    vcap_size size = { 640, 480 };

    if (vcap_set_fmt(vd, VCAP_FMT_RGB24, size) == VCAP_ERROR)
    {
        printf("%s\n", vcap_get_error(vd));
        vcap_destroy_device(vd);
        return -1;
    }

    size_t image_size = vcap_get_image_size(vd);
    uint8_t image_data[image_size];

    // Grab an image from the device
    if (vcap_grab(vd, image_size, image_data) == VCAP_ERROR)
    {
        printf("%s\n", vcap_get_error(vd));
        vcap_destroy_device(vd);
        return -1;
    }

    // Open output file
    FILE* file = fopen("rgb.raw", "wb");

    if (!file)
    {
        perror("Cannot open image");
        vcap_destroy_device(vd);
        return -1;
    }

    // Write image to file
    fwrite(image_data, image_size, 1, file);

    if (ferror(file))
    {
        perror("Unable to write to file");
        fclose(file);
        vcap_destroy_device(vd);
        return -1;
    }

    // Clean up
    fclose(file);
    vcap_destroy_device(vd);

    return 0;
}
