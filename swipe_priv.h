/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
struct QsSwipeColor
{
  /* It's a singly linked list of pixels. */
  struct QsSwipeColor *next;
  /* since the trace can wrap (trace) around the view port any number
   * of times we need to keep a track of what generation the trace is
   * at, so that we can clear traces that wrap many times.  If this counter
   * gets incremented through INT_MIN it will not matter, it just can't
   * go through all ints (2^31) in one trace draw cycle. */
  int pointCount;
};

struct QsSwipe
{
  /* surface: is a pointer to the allocated memory
   * one QsSwipeColor per pixel in the drawing area.
   * start: is the start of the list in this pixel memory. */
  struct QsSwipeColor *surface, *start, *end;
  /* Each time the inputed X (or Y) value decreases (wraps back to zero)
   * we increment pointCount. */
  int pointCount, lastValueAdded,
      lastSwipedCount, lastSwipedValue,
      /* lapCount is the value of win->swipePointCount at the
       * time of the last trace view port crossing */
      lapCount, freezeLapCount;


#ifdef QS_DEBUG
  // Used for testing that x is always increasing or wrapping
  // back through zero.
  int xMid;
#endif
};

// called to construct or resize the viewport
// can be used to set the trace, or trace = NULL to keep the
// current swiping trace
extern
void _qsTrace_reallocSwipe(struct QsTrace *trace);

/* TODO: We could do a swipe Y like a scope turned on its side with the sweep
 * moving up or down (Y direction), instead of left and right (X direction).
 */
extern // destroy swipe, free memory
void _qsTrace_cleanupSwipe(struct QsTrace *trace);


// This is called if parameters change, other then view port
// width and height.  This is called from within
// _qsWin_reallocSwipe.
// This can be used to set the trace, or trace = NULL to keep the
// current swiping trace.
extern
void _qsTrace_initSwipe(struct QsTrace *trace);

/* This is for when a pixel (x,y) is swiped away in one trace
 * and the same pixel (x,y) is drawn by this in a different trace or
 * just draw the top trace, in the traces list, at a given pixel. */
static inline // redraw one pixel if we find it return 0 else return 1
int _qsWin_redrawSwipePoint(struct QsWin *win, struct QsTrace *trace,
    GSList *traces,
    int xyIndex, int x, int y, int pointCount)
{
  if(!win->traces->next)
    return 1; // there is just one trace
  
  if(!traces) traces = win->traces;

  GSList *l;
  struct QsTrace *foundTrace = NULL;

  for(l = traces; l; l = l->next)
  {
    struct QsTrace *t;
    t = l->data;
    QS_ASSERT(t);
    if(t != trace && t->swipe)
    {
      struct QsSwipeColor *sc;
      sc = t->swipe->surface + xyIndex;
      if((sc->next || sc == t->swipe->end) &&
          // difference works even with overflow:
          sc->pointCount - pointCount > 0)
      {
        foundTrace = t;
        pointCount = sc->pointCount;
      }
    }
  }

  if(foundTrace)
  {
    setTraceColor(win, getXColor(win,
          (uint8_t) (foundTrace->red   * RMAX + 0.5F),
          (uint8_t) (foundTrace->green * GMAX + 0.5F),
          (uint8_t) (foundTrace->blue  * BMAX + 0.5F)));
    xDrawPoint(win, x, y);
    // _qsWin_drawPoints(win); will get called later
    return 0;
  }
  
  return 1;
}

// redraw the traces from the swipe pixel buffers.
static inline
void _qsWin_traceSwipeRedraws(struct QsWin *win)
{
  QS_ASSERT(win);
  int w;
  w = win->width;

  GSList *l, *traces;
  traces = win->traces;

  for(l = traces; l; l = l->next)
  {
    struct QsTrace *trace;
    trace = l->data;

    if(trace->swipe)
    {
      struct QsSwipeColor *sc, *SC;
      SC = trace->swipe->surface;
      QS_ASSERT(SC);
      uint8_t r, g, b;
      r = trace->red*RMAX;
      g = trace->green*GMAX;
      b = trace->blue*BMAX;

      // TODO: this could be pretty slow if there
      // are a lot of traces.

      for(sc = trace->swipe->start; sc; sc = sc->next)
      {
        int x, y;
        y = (sc - SC)/w;
        x = (sc - SC) - y*w;
        // Figure out which trace is drawn on top
        if(_qsWin_redrawSwipePoint(win, trace, traces,
              w * y + x, x, y, sc->pointCount))
        {
          setTraceColor(win, getXColor(win, r, g, b));
          xDrawPoint(win, x, y);
        }
      }
    }
  }

  _qsWin_drawPoints(win);
}

// We swipe it from the fade pixel buffer too, if there is a
// fade pixel buffer.  It may be quite stupid to use fade
// and swipe at the same time, but we are not going to make it
// so you can't use both fading beam trace and trace swipe at
// the same time.  There may be a reason to use both.  It sounds
// like fun anyway.

/* Remove a pixel link from the fading pixel buffer (not the swipe pixel
 * buffer) */
static inline
void _qsWin_removeFadePixel(struct QsWin *win, int w, int x, int y)
{
  if(win->fade)
  {
    QS_ASSERT(win && win->fadeSurface);
    struct QsFadingColor *fc;
    fc = win->fadeSurface + w*y + x;

    if(fc == win->fadeLastInsert)
      win->fadeLastInsert = NULL;

    if(fc->prev)
    {
      fc->prev->next = fc->next;
    }
    else if(fc == win->fadeRear)
    {
      win->fadeRear = fc->next;
      if(fc->next)
        fc->next->prev = NULL;
    }

    if(fc->next)
    {
      fc->next->prev = fc->prev;
      fc->next = NULL; // must be NULLed
    }
    else if(fc == win->fadeFront)
    {
      win->fadeFront = fc->prev;
      if(fc->prev)
        fc->prev->next = NULL;
    }
    fc->prev = NULL; // must be NULLed
  }
}

// swipe pixels to and including xEnd
// called before a drawing pixels in the viewport swiping from start
// up to X value xEnd
static inline
void _qsWin_swipeRemove(struct QsWin *win, struct QsTrace *trace,
    struct QsSwipe *swipe, int xEnd)
{
  if(!swipe->start) return;

  // Don't swipe/remove if this xEnd is the same as the last one.
  if(swipe->lastSwipedValue == xEnd &&
    swipe->lastSwipedCount == swipe->pointCount)
    return;

  swipe->lastSwipedValue = xEnd;
  swipe->lastSwipedCount = swipe->pointCount;

  struct QsSwipeColor *sc, *surface, *next;
  int i, w, x, y;
  int lapCount;
  uint8_t *r, *g, *b;

  r = win->r; g = win->g; b = win->b;
  surface = swipe->surface;
  w = win->width;

  sc = swipe->start;
  y = (sc - surface)/w;
  x = (sc - surface) - y*w;
  lapCount = swipe->lapCount;


  if(xEnd >= swipe->lastValueAdded)
  {
    // swipe to X = xEnd in the lap before the current lap
    // by using lapCount - sc->pointCount > 0  works
    // when the numbers go through INT_MAX to INT_MIN.
    while(lapCount - sc->pointCount > 0 && x <= xEnd)
    {
      next = sc->next;

      i = w * y + x;
      _qsWin_removeFadePixel(win, w, x, y);

      if(_qsWin_redrawSwipePoint(win, trace, win->traces,
            w * y + x, x, y, sc->pointCount - 5*(INT_MAX/12)))
      {
        setTraceColor(win, getXColor(win, r[i], g[i], b[i]));
        xDrawPoint(win, x, y);
        // _qsWin_drawPoints(win); will get called later
      }
      sc->next = NULL;

      if(!next)
      {
        // We have swiped the whole list.
        swipe->start = swipe->end = NULL;
        swipe->lastValueAdded = -1;
        return;
      }

      sc = next;
      y = (sc - surface)/w;
      x = (sc - surface) - y*w;
    }

    swipe->start = sc;
    return;
  }

  // swipe to X = xEnd in the (last) trace
  while(x <= xEnd || lapCount - sc->pointCount > 0)
  {
    next = sc->next;

    i = w * y + x;
    _qsWin_removeFadePixel(win, w, x, y);
    if(_qsWin_redrawSwipePoint(win, trace, win->traces,
          w * y + x, x, y, sc->pointCount - 5*(INT_MAX/12)))
    {
      setTraceColor(win, getXColor(win, r[i], g[i], b[i]));
      xDrawPoint(win, x, y);
      // _qsWin_drawPoints(win); will get called later
    }
    sc->next = NULL;

    if(!next)
    {
      // We have swiped the whole list.
      swipe->start = swipe->end = NULL;
      swipe->lastValueAdded = -1;
      return;
    }

    sc = next;
    y = (sc - surface)/w;
    x = (sc - surface) - y*w;
  }

  swipe->start = sc;
}

static inline
void _bumpCounters(struct QsWin *win, struct QsSwipe *swipe,
    struct QsSwipeColor *sc, int x)
{
  swipe->pointCount = ++win->swipePointCount;
  if(sc) sc->pointCount = swipe->pointCount;

  if(x < swipe->lastValueAdded)
    swipe->lapCount = swipe->pointCount;

  swipe->lastValueAdded = x;
}

static inline
void _qsWin_unfreeze(struct QsWin *win)
{
  QS_ASSERT(win);
  GSList *l;
  for(l = win->traces; l; l = l->next)
  {
    struct QsTrace *trace;
    trace = l->data;
    QS_ASSERT(trace);
    if(trace->swipe)
    {
#if 1
      if(trace->swipe->freezeLapCount > 2)
        // We need to swipe the whole thing.
        _qsWin_swipeRemove(win, trace, trace->swipe, win->width - 1);
#endif
      trace->swipe->freezeLapCount = 0;
    }
  }
}

// This keeps counters from becoming invalid, even when the counters
// may have overflowed a few times.  We just need the counters
// to increment a little past the current trace counters if it's
// frozen a long time.  This counting stuff will work correctly
// forever so long as win->swipePointCount does not advance more
// than about half of INT_MAX in a 3 (or so) trace laps.
// How much depends on the number of traces and their point plot rates.
// Calculating difference of counters works when the counters
// overflow, so long as the differences are not greater than
// around INT_MAX/2.
static inline
void _qsWin_swipeAddFrozenPoint(struct QsWin *win,
    struct QsTrace *trace, struct QsSwipe *swipe, int x)
{
  if(swipe->freezeLapCount < 3)
  {
    if(swipe->lastValueAdded < x)
      ++swipe->freezeLapCount;
    _bumpCounters(win, swipe, NULL, x);
  }
}

// x must be such that
// (x >= 0 && x < win->width)
// Called with y out of range to advance counters.
static inline
void _qsWin_swipeAddPoint(struct QsWin *win, struct QsTrace *trace,
    struct QsSwipe *swipe, int x, int y)
{
  QS_ASSERT(win);
  QS_ASSERT(trace && trace->swipe && trace->swipe == swipe);
  QS_ASSERT(swipe && swipe->surface);

  if(y < 0 || y >= win->height)
  {
    _bumpCounters(win, swipe, NULL, x);
    return; // we can't draw it
  }

  struct QsSwipeColor *sc;
  sc = swipe->surface + win->width * y + x;

  if(sc->next || sc == swipe->end)
  {
    // It's already marked.  The trace must
    // be dense in pixels; going up and down
    // in Y at one X value.
    QS_ASSERT(swipe->lastValueAdded == x);
    QS_ASSERT(sc->next || (sc == swipe->end && !sc->next));
    return;
  }
  else // It's not in the list.
  {
    // Now put it at the end of the list
    if(!swipe->start)
    {
      QS_ASSERT(!swipe->end);
      swipe->start = sc;
    }
    else if(swipe->end)
      swipe->end->next = sc;
    swipe->end = sc;
    sc->next = NULL;

    _bumpCounters(win, swipe, sc, x);
  }

  QS_ASSERT(x >= swipe->lastValueAdded || x < swipe->xMid);
}

