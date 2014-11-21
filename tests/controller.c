/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

/* This is a very bad user example.  It tests
 * the QsIdle controller which uses all the
 * idle CPU if not of the usleep() in the
 * Wait callback.  Check `top' for CPU usage.
 * I must admit GTK does a good job of running
 * when the usleep() is removed.  You can still
 * use the widgets and quit the program with ease
 * even with 100% CPU usage showing in `top'.
 */

#include <unistd.h>
#include <math.h> // defines M_PI
#include "quickscope.h"


static bool
Wait(struct QsSource *s, void *data)
{
  //usleep(10000);
  return true;
}


int main(int argc, char **argv)
{
  struct QsSource *sinSource, *cosSource;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = true;
  qsApp->op_fadePeriod = 0.8F;
  qsApp->op_fadeDelay =  0.2F;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;

  // controllers do not do anything unless they have
  // sources added to them.  In this case the last
  // controller made will be the default controller
  // for the sources when qsApp_main() is called.
  // The controllers before the last will not be
  // used unless the user adds sources to them,
  // overriding this default behavior.

  qsIdle_create();          // controller 1
  qsInterval_create(0.5);   // controller 2
  qsDrawSync_create(NULL);  // controller 3, last before qsApp_main()



  sinSource = qsSin_create( 1000 /* maxNumFrames */,
        0.4F /*amplitude*/, 1.2348F /*period*/,
        0.0F /*phaseShift*/, 100 /*samplesPerPeriod*/,
        NULL /* group */);

  cosSource = qsSin_create( 0 /* maxNumFrames */,
        0.4F /*amplitude*/, 1.2348F /*period*/,
        M_PI/2.0F /*phaseShift*/, 100 /*samplesPerPeriod*/,
        sinSource /* group is same as sinSource group */);

  qsTrace_create(NULL /* QsWin, NULL to make a default */,
      cosSource, 0, sinSource, 0, /* x/y source and channels */
      1, 1, 0, 0, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 0, 1, 0 /* RGB line color */);

  qsSource_addChangeCallback(sinSource,
      (bool (*)(struct QsSource *, void *)) Wait, NULL);

  qsApp_main();

  qsApp_destroy();

  return 0;
}
