/***********************************************************************
**
** klp_core.h
**
** Includes implementation of the ASTRON LOFAR UDP data packet format.
**
***********************************************************************/

#ifndef UDPTEST_H
#define UDPTEST_H

#define PROG_VERSION_NUM "1.4"
#define PROG_VERSION_DATE "20-Mar-2024"
#define PROG_DESCRIPTION "KAIRA Local Pipeline"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <sched.h>
#include <math.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include <netinet/in.h>
#include <arpa/inet.h>


/* maximum buffer length */
#define BUFLEN 9000
/* what port the packets are coming into */
#define PORT 4346
/* which ip address the packets are coming into */
#define SERVER_IP "10.211.9.42"

/* number of beamlets */
#define N_BEAMLETS 61
/* buffer was originally 400000000, 7456540 is the largest buffer */
/* we can get in kaira05 without touching some kernel parameters */
#ifdef __APPLE__
#define NET_BUFFER_SIZE 7456540
#else
#define NET_BUFFER_SIZE 400000000
#endif

/* the clock rate */
#define SAMPLE_FREQ 200000000.0

#define WRITE_DATA 1

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/* the number of samples to output (exactly 2 s) */
#define N_SAMPLES_OUT 390625

/* number of time points per packet */
#define N_SAMPLES_PER_PACKET 16

/* create directory with timestamp and current working directory */
void make_directory();

/* implement this function to process data */
void *write_output_data(void *par);

/* print usage */
void klp_core_usage(char *s);

/* convert timestamp to date string (ANSI) */
void ansi_timestamp2datestr(int tv, char *out);

typedef struct recorder_options_struct_t
{
  int maxbeamlet;
  char ip_number[24];
  int port;
  int net_buffer_size;
} recorder_options_t;

typedef struct astron_time_struct_t
{
  int64_t timestamp;
  int64_t seqnum;
} astron_time_t;

typedef struct beamlet_header_struct_t
{
  char version;
  char source_info;
  char conf1;
  char conf2;
  char station_id1;
  char station_id2;
  char n_beamlets;
  char n_blocks;
  uint32_t timestamp;
  uint32_t seqnum;
  uint16_t *data;
} beamlet_header_t;

typedef struct beamlet_data_struct_t
{
  int n_beamlets;
  int n_time;
  int16_t *data;
} beamlet_data_t;

typedef struct buffer_struct_t
{
  int16_t **datax;
  int16_t **datay;
} buffer_t;

typedef struct recorder_struct_t
{
  char buf[BUFLEN];
  beamlet_header_t beamlet_header;
  beamlet_header_t beamlet_header0;
  uint64_t count;
  uint32_t prev_seqnum;
  uint32_t seq_diff;
  int tdiff;
  int i,j;

  int n_dropped_samples;

  int16_t *xv;
  int16_t *yv;

  buffer_t *buffer;
  buffer_t double_buffer[2];

  int bufferidx;
  int wbufferidx;

  int n_channels;
  int buffersize;
  int counter;
  int filecounter;
  double timestamp;
  int unixtime;
  int seqnum;
  FILE *timestamps;
  FILE *logfile;
  recorder_options_t *opt;

  int can_write;
  pthread_t write_thread;
  pthread_mutex_t writemutex;
  pthread_attr_t attr;
  pthread_cond_t data_in_buffer;

  struct sockaddr_in si_me, si_other;
  int socket;

  astron_time_t tnow, tprev;
  double tnowd,tprevd,tnowf,tprevf;
  double delta;

  void *userdata;

} recorder_t;

/* these functions will be defined in the worker threads */
void klp_init(int argc, char **argv, recorder_t *d);
void klp_proc(recorder_t *d);

#endif

/* EOF */
