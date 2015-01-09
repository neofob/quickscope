extern // Declaring this void * make is easier to  use this to make
  // an inheriting object.  It's a GTK+ learned trick. 
void *qsSource_create(QsSource_ReadFunc_t read,
    int numChannels,  int maxNumFrames /* max num frames buffered */,
    // The maxNumFrames will only be set for the first source
    // made in the group (checked that it is the same in QS_DEBUG).
    // It is also the ring buffer length.
    const struct QsSource *group /* group=NULL to make a new group */,
    size_t objectSize);
/************************************************************
 *    QsSource types.  Inherit QsSource.
 ************************************************************/
extern
struct QsSource *qsUrandom_create(
    int numChannels, int maxNumFrames, float sampleRate,
    struct QsSource *group);
extern
struct QsSource *qsSin_create(int maxNumFrames,
    float amp, float period, float phaseShift, float samplesPerPeriod,
    struct QsSource *group);
extern
struct QsSource *qsSaw_create(int maxNumFrames,
    float amp, float period, float periodShift, float samplesPerPeriod,
    struct QsSource *group);

/* The RK4Source has more than on method */
struct QsRK4Source;

typedef void (*QsRK4Source_projectionFunc_t)(struct QsRK4Source *rk4s,
      const float *in, float *out, long double t, void *cbdata);

extern
struct QsSource *qsRossler_create(int maxNumFrames,
    float rate/*play rate multiplier*/,
    float sigma, float rho, float beta,
    QsRK4Source_projectionFunc_t projectionCallback,
    void *cbdata,
    struct QsSource *group);
extern
struct QsSource *qsLorenz_create(int maxNumFrames,
    float rate/*play rate multiplier*/,
    float sigma, float rho, float beta,
    QsRK4Source_projectionFunc_t projectionCallback,
    void *cbdata,
    struct QsSource *group);
extern
struct QsSource *qsAlsaCapture_create(int maxNumFrames, int sampleRate,
    struct QsSource *group);


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
extern
struct QsSource *qsSweep_create(
    float period, float level, int slope, float holdOff,
    float delay, struct QsSource *sourceIn, int channelNum);

static inline
void qsRK4Source_setODEData(struct QsRK4Source *rk4s, void *data)
{
  QS_ASSERT(rk4s);
  QS_ASSERT(rk4s->rk4);
  qsRungeKutta4_setODEData(rk4s->rk4, data);
}
static inline
long double qsRK4Source_getTStep(struct QsRK4Source *rk4s)
{
  QS_ASSERT(rk4s);
  QS_ASSERT(rk4s->rk4);

  return qsRungeKutta4_getTStep(rk4s->rk4);
}
static inline
void qsRK4Source_setTStep(struct QsRK4Source *rk4s, long double tStep)
{
  QS_ASSERT(rk4s);
  QS_ASSERT(rk4s->rk4);
  QS_ASSERT(tStep > 0);

  qsRungeKutta4_setTStep(rk4s->rk4, tStep);

  qsSource_setFrameSampleRate((struct QsSource *) rk4s,
      rk4s->rate/qsRungeKutta4_getTStep(rk4s->rk4));
}
static inline
void qsRK4Source_setPlayRate(struct QsRK4Source *rk4s, float rate)
{
  QS_ASSERT(rk4s);
  QS_ASSERT(rate > 0);

  rk4s->rate = rate;
  qsSource_setFrameSampleRate((struct QsSource *) rk4s,
      rk4s->rate/qsRungeKutta4_getTStep(rk4s->rk4));
}

