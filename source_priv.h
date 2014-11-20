/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

/* private method - for QsSource friend QsController */
/* Returns 1 if the source is to be destroyed.
 * It writes to the QsSource frame buffer using the
 * user read callbacks.  The "read" is thought of
 * as the Scope is sampling (reading) data in this call,
 * though internally we are writing to the QsSource frame
 * buffer. */
extern
int _qsSource_read(struct QsSource *source, long double time);
extern
bool _qsSource_checkTypes(struct QsSource *s);
