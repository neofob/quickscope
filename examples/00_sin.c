/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
/* This is just about the simplest libquickscope example that does
 * something reasonable.  It has lots of comments.  Maybe too many.  See
 * 05_sin_lessComments.c if you feel nauseous.  This example calculates
 * and plots a sine wave function on the scope, in real-time.  It's not
 * quite the simplest program, because using scope X-Y mode is simpler,
 * but this example is more expected, since most software oscilloscopes
 * are not capable of X-Y mode, and so we give you what you expect; a
 * sweep driven X trace, with a sin driven Y trace.
 *
 * This example uses a libquickscope built-in sine source.  We'll get to
 * writing arbitrary user code to scope source data in later examples;
 * that's just a little bit more complex than this example. */

#include <quickscope.h>

int main(void)
{
  struct QsSource *sin, *sweep; // y source , x source

  /* Create a source that has a single channel that is a sine signal.  In
   * this case it's calling sinf() from the math library, libm, inside
   * libquickscope. */
  sin = qsSin_create( 500 /*maxNumFrames*/,
        /* maxNumFrame is very important:
         * it's around the size of the ring buffers
         * for all sources in a source group.
         * If this is too small you may miss seeing
         * some of the data that is created in the
         * QsSource in a given read cycle. */
        0.45F /*amplitude*/, 0.061F /*period, seconds*/,
        0.0F*M_PI /*phase shift*/,
        40 /*samples-per-period, this is a request, this is compared
             with other sources requested values of this*/,
        NULL /*source group, NULL => create one */);

  /* Create a source that uses the sin source to make a sweep that we will
   * use as the X input into the trace.  In quickscope sweep is just a
   * dependent source that is the X input.  We could have just made
   * another sine source for the X part of the trace and draw Lissajous
   * figures. http://wikipedia.org/wiki/Lissajous_curve */
  sweep = qsSweep_create(0.2F /*sweep period in seconds*/,
      0.0F /*trigger level*/,
      1 /*slope may be  -1, 0, 1   0 => free run */,
      0.3F /*trigger hold off*/,
      0.0F /*delay after trigger, may be negative*/,
      sin /*source that causes the triggering and is also the group */,
      0 /*source channel number, 0=first*/);

  /* Now sin and sweep are in the same source group, and therefore can be
   * used together in a trace. */

  /* create a trace with sin (Y) and sweep (X) */
  qsTrace_create(NULL /* QsWin, NULL to use a default Win */,
      /* This will automatically make a QsWin. QsTraces do
       * not exist without a QsWin. */
      sweep, 0, /*X-source, channel number, in this case there is only
                  one channel in the sweep source, the 0 channel*/
      sin,   0, /*Y-source, channel number, in this case there is only one
                  channel in the sin source*/
      1.0F, 1.0F, 0.0F, 0.0F, /* xscale, yscale, xshift, yshift; relative
            to view port size, note the sin amplitude was set in
            qsSin_create() above, The normalized view port size in [-1/2
            to 1/2) in both X and Y, Y increases up and X increases to the
            right, quickscope handles "rescaling" when you change the
            window size */
      true, /* lines on */ 1.0F, 0, 0 /* RGB line color, 1 being max, 0 being min*/);

  /* All the objects that are created are managed by a QsApp.  The
   * creation of the sources and trace above created a QsApp
   * automatically. You run the scope by calling qsApp_main().
   * qsApp_main() will call gtk_main() for you, if you have not called
   * it yet.  There can be only one qsApp object. */
  qsApp_main();

  /* As this example shows, Quickscope tries to work with a minimum
   * of coding, by doing many things with reasonable defaults.
   * Quickscope also gives you lots of ways to override the defaults.
   * Clearly this being an oscilloscope there are lots of parameters you
   * can change.  You can change parameters on the fly by using a keyboard
   * interface.  With the scope in focus, type 'H' to print help to stdout.
   * Many more parameters changing interfaces are planned. */

  return 0; // 0 = success
}
