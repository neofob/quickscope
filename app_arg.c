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
#include "Assert.h"
#include "app.h"

// We support program option arguments like:
//  --name val
//  --name=val
//  --name       (just for bool true and string returns "")

static 
char *getString(const char *name, char **argv)
{
  QS_ASSERT(name);
  QS_ASSERT(argv);
  char *ret = NULL;
  size_t nlen;
  nlen = strlen(name) + 3;
  char *opt0, *opt1;
  opt0 = g_strdup_printf("--%s=", name);
  opt1 = g_strdup_printf("--%s", name);
  while(*argv)
  {
    if(!strncmp(opt0, *argv, nlen))
    {
      ret = &(*argv)[nlen];
    }
    else if(!strcmp(opt1, *argv) && *(argv + 1)
        &&  (*(argv + 1))[0] && (*(argv + 1))[0] != '-'
        && (*(argv + 1))[1] != '-')
    {
      ret = *(++argv);
    }
    else if(!strcmp(opt1, *argv)
          &&

          (
            ( *(argv + 1)
               &&  (*(argv + 1))[0] && (*(argv + 1))[0] == '-'
               && (*(argv + 1))[1] == '-' )

            ||
            
            !(*(argv + 1))
          )
        )
    {
      ret = "";
    }

    ++argv;
  }

  g_free(opt0);
  g_free(opt1);

  return ret;
}

float qsApp_float(const char *name, float dflt)
{
  char *val = getString(name, qsApp->argv);
  if(val && val[0])
    return strtof(val, NULL);
  return dflt;
}

double qsApp_double(const char *name, double dflt)
{
  char *val = getString(name, qsApp->argv);
  if(val && val[0])
    return strtod(val, NULL);
  return dflt;
}

int qsApp_int(const char *name, int dflt)
{
  char *val = getString(name, qsApp->argv);
  if(val && val[0])
    return (int) strtol(val, NULL, 10);
  return dflt;
}

const char *qsApp_string(const char *name, const char *dflt)
{
  char *val = getString(name, qsApp->argv);
  if(val)
    return val;
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
      // true, 1, yes, affirmative, on, asdf, --name with no arg
      return true;
  }

  return dflt;
}
