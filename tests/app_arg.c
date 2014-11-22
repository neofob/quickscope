/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
#include "quickscope.h"

#define PRINT(x, FMT) printf("%s = \""FMT"\"\n", #x, (x))

// ./app_arg --int 33 --inT=66 --bool --float 2 --double
// ./app_arg --int 33 --inT=66 --bool=1 --float --double=0.3e-2

int main(int argc, char **argv)
{
  printf("%s Command line:", argv[0]);
  int i;
  for(i=0; i<argc; ++i)
    printf(" %s", argv[i]);
  printf("\n\n");

  // copies argc and argv
  qsApp_init(&argc, &argv);

  PRINT(qsApp_int("int", 55), "%d");
  PRINT(qsApp_int("inT", 55), "%d");

  PRINT(qsApp_float("float", 55), "%.10g");
  PRINT(qsApp_float("float", NAN), "%.10g");

  PRINT(qsApp_double("double", 55), "%.20g");
  PRINT(qsApp_double("double", NAN), "%.20g");

  PRINT(qsApp_string("string", "val"), "%s");
  PRINT(qsApp_string("string", ""), "%s");
  PRINT(qsApp_string("string", NULL), "%s");

  PRINT(qsApp_bool("bool", false), "%d");
  PRINT(qsApp_bool("bool", true), "%d");

  printf("******* End %s Output ********\n", argv[0]);

  return 0;
}
