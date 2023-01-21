 /********************************\
 *    Copyright (C) 1987 by       *
 * Cygnus Cybernetics Corporation *
 *   and Oxford Systems, Inc.     *
 \********************************/

 /* MXMBX */

 /*
  * Read Multics mailboxes and retrieve
  * messages. This is basically a
  * lobotomized version of mxarc.
  */

#ifdef MXMBX
#define main_mxmbx main
#define mxlexit_mxmbx mxlexit
#define process_seg_mxmbx process_seg
#define mxlargs_mxmbx mxlargs
#endif

#include "mxload.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include "aclmodes.h"
#include "copybits.h"
#include "cvpath.h"
#include "dirsep.h"
#include "getopt.h"
#include "gettype.h"
#include "mapprint.h"
#include "mxbitio.h"
#include "mxlargs.h"
#include "mxlopts.h"
#include "mxmseg.h"
#include "optptr.h"
#include "parsctl.h"
#include "preamble.h"
#include "rdbkrcd.h"
#include "tempfile.h"
#include "utime.h"

static void usage();
static SIGT onintr();
static void convert_seg();
static void set_attrs();

struct MXLARGS mxlargs_mxmbx;
static char renamed_mbx[400] = "";

#ifdef ANSI_FUNC
int 
main_mxmbx (int argc, char *argv[])
#else
int
main_mxmbx(argc, argv)

int argc;
char *argv[];
#endif

{
  char *temp_path;
  struct PREAMBLE preamble;
  struct BRANCH_PREAMBLE branch_preamble;
  struct DIRLIST_PREAMBLE dirlist_preamble;
  struct MXLOPTS *retrieval_options_ptr;
  int option_letter;
  char *control_files[50];
  int n_control_files;
  int unpack_sw;
  int table_sw;
  int repack_sw;
  int extract_sw;
  MXBITFILE *mbx_file;
  int is_ascii;
  static char mbx_header_sentinel[4] = {
    0x55, 0x55, 0x55, 0x55
  };
  char mbx_header[10];
  int i;
  int n_mbxs;
  int mxmseg_result;
  struct stat statbuf;
  int rename_result = -1;

  if (signal(SIGINT, SIG_IGN) != SIG_IGN)
  {
    signal(SIGINT, onintr);
  }

  n_control_files = 0;
  control_files[0] = NULL;
  memset(&mxlargs, 0, sizeof ( mxlargs ));
  mxlargs.map_file = stdout;
  mxlargs.map_filename = NULL;
  unpack_sw = table_sw = extract_sw = repack_sw = 0;
  while (( option_letter = getopt(argc, argv, "c:ntrux")) != EOF)
  {
    switch (option_letter)
    {
    case 'c': /* Control file */
      control_files[n_control_files++]
        = strcpy(malloc(strlen(optarg) + 1), optarg);
      break;

    case 't': /* Map only */
      table_sw = 1;
      mxlargs.map_only = 1;
      break;

    case 'n': /* No map */
      mxlargs.no_map = 1;
      break;

    case 'r': /* Repack into stream mailbox */
      repack_sw = 1;
      break;

    case 'u': /* Unpack */
      unpack_sw = 1;
      break;

    case 'x': /* Extract */
      extract_sw = 1;
      break;

    case '?': /* ? or unrecognized option */
      usage();
      mxlexit(4);
    }
  }

  if (unpack_sw + table_sw + extract_sw + repack_sw != 1)
  {
    usage();
  }

  if (mxlargs.no_map)
  {
    mxlargs.map_file = fopen("/dev/null", "w");
  }

  /*
   * Adjust arguments to skip over
   * program name and options,
   * putting position at mailbox file name
   */

  argc -= optind;
  argv += optind;

  if (argc < 1)
  {
    usage();
  }

  n_mbxs = argc;

  if (table_sw)
  {
    retrieval_options_ptr = NULL;
  }
  else
  {

        /*
         * Get retrieval options.  Returns default
         * options only, i.e. builtin defaults plus
         * result of any global statements in 
         * control files.
         */

    retrieval_options_ptr
      = parsctl(n_control_files, control_files, 0, NULL, 1);

        /* 
         * Adjust the retrieval
         * options a bit
         */

    if (extract_sw || repack_sw)
    {
      retrieval_options_ptr->reload = "flat";
    }

    if (repack_sw)
    {
      retrieval_options_ptr->path_type = "file";
    }
  }

  /*
   * Construct the nullest
   * of preambles
   */

  memset(&preamble, 0, sizeof ( preamble ));
  preamble.record_type = SEGMENT_RECORD;
  memset(&branch_preamble, 0, sizeof ( branch_preamble ));

  for (i = 0; i < n_mbxs; ++i)
  {
    if (( mbx_file = open_mxbit_file(argv[i], "rt")) == NULL)
    {
      fprintf(stderr, "Cannot open Multics mailbox file %s.\n", argv[i]);
      continue;
    }

    if (stat(argv[i], &statbuf) == 0)
    {
      branch_preamble.dtu = statbuf.st_atime;
      branch_preamble.dtm = branch_preamble.dtbm = statbuf.st_mtime;
    }

        /*
         * Read what should be header of mailbox.
         * For MS-DOS, the translation of NL to CR-LF
         * means that we cannot reliably read an 
         * 8bit-ified mailbox.
         */

    get_mxbits(mbx_file, 72L, mbx_header);
    if (memcmp(mbx_header + 5, mbx_header_sentinel, 4) != 0)
    {
      fprintf(stderr,
        "mxmbx:        %s does not appear to be a Multics mailbox.\n",
        argv[i]);
      fprintf(stderr,
        "Be sure that the mailbox was reloaded ");
      fprintf(stderr,
        "with 9bit conversion.\n");
      continue;
    }

    if (i > 0)
    {
      fprintf(mxlargs.map_file, "\n");
    }

    if (n_mbxs > 1)
    {
      fprintf(mxlargs.map_file, "%s\n", argv[i]);
    }

    if (unpack_sw || repack_sw)
    {

          /* 
           * Rename the mailbox file to make way
           * for a new directory or file before
           * unpacking or repacking. 
           */

#ifdef sun
#define BSD
#endif /* ifdef sun */
#ifdef BSD
      cvpath(argv[i], "", "BSD", renamed_mbx);
#else  /* ifdef BSD */
      cvpath(argv[i], "", "SysV", renamed_mbx);
#endif /* ifdef BSD */
#ifdef SVR2
      if (( rename_result = link(argv[i], renamed_mbx)) == 0)
      {
        rename_result = unlink(argv[i]);
      }

#else  /* ifdef SVR2 */
      rename_result = rename(argv[i], renamed_mbx);
#endif /* ifdef SVR2 */
      if (rename_result != 0)
      {
        fprintf(stderr, "Cannot rename %s to %s.\n", argv[i], renamed_mbx);
        continue;
      }

          /*
           * Put results in directory or
           * file with old mailbox name
           */

      strcpy(retrieval_options_ptr->new_path, argv[i]);
    }

    mxmseg_result = mxmseg(
      mbx_file,
      &preamble,
      retrieval_options_ptr,
      &branch_preamble,
      repack_sw,
      "mbx");

        /* 
         * When unpacking or repacking,
         * delete the original mailbox
         */

    if (( unpack_sw || repack_sw ) && rename_result == 0)
    {
      if (mxmseg_result == 0)
      {
        unlink(renamed_mbx);
      }
      else
      {
        fprintf(stderr, "Original mailbox is in %s\n", renamed_mbx);
      }

      renamed_mbx[0] = '\0';
    }
  }

  mxlexit(0);
  return( 0 );
}

 /*
  * Print usage
  * message
  */

#ifdef ANSI_FUNC
static void 
usage (void)
#else
static void
usage()
#endif
{
  fprintf(stderr, "Usage: mxmbx -t [options] mailbox_file ...\n");
  fprintf(stderr, "Or:    mxmbx -u [options] mailbox_file ...\n");
  fprintf(stderr, "Or:    mxmbx -r [options] mailbox_file ...\n");
  fprintf(stderr, "Or:    mxmbx -x [options] mailbox_file ...\n");
  fprintf(stderr, "\nOptions are:\n");
  fprintf(
    stderr,
    "-c control_file\t= control file; may be specified more than once\n");
  fprintf(stderr, "-n\t\t= no messages list\n");
  fprintf(stderr, "-t\t\t= table; just produce a message list\n");
  fprintf(stderr, "-u\t\t= unpack; replace mailbox by ");
  fprintf(stderr, "directory containing messages\n");
  fprintf(stderr, "-r\t\t= repack; replace mailbox by ");
  fprintf(stderr, "file containing messages\n");
  fprintf(
    stderr,
    "-x\t\t= extract; put copies of messages in current directory\n");
  mxlexit(4);
}

#ifdef ANSI_FUNC
static SIGT
onintr (int sig)
#else
static SIGT
onintr(sig)

int sig;
#endif
{
  mxlexit(4);
}

 /* 
  * Exit after cleaning up.
  * Called by various subrs.
  */

#ifdef ANSI_FUNC
void 
mxlexit_ (int status)
#else
void
mxlexit_(status)

int status;
#endif

{
  if (renamed_mbx[0] != '\0')
  {
    fprintf(stderr, "Original mailbox is in %s\n", renamed_mbx);
  }

  cleanup_temp_files();
  exit(status);
}

#ifdef ANSI_FUNC
void 
process_seg_mxmbx (MXBITFILE *infile, 
                struct BRANCH_PREAMBLE *branch_preamble_ptr, 
                struct PREAMBLE *preamble_ptr, 
                struct MXLOPTS *retrieval_options_ptr, int is_ascii)
#else
void
process_seg_mxmbx(infile, branch_preamble_ptr, preamble_ptr,
                 retrieval_options_ptr, is_ascii)

MXBITFILE *infile;
struct BRANCH_PREAMBLE *branch_preamble_ptr;
struct PREAMBLE *preamble_ptr;
struct MXLOPTS *retrieval_options_ptr;
int is_ascii;
#endif

{
  int seg_has_been_read;
  char *seg_type;
  char *seg_type_by_name;
  char *seg_type_by_content;
  char *conversion_type;
  MXBITFILE *contents_file;

  if (mxlargs.map_only)
  {
    display_branch_preamble(preamble_ptr, branch_preamble_ptr);
    return;
  }

  contents_file = get_temp_file("wt", "raw file contents");
  if (contents_file == NULL)
  {
    mxlexit(4);  /* Make sure original mailbox doesn't get deleted */
  }

  if (rdseg(infile, contents_file, preamble_ptr) != 0)
  {
    goto EXIT;
  }

  display_branch_preamble(preamble_ptr, branch_preamble_ptr);
  convert_seg(
    contents_file,
    NULL,
    preamble_ptr,
    branch_preamble_ptr,
    "ascii",
    "8bit",
    retrieval_options_ptr);
EXIT:
  release_temp_file(contents_file, "raw file contents");
}

 /* 
  * Read segment from temp file and write
  * converted form into new path
  */

#ifdef ANSI_FUNC
static void 
convert_seg (MXBITFILE *contents_file,
                MXBITFILE *preconverted_contents_file,
                struct PREAMBLE *preamble_ptr,
                struct BRANCH_PREAMBLE *branch_preamble_ptr,
                char *seg_type, char *conversion_type,
                struct MXLOPTS *retrieval_options_ptr)
#else
static void
convert_seg(contents_file, preconverted_contents_file,
                        preamble_ptr, branch_preamble_ptr, seg_type,
                        conversion_type, retrieval_options_ptr)

MXBITFILE *contents_file;
struct PREAMBLE *preamble_ptr;
struct BRANCH_PREAMBLE *branch_preamble_ptr;
char *seg_type;
char *conversion_type;
struct MXLOPTS *retrieval_options_ptr;
MXBITFILE *preconverted_contents_file;
#endif

{
  char new_path[400];
  char *name_type;
  static char last_dir[170] = "";

  name_type =
    retrieval_options_ptr->attr_cv_values[get_keyword_values_index(
                                            attr_cv_types,
                                            "name_type")];
  ++retrieval_options_ptr->n_files_loaded;

  if (make_new_path(
        preamble_ptr->dname,
        preamble_ptr->ename,
        retrieval_options_ptr,
        "",
        name_type,
        new_path)
      != 0)
  {
    mxlexit(4);  /* Make sure original mailbox doesn't
                  *      get deleted */
  }

  display_conversion_info(new_path, seg_type, conversion_type);
  copy_8bit(contents_file, new_path, preamble_ptr->adjusted_bitcnt);
  set_attrs(
    new_path,
    retrieval_options_ptr,
    preamble_ptr,
    branch_preamble_ptr);
  return;
}

#ifdef ANSI_FUNC
static void 
set_attrs (char *new_path, struct MXLOPTS *retrieval_options_ptr,
                struct PREAMBLE *preamble_ptr,
                struct BRANCH_PREAMBLE *branch_preamble_ptr)
#else
static void
set_attrs(new_path, retrieval_options_ptr, preamble_ptr,
                      branch_preamble_ptr)

char *new_path;
struct MXLOPTS *retrieval_options_ptr;
struct PREAMBLE *preamble_ptr;
struct BRANCH_PREAMBLE *branch_preamble_ptr;
#endif

{
  char *access_time_action;
  char *mod_time_action;
  char *acl_action;
  char *addname_action;
  struct utimbuf utime_struct;
  char *name_type;
  char *group_action;
  char project_id[20];
  char *owner_action;
  char person_id[20];

  name_type =
    retrieval_options_ptr->attr_cv_values[get_keyword_values_index(
                                            attr_cv_types,
                                            "name_type")];

       /* 
        * Set the access time 
        * if requested
        */

  access_time_action
    = retrieval_options_ptr->attr_cv_values[get_keyword_values_index(
                                              attr_cv_types,
                                              "access_time")];
  if (strcmp(access_time_action, "dtu") == 0)
  {
    utime_struct.actime = branch_preamble_ptr->dtu;
  }
  else
  {
    utime_struct.actime = time(NULL);
  }

       /* 
        * Set the modification time
        * if requested
        */

  mod_time_action
    = retrieval_options_ptr->attr_cv_values[get_keyword_values_index(
                                              attr_cv_types,
                                              "modification_time")];
  if (strcmp(mod_time_action, "dtcm") == 0)
  {
    utime_struct.modtime = branch_preamble_ptr->dtm;
  }
  else
  {
    utime_struct.modtime = time(NULL);
  }

  utime(new_path, &utime_struct);  /* The actual time-setting */
}
