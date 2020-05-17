#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "libserialport.h"
#include "ilidarlib.h"

//------------------------------------------------------------------------
// Grabber state machine variables and enums
//------------------------------------------------------------------------
// Enum only defined here, 
// function is public and prototype found in api.h
// associated variables are all within the function

typedef enum tagSTATE {
  NOSYNC = 0,
  SYNCSEARCH0,
  SYNCSEARCH1,
  SYNCSEARCH2,
  SYNCSEARCH3,
  GETSIZE,
  GETSEQ,
  GETDATA,
  GETCKSUM,
  NUMSTATES 
} STATE;

//------------------------------------------------------------------------
// General purpose counter variables, enums, and prototypes
//------------------------------------------------------------------------

typedef enum tagCOUNTER {
  BYTES=0,
  FRAMES,
  GOODSYNCS,
  LOSTSYNCS,
  ERRSIZE,
  ERRCKSUM,
  INVALID,
	WRITE_OVERRUN,
	BUFFER_PROCESSED,
	FRAME_PROCESSED,
  NUM_COUNTERS
} COUNTER;

uint32_t counter[NUM_COUNTERS];
bool counter_has_changed[NUM_COUNTERS];
const char *counter_descr[NUM_COUNTERS] = {
  "Total bytes",
  "Good frames",
  "Good syncs",
  "Lost syncs",
  "Size errors",
  "Cksum errors",
  "Invalid errors",
  "write overrun",
  "buffer processed",
  "frame processed",
};

// The functions controlling the counters are static
static void counters_reset( void );
static void counters_clear_changes( void );
static void errors_clear_changes( void );
static void counter_incr( COUNTER cntr );

static bool errors_have_changed( void );
static bool counters_have_changed( void );
static uint32_t counter_get_value( COUNTER cntr );
static void counters_print_all( FILE *fp );
static void counters_print_all_detailed( FILE *fp );




//------------------------------------------------------------------------
// Serial port shim function prototypes, variables, and enums
//------------------------------------------------------------------------
static void hil_serialListPorts(FILE *fp);
static int hil_serialOpen(HIL_PORT *hp, char port[], int baud);
static int hil_serialClose(HIL_PORT *hp);
static int hil_serialFlushBoth(HIL_PORT *hp);
static int hil_setreadtimeout(HIL_PORT *hp, int initialtimeout);
static short hil_serialReadByte(HIL_PORT *hp);
static int hil_serialReadBlock(HIL_PORT *hp, unsigned char * dst, int size, int maxsize, bool *timedout);
static int hil_serialWriteByte(HIL_PORT *hp, unsigned char c);
static int hil_serialWriteBlock(HIL_PORT *hp, unsigned char * src, int size);

static float hil_serialReadFloat(HIL_PORT *hp);
static int hil_serialReadInt(HIL_PORT *hp);
static short hil_serialReadShort(HIL_PORT *hp);
static int hil_serialWriteFloat(HIL_PORT *hp, float src);
static int hil_serialWriteInt(HIL_PORT *hp, int src);
static int hil_serialWriteShort(HIL_PORT *hp, short src);

static int hil_errno;
static int vb=0; // verbosity, 0=silent

// opens the communications port
// input: port name, e.g., 'com6', 'dev/ttyAMA0', etc., 
//        baudrate, and timeout in msec
//        HIL_PORT structure
// output: port structure is populated with data,
//         returns NULL if error,
//         returns address of HIL_PORT if success
HIL_PORT *hil_port_open( HIL_PORT *hp, char *portname, int baudrate, int timeout ) {
  int err=hil_serialOpen( hp, portname, baudrate );
  if (err!=0) { // did it succeed?
    hil_errno=err;
    return NULL;
  }
  strncpy( hp->portname, portname, HIL_MAXNAMESIZE );
  hp->portname[HIL_MAXNAMESIZE-1]=0;
  hp->baudrate = baudrate;
  hp->timeout = timeout;
  hil_serialFlushBoth(hp);
  return hp;
}

// closes the communications port
// input: pointer to port structure
// output: none
void hil_port_close( HIL_PORT *hp ) {
  hil_serialClose(hp);
}



// serial port read buffer
#define BUFSIZE (1024)
uint8_t buf[BUFSIZE];
// ping-pong buffer
#define MAX_PINGPONG_FRAMES (16)
HIL_FRAME frame, framebuf1[MAX_PINGPONG_FRAMES], framebuf2[MAX_PINGPONG_FRAMES];
HIL_FRAME *pf = framebuf1;
HIL_FRAME *pfread = NULL;


// Frame grabber state machine
//
// Purpose:  
//   * reads the serial data stream from the sensor,
//   * aligns to the sync patterns in the byte stream, 
//   * confirm valid frames (sync, size, and checksum), and 
//   * buffers N frames in a ping-pong buffer,
// The application reads (hopefully) error-free frame data 
// from the ping pong buffer, not having to worry about the
// details of the serial format nor the framing method.
//
// This function must be called continuously within the 
// super loop of the main program
//
void hil_frame_grabber( HIL_PORT *hp, GRABBER_ACTION action ) {
  static bool looping = true;
  static int waiting=0;
  static int nrb=0;
  static uint32_t cksum=0; // total checksum of everything, for debug testing
  static uint32_t cksum2 = 0; //locally calculated checksum
  static uint8_t ch;
  static bool firstsync = false;
  static bool synced = false;
  static bool isready = false;
  static uint32_t frameno = 0;
  static uint32_t iframe = 0;
  static uint32_t index = 0;
  static STATE state = NOSYNC;
  static STATE nextstate;
  static STATE laststate;
  static bool finished = false;
  static bool triggered = false;
  static int safety=0;

  if(action == HIL_GRABBER_INIT) {
    counters_reset();
  } else if(action == HIL_GRABBER_GRAB) {
    nrb=hil_serialReadBlock(hp, buf, 200, BUFSIZE, &finished);
    //printf("%d ",nrb);
    for(int i=0;i<nrb;i++) { // Grabber
      // fprintf(stderr,"nrb, buf[%d]: %d %02x\n", i, nrb, buf[i]);
      ch = buf[i];
      counter_incr( BYTES ); // number of total bytes
      cksum += ch; // overall checksum
      switch(state) {
        case NOSYNC:
          synced = false;
          if (firstsync) counter_incr(LOSTSYNCS);
        case SYNCSEARCH0:
          if (ch == 0x5A) {
            nextstate = SYNCSEARCH1;
          }
          else {
            if(synced) nextstate = NOSYNC;
            else nextstate = SYNCSEARCH0;
          }
          break;
        case SYNCSEARCH1:
          if (ch == 0xA5) {
            nextstate = SYNCSEARCH2;
          }
          else nextstate = NOSYNC;
          break;
        case SYNCSEARCH2:
          if (ch == 0x5A) {
            nextstate = SYNCSEARCH3;
          }
          else nextstate = NOSYNC;
          break;
        case SYNCSEARCH3:
          if (ch == 0xA5) {
            nextstate = GETSIZE;
            counter_incr(GOODSYNCS);
            firstsync = true;
            synced = true;
            // fprintf(stderr, "sync found\n");
          }
          else nextstate = NOSYNC;
          break;
        case GETSIZE: //Frame size 
          if (state != laststate) {
            frame.size = 0;
            index = 0;
          } 
          frame.size = (ch<<(index*8)) + frame.size;
          index++;
          if (index >= HIL_FRAME_SIZEOF_SIZE) {
            // note: sequence and checksum now belong in the data block,
            // but are required fields and still checked here
            // RCLOTT 19 Feb 2020, final production protocol design
            // frame.size = frame.size;
            // fprintf(stderr, "frame size read, actual: %d, %d\n", frame.size, (int)HIL_PAYLOADSIZE );
            if (frame.size != HIL_PAYLOADSIZE) {
              nextstate = SYNCSEARCH0;
              synced = false;
              counter_incr(ERRSIZE);
            } else nextstate = GETSEQ;
          }
          break;
        case GETSEQ: //counter
          if (state != laststate) {
              frame.p.seq = 0;
              index = 0;
              cksum2 = 0;
          }
          frame.p.seq = (ch<<(index*8)) + frame.p.seq;
          cksum2 += ch;
          index++;
          if (index >= HIL_FRAME_SIZEOF_SEQ) {
            nextstate = GETDATA;
            // clear the payload, but don't wipe out the seq number...
            memset(frame.data+HIL_FRAME_SIZEOF_SEQ, 0, HIL_PAYLOADSIZE-HIL_FRAME_SIZEOF_SEQ);
            // fprintf(stderr, "frame seq: %04x\n", frame.p.seq);
          }
          break;
        case GETDATA:
          if (state != laststate) index = 0;
          frame.data[index+2] = ch;
          cksum2 += ch;
          index++;
          // NOTE: the 'size-6' calculation below accounts for the
          // sequence (2 bytes) and cksum (4 bytes) variables, which are processed in their
          // own within this state machine
          if( index >= frame.size - HIL_FRAME_SIZEOF_SEQ - HIL_FRAME_SIZEOF_CKSUM ) nextstate = GETCKSUM;
          break;
        case GETCKSUM:
          if (state != laststate) {
            frame.p.cksum = 0;
            index = 0;
          }
          frame.p.cksum = (ch<<(index*8)) + frame.p.cksum;
          index++;
          if (index >= HIL_FRAME_SIZEOF_CKSUM) {
            // fprintf(stderr, "cksum, frame: %08x calc: %08x\n", frame.p.cksum, cksum2);
            if( frame.p.cksum == cksum2 ) {
              isready = true;
              counter_incr(FRAMES );
            } else {
              counter_incr(ERRCKSUM);
            }
            nextstate = SYNCSEARCH0;
          }
          break;
        default:
          nextstate = NOSYNC;
          counter_incr(INVALID);
          break;
      }
      laststate = state;
      state = nextstate;

      if( errors_have_changed() ) {
        // counters_print_all(stderr);
        // errors_clear_changes();
        // safety++;
        // if(safety > 20) exit(99);
      }

      if (isready) {
        frameno = counter_get_value(FRAMES);
        // fprintf(stderr, "GOOD, frame %d\tcksum: %04x\tcalc: %04x\n", frameno, frame.p.cksum, cksum2 );
        memcpy(pf + iframe, &frame, sizeof(frame)); 
        iframe += 1;
        if (iframe >= MAX_PINGPONG_FRAMES) { // flip the ping-pong buffers...
          if (pfread == NULL) { // only flip if other buffer has been emptied
            if (pf == framebuf1) {
              pf = framebuf2;
              pfread = framebuf1;
            } else {
              pf = framebuf1;
              pfread = framebuf2;
            }
          } else { // otherwise, write overrun, lose a buffer
            counter_incr(WRITE_OVERRUN);
            //fprintf(stderr, "overrun: %d\n", counter_get_value(WRITE_OVERRUN) );
          }
          iframe = 0;
        }
        isready = false;
      }
    }
  } else if(action == HIL_GRABBER_CLEANUP) {
    // nothing to do at this time
  }
  return;
}


HIL_FRAME *hil_frame_reader( HIL_PORT *hp ) {
  static int iframe = 0;
  HIL_FRAME *hfp=NULL;
  if (pfread) {
    hfp=pfread; // return frame pointer
    counter_incr(FRAME_PROCESSED);
    pfread++;
    iframe++;
    if( iframe >= MAX_PINGPONG_FRAMES ) {
      counter_incr(BUFFER_PROCESSED);
      pfread = NULL;
      iframe = 0;
    }
  }
  return hfp;
}

//************************************************************************
//************************************************************************
// SERIAL PORT SHIM FUNCTIONS
//------------------------------------------------------------------------
// These exist in case we wish to change the serial port functions
// in the future.  Currently this is setup to use libserial from 
// the Sigrok project.
//************************************************************************
//************************************************************************

void hil_serialListPorts(FILE *fp) {
  int i;
  struct sp_port **ports;

  enum sp_return error = sp_list_ports(&ports);
  if (error == SP_OK) {
    for (i = 0; ports[i]; i++) {
      if(fp) fprintf(fp, "Found port: '%s'\n", sp_get_port_name(ports[i]));
    }
    sp_free_port_list(ports);
  } else {
    if(fp) fprintf(fp, "No serial devices detected\n");
  }
}

// public version of above
void hil_port_enumerate(FILE *fp) {
  hil_serialListPorts(fp);
}


int hil_serialOpen(HIL_PORT *hp, char port[], int baud) {
   enum sp_return err;
   err = sp_get_port_by_name(port,&(hp->sp_handle));
   if (err!=SP_OK) return (err);
   err = sp_open(hp->sp_handle,SP_MODE_READ_WRITE);
   if (err!=SP_OK) return (err);
   err = sp_set_baudrate(hp->sp_handle, baud);
   return (err);
}

int hil_serialClose(HIL_PORT *hp) {
   return sp_close(hp->sp_handle);
}

int hil_setreadtimeout(HIL_PORT *hp, int initialTimeout) {
   hp->timeout = initialTimeout;
   return 0;
}

int hil_serialFlushBoth(HIL_PORT *hp) {
  sp_flush( hp->sp_handle, SP_BUF_BOTH );
  return 0;
}

// read byte from serial port.
// returns 0x00xx on success
// returns 0x01xx on failure
short hil_serialReadByte(HIL_PORT *hp) {
    short ch = 0;
    enum sp_return iret = sp_blocking_read(hp->sp_handle, (void *)&ch, (size_t)1, hp->timeout);
    if (iret == 1) return ch;
    return 0x0100 | ch;
}

// Request SIZE bytes, but perhaps more bytes are available on the queue
// if MAXSIZE == SIZE, then only returns what is requested.
// otherwise, try to return all bytes on the queue, up to MAXSIZE bytes
// TIMEOUT is not a real timer, but a counter which needs to be set
// according to the baud rate and magic hand-waving.  This is returned
// so the caller can know a timeout happened.  
// *Note: data can be returned AND a timeout happens, and...
// it's possible that data can still be on the queue, requiring 
// an additional call to fetch the data.
// TRIGGER allows the first call to wait infinitely, useful for
// debug testing.
// Returns NRB, the number of read bytes
int hil_serialReadBlock(HIL_PORT *hp, unsigned char * dst, int size, int maxsize, bool *timedout) {
  int waiting;
  int nqueued;
  int nrb;
  static bool triggered=false;
  int timeout_cnt=0;
  *timedout=false;
  do {
    waiting=sp_input_waiting(hp->sp_handle);
    if(triggered) {
      timeout_cnt++;
      if(timeout_cnt >= hp->timeout) {
        if(timedout) *timedout = true;
        break;
      }
    }
  } while (waiting<size);
  triggered = true; // and, we're running now
  nqueued=waiting; // save how many bytes are on the queue
  if(waiting > maxsize) waiting = maxsize;
  nrb = sp_nonblocking_read( hp->sp_handle, dst, waiting );
  // fprintf(stderr,"waiting, nrp: %d\t%d\n", waiting, nrb);
  return nrb;
}

// sends a byte to the serial port.  returns 0 on success and -1 on failure. 
// TODO return error condition
int hil_serialWriteByte(HIL_PORT *hp, unsigned char ch) {
    return sp_blocking_write(hp->sp_handle, (void *)&ch, (size_t)1, hp->timeout);
}

// sends a byte to the serial port.  returns 0 on success and -1 on failure. 
// TODO return error condition
int hil_serialWriteBlock(HIL_PORT *hp, unsigned char * src, int size) {
    return sp_blocking_write(hp->sp_handle, (void *)src, (size_t)size, hp->timeout);
}

float hil_serialReadFloat(HIL_PORT *hp) {
    float dst;
    hil_serialReadBlock(hp, (unsigned char*)&dst, 4, 4, NULL);
    return dst;
}

int hil_serialReadInt(HIL_PORT *hp) {
    int dst;
    hil_serialReadBlock(hp, (unsigned char*)&dst, 4, 4, NULL);
    return dst;
}

short hil_serialReadShort(HIL_PORT *hp) {
    short dst;
    hil_serialReadBlock(hp, (unsigned char*)&dst, 2, 2, NULL);
    return dst;
}

int hil_serialWriteFloat(HIL_PORT *hp, float src) {
    int numBytes;
    numBytes = hil_serialWriteBlock(hp, (unsigned char*)&src, 4);
    if (numBytes != 4) return -1;
    return 0;
}

int hil_serialWriteInt(HIL_PORT *hp, int src) {
    int numBytes;
    numBytes = hil_serialWriteBlock(hp, (unsigned char*)&src, 4);
    if (numBytes != 4) return -1;
    return 0;
}

int hil_serialWriteShort(HIL_PORT *hp, short src) {
    int numBytes;
    numBytes = hil_serialWriteBlock(hp, (unsigned char*)&src, 2);
    if (numBytes != 2) return -1;
    return 0;
}

//************************************************************************
//************************************************************************
// General purpose counter functions
//------------------------------------------------------------------------
// used to keep track of different kinds of errors and conditions
//************************************************************************
//************************************************************************

void counters_reset( void ) {
  for(int i=0;i<NUM_COUNTERS;i++) {
    counter[i]=0;
    counter_has_changed[i]=false;
  }
}

// has ANY counter changed value?
bool counters_have_changed( void ) {
  bool bret=false;
  for(int i=0;i<NUM_COUNTERS;i++) {
    if(counter_has_changed[i]) bret=true;
  }
  return bret;
}

void counters_clear_changes( void ) {
  for(int i=0;i<NUM_COUNTERS;i++) counter_has_changed[i]=false;
}

// manually select the counters that are error conditions
// LOSTSYNCS, ERRSIZE, ERRCKSUM, INVALID,
bool errors_have_changed( void ) {
  if( counter_has_changed[LOSTSYNCS] ) return true;
  if( counter_has_changed[ERRSIZE] ) return true;
  if( counter_has_changed[ERRCKSUM] ) return true;
  if( counter_has_changed[INVALID] ) return true;
  return false;
}

void errors_clear_changes( void ) {
  counter_has_changed[LOSTSYNCS] = false;
  counter_has_changed[ERRSIZE] = false;
  counter_has_changed[ERRCKSUM] = false;
  counter_has_changed[INVALID] = false;
}

void counter_incr( COUNTER cntr ) {
  counter[cntr]++;
  counter_has_changed[cntr]=true;
}

uint32_t counter_get_value( COUNTER cntr ) {
  return counter[cntr];
}

void counters_print_all( FILE *fp ) {
  for(int i=0;i<NUM_COUNTERS;i++) fprintf(fp, "%4d\t", counter[i]);
  fprintf(fp,"\n");
}

void counters_print_all_detailed( FILE *fp ) {
  for(int i=0;i<NUM_COUNTERS;i++) {
    fprintf(fp, "%s:\t", counter_descr[i]);
    fprintf(fp, "%4d", counter[i]);
    fprintf(fp,"\n");
  }
}

// The functions that query the counters 
// may be helpful to the application, so they
// are made available byu these public wrapper functions
bool hil_grabber_errors_have_changed( void ) { return errors_have_changed(); }
bool hil_grabber_counters_have_changed( void ) { return counters_have_changed(); }
uint32_t hil_grabber_counter_get_value( HIL_COUNTER cntr ) { return counter_get_value( (COUNTER)cntr ); }
void hil_grabber_counters_print_all( FILE *fp ) { counters_print_all( fp ); }
void hil_grabber_counters_print_all_detailed( FILE *fp ) { counters_print_all_detailed( fp ); }






