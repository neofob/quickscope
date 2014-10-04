/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <quickscope.h>


static gboolean
SpewSource(struct QsSource *s, struct QsIterator *it)
{
  static int count = 0;
  float x;
  long double t;

  while(qsIterator_get(it, &x, &t))
    printf("%Lg %g\n", t, x);

  if(++count > 10)
    qsApp_destroy();

  return TRUE;
}


int main(int argc, char **argv)
{
  struct QsSource *s;

  //qsApp_init(NULL, NULL);
  //qsApp->op_defaultIntervalPeriod = 0.4; // Seconds
  
  s = qsSaw_create(100 /*maxNumFrames*/, 1/*amp*/,
      0.05/*period*/, 0/*periodShift*/,
      5/*samplesPerSecond*/, NULL/*source group*/);

  qsSource_addChangeCallback(s,
      (gboolean (*)(struct QsSource *, void *)) SpewSource,
      qsIterator_create(s, 0));

  qsApp_main();

  // qsApp_destroy();

  return 0;
}