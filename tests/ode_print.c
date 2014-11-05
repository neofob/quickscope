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

  //if(count > 1000)
    //qsApp_destroy();

  return true;
}


int main(int argc, char **argv)
{
  struct QsSource *l;
  struct QsSource *(*attractor_create)(int maxNumFrames,
    float rate/*play rate multiplier*/,
    float sigma, float rho, float beta,
    QsRK4Source_projectionFunc_t projectionCallback,
    void *cbdata,
    int numChannels/*number source channels written*/,
    struct QsSource *group) = qsLorenz_create;

  qsApp_init(&argc, &argv);

  if(strcmp(qsApp_string("system", "lorenz"), "rossler") == 0)
    attractor_create = qsRossler_create;


  l = attractor_create( 5000 /* maxNumFrames */,
      3/*play rate multiplier*/,
      -1,-1,-1,/*sigma, rho, beta -1 == use default*/
      NULL/*projectionCallback*/, NULL/*projectionCallback_data*/,
      3/*numChannels, can be different if projectionCallback*/,
      NULL /* group */);

  qsSource_addChangeCallback(l,
      (bool (*)(struct QsSource *, void *)) SpewSource,
      qsIterator2_create(l, l, 0, 1));

  qsApp_main();

  qsApp_destroy();

  return 0;
}
