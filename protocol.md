---
colorlinks: true
urlcolor: blue
geometry: margin=2cm
---

# Protocol Description: iLidar High Speed Sensor Data

_Rev 15 May 2020_

## Types of data

*NOTE: Evaluation units only send a single type of data packet.  See
below....*

Depending on the application, several different kinds of data can be
provided.  This information is available to be streamed each frame, and 
can be turned on or off by user configuration.  

  * Distance data can be 1D, 2D, 3D, or turned off
  * Intensity data can be absolute, reflective, or turned off
  * Precision data can be on or off
  * Housekeeping data can be on or off

A 16-bit flag word begins the data block, and bit fields in this word 
describe what kind(s) of data will follow:

  * Bits 1:0 (distance data)
    - 00:  off
    - 01:  1D
    - 10:  2D
    - 11:  3D
  * Bits 3:2 (intensity data)
    - 00:  off
    - 01:  absolute
    - 10:  reflective
    - 11:  (reserved)
  * Bit 4: (precision data)
    - 0: off
    - 1: on
  * Bit 5: (housekeeping data)
    - 0: off
    - 1: on

### Distance data

  * 3D distance data
  * 2D distance data
  * 1D distance data

### Intensity data

  * intensity data, absolute or 
  * intensity data, reflective

### Precision data

  * subpixel data

### Housekeeping data

  * time of the most recent peak
  * time of the most recent IMU reading
  * laser diode temperature
  * system resources status
  * input voltage
  * gyroscope reading
  * accelerometer reading

## Frame wrapper

For transmitting the data over the communication link, the block of data described above 
is encapsulated in a frame wrapper as follows:

  * sync pattern
  * frame size
  * sequential frame number
  * (data block)
  * checksum

## C-Language Pseudo-Header File

The following structure definition is meant to explain the concept of the data format.
It is NOT a real structure which should be used in a program, because the frame sizes 
vary depending on what mix of sensor data the user has requested.


```c
#include <stdint.h>
#include <stdbool.h>

#define MAXPOINTSS (240)

typedef struct tagFRAME_CONCEPT {

    uint8_t sync[4];
    uint16_t size;

    struct tagDATABLOCK {

        uint16_t seq;

        struct tagDATATYPE {
          unsigned int depth : 2;
          unsigned int intensity : 2;
          unsigned int precision : 1;
          unsigned int housekeeping : 1;
          unsigned int reserved : 10;
        } type;

        struct tagHOUSEKEEPING {
          double timePeak;
          double timeIMU;
          float tempLD;
          float sysResc;
          float sysV;
          float gyro[3];
          float acc[3];
        } housekeeping;

        uint16_t precision[MAXPOINTS];

        uint8_t intensity_absolute[MAXPOINTS];
        uint8_t intensity_reflective[MAXPOINTS];

        int16_t distance_3D[MAXPOINTSS][3];
        int16_t distance_2D[MAXPOINTSS][2];
        int16_t distance_1D[MAXPOINTSS][1];

        uint16_t cksum;

    } block;

} frame;
```

## C-Language Protocol Demo Header File

For the demo firmware being prepared this week, the format will be fixed
to 3D distance data only.  In this case, it IS possible to write a
C language header file that correctly describes the frame, since it
won't be changing.


```c
// the demo prototype units only have only 3D distance data,
// the data frame looks like this.

typedef struct tagHIL_PAYLOAD {
  uint16_t seq;
  uint16_t datatype;
  HIL_TIMESTAMP timePeak;
  HIL_TIMESTAMP timeIMU;
  int16_t pts[HIL_MAXPOINTS][3];
  uint32_t cksum;
} HIL_PAYLOAD;
#define HIL_PAYLOADSIZE ( (uint32_t)sizeof( HIL_PAYLOAD ) )

typedef struct tagHIL_FRAME {
  uint8_t sync[HIL_FRAME_SIZEOF_SYNC];
  uint32_t size;
  union {
    uint8_t data[HIL_PAYLOADSIZE];
    HIL_PAYLOAD p;
  };
} HIL_FRAME;
#define HIL_FRAMESIZE ( (uint32_t)sizeof( HIL_FRAME ) )
```




