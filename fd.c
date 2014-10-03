/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "controller_priv.h"
#include "fd.h"

struct QsFdGSource
{
  /* The glib GSource stuff */
  GSource gsource;
  GPollFD pfd;
  /* A pointer to the rest of the object */
  struct QsFd *fd;
};

struct QsFd
{
  // We inherit controller
  struct QsController controller;

  /* The glib GSource stuff */
  struct QsFdGSource *fdGSource;

  void (*destroy)(void *);
};

extern
void qsFd_destroy(struct QsFd *fd);


static inline
int check_fd_in(int fd)
{
  fd_set rfds;
  struct timeval tv = {0,0};
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);
  if(select(fd+1, &rfds, NULL, NULL, &tv) > 0)
    return 1;
  return 0;
}

static
gboolean prepare(GSource *gsource, gint *timeout)
{
  *timeout = -1; /* block */
  return FALSE;
}

static
gboolean check(struct QsFdGSource *fd)
{
  if(fd->pfd.revents & G_IO_IN)
  {
    //fd.revents = 0;
    return TRUE;
  }
  return FALSE;
}

static
gboolean dispatch(struct QsFdGSource *fD, GSourceFunc callback, gpointer data)
{
  struct QsFd *fd;
  fd = fD->fd;
  do
    _qsController_run((struct QsController *)fd);
  while(check_fd_in(fD->pfd.fd));

  return TRUE;
}

static
void _qsFd_internalDestroy(struct QsFd *fd)
{
  struct QsFdGSource *gfd;
  struct QsController *c;
  QS_ASSERT(fd);
  QS_ASSERT(fd->fdGSource);

  gfd = fd->fdGSource;
  c = (struct QsController *)fd;

  gfd->pfd.revents = 0;
  g_source_remove_poll(&gfd->gsource, &(gfd->pfd));
  /* g_source_destroy() removes a source from its' GMainContext */
  g_source_destroy(&gfd->gsource);


#ifdef QS_DEBUG
  memset(((uint8_t*)gfd)+sizeof(GSource), 0, sizeof(*gfd) - sizeof(GSource));
#endif

  /* free the GSource and struct QsFdGSource memory */
  g_source_unref(&gfd->gsource);

 if(c->destroy)
 {
   c->destroy = NULL;
   /* destroy base class which frees memory */
   qsController_destroy(c);
 }
}

struct QsController *qsFd_create(int fD, size_t size)
{
  /* this source_funcs memory needs to exist after this function returns
   * because glib requires it. */
  static GSourceFuncs source_funcs =
  {
    prepare,
    (gboolean (*)(GSource *)) check,
    (gboolean (*)(GSource *, GSourceFunc, gpointer)) dispatch,
    0, 0, 0
  };
  struct QsFd *fd;
  struct QsFdGSource *gs;
  GSource *gsource;

  QS_ASSERT(fD >= 0);

  if(size < sizeof(*fd))
    size = sizeof(*fd);

  fd = qsController_create(size);
  fd->destroy = (void(*)(void*)) qsFd_destroy;
  fd->fdGSource = gs = (struct QsFdGSource *)
    g_source_new(&source_funcs, sizeof(*gs));
  memset(((uint8_t*)gs) + sizeof(GSource), 0, sizeof(*gs) - sizeof(GSource));
  gs->pfd.fd = fD;
  gs->pfd.events = G_IO_IN;
  gs->pfd.revents = 0;
  gs->fd = fd;

  gsource = &gs->gsource;
  g_source_set_priority(gsource, G_PRIORITY_LOW
      /* larger number == lower priority */);
  g_source_attach(gsource, NULL); /* TODO: do we need the returned id ?? */
  g_source_add_poll(gsource, &gs->pfd);
  return (struct QsController *) fd;
}

struct QsController *qsFd_createFILE(FILE *file, size_t size)
{
  struct QsFd *fd;
  fd = (struct QsFd *) qsFd_create(fileno(file), size);
  return (struct QsController *) fd;
}

void qsFd_destroy(struct QsFd *fd)
{
  QS_ASSERT(fd);

  if(fd->destroy)
  {
    void (*destroy)(void *);
    destroy = fd->destroy;

    /* c->destroy == NULL flags not to call qsComposer_destroy() */
    fd->destroy = NULL;
    /* destroy inheriting object first */
    destroy(fd);
  }

  /* now destroy this and the base object */
  _qsFd_internalDestroy(fd);
}
