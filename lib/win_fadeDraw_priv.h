/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#ifndef _QS_WIN_H_
#   error "win.h must be included before this file, fadeDraw_priv.h"
#endif

#ifdef LINEAR_FADE
static inline
float _qsWin_fadeIntensity(float period, long double t0, long double t)
{
  if(t0 > t)
    return 1.0F;

  if(period > 0.000001F)
    return (period + t0 - t)/period;
  // The fade period is very small, so we act like it's zero.
  // So there's no fade, just "on" to "off" after the delay.
  return 0.0F;
}
#else
static inline
float _qsWin_fadeIntensity(float alpha, long double t0, long double t)
{
  if(alpha > -1.0e+7F)
  {
    float I;
    I = t - t0;
    //I = (period + t0 - t)/period;
    if(I <= 0.0F)
      return 1.0F;
    return expf(alpha * I);
  }
  // The fade period is very small, so we act like it's zero.
  // So there's no fade, just "on" to "off" after the delay.
  if(t0 > t)
    return 1.0F;
  else
    return 0.0F;
}
#endif

