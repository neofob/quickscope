/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <sndfile.h>
#include "debug.h"
#include "assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "group.h"
#include "source.h"
#include "iterator.h"
#include "soundFile.h"

// TODO: clearly this is not thread safe
static int createCount = 0;


struct QsSoundFile
{
  // inherit QsSource
  struct QsSource source;
  SNDFILE *sf;
  float sampleRate; /*Hz*/
  int id;
  int64_t frames; // frames left to read
};

static inline
int sfRead(SNDFILE *sf, float *values, int64_t n, int numChannels)
{
  const int BLOCK = 512;

  while(n)
  {
    int rd, req;
    req = BLOCK;
    if(req > n)
      req = n;
    rd = sf_readf_float(sf, values, req);
    QS_VASSERT(rd >= 1, "sf_readf_float(,,%d)=%d failed\n", req, rd);
    if(rd < 1) return 1; // fail
    n -= rd;

#if 0
    {
      int i;
      for(i=0; i<rd; ++i)
        fprintf(stderr, "%g ", *(values + 2*i));
    }
#endif

    values += numChannels*rd;
  }
  return 0; // success
}

static
int cb_sound_read(struct QsSoundFile *snd,
    long double tf, long double prevT, void *data)
{
  if(snd->frames == 0)
    // We are done tell the caller
    return -1; // destroy this source

  struct QsSource *s;
  s = (struct QsSource *) snd;
  int nFrames;
  QS_ASSERT(s);
  bool isMaster;
  isMaster = qsSource_isMaster(s);

  // We number of frames we write depends on the
  // maximum of all sources minimum sample rate,
  // from all sources in the source group, that was
  // set with qsSource_setMinSampleRate() in each
  // source create function or other function.
  nFrames = qsSource_getRequestedSamples(s, tf, prevT);

  if(nFrames == 0) return 0;

  long double dt;
  dt = (tf - prevT)/nFrames;

  //fprintf(stderr, "%"PRId64" ", snd->frames);
 
  if(nFrames > snd->frames)
  {
    // We use less frames than we can
    // because the file is running out
    // of data.
    nFrames = snd->frames;
    snd->frames = 0;
  }
  else
    snd->frames -= nFrames;

  int numChannels;
  numChannels = qsSource_numChannels(s);

  while(nFrames)
  {
    float *values;
    long double *t;
    int n;
    n = nFrames;
    values = qsSource_setFrames(s, &t, &n);
    QS_ASSERT(values);
    QS_ASSERT(n>0);
    if(sfRead(snd->sf, values, n, numChannels))
      return -1; // destroy this source
    if(isMaster)
    {
      // Only the master source can write the time stamps
      long double *tend;
      tend = t + n;
      while(t != tend)
      {
        *t = (prevT += dt);
        ++t;
      }
    }
    nFrames -= n;
  }
  return 1;
}

static
size_t iconText(char *buf, size_t len, struct QsSoundFile *snd)
{
  // TODO: use filename in icon
  // some kind of stupid glyph for this sound file
  return snprintf(buf, len,
      "<span bgcolor=\"#FF5645\" fgcolor=\"#A7C81F\">["
      "<span fgcolor=\"#7F3A21\">sound%d</span>"
      "]</span> ", snd->id);
}

static inline
void addIcon(struct QsAdjuster *adj, struct QsSoundFile *snd)
{
  qsAdjuster_setIconStrFunc(adj,
      (size_t (*)(char *, size_t, void *)) iconText, snd);
}

static void
_qsSoundFile_destroy(struct QsSoundFile *snd)
{
  QS_ASSERT(snd);
  QS_ASSERT(snd->sf);
  sf_close(snd->sf);
  snd->sf = NULL;

  // qsSource_checkBaseDestroy(s)
  // Is not needed since user cannot call this
  // static function.  qsSource_destroy() will
  // call _qsSoundFile_destroy().
}

static
void cb_sampleRate(struct QsSoundFile *sf)
{
  qsSource_setMinSampleRate((struct QsSource *) sf,
      sf->sampleRate);
}

struct QsSource *qsSoundFile_create(const char *filename,
    int maxNumFrames,
    float sampleRate,
    const struct QsSource *group)
{
  QS_ASSERT(filename);
  QS_ASSERT(filename[0]);
  SNDFILE *sf;
  SF_INFO info;
  errno = 0;

  if(strcmp(filename, "-") == 0)
    sf = sf_open_fd(STDIN_FILENO, SFM_READ, &info, false/*close it*/);
  else
    sf = sf_open(filename, SFM_READ, &info);

  if(!sf)
  {
    QS_VASSERT(0,
        "libsndfile failed to open sound file: %s: %s\n",
        filename, strerror(errno));
    return NULL;
  }

  QS_ASSERT(info.channels >= 1 && info.channels <= 12);
  QS_ASSERT(info.frames > 0);
  QS_ASSERT(info.samplerate > 0);

  if(maxNumFrames == 0)
    maxNumFrames = 1000;
  if(sampleRate <= 0.0F)
    sampleRate = info.samplerate;

  float maxRate;
  maxRate = info.samplerate;
  if(maxRate < sampleRate)
    maxRate = sampleRate;

  struct QsSoundFile *snd;
  snd = qsSource_create(
      (QsSource_ReadFunc_t) cb_sound_read,
      info.channels, maxNumFrames,
      group /*source group*/, sizeof(*snd));

  snd->sf = sf;
  snd->sampleRate = sampleRate;
  snd->id = createCount++;
  snd->frames = info.frames;

  qsSource_setMinSampleRate((struct QsSource *) snd,
    snd->sampleRate);

  // A QsSoundFile is a QsSource is an QsAdjusterList.
  // Ain't inheritance fun!!

  struct QsAdjuster *adjG;
  struct QsAdjusterList *adjL;
  adjL = (struct QsAdjusterList *) snd;

  addIcon(adjG = qsAdjusterGroup_start(adjL, "sound"), snd);

  addIcon(
      qsAdjusterFloat_create(adjL,
      "Rate", "Hz", &snd->sampleRate,
      0.01, /* min */ maxRate, /* max */
      (void (*)(void *)) cb_sampleRate, snd), snd);

  qsAdjusterGroup_end(adjG);

  qsSource_addSubDestroy(snd, _qsSoundFile_destroy);

  return (struct QsSource *) snd;
}

