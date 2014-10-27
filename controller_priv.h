/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

struct QsSource;

struct QsController
{
  GSList *sources;

  /* set this in objects that inherit QsController */
  /* to notify super object of a source addition or removal */
  void (*changedSource)(struct QsController *c, GSList *sources);
  void (*destroy)(void *);   /* to destroy super object */

#ifdef QS_DEBUG
  size_t objectSize;
#endif
};

QS_BASE_DECLARE(_qsController);

extern
void *_qsController_create(
    void (*changedSource)(struct QsController *c, GSList *sources),
    size_t objectSize);
extern
void _qsController_destroy(struct QsController *c);
/* protected */
/* Always returns true for super class QsInternal g_timeout thingy. */
bool _qsController_run(struct QsController *c);
