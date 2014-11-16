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
#include "../quickscope.h"

#define RUN(x) \
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


int main()
{
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;

  qsApp_init(NULL, NULL); // using the signal catchers for debugging

  RUN(snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0));
  snd_pcm_hw_params_malloc(&params);
  snd_pcm_hw_params_any(handle, params);
  RUN(snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED));
  RUN(snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT_LE));
  RUN(snd_pcm_hw_params_set_channels(handle, params, 1));

  unsigned int rate = 44100;
  int dir = 0;
  RUN(snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir));
  fprintf(stderr, "rate=%u  dir=%d\n", rate, dir);


  snd_pcm_uframes_t frames = 128;
  RUN(snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir));


  RUN(snd_pcm_hw_params(handle, params));


  RUN(snd_pcm_hw_params_get_period_size(params, &frames, &dir));
  fprintf(stderr, "frames=%ld\n", frames);

  float *buffer;
  buffer = g_malloc0(frames * sizeof(*buffer));

  unsigned int usec = 0;
  RUN(snd_pcm_hw_params_get_period_time(params, &usec, &dir));
  fprintf(stderr, "period_time=%u micro seconds\n", usec);
  
  snd_pcm_hw_params_free(params);

  RUN(snd_pcm_prepare(handle));
  
  int run;
  run = 1 /* seconds */ * 1000000 / usec;

  while(run--)
  {
    int rc;
    // We can use this blocking call to control the
    // scope ????
    // KISS: yes, great idea.  Let the blocking source 
    // read control the scope loop.
    rc = snd_pcm_readi(handle, buffer, frames);
    if(rc == frames)
    {
      int n;
      float *buf;
      buf = buffer;
      n = rc;
      while(n--)
        printf("%g\n", *buf++);
    }
    else if(rc == -EPIPE)
    {
      /* EPIPE means overrun */
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
    }
    else if(rc != frames)
    {
      fprintf(stderr,
          "error: read %d and not the requested %ld\n",
          rc, frames);
      break;
    }
    else
    {
      fprintf(stderr,
              "error from read: %d: %s\n",
              rc, snd_strerror(rc));
      break;
    }
  }


  g_free(buffer);

  RUN(snd_pcm_drain(handle));
  RUN(snd_pcm_close(handle));

  return 0;
}
