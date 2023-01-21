 /********************************\
 *    Copyright (C) 1987 by       *
 * Cygnus Cybernetics Corporation *
 *   and Oxford Systems, Inc.     *
 \********************************/

 /* MXFORUM */

 /* 
  * Read Multics forums and retrieve
  * transactions. This is basically a
  * lobotomized version of mxmbx.
  */

#ifdef MXFORUM
#define main_mxforum main
#define mxlargs_mxforum mxlargs
#define mxlexit_mxforum mxlexit
#endif

#include "mxload.h"
#include <ctype.h>
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
#include "optptr.h"
#include "parsctl.h"
#include "preamble.h"
#include "rdbkrcd.h"
#include "tempfile.h"
#include "timestr.h"
#include "utime.h"

#ifndef min
#define min(A, B) \
  (( A ) < ( B ) ? \
   ( A ) : ( B ))
#endif /* ifndef min */

static void usage();
static SIGT onintr();
static void put_in_place();
static void set_attrs();
static int mxmtg();
static long get_txn();

struct TXN
{
  int idx;
  char deleted_flags;
  long pref_pos; /* In bits */
  long nref_pos; /* In bits */
  long segno;
  long subj_pos; /* In bytes */
  long subj_len; /* In bytes */
  long text_pos; /* In bytes */
  long text_len; /* In bytes */
};

struct MXLARGS mxlargs_mxforum;
static MXBITFILE *proceedings_files[100];
static int proceedings_file_is_8bit[100];

#ifdef ANSI_FUNC
int 
main_mxforum (int argc, char *argv[])
#else
int
main_mxforum(argc, argv)

int argc;
char *argv[];
#endif

{
  char temp_path[200];
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
  MXBITFILE *txn_file;
  int is_ascii;
  static char forum_header_sentinel[4] = {
    0x55, 0x55, 0x55, 0x55
  };
  char forum_header[10];
  int i;
  int j;
  int n_forums;
  int mxmtg_result;
  struct stat statbuf;
  char txn_file_name[200];
  char temp_string[100];

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

    case 'r': /* Repack into stream file */
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
   * program name and options, putting
   * position at forum dir name
   */

  argc -= optind;
  argv += optind;

  if (argc < 1)
  {
    usage();
  }

  n_forums = argc;

  if (table_sw)
  {
    retrieval_options_ptr = NULL;
  }
  else
  {

        /* 
         * Get retrieval options.  Returns default
         * options only, i.e. builtin defaults
         * plus result of any global statements
         * in control files.
         */

    retrieval_options_ptr
      = parsctl(n_control_files, control_files, 0, NULL, 1);

        /* 
         * Adjust the retrieval
         * options a bit.
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

  for (i = 0; i < n_forums; ++i)
  {
    strcpy(txn_file_name, argv[i]);
    strcat(txn_file_name, DIR_SEPARATOR_STRING);
    strcat(txn_file_name, "Transactions");

    if (( txn_file = open_mxbit_file(txn_file_name, "rt")) == NULL)
    {
      fprintf(stderr, "Cannot open %s.\n", txn_file_name);
      continue;
    }

    get_mxstring(txn_file, temp_string, 6);
    if (strcmp(temp_string, "FMTR_1") != 0)
    {
      fprintf(
        stderr,
        "%s does not appear to be a valid transactions file.\n",
        txn_file_name);
      continue;
    }

    rewind_mxbit_file(txn_file, "r");

    if (stat(txn_file_name, &statbuf) == 0)
    {
      branch_preamble.dtu = statbuf.st_atime;
      branch_preamble.dtm = branch_preamble.dtbm = statbuf.st_mtime;
    }

    if (i > 0)
    {
      fprintf(mxlargs.map_file, "\n");
    }

    if (n_forums > 1)
    {
      fprintf(mxlargs.map_file, "%s\n", argv[i]);
    }

    if (unpack_sw)
    {

          /*
           * Put results in old
           * forum directory
           */

      strcpy(retrieval_options_ptr->new_path, argv[i]);
    }

    if (repack_sw)
    {

          /*
           * Put results in old
           * forum directory
           */

      strcpy(retrieval_options_ptr->new_path, argv[i]);
      strcat(retrieval_options_ptr->new_path, DIR_SEPARATOR_STRING);
      strcat(retrieval_options_ptr->new_path, "Proceedings");
    }

    mxmtg_result = mxmtg(
      txn_file,
      argv[i],
      &preamble,
      retrieval_options_ptr,
      &branch_preamble,
      repack_sw);

        /* 
         * When unpacking or repacking, delete
         * the original forum directory contents
         */

    if (( unpack_sw || repack_sw ) && mxmtg_result == 0)
    {
      strcpy(temp_path, argv[i]);
      strcat(temp_path, DIR_SEPARATOR_STRING);
      strcat(temp_path, "Attendees");
      unlink(temp_path);

      strcpy(temp_path, argv[i]);
      strcat(temp_path, DIR_SEPARATOR_STRING);
      strcat(temp_path, "Transactions");
      unlink(temp_path);

      j = 1;
      do
      {
        strcpy(temp_path, argv[i]);
        strcat(temp_path, DIR_SEPARATOR_STRING);
        sprintf(temp_path + strlen(temp_path), "Proceedings.%d", j);
        ++j;
      }
      while ( unlink(temp_path) == 0 );
    }
  }

  mxlexit(0);
  return ( 0 );
}

 /* 
  * Print usage
  * transaction
  */

#ifdef ANSI_FUNC
static void 
usage (void)
#else
static void
usage()
#endif
{
  fprintf(stderr,
        "Usage: mxforum -t [options] forum_dir ...\n");
  fprintf(stderr,
        "Or:    mxforum -u [options] forum_dir ...\n");
  fprintf(stderr,
        "Or:    mxforum -r [options] forum_dir ...\n");
  fprintf(stderr,
        "Or:    mxforum -x [options] forum_dir ...\n");
  fprintf(stderr,
        "\nOptions are:\n");
  fprintf(stderr,
        "-c control_file\t= control file; may be specified more than once\n");
  fprintf(stderr,
        "-n\t\t= no transactions list\n");
  fprintf(stderr,
        "-t\t\t= table; just produce a transaction list\n");
  fprintf(stderr,
        "-u\t\t= unpack; replace forum contents by ");
  fprintf(stderr,
        "files containing transactions\n");
  fprintf(stderr,
        "-r\t\t= repack; replace forum contents by ");
  fprintf(stderr,
        "file containing transactions\n");
  fprintf(stderr,
    "-x\t\t= extract; put copies of transactions in current directory\n");
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
mxlexit_mxforum (int status)
#else
void
mxlexit_mxforum(status)

int status;
#endif

{
  cleanup_temp_files();
  exit(status);
}

#ifdef ANSI_FUNC
static int 
mxmtg (MXBITFILE *txn_file, char *forum_dir,
                struct PREAMBLE *forum_preamble_ptr,
                struct MXLOPTS *retrieval_opt_ptr,
                struct BRANCH_PREAMBLE *forum_branch_preamble_ptr, int repack)
#else
static int
mxmtg(txn_file, forum_dir, forum_preamble_ptr, retrieval_opt_ptr,
                 forum_branch_preamble_ptr, repack)

MXBITFILE *txn_file;
char *forum_dir;
struct PREAMBLE *forum_preamble_ptr;
struct MXLOPTS *retrieval_opt_ptr;
struct BRANCH_PREAMBLE *forum_branch_preamble_ptr;
int repack;
#endif

{
  char temp_string[100];
  long txn_count;
  long txn_pos;
  long next_txn_pos;
  long repacked_bitcnt;
  int i;
  int first_txn;
  char *name_type;
  char new_path[200];
  struct TXN txn;
  MXBITFILE *contents_file = 0;
  MXBITFILE *repacked_file = 0;

  contents_file = get_temp_file("wt", "txn file contents");
  if (repack)
  {
    repacked_file = get_temp_file("wt", "repacked forum contents");
    repacked_bitcnt = 0L;
  }

  for (i = 0; i < 100; ++i)
  {
    proceedings_files[i] = NULL;
  }

  skip_mxbits(txn_file, 8 * 9L + 18L);  /* Version and top of
                                         * txn_count*/
  txn_count = get_18_mxbit_integer(txn_file);

  skip_mxbits(txn_file, 36L);           /* transaction_seg.deleted_count */
  txn_pos = get_18_mxbit_integer(txn_file) * 36L;  /* first_trans_offset */
  first_txn = 1;
  while (txn_pos > 0L)
  {
    rewind_mxbit_file(contents_file, "w");
    next_txn_pos = get_txn(
      txn_file,
      txn_pos,
      &txn,
      forum_preamble_ptr,
      forum_branch_preamble_ptr,
      forum_dir,
      contents_file->realfile);
    if (next_txn_pos < 0L)
    {
      break;
    }

    rewind_mxbit_file(contents_file, "rt");

    if (mxlargs.map_only)
    {
      display_branch_preamble(
        forum_preamble_ptr,
        forum_branch_preamble_ptr);
    }
    else if (repack)
    {
      if (!first_txn)
      {
        put_mxbits(repacked_file, 16L, "\f\n");
      }

      first_txn = 0;
      copy_bits(contents_file, repacked_file, forum_preamble_ptr->bitcnt);
      repacked_bitcnt += forum_preamble_ptr->bitcnt;
    }
    else
    {
      display_branch_preamble(
        forum_preamble_ptr,
        forum_branch_preamble_ptr);
      name_type =
        retrieval_opt_ptr->attr_cv_values[get_keyword_values_index(
                                            attr_cv_types,
                                            "name_type")];
      ++retrieval_opt_ptr->n_files_loaded;

      if (make_new_path(
            forum_preamble_ptr->dname,
            forum_preamble_ptr->ename,
            retrieval_opt_ptr,
            "",
            name_type,
            new_path)
          != 0)
      {
        mxlexit(4);  /* Make sure original forum
                      * doesn't get deleted */
      }

      display_conversion_info(new_path, "ascii", "8bit");
      put_in_place(
        contents_file,
        new_path,
        1,
        forum_preamble_ptr->adjusted_bitcnt / 8L);
      set_attrs(
        new_path,
        retrieval_opt_ptr,
        forum_preamble_ptr,
        forum_branch_preamble_ptr);
    }

    txn_pos = next_txn_pos;
  }
  if (repack)
  {
    strcpy(forum_preamble_ptr->ename, forum_dir);
    forum_preamble_ptr->adjusted_bitcnt = repacked_bitcnt;
    forum_preamble_ptr->length
      = ( repacked_bitcnt + 36L * 1024L - 1 ) / ( 36L * 1024L );
    display_branch_preamble(forum_preamble_ptr, forum_branch_preamble_ptr);
    name_type = retrieval_opt_ptr->attr_cv_values[get_keyword_values_index(
                                                    attr_cv_types,
                                                    "name_type")];
    ++retrieval_opt_ptr->n_files_loaded;
    if (make_new_path(
          forum_preamble_ptr->dname,
          forum_preamble_ptr->ename,
          retrieval_opt_ptr,
          "",
          name_type,
          new_path)
        != 0)
    {
      mxlexit(4);  /* Make sure original forum
                    * doesn't get deleted */
    }

    display_conversion_info(new_path, "ascii", "8bit");
    put_in_place(repacked_file, new_path, 1, repacked_bitcnt / 8L);
    set_attrs(
      new_path,
      retrieval_opt_ptr,
      forum_preamble_ptr,
      forum_branch_preamble_ptr);
  }

  release_temp_file(contents_file, "txn file contents");
  if (repack)
  {
    release_temp_file(repacked_file, "repacked forum contents");
  }

  return ( 0 );
}

 /*
  * Read a Multics transaction structure
  * (see forum_structures.incl.pl1)
  * from the input file and put in temp file
  */

#ifdef ANSI_FUNC
static long 
get_txn (MXBITFILE *tx_file, long txn_pos, struct TXN *txn_ptr,
                struct PREAMBLE *preamble_ptr,
                struct BRANCH_PREAMBLE *branch_preamble_ptr, char *forum_dir,
                FILE *f)
#else
static long
get_txn(tx_file, txn_pos, txn_ptr, preamble_ptr,
                    branch_preamble_ptr, forum_dir, f)

MXBITFILE *tx_file;
long txn_pos;
struct TXN *txn_ptr;
struct PREAMBLE *preamble_ptr;
struct BRANCH_PREAMBLE *branch_preamble_ptr;
char *forum_dir;
FILE *f;
#endif

{
  char proceedings_name[200];
  struct TXN nref;
  char test_string[5];
  int i;
  int npad;
  long count;
  unsigned long long_pair[2];
  char temp_string[100];
  struct PREAMBLE dummy_preamble;
  struct BRANCH_PREAMBLE dummy_branch_preamble;
  long next_pos;
  long outpos;
  long nchars;

  next_pos = txn_pos;
  do
  {
    mxbit_pos(tx_file, next_pos);
    skip_mxbits(tx_file, 36L + 18L);  /* skip transaction.version and
                                       * top of trans_idx, fixed bin (17) */
    txn_ptr->idx = get_18_mxbit_integer(tx_file);
    sprintf(preamble_ptr->ename, "%.4d", txn_ptr->idx);
    strcat(preamble_ptr->ename, ".TXN");

    get_mxstring(tx_file, temp_string, 22);
    strcpy(branch_preamble_ptr->author, temp_string);
    get_mxstring(tx_file, temp_string, 9);
    strcat(branch_preamble_ptr->author, ".");
    strcat(branch_preamble_ptr->author, temp_string);
    strcpy(
      branch_preamble_ptr->bitcount_author,
      branch_preamble_ptr->author);
    get_mxbits(tx_file, 2L, &( txn_ptr->deleted_flags ));
    skip_mxbits(tx_file, 34L);  /* Rest of transaction.flags */
    txn_ptr->pref_pos = get_18_mxbit_integer(tx_file) * 36L;
    txn_ptr->nref_pos = get_18_mxbit_integer(tx_file) * 36L;
    skip_mxbits(tx_file, 9L);  /* Undeclared padding */
        
        /*
         * Message_descriptor.ms_id is a 72-bit
         * clock value.  We pick off the 36 bits
         * for fstime and convert
         */

    skip_mxbits(tx_file, 20L);  /* Top of transaction.time */
    get_36_mxbit_integer(tx_file, long_pair);
    branch_preamble_ptr->dtm = branch_preamble_ptr->dtu
                                 = branch_preamble_ptr->dtbm = cvmxtime(
                                     long_pair);
    skip_mxbits(tx_file, 16L + 18L);  /* Bottom of transaction.time and
                                       * top half of segno, fixed bin (17) */
    txn_ptr->segno = get_18_mxbit_integer(tx_file);
    txn_ptr->subj_pos = get_18_mxbit_integer(tx_file) * 4L;
    skip_mxbits(tx_file, 18L + 18L);  /* Undeclared padding and
                                       * top half of subject_length,
                                       * treated as a fixed bin (17) */
    txn_ptr->subj_len = get_18_mxbit_integer(tx_file);

    txn_ptr->text_pos = get_18_mxbit_integer(tx_file) * 4L;
    skip_mxbits(tx_file, 18L + 12L);  /* Undeclared padding and
                                       * top half of text_length,
                                       * treated as a fixed bin (24) */
    txn_ptr->text_len = get_24_mxbit_integer(tx_file);

    next_pos = get_18_mxbit_integer(tx_file) * 36L;
  }
  while ( txn_ptr->deleted_flags != 0 && next_pos > 0L );

  if (txn_ptr->deleted_flags != 0)
  {
    return ( -1L );
  }

  if (proceedings_files[txn_ptr->segno] == NULL)
  {
    sprintf(
      proceedings_name,
      "%s%sProceedings.%ld",
      forum_dir,
      DIR_SEPARATOR_STRING,
      txn_ptr->segno);
    proceedings_files[txn_ptr->segno]
      = open_mxbit_file(proceedings_name, "r");
    if (proceedings_files[txn_ptr->segno] == NULL)
    {
      perror(proceedings_name);
      return ( -1 );
    }

        /*
         * If first 4 characters of proceedings
         * are printable characters, assume it's
         * already undergone 9-to-8-bit conversion
         */

    get_mxbits(proceedings_files[txn_ptr->segno], 32L, test_string);
    proceedings_file_is_8bit[txn_ptr->segno] = 1;
    for (i = 0; i < 4; ++i)
    {
      proceedings_file_is_8bit[txn_ptr->segno]
        &= ( isprint(test_string[i]) != 0 );
    }

  }

  fprintf(
    f,
    "[%04d] %s %s\n",
    txn_ptr->idx,
    branch_preamble_ptr->author,
    timestr(&( branch_preamble_ptr->dtm )));
  fprintf(f, "Subject: ");
  nchars = txn_ptr->subj_len;
  if (proceedings_file_is_8bit[txn_ptr->segno])
  {
    fseek(
      proceedings_files[txn_ptr->segno]->realfile,
      txn_ptr->subj_pos,
      0);
    while (nchars-- > 0)
    {
      putc(getc(proceedings_files[txn_ptr->segno]->realfile), f);
    }
  }
  else
  {
    mxbit_pos(proceedings_files[txn_ptr->segno], txn_ptr->subj_pos * 9L);
    while (nchars > 0)
    {
      get_mxstring(
        proceedings_files[txn_ptr->segno],
        temp_string,
        min(72L, nchars));
      npad = min(72, (int)nchars) - strlen(temp_string);
      for (i = 0; temp_string[i] != '\0'; ++i)
      {
        putc(temp_string[i], f);
      }

      for (i = 0; i < npad; ++i)
      {
        putc(' ', f);
      }

      nchars -= 72L;
    }
  }

  fprintf(f, "\n");

  nchars = txn_ptr->text_len;
  if (proceedings_file_is_8bit[txn_ptr->segno])
  {
    fseek(
      proceedings_files[txn_ptr->segno]->realfile,
      txn_ptr->text_pos,
      0);
    while (nchars-- > 0)
    {
      putc(getc(proceedings_files[txn_ptr->segno]->realfile), f);
    }
  }
  else
  {
    mxbit_pos(proceedings_files[txn_ptr->segno], txn_ptr->text_pos * 9L);
    while (nchars > 0)
    {
      get_mxstring(
        proceedings_files[txn_ptr->segno],
        temp_string,
        min(72L, nchars));
      npad = min(72, (int)nchars) - strlen(temp_string);
      for (i = 0; temp_string[i] != '\0'; ++i)
      {
        putc(temp_string[i], f);
      }

      for (i = 0; i < npad; ++i)
      {
        putc(' ', f);
      }

      nchars -= 72L;
    }
  }

  fprintf(f, "---[%04d]---", txn_ptr->idx);
  if (txn_ptr->pref_pos != 0L || txn_ptr->nref_pos != 0L)
  {
    fprintf(f, " (");
    if (txn_ptr->pref_pos != 0L)
    {
      mxbit_pos(tx_file, txn_ptr->pref_pos + 36L + 18L);
      fprintf(f, "pref = [%04ld]", get_18_mxbit_integer(tx_file));

      if (txn_ptr->nref_pos != 0L)
      {
        fprintf(f, ", ");
      }
    }

    if (txn_ptr->nref_pos != 0L)
    {
      mxbit_pos(tx_file, txn_ptr->nref_pos + 36L + 18L);
      fprintf(f, "nref = [%04ld]", get_18_mxbit_integer(tx_file));
    }

    fprintf(f, ")");
  }

  fprintf(f, "\n");
  outpos = ftell(f);
  preamble_ptr->bitcnt = preamble_ptr->adjusted_bitcnt = outpos * 8L;
  preamble_ptr->length
    = ( preamble_ptr->bitcnt + 36L * 1024L - 1 ) / ( 36L * 1024L );

  return ( next_pos );
}

 /* 
  * Renames temporary file into permanent
  * position if possible, otherwise it 
  * copies it into position.
  */

#ifdef ANSI_FUNC
static void 
put_in_place (MXBITFILE *contents_file, char *new_path, int is_ascii,
                long charcount)
#else
static void
put_in_place(contents_file, new_path, is_ascii, charcount)

MXBITFILE *contents_file;
char *new_path;
int is_ascii;
long charcount;
#endif

{
  int rename_result = -1;

#ifdef SVR2
  if (( rename_result = link(temp_file_name(contents_file), new_path)) == 0)
  {
    rename_result = unlink(temp_file_name(contents_file));
  }

#else  /* ifdef SVR2 */
  rename_result = rename(temp_file_name(contents_file), new_path);
#endif /* ifdef SVR2 */

  if (rename_result == 0)
  {
    replace_temp_file(contents_file);
  }
  else
  {
    copy_file(contents_file, new_path, is_ascii, charcount);
  }
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
