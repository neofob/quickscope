/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdbool.h>
#include <math.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "group.h"
#include "source.h"
#include "iterator.h"
#include "rungeKutta.h"
#include "sourceParticular.h"

struct QsUrandom
{
  // inherit QsSource
  struct QsSource source;
  FILE *file;
};

#define FILENAME  "/dev/urandom"

static int
cb_read(struct QsUrandom *s, long double tf,
    long double prevT, long double currentT,
    long double dt, int nFrames)
{
  struct QsSource *source;
  source = (struct QsSource *) s;

  if(nFrames == 0) return 0;

  int numChannels;
  numChannels = qsSource_numChannels(source);

  while(nFrames)
  {
    const size_t LEN = 1024;
    uint32_t data[1024];

    float *frames;
    long double *t;
    int i, n;

    if(nFrames > (LEN - LEN%numChannels)/numChannels)
      n = (LEN - LEN%numChannels)/numChannels;
    else
      n = nFrames;

    frames = qsSource_setFrames(source, &t, &n);

    nFrames -= n;

    if(fread(data, n * numChannels * 4, 1, s->file) != 1)
    {
      fprintf(stderr, "fread() failed to read \""FILENAME"\":"
          " errno=%d:%s\n", errno, strerror(errno));
      return -1; // error, destroy this source
    }

    for(i=0; i<n; ++i)
    {
      if(dt)
        *t++ = (currentT += dt);
   
      int j;
      for(j=0; j<numChannels; ++j)
      {
        // convert the random 32 bit int to a float in the interval
        // (0.5, -0.5]
        frames[numChannels*i + j] =
          ((float) data[numChannels*i + j]) / ((float) 0xFFFFFFFF) - 0.5F;
      }
    }
  }

  return 1;
}

static
void _qsUrandom_destroy(struct QsUrandom *u)
{
  QS_ASSERT(u);
  QS_ASSERT(u->file);

  fclose(u->file);
}


struct QsSource *qsUrandom_create(
    int numChannels, int maxNumFrames, float sampleRate,
    struct QsSource *group)
{
  struct QsUrandom *s;
  FILE *file;

  errno = 0;
  // TODO: this would likely be more efficient
  // using buffered steam FILE fopen() and fread().
  file = fopen(FILENAME, "r");
  if(!file)
  {
    fprintf(stderr, "fopen(\""FILENAME"\",):"
        " errno %d: %s\n", errno, strerror(errno));
    QS_ASSERT(0);
    return NULL;
  }

  s = qsSource_create((QsSource_ReadFunc_t) cb_read,
    numChannels, maxNumFrames, group, sizeof(*s));
  s->file = file;

  float minMaxSampleRates[] = { 0.01F , 100*44100.0F };
  qsSource_setFrameRateType((struct QsSource *) s, QS_TOLERANT, minMaxSampleRates,
      sampleRate/*default frame sample rate*/);

  qsSource_addSubDestroy(s, _qsUrandom_destroy);

  return (struct QsSource *) s;
}
