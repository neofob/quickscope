/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include "quickscope.h"

struct Sin
{
  struct QsRungeKutta4 rk4;
  float omega_2;
};

void f(const struct Sin *p, long double t,
    const float *x, float *xdot, void *data)
{
  xdot[0] = x[1];
  xdot[1] = - p->omega_2 * x[0];
  xdot[2] = t;
}

int main(int argc, char **argv)
{
  struct QsRungeKutta4 *solver;

  solver = qsRungeKutta4_create(
      (QsRungeKutta4_ODE_t) f, NULL/*cb_data*/,
      3/*dimensions*/,
      0/*start time*/, 0.001/*time step*/, sizeof(*solver));
  float w2 = ((struct Sin *)solver)->omega_2 = 1.0F;
  
  long double t, dt = 0.1;
  float x[3] = { 0, 1, 0 };

  //printf("t x v 1-w2*x*x-v*v\n");

  for(t = dt; t < 100; t += dt)
  {
    qsRungeKutta4_go(solver, x, t);
    printf("%Lg %13.13g %13.13g %13.13g %13.13g\n", t,
        x[0], x[1],
        // Should be constant like energy.
        1.0F - w2*x[0]*x[0] - x[1]*x[1],
        x[2] - ((float)( t*t/2)));
  }

  qsRungeKutta4_destroy(solver);

  return 0;
}
