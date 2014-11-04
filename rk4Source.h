struct QsRK4Source;

typedef void (*QsRK4Source_projectionFunc_t)(struct QsRK4Source *rk4s,
      const float *in, float *out, long double t, void *cbdata);

struct QsRK4Source
{
  // inherit source
  struct QsSource s;
  struct QsRungeKutta4 *rk4;
  QsRK4Source_projectionFunc_t projectionCallback;
  void *cbdata;
  long double lastT;
  float *x;
  float rate;
  int dof;
};


extern
void *qsRK4Source_create(int maxNumFrames,
    float rate/*play rate multiplier.
      rate=2 integrates ODE 2 times faster then real-time.*/,
    QsRungeKutta4_ODE_t ODE, void *ODE_data,
    long double tStep,
    int dof/*ODE number degrees of freedom*/,
    QsRK4Source_projectionFunc_t projectionCallback,
    void *cbdata,
    const float *xInit/*initial conditions*/,
    int numChannels/*number source channels written*/,
    /* The channels are from the first numChannels degrees of freedom
     * if projectionCallback is NULL, giving numChannels <= dof */
    struct QsSource *group, size_t size);

static inline
void qsRK4Source_setODEData(struct QsRK4Source *rk4s, void *data)
{
  QS_ASSERT(rk4s);
  QS_ASSERT(rk4s->rk4);
  qsRungeKutta4_setODEData(rk4s->rk4, data);
}
static inline
void qsRK4Source_setTStep(struct QsRK4Source *rk4s, long double tStep)
{
  QS_ASSERT(rk4s);
  QS_ASSERT(rk4s->rk4);

  qsRungeKutta4_setTStep(rk4s->rk4, tStep);
  qsSource_setMinSampleRate((struct QsSource *) rk4s,
      (rk4s->rate/qsRungeKutta4_getTStep(rk4s->rk4)));
}
static inline
long double qsRK4Source_getTStep(struct QsRK4Source *rk4s)
{
  QS_ASSERT(rk4s);
  QS_ASSERT(rk4s->rk4);

  return qsRungeKutta4_getTStep(rk4s->rk4);
}
static inline
void qsRK4Source_setPlayRate(struct QsRK4Source *rk4s, float rate)
{
  QS_ASSERT(rk4s);
  rk4s->rate = rate;
  qsSource_setMinSampleRate((struct QsSource *) rk4s,
      (rk4s->rate/qsRungeKutta4_getTStep(rk4s->rk4)));
}
