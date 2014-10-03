/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>


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

static gboolean run_cb(GtkWidget *button, const char *run)
{
  printf("running: %s\n", run);
  int pid;
  if((pid = vfork()) == 0)
  {
    execlp(run, run, NULL);
    printf("failed to execute %s\n", run);
    exit(1);
  }

  return TRUE;
}

static void addButton(const char *label, GtkContainer *vbox)
{
  GtkWidget *button;
  button = gtk_button_new_with_label(label);
  g_signal_connect(button, "clicked", G_CALLBACK(run_cb), g_strdup(label));
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  gtk_widget_show(button);
}

static void makeWidgets(void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *button;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(window, "delete_event", G_CALLBACK(gtk_main_quit), NULL);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  button = gtk_button_new_with_label("Quit");
  g_signal_connect(button, "clicked", G_CALLBACK(gtk_main_quit), NULL);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  gtk_widget_show(button);
  
  gtk_container_add(GTK_CONTAINER(window), vbox);

  char *programs[] =
  {
    "qs_circle",
    "qs_sin",
    NULL
  };
  char **program;
  program = programs;

  while(*program)
    addButton(*(program++), GTK_CONTAINER(vbox));

  gtk_widget_show(vbox);

  gtk_widget_show(window);
}

int main(int argc, char **argv)
{
  setPathEnv();
  gtk_init(&argc, &argv);
  makeWidgets();
  gtk_main();

  return 0;
}

