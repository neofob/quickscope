/* This is a "C-class object" that makes the things that are shared
 * by a group of QsSources.  All QsSources 
 *
 * */
struct QsSource;

enum QsSource_Type
{
  QS_CUSTOM = 0, /* user custom defined, not compatible with other types */
  QS_FIXED,      /* fixed periodic */
  QS_SELECTABLE, /* selectable periodic */
  QS_VARIABLE,   /* variable periodic */
  QS_TOLERANT,   /* not necessarily periodic but can be */
  QS_NONE        /* group type initialization, not used by sources */
};


struct QsGroup
{
  GSList *sources; // The sources that use this group

  struct QsSource *master; // fastest source written
  // sets the time stamps

  long double *time; // time stamp array/buffer

  int maxNumFrames,
      bufferLength; // larger than maxNumFrames
      // by a factor of qsApp->op_bufferFactor to
      // accommodate multi value frames.

  enum QsSource_Type type;
  float sampleRate; // current rate of source frames, ignoring pen lifts
  float *sampleRates;
};
