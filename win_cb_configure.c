/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <inttypes.h>
#include <string.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "debug.h"
#include "assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win.h"
#include "win_priv.h"
#include "trace.h"
#include "trace_priv.h"
#include "swipe_priv.h"


/* For reconfiguring the drawing area when some parameters change
 * but not the width and height of the drawing area, the configure
 * callback below will do that. */ 
void _qsWin_reconfigure(struct QsWin *win)
{

  _qsWin_setGridX(win);
  _qsWin_setGridY(win);

  if(!win->gc) return;

  win->bg = getXColor(win, win->bgR, win->bgG, win->bgB);
  
  int w, h;
  w = win->width;
  h = win->height;

  if(win->pixmap)
  {
    setTraceColor(win, win->bg);
    XFillRectangle(win->dsp, win->pixmap, win->gc, 0, 0, w, h);
    if(_qsWin_isGridStuff(win))
      /* we need to draw the grid and other background things
       * on the pixmap, but if there is no pixmap the grid is
       * drawn in cb_draw() */
      _qsWin_drawBackground(win);
  }

  if(win->fade)
  {
    if(!win->fadeSurface)
      win->fadeSurface = g_malloc0(sizeof(struct QsFadingColor)*w*h);
    else
    {
      _qsWin_traceFadeRedraws(win);
      //memset(win->fadeSurface, 0, sizeof(struct QsFadingColor)*w*h);
      //win->fadeFront = win->fadeRear = win->fadeLastInsert = NULL;
    }
  }
  else if(win->fadeSurface)
  {
    // win->fade == FALSE
#ifdef QS_DEBUG
    memset(win->fadeSurface, 0, sizeof(struct QsFadingColor)*w*h);
#endif
    g_free(win->fadeSurface);
    win->fadeFront = win->fadeRear = win->fadeSurface =
      win->fadeLastInsert = NULL;
  }


  // the drawing area draw callback will fix the rest.
  gtk_widget_queue_draw_area(win->da, 0, 0, w, h);
}

/* This gets called on window resize before drawing. */
gboolean _qsWin_cb_configure(GtkWidget *da, GdkEvent *e,
                        struct QsWin *win)
{
  int w, h;

  w = gtk_widget_get_allocated_width(da);
  h = gtk_widget_get_allocated_height(da);

  if(win->width == w && win->height == h)
    // Why was this called???
    return TRUE;

  win->width = w;
  win->height = h;

  /* x and y range is [ -1/2, 1/2 )
   * from and including -1/2 to and not including 1/2 */

  win->xScale =   w;
  win->yScale = - h;
  win->xShift = + w/2;
  win->yShift = - 1 + h/2;


  if(!win->gc)
  {
    win->dsp = gdk_x11_get_default_xdisplay();
    QS_ASSERT(win->dsp);
    win->xwin = gdk_x11_window_get_xid(
            gtk_widget_get_window(win->da));
    QS_ASSERT(win->xwin);
    win->cmap = DefaultColormap(win->dsp, DefaultScreen(win->dsp));
    QS_ASSERT(win->cmap);
    win->gc = XCreateGC(win->dsp, win->xwin, 0, 0);
    QS_ASSERT(win->gc);
    win->bg = getXColor(win, win->bgR, win->bgG, win->bgB);
  }

  if(win->pixmap)
  {
    if(win->pixmap != (intptr_t) 1)
      XFreePixmap(win->dsp, win->pixmap);

    win->pixmap = XCreatePixmap(win->dsp, win->xwin, w, h,
        DefaultDepth(win->dsp, DefaultScreen(win->dsp)));

    setTraceColor(win, win->bg);
    XFillRectangle(win->dsp, win->pixmap, win->gc, 0, 0, w, h);
  }


  GSList *l;
  for(l = win->traces; l; l = l->next)
  {
    QS_ASSERT(l->data);
    _qsTrace_scale((struct QsTrace *) l->data);
  }


  if(win->r)
  {
    QS_ASSERT(win->g && win->b);
    g_free(win->r);
    g_free(win->g);
    g_free(win->b);
  }

  win->r = g_malloc0(sizeof(uint8_t)*w*h);
  win->g = g_malloc0(sizeof(uint8_t)*w*h);
  win->b = g_malloc0(sizeof(uint8_t)*w*h);


  if(win->fade)
  {
    struct QsFadingColor *fc;
    /* win->fadeSurface is a color surface/pixel array.
     * Like a Cairo surface with RGB, and prev and next pointers
     * so that we can traverse just the subset of it that has
     * been drawn too. */
    if(win->fadeSurface)
      g_free(win->fadeSurface);
    win->fadeSurface = fc = g_malloc0(sizeof(struct QsFadingColor)*w*h);
    win->fadeFront = win->fadeRear = win->fadeLastInsert = NULL; 
  }

  /* setup gridXSpacing, gridXStart, gridYSpacing, gridYStart
   * and gridXLineUnits, gridYLinUnits for drawing ticks.
   * They will be needed if any grid, ticks, or axis are turned
   * on on the fly by the user. */
  _qsWin_setGridX(win);
  _qsWin_setGridY(win);

  if(_qsWin_isGridStuff(win) && win->pixmap)
    /* we need to draw the grid on the pixmap, but if
     * there is no pixmap the grid is drawn in cb_draw() */
    _qsWin_drawBackground(win);


  return TRUE; /* TRUE means the event is handled. */
}

