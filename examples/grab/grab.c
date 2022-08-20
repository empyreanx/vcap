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

#include <vcap/vcap.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    int index = 0;

    if (argc == 2)
        index = atoi(argv[1]);

    vcap_device device;

    // Find first video capture device
    int result = vcap_enum_devices(&device, index);

    if (result == VCAP_ENUM_ERROR)
    {
        printf("%s\n", vcap_get_error());
        return -1;
    }

    if (result == VCAP_ENUM_INVALID)
    {
        printf("Error: Unable to find a video capture device\n");
        return -1;
    }

    // Open device
    vcap_fg* fg = vcap_open(&device);

    if (!fg)
    {
        printf("%s\n", vcap_get_error());
        return -1;
    }


    vcap_frame* frame = NULL;
    vcap_size size = { 640, 480 };

    if (vcap_set_fmt(fg, VCAP_FMT_RGB24, size) == -1)
    {
        printf("%s\n", vcap_get_error());
        goto error;
    }

    frame = vcap_alloc_frame(fg);

    if (!frame)
    {
        printf("%s\n", vcap_get_error());
        goto error;
    }

    if (vcap_grab(fg, frame) == -1)
    {
        printf("%s\n", vcap_get_error());
        goto error;
    }

    file = fopen("out.raw", "wb");

    if (!file)
    {
        perror("Cannot open image");
        goto error;
    }

    fwrite(frame->data, frame->length, 1, file);

    if (ferror(file))
    {
        perror("Unable to write to file");
        goto error;
    }

    fclose(file);
    vcap_free_frame(frame);
    vcap_close(fg);

    return 0;

error:

    if (file)
        fclose(file);

    if (frame)
        vcap_free_frame(frame);

    vcap_close(fg);

    return -1;
}
