/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <math.h> // defines M_PI
#include "quickscope.h"

int main(int argc, char **argv)
{
  struct QsSource *sinSource, *cosSource;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = true;
  qsApp->op_fadePeriod = 0.8F;
  qsApp->op_fadeDelay =  0.2F;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;


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

  qsApp_main();

  qsApp_destroy();

  return 0;
}
