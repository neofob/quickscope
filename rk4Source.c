/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <sys/types.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "group.h"
#include "source.h"
#include "rungeKutta.h"
#include "rk4Source.h"


static
int cb_sourceRead(struct QsRK4Source *rk4s, long double tf,
    long double prevT, void *data)
{
  struct QsSource *s;
  s = (struct QsSource *) rk4s;
  int nFrames, numChannels;
  numChannels = qsSource_numChannels(s);
  QS_ASSERT(numChannels <= rk4s->dof);
  bool isMaster;
  isMaster = qsSource_isMaster(s);
  nFrames = qsSource_getRequestedSamples(s, tf, prevT);
  QS_ASSERT(nFrames >= 0);
  if(nFrames == 0) return 0;

  long double dt, lt;
  float rate;
  rate = rk4s->rate;
  dt = (tf - prevT)/nFrames;
  lt = rk4s->lastT;

  while(nFrames)
  {
    float *frames;
    long double *t;
    int i, n;
    n = nFrames;
    frames = qsSource_setFrames(s, &t, &n);

    for(i=0; i<n; ++i)
    {
      if(isMaster)
        // TODO:
        // We are assuming that time changes at a uniform rate,
        // but that may not be true if we are not the master.
        *t = (prevT += dt);
      ++t;
      // ODE system time changes at a scaled rate.
      // lt += (dt * rate);
      lt += dt * rate;
      qsRungeKutta4_go(rk4s->rk4, rk4s->x, lt);

      if(rk4s->projectionCallback)
      {
        rk4s->projectionCallback(rk4s, rk4s->x, frames, lt, rk4s->cbdata);
        frames += numChannels;
      }
      else
      {
        int j;
        for(j=0; j<numChannels; ++j)
          *frames++ = rk4s->x[j];
      }
    }

    nFrames -= n;
  }

  rk4s->lastT = lt;
  
  return 1;
}


static
void _qsRK4Source_destroy(struct QsRK4Source *rk4s)
{
  QS_ASSERT(rk4s);
  QS_ASSERT(rk4s->rk4);

#ifdef QS_DEBUG
  memset(rk4s->x, 0, sizeof(float)*rk4s->dof);
#endif
  g_free(rk4s->x);

  qsRungeKutta4_destroy(rk4s->rk4);
}

void *qsRK4Source_create(int maxNumFrames,
    float rate/*play rate multiplier.  rate=2 integrates ODE 2 times faster.*/,
    QsRungeKutta4_ODE_t ODE, void *ODE_data, long double tStep,
    int dof/*ODE number degrees of freedom*/,
    QsRK4Source_projectionFunc_t projectionCallback,
    void *cbdata,
    const float *xInit,
    int numChannels/*number source channels written*/,
    /* The channels are from the first numChannels degrees of freedom
     * if projectionCallback is NULL, giving numChannels <= dof */
    struct QsSource *group, size_t size)
{
  QS_ASSERT(projectionCallback || dof >= numChannels);
  QS_ASSERT(ODE);

  struct QsRK4Source *rk4s;

  if(size < sizeof(*rk4s))
    size = sizeof(*rk4s);

  rk4s = qsSource_create((QsSource_ReadFunc_t) cb_sourceRead,
      numChannels, maxNumFrames, group, size);

  rk4s->rk4 = qsRungeKutta4_create(ODE, ODE_data/*callback data*/,
      dof, 0/*tStart*/, tStep, 0/*object size*/);

  rk4s->projectionCallback = projectionCallback;
  rk4s->cbdata = cbdata;
  rk4s->rate = rate;
  rk4s->dof = dof;
  rk4s->x = g_malloc0(sizeof(float)*dof);
  memcpy(rk4s->x, xInit, sizeof(float)*dof);

  qsSource_addSubDestroy(rk4s, _qsRK4Source_destroy);

return rk4s;
}

