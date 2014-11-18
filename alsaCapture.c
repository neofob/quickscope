/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <string.h>
#ifndef __USE_GNU
#define ISET___USE_GNU
#define __USE_GNU
// To define M_PIl long double Pi
#endif
#include <math.h>
#ifdef ISET___USE_GNU
#undef __USE_GNU
#endif
#include <stdbool.h>
#include <gtk/gtk.h>
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "group.h"
#include "source.h"
#include "iterator.h"
#include "rungeKutta.h"
#include "sourceTypes.h"

// TODO: make this thread safe and cleaner
static int createCount = 0;

struct QsAlsaCapture
{
  struct QsSource source; // inherit QsSource
  snd_pcm_t *handle;
  int id, sampleRate;
  int frames; // frames per source read callback
};

static
void _qsAlsaCapture_parameterChange(struct QsAlsaCapture *s)
{
  // TODO: more pcm_shit() ... here.

  // Another way to handle a parameter change is to
  // change other parameters too so that continuity is
  // preserved, but that may be considered misleading
  // or unexpected behavior.
  qsSource_addPenLift((struct QsSource *) s);

  /* We get a second chance here to adjust parameters
   * here. */
  // TODO: THIS ...
  //qsSource_setMinSampleRate((struct QsSource *) s, s->sampleRate/60);
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

    if(rc == -EPIPE)
    {
#if 0
      fprintf(stderr,
              "snd_pcm_readi() failed: %d: %s\n"
              "Must be a capture buffer over-run "
              "calling snd_pcm_prepare() to fix it.\n",
              rc, snd_strerror(rc));
#endif
      snd_pcm_prepare(handle);
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

static
int cb_read(struct QsAlsaCapture *s, long double tf, long double prevT)
{
  struct QsSource *source;
  source = (struct QsSource *) s;
  int nFrames;
  bool isMaster;
  isMaster = qsSource_isMaster(source);

  nFrames = qsSource_getRequestedSamples(source, tf, prevT);
  QS_ASSERT(nFrames >= 0);

  if(nFrames == 0) return 0;

  // TODO: this will not work if the qsSource_getRequestedSamples()
  // is not using this source sampleRate.

  if(nFrames > s->frames)
  {
    // This seems to fix the scope under-run,
    // which is when the scope not getting data
    // fast enough:
    //
    // We lift the drawing pen so there will
    // be no crazy long lines in the display.
    qsSource_addPenLift(source);

    nFrames = s->frames;
 
    // The time jumps ahead.
    prevT = tf - nFrames / ((long double) s->sampleRate);
  }
  

  long double dt;
  dt = 1.0L / ((long double) s->sampleRate);

  while(nFrames)
  {
    float *frames;
    long double *t;
    int i, n;
    n = nFrames;

    frames = qsSource_setFrames(source, &t, &n);

    if(isMaster)
      for(i=0; i<n; ++i)
        *t++ = (prevT += dt);

    if(ReadNFrames(s->handle, frames, (snd_pcm_uframes_t) n))
      return -1; // fail

    nFrames -= n;
  }
  return 1;
}

static
size_t iconText(char *buf, size_t len, struct QsAlsaCapture *s)
{
  // some kind of colorful glyph for this source
  return snprintf(buf, len,
      "<span bgcolor=\"#CF86A5\" fgcolor=\"#97C81F\">["
      "<span fgcolor=\"#3F3A21\">sound in %d</span>"
      "]</span> ", s->id);
}

static inline
bool isLittleEndian(void)
{
  int num = 1;
  return (*(char*) &num == 01);
}

static
void _qsAlsaCapture_destroy(struct QsAlsaCapture *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->handle);
  snd_pcm_close(s->handle);
}

// ALSA libasound programming gets very repetitive
// hence we use this CPP macro to call alsa functions
#define CALL(x) \
  do\
  {\
    /* make it so (x) is only called once */\
     int ret;\
     if( (ret = (x)) < 0 )\
     {\
        fprintf(stderr, "%s failed err=%d: %s\n", #x, ret,\
            snd_strerror(ret));\
       /* Cleanup and bail */\
        if(params)\
          snd_pcm_hw_params_free(params);\
        if(handle)\
          snd_pcm_close(handle);\
        qsSource_destroy((struct QsSource*) s);\
        QS_ASSERT(0);\
        return NULL;\
     }\
     QS_SPEW("%s = %d\n", #x, ret);\
  } while (0)


struct QsSource *qsAlsaCapture_create(int maxNumFrames,
    int sampleRate, struct QsSource *group)
{
  struct QsAlsaCapture *s;
  snd_pcm_t *handle = NULL;
  snd_pcm_hw_params_t *params = NULL;

  if(sampleRate <= 0)
    sampleRate = 44100; /*Hz*/

  s = qsSource_create((QsSource_ReadFunc_t) cb_read,
      1 /* numChannels */, maxNumFrames, group, sizeof(*s));
  qsSource_setMinSampleRate((struct QsSource *) s, sampleRate);

  /* BEGIN alsa init code */
  
  CALL(snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0));
  snd_pcm_hw_params_malloc(&params);
  snd_pcm_hw_params_any(handle, params);
  CALL(snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED));
  if(isLittleEndian())
    CALL(snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT_LE));
  else
    CALL(snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT_BE));

  CALL(snd_pcm_hw_params_set_channels(handle, params, 1));

  unsigned int rate = sampleRate;
  int dir = 0;
  CALL(snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir));

  snd_pcm_uframes_t frames = 32; /* TODO: What's a good value ??? */
  
  CALL(snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir));
  CALL(snd_pcm_hw_params(handle, params));
  CALL(snd_pcm_hw_params_get_period_size(params, &frames, &dir));
  QS_SPEW("pcm frames=%ld\n", frames);
  s->frames = frames;
  snd_pcm_hw_params_free(params);
  params = NULL;
  CALL(snd_pcm_prepare(handle));
  s->handle = handle;

  /* END alsa init code */
#undef CALL

  s->id = createCount++;
  s->sampleRate = sampleRate;

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) s;

  adjG = qsAdjusterGroup_start(adjL, "AlsaCapture");
  qsAdjuster_setIconStrFunc(adjG,
    (size_t (*)(char *, size_t, void *)) iconText, s);

  qsAdjusterInt_create(adjL,
      "Sample Rate", "Hz", &s->sampleRate,
      600, /* min */ 44100, /* max */
      (void (*) (void *)) _qsAlsaCapture_parameterChange, s);

  qsAdjusterGroup_end(adjG);

  qsSource_addSubDestroy(s, _qsAlsaCapture_destroy);
  
  return (struct QsSource *) s;
}
