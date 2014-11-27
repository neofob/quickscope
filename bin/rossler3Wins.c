/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include "quickscope.h"

int main(int argc, char **argv)
{
  struct QsSource *s;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = false;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;

  s = qsRossler_create( 5000 /* maxNumFrames */,
      5/*play rate multiplier*/,
      -1,-1,-1,/*sigma, rho, beta -1 == use default*/
      NULL/*projectionCallback*/, NULL/*projectionCallback_data*/,
      3/*numChannels, can be different if projectionCallback*/,
      NULL /* group */);

  struct QsSource *sweep;

  sweep = qsSweep_create(12.731F /*period*/,
      0.0F/*level*/, qsApp_int("slope", 0),
      0.0F/*holdOff*/,
      qsApp_float("delay", 0.0F),
      s/*source*/, 0/*sourceChannelNum*/);

  qsApp->op_x = 0;
  qsApp->op_y = 0;
  qsApp->op_width = 4000;
  qsApp->op_height = 500;
  qsApp->op_showWindowBorder = false;
  qsApp->op_showControlbar =false;

  int i;
  for(i=0; i<3; ++i)
  {
    // The first window has 3 traces.
    struct QsTrace *trace;
    trace = qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
        sweep, 0, s, i, /* x/y source and channels */
        1.0F, 0.02F, 0, 0,
        /* xscale, yscale, xshift, yshift */
        true, /* lines */
        i%1?0:1, i%2?0:1, i%4?1:0 /* RGB line color */);
    
    qsTrace_setSwipeX(trace, qsApp_bool("swipe", true));
  }

  qsApp->op_x = 0;
  qsApp->op_y = INT_MIN; // INT_MIN is for -0
  qsApp->op_width = 822;
  qsApp->op_height = 522;

  qsApp->op_fade = true;
  qsApp->op_fadePeriod = 11.0F;
  qsApp->op_fadeDelay =  5.6F;
  qsApp->op_showWindowBorder = true;
  qsApp->op_showControlbar = true;

  // new win for this trace
  qsTrace_create(qsWin_create(),
      s, 0, s, 1, /* x/y source and channels */
      0.04F/*xscale*/,
      0.04F/*yscale*/,
      0, 0,/*xshift, yshift */
      true, /* lines */
      1, 0.2, 1 /* RGB line color */);


  qsApp->op_x = INT_MIN; // INT_MIN is for -0
  qsApp->op_y = INT_MIN; // INT_MIN is for -0
  qsApp->op_width = 822;
  qsApp->op_height = 522;

  qsApp->op_axis = false;
  qsApp->op_ticks = false;
  // new win for this trace
  qsTrace_create(qsWin_create(),
      s, 0, s, 2, /* x/y source and channels */
      0.04F,/*xscale*/
      0.04F,/*yscale*/
      0, -0.4, /*xshift, yshift */
      true, /* lines */ 1, 0.2, 0.6 /* RGB line color */);

  qsApp_main();
  qsApp_destroy();

  return 0;
}
