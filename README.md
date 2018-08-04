# Vcap

## Introduction

Vcap aims to provide a concise API for working with cameras and other video capture devices that have drivers implementing the V4L2 spec. It is built on top of the libv4l userspace library (the only required dependency) which provides seamless decoding for a variety of formats.

Vcap is built with performance in mind and thus minimizes the use of dynamic memory allocation. Vcap provides simple, low-level access to device controls, enabling applications to make use of the full range of functionality provided by V4L2.

This is the second iteration of Vcap, the previous version was written in 2015 when I was still a fledgling C/C++ developer. This version contains many improvements including of how formats and controls are enumerated (iterators!) and how memory is managed. Enjoy!

## Building and Installation

You must have CMake and a C compiler installed.

*$ mkdir vcap-build && cd vcap-build*

*$ cmake ../vcap && make*

To install:

*$ sudo make install*

To generate documentation (if Doxygen is installed):

*$ make docs*

There are a variety of examples available. To build the PNG example use the
following *cmake* command and run *make* again:

*$ cmake ../vcap -DBUILD\_PNG\_EXAMPLE=ON*

Other examples are built similarly.

## Example

A minimal example of grabbing a raw frame and saving it to a file (without error checking):

```c
#include <vcap/vcap.h>
#include <stdio.h>

int main(int argc, char** argv) {
    vcap_device device;

    // Find first device on the bus
    vcap_enum_devices(&device, 0);

    // Open device
    vcap_fg* fg = vcap_open(&device);

    // Set format to RGB24
    vcap_set_fmt(fg, V4L2_PIX_FMT_RGB24, size);

    // Allocate a frame
    vcap_frame* vcap_frame = vcap_alloc_frame(fg);

    // Grab a frame
    vcap_grab(fg, frame);

    // Save raw frame
    File* file = fopen("out.raw", "w");
    fwrite(frame->data, frame->length, 1, file);
    fclose(file);

    // Free frame
    vcap_free_frame(frame);

    // Close device
    vcap_close(fg);

    return 0;
}
```

## Acknowledgements
I would like to thank Gavin Baker (author of [libfg](http://antonym.org/libfg/)) and Matthew Brush (author of [libfg2](https://github.com/codebrainz/libfg2)).
Although Vcap is different in many ways, I found their approach inspiring when writing the second iteration of Vcap.

## License
Copyright (c) 2018 James McLean  
Licensed under LGPL v2.1.
