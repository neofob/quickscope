/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
struct QsSweep; // QsSweep is opaque 
 

/* This makes a QsSource that is a triggered sweep */
struct QsSource *qsSweep_create(
    float period, float level, int slope, long double holdOff,
    long double delay,
    struct QsSource *sourceIn, int channelNum);
/* qsComposer_destroy() or qsSource_destroy() may be used to
 * destroy a QsSweep object. */

