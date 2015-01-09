/* This example happens to use the on the fly solution of ordinary
 * differential equations to get the values for the source.  It could use
 * any floats that can be gotten from a C function call.  Like floats
 * from reading a sound card or other device. */

#include <math.h> // defines M_PI
#include "quickscope.h"


struct MySource
{
  // We inherit QsSource
  struct QsSource source;
  // ODE solver
  struct QsRungeKutta4 *rk4;
  float omega2, period, alpha0, alpha, eMax, eMin;
  float x[2]; // x v
};

static
void mySource_ode(struct QsRungeKutta4 *rk4, long double t,
    const float *x, float *xdot, struct MySource *ms)
{
  /* ODE for sine wave with angular frequency of
   * sqrtf(ms->omega2) */

  xdot[0] = x[1];
  xdot[1] = - ms->omega2 * x[0] - ms->alpha * x[1];
}

static
int mySource_readCB(struct QsSource *s,
    long double tB, long double tBPrev, long double tCurrent,
    long double deltaT, int nFrames, bool underrun,
    /* rk4 is the user callback data */
    struct QsRungeKutta4 *rk4)
{
  if(nFrames == 0)
    // You might ask why did you call me in the first place?  If your
    // controller was not chosen well this can happen.  Or the scope user
    // messes with a parameter this can happen.  Some sources may just
    // need to know when an interval has popped so they can adjust it, or
    // whatever you like.  We keep it simple and do nothing if no data is
    // requested.
    return 0; // we set no data.

  float *x;
  struct MySource *ms;
  ms = (struct MySource *) s;
  x = ms->x;

  // We have been told to fill in nFrames values
  // into the quickscope circular buffer.
  while(nFrames)
  {
    int i, n;
    float *vals; // pointer for values to set
    long double *t; // pointer for time array

    n = nFrames;
    vals = qsSource_setFrames(s, &t, &n);
    /* now n is the number of values we can fill. */

    for(i=0; i<n; ++i)
    {
      if(deltaT)
        // We must set the time for this frame if deltaT is set.
        // In our example we will, but sometimes another
        // source in the source group will set the time.
        t[i] = (tCurrent += deltaT);

      // Calculate x[0], and x[1] from ODE.
      qsRungeKutta4_go(rk4, x, t[i]);

      /* We flip the damping to anti-damping and vise-versa,
       * depending on which we need. */
      float energy;
      energy = ms->omega2 * x[0]*x[0] + x[1]*x[1];
      energy *= 0.5F;

      if(energy < ms->eMin && ms->alpha > 0.0F)
      {
        ms->alpha = - ms->alpha0;
        fprintf(stderr, "changed damping to %g\n", ms->alpha);
      }
      else if(energy > ms->eMax && ms->alpha < 0.0F)\
      {
        ms->alpha = ms->alpha0;
        fprintf(stderr, "changed damping to %g\n", ms->alpha);
      }

      // fill this frame
      vals[i*2] = x[0];
      vals[i*2 + 1] = x[1];
    }

    nFrames -= n;
  }
 
  return 1; // we have data
}


int main(int argc, char **argv)
{
  struct MySource *ms;
  struct QsSource *s, *sweep;

  /* We'll make it read slowly just see we can play. */
  qsInterval_create(0.1F /* interval period seconds */);


  qsApp->op_fade = true; // fade on for sure
  qsApp->op_fadePeriod = 1.0F; // seconds
  qsApp->op_fadeDelay = 3.0F; // seconds

  ms = qsSource_create( (QsSource_ReadFunc_t) mySource_readCB,
      2 /* numChannels */,
      100 /* maxNumFrames  per cb_read() call */,
      NULL /* source Group NULL = create*/,
      sizeof(*ms));
  s = (struct QsSource*) ms;

  ms->period = 0.84F; /* period of the sin was in MySource in seconds */
  float omega;
  omega = 2*M_PI/ms->period;
  ms->omega2 = omega*omega;
  ms->alpha = 0.1; // damping
  ms->alpha0 = ms->alpha; // copy of original damping
  ms->x[0] = 0.49F;    // initial x
  ms->x[1] = 0.0F; // initial v
  // ms->eMax is a constant of the motion if not for the
  // damping.  We use it to play with the damping parameter
  // above.
  ms->eMax = ms->omega2*ms->x[0]*ms->x[0] + ms->x[1]*ms->x[1];
  ms->eMax *= 0.5; // so it looks like energy
  ms->eMin = ms->eMax/25.0F; // eMin has 1/5 the amplitude

  ms->rk4 = qsRungeKutta4_create(
    (QsRungeKutta4_ODE_t) mySource_ode /* equations defined above */,
    ms /* solver callback data passed as last arg of mySource_ode() */,
    2 /*dimensions, 2 equations*/, 0.0L /* start time */,
    0.02/* time step, must be much smaller than period of equations*/,
    0 /* size of object to allocate, 0 => default */);

  /* We get the rk4 object pointer passed to the source read callback at
   * each call.  Just to show that you can add user callback data, but
   * it's not a necessity in this case. */
  qsSource_setCallbackData(s, ms->rk4);

  /* We'll make a source that can vary the source sample rate.*/
  const float minMaxSampleRates[] = { 0.01F , 2*44100.0F };
  qsSource_setFrameRateType(s, QS_TOLERANT, minMaxSampleRates,
      50.0F/*Hz, requested frame sample rate, that's 50 points plotted per
             second */);

 sweep = qsSweep_create(4.0F /*sweep period, seconds*/,
      0.0F /*trigger level*/,
      0 /*slope may be  -1, 0, 1   0 => free run */,
      0.0F /*trigger hold off*/,
      0.0F /*delay after trigger, may be negative*/,
      s /*source that causes the triggering and is also the group */,
      0 /*source channel number, 0=first*/);

  /* create a trace */
  qsTrace_create(NULL /* QsWin, NULL to use a default Win */,
      sweep, 0, /*X-source, channel number*/
      s,   0, /*Y-source, channel number*/
      1.0F, omega/(2*sqrtf(2 * ms->eMax)),/* xscale, yscale*/
      0.0F, 0.0F, /* xshift, yshift */
      true, /* lines */ 1, 0, 1 /* RGB line color */);

  /* create another trace */
  qsTrace_create(NULL /* QsWin, NULL to use a default Win */,
      s, 0, /*X-source, channel number*/
      s, 1, /*Y-source, channel number*/
      /* We scale it based on the max energy that we have */
      omega/(2*sqrtf(2 * ms->eMax)),  1.0F/(2*sqrtf(2*ms->eMax)),
      0.0F, 0.0F, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 0, 1, 1 /* RGB line color */);

  qsApp_main();


  return 0;
}
