/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

// See nice intro to alsa at: http://www.linuxjournal.com/article/6735
//
#include <stdio.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include "quickscope.h"

#define CALL(x) \
  do\
  {\
    /* make it so (x) is only called once */\
     int ret = (x);\
     if( ret < 0 )\
     {\
        fprintf(stderr, "%s failed err=%d: %s\n", #x, ret,\
            snd_strerror(ret));\
        return 1;\
     }\
     else\
     {\
       fprintf(stderr, "%s = %d\n", #x, ret); \
     }\
  } while (0)


static inline
bool isLittleEndian(void)
{
  int num = 1;
  return (*(char*) &num == 01);
}

// This assumes that there is only one channel in the frame
static inline
bool ReadNFrames(snd_pcm_t *handle, float *buf, snd_pcm_uframes_t n)
{
  while(n)
  {
    int rc;

    rc = snd_pcm_readi(handle, buf, n);
    if(rc > 0 && rc <= n)
    {
      buf += rc;
      n -= rc;
      continue;
    }

    if(rc < 0)
    {
      fprintf(stderr,
              "snd_pcm_readi() failed: %d: %s\n",
              rc, snd_strerror(rc));
      QS_ASSERT(0);
      return true;
    }

    fprintf(stderr,
        "error: snd_pcm_readi(,,frames=%ld) returned %d and not the requested %ld\n",
        n, rc, n);
    QS_ASSERT(0);
    return true;
  }

  return false; // success
}


int main()
{
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;

  qsApp_init(NULL, NULL); // using the signal catchers for debugging

  CALL(snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0));
  snd_pcm_hw_params_malloc(&params);
  snd_pcm_hw_params_any(handle, params);
  CALL(snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED));
  CALL(snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT_LE));
  CALL(snd_pcm_hw_params_set_channels(handle, params, 1));

  unsigned int rate = 8000;
  int dir = 0;
  CALL(snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir));
  fprintf(stderr, "rate=%u  dir=%d\n", rate, dir);


  snd_pcm_uframes_t frames = 128;
  CALL(snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir));


  CALL(snd_pcm_hw_params(handle, params));


  CALL(snd_pcm_hw_params_get_period_size(params, &frames, &dir));
  fprintf(stderr, "frames=%ld\n", frames);

  float *buffer;
  buffer = g_malloc0(frames * sizeof(*buffer));

  unsigned int usec = 0;
  CALL(snd_pcm_hw_params_get_period_time(params, &usec, &dir));
  fprintf(stderr, "period_time=%u micro seconds\n", usec);
  
  snd_pcm_hw_params_free(params);

  CALL(snd_pcm_prepare(handle));

  int run;
  run = 1 /* seconds */ * 1000000 / usec;

  while(run--)
  {
    if(ReadNFrames(handle, buffer, frames))
      break;

    float *buf;
    buf = buffer;
    snd_pcm_uframes_t i;
    for(i=0; i<frames; ++i)
      printf("%g\n", *buf++);
  }

  g_free(buffer);

  CALL(snd_pcm_drain(handle));
  CALL(snd_pcm_close(handle));

  return 0;
}
