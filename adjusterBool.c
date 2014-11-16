/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "adjuster.h"
#include "adjuster_priv.h"


struct QsAdjusterBool
{
  struct QsAdjuster adjuster;
  bool *value;
};


static
void getTextRender(struct QsAdjusterBool *adj, char *str,
    size_t maxLen, size_t *len)
{
  *len += snprintf(str, maxLen,
      "[<span size=\"xx-small\" face=\"cmsy10\" weight=\"bold\">"
      "l</span>] "
      "<span "ACTIVE_BG_COLOR">%s</span>",
      (*adj->value)?"ON ":"OFF");
}

static
bool inc(struct QsAdjusterBool *adj, struct QsWidget *w)
{
  QS_ASSERT(adj);
  if(!(*adj->value))
  {
    *adj->value = true;
    return true;
  }
  return false; // no change
}

static
bool dec(struct QsAdjusterBool *adj, struct QsWidget *w)
{
  QS_ASSERT(adj);
  if((*adj->value))
  {
    *adj->value = false;
    return true;
  }
  return false; // no change
}

struct QsAdjuster *qsAdjusterBool_create(struct QsAdjusterList *adjs,
    const char *description, bool *value,
    void (*changeValueCallback)(void *data), void *data)
{
  struct QsAdjusterBool *adj;
  QS_ASSERT(value);

  adj = _qsAdjuster_create(adjs, description,
      changeValueCallback, data, sizeof(*adj));
  adj->value = value;
  adj->adjuster.getTextRender =
    (void (*)(void *obj, char *str, size_t maxLen, size_t *len))
    getTextRender;
  adj->adjuster.inc        = (bool (*)(void *, struct QsWidget *)) inc;
  adj->adjuster.dec        = (bool (*)(void *, struct QsWidget *)) dec;
  // adj->adjuster.reset not needed for this bool thingy

  return (struct QsAdjuster *) adj;
}

