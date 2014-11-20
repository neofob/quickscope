/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include "quickscope.h"


static bool
SpewSource(struct QsSource *s, struct QsIterator2 *it)
{
  float x, y;
  long double t;

  while(qsIterator2_get(it, &x, &y, &t))
    printf("%Lg %g %g\n", t, x, y);

  return true;
}


int main(int argc, char **argv)
{
  if(!argv[1])
  {
    fprintf(stderr, "Usage: %s SND_FILE\n", argv[0]);
    return 1;
  }

  struct QsSource *snd;

  snd = qsSoundFile_create(argv[1],
    10000/*scope frames per scope source read call*/,
    0, /* play back rate, 0 for same as file play rate*/
    NULL/*source group*/); 
  
  qsSource_addChangeCallback(snd,
      (bool (*)(struct QsSource *, void *)) SpewSource,
      qsIterator2_create(snd, snd, 0/*channel0*/,
        (qsSource_numChannels(snd) < 2) ? 0: 1/*channel1*/));

  qsApp_main();

  qsApp_destroy();

  return 0;
}
