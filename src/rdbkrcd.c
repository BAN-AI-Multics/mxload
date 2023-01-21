 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

 /*
  * Routines to read a backup logical record.
  * The header is read into a temporary file by
  * rdbkrcd and the segment contents are either
  * read into another file or discarded by rdseg.
  * Returns EOF if end of file is encountered, -1
  * if an error occured, 0 otherwise. This routine
  * corresponds to the calls to iox_$get_chars
  * near the beginning of bk_input.pl1.
  */

#include "mxload.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "copybits.h"
#include "mxbitio.h"
#include "preamble.h"

static int find_backup_logical_record();

#ifdef ANSI_FUNC
int
rdbkrcd (MXBITFILE *infile, MXBITFILE *preamble_file,
                struct PREAMBLE *preamble_ptr)
#else
int
rdbkrcd(infile, preamble_file, preamble_ptr)

MXBITFILE *infile;
MXBITFILE *preamble_file;
struct PREAMBLE *preamble_ptr;
#endif

{
  long preamble_length;
  long segment_length;
  unsigned long long_pair[2];
  int overflow_long;

  /*
   * Get
   * theader.compare
   */

  if (find_backup_logical_record(infile) == EOF)
  {
    return ( EOF );
  }

  /*
   * Get
   * theader.hdrcnt
   */

  overflow_long = get_36_mxbit_integer(infile, long_pair);
  preamble_length = long_pair[1];

  /*
   * Get
   * theader.segcnt
   */

  overflow_long = get_36_mxbit_integer(infile, long_pair);
  segment_length = long_pair[1];

  if (copy_to_empty_file(
        infile,
        preamble_file,
        (long)( preamble_length * 36 ))
      != 0)
  {
    return ( -1 );
  }

  if (( 32 + preamble_length ) % 256 != 0)
  {

        /*
         * Skip rest of
         * 256-word block
         */

    if (skip_mxbits(
          infile,
          (long)( 36 * ( 256 - (( 32 + preamble_length ) % 256 ))))
        != 0)
    {
      return ( -1 );
    }
  }

  get_preamble_min(preamble_file, preamble_ptr);

  preamble_ptr->length = segment_length;
  preamble_ptr->maximum_bitcnt = segment_length * 36;
  if (preamble_ptr->bitcnt == 0
      || preamble_ptr->bitcnt > preamble_ptr->maximum_bitcnt)
  {
    preamble_ptr->adjusted_bitcnt = preamble_ptr->maximum_bitcnt;
  }
  else
  {
    preamble_ptr->adjusted_bitcnt = preamble_ptr->bitcnt;
  }

  return ( 0 );
}

#ifdef ANSI_FUNC
int
rdseg (MXBITFILE *infile, MXBITFILE *contents_file,
                struct PREAMBLE *preamble_ptr)
#else
int
rdseg(infile, contents_file, preamble_ptr)

MXBITFILE *infile;
MXBITFILE *contents_file;
struct PREAMBLE *preamble_ptr;
#endif

{

  /*
   * Copy segment contents
   * to temp file
   */

  if (preamble_ptr->length == 0)
  {
    return ( 0 );
  }

  /*
   * Null file means to skip data
   * away instead of reading it
   */

  if (contents_file == NULL)
  {
    skip_mxbits(infile, (long)( preamble_ptr->maximum_bitcnt ));
    return ( 0 );
  }

  if (copy_to_empty_file(
        infile,
        contents_file,
        (long)( preamble_ptr->adjusted_bitcnt ))
      != 0)
  {
    fprintf(stderr, "Cannot copy temp file for segment contents.\n");
    return ( -1 );
  }

  skip_mxbits(
    infile,
    (long)( preamble_ptr->maximum_bitcnt
            - preamble_ptr->adjusted_bitcnt ));

  if (preamble_ptr->length % 256 != 0)
  {

        /*
         * Skip rest of
         * 256-word block
         */

    skip_mxbits(
      infile,
      (long)( 36 * ( 256 - ( preamble_ptr->length % 256 ))));
  }

  return ( 0 );
}

 /*
  * Find backup record sentinel,
  * return positioned to the preamble
  * length field of theader structure
  * defined in bk_input
  */

#ifdef ANSI_FUNC
static int
find_backup_logical_record (MXBITFILE *infile)
#else
static int
find_backup_logical_record(infile)

MXBITFILE *infile;
#endif

{
  static char backup_record_sentinel[121] =
    " z z z z z z z z z z z z z z z z\
This is the beginning of a backup logical record.        \
z z z z z z z z z z z z z z z z";

  char temp_string[121];
  int record_header_found;

  record_header_found = 0;
  if (eof_reached(infile))
  {
    return ( EOF );
  }

  while (!record_header_found)
  {
    get_mxstring(infile, temp_string, 120);
    if (memcmp(backup_record_sentinel, temp_string, 120) == 0)
    {
      record_header_found = 1;
    }
    else
    {

          /*
           * Skip 36 bits for preamble length
           * + 36 bits for segment length
           */

      skip_mxbits(infile, 72L);
      fprintf(
        stderr,
        "Skipping 128 bytes looking for backup record header.\n");
    }
  }
  return ( 0 );
}
