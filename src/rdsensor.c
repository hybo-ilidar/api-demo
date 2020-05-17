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

#include "../include/ilidarlib.h"

//#define DEFAULT_BAUDRATE (921600)
#define DEFAULT_BAUDRATE (460800)
#define DEFAULT_TIMEOUT (50000)

#define MAXFILENAME (512)
FILE *fpout=NULL;
char fnameout[MAXFILENAME];

int main(int argc, char *argv[]) {
  int err;
  int i;
  int iarg=0;

  char portname[HIL_MAXNAMESIZE];
  int baud;
  int tout;

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
    fprintf(stderr,"  rdsensor serial-port [baud] [timeout]\n");
    hil_port_enumerate(stderr);
    exit(1);
  }

// get serial port 
  iarg++;
  strncpy( portname, argv[iarg], HIL_MAXNAMESIZE );

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

// get optional timeout
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
    exit(99);
  } else {
    fprintf(stderr,"Connected to sensor, port %s at %d baud with timeout %d msec\n", portname, baud, tout);
  }

//------------------------------------------------------------------------
// Open capture output file
//------------------------------------------------------------------------

  strncpy(fnameout, "xyzdata.txt", MAXFILENAME);
  fpout=fopen( fnameout, "w" );
  if(!fpout) {
    fprintf(stderr, "Error opening output capture file\n");
    exit(99);
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
      printf("iloop:%d, nf:%d, seq=%d \n", iloop, nf, hfp->p.seq );
      if(fpout) { // log some data, every 50th frame
        if( 0==(nf % 50)) {
          rewind(fpout);
          for(int k=0; k<HIL_MAXPOINTS; k++) {
            fprintf(fpout, "%d %d %d %d\n", k, hfp->p.pts[k][0], hfp->p.pts[k][1], hfp->p.pts[k][2] );
          }
          fprintf(fpout, "                   \n");
          fprintf(fpout, "                   \n");
          fprintf(fpout, "                   \n");
          fprintf(fpout, "                   \n");
          fprintf(stderr, "XYZ written\n");
        }
      }
      nf++;
      hfp=NULL;
#ifdef _WIN32
      Sleep(48);
#else
      usleep(48000);
#endif
    }
    if(nf > 100) break;
    iloop++;
  }

  hil_port_close(hp);
  fclose(fpout);
  hil_grabber_counters_print_all_detailed(stderr);
  fprintf(stderr, "Recd: %d sync patterns\n", hil_grabber_counter_get_value(HIL_GOODSYNCS) );
  fprintf(stderr, "Recd: %d valid frames\n", hil_grabber_counter_get_value(HIL_FRAMES) );
  fprintf(stderr, "Recd: %d bytes\n", hil_grabber_counter_get_value(HIL_BYTES) );
  printf("The end.\n");

}


