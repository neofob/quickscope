/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

struct QsInterval;

extern
void *qsInterval_create(float period /* seconds */);
// Destroy with qsController_destroy();
