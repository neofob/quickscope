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
#include "adjuster.h"
#include "adjuster_priv.h"
#include "sourceGroup.h"
#include "source.h"
#include "source_priv.h"
#include "composer.h"
#include "composer_priv.h"
#include "sweep.h"
#include "drawsync.h"

// TODO: clearly this is not thread safe
static int createCount = 0;

struct QsSweep
{
  struct QsComposer composer; // base object

  long double start, holdOff, waitUntil, delay;
  float period, level, lastValueIn, lastTIn;
  int slope; /* +1 or 0 for free run or -1 */
  int channelNum;
  int count; // this sweep createCount
};


static
int cb_sweep(struct QsSweep *sweep, float *valOut,
    long double *tOut, long double t, long double prevT,
    int *numFrames, void *data)
{
  int numFramesIn, i, gotValue = 0, channelNumIn;
  struct QsSource *sourceIn;

  sourceIn = sweep->composer.sourcesIn->data;
  numFramesIn = _qsSource_framesWritten(sourceIn);

  if(numFramesIn == 0)
    /* this is in the case when there is no new data */
    return 0;

  if(_qsSource_time(sourceIn, numFramesIn-1) < sweep->waitUntil)
  {
    tOut[0] = _qsSource_time(sourceIn, 0);
    /* We must cut the line or lift the pen
     * in case the pen is down now. */
    valOut[0] = NAN;
    *numFrames = 1;
    return 1;
  }

  channelNumIn = sweep->channelNum;

  /* We must produce the same number of frames as
   * sourceIn->framesWritten or less if no drawing is needed. */

  for(i=0;i<numFramesIn;++i)
  {
    long double tIn;

    tOut[i] = tIn = _qsSource_time(sourceIn, i);

    if(!isinf(sweep->start))
    {
      // A trace is sweeping

      if(tIn >= sweep->start)
      {
        valOut[i] = -0.5 + (tIn - sweep->start)/sweep->period;
        if(valOut[i] < 0.5)
          gotValue = 1; /* We got data this call! */
        else
        {
          sweep->waitUntil = sweep->holdOff + tIn;
          valOut[i] = NAN;
          sweep->start = INFINITY;
        }
      }
      else // tIn < sweep->start
        valOut[i] = NAN;
    }
    else if(tIn >= sweep->waitUntil
      && !isnan(sweep->lastValueIn)
      && !isnan(_qsSource_value(sourceIn, channelNumIn, i)))
    {
      if(sweep->slope == 0)
      {
        /* slope == 0 is free run */
        gotValue = 1;  /* We got data this call! */
        sweep->start = tIn;
        valOut[i] = - 0.5;
      }
      else if(
          (sweep->slope > 0 &&
          sweep->lastValueIn < sweep->level &&
          _qsSource_value(sourceIn, channelNumIn, i) >= sweep->level)
        ||
          (sweep->slope < 0 &&
          sweep->lastValueIn > sweep->level &&
          _qsSource_value(sourceIn, channelNumIn, i) <= sweep->level))
      {
        /* Linearly interpolate a sweep start time.
         * Without this interpolation the trace will giggle. */
        sweep->start = tIn - (_qsSource_value(sourceIn, channelNumIn, i) - sweep->level)
          * (tIn - sweep->lastTIn)/(_qsSource_value(sourceIn, channelNumIn, i) -
              sweep->lastValueIn) +
         sweep->delay;

        if(tIn >= sweep->start)
        {
          valOut[i] = - 0.5 + (tIn - sweep->start)/sweep->period;
          gotValue = 1;  /* We got data this call! */
        }
        else
          valOut[i] = NAN;
      }
      else
        valOut[i] = NAN;
    }
    else
      valOut[i] = NAN;

    sweep->lastValueIn = _qsSource_value(sourceIn, channelNumIn, i);
    sweep->lastTIn = tIn;
  }

  if(gotValue)
    *numFrames = numFramesIn;
  else
    /* We just send (mark) the first NAN to signal
     * lift the pen incase it's in the middle
     * of drawing lines and the pen is down. */
    *numFrames = 1;


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

struct QsSource *qsSweep_create(
    float period, float level, int slope, long double holdOff,
    long double delay,
    struct QsSource *sourceIn, int channelNum)
{
  struct QsSweep *sweep;

  QS_ASSERT(period > 0);
  QS_ASSERT(delay >= 0);
  QS_ASSERT(holdOff >= 0);

  sweep = qsComposer_create(
      (QsComposer_ReadFunc_t) cb_sweep,
      1 /* one channel out */,
      sourceIn->numFrames,
      sizeof(*sweep));
  qsComposer_addSourceIn(&sweep->composer, sourceIn);
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

  // A Sweep is a Composer is a Source is an AdjusterList.
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
      NULL, NULL), sweep);
  addIcon(
      qsAdjusterLongDouble_create(adjL,
      "Hold Off", "sec", &sweep->holdOff,
      0.0L, /* min */ 1000.0L, /* max */
      NULL, NULL), sweep);
  addIcon(
      qsAdjusterLongDouble_create(adjL,
      "Delay", "sec", &sweep->delay,
      0.0L, /* min */ 1000.0L, /* max */
      NULL, NULL), sweep);
  {
    int slopeValues[] = { -1, 0, 1 };
    const char *label[] = { "-", "free", "+" };
 
    addIcon(
      qsAdjusterSelector_create(adjL,
      "Slope", &sweep->slope,
      slopeValues, label, 3 /* num values */,
      NULL, NULL), sweep);
  }

  qsAdjusterGroup_end(adjG);

  return (struct QsSource *) sweep;
}

