/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <quickscope.h>

int main(int argc, char **argv)
{
  struct QsSource *sin, *sweep;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = TRUE;
  qsApp->op_fadePeriod = 4.0F;
  qsApp->op_fadeDelay =  0.6F;
  qsApp->op_doubleBuffer = TRUE;
  qsApp->op_grid = 0;

  sin = qsSin_create( 1000 /* maxNumFrames */,
        0.45F /*amplitude*/, 0.5F /*period*/,
        0.0F*M_PI /*phaseShift*/, 40 /*samplesPerPeriod*/,
        NULL /* group */);

  sweep = qsSweep_create(2.7F /*period*/,
      0.0F/*level*/, 1/*slope 0==free*/, 0.0F/*holdOff*/,
      -1.125F/*delay*/, sin/*source*/, 0/*sourceChannelNum*/);

  qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      sweep, 0, sin, 0, /* x/y source and channels */
      1.0F, 1.0F, 0, 0, /* xscale, yscale, xshift, yshift */
      TRUE, /* lines */ 1, 0, 0 /* RGB line color */);

  qsApp_main();

  qsApp_destroy();

  return 0;
}
