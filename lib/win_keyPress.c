/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <paths.h>
#include <errno.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win.h"
#include "win_priv.h"
#include "trace.h"
#include "trace_priv.h"
#include "swipe_priv.h"


bool cb_quit(GtkWidget *w, gpointer data)
{
  while(qsApp->wins)
    qsWin_destroy((struct QsWin *)qsApp->wins->data);
  gtk_main_quit();
  return true;
}

bool cb_savePNG(GtkWidget *w, struct QsWin *win)
{
  // GTK+ kind of sucks!!!  Here's why ...

  char *filename = NULL;
  QS_ASSERT(win);
  cairo_surface_t *surface;
  char *feedbackStr = NULL;
  uint32_t *data = NULL;
  int tmpFD;
  char tempfilename[] = _PATH_TMP "Quickscope_png_XXXXXXX";
  // We create a cairo_surface and write it to a file before we pop up a
  // window that may interfere with the current drawing area X11 window.
  // It appears that GTK+ draws to this surface when we pop up the dialog,
  // so we write it to a temp file before we pop up the dialog.
  surface = _qsWin_savePNG(win, &feedbackStr, &data);
  errno = 0;
  if(-1 == (tmpFD = mkstemp(tempfilename)))
  {
    fprintf(stderr, "mkstemp(\"%s\") failed: errno=%d: %s\n",
        tempfilename, errno, strerror(errno));
    return true;
  }
  // We don't know the filename yet, so we use a temp file.
  // Popping up the dialog fucks up the cairo_surface so we
  // must save it before the dialog pops up. 
  if(CAIRO_STATUS_SUCCESS != cairo_surface_write_to_png(surface, tempfilename))
  {
    close(tmpFD);
    unlink(tempfilename);
    fprintf(stderr, "cairo_surface_write_to_png(,\"%s\") failed:"
          " errno=%d: %s\n", tempfilename, errno, strerror(errno));
    return true;
  }
  cairo_surface_destroy(surface);
  g_free(data);

QS_SPEW("saved tempfile image: \"%s\"\n", tempfilename);


  GtkWidget *dialog;
  GtkFileFilter *filter;
  //const char *name = "x.png";

  dialog = gtk_file_chooser_dialog_new ("Save PNG File",
		  GTK_WINDOW(win->win),
		  GTK_FILE_CHOOSER_ACTION_SAVE,
                  "_Cancel", GTK_RESPONSE_CANCEL,
                  "_Save", GTK_RESPONSE_ACCEPT,
                  NULL);
  
  filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "PNG Image File");
  gtk_file_filter_add_pattern(filter, "*.png");
  gtk_file_filter_add_pattern(filter, "*.PNG");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "All Files");
  gtk_file_filter_add_pattern(filter, "*");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);


  gtk_file_chooser_set_do_overwrite_confirmation(
      GTK_FILE_CHOOSER(dialog), true);
  //gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), name);
  // We assume that GTK+ will not call ecb_keyPress() while
  // we are in this function.  Should be a good assumption.


  // Looks like gtk_dialog_run() will spawns a thread.
  // The source read callbacks from qsInterval will continue
  // while this dialog runs, drawing new trace lines and/or points.
  // It used to draw to the cairo_surface from above, but
  // it can't now given we destroyed the surface and wrote it
  // to a temp file.

  if(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG (dialog)))
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
  gtk_widget_destroy(dialog);

  QS_SPEW("coping \"%s\" to \"%s\"\n", tempfilename, filename);

  if(filename)
  {
    // copy tempfilename to filename
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC,
        S_IRUSR|S_IWUSR |S_IRGRP|S_IWGRP |S_IROTH|S_IWOTH);
    if(fd < 0)
      fprintf(stderr, "open(\"%s\", O_WRONLY|O_CREAT|O_TRUNC) failed:"
          " errno=%d: %s\n", filename, errno, strerror(errno));
    else
    {
      char buf[4096];
      while (1)
      {
        ssize_t nrd = read(tmpFD, buf, sizeof(buf));
        if(nrd == 0)
        {
          fprintf(stderr, "Wrote PNG file \"%s\" %s\n",
              filename, feedbackStr);
          break;
        }
        else if(nrd < 0)
        {
          fprintf(stderr, "read(\"%s\" fd=%d,,) failed:"
              " errno=%d: %s\n", tempfilename, tmpFD,
              errno, strerror(errno));
          unlink(filename);
          break;
        }

        ssize_t n, nwr;
        for(n = nrd; n; n -= nwr)
          if((nwr = write(fd, &buf[nrd - n], n)) < 1)
          {
            fprintf(stderr, "write(\"%s\" fd=%d,,)=%zd "
                "failed to write a byte:"
                " errno=%d: %s\n", filename, fd, nwr,
                errno, strerror(errno));
            unlink(filename);
            break;
          }
        if(n) break; // failed to write all n
      }
      close(fd);
    }
    g_free(filename);
  }

  // Clean up

  close(tmpFD);
  unlink(tempfilename);
  
  if(feedbackStr)
    g_free(feedbackStr);

  return true;
}

static inline
bool flipViewCheckMenuItem(GtkWidget *w)
{
  gtk_check_menu_item_set_active(
    GTK_CHECK_MENU_ITEM(w),
    gtk_check_menu_item_get_active(
      GTK_CHECK_MENU_ITEM(w))?false:true);
  return true;
}

static inline
bool checkViewMenuItem(GtkWidget *w)
{
  return gtk_check_menu_item_get_active(
      GTK_CHECK_MENU_ITEM(w));
}

bool cb_viewStatusbar(GtkWidget *menuItem, struct QsWin *win)
{
  if(checkViewMenuItem(menuItem))
  {
    _qsWin_updateStatusbar(win);
    gtk_widget_show(win->statusbar);
  }
  else
    gtk_widget_hide(win->statusbar);
  return true;
}

bool cb_viewMenuItem(GtkWidget *menuItem, GtkWidget *w)
{
  if(checkViewMenuItem(menuItem))
    gtk_widget_show(w);
  else
    gtk_widget_hide(w);
  return true;
}

bool cb_viewFullscreen(GtkWidget *menuItem, GtkWidget *win)
{
  if(checkViewMenuItem(menuItem))
    gtk_window_fullscreen(GTK_WINDOW(win));
  else
     gtk_window_unfullscreen(GTK_WINDOW(win));
  return true;
}

bool cb_viewWindowBorder(GtkWidget *menuItem, GtkWidget *win)
{
  if(checkViewMenuItem(menuItem))
    gtk_window_set_decorated(GTK_WINDOW(win), true);
  else
    gtk_window_set_decorated(GTK_WINDOW(win), false);
  return true;
}

bool ecb_keyPress(GtkWidget *w, GdkEvent *e, struct QsWin *win)
{
  switch(e->key.keyval)
  {
    case GDK_KEY_B:
    case GDK_KEY_b:
      return flipViewCheckMenuItem(win->viewWindowBorder);
      break;
    case GDK_KEY_c:
    case GDK_KEY_C:
      return flipViewCheckMenuItem(win->viewControlbar);
      break;
    case GDK_KEY_s:
    case GDK_KEY_S:
      return flipViewCheckMenuItem(win->viewStatusbar);
      break;
    case GDK_KEY_Escape:
    case GDK_KEY_D:
    case GDK_KEY_d:
      if(qsApp->wins->next) // more than one in list
        qsWin_destroy(win);
      return true;
      break;
    case GDK_KEY_F:
    case GDK_KEY_f:
    case GDK_KEY_F11:
      return flipViewCheckMenuItem(win->viewFullscreen);
      break;
    case GDK_KEY_I:
    case GDK_KEY_i:
      cb_savePNG(NULL, win);
      return true;
      break;
    case GDK_KEY_n:
    case GDK_KEY_N:
    case GDK_KEY_J:
    case GDK_KEY_j:
      if(checkViewMenuItem(win->viewControlbar))
        _qsWidget_next(win->adjsWidget);
      return true;
      break;
    case GDK_KEY_m:
    case GDK_KEY_M:
      return flipViewCheckMenuItem(win->viewMenubar);
      break;
    case GDK_KEY_K:
    case GDK_KEY_k:
    case GDK_KEY_P:
    case GDK_KEY_p:
      if(checkViewMenuItem(win->viewControlbar))
        _qsWidget_prev(win->adjsWidget);
      return true;
      break;
    case GDK_KEY_Z:
    case GDK_KEY_z:
      qsApp->freezeDisplay = qsApp->freezeDisplay?false:true;
      if(!qsApp->freezeDisplay)
      {
        GSList *l;
        for(l=qsApp->wins; l; l=l->next)
          _qsWin_unfreeze((struct QsWin *) l->data);
      }
      return true;
      break;
    case GDK_KEY_Q:
    case GDK_KEY_q:
      cb_quit(NULL, NULL);
      return true;
      break;
    case GDK_KEY_Left:
    case GDK_KEY_leftarrow:
      if(checkViewMenuItem(win->viewControlbar))
        _qsWidget_shiftLeft(win->adjsWidget);
      return true;
      break;
    case GDK_KEY_Right:
    case GDK_KEY_rightarrow:
      if(checkViewMenuItem(win->viewControlbar))
          _qsWidget_shiftRight(win->adjsWidget);
      return true;
      break;
    case GDK_KEY_Up:
    case GDK_KEY_uparrow:
      if(checkViewMenuItem(win->viewControlbar))
        _qsWidget_inc(win->adjsWidget);
      return true;
      break;
    case GDK_KEY_Down:
    case GDK_KEY_downarrow:
      if(checkViewMenuItem(win->viewControlbar))
        _qsWidget_dec(win->adjsWidget);
      return true;
      break;
  }
  return false;
}
