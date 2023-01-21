 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

#include "mxload.h"
#include <stdio.h>
#include "copybits.h"
#include "mxbitio.h"
#include "tempfile.h"

static unsigned char big_buffer[4608]; /* 1024 Multics words */

#ifdef ANSI_FUNC
int
copy_to_empty_file (MXBITFILE *infile, MXBITFILE *outfile, long n_bits)
#else
int
copy_to_empty_file(infile, outfile, n_bits)

MXBITFILE *infile;
MXBITFILE *outfile;
long n_bits;
#endif

{
  long n_read;
  long n_left;
  int n_bytes;
  int i;
  long buffer_bits;

  rewind_mxbit_file(outfile, "w");
  n_left = n_bits;
  while (n_left > 0)
  {

        /*
         * Try to read the biggest possible chunk, limited by
         * remaining space in tape block if reading from tape
         */

    buffer_bits = sizeof ( big_buffer ) * 8L;
    if (infile->n_bytes_left_in_tape_block > 0
        && buffer_bits > infile->n_bytes_left_in_tape_block * 8)
    {
      buffer_bits = infile->n_bytes_left_in_tape_block * 8L;
    }

    if (n_left <= buffer_bits)
    {
      buffer_bits = n_left;
    }

    n_read = get_mxbits(infile, buffer_bits, big_buffer);
    if (n_read != buffer_bits)
    {
      return ( -1 );
    }

    n_bytes = ( buffer_bits + 7 ) / 8;
    fwrite(big_buffer, 1, n_bytes, outfile->realfile);
    n_left -= buffer_bits;
  }

  return ( 0 );
}

#ifdef ANSI_FUNC
int
copy_bits (MXBITFILE *infile, MXBITFILE *outfile, long n_bits)
#else
int
copy_bits(infile, outfile, n_bits)

MXBITFILE *infile;
MXBITFILE *outfile;
long n_bits;
#endif

{
  char buffer[1000];
  long n_read;
  long buffer_bits;
  long n_left;

  n_left = n_bits;
  buffer_bits = sizeof ( buffer ) * 8L;
  while (n_left > 0)
  {
    if (n_left <= buffer_bits)
    {
      buffer_bits = n_left;
    }

    n_read = get_mxbits(infile, buffer_bits, buffer);
    n_read = put_mxbits(outfile, n_read, buffer);
    n_left -= n_read;
    if (n_read != buffer_bits)
    {
      return ( n_bits - n_left );
    }
  }

  return ( n_bits );
}

/*
 * Routine to convert a sequence of Multics 9-bit bytes into 8-bit bytes.
 * Usually this is for conversion of ascii data. This is done in a semi-
 * complicated way because metering has shown this routine to be very
 * important for performance. The main loop reads 72-bit chunks and
 * converts the 8 9-bit bytes to 9 8-bit bytes. So, let's diagram this out
 * for clarity...here's how 8 9-bit bytes map onto 9 8-bit bytes:
 *
 *                                8 9-bit bytes
 * 012345678    1    012345678    3    012345678    5    012345678    7
 *     0    012345678    2    012345678    4    012345678    6    012345678
 * ------------------------------------------------------------------------
 *     0   80123456    2   67801234    4   45678012    6   23456780    8
 * 01234567    1   78012345    3   56780123    5   34567801    7   12345678
 *                               9 8-bit bytes
 *
 */

#ifdef ANSI_FUNC
void
copy_8bit (MXBITFILE *contents_file, char *new_path, unsigned long bitcount)
#else
void
copy_8bit(contents_file, new_path, bitcount)

MXBITFILE *contents_file;
char *new_path;
unsigned long bitcount;
#endif

{
  FILE *outfile;

  if (( outfile = fopen(new_path, "w")) == NULL)
  {
    fprintf(stderr, "Cannot open file %s for writing.\n", new_path);
    return;
  }

  copy_8bit_to_file(contents_file, outfile, bitcount);
}

#ifdef ANSI_FUNC
void
copy_8bit_to_file (MXBITFILE *contents_file, FILE *outfile, unsigned long bitcount)
#else
void
copy_8bit_to_file(contents_file, outfile, bitcount)

MXBITFILE *contents_file;
FILE *outfile;
unsigned long bitcount;
#endif

{
  long n_read;
  char *type;
  long n_72_bit_chunks_left;
  unsigned char *subbuffer; /* For addressing 72 bits within buffer */
  long buffer_bits;
  int n_chunks_this_buffer;
  int leftover_chars;

  rewind_mxbit_file(contents_file, "rt");

  n_72_bit_chunks_left = bitcount / 72L;
  leftover_chars = bitcount / 9L - n_72_bit_chunks_left * 8L;

  while (n_72_bit_chunks_left > 0)
  {

        /*
         * Try to read the biggest possible chunk, limited by
         * remaining space in tape block if reading from tape
         */

    buffer_bits = sizeof ( big_buffer ) * 8L;
    if (contents_file->n_bytes_left_in_tape_block > 0
        && buffer_bits > contents_file->n_bytes_left_in_tape_block * 8)
    {
      buffer_bits = contents_file->n_bytes_left_in_tape_block * 8L;
    }

    if (n_72_bit_chunks_left * 72 <= buffer_bits)
    {
      buffer_bits = n_72_bit_chunks_left * 72;
    }

    n_read = get_mxbits(contents_file, buffer_bits, big_buffer);

    n_chunks_this_buffer = n_read / 72;
    n_72_bit_chunks_left -= n_chunks_this_buffer;
    subbuffer = big_buffer;
    while (n_chunks_this_buffer > 0)
    {
      putc(subbuffer[0] << 1 | subbuffer[1] >> 7, outfile);  /* 0 */
      putc(subbuffer[1] << 2 | subbuffer[2] >> 6, outfile);  /* 1 */
      putc(subbuffer[2] << 3 | subbuffer[3] >> 5, outfile);  /* 2 */
      putc(subbuffer[3] << 4 | subbuffer[4] >> 4, outfile);  /* 3 */
      putc(subbuffer[4] << 5 | subbuffer[5] >> 3, outfile);  /* 4 */
      putc(subbuffer[5] << 6 | subbuffer[6] >> 2, outfile);  /* 5 */
      putc(subbuffer[6] << 7 | subbuffer[7] >> 1, outfile);  /* 6 */
      putc(subbuffer[8], outfile);                           /* 7 */
      subbuffer += 9;
      --n_chunks_this_buffer;
    }
  }

  /*
   * And read to the end of the file using bit i/o
   * routines to pick remaining bytes apart
   */

  while (leftover_chars-- > 0)
  {
    skip_mxbits(contents_file, 1L);
    n_read = get_mxbits(contents_file, 8L, big_buffer);
    putc(big_buffer[0], outfile);
  }
  fclose(outfile);
}

#ifdef ANSI_FUNC
void
copy_file (MXBITFILE *contents_file, char *new_path, int is_ascii, long char_count)
#else
void
copy_file(contents_file, new_path, is_ascii, char_count)

MXBITFILE *contents_file;
char *new_path;
int is_ascii;
long char_count;
#endif

{
  FILE *outfile;
  int n_read;
  int n_to_read;
  long n_left;

  rewind_mxbit_file(contents_file, "rt");

  if (( outfile = fopen(new_path, "w")) == NULL)
  {
    fprintf(stderr, "Cannot open file %s for writing.\n", new_path);
    return;
  }

  n_left = char_count;
  while (n_left > 0)
  {
    if (n_left < 4096)
    {
      n_to_read = n_left;
    }
    else
    {
      n_to_read = 4096;
    }

    n_read = fread(big_buffer, 1, n_to_read, contents_file->realfile);
    fwrite(big_buffer, 1, n_read, outfile);
    n_left -= n_to_read;
  }

  fclose(outfile);
}
