/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <math.h>
#include <inttypes.h>
#include <X11/Xlib.h>
#include <string.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win_priv.h"
#include "group.h"
#include "source.h"
#include "source_priv.h"
#include "iterator.h"
#include "trace.h"
#include "trace_priv.h"
#include "swipe_priv.h"
#include "win.h"

#define SMALL 0.01F


#define SKIP(x, y)    (isnan(x) || isnan(y))
#define NSKIP(x, y)   (!isnan(x) && !isnan(y))



// TODO: removing double culling codes


// Do not cull out due to y value when using this,
// so that the swipe may keep the lap count up to date
// with: _qsWin_swipeAddPoint(win, trace, swipe, x, y)
static inline
void _qsWin_drawPoint(struct QsWin *win,
    struct QsTrace *trace, struct QsSwipe *swipe,
    int w, int h,
    int x, int y, float r, float g, float b,
    long double t)
{
  QS_ASSERT(x >= 0 && x < w);

  if(swipe)
    // This needs valid x and "any y" so it can
    // record the current trace lap (count).
    _qsWin_swipeAddPoint(win, trace, swipe, x, y);

  if(y >= 0 && y < h)
    _qsWin_drawTracePoint(win, x, y, r, g, b, t);
}

static inline
int Round(float x)
{
  if(x >= 0)
    return (int)(x + 0.5F);
  else
    return (int)(x - 0.5F);
}

/* Each pixel (point) drawn has an associated time.  We must draw
 * the pixels in the same order as their time so that beam trace fading
 * and/or beam trace swiping can keep ordered listed of aging pixels. */

/* TODO: add line drawing with one point outside of the view port */
/* TODO: interpolate the fade time between the time at the two ends
 * of the line, passing in two fade times. But the gradient in the
 * color may use more system */

// We don't like wasting time dereferencing pointers again
// and again, so we are passing win, trace, and swipe, when
// the minimum needed would be trace.

/* This is the simplest line drawing method with no anti-aliasing.
 * Pretty much what X11 does in XDrawLine() */
static
void _qsWin_drawLine(struct QsWin *win,
    struct QsTrace *trace, struct QsSwipe *swipe,
    float x1, float y1, float x2, float y2,
    float r, float g, float b,
    long double t)
{
  QS_ASSERT(win);
  
  int w, h;
  w = win->width;
  h = win->height;

  float dy;
  dy = fabsf(y2 - y1);

  if(fabs(x2 - x1) > SMALL && dy > SMALL)
  {
    /* The line is not close to vertical or horizontal line */
    float slope, a, x, k;
    slope = (y2 -y1)/(x2 - x1);
    a = y2 - slope * x2;

    int prevX, prevY, ix, iy;

    if(x2 < x1)
    {
      /* switch the order */
      float f;
      f = x1;
      x1 = x2;
      x2 = f;
      f = y1;
      y1 = y2;
      y2 = f;
    }

    /* We have no time for anti-aliasing.  It's not just
     * this drawing time, we will have to fade any
     * pixels that we add and anti-aliasing doubles or
     * triples the number of pixels in a line, not to
     * mention the increase in colors.  So maybe with
     * a faster computer we could consider it.
     *
     * TODO: Add anti-aliasing as an option, that is
     * off by default.
     *
     * equation of the line is:  y = slope * x + a
     *
     * We go along the line a distance of 1 for each point
     * which is a x distance of cosine of the angle and
     * a y distance of sine of the angle.  slope is the
     * tangent of the angle.
     *
     * Do some trigonometry...
     * So x changes by cosine of the angle which is (k):
     */
    k = 1.0F/sqrtf(1.0F + slope * slope);

    prevX = INT_MIN;
    prevY = INT_MIN;

    // Check for Cull
    if(x2 >= w)
      x2 = w - 1;
    if(x1 < 0.0F)
      x1 = 0.0F;

    if(x1 > x2)
      // culled all the line due to x values.
      // This will not catch all culls due to
      // rounding to int.
      return;

    if(swipe)
    {
      // Dam floating point round off requires we cull check
      // yet again.
      ix = Round(x2);
      if(ix >= w)
      {
        if(Round(x1) < w)
          ix = w - 1;
        else
          return; // culled due to x values
      }

      // This will swipe away all of points older than
      // and equal to the ones with x value ix
      _qsWin_swipeRemove(win, trace, swipe, ix);
    }
    
    // We let the point drawing cull in the y
    // so X swipe works correctly.

    for(x=x1;x<x2;x+=k)
    {
      ix = Round(x);
      iy = Round(x*slope + a);
      /* We do not draw the same point twice.  That
       * will happen lots if the angle is near 45 degrees
       * and the pixels close to the line, if not for
       * this check.*/
      if((prevX != ix || prevY != iy) && ix >= 0 && ix < w)
        _qsWin_drawPoint(win, trace, swipe, w, h,
            prevX = ix, prevY = iy,
            r, g, b, t);
    }
    /* See if we got the last point, if not draw it. */
    ix = Round(x2);
    iy = Round(x2*slope + a);
    if((prevX != ix || prevY != iy) && ix >= 0 && ix < w)
      _qsWin_drawPoint(win, trace, swipe, w, h,
          ix, iy, r, g, b, t);
  }
  else if(dy > SMALL) /* fabs(x2 - x1) <= SMALL */
  {
    /* Line is close to vertical */
    int y, ymin, ymax, x;
    if(y2 > y1)
    {
      ymax = Round(y2);
      ymin = Round(y1);
    }
    else
    {
      ymax = Round(y1);
      ymin = Round(y2);
    }
    x = Round((x2 + x1)/2.0F);

    if(x >= w || x < 0)
      // Culled
      return;

    // We must let qsWin_drawPoint() cull due to y
    // so X swipe works correctly.

    if(ymin < 0)
      ymin = 0;
    if(ymax >= h)
      ymax = h - 1;

    if(swipe)
      // We may not draw, but we still need to
      // swipe old points at x and older.
      _qsWin_swipeRemove(win, trace, swipe, x);

    if(ymin > ymax)
    {
      // culled due to y, but need to keep trace lap count
      // up to date.
      if(swipe)
        _qsWin_swipeAddPoint(win, trace, swipe, x, ymin);

      return;
    }

    for(y=ymin;y<=ymax;++y)
      _qsWin_drawPoint(win, trace, swipe, w, h,
          x, y, r, g, b, t);
  }
  else /* fabs(y2 - y1) <= SMALL */
  {
    /* The line is close to horizontal or just a single point */
    int x, xmin, xmax, y;

    if(x2 > x1)
    {
      /* switch the order */
      xmin = Round(x1);
      xmax = Round(x2);
    }
    else
    {
      xmin = Round(x2);
      xmax = Round(x1);
    }

    y = Round((y2 + y1)/2.0F);

    // Culling
    if(xmax >= w)
      xmax = w - 1;
    if(xmin < 0)
      xmin = 0;

    if(xmin > xmax)
      // Culled due to x
      return;

    if(swipe)
      // We may not draw, but we still need to
      // swipe old points at x and older.
      _qsWin_swipeRemove(win, trace, swipe, xmax);

    if(y < 0 || y >= h)
    {
      // culled due to y, but need to keep trace lap count
      // up to date by passing xmax to this.
      if(swipe)
        _qsWin_swipeAddPoint(win, trace, swipe, xmax, y);
      
      return;
    }

    for(x=xmin;x<=xmax;++x)
      _qsWin_drawPoint(win, trace, swipe, w, h,
          x, y, r, g, b, t);
  }
}

static
size_t iconText(char *buf, size_t len, struct QsTrace *trace)
{
  // TODO: consider saving this and only recomputing this if
  // one or more of the colors changes.

  // color strings
  char lC[8]; // line color
  char bgC[8]; // background color
  colorStr(lC, trace->red*RMAX, trace->green*GMAX, trace->blue*BMAX);
  colorStr(bgC, trace->win->bgR, trace->win->bgG, trace->win->bgB);

  return snprintf(buf, len,
      "<span bgcolor=\"%s\" fgcolor=\"%s\" weight=\"heavy\">"
      "-----"
      "</span> ",
      bgC, lC);
}

static
void _qsTrace_cb_rescale(struct QsTrace *trace)
{
  _qsTrace_scale(trace);
  _qsWin_reconfigure(trace->win);
}

static inline
void makeAdjuster(struct  QsTrace *trace, const char *text,
    float *val, float max, float min)
{
  char desc[64];
  snprintf(desc, 64, "trace%d: %s", trace->id, text);
  qsAdjusterFloat_create(&trace->win->adjusters,
      desc, "", val, max, min,
      (void (*)(void *)) _qsTrace_cb_rescale, trace);
}

static
void _qsTrace_cb_swipe(struct  QsTrace *trace)
{
  qsTrace_setSwipeX(trace, trace->isSwipe);
}

struct QsTrace *qsTrace_create(struct QsWin *win,
      struct QsSource *xs, int xChannelNum,
      struct QsSource *ys, int yChannelNum,
      float xScale, float yScale, float xShift, float yShift,
      bool lines,
      float red, float green, float blue)
{
  struct QsTrace *trace;
  QS_ASSERT(xs);
  QS_ASSERT(ys);
  QS_ASSERT(xChannelNum >= 0);
  QS_ASSERT(yChannelNum >= 0);
  QS_ASSERT(xScale > 0.0);
  QS_ASSERT(yScale > 0.0);
  QS_ASSERT(red >= 0 && red <= 1.0F);
  QS_ASSERT(green >= 0 && green <= 1.0F);
  QS_ASSERT(blue >= 0 && blue <= 1.0F);

  if(!win)
  {
    // Use and/or make the default QsWin
    if(!qsApp->wins)
    {
      win = qsWin_create();
    }
    else
    {
      QS_ASSERT(qsApp->wins->data);
      win = qsApp->wins->data;
    }
  }
  QS_ASSERT(win);
  QS_ASSERT(qsApp->wins);
  QS_ASSERT(qsApp->wins->data);


  trace = g_malloc0(sizeof(*trace));
  trace->win = win;
  trace->it = qsIterator2_create(xs, ys, xChannelNum, yChannelNum);
  trace->xChannelNum = xChannelNum;
  trace->yChannelNum = yChannelNum;
  trace->xID = xs->id;
  trace->yID = ys->id;
  trace->xScale = xScale;
  trace->yScale = yScale;
  trace->xShift = xShift;
  trace->yShift = yShift;
  trace->lines = lines;
  trace->red = red;
  trace->green = green;
  trace->blue = blue;
  trace->prevX = NAN;
  trace->prevY = NAN;
  trace->prevPrevX = NAN;
  trace->prevPrevY = NAN;
  trace->id = win->traceCount;
  ++win->traceCount;
  _qsTrace_scale(trace);

  win->traces = g_slist_prepend(win->traces, trace);


  char desc[64];
  snprintf(desc, 64, "trace%d", trace->id);
  struct QsAdjuster *group;
  trace->adjusterGroup = group = qsAdjusterGroup_start(&win->adjusters, desc);
  group->icon = (size_t (*)(char *buf, size_t len, void *iconData)) iconText;
  group->iconData = trace;

  if(qsSource_isSwipable(xs))
  {
    snprintf(desc, 64, "trace%d: Swipe", trace->id);
    qsAdjusterBool_create(&trace->win->adjusters, desc,
        &trace->isSwipe, (void (*)(void *)) _qsTrace_cb_swipe, trace);
  }

  snprintf(desc, 64, "trace%d: Lines", trace->id);
  qsAdjusterBool_create(&trace->win->adjusters, desc,
      &trace->lines, NULL, NULL);

  makeAdjuster(trace, "X Scale", &trace->xScale,
    trace->xScale/100.0, /* min */ trace->xScale*100.0 /* max */);

  makeAdjuster(trace, "Y Scale", &trace->yScale,
    trace->yScale/100.0, /* min */ trace->yScale*100.0 /* max */);

  makeAdjuster(trace, "X Shift", &trace->xShift,
     -10.0, /* min */ 10.0 /* max */);

  makeAdjuster(trace, "Y Shift", &trace->yShift,
     -10.0, /* min */ 10.0 /* max */);

  qsAdjusterGroup_end(group);


  // Now add the source adjusters to the QsWin adjustersList
  // if there are any.
  _qsAdjusterList_prepend(&win->adjusters, &xs->adjusters);
  _qsAdjusterList_prepend(&win->adjusters, &ys->adjusters);

  // TODO: We hope that this will add users source adjusters too.
  // They should not be added after this function.
  
  return trace;
}

void _qsTrace_scale(struct QsTrace *trace)
{
  struct QsWin *win;
  QS_ASSERT(trace);
  QS_ASSERT(trace->win);
  win = trace->win;
  trace->xScalePix = trace->xScale*win->xScale;
  trace->xShiftPix = trace->xShift*win->xScale + win->xShift;
  trace->yScalePix = trace->yScale*win->yScale;
  trace->yShiftPix = trace->yShift*win->yScale + win->yShift;
  // We must lift the pen so we do not draw a line from
  // a point in the old scale to a point in the new scale.
  trace->prevX = NAN;
  trace->prevY = NAN;
  trace->prevPrevX = NAN;
  trace->prevPrevY = NAN;

  if(trace->swipe)
  {
    _qsTrace_reallocSwipe(trace);
    _qsTrace_initSwipe(trace);
  }
}

// This drawing function assumes that the x source and the y source
// have the same time values, so no temperal interpolation is needed.
static inline
void _qsTrace_drawSameXY(struct QsTrace *trace,
    struct QsSwipe *swipe, long double t)
{
  QS_ASSERT(trace);
  QS_ASSERT(trace->win);

  struct QsWin *win;
  win = trace->win;
  float prevX, prevY, x, y, r, g, b;
  long double time;
  struct QsIterator2 *it;
  it = trace->it;

  prevX = trace->prevX;
  prevY = trace->prevY;

  r = trace->red;
  g = trace->green;
  b = trace->blue;

  if(trace->lines)
  {
    float prevPrevX, prevPrevY;
    prevPrevX = trace->prevPrevX;
    prevPrevY = trace->prevPrevY;

    while(qsIterator2_get(it, &x, &y, &time))
    {
      x = x*trace->xScalePix + trace->xShiftPix;
      y = y*trace->yScalePix + trace->yShiftPix;

      /* The value NAN [SKIP()] has the effect of lifting
       * the pen and braking the line. */

      if(NSKIP(x,y) && NSKIP(prevX,prevY)
          && (x != prevX || y != prevY))
      {
        _qsWin_drawLine(win, trace, swipe,
          prevX, prevY, x, y, r, g, b, time);
      }
      else if(SKIP(prevPrevX,prevPrevY) && NSKIP(prevX,prevY) && (SKIP(x,y)) &&
          Round(prevX) >= 0 && Round(prevX) < win->width)
      {
        // In this odd case there is a x,y point with a NAN
        // on either side should at least draw a point.
        // This happened to me without this code and
        // nothing was drawn.
        if(swipe)
            _qsWin_swipeRemove(win, trace, swipe, Round(prevX));
        // This must and will cull for y
        _qsWin_drawPoint(win, trace, swipe, win->width, win->height,
              Round(prevX), Round(prevY), r, g, b, time);
      }

      prevPrevX = prevX;
      prevPrevY = prevY;

      prevX = x;
      prevY = y;
    }

    trace->prevPrevX = prevPrevX;
    trace->prevPrevY = prevPrevY;

  }
  else // just points
  {
    while(qsIterator2_get(it, &x, &y, &time))
    {

      if(NSKIP(x,y) && (x != prevX || y != prevY))
      {
        int ix, iy;
        ix = x = x*trace->xScalePix + trace->xShiftPix;
        iy = y = y*trace->yScalePix + trace->yShiftPix;
 
        if(ix >= 0 && ix < win->width)
        {
          if(swipe)
            _qsWin_swipeRemove(win, trace, swipe, ix);
          // This must and will cull for y
          _qsWin_drawPoint(win, trace, swipe,
              win->width, win->height,
              ix, iy, r, g, b, time);
        }
      }

      prevX = x;
      prevY = y;
    }
  }

  trace->prevX = prevX;
  trace->prevY = prevY;

  _qsWin_drawPoints(win); // flush points
}

void _qsTrace_draw(struct QsTrace *trace, long double t)
{
  QS_ASSERT(trace);
  QS_ASSERT(trace->win);
  QS_ASSERT(trace->it);

  struct QsSwipe *swipe;
  swipe = trace->swipe;

  if(trace->win->freezeDisplay)
  {
    if(swipe)
    {
      /* We must keep the pointCount going even though we
       * are not drawing so that swipe works consistently
       * through the freezeDisplay thingy. */
      int w;
      w = trace->win->width;
      float x, y;
      long double time;
      struct QsIterator2 *it;
      it = trace->it;

      while(qsIterator2_get(it, &x, &y, &time))
      {
        if(NSKIP(x,y))
        {
          int ix;
          ix = x*trace->xScalePix + trace->xShiftPix;
          if(x >= 0 && x < w) // cull check
          {
            // We call this so
            // counters stay consistent and nothing is plotted.
            _qsWin_swipeAddFrozenPoint(trace->win, trace, swipe, ix);
          }
        }
      }
    }

    /* We must lift the pen now so that when we
     * resume we do not draw a line connecting
     * between two non-temporally-adjacent points. */
    trace->prevPrevX = trace->prevX = QS_LIFT;
    return;
  }

  /* TODO: We are thinking about added other draw modes */

  _qsTrace_drawSameXY(trace, swipe, t); // X and Y have same times
}

void qsTrace_destroy(struct QsTrace *trace)
{
  QS_ASSERT(trace);
  QS_ASSERT(trace->it);
  QS_ASSERT(trace->win);
  QS_ASSERT(trace->win->traces);
  struct QsWin *win;
  win = trace->win;

  if(trace->swipe)
    _qsTrace_cleanupSwipe(trace);

  win->traces = g_slist_remove(win->traces, trace);


  // Now remove (or derefenence) the source adjusters from the QsWin
  // adjustersList if we can.  This may have been done in qsSource_destroy().
  // Destroying sources will destroy traces and the traces
  // may be destroyed directly, which will not destroy the trace
  // sources.   We make sure that the source in the list is the same
  // source that we started with.  If not that source must have been
  // destroyed a little before now.  Other traces may have used
  // one or both of these sources, so removing the source
  // adjusters is really just decrementing reference counter.
  if(g_slist_find(qsApp->sources, trace->it->source0) &&
      trace->xID == trace->it->source0->id)
    // The x source has not been destroyed yet so we dereference
    // its' adjusters.
    _qsAdjusterList_remove(&win->adjusters, &trace->it->source0->adjusters);

  if(g_slist_find(qsApp->sources, trace->it->source1) &&
      trace->yID == trace->it->source1->id)
    // The y source has not been destroyed yet so we dereference
    // its' adjusters.
    _qsAdjusterList_remove(&win->adjusters, &trace->it->source1->adjusters);


  /* This trace->win = NULL acts like a qsTrace_destroy()
   * re-entrance prevention flag in qsSource_removeTraceDraw() */
  trace->win = NULL;

  // Destroys all the adjuster(s) that we added in qsTrace_create()
  _qsAdjuster_destroy(trace->adjusterGroup);

  while(trace->drawSources)
    /* qsSource_removeTraceDraw() removes trace from source
     * and source from trace.  This source is the one that
     * causes the trace to be drawn after the source reads data.
     * We have a list of draw sources so there could be more than
     * one. It's common to have just one draw source. */
    qsSource_removeTraceDraw(
        (struct QsSource *) trace->drawSources->data, trace);

  qsIterator2_destroy(trace->it);

#ifdef QS_DEBUG
  memset(trace, 0, sizeof(*trace));
#endif

  g_free(trace);

  /* Redisplay the windows adjusters widget after we removed
   * adjusters above. */
  _qsAdjusterList_display((struct QsAdjusterList *) win);
}

