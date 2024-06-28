/***********************************************************************
**
** klp_core.c
**
***********************************************************************/

#include "klp_core.h"

recorder_t *new_recorder()
{
  recorder_t *d = (recorder_t *) malloc(sizeof(recorder_t));
  d->n_channels = N_BEAMLETS;
  d->buffersize = N_SAMPLES_OUT;
  d->logfile = stdout;
  d->can_write = 0;
  d->userdata = 0;
  d->n_dropped_samples=0;
  return(d);
}

void allocate_buffers(recorder_t *d)
{
  int i,j;

  for(j=0 ; j<2 ; j++)
  {
    d->double_buffer[j].datax = (int16_t **)malloc(sizeof(int16_t *)*d->n_channels);
    d->double_buffer[j].datay = (int16_t **)malloc(sizeof(int16_t *)*d->n_channels);
  }

  d->bufferidx=0;
  for(i=0 ; i<d->n_channels ; i++)
  {
    for(j=0 ; j<2 ; j++)
    {
      d->double_buffer[j].datax[i] = (int16_t *)malloc(sizeof(int16_t)*d->buffersize*2);
      d->double_buffer[j].datay[i] = (int16_t *)malloc(sizeof(int16_t)*d->buffersize*2);
    }
  }
  d->counter=0;
  if(d->userdata == 0)
    printf("Warning, d->userdata not set\n");
}

int time_diff(astron_time_t t1, astron_time_t t0)
{
  return( ((t1.timestamp-t0.timestamp)*390625 + 2*(t1.seqnum - t0.seqnum))/2 );
}

void diep(char *s)
{
  perror(s);
  exit(0);
}

void lock_memory()
{
#ifdef __APPLE__
  printf("Memory locking disabled under OS X.\n");
#else
  if(mlockall(MCL_CURRENT) == -1)
  {
    printf("Couldn't lock memory to RAM (MCL_CURRENT). This requires sudo.\n");
    exit(1);
  }
  if(mlockall(MCL_FUTURE) == -1)
  {
    printf("Couldn't lock memory to RAM (MCL_FUTURE). This requires sudo.\n");
    exit(1);
  }
#endif
}

void make_directory()
{
  struct timeval tv;
  char dirname[4096];
  char cmd[4128];
  int res;
  gettimeofday(&tv, NULL);
  sprintf(dirname,"data-%1.4f",((double)tv.tv_sec)+(double)tv.tv_usec*1.0/1e6);
  sprintf(cmd,"mkdir -p %s",dirname);
  res = system(cmd);
  printf("Creating directory %s\n",dirname);
  res = chdir(dirname);
  if (res != 0)
  {
    printf("Error creating %s",dirname);
  }
}

void klp_core_usage(char *s)
{
  printf ("Usage: %s ipaddr port [optional parameters].\n", s);
  printf("\n");
}


void parse_args(int argc, char **argv, recorder_options_t *opt)
{
  opt->maxbeamlet=61;
  opt->net_buffer_size=NET_BUFFER_SIZE;
  printf("net_buffer_size: %f\n",(double)opt->net_buffer_size);

  if(argc < 3)
  {
    printf("Requires at least two arguments (e.g., udptest 10.211.9.42 4346)\n");
    klp_core_usage(argv[0]);
    exit(1);
  }
  strcpy(opt->ip_number, argv[1]);
  sscanf(argv[2],"%d",&opt->port);
  printf("ip number: %s port %d\n",opt->ip_number, opt->port);
}

/* remember to increase buffer size limits */
/* sudo sysctl -w net.core.rmem_default=100000000 */
/* sudo sysctl -w net.core.rmem_max=100000000 */
void open_socket(recorder_t *rec)
{
  uint64_t n;
  recorder_options_t *opt;
  opt = rec->opt;

  printf("Create UDP socket\n");
  if ((rec->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    diep("Couldn't open socket");

  n = opt->net_buffer_size;
  printf("buffer size: %f\n",(double)opt->net_buffer_size);
  if (setsockopt(rec->socket, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) == -1)
  {
    printf("Couldn't set buffer size\n");
  }

  /* setup socket */
  memset((char *) &rec->si_me, 0, sizeof(rec->si_me));
  rec->si_me.sin_family = AF_INET;
  rec->si_me.sin_port = htons(opt->port);
  printf("Setup ip %s port %d\n", opt->ip_number, opt->port);
  if(inet_aton(opt->ip_number, &rec->si_me.sin_addr)==0)
  {
    printf("Address %s failed.\n", opt->ip_number);
    exit(1);
  }
  printf("bind\n");
  /* bind socket to address */
  if (bind(rec->socket,(struct sockaddr *)&rec->si_me, sizeof(rec->si_me))==-1)
    diep("bind failed");
}

void create_writer_thread(recorder_t *rec)
{
  pthread_mutex_init(&rec->writemutex, NULL);
  pthread_cond_init (&rec->data_in_buffer, NULL);
  pthread_attr_init(&rec->attr);
  pthread_attr_setdetachstate(&rec->attr, PTHREAD_CREATE_JOINABLE);
  pthread_create(&rec->write_thread, &rec->attr, &write_output_data, (void *)rec);
  pthread_attr_destroy(&rec->attr);
}


void parse_packet(recorder_t *rec)
{
  /* read in new samples */
  for(rec->j=0 ; rec->j < N_SAMPLES_PER_PACKET ; rec->j++)
  {
    for(rec->i=0 ; rec->i<N_BEAMLETS ; rec->i++)
    {
      rec->xv = rec->double_buffer[rec->bufferidx].datax[rec->i];
      rec->yv = rec->double_buffer[rec->bufferidx].datay[rec->i];

      /* j:th sample in packet for beam i is: */
      /* rex = i*4*N_SAMPLES_PER_PACKET + 4*j + 0 */
      /* imx = i*4*N_SAMPLES_PER_PACKET + 4*j + 1 */
      /* rey = i*4*N_SAMPLES_PER_PACKET + 4*j + 2 */
      /* imy = i*4*N_SAMPLES_PER_PACKET + 4*j + 3 */
      rec->xv[2*rec->counter] = rec->beamlet_header.data[rec->i*4*N_SAMPLES_PER_PACKET + rec->j*4];
      rec->xv[2*rec->counter+1] = rec->beamlet_header.data[rec->i*4*N_SAMPLES_PER_PACKET + rec->j*4 + 1];
      rec->yv[2*rec->counter] = rec->beamlet_header.data[rec->i*4*N_SAMPLES_PER_PACKET + rec->j*4 + 2];
      rec->yv[2*rec->counter+1] = rec->beamlet_header.data[rec->i*4*N_SAMPLES_PER_PACKET+ rec->j*4 + 3];
    }
    rec->counter++;

    if(rec->counter == rec->buffersize)
    {
      /*  rec->timestamp = rec->tnowd + rec->tnowf - ((double)N_SAMPLES_PER_PACKET)*rec->delta; */
      rec->unixtime = rec->beamlet_header.timestamp;
      rec->seqnum = rec->beamlet_header.seqnum + rec->j;
      rec->timestamp = (double)rec->unixtime + 5.12e-6 * (double)rec->seqnum;
      if( rec->unixtime % 2 != 0 )
	{
	  rec->timestamp += 2.56e-6;
	}


      /* setup pointer to background buffer */
      rec->wbufferidx = rec->bufferidx;
      rec->buffer = &rec->double_buffer[rec->wbufferidx];
      rec->bufferidx = (rec->bufferidx+1)%2;

      pthread_mutex_lock(&rec->writemutex);
      rec->can_write = 1;
      pthread_cond_signal(&rec->data_in_buffer);
      pthread_mutex_unlock(&rec->writemutex);
      rec->counter = 0;
    }
  }
}

void pad_zeros(recorder_t *rec, int nzeros)
{
  if(nzeros > 64)
  {
    printf("Lost more than two packets in a row, quitting to prevent system overload.\n");
    exit(1);
  }

  for(rec->j=0 ; rec->j < nzeros ; rec->j++)
  {
    for(rec->i=0 ; rec->i<N_BEAMLETS ; rec->i++)
    {
      rec->xv = rec->double_buffer[rec->bufferidx].datax[rec->i];
      rec->yv = rec->double_buffer[rec->bufferidx].datay[rec->i];

      rec->xv[2*rec->counter] = 0;
      rec->xv[2*rec->counter+1] = 0;
      rec->yv[2*rec->counter] = 0;
      rec->yv[2*rec->counter+1] = 0;
    }
    rec->counter++;
    rec->n_dropped_samples++;

    if(rec->counter == rec->buffersize)
    {
      /*      rec->timestamp = rec->tnowd + rec->tnowf - ((double)N_SAMPLES_PER_PACKET)*rec->delta; */
      rec->unixtime = rec->beamlet_header.timestamp;
      rec->seqnum = rec->beamlet_header.seqnum + rec->j;
      rec->timestamp = (double)rec->unixtime + 5.12e-6 * (double)rec->seqnum;
      if( rec->unixtime % 2 != 0 )
	{
	  rec->timestamp += 2.56e-6;
	}

      rec->wbufferidx = rec->bufferidx;
      rec->buffer = &rec->double_buffer[rec->wbufferidx];
      rec->bufferidx = (rec->bufferidx+1)%2;

      pthread_mutex_lock(&rec->writemutex);
      rec->can_write = 1;
      pthread_cond_signal(&rec->data_in_buffer);
      pthread_mutex_unlock(&rec->writemutex);
      rec->counter = 0;
    }
  }
}

void read_data(recorder_t *rec)
{
  int slen=sizeof(rec->si_other);
  recorder_options_t *opt;

  opt = rec->opt;
  open_socket(rec);
  create_writer_thread(rec);
  rec->count=0;
  rec->delta = 1024.0/SAMPLE_FREQ;

  /* while true, read data from socket and do something with it. */
  while(1)
  {
    if(recvfrom(rec->socket,
                &rec->buf[0] ,
                BUFLEN,
                0,
                (struct sockaddr *)&rec->si_other,
                &slen)==-1)
      diep("recvfrom()");

    /* first copy the packet header */
    memcpy(&rec->beamlet_header, &rec->buf[0], 16);

    /* then setup the pointer to the data */
    rec->beamlet_header.data = (uint16_t*)&rec->buf[16];
    rec->tnow.timestamp = rec->beamlet_header.timestamp;
    rec->tnow.seqnum = rec->beamlet_header.seqnum;
    rec->tnowd = (double)rec->beamlet_header.timestamp;
    rec->tnowf = ((double)rec->beamlet_header.seqnum)*(1024.0/200e6);

    if(rec->count != 0)
    {
/*      rec->tdiff = ((int)round(((double)time_diff(rec->tnow,rec->tprev))/16.0))*16; */
      rec->tdiff =
           (
             lroundl(
               ( (double)time_diff(rec->tnow,rec->tprev) )/16.0
             )
           )*16;

      /* if(rec->tdiff != 16 && rec->tdiff != 15) */
      if(rec->tdiff != 16)
      {
        printf("wrong delta, dropped packet %d %lf\n",rec->tdiff,(rec->tnowd-rec->tprevd)+(rec->tnowf-rec->tprevf));
        printf("timestamp %d seqnum %d prev ts %lu prev sn %lu\n",rec->beamlet_header.timestamp,rec->beamlet_header.seqnum,rec->tprev.timestamp,rec->tprev.seqnum);
	if(rec->beamlet_header.timestamp == -1)
	{
	  printf("Error. Stream broken due to timestamp=-1. Exiting.\n");
	  exit(0);
	}
        pad_zeros(rec,rec->tdiff-16);
        printf("Overrun, padding %d zeros\n",rec->tdiff-16);
      }
    }
    else
    {
      /* store the first packet */
      memcpy(&rec->beamlet_header0, &rec->buf[0], 16);
    }
    rec->tprev = rec->tnow;
    rec->tprevd = rec->tnowd;
    rec->tprevf = rec->tnowf;

#ifdef WRITE_DATA
    parse_packet(rec);
#endif
    /* another thread is then responsible for storing this to a file when the output buffer is full */
    rec->count++;

    if(rec->count%100000 == 0)
    {
      printf("Received %lu packets, last from %s:%d dropped %d\n",rec->count,inet_ntoa(rec->si_other.sin_addr), ntohs(rec->si_other.sin_port),rec->n_dropped_samples);
    }
  }
}

void set_realtime_priority()
{
#ifdef __APPLE__
  printf("Set priority disabled under OS X.\n");
#else
  struct sched_param sp;
  sp.sched_priority=99;
  sched_setscheduler(0,SCHED_RR,&sp);
#endif
}

/* ANSI-compliant replacement version of timestamp2datestr */
void ansi_timestamp2datestr(int tv, char *out)
{
  struct tm tm;
  time_t ts;

  ts = (time_t)tv;
  memset(&tm, 0, sizeof(struct tm));
  tm = *gmtime(&ts);
  strftime(out, 1024, "%d %b %Y %H:%M", &tm);
}

void *write_output_data(void *par)
{
  recorder_options_t *opt;
  recorder_t *d;

  d = (recorder_t *)par;
  opt = d->opt;

  while(1)
  {
    pthread_mutex_lock(&d->writemutex);

    if(d->can_write == 0)
    {
      pthread_cond_wait(&d->data_in_buffer, &d->writemutex);
    }
    else
    {
      d->can_write=0;
      klp_proc(d);
    }
    pthread_mutex_unlock(&d->writemutex);
  }
}

int main(int argc, char **argv)
{
  recorder_t *rec;
  recorder_options_t opt;

  rec = new_recorder();
  lock_memory();
  parse_args(argc, argv, &opt);
  rec->opt = &opt;
  klp_init(argc, argv, rec);
  allocate_buffers(rec);
  read_data(rec);

  return 0;
}

/* EOF */
