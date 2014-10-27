/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
struct QsTimer;

struct QsApp
{
  GSList *wins; /* list of all QsWin objects */
  GSList *controllers; /* list of all QsController objects */
  GSList *sources; /* list of all QsSource objects */
  struct QsTimer *timer;

  char **argv; // We use NULL termination


  /* After op_fadeDelay seconds the beam colors start to
   * fade so beams are not seen op_fadePeriod seconds +
   * op_fadeDelay seconds after the beams are first drawn.


   beam intensity (normalized)
        ^
        |
        |
     1 -| ***************************
        |                            *
        |                             *     
        |                              *
        |                               *
        |                                *
        |                                 * 
        |                                  *
        |                                   *  
        |                                    *
        |                                     *
     0 -|----------------------------|---------|----------> time (seconds)
       
        0                       fadeDelay    + fadePeriod 
   first Drawn

   */

  float op_fadePeriod; /* time to fade to zero after fading starts */
  float op_fadeDelay; /* no fading until fadeDelay seconds */

  float op_bgR, op_bgG, op_bgB, /* background color */
        op_gridR, op_gridG, op_gridB, /* grid color */
        op_axisR, op_axisG, op_axisB, /* X and Y axis color */
        op_subGridR, op_subGridG, op_subGridB, /* sub grid color */
        op_tickR, op_tickG, op_tickB; /* axis ticks color */

  float op_savePNG_bgAlpha, op_savePNG_alpha;

  /* The spacing of the grid */
  /* in pixels. All must be positive. */
  float op_gridXMinPixelSpace, op_gridXWinOffset;
  float op_gridYMinPixelSpace, op_gridYWinOffset;

  float op_gridLineWidth, op_tickLineWidth, op_axisLineWidth,
         op_tickLineLength, op_subGridLineWidth; // all in pixels

  double op_defaultIntervalPeriod;

  // (source ring bufferSize)/maxNumFrames so we can have frames
  // with more than one value for each channel.
  float op_bufferFactor;

  gboolean op_doubleBuffer, /* with XPixmap */
        op_grid, /* draw background grid */
        op_axis, /* draw X and Y axis */
        op_subGrid, /* draw grid in the grid */
        op_ticks, /* draw axis ticks or not */
        op_fade; /* beam lines and points don't fade if FALSE */
  
  gboolean op_fullscreen, op_showWindowBorder,
           op_showMenubar, op_showStatusbar;

  gboolean opSourceRequireController; // require this in qsApp_main()

  // Use gtk_widget_add_tick_callback() if true
  gboolean op_syncFadeDraw; // or uses g_timeout_add_full()

  gboolean inAppLevel; // to see where we are in gtk_main(),
    // qsApp_main() and qsApp_destroy();

  int sourceCreateCount;
};

/* singlet container of all things QS */
extern
struct QsApp *qsApp;

/* For a singlet we have init instead of create.
 * It may be called any number of times.
 * There is no destroy like function. */
extern
struct QsApp *qsApp_init(int *argc, char ***argv);

extern
void qsApp_main(void);

extern
void qsApp_destroy(void);

extern
float qsApp_float(const char *name, float dflt);
extern
double qsApp_double(const char *name, double dflt);
extern
const char *qsApp_string(const char *name, const char *dflt);
extern
int qsApp_int(const char *name, int dflt);
extern
bool qsApp_bool(const char *name, bool dflt);
