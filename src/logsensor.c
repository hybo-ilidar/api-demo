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

#include "ilidarlib.h"


//#define DEFAULT_BAUDRATE (921600)
#define DEFAULT_BAUDRATE (460800)
#define DEFAULT_TIMEOUT (50000)

#define MAXFILENAME (512)
FILE *fplog=NULL;
char fnamelog[MAXFILENAME];

#define MAX_PINGPONG_FRAMES (16)
extern HIL_FRAME frame, framebuf1[MAX_PINGPONG_FRAMES], framebuf2[MAX_PINGPONG_FRAMES];
int main(int argc, char *argv[]) {
  int iarg=0;

  char portname[HIL_MAXNAMESIZE];
  int baud;
  int tout;
  int logsize=0;
  int llogsize=0;
  int max_sizeKB = 2048;
  int delta_sizeKB = 512; // notification prompt to screen

  HIL_PORT hil_port;
  HIL_PORT *hp=NULL;


//------------------------------------------------------------------------
// Command line argument processing
//------------------------------------------------------------------------
// At the least, we need a serial port name.
// If no arguments, show program usage
// and list available serial ports
  iarg=0;
  if (argc<2) {
    fprintf(stderr,"Usage:\n");
    fprintf(stderr,"  logsensor serial-port logfile [baud]\n");
    fprintf(stderr,"note:\n");
    fprintf(stderr,"  these are the currently connected serial ports:\n" );
    hil_port_enumerate(stderr);
#if 0
    fprintf(stderr, "size of   payload type: %10u %8ux\n", HIL_PAYLOADSIZE, HIL_PAYLOADSIZE );
    fprintf(stderr, "size of     frame type: %10u %8ux\n", HIL_FRAMESIZE,   HIL_FRAMESIZE );
    fprintf(stderr, "size of     frame  var: %10u %8ux\n", (uint32_t)sizeof(frame), (uint32_t)sizeof(frame) );
    fprintf(stderr, "no.  frames per buffer: %10d\n", MAX_PINGPONG_FRAMES );
    fprintf(stderr, "size of      framebuf1: %10u %8ux\n", (uint32_t)sizeof(framebuf1), (uint32_t)sizeof(framebuf1) );
    fprintf(stderr, "size of      framebuf2: %10u %8ux\n", (uint32_t)sizeof(framebuf1), (uint32_t)sizeof(framebuf1) );
    fprintf(stderr, "calc size of  framebuf: %10u %8ux\n", HIL_FRAMESIZE*MAX_PINGPONG_FRAMES, HIL_FRAMESIZE*MAX_PINGPONG_FRAMES );
#endif
    exit(1);
  }

// get serial port 
  iarg++;
  strncpy( portname, argv[iarg], HIL_MAXNAMESIZE );

// get logfile name 
  iarg++;
  strncpy( fnamelog, argv[iarg], MAXFILENAME );
  fplog=fopen( fnamelog, "w" );
  if(!fplog) {
    fprintf(stderr, "Error opening output capture file: %s\n", fnamelog);
    exit(99);
  }

// get optional baudrate
  iarg++;
  if (argc>iarg) {
    char *endptr=NULL;
    errno=0;
    fprintf(stderr, "baud string: %d, %s\n", iarg, argv[iarg]);
    baud = (int)strtol( argv[iarg], &endptr, 10 );
    if(endptr == argv[iarg]) {
      fprintf(stderr,"error reading baudrate: %s\n", argv[iarg] );
      exit(99);
    }
  } else {
    baud = DEFAULT_BAUDRATE;
  }

// get optional timeout (only works if baud rate is specified)
  iarg++;
  if (argc>iarg) {
    char *endptr=NULL;
    errno=0;
    fprintf(stderr, "timeout string: %d, %s\n", iarg, argv[iarg]);
    tout = (int)strtol( argv[iarg], &endptr, 10 );
    if(endptr == argv[iarg]) {
      fprintf(stderr,"error reading timeout: %s\n", argv[iarg] );
      exit(99);
    }
  } else {
    tout = DEFAULT_TIMEOUT;
  }

//------------------------------------------------------------------------
// Connect to the sensor
//------------------------------------------------------------------------
  hp = hil_port_open( &hil_port, portname, baud, tout );
  if(!hp) {
    fprintf(stderr,"Error connecting to sensor on port: %s\n", portname );
    fprintf(stderr,"These are the currently connected serial ports:\n" );
    hil_port_enumerate(stderr);
    exit(99);
  } else {
    fprintf(stderr,"Connected to sensor, port %s at %d baud with timeout %d msec\n", portname, baud, tout);
  }

  // Initialize grabber state machine processing
  hil_frame_grabber( hp, HIL_GRABBER_INIT );
  int nf=0;
  int iloop=0;
  HIL_FRAME *hfp=NULL;
  while(true) {
    hil_frame_grabber( hp, HIL_GRABBER_GRAB );
    hfp = hil_frame_reader( hp );
    if(hfp) { // we have a valid frame...
      printf("iloop:%d, nf:%d, size=%d, seq=%d, datatype=%04x\n", iloop, nf, hfp->size, hfp->p.seq, hfp->p.datatype );
      fprintf(fplog, "TIME:\t%d.%06d  \t%d.%06d\n", 
              hfp->p.timePeak.seconds, hfp->p.timePeak.micros,
              hfp->p.timeIMU.seconds, hfp->p.timeIMU.micros );
      for(int k=0; k<HIL_MAXPOINTS; k++) {
        fprintf(fplog, "%d\t%d  \t%d\t%d\t%d\n", hfp->p.seq, k, 
                hfp->p.pts[k][0], hfp->p.pts[k][1], hfp->p.pts[k][2] );
      }
      nf++;
      hfp=NULL;
      //usleep(48000);
    }
    if(nf > 100) break;
    iloop++;
    logsize = ftell(fplog) / 1024;
    if( (logsize - llogsize) >= delta_sizeKB ) {
      fprintf(stderr,"%8d Kb\n", logsize );
      llogsize = logsize;
    }
    if( logsize > max_sizeKB ) break; 
  }

  hil_port_close(hp);
  fclose(fplog);
  hil_grabber_counters_print_all_detailed(stderr);
  fprintf(stderr, "Recd: %d sync patterns\n", hil_grabber_counter_get_value(HIL_GOODSYNCS) );
  fprintf(stderr, "Recd: %d valid frames\n", hil_grabber_counter_get_value(HIL_FRAMES) );
  fprintf(stderr, "Recd: %d bytes\n", hil_grabber_counter_get_value(HIL_BYTES) );
  printf("The end.\n");

}


