/* This is a "C-class object" that makes the things that are shared
 * by a group of QsSources.  All QsSources 
 *
 * */
struct QsSource;

struct QsGroup
{
  GSList *sources; // The sources that use this group

  struct QsSource *master; // fastest source written
  // sets the time stamps

  long double *time; // time stamp array/buffer

  // The max of source minSampleRate
  float sampleRate;

  int maxNumFrames,
      bufferLength; // larger than maxNumFrames
      // by a factor of qsApp->op_bufferFactor to
      // accommodate multi value frames.
};
