/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#include <time.h>
#include <string.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "timer_priv.h"


struct QsTimer
{
  struct timespec offset;
  struct timespec stop;
};


#ifdef CLOCK_MONOTONIC
#  define GETTIME(x)  clock_gettime(CLOCK_MONOTONIC, (x))
#else
#  define GETTIME(x)  clock_gettime(CLOCK_REALTIME, (x))
#endif

#ifdef QS_DEBUG
#  define TIME(x)  QS_ASSERT(GETTIME(x) == 0)
#else
#  define TIME(x)  GETTIME(x)
#endif


#ifdef TEST_THIS
static inline
void fixTimespec(struct timespec *t)
{
  if(t->tv_nsec > 1000000000L)
  {
    t->tv_nsec -= 1000000000L;
    ++t->tv_sec;
  }
  else if(t->tv_nsec < 0)
  {
    t->tv_nsec += 1000000000L;
    --t->tv_sec;
  }
}

void qsTimer_stop(struct QsTimer *timer)
{
  if(timer->stop.tv_sec) return;
  TIME(&timer->stop);
}

void qsTimer_start(struct QsTimer *timer)
{
  struct timespec t;
  if(!timer->stop.tv_sec) return;

  TIME(&t);
  
  timer->offset.tv_sec  += t.tv_sec  - timer->stop.tv_sec;
  timer->offset.tv_nsec += t.tv_nsec - timer->stop.tv_nsec;
  fixTimespec(&timer->offset);
  timer->stop.tv_sec = 0;
}

void qsTimer_set(struct QsTimer *timer, long double time)
{
  int64_t s, ns;
  s = time;
  ns = (((long double)(time - s))*1000000000.5L);

  if(timer->stop.tv_sec)
  {
    timer->offset.tv_sec = timer->stop.tv_sec - s;
    timer->offset.tv_nsec = timer->stop.tv_nsec - ns;
    fixTimespec(&timer->offset);
  }
  else
  {
    struct timespec t;
    TIME(&t);
    timer->offset.tv_sec = t.tv_sec - s;
    timer->offset.tv_nsec = t.tv_nsec - ns;
    fixTimespec(&timer->offset);
  }
}

void qsTimer_sync(struct QsTimer *to, struct QsTimer *timer)
{
  QS_ASSERT(to);
  QS_ASSERT(timer);
  memcpy(timer, to, sizeof(*timer));
}
#endif // #ifdef TEST_THIS

void _qsTimer_destroy(struct QsTimer *timer)
{
  QS_ASSERT(timer);
  g_free(timer);
}

struct QsTimer *_qsTimer_create(void)
{
  struct QsTimer *timer;
  timer = g_malloc0(sizeof(*timer));
  TIME(&timer->offset);
  return timer;
}


long double _qsTimer_get(struct QsTimer *timer)
{
  struct timespec t;
  if(timer->stop.tv_sec)
    return ((long double) timer->stop.tv_sec)- timer->offset.tv_sec +
        (timer->stop.tv_nsec - timer->offset.tv_nsec) * 0.000000001L;
  TIME(&t);
  return  ((long double) t.tv_sec)- timer->offset.tv_sec +
        (t.tv_nsec - timer->offset.tv_nsec) * 0.000000001L;
}

#ifdef TEST_THIS
/*
 * Test this code by running:

gcc -DTEST_THIS timer.c app.c -o x\
 `pkg-config --cflags gtk+-3.0`\
 `pkg-config --libs gtk+-3.0` &&\
 ./x | quickplot -P

*/
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
  struct QsTimer *t;
  int i, count = 0;
  long double t1, t2;
  t = qsTimer_create();
  usleep(1000);
  qsTimer_stop(t);
  for(i=0;i<20;++i)
  {
    printf("%d %.40Lg\n", count++, qsTimer_get(t));
  }
  qsTimer_start(t);
  for(i=0;i<20;++i)
  {
    usleep(1000);
    printf("%d %Lg\n", count++, qsTimer_get(t));
  }
  qsTimer_set(t, -10.01);
  for(i=0;i<20;++i)
  {
    usleep(1000);
    printf("%d %Lg\n", count++, qsTimer_get(t));
  } 
  qsTimer_set(t, 0);
  for(i=0;i<20;++i)
  {
    usleep(1000);
    printf("%d %Lg\n", count++, qsTimer_get(t));
  }
  qsTimer_set(t, 10.01);
  for(i=0;i<20;++i)
  {
    usleep(1000);
    printf("%d %Lg\n", count++, qsTimer_get(t));
  }
  return 0;
}
#endif
