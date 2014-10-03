/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
struct QsDrawSync;
struct QsWin;

extern
void *qsDrawSync_create(struct QsWin *win);
extern
void qsDrawSync_destroy(struct QsDrawSync *ds);

