
// http://.wikipedia.org/wiki/Lissajous_curve

#include <quickscope.h>

int main(int argc, char **argv)
{
  struct QsSource *sin, *cos;

  qsApp_init(&argc, &argv);

  qsApp->op_fade = true;
  qsApp->op_fadePeriod = 1.5; // seconds
  qsApp->op_fadeDelay = 6.5; // seconds

  sin = qsSin_create( 500 /*maxNumFrames*/,
        0.45F /*amplitude*/, 0.7F /*period in seconds*/,
        0.0F*M_PI /*phase shift*/,
        100 /*samples-per-period*/,
        NULL /* source group, NULL => create one */);

  cos = qsSin_create( 500 /*maxNumFrames*/,
        0.45F /*amplitude*/, 1.407F /*period*/,
        0.5*M_PI /*phase shift*/,
        100 /*samples-per-period*/,
        sin /* source group, sin and cos are now a group together */);

  qsTrace_create(NULL,
      cos, 0, /*X-source, channel number*/
      sin, 0, /*Y-source, channel number*/
      1.0F, 1.0F, 0.0F, 0.0F, /* xscale, yscale, xshift, yshift */
      true, /* lines */ 1.0F, 0.9F, 0.1F /* RGB line color */);

  qsApp_main();

  return 0;
}
