/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "adjuster.h"
#include "adjuster_priv.h"


static inline
GList *_qsWidget_initAdjuster(struct QsWidget *w)
{
  QS_ASSERT(w);
  QS_ASSERT(w->adjs);
  if(!w->current)
  {
    // Set the widget to act on the first adjuster in the list
    w->current = w->adjs->adjusters;
    w->adjCount = 1;
  }

  return w->current;
}

#define TEXTLEN  512

void _qsWidget_display(struct QsWidget *w)
{
  char text[TEXTLEN];
  struct QsAdjuster *adj = NULL;

  if(_qsWidget_initAdjuster(w))
  {
    size_t len = 0;
    
    adj = w->current->data;

    if(adj->icon)
      len += adj->icon(&text[len], TEXTLEN - len, adj->iconData);

    len += snprintf(&text[len], TEXTLEN - len, "%s ", adj->description);

    if(adj->getTextRender)
      adj->getTextRender(adj, &text[len], TEXTLEN - len, &len);

    // TODO: the counts will be wrong if any adjusters are destroyed
    snprintf(&text[len], TEXTLEN - len,
        " <span fgcolor=\"#777777\">%dof%d</span> [np]",
        w->adjCount, w->adjs->count);

  }
  else // no adjuster available
  {
    snprintf(text, TEXTLEN, "no adjuster in list");
  }

  if(w->displayCallback)
      w->displayCallback(w, adj, text, w->displayCallbackData);
#ifdef QS_DEBUG
  else
    printf("%s() without callback set text=\"%s\"\n", __func__, text);
#endif
}

// _qsWidget_next() and _qsWidget_prev() must be good
// friends with the code in adjusterGroup.c
void _qsWidget_next(struct QsWidget *w)
{
  if(!_qsWidget_initAdjuster(w)) return;

  struct QsAdjuster *adj;
  QS_ASSERT(w->current->data);
  adj = w->current->data;

  if(adj->next)
  {
    w->current = adj->next(adj, w->adjs);
    w->adjCount = g_list_position(w->adjs->adjusters, w->current) + 1;
  }
  else if(w->current->next)
  {
    w->current = w->current->next;
    ++w->adjCount;
  }
  else
  {
    w->current = w->adjs->adjusters;
    w->adjCount = 1;
  }

  _qsWidget_display(w);
}

void _qsWidget_prev(struct QsWidget *w)
{
  if(!_qsWidget_initAdjuster(w)) return;

  struct QsAdjuster *adj;
  QS_ASSERT(w->current->data);
  adj = w->current->data;

  if(adj->prev)
  {
    w->current = adj->prev(adj, w->adjs);
    // TODO: redesign to make this faster
    w->adjCount = g_list_position(w->adjs->adjusters, w->current) + 1;
  }
  else if(w->current->prev)
  {
    w->current = w->current->prev;
    --w->adjCount;
  }
  else
  {
    w->current = g_list_last(w->current);
    w->adjCount = w->adjs->count;
  }

  _qsWidget_display(w);
}

static inline
struct QsAdjuster *_getAdjuster(struct QsWidget *w)
{
  if(!_qsWidget_initAdjuster(w)) return NULL;

  QS_ASSERT(w->current->data);

  return (struct QsAdjuster *)  w->current->data;
}

void _qsWidget_shiftLeft(struct QsWidget *w)
{
  struct QsAdjuster *adj;
  adj = _getAdjuster(w);
  if(!adj) return;

  if(adj->shiftLeft && adj->shiftLeft(adj))
      _qsWidget_display(w);
}

void _qsWidget_shiftRight(struct QsWidget *w)
{
  struct QsAdjuster *adj;
  adj = _getAdjuster(w);
  if(!adj) return;

  if(adj->shiftRight && adj->shiftRight(adj))
      _qsWidget_display(w);
}

void _qsWidget_inc(struct QsWidget *w)
{
  struct QsAdjuster *adj;
  adj = _getAdjuster(w);
  if(!adj) return;

  if(adj->inc)
  {
    if(adj->inc(adj, w))
    {
      if(adj->changeValueCallback) adj->changeValueCallback(adj->data);
      _qsWidget_display(w);
    }
  }
}

void _qsWidget_dec(struct QsWidget *w)
{
  struct QsAdjuster *adj;
  adj = _getAdjuster(w);
  if(!adj) return;

  if(adj->dec)
  {
    if(adj->dec(adj, w))
    {
      if(adj->changeValueCallback) adj->changeValueCallback(adj->data);
      _qsWidget_display(w);
    }
  }
}

static inline
void _qsAjusterList_WidgetsReset(struct QsAdjusterList *adjs)
{
  GList *wl;
  wl = adjs->widgets;
  while(wl)
  {
    QS_ASSERT(wl->data);
    ((struct QsWidget *) wl->data)->current = NULL;
    // Now the next time this renders it all
    // pop back to the first adjuster.
    wl = wl->next;
  }
}

void *_qsAdjuster_create(struct QsAdjusterList *adjs,
    const char *description,
    void (*changeValueCallback)(void *data), void *data,
    size_t objSize)
{
  struct QsAdjuster *adj;
  if(objSize < sizeof(*adj))
    objSize = sizeof(*adj);
  adj = g_malloc0(objSize);
  adj->description = g_strdup(description);
  adj->changeValueCallback = changeValueCallback;
  adj->data = data;

  adj->adjs = g_slist_prepend(NULL, adjs);

#ifdef QS_DEBUG
  adj->objSize = objSize;
#endif

  if(adjs->adjusters)
  {
    // Copy the icon callback from the last adjuster
    // the user can change it if necessary.
    struct QsAdjuster *lastAdj;
    lastAdj = g_list_last(adjs->adjusters)->data;
    adj->icon = lastAdj->icon;
    adj->iconData = lastAdj->iconData;
  }

  adjs->adjusters = g_list_append(adjs->adjusters, adj);
  ++adjs->changeCount;
  ++(adjs->count);

  return adj;
}

static inline
gboolean _qsAdjuster_isInList(struct QsAdjuster *adj,
    struct QsAdjusterList *adjL)
{
  QS_ASSERT(adj);
  if(!adjL) return TRUE; // yes have nothings in the list
  GSList *l;
  for(l = adj->adjs; l; l = l->next)
    if(l->data == (void *) adjL)
#ifndef QS_DEBUG
      return TRUE;
#else
      break;

  if(l)
  {
    GList *dl;
    for(dl = adjL->adjusters; dl; dl = dl->next)
      if(dl->data == (void *) adj)
        return TRUE;
    QS_VASSERT(0,
        "adjuster not found in adjusterList that adjuster says it's in");
  }
#endif

  return FALSE; // no it's not in the list
}

static inline
void _qsAdjuster_disassociate(struct QsAdjuster *adj, struct QsAdjusterList *adjL)
{
  // Keep the Widgets from getting screwed
  _qsAjusterList_WidgetsReset(adjL);
  // remove adj from QsAdjusterList
  adjL->adjusters = g_list_remove(adjL->adjusters, adj);
  ++adjL->changeCount;
  --adjL->count;
  // remove adjL from adjs' list
  adj->adjs = g_slist_remove(adj->adjs, adjL);
}

// disassociates an adjuster from an AdjusterList and if there will
// be no remaining associated AdjusterLists is self destructs
static inline
void _qsAdjuster_dereference(struct QsAdjuster *adj, struct QsAdjusterList *adjL)
{
  QS_ASSERT(adj && adj->adjs);

  if(_qsAdjuster_isInList(adj, adjL))
  {
    if(adj->adjs->next)
      // There is more than one QsAdjusterList that lists this
      // so just remove this from adjL
      _qsAdjuster_disassociate(adj, adjL);
    else
      // adj has no reason to live since the one and only
      // QsAdjusterList has abandoned it
      _qsAdjuster_destroy(adj);
  }
}

void _qsAdjuster_destroy(struct QsAdjuster *adj)
{
  QS_ASSERT(adj && adj->adjs);

  if(adj->destroy)
  {
    void (*destroy)(void *adj);
    destroy = adj->destroy;
    // stop re entrance
    adj->destroy = NULL;
    destroy(adj);
  }

  // Remove this adjuster from all QsAdjusterList objects
  // This is not just called from _qsAdjuster_dereference()
  while(adj->adjs)
    _qsAdjuster_disassociate(adj, (struct QsAdjusterList *) adj->adjs->data);

  g_free(adj->description);
#ifdef QS_DEBUG
  memset(adj, 0, adj->objSize);
#endif
  g_free(adj);
}

void *_qsAdjusterList_create(size_t objSize)
{
  struct QsAdjusterList *adjs;
  if(objSize < sizeof(*adjs))
    objSize = sizeof(*adjs);
  adjs = g_malloc0(objSize);

#ifdef QS_DEBUG
  adjs->objSize = objSize;
#endif

  return adjs;
}

void *_qsWidget_create(struct QsAdjusterList *adjs,
    void (*displayCallback)(struct QsWidget *w,
      const struct QsAdjuster *adj, const char *text, void *data),
    void *displayCallbackData, size_t objSize)
{
  struct QsWidget *w;
  QS_ASSERT(adjs);

  if(objSize < sizeof(*w))
    objSize = sizeof(*w);
  w = g_malloc0(objSize);
  w->adjs = adjs;
  w->current = adjs->adjusters;
  adjs->widgets = g_list_prepend(adjs->widgets, w);
  w->displayCallback = displayCallback;
  w->displayCallbackData = displayCallbackData;
#ifdef QS_DEBUG
  w->objSize = objSize;
#endif
  return w;
}

void _qsWidget_destroy(struct QsWidget *w)
{
  QS_ASSERT(w);
  QS_ASSERT(w->adjs);
  QS_ASSERT(w->adjs->widgets);
  w->adjs->widgets = g_list_remove(w->adjs->widgets, w);

#ifdef QS_DEBUG
  memset(w, 0, w->objSize);
#endif
  g_free(w);
}

void _qsAdjusterList_display(struct QsAdjusterList *adjs)
{
  GList *wl;
  wl = adjs->widgets;
  while(wl)
  {
    QS_ASSERT(wl->data);
    _qsWidget_display((struct QsWidget *) wl->data);
    wl = wl->next;
  }
}

void _qsAdjusterList_destroy(struct QsAdjusterList *adjs)
{
  // By destroying the widgets before the adjusters we
  // don't reset the widgets while destroying the adjusters.
  while(adjs->widgets)
    _qsWidget_destroy((struct QsWidget *) adjs->widgets->data);

  while(adjs->adjusters)
    _qsAdjuster_dereference((struct QsAdjuster *) adjs->adjusters->data, adjs);

#ifdef QS_DEBUG
  memset(adjs, 0, adjs->objSize);
#endif
  g_free(adjs);
}

static inline
gboolean check_listEmpty(struct QsAdjusterList *actL)
{
  if(!actL || !actL->adjusters) return TRUE;

  return FALSE;
}

void _qsAdjusterList_append(struct QsAdjusterList *adjL,
    struct QsAdjusterList *apL)
{
  QS_ASSERT(adjL);
  QS_ASSERT(adjL != apL);
  if(check_listEmpty(apL)) return;
  GList *lastAdded = NULL, *first = NULL;

  GList *l;
  for(l = apL->adjusters; l; l = l->next)
    // We add only the elements that we need to
    if(!_qsAdjuster_isInList((struct QsAdjuster *)l->data, adjL))
    {
      GList *n;
      // prepend l to adjL adjuster list
      n = g_list_alloc();
      n->prev = lastAdded;
      n->next = NULL;
      n->data = l->data;
      QS_ASSERT(l->data);
      if(lastAdded)
        lastAdded->next = n;
      else
        first = n;

      lastAdded = n;
      // add to adjuster adjusterList slist
      struct QsAdjuster *adj;
      adj = l->data;
      QS_ASSERT(adj);
      adj->adjs = g_slist_prepend(adj->adjs, adjL);
      ++adjL->count;
    }

  if(first)
  {
    if(adjL->adjusters)
    {
      // put first on the end
      GList *end;
      for(end = adjL->adjusters; end->next; end = end->next);
      end->next = first;
      first->prev = end;
    }
    else
      // make first the list
      adjL->adjusters = first;

    ++adjL->changeCount;
  }

  // The user of this needs to queue a draw of the Widgets.
  // We can't here because we don't know the state of the
  // Widgets and there may be more changes to come.
}

void _qsAdjusterList_prepend(struct QsAdjusterList *adjL,
    struct QsAdjusterList *ppL)
{
  QS_ASSERT(adjL);
  QS_ASSERT(adjL != ppL);

  if(check_listEmpty(ppL)) return;
  GList *lastAdded = NULL, *first = NULL;

  GList *l;
  for(l = ppL->adjusters; l; l = l->next)
    // We add only the elements that we need to
    if(!_qsAdjuster_isInList((struct QsAdjuster *)l->data, adjL))
    {
      GList *n;
      // prepend l to adjL adjuster list
      n = g_list_alloc();
      n->prev = lastAdded;
      n->next = NULL;
      n->data = l->data;
      QS_ASSERT(l->data);
      if(lastAdded)
        lastAdded->next = n;
      else
        first = n;

      lastAdded = n;
      // add to adjuster adjusterList slist
      struct QsAdjuster *adj;
      adj = l->data;
      QS_ASSERT(adj);
      adj->adjs = g_slist_prepend(adj->adjs, adjL);
      ++adjL->count;
    }

  if(first)
  {
    if(adjL->adjusters)
    {
      adjL->adjusters->prev = lastAdded;
      lastAdded->next = adjL->adjusters;
    }
    adjL->adjusters = first;
    
    ++adjL->changeCount;
  }

  
  // The user of this needs to queue a draw of the Widgets.
  // We can't here because we don't know the state of the
  // Widgets and there may be more changes to come.
}

void _qsAdjusterList_remove(struct QsAdjusterList *adjL,
    struct QsAdjusterList *rmL)
{
  QS_ASSERT(adjL);

  if(check_listEmpty(rmL)) return;

  GList *l, *n;

  // TODO: This is stupid slow
  for(l = rmL->adjusters; l; l = n)
  {
    n = l->next;
    QS_ASSERT(l->data);
    _qsAdjuster_dereference((struct QsAdjuster *) l->data, adjL);
  }

  // The user of this needs to queue a draw of the Widgets.
  // We can't here because we don't know the state of the
  // Widgets and there may be more changes to come.
}

