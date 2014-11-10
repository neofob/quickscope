/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <quickscope.h>


int main(int argc, char **argv)
{
  struct QsSource *s;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = false;
  qsApp->op_fadePeriod = 10.0F;
  qsApp->op_fadeDelay =  5.0F;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;

  s = qsUrandom_create(6/*numChannels*/, 1000/*maxNumFrames*/,
      1000/*sampleRate Hz*/, NULL/*group*/);
  if(!s) return 1;

  qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      s, 0, s, 1, /* x/y source and channels */
      1.0F, 1.0F, 0, 0, /* xscale, yscale, xshift, yshift */
      false, /* lines */ 1, 0, 0 /* RGB line color */);
  qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      s, 2, s, 3, /* x/y source and channels */
      1.0F, 1.0F, 0, 0, /* xscale, yscale, xshift, yshift */
      false, /* lines */ 0, 1, 0 /* RGB line color */);
  qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      s, 4, s, 5, /* x/y source and channels */
      1.0F, 1.0F, 0, 0, /* xscale, yscale, xshift, yshift */
      false, /* lines */ 0, 0, 1 /* RGB line color */);
 
  qsApp_main();
  qsApp_destroy();

  return 0;
}
