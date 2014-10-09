/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <inttypes.h>
#include <math.h>
#include <X11/Xlib.h>
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
  gboolean (*callback)(struct QsSource *, void *);
  void *data;
};


void *qsSource_addChangeCallback(struct QsSource *s,
    gboolean (*callback)(struct QsSource *, void *), void *data)
{
  QS_ASSERT(s);
  QS_ASSERT(callback);

  struct ChangeCallback *cb;
  cb = g_malloc0(sizeof(*cb));
  cb->callback = callback;
  cb->data = data;
  s->changeCallbacks = g_slist_append(s->changeCallbacks, cb);
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
void _qsSource_internalDestroy(struct QsSource *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->group);
  QS_ASSERT(s->read);
  QS_ASSERT(s->numChannels >= 1);

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

  while(s->changeCallbacks)
    qsSource_removeChangeCallback(s, s->changeCallbacks->data);

#ifdef QS_DEBUG
  if(s->isMaster)
  {
    QS_ASSERT(s == s->group->master);
    memset(s->framePtr, 0, sizeof(float)*s->numChannels*s->group->maxNumFrames);
    memset(s->timeIndex, 0, sizeof(int)*s->group->maxNumFrames);
  }
  else
  {
    QS_ASSERT(s != s->group->master);
    memset(s->framePtr, 0, sizeof(float)*s->numChannels*s->group->bufferLength);
    memset(s->timeIndex, 0, sizeof(int)*s->group->bufferLength);
  }
#endif

  g_free(s->framePtr);
  g_free(s->timeIndex);

  s->framePtr = NULL; // block reentry to this function from qsSource_destroy().

  // This will destroy the source group if it has no more sources in it
  // and sets s->group to NULL.
  _qsGroup_removeSource(s->group, s);

  if(s->isMaster)
  {
    struct QsGroup *g;
    g = s->group;
    while(g->sources)
      qsSource_destroy(((struct QsSource *) g->sources->data));
    // This is the last source in the group
    _qsGroup_destroy(s->group);
  }

  qsApp->sources = g_slist_remove(qsApp->sources, s);

  // Free the memory of this object
  _qsAdjusterList_destroy(&s->adjusters);
}

void qsSource_destroy(struct QsSource *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->group);
  QS_ASSERT(s->group->master);

  if(s->destroy)
  {
    void (*destroy)(void *);
    destroy = s->destroy;
    /* c->destroy == NULL flags not to call qsSource_destroy() */
    s->destroy = NULL;
    /* destroy inheriting object first */
    destroy(s);
  }

  /* now destroy this and the base object
   * if it's not being destroyed now. */
  if(s->framePtr)
    _qsSource_internalDestroy(s);
}

void *qsSource_create(QsSource_ReadFunc_t read,
    int numChannels, int maxNumFrames /* max num frames buffered */,
    struct QsSource *groupSource /* groupSource=NULL to make a new group */,
    size_t objectSize)
{
  struct QsSource *s;
  QS_ASSERT(read);
  QS_ASSERT(numChannels > 0);

  if(!qsApp) qsApp_init(NULL, NULL);

  if(objectSize < sizeof(*s))
    objectSize = sizeof(*s);

  s = _qsAdjusterList_create(objectSize);
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
    s->isMaster = TRUE;
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

  qsApp->sources = g_slist_append(qsApp->sources, s);

  return s;
}

void qsSource_initIt(struct QsSource *s, struct QsIterator *it)
{
  QS_ASSERT(s);
  QS_ASSERT(!s->isMaster);
  QS_ASSERT(it);

  s->i = it->i;
  s->wrapCount = it->wrapCount;

  // Empty all the iterators that use this source s.
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

void qsSource_addTraceDraw(struct QsSource *s,  struct QsTrace *trace)
{
  QS_ASSERT(s);
  QS_ASSERT(trace);
  // Don't add a trace draw callback twice between trace and
  // a particular source.
  if(g_slist_find(s->traces, trace) == NULL)
  {
    s->traces = g_slist_append(s->traces, trace);
    trace->drawSources = g_slist_append(trace->drawSources, s);
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

  if(ret == 1)
  {
    GSList *l, *next;

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
    for(l = s->traces;l;l=l->next)
    {
      // We queue the drawing areas after we finish having all the traces
      // add their pixel points.
      QS_ASSERT(l->data);
      // This is an inline function that will handle being called
      // more than once, and when it's not really needed.
      _qsWin_postTraceDraw(((struct QsTrace *) l->data)->win);
    }

    // The prevT is the previous time that we got data,
    // so that source read callbacks can know when the
    // last time we got data was.
    s->prevT = time;
  }
  else if(ret == -1)
  {
    qsSource_destroy(s);
    return 1;
  }

  return 0;
}
