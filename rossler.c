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
#include "sourceParticular.h"

static int createCount = 0;

struct QsRossler
{
  // inherit source
  struct QsRK4Source s;
  float rate, a, b, c;
  int id;
};

static
void ODE(struct QsRungeKutta4 *rk4, long double t,
    const float *x, float *xdot, struct QsRossler *r)
{
  xdot[0] = - x[1] - x[2];
  xdot[1] = x[0] + r->a * x[1];
  xdot[2] = r->b + x[2] * (x[0] - r->c);
}

static
void cb_setParameters(struct QsRossler *r)
{
  float max;
  max = r->a;
  if(max < r->b)
    max = r->b;
  if(max < r->c)
    max = r->c;

  qsRK4Source_setTStep((struct QsRK4Source *) r,
          1.0L/(max * 8.0L));

  qsRK4Source_setPlayRate((struct QsRK4Source *) r,
      r->rate);
}

static
size_t iconText(char *buf, size_t len, struct QsRossler *r)
{
  // some kind of colorful glyph for this source
  return snprintf(buf, len,
      "<span bgcolor=\"#DF46A5\" fgcolor=\"#97C81F\">["
      "<span fgcolor=\"#3F3A21\">Rossler%d</span>"
      "]</span> ", r->id);
}

struct QsSource *qsRossler_create(int maxNumFrames,
    float rate/*play rate multiplier*/,
    float a, float b, float c,
    QsRK4Source_projectionFunc_t projectionCallback,
    void *cbdata,
    int numChannels/*number source channels written*/,
    struct QsSource *group)
{
  struct QsRossler *r;
  const float xInit[3] = { 5.0F, 5.32F, 0.02F };

  r = qsRK4Source_create(maxNumFrames, rate,
      (QsRungeKutta4_ODE_t) ODE, NULL/*ODE_data*/,
      0.1/*tStep will be reset*/,
      3/*ODE degrees of freedom*/,
      projectionCallback, cbdata,
      xInit,
      3/*numChannels*/, group, sizeof(*r));
  qsRK4Source_setODEData((struct QsRK4Source *)r, r);

  if(rate < 0)
    rate = 1;
  if(a < 0)
    a = 0.2F;
  if(b < 0)
    b = 0.2F;
  if(c < 0)
    c = 5.7F;

  r->rate = rate;
  r->a = a;
  r->b = b;
  r->c = c;
  r->id = createCount++;
  cb_setParameters(r);

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) r;

  adjG = qsAdjusterGroup_start(adjL, "Rossler");
  qsAdjuster_setIconStrFunc(adjG,
    (size_t (*)(char *, size_t, void *)) iconText, r);

  qsAdjusterFloat_create(adjL,
      "Rate", "Play speed", &r->rate,
      0.1F, /* min */ 100, /* max */
      (void (*) (void *)) cb_setParameters, r);
  qsAdjusterFloat_create(adjL,
      "a", "Hz", &r->a,
      0.05F, /* min */ 100, /* max */
      (void (*) (void *)) cb_setParameters, r);
  qsAdjusterFloat_create(adjL,
      "b", "Hz", &r->b,
      0.05F, /* min */ 100, /* max */
      (void (*) (void *)) cb_setParameters, r);
  qsAdjusterFloat_create(adjL,
      "c", "Hz", &r->c,
      0.1F, /* min */ 100, /* max */
      (void (*) (void *)) cb_setParameters, r);

  qsAdjusterGroup_end(adjG);

  return (struct QsSource *) r;
}
