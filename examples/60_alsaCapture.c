/* This srope program reads the microphone or what ever the default
 * ASLA capture device it.*/

#include <quickscope.h>

int main(int argc, char **argv)
{
  struct QsSource *soundIn, *sweep;
  struct QsTrace *trace;
  
  qsApp_init(&argc, &argv);

  // Setup some default quickscope parameters
  qsApp->op_fade = false; // We don't fade the beam trace
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;
  qsApp->op_axis = false;
  qsApp->op_width = 1000;
  qsApp->op_showStatusbar =false;
  qsApp->op_showControlbar =false;
  qsApp->op_height = 200;
  qsApp->op_showWindowBorder = false;

  // qsDrawSync_create() makes a scope controller that syncs to
  // the draw cycle.  That should be similar to the qsInterval
  // thing, but this is directly controlled by the drawing cycle,
  // which in some ways seems like a better design.  I have
  // seen the ALSA read take over and do it's CPU sucking
  // spin wait thing when using any controller.
  // In the future we'll write a better AlsaCapture source
  // object, but now it's a simple "blocking read".
  // We think it's the ALSA driver trying to do "better real-time"
  // performance, and in the process wasting lots of CPU.
  // In general a blocking read is a good thing, but maybe
  // not in this ALSA case on my system.
  //
  qsDrawSync_create(NULL/*win object NULL => default*/);

  /* Create a source that reads the ALSA capture device. */
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

  /* We use swiping beam trace instead of fade.  It tends
   * to use much less system resources than fade.  swipeX
   * is a trace thing and not a property of win. */
  qsTrace_setSwipeX(trace, true);

  qsApp_main();
  qsApp_destroy();

  return 0;
}
