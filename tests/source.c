/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <math.h> // defines M_PI
#include <quickscope.h>


static void
SpewSource(struct QsSource *s)
{
  static struct QsIterator *it = NULL;
  if(!it)
    it = qsIterator_create(s, 0);

  float x;
  long double t;

  while(qsIterator_get(it, &x, &t))
    printf("%Lg %g\n", t, x);
}


static
int cb_read(struct QsSource *s, long double tf, long double prevT, void *data)
{
  static float value = 0;
  int frames = 3;
  long double dt;
  dt = (tf - prevT)/frames;
  
  while(frames)
  {
    int i, n;
    float *vals;
    long double *t;

    n = frames;
    vals = qsSource_setFrames(s, &t, &n);

    for(i=0; i<n; ++i)
    {
      vals[i] = (value += 0.5);
      t[i] = (prevT += dt);
    }
    frames -= n;
  }

  SpewSource(s);

  if(value > 25)
  {
    // qsApp_destroy() knows what to do from here.
    // It calls itself again after gtk_main()
    // returns.
    qsApp_destroy();
    return -1; // destroy this source
  }

  return 1;
}


int main(int argc, char **argv)
{
  //qsApp_init(NULL, NULL);
  //qsApp->op_defaultIntervalPeriod = 0.4; // Seconds
  
  qsSource_create( cb_read, 1 /* numChannels */,
      10 /* maxNumFrames */, NULL, 0);

  qsApp_main();

  // qsApp_destroy();

  return 0;
}
