# Hybo iLidar (HIL) Demo Repository

This repository contains some examples of communicating with the iLidar
sensor using the provided API.  These examples can be run on Windows,
Linux, or MacOS (see Note 1).

Look for additional examples coming soon for other platforms...

These programs are meant to be used as a playground for experimenting
with the iLidar data, and also as a way to begin rapid prototyping of
systems using the iLidar.

## API

The API functions which are used to control and get data from the sensor
are outlined in the header file `ilidarlib.h`.  Descriptions of each
function is contained therein.

In some applications, the data from the sensor will be coming at high
speeds.  To assist the application in consuming this data, the API
breaks up the reading of sensor packets into two functions:
`hil_frame_grabber()` and `hil_frame_reader()`.  These can be called
in-line, as in these examples below, for super-loop style of operation.
Or the `hil_grabber()` function can be put in a separate thread
from the consumer `hil_frame_reader()` function.

NOTE: A threaded example `rdsensor_t.cpp` to be posted soon.


## Build Tools

These examples are written in C, and intended to be run from the command
line.  A Microsoft Visual Studio 2019 solution and project files are also provided.

You will need standard C compiler and `make` utility for your system installed.  
Check the `Makefile` and edit accordingly.

## Dependencies

These tools utilize the Serial Port library from project Sigrok:

* [https://github.com/martinling/libserialport](https://github.com/martinling/libserialport)

This is a general purpose, multiplatform serial port library used
internally in the Sigrok project, and made available as a stand-alone
project.

Install `libserialport` on your system per the instructions provided.

* For Linux and MacOS, the standard final installation command `make install` is all that is needed.
* On Windows, you need to:
    - point to `libserialport.lib` from the `Makefile` and/or VS2019 project file
    - copy `libserialport.dll` to the `C:\Windows\System32\` directory


## iLidar API Demo Installation

Clone the repository to your local machine.

Check the `Makefile` and edit as necessary for the proper library locations
(if applicable, check the VS2019 project file as well).

*  `$ make clean`   clean output directories
*  `$ make rdsensor`  build program `rdsensor`
*  `$ make logsensor` build program `logsensor`

## Running the programs

### `rdsensor`

This program opens the sensor, reads data frames and saves every 50th
frame to a file `xyzdata.txt`, overwriting the previous contents.  It
also displays a one-line summary of each frame received to `stdout`.


### `logsensor`

This program opens the sensor, reads data frames and every frame to file
which is specified on the command line.

Both programs run for 100 frames then quit.





Note 1:  There is currently an issue running this program from within
WSL on Windows, due to the serial port library.  
