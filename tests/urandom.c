/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <quickscope.h>

struct Urandom
{
  // inherit QsSource
  struct QsSource source;
  int fd;
  float sampleRate;
};

static inline
bool readn(int fd, char *buf, size_t n)
{
  size_t nrd = 0;

  while(nrd < n)
  {
    ssize_t rd;
    if((rd = read(fd, &buf[nrd], n - nrd)) < 1)
    {
      fprintf(stderr, "read(fd=%d,,%zu) failed to read 1 byte:"
          " %d: %s\n", fd, n - nrd, errno, strerror(errno));
      return true; // fail
    }
    nrd += rd;
  }
  return false; //success
}

static int
cb_read(struct Urandom *s, long double tf, long double prevT, void *data)
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
    if(readn(s->fd, (char *) data, n * numChannels * 4))
      return -1; // error, destroy this source

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
void _Urandom_destroy(struct Urandom *u)
{
  QS_ASSERT(u);
  QS_ASSERT(u->fd > -1);

  close(u->fd);
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
void parameterChange(struct Urandom *s)
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

static struct QsSource *Urandom_create(
    int numChannels, int maxNumFrames, float sampleRate,
    struct QsSource *group)
{
  struct Urandom *s;
  int fd;

  errno = 0;
  // TODO: this would likely be more efficient
  // using buffered steam FILE fopen() and fread().
  fd = open("/dev/urandom",O_RDONLY);
  if(fd == -1)
  {
    fprintf(stderr, "failed to open /dev/urandom:"
        " %d: %s\n", errno, strerror(errno));
    QS_ASSERT(0);
    return NULL;
  }

  s = qsSource_create((QsSource_ReadFunc_t) cb_read,
    numChannels, maxNumFrames, group, sizeof(*s));
  s->fd = fd;
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
  qsSource_addSubDestroy(s, _Urandom_destroy);

  return (struct QsSource *) s;
}


int main(int argc, char **argv)
{
  struct QsSource *s;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = false;
  qsApp->op_fadePeriod = 10.0F;
  qsApp->op_fadeDelay =  5.0F;
  qsApp->op_doubleBuffer = true;
  qsApp->op_grid = 0;

  s = Urandom_create(6/*numChannels*/, 1000/*maxNumFrames*/,
      1000/*sampleRate Hz*/, NULL/*group*/);
  if(!s) return 1;

  qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      s, 0, s, 1, /* x/y source and channels */
      1.0F, 1.0F, 0, 0, /* xscale, yscale, xshift, yshift */
      false, /* lines */ 1, 0, 0 /* RGB line color */);
  qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      s, 2, s, 3, /* x/y source and channels */
      1.0F, 1.0F, 0, 0, /* xscale, yscale, xshift, yshift */
      false, /* lines */ 0, 1, 0 /* RGB line color */);
    qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      s, 4, s, 5, /* x/y source and channels */
      1.0F, 1.0F, 0, 0, /* xscale, yscale, xshift, yshift */
      false, /* lines */ 0, 0, 1 /* RGB line color */);
 
  qsApp_main();
  qsApp_destroy();

  return 0;
}
