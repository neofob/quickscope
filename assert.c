/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include "debug.h"
#include "assert.h"

#ifdef QS_DEBUG
void qsSpew(const char *file, int line,
    const char *func, const char *format, ...)
{
  fprintf(stderr, "%s:%s():%d: ", file, func, line);
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

void qsAssert(const char *file, int line,
    const char *func, long bool_arg,
    const char *arg, const char *format, ...)
{
  if(!bool_arg)
  {
    va_list ap;

    if(errno)
    {
      char error[64];
      error[63] = '\0';
      strerror_r(errno, error, 64);
      fprintf(stderr, "QS_ASSERT: %s:%d:%s(): errno=%d:%s\n"
        "ASSERTION(%s) FAILED: pid: %d\n",
        file, line, func, errno, error, arg, getpid());
    }
    else
      fprintf(stderr, "QS_ASSERT: %s:%d:%s():\n"
        "ASSERTION(%s) FAILED: pid: %d\n",
        file, line, func, arg, getpid());

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\nsignal to exit, quickscope sleeping here ...\n"
        "try running:   gdb --pid %d\n", getpid());
    // We could run GDB here but who are we to take control and tell
    // the parent process what to do, we can't be sure what kind of
    // shell it is, if it is one, anyway.  If they have job control they
    // are better off doing it themselves.

    // Stop. Why not stop?  Works nice from bash parent process.
    // kill(getpid(), SIGSTOP);
    // if not stopped or awakened then hang in sleep.
    int i = 1; // unset in debugger to stop sleeping and return.
    while(i)
      sleep(10);
  }
}
#endif

