#define VCAP_SETTINGS_IMPLEMENTATION
#include "../../extensions/vcap_settings.h"

int main(int argc, char* argv[])
{
    vcap_import_settings(NULL, NULL);
    vcap_export_settings(NULL, NULL);

    return 0;
}
