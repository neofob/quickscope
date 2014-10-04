/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "group.h"
#include "source.h"
#include "source_priv.h"
#include "iterator.h"

// TODO: make this thread safe and cleaner
static int createCount = 0;

struct QsSaw
{
  struct QsSource source; // inherit QsSource
  float amp, period, periodShift, omega, samplesPerSecond;
  int count;
  gboolean addPenLift;
};

static
void _qsSaw_adj(struct QsSaw *s)
{
  s->addPenLift = TRUE;
}

static
int cb_sin_read(struct QsSaw *s, long double tf, long double prevT)
{
  struct QsSource *source;
  source = (struct QsSource *) s;
  int nFrames;
  gboolean isMaster;
  isMaster = qsSource_isMaster(source);

  if(isMaster)
  {
    nFrames = s->samplesPerSecond*(tf - prevT);
    if(s->addPenLift) ++nFrames;
    if(nFrames > qsSource_maxNumFrames(source))
      // The user choose numFrames too small
      // and/or samplesPerSecond too large.
      nFrames = qsSource_maxNumFrames(source);
  }
  else
  {
    nFrames = qsSource_numFrames(source);
  }

  if(nFrames == 0) return 0;

  long double amp, amp2, dt, rate, periodShift;
  if(s->addPenLift)
    dt = (tf - prevT)/(nFrames - 1);
  else
    dt = (tf - prevT)/nFrames;
  amp = s->amp;
  amp2 = amp * 2;
  rate = amp2/s->period;
  // add 2 periods to keep the fmodl() from
  // having a negative argument.
  periodShift = s->periodShift * s->period + 2 * s->period;
  float *val;
  long double *t;

  if(s->addPenLift)
  {
    if(isMaster)
    {
      val = qsSource_setFrame(source, &t);
      *t++ = prevT;
      *val++ = QS_LIFT;
      --nFrames;
    }
    else
    {
      *qsSource_appendFrame(source) = QS_LIFT;
    }
    s->addPenLift = FALSE;
  }

  
  while(nFrames)
  {
    int n, i;
    val = qsSource_setFrames(source, &t, &n);

    for(i=0;i<n;++i)
    {
      if(isMaster)
        *t = (prevT += dt);
      *val++ = fmodl(((*t++) + periodShift) * rate  , amp2) - amp;
    }
    nFrames -= n;
  }

  return 1;
}

static
size_t iconText(char *buf, size_t len, struct QsSaw *s)
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

struct QsSource *qsSaw_create(int maxNumFrames,
    float amp, float period, float periodShift, float samplesPerSecond,
    struct QsSource *group)
{
  struct QsSaw *s;
  QS_ASSERT(amp >= 0.0);
  QS_ASSERT(period >= MIN_PERIOD && period <= MAX_PERIOD);
  QS_ASSERT(samplesPerSecond >= MIN_SAMPLES && samplesPerSecond <= MAX_SAMPLES);

  s = qsSource_create((QsSource_ReadFunc_t) cb_sin_read,
      1 /* numChannels */, maxNumFrames, group, sizeof(*s));

  s->amp = amp;
  s->period = period;
  s->periodShift = periodShift;
  s->count = ++createCount;
  s->samplesPerSecond = samplesPerSecond;

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) s;

  adjG = qsAdjusterGroup_start(adjL, "Saw");
  adjG->icon = (size_t (*)(char *, size_t, void *)) iconText;
  adjG->iconData = s;

  qsAdjusterFloat_create(adjL,
      "Saw Period", "sec", &s->period,
      MIN_PERIOD, /* min */ MAX_PERIOD, /* max */
      (void (*) (void *)) _qsSaw_adj, s);
  qsAdjusterFloat_create(adjL,
      "Amplitude", "", &s->amp,
      0.000000, /* min */ 10.0, /* max */
      (void (*) (void *)) _qsSaw_adj, s);
  qsAdjusterFloat_create(adjL,
      "Period Shift", "", &s->periodShift,
      -1.0, /* min */ 1.0, /* max */
      (void (*) (void *)) _qsSaw_adj, s);
  qsAdjusterFloat_create(adjL,
      "Samples Per Second", "", &s->samplesPerSecond,
      MIN_SAMPLES, /* min */ MAX_SAMPLES, /* max */
      (void (*) (void *)) _qsSaw_adj, s);

  qsAdjusterGroup_end(adjG);

  return (struct QsSource *) s;
}
