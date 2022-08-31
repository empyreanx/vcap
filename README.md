# Vcap (1.0)

## Introduction

Vcap aims to provide a concise API for working with cameras and other video capture devices that have drivers implementing the Video for Linux API (V4L2) specification. It is built on top of the libv4l userspace library (the only dependency) which provides seamless decoding for a variety of formats.

Vcap provides simple, low-level access to device controls, enabling applications to make use of the full range of functionality provided by V4L2.

## Features

* MIT licensed
* Written in C99
* Compiles cleanly as C++11
* Two files for easy integration into any build system. Can also be built as a library (shared or static)
* Simple enumeration and handling of video devices
* Streaming and read modes are supported
* Iterators for enumerating formats, frame sizes, frame rates, controls, and control menu items
* Simple get/set functions for managing camera state
* Ability to retrieve details about formats and controls
* Extensive error checking and reporting

## Improvements

This is the third iteration of Vcap, the previous versions were written in 2015 and 2018. This version contains many improvements, both internally and to the API. Among these are: 

* Vcap is now licensed under the MIT license.
* The source and header files of the previous version have been consolidated into a two files. 
* Code that relied on external dependencies for import/export of settings, and saving frames to PNG/JPEG has been removed for portability, simplicity, and brevity.
* Both streaming and read modes are now supported.
* Code for allocating/manipulating structs containing image buffer data has been eliminated in favor of direct allocation of image data buffers (on the stack or heap) using `vcap_get_image_size` to get the appropriate size.
* Format/control iterators are no longer allocated on the heap.
* Error messages are now device specific.

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

A minimal example (without error checking) of grabbing a frame:

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
