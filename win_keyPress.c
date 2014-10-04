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
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include "debug.h"
#include "assert.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win.h"
#include "win_priv.h"
#include "trace.h"
#include "trace_priv.h"
#include "swipe_priv.h"


gboolean cb_quit(GtkWidget *w, gpointer data)
{
  while(qsApp->wins)
    qsWin_destroy((struct QsWin *)qsApp->wins->data);
  gtk_main_quit();
  return TRUE;
}

gboolean cb_savePNG(GtkWidget *w, struct QsWin *win)
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
    return TRUE;
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
    return TRUE;
  }
  cairo_surface_destroy(surface);


  GtkWidget *dialog;
  GtkFileFilter *filter;
  //const char *name = "x.png";

  dialog = gtk_file_chooser_dialog_new ("Save PNG File",
		  GTK_WINDOW(win->win),
		  GTK_FILE_CHOOSER_ACTION_SAVE,
                  ("_Cancel"), GTK_RESPONSE_CANCEL,
		  //GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		  //GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
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
      GTK_FILE_CHOOSER(dialog), TRUE);
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

  if(filename)
  {
    // copy tempfilename to filename
    int fd = open(filename, O_WRONLY, O_CREAT|O_TRUNC);
    if(fd < 0)
      fprintf(stderr, "open(\"%s\", O_WRONLY, O_CREAT|O_TRUNC) failed:"
          " errno=%d: %s\n", filename, errno, strerror(errno));
    else
    {
      char buf[4096];
      while (1)
      {
        ssize_t result = read(tmpFD, &buf[0], sizeof(buf));
        if(result == 0)
        {
          fprintf(stderr, "Wrote PNG file \"%s\" %s\n",
              filename, feedbackStr);
          break;
        }
        else if(result < 0)
        {
          fprintf(stderr, "read(\"%s\", O_WRONLY) failed:"
              " errno=%d: %s\n", tempfilename, errno, strerror(errno));
          unlink(filename);
          break;
        }
        if(write(fd, &buf[0], result) != result)
        {
          fprintf(stderr, "open(\"%s\", O_WRONLY) failed:"
            " errno=%d: %s\n", filename, errno, strerror(errno));
          unlink(filename);
          break;
        }
      }
      close(fd);
    }
    g_free(filename);
  }

  // Clean up this F-ing mess.

  close(tmpFD);
  unlink(tempfilename);
  
  if(feedbackStr)
    g_free(feedbackStr);

  if(data)
    g_free(data);

  return TRUE;
}

static inline
gboolean flipViewCheckMenuItem(GtkWidget *w)
{
  gtk_check_menu_item_set_active(
    GTK_CHECK_MENU_ITEM(w),
    gtk_check_menu_item_get_active(
      GTK_CHECK_MENU_ITEM(w))?FALSE:TRUE);
  return TRUE;
}

static inline
gboolean checkViewMenuItem(GtkWidget *w)
{
  return gtk_check_menu_item_get_active(
      GTK_CHECK_MENU_ITEM(w));
}

gboolean cb_viewMenuItem(GtkWidget *menuItem, GtkWidget *w)
{
  if(checkViewMenuItem(menuItem))
    gtk_widget_show(w);
  else
    gtk_widget_hide(w);
  return TRUE;
}

gboolean cb_viewFullscreen(GtkWidget *menuItem, GtkWidget *win)
{
  if(checkViewMenuItem(menuItem))
    gtk_window_fullscreen(GTK_WINDOW(win));
  else
     gtk_window_unfullscreen(GTK_WINDOW(win));
  return TRUE;
}

gboolean cb_viewWindowBorder(GtkWidget *menuItem, GtkWidget *win)
{
  if(checkViewMenuItem(menuItem))
    gtk_window_set_decorated(GTK_WINDOW(win), TRUE);
  else
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
  return TRUE;
}

gboolean ecb_keyPress(GtkWidget *w, GdkEvent *e, struct QsWin *win)
{
  switch(e->key.keyval)
  {
    case GDK_KEY_B:
    case GDK_KEY_b:
      return flipViewCheckMenuItem(win->viewWindowBorder);
      break;
    case GDK_KEY_Escape:
    case GDK_KEY_D:
    case GDK_KEY_d:
      if(qsApp->wins->next) // more than one in list
        qsWin_destroy(win);
      return TRUE;
      break;
    case GDK_KEY_F:
    case GDK_KEY_f:
    case GDK_KEY_F11:
      return flipViewCheckMenuItem(win->viewFullscreen);
      break;
    case GDK_KEY_I:
    case GDK_KEY_i:
      cb_savePNG(NULL, win);
      return TRUE;
      break;
    case GDK_KEY_n:
    case GDK_KEY_N:
    case GDK_KEY_J:
    case GDK_KEY_j:
      if(checkViewMenuItem(win->viewStatusbar))
        _qsWidget_next(win->adjsWidget);
      return TRUE;
      break;
    case GDK_KEY_m:
    case GDK_KEY_M:
      return flipViewCheckMenuItem(win->viewMenubar);
      break;
    case GDK_KEY_s:
    case GDK_KEY_S:
      return flipViewCheckMenuItem(win->viewStatusbar);
      break;
    case GDK_KEY_K:
    case GDK_KEY_k:
    case GDK_KEY_P:
    case GDK_KEY_p:
      if(checkViewMenuItem(win->viewStatusbar))
        _qsWidget_prev(win->adjsWidget);
      return TRUE;
      break;
    case GDK_KEY_Z:
    case GDK_KEY_z:
      win->freezeDisplay = win->freezeDisplay?FALSE:TRUE;
      if(!win->freezeDisplay)
        _qsWin_unfreeze(win);
      return TRUE;
      break;
    case GDK_KEY_Q:
    case GDK_KEY_q:
      cb_quit(NULL, NULL);
      return TRUE;
      break;
    case GDK_KEY_Left:
    case GDK_KEY_leftarrow:
      if(checkViewMenuItem(win->viewStatusbar))
        _qsWidget_shiftLeft(win->adjsWidget);
      return TRUE;
      break;
    case GDK_KEY_Right:
    case GDK_KEY_rightarrow:
      if(checkViewMenuItem(win->viewStatusbar))
          _qsWidget_shiftRight(win->adjsWidget);
      return TRUE;
      break;
    case GDK_KEY_Up:
    case GDK_KEY_uparrow:
      if(checkViewMenuItem(win->viewStatusbar))
        _qsWidget_inc(win->adjsWidget);
      return TRUE;
      break;
    case GDK_KEY_Down:
    case GDK_KEY_downarrow:
      if(checkViewMenuItem(win->viewStatusbar))
        _qsWidget_dec(win->adjsWidget);
      return TRUE;
      break;
  }
  return FALSE;
}
