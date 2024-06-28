/***********************************************************************
**
** filewriter.c
**
***********************************************************************/

#include "klp_core.h"

typedef struct klp_fwdata_struct_t
{
  FILE *timestamps;
  FILE *samplerlog;
  int logfile_written;
  int filecounter;
  int n_save_channels;
} klp_fwdata_t;

void klp_fw_usage(char *s)
{
  klp_core_usage(s);
  printf("       -n <int> specifies the number of beamlets to store [0..n]\n");
  printf("       -b <int> specifies the net buffer size (default 1e8)\n");
  printf("\n");
  exit(0);
}

void klp_fw_defaults(klp_fwdata_t *ud)
{
  ud->n_save_channels=61;
}

void klp_fw_parse_args(int argc, char **argv, klp_fwdata_t *ud, recorder_t *d)
{
  char c;
#ifdef __APPLE__
  int errcount;
#endif

  klp_fw_defaults(ud);

#ifdef __APPLE__
  errcount = 0;
  while (1)
  {
    /* The IP and port inputs cause some problems in OS X... */
    c = getopt (argc, argv, "hn:b:");
    if (c == -1)
      {
	errcount++;
	if(optind < (argc-1))
	  {
	    optind++;
	  }
	else
	  {
	    break;
	  }
	if(errcount>10) break;
      }
    switch (c)
    {
    case 'n':
      sscanf(optarg,"%d",&ud->n_save_channels);
      if(ud->n_save_channels > N_BEAMLETS)
      {
        ud->n_save_channels = N_BEAMLETS;
        printf("Warning, only 61 beamlets can be acquired per lane\n");
      }
      break;
    case 'b':
      sscanf(optarg,"%d",&d->opt->net_buffer_size);
      break;
    case 'h':
      klp_fw_usage(argv[0]);
    default:
      ;
      /* klp_fw_usage(argv[0]); */
    }
  }
#else
  while (1)
  {
    c = getopt (argc, argv, "n:h");
    if (c == -1)
      break;
    switch (c)
    {
    case 'n':
      sscanf(optarg,"%d",&ud->n_save_channels);
      if(ud->n_save_channels > N_BEAMLETS)
      {
        ud->n_save_channels = N_BEAMLETS;
        printf("Warning, only 61 beamlets can be acquired per lane\n");
      }
      break;
    case 'b':
      sscanf(optarg,"%d",&d->opt->net_buffer_size);
      break;
    case 'h':
      klp_fw_usage(argv[0]);
    default:
      klp_fw_usage(argv[0]);
    }
  }

#endif
}

void klp_fw_write_logfile(recorder_t *d)
{
  klp_fwdata_t *ud;
  char tstamp[1024];
  ud = (klp_fwdata_t *)d->userdata;

  fprintf(ud->samplerlog, "KLP beamlet data\n");
  fprintf(ud->samplerlog, "filewriter %s\n",PROG_VERSION_NUM);
  fprintf(ud->samplerlog, "\n");
  fprintf(ud->samplerlog, "Initial_timestamp: %d\n",d->beamlet_header0.timestamp);
  fprintf(ud->samplerlog, "Initial_seqnum: %d\n",d->beamlet_header0.seqnum);
  fprintf(ud->samplerlog, "Initial_timestamp: %1.12f\n", (double)d->beamlet_header0.timestamp + ((double)d->beamlet_header0.seqnum)*(1024.0/200e6));

  ansi_timestamp2datestr(d->beamlet_header0.timestamp, tstamp);
  fprintf(ud->samplerlog, "Timestamp_datetime: %s\n", tstamp);
  fprintf(ud->samplerlog, "Number_of_beamlets: %d\n", ud->n_save_channels);
  fprintf(ud->samplerlog, "IP_number: %s\n", d->opt->ip_number);
  fprintf(ud->samplerlog, "Port: %d\n", d->opt->port);
  fprintf(ud->samplerlog, "\n");

  fclose(ud->samplerlog);
  ud->logfile_written=1;
}

/*
  This will be called once at startup.
 */
void klp_init(int argc, char **argv, recorder_t *d)
{
  int i;
  int res = 0;
  char cmd[4096];
  klp_fwdata_t *ud;

  ud = (klp_fwdata_t *) malloc(sizeof(klp_fwdata_t));
  klp_fw_parse_args(argc,argv,ud,d);

  printf("number of channels to save %d\n",ud->n_save_channels);

  make_directory();
  d->buffersize = 390625;

  ud->filecounter = 1;
  d->userdata = (void *)ud;

  for(i=0 ; i<ud->n_save_channels ; i++)
  {
    sprintf(cmd,"mkdir -p %03d/x",i);
    res |= system(cmd);
    sprintf(cmd,"mkdir -p %03d/y",i);
    res |= system(cmd);
  }
  ud->timestamps = fopen("timestamps.log","w");
  ud->samplerlog = fopen("sampler.log","w");
  ud->logfile_written=0;
  if (res != 0)
  {
    printf("Warning: non-zero return status (|%d)",res);
  }
}

/*
  This will be called by the data processing thread
 */
void klp_proc(recorder_t *d)
{
  klp_fwdata_t *ud;
  char str[4096];
  int i;
  FILE *f;
  ud = (klp_fwdata_t *)d->userdata;

  for(i=0 ; i < MIN(d->n_channels,ud->n_save_channels) ; i++)
  {
    sprintf(str,"%03d/x/data-%06d.gdf",i,ud->filecounter);
    f = fopen(str,"w");
    fwrite(d->buffer->datax[i],sizeof(uint16_t),2*d->buffersize,f);
    fclose(f);
    sprintf(str,"%03d/y/data-%06d.gdf",i,ud->filecounter);
    f = fopen(str,"w");

    fwrite(d->buffer->datay[i],sizeof(uint16_t),2*d->buffersize,f);
    fclose(f);
  }
  fprintf(ud->timestamps,"data-%06d.gdf %1.20lf %d %d\n",ud->filecounter,d->timestamp,d->unixtime,d->seqnum);
  fflush(ud->timestamps);
  ud->filecounter++;


  if(ud->logfile_written == 0)
  {
    klp_fw_write_logfile(d);
  }
}

/* EOF */
