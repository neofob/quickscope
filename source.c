/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <inttypes.h>
#include <math.h>
#include <X11/Xlib.h>
#include <string.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "app.h"
#include "app_priv.h"
#include "timer_priv.h"
#include "trace.h"
#include "trace_priv.h"
#include "controller.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "group.h"
#include "source.h"
#include "source_priv.h"
#include "iterator.h"
#include "win.h"
#include "win_priv.h"


// These QsGroup functions are only called from this file.
extern struct QsGroup
*_qsGroup_create(struct QsSource *s, int maxNumFrames);
extern void
_qsGroup_destroy(struct QsGroup *g);
extern void
_qsGroup_addSource(struct QsGroup *g, struct QsSource *s);
extern void
_qsGroup_removeSource(struct QsGroup *g, struct QsSource *s);


struct ChangeCallback
{
  bool (*callback)(struct QsSource *, void *);
  void *data;
};


QS_BASE_DEFINE(qsSource, struct QsSource)


void *qsSource_addChangeCallback(struct QsSource *s,
    bool (*callback)(struct QsSource *, void *), void *data)
{
  QS_ASSERT(s);
  QS_ASSERT(callback);

  struct ChangeCallback *cb;
  cb = g_malloc0(sizeof(*cb));
  cb->callback = callback;
  cb->data = data;
  s->changeCallbacks = g_slist_prepend(s->changeCallbacks, cb);
  return (void *) cb;
}

void qsSource_removeChangeCallback(struct QsSource *s, void *ref)
{
  QS_ASSERT(s);
  QS_ASSERT(ref);
  QS_ASSERT(g_slist_find(s->changeCallbacks, ref));

  s->changeCallbacks = g_slist_remove(s->changeCallbacks, ref);
  g_free(ref);
}

static inline
/* This just destroys the QsSource and not the inheriting object */
void _qsSource_internalDestroy(struct QsSource *s, struct QsGroup *g)
{
  QS_ASSERT(s);
  QS_ASSERT(s->read);
  QS_ASSERT(s->numChannels >= 1);

  // Make it so that there is no minimum sample rate requirement
  // for this now dead source.
  qsSource_setMinSampleRate(s, 0);

  if(s->controller)
    // This will not destroy the source.
    qsController_removeSource(s->controller, s);

  // Destroy traces that use this source for X or Y points
  // and not for source trace draw callback yet.
  // The user should know that destroying a source
  // will destroy its' traces.
  GSList *l;
  for(l = qsApp->wins; l; l = l->next)
  {
    GSList *t;
    struct QsWin *win;
    win = l->data;
    QS_ASSERT(win);
    for(t = win->traces; t; t = t->next)
    {
      struct QsTrace *trace;
      trace = t->data;
      if((trace->it->source0 == s || trace->it->source1 == s) &&
          !g_slist_find(s->traces, trace))
        // We destroy traces in the draw callback list later.
        qsTrace_destroy(trace);
    }
  }

  while(s->changeCallbacks)
    qsSource_removeChangeCallback(s, s->changeCallbacks->data);

  // Now destroy traces that use this source for a trace draw callback
  while(s->traces)
    qsSource_removeTraceDraw(s, (struct QsTrace *) s->traces->data);

  // Destroy all iterators that read this source
  while(s->iterators)
  {
    QS_ASSERT(s == ((struct QsIterator *) s->iterators->data)->source);
    qsIterator_destroy((struct QsIterator *) s->iterators->data);
  }

  // Destroy all iterator2s that read this source
  while(s->iterator2s)
  {
    QS_ASSERT(s == ((struct QsIterator2 *) s->iterator2s->data)->source0 ||
        s == ((struct QsIterator2 *) s->iterator2s->data)->source1);
    qsIterator2_destroy((struct QsIterator2 *) s->iterator2s->data);
  }
  

#ifdef QS_DEBUG
  if(s->isMaster)
  {
    QS_ASSERT(s == g->master);
    memset(s->framePtr, 0, sizeof(float)*s->numChannels*g->maxNumFrames);
    memset(s->timeIndex, 0, sizeof(int)*g->maxNumFrames);
  }
  else
  {
    QS_ASSERT(s != g->master);
    memset(s->framePtr, 0, sizeof(float)*s->numChannels*g->bufferLength);
    memset(s->timeIndex, 0, sizeof(int)*g->bufferLength);
  }
#endif

  g_free(s->framePtr);
  g_free(s->timeIndex);

  _qsGroup_removeSource(g, s);

  if(s->isMaster)
   // This is the last source in the group
    _qsGroup_destroy(g);

  qsApp->sources = g_slist_remove(qsApp->sources, s);

  // Call the base destroy if we are not calling
  // it already.
  _qsAdjusterList_checkBaseDestroy(s);
}

void qsSource_destroy(struct QsSource *s)
{
  QS_ASSERT(s);
  struct QsGroup *g;
  g = s->group;
  QS_ASSERT(g);
  QS_ASSERT(g->master);
  QS_SPEW("source->id=%d\n", s->id);

  if(s->isMaster)
  {
    QS_ASSERT(g->master == s);
    // The master source is the first source created and
    // all the other sources in the group depend on it.
    // The g->sources list starts at the newest source
    // so source destruction is in reverse order of
    // creation.
    GSList *l, *next;
    for(l = g->sources; l; l=next)
    {
      // We are editing the list as we iterate through
      // so we save next.
      next = l->next;
      if(!((struct QsSource *) l->data)->isMaster)
        qsSource_destroy(((struct QsSource *) l->data));
#ifdef DEBUG
      else
        QS_ASSERT(((struct QsSource *) l->data) == s);
#endif
    }
  }

  QS_BASE_CHECKSUBDESTROY(s);

  /* now destroy this and the base object
   * if it's not being destroyed now. */
  _qsSource_internalDestroy(s, g);

  _qsApp_checkDestroy();
}

void qsSource_setReadFunc(struct QsSource *s, QsSource_ReadFunc_t read)
{
  QS_ASSERT(s);
  QS_ASSERT(read);
  s->read = read;
}

void *qsSource_create(QsSource_ReadFunc_t read,
    int numChannels, int maxNumFrames /* max num frames buffered */,
    const struct QsSource *groupSource /* groupSource=NULL to make a new group */,
    size_t objectSize)
{
  struct QsSource *s;
  QS_ASSERT(read);
  QS_ASSERT(numChannels > 0);

  if(!qsApp) qsApp_init(NULL, NULL);

  if(objectSize < sizeof(*s))
    objectSize = sizeof(*s);

  s = _qsAdjusterList_create(objectSize);
  _qsAdjusterList_addSubDestroy(s, qsSource_destroy);
  s->id = qsApp->sourceCreateCount++;
  s->read = read;
  s->numChannels = numChannels;

  struct QsGroup *group;
  int *tI;

  if(groupSource)
  {
    QS_ASSERT(groupSource->group);
    group = groupSource->group;
    QS_ASSERT(group->master);
    QS_ASSERT(!maxNumFrames || group->maxNumFrames == maxNumFrames);
    maxNumFrames = group->maxNumFrames;
    s->group = group;
    struct QsSource *master;
    master = group->master;
    // Initialize the write index in a valid point in the ring buffer.
    s->i = master->i;
    s->wrapCount = master->wrapCount;
    // The frame array is larger than the master case.
    s->framePtr = g_malloc(sizeof(float)*numChannels*group->bufferLength);
    tI = s->timeIndex = g_malloc(sizeof(int)*group->bufferLength);
    _qsGroup_addSource(group, s);
  }
  else
  {
    QS_ASSERT(maxNumFrames > 0);
    // This is the master source
    s->isMaster = true;
    group = s->group = _qsGroup_create(s, maxNumFrames);
    s->iMax = s->i = maxNumFrames - 1;
    s->wrapCount = -1;
    // The frame array is smaller in this master case.
    s->framePtr = g_malloc(sizeof(float)*numChannels*group->maxNumFrames);
    tI = s->timeIndex = g_malloc(sizeof(int)*group->maxNumFrames);
  }
  
  int i;
  for(i=0; i<maxNumFrames; ++i)
    tI[i] = i;

  if(!groupSource)
    // This is the master source
    // We set a time for the last frame which
    // has no valid data, but has a valid time now
    // so that qsSource_lastMasterTime() works now.
    group->time[s->i] = _qsTimer_get(qsApp->timer);

  // prepend!  It's very important that it's not append,
  // so that we auto destroy in reverse order of creation.
  qsApp->sources = g_slist_prepend(qsApp->sources, s);

  return s;
}

long double qsSource_lastTime(struct QsSource *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->group);
  _qsSource_checkWithMaster(s, s->group->master);
  return s->group->time[s->i];
}

void qsSource_emptyIterators(struct QsSource *s)
{
  GSList *l;
  for(l=s->iterators; l; l=l->next)
  {
    struct QsIterator *ir;
    ir = l->data;
    QS_ASSERT(ir->source == s);
    qsIterator_reInit(ir);
  }

  for(l=s->iterator2s; l; l=l->next)
  {
    struct QsIterator2 *ir;
    ir = l->data;
    QS_ASSERT(ir->source0 == s || ir->source1 == s);
    qsIterator2_reInit(ir);
  }
}

// Make source (s) be ready to write to the position
// of the last read in the iterator (it).
void qsSource_sync(struct QsSource *s, struct QsIterator *it)
{
  QS_ASSERT(s);
  QS_ASSERT(it);
  QS_ASSERT(it->source);
  QS_ASSERT(s->group);
  QS_ASSERT(s->group == it->source->group);
  struct QsSource *master;
  master = s->group->master;

  _qsSource_checkWithMaster(it->source, master);
  _qsIterator_checkWithMaster(it, master);

  s->i = it->i;
  s->wrapCount = it->wrapCount;

  --s->i;
  if(s->i < 0)
  {
    s->i = s->group->maxNumFrames - 1;
    --s->wrapCount;
    // Do we need to set s->timeIndex[s->i]?
    // So long as we empty all source (s) iterators
    // the timeIndex should not have to be valid,
    // except when there are appended frames.
    // Less than it->source->timeIndex[it->i] should
    // keep iterators from reading it at an appended
    // frame.
    s->timeIndex[s->i] = it->source->timeIndex[it->i] - 1;
    if(s->timeIndex[s->i] < 0)
      // case when all data in it->source is in
      // 1 frame (time index or time stamp)
      // Not likely.  And would brake other things.
      // case when source buffers are too small.
      s->timeIndex[s->i] = 0;
  }

  // Make sure we did not go back too far in the
  // frame buffer.
  _qsSource_checkWithMaster(s, master);

  qsSource_emptyIterators(s);
}

// Make it so we may read the iterator and than write the
// source at the same frame (time stamp) that was read
// by the iterator.
void qsSource_initIterator(struct QsSource *s, struct QsIterator *it)
{
  QS_ASSERT(s);
  QS_ASSERT(!s->isMaster);
  QS_ASSERT(it);
  QS_ASSERT(s->group);
  struct QsSource *master, *is;
  master = s->group->master;
  is = it->source;

  _qsSource_checkWithMaster(is, master);

  // All in sync and iterator (it) with no data to read.
  s->i = it->i = is->i;
  s->wrapCount = it->wrapCount = is->wrapCount;
  s->timeIndex[s->i] = is->timeIndex[is->i];

  // The source s buffer has no valid data in it, so
  // we empty all the iterators that use this source s.
  qsSource_emptyIterators(s);
}

void qsSource_addTraceDraw(struct QsSource *s,  struct QsTrace *trace)
{
  QS_ASSERT(s);
  QS_ASSERT(trace);
  // Don't add a trace draw callback twice between trace and
  // a particular source.
  if(g_slist_find(s->traces, trace) == NULL)
  {
    s->traces = g_slist_prepend(s->traces, trace);
    trace->drawSources = g_slist_prepend(trace->drawSources, s);
  }
}

void qsSource_removeTraceDraw(struct QsSource *s, struct QsTrace *trace)
{
  QS_ASSERT(s);
  QS_ASSERT(trace);
  QS_ASSERT(g_slist_find(s->traces, trace));
  QS_ASSERT(g_slist_find(trace->drawSources, s));

  s->traces = g_slist_remove(s->traces, trace);
  trace->drawSources = g_slist_remove(trace->drawSources, s);

  /* If this trace still has a window then qsTrace_destroy()
   * is not in the current function call stack, so we can
   * call qsTrace_destroy(trace). */
  if(trace->win)
    /* This trace has no reason to live. */
    qsTrace_destroy(trace);
}

int _qsSource_read(struct QsSource *s, long double time, void *data)
{
  int ret;
  QS_ASSERT(s);
  QS_ASSERT(s->group);
  QS_ASSERT(s->read);
  QS_ASSERT(time > s->prevT);

  s->t = time;

  ret = s->read(s, time, s->prevT, data);

  GSList *l;

  if(ret == 1)
  {
    GSList *next;

    for(l=s->changeCallbacks;l;l=next)
    {
      struct ChangeCallback *cb;
      // We get next now so we can get to it
      // if we remove the current l.
      next = l->next;
      cb = l->data;
      QS_ASSERT(cb);
      if(!cb->callback(s, cb->data))
        qsSource_removeChangeCallback(s, cb);
    }
    for(l = s->traces;l;l=l->next)
    {
      QS_ASSERT(l->data);
      _qsTrace_draw((struct QsTrace *)l->data, time);
    }

    // The prevT is the previous time that we got data,
    // so that source read callbacks can know when the
    // last time we got data was.
    s->prevT = time;
  }

  if(ret != -1)
  {
    // We may need to draw even if we got no new data
    // because there may be fading (fade drawing) to do.
    for(l = s->traces;l;l=l->next)
    {
      // We queue the drawing areas after we finish having all the traces
      // add their pixel points.
      QS_ASSERT(l->data);
      // This is an inline function that will handle being called
      // more than once, and when it's not really needed.
      _qsWin_postTraceDraw(((struct QsTrace *) l->data)->win, time);
    }
  }
  else // if(ret == -1)
  {
    // Mark this source for destruction
    s->controller = NULL;
    return 1;
  }

  return 0;
}
