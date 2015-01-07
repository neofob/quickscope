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
  QS_TOLERANT,   /* not necessarily periodic, works with all but QS_CUSTOM */
  QS_NONE        /* group type initialization, not used by sources */
};


struct QsGroup
{
  GSList *sources; // The sources that use this group

  struct QsSource *master; // fastest source written
  // sets the time stamps

  long double *time; // time stamp array/buffer

  int maxNumFrames,
      bufferLength, // larger than maxNumFrames
      // by a factor of qsApp->op_bufferFactor to
      // accommodate multi value frames.
      underrunCount; // counter/flag to detect source buffer under-run

  enum QsSource_Type type;
  // sample rates of frames, in Hz, ignoring pen lifts
  float sampleRate; // current sample rate (Hz)
  float *sampleRates; // available sample rates (Hz)

  bool sourceTypeChange; // flag to let us know to call
  // _qsSource_checkTypes(s) to fix the group type, sampleRates,
  // and sampleRate.
};
