/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <inttypes.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "timer.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win.h"
#include "win_priv.h"
#include "trace.h"
#include "trace_priv.h"
#include "swipe_priv.h"
#include "quickscope_32.xpm"
#include "imgSaveImage.xpm"


static
gboolean cb_close(GtkWidget *w, GdkEvent *e, struct QsWin *win)
{
  QS_ASSERT(win);
  qsWin_destroy(win);
  if(qsApp->wins == NULL)
    // TODO: Is this what we want to do here?
    // or qsApp_destroy()? or what?  Very hard to
    // answer question.
    gtk_main_quit();

  return TRUE;
}

// This is just call when the drawing area is exposed or resized.
// It's a redraw or initial draw.  The animation drawing happens
// from trace.c.
gboolean _qsWin_cbDraw(GtkWidget *da, cairo_t *cr, struct QsWin *win)
{
  if(win->pixmap)
  {
    XCopyArea(win->dsp, win->pixmap, win->xwin, win->gc,
      0, 0, win->width, win->height, 0, 0);
  }
  else
  {
    setTraceColor(win, win->bg);
    XFillRectangle(win->dsp, win->xwin, win->gc, 0, 0,
        win->width, win->height);
    if(_qsWin_isGridStuff(win))
      _qsWin_drawBackground(win);

    if(win->fade)
      // Draw traces from the fading color buffer
      _qsWin_traceFadeRedraws(win);
    else
      // Draw traces from the trace swipe buffer if we can.
      _qsWin_traceSwipeRedraws(win);
    // else
    // If there are no buffers with traces in them
    // then we can't draw traces in this GTK draw callback.
  }

#ifdef QS_DEBUG
#if 0
static int64_t frameCount = 0;
  if((++frameCount) % 20 == 0)
    printf("%s() %"PRId64"  %Lg frames/sec\n", __func__,
        frameCount, frameCount/qsTimer_get(qsApp->timer));
#endif
#endif


  return TRUE; /* TRUE means the event is handled. */
}

static inline
GtkWidget *create_menu(GtkWidget *menubar,
    GtkAccelGroup *accelGroup, const char *label)
{
  GtkWidget *menuitem, *menu;
  // TODO: figure out how to make the menu able to be
  // squished to zero size when the window is squished
  // to its smallest.
  menuitem = gtk_menu_item_new_with_label(label);
  menu = gtk_menu_new();
  gtk_menu_set_accel_group(GTK_MENU(menu), accelGroup);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuitem);
  gtk_widget_show(menuitem);
  /* The menu is shown when the user activates it. */
  return menu;
}

static
GtkWidget *create_menu_item(GtkWidget *menu,
	 const char *label,
	 const char *pixmap[], const gchar *STOCK,
	 guint key, const char *mnemonic,
	 void (*callback)(GtkWidget*, void*),
	 gpointer data, gboolean is_sensitive)
{

  /* TODO: BUG: How do we get rid of the space before the menu item? */

  GtkWidget *mi, *hbox, *l, *image = NULL;

  mi = gtk_menu_item_new();

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  if(pixmap)
    image = gtk_image_new_from_pixbuf(
	gdk_pixbuf_new_from_xpm_data(pixmap));
  else if(STOCK)
    image = gtk_image_new_from_icon_name(STOCK, GTK_ICON_SIZE_MENU);

  if(image)
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 1);

  QS_ASSERT(label);
  l = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(l), label);
  gtk_box_pack_start(GTK_BOX(hbox), l, FALSE, TRUE, 1);

  if(mnemonic)
  {
    l = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(l), mnemonic);
    gtk_box_pack_end(GTK_BOX(hbox), l, FALSE, FALSE, 1);
  }
 
  //printf("gtk_bin_get_child(GTK_BIN(mi))=%p\n", gtk_bin_get_child(GTK_BIN(mi)));
  
  gtk_container_add(GTK_CONTAINER(mi), hbox);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

  if(callback)
    g_signal_connect(G_OBJECT(mi), "activate", G_CALLBACK(callback), data);

  gtk_widget_set_sensitive(mi, is_sensitive);
  gtk_widget_show_all(mi);
  return mi;
}

static inline
void create_menu_item_seperator(GtkWidget *menu)
{
  GtkWidget *mi;
  mi = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
  gtk_widget_show(mi);
}

static
GtkWidget *create_check_menu_item(GtkWidget *menu, const char *label,
    guint key, gboolean is_checked,
    void (*callback)(GtkWidget*, void*),
    gpointer data)
{
  GtkWidget *mi;
  mi = gtk_check_menu_item_new_with_mnemonic(label);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), is_checked);

  if(key)
    gtk_widget_add_accelerator(mi, "activate",
      gtk_menu_get_accel_group(GTK_MENU(menu)),
      key, (GdkModifierType) 0x0, GTK_ACCEL_VISIBLE);

  g_signal_connect(G_OBJECT(mi), "toggled", G_CALLBACK(callback), data);

  gtk_widget_show(mi);
  return mi;
}

void _qsWin_makeGtkWidgets(struct QsWin *win)
{
  /*******************************************************************
   *             GTK+ Widgets
   *******************************************************************/
  GtkWidget *w;
  GdkPixbuf *pixbuf;
  GtkAccelGroup *accelGroup;

  win->win = w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(w), "Quickscope");
  gtk_window_set_icon(GTK_WINDOW(win->win),
	      pixbuf = gdk_pixbuf_new_from_xpm_data(quickscope_32));
  g_object_unref(G_OBJECT(pixbuf));
  gtk_window_set_default_size(GTK_WINDOW(w), 600, 460);
  g_signal_connect(w, "delete_event", G_CALLBACK(cb_close), win);
  g_signal_connect(G_OBJECT(w), "key-press-event",
      G_CALLBACK(ecb_keyPress), win);
  gtk_container_set_border_width(GTK_CONTAINER(w), 0);

  accelGroup = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(w), accelGroup);

  /**************************************************************************
   *               Main VBox
   **************************************************************************/
  {
    GtkWidget *vbox;
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* We get the statusbar now so we can pass this pointer to
     * setup the veiwStatusbar callback. */
    win->statusbar = gtk_label_new("");
    
    /**************************************************************************
     *              Top Menu Bar 
     **************************************************************************/
    {
      GtkWidget *menubar, *menu;

      win->menubar =
        menubar = gtk_menu_bar_new();
      gtk_menu_bar_set_pack_direction(GTK_MENU_BAR(menubar),
          GTK_PACK_DIRECTION_LTR);

      gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    
      /************************************************************************
       *              File Menu 
       ************************************************************************/
      menu = create_menu(menubar, accelGroup, "File");
      create_menu_item(menu,
          "Save PNG <span underline=\"single\">I</span>mage File",
          imgSaveImage, NULL, GDK_KEY_I, "I",
          (void (*)(GtkWidget*, gpointer)) cb_savePNG,
          win, TRUE);
      create_menu_item(menu,
          "<span underline=\"single\">Q</span>uit",
          NULL, "quit", GDK_KEY_Q, "Q",
          (void (*)(GtkWidget*, gpointer)) cb_quit,
          NULL, TRUE);
      create_menu_item_seperator(menu);

      /************************************************************************
       *              View Menu 
       ************************************************************************/
      menu = create_menu(menubar, accelGroup, "View");
      win->viewMenubar =
        create_check_menu_item(menu, "_Menu Bar", GDK_KEY_M,
        qsApp->op_showMenubar,
        (void (*)(GtkWidget*, gpointer)) cb_viewMenuItem, menubar);
      win->viewStatusbar =
        create_check_menu_item(menu, "_Status Bar", GDK_KEY_S,
        qsApp->op_showStatusbar,
        (void (*)(GtkWidget*, gpointer)) cb_viewMenuItem, win->statusbar);
      win->viewWindowBorder =
        create_check_menu_item(menu, "Window _Border", GDK_KEY_B,
        qsApp->op_showWindowBorder,
        (void (*)(GtkWidget*, gpointer)) cb_viewWindowBorder, w);
      win->viewFullscreen =
        create_check_menu_item(menu, "_Full Screen", GDK_KEY_F,
        qsApp->op_fullscreen,
        (void (*)(GtkWidget*, gpointer)) cb_viewFullscreen, w);


      /************************************************************************
       *              Help Menu 
       ************************************************************************/

      // TODO


      if(qsApp->op_showMenubar)
        gtk_widget_show(menubar);
    }
    /**************************************************************************
     *              Drawing Area 
     **************************************************************************/
    {
      GtkWidget *da;
      win->da = da = gtk_drawing_area_new();
      gtk_widget_set_double_buffered(da, FALSE);
      g_signal_connect(G_OBJECT(da),"configure-event",
          G_CALLBACK(_qsWin_cb_configure), win);
      g_signal_connect(G_OBJECT(da), "draw", G_CALLBACK(_qsWin_cbDraw), win);
      //gtk_widget_set_app_paintable(da, TRUE);
      //gtk_widget_override_background_color(da, 0xFF, &win->bgColor);
      gtk_box_pack_start(GTK_BOX(vbox), da, TRUE, TRUE, 0);
      gtk_widget_show(da);
    }
    /**************************************************************************
     *                Status Bar
     **************************************************************************/
    {
      PangoFontDescription *pfd;
      // tried label in place of entry but it made the window not get
      // smaller than the width of the label text
      //win->statusbar = gtk_label_new("");
      
      gtk_label_set_ellipsize(GTK_LABEL(win->statusbar), PANGO_ELLIPSIZE_END);
      gtk_label_set_use_markup(GTK_LABEL(win->statusbar), TRUE);
      pfd = pango_font_description_from_string("Monospace 11");
      if(pfd)
      {
       gtk_widget_override_font(win->statusbar, pfd);
       pango_font_description_free(pfd);
      }
      gtk_label_set_markup(GTK_LABEL(win->statusbar), "status bar");
      gtk_box_pack_start(GTK_BOX(vbox), win->statusbar, FALSE, FALSE, 0);
      if(qsApp->op_showStatusbar)
        gtk_widget_show(win->statusbar);
    }

    gtk_container_add(GTK_CONTAINER(w), vbox);
    gtk_widget_show(vbox);
  }
  /**************************************************************************
   *               End of Main VBox
   **************************************************************************/

  if(qsApp->op_fullscreen)
    gtk_window_fullscreen(GTK_WINDOW(w));
  if(!qsApp->op_showWindowBorder)
    gtk_window_set_decorated(GTK_WINDOW(w), FALSE);
  gtk_widget_show(w);
}
