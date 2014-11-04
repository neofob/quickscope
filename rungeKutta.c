/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include <debug.h>
#include <assert.h>
#include <rungeKutta.h>


struct QsRungeKutta4 *qsRungeKutta4_create(
    QsRungeKutta4_ODE_t derivatives,
    void *data,
    int n /*dimensions*/, long double t,
    long double tStep, size_t size)
{
  QS_ASSERT(derivatives);
  QS_ASSERT(n > 0);
  QS_ASSERT(tStep > 0);

  struct QsRungeKutta4 *rk4;
  if(size < sizeof(*rk4))
    size = sizeof(*rk4);

  rk4 = g_malloc0(size);
#ifdef QS_DEBUG
  rk4->size = size;
#endif
  rk4->n = n;
  rk4->derivatives = derivatives;
  rk4->t = t;
  rk4->tStep = tStep;
  rk4->k1 = g_malloc0(sizeof(RK4_TYPE)*n*7);
  rk4->k2 = rk4->k1 + n;
  rk4->k3 = rk4->k2 + n;
  rk4->k4 = rk4->k3 + n;
  rk4->k_2 = rk4->k4 + n;
  rk4->k_3 = rk4->k_2 + n;
  rk4->k_4 = rk4->k_3 + n;
  rk4->data = data;
  return rk4;
}

void qsRungeKutta4_destroy(struct QsRungeKutta4 *rk4)
{
  QS_ASSERT(rk4);

#ifdef QS_DEBUG
  memset(rk4->k1, 0, sizeof(RK4_TYPE)*7*rk4->n);
#endif
  g_free(rk4->k1);

#ifdef QS_DEBUG
  memset(rk4, 0, rk4->size);
#endif
  g_free(rk4);
}

void qsRungeKutta4_go(struct QsRungeKutta4 *rk4,
    RK4_TYPE *x, long double to)
{
  QS_ASSERT(rk4);
  QS_ASSERT(rk4->t < to);

  long double t, dt;
  dt = rk4->tStep;
  t = rk4->t;
  int n;
  n = rk4->n;
  bool running = true;
  void *data;
  data = rk4->data;

  while(running)
  {
    if(t + dt > to)
    {
      dt = to - t;
      running = false;
    }

    int i;

    rk4->derivatives(rk4, t, x, rk4->k1, data);
    for(i=0; i<n; ++i)
    {
      rk4->k1[i] *= dt;
      rk4->k_2[i] = x[i] + rk4->k1[i]/2;
    }

    rk4->derivatives(rk4, t + dt/2, rk4->k_2, rk4->k2, data);
    for(i=0; i<n; ++i)
    {
      rk4->k2[i] *= dt;
      rk4->k_3[i] = x[i] + rk4->k2[i]/2;
    }

    rk4->derivatives(rk4, t + dt/2, rk4->k_3, rk4->k3, data);
    for(i=0; i<n; ++i)
    {
      rk4->k3[i] *= dt;
      rk4->k_4[i] = x[i] + rk4->k3[i];
    }

    rk4->derivatives(rk4, t + dt, rk4->k_4, rk4->k4, data);
    for(i=0; i<n; ++i)
    {
      rk4->k4[i] *= dt;
      x[i] += (rk4->k1[i] + 2*rk4->k2[i] + 2*rk4->k3[i] + rk4->k4[i])/6;
    }

    t += dt;
  }

  rk4->t = to;
}
