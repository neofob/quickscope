/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <quickscope.h>

struct Sin
{
  struct QsRungeKutta4 rk4;
  float omega_2;
};

void f(long double t, const float *x, float *xdot,
    const struct Sin *p)
{
  xdot[0] = x[1];
  xdot[1] = - p->omega_2 * x[0];
}

int main(int argc, char **argv)
{
  struct QsRungeKutta4 *solver;

  solver = qsRungeKutta4_create(
      (QsRungeKutta4_ODE_t) f, 2/*dimensions*/,
      0/*start time*/, 0.01/*time step*/, sizeof(*solver));
  float w2 = ((struct Sin *)solver)->omega_2 = 1.0F;
  
  long double t, dt = 0.1;
  float x[2] = { 0, 1 };

  //printf("t x v 1-w2*x*x-v*v\n");

  for(t = dt; t < 100; t += dt)
  {
    qsRungeKutta4_go(solver, x, t);
    printf("%Lg %13.13g %13.13g %13.13g\n", t,
        x[0], x[1],
        // Should be constant like energy.
        1.0F - w2*x[0]*x[0] - x[1]*x[1]);
  }

  qsRungeKutta4_destroy(solver);

  return 0;
}
