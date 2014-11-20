/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

/*
    This makes a saw tooth wave like so:

    |
    |   *       *       *
    |  *       *       *
    | *       *       *
    |*       *       *
    *-------*-------*-----------
           *       *
          *       *
         *       *
        *       *
*/

#include <string.h>
#include <math.h>
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

struct QsSaw
{
  struct QsSource source; // inherit QsSource
  float amp, period, periodShift, omega;
  int id;
};

static
void _qsSaw_parameterChange(struct QsSaw *s)
{
  // Another way to handle a parameter change is to
  // change other parameters too so that continuity is
  // preserved, but that may be considered misleading
  // or unexpected behavior.
  qsSource_addPenLift((struct QsSource *) s);
}

static
int cb_saw(struct QsSaw *s, long double tf,
    long double prevT, long double currentT,
    long double dt, int nFrames)
{
  struct QsSource *source;
  source = (struct QsSource *) s;

  QS_ASSERT(nFrames >= 0);
  if(nFrames == 0) return 0;

  long double amp, amp2, rate, periodShift;
  amp = s->amp;
  amp2 = amp * 2;
  rate = amp2/s->period;
  // add 2 periods to keep the fmodl() from having a negative argument.
  periodShift = (s->periodShift + 0.5) * s->period + 2 * s->period;

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

  return 1;
}

static
size_t iconText(char *buf, size_t len, struct QsSaw *s)
{
  // Make some kind of colorful glyph for this source
  // widget.
  return snprintf(buf, len,
      "<span bgcolor=\"#CF86A5\" fgcolor=\"#97C81F\">["
      "<span fgcolor=\"#3F3A21\">saw%d</span>"
      "]</span> ", s->id);
}

#define MIN_PERIOD   (0.01F)
#define MAX_PERIOD   (1000.0F)

struct QsSource *qsSaw_create(int maxNumFrames,
    float amp, float period, float periodShift, float samplesPerPeriod,
    struct QsSource *group)
{
  struct QsSaw *s;
  QS_ASSERT(amp >= 0.0);
  QS_ASSERT(period >= MIN_PERIOD && period <= MAX_PERIOD);
  QS_ASSERT(samplesPerPeriod >= 4 && samplesPerPeriod <= 1000);

  s = qsSource_create((QsSource_ReadFunc_t) cb_saw,
      1 /* numChannels */, maxNumFrames, group, sizeof(*s));
  s->amp = amp;
  s->period = period;
  s->periodShift = periodShift;
  s->id = createCount++;

  const float minMaxSampleRates[] = { 0.01F , 2*44100.0F };
  qsSource_setType((struct QsSource *) s, QS_TOLERANT, minMaxSampleRates,
      samplesPerPeriod/period/*default frame sample rate*/);


  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) s;

  adjG = qsAdjusterGroup_start(adjL, "Saw");
  qsAdjuster_setIconStrFunc(adjG,
    (size_t (*)(char *, size_t, void *)) iconText, s);

  qsAdjusterFloat_create(adjL,
      "Saw Period", "sec", &s->period,
      MIN_PERIOD, /* min */ MAX_PERIOD, /* max */
      (void (*) (void *)) _qsSaw_parameterChange, s);
  qsAdjusterFloat_create(adjL,
      "Amplitude", "", &s->amp,
      0.000000, /* min */ 10.0, /* max */
      (void (*) (void *)) _qsSaw_parameterChange, s);
  qsAdjusterFloat_create(adjL,
      "Period Shift", "", &s->periodShift,
      -1.0, /* min */ 1.0, /* max */
      (void (*) (void *)) _qsSaw_parameterChange, s);

  qsAdjusterGroup_end(adjG);

  return (struct QsSource *) s;
}
