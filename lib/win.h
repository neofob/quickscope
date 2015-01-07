struct QsWin;
struct QsTrace;

extern
struct QsWin *qsWin_create(void);
extern
void qsWin_destroy(struct QsWin *win);

/* scale is in normalized units so that 1 is the full window
 * area.  Same for offset.  Offset will be the position of
 * one of the vertical grid lines.  The position of the rest
 * of the lines will equally spaced out from from there. */
extern
void qsWin_setXGridPixelSpace(struct QsWin *win, float minPixelGridSpace);
extern
void qsWin_setYGridPixelSpace(struct QsWin *win, float minPixelGridSpace);
extern
void qsWin_shiftXGrid(struct QsWin *win, float normalizedOffset);
extern
void qsWin_shiftYGrid(struct QsWin *win, float normalizedOffset);
extern
void qsWin_setXUnits(struct QsWin *win, const char *unitsLabel);
extern
void qsWin_setYUnits(struct QsWin *win, const char *unitsLabel);
extern
struct QsWin *qsWin_getDefault(struct QsWin *win);

