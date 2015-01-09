/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2015  Lance Arsenault
 * GNU General Public License version 3
 */
#include "../lib/quickscope.h"

int main(int argc, char **argv)
{
  struct QsSource *soundIn, *sweep;
  struct QsTrace *trace;
 
  qsApp_init(&argc, &argv);

  // Setup some default quickscope parameters
  qsApp->op_fade = false;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = false;
  qsApp->op_axis = false;
  qsApp->op_width = 1000;
  qsApp->op_showStatusbar =false;
  qsApp->op_showControlbar =false;
  qsApp->op_height = 200;
  qsApp->op_showWindowBorder = false;


  qsIdle_create();

  soundIn = qsPulseCapture_create( 2000 /* maxNumFrames */,
      44100/*sampleRate (Hz)*/, NULL /* group */);

  sweep = qsSweep_create(2.0F/100.0F/*sweep period*/,
      0.01F/*level*/, 1/*slope 0==free run*/,
      0.0F/*holdOff*/,0/*delay*/,
      soundIn/*source*/, 0/*sourceChannelNum*/);

  trace = qsTrace_create(NULL /* QsWin, NULL to use a default Win */,
      sweep, 0, soundIn, 0, /* x/y source and channels */
      1.0F, 0.4F, 0, 0, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 1, 0, 0 /* RGB line color */);

  qsTrace_setSwipeX(trace, true);

  qsApp_main();
  qsApp_destroy();

  return 0;
}
