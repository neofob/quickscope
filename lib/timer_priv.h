/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
struct QsTimer;

#if 0 // TODO remove these declarations:
// The only function we seem to need is _qsTimer_get()
extern
void qsTimer_stop(struct QsTimer *timer);
extern
void qsTimer_start(struct QsTimer *timer);
extern
void qsTimer_sync(struct QsTimer *to, struct QsTimer *timer);
extern
void qsTimer_set(struct QsTimer *timer, long double t);
#endif

extern
long double _qsTimer_get(struct QsTimer *timer);
extern
struct QsTimer *_qsTimer_create(void);
extern
void _qsTimer_destroy(struct QsTimer *timer);
