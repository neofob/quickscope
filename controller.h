/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
struct QsSource;
struct QsController;

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

