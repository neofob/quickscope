/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
struct QsSource;
struct QsIterator2;
struct QsAdjuster;
struct QsSwipe;

struct QsTrace
{
  struct QsWin *win; // window to draw into
  struct QsIterator2 *it; // iterator reads X and Y sources
  int xChannelNum, yChannelNum,
      xID, yID; // ID of the sources so we uniquely know them
  int id; // trace ID from counting creates in a given QsWin

  /* scale and shift from source [I]nput to [N]ormalized [-1/2, 1/2)
   * Set in qsTrace_create().  Is fixed for life of trace. */
  // N = I * scale + shift
  float xScale, yScale, xShift, yShift;

  /* scale from source [I]nput to window [P]ixels.
   * Changes with window resize */
  // P = I * scale + shift
  float xScalePix, yScalePix, xShiftPix, yShiftPix;

  float red, green, blue;
  bool lines, isSwipe;
  float prevX, prevY; /* for line and point drawing */
  float prevPrevX, prevPrevY; /* for line drawing */

  /* Sources that cause this trace to draw.  We keep the option to have
   * more than one source trigger the draw.  Who are we to limit that? */
  GSList *drawSources;
  struct QsAdjuster *adjusterGroup;

  /* Each trace may or may not have a swipe */
  struct QsSwipe *swipe;
};

/* call _qsTrace_scale() on create and window resize and
 * window rescale/zoom */
extern
void _qsTrace_scale(struct QsTrace *trace);
extern
void _qsTrace_draw(struct QsTrace *trace, long double t);
extern
size_t _qsTrace_iconText(char *buf, size_t len, struct QsTrace *trace);
