/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "app.h"
#include "timer.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win.h"
#include "win_priv.h"
#include "group.h"
#include "source.h"
#include "controller_priv.h"
#include "drawsync.h"

struct QsDrawSync
{
  // We inherit a controller
  struct QsController controller;
  struct QsWin *win;
  gint callbackID;
};

static gboolean _gtkTickCallback(GtkWidget *widget,
    GdkFrameClock *frame_clock, struct QsController *c)
{
  QS_ASSERT(c);

  // Let the controller call the source reads
  return _qsController_run(c);
}

static
void _qsDrawSync_changedSource(struct QsDrawSync *ds, GSList *sources)
{
  QS_ASSERT(ds);
  QS_ASSERT(ds->win);
  QS_ASSERT(ds->win->da);

  if(!sources && ds->callbackID)
  {
    gtk_widget_remove_tick_callback(ds->win->da, ds->callbackID);
    ds->callbackID = 0;
  }
  else if(sources && !ds->callbackID)
    ds->callbackID = gtk_widget_add_tick_callback(ds->win->da,
        (GtkTickCallback) _gtkTickCallback, ds, NULL);
}

void *qsDrawSync_create(struct QsWin *win)
{
  struct QsDrawSync *ds;
  QS_ASSERT(win);

  ds = (struct QsDrawSync *) qsController_create(sizeof(*ds));
  ds->win = win;
  ds->controller.destroy = (void (*)(void *)) qsDrawSync_destroy;
  ds->controller.changedSource =
    (void (*)(struct QsController *c, GSList *sources))
    _qsDrawSync_changedSource;
  win->drawSyncs = g_slist_prepend(ds->win->drawSyncs, ds);

  return ds;
}

void qsDrawSync_destroy(struct QsDrawSync *ds)
{
  QS_ASSERT(ds);
  QS_ASSERT(ds->win);
  QS_ASSERT(ds->win->da);
  QS_ASSERT(ds->win->drawSyncs);

  ds->controller.changedSource = NULL;

  if(ds->callbackID)
  {
    gtk_widget_remove_tick_callback(ds->win->da, ds->callbackID);
    ds->callbackID = 0;
  }

  ds->win->drawSyncs = g_slist_remove(ds->win->drawSyncs, ds);
  ds->win = NULL;

  /* We are in qsController_destroy()
   * if ds->controller.destroy == NULL */

  if(ds->controller.destroy)
  {
    ds->controller.destroy = NULL;
    qsController_destroy(&ds->controller);
  }
}

