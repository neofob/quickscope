/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include "quickscope.h"

int main(int argc, char **argv)
{
  struct QsSource *s;

  /* In this example we wish to change some of the default settings by
   * making a qsApp before we make a qsSource or any other qs object.
   * qsApp is a singleton, and you always get one whither you call
   * qsApp_init() or not.  qsApp manages all other qs objects. */
  qsApp_init(&argc, &argv);

  /* These parameters being set here are read by various qs objects when
   * they are constructed.  Once the qs object is constructed changing
   * these values will not effect it.  For example when the qsTrace is
   * created below it creates a qsWin which will initialize itself with
   * these particular qsApp options.  This gets around one of the problems
   * with an oscilloscope "too many parameters".  Calling a object create
   * function with 40 parameters is not practical.  So we use qsApp as a
   * constructor parameter sittings object, among other things. */
  qsApp->op_fade = true; // turn on beam fading, could be default but ya.
  qsApp->op_fadePeriod = 4.0F; // seconds
  qsApp->op_fadeDelay =  3.6F; // seconds
  qsApp->op_doubleBuffer = true; // turn on X pixmap buffering
  qsApp->op_grid = false; // grid off
  qsApp->op_axis = false;
  qsApp->op_ticks = false;

  /* This creates a source that solves 3 very famous ordinary differential
   * equations (ODE).  You can use your own ODEs with quickscope but we
   * have this ODE system built in. */
  s = qsLorenz_create( 5000 /* maxNumFrames in a given source read call. */,
      10/*play rate multiplier*/,
      -1,-1,-1,/*Lorenz parameters: sigma, rho, beta; -1 == use default*/
      NULL/*projectionCallback if you wanted to rotated view or something*/,
      NULL/*projectionCallback_data*/,
      NULL/*source group, NULL = make a new group */);

  /* Clearly knowing what parameters to use can only be obvious with of
   * experience.  You need to write your own qsSources to get the feel for
   * it. */

  /* The Lorenz source has 3 channels 0, 1, 2 or x, y, z */

  qsTrace_create(NULL /* QsWin, NULL to make a default Win */,
      s, 0, s, 2, /* x/y source and trace channels are x z Lorenz */
      0.02F,/*xscale*/ 0.02F,/*yscale*/
      0, -0.5, /*xshift, yshift */
      true, /* lines */ 1, 0.2, 0.7 /* RGB line color */);

  qsApp_main(); // This calls gtk_main()

  /* This is not needed in this case since we exit just after. But just so
   * you know, so you can remove all quickscope resources in future code. */
  qsApp_destroy(); 

  return 0;
}
