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

extern
void *qsController_create(size_t objectSize);
extern
void qsController_destroy(struct QsController *c);
extern
void qsController_appendSource(struct QsController *c,
    struct QsSource *s, void *callbackData);
extern
void qsController_prependSource(struct QsController *c,
    struct QsSource *s, void *sourceCallbackData);
extern
void qsController_removeSource(struct QsController *c, struct QsSource *s);

/* protected */
/* Always returns TRUE for super class QsInternal g_timeout thingy. */
gboolean _qsController_run(struct QsController *c);

