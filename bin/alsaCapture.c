/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include "quickscope.h"

int main(int argc, char **argv)
{
  struct QsSource *soundIn, *sweep;
  struct QsTrace *trace;
  
  qsApp_init(&argc, &argv);

  // Setup some default quickscope parameters
  qsApp->op_fade = false;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;
  qsApp->op_axis = false;
  qsApp->op_width = 1000;
  qsApp->op_showStatusbar =false;
  qsApp->op_showControlbar =false;
  qsApp->op_height = 200;
  qsApp->op_showWindowBorder = false;

#if 0 // Idle or Other
  // qsIdle_create() makes a scope controller that just keeps
  // looping without a direct blocking call.  The ALSA sound
  // capture source (from qsAlsaCapture_create()) does a blocking
  // read of the sound capture (default ALSA sound input device).
  // Looks like on my system the ALSA blocking read uses CPU
  // time when it's blocking.  I don't know what the kernel is
  // doing, a "spin wait", or other like thing.  Anyway it
  // seems to work well even when is show 100% CPU usage.
  // Keep in mind, it's the simplest possible alsa code, just
  // a blocking read which is usually a good/simple design.
  qsIdle_create();
#else
#  if 1 // Interval or DrawSync
  // qsInterval_create() uses an interval timer to call the ALSA
  // read code.  Seems a little redundant to have two blocking
  // calls in a row, the GTK interval polling in it's main loop
  // and the ALSA read call.  1/60 is used because it's about
  // the refresh rate of most computer monitors, so any faster
  // is pointless.  It appears that this controller does most
  // of the blocking, in place of the ALSA read call.  I have
  // seen the ALSA read take over and do it's CPU sucking
  // spin wait thing when using this controller.
  //
  qsInterval_create(1.0F/60.0F);

#  else
  // qsDrawSync_create() makes a scope controller that syncs to
  // the draw cycle.  That should be similar to the qsInterval
  // thing, but this is directly controlled by the drawing cycle,
  // which in some ways seems like a better design.  I have
  // seen the ALSA read take over and do it's CPU sucking
  // spin wait thing when using this controller.
  //
  qsDrawSync_create(NULL/*win object NULL => default*/);
#  endif
#endif



  soundIn = qsAlsaCapture_create( 2000 /* maxNumFrames */,
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
