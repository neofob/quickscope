/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

// TODO: remove unneeded includes
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "app.h"
#include "base.h"
#include "trace.h"
#include "trace_priv.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "group.h"
#include "source.h"
#include "source_priv.h"
#include "iterator.h"


struct QsIterator
*qsIterator_create(struct QsSource *s, int channel)
{
  QS_ASSERT(s);
  QS_ASSERT(channel < s->numChannels);
  QS_ASSERT(channel >= 0);
  QS_ASSERT(s->group);
  QS_ASSERT(s->group->master);

  struct QsIterator *it;

  it = g_malloc0(sizeof(*it));
  it->source = s;
  it->channel = channel;
  
  // initialize to having no data to read and
  // will read the next point written.
  qsIterator_reInit(it);

  s->iterators = g_slist_prepend(s->iterators, it);

  return it;
}

void qsIterator_destroy(struct QsIterator *it)
{
  QS_ASSERT(it);
  QS_ASSERT(it->source);
  QS_ASSERT(g_slist_find(it->source->iterators, it));
  it->source->iterators = g_slist_remove(it->source->iterators, it);
#ifdef QS_DEBUG
  memset(it, 0, sizeof(*it));
#endif
  g_free(it);
}

struct QsIterator2
*qsIterator2_create(struct QsSource *s0, struct QsSource *s1,
    int channel0, int channel1)
{
  QS_ASSERT(s0);
  QS_ASSERT(s0->group);
  QS_ASSERT(channel0 < s0->numChannels);
  QS_ASSERT(channel0 >= 0);
  QS_ASSERT(s1);
  QS_ASSERT(s1->group);
  QS_ASSERT(channel1 < s1->numChannels);
  QS_ASSERT(channel1 >= 0);
  QS_ASSERT(s0->group == s1->group);

  struct QsIterator2 *it;

  it = g_malloc0(sizeof(*it));
  it->source0 = s0;
  it->source1 = s1;
  it->channel0 = channel0;
  it->channel1 = channel1;
 
  // initialize to having no data to read
  qsIterator2_reInit(it);
 
  s0->iterator2s = g_slist_prepend(s0->iterator2s, it);
  if(s0 != s1)
    s1->iterator2s = g_slist_prepend(s1->iterator2s, it);

  return it;
}

void qsIterator2_destroy(struct QsIterator2 *it)
{
  QS_ASSERT(it);
  struct QsSource *s0, *s1;
  s0 = it->source0;
  s1 = it->source1;
  QS_ASSERT(s0);
  QS_ASSERT(s1);
  
  QS_ASSERT(g_slist_find(s0->iterator2s, it));
  s0->iterator2s = g_slist_remove(s0->iterator2s, it);
  if(s0 != s1)
  {
    QS_ASSERT(g_slist_find(s1->iterator2s, it));
    s1->iterator2s = g_slist_remove(s1->iterator2s, it);
  }

#ifdef QS_DEBUG
  memset(it, 0, sizeof(*it));
#endif
  g_free(it);
}
