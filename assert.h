/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#ifndef _QS_DEBUG_H_
#  error  "You must include debug.h before this file"
#endif


#ifndef QS_DEBUG

#  define QS_ASSERT(x)        /* empty macro */
#  define QS_VASSERT(x, ...)  /* empty macro */

#else

#  define __printf(a,b)  __attribute__((format(printf,a,b)))

extern
void qsAssert(const char *file, int line,
    const char *func, long bool_arg,
    const char *arg, const char *format, ...)
    __printf(6,7);

#  define QS_ASSERT(x)          qsAssert(__FILE__, __LINE__,            \
                               __func__, (long) (x), #x, " ")
#  define QS_VASSERT(x, fmt, ...)                                       \
                                qsAssert(__FILE__, __LINE__, __func__,  \
                                (long) (x), #x, fmt, ##__VA_ARGS__)
#endif

