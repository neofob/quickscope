/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include "../quickscope.h"

int main(int argc, char **argv)
{
  char *filename = NULL;
  
  {
    char **arg;
    arg = argv;
    ++arg;
    while(*arg && **arg == '-')
      ++arg;
    if(*arg)
      filename = *arg;
  }

  if(!filename)
  {
    fprintf(stderr, "Usage: %s SND_FILE [SND_FILE ...]\n",
        argv[0]);
    return 1;
  }

  struct QsSource *snd, *sweep;
  struct QsTrace *trace;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = qsApp_bool("fade", false);
  qsApp->op_fadePeriod = 0.04F;
  qsApp->op_fadeDelay =  0.04F;
  qsApp->op_doubleBuffer = true;
  qsApp->op_width = 1200;
  qsApp->op_height = 400;

  qsApp->op_grid = 0;


  snd = qsSoundFile_create(filename, 20000/*maxNumFrames*/,
        0.0F/*sampleRate Hz*/, NULL/*source Group*/);
  if(!snd)
  {
    fprintf(stderr, "qsSoundFile_create(\"%s\",,) failed",
        filename);
    return 1;
  }

  sweep = qsSweep_create(0.01F /*period*/,
      qsApp_float("level", 0.0F),
      qsApp_int("slope", 1),
      qsApp_float("holdoff", 0.0F),
      qsApp_float("delay", 0.0F),
      snd/*source*/, 0/*sourceChannelNum*/);

  int numChannels;
  numChannels = qsSource_numChannels(snd);

  while(numChannels)
  {
    int r, g, b;
    r = (numChannels & 01)?1:0;
    g = (numChannels & 02)?1:0;
    b = (numChannels & 04)?1:0;
    trace = qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      sweep, 0, snd, --numChannels, /* x/y source and channels */
      1.0F, 0.8F, 0, 0, /* xscale, yscale, xshift, yshift */
      true, /* lines */ r, g, b /* RGB line color */);
    qsTrace_setSwipeX(trace, qsApp_bool("swipe", true));
  }

  qsApp_main();
  qsApp_destroy();

  return 0;
}
