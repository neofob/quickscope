/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include "quickscope.h"

int main(int argc, char **argv)
{
  struct QsSource *s;
  struct QsSource *(*attractor_create)(int maxNumFrames,
    float rate/*play rate multiplier*/,
    float sigma, float rho, float beta,
    QsRK4Source_projectionFunc_t projectionCallback,
    void *cbdata,
    struct QsSource *group) = qsLorenz_create;
  bool isRossler = false;

  qsApp_init(&argc, &argv);

  if(strcmp(qsApp_string("system", "lorenz"), "rossler") == 0)
  {
    attractor_create = qsRossler_create;
    isRossler = true;
  }


  qsApp->op_fade = qsApp_bool("fade", !qsApp_bool("sweep", false));
  qsApp->op_fadePeriod = 4.0F;
  qsApp->op_fadeDelay =  3.6F;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;

  s = attractor_create( 5000 /* maxNumFrames */,
      isRossler?10:3/*play rate multiplier*/,
      -1,-1,-1,/*sigma, rho, beta -1 == use default*/
      NULL/*projectionCallback*/, NULL/*projectionCallback_data*/,
      NULL /* group */);

  if(qsApp_bool("sweep", false))
  {
    struct QsSource *sweep;
    
    sweep = qsSweep_create(1.731F /*period*/,
      0.0F/*level*/, qsApp_int("slope", 0),
      0.0F/*holdOff*/,
      qsApp_float("delay", 0.0F),
      s/*source*/, 0/*sourceChannelNum*/);

    const float yScale[3] = { 0.01F, 0.01F, 0.01F };
    const float yShift[3] = { 0.0F, 0.0F, 0.0F };

    int i;
    for(i=0; i<3; ++i)
    {
      struct QsTrace *trace;
      trace = qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
        sweep, 0, s, i, /* x/y source and channels */
        1.0F, isRossler?2*yScale[i]:yScale[i], 0, yShift[i],
        /* xscale, yscale, xshift, yshift */
        true, /* lines */ i%1?0:1, i%2?0:1, i%4?1:0 /* RGB line color */);
    
      qsTrace_setSwipeX(trace, qsApp_bool("swipe", true));
    }
  }
  else
  {
    qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      s, 0, s, (isRossler)?1:2, /* x/y source and channels */
      (isRossler)?0.04F:0.02F,/*xscale*/
      (isRossler)?0.04F:0.02F,/*yscale*/
      0, (isRossler)?0:-0.5, /*xshift, yshift */
      true, /* lines */ 1, 0.2, (isRossler)?1:0 /* RGB line color */);
  }

  qsApp_main();
  qsApp_destroy();

  return 0;
}
