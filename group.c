/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <string.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "app.h"
#include "trace.h"
#include "trace_priv.h"
#include "controller.h"
#include "controller_priv.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "group.h"
#include "source.h"


static void
_qsGroup_destroy(struct QsGroup *g)
{
  QS_ASSERT(g);
  QS_ASSERT(g->time);
  QS_ASSERT(!g->sources); // the list should be empty

#ifdef QS_DEBUG
  memset(g->time, 0, sizeof(long double)*g->maxNumFrames);
#endif
  g_free(g->time);

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
  g->bufferLength = (maxNumFrames + 1.001F) * qsApp->op_bufferFactor;

  g->time = g_malloc(sizeof(long double)*g->maxNumFrames);
  int i;
  for(i=0; i<maxNumFrames; ++i)
    // The quickscope valid time stamp starts at zero.
    // We want this time to be less than that.
    g->time[i] = -1;

  g->sources = g_slist_append(g->sources, s);

  return g;
}

void
_qsGroup_addSource(struct QsGroup *g, struct QsSource *s)
{
  QS_ASSERT(g);
  QS_ASSERT(g->time);
  QS_ASSERT(!g_slist_find(g->sources, s));
  g->sources = g_slist_append(g->sources, s);
}

void
_qsGroup_removeSource(struct QsGroup *g, struct QsSource *s)
{
  QS_ASSERT(g);
  QS_ASSERT(g->sources);
  QS_ASSERT(g_slist_find(g->sources, s));

  g->sources = g_slist_remove(g->sources, s);

  if(!g->sources)
  {
    // signal that there are no more
    // source group.
    s->group = NULL;
    _qsGroup_destroy(g);
  }
}
