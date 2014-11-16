/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <stdint.h>
#include <X11/Xlib.h>
#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win.h"
#include "win_priv.h"
#include "trace.h"
#include "trace_priv.h"
#include "swipe_priv.h"


// Just to initialize, without resize and allocating.
void _qsTrace_initSwipe(struct QsTrace *trace)
{
  QS_ASSERT(trace && trace->win);
  QS_ASSERT(trace->swipe && trace->swipe->surface);
  QS_ASSERT((void *) trace->swipe != (void *) 1);

  if(!trace->win->gc)
    // We have not got a GTK+ configure event yet.
    // When we do this will be called again and
    // this will not return here.
    return;

  struct QsSwipe *swipe;
  struct QsWin *win;

  win = trace->win;
  swipe = trace->swipe;
  swipe->start = swipe->end = NULL;
  swipe->lapCount = swipe->pointCount = trace->win->swipePointCount - 1;
  swipe->lastSwipedCount = swipe->pointCount - 10;
  swipe->lastValueAdded = -1;
  swipe->lastSwipedValue = -1;
  swipe->freezeLapCount = 0;

  struct QsSwipeColor *sc;
  int i, n;
  n = win->width * win->height;
  for(sc = swipe->surface, i = 0; i < n; ++i)
  {
    sc->next = NULL;
    ++sc;
  }
}

// This may be called to create or resize the swipe thingy.
// _qsWin_drawGrid() must be called after this.
void _qsTrace_reallocSwipe(struct QsTrace *trace)
{
  QS_ASSERT(trace && trace->win);
  struct QsSwipe *swipe;
  struct QsWin *win;

  win = trace->win;

  if(!trace->swipe)
    trace->swipe = swipe = g_malloc0(sizeof(*swipe));
  else
  {
    swipe = trace->swipe;
    memset(swipe, 0, sizeof(*swipe));
  }

  if(!win->gc)
    // We need the drawing area width and height
    return; // we should have X11 window stuff setup later.
  
  if(swipe->surface)
    g_free(swipe->surface);

#ifdef QS_DEBUG
  // Some value a little less than width
  swipe->xMid = 2*win->width/3;
#endif

  swipe->surface = g_malloc0(sizeof(*swipe->surface) *
      win->width * win->height);

  trace->isSwipe = true;
}

void _qsTrace_cleanupSwipe(struct QsTrace *trace)
{
  QS_ASSERT(trace && trace->win);
  QS_ASSERT(trace->swipe && trace->swipe->surface);

#ifdef QS_DEBUG
  memset(trace->swipe->surface, 0,
      sizeof(*trace->swipe->surface) *
      trace->win->width * trace->win->height);
#endif

  g_free(trace->swipe->surface);

#ifdef QS_DEBUG
  memset(trace->swipe, 0, sizeof(*trace->swipe));
#endif

  g_free(trace->swipe);
  trace->swipe = NULL;
  trace->isSwipe = false;
}

void qsTrace_setSwipeX(struct QsTrace *trace, bool on)
{
  QS_ASSERT(trace && trace->win);

  if((trace->swipe && on) || (!trace->swipe && !on))
    return;

  if(on)
  {
    // Make a swipe thingy
    _qsTrace_reallocSwipe(trace);
    _qsWin_reconfigure(trace->win);
  }
  else
  {
    _qsTrace_cleanupSwipe(trace);
  }
}

