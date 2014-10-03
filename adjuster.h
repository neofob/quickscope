/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

/* QsAdjuster and it's manager class QsAdjusterList is a method to adjust any
 * number of parameters.  Each parameter you wish to control has a
 * corresponding QsAdjuster.  All the QsAdjuster(s) are managed as a group
 * with a single QsAdjusterList.  This code does not provide a display widget
 * for these things, that is added a Display Callback function.  So in
 * that sense this is display independent.  The user of this can display a
 * single widget and select through a large number of parameters that this
 * widget will control.  It's just not practical to display all the
 * widgets that a 10 channel oscilloscope has when there are 20 parameters
 * for each channel in addition to say 30 or more global parameters.  So
 * we have just a few (one or more) QsWidget(s) that display/feed many
 * parameters and select through the parameter that the QsWiget(s)
 * currently display/feed.
 *
 * The QsAdjusterList and QsAdjuster code is independent of display and
 * input, and most other code.  Very simple and modular.  You can have any
 * number of QsWidget(s) to display and feed any QsAdjuster (parameter) in
 * the QsAdjusterList.
 */

struct QsAdjusterList
{
  // TODO: It would be nice not to expose this the user
  // but if we let the user inherit QsSource in our crude way
  // we end up needing to expose this and struct QsSource

  // List of adjuster(s) and widget(s)
  GList *adjusters, *widgets;
  int count; // number of adjuster(s)
  int changeCount; // counting changes to adjusterList
#ifdef QS_DEBUG
  size_t objSize;
#endif
};


/* Base Class for parameter input thingy */
struct QsAdjuster;


extern
struct QsAdjuster *qsAdjusterFloat_create(struct QsAdjusterList *adjs,
    const char *description, const char *units,
    float *value, float min, float max,
    void (*valueChangeCallback)(void *data), void *data);
extern
struct QsAdjuster *qsAdjusterDouble_create(struct QsAdjusterList *adjs,
    const char *description, const char *units,
    double *value, double min, double max,
    void (*valueChangeCallback)(void *data), void *data);
extern
struct QsAdjuster *qsAdjusterLongDouble_create(struct QsAdjusterList *adjs,
    const char *description, const char *units,
    long double *value, long double min, long double max,
    void (*valueChangeCallback)(void *data), void *data);
extern
struct QsAdjuster *qsAdjusterInt_create(struct QsAdjusterList *adjs,
    const char *description, const char *units,
    int *value, int min, int max,
    void (*valueChangeCallback)(void *data), void *data);
extern
struct QsAdjuster *qsAdjusterBoolean_create(struct QsAdjusterList *adjs,
    const char *description, gboolean *value,
    void (*valueChangeCallback)(void *data), void *data);
extern
struct QsAdjuster *qsAdjusterSelector_create(struct QsAdjusterList *adjs,
    const char *description, int *value, const int *values,
    const char **labels, int valuesLen,
    void (*valueChangeCallback)(void *data), void *data);

// Destroying a Group will destroy all in the group
extern
struct QsAdjuster *qsAdjusterGroup_start(struct QsAdjusterList *adjs,
    const char *description);
extern
void qsAdjusterGroup_end(struct QsAdjuster *start);

