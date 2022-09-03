# Vcap (1.0)

## Introduction

Vcap aims to provide a concise API for working with cameras and other video capture devices that have drivers implementing the Video for Linux API (V4L2) specification. It is built on top of the libv4l userspace library (the only dependency) which provides seamless decoding for a variety of formats.

Vcap provides low-level, yet simple access to device formats and controls, enabling applications to make use of the full range of functionality provided by V4L2.

There are iterators for formats, frame sizes, frame rates, controls, and control menu items. These are especially helpful when contructing user interfaces that use Vcap.

Two I/O modes are available: MMAP buffers (streaming) and direct read. Streaming should have better performance for those devices that support it. The mode is set in the `vcap_create_device` function using the `buffer_count` parameter. A value greater than zero indicates streaming mode whereas read mode is used otherwise. Generally `buffer_count` should have a value in the range 2-4.

There is extensive error checking. Any function that can fail in release mode should do so gracefully and provide an error message detailing the underlying cause. There are also plenty of debug mode assertions.

A significant amount documentation is provided in the header and can be built as HTML using Doxygen. There are also a few examples the demonstrate how to work with the library. 

Opening an issue is welcome for any bugs or trouble using the library. Pull requests will be reviewed and possibly merged, but please raise an issue before making one first.

For a more or less complete example see [Vcap Qt](https://github.com/sonicpulse/vcap-qt)

## Features

* MIT licensed
* Written in C99, but also compiles cleanly as C++11
* Only two files for easy integration into any build system. Can also be built as a library (shared and/or static)
* Simple enumeration and handling of video devices and related information
* Streaming and read modes are supported
* Iterators for formats, frame sizes, frame rates, controls, and control menu items
* Simple get/set functions for managing camera state
* Ability to retrieve details about formats and controls
* Extensive error checking in both debug and release modes
* Examples and lots of documentation

## Building and Installation

To install the the required libv4l dependency, run:

*$ sudo apt install libv4l-dev*

You must have also have CMake and a C99+ compiler installed. To build run:

*$ mkdir vcap-build && cd vcap-build*

*$ cmake ../vcap && make*

To install, run:

*$ sudo make install*

To generate documentation (if Doxygen is installed), run:

*$ make docs*

There are a few examples available. To build the "info" example use the following *cmake* command and run *make* again:

*$ cmake ../vcap -DBUILD\_INFO\_EXAMPLE=ON*

Other examples are built similarly using `BUILD_CAPTURE_EXAMPLE` and `BUILD_SDL_EXAMPLE`.

## Example

A minimal example (without error checking) of capturing an image:

```c
#include <vcap.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    vcap_device_info info;

    // Find first device on the bus
    vcap_enumerate_devices(0, &info);

    // Create device
    vcap_device* vd = vcap_create_device(info.path, true, 0); // Use read mode

    // Open device
    vcap_open(vd);

    // Set format to RGB24
    vcap_size size = { 640, 480 };
    vcap_set_format(vd, VCAP_FMT_RGB24, size);

    // Allocate a buffer for the image
    size_t image_size = vcap_get_image_size(vd);
    uint8_t image_data[image_size];

    // Capture an image
    vcap_capture(vd, image_size, image_data);

    // Do something with the image_data...

    // Close and destroy device 
    vcap_destroy_device(vd); // Calls vcap_close and then deallocates memory 
                             // associated with 'vcap_device'

    return 0;
}
```

## Acknowledgements
I would like to thank Gavin Baker (author of [libfg](http://antonym.org/libfg/)) and Matthew Brush (author of [libfg2](https://github.com/codebrainz/libfg2)). Although Vcap is different in many ways, I found their approach inspiring.

## License
Copyright (c) 2022 James McLean <br/>
Licensed under the MIT license
