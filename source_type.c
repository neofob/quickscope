/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "Assert.h"
#include "base.h"
#include "app.h"
#include "controller.h"
#include "adjuster.h"
#include "adjuster_priv.h"
#include "group.h"
#include "source.h"
#include "source_priv.h"


static
const char *_typeStr(enum QsSource_Type type)
{
  switch(type)
  {
    case QS_FIXED:
      return "QS_FIXED";
      break;
    case QS_SELECTABLE:
      return "QS_SELECTABLE";
      break;
    case QS_VARIABLE:
      return "QS_VARIABLE";
      break;
    case QS_TOLERANT:
      return "QS_TOLERANT";
      break;
    case QS_CUSTOM:
      return "QS_CUSTOM";
      break;
    default:
      QS_ASSERT(0);
      return "UNKNOWN";
      break;
    }
}

static
void _spewType(enum QsSource_Type type, const float *sampleRates,
    float sampleRate)
{
  fprintf(stderr, "%s\n", _typeStr(type));
  switch(type)
  {
    case QS_FIXED:
      fprintf(stderr, "sampleRate = %f\n", sampleRate);
      break;
    case QS_SELECTABLE:
      fprintf(stderr, "selectable sampleRates (Hz):");
      while(*sampleRates)
        fprintf(stderr, " %f", *sampleRates++);
      fprintf(stderr, "\n");
      break;
    case QS_VARIABLE:
      fprintf(stderr, "sampleRates at and between (min) %f Hz and (max) %f Hz\n",
          sampleRates[0], sampleRates[1]); 
      break;
    case QS_TOLERANT:
      fprintf(stderr, "Works with any sample rate\n");
      break;
    case QS_CUSTOM:
      fprintf(stderr, "User custom frame times\n");
      break;
    default:
      QS_ASSERT(0);
      fprintf(stderr, "Invalid source type\n");
      break;
  }
}

static
void _qsSource_spewType(const struct QsSource *s)
{
  fprintf(stderr, "Source %d is of type ", s->id);
  _spewType(s->type, s->sampleRates, s->sampleRate);
}

static
void _qsSource_spewTypes(const struct QsSource *s)
{
  QS_ASSERT(s);
  QS_ASSERT(s->group);
  GSList *l;
  for(l = s->group->sources; l; l = l->next)
    _qsSource_spewType((const struct QsSource *) l->data);
}

static
bool Incompatible(const struct QsSource *s, struct QsGroup *g)
{
  fprintf(stderr,
            "There are incompatible %s and %s sources\n",
            _typeStr(s->type), _typeStr(g->type));
  _qsSource_spewTypes(s);

  // cleanup g
  g->type = QS_NONE;
  if(g->sampleRates)
  {
    g_free(g->sampleRates);
    g->sampleRates = NULL;
  }
  g->sampleRate = 0;
  return true;
}

static inline
bool _qsGroup_setFixed(struct QsGroup *g, float rate)
{
  if(g->sampleRates)
  {
    g_free(g->sampleRates);
    g->sampleRates = NULL;
  }
  g->type = QS_FIXED;
  g->sampleRate = rate;
  return false;
}

// returns false on success
static inline
bool _checkFixed(const struct QsSource *s, struct QsGroup *g)
{
  QS_ASSERT(s->sampleRates && *s->sampleRates);
  QS_ASSERT(s->type == QS_FIXED);

  int sampleRate = s->sampleRate;

  QS_ASSERT(sampleRate > 0);

  switch(g->type)
  {
    case QS_SELECTABLE:
    {
      int i;
      for(i=0; g->sampleRates[i]; ++i)
        if(g->sampleRates[i] == sampleRate)
          break;
      if(!g->sampleRates[i])
        return Incompatible(s, g);
      return _qsGroup_setFixed(g, sampleRate);
    }
    break;
    case QS_VARIABLE:
      if(sampleRate >= g->sampleRates[0] // >= min
          && sampleRate <= g->sampleRates[1] // <= max
        ) return _qsGroup_setFixed(g, sampleRate);
      return Incompatible(s, g);
    break;
    case QS_FIXED:
      if(g->sampleRates[0] != sampleRate)
        return Incompatible(s, g);
      return false;
    break;
    case QS_NONE:
    case QS_TOLERANT:
      return _qsGroup_setFixed(g, sampleRate);
      break;
    default: // error
      return Incompatible(s, g);
    break;
  }

  return true; //error WTF
}

static inline
bool _checkSelectable(const struct QsSource *s, struct QsGroup *g)
{
  QS_ASSERT(s->sampleRates && *s->sampleRates);
  QS_ASSERT(s->type == QS_SELECTABLE);

  switch(g->type)
  {
    case QS_SELECTABLE:
      {
        int i, j;
        size_t n = 0;
        float *commonRates = NULL;
        g->sampleRate = 0;
        for(i=0; g->sampleRates[i]; ++i)
          for(j=0; s->sampleRates[j]; ++j)
            if(g->sampleRates[i] == s->sampleRates[j])
            {
              commonRates = g_realloc(commonRates, sizeof(float)*(n + 2));
              commonRates[n++] = g->sampleRates[i];
              commonRates[n] = 0; // terminator
              if(s->sampleRate == s->sampleRates[j])
                g->sampleRate = s->sampleRate;
            }

        if(!commonRates)
          return Incompatible(s, g);
        if(g->sampleRates)
          g_free(g->sampleRates);
        g->sampleRates = commonRates;
        if(!g->sampleRate)
          g->sampleRate = commonRates[0];
        return false;
      }
    break;
    case QS_VARIABLE:
      {
        int i;
        size_t n = 0;
        float *commonRates = NULL;
        g->sampleRate = 0;
        for(i=0; s->sampleRates; ++i)
          if(s->sampleRates[i] >= g->sampleRates[0] // >= min
            && s->sampleRates[i] <= g->sampleRates[1]) // <= max
          {
            commonRates = g_realloc(commonRates, sizeof(float)*(n + 2));
            commonRates[n++] = g->sampleRates[i];
            commonRates[n] = 0; // terminator
            if(s->sampleRate == s->sampleRates[i])
              g->sampleRate = s->sampleRate;
          }

        if(!commonRates)
          return Incompatible(s, g);
        if(g->sampleRates)
          g_free(g->sampleRates);
        g->sampleRates = commonRates;
        if(!g->sampleRate)
          g->sampleRate = commonRates[0];
        g->type = QS_SELECTABLE;
      }
      return false;
    break;
    case QS_FIXED:
      {
        int i;
        for(i=0; s->sampleRates[i]; ++i)
          if(s->sampleRates[i] == g->sampleRate)
            break;

        if(!s->sampleRates[i])
          return Incompatible(s, g);
      }
      return false;
    break;
    case QS_NONE:
    case QS_TOLERANT:
      {
        int i, n;
        for(n=0; s->sampleRates[n]; ++n);
        QS_ASSERT(!g->sampleRates);
        g->sampleRates = g_malloc(sizeof(float)*(n+1));
        for(i=0; i<n; ++i)
          g->sampleRates[i] = s->sampleRates[i];
        g->type = QS_SELECTABLE;
        g->sampleRate = s->sampleRate;
      }
      return false;
      break;
    default: // error
      return Incompatible(s, g);
    break;
  }

  return true; // WTF
}

static inline
bool _checkVariable(const struct QsSource *s, struct QsGroup *g)
{
  QS_ASSERT(s->type == QS_VARIABLE);

  switch(g->type)
  {
    case QS_TOLERANT:
    case QS_NONE:
      QS_ASSERT(!g->sampleRates);
      g->sampleRates = g_malloc(sizeof(float)*2);
      g->sampleRates[0] = s->sampleRates[0];
      g->sampleRates[1] = s->sampleRates[1];
      g->type = QS_VARIABLE;
      g->sampleRate = s->sampleRate;
      return false;
      break;
    case QS_VARIABLE:
      // ranges must overlap
      if(g->sampleRates[0] > s->sampleRates[1] ||
        g->sampleRates[1] < s->sampleRates[0])
        return Incompatible(s, g);
      // They do overlap
      if(g->sampleRates[0] < s->sampleRates[0])
        g->sampleRates[0] = s->sampleRates[0];
      if(g->sampleRates[1] > s->sampleRates[1])
        g->sampleRates[1] = s->sampleRates[1];
      if(s->sampleRate >= g->sampleRates[0] &&
          s->sampleRate <= g->sampleRates[1])
        g->sampleRate = s->sampleRate;
      else
        g->sampleRate = 0.5F * (g->sampleRates[0] + g->sampleRates[1]);
      return false;
      break;
    case QS_FIXED:
      if(g->sampleRate >= s->sampleRates[0] &&
          g->sampleRate <= s->sampleRates[1])
        return false;
      return Incompatible(s, g);
      break;
    case QS_SELECTABLE:
      {
        int i;
        size_t n = 0;
        float sampleRate, *commonRates = NULL;
        sampleRate = g->sampleRate;
        g->sampleRate = 0;
        for(i=0; g->sampleRates; ++i)
          if(g->sampleRates[i] >= s->sampleRates[0] // >= min
            && g->sampleRates[i] <= s->sampleRates[1]) // <= max
          {
            commonRates = g_realloc(commonRates, sizeof(float)*(n + 2));
            commonRates[n++] = g->sampleRates[i];
            commonRates[n] = 0; // terminator
            if(sampleRate == g->sampleRates[i])
              g->sampleRate = sampleRate;
          }

        if(!commonRates)
          return Incompatible(s, g);
        if(g->sampleRates)
          g_free(g->sampleRates);
        g->sampleRates = commonRates;
        if(!g->sampleRate)
          g->sampleRate = commonRates[0];
      }
      return false;
      break;
    default:
      return Incompatible(s, g);
      break;
  }

  return true;
}

static inline
bool _checkTolerant(const struct QsSource *s, struct QsGroup *g)
{
  QS_ASSERT(s->type == QS_TOLERANT);

  switch(g->type)
  {
    case QS_FIXED:
    case QS_SELECTABLE:
    case QS_VARIABLE:
    case QS_TOLERANT:
    case QS_NONE:
      return false;
      break;
    // case QS_CUSTOM:
    default:
      return Incompatible(s, g);
      break;
  }

  return true;
}

static inline
bool _checkCustom(const struct QsSource *s, struct QsGroup *g)
{
   QS_ASSERT(s->type == QS_CUSTOM);
  return (g->type == QS_CUSTOM)?false:true;
}

// setup source group rates and shit
// returns false on success
static
bool _qsSource_checkTypes(struct QsSource *s)
{
  QS_ASSERT(s);
  struct QsGroup *g;
  g = s->group;
  QS_ASSERT(g);

  // First reset the group rate settings
  if(g->sampleRates)
  {
    g_free(g->sampleRates);
    g->sampleRates = NULL;
  }
  g->sampleRate = 0;
  g->type = QS_NONE;


  GSList *l;
  for(l = g->sources; l; l = l->next)
  {
    struct QsSource *s;
    s = l->data;
    QS_ASSERT(s);
    QS_ASSERT(s->group == g);

    switch(s->type)
    {
      case QS_FIXED:
        if(_checkFixed(s, g))
          return true; // error
        break;
      case QS_SELECTABLE:
        if(_checkSelectable(s, g))
          return true; // error
        break;
      case QS_VARIABLE:
        if(_checkVariable(s, g))
          return true; // error
        break;
      case QS_TOLERANT:
        if(_checkTolerant(s, g))
          return true;
        break;
      case QS_CUSTOM:
        if(_checkCustom(s, g))
          return true;
        break;
      default:
        fprintf(stderr, "Invalid source type %s\n", _typeStr(s->type));
        _qsSource_spewTypes(s);
        return true; // error
        break;
      }
  }


  // check if current group sample rate is compatible

  return false;

}

void qsSource_setType(struct QsSource *s,
    enum QsSource_Type type,
    float *sampleRates /* one value for QS_FIXED
                          two values min and max for QS_VARIABLE
                          N+1 values (0 terminated) for QS_SELECTABLE
                          NULL of QS_TOLERANT and QS_CUSTOM
                       */)
{
  QS_ASSERT(s);

  s->type = type;

  switch(type)
  {
    case QS_FIXED:
      QS_ASSERT(sampleRates);
      if(s->sampleRates)
      {
        g_free(s->sampleRates);
        s->sampleRates = NULL;
      }
      s->sampleRate = *sampleRates; // one value
      break;
    case QS_VARIABLE:
      QS_ASSERT(sampleRates);
      if(s->sampleRates)
        g_free(s->sampleRates);
      QS_ASSERT(sampleRates[0] && sampleRates[1]);
      s->sampleRates = g_malloc(sizeof(float)*2);
      s->sampleRates[0] = sampleRates[0]; // min
      s->sampleRates[1] = sampleRates[1]; // max
      break;
    case QS_SELECTABLE:
      QS_ASSERT(sampleRates);
      if(s->sampleRates)
        g_free(s->sampleRates);
      int n = 0;
      for(n=0; sampleRates[n]; ++n);
      QS_ASSERT(n);
      s->sampleRates = g_malloc(sizeof(float)*(n+1));
      int i;
      for(i=0; i<n; ++i)
        s->sampleRates[i] = sampleRates[i];
      s->sampleRates[i] = sampleRates[i]; // zero terminated
      break;
    case QS_TOLERANT:
    case QS_CUSTOM:
      if(s->sampleRates)
      {
        g_free(s->sampleRates);
        s->sampleRates = NULL;
      }
      break;
    default:
      fprintf(stderr, "%s() bad type\n", __func__);
      QS_VASSERT(0, "bad type\n");
      if(s->sampleRates)
      {
        g_free(s->sampleRates);
        s->sampleRates = NULL;
      }
      s->type = QS_NONE;
      break;
  }

  _qsSource_checkTypes(s);
}
