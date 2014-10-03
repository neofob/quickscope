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


struct QsAdjusterBoolean
{
  struct QsAdjuster adjuster;
  gboolean *value;
};


static
void getTextRender(struct QsAdjusterBoolean *adj, char *str,
    size_t maxLen, size_t *len)
{
  *len += snprintf(str, maxLen,
      "[<span size=\"xx-small\" face=\"cmsy10\" weight=\"bold\">"
      "l</span>] "
      "<span "ACTIVE_BG_COLOR">%s</span>",
      (*adj->value)?"ON ":"OFF");
}

static
gboolean inc(struct QsAdjusterBoolean *adj, struct QsWidget *w)
{
  QS_ASSERT(adj);
  if(!(*adj->value))
  {
    *adj->value = TRUE;
    return TRUE;
  }
  return FALSE; // no change
}

static
gboolean dec(struct QsAdjusterBoolean *adj, struct QsWidget *w)
{
  QS_ASSERT(adj);
  if((*adj->value))
  {
    *adj->value = FALSE;
    return TRUE;
  }
  return FALSE; // no change
}

struct QsAdjuster *qsAdjusterBoolean_create(struct QsAdjusterList *adjs,
    const char *description, gboolean *value,
    void (*changeValueCallback)(void *data), void *data)
{
  struct QsAdjusterBoolean *adj;
  QS_ASSERT(value);

  adj = _qsAdjuster_create(adjs, description,
      changeValueCallback, data, sizeof(*adj));
  adj->value = value;
  adj->adjuster.getTextRender =
    (void (*)(void *obj, char *str, size_t maxLen, size_t *len))
    getTextRender;
  adj->adjuster.inc        = (gboolean (*)(void *, struct QsWidget *)) inc;
  adj->adjuster.dec        = (gboolean (*)(void *, struct QsWidget *)) dec;
  // adj->adjuster.reset not needed for this boolean thingy

  return (struct QsAdjuster *) adj;
}

