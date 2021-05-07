 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

 /* MXASCI */

#ifdef MXASCII
#define main_mxascii main
#define mxlexit_mxascii mxlexit
#endif

 /* 
  * Read a Multics file and translate
  * 9-bit ASCII to 8-bit
  */

#include "mxload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "copybits.h"
#include "mxbitio.h"

#ifdef ANSI_FUNC
int 
main_mxascii (int argc, char *argv[])
#else
int
main_mxascii(argc, argv)

int argc;
char *argv[];
#endif

{
  MXBITFILE *ninebit_file;
  FILE *f;
  struct stat statbuf;
  long bitcount;

  if (argc < 2 || argc > 3)
  {
    fprintf(stderr, "Usage:  %s ninebitfile [eightbitfile]\n", argv[0]);
    exit(1);
  }

  if (( ninebit_file = open_mxbit_file(argv[1], "rt")) == NULL)
  {
    fprintf(stderr, "Cannot open %s.\n", argv[1]);
    exit(1);
  }

  stat(argv[1], &statbuf);

  bitcount = statbuf.st_size * 8L;
  bitcount -= bitcount % 9;

  if (argc == 2)
  {
    copy_8bit_to_file(ninebit_file, stdout, bitcount);
  }
  else
  {
    if (( f = fopen(argv[2], "r")) != NULL)
    {
      fprintf(stderr, "%s already exists.\n", argv[2]);
      exit(1);
    }

    fclose(f);
    copy_8bit(ninebit_file, argv[2], bitcount);
  }
}

 /*
  * Exit after cleaning up.
  * Called by various subrs.
  */

#ifdef ANSI_FUNC
void 
mxlexit_mxascii (int status)
#else
void
mxlexit_mxascii(status)

int status;
#endif

{
  exit(status);
}
