/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

/* This is a failed attempt at drawing without Cario
 * using libX11 in a modified copy of GtkDrawingArea.
 * gtk_widget_set_double_buffered() is depreciated,
 * but there is no way to draw in a GTK widget without
 * Cario, without the use of gtk_widget_set_double_buffered().
 */
// Many days of playing with GTK+3 source and googling; conclusion
// there is no "proper" work-around to keep GTK+ from clobbering things
// you draw without cairo (Thu Nov 20 12:49:22 EST 2014).
 
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "quickscope.h"

G_BEGIN_DECLS

#define QS_TYPE_DRAWING_AREA            (qs_drawing_area_get_type ())
#define QS_DRAWING_AREA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QS_TYPE_DRAWING_AREA, QsDrawingArea))
#define QS_DRAWING_AREA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), QS_TYPE_DRAWING_AREA, QsDrawingAreaClass))
#define QS_IS_DRAWING_AREA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QS_TYPE_DRAWING_AREA))
#define QS_IS_DRAWING_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), QS_TYPE_DRAWING_AREA))
#define QS_DRAWING_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), QS_TYPE_DRAWING_AREA, QsDrawingAreaClass))


typedef struct _QsDrawingArea       QsDrawingArea;
typedef struct _QsDrawingAreaClass  QsDrawingAreaClass;

struct _QsDrawingArea
{
  GtkWidget widget;

  /*< private >*/
  gpointer dummy;
};

struct _QsDrawingAreaClass
{
  GtkWidgetClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType qs_drawing_area_get_type(void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget* qs_drawing_area_new(void);

G_END_DECLS


static void qs_drawing_area_realize(GtkWidget *widget);
static void qs_drawing_area_size_allocate(GtkWidget *widget,
                            GtkAllocation *allocation);
static void qs_drawing_area_send_configure(QsDrawingArea *darea);

G_DEFINE_TYPE(QsDrawingArea, qs_drawing_area, GTK_TYPE_WIDGET)

struct QsDrawX11Context
{
  Display *dsp;
  Window xwin;
  Colormap cmap;
  GC gc; /* X11 Graphics Context */
  XColor bg; /* background color */
};

static
struct QsDrawX11Context *qsDrawX11Context_create(GtkWidget *w)
{
  struct QsDrawX11Context *x;
  x = g_malloc0(sizeof(*x));
  x->dsp = gdk_x11_get_default_xdisplay();
  QS_ASSERT(x->dsp);
  x->xwin = gdk_x11_window_get_xid(gtk_widget_get_window(w));
  QS_ASSERT(x->xwin);
  x->cmap = DefaultColormap(x->dsp, DefaultScreen(x->dsp));
  QS_ASSERT(x->cmap);
  x->gc = XCreateGC(x->dsp, x->xwin, 0, 0);
  QS_ASSERT(x->gc);
  x->bg.pixel = 0;
  x->bg.red   = 0x0001; /* X11 uses shorts not bytes */
  x->bg.green = 0xFFFF;
  x->bg.blue  = 0xFFFF;
  x->bg.flags = 0;

#ifdef QS_DEBUG
  QS_ASSERT(XAllocColor(x->dsp, x->cmap, &x->bg));
#else
  XAllocColor(x->dsp, x->cmap, &x->bg);
#endif
  
  return x;
}

struct QsDrawX11Context *x = NULL;


static
gboolean cbDraw(GtkWidget *da, cairo_t *cr)
{
#ifdef QS_DEBUG
  static int count = 0;
  QS_SPEW("%d\n", ++count);
#endif
  if(!x)
    x = qsDrawX11Context_create(da);
#if 1
  XSetForeground(x->dsp, x->gc, x->bg.pixel);
#else
  guint width, height;
  GdkRGBA color;
 
  width = gtk_widget_get_allocated_width(da);
  height = gtk_widget_get_allocated_height(da);
  cairo_arc(cr,
             width / 2.0, height / 2.0,
             MIN (width, height) / 2.0,
             0, 2 * G_PI);
  gtk_style_context_get_color(
      gtk_widget_get_style_context(da), 0, &color);
  gdk_cairo_set_source_rgba(cr, &color);
  cairo_fill (cr);
#endif
  return true;
}

static void
qs_drawing_area_class_init(QsDrawingAreaClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

  widget_class->realize = qs_drawing_area_realize;
  widget_class->size_allocate = qs_drawing_area_size_allocate;
  //widget_class->draw = cbDraw;
  gtk_widget_class_set_accessible_role(widget_class, ATK_ROLE_DRAWING_AREA);
}

static void
qs_drawing_area_init(QsDrawingArea *darea)
{
}

GtkWidget *qs_drawing_area_new(void)
{
  return g_object_new(QS_TYPE_DRAWING_AREA, NULL);
}

static void
qs_drawing_area_realize(GtkWidget *widget)
{
  GtkAllocation allocation;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  if (!gtk_widget_get_has_window(widget))
    {
      GTK_WIDGET_CLASS(qs_drawing_area_parent_class)->realize (widget);
    }
  else
    {
      gtk_widget_set_realized(widget, TRUE);

      gtk_widget_get_allocation(widget, &allocation);

      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.x = allocation.x;
      attributes.y = allocation.y;
      attributes.width = allocation.width;
      attributes.height = allocation.height;
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.visual = gtk_widget_get_visual (widget);
      attributes.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;
      //attributes.event_mask = gtk_widget_get_events(widget);

      attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

      window = gdk_window_new(gtk_widget_get_parent_window(widget),
          &attributes, attributes_mask);
      gtk_widget_register_window(widget, window);
      gtk_widget_set_window(widget, window);

      gtk_style_context_set_background(gtk_widget_get_style_context(widget), window);
    }

  qs_drawing_area_send_configure(QS_DRAWING_AREA(widget));
}

static void
qs_drawing_area_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
  g_return_if_fail(QS_IS_DRAWING_AREA(widget));
  g_return_if_fail(allocation != NULL);

  gtk_widget_set_allocation(widget, allocation);

  if(gtk_widget_get_realized(widget))
  {
    if(gtk_widget_get_has_window(widget))
        gdk_window_move_resize(gtk_widget_get_window (widget),
                        allocation->x, allocation->y,
                        allocation->width, allocation->height);

      qs_drawing_area_send_configure(QS_DRAWING_AREA(widget));
    }
}

static
void Configure(GtkWidget *da)
{
QS_SPEW("before\n");
  //g_signal_connect(G_OBJECT(gtk_widget_get_window(da)), "expose_event", G_CALLBACK(cbDraw), NULL);
QS_SPEW("after\n");
}

static void
qs_drawing_area_send_configure(QsDrawingArea *darea)
{
  GtkAllocation allocation;
  GtkWidget *widget;
  GdkEvent *event = gdk_event_new(GDK_CONFIGURE);

  widget = GTK_WIDGET(darea);
  gtk_widget_get_allocation(widget, &allocation);

  event->configure.window = g_object_ref(gtk_widget_get_window (widget));
  event->configure.send_event = TRUE;
  event->configure.x = allocation.x;
  event->configure.y = allocation.y;
  event->configure.width = allocation.width;
  event->configure.height = allocation.height;

  gtk_widget_event(widget, event);
  gdk_event_free(event);
}

static
bool ecb_keyPress(GtkWidget *w, GdkEvent *e, void* data)
{
  switch(e->key.keyval)
  {
    case GDK_KEY_Q:
    case GDK_KEY_q:
    case GDK_KEY_Escape:
      gtk_main_quit();
      return true;
      break;
  }
  return false;
}


static
void makeWidgets(const char *title)
{
  GtkWidget *window, *vbox, *button, *da;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), title);
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
  g_signal_connect(G_OBJECT(window), "key-press-event",
      G_CALLBACK(ecb_keyPress), NULL);
  g_signal_connect(window, "delete_event", G_CALLBACK(gtk_main_quit), NULL);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  button = gtk_button_new_with_label("Quit");
  g_signal_connect(button, "clicked", G_CALLBACK(gtk_main_quit), NULL);
  gtk_box_pack_start(GTK_BOX(vbox), button, false, false, 0);
  gtk_widget_show(button);
  da = qs_drawing_area_new();
  gtk_box_pack_start(GTK_BOX(vbox), da, true, true, 0);
  g_signal_connect(G_OBJECT(da),"configure-event", G_CALLBACK(Configure), da);
  g_signal_connect(G_OBJECT(da), "draw", G_CALLBACK(cbDraw), NULL);
  gtk_widget_show(da);

  gtk_container_add(GTK_CONTAINER(window), vbox);

  gtk_widget_show(vbox);

  gtk_widget_show(window);
}

int main(int argc, char **argv)
{
  gtk_init(&argc, &argv);
  makeWidgets(argv[0]);
  gtk_main();

  return 0;
}

