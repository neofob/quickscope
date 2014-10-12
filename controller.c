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
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "base.h"
#include "timer.h"
#include "app.h"
#include "controller.h"
#include "controller_priv.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "group.h"
#include "source.h"
#include "source_priv.h"


// butt ugly marco coding
//
// QS_BASE_DEFINE(), from base.h, defines:
// _qsController_checkBaseDestroy(void *controller)
// and
// _qsController_addSubDestroy(void *controller, (void (*destroy)(void *)))
QS_BASE_DEFINE(_qsController, struct QsController)


// See qsController_destroy() below
void _qsController_destroy(struct QsController *c)
{
  // destroy the sub class
  QS_BASE_CHECKSUBDESTROY(c);

  // now destroy this class
  while(c->sources)
  {
#ifdef QS_DEBUG
    void *data;
    data = c->sources->data;
#endif
    qsSource_destroy((struct QsSource *) c->sources->data);
    /* Make sure it was removed from the list. */
    QS_ASSERT((c->sources && c->sources->data != data) || !c->sources);
  }

  qsApp->controllers = g_slist_remove(qsApp->controllers, c);

#ifdef QS_DEBUG
  memset(c, 0, sizeof(c->objectSize));
#endif
  g_free(c);
}



gboolean _qsController_run(struct QsController *c)
{
  GSList *l;
  long double t;
  int sourceChanged = 0;

  QS_ASSERT(c);

  /* we will go through the list of QsSources */
  l = c->sources;
  t = qsTimer_get(qsApp->timer);

  do
  {
    struct QsSource *s;
    s = l->data;
    l = l->next;

    /* This may remove and destroy the QsSource */
    sourceChanged |= _qsSource_read(s, t, s->callbackData);
  }
  while(l);

  if(sourceChanged && c->changedSource)
    c->changedSource(c, c->sources);

  return TRUE;
}

void *_qsController_create(
    void (*changedSource)(struct QsController *c, GSList *sources),
    size_t size)
{
  struct QsController *c;

  if(!qsApp) qsApp_init(NULL, NULL);

  if(size < sizeof(struct QsController *))
    size = sizeof(struct QsController *);
  c = g_malloc0(size);
#ifdef QS_DEBUG
  c->objectSize = size;
#endif

  c->changedSource = changedSource;

  qsApp->controllers = g_slist_prepend(qsApp->controllers, c);

  return c;
}


// We need to expose the _destroy() thingy to the
// user API while keeping the _qsController_destroy(),
// so we have this wrapper.
void qsController_destroy(struct QsController *c)
{
  _qsController_destroy(c);
}

static
void _qsController_changeSource(struct QsController *c,
    struct QsSource *s, void *callbackData,
    GSList *(*g_list_func)(GSList *, gpointer data))
{
  c->sources = g_list_func(c->sources, s);
  s->callbackData = callbackData;
  if(c->changedSource)
    c->changedSource(c, c->sources);
}

void qsController_appendSource(struct QsController *c,
    struct QsSource *s, void *callbackData)
{
  QS_ASSERT(c);
  QS_ASSERT(s);
  if(s->controller)
    qsController_removeSource(c, s);
  _qsController_changeSource(c, s, callbackData, g_slist_append);
  s->controller = c;
}

void qsController_prependSource(struct QsController *c,
    struct QsSource *s, void *callbackData)
{
  QS_ASSERT(c);
  QS_ASSERT(s);
  if(s->controller)
    qsController_removeSource(c, s);
  _qsController_changeSource(c, s, callbackData, g_slist_prepend);
  s->controller = c;
}

// This does not destroy the source object.
void qsController_removeSource(struct QsController *c, struct QsSource *s)
{
  QS_ASSERT(c);
  QS_ASSERT(s);
  _qsController_changeSource(c, s, NULL,
      (GSList *(*)(GSList *, gpointer data)) g_slist_remove);
  s->controller = NULL;
}

