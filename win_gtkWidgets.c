/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include "debug.h"
#include "assert.h"
#include "base.h"
#include "timer.h"
#include "app.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "win.h"
#include "win_priv.h"
#include "trace.h"
#include "drawsync.h"
#include "quickscope_32.xpm"
#include "imgSaveImage.xpm"


static inline
GtkWidget *create_menu(GtkWidget *menubar,
    GtkAccelGroup *accelGroup, const char *label)
{
  GtkWidget *menuitem, *menu;
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
	 guint key, gboolean with_mnemonic,
	 void (*callback)(GtkWidget*, void*),
	 gpointer data, gboolean is_sensitive)
{
  GtkWidget *mi;

  if(with_mnemonic)
    mi = gtk_image_menu_item_new_with_mnemonic(label);
  else
    mi =  gtk_image_menu_item_new_with_label(label);

  if(pixmap)
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mi),
      gtk_image_new_from_pixbuf(
	gdk_pixbuf_new_from_xpm_data(pixmap)));
  else if(STOCK)
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mi),
      gtk_image_new_from_stock(STOCK, GTK_ICON_SIZE_MENU));

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

  if(callback)
    g_signal_connect(G_OBJECT(mi), "activate", G_CALLBACK(callback), data);

  if(key)
    gtk_widget_add_accelerator(mi, "activate",
      gtk_menu_get_accel_group(GTK_MENU(menu)),
      key, (GdkModifierType) 0x0, GTK_ACCEL_VISIBLE);

  gtk_widget_set_sensitive(mi, is_sensitive);
  gtk_widget_show(mi);
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
      create_menu_item(menu, "Save PNG _Image File", imgSaveImage, NULL,
          GDK_KEY_I, TRUE, (void (*)(GtkWidget*, gpointer)) cb_savePNG,
          win, TRUE);
      create_menu_item(menu, "_Quit", NULL, GTK_STOCK_QUIT,
          GDK_KEY_Q, TRUE, (void (*)(GtkWidget*, gpointer)) cb_quit,
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
      g_signal_connect(G_OBJECT(da), "draw", G_CALLBACK(cb_draw), win);
      gtk_widget_set_app_paintable(da, TRUE);
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

