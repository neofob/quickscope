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
const char *TypeStr(enum QsSource_Type type)
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
    case QS_NONE:
      return "QS_NONE";
      break;
    default:
      QS_ASSERT(0);
      return "UNKNOWN";
      break;
  }
}

static
void SpewType(enum QsSource_Type type,
    const float *sampleRates, float sampleRate)
{
  fprintf(stderr, "%s\n", TypeStr(type));
  switch(type)
  {
    case QS_FIXED:
      fprintf(stderr, "frame sample rate = %g\n", sampleRate);
      break;
    case QS_SELECTABLE:
      fprintf(stderr, "frame sample rates (Hz):");
      while(*sampleRates)
        fprintf(stderr, " %g", *sampleRates++);
      fprintf(stderr, "\n");
      break;
    case QS_VARIABLE:
      fprintf(stderr, "frame sample rates must be at and between "
          "(min) %f Hz and (max) %f Hz\n",
          sampleRates[0], sampleRates[1]); 
      break;
    case QS_TOLERANT:
      fprintf(stderr, "Works with any frame sample rate with default rate= %g\n",
          sampleRate);
      if(isinf(sampleRate))
        fprintf(stderr, "%g means that the number of frames per scope display\n"
            " cycle is only limited by the max number of frame the buffer holds\n",
            sampleRate);
      fprintf(stderr, "The default range is between (min) %g Hz and (max) %g Hz\n",
          sampleRates[0], sampleRates[1]);
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
  SpewType(s->type, s->sampleRates, s->sampleRate);
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
            "There are incompatible %s and %s sources (at least)\n",
            TypeStr(s->type), TypeStr(g->type));
  _qsSource_spewTypes(s);

  QS_ASSERT(0);

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

// returns false on success, or true otherwise.
// Make the group, g, compatible with the source, s;
// or fail trying.
static inline
bool CheckFixed(const struct QsSource *s, struct QsGroup *g)
{
  QS_ASSERT(s);
  QS_ASSERT(!s->sampleRates);
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
      if(g->sampleRates[i])
        return _qsGroup_setFixed(g, sampleRate);
      return Incompatible(s, g);
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
      return false; // success
    break;
    case QS_NONE:
    case QS_TOLERANT:
      return _qsGroup_setFixed(g, sampleRate);
      break;
    //case QS_CUSTOM:
    default: // error
      return Incompatible(s, g);
    break;
  }
}

static inline
bool CheckSelectable(const struct QsSource *s, struct QsGroup *g)
{
  QS_ASSERT(s);
  QS_ASSERT(s->sampleRates && *s->sampleRates && s->sampleRate);
  QS_ASSERT(s->type == QS_SELECTABLE);

  switch(g->type)
  {
    case QS_SELECTABLE:
      {
        int i, j;
        size_t n = 0;
        float oldSampleRate, *commonRates = NULL;
        oldSampleRate = g->sampleRate;
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
              else if(!g->sampleRate && oldSampleRate == g->sampleRates[i])
                g->sampleRate = oldSampleRate;
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
        float oldSampleRate, *commonRates = NULL;
        oldSampleRate = g->sampleRate;
        g->sampleRate = 0;
        for(i=0; s->sampleRates[i]; ++i)
          if(s->sampleRates[i] >= g->sampleRates[0] // >= min
            && s->sampleRates[i] <= g->sampleRates[1]) // <= max
          {
            commonRates = g_realloc(commonRates, sizeof(float)*(n + 2));
            commonRates[n++] = g->sampleRates[i];
            commonRates[n] = 0; // terminator

            if(s->sampleRate == s->sampleRates[i])
              g->sampleRate = s->sampleRate;
            else if(!g->sampleRate && oldSampleRate == s->sampleRates[i])
              g->sampleRate = oldSampleRate;
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
        QS_ASSERT(!g->sampleRates);
        int i;
        for(i=0; s->sampleRates[i]; ++i)
          if(s->sampleRates[i] == g->sampleRate)
            break;

        if(s->sampleRates[i])
          return false; // success
      }
      return Incompatible(s, g);
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
        g->sampleRates[i] = 0; // terminator
        g->type = QS_SELECTABLE;
        g->sampleRate = s->sampleRate;
      }
      return false;
      break;
    default: // QS_CUSTOM
      return Incompatible(s, g);
    break;
  }
}

static inline
bool CheckVariable(const struct QsSource *s, struct QsGroup *g)
{
  QS_ASSERT(s->type == QS_VARIABLE);

  switch(g->type)
  {
    case QS_NONE:
      g->sampleRates = g_malloc(sizeof(float)*2);
    case QS_TOLERANT:
      QS_ASSERT(g->sampleRates);
      g->sampleRates[0] = s->sampleRates[0]; // min
      g->sampleRates[1] = s->sampleRates[1]; // max
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
      else if(!(g->sampleRate >= g->sampleRates[0] &&
        g->sampleRate <= g->sampleRates[1]))
        g->sampleRate = 0.5F *
          (g->sampleRates[0] + g->sampleRates[1]);
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
        float oldSampleRate, *commonRates = NULL;
        oldSampleRate = g->sampleRate;
        g->sampleRate = 0;
        for(i=0; g->sampleRates[i]; ++i)
          if(g->sampleRates[i] >= s->sampleRates[0] // >= min
            && g->sampleRates[i] <= s->sampleRates[1]) // <= max
          {
            commonRates = g_realloc(commonRates, sizeof(float)*(n + 2));
            commonRates[n++] = g->sampleRates[i];
            commonRates[n] = 0; // terminator
            if(g->sampleRates[i] == s->sampleRate)
              g->sampleRate = s->sampleRate;
            else if(!g->sampleRate && oldSampleRate == g->sampleRates[i])
              g->sampleRate = oldSampleRate;
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
}

static inline
bool CheckTolerant(const struct QsSource *s, struct QsGroup *g)
{
  QS_ASSERT(s->type == QS_TOLERANT);

  switch(g->type)
  {
    case QS_FIXED:
    case QS_SELECTABLE:
    case QS_VARIABLE:
      return false;
      break;
    case QS_TOLERANT:
      // The sampleRate range is the union of the min/max intervals
      // with possible gap between removed.
      if(g->sampleRates[0] > s->sampleRates[0])
        g->sampleRates[0] = s->sampleRates[0];
      if(g->sampleRates[1] < s->sampleRates[1])
        g->sampleRates[1] = s->sampleRates[1];
      if(g->sampleRate < s->sampleRate)
        // We make the current rate the largest
        // of all requested.
        g->sampleRate = s->sampleRate;
      return false;
      break;
    case QS_NONE:
      g->type = QS_TOLERANT;
      g->sampleRate = s->sampleRate;
      g->sampleRates = g_malloc(sizeof(float)*2);
      g->sampleRates[0] = s->sampleRates[0];
      g->sampleRates[1] = s->sampleRates[1];
      return false;
      break;
    // case QS_CUSTOM:
    default:
      return Incompatible(s, g);
      break;
  }
}

static inline
bool CheckCustom(const struct QsSource *s, struct QsGroup *g)
{
  QS_ASSERT(s->type == QS_CUSTOM);
  QS_ASSERT(s->sampleRate == 0);

  switch(g->type)
  {
    case QS_NONE:
      g->type = QS_CUSTOM;
      g->sampleRate = 0;
    case QS_CUSTOM:
      return false;
      break;
    default:
      return Incompatible(s, g);
      break;
  }
}

// setup source group rates and shit
// returns false on success
bool _qsSource_checkTypes(struct QsSource *s)
{
  QS_ASSERT(s);
  struct QsGroup *g;
  g = s->group;
  QS_ASSERT(g);

  QS_ASSERT(g->sourceTypeChange);

  g->sourceTypeChange = false;

  // First reset the group rate settings
  if(g->sampleRates)
  {
    g_free(g->sampleRates);
    g->sampleRates = NULL;
  }
  g->sampleRate = 0;
  g->type = QS_NONE;

  // The g->sources is a list of source from last to
  // first (master).  We override g->sampleRate as we
  // go through all the sources sample rates, if they
  // are compatible.

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
        if(CheckFixed(s, g))
          return true; // error
        break;
      case QS_SELECTABLE:
        if(CheckSelectable(s, g))
          return true; // error
        break;
      case QS_VARIABLE:
        if(CheckVariable(s, g))
          return true; // error
        break;
      case QS_TOLERANT:
        if(CheckTolerant(s, g))
          return true;
        break;
      case QS_CUSTOM:
        if(CheckCustom(s, g))
          return true;
        break;
      default:
        fprintf(stderr, "Invalid source type %s\n", TypeStr(s->type));
        _qsSource_spewTypes(s);
        return true; // error
        break;
      }
  }

#if 1
#ifdef QS_DEBUG
  QS_SPEW("Source Group\n");
  _qsSource_spewTypes(s);
  fprintf(stderr, "\n");
#endif
#endif

  return false; // success
}

static inline
void SetSelectableSampleRate(const float *rates, float *out, float sampleRate)
{
  for(*out = *rates++; *rates; ++rates)
    if(fabs(sampleRate - *rates) < fabs(sampleRate - *out))
      *out = *rates;
}

bool qsSource_setFrameRateType(struct QsSource *s,
    enum QsSource_Type type,
    const float *sampleRates /* NULL for QS_FIXED
                          two values min and max for QS_VARIABLE
                          N+1 values (0 terminated) for QS_SELECTABLE
                          NULL of QS_TOLERANT and QS_CUSTOM
                       */,
    float sampleRate /*starting, default, sample rate*/)
{
  QS_ASSERT(s);
  QS_ASSERT(type == QS_CUSTOM || sampleRate > 0);
  QS_ASSERT(s->group);

  s->type = type;
  s->sampleRate = sampleRate;

  switch(type)
  {
    case QS_FIXED:
      if(s->sampleRates)
      {
        g_free(s->sampleRates);
        s->sampleRates = NULL;
      }
      break;
    case QS_VARIABLE:
    case QS_TOLERANT:
      QS_ASSERT(sampleRates);
      if(s->sampleRates)
        g_free(s->sampleRates);
      QS_ASSERT(sampleRates[0] > 0);
      QS_ASSERT(sampleRates[0] <= sampleRates[1]);
      QS_ASSERT(sampleRate > 0);

      s->sampleRates = g_malloc(sizeof(float)*2);
      s->sampleRates[0] = sampleRates[0]; // min
      s->sampleRates[1] = sampleRates[1]; // max
      if(sampleRate < s->sampleRates[0])
      {
        if(type == QS_TOLERANT)
          s->sampleRates[0] = sampleRate;
        else // type == QS_VARIABLE
          sampleRate = s->sampleRates[0];
      }
      if(sampleRate > s->sampleRates[1])
      {
        if(type == QS_TOLERANT)
          s->sampleRates[1] = sampleRate;
        else // type == QS_VARIABLE
          sampleRate = s->sampleRates[1];
      }
      break;
    case QS_SELECTABLE:
      {
        QS_ASSERT(sampleRates);
        QS_ASSERT(sampleRates[0] > 0);
        if(s->sampleRates)
          g_free(s->sampleRates);
    
        int n, end, i;
        for(n=0; sampleRates[n]; ++n);
        float *sr;
        sr = g_malloc(sizeof(float)*(n+1));
        for(i=0; i<n; ++i)
          sr[i] = sampleRates[i];
        sr[i] = sampleRates[i]; // zero terminated

        // bubble sort: they should be ordered already
        for(end = n-1; end;)
        {
          int last;
          for(last=i=0; i+1<=end; ++i)
          {
            if(sr[i] > sr[i+1])
            {
              QS_ASSERT(sr[i] > 0);
              float r;
              r = sr[i];
              sr[i] = sr[i+1];
              sr[i+1] = r;
              last = i;
            }
            QS_VASSERT(sr[i] != sr[i+1],
              "duplicate frame rate (%f) in list",
              sr[i]);
          }
          end = last;
        }
        s->sampleRates = sr;
        SetSelectableSampleRate(s->sampleRates, &s->sampleRate, sampleRate);
        QS_SPEW("set sample rate to %g, %g was requested\n",
              s->sampleRate, sampleRate);
      }
      break;
    case QS_CUSTOM:
      s->sampleRate = 0;
      if(s->sampleRates)
      {
        g_free(s->sampleRates);
        s->sampleRates = NULL;
      }
      break;
    default:
      fprintf(stderr, "Error: %s() bad type\n", __func__);
      QS_VASSERT(0, "bad source type\n");
      if(s->sampleRates)
      {
        g_free(s->sampleRates);
        s->sampleRates = NULL;
      }
      s->type = QS_NONE;
      break;
  }

  s->group->sourceTypeChange = true;

  return _qsSource_checkTypes(s);
}

static
void _qsSource_setGroupFrameRate(const struct QsSource *si, struct QsGroup *g)
{
  QS_ASSERT(g);

  // source s has requested that it be the group frame rate
  // so lets see about that...
  // The rate for the group must be compatible with all sources in the
  // group.  The group is a type and has rates that is compatible with
  // all the sources in the group, so we don't need to check this
  // requested rate with all the sources, just the group.

  if(g->sampleRate == si->sampleRate)
    return;

  switch(g->type)
  {
    case QS_TOLERANT:
      // all sources are QS_TOLERANT
      // find the max frame sample rate
      {
        GSList *l;
        g->sampleRate = 0;
        for(l=g->sources; l; l=l->next)
        {
          struct QsSource *s;
          s = l->data;
          if(g->sampleRate < s->sampleRate)
            g->sampleRate = s->sampleRate;
        }
      }
      break;
    case QS_VARIABLE:
      // set to the largest and make it in range
      QS_ASSERT(g->sampleRate >= g->sampleRates[0]);
      QS_ASSERT(g->sampleRate <= g->sampleRates[1]);
      {
        GSList *l;
        g->sampleRate = 0;
        for(l=g->sources; l; l=l->next)
        {
          struct QsSource *s;
          s = l->data;
          if(g->sampleRate < s->sampleRate)
            g->sampleRate = s->sampleRate;
        }
      }
      // make it in range
      if(g->sampleRate < g->sampleRates[0])
          g->sampleRate = g->sampleRates[0];
      else if(g->sampleRate > g->sampleRates[1])
        g->sampleRate = g->sampleRates[1];
      break;
    case QS_SELECTABLE:
      {
        // Find the highest frame sample rate
        GSList *l;
        g->sampleRate = 0;
        for(l=g->sources; l; l=l->next)
        {
          struct QsSource *s;
          s = l->data;
          if(g->sampleRate < s->sampleRate)
            g->sampleRate = s->sampleRate;
        }

        SetSelectableSampleRate(g->sampleRates, &g->sampleRate, g->sampleRate);
        QS_SPEW("set group sample rate to %g\n", g->sampleRate);
      }
      break;
    case QS_FIXED:
      // We cannot change the frame sample rate
      break;
    default:
      QS_VASSERT(0, "Write more case code here (WTF)");
      break;
  }
}


// returns true on success even if the rate stays the same.
// else returns false and the rate stays the same.
void qsSource_setFrameRate(struct QsSource *s, float sampleRate)
{
  QS_ASSERT(s);
  
  switch(s->type)
  {
    case QS_TOLERANT:
      QS_ASSERT(sampleRate > 0);
      if(sampleRate <= 0)
        return;
      s->sampleRate = sampleRate;
      if(s->sampleRates[0] > sampleRate)
        s->sampleRates[0] = sampleRate; // min
      if(s->sampleRates[1] < sampleRate)
        s->sampleRates[1] = sampleRate; // max
      break;
    case QS_VARIABLE:
      if(s->sampleRates[0] <= sampleRate &&
          s->sampleRates[1] >= sampleRate)
      {
        s->sampleRate = sampleRate;
        break;
      }
      return;
      break;
    case QS_SELECTABLE:
      SetSelectableSampleRate(s->sampleRates, &s->sampleRate, sampleRate);
      QS_SPEW("set sample rate to %g, %g was requested\n",
          s->sampleRate, sampleRate);
      break;
    case QS_FIXED:
      QS_ASSERT(0);
      return;
      break;
    case QS_CUSTOM:
      QS_ASSERT(s->sampleRate == 0);
      QS_VASSERT(0, "A QS_CUSTOM source should not call this\n");
      return;
      break;
    case QS_NONE:
      QS_VASSERT(0, "bad source frame rate type QS_NONE");
    default:
      QS_VASSERT(0, "bad source frame rate type");
      return;
      break;
  }

  _qsSource_setGroupFrameRate(s, s->group);
}
