/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <math.h> // defines M_PI
#include "../lib/quickscope.h"


int main(int argc, char **argv)
{
  struct QsSource *sinSource, *sweepSource;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = true;
  qsApp->op_fadePeriod = 4.0F;
  qsApp->op_fadeDelay = 5.0F;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;

  sinSource = qsSin_create( 100 /* maxNumFrames */,
        0.4F /*amplitude*/, 1.2348F /*period*/,
        0.0F /*phaseShift*/, 35 /*samplesPerPeriod*/,
        NULL /* group */);

  sweepSource = qsSweep_create(4/*period seconds*/, 0/*level*/,
    0/*slope 0==freerun*/, 0/*holdOff*/, 0/*delay*/, sinSource, 0/*channelNum*/);


  qsTrace_create(NULL /* QsWin, NULL to make a default */,
      sweepSource, 0, sinSource, 0, /* x/y source and channels */
      1.0F, 0.9F, 0, 0, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 0.1, 1, 1 /* RGB line color */);

  qsApp_main();

  qsApp_destroy();

  return 0;
}
