/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#define _QS_WIN_H_


// comment out for exponential temporal fade
// uncomment for linear temporal fade
#define LINEAR_FADE



#define SMALL_LDBL  (-1.0e-30)

struct QsWin;
struct QsXColor
{
  unsigned long pixel;/* X11 pixel color value */
  struct QsWin *win; /* so we can free it */
};

struct QsWin
{
  /* This must be the first struct.
   * QsWin inherits QsAdjusterList. */
  struct QsAdjusterList adjusters;

  struct QsWidget *adjsWidget;

  // We don't need to have a pointer to all GTK+ widgets,
  // just some.
  GtkWidget 
      *win,
        *menubar,
            *viewMenubar, *viewStatusbar,
            *viewWindowBorder, *viewFullscreen,
        *da,
        *statusbar;


  GSList *traces;
  GSList *drawSyncs;

  int traceCount;

  /* X11 stuff */
  Display *dsp;
  Window xwin;
  Colormap cmap;
  GC gc; /* X11 Graphics Context */

  /* normalized values are bounded
   * -1/2 <= x < 1/2    -1/2 <= y < 1/2
   *
   * pixel draw values are from
   *  0 <= x_da <= width-1
   *  0 <= y_da <= height-1
   * 
   * draw vals       normalized vals x, y
   *   x_da       =  x * xScale + xShift
   *   y_da       =  y * yScale + yShift  */
  float xScale, yScale;
  int xShift, yShift;



  // This fade stuff could be another class-like struct
  // but since there is only one per QsWin we just make
  // it part of QsWin, making it faster by having less
  // pointers to dereference.  Speed is very important
  // in this fade stuff.  We do a lot of iterating of lists.
  // This fade stuff is the biggest performance hit of
  // all the parts of Quickscope.  "fade" is the fade class
  // "namespace" within QsWin.

  struct QsFadingColor *fadeSurface, *fadeFront, *fadeRear, *fadeLastInsert;
  long double fadeLastTime; // last time _qsWin_fadeDraw() was called.
  /* user controllable fade parameters */
  float fadePeriod, fadeDelay, fadeMaxDrawPeriod;
#ifndef LINEAR_FADE
  /* alpha is used to get intensity by: I = exp(-alpha*t) */
  float fadeAlpha;
#endif

  bool fade; /* beam trace lines and points fade in time or not */


  Pixmap pixmap; /* if double buffered */

  uint8_t bgR, bgG, bgB, /* background color */
    gridR, gridG, gridB, /* grid color */
    axisR, axisG, axisB, /* axis color */
    subGridR, subGridG, subGridB, /* grid color */
    tickR, tickG, tickB; /* axis tick color */

  /* color to revert to if the trace pixel is not drawn.
   * Includes grid, axis, ticks and subgrid.
   * Index like r[y * win->width + x] where x and y of
   * X11 pixel coordinates in the drawing area. */
  uint8_t *r, *g, *b;

  uint8_t gridXLineUnits, gridYLineUnits; // This is the spacing unit
  // between grid lines with factors of 10 removed; it's 1, 2, or 5
  // It's needed so that we know how to draw tick marks.

  char *unitsXLabel, *unitsYLabel;

  /* Grid geometry in normalized user units.  Window width and height
   * are X and Y ranges are [-1/2 to 1/2).  These are set by the user. */
  float gridXMinPixelSpace, gridXWinOffset;
  float gridYMinPixelSpace, gridYWinOffset;

  /* parameters used to draw the x/y grid.  They need to be
   * recalculated from *Normal* (above) at window resize.
   * These numbers are all distances in pixels.  They are floats
   * so that we can draw lines at factions of pixels and with line
   * widths that are floating point numbers using anti-aliasing.
   * The grid looks like shit without anti-aliasing. */
  float gridXSpacing, gridXStart, gridYSpacing, gridYStart;

  float gridLineWidth, tickLineWidth,
         tickLineLength, subGridLineWidth,
         axisLineWidth; // all in pixels

  bool grid, ticks, subGrid, axis;

  bool needPostPointDraw; // stupid internal flag

  unsigned long bg; /* X11 pixel color of background color */


  /* To keep from setting the same foreground color again and again */
  unsigned long lastFGColor;
 
  /* We buffer up the pixel point draws */
  XPoint *points;
  size_t pointsLen;
  int npoints;

  /* We store the X colors in a hash table so we do not have
   * to get the colors again and again. */
  GHashTable *hashTable;

  int width, height; /* drawing area window width and height */

  int swipePointCount; /* window global counter used by trace swipe */
};

/* To cut down on the number of colors and the size
 * of the color hash table we use to 6 bits (64 values)
 * for each r,g,b.  That is 2^18 = 262144 colors.
 * Adding an additional green bit may not be a bad idea,
 * but we thought that it looked okay without it.
 * As you may know humans are more sensitive to green
 * then red and blue. */
#define RMAX (0x3F) // first 6 bits
#define GMAX (0x3F) // first 6 bits
#define BMAX (0x3F) // first 6 bits

#define MIN_INTENSITY (1.0F/((float)GMAX))

/* We are not using Cairo to draw because it is
 * much slower than this is. We tried three different
 * ways with Cairo and it's just too slow in all
 * cases that we dreamed up. Cairo sucks at real-time
 * animation.  It's too slow. */

struct QsFadingColor
{
  /* This is a pixel map that adds temporal color fading
   * along a doubly linked list.  The linked list is
   * is added to as trace points are plotted.  We add
   * new bright unfaded points to the rear of the list
   * and fadeRear->next is the next point, and the front
   * point is oldest and most faded, like a line at the
   * DMV (department of motor vehicles). */
  struct QsFadingColor *prev, *next;
  /* Intensity decreases with time whereby making colors
   * fade from r,g,b to win->r, win->g, win->b.  When a
   * colors' intensity is zero it is removed from the
   * list and that color get set to win->r, win->g, win->b.
   * until it is drawn on again.  This intensity is a simple
   * linear function of time. */

  /* t0 is the time that the point was added plus the
   * fade Delay time.  Similar to your customer number
   * when you stand in line at the DMV.  When
   * period + t0 - t is zero the intensity of the
   * corresponding pixel, which t is the current wall clock
   * time from gettimeofday().  t0 is always increasing from
   * front (low) to rear (high). */
  long double t0;
  uint8_t r, g, b;
};

/* Returns pointer to cairo_surface_t on success, and NULL on failure.
 * Use g_free() on errorStr and cairo_surface_destroy() on returned value
 * if non-NULL, and g_free(*data). */
extern
cairo_surface_t *_qsWin_savePNG(struct QsWin *win, char **errorStr,
    uint32_t **data);
extern
cairo_surface_t * _qsWin_savePNGwithAphas(struct QsWin *win,
    float alpha, float bgAlpha, char **errorStr, uint32_t **data);
extern
void _qsWin_setGridX(struct QsWin *win);
extern
void _qsWin_setGridY(struct QsWin *win);
extern
void _qsWin_drawBackground(struct QsWin *win);
extern
void _qsWin_reconfigure(struct QsWin *win);
extern
bool _qsWin_cb_configure(GtkWidget *da, GdkEvent *e, struct QsWin *win);
extern
void _qsWin_drawTracePoint(struct QsWin *win, int x, int y,
    float r, float g, float b, long double t);
extern
bool _qsWin_fadeDraw(struct QsWin *win);
extern
bool ecb_keyPress(GtkWidget *w, GdkEvent *e, struct QsWin *win);
extern
bool cb_savePNG(GtkWidget *w, struct QsWin *win);
extern
bool cb_quit(GtkWidget *w, gpointer data);
extern
bool cb_viewFullscreen(GtkWidget *w, GtkWidget *win);
extern
bool cb_viewMenuItem(GtkWidget *menuItem, GtkWidget *w);
extern
bool cb_viewWindowBorder(GtkWidget *menuItem, GtkWidget *win);
extern
void _qsWin_makeGtkWidgets(struct QsWin *win);
extern
void _qsWin_initFadeCallback(struct QsWin *win);
extern
void _qsWin_traceFadeRedraws(struct QsWin *win);


static inline
void _qsWin_drawPoints(struct QsWin *win)
{
  if(win->npoints)
  {
#if 1 /* removing this code removes X11 drawing:
         Doing so shows this drawing uses less
         resources than the filling of the win->points
         buffer. 

         with this code (within 10%):
                         this program uses 57% cpu
                         and xorg uses %20 cpu
         without this code and all other code on:
                         this program uses 50% cpu
                         and xorg uses %1 cpu

         */

      /* We can hope that libX11 is using an efficient
       * drawing method, like OpenGL or Xrender.  We
       * opted to let the system override slower libX11
       * functions with libXrender --> libGL linker
       * magic.  We don't care how, just so long as it
       * runs fast.  'ldd' output shows libGL.
       * Running with 'gdb' shows that XDrawPoints() calls
       * __memcpy_ssse3_back() and thats it.
       * That was without win->pixmap.  We guess that
       * this is not direct rendering and the memcpy is
       * to the Xserver using shared memory.  Oh well
       * at least it's not talking on a local socket
       * file descriptor to the Xserver.  Anyhow we don't
       * think this is the current bottle-neck.  We
       * think that win.c:_qsWin_fadeDraw() is the current top
       * CPU (for-loop) pig.  Given todays multi-core systems,
       * having the XServer draw may be better than
       * having a single thread (this code) do direct
       * rendering.  CPU usage shows this program process is
       * using similar CPU usage to the Xserver. Using direct
       * rendering may wreck that synergy, putting at least
       * some if not all that CPU usage into this one process.
       * So it looks like we are getting parallelization for free.
       */
    if(win->pixmap)
      XDrawPoints(win->dsp, win->pixmap, win->gc,
        win->points, win->npoints, CoordModeOrigin);
    else
      XDrawPoints(win->dsp, win->xwin, win->gc,
        win->points, win->npoints, CoordModeOrigin);
    //printf("%d\n", win->npoints);

    win->needPostPointDraw = true;
    win->npoints = 0;
#endif
  }
}

extern
bool _qsWin_cbDraw(GtkWidget *da, cairo_t *cr, struct QsWin *win);

static inline
void _qsWin_postTraceDraw(struct QsWin *win, long double t)
{
  if((win->needPostPointDraw ||
    (win->fadeFront && t >= win->fadeLastTime + win->fadeMaxDrawPeriod)
    ) && !qsApp->freezeDisplay)
  {
    if(win->fade)
       _qsWin_fadeDraw(win);

    // gtk_widget_queue_draw_area() does not give good results.
    // It's is not necessarily drawing at a good time.
    // We control all the drawing except when an expose event and
    // stuff like that calls our drawing area draw callback.
    if(win->pixmap)
      XCopyArea(win->dsp, win->pixmap, win->xwin, win->gc,
        0, 0, win->width, win->height, 0, 0);
    win->needPostPointDraw = false;
  }
}

static inline
void setTraceColor(struct QsWin *win, unsigned long foreground)
{
  if(win->lastFGColor == foreground) return;

  /* We flush any points to draw before we
   * change the foreground color. */
  _qsWin_drawPoints(win);

  XSetForeground(win->dsp, win->gc, foreground);
  win->lastFGColor = foreground;
}

/* We buffer the point draws so that we use less
 * system resources. */
static inline
void xDrawPoint(struct QsWin *win, int x, int y)
{
  if(win->pointsLen <= win->npoints)
  {
    win->pointsLen += 256;
    win->points = g_realloc(win->points,
        sizeof(*(win->points))*win->pointsLen);
  }

  win->points[win->npoints].x = x;
  win->points[win->npoints++].y = y;
}

// TODO: maybe this could be made faster???
static inline
unsigned long getXColor(struct QsWin *win, float r, float g, float b)
{
  XColor x;
  int red, green, blue;
  int hashKey;
  struct QsXColor *c;

  // Round
  red   = r + 0.5F;
  green = g + 0.5F;
  blue  = b + 0.5F;

  QS_VASSERT(red >= 0 && red <= RMAX, "red=%d", red);
  QS_VASSERT(green >= 0 && green <= GMAX, "green=%d", green);
  QS_VASSERT(blue >= 0 && blue <= BMAX, "blue=%d", blue);

  hashKey = (red << 16) | (green << 8) | blue;

  /* Before this caching hash table we hammered the X server
   * by allocating the same colors again and again.  Running
   * top showed a X server that used 100% CPU usage as
   * compared to around %1 to 10% after adding this code.
   * Why doesn't libX11 do this for us? */
  if((c = g_hash_table_lookup(win->hashTable, &hashKey)))
    return c->pixel;

  x.pixel = 0;
  x.red   = (0xFFFF * red)/RMAX; /* X11 uses shorts not bytes */
  x.green = (0xFFFF * green)/GMAX;
  x.blue  = (0xFFFF * blue)/BMAX;
  x.flags = 0;

#ifdef QS_DEBUG
  QS_ASSERT(XAllocColor(win->dsp, win->cmap, &x));
#else
  XAllocColor(win->dsp, win->cmap, &x);
#endif

  c = g_malloc(sizeof(*c));
  c->pixel = x.pixel;
  c->win = win;

  g_hash_table_insert(win->hashTable, &hashKey, c);

  return x.pixel;
}

// returns a color like str="#00FF00" for green
// input floats r, g, b, are in the range 0 to RMAX, GMAX, BMAX
// TODO: This does not exactly match the colors that
// X11 allocates because X11 uses 2 bytes for each R G B
// and we are displaying them as one byte per R G B here.
static inline
void colorStr(char str[8], float r, float g, float b)
{
  QS_VASSERT(r >= 0 && r <= RMAX+0.01F, "r=%f", r);
  QS_VASSERT(g >= 0 && g <= GMAX+0.01F, "g=%f", g);
  QS_VASSERT(b >= 0 && b <= BMAX+0.01F, "b=%f", b);

  int R, G, B;

  R = r + 0.5F;
  G = g + 0.5F;
  B = b + 0.5F;

  snprintf(str, 8, "#%6.6X",
    (((R * 0xFF)/RMAX) << 16) +
    (((G * 0xFF)/GMAX) << 8) +   
    (B * 0xFF)/BMAX);
}

static inline
bool _qsWin_isGridStuff(struct QsWin *win)
{
  return (win->grid || win->subGrid ||
      win->ticks || win->axis);
}

static inline
unsigned long fadeColor(struct QsWin *win, struct QsFadingColor *fc,
    int w, int x, int y, float intensity)
{
  float fade;
  int i;
  i = w * y + x;
  QS_ASSERT(intensity >= MIN_INTENSITY && intensity < 1.0F);

  fade = 1.0F - intensity;

  return getXColor(win,
      fc->r * intensity + fade * win->r[i],
      fc->g * intensity + fade * win->g[i],
      fc->b * intensity + fade * win->b[i]);
}

