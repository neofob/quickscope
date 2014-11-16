/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <sys/types.h>
#include <math.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "group.h"
#include "source.h"
#include "rungeKutta.h"
#include "sourceTypes.h"

static int createCount = 0;

struct QsLorenz
{
  // inherit source
  struct QsRK4Source s;
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

  qsRK4Source_setTStep((struct QsRK4Source *) l,
          1.0L/(max * 5.0L));

  qsRK4Source_setPlayRate((struct QsRK4Source *) l,
      l->rate);
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

struct QsSource *qsLorenz_create(int maxNumFrames,
    float rate/*play rate multiplier*/,
    float sigma, float rho, float beta,
    QsRK4Source_projectionFunc_t projectionCallback,
    void *cbdata,
    int numChannels/*number source channels written*/,
    struct QsSource *group)
{
  struct QsLorenz *l;
  const float xInit[3] = { 0.2F, 0.32F, 0.37F };

  l = qsRK4Source_create(maxNumFrames, rate,
      (QsRungeKutta4_ODE_t) ODE, NULL/*ODE_data*/,
      0.1/*tStep will be reset*/,
      3/*ODE degrees of freedom*/,
      projectionCallback, cbdata,
      xInit,
      3/*numChannels*/, group, sizeof(*l));
  qsRK4Source_setODEData((struct QsRK4Source *)l, l);

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
  l->id = createCount++;
  cb_setParameters(l);

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
