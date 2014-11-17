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
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "controller_priv.h"
#include "controller.h"

struct QsIdle
{
  // inherits controller
  struct QsController controller;
  guint tag; // a GTK source event id
};

static
void _qsIdle_changedSource(struct QsIdle *in, const GSList *sources)
{
  QS_ASSERT(in);
  
  if(!sources && in->tag)
  {
    g_source_remove(in->tag);
    in->tag = 0;
  }
  else if(sources && !in->tag)
    in->tag = g_idle_add((GSourceFunc) _qsController_run, in);
}

static
void _qsIdle_destroy(struct QsIdle *in)
{
  QS_ASSERT(in);

  if(in->tag)
  {
    g_source_remove(in->tag);
    in->tag = 0;
  }

  // Destroy base class if needed.
  // If we make this function public.
  _qsController_checkBaseDestroy(in);
}

struct QsController *qsIdle_create(void)
{
  struct QsIdle *in;

  in = _qsController_create(
      (void (*)(struct QsController *c, const GSList *sources))
      _qsIdle_changedSource, sizeof(*in));
  _qsController_addSubDestroy(in, _qsIdle_destroy);

  return (struct QsController *) in;
}
