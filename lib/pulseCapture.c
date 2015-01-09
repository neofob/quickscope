/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
/* The latency looks to be in the order of a few seconds, though it does
 * not a seem to use much CPU. The libALSA method has much lower latency
 * (much less than a second) but seems to spin the CPU some times, like a
 * spin lock of something. */

#include <unistd.h>
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
#include <pulse/simple.h>
#include <pulse/error.h>

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

static int createCount = 0;

struct QsPulseCapture
{
  struct QsSource source; // inherit QsSource
  pa_simple *handle;
  int id, sampleRate;
  int frames; // frames per source read callback
};

static inline
bool ReadNFrames(pa_simple *handle, float *buf, int n)
{
  int error;
  if(pa_simple_read(handle, buf, sizeof(float)*n, &error) < 0)
  {
    fprintf(stderr,"pa_simple_read() failed: error=%d:%s\n",
        error, pa_strerror(error));
    return true;
  }
  return false; // success
}

static
int cb_read(struct QsPulseCapture *s,
    long double tf, long double prevT,
    long double currentT,
    long double dt, int nFrames,
    bool underrun)
{
  if(nFrames == 0)
  {
    // May not be the best solution
    usleep(10000);
    return 0;
  }

  if(underrun)
  {
    // This means that we are not calling this
    // callback fast enough.

    // Skip the oldest frames?  I think that's what this
    // does.
    fprintf(stderr, "Scope buffer got underrun\n");
  }

  while(nFrames)
  {
    float *frames;
    long double *t;
    int n;

    n = nFrames;

    frames = qsSource_setFrames((struct QsSource *) s, &t, &n);

    if(ReadNFrames(s->handle, frames, n))
      return -1; // fail

    nFrames -= n;

    if(dt)
      // This is the master source it must set the time
      // stamps.
      for(; n; --n)
        *t++ = (currentT += dt);
  }
  return 1;
}

#if 0
static
size_t iconText(char *buf, size_t len, struct QsPulseCapture *s)
{
  // some kind of colorful glyph for this source
  return snprintf(buf, len,
      "<span bgcolor=\"#9F86A5\" fgcolor=\"#97C81F\">["
      "<span fgcolor=\"#7F3A21\">sound in %d</span>"
      "]</span> ", s->id);
}
#endif

static inline
bool isLittleEndian(void)
{
  int num = 1;
  return (*(char*) &num == 01);
}

static
void _qsPulseCapture_destroy(struct QsPulseCapture *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->handle);
  pa_simple_free(s->handle);
}

struct QsSource *qsPulseCapture_create(int maxNumFrames,
    int sampleRate, struct QsSource *group)
{
  struct QsPulseCapture *s;

  if(sampleRate <= 0)
    sampleRate = 44100; /*Hz*/

  s = qsSource_create((QsSource_ReadFunc_t) cb_read,
      1 /* numChannels */, maxNumFrames, group, sizeof(*s));
 
  static pa_sample_spec ss = {
    .channels = 1
  };
  int error;
  ss.format = isLittleEndian()?PA_SAMPLE_FLOAT32LE:PA_SAMPLE_FLOAT32BE;
  ss.rate = sampleRate;

  /* BEGIN pulse init code */
  
  s->handle = pa_simple_new(NULL, "Quickscope PulseAudio",
      PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error);
  QS_VASSERT(s->handle, "pa_simple_new() failed error=%d:%s\n",
      error, pa_strerror(error));
  if(!s->handle)
  {
    fprintf(stderr,"pa_simple_new() failed error=%d:%s\n",
      error, pa_strerror(error));
    return NULL;
  }

  /* END pulse init code */
#undef CALL

  s->id = createCount++;
  s->sampleRate = sampleRate;

  // TODO: make this QS_SELECTABLE
  qsSource_setFrameRateType((struct QsSource *) s, QS_FIXED, NULL, sampleRate);
  

  qsSource_addSubDestroy(s, _qsPulseCapture_destroy);
  
  return (struct QsSource *) s;
}
