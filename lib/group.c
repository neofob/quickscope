/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "app.h"
#include "trace.h"
#include "trace_priv.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "group.h"
#include "source.h"


void _qsGroup_destroy(struct QsGroup *g)
{
  QS_ASSERT(g);
  QS_ASSERT(g->time);
  QS_ASSERT(!g->sources); // the list should be empty

#ifdef QS_DEBUG
  memset(g->time, 0, sizeof(long double)*g->maxNumFrames);
#endif
  g_free(g->time);
  if(g->sampleRates)
  {
    g_free(g->sampleRates);
    g->sampleRates = NULL;
  }

#ifdef QS_DEBUG
  memset(g, 0, sizeof(*g));
#endif
  g_free(g);
}

struct QsGroup
*_qsGroup_create(struct QsSource *s, int maxNumFrames)
{
  QS_ASSERT(s);
  QS_ASSERT(maxNumFrames >= 1);
  QS_ASSERT(qsApp->op_bufferFactor >= 1.0F);

  struct QsGroup *g;
  g = g_malloc0(sizeof(*g));
  g->master = s;
  g->maxNumFrames = maxNumFrames;

  // bufferLength must be the same or larger than maxNumFrames.
  g->bufferLength = (maxNumFrames + 1.001F) * qsApp->op_bufferFactor + 10;

  g->time = g_malloc0(sizeof(long double)*g->maxNumFrames);

  g->sources = g_slist_prepend(g->sources, s);

  g->type = QS_NONE;

  return g;
}

void
_qsGroup_addSource(struct QsGroup *g, struct QsSource *s)
{
  QS_ASSERT(g);
  QS_ASSERT(g->time);
  QS_ASSERT(!g_slist_find(g->sources, s));
  // newest source is now first in this list.
  g->sources = g_slist_prepend(g->sources, s);
}

void
_qsGroup_removeSource(struct QsGroup *g, struct QsSource *s)
{
  QS_ASSERT(g);
  QS_ASSERT(g->sources);
  QS_ASSERT(g_slist_find(g->sources, s));

  g->sources = g_slist_remove(g->sources, s);
}
