/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#include <limits.h>
#include <sys/wait.h>
#include <libgen.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <gtk/gtk.h>

static const char *const default_programs[] =
{
#ifndef TESTS

  // bin/ installed qs_launcher code

  "qs_alsaCapture",
  "qs_rossler3Wins",
  "qs_circle",
  "qs_sin",
  "qs_sin --slope=0",
  "qs_sin --fast --slope=0",
  "qs_urandom",
  "qs_urandom --fast",
  "qs_ode --system=lorentz",

#else

#  define LONG_MUSIC  "'/home/lanceman/Music/Alanis Morissette/Jagged"\
                      " Little Pill/02. You Oughta Know.ogg'"

#  define SHORT_MUSIC "/usr/share/tuxpaint/sounds/mirror.wav"

  // tests/ code

  "hello",
  "qsSource | quickplot -",
  "circle",
  "saw_print | quickplot -",
  "non_master_print | quickplot -",
  "sin",
  "sin --delay=2.125",
  "sin --delay=-2.125",
  "sin --samples-per-period=11",
  "sin --slope=0 --swipe=1",
  "sin --slope=0 --swipe=1 --fade=0",
  "sin --slope=0 --swipe=1 --fade=0 --cos=1",
  "soundFile_print "SHORT_MUSIC" | quickplot -",
  "soundFile "LONG_MUSIC,
  "soundFile "SHORT_MUSIC,
  "soundFile --swipe=0 --fade=1 --holdoff=0.03 "LONG_MUSIC,
  "rk4_print | quickplot -",
  "ode_print system=lorenz | quickplot -",
  "ode --system=lorenz",
  "ode --system=lorenz --sweep=1",
  "ode --system=rossler",
  "rossler3Wins",
  "urandom",
  "urandom --hammer-time",
  "sin --fast --slope=0 --swipe=1 --fade=0",
  "alsa_capture_print | quickplot -",
  "alsaCapture",

#endif
  NULL // Null terminator
};

struct Run
{
  int run_count;
  GtkWidget *button;
  char *program;
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
#ifdef QS_TESTS
  if(strcmp(".","."))
    // built in other dir
    newPath = g_strdup_printf("%s:%s:%s", "./tests", path, envPath);
  else
    // built in top source dir
    newPath = g_strdup_printf("%s:%s", path, envPath);
#else
  newPath = g_strdup_printf("%s:%s", path, envPath);
#endif

  setenv("PATH", newPath, 1);
  fprintf(stderr, "set PATH=\"%s\"\n", newPath);

  g_free(newPath);

cleanup:

  free(path);
}

static
void setLabelString(struct Run *run)
{
  char *text = NULL;
  GtkLabel *label;
  label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(run->button)));
  char *color;
  switch(run->run_count)
  {
    case 0:
      color = "#000";
      break;
    default:
      color = "#f55";
      break;
  }

  text = g_strdup_printf("%s", run->program);
  
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

static bool run_cb(GtkWidget *button, struct Run *run)
{
  fprintf(stderr, "running:");
  fprintf(stderr, " %s", run->program);
  fprintf(stderr, "\n");

  setLabelString(run);

  // pipe command to bash

  int fd[2];

  if(pipe(fd) == -1)
  {
    fprintf(stderr, "pipe() failed: %s\n",
        strerror(errno));
    return true;
  }

  if(fork() == 0)
  {
    // child
    close(fd[1]);
    // make stdin be the reading end of the pipe
    if(dup2(fd[0], 0) == -1)
    {
      fprintf(stderr, "dup2(%d, 0) failed: %s\n",
          fd[0], strerror(errno));
      exit(1);
    }
    // bash will read the pipe
    execl("/bin/bash", "bash", NULL);
    fprintf(stderr, "failed to execute: %s in bash pipeline\n", run->program);
    exit(1);
  }

  // The parent write the bash command to the pipe
  if(write(fd[1], run->program, strlen(run->program)) != strlen(run->program))
  {
    fprintf(stderr, "write(%d, \"%s\", %zu) failed: %s\n",
        fd[1], run->program, strlen(run->program), strerror(errno));
    exit(1);
  }
  if(write(fd[1], "\n", 2) != 2)
  {
    fprintf(stderr, "write(%d, \"%s\", %d) failed: %s\n",
        fd[1], "\\n", 2, strerror(errno));
    exit(1);
  }
  close(fd[0]);
  close(fd[1]);
  return true;
}

static void addButton(const char *args, GtkContainer *vbox)
{
  struct Run *run;
  run = g_malloc0(sizeof(*run));
  run->button = gtk_button_new_with_label(" ");
  run->program = (char *) args;
  g_signal_connect(run->button, "clicked", G_CALLBACK(run_cb), (void *) run);
  gtk_box_pack_start(GTK_BOX(vbox), run->button, false, false, 0);
  gtk_widget_show(run->button);
  setLabelString(run);
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

static void makeWidgets(const char *title, const char *const programs[])
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
  gtk_box_pack_start(GTK_BOX(vbox), button, false, false, 0);
  gtk_widget_show(button);
  
  gtk_container_add(GTK_CONTAINER(window), vbox);

  const char *const *program;
  program = programs;

  while(*program)
  {
    addButton(*program, GTK_CONTAINER(vbox));
    ++program;
  }

  gtk_widget_show(vbox);

  gtk_widget_show(window);
}

#define DSO_SYMBOL   "run"

static int
usage(char *argv0)
{
    fprintf(stderr,
        "Usage: %s [--help|-h] [--dso-list LIST|--list LIST]\\\n"
        "            [--title TITLE]\n\n"
        "   A simple button launcher program with a default list\n"
        " of programs to launch.  This launcher does not manage the\n"
        " programs that it launches, so quiting this launcher will\n"
        " not terminate all the launched programs.  If you terminate\n"
        " this launcher by signaling it from a shell, the shell may\n"
        " also terminate all the launched programs too, but that's\n"
        " just the shell doing its thing and not this launcher.\n"
        " This launcher program is intentionally simple and is not\n"
        " a graphical shell.  It's just a program launcher.\n"
        "\n"
        "\n"
        "    OPTIONS\n"
        "\n"
        "  --help|-h          print this help and exit\n"
        "\n"
        "  --dso-list LIST    load dynamic shared object file LIST which\n"
        "                     should have the list of programs to run in\n"
        "                     the symbol const char *const " DSO_SYMBOL "[].\n"
        "                     Example DSO source file:\n"
        "\n"
        "                       const char *const " DSO_SYMBOL "[] = {\n"
        "                            \"hello\",\n"
        "                            \"goodbye\", \"--day=Tuesday\",\n"
        "                             NULL };\n"
        "\n"
        "  --list LIST        load a list of programs to run from the file LIST\n"
        "                     one on each line in the file.  The programs are\n"
        "                     run using /bin/bash\n"
        "\n"
        "  --title TITLE     set window title to TITLE\n"
        "\n"
        ,
        basename(argv0));
    return 1;
}

static
const char *const *getFileList(char *filename)
{
  FILE *file;
  if(filename[0] == '-' && !filename[1])
    file = stdin;
  else if(!(file = fopen(filename, "r")))
  {
    fprintf(stderr, "fopen(\"%s\", 'r') failed: %s\n",
        filename, strerror(errno));
    return NULL;
  }
  char *line = NULL;
  size_t num_progs = 0;
  char **programs = NULL;
  size_t line_length = 0;
  while(getline(&line, &line_length, file) != -1)
  {
    if(line && line[0] && line[0] != '#')
    {
      size_t l;
      while((l = strlen(line)) && (line[strlen(line) - 1] == '\n' ||
        line[strlen(line) - 1] == '\r'))
        line[strlen(line) - 1] = '\0';
      if(line[0])
      {
        programs = realloc(programs, sizeof(char *) * (++num_progs + 1));
        programs[num_progs - 1] = strdup(line);
        programs[num_progs] = NULL;
      }
    }
  }
  if(line)
    free(line);

  if(!programs)
    fprintf(stderr,
        "No programs listed in file: %s\n",
        filename);

  if(file != stdin)
    fclose(file);

  return (const char *const *) programs;
}

static
const char *const *getDSOList(char *dso_name)
{
  void *hndl;
  const char *const *programs;
  char *error;

  dlerror();    /* Clear any existing error */
  hndl = dlopen(dso_name, RTLD_NOW|RTLD_LOCAL);
  if(!hndl)
  {
    fprintf(stderr, "dlopen(\"%s\",) failed: %s\n\n", dso_name, dlerror());
    return NULL;
  }
  dlerror();    /* Clear any existing error */
  programs = dlsym(hndl, DSO_SYMBOL);
  if((error = dlerror()))
  {
    fprintf(stderr, "dlsym(,\"%s\",) failed: %s\n\n", DSO_SYMBOL, dlerror());
    dlclose(hndl);
    return NULL;
  }
  if(!programs)
  {
    fprintf(stderr, "dlsym(,\"%s\",) returned 0.\n\n", DSO_SYMBOL);
  }
  dlclose(hndl);
  return programs;
}

void sigChildCatcher(int sig)
{
  int status;
  wait(&status);
}

int main(int argc, char **argv)
{
  char *dso_name = NULL, *list_file= NULL;
  const char *const *programs;
  programs = default_programs;
  char **args;
  args = argv;
  char *title;
  title = basename(argv[0]);

  while(*(++args))
  {
    if(strncmp(*args, "--dso-list=", 11) == 0)
      dso_name = &(*args)[11];
    else if(strcmp(*args, "--dso-list") == 0)
      dso_name = *(++args);
    else if(strncmp(*args, "--list=", 7) == 0)
      list_file = &(*args)[7];
    else if(strcmp(*args, "--list") == 0)
      list_file = *(++args);
    else if(strncmp(*args, "--title=", 8) == 0)
      title = &(*args)[8];
    else if(strcmp(*args, "--title") == 0)
      title = *(++args);
    else
      return usage(argv[0]);

    // Make run list only one way.
    if((dso_name || list_file) && programs != default_programs)
      return usage(argv[0]);
    if(dso_name && !(programs = getDSOList(dso_name)))
      return usage(argv[0]);
    if(list_file && !(programs = getFileList(list_file)))
      return usage(argv[0]);
  }

  setPathEnv();
  gtk_init(&argc, &argv);
  makeWidgets(title, programs);
  signal(SIGCHLD, sigChildCatcher);
  gtk_main();

  return 0;
}

