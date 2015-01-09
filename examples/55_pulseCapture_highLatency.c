/* This scope program reads the microphone or what ever the default Pulse
 * Audio capture device is.  The Pulse Audio client/server method adds
 * lots of latency, like many seconds. That is to be expected, giving that
 * all the data must go through many system I/O operations to get from the
 * PulseAudio server to this PulseAudio client program.  You can tell by
 * comparing this running program to the alsaCapture program which has
 * sub-second latency between time of sound input to time of seeing scope
 * plotted sound. */

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

  /* We use an idle quickscope controller which only blocks when GTK+
   * widgets need to get their turn at the CPU.  So in that sense it's not
   * blocking when GTK+ widgets are not busy and is in a spinning loop
   * calling the source read callbacks.  Since the PulseCapture source
   * read callback calls a blocking read it's the best design.
   * qsIdle_create() uses the glib g_idle_add() function. */
  qsIdle_create();

  /* Create a source that reads the PulseAudio capture device. */
  soundIn = qsPulseCapture_create( 2000 /* maxNumFrames */,
      44100/*sampleRate (Hz)*/, NULL /* group */);

  sweep = qsSweep_create(2.0F/100.0F/*sweep period*/,
      0.01F/*level*/, 1/*slope 0==free run*/,
      0.0F/*holdOff*/,0/*delay*/,
      soundIn/*source*/, 0/*sourceChannelNum*/);

  trace = qsTrace_create(NULL /* QsWin, NULL to use a default Win */,
      sweep, 0, soundIn, 0, /* x/y source and channels */
      1.0F, 0.4F, 0, 0, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 1, 0, 0.9F /* RGB line color */);

  /* We use swiping beam trace instead of fade.  It tends
   * to use much less system resources than fade.  swipeX
   * is a trace thing and not a property of win. */
  qsTrace_setSwipeX(trace, true);

  qsApp_main();
  qsApp_destroy();

  return 0;
}
