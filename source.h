// We use this 1 special float value
#define QS_LIFT   (NAN)  /* lift pen */

struct QsTrace;
struct QsSource;
struct QsGroup;
struct QsIterator;
struct QsIterator2;


#if 0
typedef int (*QsSource_ReadFunc_t)(struct QsSource *s,
    long double t, long double prevT, void *data);

#else
/* under-run is the state when the source read callback
 * is not called frequently enough, and the source
 * buffers may have a very old frame adjacent to a
 * recent frame.  If the group source type is a
 * periodic source type, a pen-lift will automatically
 * be inserted in between the old and new frame if this
 * happens.
 *
 * If the source group is not periodic there is no
 * automatically defined under-run condition.  So source
 * groups that are of type QS_CUSTOM will
 * not have under-run automatically handled.
 *
 * time is, measured in seconds, is the time since
 * the program started, real wall clock time
 *
 * tB = current time from when program started as gotten
 *   by the controller at the start of the current read
 *   callback loops
 * tBPrev = the value of tB at the last call that any
 *   data was written for this source
 * tA = 
 *   if master:
 *     tA + deltaT should be the first time that should be
 *     written in the callback based on the source group
 *     sample rate and under-run state
 *   else is not master:
 *     0
 *
 * consider qsSource_lastTime(s) to get the last time stamp
 * that this source was written to by this source, s.
 * That time stamp may not be correct is the source, s,
 * is under-run.
 *
 * Note: sample rates can change between read callback cycles.
 *   So detecting under-run is not so simple.
 *
 * nFrames = 
 *        if master:
 *          the expected number of frames to write
 *          given the current group source type
 *          and the group rate parameters.
 *          nFrames = 0 if this has no sample rate
 *          as in the case a source group that is QS_CUSTOM
 *        else if not master:
 *          the number of frames to write in order
 *          to catch up to the master source in writing
 * deltaT =
 *        if master:
 *          the expected time difference between frames
 *          based on source group type or
 *          0 if there is no sample rate as in the
 *          case a source group that is QS_CUSTOM
 *        else if not master:
 *          0; sources should use the time
 *          stamps that are from the master
 *          even for source group of QS_CUSTOM
 * underrun = true is the master source is under-run,
 *            is false otherwise.
 *
 * data = user callback data
 */
typedef int (*QsSource_ReadFunc_t)(struct QsSource *s,
    long double tB, long double tBPrev, long double tA,
    long double deltaT, int nFrames, bool underrun,
    void *data);
#endif


struct QsSource
{
  // inherit QsAdjusterList
  //
  // Holds adjuster(s) that are related to this source object which
  // get added to each QsWin that has a trace with this source.
  // The QS API users should add adjusters to this QsAdjusterList
  // since sources are made by users.  Example: an adjuster that changes
  // a parameter in source that is a physics model simulation or input
  // device.
  struct QsAdjusterList adjusters;

  /* TODO: make this interface opaque.  Requires a rewrite of almost every
   * source file in the Quickscope library.  Would also run a little
   * slower and with a little more memory.  So, okay we thought about it. */

  /* current and previous QsSource_ReadFunc_t call time */
  long double prevT;

  /* read() returns
   *    1  -> have new data
   *    0  -> no new data
   *   -1  -> destroy source */
  QsSource_ReadFunc_t read;
  void *callbackData; // user read callback data.

#ifdef QS_DEBUG
  size_t objectSize;
#endif

  float *framePtr; // pointer to allocated float array memory,
  // the ring buffer.
  int *timeIndex; // Mapping from frame index to time stamp
  // index.  This is simple for the master source since
  // that mapping is just timeIndex[N] = N.
  // The mapping is indirect so that we may have multiple sets
  // of values in single frames for non-master sources.
  // timeIndex[i] is the corresponding master source index
  // of data at framePtr[i].  If timeIndex[n] == timeIndex[n+1]
  // than there will be at least two values for that frame in
  // framePtr[n] and framePtr[n+1].
  //
  // This QsSource thing is like a synchronized pool of ring
  // buffers (QsSources).  They only need to be synchronized
  // to one within the same wrap count.  They automatically
  // recover if they fall way out of sync by one lap or more
  // from the master source.
  //
  // If the master source needs to write one than one value
  // at a given time than, don't make that source the master.
  // Instead make a dummy master source, that has the job of
  // just setting timestamps, and does not set values that are
  // used.

  int wrapCount, // counts number of times the buffer loops back
      // to buffer index zero.  wrapCount wrapping back to zero
      // is no problem because we calculate and use only
      // differences between different sources' and iterators
      // wrapCount values.  We never do:
      //   if(wrapCount0 > wrapCount1)
      // but use:
      //   if(wrapCount0 - wrapCount1 > 0)
      // to make the wrapCounts INT_MAX wrap-able.
      //   INT_MIN - INT_MAX == 1
      //   INT_MAX + 1 == INT_MIN
      //   shit like that, in effect makes it appear that
      //   INT_MIN > INT_MAX, which is what we want.
      //
      // TODO: We may only need to detect 3 or 2 buffer positions:
      // 1 behind one lap, 1 at current lap, and maybe 1 ahead one
      // lap; so using 2^32 (>3) states is a little over-kill, but
      // this also lets us recover from a large number of lap
      // overruns, not just a one lap overrun.  That could be
      // required.

    i, // the last framePtr[] index we wrote
      // We make the memory pointed to by framePtr into
      // a circular buffer that overwrites older values.
    iMax, // largest index to framePtr.  iMax will always be
    // maxNumFrames - 1 for the master.  It will be maxNumFrames - 1
    // plus the number of multi-value additions from calls to
    // qsSource_appendFrame(), per ring buffer loop, for non-master
    // sources.  The ring buffer is larger for non-master sources.

      // NOTE: The QsSource does not keep memory of where to
      // read from the framePtr buffer.  QsIterator and
      // QsIterator2 (iterators) do that.

    numChannels; // number of channels per frame

  // traces to call after read, also manage these traces
  GSList *traces,
         // Lists of iterators that read from this source.
         *iterators, *iterator2s,
         *changeCallbacks;

  // The Group that this source belongs to.
  // Sources in a group use the same source frame indexing
  // and frame time array which marks the time for each frame.
  struct QsGroup *group;

  // The QsController that triggers the _qsSource_read() to be called.
  // That's what QsController is for.
  struct QsController *controller;

  /* set this in objects that inherit QsSource.  destroy is also used to
   * stop re-entrance into qsSource_destroy() by being set to NULL in
   * qsSource_destroy() and it should be set to NULL in the inheriting
   * *_destroy() function too. */
  void (*destroy)(void *);

  int id; // Unique ID so we know this source object from all others.

  bool isMaster; // set to true if this is the source that is
  // setting the write pace and hopefully the time stamps.
  // Sources that fall behind in writing by a buffer length or more
  // of the master will get reset (??) to writing to the masters
  // oldest index.  We set the master to the first source created
  // in the QsGroup.  This isMaster variable is redundant
  // given that a pointer to the QsSource master is kept in the
  // QsGroup structure, but this may add speed by not having
  // to dereference two layers of pointers and compute a boolean
  // statement to see if we have the master.
  // Who is the master? (??) The QsSource that is writing the fastest.
  // TODO: Can we have the master change?  Maybe not.
  bool isSwipable;
  
  enum QsSource_Type type;
  float sampleRate, *sampleRates;
};


QS_BASE_DECLARE(qsSource);

extern // Declaring this void * make is easier to  use this to make
  // an inheriting object.  It's a GTK+ learned trick. 
void *qsSource_create(QsSource_ReadFunc_t read,
    int numChannels,  int maxNumFrames /* max num frames buffered */,
    // The maxNumFrames will only be set for the first source
    // made in the group (checked that it is the same in QS_DEBUG).
    // It is also the ring buffer length.
    const struct QsSource *group /* group=NULL to make a new group */,
    size_t objectSize);
extern
void qsSource_setReadFunc(struct QsSource *s, QsSource_ReadFunc_t read);
static inline
bool qsSource_isSwipable(struct QsSource *s)
{
  QS_ASSERT(s);
  return s->isSwipable;
}
// Returns the frame sample rate if not type QS_CUSTOM
static inline
float qsSource_getSampleRate(struct QsSource *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->group);
  return s->group->sampleRate;
}
static inline
int qsSource_numChannels(struct QsSource *s)
{
  QS_ASSERT(s);
  return s->numChannels;
}
static inline
void qsSource_setIsSwipable(struct QsSource *s, bool isSwipable)
{
  QS_ASSERT(s);
  s->isSwipable = isSwipable;
}

// TODO: make plotting (traces) between 2 sources in different groups
// by interpolating with the time stamps of the two different source
// groups.  Currently we only support traces with 2 sources from the
// same group which means that the 2 sources use the same time stamps.

extern
void qsSource_destroy(struct QsSource *source);
extern
void qsSource_removeTraceDraw(struct QsSource *s,  struct QsTrace *trace);
extern
void qsSource_addTraceDraw(struct QsSource *s,  struct QsTrace *trace);
// Initialize a source so that it may be written to at the same
// place as that the iterator reads.
// Call this and then read the QsIterator and then qsSource_setFrame()
// again and again, that's read 1 then write 1 and then
// read 1 then write 1 and so on for a long time.
extern
void qsSource_initIterator(struct QsSource *s, struct QsIterator *it);
extern
void *qsSource_addChangeCallback(struct QsSource *s,
    // return false to remove the callback
    bool (*callback)(struct QsSource *, void *), void *data);
extern
void qsSource_removeChangeCallback(struct QsSource *s, void *ref);
// changes source (to) to be at the same frame as (from)
extern
void qsSource_sync(struct QsSource *to, struct QsIterator *it);
extern
long double qsSource_lastTime(struct QsSource *s);

// return false on success
extern
bool qsSource_setType(struct QsSource *s, enum QsSource_Type type,
    const float *sampleRates, float startingSampleRate);


// Returns the number of frames that may be written to
// catch up to the master QsSource.
static inline
int qsSource_numFrames(const struct QsSource *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->group);
  QS_ASSERT(s->group->master);
  if(s->isMaster) return 0;

  struct QsSource *master;
  master = s->group->master;
  QS_ASSERT(master != s);
  int i, wrapDiff;
  wrapDiff = master->wrapCount - s->wrapCount;
  i = s->timeIndex[s->i];

  QS_ASSERT(wrapDiff >= 0);
  QS_ASSERT(wrapDiff > 0 || master->i >= i);

  if(wrapDiff > 1 || (wrapDiff == 1 && i <= master->i))
    return s->group->maxNumFrames; // We can write the whole buffer.
  else if(wrapDiff == 0)
    return master->i - i;
  else // wrapDiff == 1
    return master->i + s->group->maxNumFrames - i;
}

static inline
int qsSource_maxNumFrames(const struct QsSource *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->group);
  return s->group->maxNumFrames;
}

static inline
bool qsSource_isMaster(const struct QsSource *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->group);
  QS_ASSERT((s->isMaster && s == s->group->master) ||
      (!s->isMaster && s != s->group->master));
  return s->isMaster;
}

// This should be private but can't be if
// qsSource_checkWithMaster() is static inline.
extern
void qsSource_emptyIterators(struct QsSource *s);


// private to source and iterator stuff.
// Makes sure that the source is within a lap of the master.
// Time stamps are only valid within a lap behind the master.
static inline
bool _qsSource_checkWithMaster(struct QsSource *s,
    const struct QsSource *master)
{
  QS_ASSERT(master);

  if(s == master)
    return false; // no change

  int wrapDiff;

  wrapDiff = master->wrapCount - s->wrapCount;
  // No source should write ahead of the master source.
  // We must be behind or at the master in writing:
  QS_ASSERT((wrapDiff == 0 && s->timeIndex[s->i] <= master->i) || wrapDiff > 0);
  QS_ASSERT(master->i >= 0 && master->i < s->group->maxNumFrames);


  if(wrapDiff > 1 || (wrapDiff == 1 &&
        (s->timeIndex[s->i] < master->i || s->i < master->i)))
  {
    // The source fell behind in writing by more than one wrap so
    // we reset the source writing to one wrap behind.
    // The buffer is only valid from within one lap of the master.
    // It's the first invalid index, the next index is valid.
    s->wrapCount = master->wrapCount - 1;
    s->i = master->i;
    s->timeIndex[s->i] = s->i;
    s->iMax = 0;
    // Note: the last time stamp index must be valid
    // so that it can be used by iterators to compare
    // to the last write index to the current read index.

    // The source s buffer has no valid data in it, so
    // we empty all the iterators that use this source s.
    qsSource_emptyIterators(s);

    QS_SPEW("had to reset source buffer that may be too small\n"
        "source id=%d maxNumFrames=%d\n",
        s->id, master->group->maxNumFrames);
    return true; // there is a change to s
  }

  return false; // there is not change to s
}

// TODO: Add auto-resizing of the source framePtr buffer.

// Set frames and set (or get for non-master) time arrays.
// num is changed to be less than or equal to what
// was passed in.  If the source maxNumFrames is
// large you'll get num most of the time.
// This can't write any multi-value frames.
// The return float array may be filled with float
// values in the order of frames with the sequence
// of all channels in the source in each frame:
//
//   Frame0: value0_channel0, value0_channel1, ..., value0_channelN-1
//   Frame1: value1_channel0, value1_channel1, ..., value1_channelN-1
//   ... etc.
//   t time is a simple array of long double times in seconds.
//
//  Put another way:
//
//    value = returnedFrames[ frameIndex * numChannels + channelIndex ];
//    time = t[frameIndex];
//
//  See also qsSource_appendFrame() which more than one set of values
//  to a frame, giving more than one value per channel at a given time.
//
static inline
float *qsSource_setFrames(struct QsSource *s, long double **t,
    int *num)
{
  QS_ASSERT(s && t && num);
  QS_ASSERT(*num >= 1);
  QS_ASSERT(s->group);
  QS_ASSERT(s->group->master);
  QS_ASSERT(s->i >= 0);
  int maxNumFrames;
  maxNumFrames = s->group->maxNumFrames;
  QS_ASSERT(maxNumFrames > 0);
  QS_ASSERT(s->numChannels > 0);

  if(qsSource_isMaster(s))
  {
    // Easy case!

    QS_ASSERT(s->i < maxNumFrames);

    ++s->i;
    if(s->i == maxNumFrames)
    {
      s->i = 0;
      ++s->wrapCount;
    }
    // s->i is now the current index
    // that is used to start the frames

    int len; // len = how many frames can we write?
    len = maxNumFrames - s->i;
    if(*num > len)
      *num = len;
    else if(*num < len)
      len = *num;

    float *ret;
    ret = &s->framePtr[s->numChannels*s->i];
    *t = &s->group->time[s->i];
    // Make s->i the last index written to
    // so that we know what to do next call.
    s->i += len - 1;
    return ret;
  }

  // Dealing with the non-master is not as easy
  // because there may be multi-entry-frames.
  // It's just a way that lets us have
  // more than one value at the same time,
  // like at end points in the trace where we
  // have a point at the back edge, lift the pen,
  // and then a point at the leading edge, as in
  // a sweep signal (source); the leading edge
  // point has the same time and Y-value as does
  // the trailing edge.  Without this complexity
  // we have a suboptimal sweep display.

  QS_ASSERT(s->i < s->group->bufferLength);

  // !s->isMaster
  struct QsSource *master;
  master = s->group->master;

  _qsSource_checkWithMaster(s, master);

  int ti; // ti = the index relative to the master or time array
  ti = s->timeIndex[s->i]; // last index used
  QS_ASSERT(ti >= 0 && ti < maxNumFrames);
  int wrapDiff;
  wrapDiff = master->wrapCount - s->wrapCount;

  // We must be behind or at the master in writing:
  QS_ASSERT((wrapDiff == 0 && ti <= master->i) || wrapDiff > 0);
  QS_ASSERT(master->i >= 0 && master->i < maxNumFrames);

  if(wrapDiff == 0 && ti == master->i)
  {
    // We have caught up to the master in writing.
    // TODO: Can this source become the master?
    // For now, no.
    QS_VASSERT(0, "Cannot write ahead of master source\n");
    *num = 0;
    *t = NULL;
    return NULL;
  }

  int len;
  // first calculate (len) the maximum possible length
  // that we can write.
  if(wrapDiff == 0)
  {
    len = master->i - ti;
  }
  else if(ti != maxNumFrames - 1) // && wrapDiff == 1
  {
    len = maxNumFrames - 1 - ti;
  }
  else // wrapDiff == 1 && ti == maxNumFrames - 1
  {
    len = master->i + 1;
  }

  // Now pick the lesser of len and *num:
  if(len < *num)
    *num = len;
  else if(len > *num)
    len = *num;

  QS_ASSERT(len > 0);

  int initI;
  if(ti + 1 != maxNumFrames)
    initI = s->i + 1;
  else
    initI = 0;


  // Check that the ring buffer is long enough:
  QS_VASSERT(initI + len <= s->group->bufferLength,
      "%s() ring buffer is too small:"
        "increase qsApp->op_bufferFactor (=%f)\n", __func__,
        qsApp->op_bufferFactor);

  if(initI + len > s->group->bufferLength)
  {
    fprintf(stderr, "%s() ring buffer is too small:"
        "increase qsApp->op_bufferFactor (=%f)\n", __func__,
        qsApp->op_bufferFactor);
    *num = len = s->group->bufferLength - initI;
    if(len <= 0)
    {
      // This would be bad.
      *t = NULL;
      QS_VASSERT(0, "%s() ring buffer is too small:"
        "increase qsApp->op_bufferFactor (=%f)\n", __func__,
        qsApp->op_bufferFactor);
      return NULL;
    }
  }

  // Set the new starting framePtr and time index
  // to the next index.
  if(ti + 1 != maxNumFrames)
  {
    ++ti;
    ++s->i;
  }
  else // ti + 1 == maxNumFrames
  {
    // We are at a buffer end and it's time to wrap back
    // writing to index 0.
    s->iMax = s->i;
    s->i = ti = 0;
    s->wrapCount = master->wrapCount;
  }


  float *ret;
  ret = &s->framePtr[s->numChannels*s->i];
  *t = &s->group->time[ti];

  // If the current and last time indexes are okay than
  // all the indexes should be okay.
  if(s->timeIndex[s->i] != ti ||
      s->timeIndex[s->i + len - 1] != ti + len - 1)
  {
    // We set only the time indexes that get written to.
    int j;
    s->timeIndex[s->i] = ti;
    for(j=1; j<len; ++j)
      s->timeIndex[++s->i] = ++ti;
  }
#ifdef QS_DEBUG
  else
  {
    int j;
    for(j=1; j<len; ++j)
      QS_ASSERT(s->timeIndex[++s->i] == ++ti);
  }
#else
  else
    s->i += (len - 1);
#endif

  // s->i is now the last framePtr index that will be written.

  return ret;
}


// Returns float array pointer for frame to write, if the
// QsSource has not catch up to the master QsSource,
// Returns NULL if it has caught up to the master.
static inline
float *qsSource_setFrame(struct QsSource *s, long double **t)
{
  int num = 1;
  return qsSource_setFrames(s, t, &num);
}

// Appends another set of values to the frame, if not the master.
// Adds a frame with the same time stamp as the last, if it is the master.
// A frame may have one or more sets of values, for non-master sources,
// for each channel.
// So the master QsSource can't be changed to a different QsSource.
static inline
float *qsSource_appendFrame(struct QsSource *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->group);
  QS_ASSERT(s->i >= 0);
  
  if(qsSource_isMaster(s))
  {
    long double oldT, *t;
    oldT = s->group->time[s->i];
    float *val;
    val = qsSource_setFrame(s, &t);
    *t = oldT;
    return val;
  }

  QS_ASSERT(s->i < s->group->bufferLength - 1);

  if(s->i >= s->group->bufferLength - 1)
  {
    fprintf(stderr, "QsSource buffer length is too small.\n"
        "Increase qsApp->op_bufferFactor=%f to a larger value.\n",
        qsApp->op_bufferFactor);
    return NULL; // The ring buffer is too small.
  }

  int i; // i is the array index and not the time array index
  i = ++s->i;
  // We repeat the last time stamp.
  s->timeIndex[i] = s->timeIndex[i-1];

  return &s->framePtr[s->numChannels*i];
}

// Returns 1 for the master and 0 from a non-master source
// which is the number of frames used.
// Writes the next frame with the last time for the master source and
// appends to the last frame for a non master source.
static inline
int qsSource_addPenLift(struct QsSource *s)
{
  QS_ASSERT(s);

  if(s->isMaster)
  {
    long double lastT, *t;
    lastT = s->group->time[s->i];
    float *val;
    val = qsSource_setFrame(s, &t);
    *t = lastT;
    *val = QS_LIFT;
    return 1;
  }

  // non master source
  *qsSource_appendFrame(s) = QS_LIFT;
  return 0;
}
// returns false on success
static inline
bool qsSource_setFrameSampleRate(struct QsSource *s, float rate)
{
  QS_ASSERT(s);
  struct QsGroup *g;
  g = s->group;
  QS_ASSERT(g);

  switch(g->type)
  {
    case QS_TOLERANT:
      g->sampleRate = rate;
      if(g->sampleRates[0] > rate)
        g->sampleRates[0] = rate;
      else if(g->sampleRates[1] < rate)
        g->sampleRates[1] = rate;
      return false;
      break;
    case QS_FIXED:
      QS_VASSERT(0, "Cannot set frame sample rate "
          "for QS_FIXED source group.  Sample rate=%f\n",
          g->sampleRate);
      return true;
      break;
    default:
      QS_VASSERT(0, "Write MORE CODE HERE\n");
      return true;
      break;
  }
}
