/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

#include "config.h"
#include <gnu/lib-names.h>
#include <stdio.h>
#include <stdlib.h>

/* This is the stuff that will tell the compiler to make this library,
 * libshm_arena.so, one that can be executed. */
#ifdef __i386__
# define LIBDIR  "/lib/"
#endif

#ifdef __x86_64__
# define LIBDIR  "/lib64/"
#endif

const char my_interp[] __attribute__((section(".interp"))) = LIBDIR LD_SO;

/** \internal
 * \brief entry point for using the library as an executable.
 *
 * lib_main() is the entry point for using the library as an
 * executable.  It prints some helpful configuration information. */

void _lib_main(void)
{
  printf(" =============================================\n"
	 "    " PACKAGE_NAME "  version: " VERSION     "\n"
	 " =============================================\n"
	 "\n"
	 "  Compiled ("__FILE__") on date: "__DATE__ "\n"
	 "                             at: "__TIME__"\n"
         "    Library version: " LIB_VERSION "\n"
	 "\n"
	 " ------------------------------------------------\n"
	 "      BUILD OPTIONS\n"
	 "\n"
#ifdef QS_DEBUG
	 "    This is a DEBUG BUILD.  QS_DEBUG was defined.\n"
#else
	 "    This is NOT a DEBUG BUILD.  QS_DEBUG was not defined.\n"
#endif
	 " ------------------------------------------------\n"
	 "\n"
	 "  The quickscope homepage is:\n"
	 "   " PACKAGE_URL "\n"
	 "\n"
	 "  Copyright (C) 2012-2014 - Lance Arsenault\n"
	 "  This is free software licensed under the GNU GPL (v3).\n"
	 );

  /* This needs to call exit and not return 0 since this is not main()
   * and main is not called. */
  exit(0);
}
