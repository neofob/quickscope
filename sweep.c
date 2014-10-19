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

// There are 4 states for this sweep
// held, armed, triggered, and running
enum STATE { HELD, ARMED, TRIGGERED, RUNNING };


struct QsSweep
{
  // inherit QsSource
  struct QsSource source;
  struct QsSource *sourceIn; // The source we read to make
  // this sweep source.

  struct QsIterator *backIt, // for reading trigger source
    *timeIt; // for reading the time stamps that we write after
    // getting triggered.  backIt reads ahead of timeIt
    // until there is a trigger value is read, and then timeIt
    // is used to get the appropriate frames that depend on the
    // holdOff parameter.
  long double startT, lastTRead, waitUntilT;
  float period, holdOff, oldHoldOff, delay, oldDelay,
        level, lastValueRead, lastValueOut;
  int slope; /* +1 or 0 for free run or -1 */
  int channelNum;
  int id; // this sweep createCount
  int sourceInID;
  enum STATE state; // sweep state when not free run
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
int cb_sweep(struct QsSweep *sw,
    long double tf, long double prevT, void *data)
{
  struct QsSource *s;
  s = (struct QsSource *) sw;
  float lastValueOut;
  lastValueOut = sw->lastValueOut;
  float x;
  long double lastTRead;
  lastTRead = sw->lastTRead;
  struct QsIterator *tit;
  tit = sw->timeIt;
  long double startT;
  startT = sw->startT;
  long double *t = NULL;
  float period;
  period = sw->period;
 
  if(!sw->slope && !sw->delay && !sw->holdOff)
  {
    // free run with no waiting
 
    while(qsIterator_get(tit, &x, &lastTRead))
    {
      float val;
      
      val = _qsSweep_val(lastTRead, startT, period);
      if(val < lastValueOut)
      {
        // Outputting a decrease in value
        // which is sweeping back with the same
        // time stamp, *t.
        *qsSource_appendFrame(s) = QS_LIFT;
        *qsSource_appendFrame(s) = lastValueOut - 1.0F;
      }

      *qsSource_setFrame(s, &t) = val;
      
      QS_ASSERT(lastTRead == *t);

      lastValueOut = val;
    }

    if(t)
    {
      sw->lastValueOut = lastValueOut;
      sw->lastTRead = lastTRead;
      return 1; // yes new data
    }
    return 0; // no new data
  }

  //////////////////////////////////////
  // General case
  //////////////////////////////////////
  
  long double waitUntilT;
  waitUntilT = sw->waitUntilT;
  float lastValueRead;
  lastValueRead = sw->lastValueRead;
  enum STATE state;
  state = sw->state;
  int slope;
  slope = sw->slope;
  float level;
  level = sw->level;
  prevT = sw->lastTRead;

  if(qsIterator_get(tit, &x, &lastTRead))
    do
    {

      if(state == HELD)
      {
        do
          {
            if(lastTRead >= waitUntilT)
            {
              if(slope > 0)
                lastValueRead = INFINITY;
              else if(slope < 0)
                lastValueRead = -INFINITY;
              state = ARMED;
              break;
            }
            prevT = lastTRead;
            // We must write one for each read on tit.
            *qsSource_setFrame(s, &t) = QS_LIFT;
          }
          while(qsIterator_get(tit, &x, &lastTRead)); // read
        if(state == HELD) break;
      }

      
      if(state == ARMED && slope > 0)
      {
        do
          {
            if(lastValueRead <= level &&
                level <= x && lastValueRead < x)
            {
              state = TRIGGERED;
              break;
            }
            
            lastValueRead = x;
            prevT = lastTRead;
            // We must write one for each read on tit.
            *qsSource_setFrame(s, &t) = QS_LIFT;
          }
          while(qsIterator_get(tit, &x, &lastTRead));
      }
      else if(state == ARMED && slope < 0)
      {
        do
          {
            if(lastValueRead >= level &&
                level >= x && lastValueRead > x)
            {
              state = TRIGGERED;
              break;
            }
            
            lastValueRead = x;
            prevT = lastTRead;
            // We must write one for each read on tit.
            *qsSource_setFrame(s, &t) = QS_LIFT;
          }
          while(qsIterator_get(tit, &x, &lastTRead));
      }
      else if(state == ARMED)
      {
        // free run
        state = TRIGGERED;
      }
      if(state == ARMED) break;


      if(state == TRIGGERED && sw->delay > 0)
      {
        do
          {
            if(1)
            {
          
              state = RUNNING;
              break;
            }
            prevT = lastTRead;
            // We must write one for each read on tit.
            *qsSource_setFrame(s, &t) = QS_LIFT;
          }
          while(qsIterator_get(tit, &x, &lastTRead));
        if(state == TRIGGERED) break;
      }

      else if(state == TRIGGERED && sw->delay < 0)
      {
        do
          {
            if(1)
            {
              state = RUNNING;
              break;
            }
            prevT = lastTRead;
            // We must write one for each read on tit.
            *qsSource_setFrame(s, &t) = QS_LIFT;
          }
          while(qsIterator_get(tit, &x, &lastTRead));
        if(state == TRIGGERED) break;
      }


      // state == RUNNING

      do
        {
          float val; 
          val = _qsSweep_val(lastTRead, startT, period);

          if(val < lastValueOut)
          {
            state = HELD;
            waitUntilT = lastTRead + sw->holdOff;
            *qsSource_setFrame(s, &t) = val + 1.0F;
            *qsSource_appendFrame(s) = QS_LIFT;
            lastValueOut = -INFINITY;
            break;
          }

          *qsSource_setFrame(s, &t) = (lastValueOut = val);
          prevT = lastTRead;
        }
        while(qsIterator_get(tit, &x, &lastTRead));

      if(state == HELD && !qsIterator_get(tit, &x, &lastTRead))
        break;
    }
    while(state == HELD);


  sw->waitUntilT = waitUntilT;
  sw->lastTRead = lastTRead;
  sw->state = state;
  sw->startT = startT;
  sw->lastValueRead = lastValueRead;
  sw->lastValueOut = lastValueOut;

  return (t)?1:0;
}

static
int cb_sweep_init(struct QsSweep *sw,
    long double tf, long double prevT, void *data)
{
  struct QsSource *s;
  s = (struct QsSource *) sw;

  sw->lastTRead = sw->startT = prevT;
  sw->waitUntilT = prevT + sw->holdOff;

  qsSource_setReadFunc(s, (QsSource_ReadFunc_t) cb_sweep);
  return cb_sweep(sw, tf, prevT, data);
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
void addIcon(struct QsAdjuster *adj, struct QsSweep *t)
{
  adj->icon =
    (size_t (*)(char *, size_t, void *)) iconText;
  adj->iconData = t;
}

static inline
void _qsSweep_setContStart(struct QsSweep *sweep)
{
  /* This keeps continuity in the trace so
   * we don't need to lift the pen. */
  if(sweep->lastValueOut <= 0.5F && sweep->lastValueOut >= -0.5F)
    sweep->startT = sweep->lastTRead -
        (sweep->lastValueOut + 0.5F)*sweep->period;
  else
    sweep->startT = sweep->lastTRead;
}

static
void cb_changePeriod(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);

  if(!sweep->slope)
  {
    // mode == free run
    _qsSweep_setContStart(sweep);
  }
}

static
void cb_changeSlope(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
  if(!sweep->slope)
  {
    // free run
    // This will cause us to write QS_LIFT in the next frame
    // written.
    sweep->lastValueOut = INFINITY;
    qsSource_initIterator((struct QsSource *) sweep, sweep->timeIt);
  }
  else if(sweep->slope > 0)
    sweep->lastValueRead = INFINITY;
  else if(sweep->slope < 0)
    sweep->lastValueRead = -INFINITY;
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
  gboolean free, wasFree;
  free = !sweep->slope && !sweep->holdOff && !sweep->delay;
  wasFree = !sweep->slope && !sweep->holdOff && !sweep->oldDelay;

  _qsSweep_setContStart(sweep);
  if(!free && wasFree)
    sweep->state = RUNNING;
  
}

static
void cb_changeHoldOff(struct QsSweep *sweep)
{
  QS_ASSERT(sweep);
  gboolean free, wasFree;
  free = !sweep->slope && !sweep->holdOff && !sweep->delay;
  wasFree = !sweep->slope && !sweep->oldHoldOff && !sweep->delay;

  if(!free && sweep->state == HELD)
    sweep->waitUntilT += sweep->holdOff - sweep->oldHoldOff;

  sweep->oldHoldOff = sweep->holdOff;
  _qsSweep_setContStart(sweep);

  if(!free && wasFree)
    sweep->state = RUNNING;
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
  QS_ASSERT(delay >= 0);
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
  sweep->slope = (slope>0)?1:((slope<0)?-1:0); // slope==0 is free run
  sweep->delay = delay;
  sweep->oldDelay = delay;
  if(slope > 0)
    sweep->lastValueRead = INFINITY;
  else
    sweep->lastValueRead = -INFINITY;
  sweep->lastValueOut = -INFINITY;
  sweep->channelNum = channelNum;
  sweep->id = createCount++;
  sweep->backIt = qsIterator_create(sourceIn, channelNum);
  sweep->timeIt = qsIterator_create(sourceIn, channelNum);
  qsSource_initIterator((struct QsSource *) sweep, sweep->timeIt);
  sweep->state = HELD;

  // A Sweep is a Source is an AdjusterList.
  // Ain't inheritance fun:

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) sweep;

  addIcon(adjG = qsAdjusterGroup_start(adjL, "Sweep"), sweep);

  addIcon(
      qsAdjusterFloat_create(adjL,
      "Sweep Period", "sec", &sweep->period,
      0.01, /* min */ 1000.0, /* max */
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
      "Draw Delay", "sec", &sweep->delay,
      -1000.0F, /* min */ 1000.0F, /* max */
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

