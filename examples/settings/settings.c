#include <vcap.h>

#include <stdio.h>
#include <stdlib.h>

#define VCAP_SETTINGS_IMPLEMENTATION
#include "../../extensions/vcap_settings.h"

int main(int argc, char* argv[])
{
    int index = 0;

    // First argument is device index
    if (argc == 2)
        index = atoi(argv[1]);

    vcap_device_info info = { 0 };

    // Find first video capture device
    int result = vcap_enumerate_devices(index, &info);

    if (result == VCAP_ERROR)
    {
        printf("Error: Enumerating devices failed\n");
        return -1;
    }

    if (result == VCAP_INVALID)
    {
        printf("Error: Unable to find capture device\n");
        return -1;
    }

    // Create device
    vcap_device* vd = vcap_create_device(info.path, true, 0); // Force read

    if (!vd)
    {
        printf("Error: Failed to create device\n");
        return -1;
    }

    // Open device
    if (vcap_open(vd) == VCAP_ERROR)
    {
        printf("Error: %s\n", vcap_get_error(vd));
        vcap_destroy_device(vd);
        return -1;
    }

    // Set format
    if (vcap_set_format(vd, VCAP_FMT_RGB24, (vcap_size){ 640, 480 }) != VCAP_OK)
    {
        printf("Error: %s\n", vcap_get_error(vd));
        vcap_destroy_device(vd);
        return -1;
    }

    // Export settings to string
    char* json_str = NULL;

    if (vcap_export_settings(vd, &json_str) == VCAP_ERROR)
    {
        printf("Error: %s\n", vcap_get_error(vd));
        vcap_destroy_device(vd);
        return -1;
    }

    printf("Device settings: \n%s", json_str);

    // Read settings back into device
    if (vcap_import_settings(vd, json_str) == VCAP_ERROR)
    {
        printf("Error: %s\n", vcap_get_error(vd));
        free(json_str);
        vcap_destroy_device(vd);
        return -1;
    }

    // Clear ujp
    free(json_str);
    vcap_destroy_device(vd);

    return 0;
}
