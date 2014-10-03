/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <math.h>
#include <limits.h>
#include <inttypes.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "timer.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win.h"
#include "win_priv.h"
#include "fadeDraw_priv.h"


#if 0
//#ifdef QS_DEBUG

#  define CHECK_LIST(win) _check_list(win)

void _check_list(struct QsWin *win)
{
  QS_ASSERT(win && win->fade && win->fadeSurface);

  QS_VASSERT((win->fadeFront && win->fadeRear) ||
      (!win->fadeFront && !win->fadeRear), "front=%p rear=%p",
      win->fadeFront, win->fadeRear);

  struct QsFadingColor *fc, *last = NULL;
  int count = 0;
  
  for(fc = win->fadeFront; fc; fc = fc->prev)
  {
    if(fc->next)
    {
      QS_VASSERT(fc->next->prev == fc, "element %d from front", count);
      QS_ASSERT(fc->next->t0 <= fc->t0);
    }
    if(fc->prev)
    {
      QS_VASSERT(fc->prev->next == fc, "element %d from front", count);
      QS_ASSERT(fc->prev->t0 >= fc->t0);
    }

    last = fc;

    ++count;
  }

  QS_ASSERT(last == win->fadeRear);
  count = 0;

  for(fc = win->fadeRear; fc; fc = fc->next)
  {
    if(fc->next)
      QS_VASSERT(fc->next->prev == fc, "element %d from rear", count);
    if(fc->prev)
      QS_VASSERT(fc->prev->next == fc, "element %d from rear", count);

    last = fc;

    ++count;
  }

  QS_ASSERT(last == win->fadeFront);

}

#else

#  define CHECK_LIST(win)  /*empty macro*/

#endif

static inline
void removeFC(struct QsWin *win, struct QsFadingColor *fc)
{
  if(fc->next)
    fc->next->prev = fc->prev;
  else if(fc == win->fadeFront)
  {
    win->fadeFront = fc->prev;
    if(fc->prev)
      fc->prev->next = NULL;
  }

  if(fc->prev)
    fc->prev->next = fc->next;
  else if(fc == win->fadeRear)
  {
    win->fadeRear = fc->next;
    if(fc->next)
      fc->next->prev = NULL;
  }

  if(fc == win->fadeLastInsert)
    win->fadeLastInsert = NULL;
}

// x, y must satisfy (x >= 0 && x < w && y >= 0 && y < h)
void _qsWin_drawTracePoint(struct QsWin *win, int x, int y,
    float r, float g, float b, long double t)
{
  QS_ASSERT(win);
  QS_ASSERT(r >= 0.0F && r <= 1.0F);
  QS_ASSERT(g >= 0.0F && g <= 1.0F);
  QS_ASSERT(b >= 0.0F && b <= 1.0F);
  QS_ASSERT(x >= 0.0F && x < win->width && y >= 0 && y < win->height);

  if(!win->fade)
  {
    QS_ASSERT(!win->fadeSurface);
    setTraceColor(win, getXColor(win, r*RMAX, g*GMAX, b*BMAX));
    xDrawPoint(win, x, y);
    return;
  }

  QS_ASSERT(win->fadeSurface);


  float I; // intensity
  long double t0;
  struct QsFadingColor *fc, *f;

  fc = win->fadeSurface + win->width*y + x;
  t0 = t + win->fadeDelay;
#ifdef LINEAR_FADE
  I = _qsWin_fadeIntensity(win->fadePeriod, t0, t);
#else
  I =  _qsWin_fadeIntensity(win->fadeAlpha, t0, t);
#endif

  // If something slowed the running of this program
  // this could happen.  The pixel has faded to zero
  // already.
  if(I < MIN_INTENSITY) return;

  CHECK_LIST(win);
  /* Remove it from the list if first, because the
   * chance of it being in the correct position already
   * is small, and this makes the code simpler. */
  removeFC(win, fc);

  CHECK_LIST(win);


  /* Find where to put it in the list. */

  if(!win->fadeLastInsert)
    win->fadeLastInsert = win->fadeRear;

  if(win->fadeLastInsert)
  {
    QS_ASSERT(win->fadeRear);
    QS_ASSERT(win->fadeFront);

    // look to the rear of the list
    for(f = win->fadeLastInsert; f && t0 >= f->t0; f = f->prev);
    if(!f)
    {
      QS_ASSERT(t0 >= win->fadeRear->t0);
      // make it the rear
      fc->prev = NULL;
      win->fadeRear->prev = fc;
      fc->next = win->fadeRear;
      win->fadeRear = fc;
//printf("-");
    }
    else if(f != win->fadeLastInsert)
    {
      // It iterated back in the list
      // f and f->next are both not NULL
      QS_ASSERT(f->next);
      // make it be between f and f->next
      f->next->prev = fc;
      fc->next = f->next;
      fc->prev = f;
      f->next = fc;
//printf("^");
    }
    else
    {
      // look to the front of the list
      for(; f && t0 < f->t0; f = f->next);
      if(f)
      {
        // t0 >= f->t
        // make it be between f->prev and f
        f->prev->next = fc;
        fc->prev = f->prev;
        fc->next = f;
        f->prev = fc;
//printf("+");
      }
      else // !f
      {
        QS_ASSERT(t0 < win->fadeFront->t0);
        // make it the front
        fc->next = NULL;
        win->fadeFront->next = fc;
        fc->prev = win->fadeFront;
        win->fadeFront = fc;
//printf("|");
      }
    }
  }
  else // this one will be the whole list
  {
    QS_ASSERT(!fc->next && !fc->prev && !win->fadeFront);
    win->fadeRear = win->fadeFront = fc;
  }

  fc->t0 = t0;
  win->fadeLastInsert = fc;

  CHECK_LIST(win);

  /* It's in the list in the correct position */

  fc->r = r * RMAX + 0.5F; // rounding to integer is required
  fc->g = g * GMAX + 0.5F;
  fc->b = b * BMAX + 0.5F;
  
  if(I >= 1.0F)
    setTraceColor(win, getXColor(win, fc->r, fc->g, fc->b));
  else
    setTraceColor(win, fadeColor(win, fc, win->width, x, y, I));

  xDrawPoint(win, x, y);

  // _qsWin_drawPoints(win);// gets called later.
}

// This gets called regularly to fade the beam points.
// Beam lines are just lots of points.
gboolean _qsWin_fadeDraw(struct QsWin *win)
{
  QS_ASSERT(win);

  if(!win->gc || !win->fadeFront)
    // wait until configure event or something
    // is in the list:
    return TRUE;

  QS_ASSERT(win->fade && win->fadeSurface);
  QS_ASSERT((win->fadeFront && win->fadeRear) ||
      (!win->fadeFront && !win->fadeRear));
  QS_ASSERT(!win->fadeFront ||
      (win->fadeFront && !win->fadeFront->next));
  QS_ASSERT(!win->fadeRear ||
      (win->fadeRear && !win->fadeRear->prev));
  QS_ASSERT(win->da);

  int w, h, x, y;

  // We can't stop GTK+ from calling this before a needed
  // window configure (resize) event, so we need this check here.
  w = gtk_widget_get_allocated_width(win->da);
  h = gtk_widget_get_allocated_height(win->da);
  if(w != win->width || h != win->height)
    /* We need to wait for a resize configure.  Yes,
     * this was a fix for a nasty BUG. */
    return TRUE;

  struct QsFadingColor *fc, *FC, *prev;
  long double t;
  float alpha, I; // I = trace point color intensity
#ifdef QS_DEBUG
  long double lastT0 = SMALL_LDBL;
#endif

  win->fadeLastTime = t = qsTimer_get(qsApp->timer);
#ifdef LINEAR_FADE
  alpha = win->fadePeriod;
#else
  alpha = win->fadeAlpha;
#endif
  FC = win->fadeSurface;

  CHECK_LIST(win);


  /* This list contains a list of struct QsFadingColor with increasing
   * t0, largest at fadeRear and smallest at fadeFront. */

  for(fc = win->fadeFront; fc; fc = prev)
  {
    QS_ASSERT(!fc->next);

    prev = fc->prev;

    // At this time, t, the trace intensity at this pixel is:
    I = _qsWin_fadeIntensity(alpha, fc->t0, t);

#ifdef QS_DEBUG
    // t0 is not decreasing
    QS_ASSERT(fc->t0 >= lastT0);
    lastT0 = fc->t0;
#endif

    if(I < MIN_INTENSITY)
    {
      int i;
      // Draw the pixel as the background drawing area color
      // over the now completely faded trace pixel.
      y = (fc - FC)/w;
      x = (fc - FC) - y*w;
      i = w * y + x;
      setTraceColor(win, getXColor(win, win->r[i], win->g[i], win->b[i]));
      xDrawPoint(win, x, y);

      if(win->fadeLastInsert == fc)
        win->fadeLastInsert = NULL;

      // Remove the point from the front of the list.
      if(fc->prev)
      {
        fc->prev->next = NULL;
        fc->prev = NULL;
      }
      QS_ASSERT(!fc->next);
    }
    else
      break;
  }

  if(!fc)
  {
    _qsWin_drawPoints(win);
    win->fadeFront = win->fadeRear = fc;
    // All traces are completely faded away.
    return TRUE;
  }

  if(I < 1.0F)
  {
    win->fadeFront = fc;
    
    // We have I (intensity) already from above
    // for this first fc element
    y = (fc - FC)/w;
    x = (fc - FC) - y*w;
    setTraceColor(win, fadeColor(win, fc, w, x, y, I));
    xDrawPoint(win, x, y);

    for(fc = fc->prev; fc; fc = fc->prev)
    {
      I = _qsWin_fadeIntensity(alpha, fc->t0, t);

      QS_ASSERT(I >= MIN_INTENSITY);

#ifdef QS_DEBUG
      // t0 is not decreasing
      QS_ASSERT(fc->t0 >= lastT0);
      lastT0 = fc->t0;
#endif

      if(I < 1.0F)
      {
        y = (fc - FC)/w;
        x = (fc - FC) - y*w;
        setTraceColor(win, fadeColor(win, fc, w, x, y, I));
        xDrawPoint(win, x, y);
      }
      else
        // If I is 1 or greater than it's drawn already;
        // this is not a redraw.
        break;
    }

  CHECK_LIST(win);

    _qsWin_drawPoints(win);
  }
  else if(win->fadeFront != fc)
  {
    win->fadeFront = fc;
    _qsWin_drawPoints(win);
  }


  // There is no need to redraw unfaded pixels with I >= 1
  // because they are drawn already.

  return TRUE;
}

void _qsWin_traceFadeRedraws(struct QsWin *win)
{
  QS_ASSERT(win && win->fade && win->fadeSurface);
  
  // Draw pixels from the fading color buffer
  int w;
  float alpha;
#ifdef LINEAR_FADE
  alpha = win->fadePeriod;
#else
  alpha = win->fadeAlpha;
#endif
  long double lastTime = win->fadeLastTime;
  w = win->width;
  struct QsFadingColor *fc, *FC;
  FC = win->fadeSurface;
  for(fc = win->fadeFront; fc; fc = fc->prev)
  {
    int x, y;
    float I;
    y = (fc - FC)/w;
    x = (fc - FC) - y*w;
    I = _qsWin_fadeIntensity(alpha, fc->t0, lastTime);
    /* Because we are using fadeLastTime the intensities
     * should not have changed since the last call to 
     * _qsWin_fadeDraw() and so: */
    QS_ASSERT(I >= MIN_INTENSITY);

    /* ??? Given that the fade pixel list is ordered, would it be
     * much faster to split this for loop into two for loops:
     * first one for I < 1 and the second one continuing after
     * for the rest of the list I >= 1, then there would be no
     * if's in the second loop.  This here may be better. */
    if(I >= 1.0F)
      setTraceColor(win, getXColor(win, fc->r, fc->g, fc->b));
    else
      setTraceColor(win, fadeColor(win, fc, w, x, y, I));
    xDrawPoint(win, x, y);
  }
  _qsWin_drawPoints(win);
}

