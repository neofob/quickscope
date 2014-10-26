/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#include <limits.h>
#include <libgen.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <gtk/gtk.h>

static const char *const programs[] =
{
#ifndef TESTS
  // bin/ installed qs_launcher code
  "qs_circle", NULL,
  "qs_sin", NULL,
#else
  // tests/ code
  "hello", NULL,
  "quickplot.bash", "./source", NULL,
  "circle", NULL,
  "quickplot.bash", "./saw_print", NULL,
  "quickplot.bash", "./non_master_print", NULL,
  "sin_sweep", NULL,
#endif
  NULL
};

struct Run
{
  int run_count;
  GtkWidget *button;
  char **args;
};


// Add the Path of this program to the environment
// variable PATH
static void setPathEnv(void)
{
  // TODO: /proc/self/exe is specific to Linux
  //
  char *path = realpath("/proc/self/exe" , NULL);
  if(!path) return;
  if(!path[0]) goto cleanup;

  // Remove filename and last '/'
  char *end;
  end = path;
  while(*(++end));
  --end;
  while(*end != '/' && end != path)
    *(end--) = '\0';
  if(end != path && *end == '/')
    *end = '\0';

  char *envPath;
  envPath = getenv("PATH");
  if(!envPath) goto cleanup;

  char *newPath;
  size_t len;
  len = strlen(envPath) + strlen(path) + 2;
  newPath = g_malloc(len);
  snprintf(newPath, len, "%s:%s", path, envPath);
  setenv("PATH", newPath, 1);

  g_free(newPath);

cleanup:

  free(path);
}

static
void setLabelString(struct Run *run)
{
  char *const *arg;
  char *text = NULL;
  GtkLabel *label;
  label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(run->button)));
  arg = run->args;
  char *color;

  while(*arg)
  {
    switch(run->run_count)
    {
      case 0:
        color = "#000";
        break;
      default:
        color = "#f55";
        break;
    }

    if(text)
    {
      char *old;
      old = text;
      text = g_strdup_printf("%s %s", old, *arg++);
      g_free(old);
    }
    else
    {
      text = g_strdup_printf("%s", *arg++);
    }
  }
  char *old;
  old = text;
  text = g_strdup_printf(
          "<span font_family=\"monospace\" "
          ">%4d)  </span>"
          "<span font_family=\"monospace\" "
          "font_weight=\"bold\" "
          "color=\"%s\">%s %s",
          run->run_count++, color, old,
          "                                                 "
          "                                                 "
          "                                                 ");
  g_free(old);
  old = text;
  text = g_strdup_printf(
          "%168.168s</span>", old);
  g_free(old);

  gtk_label_set_markup(label, text);
  g_free(text);
}

static gboolean run_cb(GtkWidget *button, struct Run *run)
{
  printf("running: %s\n", *run->args);

  setLabelString(run);

  int pid;
  if((pid = vfork()) == 0)
  {
    execvp(*run->args, run->args);
    printf("failed to execute %s\n", *run->args);
    exit(1);
  }

  return TRUE;
}

static void addButton(char *const *args, GtkContainer *vbox)
{
  struct Run *run;
  run = g_malloc0(sizeof(*run));
  run->button = gtk_button_new_with_label(" ");
  run->args = (char **) args;
  g_signal_connect(run->button, "clicked", G_CALLBACK(run_cb), (void *) run);
  gtk_box_pack_start(GTK_BOX(vbox), run->button, FALSE, FALSE, 0);
  gtk_widget_show(run->button);
  setLabelString(run);
}

gboolean ecb_keyPress(GtkWidget *w, GdkEvent *e, void* data)
{
  switch(e->key.keyval)
  {
    case GDK_KEY_Q:
    case GDK_KEY_q:
    case GDK_KEY_Escape:
      gtk_main_quit();
      return TRUE;
      break;
  }
  return FALSE;
}

static void makeWidgets(const char *title)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *button;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), title);
  g_signal_connect(G_OBJECT(window), "key-press-event",
      G_CALLBACK(ecb_keyPress), NULL);
  g_signal_connect(window, "delete_event", G_CALLBACK(gtk_main_quit), NULL);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  button = gtk_button_new_with_label("Quit");
  g_signal_connect(button, "clicked", G_CALLBACK(gtk_main_quit), NULL);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  gtk_widget_show(button);
  
  gtk_container_add(GTK_CONTAINER(window), vbox);

  const char *const *program;
  program = programs;

  while(*program)
  {
    addButton((char **) program, GTK_CONTAINER(vbox));
    while(*(++program));
    ++program;
  }

  gtk_widget_show(vbox);

  gtk_widget_show(window);
}


int main(int argc, char **argv)
{
  if(argc > 1)
  {
    printf("Usage: %s [--help|-h]\n\n"
        "   A simple button launcher program with built in list\n"
        " of programs to launch.  This launcher does not manage the\n"
        " programs that it launches, so quiting this launcher will\n"
        " not terminate all the launched programs.  If you terminate\n"
        " this launcher by signaling it from a shell, the shell may\n"
        " also terminate all the launched programs too, but that's\n"
        " just the shell doing its thing and not this launcher.\n"
        " This launcher program is intentionally simple and is not\n"
        " a graphical shell.  It's just a program launcher.\n\n",
        basename(argv[0]));
    return 1;
  }

  setPathEnv();
  gtk_init(&argc, &argv);
  makeWidgets(basename(argv[0]));
  gtk_main();

  return 0;
}

