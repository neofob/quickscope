/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <quickscope.h>

int main(int argc, char **argv)
{
  struct QsSource *lorenz;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = qsApp_bool("fade", !qsApp_bool("sweep", false));
  qsApp->op_fadePeriod = 4.0F;
  qsApp->op_fadeDelay =  3.6F;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;

  lorenz = qsLorenz_create( 5000 /* maxNumFrames */,
      3/*play rate multiplier*/,
      -1,-1,-1,/*sigma, rho, beta -1 == use default*/
      NULL/*projectionCallback*/, NULL/*projectionCallback_data*/,
      3/*numChannels, can be different if projectionCallback*/,
      NULL /* group */);

  if(qsApp_bool("sweep", false))
  {
    struct QsSource *sweep;
    struct QsTrace *trace;
    
    sweep = qsSweep_create(1.731F /*period*/,
      0.0F/*level*/, qsApp_int("slope", 0),
      0.0F/*holdOff*/,
      qsApp_float("delay", 0.0F),
      lorenz/*source*/, 0/*sourceChannelNum*/);

    trace = qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      sweep, 0, lorenz, 0, /* x/y source and channels */
      1.0F, 0.02F, 0, 0, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 0, 1, 0 /* RGB line color */);
    
    qsTrace_setSwipeX(trace, qsApp_bool("swipe", true));
  }
  else
    qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      lorenz, 0, lorenz, 2, /* x/y source and channels */
      0.02F, 0.02F, 0, -0.5, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 1, 0.2, 1 /* RGB line color */);

  qsApp_main();
  qsApp_destroy();

  return 0;
}
