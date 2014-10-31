/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
/* We use this file for source of the
 *
 *    QsAdjusterFloat, QsAdjusterDouble and QsAdjusterLongDouble
 *
 * by using CPP MACROs as a template..
 */
#include <math.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "assert.h"
#include "base.h"
#include "adjuster.h"
#include "adjuster_priv.h"


/* This is kind of like a template in C but it compiles about 10 times
 * faster than C++ templates. */


#if ! defined DOUBLE && ! defined LONG_DOUBLE && ! defined INT
#  define FLOAT
#endif

#ifdef FLOAT
#  define ADJUSTER_CREATE       qsAdjusterFloat_create
#  define ADJ_TYPE              QsAdjusterFloat
#  define TYPE                  float
#  define TYPE_STR              "float"
#  define DIGITS                7
#  define FABS                  fabsf
#  define FM                    "f"
#  define STRTOF                strtof
char _qs_decPoint = '\0';
#else
extern char _qs_decPoint;
#endif

#ifdef DOUBLE
#  define ADJUSTER_CREATE       qsAdjusterDouble_create
#  define ADJ_TYPE              QsAdjusterDouble
#  define TYPE                  double
#  define TYPE_STR              "double"
#  define DIGITS                12
#  define FABS                  fabs
#  define FM                    "lf"
#  define STRTOF                strtod
#endif

#ifdef LONG_DOUBLE
#  define ADJUSTER_CREATE       qsAdjusterLongDouble_create
#  define ADJ_TYPE              QsAdjusterLongDouble
#  define TYPE                  long double
#  define TYPE_STR              "long double"
#  define DIGITS                16
#  define FABS                  fabsl
#  define FM                    "Lf"
#  define STRTOF                strtold
#endif

#ifdef INT
#  define INT_TYPE
#  define ADJUSTER_CREATE       qsAdjusterInt_create
#  define ADJ_TYPE              QsAdjusterInt
#  define TYPE                  int
#  define TYPE_STR              "int"
#  define DIGITS                10
#  define FABS                  abs
#  define FM                    "d"
#  define STRTOF(X, Y)          strtol((X),(Y),10)
#endif


#define CHAR_LEN                (DIGITS + 6)

#ifndef ADJ_TYPE
#  error "ADJ_TYPE is not defined: fix this stupid file or the make file or something"
#endif



struct ADJ_TYPE
{
  struct QsAdjuster adjuster;
  char *units;
  TYPE *value;
  char strValue[CHAR_LEN];
  char pIndex, // decimal point index in strValue[]
      index,  // index relative to decimal point being edited
      valLen; // length of strValue[] string
  TYPE min, max;
  bool setValue; // flag that we need to setup strValue.
};



// Decimal Point Character
static inline
void init_qs_decPoint(void)
{
#ifndef INT_TYPE
  if(!_qs_decPoint)
  {
    // We get decimal point for this locale the easy way.
    char d[8], *s;
    snprintf(d, 8, "%1.1f", 1.1F);
    for(s = d; *s && *s == '1'; ++s);
    _qs_decPoint = *s;
  }
#endif
}

static inline
TYPE clipValue(struct ADJ_TYPE *adj, TYPE val)
{
  if(val > adj->max)
    val = adj->max;
  else if(val < adj->min)
    val = adj->min;

  return val;
}


static inline
void int_init(struct ADJ_TYPE *adj)
{
#ifdef INT_TYPE
  // Sets index, valLen
  TYPE absMax;
  absMax = (FABS(adj->max) > FABS(adj->min))?FABS(adj->max):FABS(adj->min);
  char str[40];
  adj->valLen = snprintf(str, 40, "%"FM, absMax);
  snprintf(str, 40, "%*.*"FM, adj->valLen, adj->valLen, *adj->value);
  char *s;
  s = str;
  QS_ASSERT(*s);
  adj->index = 0;
  for(; *s && *s == '0'; ++s)
    ++adj->index;
  if(!(*s))
    --adj->index;
  if(adj->min < 0)
  {
    ++adj->index;
    ++adj->valLen;
  }
#endif
}

static inline
void setValue(struct ADJ_TYPE *adj, TYPE val)
{
  // This sets value, valueStr, setValue
  val = clipValue(adj, val);
#ifdef QS_DEBUG
  if(val == adj->max)
    QS_SPEW("%s value is maxed out at %10.10"FM" %s\n",
        ((struct QsAdjuster *)adj)->description, val, adj->units);
  else if(val == adj->min)
    QS_SPEW("%s value is minimumized at %10.10"FM" %s\n",
        ((struct QsAdjuster *)adj)->description, val, adj->units);
#endif

#ifdef INT_TYPE
  int len;
  char sign[2];
  len = adj->valLen;
  sign[0] = '\0';
  if(adj->min < 0)
  {
    sign[0] = (val < 0)? '-':'+';
    sign[1] = '\0';
    --len;
  }

  adj->valLen = snprintf(adj->strValue, CHAR_LEN, "%s%*.*"FM,
        sign, len, len, val);
  *adj->value = val;
  adj->setValue = false;

#else // #ifdef INT_TYPE

  // This sets value, valueStr, index, pIndex, setValue, valLen
  val = clipValue(adj, val);

# ifdef QS_DEBUG
  if(val == adj->max)
    printf("%s value is maxed out at %10.10"FM" %s\n",
        ((struct QsAdjuster *)adj)->description, val, adj->units);
  else if(val == adj->min)
    printf("%s value is minimumized at %10.10"FM" %s\n",
        ((struct QsAdjuster *)adj)->description, val, adj->units);
# endif

  TYPE absMax;
  int len, intLen, digits;
  char sign[40];
  absMax = (FABS(adj->max) > FABS(adj->min))? FABS(adj->max): FABS(adj->min);
  adj->pIndex = intLen = snprintf(sign, 40, "%0.0"FM, absMax);
  len = snprintf(sign, 40, "%0.0"FM, FABS(val));
  sign[0] = '\0';
  if(adj->min < 0)
  {
    sign[0] = (val < 0)? '-':'+';
    sign[1] = '\0';
    ++adj->pIndex;
  }

  digits = DIGITS - len;
  if(digits < 0)
    digits = 1;
  adj->valLen = snprintf(adj->strValue, CHAR_LEN, "%s%*.*s%*.*"FM,
      sign, intLen - len, intLen - len, "000000000000000000",
      digits, digits, FABS(val));

  if(adj->index == CHAR_MAX)
  {
    char *s;
    for(s = adj->strValue; *s && (*s < '1' || *s > '9'); ++s);
    if(*s)
      adj->index = (s - adj->strValue) - adj->pIndex;
    else
      adj->index =  1;
  }
  else if(adj->pIndex + adj->index >= adj->valLen)
    adj->index = adj->valLen - adj->pIndex - 1;

  *adj->value = val;
  adj->setValue = false;

# if 0
printf("adjuster %s \"%s\" %s i=%d\n",
    adj->adjuster.description, adj->strValue, adj->units, adj->index + adj->pIndex);
# endif
#endif // #ifdef INT_TYPE #else
}

static
void getTextRender(struct ADJ_TYPE *adj, char *str,
    size_t maxLen, size_t *len)
{
  int i;

  if(adj->setValue)
    setValue(adj, *adj->value);

  i = adj->pIndex + adj->index;

  QS_ASSERT(i >= 0 && i < adj->valLen);

  *len += snprintf(str, maxLen,
      // it uses gtk_label_set_markup(str) with this string
      "[<span size=\"xx-small\" face=\"cmsy10\" weight=\"bold\">"
      "l $</span>] "
      "<span color=\"#000040\" "INACTIVE_BG_COLOR"> %*.*s</span>"
      "<span color=\"#440F19\" "ACTIVE_BG_COLOR" weight=\"bold\">%c</span>"
      "<span color=\"#000040\" "INACTIVE_BG_COLOR">%s %s%s</span>",
      i, i, adj->strValue, adj->strValue[i], &adj->strValue[i + 1],
      adj->units, (adj->units[0])?" ":"");
//printf("render=%s\n", str);
}

static 
bool reset(struct ADJ_TYPE *adj)
{
  adj->strValue[0] = 0;
  adj->setValue = true;
  return true;
}

static
bool shiftLeft(struct ADJ_TYPE *adj)
{
  if(adj->index + adj->pIndex > 0)
  {
    --adj->index;
#ifndef INT_TYPE
    if(adj->index == 0)
      adj->index = -1;
#endif
    return true; // rerender widget
  }
  return false; // no rendering needed
}

static
bool shiftRight(struct ADJ_TYPE *adj)
{
  if(adj->index + adj->pIndex < adj->valLen - 1)
  {
    ++adj->index;
#ifndef INT_TYPE
    if(adj->index == 0)
      adj->index = 1;
#endif
    return true; // rerender widget
  }
  return false; // no rendering needed
}

static inline
bool incDec_return(struct ADJ_TYPE *adj, TYPE val)
{
  val = clipValue(adj, val);
  if(val != *adj->value)
  {
    *adj->value = val;
    return (adj->setValue = true);
  }
  return adj->setValue;
}

static
bool inc(struct ADJ_TYPE *adj, struct QsWidget *w)
{
  int i, j;
  char s[CHAR_LEN];
  strcpy(s, adj->strValue);
  i = j = adj->index + adj->pIndex;

  // example:
  //
  // s= "-0234.567800"
  //         ^
  //         |
  //

  // The sign can only change from '-' to '+'
  if(s[i] == '-')
  {
    QS_ASSERT(i == 0);
    s[0] = '+';
    return incDec_return(adj, STRTOF(s, NULL));
  }
  else if(s[i] == '+')
  {
    QS_ASSERT(i == 0);
    return adj->setValue;
  }

  if(s[0] != '-')
  {
    // incrementing a positive number

    if(s[0] != '+')
    {
      // It can't be negative
      QS_ASSERT(adj->min >= 0);

      // Are there all '9' from i to the left ?
      while(i >= 0 && (s[i] == '9' || s[i] == _qs_decPoint))
          --i;

      if(i != -1)
      {
        // They are not all '9' from j to the left
        i = j;
        while(s[i] == '9' || s[i] == _qs_decPoint)
        {
          if(s[i] != _qs_decPoint)
            s[i] = '0';
          --i;
        }
        s[i] += 1;
        return incDec_return(adj, STRTOF(s, NULL));
      }
        
      // Yes they are all '9' from j to the left
      return incDec_return(adj, adj->max); // peg it to max!
    }
    else
    {
      // It can be negative and so s[0] == '+'
      QS_ASSERT(adj->min < 0);

      // Are there all '9' from i left to '+' ?
      while(i >= 1 && (s[i] == '9' || s[i] == _qs_decPoint))
        --i;

      if(i != 0)
      {
        // They are not all '9' from j to the left to '+'
        i = j;
        while(s[i] == '9' || s[i] == _qs_decPoint)
        {
          if(s[i] != _qs_decPoint)
            s[i] = '0';
          --i;
        }
        s[i] += 1;
        return incDec_return(adj, STRTOF(s, NULL));
      }
      // Yes they are all '9' from j left to '+'
      return incDec_return(adj, adj->max); // peg to max!
    }
  }

  // The sign is negative
  QS_ASSERT(*adj->value < 0);
      
  // Are there all '0' from i to the left from '-' ?
  while(i >= 1 && (s[i] == '0' || s[i] == _qs_decPoint))
    --i;

  if(i != 0)
  {
    // They are not all '0' to the left from '-'
    i = j;
    while(s[i] == '0' || s[i] == _qs_decPoint)
    {
      if(s[i] != _qs_decPoint)
        s[i] = '9';
      --i;
    }
    s[i] -= 1;
    return incDec_return(adj, STRTOF(s, NULL));
  }

  // They are all '0' to the left of i from '-'
  //
  // Like: "-000.00???"
  //              ^
  //              |
  //
  // We handle it in a way like the reverse of dec()
  // "weired" case, but since there is no "-000.0000000"
  // we do not need to do as much.
  //
  return incDec_return(adj, 0);
}

static
bool dec(struct ADJ_TYPE *adj, struct QsWidget *w)
{
  int i, j;
  char s[CHAR_LEN];
  strcpy(s, adj->strValue);
  i = j = adj->index + adj->pIndex;

  // example:
  // i = 4
  //         |
  //         V
  // s= "-1234.567800"
  //

  // The sign can only change from '+' to '-'
  if(s[i] == '+')
  {
    QS_ASSERT(i == 0);
    s[0] = '-';
    return incDec_return(adj, STRTOF(s, NULL));
  }
  else if(s[i] == '-')
  {
    QS_ASSERT(i == 0);
    return adj->setValue;
  }

  if(s[0] != '-')
  {
    // decrementing a positive number

    if(s[0] != '+')
    {
      // It can't be negative
      QS_ASSERT(adj->min >= 0);

      // Are there all '0' from i to the left ?
      while(i >= 0 && (s[i] == '0' || s[i] == _qs_decPoint))
          --i;

      if(i != -1)
      {
        // They are not all '0' from j to the left
        i = j;
        while(s[i] == '0' || s[i] == _qs_decPoint)
        {
          if(s[i] != _qs_decPoint)
            s[i] = '9';
          --i;
        }
        s[i] -= 1;
        return incDec_return(adj, STRTOF(s, NULL));
      }
        
      // Yes they are all '0' from j to the left
      // and it can't be negative so peg it to the min.
      return incDec_return(adj, adj->min);
    }
    else
    {
      // It can be negative so s[0] == '+'
      QS_ASSERT(adj->min < 0);
      
      // Are there all '0' from i left to '+' ?
      while(i >= 1 && (s[i] == '0' || s[i] == _qs_decPoint))
        --i;

      if(i != 0)
      {
        // They are not all '0' from j to the left
        i = j;
        while(s[i] == '0' || s[i] == _qs_decPoint)
        {
          if(s[i] != _qs_decPoint)
            s[i] = '9';
          --i;
        }
        s[i] -= 1;
        return incDec_return(adj, STRTOF(s, NULL));
      }
      else
      {
        // Yes, they are all '0' from j left to '+'
        // This is the "weired" case.  What to do is
        // not obvious.  We let the special value
        // "+000.00000" appear as in the sequence:
        //
        //         |                    |                 |
        //         V      ==>           V      ==>        *
        // "+000.000234"        "+000.000000"       "-0.001000"
        //
        // Note the intervals are not the same, but it looks nicer
        // then the case with constant differences.  There are at
        // least 4 ways to do it:
        //
        //   1. like above
        //   2. keep the differences constant (for above 0.001)
        //   3. skip the all zeros value and just go to "-0.001234"
        //   4. from "+000.000234" -> "+000.000000" -> "-0.001234"
        //   5. Yes, there are more possibilities
        //
        //   We are using 1.
        //

        i = j + 1;

        while(s[i] == '0' || s[i] == _qs_decPoint)
          i++;

        if(!s[i])
        {
          //                              |
          //                              V
          // we have all '0' like: "+000.0000000"
          //
          
          s[j] = '1';
          s[0] = '-'; // flip the sign
          return incDec_return(adj, STRTOF(s, NULL));
        }

        //                         |
        //                         V
        // we have 0's like: "+000.00????"
        //
        // where '????' are not all '0'
        //

        return incDec_return(adj, 0);
      }
    }
  }

  // The sign is negative
  QS_ASSERT(adj->min < 0 && *adj->value < 0);
      
  // Are there all '9' from i to the left from '-' ?
  while(i >= 1 && (s[i] == '9' || s[i] == _qs_decPoint))
    --i;

  if(i != 0)
  {
    // They are not all '9' to the left from '-'
    i = j;
    while(s[i] == '9' || s[i] == _qs_decPoint)
    {
      if(s[i] != _qs_decPoint)
        s[i] = '0';
      --i;
    }
    s[i] += 1;
    return incDec_return(adj, STRTOF(s, NULL));
  }

  // They are all '9' to the left of j from '-'
  //
  //              |
  //              V
  // Like: "-999.99????"
  //
  // peg it to negative min
  return incDec_return(adj, adj->min);
}

static
void destroy(struct ADJ_TYPE *adj)
{
    QS_ASSERT(adj);
    QS_ASSERT(adj->units);
#ifdef QS_DEBUG
    memset(adj->units, 0, strlen(adj->units));
#endif
    g_free(adj->units);

    // _qsAdjuster_checkBaseDestroy() is not necessary
    // since only the qsAdjuster_destroy() can call this
    // from adj->destroy, but maybe someday
    // this may be inherited and we'll need to expose this
    // destroy() function and call:
    //_qsAdjuster_checkBaseDestroy(adj);
}

struct QsAdjuster *ADJUSTER_CREATE(struct QsAdjusterList *adjs,
    const char *description, const char *units,
    TYPE *value, TYPE min, TYPE max,
    void (*changeValueCallback)(void *data), void *data)
{
  struct ADJ_TYPE *adj;
  QS_ASSERT(value);
  QS_ASSERT(*value <= max);
  QS_ASSERT(*value >= min);

  adj = _qsAdjuster_create(adjs, description,
      changeValueCallback, data, sizeof(*adj));
  adj->min = min;
  adj->max = max;
  adj->index = CHAR_MAX;
  adj->value = value;
  adj->setValue = true;
  adj->units = g_strdup(units);
  _qsAdjuster_addSubDestroy(adj, destroy);
  adj->adjuster.getTextRender =
    (void (*)(void *obj, char *str, size_t maxLen, size_t *len))
    getTextRender;
  adj->adjuster.shiftLeft  = (bool (*)(void *)) shiftLeft;
  adj->adjuster.shiftRight = (bool (*)(void *)) shiftRight;
  adj->adjuster.inc        = (bool (*)(void *, struct QsWidget *)) inc;
  adj->adjuster.dec        = (bool (*)(void *, struct QsWidget *)) dec;
  adj->adjuster.reset      = (bool (*)(void *)) reset;

  init_qs_decPoint();

  int_init(adj);

  return (struct QsAdjuster *) adj;
}

