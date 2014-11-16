/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include "../quickscope.h"


static bool
SpewSource(struct QsSource *s, struct QsIterator2 *it)
{
  static int count = 0;
  float x, y;
  long double t;

  while(qsIterator2_get(it, &x, &y, &t))
  {
    printf("%Lg %g %g\n", t, x, y);
    ++count;
  }

  if(count > 1000)
    qsApp_destroy();

  return true;
}


int main(int argc, char **argv)
{
  struct QsSource *s0, *s1;

  // master source
  s0 = qsSaw_create(200 /*maxNumFrames*/, 0.5/*amp*/,
      0.05/*period*/, 0.0/*periodShift*/,
      100/*samplesPerPeriod*/, NULL/*source group*/);

  // non-master source
  s1 = qsSaw_create(0 /*maxNumFrames*/, 0.3/*amp*/,
      0.03/*period*/, 0.1/*periodShift*/,
      5/*samplesPerPeriod*/, s0/*source group*/);

  qsSource_addChangeCallback(s1,
      (bool (*)(struct QsSource *, void *)) SpewSource,
      qsIterator2_create(s0, s1, 0/*channel0*/, 0/*channel1*/));

  qsApp_main();

  // qsApp_destroy();

  return 0;
}
