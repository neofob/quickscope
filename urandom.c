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
#include "sourceTypes.h"

struct QsUrandom
{
  // inherit QsSource
  struct QsSource source;
  FILE *file;
  float sampleRate;
};

#define FILENAME  "/dev/urandom"

static int
cb_read(struct QsUrandom *s, long double tf, long double prevT, void *data)
{
  struct QsSource *source;
  source = (struct QsSource *) s;
  int nFrames;
  bool isMaster;
  isMaster = qsSource_isMaster(source);
 
  nFrames = qsSource_getRequestedSamples(source, tf, prevT);

  QS_ASSERT(nFrames >= 0);

  if(nFrames == 0) return 0;

  int numChannels;
  numChannels = qsSource_numChannels(source);

  long double dt;
  dt = (tf - prevT)/nFrames;

  while(nFrames)
  {
    uint32_t data[128];

    float *frames;
    long double *t;
    int i, n;
    if(nFrames > (128 - 128%numChannels)/numChannels)
      n = (128 - 128%numChannels)/numChannels;
    else
      n = nFrames;

    frames = qsSource_setFrames(source, &t, &n);
    if(fread(data, n * numChannels * 4, 1, s->file) != 1)
    {
      fprintf(stderr, "fread() failed to read \""FILENAME"\":"
          " errno=%d:%s\n", errno, strerror(errno));
      return -1; // error, destroy this source
    }

    for(i=0; i<n; ++i)
    {
      if(isMaster)
        *t++ = (prevT += dt);
      int j;
      for(j=0; j<numChannels; ++j)
      {
        // convert the random 32 bit int to a float in the interval
        // (0.5, -0.5]
        frames[numChannels*i + j] =
          ((float) data[numChannels*i + j]) / ((float) 0xFFFFFFFF) - 0.5F;
      }
    }

    nFrames -= n;
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

static
size_t iconText(char *buf, size_t len, void *data)
{
  // some kind of colorful glyph for this source
  return snprintf(buf, len,
      "<span bgcolor=\"#CF86F5\" fgcolor=\"#97C81F\">["
      "<span fgcolor=\"#3F3A21\">urandom</span>"
      "]</span> ");
}

static
void parameterChange(struct QsUrandom *s)
{
  // Another way to handle a parameter change is to
  // change other parameters too so that continuity is
  // preserved, but that may be considered misleading
  // or unexpected behavior.
  qsSource_addPenLift((struct QsSource *) s);

  /* We get a second change here to adjust parameters
   * here. */
  qsSource_setMinSampleRate((struct QsSource *) s,
      s->sampleRate);
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
  s->sampleRate = sampleRate;/*Hz*/
  qsSource_setMinSampleRate((struct QsSource *) s,
      s->sampleRate/*samples/second*/);
  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) s;
  adjG = qsAdjusterGroup_start(adjL, "Urandom");
  qsAdjuster_setIconStrFunc(adjG,
    (size_t (*)(char *, size_t, void *)) iconText, s);
  qsAdjusterFloat_create(adjL,
      "Samples Per Second", "Hz", &s->sampleRate,
      0.1, /* min */ 10000, /* max */
      (void (*) (void *)) parameterChange, s);
  qsAdjusterGroup_end(adjG);
  qsSource_addSubDestroy(s, _qsUrandom_destroy);

  return (struct QsSource *) s;
}
