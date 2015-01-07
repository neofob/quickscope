/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include "quickscope.h"

int main(int argc, char **argv)
{
  struct QsSource *sin, *cos=NULL, *sweep;
  struct QsTrace *trace;
  float period = 0.5F, sweepPeriod = 4.132F;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = qsApp_bool("fade", true);
  qsApp->op_fadePeriod = 9.0F;
  qsApp->op_fadeDelay =  0.6F;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;
  qsApp->op_axis = false;

  if(qsApp_bool("fast", false))
  {
    period = 1.23421234F/200.0F;
    sweepPeriod = 0.01F;
    qsApp->op_fadePeriod = 0.02F;
    qsApp->op_fadeDelay =  0.04;
  }


  sin = qsSin_create( 30000 /* maxNumFrames */,
        0.45F /*amplitude*/, period,
        0.0F*M_PI /*phaseShift*/,
        qsApp_int("samples-per-period", 100),
        NULL /* group */);

  sweep = qsSweep_create(sweepPeriod,
      0.0F/*level*/, qsApp_int("slope", 1),
      0.0F/*holdOff*/,
      qsApp_float("delay", 0.0F),
      sin/*source*/, 0/*sourceChannelNum*/);

  trace = qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      sweep, 0, sin, 0, /* x/y source and channels */
      1.0F, 1.0F, 0, 0, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 1, 0, 0.8F /* RGB line color */);

  qsTrace_setSwipeX(trace, qsApp_bool("swipe", false));

  if(qsApp_bool("cos", false))
  {
    cos = qsSin_create(0 /* maxNumFrames */,
        0.45F /*amplitude*/, 0.5F /*period*/,
        0.5F*M_PI /*phaseShift*/,
        qsApp_int("samples-per-period", 100),
        sin /* group */);

    trace = qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
        sweep, 0, cos, 0, /* x/y source and channels */
        1.0F, 1.0F, 0, 0, /* xscale, yscale, xshift, yshift */
        true, /* lines */ 0, 1, 0 /* RGB line color */);

    qsTrace_setSwipeX(trace, qsApp_bool("swipe", false));
  }

  qsApp_main();
  qsApp_destroy();

  return 0;
}
