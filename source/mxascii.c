                  /********************************\
                  *     Copyright (C) 1987 by      *
                  * Cygnus Cybernetics Corporation *
                  *    and Oxford Systems, Inc.    *
                  \********************************/

/* MXASCI */

/* Read a Multics file and translate 9-bit ASCII to 8-bit */

#include "mxload.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "copybits.h"
#include "mxbitio.h"

main (argc, argv)

int argc;
char *argv[];

{
     MXBITFILE      *ninebit_file;
     FILE            *f;
     struct stat     statbuf;
     long            bitcount;

     if (argc < 2 || argc > 3)
           {
           fprintf (stderr, "Usage:  mxascii ninebitfile [eightbitfile]\n");
           exit (1);
     }

     if ((ninebit_file = open_mxbit_file (argv[1], "rt")) == NULL) 
           {
           fprintf (stderr,
                "Cannot open %s.\n", argv[1]);
           exit (1);
     }

     stat (argv[1], &statbuf);
     
     bitcount = statbuf.st_size * 8L;
     bitcount -= bitcount % 9;

     if (argc == 2)
           copy_8bit_to_file (ninebit_file, stdout, bitcount);
     else 
           {
           if ((f = fopen(argv[2], "r")) != NULL)
                {
                fprintf (stderr, "%s already exists.\n", argv[2]);
                exit (1);
           }
           fclose (f);
           copy_8bit (ninebit_file, argv[2], bitcount);
     }
}

/* Exit after cleaning up.  Called by various subrs. */
void mxlexit (status)

int                 status;

{
     exit (status);
}
