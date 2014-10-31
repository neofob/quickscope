/* QsIterator and QsIterator2 are "C-Classes"
 * for reading (not writing) the source frame ring-buffers.
 */

/* Special float value NAN is from math.h
 * and the compiler.  NAN and INFINITY work as float, double,
 * and long double.  It's magic. */


// A friend of QsSource.
// reads 1 channel from a Source
struct QsIterator
{
  struct QsSource *source;

  int i, // The last read index to source frame buffer
    wrapCount, // to detect when we read slower than the writer
    channel; // channel number to read
#ifdef QS_DEBUG
  long double lastT; // Make sure time always increases
#endif
};

struct QsIterator2
{
  struct QsSource *source0, *source1;

  int i0, i1, // The last read index to source frame buffer
    wrapCount, // to detect when we read slower than the writer
    channel0, channel1; // channel number to read
#ifdef QS_DEBUG
  long double lastT; // Make sure time always increases
#endif
};


extern
struct QsIterator
*qsIterator_create(struct QsSource *source, int channel);
extern
// Iterators use very little data so this
// just costs about as much as doing malloc(20)
struct QsIterator
*qsIterator_createCopy(struct QsIterator *it);


extern
void qsIterator_destroy(struct QsIterator *it);

extern
struct QsIterator2
*qsIterator2_create(struct QsSource *s0, struct QsSource *s1,
    int channel0, int channel1);

extern
void qsIterator2_destroy(struct QsIterator2 *it);

static inline
void qsIterator_copy(struct QsIterator *it,
    const struct QsIterator *from)
{
  QS_ASSERT(it);
  QS_ASSERT(from);
  memcpy(it, from, sizeof(*it));
}

static inline
void qsIterator_reInit(struct QsIterator *it)
{
  // initialize to having no data to read and
  // will read the next point written.

  QS_ASSERT(it);
  QS_ASSERT(it->source);
  it->i = it->source->i;
  it->wrapCount = it->source->wrapCount;
}

static inline
void qsIterator2_reInit(struct QsIterator2 *it)
{
  // initialize to having no data to read.

  QS_ASSERT(it);
  struct QsSource *s0, *s1;
  s0 = it->source0;
  s1 = it->source1;
  QS_ASSERT(s0);
  QS_ASSERT(s1);

  it->i0 = s0->i;
  it->i1 = s1->i;
  if(s0->wrapCount - s1->wrapCount >= 0)
    it->wrapCount = s0->wrapCount;
  else
    it->wrapCount = s1->wrapCount;
}

// private to libquickscope
static inline
bool _qsIterator_checkWithMaster(struct QsIterator *it,
    const struct QsSource *master)
{
  int wrapDiff;
  wrapDiff = master->wrapCount - it->wrapCount;

  while(wrapDiff > 1 || (wrapDiff == 1 &&
        (it->source->timeIndex[it->i] <= master->i ||
         it->i <= master->i)))
  {
    // The iterator is a lap or more behind the master source
    // at an invalid (nonexistent) old point in the ring buffer.
    // The buffer is only valid from one lap behind the master
    // to the current masters last write position.
    it->i = master->i + 1;
    it->wrapCount = master->wrapCount - 1;

    if(it->i >= master->group->maxNumFrames)
    {
      it->i = 0;
      it->wrapCount = master->wrapCount;
    }

#ifdef QS_DEBUG
    QS_VASSERT(0, "%s() had to reset iterator buffer that may be too small\n"
        "maxNumFrames=%d\n",
        __func__, master->group->maxNumFrames);
#endif
    return true;
  }
  return false;
}

static inline
bool qsIterator_check(struct QsIterator *it)
{
  QS_ASSERT(it);
  QS_ASSERT(it->source);

  struct QsSource *s, *master;
  s = it->source;
  master = s->group->master;

  if(_qsSource_checkWithMaster(s, master) ||
    _qsIterator_checkWithMaster(it, master))
    return false; // no data to read

  int wrapDiff;
  wrapDiff = s->wrapCount - it->wrapCount;


  if(wrapDiff < 0 || (wrapDiff == 0 && it->i >= s->i))
    return false; // no data to read

  QS_ASSERT(wrapDiff == 1 || wrapDiff == 0);
  
  return true; // we have data to read.
}

// Read a particular source and channel
// Returns true if there is a value, else
// returns false if there is no value.
static inline
bool qsIterator_get(struct QsIterator *it, float *x, long double *t)
{
  if(!qsIterator_check(it))
    return false; // no data to read.

  struct QsSource *s;
  s = it->source;

  // TODO: FIX computing this twice
  // once below and once in qsIterator_check().
  int wrapDiff;
  wrapDiff = s->wrapCount - it->wrapCount;

  // increment the iterator
  ++it->i;

  if(wrapDiff && it->i > s->iMax)
  {
    ++it->wrapCount;
    it->i = 0;
  }

  *x = s->framePtr[it->i * s->numChannels + it->channel];
  *t = s->group->time[s->timeIndex[it->i]];

#ifdef QS_DEBUG
  QS_VASSERT(*t >= it->lastT, "Time is decreasing:\n"
      "Time went from (it->lastT=) %Lg to (*t=) %Lg",
      it->lastT, *t);
  it->lastT = *t;
#endif

  return true;
}

// Read a particular source and channel
// Returns true if there is a value, else
// returns false if there is no value.
// Does not advance to the next value like in
// qsInterator_get().
static inline
bool qsIterator_poll(struct QsIterator *it, float *x, long double *t)
{
  if(!qsIterator_check(it))
    return false; // no data to read.

  struct QsSource *s;
  s = it->source;

  // TODO: FIX computing this twice
  // once below and once in qsIterator_check().
  int wrapDiff;
  wrapDiff = s->wrapCount - it->wrapCount;

  // increment the iterator
  int i;
  i = it->i + 1;

  if(wrapDiff && i > s->iMax)
    i = 0;

  *x = s->framePtr[i * s->numChannels + it->channel];
  *t = s->group->time[s->timeIndex[i]];

#ifdef QS_DEBUG
  QS_VASSERT(*t >= it->lastT, "Time is decreasing:\n"
      "Time went from (it->lastT=) %Lg to (*t=) %Lg",
      it->lastT, *t);
#endif

  return true;
}

static inline
bool _qsIterator2_bumpIts(struct QsIterator2 *it,
    const struct QsSource *s0,
    const struct QsSource *s1)
{
  if(s1->timeIndex[it->i1 + 1] != s1->timeIndex[it->i1] &&
    s0->timeIndex[it->i0 + 1] != s0->timeIndex[it->i0])
  {
    // Most common case. s0 -> n , n+1
    //                   s1 -> n , n+1

    QS_ASSERT(s1->timeIndex[it->i1 + 1] == s1->timeIndex[it->i1] + 1);
    QS_ASSERT(s0->timeIndex[it->i0 + 1] == s0->timeIndex[it->i0] + 1);

    ++it->i0;
    ++it->i1;
    return true; // got a new pair!
  }
  if(s1->timeIndex[it->i1 + 1] != s1->timeIndex[it->i1] &&
      s0->timeIndex[it->i0 + 1] == s0->timeIndex[it->i0])
  {
    // case. s0 -> n , n
    //       s1 -> n , n+1

    QS_ASSERT(s1->timeIndex[it->i1 + 1] == s1->timeIndex[it->i1] + 1);
    QS_ASSERT(s0->timeIndex[it->i0 + 1] == s0->timeIndex[it->i0]);

    ++it->i0;
    return true; // got a new pair!
  }
  if(s1->timeIndex[it->i1 + 1] == s1->timeIndex[it->i1] &&
      s0->timeIndex[it->i0 + 1] != s0->timeIndex[it->i0])
  {
    // case. s0 -> n , n+1
    //       s1 -> n , n

    QS_ASSERT(s1->timeIndex[it->i1 + 1] == s1->timeIndex[it->i1]);
    QS_ASSERT(s0->timeIndex[it->i0 + 1] == s0->timeIndex[it->i0] + 1);

    ++it->i1;
    return true; // got a new pair!
  }
  if(s1->timeIndex[it->i1 + 1] == s1->timeIndex[it->i1] &&
      s0->timeIndex[it->i0 + 1] == s0->timeIndex[it->i0])
  {
    // case. s0 -> n , n
    //       s1 -> n , n

    QS_ASSERT(s1->timeIndex[it->i1 + 1] == s1->timeIndex[it->i1]);
    QS_ASSERT(s0->timeIndex[it->i0 + 1] == s0->timeIndex[it->i0]);

    // TODO: It's questionable what this case is for and
    // what to do in this case.

    ++it->i0;
    ++it->i1;
    return true; // got a new pair!
  }
  
  QS_VASSERT(0, "Got iterator problems in qsIterator_get2()\n");
  return false;
}

#define PRINT()    /* empty */
//#define PRINT()  fprintf(stderr, "%s line:%d:%s()\n", __FILE__, __LINE__, __func__)

#if 0
#define  PRINTALL() \
{\
  {\
    fprintf(stderr, "*t=%Lg\n", *t);\
    fprintf(stderr, "it->i0=%d "\
      "it->i1=%d it->wrapCount=%d\n",\
      it->i0,\
      it->i1, it->wrapCount);\
\
    {\
      GSList *l;\
      for(l=qsApp->sources; l; l=l->next)\
      {\
        struct QsSource *s;\
        s = l->data;\
        fprintf(stderr, "%s%d ", (s->isMaster)?"master":"source",\
            s->id);\
      }\
      fprintf(stderr, "\n");\
    }\
    {\
      GSList *l;\
      for(l=qsApp->sources; l; l=l->next)\
      {\
        struct QsSource *s;\
        s = l->data;\
        fprintf(stderr, "iMax=%3.3d  ", s->iMax);\
      }\
      fprintf(stderr, "\n");\
    }\
    {\
      GSList *l;\
      for(l=qsApp->sources; l; l=l->next)\
      {\
        struct QsSource *s;\
        s = l->data;\
        fprintf(stderr, "i=%3.3d    ", s->i);\
      }\
      fprintf(stderr, "\n");\
    }\
    {\
      GSList *l;\
      for(l=qsApp->sources; l; l=l->next)\
      {\
        struct QsSource *s;\
        s = l->data;\
        fprintf(stderr, "wc=%3.3d   ", s->wrapCount);\
      }\
      fprintf(stderr, "\n");\
    }\
\
    int i;\
\
    for(i=0; i<s0->group->bufferLength; ++i)\
    {\
      fprintf(stderr, "TI %d   ", i);\
      GSList *l;\
      for(l=qsApp->sources; l; l=l->next)\
      {\
        struct QsSource *s;\
        s = l->data;\
        if(!s->isMaster || i < s0->group->maxNumFrames)\
          fprintf(stderr, "v%3.3g TI%3.3d  ",\
              s->framePtr[i*s->numChannels],\
              s->timeIndex[i]);\
        else\
          fprintf(stderr, "[            ] ");\
      }\
      if(i < s0->group->maxNumFrames)\
        fprintf(stderr, " tstamp=%3.3Lg\n", s0->group->time[i]);\
      else\
        fprintf(stderr, "\n");\
    }\
  }\
} while(0)
#endif

// Read 2 particular sources and channels
// Returns true if there is a value, else
// returns false if there is no value.
static inline
bool qsIterator2_get(struct QsIterator2 *it,
    float *x0, float *x1, long double *t)
{
  QS_ASSERT(it);
  struct QsSource *s0, *s1, *master;
  s0 = it->source0;
  s1 = it->source1;

  QS_ASSERT(s0);
  QS_ASSERT(s1);
  struct QsGroup *g;
  g = s0->group;
  QS_ASSERT(g);
  QS_ASSERT(s1->group);
  QS_ASSERT(g == s1->group);

  master = g->master;

  if(_qsSource_checkWithMaster(s0, master) ||
    _qsSource_checkWithMaster(s1, master))
    return false;


  int wrapDiff, mi;

  mi = master->i;
  wrapDiff = master->wrapCount - it->wrapCount;

  if(wrapDiff > 1 ||
      (wrapDiff == 1 && 
       (s0->timeIndex[it->i0] < mi ||
        s1->timeIndex[it->i1] < mi)))
  {
    // The iterator is a lap or more behind the master source.
    // The buffer is only valid from within one lap of the master.
    // We set it to the first invalid index so that the next
    // one we read will be valid.

    QS_SPEW("QsInterator2 1 or 2 sources (id=%d,%d) "
        "got lapped by the master source\n", s0->id, s1->id);

    it->i0 = ++mi;
    it->i1 = mi;
    it->wrapCount = master->wrapCount - 1;

    while(s0->timeIndex[it->i0] < mi ||
      s1->timeIndex[it->i1] < mi)
    {
      ++it->i0;
      ++it->i1;
      if(mi >= g->maxNumFrames)
      {
        // May as well wrap back to zero at this point.
        // Then this works if we are reading the master
        // too.
        it->i0 = 0;
        it->i1 = 0;
        it->wrapCount = master->wrapCount;
        break;
      }
    }

#ifdef QS_DEBUG
    fprintf(stderr, "%s() had to reset iterator buffer that may be too small\n"
        "or some object stopped reading a source, as in freeZe (z-key):\n"
        "source ids=%d,%d maxNumFrames=%d\n",
        __func__, s0->id, s1->id, g->maxNumFrames);
#endif
  }

  int wrapDiff0, wrapDiff1;

  wrapDiff0 = s0->wrapCount - it->wrapCount;
  wrapDiff1 = s1->wrapCount - it->wrapCount;

  if( // looking ahead on i0
      wrapDiff0 < 0 || (wrapDiff0 == 0 && it->i0 >= s0->i) ||
      // looking ahead on i1
      wrapDiff1 < 0 || (wrapDiff1 == 0 && it->i1 >= s1->i))
  {
PRINT();
    return false; // We have nothing to read at this point.
  }
  QS_ASSERT(wrapDiff0 == 0 || wrapDiff0 == 1);
  QS_ASSERT(wrapDiff1 == 0 || wrapDiff1 == 1);

  bool didIncrement = false;

  // Align the iterator timeIndexes by incrementing
  // i0 and/or i1 if we can.

  while(((wrapDiff0 && wrapDiff1) || (!wrapDiff0 && it->i0 < s0->i))
      && (s0->timeIndex[it->i0] < s1->timeIndex[it->i1]))
  {
    if(!didIncrement)
      didIncrement = true;
    ++it->i0;
    if(wrapDiff0 && wrapDiff1 && it->i0 > s0->iMax)
    {
      // We must move both iterators in this wrap-around
      // event, because the timestamps will not align
      // if they are not in the same wrap count.
      QS_ASSERT(it->i0 == s0->iMax + 1);
      QS_ASSERT(s0->timeIndex[0] == 0 &&
          s1->timeIndex[0] == 0);
      ++it->wrapCount;
      wrapDiff0 = wrapDiff1 = 0;
      it->i0 = it->i1 = 0;
      break;
    }
  }

  while(((wrapDiff0 && wrapDiff1) || (!wrapDiff1 && it->i1 < s1->i))
      && (s1->timeIndex[it->i1] < s0->timeIndex[it->i0]))
  {
    if(!didIncrement)
      didIncrement = true;
    ++it->i1;
    if(wrapDiff0 && wrapDiff1 && it->i1 > s1->iMax)
    {
      // We must move both iterators in this wrap-around
      // event, because the timestamps will not align
      // if they are not in the same wrap count.
      QS_ASSERT(it->i1 == s1->iMax + 1);
      QS_ASSERT(s0->timeIndex[0] == 0 &&
          s1->timeIndex[0] == 0);
      ++it->wrapCount;
      wrapDiff0 = wrapDiff1 = 0;
      it->i0 = it->i1 = 0;
      break;
    }
  }

  if(s1->timeIndex[it->i1] != s0->timeIndex[it->i0])
  {
    // The two iterators are not at the at the same timestamp
    // and we cannot increment the iterators to get them there.
PRINT();
    return false;
  }
  // Now we have the same timestamp for both iterators
  
  if(didIncrement)
  {
    // And we did increment one of the iterators in this
    // function call, so we have a new value pair.
    goto ret;
  }

  // Now we must increment one or both of the iterators
  // to find the next pair of values with the same
  // time index (timestamp).

  if(wrapDiff0 || it->i0 < s0->i)
  {
    // We can increment i0:

    if(it->i0 < s0->iMax || !wrapDiff0)
    {
      // We can increment i0 without a wrap count change
      
      if(wrapDiff1 || it->i1 < s1->i)
      {
        // We can increment it1 too.

        if(it->i1 < s1->iMax || !wrapDiff1)
        {
          // We can increment i0 and i1 both without a
          // wrap count change.

          if(_qsIterator2_bumpIts(it, s0, s1))
            goto ret;
PRINT();
          return false;
        }
      
        QS_ASSERT(it->i1 == s1->iMax);

        it->i0 = it->i1 = 0;
        ++it->wrapCount;

        if(wrapDiff0)
          goto ret;
PRINT(); 
        return false; // i0 is looking forward in source 0
      }

      // We cannot increment i1.

      ++it->i0;

      if(s0->timeIndex[it->i0] == s1->timeIndex[it->i1])
        goto ret;
PRINT();
      return false;
    }
    else
    {
      // We can increment i0 with a wrap added

      QS_ASSERT(it->i0 == s0->iMax);


      it->i0 = it->i1 = 0;
      ++it->wrapCount;

      if(wrapDiff1)
        goto ret;
PRINT();
      return false; // it1 is looking forward in source 1
    }
  }
  else if(wrapDiff1 || it->i1 < s1->i)
  {
    // We can increment it1 but not it0.

    if(it->i1 < s1->iMax || !wrapDiff1)
    {
      // We can increment i1 without a wrap count change
      // but not i0

      ++it->i1;
      if(s0->timeIndex[it->i0] == s1->timeIndex[it->i1])
        goto ret;
PRINT();
      return false;
    }

    // We can increment i1 with a wrap count change
    // but not i0

    QS_ASSERT(it->i0 == s0->iMax);

    it->i0 = it->i1 = 0;
    ++it->wrapCount;
PRINT();
    return false; // i0 is looking forward in source 0
  }
  else
  {
PRINT();
    return false; // We cannot increment i0 or i1
  }

ret:

  *x0 = s0->framePtr[it->i0 * s0->numChannels + it->channel0];
  *x1 = s1->framePtr[it->i1 * s1->numChannels + it->channel1];
  *t = s0->group->time[s0->timeIndex[it->i0]];

  QS_ASSERT(s0->timeIndex[it->i0] == s1->timeIndex[it->i1]);


#ifdef QS_DEBUG

 // if(*t < it->lastT)
 //   PRINTALL();

  QS_VASSERT(*t >= it->lastT, "Time is decreasing:\n"
      "Time went from (it->lastT=) %Lg to (*t=) %Lg\n"
      "timeIndex0[%d]=timeIndex1[%d]=%d",
      it->lastT, *t, it->i0, it->i1, s0->timeIndex[it->i0]);
  it->lastT = *t;
#endif

PRINT();
  return true;
}

// Sets frame at index based on an iterator which was read with
// call again with the same iterator to write more than one value
// in the given frame, from a call to qsIterator_get(it,...).
// This keeps the reading and than writing of QsSource's synchronized
// by keeping them with values with the same time stamps.
static inline
float *qsSource_setFrameIt(struct QsSource *s, const struct QsIterator *it)
{
  QS_ASSERT(s);
  QS_ASSERT(it);
  QS_ASSERT(it->source);
  QS_ASSERT(!s->isMaster);
  struct QsGroup *group;
  struct QsSource *otherS;
  group = s->group;
  otherS = it->source;
  QS_ASSERT(group);
  QS_ASSERT(otherS);
  int wrapDiff, i, j;
  i = s->timeIndex[s->i];
  j = otherS->timeIndex[it->i];
  wrapDiff = it->wrapCount - s->wrapCount;
  // The iterator should be one time stamp ahead of this source or
  // at the same time stamp.
  QS_ASSERT(
    (
      // one ahead
      (wrapDiff == 0 && j == i + 1)
      ||
      (wrapDiff == 1 && j == 0 && i == group->maxNumFrames - 1)
    )
    ||
    // at the same time stamp
    (wrapDiff == 0 && j == i)
  );

  if(
      (wrapDiff == 0 && j == i + 1)
      ||
      (wrapDiff == 1 && j == 0 && i == group->maxNumFrames - 1)
    )
  {
    // The iterator is one frame ahead so we add a new frame
    // making them now at the same frame.
    long double *t;
    return qsSource_setFrame(s, &t);
  }

  if(wrapDiff == 0 && j == i)
  {
    // The iterator is at the same frame so we append to
    // the last frame, so that we stay at the same frame.
    return qsSource_appendFrame(s);
  }
 
  fprintf(stderr, "%s(s=%p, i=%p) the source at %p is out of sync with"
      " the iterator at %p\n"
      "You must call:\n"
      "qsSource_initIt(s, i) then\n"
      "qsIterator_getValue(i,..) or qsIterator_getFrames(i,..) and then\n"
      "qsSource_initIt(s, it)\n", __func__, s, it, s, it);

  return NULL;
}
