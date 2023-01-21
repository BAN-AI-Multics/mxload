 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

#ifdef MXMAP
#define main_mxmap main
#define mxlexit_mxmap mxlexit
#define mxlargs_mxmap mxlargs
#endif

#include "mxload.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "dirsep.h"
#include "getopt.h"
#include "mapprint.h"
#include "mxbitio.h"
#include "mxlargs.h"
#include "mxload.h"
#include "preamble.h"
#include "rdbkrcd.h"
#include "tempfile.h"

struct MXLARGS mxlargs_mxmap;
static void mxmap_usage();

#ifdef ANSI_FUNC
int
main_mxmap (int argc, char *argv[])
#else
int
main_mxmap(argc, argv)

int argc;
char *argv[];
#endif

{
  MXBITFILE *infile;
  char *temp_path;
  struct PREAMBLE preamble;
  struct BRANCH_PREAMBLE branch_preamble;
  struct DIRLIST_PREAMBLE dirlist_preamble;
  int i;
  int option_letter;
  MXBITFILE *preamble_file;

  memset(&mxlargs, 0, sizeof ( mxlargs ));
  while (( option_letter = getopt(argc, argv, "vx")) != EOF)
  {
    switch (option_letter)
    {
    case 'v': /* Verbose:  long listing output */
      mxlargs.verbose = 1;
      break;

    case 'x': /* Extremely verbose:
               * full listing output */
      mxlargs.extremely_verbose = 1;
      break;

    case '?': /* ? or unrecognized option */
      mxmap_usage();
      mxlexit(4);
    }
  }

  mxlargs.map_file = stdout;

  /*
   * Adjust arguments to skip over
   * program name and options
   */

  argc -= optind;
  argv += optind;

  if (argc != 1)
  {
    mxmap_usage();
  }

  if (( infile = open_mxbit_file(argv[0], "r")) == NULL)
  {
    fprintf(stderr, "Cannot open Multics backup file %s.\n", argv[0]);
    exit(1);
  }

  preamble_file = get_temp_file("wt", "preamble");

  while (rdbkrcd(infile, preamble_file, &preamble) == 0
         && rdseg(infile, NULL, &preamble) == 0)
  {
    if (preamble.record_type == SEGMENT_RECORD)
    {
      get_branch_preamble(preamble_file, &branch_preamble, &preamble);
      display_branch_preamble(&preamble, &branch_preamble);
      if (branch_preamble.addnames != NULL)
      {
        free(branch_preamble.addnames);
      }

      if (branch_preamble.acl != NULL)
      {
        free((char *)branch_preamble.acl);
      }
    }
    else if (preamble.record_type == DIRECTORY_RECORD)
    {
      get_branch_preamble(preamble_file, &branch_preamble, &preamble);
      display_branch_preamble(&preamble, &branch_preamble);
      if (branch_preamble.addnames != NULL)
      {
        free(branch_preamble.addnames);
      }

      if (branch_preamble.acl != NULL)
      {
        free((char *)branch_preamble.acl);
      }
    }
    else if (preamble.record_type == DIRLIST_RECORD)
    {
      get_dirlist_preamble(preamble_file, &dirlist_preamble, &preamble);
      display_dirlist_preamble(&preamble, &dirlist_preamble);
      for (i = 0; i < dirlist_preamble.nlinks; ++i)
      {
        if (( dirlist_preamble.links + i )->addnames != NULL)
        {
          free(( dirlist_preamble.links + i )->addnames);
        }
      }

      free((char *)dirlist_preamble.links);
    }
  }

  mxlexit(0);
  return( 0 );
}

#ifdef ANSI_FUNC
void
mxlexit_mxmap (int status)
#else
void
mxlexit_mxmap(status)

int status;
#endif

{
  cleanup_temp_files();
  exit(status);
}

 /*
  * Print usage
  * message
  */

#ifdef ANSI_FUNC
static void
mxmap_usage (void)
#else
static void
mxmap_usage()
#endif
{
  fprintf(stderr, "Usage: mxmap [-options] dump_file\n");
  fprintf(stderr, "\nOptions are:\n");
  fprintf(stderr, "-v\t\t= verbose; produce long map\n");
  fprintf(stderr, "-x\t\t= extremely verbose; produce full map\n");
  mxlexit(4);
}
