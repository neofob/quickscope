/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <quickscope.h>

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
  struct QsSource *l;

  l = qsLorenz_create( 5000 /* maxNumFrames */,
      3/*play rate multiplier*/,
      -1,-1,-1,/*sigma, rho, beta -1 == use default*/
      NULL/*projectionCallback*/, NULL/*projectionCallback_data*/,
      3/*numChannels, can be different if projectionCallback*/,
      NULL /* group */);

  qsSource_addChangeCallback(l,
      (bool (*)(struct QsSource *, void *)) SpewSource,
      qsIterator2_create(l, l, 1, 2));

  qsApp_main();

  qsApp_destroy();

  return 0;
}
