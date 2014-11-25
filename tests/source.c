/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <math.h> // defines M_PI
#include "quickscope.h"

static bool
SpewSource(struct QsSource *s, void *data)
{
  static struct QsIterator *it = NULL;
  if(!it)
    it = qsIterator_create(s, 0);

  float x;
  long double t;

  while(qsIterator_get(it, &x, &t))
    printf("%Lg %g\n", t, x);

  return true;
}


static
int cb_read(struct QsSource *s, long double tB,
    long double tBPrev, long double tCurrent,
    long double dt, int nFrames, bool runderrun,
    void *data)
{
  static int totalFrames = 0;
  static bool setRate = false;

  if(nFrames == 0)
  {
    return 0;
  }

  while(nFrames)
  {
    int i, n;
    float *vals;
    long double *t;

    n = nFrames;
    vals = qsSource_setFrames(s, &t, &n);

    for(i=0; i<n; ++i)
    {
      vals[i] = ++totalFrames;
      if(dt)
        t[i] = (tCurrent += dt);
    }

    nFrames -= n;
  }

  totalFrames += nFrames;

  if(tB > 0.04 && !setRate)
  {
    setRate = true;
    qsSource_setFrameRate(s, 50020);
  }
  else if(tB > 0.07)
    qsSource_destroy(s);

  return 1;
}


int main(int argc, char **argv)
{
  struct QsSource *s;

  qsIdle_create();

  s = qsSource_create( cb_read, 1 /* numChannels */,
      10 /* maxNumFrames */, NULL, 0);

  const float rates[] = { 70000, 9900, 20, 30, 6000, 30000, 50000 };
  qsSource_setFrameRateType(s, QS_SELECTABLE, rates, 30002);

  qsSource_addChangeCallback(s, SpewSource, NULL);

  qsApp_main();

  // qsApp_destroy();

  return 0;
}
