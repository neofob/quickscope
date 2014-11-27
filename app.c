/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "timer_priv.h"
#include "app.h"
#include "app_priv.h"
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

static void
_qsApp_freeArgv(void)
{
  if(qsApp && qsApp->argv)
  {
    char **argv;
    argv = qsApp->argv;
    while(*argv)
      g_free(*argv++);
    g_free(qsApp->argv);
    qsApp->argv = NULL;
  }
}

struct QsApp *qsApp_init(int *argc, char ***argv)
{
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


  if(!qsApp)
  {
    qsApp = g_malloc0(sizeof(*qsApp));
    qsApp->timer = _qsTimer_create();
  }
  else
  {
    _qsApp_freeArgv();
#ifdef QS_DEBUG
    // We don't re-initialize the timer, because
    // we don't like time to go backwards.
    memset(qsApp, 0, sizeof(*qsApp));
#endif
  }

  QS_ASSERT(qsApp);
  QS_ASSERT(qsApp->timer);

  /*****************************
   * initialization of options *
   *****************************/

  qsApp->op_sourceRequireController = true;
  
  qsApp->op_fade = true;
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

  qsApp->op_doubleBuffer = true; /* use XPixmap to double buffer */
  qsApp->op_grid = true; /* background grid */
  qsApp->op_axis = true; /* draw X and Y axis */
  qsApp->op_subGrid = true; /* background sub grid */
  qsApp->op_ticks = true;

  qsApp->op_fullscreen = false;
  qsApp->op_showWindowBorder = true;
  qsApp->op_showMenubar = false;
  qsApp->op_showControlbar = true;
  qsApp->op_showStatusbar = false;

  qsApp->op_defaultIntervalPeriod = 1.0/60.0;
  qsApp->op_exitOnNoSourceWins = true;

  // win geometry
  qsApp->op_x = INT_MAX;
  qsApp->op_x = INT_MAX;
  qsApp->op_width = 600;
  qsApp->op_height = 460;
  
  /*****************************/

  if(argc && *argc && argv && *argv)
  {
    qsApp->argv = g_malloc(sizeof(char *)*(*argc+1));
    int i;
    for(i=0; i<(*argc); ++i)
      qsApp->argv[i] = g_strdup((*argv)[i]);
    qsApp->argv[i] = NULL;
  }

  if(gtk_main_level() == 0 &&
      !gtk_init_check(argc, argv))
    qsApp_destroy();

  return qsApp;
}

// We record the state of the call stack
// so that the user can call any combo of
// qsApp_init(), qsApp_main() and qsApp_destroy()
// any number of times without crashing.

// QS_APP_INGTKMAIN and QS_APP_NEEDAPPDESTROY lets
// us call qsApp_destroy() and/or gtk_main_quit()
// from within qsApp_main()
#define QS_APP_INGTKMAIN       (1<<0)
#define QS_APP_NEEDAPPDESTROY  (1<<1)
// QS_APP_INAPPMAIN keeps us from reentering qsApp_main()
#define QS_APP_INAPPMAIN       (1<<2)



void qsApp_main(void)
{
  if(!qsApp)
    qsApp_init(NULL, NULL);

  if(qsApp->inAppLevel & QS_APP_INAPPMAIN)
    // stop qsApp_main() re entrance.  Stupid user.
    return;

  qsApp->inAppLevel |= QS_APP_INAPPMAIN;


  if(qsApp->op_sourceRequireController)
  {
    // Make sure we have at least one QsController
    if(!qsApp->controllers && qsApp->sources)
    {
      // The user never made a QsController
      // and we have at least one source,
      // so we'll define a default action here.
      qsInterval_create(qsApp->op_defaultIntervalPeriod);
    }

    // We require that all QsSources have a QsController.
    GSList *l;
    for(l = qsApp->sources; l; l = l->next)
    {
      // Source in this list are from newest to oldest.
      // The oldest and last in this list is the master.
      struct QsSource *s;
      s = l->data;
      QS_ASSERT(s);
      if(!s->controller)
      {
        // The user did not add this source to
        // a controller, so we add it to the default
        // controller.
        QS_ASSERT(qsApp->controllers);
        QS_ASSERT(qsApp->controllers->data);

        qsController_prependSource(
          (struct QsController *) qsApp->controllers->data,
          s, NULL); // append to the start of controller list
        // This can give slower results if the source was
        // dependent on another source that is listed
        // later in the qsApp->sources list.
        // The controllers list of sources will be from
        // oldest source to newest source.
        //
      }
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
        // This trace has nothing causing it to be drawn.
        QS_ASSERT(xs->controller);
        QS_ASSERT(ys->controller);
        if(xs->controller == ys->controller)
        {
          // both sources are using the same controller
          // to call their _qsSource_read() function,
          // so we assume that we want the later source
          // in the controller list to trigger the trace
          // drawing.
          struct QsSource *newestSource = NULL;
          GSList *l;
          for(l = xs->controller->sources; l; l = l->next)
            if(l->data == xs || l->data == ys)
              newestSource = l->data;

          QS_ASSERT(newestSource);
          qsSource_addTraceDraw(newestSource, trace);
          fprintf(stderr,
              "Added trace (id=%d) in win (%p) to source (id=%d) draw pusher.\n",
              trace->id, trace->win, newestSource->id);
        }
        else
        {
          // Assert and fail in the DEBUG case, because what the default
          // should be is not obvious ... yet.
          QS_VASSERT(0, "we have a trace with two sources that have different"
              "QsControllers:\n"
              "you must call qsSource_addTraceDraw(source, trace)"
              " after trace = qsTrace_create()"
              " to choose a source to cause the trace drawing.\n");
          // Don't let them run the scope with an unused trace.
          // Works this way by default.  We could use the newer source
          // instead of just using the X source.
          fprintf(stderr,
              "Using the traces X Source to cause the draw:\n"
              "Added trace (id=%d) in win (%p) to source (id=%d) draw pusher.\n",
              trace->id, trace->win, xs->id);
          qsSource_addTraceDraw(xs, trace);
        }
      }
    }
  }



  // We keep from calling gtk_main() twice:
  if(gtk_main_level() == 0)
  {
    qsApp->inAppLevel |= QS_APP_INGTKMAIN;
    gtk_main();
    qsApp->inAppLevel &= ~QS_APP_INGTKMAIN;

    if(qsApp->inAppLevel & QS_APP_NEEDAPPDESTROY)
    {
      qsApp_destroy();
      return;
    }
  }

  qsApp->inAppLevel &= ~QS_APP_INAPPMAIN;
}

void qsApp_destroy(void)
{
  if(!qsApp)
    // Let qsApp_destroy() be called many times.
    return;

  if(qsApp->inAppLevel & QS_APP_INGTKMAIN)
  {
    // We are calling qsApp_destroy() from within
    // gtk_main() within qsApp_main().

    // Flag that we need to call qsApp_destroy again.
    qsApp->inAppLevel |= QS_APP_NEEDAPPDESTROY;

    if(gtk_main_level())
      // This will get us out of gtk_main()
      gtk_main_quit();

    return; // We will hopefully return to within
    // qsApp_main() at the gtk_main() call.
  }

  // Destroy all things Quickscope:

  while(qsApp->sources)
    // Because sources can depend on other sources, we
    // destroy sources in reverse order of creation.
    // We prepended sources to this list as the sources
    // where created in qsSource_create().
    qsSource_destroy((struct QsSource *) qsApp->sources->data);

  while(qsApp->controllers)
    qsController_destroy((struct QsController *) qsApp->controllers->data);

  while(qsApp->wins)
    qsWin_destroy((struct QsWin *) qsApp->wins->data);

  _qsApp_freeArgv();

  _qsTimer_destroy(qsApp->timer);

#ifdef QS_DEBUG
  memset(qsApp, 0, sizeof(*qsApp));
#endif
  g_free(qsApp);
  qsApp = NULL;
}

void _qsApp_checkDestroy()
{
  QS_ASSERT(qsApp);
  if(qsApp->inAppLevel & QS_APP_INGTKMAIN)
  {
    // we are in a call to gtk_main()
    // in a call to qsApp_main()
    if(!qsApp->sources && !qsApp->wins &&
        qsApp->op_exitOnNoSourceWins)
    {
      // We have no sources or windows
      qsApp_destroy();
    }
  }
}
