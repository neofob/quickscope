/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "timer.h"
#include "app.h"
#include "controller.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win.h"
#include "win_priv.h"
#include "group.h"
#include "source.h"
#include "source_priv.h"
#include "iterator.h"
#include "trace_priv.h"
#include "controller_priv.h"
#include "interval.h"

struct QsApp *qsApp = NULL;

#ifdef QS_DEBUG
static
void sigHandler(int sig_num)
{
  QS_VASSERT(0, "We caught signal %d", sig_num);
}

static
void logHandler(const gchar *log_domain,
          GLogLevelFlags log_level,
          const gchar *message,
          gpointer user_data)
{
  if(log_level == G_LOG_LEVEL_WARNING)
  {
    const char *GTK_BUG_MESSAGE =
      "Couldn't register with accessibility bus: "
      "Did not receive a reply. Possible causes i"
      "nclude: t";
    if(!strncmp(GTK_BUG_MESSAGE, message, strlen(GTK_BUG_MESSAGE)))
    {
      fprintf(stderr, "We caught error: "
      "log_domain=\"%s\" log_level=%d message=\"%s\"",
      log_domain, log_level, message);
      // Keep running if this is from this known GTK 3 bug.
      fprintf(stderr, "This is from a known bug in GTK 3\n");
      return;
    }
  }

  QS_VASSERT(0, "We caught error: "
      "log_domain=\"%s\" log_level=%d message=\"%s\"",
      log_domain, log_level, message);
}
#endif

struct QsApp *qsApp_init(int *argc, char ***argv)
{
  if(qsApp) return qsApp;

#ifdef QS_DEBUG
  // The following let us attach the GDB debugger when some
  // things go wrong, by calling QS_ASSERT() or QS_VASSERT()
  // in the handlers above.
  errno = 0;
  signal(SIGSEGV, sigHandler);
  signal(SIGABRT, sigHandler);
  g_log_set_handler(NULL,
      G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL |
      G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL |
      G_LOG_FLAG_RECURSION, logHandler, NULL);
  g_log_set_handler("Gtk",
      G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL |
      G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL |
      G_LOG_FLAG_RECURSION, logHandler, NULL);
  g_log_set_handler("GLib",
      G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL |
      G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL |
      G_LOG_FLAG_RECURSION, logHandler, NULL);
  // Just guessing that there is a handler for the Gdk API too.
  g_log_set_handler("Gdk",
      G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL |
      G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL |
      G_LOG_FLAG_RECURSION, logHandler, NULL);
#endif


  qsApp = g_malloc0(sizeof(*qsApp));
  qsApp->timer = qsTimer_create();

  /*****************************
   * initialization of options *
   *****************************/

  qsApp->op_fade = TRUE;
  qsApp->op_fadePeriod = 1;
  qsApp->op_fadeDelay = 0;

  qsApp->op_gridLineWidth = 1.73F;
  qsApp->op_axisLineWidth = 2.3F;
  qsApp->op_subGridLineWidth = 3.2F;
  qsApp->op_tickLineWidth = 2.7F;
  qsApp->op_tickLineLength = 18.4F;

  /* in pixels.  All must be positive. */
  qsApp->op_gridXMinPixelSpace = 90;
  qsApp->op_gridXWinOffset = 0;
  qsApp->op_gridYMinPixelSpace = 80;
  qsApp->op_gridYWinOffset = 0;


  /* colors values must be >= 0 and <= 1 */
  qsApp->op_bgR = 0.02F;
  qsApp->op_bgG = 0.02F;
  qsApp->op_bgB = 0.1F;

  qsApp->op_gridR = 0.6F;
  qsApp->op_gridG = 0.9F;
  qsApp->op_gridB = 0.6F;

  qsApp->op_axisR = 0.9F;
  qsApp->op_axisG = 0.9F;
  qsApp->op_axisB = 0.6F;

  qsApp->op_tickR = 0.6F;
  qsApp->op_tickG = 0.9F;
  qsApp->op_tickB = 0.6F;

  qsApp->op_subGridR = 0.15F;
  qsApp->op_subGridG = 0.15F;
  qsApp->op_subGridB = 0.23F;

  qsApp->op_savePNG_bgAlpha = 1.0F;
  qsApp->op_savePNG_alpha = 1.0F;

  qsApp->op_bufferFactor = 1.5F;

  qsApp->op_doubleBuffer = TRUE; /* use XPixmap to double buffer */
  qsApp->op_grid = TRUE; /* background grid */
  qsApp->op_axis = TRUE; /* draw X and Y axis */
  qsApp->op_subGrid = TRUE; /* background sub grid */
  qsApp->op_ticks = TRUE;

  qsApp->op_fullscreen = FALSE;
  qsApp->op_showWindowBorder = TRUE;
  qsApp->op_showMenubar = FALSE;
  qsApp->op_showStatusbar = TRUE;

  /*****************************/

  gtk_init(argc, argv);
  return qsApp;
}

void qsApp_main(void)
{
  QS_ASSERT(qsApp);
  QS_ASSERT(qsApp->sources);

  // Make sure we have at least one QsController
  if(!qsApp->controllers)
  {
    // The user never made a QsController
    // so we'll take a default action here.
    qsInterval_create(1.0/60.0);
  }

  // We require that all QsSources have a QsController
  GSList *l;
  for(l = qsApp->sources; l; l = l->next)
  {
    struct QsSource *s;
    s = l->data;
    QS_ASSERT(s);
    if(!s->controller)
    {
      QS_ASSERT(qsApp->controllers);
      QS_ASSERT(qsApp->controllers->data);

      qsController_appendSource(
          (struct QsController *) qsApp->controllers->data,
          s, NULL); // append to end of first controller
      // This can give bad results if the source was
      // dependent on another source that is listed
      // later in the qsApp->sources list.
    }
  }

  GSList *w;
  for(w = qsApp->wins; w; w = w->next)
  {
    struct QsWin *win;
    win = w->data;
    _qsAdjusterList_display((struct QsAdjusterList *) win);

    // Check that all traces in this window have
    // a source that causes it to draw.
    GSList *t;
    for(t = win->traces; t; t = t->next)
    {
      struct QsTrace *trace;
      trace = t->data;
      struct QsIterator2 *it;
      it = trace->it;
      struct QsSource *xs, *ys;
      xs = it->source0;
      ys = it->source1;
      if(!trace->drawSources)
      {
        // This trace has nothing causing it to be
        // drawn.
        QS_ASSERT(xs->controller);
        QS_ASSERT(ys->controller);
        if(xs->controller == ys->controller)
        {
          // both sources are using the same controller
          // to call their _qsSource_read() function,
          // so we assume that we want the later source
          // in the controller list to trigger the trace
          // drawing.
          struct QsSource *lastSource = NULL;
          for(l = xs->controller->sources; l; l = l->next)
            if(l->data == xs || l->data == ys)
              lastSource = l->data;

          QS_ASSERT(lastSource);
          qsSource_addTraceDraw(lastSource, trace);
        }
        else
        {
          QS_VASSERT(0, "we have a trace with sources that have different"
              "QsController(s):\n"
              "you must call qsSource_addTraceDraw(source, trace)"
              " after trace = qsTrace_create()"
              " to choose a source to cause the trace drawing.\n");
          // Don't let them run the scope with an unused trace.
          fprintf(stderr, "Using the traces X Source to cause the draw.\n");
          qsSource_addTraceDraw(xs, trace);
        }
      }
    }
  }

  gtk_main();
}

void qsApp_destroy(void)
{
  QS_ASSERT(qsApp);

  // Destroy all things Quickscope:

  while(qsApp->wins)
    qsWin_destroy((struct QsWin *) qsApp->wins->data);

  while(qsApp->controllers)
    qsController_destroy((struct QsController *) qsApp->controllers->data);

  while(qsApp->sources)
    qsSource_destroy((struct QsSource *) qsApp->sources->data);

  qsTimer_destroy(qsApp->timer);

#ifdef QS_DEBUG
  memset(qsApp, 0, sizeof(*qsApp));
#endif
  g_free(qsApp);
  qsApp = NULL;
}

