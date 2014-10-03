/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <inttypes.h>
#include <X11/Xlib.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include "debug.h"
#include "assert.h"
#include "timer.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win.h"
#include "win_priv.h"

static
void _setGridX(struct QsWin *win, float gridXSpacing, float gridXOffset)
{
  int n;
  win->gridXSpacing = gridXSpacing;
  n = gridXOffset/gridXSpacing;
  win->gridXStart = gridXOffset - n * gridXSpacing;
  if(win->gridXStart >  gridXSpacing - win->gridLineWidth)
    win->gridXStart = gridXOffset - (n + 1) * gridXSpacing;
}

static
void _setGridY(struct QsWin *win, float gridYSpacing, float gridYOffset)
{
  int n;
  win->gridYSpacing = gridYSpacing;
  n = gridYOffset/gridYSpacing;
  win->gridYStart = gridYOffset - n * gridYSpacing;
  if(win->gridYStart >  gridYSpacing - win->gridLineWidth)
    win->gridYStart = gridYOffset - (n + 1) * gridYSpacing;
}

static
float GridSpacing(float minUnitsPerSpacing, uint8_t *gridLineUnits)
{
  float upow;
  int incMat;

  upow = ((int)(log10f(minUnitsPerSpacing))) - 2;
  upow = powf(10.0F, upow);

  {
    // strip off the round-off error off of upow.
    char s[16];
    //for example: upow=0.2135465 --> 
    //              --> upow=0.2100000
    sprintf(s, "%.2g", upow);
    /* sscanf() knows the format of a float number
     * so we ask it what the float is. */
    sscanf(s, "%f", &upow);
    //printf("s=%s  upow=%.15g\n",s, *upow);
  }

  incMat = (minUnitsPerSpacing/(upow) + .999999999);

  /* Now incMat * upow  are the next two decimal
   * place floating point number greater than or equal
   * to minUnitsPerSpacing. */

  //printf("         minUnitsPerSpacing=%g  ~  %d x %g\n", minUnitsPerSpacing, incMat, upow);

  /* round up to 1, 2, 5, times 10^N */
       if(incMat > 50) { incMat = 100; *gridLineUnits = 1; }
  else if(incMat > 20) { incMat = 50;  *gridLineUnits = 5; }
  else if(incMat > 10) { incMat = 20;  *gridLineUnits = 2; }
  else if(incMat > 5)  { incMat = 10;  *gridLineUnits = 1; }
  else                 { incMat = 5;   *gridLineUnits = 5; }

  //printf("round UP    UnitsPerSpacing=%g  ~  %d x %g\n", minUnitsPerSpacing, incMat, upow);

  return  incMat * upow;
}

void _qsWin_setGridX(struct QsWin *win)
{
  /* Here is where we find a grid X pixel spacing that
   * will put grid lines at reasonable positions. */

  QS_ASSERT(win->gridXMinPixelSpace >= 1.0F);

  _setGridX(win,
        GridSpacing(win->gridXMinPixelSpace / win->xScale, &(win->gridXLineUnits)) * win->xScale,
        win->gridXWinOffset * win->xScale + win->xShift);
}

void _qsWin_setGridY(struct QsWin *win)
{
  /* Here is where we find a grid Y pixel spacing that
   * will put grid lines at reasonable positions. */
  QS_ASSERT(win->gridYMinPixelSpace >= 1.0F);

  _setGridY(win,
        - GridSpacing(-win->gridYMinPixelSpace / win->yScale,
            &(win->gridYLineUnits)) * win->yScale,
        win->gridYWinOffset * win->yScale + win->yShift);
}

/* x and y "normalized range" is [ -1/2, 1/2 )
 * from and including -1/2 to and not including 1/2
 * The point -0.5,-0.5 is at the left, bottom corner pixel. */
void qsWin_setXGridPixelSpace(struct QsWin *win, float minPixelGridSpace)
{
  QS_ASSERT(win);
  QS_ASSERT(minPixelGridSpace > 0);

  win->gridXMinPixelSpace = minPixelGridSpace;
  _qsWin_reconfigure(win);
}

void qsWin_setYGridPixelSpace(struct QsWin *win, float minPixelGridSpace)
{
  QS_ASSERT(win);
  QS_ASSERT(minPixelGridSpace > 0);

  win->gridYMinPixelSpace = minPixelGridSpace;
  _qsWin_reconfigure(win);
}

void qsWin_shiftXGrid(struct QsWin *win, float normalizedOffset)
{
  QS_ASSERT(win);
  QS_ASSERT(normalizedOffset <= 1.0F && normalizedOffset >= -1.0F);

  win->gridXWinOffset = normalizedOffset;
  _qsWin_reconfigure(win);
}

void qsWin_shiftYGrid(struct QsWin *win, float normalizedOffset)
{
  QS_ASSERT(win);
  QS_ASSERT(normalizedOffset <= 1.0F && normalizedOffset >= -1.0F);

  win->gridYWinOffset = normalizedOffset;
  _qsWin_reconfigure(win);
}

void qsWin_setXUnits(struct QsWin *win, const char *unitsLabel)
{
  const char *label;
  if(unitsLabel && unitsLabel[0])
    label = unitsLabel;
  else
    label = "units";

  QS_ASSERT(win);

  if(win->unitsXLabel)
    g_free(win->unitsXLabel);
  win->unitsXLabel = g_strdup(label);
}

void qsWin_setYUnits(struct QsWin *win, const char *unitsLabel)
{
  const char *label;
  if(unitsLabel && unitsLabel[0])
    label = unitsLabel;
  else
    label = "units";

  QS_ASSERT(win);

  if(win->unitsYLabel)
    g_free(win->unitsYLabel);
  win->unitsYLabel = g_strdup(label);
}

