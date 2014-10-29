/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

// Called after objects are destroyed to see
// if there is any reason to keep qsApp running
// in gtk_main().
extern
void _qsApp_checkDestroy();
