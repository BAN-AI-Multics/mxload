 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

#include "mxload.h"
#include <stdio.h>
#include <string.h>
#include "multtape.h"
#include "mxbitio.h"

 /*
  * See if file is a tape file. If it is, see
  * if we're positioned at beginning of tape,
  * i.e. the label, or at the first data record.
  * (See mstr.incl.pl1 and AG91 Appendix F for
  * descriptions of tape_mult_ formats.)
  */

#ifdef ANSI_FUNC
void
read_tape_label (MXBITFILE *mxbitfile)
#else
void
read_tape_label(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  int tape_byte[4];
  int byteN;
  char volume_id[34];
  long rec_within_file;
  long phy_file;
  long data_bits_used;

  long data_bit_len;
  unsigned long flags;

  /*
   * Read first 36 bits of file
   * to see if they contain the
   * magic constant 670314355245
   * (octal) that is in first word
   * of every tape_mult_ record.
   */

  for (byteN = 0; byteN < 4; ++byteN)
  {
    tape_byte[byteN] = get_9_mxbit_integer(mxbitfile);
  }

  if (tape_byte[0] != 0670 || tape_byte[1] != 0314 || tape_byte[2] != 0355
      || tape_byte[3] != 0245)
  {
    mxbit_pos(mxbitfile, 0L);
    return;                                        /* Not tape_mult_ */
  }

  mxbitfile->tape_mult_sw = 1;

  /*
   * See if this is
   * a label record
   */

  skip_mxbits(mxbitfile, 72L);                     /* Skip UID */
  rec_within_file = get_18_mxbit_integer(mxbitfile);
  phy_file = get_18_mxbit_integer(mxbitfile);
  data_bits_used = get_18_mxbit_integer(mxbitfile);
  data_bit_len = get_18_mxbit_integer(mxbitfile);
  flags = get_18_mxbit_integer(mxbitfile);
  if (( flags & 0x30000 ) == 0x30000)              /* If flags.admin */
  {                                                /*  and flags.label */
                                                   /* It is a label */
    skip_mxbits(mxbitfile, 18L);                   /* Skip rest of flags */
    skip_mxbits(mxbitfile, 72L);                   /* Skip rest of header */
    skip_mxbits(mxbitfile, 32L * 9L);              /* Skip installation ID */
    get_mxstring(mxbitfile, volume_id, 32);
    printf("Reading Multics Tape %s\n", volume_id);
    mxbit_pos(mxbitfile, 0L);                      /* Go back to beginning */
    read_tape_record_header(mxbitfile);
    skip_mxbits(mxbitfile, data_bits_used);        /* Skip label record */
    read_tape_record_trailer(mxbitfile);

         /*
          * Label record was separate
          * file on tape, so next record
          * is record 0 of 2nd file
          */

    mxbitfile->next_tape_block_number = 0;
    read_tape_record_header(mxbitfile);
  }
  else
  {
    mxbit_pos(mxbitfile, 0L);                      /* Go back to beginning */
    read_tape_record_header(mxbitfile);
  }

  return;
}

#ifdef ANSI_FUNC
void
read_tape_record_header (MXBITFILE *mxbitfile)
#else
void
read_tape_record_header(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  int tape_byte[4];
  int i;
  int looking_for_next_record;
  int consecutive_errors;
  long rec_within_file;
  long phy_file;
  long data_bits_used;
  long data_bit_len;
  long flags;

  mxbitfile->reading_tape_data = 0;                /* Reading control info */
  looking_for_next_record = 1;
  while (looking_for_next_record)
  {

        /*
         * Skip over tape marks after label,
         * before end-of-reel record, and
         * after every 128 blocks.  Also skips
         * past any block read errors
         */

    consecutive_errors = 0;
    while (( tape_byte[0] = get_9_mxbit_integer(mxbitfile)) == EOF)
    {
      if (++consecutive_errors > 10)
      {
        fprintf(stderr, "Too many tape read errors.\n");
        mxlexit(4);
      }

      clearerr(mxbitfile->realfile);
    }
    for (i = 1; i < 4; ++i)
    {
      tape_byte[i] = get_9_mxbit_integer(mxbitfile);
    }

    if (tape_byte[0] != 0670 || tape_byte[1] != 0314 || \
        tape_byte[2] != 0355 || tape_byte[3] != 0245)
    {
      fprintf(stderr, "Invalid tape block header.  ");
      fprintf(
        stderr,
        "First word contains %3.3o%3.3o%3.3o%3.3oo ",
        tape_byte[0],
        tape_byte[1],
        tape_byte[2],
        tape_byte[3]);
      fprintf(stderr, "instead of 670314355245o.\n");
      mxlexit(4);
    }

    skip_mxbits(mxbitfile, 72L);           /* Skip UID */
    rec_within_file = get_18_mxbit_integer(mxbitfile);
    phy_file = get_18_mxbit_integer(mxbitfile);
    data_bits_used = get_18_mxbit_integer(mxbitfile);
    data_bit_len = get_18_mxbit_integer(mxbitfile);
    flags = get_18_mxbit_integer(mxbitfile);
    skip_mxbits(mxbitfile, 18L);           /* Skip rest of flags */
    skip_mxbits(mxbitfile, 72L);           /* Skip rest of header */
    if (( flags & 0x28000 ) == 0x28000)    /* If flags.admin and flags.eor */
    {
      printf("End of reel encountered\n");
      mxbitfile->end_of_reel_reached = 1;
      mxbitfile->reading_tape_data = 1;
      return;
    }

    if (rec_within_file == mxbitfile->next_tape_block_number)
    {
      looking_for_next_record = 0;
      ++( mxbitfile->next_tape_block_number );
      if (mxbitfile->next_tape_block_number == 128)
      {
        mxbitfile->next_tape_block_number = 0;
      }
    }
    else if (rec_within_file < mxbitfile->next_tape_block_number)
    {
      fprintf(stderr, "Skipping doubly-written tape block.\n");
      looking_for_next_record = 1;         /* Skipping repeated record */

          /*
           * Skip over
           * whole record
           */

      skip_mxbits(mxbitfile, data_bit_len);
      read_tape_record_trailer(mxbitfile);
    }
    else                                   /* rec_within_file > mxbitfile */
    {                                      /* *-> next_tape_block_number) */
      fprintf(
        stderr,
        "%ld tape blocks skipped due to read errors.\n",
        rec_within_file - mxbitfile->next_tape_block_number);
      looking_for_next_record = 0;
      mxbitfile->next_tape_block_number = rec_within_file + 1;
    }
  }
  mxbitfile->n_bytes_left_in_tape_block = \
      data_bits_used / 8L;
  mxbitfile->n_pad_bytes_in_tape_block = \
    ( data_bit_len - data_bits_used ) / 8L;
  mxbitfile->reading_tape_data = 1;        /* Back to reading real data */
  return;
}

#ifdef ANSI_FUNC
void
read_tape_record_trailer (MXBITFILE *mxbitfile)
#else
void
read_tape_record_trailer(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  int tape_byte[4];
  int i;

  mxbitfile->reading_tape_data = 0;        /* Reading control info */
  skip_mxbits(
    mxbitfile,
    (long)( mxbitfile->n_pad_bytes_in_tape_block * 8L ));
  for (i = 0; i < 4; ++i)
  {
    tape_byte[i] = get_9_mxbit_integer(mxbitfile);
  }

  if (tape_byte[0] != 0107 || tape_byte[1] != 0463 || \
      tape_byte[2] != 0422 || tape_byte[3] != 0532)
  {
    fprintf(stderr, "Invalid tape block trailer.  ");
    fprintf(
      stderr,
      "First word contains %3.3o%3.3o%3.3o%3.3oo ",
      tape_byte[0],
      tape_byte[1],
      tape_byte[2],
      tape_byte[3]);
    fprintf(stderr, "instead of 107463422532o.\n");
    mxlexit(4);
  }

  skip_mxbits(mxbitfile, 7L * 36L);
  mxbitfile->reading_tape_data = 1;        /* Back to reading real data */
}
