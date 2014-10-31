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
#include "assert.h"
#include "base.h"
#include "controller_priv.h"
#include "controller.h"
#include "interval.h"

struct QsInterval
{
  // inherits controller
  struct QsController controller;
  float period;
  int timeoutTag;
};

static
void _qsInterval_changedSource(struct QsInterval *in, const GSList *sources)
{
  QS_ASSERT(in);
  
  if(!sources && in->timeoutTag)
  {
    g_source_remove(in->timeoutTag);
    in->timeoutTag = 0;
  }
  else if(sources && !in->timeoutTag)
    in->timeoutTag = g_timeout_add_full(
        /* This priority much be lower than the priority
         * of a configure/resize event */
        G_PRIORITY_HIGH_IDLE - 40,
        (guint)(in->period*1000+0.5) /* 1/1000ths of a second */,
        (GSourceFunc) _qsController_run, in, NULL);
}

static
void _qsInterval_destroy(struct QsInterval *in)
{
  QS_ASSERT(in);

  if(in->timeoutTag)
  {
    g_source_remove(in->timeoutTag);
    in->timeoutTag = 0;
  }

  // Destroy base class if needed.
  // If we make this function public.
  _qsController_checkBaseDestroy(in);
}

void *qsInterval_create(float period)
{
  struct QsInterval *in;

  if(period > 24*3600)
    period = 24*3600;
  else if(period < 0.001)
    period = 0.001;

  in = _qsController_create(
      (void (*)(struct QsController *c, const GSList *sources))
      _qsInterval_changedSource, sizeof(*in));
  in->period = period;
  _qsController_addSubDestroy(in, _qsInterval_destroy);

  return in;
}
