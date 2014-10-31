/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include "debug.h"
#include "assert.h"
#include "base.h"
#include "win.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win_priv.h"


// Speed is not too much of an issue in the drawing in this file.
// This drawing is only done when there is an event that causes
// a redraw, like a window uncovering, or window resize.
// We expect that this code draws a lot faster than Cairo.
// Speed may become an issue if we use Cairo to draw in here.


static inline
void DrawPoint(struct QsWin *win,
    float r, float g, float b, // rbg color
    float frac, // fraction of the rgb color. One for all.
    int w, // viewport width
    int x, int y)
{
  QS_ASSERT(frac >= 0);
  QS_ASSERT(frac <= 1);
  
  // not thread safe.
  static float lastR = -100, lastG = -100, lastB = -100;
  int i;
  i = w*y + x;


  if(frac == 1)
  {
    win->r[i] = r;
    win->g[i] = g;
    win->b[i] = b;
  }
  else
  {
    // We have a fraction of the color on top of
    // what is there now.
    r = win->r[i] += (r - win->r[i]) * frac + 0.5F;
    g = win->g[i] += (g - win->g[i]) * frac + 0.5F;
    b = win->b[i] += (b - win->b[i]) * frac + 0.5F;
  }

  if(r != lastR || g != lastG || b != lastB)
  {
    // Don't change the color unless we must.
    // With all this line drawing the color
    // will not change all the time.
    lastR = r; lastG = g; lastB = b;
    setTraceColor(win, getXColor(win, r, g, b));
  }

  xDrawPoint(win, x, y);
}

/*
 * Our grid drawing anti-aliasing looks something like this:
 

      ^
      |
      |  color intensity (relative to bg color)
      |
      |
      |
      |      |<------ line width ---->|
      |      |                        |
   1 --      |  ********************  |
      |      | *                    * |
      |      |*                      *|
      |      *                        *
      |     *                          *
      |    *                            *
   0 --  @     @     @     @     @     @     @     @     @

                                 pixel position --->

      
      This just needs 1-D anti-aliasing because we
      are drawing horizontal and vertical lines.
      The piece-wise linear function above seems
      to do well enough.

      The program `xmag' is my friend.

      Why?  X11 drawing does not do anti-aliasing and
            Cario is slow, and adds a lot of stuff.

      TODO: Since this grid and background stuff is only
            drawn at the start of a window configure event
            we could use Cario to draw it but we still
            need to query all the pixels to fill in the
            fade-to surface background colors, so it would
            be more code and not less, and I'm pretty
            sure it would be a little slower.  Text drawing
            with Cario may be nicer.

 */
static inline
void DrawHLine(struct QsWin *win,
    float R, float G, float B,
    float lineWidth, int w, int h,
    float y)
{
  float ymin, ymax;
  int iy, iymax;

  QS_ASSERT(lineWidth >= 1);

  ymin = y - (lineWidth + 1.0F)/2.0;
  ymax = y + (lineWidth + 1.0F)/2.0;

  if(ymax <= 0 || ymin >= h-1)
    // culled
    return;

  iy = ymin;
  if(iy <= ymin)
    ++iy;
  /* now iy is the next integer value after ymin */
  if(iy < 0) // culling
    iy = 0;

  iymax = ymax;
  if(iymax >= h) // culling
    iymax = h - 1;

  for(;iy <= iymax; ++iy)
  {
    int ix;
    float frac;
    if(iy < ymin + 1)
      frac = iy - ymin;
    else if(iy > ymax - 1)
      frac = ymax - iy;
    else
      frac = 1;


    for(ix=0; ix < w; ++ix)
      DrawPoint(win, R, G, B, frac, w, ix, iy);
  }
}

static inline
void DrawVLine(struct QsWin *win,
    float R, float G, float B,
    float lineWidth, int w, int h,
    float x)
{
  float xmin, xmax;
  int ix, ixmax;

  QS_ASSERT(lineWidth >= 1);

  xmin = x - (lineWidth + 1.0F)/2.0;
  xmax = x + (lineWidth + 1.0F)/2.0;


  if(xmax <= 0 || xmin >= w-1)
    // culled
    return;

  ix = xmin;
  if(ix <= xmin)
    ++ix;
  /* now ix is the next integer value after xmin */
  if(ix < 0) // culling
    ix = 0;

  ixmax = xmax;
  if(ixmax >= w) // culling
    ixmax = w - 1;
  
  for(;ix <= ixmax; ++ix)
  {
    int iy;
    float frac;
    if(ix < xmin + 1)
      frac = ix - xmin;
    else if(ix > xmax - 1)
      frac = xmax - ix;
    else
      frac = 1;

    for(iy=0; iy < h; ++iy)
      DrawPoint(win, R, G, B, frac, w, ix, iy);
  }
}

/*

  We draw axis ticks as a rectangle with anti-aliased edges.
  They'd look like shit without anti-aliasing.  We wish to
  be able to draw them at any floating point position in the
  discrete pixel space.


           |<---------- width ---------->|
  |        |                             |
  -
  |     -------------------------------------
  |     |     linear graded mixed color     |
  |     |                                   |  -------
  -     |     -------------------------     |       ^
  |     |     |                       |     |       |
  |     |     |                       |     |       |
  |     |     |                       |     |
  -     |     |    rectangle color    |     |   height
  |     |     |                       |     |
  |     |     |                       |     |       |
  |     |     |                       |     |       |
  -     |     -------------------------     |       v
  |     |                                   |  ------
  |     |                                   |
  |     -------------------------------------
  -
  |                background color
  |
  |
  ------|-----|-----|-----|-----|-----|-----|->  X  [pixels]
  |
  |
  |
  -        Y axis is down (not that is matters)
  |
  |
  V

 Y [pixels]

  
 

     The program 'xmag' is our best friend.  

 */

static
void DrawTickRec(struct QsWin *win,
    float tickR, float tickG, float tickB, // rbg color
    int w, int h, // viewport width and height
    float wD2, float hD2, // width and height divided by 2
    float x, float y /* center of rectangle */)
{
  int ixmin, iymin, ixmax, iymax;
  float xmin, xmax, ymin, ymax;

  xmin = x - wD2 - 0.5;
  xmax = x + wD2 + 0.5;
  ymin = y - hD2 - 0.5;
  ymax = y + hD2 + 0.5;

  QS_ASSERT(xmax - xmin >= 2);
  QS_ASSERT(ymax - ymin >= 2);
  if(xmax < 0) return;
  if(ymax < 0) return;

  /* We restrict the width and height of the rectangle
   * to be at least 1 pixel, and we pad a 1/2 to either
   * side making it at least 2 pixels in width and height.
   * We linearly mix the one pixel color on all sides
   * forming a parameter of color smug. */

  ixmin = xmin;
  if(ixmin <= xmin)
    ++ixmin; // bounded by xmin
  /* ix is the next integer value after xmin */
  if(ixmin >= w) return;

  iymin = ymin;
  if(iymin <= ymin)
    ++iymin; // bounded by ymin
  /* iy is the next integer value after ymin */
  if(iymin >= h) return;

  ixmax = xmax;
  iymax = ymax;

  QS_ASSERT(ixmax - ixmin >= 1);
  QS_ASSERT(iymax - iymin >= 1);

  int ix, iy;
  float frac, fracX, fracY;
  const float SMALL = 0.001F;

  /* top left corner ixmin, iymin */
  fracX = ixmin - xmin;
  fracY = iymin - ymin;
  if(fracX > SMALL && fracY > SMALL && ixmin >= 0 && iymin >= 0)
  {
    // This is how much tick color we will have.
    frac = 2 * fracX * fracY / (fracX + fracY);
    DrawPoint(win, tickR, tickG, tickB, frac, w, ixmin, iymin);
  }

  /* top right corner ixmax, iymin */
  fracX = xmax - ixmax;
  if(fracX > SMALL && fracY > SMALL && ixmax < w && iymin >= 0)
  {
    // This is how much tick color we will have.
    frac = 2 * fracX * fracY / (fracX + fracY);
    DrawPoint(win, tickR, tickG, tickB, frac, w, ixmax, iymin);
  }

  /* bottom right corner ixmax, iymax */
  fracY = ymax - iymax;
  if(fracX > SMALL && fracY > SMALL && ixmax < w && iymax < h)
  {
    // This is how much tick color we will have.
    frac = 2 * fracX * fracY / (fracX + fracY);
    DrawPoint(win, tickR, tickG, tickB, frac, w, ixmax, iymax);
  }

  /* bottom left corner ixmin, iymax */
  fracX = ixmin - xmin;
  if(fracX > SMALL && fracY > SMALL && ixmin >= 0 && iymax < h)
  {
    // This is how much tick color we will have.
    frac = 2 * fracX * fracY / (fracX + fracY);
    DrawPoint(win, tickR, tickG, tickB, frac, w, ixmin, iymax);
  }

  /* down the left side without the top and bottom corners */
  ix = ixmin;
  if(ix >= 0) // cull check
  {
    frac = ix - xmin;
    for(iy = iymin + 1; iy < iymax; ++iy)
      if(iy >= 0 && iy < h) // cull check
        DrawPoint(win, tickR, tickG, tickB, frac, w, ix, iy);
  }

  /* down the right side without the top and bottom corners */
  ix = ixmax;
  if(ix < w) // cull check
  {
    frac = xmax - ix;
    for(iy = iymin + 1; iy < iymax; ++iy)
      if(iy >= 0 && iy < h) // cull check
        DrawPoint(win, tickR, tickG, tickB, frac, w, ix, iy);
  }

  /* across the top without the start and end corner pixels */
  iy = iymin;
  if(iy >= 0) // cull check
  {
    frac = iy - ymin;
    for(ix = ixmin + 1; ix < ixmax; ++ix)
      if(ix >=0 && ix < w) // cull check
        DrawPoint(win, tickR, tickG, tickB, frac, w, ix, iy);
  }

  /* across the bottom without the start and end corner pixels */
  iy = iymax;
  if(iy < h) // cull check
  {
    frac = ymax - iy;
    for(ix = ixmin + 1; ix < ixmax; ++ix)
      if(ix >=0 && ix < w) // cull check
        DrawPoint(win, tickR, tickG, tickB, frac, w, ix, iy);
  }

  /* Fill in the inner rectangle with tick color */
  setTraceColor(win, getXColor(win, tickR, tickG, tickB));
  for(ix = ixmin + 1; ix < ixmax; ++ix)
  {
    for(iy = iymin + 1; iy < iymax; ++iy)
    {
      if(ix >= 0 && iy >= 0 && ix < w && iy < h) // cull check
      {
        DrawPoint(win, tickR, tickG, tickB, 1, w, ix, iy);
      }
    }
  }
}

/* The grid is assumed to be drawn just after the background is
 * set to the background color.  If this is not the case this
 * grid drawing code will need to be rewritten.  The grid
 * drawing does some crude anti-aliasing so that the grid lines
 * can be put at any floating point pixels positions with any
 * floating point pixel width greater than or equal to one. */
void _qsWin_drawBackground(struct QsWin *win)
{
  int w, h;
  
  QS_ASSERT(win);

  w = win->width;
  h = win->height;


  { // We must (re)initialize the drawing area background pixels.
    int i, n;
    n = w*h;
    for(i=0; i<n; ++i)
    {
      // blank background
      win->r[i] =  win->bgR;
      win->g[i] =  win->bgG;
      win->b[i] =  win->bgB;
    }
  }

  if(!_qsWin_isGridStuff(win))
    // Nothing but the blank background
    return;


  bool noGrid;
  noGrid = (win->grid)?false:true;
  
  /* THE ORDER OF THINGS DRAW HERE:
   *
   * We draw the
   *    1) sub-Grid (or not),
   *    2) the axis ticks (or not), and then
   *    3) the grid lines (or not).
   *    4) axises
   *
   * We found this order to be the prettiest.
   * TODO: Changing this order is not an option yet.
   */


  if(win->subGrid)
  {
    // We draw the vertical sub grid lines first
    float i, delta, max, lW, tLW;
    float R, G, B;

    R = win->subGridR;
    G = win->subGridG;
    B = win->subGridB;


    lW = win->subGridLineWidth;
    max = w + lW;

    tLW = lW * 0.33;
    if(tLW < 1)
      tLW = 1;

    switch(win->gridXLineUnits)
    {
      case 1:
        {
          int skip = 1;
          delta = win->gridXSpacing/5;
          i = win->gridXStart - win->gridXSpacing + delta;
          for(; i < max; i += delta, ++skip)
          {
            if(skip % 5) // skip every 5th if grid lines
              // thin
              DrawVLine(win, R, G, B, tLW, w, h, i);
            else if(noGrid)
              // regular
              DrawVLine(win, R, G, B, lW, w, h, i);
          }
        }
        break;
      case 5:
        {
          delta = win->gridXSpacing/5;
          i = win->gridXStart - win->gridXSpacing + delta;

          if(noGrid)
          {
            int lineNum;
            float org;

            /* We will be putting regular thickness lines at every
             * 10th line, because the grid is not present at every
             * 5th line.  It just makes better sense since in this
             * sub grid thing, with the grid missing.  If this is
             * not what was wanted then the grid should have been
             * drawn. */

            org =  win->gridXWinOffset * win->xScale + win->xShift;

            if(org > i)
            {
              lineNum = 0.5 + (org - i)/delta;
              lineNum = 10 - (lineNum % 10);
              // lineNum is now the number of lines
              // past an even number of 10 lines back
              // from the origin, bla bla, bla ...
            }
            else
              // lineNum is now the number of sub grid lines
              // from the origin where we start drawing
              lineNum = 0.5 + (i - org)/delta;
        
            for(; i < max; i += delta, ++lineNum)
            {
              if(lineNum % 10) // because we have no grid we uses 10's
                // thin
                DrawVLine(win, R, G, B, tLW, w, h, i);
              else
                // regular
                DrawVLine(win, R, G, B, lW, w, h, i);
            }
          }
          else
          {
            int skip = 1;

            for(; i < max; i += delta, ++skip)
              if(skip % 5) // skip every 5th if grid lines
                // thin
                DrawVLine(win, R, G, B, tLW, w, h, i);
          }
        }
        break;
      case 2:
        {
          int skip = 1;
          delta = win->gridXSpacing/4;
          i = win->gridXStart - win->gridXSpacing + delta;
          for(; i < max; i += delta, ++skip)
            if(skip % 4 || noGrid) // skip every 4th if grid lines
            {
              if(skip % 2) // every other line is fat
                // thin
                DrawVLine(win, R, G, B, tLW, w, h, i);
              else
                // regular
                DrawVLine(win, R, G, B, lW, w, h, i);
            }
        }
        break;
#ifdef QS_DEBUG
      default:
        QS_VASSERT(0, "gridXLineUnits is not 1,2, or 5");
        break;
#endif    
        
     }

    // Now we draw the horizontal sub grid lines
    max = h + lW;
    
    switch(win->gridYLineUnits)
    {
      case 1:
        {
          int skip = 1;
          delta = win->gridYSpacing/5;
          i = win->gridYStart - win->gridYSpacing + delta;
          for(; i < max; i += delta, ++skip)
          {
            if(skip % 5) // skip every 5th if grid lines
              // thin
              DrawHLine(win, R, G, B, tLW, w, h, i);
            else if(noGrid)
              // regular
              DrawHLine(win, R, G, B, lW, w, h, i);
          }
        }
        break;
      case 5:
        {
          delta = win->gridYSpacing/5;
          i = win->gridYStart - win->gridYSpacing + delta;

          if(noGrid)
          {
            int lineNum;
            float org;

            /* We will be putting regular thickness lines at every
             * 10th line, because the grid is not present at every
             * 5th line.  It just makes better sense in this
             * sub grid thing with the grid missing.  If this is
             * not what was wanted then the grid should have been
             * drawn. */

            org =  win->gridYWinOffset * win->yScale + win->yShift;

            if(org > i)
            {
              lineNum = 0.5 + (org - i)/delta;
              lineNum = 10 - (lineNum % 10);
              // lineNum is now the number of lines
              // past an even number of 10 lines back
              // from the origin, bla bla, bla ...
            }
            else
              // lineNum is now the number of sub grid lines
              // from the origin where we start drawing
              lineNum = 0.5 + (i - org)/delta;
        
            for(; i < max; i += delta, ++lineNum)
            {
              if(lineNum % 10) // because we have no grid we uses 10's
                // thin
                DrawHLine(win, R, G, B, tLW, w, h, i);
              else
                // regular
                DrawHLine(win, R, G, B, lW, w, h, i);
            }
          }
          else
          {
            int skip = 1;

            for(; i < max; i += delta, ++skip)
              if(skip % 5) // skip every 5th if grid lines
                // thin
                DrawHLine(win, R, G, B, tLW, w, h, i);
          }
        }
        break;
      case 2:
        {
          int skip = 1;
          delta = win->gridYSpacing/4;
          i = win->gridYStart - win->gridYSpacing + delta;
          for(; i < max; i += delta, ++skip)
            if(skip % 4 || noGrid) // skip every 4th if grid lines
            {
              if(skip % 2) // every other line is fat
                // thin
                DrawHLine(win, R, G, B, tLW, w, h, i);
              else
                // regular
                DrawHLine(win, R, G, B, lW, w, h, i);
            }
        }
        break;
#ifdef QS_DEBUG
      default:
        QS_VASSERT(0, "gridYLineUnits is not 1,2, or 5");
        break;
#endif
    }
  }


  if(win->ticks)
  {
    float x, y, tWD2, tLD2, tTLD2, tTWD2, skipOrigin;
    float tickR, tickG, tickB;

    tickR = win->tickR;
    tickG = win->tickG;
    tickB = win->tickB;
    // regular tick width divided by 2
    tWD2 = win->tickLineWidth/2.0;
    // regular tick length divided by 2
    tLD2 = win->tickLineLength/2.0;

    // thin tick width divided by 2
    tTWD2 = tWD2 * 0.6;
    if(tTWD2 < 0.5001)
      tTWD2 = 0.5001;
    // thin tick length divided by 2
    tTLD2 = tLD2 * 0.63;
    if(tTLD2 < 0.5001)
      tTLD2 = 0.5001;


    y = win->gridYWinOffset * win->yScale + win->yShift; // y = 0
    
    if(y >= - tLD2 - 0.5 && y < h + tLD2 + 0.5)
    {
      // Drawing ticks along the X axis
      ///////////////////////////////////////////////////////////
      float delta, max;
      max = w + tWD2 + 0.5;

      switch(win->gridXLineUnits)
      {
        case 1:
          {
            int fat = 0; // every 5th tick is fat and longer
            delta = win->gridXSpacing/5;

            skipOrigin = win->gridXWinOffset * win->xScale + win->xShift - delta/2.0;

            for(x = win->gridXStart - win->gridXSpacing + delta; x < max; x += delta)
            {
              ++fat;
              if(x > skipOrigin)
              {
                skipOrigin = INFINITY;
                continue;
              }

              if(fat % 5)
                // thin
                DrawTickRec(win, tickR, tickG, tickB, w, h,
                  tTWD2, tTLD2, x, y /* x,y center of line */);
              else if(noGrid)
                // regular
                DrawTickRec(win, tickR, tickG, tickB, w, h,
                  tWD2, tLD2, x, y /* x,y center of line */);
              }
          }
          break;
        case 2:
          {
            int fat = 0; // every other tick is fat and longer
            delta = win->gridXSpacing/4;

            skipOrigin = win->gridXWinOffset * win->xScale + win->xShift - delta/2.0;

            for(x = win->gridXStart - win->gridXSpacing + delta; x < max; x += delta)
            {
              ++fat;
              if(x > skipOrigin)
              {
                skipOrigin = INFINITY;
                continue;
              }
              if(fat % 2)
                // thin
                DrawTickRec(win, tickR, tickG, tickB, w, h,
                  tTWD2, tTLD2, x, y /* x,y center of line */);
              else if(fat % 4 || noGrid)
                // regular
                DrawTickRec(win, tickR, tickG, tickB, w, h,
                  tWD2, tLD2, x, y /* x,y center of line */);
              }
          }
          break;
        case 5:
          {
            delta = win->gridXSpacing/5;
            x = win->gridXStart - win->gridXSpacing + delta;
            skipOrigin = win->gridXWinOffset * win->xScale + win->xShift - delta/2.0;

            if(noGrid)
            {
              int lineNum;
              float org;

            /* We will be putting regular thickness ticks at every
             * 10th line, because the grid is not present at every
             * 5th line.  It just makes better sense since in this
             * sub grid thing, with the grid missing.  If this is
             * not what was wanted then the grid should have been
             * drawn. */

              org =  win->gridXWinOffset * win->xScale + win->xShift;

              if(org > x)
              {
                lineNum = 0.5 + (org - x)/delta;
                lineNum = 10 - (lineNum % 10);
                // lineNum is now the number of lines
                // past an even number of 10 lines back
                // from the origin.
              }
              else
                // lineNum is now the number of sub grid lines
                // from the origin where we start drawing
                lineNum = 0.5 + (x - org)/delta;

              for(; x < max; x += delta, ++lineNum)
              {
                if(x > skipOrigin)
                {
                  skipOrigin = INFINITY;
                  continue;
                }

                if(lineNum % 10) // because we have no grid we uses 10's
                  // thin
                  DrawTickRec(win, tickR, tickG, tickB, w, h,
                    tTWD2, tTLD2, x, y /* x,y center of line */);
                else
                  // regular
                  DrawTickRec(win, tickR, tickG, tickB, w, h,
                    tWD2, tLD2, x, y /* x,y center of line */);
              }
            }
            else
            {
              int skip = 1;

              for(; x < max; x += delta, ++skip)
              {
                if(x > skipOrigin)
                {
                  skipOrigin = INFINITY;
                  continue;
                }

                if(skip % 5) // skip every 5th if grid lines
                  // thin
                  DrawTickRec(win, tickR, tickG, tickB, w, h,
                    tTWD2, tTLD2, x, y /* x,y center of line */);
              }
            }
          }
          break;

#ifdef QS_DEBUG
        default:
          QS_VASSERT(0, "gridXLineUnits is not 1,2, or 5");
          break;
#endif
      }
      /////////////////////////////////////////////////////////
    }

    x = win->gridXWinOffset * win->xScale + win->xShift; // x = 0

    if(x >= - tLD2 - 0.5 && x < w + tLD2 + 0.5)
    {
      // Drawing ticks along the Y axis
      /////////////////////////////////////////////////////////
      float delta, max;

      max = h + tWD2 + 0.5;

      switch(win->gridYLineUnits)
      {
        case 1:
          {
            int fat = 0; // every 5th tick is fat and longer
            delta = win->gridYSpacing/5;

            skipOrigin = win->gridYWinOffset * win->yScale + win->yShift - delta/2.0;

            for(y = win->gridYStart - win->gridYSpacing + delta; y < max; y += delta)
            {
              ++fat;
              if(y > skipOrigin)
              {
                skipOrigin = INFINITY;
                continue;
              }
              if(fat % 5)
                // thin
                DrawTickRec(win, tickR, tickG, tickB, w, h,
                  tTLD2, tTWD2, x, y /* x,y center of line */);
              else if(noGrid)
                // regular
                DrawTickRec(win, tickR, tickG, tickB, w, h,
                  tLD2, tWD2, x, y /* x,y center of line */);
              }
          }
          break;
        case 2:
          {
            int fat = 0; // every other tick is fat and longer
            delta = win->gridYSpacing/4;

            skipOrigin = win->gridYWinOffset * win->yScale + win->yShift - delta/2.0;

            for(y = win->gridYStart - win->gridYSpacing + delta; y < max; y += delta)
            {
              ++fat;
              if(y > skipOrigin)
              {
                skipOrigin = INFINITY;
                continue;
              }
              if(fat % 2)
                // thin
                DrawTickRec(win, tickR, tickG, tickB, w, h,
                  tTLD2, tTWD2, x, y /* x,y center of line */);
              else if(fat % 4 || noGrid)
                // regular
                DrawTickRec(win, tickR, tickG, tickB, w, h,
                  tLD2, tWD2, x, y /* x,y center of line */);
              }
          }
          break;
        case 5:
          {
            delta = win->gridYSpacing/5;
            y = win->gridYStart - win->gridYSpacing + delta;
            skipOrigin = win->gridYWinOffset * win->yScale + win->yShift - delta/2.0;
            
            if(noGrid)
            {
              int lineNum;
              float org;

            /* We will be putting regular thickness ticks at every
             * 10th line, because the grid is not present at every
             * 5th line.  It just makes better sense since in this
             * sub grid thing, with the grid missing.  If this is
             * not what was wanted then the grid should have been
             * drawn. */

              org =  win->gridYWinOffset * win->yScale + win->yShift;

              if(org > y)
              {
                lineNum = 0.5 + (org - y)/delta;
                lineNum = 10 - (lineNum % 10);
                // lineNum is now the number of lines
                // past an even number of 10 lines back
                // from the origin.
              }
              else
                // lineNum is now the number of sub grid lines
                // from the origin where we start drawing
                lineNum = 0.5 + (y - org)/delta;
        
              for(; y < max; y += delta, ++lineNum)
              {
                if(y > skipOrigin)
                {
                  skipOrigin = INFINITY;
                  continue;
                }

                if(lineNum % 10) // because we have no grid we uses 10's
                  // thin
                  DrawTickRec(win, tickR, tickG, tickB, w, h,
                    tTLD2, tTWD2, x, y /* x,y center of line */);
                else
                  // regular
                  DrawTickRec(win, tickR, tickG, tickB, w, h,
                    tLD2, tWD2, x, y /* x,y center of line */);
              }
            }
            else
            {
              int skip = 1;

              for(; y < max; y += delta, ++skip)
              {
                if(y > skipOrigin)
                {
                  skipOrigin = INFINITY;
                  continue;
                }

                if(skip % 5) // skip every 5th if grid lines
                  // regular
                  DrawTickRec(win, tickR, tickG, tickB, w, h,
                    tTLD2, tTWD2, x, y /* x,y center of line */);
              }
            }
          }
          break;

#ifdef QS_DEBUG
        default:
          QS_VASSERT(0, "gridYLineUnits is not 1,2, or 5");
          break;
#endif
      }
      /////////////////////////////////////////////////////////
    }

  }

  if(win->grid)
  {
    // We draw the vertical lines first so they have
    // no horizonal lines to cross.
    float i, delta, max, lW;
    float gridR, gridG, gridB;
    float axisPos;

    gridR = win->gridR;
    gridG = win->gridG;
    gridB = win->gridB;
    
    lW = win->gridLineWidth;


    delta = win->gridXSpacing;
    axisPos =  win->gridXWinOffset * win->xScale + win->xShift - delta/2.0;

    for(max = w + lW, i = win->gridXStart; i < axisPos; i += delta)
      DrawVLine(win, gridR, gridG, gridB, lW, w, h, i);

    if(!win->axis)
      DrawVLine(win, gridR, gridG, gridB, lW, w, h, i);

    for(i += delta; i < max; i += delta)
      DrawVLine(win, gridR, gridG, gridB, lW, w, h, i);


    delta = win->gridYSpacing;
    axisPos =  win->gridYWinOffset * win->yScale + win->yShift - delta/2.0;

    for(max = h + lW, i = win->gridYStart; i < axisPos; i += delta)
      DrawHLine(win, gridR, gridG, gridB, lW, w, h, i);

    if(!win->axis)
      DrawHLine(win, gridR, gridG, gridB, lW, w, h, i);

    for(i += delta; i < max; i += delta)
      DrawHLine(win, gridR, gridG, gridB, lW, w, h, i);
  }

  if(win->axis)
  {
    // Draw just the X and Y axis
    DrawVLine(win, win->axisR, win->axisG, win->axisB,
          win->axisLineWidth, w, h,
          win->gridXWinOffset * win->xScale + win->xShift);
    DrawHLine(win, win->axisR, win->axisG, win->axisB,
          win->axisLineWidth, w, h,
          win->gridYWinOffset * win->yScale + win->yShift);
  }

  _qsWin_drawPoints(win);
}

