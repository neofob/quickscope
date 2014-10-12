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
#include "base.h"
#include "adjuster.h"
#include "adjuster_priv.h"


struct QsAdjusterSelector
{
  struct QsAdjuster adjuster;
  int index, len;
  int *value, *values;
  char **label;
};


static
void getTextRender(struct QsAdjusterSelector *adj, char *str,
    size_t maxLen, size_t *len)
{
  *len += snprintf(str, maxLen,
      "[<span size=\"xx-small\" face=\"cmsy10\" weight=\"bold\">"
      "l</span>] "
      "<span "ACTIVE_BG_COLOR"> %s </span>",
      adj->label[adj->index]);
}

static
gboolean inc(struct QsAdjusterSelector *adj, struct QsWidget *w)
{
  QS_ASSERT(adj);
  if(adj->index < adj->len - 1)
  {
    *adj->value = adj->values[++adj->index];
    return TRUE;
  }
  return FALSE; // no change
}

static
gboolean dec(struct QsAdjusterSelector *adj, struct QsWidget *w)
{
  QS_ASSERT(adj);
  if(adj->index > 0)
  {
    *adj->value = adj->values[--adj->index];
    return TRUE;
  }
  return FALSE; // no change
}

static
void destroy(struct QsAdjusterSelector *adj)
{
  QS_ASSERT(adj && adj->values && adj->label);

  int i, len;
  len = adj->len;
  for(i=0; i<len; ++i)
    g_free(adj->label[i]);

#ifdef QS_DEBUG
  memset(adj->values, 0, sizeof(int)*adj->len);
  memset(adj->label, 0, sizeof(char *)*adj->len);
#endif

  g_free(adj->values);
  g_free(adj->label);

  // _qsAdjuster_checkBaseDestroy() is not necessary
  // since only the qsAdjuster_destroy() can call this
  // from adj->destroy, but maybe someday
  // this may be inherited and we'll need to expose this
  // destroy() function and call:
  //_qsAdjuster_checkBaseDestroy(adj);
}

struct QsAdjuster *qsAdjusterSelector_create(struct QsAdjusterList *adjs,
    const char *description, int *value, const int *values,
    const char **labels, int valuesLen,
    void (*valueChangeCallback)(void *), void *data)
{
  struct QsAdjusterSelector *adj;
  QS_ASSERT(value);

  adj = _qsAdjuster_create(adjs, description,
      valueChangeCallback, data, sizeof(*adj));
  adj->len = valuesLen;
  adj->values = g_malloc(sizeof(int)*valuesLen);
  adj->label = g_malloc(sizeof(char *)*valuesLen);
  adj->index = -1;

  int i;
  for(i=0; i<valuesLen; ++i)
  {
    adj->values[i] = values[i];
    adj->label[i] = g_strdup(labels[i]);
    if(values[i] == *value)
      adj->index = i;
  }
  QS_VASSERT(adj->index != -1,
      "value=%d not found in list of values", *value);

  adj->value = value;
  adj->adjuster.getTextRender =
    (void (*)(void *obj, char *str, size_t maxLen, size_t *len))
    getTextRender;
  adj->adjuster.inc     = (gboolean (*)(void *, struct QsWidget *)) inc;
  adj->adjuster.dec     = (gboolean (*)(void *, struct QsWidget *)) dec;
  _qsAdjuster_addSubDestroy(adj, destroy);
  return (struct QsAdjuster *) adj;
}

