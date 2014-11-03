/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <sys/types.h>
#include <math.h>
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
#include "lorenz.h"

static int createCount = 0;

struct QsLorenz
{
  // inherit source
  struct QsSource s;
  struct QsRungeKutta4 *rk4;
  float x[3];

  float rate, sigma, rho, beta;
  int id;
};

static
void ODE(struct QsRungeKutta4 *rk4, long double t,
    const float *x, float *xdot, struct QsLorenz *l)
{
  xdot[0] = l->sigma * (x[1] - x[0]);
  xdot[1] = x[0] * (l->rho - x[2]) - x[1];
  xdot[2] = x[0] * x[1] - l->beta * x[2];
}

static
void cb_setParameters(struct QsLorenz *l)
{
  float max;
  max = l->sigma;
  if(max < l->rho)
    max = l->rho;
  if(max < l->beta)
    max = l->beta;
  
  qsRungeKutta4_setTStep(l->rk4, 1.0L/(max * 10.0L));

  qsSource_setMinSampleRate((struct QsSource *) l,
      (1.0F/qsRungeKutta4_getTStep(l->rk4)) * l->rate);
}

static
size_t iconText(char *buf, size_t len, struct QsLorenz *l)
{
  // some kind of colorful glyph for this source
  return snprintf(buf, len,
      "<span bgcolor=\"#CF46A5\" fgcolor=\"#97C81F\">["
      "<span fgcolor=\"#3F3A21\">Lorenz%d</span>"
      "]</span> ", l->id);
}

static
int cb_sourceRead(struct QsLorenz *l, long double tf,
    long double prevT, void *data)
{
  struct QsSource *s;
  s = (struct QsSource *) l;
  int nFrames;
  bool isMaster;
  isMaster = qsSource_isMaster(s);
  nFrames = qsSource_getRequestedSamples(s, tf, prevT);
  QS_ASSERT(nFrames >= 0);
  if(nFrames == 0) return 0;

  long double dt;
  dt = (tf - prevT)/nFrames;

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
        *t = (prevT += dt);
      qsRungeKutta4_go(l->rk4, l->x, *t++);
      frames[0] = l->x[0];
      frames[1] = l->x[1];
      frames[2] = l->x[2];
      frames += 3;
    }

    nFrames -= n;
  }
  return 1;
}

static
void _qsLorenz_destroy(struct QsLorenz *l)
{
  QS_ASSERT(l);
  QS_ASSERT(l->rk4);

  qsRungeKutta4_destroy(l->rk4);
}

struct QsSource *qsLorenz_create(int maxNumFrames,
    float rate/*play rate multiplier*/,
    float sigma, float rho, float beta,
    struct QsSource *group)
{
  struct QsLorenz *l;

  l = qsSource_create((QsSource_ReadFunc_t) cb_sourceRead,
      3/*numChannels*/, maxNumFrames, group, sizeof(*l));

  if(rate < 0)
    rate = 1;
  if(sigma < 0)
    sigma = 10;
  if(rho < 0)
    rho = 28;
  if(beta < 0)
    beta = 8.0F/3;

  l->rate = rate;
  l->sigma = sigma;
  l->rho = rho;
  l->beta = beta;
  l->x[0] = 0.19;
  l->x[1] = 0.23;
  l->x[2] = 0.37;
  l->id = createCount++;
  l->rk4 = qsRungeKutta4_create((QsRungeKutta4_ODE_t) ODE,
      l/*callback data*/,
      3/*dimensions*/, 0/*tStart*/, 0.1/*tStep*/,
      0/*object size*/);
  cb_setParameters(l);
  qsSource_addSubDestroy(l, _qsLorenz_destroy);

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) l;

  adjG = qsAdjusterGroup_start(adjL, "Lorenz");
  qsAdjuster_setIconStrFunc(adjG,
    (size_t (*)(char *, size_t, void *)) iconText, l);

  qsAdjusterFloat_create(adjL,
      "Rate", "Play speed", &l->rate,
      0.1F, /* min */ 100, /* max */
      (void (*) (void *)) cb_setParameters, l);
  qsAdjusterFloat_create(adjL,
      "Sigma", "Hz", &l->sigma,
      0.1F, /* min */ 100, /* max */
      (void (*) (void *)) cb_setParameters, l);
  qsAdjusterFloat_create(adjL,
      "Rho", "Hz", &l->rho,
      0.1F, /* min */ 100, /* max */
      (void (*) (void *)) cb_setParameters, l);
  qsAdjusterFloat_create(adjL,
      "Beta", "Hz", &l->beta,
      0.1F, /* min */ 100, /* max */
      (void (*) (void *)) cb_setParameters, l);

  qsAdjusterGroup_end(adjG);

  return (struct QsSource *) l;
}

