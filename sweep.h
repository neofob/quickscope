/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
struct QsSweep; // QsSweep is opaque 
 

/* This makes a QsSource that is a triggered sweep */
struct QsSource *qsSweep_create(
    float period, float level, int slope, float holdOff,
    float delay, struct QsSource *sourceIn, int channelNum);
/* The user may use qsSource_destroy() to destroy a QsSweep
 * object if they are so inclined.  qsSource_destroy() will
 * call the inheriting QsSweep objects private destroy thingy.
 */
