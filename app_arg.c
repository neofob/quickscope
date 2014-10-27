/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "app.h"

// We support program arguments like:
//  --name val
//  --name=val

static 
char *getString(const char *name, char **argv)
{
  QS_ASSERT(name);
  char *ret = NULL;
  size_t nlen;
  nlen = strlen(name) + 3;
  argv = qsApp->argv;
  char *opt0, *opt1;
  opt0 = g_strdup_printf("--%s=", name);
  opt1 = g_strdup_printf("--%s", name);
  while(argv && *argv)
  {
    if(!strncmp(opt0, *argv, nlen))
    {
      ret = &(*argv++)[nlen];
    }
    else if(!strcmp(opt1, *argv) && *(argv + 1))
    {
      ret = *(++argv);
      ++argv;
    }
    else
    {
      ++argv;
    }
  }

  return ret;
}

float qsApp_float(const char *name, float dflt)
{
  char *val = getString(name, qsApp->argv);
  if(val)
    return strtof(val, NULL);
  return dflt;
}

double qsApp_double(const char *name, double dflt)
{
  char *val = getString(name, qsApp->argv);
  if(val)
    return strtod(val, NULL);
  return dflt;
}

const char *qsApp_string(const char *name, const char *dflt)
{
  char *val = getString(name, qsApp->argv);
  if(val)
    return val;
  return dflt;
}

int qsApp_int(const char *name, int dflt)
{
  char *val = getString(name, qsApp->argv);
  if(val)
    return (int) strtol(val, NULL, 10);
  return dflt;
}

bool qsApp_bool(const char *name, bool dflt)
{
  char *val = getString(name, qsApp->argv);

  if(val)
  {
    if(*val == 'f' || *val == 'F' ||
        *val == '0' ||
        *val == 'n' || *val == 'N' ||
        ((*val == 'o' || *val == 'O') && (val[1] == 'f' || val[1] == 'F')))
      // false, 0, no, neg, off
      return false;
    else
      // true, 1, yes, affirmative, on, asdf
      return true;
  }

  return dflt;
}
