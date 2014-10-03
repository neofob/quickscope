/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
struct QsTimer;

extern
void qsTimer_stop(struct QsTimer *timer);
extern
void qsTimer_start(struct QsTimer *timer);
extern
void qsTimer_set(struct QsTimer *timer, long double t);
extern
long double qsTimer_get(struct QsTimer *timer);
extern
struct QsTimer *qsTimer_create(void);
extern
void qsTimer_sync(struct QsTimer *to, struct QsTimer *timer);
extern
void qsTimer_destroy(struct QsTimer *timer);

