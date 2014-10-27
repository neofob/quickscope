/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <inttypes.h>
#include <X11/Xlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
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
#include "win_fadeDraw_priv.h"


static inline
uint32_t GetARGBColor(float a, float r, float g, float b)
{
  return
      (((uint32_t)(0xFF * a   + 0.5F)) << 24) +
      (((uint32_t)(0xFF * r*a/((float)RMAX) + 0.5F)) << 16) +
      (((uint32_t)(0xFF * g*a/((float)GMAX) + 0.5F)) <<  8) +
       ((uint32_t)(0xFF * b*a/((float)BMAX) + 0.5F));
}

static inline
uint32_t GetRGBColor(float r, float g, float b)
{
  return GetARGBColor(1, r, g, b);
}

static inline
uint32_t GetColor(float a, float r, float g, float b,
    uint32_t bgARGB,  uint32_t bgRGB)
{
  if(GetRGBColor(r, g, b) == bgRGB)
    return bgARGB; // using drawing area background color and alpha
  return GetARGBColor(a, r, g, b);
}

// It assumes all traces in the win have a swipe surface buffer.
// Returns 1 if it does not write a color to ret else
// returns 0 if it does write to ret.
static inline
int _qsWin_getTopSwipeColor(struct QsWin *win, float alpha,
    int xyIndex, uint32_t bgARGB, uint32_t bgRGB,
    uint32_t *ret)
{
  GSList *l;
  int maxPointCount;
  struct QsTrace *topTrace = NULL;
  // Less than 1/2 of INT_MAX from win->swipePointCount
  maxPointCount = win->swipePointCount - 6*(INT_MAX/13);
  for(l = win->traces; l; l = l->next)
  {
    struct QsTrace *trace;
    struct QsSwipeColor *sc;
    trace = l->data;
    QS_ASSERT(trace->swipe);
    QS_ASSERT(trace->swipe->surface);
    sc = trace->swipe->surface + xyIndex;
    if((sc->next || sc == trace->swipe->end) &&
        sc->pointCount - maxPointCount > 0)
    {
      topTrace = trace;
      maxPointCount = sc->pointCount;
    }
  }

  if(topTrace)
  {
    *ret = GetColor(alpha,
        topTrace->red * RMAX, topTrace->green * GMAX, topTrace->blue * BMAX,
        bgARGB, bgRGB);
    return 0;
  }
  return 1;
}

cairo_surface_t *_qsWin_savePNG(struct QsWin *win, char **feedbackStr,
    uint32_t **data)
{
  return _qsWin_savePNGwithAphas(win, qsApp->op_savePNG_bgAlpha,
      qsApp->op_savePNG_alpha, feedbackStr, data);
}

// Saves a PNG file from the fading color surface, trace swipe buffers,
// x11 pixmap, or X11 drawing area window.
// feedbackStr must be freed with g_free()
// returns a pointer to a cairo_surface_t.
cairo_surface_t *_qsWin_savePNGwithAphas(struct QsWin *win,
    float alpha, float bgAlpha, char **feedbackStr, uint32_t **data)
{
  QS_ASSERT(win && win->gc);

  bool allSwipingTraces = false;

  if(feedbackStr)
    *feedbackStr = NULL;

  // We do all the bailing cases first

  if(!win->fade && win->traces)
  {
    GSList *l;
    allSwipingTraces = true;
    for(l = win->traces; l; l = l->next)
      if(!((struct QsTrace *) l->data)->swipe)
      {
        allSwipingTraces = false;
        break;
      }
  }

  if(!win->fade && !allSwipingTraces && win->pixmap)
  {
    if(feedbackStr)
      *feedbackStr = g_strdup("using X11 pixmap");
    return cairo_xlib_surface_create(win->dsp, win->pixmap,
        gdk_x11_visual_get_xvisual(gdk_visual_get_system()),
        win->width, win->height);
  }
  else if(!win->fade && !allSwipingTraces)
  {
    if(feedbackStr)
      *feedbackStr = g_strdup("using drawing area X11 window");
    return cairo_xlib_surface_create(win->dsp, win->xwin,
        gdk_x11_visual_get_xvisual(gdk_visual_get_system()),
        win->width, win->height);
  }

  int i, x, y, w, h;
  uint8_t *wr, *wg, *wb;
  uint32_t *row;
  uint32_t bgRGB, bgARGB;
  cairo_surface_t *surface;
  int stride;

  wr = win->r; wg = win->g; wb = win->b;
  w = win->width;
  h = win->height;

  stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
  *data = g_malloc(stride * h);
  surface = cairo_image_surface_create_for_data((unsigned char*) (*data),
      CAIRO_FORMAT_ARGB32, w, h, stride);
  stride /= 4; /* change stride from number of bytes to number of uint32_t */
  row = *data;

  bgARGB = GetARGBColor(bgAlpha, win->bgR, win->bgG, win->bgB);
  bgRGB = GetRGBColor(win->bgR, win->bgG, win->bgB);

  if(win->fade)
  {
    struct QsFadingColor *fc;
    float fadeAlpha;
    long double fadeLastTime;
#ifdef LINEAR_FADE
    fadeAlpha = win->fadePeriod;
#else
    fadeAlpha = win->fadeAlpha;
#endif
    fadeLastTime = win->fadeLastTime;

    fc = win->fadeSurface;

    for(y=0;y<h;++y)
    {
      for(x=0;x<w;++x)
      {
        i = w * y + x;
        
        if(!fc->next && fc != win->fadeFront)
        {
          // It's a pixel that is not in the fade linked list
          // of trace pixels in the fade surface buffer.
          /* The color is from the fixed background: wr[i], wg[i], wb[i] */
          row[x] = GetColor(alpha, wr[i], wg[i], wb[i], bgARGB, bgRGB);
        }
        else
        {
          float I; // trace point intensity
          I = _qsWin_fadeIntensity(fadeAlpha, fc->t0, fadeLastTime);

          if(I >= 1.0F)
          {
            /* color is fc->r, fc->g, fc->b */
            row[x] = GetColor(alpha, fc->r, fc->g, fc->b, bgARGB, bgRGB);
          }
          else if(I > 0.0F)
          {
            /* color is computed as a function of intensity and two colors */
            float fade;
            fade = 1.0F - I;

            /* If there is an intensity than this is not the background color
             * but we will fade to the background if it is fading to it. */

            if(GetRGBColor(wr[i], wg[i], wb[i]) == bgRGB)
              row[x] = GetARGBColor(
                    (alpha * I + bgAlpha * fade),
                    (fc->r * I + wr[i] * fade),
                    (fc->g * I + wg[i] * fade),
                    (fc->b * I + wb[i] * fade));
            else
              row[x] = GetARGBColor(
                    alpha,
                    (fc->r * I + wr[i] * fade),
                    (fc->g * I + wg[i] * fade),
                    (fc->b * I + wb[i] * fade));
          }
        }

        ++fc; // go to next fade buffer pixel grid, not list
      }

      row += stride;
    }
    if(feedbackStr)
      *feedbackStr = g_strdup("using fade buffer");
  }
  //else
  if(allSwipingTraces)
  {
    for(y=0;y<h;++y)
    {
      for(x=0;x<w;++x)
      {
        i = w * y + x;
        if(_qsWin_getTopSwipeColor(win, alpha, i, bgARGB, bgRGB, &row[x]))
          /* color is wr[i], wg[i], wb[i] */
          row[x] = GetColor(alpha, wr[i], wg[i], wb[i], bgARGB, bgRGB);
      }
      row += stride;
    }
    if(feedbackStr)
      *feedbackStr = g_strdup("using swipe buffers");
  }
#if 0
  else // use X11 pixmap and/or X11 window of drawing area
  {

    // TODO BUG: If any of the X11 window of the drawing area is covered
    // by another X11 window this will get an image of what
    // is currently shown to the user with the covering window
    // pieces.  If there is no extra image buffer like win->pixmap
    // there is no easy fix.

    GdkPixbuf *pixbuf;
    gint rowstride, x, y, n_channels;
    guchar *p, *pixels;
    if(win->pixmap)
      // Fixes bug where window on top is added into image.
      XCopyArea(win->dsp, win->pixmap, win->xwin, win->gc,
        0, 0, win->width, win->height, 0, 0);
    pixbuf = gdk_pixbuf_get_from_window(
        gtk_widget_get_window(win->da), 0, 0, w, h);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    p = pixels = gdk_pixbuf_get_pixels(pixbuf);
    n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    QS_ASSERT(n_channels >= 3);

    for(y=0; y<h; ++y)
    {
      for(x=0; x<w; ++x)
      {
        row[x] = ((uint32_t)(0xFF) << 24) | // alpha
                 ((uint32_t)(p[0]) << 16) | // red
                 ((uint32_t)(p[1]) << 8 ) | // green
                 ((uint32_t)(p[2])      );  // blue
        p += n_channels;
      }
      p = pixels + y * rowstride;
      row += stride;
    }
    g_object_unref(G_OBJECT(pixbuf));
    if(feedbackStr && win->pixmap)
      *feedbackStr = g_strdup("using X11 pixmap");
    else if(feedbackStr)
      *feedbackStr = g_strdup("using drawing area X11 window");
  }
#endif

  return surface;
}

