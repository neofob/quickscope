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
#include <math.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "base.h"
#include "app.h"
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

static
void _qsDrawSync_destroy(struct QsDrawSync *ds)
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

  // In case the user can some day calls qsDrawSync_destroy()
  // we destroy the base class if needed.
  _qsController_checkBaseDestroy(ds);
}

void *qsDrawSync_create(struct QsWin *win)
{
  struct QsDrawSync *ds;
  QS_ASSERT(win);

  ds = _qsController_create(
      (void (*)(struct QsController *c, GSList *sources))
      _qsDrawSync_changedSource, sizeof(*ds));
  ds->win = win;
  _qsController_addSubDestroy(ds, _qsDrawSync_destroy);
  win->drawSyncs = g_slist_prepend(ds->win->drawSyncs, ds);

  return ds;
}
