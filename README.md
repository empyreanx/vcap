# VCap (V4L2 Capture Library)
 
### Introduction

VCap (the V4L2 Capture Library) is a library written in C that aims to make using the Video4Linux2 API
simple. It was designed to make the common case of capturing frames using memory-mapped device buffers quick and easy.
VCap was also written with computer vision applications in mind and thus allows setting camera
controls that enhance or interfere with computer vision and image processing techniques. VCap also includes
a (growing) number of pixel format decoding functions, enabling seamless support for many devices. In contrast
with similar projects, VCap is licensed under the LGPL v2.1 which permits it's use in commercial products.
	
### Building

You make have CMake and a C compiler installed. 

*$ cmake . && make*

To install:

*$ make install*

To generate documentation (if Doxygen is installed):

*$ make doc*
