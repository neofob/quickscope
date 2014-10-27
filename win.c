/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
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
#include "drawsync.h"
#include "controller.h"


static
void _qsXColor_destroy(struct QsXColor *c)
{
  XFreeColors(c->win->dsp, DefaultColormap(c->win->dsp,
          DefaultScreen(c->win->dsp)), &c->pixel, 1, 0);
  g_free(c);
}

static
void _qsWin_displayParameter(struct QsWidget *w,
    const struct QsAdjuster *adj,
    const char *text, struct QsWin *win)
{
  gtk_label_set_markup(GTK_LABEL(win->statusbar), text);
}

static
void _qsWin_changeFadePeriod(struct QsWin *win)
{
#ifndef LINEAR_FADE
  // fadeAlpha will be less than zero
  win->fadeAlpha = logf((float) MIN_INTENSITY)/win->fadePeriod;
#endif
  win->fadeMaxDrawPeriod = win->fadePeriod/20;
  if(win->fadeMaxDrawPeriod < 1.0F/60.0F)
    win->fadeMaxDrawPeriod = 1.0F/60.0F;
}

static
size_t iconText(char *buf, size_t len, struct QsWin *win)
{
  // TODO: consider saving this and only recomputing this if
  // one or more of the colors changes.
  // color strings
  char bgC[8]; // background color
  char gridC[8];
  char axisC[8];
  colorStr(bgC, win->bgR, win->bgG, win->bgB);
  colorStr(gridC, win->gridR, win->gridG, win->gridB);
  colorStr(axisC, win->axisR, win->axisG, win->axisB);

  return snprintf(buf, len,
      "<span bgcolor=\"%s\" fgcolor=\"%s\">["
      "<span fgcolor=\"%s\">-|-</span>"
      "]</span> ",
      bgC, gridC, axisC);
}

static inline
void addIcon(struct QsAdjuster *adj, struct QsWin *win)
{
  adj->icon =
    (size_t (*)(char *, size_t, void *)) iconText;
  adj->iconData = win;
}

struct QsWin *qsWin_create(void)
{
  struct QsWin *win;

  if(!qsApp) qsApp_init(NULL, NULL);

  QS_ASSERT(qsApp->op_gridXMinPixelSpace > 0);
  QS_ASSERT(qsApp->op_gridYMinPixelSpace > 0);
  QS_ASSERT(qsApp->op_gridLineWidth >= 1);
  QS_ASSERT(qsApp->op_axisLineWidth >= 1);
  QS_ASSERT(qsApp->op_subGridLineWidth >= 1);
  QS_ASSERT(qsApp->op_tickLineWidth >= 1);
  QS_ASSERT(qsApp->op_tickLineLength >= 1);

  // QsWin inherits QsAdjusterList
  win = _qsAdjusterList_create(sizeof(*win));
  _qsAdjusterList_addSubDestroy(win, qsWin_destroy);

  /* We get most of our win options from the App object */

  win->bgR = qsApp->op_bgR * RMAX + 0.5F;
  win->bgG = qsApp->op_bgG * GMAX + 0.5F;
  win->bgB = qsApp->op_bgB * BMAX + 0.5F;

  win->gridR = qsApp->op_gridR * RMAX + 0.5F;
  win->gridG = qsApp->op_gridG * GMAX + 0.5F;
  win->gridB = qsApp->op_gridB * BMAX + 0.5F;

  win->axisR = qsApp->op_axisR * RMAX + 0.5F;
  win->axisG = qsApp->op_axisG * GMAX + 0.5F;
  win->axisB = qsApp->op_axisB * BMAX + 0.5F;

  win->subGridR = qsApp->op_subGridR * RMAX + 0.5F;
  win->subGridG = qsApp->op_subGridG * GMAX + 0.5F;
  win->subGridB = qsApp->op_subGridB * BMAX + 0.5F;

  win->tickR = qsApp->op_tickR * RMAX + 0.5F;
  win->tickG = qsApp->op_tickG * GMAX + 0.5F;
  win->tickB = qsApp->op_tickB * BMAX + 0.5F;

  if(qsApp->op_doubleBuffer)
    // use as a flag for now
    win->pixmap = (intptr_t) 1;

  qsWin_setXUnits(win, NULL);
  qsWin_setYUnits(win, NULL);

  win->grid = qsApp->op_grid;
  win->gridLineWidth = qsApp->op_gridLineWidth;
  win->axis = qsApp->op_axis;
  win->axisLineWidth = qsApp->op_axisLineWidth;
  win->subGrid = qsApp->op_subGrid;
  win->subGridLineWidth = qsApp->op_subGridLineWidth;
  win->ticks = qsApp->op_ticks;
  win->tickLineWidth = qsApp->op_tickLineWidth;
  win->tickLineLength = qsApp->op_tickLineLength;

  win->width = -100;
  win->height = -100;
  win->gridXMinPixelSpace = qsApp->op_gridXMinPixelSpace;
  win->gridXWinOffset = qsApp->op_gridXWinOffset;
  win->gridYMinPixelSpace = qsApp->op_gridYMinPixelSpace;
  win->gridYWinOffset = qsApp->op_gridYWinOffset;

  win->fadePeriod = qsApp->op_fadePeriod;
  win->fadeDelay = qsApp->op_fadeDelay;
  _qsWin_changeFadePeriod(win);
  win->fade = qsApp->op_fade;


  win->hashTable = g_hash_table_new_full(g_int_hash,
        g_int_equal, NULL, (GDestroyNotify) _qsXColor_destroy);

  _qsWin_makeGtkWidgets(win);
  

  win->adjsWidget = _qsWidget_create((struct QsAdjusterList *) win,
      (void (*)(struct QsWidget *, const struct QsAdjuster *,
                const char *, void *)) _qsWin_displayParameter,
      win, 0);


  struct QsAdjuster *adjG;

  adjG = qsAdjusterGroup_start(&win->adjusters, "Window");
  adjG->icon = (size_t (*)(char *, size_t, void *)) iconText;
  adjG->iconData = win;


  qsAdjusterBool_create(&win->adjusters,
      "Fade", &win->fade, 
      (void (*)(void *)) _qsWin_reconfigure,
      win);
  qsAdjusterFloat_create(&win->adjusters,
      "Fade Period", "sec", &win->fadePeriod,
      0.0F, /* min */ 10000.0F, /* max */
      (void (*)(void *)) _qsWin_changeFadePeriod,
      win);
  qsAdjusterFloat_create(&win->adjusters,
      "Fade Delay", "sec", &win->fadeDelay,
      0.0F, /* min */ 10000.0F, /* max */
      NULL, NULL);
  qsAdjusterFloat_create(&win->adjusters,
      "X Grid Shift", "*width", &win->gridXWinOffset,
      -0.6, /* min */ 0.6, /* max */
      (void (*)(void *)) _qsWin_reconfigure, win);
  qsAdjusterFloat_create(&win->adjusters,
      "Y Grid Shift", "*height", &win->gridYWinOffset,
      -0.6, /* min */ 0.6, /* max */
      (void (*)(void *)) _qsWin_reconfigure, win);
  qsAdjusterBool_create(&win->adjusters,
      "Show Grid", &win->grid, 
      (void (*)(void *)) _qsWin_reconfigure,
      win);
  qsAdjusterBool_create(&win->adjusters,
      "Show Subgrid", &win->subGrid, 
      (void (*)(void *)) _qsWin_reconfigure,
      win);
  qsAdjusterBool_create(&win->adjusters,
      "Show Axis", &win->axis, 
      (void (*)(void *)) _qsWin_reconfigure,
      win);
  qsAdjusterBool_create(&win->adjusters,
      "Show Ticks", &win->ticks, 
      (void (*)(void *)) _qsWin_reconfigure,
      win);

  qsAdjusterGroup_end(adjG);
 
  // This is called in qsApp_main()
  //_qsAdjusters_display(&win->adjusters);

  qsApp->wins = g_slist_prepend(qsApp->wins, win); 
  
  return win;
}

void qsWin_destroy(struct QsWin *win)
{
  QS_ASSERT(win);
  QS_ASSERT(win->win);
  QS_ASSERT(win->da);
  QS_ASSERT(qsApp);


  while(win->drawSyncs)
  {
#ifdef QS_DEBUG
    void *ds;
    ds = win->drawSyncs->data;
#endif
    qsController_destroy((struct QsController*) win->drawSyncs->data);
    /* this should have changed the list */
    QS_ASSERT(!win->drawSyncs || win->drawSyncs->data != ds);
  }

  if(win->hashTable)
    g_hash_table_destroy(win->hashTable);

  if(win->gc)
    XFreeGC(gdk_x11_get_default_xdisplay(), win->gc);
  if(win->pixmap && win->pixmap != (intptr_t) 1)
    XFreePixmap(win->dsp, win->pixmap);
  if(win->fadeSurface)
    g_free(win->fadeSurface);
  if(win->points)
    g_free(win->points);
  if(win->unitsXLabel)
    g_free(win->unitsXLabel);
  if(win->unitsYLabel)
    g_free(win->unitsYLabel);


  while(win->traces)
  {
#ifdef QS_DEBUG
    void *trace;
    trace = win->traces->data;
#endif
    qsTrace_destroy((struct QsTrace *) win->traces->data);
    /* this should have changed the list */
    QS_ASSERT(!win->traces || win->traces->data != trace);
  }

  qsApp->wins = g_slist_remove(qsApp->wins, win);
  gtk_widget_destroy(win->win);

  _qsWidget_destroy(win->adjsWidget);

  // Call the base destroy if we are not calling
  // it already.
  _qsAdjusterList_checkBaseDestroy(win);
}

