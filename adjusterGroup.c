/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "adjuster.h"
#include "adjuster_priv.h"

struct QsAdjusterGroup
{
  struct QsAdjuster adjuster;
  struct QsAdjusterGroup *otherGroup; // pointer to other end of group
#if 0 // TODO: add data to make this faster by saving more state
  GList *jumpElement;
  int numAdjusters;
  int adjustersChangeCount;
#endif
};

static
void getTextRender(struct QsAdjusterGroup *adj, char *str,
    size_t maxLen, size_t *len)
{
  QS_ASSERT(adj);
  QS_ASSERT(adj->adjuster.next || adj->adjuster.prev);

  if(!adj->adjuster.next && !adj->adjuster.prev)
  {
    *len += snprintf(str, maxLen, "qsAdjusterGroup_end() was not called");
    return;
  }

  *len += snprintf(str, maxLen,
      "[<span size=\"xx-small\" face=\"cmsy10\" weight=\"bold\">"
      "l</span>] <span style=\"italic\">Group</span>");
}

static
GList *next(struct QsAdjusterGroup *adj, struct QsAdjusterList *adjL)
{
  GList *next;
  QS_ASSERT(adj);
  QS_ASSERT(adj->otherGroup);

  // TODO: make this quicker
  next = g_list_find(adjL->adjusters, adj->otherGroup);
  QS_ASSERT(next);

  if(next->next)
  {
    // Check bug that was:
    QS_ASSERT(next->next->data != (void *) adj);
    return next->next;
  }

  return adjL->adjusters;
}

static
GList *prev(struct QsAdjusterGroup *adj, struct QsAdjusterList *adjL)
{
  GList *prev;
  QS_ASSERT(adj);
  QS_ASSERT(adj->otherGroup);

  // TODO: make this faster
  prev = g_list_find(adjL->adjusters, adj->otherGroup);
  QS_ASSERT(prev);

  if(prev->prev)
  {
    // Check bug that was:
    QS_ASSERT(prev->prev->data != (void *) adj);
    return prev->prev;
  }
  return g_list_last(adjL->adjusters);
}

static
bool startToAdj(struct QsAdjusterGroup *adj, struct QsWidget *w)
{
  QS_ASSERT((void *) adj->adjuster.next == (void *) next);
  if(w->current->next)
  {
    w->current = w->current->next;
    ++w->adjCount;
  }
  else
  {
    w->current = w->adjs->adjusters;
    w->adjCount = 1;
  }
  return true;
}

static
bool endToAdj(struct QsAdjusterGroup *adj, struct QsWidget *w)
{
  QS_ASSERT(adj);
  QS_ASSERT(adj->otherGroup);
  QS_ASSERT((void *) adj->adjuster.prev == (void *) prev);
  GList *start;
  start = g_list_find(w->adjs->adjusters, adj->otherGroup);
  QS_ASSERT(start);

  if(start->next)
    w->current = start->next;
  else
    w->current = start;
  
  // TODO: make this faster
  w->adjCount = g_list_position(w->adjs->adjusters, w->current) + 1;
  
  return true;
}

// Destroy all adjuster(s) in the group
static
void destroy(struct QsAdjusterGroup *adj)
{
  GList *l, *end, *next;

  // Without the other end we don't know where the group
  // ends.
  QS_ASSERT(adj->otherGroup);
  // We should have at least one list with the
  // other group end in it.
  QS_ASSERT(adj->adjuster.adjs && adj->adjuster.adjs->data);
  struct QsAdjusterList *adjL;
  adjL = adj->adjuster.adjs->data;

  l = g_list_find(adjL->adjusters, adj);
  end =  g_list_find(l, adj->otherGroup);

  QS_ASSERT(end);
  QS_ASSERT((void *) adj->otherGroup == end->data);
  
  end = end->next;

  for(l = l->next; l != end; l = next)
  {
    // qsAdjuster_destroy() will remove l from the
    // list.  Lets hope that next is not effected in
    // the removal process.
    next = l->next;
    // This will remove the adjuster for all QsAdjusterList
    // GList(s) that it is in.
    _qsAdjuster_destroy((struct QsAdjuster *) l->data);
  }
  
  // _qsAdjuster_checkBaseDestroy() is not necessary
  // since only the qsAdjuster_destroy() can call this
  // from adj->destroy, but maybe someday
  // this may be inherited and we'll need to expose this
  // destroy() function and call:
  //_qsAdjuster_checkBaseDestroy(adj);
}

struct QsAdjuster *qsAdjusterGroup_start(struct QsAdjusterList *adjs,
     const char *description)
{
  // We wait until qsAdjusterEndGroup_create() to set up
  // all the data needed by this object.
  struct QsAdjusterGroup *adj;
  adj = _qsAdjuster_create(adjs, description, NULL, NULL, sizeof(*adj));
  adj->adjuster.getTextRender =
    (void (*)(void *obj, char *str, size_t maxLen, size_t *len))
    getTextRender;

  return (struct QsAdjuster *) adj;
}

// TODO: make sure that all QsAdjusters in the Group all share
// the same QsAdjusterList objects in their GSList.
// All the QsAdjuters in the group can't use the same GSLists
// without braking _qsAdjuster_destroy() and so on.
void qsAdjusterGroup_end(struct QsAdjuster *s)
{
  QS_ASSERT(s && s->adjs && s->adjs->data);
  struct QsAdjusterGroup *adj, *start;
  struct QsAdjusterList *adjL;
  start = (struct QsAdjusterGroup *) s;
  adjL = s->adjs->data;
  QS_VASSERT((void *) start->adjuster.getTextRender == (void *) getTextRender,
      "Wrong kind of adjuster passed in.");

  adj = _qsAdjuster_create(adjL, s->description, NULL, NULL, sizeof(*adj));
  adj->adjuster.getTextRender =
    (void (*)(void *obj, char *str, size_t maxLen, size_t *len))
    getTextRender;
  adj->adjuster.prev = (GList *(*)(void *obj, struct QsAdjusterList *)) prev;
  adj->adjuster.inc   = (bool (*)(void *, struct QsWidget *)) endToAdj;
  adj->adjuster.dec   = (bool (*)(void *, struct QsWidget *)) endToAdj;
  start->adjuster.inc = (bool (*)(void *, struct QsWidget *)) startToAdj;
  start->adjuster.dec = (bool (*)(void *, struct QsWidget *)) startToAdj;
  _qsAdjuster_addSubDestroy(start, destroy);
  start->adjuster.next = (GList *(*)(void *obj, struct QsAdjusterList *)) next;
  start->otherGroup = adj;
  adj->adjuster.icon = start->adjuster.icon;
  adj->adjuster.iconData = start->adjuster.iconData;
  adj->otherGroup = start;
}

