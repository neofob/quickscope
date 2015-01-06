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

// C templating -- fun stuff
// This is the source file for qsSin and qsSaw classes
#ifdef SAW

/*
    This makes a saw tooth wave like so:

    |
    |   *       *       *
    |  *|      *|      *|
    | * |     * |     * |
    |*  |    *  |    *  |
    *-------*-------*-----------
        |  *    |  *    |  *
        | *     | *     | *
        |*      |*      |*
        *       *       *
*/

#  define CREATE         qsSaw_create
#  define NAME           saw
#  define NAME_STR       "saw"
#  define NAME_CAP_STR   "Saw"
#  define BCOLOR         "#AF86A5"
#else // default is sin

/* This make a sine wave */

#  define SIN
#  define CREATE         qsSin_create
#  define NAME           sin
#  define NAME_STR       "sin"
#  define NAME_CAP_STR   "Sine"
#  define BCOLOR         "#CF86A5"
#endif


// TODO: make this thread safe and cleaner
static int createCount = 0;

struct QsSin
{
  struct QsSource source; // inherit QsSource
  float amp, period,
        phaseShift, // periodShift in SAW
        omega, samplesPerPeriod;
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

#ifdef SAW
  long double amp, amp2, rate, periodShift;
  amp = s->amp;
  amp2 = amp * 2;
  rate = amp2/s->period;
  // add 2 periods to keep the fmodl() from having a negative argument.
  periodShift = (s->phaseShift + 0.5) * s->period + 2 * s->period;

  while(nFrames)
  {
    float *val;
    long double *t;
    int n;

    n = nFrames;
    val = qsSource_setFrames(source, &t, &n);

    for(nFrames -= n; n; --n)
    {
      if(dt)
        // this is Master source
        *t = (currentT += dt);
      *val++ = fmodl(((*t++) + periodShift) * rate  , amp2) - amp;
    }
  }
#else
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
#endif
  return 1;
}

static
size_t iconText(char *buf, size_t len, struct QsSin *s)
{
  // some kind of colorful glyph for this source
  return snprintf(buf, len,
      "<span bgcolor=\""BCOLOR"\" fgcolor=\"#97C81F\">["
      "<span fgcolor=\"#3F3A21\">"NAME_STR"%d</span>"
      "]</span> ", s->id);
}

#define MIN_PERIOD   (0.0001F)
#define MAX_PERIOD   (1000.0F)

struct QsSource *CREATE(int maxNumFrames,
    float amp, float period, float phaseShift, float samplesPerPeriod,
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
  s->phaseShift = phaseShift;
#ifdef SIN
  s->phaseShift /= M_PI;
#endif
  s->id = createCount++;
  s->samplesPerPeriod = samplesPerPeriod;

  const float minMaxSampleRates[] = { 0.01F , 2*44100.0F };
  qsSource_setFrameRateType((struct QsSource *) s, QS_TOLERANT, minMaxSampleRates,
      samplesPerPeriod/period/*default frame sample rate*/);
  qsSource_setFrameRate((struct QsSource *) s, samplesPerPeriod/period);

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) s;

  adjG = qsAdjusterGroup_start(adjL, NAME_CAP_STR);
  qsAdjuster_setIconStrFunc(adjG,
    (size_t (*)(char *, size_t, void *)) iconText, s);

  qsAdjusterFloat_create(adjL,
      NAME_CAP_STR" Period", "sec", &s->period,
      MIN_PERIOD, /* min */ MAX_PERIOD, /* max */
      (void (*) (void *)) _qsSin_parameterChange, s);
  qsAdjusterFloat_create(adjL,
      "Amplitude", "", &s->amp,
      0.000000, /* min */ 10.0, /* max */
      (void (*) (void *)) _qsSin_parameterChange, s);
  qsAdjusterFloat_create(adjL,
#ifdef SIN
      "Phase Shift", "Pi", &s->phaseShift,
      -10.0, /* min */ 10.0, /* max */
#endif
#ifdef SAW
      "Period Shift", "sec", &s->phaseShift,
      -1.0, /* min */ 1.0, /* max */
#endif
      (void (*) (void *)) _qsSin_parameterChange, s);

  qsAdjusterGroup_end(adjG);

  return (struct QsSource *) s;
}
