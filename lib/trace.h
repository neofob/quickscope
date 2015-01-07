/* All drawing will be time faded. So if your
 * source data read rate is higher than the
 * monitor refresh rate you will still see traces
 * that fade in order of data reading. */

struct QsSource;
struct QsTrace;
struct QsWin;

extern
struct QsTrace *qsTrace_create(struct QsWin *win,
      struct QsSource *xs, int xChannelNum,
      struct QsSource *ys, int yChannelNum,
      float xScale, float yScale, float xShift, float yShift,
      // There are just two traces modes
      //    1. just points
      //    2. lines
      // TODO: Only thing to consider missing different color
      // for points and lines, but that maybe resource expensive.
      bool lines, // or just points
      float red, float green, float blue);
extern
void qsTrace_setSwipeX(struct QsTrace *trace, bool on);

/* destroying the QsWin will destroy the QsTrace unless
 * you call qsTrace_destroy() before you destroy the
 * QsWin. */
extern
void qsTrace_destroy(struct QsTrace *trace);

