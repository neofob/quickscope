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

struct QsSin
{
  struct QsSource source; // inherit QsSource
  float amp, period, phaseShift, omega, samplesPerPeriod;
  int id;
};

static
void _qsSin_parameterChange(struct QsSin *sin)
{
  // Another way to handle a parameter change is to
  // change other parameters too so that continuity is
  // preserved, but that may be considered misleading
  // or unexpected behavior.
  
  qsSource_addPenLift((struct QsSource *) sin);
  qsSource_setFrameRate((struct QsSource *) sin, sin->samplesPerPeriod/sin->period);

#if 0
  QS_SPEW("qsSource_setFrameRate()=%g sin->samplesPerPeriod/sin->period=%g\n",
    qsSource_getFrameRate((struct QsSource *) sin), sin->samplesPerPeriod/sin->period);
#endif
}

static
int cb_read(struct QsSin *s, long double tf,
    long double prevT, long double currentT,
    long double dt, int nFrames)
{
  struct QsSource *source;
  source = (struct QsSource *) s;
 
  QS_ASSERT(nFrames >= 0);
  if(nFrames == 0) return 0;

  float amp, phaseShift;
  long double omega;
  omega = 2.0L*M_PIl/s->period;
  phaseShift = s->phaseShift * M_PI;
  amp = s->amp;

  while(nFrames)
  {
    float phi, *frames;
    long double *t;
    int n;

    n = nFrames;
    frames = qsSource_setFrames(source, &t, &n);

    for(nFrames -= n; n; --n)
    {
      if(dt)
        // we are the master
        *t = (currentT += dt);
      phi = fmodl((*t++)*omega, 2.0L*M_PIl) + phaseShift;
      *frames++ = amp * sinf(phi);
    }
  }
  return 1;
}

static
size_t iconText(char *buf, size_t len, struct QsSin *s)
{
  // some kind of colorful glyph for this source
  return snprintf(buf, len,
      "<span bgcolor=\"#CF86A5\" fgcolor=\"#97C81F\">["
      "<span fgcolor=\"#3F3A21\">sine%d</span>"
      "]</span> ", s->id);
}

#define MIN_PERIOD   (0.0001F)
#define MAX_PERIOD   (1000.0F)

struct QsSource *qsSin_create(int maxNumFrames,
    float amp, float period, float phaseShift, int samplesPerPeriod,
    struct QsSource *group)
{
  struct QsSin *s;
  QS_ASSERT(amp >= 0.0);
  QS_ASSERT(period >= MIN_PERIOD && period <= MAX_PERIOD);
  QS_ASSERT(samplesPerPeriod >= 4 && samplesPerPeriod <= 10000);

  s = qsSource_create((QsSource_ReadFunc_t) cb_read,
      1 /* numChannels */, maxNumFrames, group, sizeof(*s));
  s->amp = amp;
  s->period = period;
  s->phaseShift = phaseShift/M_PI;
  s->id = createCount++;
  s->samplesPerPeriod = samplesPerPeriod;

  const float minMaxSampleRates[] = { 0.01F , 2*44100.0F };
  qsSource_setFrameRateType((struct QsSource *) s, QS_TOLERANT, minMaxSampleRates,
      samplesPerPeriod/period/*default frame sample rate*/);
  qsSource_setFrameRate((struct QsSource *) s, samplesPerPeriod/period);

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) s;

  adjG = qsAdjusterGroup_start(adjL, "Sine");
  qsAdjuster_setIconStrFunc(adjG,
    (size_t (*)(char *, size_t, void *)) iconText, s);

  qsAdjusterFloat_create(adjL,
      "Sine Period", "sec", &s->period,
      MIN_PERIOD, /* min */ MAX_PERIOD, /* max */
      (void (*) (void *)) _qsSin_parameterChange, s);
  qsAdjusterFloat_create(adjL,
      "Amplitude", "", &s->amp,
      0.000000, /* min */ 10.0, /* max */
      (void (*) (void *)) _qsSin_parameterChange, s);
  qsAdjusterFloat_create(adjL,
      "Phase Shift", "Pi", &s->phaseShift,
      -10.0, /* min */ 10.0, /* max */
      (void (*) (void *)) _qsSin_parameterChange, s);

  qsAdjusterGroup_end(adjG);

  return (struct QsSource *) s;
}
