/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <math.h> // defines M_PI
#include <quickscope.h>

struct FreeRunSweep
{
  struct QsSource source;
  struct QsIterator *triggerIt;
  float period;
  long double start;
};


static
int cb_read(struct FreeRunSweep *s, long double tf, long double prevT)
{
  float x, period;
  long double t, start, *time;
  struct QsIterator *it;
  struct QsSource *source;
  it = s->triggerIt;
  start = s->start;
  period = s->period;
  source = (struct QsSource *)s;

  while(qsIterator_get(it, &x, &t))
  {
    float x_out;
    // first range from 0 to 1
    x_out = (t - start)/period;
    if(x_out >= 1.0F)
    {
      x_out = fmodf(x_out, 1.0F);
      start = t - x_out * period;
      // TODO: make this better
      qsSource_setFrame(source, &time)[0] = QS_LIFT;
    }
    else
      qsSource_setFrame(source, &time)[0] = x_out - 0.5F;
  }

  s->start = start;

  return 1;
}

static void
FreeRunSource_destroy(struct FreeRunSweep *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->triggerIt);
  qsIterator_destroy(s->triggerIt);
}

static
struct QsSource *
FreeRunSource_create(struct QsSource *y, float period)
{
  struct FreeRunSweep *s;
  s = qsSource_create((QsSource_ReadFunc_t) cb_read,
      1 /* numChannels */, 0 /* maxNumFrames */,
      y /* group */, sizeof(*s));
  s->triggerIt = qsIterator_create(y, 0 /*channel*/);
  s->period = period;
  ((struct QsSource *)s)->destroy = (void (*)(void *)) FreeRunSource_destroy;

  return (struct QsSource *) s;
}


int main(int argc, char **argv)
{
  struct QsSource *sinSource, *freeRunSource;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = true;
  qsApp->op_fadePeriod = 4.0F;
  qsApp->op_fadeDelay = 5.0F;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;

  sinSource = qsSin_create( 100 /* maxNumFrames */,
        0.4F /*amplitude*/, 1.2348F /*period*/,
        0.0F /*phaseShift*/, 35 /*samplesPerPeriod*/,
        NULL /* group */);

  freeRunSource = FreeRunSource_create(sinSource, 4 /*sweep period in seconds*/);

  qsTrace_create(NULL /* QsWin, NULL to make a default */,
      freeRunSource, 0, sinSource, 0, /* x/y source and channels */
      1.0F, 0.9F, 0, 0, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 0, 1, 0 /* RGB line color */);

  qsApp_main();

  qsApp_destroy();

  return 0;
}
