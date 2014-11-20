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
#include "sourceParticular.h"

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
int cb_read(struct QsAlsaCapture *s,
    long double tf, long double prevT,
    long double currentT,
    long double dt, int nFrames,
    bool underrun)
{
  struct QsSource *source;
  source = (struct QsSource *) s;

  if(nFrames == 0) return 0;

  if(underrun)
  {
    // This means that we are not calling this
    // callback fast enough.

    // Skip the oldest frames?  I think that's what this
    // does.
    snd_pcm_sframes_t total;
    total = snd_pcm_forwardable(s->handle);
    if(total > nFrames)
      snd_pcm_forward(s->handle, total - nFrames);
  }

  while(nFrames)
  {
    float *frames;
    long double *t;
    int n;

    n = nFrames;

    frames = qsSource_setFrames(source, &t, &n);

    if(ReadNFrames(s->handle, frames, (snd_pcm_uframes_t) n))
      return -1; // fail

    nFrames -= n;

    if(dt)
      // This is the master source
      for(; n; --n)
        *t++ = (currentT += dt);
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
  snd_pcm_drain(s->handle);
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

  // TODO: figure out what the sample rate really is.
  // TODO: make this QS_SELECTABLE
  qsSource_setType((struct QsSource *) s, QS_FIXED, NULL, sampleRate);
  
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
