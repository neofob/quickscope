/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <string.h>
#include <math.h>
#include <inttypes.h>
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
#include "sweep.h"

// TODO: clearly this is not thread safe
static int createCount = 0;

struct QsSweep
{
  // inherit QsSource
  struct QsSource source;

  struct QsIterator *triggerIt, // for reading trigger source
    *timeIt; // for reading the time stamps that we write after
    // getting triggered.  triggerIt reads ahead of timeIt
    // until there is a trigger value is read, and then timeIt
    // is used to get the appropriate frames that depend on the
    // holdOff parameter.
  long double start, holdOff, waitUntil, delay;
  float period, level, lastValueIn, lastTIn;
  int slope; /* +1 or 0 for free run or -1 */
  int channelNum;
  int count; // this sweep createCount
};


static
int cb_sweep(struct QsSweep *sweep,
    long double tf, long double prevT, void *data)
{
  return 1;
}

static
size_t iconText(char *buf, size_t len, struct QsSweep *t)
{
  // some kind of stupid glyph for this sweep
  return snprintf(buf, len,
      "<span bgcolor=\"#FF86C5\" fgcolor=\"#97C81F\">["
      "<span fgcolor=\"#3F3A21\">sweep%d</span>"
      "]</span> ", t->count);
}

static inline
void addIcon(struct QsAdjuster *adj, struct QsSweep *t)
{
  adj->icon =
    (size_t (*)(char *, size_t, void *)) iconText;
  adj->iconData = t;
}

static
void cb_changePeriod(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
  // This will reset the sweep.
  sweep->start = INFINITY;
  if(sweep->slope > 0)
    sweep->lastValueIn = INFINITY;
  else
    sweep->lastValueIn = -INFINITY;
}

static
void cb_changeSlope(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
}

static
void cb_changeLevel(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
}

static
void cb_changeDelay(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
}

static
void cb_changeHoldOff(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
}

static void
_qsSweep_destroy(struct QsSweep *s)
{
  QS_ASSERT(s);
  qsIterator_destroy(s->triggerIt);
  qsIterator_destroy(s->timeIt);

  qsSource_checkBaseDestroy(s);
}

struct QsSource *qsSweep_create(
    float period, float level, int slope, long double holdOff,
    long double delay,
    struct QsSource *sourceIn, int channelNum)
{
  struct QsSweep *sweep;

  QS_ASSERT(period > 0);
  QS_ASSERT(delay >= 0);
  QS_ASSERT(holdOff >= 0);

  sweep = qsSource_create(
      (QsSource_ReadFunc_t) cb_sweep,
      1 /*numChannels*/,
      0 /*maxNumFrames*/,
      sourceIn /*source group*/,
      sizeof(*sweep));
  sweep->period = period;
  sweep->start = INFINITY;
  sweep->holdOff = holdOff;
  sweep->level = level;
  sweep->slope = (slope>0)?1:((slope<0)?-1:0); // slope==0 is free run
  sweep->delay = delay;
  sweep->waitUntil = -INFINITY;
  if(slope > 0)
    sweep->lastValueIn = INFINITY;
  else
    sweep->lastValueIn = -INFINITY;
  sweep->channelNum = channelNum;
  sweep->count = createCount++;
  sweep->triggerIt = qsIterator_create(sourceIn, channelNum);
  sweep->timeIt = qsIterator_create(sourceIn, channelNum);
  qsSource_initIt((struct QsSource *) sweep, sweep->timeIt);

  // A Sweep is a Source is an AdjusterList.
  // Ain't inheritance fun:

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) sweep;

  addIcon(adjG = qsAdjusterGroup_start(adjL, "Sweep"), sweep);

  addIcon(
      qsAdjusterFloat_create(adjL,
      "Sweep Period", "sec", &sweep->period,
      0.0000001, /* min */ 1000.0, /* max */
      (void (*)(void *)) cb_changePeriod, sweep), sweep);
  addIcon(
      qsAdjusterFloat_create(adjL,
      "sweep Level", "", &sweep->level,
      -100.0, /* min */ 100.0, /* max */
      (void (*)(void *)) cb_changeLevel, sweep), sweep);
  addIcon(
      qsAdjusterLongDouble_create(adjL,
      "Hold Off", "sec", &sweep->holdOff,
      0.0L, /* min */ 1000.0L, /* max */
      (void (*)(void *)) cb_changeHoldOff, sweep), sweep);
  addIcon(
      qsAdjusterLongDouble_create(adjL,
      "Delay", "sec", &sweep->delay,
      0.0L, /* min */ 1000.0L, /* max */
      (void (*)(void *)) cb_changeDelay, sweep), sweep);
  {
    int slopeValues[] = { -1, 0, 1 };
    const char *label[] = { "-", "free", "+" };
 
    addIcon(
      qsAdjusterSelector_create(adjL,
      "Slope", &sweep->slope,
      slopeValues, label, 3 /* num values */,
      (void (*)(void *)) cb_changeSlope, sweep), sweep);
  }

  qsAdjusterGroup_end(adjG);

  qsSource_addSubDestroy(sweep, _qsSweep_destroy);

  return (struct QsSource *) sweep;
}

