/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
/////////////////////// WHY? ///////////////////////////
// This file just includes all the public quickscope
// include files.  We do not use ../lib/quickplot.h so
// that we can access each of the separate header
// files that make up the installed ../lib/quickscope.h
// when debugging the programs in this directory.
// When we have "-g" option in CFLAGS the debugger knows
// where to find the source to these files.  Where
// as, if we just used ../lib/quickscope.h, the debugger
// would not find these files included below:

#include "../lib/top_quickscope.h"
#include "../lib/debug.h"
#include "../lib/Assert.h"
#include "../lib/base.h"
#include "../lib/adjuster.h"
#include "../lib/app.h"
#include "../lib/controller.h"
#include "../lib/group.h"
#include "../lib/source.h"
#include "../lib/iterator.h"
#include "../lib/soundFile.h"
#include "../lib/trace.h"
#include "../lib/win.h"
#include "../lib/rungeKutta.h"
#include "../lib/sourceParticular.h"
