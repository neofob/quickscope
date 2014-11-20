/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

// This file just includes all the public quickscope
// include files.  We do not use ../quickplot.h so
// that we can access each of the separate header
// files that make up the installed ../quickscope.h
// when debugging the programs in this directory.
// When we have "-g" option in CFLAGS the debugger knows
// where to find the source to these files.  Where
// as, if we just used ../quickscope.h, the debugger
// would not find:w these files included below:

#include "../top_quickscope.h"
#include "../debug.h"
#include "../Assert.h"
#include "../base.h"
#include "../adjuster.h"
#include "../app.h"
#include "../controller.h"
#include "../group.h"
#include "../source.h"
#include "../iterator.h"
#include "../soundFile.h"
#include "../trace.h"
#include "../win.h"
#include "../rungeKutta.h"
#include "../sourceParticular.h"
