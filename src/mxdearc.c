 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

 /* MXDEARC */

 /*
  * Process Multics
  * archive file
  */

#include "mxload.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "copybits.h"
#include "dirsep.h"
#include "mxbitio.h"
#include "mxdearc.h"
#include "mxlopts.h"
#include "optptr.h"
#include "preamble.h"
#include "tempfile.h"
#include "timestr.h"

static int arcintr();
static int get_component();
static long cv_arctime();

void process_ascii_seg (struct BRANCH_PREAMBLE *branch_preamble_ptr,
                MXBITFILE *contents_file, struct PREAMBLE *preamble_ptr,
                struct MXLOPTS *retrieval_options_ptr);

static char comp_contents_filename[400];

#ifdef ANSI_FUNC
int
mxdearc (MXBITFILE *arc_contents_file, struct PREAMBLE *arc_preamble_ptr,
                struct MXLOPTS *retrieval_opt_ptr,
                struct BRANCH_PREAMBLE *arc_branch_preamble_ptr, int is_ascii)
#else
int
mxdearc(arc_contents_file, arc_preamble_ptr, retrieval_opt_ptr,
            arc_branch_preamble_ptr, is_ascii)

MXBITFILE *arc_contents_file;
struct PREAMBLE *arc_preamble_ptr;
struct MXLOPTS *retrieval_opt_ptr;
struct BRANCH_PREAMBLE *arc_branch_preamble_ptr;
int is_ascii;
#endif

{
  MXBITFILE *comp_contents_file;
  struct PREAMBLE comp_preamble;
  struct BRANCH_PREAMBLE comp_branch_preamble;
  struct MXLOPTS comp_retrieval_opt;
  int gc_code;

  rewind_mxbit_file(arc_contents_file, "rt");

  comp_contents_file = get_temp_file("wt", "archive component contents");
  if (comp_contents_file == NULL)
  {
    return ( -1 );
  }

  /*
   * Construct preamble structures to be
   * used for each component in turn
   */

  memcpy(&comp_preamble, arc_preamble_ptr, sizeof ( comp_preamble ));
  if (comp_preamble.dname[0] != '\0')
  {
    strcat(comp_preamble.dname, ">");
  }

  strcat(comp_preamble.dname, arc_preamble_ptr->ename);

  memcpy(
    &comp_branch_preamble,
    arc_branch_preamble_ptr,
    sizeof ( comp_branch_preamble ));
  comp_branch_preamble.naddnames = 0;

  /*
   * Make a retrieval options structure that will make the archive
   * look like a directory within a subtree.  But if mxarc is
   * just doing a table listing, avoid null pointer fault.
   */

  if (retrieval_opt_ptr != NULL)
  {
    memcpy(
      &comp_retrieval_opt,
      retrieval_opt_ptr,
      sizeof ( comp_retrieval_opt ));
  }

  comp_retrieval_opt.path_type = "subtree";

  while (( gc_code
             = get_component(
                 arc_contents_file,
                 comp_contents_file,
                 &comp_preamble,
                 &comp_branch_preamble,
                 is_ascii))
         == 0)
  {
    rewind_mxbit_file(comp_contents_file, "rt");
    if (is_ascii)
    {
      process_ascii_seg(
        &comp_branch_preamble,
        comp_contents_file,
        &comp_preamble,
        &comp_retrieval_opt);
    }
    else
    {
      process_seg(
        comp_contents_file,
        &comp_branch_preamble,
        &comp_preamble,
        &comp_retrieval_opt,
        0);
    }

    rewind_mxbit_file(comp_contents_file, "w");
  }
  if (gc_code != EOF)
  {
    if (arc_preamble_ptr->ename[0] != '\0')
    {
      fprintf(
        stderr,
        "Archive format error encountered unpacking %s>%s.\n",
        arc_preamble_ptr->dname,
        arc_preamble_ptr->ename);
    }
    else
    {
      fprintf(stderr, "Archive format error encountered\n");
    }
  }

EXIT:
  release_temp_file(comp_contents_file, "archive component contents");
  if (gc_code != EOF)
  {
    return ( -1 );
  }
  else
  {
    return ( 0 );
  }
}

#ifdef ANSI_FUNC
static int
get_component (MXBITFILE *arc_contents_file, MXBITFILE *comp_contents_file,
                struct PREAMBLE *comp_preamble_ptr,
                struct BRANCH_PREAMBLE *comp_branch_preamble_ptr, int is_ascii)
#else
static int
get_component(arc_contents_file, comp_contents_file,
                         comp_preamble_ptr, comp_branch_preamble_ptr,
                         is_ascii)

MXBITFILE *arc_contents_file;
MXBITFILE *comp_contents_file;
struct PREAMBLE *comp_preamble_ptr;
struct BRANCH_PREAMBLE *comp_branch_preamble_ptr;
int is_ascii;
#endif

{
  static char archive_header_begin[8]
    = {
    014, 012, 012, 012, 017, 012, 011, 011
    };
  static char archive_header_end[8]
    = {
    017, 017, 017, 017, 012, 012, 012, 012
    };

  char comp_header[101];
  char bit_count_str[10];
  int i;
  int n_read;

  /*
   * Each component header is 100 bytes long
   *  and has the following fields:
   *
   *     Field     Length  Offset
   *
   *     header_begin   8       0
   *     pad1           4       8
   *     name          32      12
   *     timeup        16      44
   *     mode           4      60
   *     time          16      64
   *     pad            4      80
   *     bit_count      8      84
   *     header_end     8      92
   */

  rewind_mxbit_file(comp_contents_file, "w");

  if (eof_reached(arc_contents_file))
  {
    return ( -1 );
  }

  if (is_ascii)
  {
    n_read = get_mxbits(arc_contents_file, 800L, comp_header);
    if (n_read != 800)
    {
      return ( -2 );
    }
  }
  else
  {
    if (get_mxstring(arc_contents_file, comp_header, 100) != 100)
    {
      return ( -2 );
    }
  }

  if (memcmp(comp_header, archive_header_begin, 8) != 0)
  {
    return ( -2 );
  }

  if (memcmp(comp_header + 92, archive_header_end, 8) != 0)
  {
    return ( -2 );
  }

  for (i = 0; i < 32 && comp_header[12 + i] != ' '; ++i)
  {
    comp_preamble_ptr->ename[i] = comp_header[12 + i];
  }

  comp_preamble_ptr->ename[i] = '\0';

  comp_branch_preamble_ptr->dtu = cv_arctime(comp_header + 44);
  comp_branch_preamble_ptr->dtbm = comp_branch_preamble_ptr->dtm
                                     = cv_arctime(comp_header + 64);

  memcpy(bit_count_str, comp_header + 84, 8);
  bit_count_str[8] = '\0';
  comp_preamble_ptr->bitcnt = atol(bit_count_str);
  comp_preamble_ptr->adjusted_bitcnt = comp_preamble_ptr->bitcnt;
  comp_preamble_ptr->length = ( comp_preamble_ptr->bitcnt + 35L ) / 36L;
  comp_preamble_ptr->maximum_bitcnt = comp_preamble_ptr->length * 36L;
  if (copy_to_empty_file(
        arc_contents_file,
        comp_contents_file,
        is_ascii ? comp_preamble_ptr->bitcnt * 8 / 9
                                   : comp_preamble_ptr->bitcnt)
      != 0)
  {
    return ( -2 );
  }

  if (eof_reached(arc_contents_file))
  {
    return ( 0 );
  }

  if (is_ascii)
  {
    skip_mxbits(
      arc_contents_file,
      ( comp_preamble_ptr->maximum_bitcnt
        - comp_preamble_ptr->bitcnt )
      * 8 / 9);
  }
  else
  {
    skip_mxbits(
      arc_contents_file,
      comp_preamble_ptr->maximum_bitcnt
      - comp_preamble_ptr->bitcnt);
  }

  return ( 0 );
}

#ifdef ANSI_FUNC
static long
cv_arctime (char *time_string)
#else
static long
cv_arctime(time_string)

char *time_string;
#endif

{

  /*
   * "10/18/82  1106.1"
   * is format of times in archive headers
   * "0123456789012345"
   */

  struct tm time_struct;
  char temp_str[3];
  memset(&time_struct, 0, sizeof ( time_struct ));
  temp_str[2] = '\0';

  memcpy(temp_str, time_string, 2);
  time_struct.tm_mon = atoi(temp_str) - 1;

  memcpy(temp_str, time_string + 3, 2);
  time_struct.tm_mday = atoi(temp_str);

  memcpy(temp_str, time_string + 6, 2);
  time_struct.tm_year = atoi(temp_str);

  memcpy(temp_str, time_string + 10, 2);
  time_struct.tm_hour = atoi(temp_str);

  memcpy(temp_str, time_string + 12, 2);
  time_struct.tm_min = atoi(temp_str);

  memcpy(temp_str, time_string + 15, 2);
  time_struct.tm_sec = atoi(temp_str) * 6;

  return ((long)encodetm(&time_struct));
}
