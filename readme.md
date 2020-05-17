# Hybo iLidar (HIL) Demo Repository

This repository contains some examples of communicating with the iLidar
sensor using the provided API.  These examples have been tested on Windows,
Linux, Raspberry Pi, or MacOS (see Note 1).

Look for additional examples coming soon for other languages and platforms...

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
Note: build each project separately.  TODO: carve-out the library to
enable building the entire solution.

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
    - the VS2019 project files point to the project's `lib\serial` folder by default


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
For example:

```
C:\files\api-demo> bin\rdsensor
Usage:
  rdsensor serial-port [baud] [timeout]
Found port: 'COM3'
Found port: 'COM4'

C:\files\api-demo> bin\rdsensor COM4 1500000
baud string: 2, 1500000
Connected to sensor, port COM4 at 1500000 baud with timeout 50000 msec
iloop:216, nf:0, seq=26809
XYZ written
iloop:217, nf:1, seq=26810
iloop:218, nf:2, seq=26811
iloop:219, nf:3, seq=26812
iloop:220, nf:4, seq=26813
iloop:221, nf:5, seq=26814
iloop:222, nf:6, seq=26815
iloop:223, nf:7, seq=26816
iloop:224, nf:8, seq=26817
...etc...
```


### `logsensor`

This program opens the sensor, reads data frames and every frame to file
which is specified on the command line.  For example:


```
C:\files\api-demo> bin\logsensor
Usage:
  logsensor serial-port logfile [baud]
  note:
    these are the currently connected serial ports:
    Found port: 'COM3'
    Found port: 'COM4'

C:\files\api-demo>bin\logsensor COM4 out.dat
1500000
baud string: 3, 1500000
Connected to sensor, port COM4 at 1500000 baud with timeout 50000 msec
iloop:217, nf:0, size=2904, seq=35125, datatype=f001
iloop:218, nf:1, size=2904, seq=35126, datatype=f001
iloop:219, nf:2, size=2904, seq=35127, datatype=f001
iloop:220, nf:3, size=2904, seq=35128, datatype=f001
iloop:221, nf:4, size=2904, seq=35129, datatype=f001
iloop:222, nf:5, size=2904, seq=35130, datatype=f001
iloop:223, nf:6, size=2904, seq=35131, datatype=f001
iloop:224, nf:7, size=2904, seq=35132, datatype=f001
iloop:225, nf:8, size=2904, seq=35133, datatype=f001
iloop:226, nf:9, size=2904, seq=35134, datatype=f001
iloop:227, nf:10, size=2904, seq=35135, datatype=f001
iloop:228, nf:11, size=2904, seq=35136, datatype=f001
iloop:229, nf:12, size=2904, seq=35137, datatype=f001
iloop:230, nf:13, size=2904, seq=35138, datatype=f001
iloop:231, nf:14, size=2904, seq=35139, datatype=f001
iloop:232, nf:15, size=2904, seq=35140, datatype=f001
iloop:426, nf:16, size=2904, seq=35141, datatype=f001
iloop:427, nf:17, size=2904, seq=35142, datatype=f001
iloop:428, nf:18, size=2904, seq=35143, datatype=f001
...etc...
```

Both programs run for 100 frames then quit.




Note 1:  There is currently an issue running this program from within
WSL on Windows, due to the serial port library.  
