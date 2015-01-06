/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2015  Lance Arsenault
 * GNU General Public License version 3
 */

#include <quickscope.h>

int main(int argc, char **argv)
{
  struct QsSource *saw, *sweep;

  saw = qsSaw_create( 500 /*maxNumFrames*/,
        0.45F /*amplitude*/, 0.061F /*period*/,
        0.0F /*relative period shift = -1 to 1*/,
        40 /*samples-per-period*/,
        NULL /* source group, NULL => create one */);

  sweep = qsSweep_create(0.2F /*sweep period*/,
      0.0F /*trigger level*/,
      1 /*slope may be  -1, 0, 1   0 => free run */,
      0.3F /*trigger hold off*/,
      0.0F /*delay after trigger, may be negative*/,
      saw /*source group: is from saw QsSource above */,
      0 /*source channel number, 0=first*/);

  qsTrace_create(NULL,
      sweep, 0, /*X-source, channel number*/
      saw,   0, /*Y-source, channel number*/
      1.0F, 1.0F, 0.0F, 0.0F, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 1, 0, 0 /* RGB line color */);

  qsApp_main();

  return 0;
}
