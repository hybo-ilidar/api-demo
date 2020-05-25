#ifndef ILIDAR_H
#define ILIDAR_H

//************************************************************************
//  Hybo iLidar C Language Library, Application Programming Interface
//************************************************************************

// API types use:
// #include <stdint.h>
// #include <stdbool.h>

#include "libserialport.h"

//================================================
// structure typedefs
//================================================
 
// Communications channel to sensor

#define HIL_MAXNAMESIZE (128)
typedef struct tagHIL_PORT { 
  char portname[HIL_MAXNAMESIZE];
  struct sp_port *sp_handle;
  int baudrate;
  int timeout;
} HIL_PORT;

typedef struct tagHIL_SENSOR {  // TBD
  HIL_PORT *port;
} HIL_SENSOR;

// Data packets from sensor
//
// Here are individual pieces of a data packet/frame from the sensor

#define HIL_MAXPOINTS (480)
#define HIL_FRAME_SYNC_PATTERN  { 0x5a, 0xa5, 0x5a, 0xa5 }
#define HIL_FRAME_SIZEOF_SYNC (4)
#define HIL_FRAME_SIZEOF_SIZE (4)
#define HIL_FRAME_SIZEOF_SEQ   (2)
#define HIL_FRAME_SIZEOF_CKSUM (4)

// every frame begins with this header...
typedef struct tagHIL_FRAME_HEADER {
    uint8_t sync[HIL_FRAME_SIZEOF_SYNC];
    uint32_t size;
} HIL_FRAME_HEADER;

typedef struct tagHIL_DATATYPE {
  unsigned int depth : 2;
  unsigned int intensity : 2;
  unsigned int precision : 1;
  unsigned int housekeeping : 1;
  unsigned int reserved : 10;
} HIL_DATATYPE;

typedef struct tagHIL_DATA_HEADER {
  uint16_t seq;
  HIL_DATATYPE datatype;
} HIL_DATA_HEADER;

typedef struct tagHIL_TIMESTAMP {
  uint32_t seconds;
  uint32_t micros;
} HIL_TIMESTAMP;

typedef struct tagHIL_HOUSEKEEPING {
  HIL_TIMESTAMP timePeak;
  HIL_TIMESTAMP timeIMU;
  float tempLD;
  float sysResc;
  float sysV;
  float gyro[3];
  float acc[3];
} HIL_HOUSEKEEPING;

typedef struct tagHIL_PRECISION {
  int16_t precision[HIL_MAXPOINTS];
} HIL_PRECISION;

typedef struct tagHIL_INTENSITY_ABS {
  uint8_t intensity_absolute[HIL_MAXPOINTS];
} HIL_INTENSITY_ABS;

typedef struct tagHIL_INTENSITY_REF {
  uint8_t intensity_reflective[HIL_MAXPOINTS];
} HIL_INTENSITY_REF;

typedef struct tagHIL_DISTANCE_3D {
  int16_t distance_3D[HIL_MAXPOINTS][3];
} HIL_DISTANCE_3D;

typedef struct tagHIL_DISTANCE_2D {
  int16_t distance_2D[HIL_MAXPOINTS][2];
} HIL_DISTANCE_2D;

typedef struct tagHIL_DISTANCE_1D {
  int16_t distance_1D[HIL_MAXPOINTS][1];
} HIL_DISTANCE_1D;

typedef struct tagHIL_DATA_TRAILER {
  uint32_t cksum;
} HIL_DATA_TRAILER;

// Depending on the configuration, some combination of the above
// structures will be combined to make a total frame

// for the demo prototype unit, having only 3D distance data,
// the data frame looks like this.


typedef struct tagHIL_PAYLOAD {
  uint16_t seq;
  uint16_t datatype;
  HIL_TIMESTAMP timePeak;
  HIL_TIMESTAMP timeIMU;
  int16_t pts[HIL_MAXPOINTS][3];
  uint32_t cksum;
} HIL_PAYLOAD;
#define HIL_PAYLOADSIZE ( sizeof( HIL_PAYLOAD ) )

typedef struct tagHIL_FRAME {
  uint8_t sync[HIL_FRAME_SIZEOF_SYNC];
  uint32_t size;
  union {
    uint8_t data[HIL_PAYLOADSIZE];
    HIL_PAYLOAD p;
  };
} HIL_FRAME;
#define HIL_FRAMESIZE ( sizeof( HIL_FRAME ) )

//================================================
// enumerations
//================================================

// TBD
// enum tagHIL_ERRORS{ /*****/ } HIL_ERRORS;

typedef enum tagHIL_IO_MODE { 
  HIL_USB_ASYNC,
  HIL_USER_ASYNC,
  HIL_USER_SPI,
  HIL_USER_I2C,
} HIL_IO_MODE;

typedef enum tagHIL_DISTANCE_TYPE {
  HIL_DIST_OFF,
  HIL_DIST_1D,
  HIL_DIST_2D,
  HIL_DIST_3D,
} HIL_DISTANCE_TYPE;

typedef enum tagHIL_INTENSITY_TYPE {
  HIL_ITEN_OFF,
  HIL_ITEN_ABS,
  HIL_ITEN_REF,
} HIL_INTENSITY_TYPE;

typedef enum tagHIL_PRECISION_TYPE {
  HIL_PREC_OFF,
  HIL_PREC_ON,
} HIL_PRECISION_TYPE;

typedef enum tagHIL_HOUSEKEEPING_TYPE {
  HIL_HOUSE_OFF,
  HIL_HOUSE_ON,
} HIL_HOUSEKEEPING_TYPE;

//================================================
// function prototyes
//================================================

//--------------------------------
// port and sensor operations
//--------------------------------

// gets the error and description of the last call
int hil_last_error( void );
char *hil_error_description( int ierr );

// opens the communications port
// input: port name, e.g., 'com6', 'dev/ttyAMA0', etc.,
// output: pointer to port structure, NULL if error
HIL_PORT *hil_port_open( HIL_PORT *hp, char *portname, int baudrate, int timeout );

// closes the communications port
// input: pointer to port structure
// output: none
void hil_port_close( HIL_PORT *pPort );

void hil_port_enumerate(FILE *fp);

// connects to the iLidar sensor
// input: port structure
// output: pointer to sensor structure, NULL if error
HIL_SENSOR *hil_sensor_open( HIL_PORT *port );

// closes the connection to the sensor
// input: pointer to sensor structure
// output: none
void hil_sensor_close( HIL_SENSOR *pSensor );


//************************************************************************
// NOTE: configuration funcs are not supported in the evaluation units.
//************************************************************************

//--------------------------------
// configuration, comms
//--------------------------------

void hil_io_mode_set( HIL_SENSOR *pSensor, HIL_IO_MODE mode );
HIL_IO_MODE hil_io_mode_get( HIL_SENSOR *pSensor );

void hil_baudrate_set( HIL_SENSOR *pSensor, int baud );
int hil_baudrate_get( HIL_SENSOR *pSensor );

//--------------------------------
// configuration, sensor
//--------------------------------

// controls the sensor frame rate (frames per second)
void hil_frame_rate_set( HIL_SENSOR *pSensor, int fps );
int hil_frame_rate_get( HIL_SENSOR *pSensor );

// controls the sending of packets from the sensor
void hil_packet_control_set( HIL_SENSOR *pSensor, bool on );
bool hil_packet_control_get( HIL_SENSOR *pSensor );

// controls the type of distance data in the packets
void hil_packet_distance_set( HIL_SENSOR *pSensor, HIL_DISTANCE_TYPE );
HIL_DISTANCE_TYPE hil_packet_distance_get( HIL_SENSOR *pSensor );

// controls the type of intensity data in the packets
void hil_packet_intensity_set( HIL_SENSOR *pSensor, HIL_INTENSITY_TYPE );
HIL_INTENSITY_TYPE hil_packet_intensity_get( HIL_SENSOR *pSensor );

// controls the type of precision data in the packets
void hil_packet_precision_set( HIL_SENSOR *pSensor, HIL_PRECISION_TYPE );
HIL_PRECISION_TYPE hil_packet_precision_get( HIL_SENSOR *pSensor );

// controls the type of housekeeping data in the packets
void hil_packet_housekeeping_set( HIL_SENSOR *pSensor, HIL_HOUSEKEEPING_TYPE );
HIL_HOUSEKEEPING_TYPE hil_packet_housekeeping_get( HIL_SENSOR *pSensor );



typedef enum tagHIL_COUNTER {
  HIL_BYTES=0,
  HIL_FRAMES,
  HIL_GOODSYNCS,
  HIL_LOSTSYNCS,
  HIL_ERRSIZE,
  HIL_ERRCKSUM,
  HIL_INVALID,
} HIL_COUNTER;

typedef enum tagGRABBER_ACTION {
  HIL_GRABBER_INIT=0,
  HIL_GRABBER_GRAB,
  HIL_GRABBER_CLEANUP,
} GRABBER_ACTION;

void hil_frame_grabber( HIL_PORT *hp, GRABBER_ACTION action );
HIL_FRAME *hil_frame_reader( HIL_PORT *hp );

bool hil_grabber_errors_have_changed( void );
bool hil_grabber_counters_have_changed( void );
uint32_t hil_grabber_counter_get_value( HIL_COUNTER cntr );
void hil_grabber_counters_print_all( FILE *fp );
void hil_grabber_counters_print_all_detailed( FILE *fp );

//--------------------------------
// packet data from sensor
//--------------------------------

// reads (blocking) a frame packet from the sensor
// output: pointer to structure, 
//         interpretation depends on the requested data type
// returns: size of data returned in the packet
//          zeor if there was a timeout while waiting
int hil_packet_read_blocking( HIL_SENSOR *pSensor, void *packet );

int hil_packet_read_non_blocking( HIL_SENSOR *pSensor, void *packet );

// these 
//void *hil_packet_data_get( void *packet );
//void *hil_packet_intensity_get( void *packet );
//void *hil_packet_precision_get( void *packet );

#endif
