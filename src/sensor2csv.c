#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#include "ya_getopt.h"
#define strncasecmp _strnicmp
#else
#include <unistd.h>
#include <getopt.h>
#endif

#include "ilidarlib.h"


#define DEFAULT_NFRAMES (1)
#define DEFAULT_BAUDRATE (460800)
#define DEFAULT_TIMEOUT (50000)




#define MAX_PINGPONG_FRAMES (16)
extern HIL_FRAME frame, framebuf1[MAX_PINGPONG_FRAMES], framebuf2[MAX_PINGPONG_FRAMES];
void write_frame_csv( FILE *fpout, HIL_FRAME *hfp );
void write_frame_oneshot_csv( FILE *fpout, HIL_FRAME *hfp );

void print_help(void) {
  fprintf(stderr,"Usage:\n");
  fprintf(stderr,"  sensor2csv serial-port csv-file-name [-n nframes] [-b baud]\n");
  fprintf(stderr,"Where:\n");
  fprintf(stderr,"  serial-port     port which iLidar is connected, e.g., /dev/ttyUSB0 or COM5\n");
  fprintf(stderr,"  csv-file-name   output file, default is stdout\n");
  fprintf(stderr,"  nframes         number frames to write, default=1\n");
  fprintf(stderr,"  baudrate        serial port baud, default=480600\n");
  fprintf(stderr,"Note:\n");
  fprintf(stderr,"  these are the currently connected serial ports:\n" );
  hil_port_enumerate(stderr);
}

int main(int argc, char *argv[]) {
  int iarg=0;

  FILE *fplog=NULL;
  char fnamelog[HIL_MAXNAMESIZE];
  char portname[HIL_MAXNAMESIZE];

  int nframes=DEFAULT_NFRAMES;
  int baud=DEFAULT_BAUDRATE;
  int tout=DEFAULT_TIMEOUT;

  int logsize=0;
  int llogsize=0;
  int max_sizeKB = 2048;
  int delta_sizeKB = 512; // notification prompt to screen

  HIL_PORT hil_port;
  HIL_PORT *hp=NULL;

  time_t now;
  struct tm* nowtm;
  char datetime[HIL_MAXNAMESIZE];

  // This generates an automatic filename based on time and date
  now = time(NULL);
  nowtm = localtime(&now);
  strftime(datetime, HIL_MAXNAMESIZE, "%Y%m%d-%H%M%S.csv", nowtm);
  // printf("Time string: %s\n", datetime);


  //------------------------------------------------------------------------
  // Command line argument processing (using getopt_long)
  //------------------------------------------------------------------------
  int c;
  struct option long_options[] = {
    {"baud",       required_argument,   NULL, 'b'},
    {"mode",       required_argument,   NULL, 'm'},
    {"numframes",  required_argument,   NULL, 'n'},
    {"timeout",    required_argument,   NULL, 't'},
    {"help",       no_argument,         NULL, 'h'},
    {0, 0, 0, 0}
  };

  baud = DEFAULT_BAUDRATE;
  nframes = DEFAULT_NFRAMES;
  tout = DEFAULT_TIMEOUT;
  char *endptr=NULL;

  while (1) {
    int option_index = 0; // getopt_long stores option index here
    c = getopt_long (argc, argv, "b:m:n:h", long_options, &option_index);
    if (c == -1) break; // Detect the end of the options.
    switch (c) {
      case 0:
        if (long_options[option_index].flag != 0) break;
        printf ("option %s", long_options[option_index].name);
        if (optarg) printf (" with arg %s", optarg);
        printf ("\n");
        break;
      case 'b':
        endptr=NULL;
        errno=0;
        fprintf(stderr, "baud string: %%s\n", optarg);
        baud = (int)strtol( optarg, &endptr, 10 );
        if(endptr == optarg) {
          fprintf(stderr,"error reading baudrate: %s\n", optarg );
          exit(99);
        }
        break;
      case 'm':
        fprintf(stderr, "TBD: port mode option, value %s\n", optarg);
        break;
      case 'h':
        print_help();
        exit(0);
        break;
      case 'n':
        endptr=NULL;
        errno=0;
        fprintf(stderr, "nframes: %s\n", optarg);
        nframes = (int)strtol( optarg, &endptr, 10 );
        if(endptr == optarg) {
          fprintf(stderr,"error reading nframes: %s\n", argv[iarg] );
          exit(99);
        }
        break;
      case 't': // get optional timeout
        endptr=NULL;
        errno=0;
        fprintf(stderr, "timeout string: %s\n", optarg);
        tout = (int)strtol( optarg, &endptr, 10 );
        if(endptr == argv[iarg]) {
          fprintf(stderr,"error reading timeout: %s\n", optarg );
          exit(99);
        }
      case '?':
        // getopt_long already printed an error message.
        break;
      default:
        exit(99);
    }
  }


  //if (verbose_flag) puts ("verbose flag is set");

  // handle positional arguments
  if (optind < argc) {
    strncpy(portname, argv[optind++], HIL_MAXNAMESIZE);
  } else {
    fprintf(stderr, "Must specify a serial port\n");
    print_help();
    exit(99);
  }

  if (optind < argc) {
    strncpy( fnamelog, argv[optind++], HIL_MAXNAMESIZE );
    if( 0 == strncasecmp( fnamelog, "time", HIL_MAXNAMESIZE )) {
      // if user speficies "time", filename will be date/time.csv
      strncpy( fnamelog, datetime, HIL_MAXNAMESIZE );
    }
    if( 0 == strncasecmp( fnamelog, "default", HIL_MAXNAMESIZE )) {
      // if user speficies "default", filename will be ilidardata.csv
      strncpy( fnamelog, "ilidardata.csv", HIL_MAXNAMESIZE );
    }
    fplog=fopen( fnamelog, "w" );
    if(!fplog) {
      fprintf(stderr, "Error opening output capture file: %s\n", fnamelog);
      exit(99);
    }
  } else {
    fplog = stdout;
  }

//------------------------------------------------------------------------
// Connect to the sensor
//------------------------------------------------------------------------
  hp = hil_port_open( &hil_port, portname, baud, tout );
  if(!hp) {
    fprintf(stderr,"Error connecting to sensor on port: %s\n", portname );
    fprintf(stderr,"These are the currently connected serial ports:\n" );
    hil_port_enumerate(stderr);
		exit(0);
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
      if( nframes == 1 ) write_frame_oneshot_csv( fplog, hfp );
      else               write_frame_csv( fplog, hfp );
      nf++;
      hfp=NULL;
      //usleep(48000);
    }
    if(nf >= nframes) break;
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
  fprintf(stderr, "Wrote %d frames to logfile\n", iloop);
  hil_grabber_counters_print_all_detailed(stderr);
  fprintf(stderr, "Recd: %d sync patterns\n", hil_grabber_counter_get_value(HIL_GOODSYNCS) );
  fprintf(stderr, "Recd: %d valid frames\n", hil_grabber_counter_get_value(HIL_FRAMES) );
  fprintf(stderr, "Recd: %d bytes\n", hil_grabber_counter_get_value(HIL_BYTES) );
  printf("The end.\n");

}

const char csv_header[] = "time,x0,y0,z0,y1,x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4,x5,y5,z5,x6,y6,z6,x7,y7,z7,x8,y8,z8,x9,y9,z9,x10,y10,z10,x11,y11,z11,x12,y12,z12,x13,y13,z13,x14,y14,z14,x15,y15,z15,x16,y16,z16,x17,y17,z17,x18,y18,z18,x19,y19,z19,x20,y20,z20,x21,y21,z21,x22,y22,z22,x23,y23,z23,x24,y24,z24,x25,y25,z25,x26,y26,z26,x27,y27,z27,x28,y28,z28,x29,y29,z29,x30,y30,z30,x31,y31,z31,x32,y32,z32,x33,y33,z33,x34,y34,z34,x35,y35,z35,x36,y36,z36,x37,y37,z37,x38,y38,z38,x39,y39,z39,x40,y40,z40,x41,y41,z41,x42,y42,z42,x43,y43,z43,x44,y44,z44,x45,y45,z45,x46,y46,z46,x47,y47,z47,x48,y48,z48,x49,y49,z49,x50,y50,z50,x51,y51,z51,x52,y52,z52,x53,y53,z53,x54,y54,z54,x55,y55,z55,x56,y56,z56,x57,y57,z57,x58,y58,z58,x59,y59,z59,x60,y60,z60,x61,y61,z61,x62,y62,z62,x63,y63,z63,x64,y64,z64,x65,y65,z65,x66,y66,z66,x67,y67,z67,x68,y68,z68,x69,y69,z69,x70,y70,z70,x71,y71,z71,x72,y72,z72,x73,y73,z73,x74,y74,z74,x75,y75,z75,x76,y76,z76,x77,y77,z77,x78,y78,z78,x79,y79,z79,x80,y80,z80,x81,y81,z81,x82,y82,z82,x83,y83,z83,x84,y84,z84,x85,y85,z85,x86,y86,z86,x87,y87,z87,x88,y88,z88,x89,y89,z89,x90,y90,z90,x91,y91,z91,x92,y92,z92,x93,y93,z93,x94,y94,z94,x95,y95,z95,x96,y96,z96,x97,y97,z97,x98,y98,z98,x99,y99,z99,x100,y100,z100,x101,y101,z101,x102,y102,z102,x103,y103,z103,x104,y104,z104,x105,y105,z105,x106,y106,z106,x107,y107,z107,x108,y108,z108,x109,y109,z109,x110,y110,z110,x111,y111,z111,x112,y112,z112,x113,y113,z113,x114,y114,z114,x115,y115,z115,x116,y116,z116,x117,y117,z117,x118,y118,z118,x119,y119,z119,x120,y120,z120,x121,y121,z121,x122,y122,z122,x123,y123,z123,x124,y124,z124,x125,y125,z125,x126,y126,z126,x127,y127,z127,x128,y128,z128,x129,y129,z129,x130,y130,z130,x131,y131,z131,x132,y132,z132,x133,y133,z133,x134,y134,z134,x135,y135,z135,x136,y136,z136,x137,y137,z137,x138,y138,z138,x139,y139,z139,x140,y140,z140,x141,y141,z141,x142,y142,z142,x143,y143,z143,x144,y144,z144,x145,y145,z145,x146,y146,z146,x147,y147,z147,x148,y148,z148,x149,y149,z149,x150,y150,z150,x151,y151,z151,x152,y152,z152,x153,y153,z153,x154,y154,z154,x155,y155,z155,x156,y156,z156,x157,y157,z157,x158,y158,z158,x159,y159,z159,x160,y160,z160,x161,y161,z161,x162,y162,z162,x163,y163,z163,x164,y164,z164,x165,y165,z165,x166,y166,z166,x167,y167,z167,x168,y168,z168,x169,y169,z169,x170,y170,z170,x171,y171,z171,x172,y172,z172,x173,y173,z173,x174,y174,z174,x175,y175,z175,x176,y176,z176,x177,y177,z177,x178,y178,z178,x179,y179,z179,x180,y180,z180,x181,y181,z181,x182,y182,z182,x183,y183,z183,x184,y184,z184,x185,y185,z185,x186,y186,z186,x187,y187,z187,x188,y188,z188,x189,y189,z189,x190,y190,z190,x191,y191,z191,x192,y192,z192,x193,y193,z193,x194,y194,z194,x195,y195,z195,x196,y196,z196,x197,y197,z197,x198,y198,z198,x199,y199,z199,x200,y200,z200,x201,y201,z201,x202,y202,z202,x203,y203,z203,x204,y204,z204,x205,y205,z205,x206,y206,z206,x207,y207,z207,x208,y208,z208,x209,y209,z209,x210,y210,z210,x211,y211,z211,x212,y212,z212,x213,y213,z213,x214,y214,z214,x215,y215,z215,x216,y216,z216,x217,y217,z217,x218,y218,z218,x219,y219,z219,x220,y220,z220,x221,y221,z221,x222,y222,z222,x223,y223,z223,x224,y224,z224,x225,y225,z225,x226,y226,z226,x227,y227,z227,x228,y228,z228,x229,y229,z229,x230,y230,z230,x231,y231,z231,x232,y232,z232,x233,y233,z233,x234,y234,z234,x235,y235,z235,x236,y236,z236,x237,y237,z237,x238,y238,z238,x239,y239,z239,x240,y240,z240,x241,y241,z241,x242,y242,z242,x243,y243,z243,x244,y244,z244,x245,y245,z245,x246,y246,z246,x247,y247,z247,x248,y248,z248,x249,y249,z249,x250,y250,z250,x251,y251,z251,x252,y252,z252,x253,y253,z253,x254,y254,z254,x255,y255,z255,x256,y256,z256,x257,y257,z257,x258,y258,z258,x259,y259,z259,x260,y260,z260,x261,y261,z261,x262,y262,z262,x263,y263,z263,x264,y264,z264,x265,y265,z265,x266,y266,z266,x267,y267,z267,x268,y268,z268,x269,y269,z269,x270,y270,z270,x271,y271,z271,x272,y272,z272,x273,y273,z273,x274,y274,z274,x275,y275,z275,x276,y276,z276,x277,y277,z277,x278,y278,z278,x279,y279,z279,x280,y280,z280,x281,y281,z281,x282,y282,z282,x283,y283,z283,x284,y284,z284,x285,y285,z285,x286,y286,z286,x287,y287,z287,x288,y288,z288,x289,y289,z289,x290,y290,z290,x291,y291,z291,x292,y292,z292,x293,y293,z293,x294,y294,z294,x295,y295,z295,x296,y296,z296,x297,y297,z297,x298,y298,z298,x299,y299,z299,x300,y300,z300,x301,y301,z301,x302,y302,z302,x303,y303,z303,x304,y304,z304,x305,y305,z305,x306,y306,z306,x307,y307,z307,x308,y308,z308,x309,y309,z309,x310,y310,z310,x311,y311,z311,x312,y312,z312,x313,y313,z313,x314,y314,z314,x315,y315,z315,x316,y316,z316,x317,y317,z317,x318,y318,z318,x319,y319,z319,x320,y320,z320,x321,y321,z321,x322,y322,z322,x323,y323,z323,x324,y324,z324,x325,y325,z325,x326,y326,z326,x327,y327,z327,x328,y328,z328,x329,y329,z329,x330,y330,z330,x331,y331,z331,x332,y332,z332,x333,y333,z333,x334,y334,z334,x335,y335,z335,x336,y336,z336,x337,y337,z337,x338,y338,z338,x339,y339,z339,x340,y340,z340,x341,y341,z341,x342,y342,z342,x343,y343,z343,x344,y344,z344,x345,y345,z345,x346,y346,z346,x347,y347,z347,x348,y348,z348,x349,y349,z349,x350,y350,z350,x351,y351,z351,x352,y352,z352,x353,y353,z353,x354,y354,z354,x355,y355,z355,x356,y356,z356,x357,y357,z357,x358,y358,z358,x359,y359,z359,x360,y360,z360,x361,y361,z361,x362,y362,z362,x363,y363,z363,x364,y364,z364,x365,y365,z365,x366,y366,z366,x367,y367,z367,x368,y368,z368,x369,y369,z369,x370,y370,z370,x371,y371,z371,x372,y372,z372,x373,y373,z373,x374,y374,z374,x375,y375,z375,x376,y376,z376,x377,y377,z377,x378,y378,z378,x379,y379,z379,x380,y380,z380,x381,y381,z381,x382,y382,z382,x383,y383,z383,x384,y384,z384,x385,y385,z385,x386,y386,z386,x387,y387,z387,x388,y388,z388,x389,y389,z389,x390,y390,z390,x391,y391,z391,x392,y392,z392,x393,y393,z393,x394,y394,z394,x395,y395,z395,x396,y396,z396,x397,y397,z397,x398,y398,z398,x399,y399,z399,x400,y400,z400,x401,y401,z401,x402,y402,z402,x403,y403,z403,x404,y404,z404,x405,y405,z405,x406,y406,z406,x407,y407,z407,x408,y408,z408,x409,y409,z409,x410,y410,z410,x411,y411,z411,x412,y412,z412,x413,y413,z413,x414,y414,z414,x415,y415,z415,x416,y416,z416,x417,y417,z417,x418,y418,z418,x419,y419,z419,x420,y420,z420,x421,y421,z421,x422,y422,z422,x423,y423,z423,x424,y424,z424,x425,y425,z425,x426,y426,z426,x427,y427,z427,x428,y428,z428,x429,y429,z429,x430,y430,z430,x431,y431,z431,x432,y432,z432,x433,y433,z433,x434,y434,z434,x435,y435,z435,x436,y436,z436,x437,y437,z437,x438,y438,z438,x439,y439,z439,x440,y440,z440,x441,y441,z441,x442,y442,z442,x443,y443,z443,x444,y444,z444,x445,y445,z445,x446,y446,z446,x447,y447,z447,x448,y448,z448,x449,y449,z449,x450,y450,z450,x451,y451,z451,x452,y452,z452,x453,y453,z453,x454,y454,z454,x455,y455,z455,x456,y456,z456,x457,y457,z457,x458,y458,z458,x459,y459,z459,x460,y460,z460,x461,y461,z461,x462,y462,z462,x463,y463,z463,x464,y464,z464,x465,y465,z465,x466,y466,z466,x467,y467,z467,x468,y468,z468,x469,y469,z469,x470,y470,z470,x471,y471,z471,x472,y472,z472,x473,y473,z473,x474,y474,z474,x475,y475,z475,x476,y476,z476,x477,y477,z477,x478,y478,z478,x479,y479,z479,roll,pitch,yaw";
void write_frame_csv( FILE *fpout, HIL_FRAME *hfp ) {
  static bool first=true;
  if(first) {
    first=false;
    fprintf(fpout, "%s\n", csv_header);
  }
  // fprintf(fplog, "TIME:\t%d.%06d  \t%d.%06d\n", 
  //         hfp->p.timePeak.seconds, hfp->p.timePeak.micros,
  //         hfp->p.timeIMU.seconds, hfp->p.timeIMU.micros );
  // fprintf(fplog, "20200525-000000" ); // TODO put the real date-time here
  fprintf(fpout, "%d.%06d,", hfp->p.timePeak.seconds, hfp->p.timePeak.micros ); 
  for(int k=0; k<HIL_MAXPOINTS; k++) {
    fprintf(fpout, "%.3lf,%.3lf,%.3lf",
            (float)hfp->p.pts[k][0]/1000.0f, 
            (float)hfp->p.pts[k][1]/1000.0f, 
            (float)hfp->p.pts[k][2]/1000.0f);
  }
  fprintf(fpout, "\n");
}

const char csv_oneshot_header[] = "x0,y0,z0";
void write_frame_oneshot_csv( FILE *fpout, HIL_FRAME *hfp ) {
  fprintf(fpout, "%s\n", csv_oneshot_header);
  for(int k=0; k<HIL_MAXPOINTS; k++) {
    fprintf(fpout, "%.3lf,%.3lf,%.3lf\n",
            (float)hfp->p.pts[k][0]/1000.0f, 
            (float)hfp->p.pts[k][1]/1000.0f, 
            (float)hfp->p.pts[k][2]/1000.0f);
  }
}



