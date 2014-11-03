
#define RK4_TYPE  float

// Constant step size 4th order Runge Kutta for ODEs

typedef void (*QsRungeKutta4_ODE_t)(long double t, const RK4_TYPE *x,
    RK4_TYPE *xDot, void *data);

struct QsRungeKutta4
{
  int n; // dimensions
  long double t;
  long double tStep; // maximum time step
  QsRungeKutta4_ODE_t derivatives;
  RK4_TYPE *k1, *k2, *k3, *k4, *k_2, *k_3, *k_4;
#ifdef QS_DEBUG
  size_t size;
#endif
};

extern
struct QsRungeKutta4 *qsRungeKutta4_create(QsRungeKutta4_ODE_t derivatives,
    int n /*dimensions*/, long double t, long double tStep, size_t size);
extern
void qsRungeKutta4_destroy(struct QsRungeKutta4 *rk4);
extern
void qsRungeKutta4_go(struct QsRungeKutta4 *rk4, RK4_TYPE *x, long double to);
