/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
/* This is just about the simplest libquickscope example
 * that does something reasonable. */

#include <quickscope.h>

int main(int argc, char **argv)
{
  struct QsSource *sin, *sweep;

  /* create a source that has a single channel
   * that is a sine signal. */
  sin = qsSin_create( 500 /*maxNumFrames*/,
        /* maxNumFrame is very important:
         * it's the size of the ring buffers
         * for all sources in a source group.
         * If this is too small you may miss seeing
         * some of the data that is created in the
         * QsSource in a given read cycle. */
        0.45F /*amplitude*/, 0.061F /*period*/,
        0.0F*M_PI /*phase shift*/,
        40 /*samples-per-period*/,
        NULL /* source group, NULL => create one */);

  /* create a source that uses the sin source to
   * make a sweep that we will use as the X input
   * into the trace. */
  sweep = qsSweep_create(0.2F /*sweep period*/,
      0.0F /*trigger level*/,
      1 /*slope may be  -1, 0, 1   0 => free run */,
      0.3F /*trigger hold off*/,
      0.0F /*delay after trigger, may be negative*/,
      sin /*source group: is from sin QsSource above */,
      0 /*source channel number, 0=first*/);

  /* create a trace */
  qsTrace_create(NULL /* QsWin, NULL to use a default Win */,
      /* This will automatically make a QsWin. QsTraces do
       * not exist without a QsWin. */
      sweep, 0, /*X-source, channel number*/
      sin,   0, /*Y-source, channel number*/
      1.0F, 1.0F, 0.0F, 0.0F, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 1, 0, 0 /* RGB line color */);

  /* All the objects that are created are managed by a QsApp.
   * The creation of the sources and trace above created a QsApp
   * automatically. You run the scope by calling qsApp_main(). */
  qsApp_main();

  /* As this example shows, Quickscope tries to work with a minimum
   * of coding, by doing many things with reasonable defaults.
   * Quickscope also gives you lots of ways to override the defaults. */

  return 0;
}
