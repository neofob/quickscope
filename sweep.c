/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "group.h"
#include "source.h"
#include "rungeKutta.h"
#include "sourceParticular.h"
#include "iterator.h"

// TODO: clearly this is not thread safe
static int createCount = 0;

// There are 4 states for this sweep
// held, armed, triggered, and run
// Unless we are in "free-run" mode with no delay and holdoff
// we always cycle through all 4 states in sequence.
enum STATE { HELD, ARMED, TRIGGERED, RUN, POSTRUN };


struct QsSweep
{
  // inherit QsSource
  struct QsSource source;
  struct QsSource *sourceIn; // The source we read to make
  // this sweep source.

  struct QsIterator *timeIt, // for reading trigger source
    *backIt; // for reading back in time when there is negative
    // draw delay.
  long double startT, prevTIn, holdoffUntilT;
  float period, holdOff, oldHoldOff, delay, newDelay,
        level, prevValueRead, prevValueOut;
  int slope, oldSlope; /* +1 or 0 for free run or -1 */
  int id; // this sweep createCount
  int sourceInID;
  enum STATE state; // sweep state when not free run
  bool wasHoldoff; // holdoff change any number of
  // times in a cycle, which means we can't tell if
  // there was a holdoff by looking at the value of holdoff,
  // so we use this wasHoldoff flag.
};

static inline
float _qsSweep_val(long double t, long double startT,
    float period)
{
  // TODO: Figure out trace better
  // scaling shit for here.
  return (float) fmodl(t - startT, period)/period - 0.5F;
}

static
int cb_sweep(struct QsSweep *sw)
{
  // TODO: add underrun correction

  struct QsSource *s;
  s = (struct QsSource *) sw;
  float prevValueOut;
  prevValueOut = sw->prevValueOut;
  float y;
  long double t, prevTIn;
  prevTIn = sw->prevTIn;
  struct QsIterator *tit;
  tit = sw->timeIt;
  long double startT;
  startT = sw->startT;
  long double *tOut = NULL;
  float period;
  period = sw->period;
 
  if(!sw->slope && !sw->delay && !sw->holdOff && 0)
  {
    // free run with no waiting
 
    while(qsIterator_get(tit, &y, &t))
    {
      float val;

      val = _qsSweep_val(t, startT, period);
      if(val < prevValueOut)
      {
        // Outputting a decrease in value
        // which is sweeping back with the same
        // time stamp, *t.
        *qsSource_appendFrame(s) = QS_LIFT;
        *qsSource_appendFrame(s) = prevValueOut - 1.0F;
      }

      *qsSource_setFrame(s, &tOut) = val;

      QS_ASSERT(t == *tOut);
      prevValueOut = val;
      prevTIn = t;
    }

    if(tOut)
    {
      sw->prevValueOut = prevValueOut;
      sw->prevTIn = prevTIn;
      return 1; // yes new data
    }
    return 0; // no new data
  }

  //////////////////////////////////////
  // General case
  //////////////////////////////////////
  
  long double holdoffUntilT;
  holdoffUntilT = sw->holdoffUntilT;
  float prevValueRead;
  prevValueRead = sw->prevValueRead;
  enum STATE state;
  state = sw->state;
  int slope;
  slope = sw->slope;
  float level;
  level = sw->level;

  if(qsIterator_get(tit, &y, &t))
    do
    {

      long double checkT;
      checkT = t;

      if(state == HELD)
      {
        do
          {
            if(t >= holdoffUntilT)
            {
              if(slope > 0)
                prevValueRead = INFINITY;
              else if(slope < 0)
                prevValueRead = -INFINITY;
              state = ARMED;
              startT = holdoffUntilT;
              break;
            }
            prevTIn = t;
          }
          while(qsIterator_get(tit, &y, &t)); // read

        if(checkT != t)
          sw->wasHoldoff = true;

        if(state == HELD)
          break;
      }

      
      if(state == ARMED && slope > 0)
      {
        do
          {
            if(prevValueRead <= level &&
                level <= y && prevValueRead < y)
            {
              state = TRIGGERED;
              // reset flag
              sw->wasHoldoff = false;
              // linearly interpolate a start time
              startT = prevTIn +
                (level - prevValueRead)*(t - prevTIn)/(y - prevValueRead)
                + sw->delay;
               prevValueOut = -INFINITY;
              break;
            }
            prevValueRead = y;
            prevTIn = t;
          }
          while(qsIterator_get(tit, &y, &t));
      }
      else if(state == ARMED && slope < 0)
      {
        do
          {
            if(prevValueRead >= level &&
                level >= y && prevValueRead > y)
            {
              state = TRIGGERED;
              // reset flag
              sw->wasHoldoff = false;
              // linearly interpolate a start time
              startT = prevTIn +
                (level - prevValueRead)*(t - prevTIn)/(y - prevValueRead)
                + sw->delay;
              prevValueOut = -INFINITY;
              break;
            }
            
            prevValueRead = y;
            prevTIn = t;
          }
          while(qsIterator_get(tit, &y, &t));
      }
      else if(state == ARMED)
      {
        // free run and maybe hold off and/or delay
        state = TRIGGERED;
        if(sw->wasHoldoff)
        {
          startT = t + sw->delay;
          sw->wasHoldoff = false;
        }
        prevValueOut = -INFINITY;
      }

      if(state == ARMED)
        break;


      if(state == TRIGGERED && sw->delay == 0)
      {
        state = RUN;
      }

      else if(state == TRIGGERED && sw->delay > 0)
      {
        do
          {
            if(t >= startT)
            {
              state = RUN;
              break;
            }
            prevTIn = t;
          }
          while(qsIterator_get(tit, &y, &t));
      }

      else if(state == TRIGGERED) // sw->delay < 0
      {
        QS_ASSERT(sw->delay < 0);

        if(qsIterator_poll(sw->backIt, &y, &t))
          // TODO: make a time finding iterator function.
          // backup the iterator to an older time
          // This is not so bad, we just need to re-read
          // the last sweep of data, and not the whole
          // buffer if it's much larger than the amount
          // of data in a sweep period.
          qsIterator_copy(tit, sw->backIt);
        else
          break;

        do
          {
            if(t >= startT)
            {
              state = RUN;
              break;
            }
            prevTIn = t;
            // TODO: This does a slow linear
            // search to find t >= startT.
            // Can we do better?
          }
          while(qsIterator_get(tit, &y, &t));
      }

      if(state == TRIGGERED)
        break;


      // Catch up to the source s to one behind tit
      // by inserting QS_LIFT values in all empty
      // frames in between.
      float *valueOut = NULL;

      if(!tOut)
        *(valueOut = qsSource_setFrame(s, &tOut)) = QS_LIFT;
      QS_ASSERT(tOut);

      while(*tOut < t)
        *(valueOut = qsSource_setFrame(s, &tOut)) = QS_LIFT;
      QS_ASSERT(*tOut == t);
      // We start by writing to this valueOut frame.


    //if(state == RUN)
        do
          {
            if(!valueOut)
            {
              valueOut = qsSource_setFrame(s, &tOut);
              QS_VASSERT(*tOut == t, "*tOut=%Lg  t=%Lg\n", *tOut, t);
            }

            float val; 
            val = _qsSweep_val(t, startT, period);
            
            if(val < prevValueOut)
            {
              state = HELD;
              // We let sw->delay just change here
              // so that things stay consistent when
              // sw->delay is used between states.
              if(sw->delay != sw->newDelay)
                sw->delay = sw->newDelay;
              if(sw->holdOff >= -sw->delay)
                holdoffUntilT = t + sw->holdOff;
              else // if(sw->holdOff < -sw->delay)
                holdoffUntilT = t - sw->delay;
              if(sw->delay < 0)
                qsIterator_copy(sw->backIt, tit);

              *valueOut = val + 1.0F;
              *qsSource_appendFrame(s) = QS_LIFT;
              prevValueOut = -INFINITY;
              break;
            }

 
            *valueOut = (prevValueOut = val);

            prevTIn = t;
            valueOut = NULL; // get a new frame next loop
          }
          while(qsIterator_get(tit, &y, &t));


        if(state == HELD)
        {
          if(!qsIterator_get(tit, &y, &t))
            // No more data to read from sourceIn
            break;
        }
        else if(state == RUN)
          break;

    }
    while(state == HELD);


  sw->holdoffUntilT = holdoffUntilT;
  sw->prevTIn = prevTIn;
  sw->state = state;
  sw->startT = startT;
  sw->prevValueRead = prevValueRead;
  sw->prevValueOut = prevValueOut;

  return (tOut)?1:0;
}

// Kind of like self-removing initialization code.
// It keeps this code out of the running source
// read callback.
static
int cb_sweep_init(struct QsSweep *sw)
{
  QS_ASSERT(sw);
  struct QsSource *s;
  s = (struct QsSource *) sw;
  float x;

  // state starts at HELD

  if(qsIterator_poll(sw->timeIt, &x, &sw->startT))
  {
    if(sw->holdoffUntilT >= -sw->delay)
      sw->holdoffUntilT = sw->startT + sw->holdOff;
    else // if(sw->holdoffUntilT < -sw->delay)
      sw->holdoffUntilT = sw->startT - sw->delay;
    sw->prevTIn = sw->startT;

    // Next time they call cb_sweep():
    qsSource_setReadFunc(s, (QsSource_ReadFunc_t) cb_sweep);
    return cb_sweep(sw);
  }

  // No data to read from sourceIn yet.
  return 0;
}

static
size_t iconText(char *buf, size_t len, struct QsSweep *t)
{
  // some kind of stupid glyph for this sweep
  return snprintf(buf, len,
      "<span bgcolor=\"#FF86C5\" fgcolor=\"#97C81F\">["
      "<span fgcolor=\"#3F3A21\">sweep%d</span>"
      "]</span> ", t->id);
}

static inline
void addIcon(struct QsAdjuster *adj, struct QsSweep *sw)
{
  qsAdjuster_setIconStrFunc(adj,
      (size_t (*)(char *, size_t, void *)) iconText,
      sw);
}

static inline
void _qsSweep_setContStart(struct QsSweep *sweep)
{
  /* This keeps continuity in the trace so
   * we don't need to lift the pen. */
  if(sweep->prevValueOut <= 0.5F && sweep->prevValueOut >= -0.5F)
    sweep->startT = sweep->prevTIn -
        (sweep->prevValueOut + 0.5F)*sweep->period;
  else
    sweep->startT = sweep->prevTIn;
}

static
void cb_changePeriod(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
  //bool free;
  //free = !sweep->slope && !sweep->holdOff && !sweep->delay;
  _qsSweep_setContStart(sweep);

  // We need enough samples in the period of the sweep.
  qsSource_setFrameRate((struct QsSource *) sweep, 10/sweep->period);
}

static
void cb_changeSlope(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
  bool free, wasFree;
  free = !sweep->slope && !sweep->holdOff && !sweep->delay;
  wasFree = !sweep->oldSlope && !sweep->holdOff && !sweep->delay;

  if(free && !wasFree)
  {
    // changed to free run
    // This will cause us to write QS_LIFT in the next frame
    // written.
    sweep->prevValueOut = -INFINITY;
  }
  else if(sweep->slope > 0)
    sweep->prevValueRead = INFINITY;
  else if(sweep->slope < 0)
    sweep->prevValueRead = -INFINITY;

  sweep->oldSlope = sweep->slope;
}

static
void cb_changeLevel(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
}

#if 0
static
void cb_changeDelay(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
}
#endif

static
void cb_changeHoldOff(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
  bool free, wasFree;
  free = !sweep->slope && !sweep->holdOff && !sweep->delay;
  wasFree = !sweep->slope && !sweep->oldHoldOff && !sweep->delay;

  if(sweep->state == HELD)
  {
    if(sweep->holdOff < -sweep->delay &&
        sweep->oldHoldOff < -sweep->delay)
      ;
    else if(sweep->holdOff < -sweep->delay)
      sweep->holdoffUntilT += - sweep->delay - sweep->oldHoldOff;
    else
      sweep->holdoffUntilT += sweep->holdOff - sweep->oldHoldOff;
  }

  sweep->oldHoldOff = sweep->holdOff;

  _qsSweep_setContStart(sweep);

  if(!free && wasFree)
    sweep->state = RUN;
}

static void
_qsSweep_destroy(struct QsSource *s)
{
  QS_ASSERT(s);
  struct QsGroup *g;
  struct QsSource *si;
  g = s->group;
  QS_ASSERT(g);

  GSList *l;
  l = g_slist_find(g->sources, ((struct QsSweep *)s)->sourceIn);

  if(l && (si = l->data) &&
      ((struct QsSweep *)s)->sourceIn == si &&
      // Yes, we make sure that this is the same source.
      // It could be a different source at the same address,
      // but not if it has the same source ID unless
      // the user made a few billion sources.
      ((struct QsSweep *)s)->sourceInID == si->id)
  {
    // sourceIn is still a valid source.
    //
    // All the automatic managed source destruction
    // happens in the reverse order of the creation,
    // so these iterators should be as good as the
    // sourceIn unless the user intentionally
    // destroyed things out of order.  We check
    // anyway, because we can, giving the user more
    // options.
    qsIterator_destroy(((struct QsSweep *)s)->backIt);
    qsIterator_destroy(((struct QsSweep *)s)->timeIt);
  }

  // qsSource_checkBaseDestroy(s)
  // Is not needed since user cannot call this
  // static function.  qsSource_destroy() will
  // call _qsSweep_destroy().
}

struct QsSource *qsSweep_create(
    float period, float level, int slope, float holdOff,
    float delay, struct QsSource *sourceIn, int channelNum)
{
  struct QsSweep *sweep;

  QS_ASSERT(period > 0);
  QS_ASSERT(holdOff >= 0);

  sweep = qsSource_create(
      (QsSource_ReadFunc_t) cb_sweep_init,
      1 /*numChannels*/,
      0 /*maxNumFrames*/,
      sourceIn /*source group*/,
      sizeof(*sweep));

  sweep->sourceIn = sourceIn;
  sweep->sourceInID = sourceIn->id;
  sweep->period = period;
  sweep->holdOff = holdOff;
  sweep->oldHoldOff = holdOff;
  sweep->level = level;
  slope = (slope>0)?1:((slope<0)?-1:0); // slope==0 is free run
  sweep->slope = slope;
  sweep->oldSlope = slope;
  sweep->delay = delay;
  sweep->newDelay = delay;
  if(slope > 0)
    sweep->prevValueRead = INFINITY;
  else
    sweep->prevValueRead = -INFINITY;
  sweep->prevValueOut = -INFINITY;
  sweep->id = createCount++;
  sweep->backIt = qsIterator_create(sourceIn, channelNum);
  sweep->timeIt = qsIterator_create(sourceIn, channelNum);
  qsSource_initIterator((struct QsSource *) sweep, sweep->timeIt);
  sweep->state = HELD;
  qsSource_setIsSwipable((struct QsSource *)sweep, true);

  // The sweep does not care what the frame sample rate is.
  // The sweep is a dependent source so this frame sample rate
  // will be overridden anyway.
  const float minMaxSampleRates[] = { 0.01F , 2*44100.0F };
  qsSource_setFrameRateType((struct QsSource *) sweep, QS_TOLERANT, minMaxSampleRates,
      10/period/*default min frame sample rate*/);


  // More of struct sweep gets initialized in cb_sweep_init()
  // above.

  // A Sweep is a Source is an AdjusterList.
  // Ain't inheritance fun:

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) sweep;

  addIcon(adjG = qsAdjusterGroup_start(adjL, "Sweep"), sweep);

  addIcon(
      qsAdjusterFloat_create(adjL,
      "Sweep Period", "sec", &sweep->period,
      0.005, /* min */ 1000.0, /* max */
      (void (*)(void *)) cb_changePeriod, sweep), sweep);
  addIcon(
      qsAdjusterFloat_create(adjL,
      "sweep Level", "", &sweep->level,
      -100.0, /* min */ 100.0, /* max */
      (void (*)(void *)) cb_changeLevel, sweep), sweep);
  addIcon(
      /* holdOff adds a time delay to when we start
       * waiting for a trigger. */
      qsAdjusterFloat_create(adjL,
      "Trigger Hold Off", "sec", &sweep->holdOff,
      0.0F, /* min */ 1000.0F, /* max */
      (void (*)(void *)) cb_changeHoldOff, sweep), sweep);
  addIcon(
      /* delay adds a time delay to when we start
       * tracing after a trigger.  delay may be
       * positive of negative.  A negative delay
       * will let you view data that is before
       * the trigger event.  A negative delay
       * cannot read data from the source if the
       * history in the source buffer is long enough. */
      qsAdjusterFloat_create(adjL,
      "Draw Delay", "sec", &sweep->newDelay,
      -1000.0F, /* min */ 1000.0F, /* max */
      NULL, NULL), sweep);
      //(void (*)(void *)) cb_changeDelay, sweep), sweep);
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

