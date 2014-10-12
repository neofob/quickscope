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
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "group.h"
#include "source.h"
#include "source_priv.h"
#include "iterator.h"

// TODO: make this thread safe and cleaner
static int createCount = 0;

struct QsSin
{
  struct QsSource source; // inherit QsSource
  float amp, period, phaseShift, omega;
  int count, samplesPerPeriod;
  gboolean addPenLift;
};

static
void _qsSin_adj(struct QsSin *s)
{
  s->addPenLift = TRUE;
}

static
int cb_sin_read(struct QsSin *s, long double tf, long double prevT)
{
  struct QsSource *source;
  source = (struct QsSource *) s;
  int nFrames;
  gboolean isMaster;
  isMaster = qsSource_isMaster(source);

  if(isMaster)
  {
    nFrames = s->samplesPerPeriod*(tf - prevT)/s->period + 1;
    if(nFrames > qsSource_maxNumFrames(source))
      // The user choose numFrames too small
      // and/or samplesPerPeriod too large.
      nFrames = qsSource_maxNumFrames(source);
  }
  else
    nFrames = qsSource_numFrames(source);

  float amp, phaseShift;
  long double omega, dt;

  dt = (tf - prevT)/nFrames;
  // TODO: pre-compute this:
  omega = 2.0L*M_PIl/s->period;
  phaseShift = s->phaseShift;
  amp = s->amp;

  prevT += dt;

  int n = 1;

  while(n && nFrames)
  {
    float phi, *frames;
    long double t=0, *time;
    int i;
    n = nFrames;

    frames = qsSource_setFrames(source, &time, &n);
    
    for(i=0; i<n; ++i)
    {
      if(isMaster)
        time[i] = prevT + i*dt;
      t = time[i];
      phi = fmodl(t*omega, 2.0L*M_PIl) + phaseShift;
      frames[i] = amp * sinf(phi);
    }
    prevT += t;

    nFrames -= n;
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
      "]</span> ", s->count);
}


#define MIN_SAMPLES  (4)
#define MAX_SAMPLES  (1000000)
#define MIN_PERIOD   (0.000001F)
#define MAX_PERIOD   (1000.0F)

struct QsSource *qsSin_create(int maxNumFrames,
    float amp, float period, float phaseShift, int samplesPerPeriod,
    struct QsSource *group)
{
  struct QsSin *s;
  QS_ASSERT(amp >= 0.0);
  QS_ASSERT(period >= MIN_PERIOD && period <= MAX_PERIOD);
  QS_ASSERT(samplesPerPeriod >= MIN_SAMPLES && samplesPerPeriod <= MAX_SAMPLES);

  s = qsSource_create((QsSource_ReadFunc_t) cb_sin_read,
      1 /* numChannels */, maxNumFrames, group, sizeof(*s));

  s->amp = amp;
  s->period = period;
  s->phaseShift = phaseShift;
  s->count = ++createCount;
  s->samplesPerPeriod = samplesPerPeriod;

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) s;

  adjG = qsAdjusterGroup_start(adjL, "Sine");
  adjG->icon = (size_t (*)(char *, size_t, void *)) iconText;
  adjG->iconData = s;

  qsAdjusterFloat_create(adjL,
      "Sine Period", "sec", &s->period,
      MIN_PERIOD, /* min */ MAX_PERIOD, /* max */
      (void (*) (void *)) _qsSin_adj, s);
  qsAdjusterFloat_create(adjL,
      "Amplitude", "", &s->amp,
      0.000000, /* min */ 10.0, /* max */
      (void (*) (void *)) _qsSin_adj, s);
  qsAdjusterFloat_create(adjL,
      "Phase Shift", "2 Pi", &s->phaseShift,
      -10.0, /* min */ 10.0, /* max */
      (void (*) (void *)) _qsSin_adj, s);
  qsAdjusterInt_create(adjL,
      "Samples Per Period", "", &s->samplesPerPeriod,
      MIN_SAMPLES, /* min */ MAX_SAMPLES, /* max */
      (void (*) (void *)) _qsSin_adj, s);

  qsAdjusterGroup_end(adjG);

  return (struct QsSource *) s;
}
