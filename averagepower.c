/***********************************************************************
**
** averagepower.c
**
***********************************************************************/

#include "klp_core.h"

#define KLP_AVG_NCHAN 61

typedef struct klp_avgdata_struct_t
{
  FILE *timestamps;
  int samplecount;
  float *avgx;
  float *avgy;
  int navg;
  FILE **filesx;
  FILE **filesy;
} klp_avgdata_t;

void klp_init(int argc, char **argv, recorder_t *d)
{
  klp_avgdata_t *ad;
  char cmd[4096];
  int i;

  /*  set_realtime_priority(); */
  make_directory();

  /* let klp core know that we want 400e3 samples per block */
  d->buffersize = 400000;
  d->n_channels = KLP_AVG_NCHAN;

  ad = (klp_avgdata_t *) malloc(sizeof(klp_avgdata_t));
  ad->navg=1000;
  ad->avgx = (float *)malloc(sizeof(float)*d->buffersize/ad->navg);
  ad->avgy = (float *)malloc(sizeof(float)*d->buffersize/ad->navg);
  ad->samplecount=0;

  ad->filesx = (FILE **)malloc(sizeof(FILE *)*d->n_channels);
  ad->filesy = (FILE **)malloc(sizeof(FILE *)*d->n_channels);
  for(i=0 ; i<d->n_channels ; i++)
  {
    sprintf(cmd,"data-%03d-x.avg",i);
    ad->filesx[i] = fopen(cmd,"w");
    sprintf(cmd,"data-%03d-y.avg",i);
    ad->filesy[i] = fopen(cmd,"w");
  }
  ad->timestamps = fopen("timestamps.log","w");

  /* important! remember to store your local user data. */
  d->userdata = (void *)ad;
  printf("Allocated\n");
}

void klp_proc(recorder_t *d)
{
  klp_avgdata_t *ad;
  int i,j,k;
  float re,im;
  ad = (klp_avgdata_t *)d->userdata;

  for(i=0 ; i < MIN(d->n_channels,d->opt->maxbeamlet) ; i++)
  {
    for(j=0 ; j < d->buffersize/ad->navg ; j++)
    {
      ad->avgx[j] = 0.0;
      ad->avgy[j] = 0.0;
    }
    for(j=0 ; j < d->buffersize/ad->navg ; j++)
    {
      for(k=0 ; k < ad->navg ; k++)
      {
        /* 2*(j*1000 + k) */
        re = (float)d->buffer->datax[i][2*(j*ad->navg + k)];
        im = (float)d->buffer->datax[i][2*(j*ad->navg + k)+1];
        ad->avgx[j] = ad->avgx[j] + re*re + im*im;
        re = (float)d->buffer->datay[i][2*(j*ad->navg + k)];
        im = (float)d->buffer->datay[i][2*(j*ad->navg + k)+1];
        ad->avgy[j] = ad->avgy[j] + re*re + im*im;
      }
    }
    fwrite(ad->avgx,sizeof(float),d->buffersize/ad->navg,ad->filesx[i]);
    fwrite(ad->avgy,sizeof(float),d->buffersize/ad->navg,ad->filesy[i]);
  }
  fprintf(ad->timestamps,"%d %1.20lf %d %d %d\n",ad->samplecount,d->timestamp,d->unixtime,d->seqnum,d->n_dropped_samples);
  fflush(ad->timestamps);
  ad->samplecount++;
}

/* EOF */
