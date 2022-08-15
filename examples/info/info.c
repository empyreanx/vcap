//==============================================================================
// Vcap - A Video4Linux2 capture library
//
// Copyright (C) 2018 James McLean
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
//==============================================================================

#include <vcap/vcap.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    int index = 0;

    if (argc == 2)
        index = atoi(argv[1]);

    vcap_dev_info info = { 0 };

    int ret = vcap_enum_devices(index, &info);

    if (ret == VCAP_ENUM_ERROR)
    {
        printf("Error while enumerating devices\n");
        return -1;
    }

    if (ret == VCAP_ENUM_INVALID)
    {
        printf("Error: Unable to find a video capture device\n");
        return -1;
    }

    vcap_dev* vd = vcap_create_device(info.path, true, 0);

    // Open device
    ret = vcap_open(vd);

    if (ret == -1)
    {
        printf("%s\n", vcap_get_error(vd));
        return -1;
    }

    // Dump info
    if (vcap_dump_info(vd, stdout) == -1)
        printf("%s\n", vcap_get_error(vd));

    // Close device
    vcap_close(vd);

    vcap_destroy_device(vd);

    return 0;
}
