/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

// We define _GNU_SOURCE to declare extra GNU stuff
// like program_invocation_short_name
// but if __USE_GNU does not come to be defined
// in the system header files than we don't get
// the extra GNU stuff and _GNU_SOURCE is ignored.
// glibc users should never define __USE_GNU directly.
#define _GNU_SOURCE
#include <errno.h> // for program_invocation_short_name
#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
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
#include "group.h"
#include "source.h"
#include "iterator.h"
#include "quickscope_32.xpm"
#include "imgSaveImage.xpm"


static
bool cb_close(GtkWidget *w, GdkEvent *e, struct QsWin *win)
{
  QS_ASSERT(win);
  qsWin_destroy(win);
  if(qsApp->wins == NULL)
    // TODO: Is this what we want to do here?
    // or qsApp_destroy()? or what?  Very hard to
    // answer question.
    gtk_main_quit();

  return true;
}

// This is just called when the drawing area is exposed or resized.
// It's a redraw or initial draw.  The animation drawing happens
// from trace.c.
//
// TODO: If GTK+ removes gtk_widget_set_double_buffered() we can
// have this draw to X11 and copy it to the cairo_t thingy.
// This will not be a big performance hit given it is only
// drawing the one, first, frame, and all other scope frames
// will not use this callback, but use libX11 having the draw
// triggered by the traces as they are looping by using whatever
// QsController they are having call their draw calls with the
// QsSource reads.
//
// We need this stupid callback incase the scope is frozen and the window
// is resized, or iconified and then de-iconified.
static
bool _qsWin_cbDraw(GtkWidget *da, cairo_t *cr, struct QsWin *win)
{
  if(win->pixmap)
  {
    static int count = 0;
    ++count;
    XCopyArea(win->dsp, win->pixmap, win->xwin, win->gc,
      0, 0, win->width, win->height, 0, 0);
  }
  else
  {
    setTraceColor(win, win->bg);
    XFillRectangle(win->dsp, win->xwin, win->gc, 0, 0,
        win->width, win->height);
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

  return true; /* true means the event is handled. */
}

// TODO: thread safety
// We assume that there is just one pointer.
// Good for old X11 but things change.
static bool leftButtonDown = false;
static gdouble lastX = 0.0, lastY = 0.0;

#define LEFT_BUTTON  (1)

void _qsWin_updateStatusbar(struct QsWin *win)
{
  const size_t size = 1024;
  char text[size];
  size_t len = 0;
  
  GSList *l;
  for(l=win->traces; l; l=l->next)
  {
    struct QsTrace *t;
    t = l->data;
    QS_ASSERT(t);
    struct QsSource *sx, *sy;
    sx = t->it->source0;
    sy = t->it->source1;
    int xChannel, yChannel;
    xChannel = t->xChannelNum;
    yChannel = t->yChannelNum;
    char *xunit = "", *yunit = "";
    if(sx->units)
      xunit = sx->units[xChannel];
    if(sy->units)
      yunit = sy->units[yChannel];

    if(size > len)
      len += _qsTrace_iconText(&text[len], size - len, t);
    if(size > len)
      len += snprintf(&text[len], size - len,
            "%+3.3g %s %+3.3g %s",
            ((lastX - t->xShiftPix)/t->xScalePix) *
            sx->scale[xChannel] + sx->shift[xChannel] , xunit,
            ((lastY - t->yShiftPix)/t->yScalePix) *
            sy->scale[yChannel] + sy->shift[yChannel], yunit);
    if(len == size)
      break;
  }

  gtk_label_set_markup(GTK_LABEL(win->statusbar), text);
}

static
bool ecbPointerMotion(GtkWidget *w, GdkEvent *event, struct QsWin *win)
{
  QS_ASSERT(event);
  QS_ASSERT(event->type == GDK_MOTION_NOTIFY);

  lastX = event->motion.x;
  lastY = event->motion.y;

  if(!gtk_check_menu_item_get_active(
        GTK_CHECK_MENU_ITEM(win->viewStatusbar)))
    return true;

  _qsWin_updateStatusbar(win);

  //QS_SPEW("x y = %g %g\n", event->motion.x, event->motion.y);

  return true;
}

static
bool ecbButtonPress(GtkWidget *w, GdkEvent *event, struct QsWin *win)

{
  QS_ASSERT(event);
  // This below assertion fails is we click the window a lot.
  //QS_ASSERT(event->type == GDK_BUTTON_PRESS);
  if(event->button.button != LEFT_BUTTON || event->type != GDK_BUTTON_PRESS)
    return false;

  QS_ASSERT(leftButtonDown == false);

  leftButtonDown = true;

  //QS_SPEW("x y = %g %g\n", event->button.x, event->button.y);
  return true;
}

static
bool ecbButtonRelease(GtkWidget *w, GdkEvent *event, struct QsWin *win)
{
  QS_ASSERT(event);
  QS_ASSERT(event->type == GDK_BUTTON_RELEASE);
  if(event->button.button != LEFT_BUTTON)
    return false;

  QS_ASSERT(leftButtonDown == true);


  leftButtonDown = false;

  //QS_SPEW("x y = %g %g\n", event->button.x, event->button.y);
  return true;
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
	 gpointer data, bool is_sensitive)
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
    gtk_box_pack_start(GTK_BOX(hbox), image, false, false, 1);

  QS_ASSERT(label);
  l = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(l), label);
  gtk_box_pack_start(GTK_BOX(hbox), l, false, true, 1);

  if(mnemonic)
  {
    l = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(l), mnemonic);
    gtk_box_pack_end(GTK_BOX(hbox), l, false, false, 1);
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
    guint key, bool is_checked,
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

static inline void getRootWindowSize(void)
{
  GdkWindow *root;
  root = gdk_get_default_root_window();
  QS_ASSERT(root);
  qsApp->rootWindowWidth = gdk_window_get_width(root);
  qsApp->rootWindowHeight = gdk_window_get_height(root);
  QS_ASSERT(qsApp->rootWindowWidth > 0);
  QS_ASSERT(qsApp->rootWindowHeight > 0);
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
  char title[200] = "Quickscope";
#ifdef __USE_GNU  // if using glibc
  snprintf(title, 200, "%s <Quickscope>", program_invocation_short_name);
#endif
  gtk_window_set_title(GTK_WINDOW(w), title);
  gtk_window_set_icon(GTK_WINDOW(win->win),
	      pixbuf = gdk_pixbuf_new_from_xpm_data(quickscope_32));
  g_object_unref(G_OBJECT(pixbuf));

  if(qsApp->rootWindowWidth < 1)
    getRootWindowSize();

  {
    int width, height;
    width = qsApp->op_width;
    height = qsApp->op_height;
    if(width > qsApp->rootWindowWidth)
    {
      QS_SPEW("reducing window width=%d "
          "to root window width=%d\n", width,
          qsApp->rootWindowWidth);
      width = qsApp->rootWindowWidth;
    }
    if(height > qsApp->rootWindowHeight)
    {
      QS_SPEW("reducing window height=%d "
          "to root window height=%d\n", height,
          qsApp->rootWindowHeight);
       height = qsApp->rootWindowHeight;
    }
    gtk_window_set_default_size(GTK_WINDOW(w), width, height);
  }

  if(qsApp->op_x != INT_MAX && qsApp->op_y != INT_MAX)
  {
    int x, y;
    x = qsApp->op_x;
    y = qsApp->op_y;

    if(x >= qsApp->rootWindowWidth)
      x = qsApp->rootWindowWidth - 1;
    else if(x <= -qsApp->rootWindowWidth && x != INT_MIN)
      x = -qsApp->rootWindowWidth - 1;

    if(y >= qsApp->rootWindowHeight)
      y = qsApp->rootWindowHeight - 1;
    else if(y <= -qsApp->rootWindowHeight && y != INT_MIN)
      y = -qsApp->rootWindowHeight - 1;

    if(x == INT_MIN) /* like example: --geometry=600x600-0+0 */
      x = qsApp->rootWindowWidth - qsApp->op_width;
    else if(x < 0)
      x = qsApp->rootWindowWidth - qsApp->op_width + x;
  
    if(y == INT_MIN) /* like example: --geometry=600x600+0-0 */
      y = qsApp->rootWindowHeight - qsApp->op_height;
    else if(y < 0)
      y = qsApp->rootWindowHeight - qsApp->op_height + y;

    QS_SPEW("moving WINDOW to (%d,%d)\n", x, y);

    gtk_window_move(GTK_WINDOW(w), x, y);
  }


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

    /* We get the controlbar now so we can pass this pointer to
     * setup the veiwcontrolbar callback. */
    win->controlbar = gtk_label_new("");
    /* same for statusbar */
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

      gtk_box_pack_start(GTK_BOX(vbox), menubar, false, false, 0);
    
      /************************************************************************
       *              File Menu 
       ************************************************************************/
      menu = create_menu(menubar, accelGroup, "File");
      create_menu_item(menu,
          "Save PNG <span underline=\"single\">I</span>mage File",
          imgSaveImage, NULL, GDK_KEY_I, "I",
          (void (*)(GtkWidget*, gpointer)) cb_savePNG,
          win, true);
      create_menu_item(menu,
          "<span underline=\"single\">Q</span>uit",
          NULL, "quit", GDK_KEY_Q, "Q",
          (void (*)(GtkWidget*, gpointer)) cb_quit,
          NULL, true);
      create_menu_item_seperator(menu);

      /************************************************************************
       *              View Menu 
       ************************************************************************/
      menu = create_menu(menubar, accelGroup, "View");
      win->viewMenubar =
        create_check_menu_item(menu, "_Menu Bar", GDK_KEY_M,
        qsApp->op_showMenubar,
        (void (*)(GtkWidget*, gpointer)) cb_viewMenuItem, menubar);
      win->viewControlbar =
        create_check_menu_item(menu, "_Control Bar", GDK_KEY_C,
        qsApp->op_showControlbar,
        (void (*)(GtkWidget*, gpointer)) cb_viewMenuItem, win->controlbar);
     win->viewStatusbar =
        create_check_menu_item(menu, "_Status Bar", GDK_KEY_S,
        qsApp->op_showStatusbar,
        (void (*)(GtkWidget*, gpointer)) cb_viewStatusbar, win);
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
      // TODO: gtk_widget_set_double_buffered(,false) is depreciated
      // but we still need what it did.
      // We don't want GTK to double buffer, because we are doing
      // that in this quickscope code using the X11 client API,
      // and when GTK does the double buffering it clobbers what we
      // draw when there is a redraw event.  We cannot use Cairo to
      // draw, it's much too slow.

      // Many days of playing with GTK3 source and googling; conclusion
      // there is no "proper" work-around to keep GTK from clobbering things
      // you draw without cairo (Thu Nov 20 12:49:22 EST 2014).
      // Calling gtk_widget_set_double_buffered() is the best I can do.
      gtk_widget_set_double_buffered(da, false);
      gtk_widget_set_events(da,
                  gtk_widget_get_events(da) |
		  GDK_BUTTON_PRESS_MASK |
		  GDK_BUTTON_RELEASE_MASK |
		  GDK_POINTER_MOTION_MASK);

      g_signal_connect(G_OBJECT(da),"configure-event",
          G_CALLBACK(_qsWin_cb_configure), win);
      g_signal_connect(G_OBJECT(da), "draw",
          G_CALLBACK(_qsWin_cbDraw), win);
      g_signal_connect(G_OBJECT(da), "button-press-event",
          G_CALLBACK(ecbButtonPress), win);
      g_signal_connect(G_OBJECT(da), "button-release-event",
          G_CALLBACK(ecbButtonRelease), win);
      g_signal_connect(G_OBJECT(da), "motion-notify-event",
          G_CALLBACK(ecbPointerMotion), win);

      //gtk_widget_override_background_color(da, 0xFF, &win->bgColor);
      gtk_box_pack_start(GTK_BOX(vbox), da, true, true, 0);
      gtk_widget_show(da);
    }
    /**************************************************************************
     *              Control Bar (Scope Parameter adjusters)
     **************************************************************************/
    {
      PangoFontDescription *pfd;
      // tried label in place of entry but it made the window not get
      // smaller than the width of the label text
      
      gtk_label_set_ellipsize(GTK_LABEL(win->controlbar), PANGO_ELLIPSIZE_END);
      gtk_label_set_use_markup(GTK_LABEL(win->controlbar), true);
      pfd = pango_font_description_from_string("Monospace 11");
      if(pfd)
      {
        gtk_widget_override_font(win->controlbar, pfd);
        pango_font_description_free(pfd);
      }
      gtk_label_set_markup(GTK_LABEL(win->controlbar), "control bar");
      gtk_box_pack_start(GTK_BOX(vbox), win->controlbar, false, false, 0);
      //const GdkRGBA rgba = { /*r*/ 1.0, /*g*/ 0.0, /*b*/ 0.0, /*alpha*/ 0.5 };
      //gtk_widget_override_background_color(win->controlbar, 0xFFFFFFFF, &rgba);
      if(qsApp->op_showControlbar)
        gtk_widget_show(win->controlbar);
    }
    /**************************************************************************
     *                Status Bar
     **************************************************************************/
    {
      PangoFontDescription *pfd;
      // tried label in place of entry but it made the window not get
      // smaller than the width of the label text
      
      gtk_label_set_ellipsize(GTK_LABEL(win->statusbar), PANGO_ELLIPSIZE_END);
      gtk_label_set_use_markup(GTK_LABEL(win->statusbar), true);
      pfd = pango_font_description_from_string("Monospace 11");
      if(pfd)
      {
        gtk_widget_override_font(win->statusbar, pfd);
        pango_font_description_free(pfd);
      }
      gtk_label_set_markup(GTK_LABEL(win->statusbar), "status bar");
      gtk_box_pack_start(GTK_BOX(vbox), win->statusbar, false, false, 0);
      if(qsApp->op_showStatusbar)
        gtk_widget_show(win->statusbar);
    }
  /**************************************************************************
   *               End of Main VBox
   **************************************************************************/
    gtk_container_add(GTK_CONTAINER(w), vbox);
    gtk_widget_show(vbox);
  }

  if(qsApp->op_fullscreen)
    gtk_window_fullscreen(GTK_WINDOW(w));
  if(!qsApp->op_showWindowBorder)
    gtk_window_set_decorated(GTK_WINDOW(w), false);
  gtk_widget_show(w);
}
