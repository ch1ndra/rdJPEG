# rdJPEG
rdJPEG is a small cross-platform C library to read and convert JPEG images

# Features
+ Small and portable
+ Code is heavily commented to explain the entire decoding process
+ Can be easily linked with other C programs
+ New features, functionalites and extensions can be easily added
+ A small utility program is also included to convert jpeg images to bmp images

# Limitations
- Only sequential JPEGs are supported at this time
- Arithematic coding is not supported due to patent issues

# Requirements
* Requires GCC (and the accompanying toolchain)
* Requires GNU make

# To compile/build the jpeg library, run
make jpglib

# To also compile/build the utility program (jpg2bmp) run
make all

# That's all folks.
