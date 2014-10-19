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
  struct QsWin *win;
  struct QsIterator2 *it;
  int xChannelNum, yChannelNum, xID, yID;
  int id; // trace ID from counting creates in a given QsWin

  /* Scale from input to normalized */
  float xScale, yScale, xShift, yShift;
 
  /* Scale from input to window pixels */
  float xScalePix, yScalePix, xShiftPix, yShiftPix;

  float red, green, blue;
  gboolean lines;
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

