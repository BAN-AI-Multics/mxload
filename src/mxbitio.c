 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

 /* MXBITIO */

 /*
  * Routines to do bit-by-bit I/O on files.
  * Users of these programs should use the
  * include file mxbitio.h which declares
  * and describes the use of these routines.
  */

#include "mxload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "multtape.h"
#include "mxbitio.h"

#define getbit(bf)                                                         \
  ( bf->byte_buffer_pos <= 7                                               \
       ? bf->byte_buffer & ( 0x80 >> ( bf->byte_buffer_pos )++ )           \
       : first_bit_next_byte(bf))

static int get_8_mxbits_from_9();
static int putbit();
static int get_whole_bytes();
static int next_tape_byte();
static int first_bit_next_byte();
static int put_byte();

#define SEEK_SET 0
#define SEEK_CUR 1

int freadwrapper (char *ptr, int size, int nitems, MXBITFILE *mxbitfile);
int fseekwrapper (MXBITFILE *mxbitfile, int n);
int read_rewind (MXBITFILE *mxbitfile);
int getcwrapper (MXBITFILE *mxbitfile);

#ifdef ANSI_FUNC
MXBITFILE *
open_mxbit_file (char *name, char *mode)
#else
MXBITFILE *
open_mxbit_file(name, mode)

char *name;
char *mode;
#endif

{
  FILE *realfile;
  char realmode[3];
  MXBITFILE *mxbitfile;

  /*
   * Make real mode string
   * for read or write
   */

  if (strchr(mode, 'r') != NULL)
  {
    strcpy(realmode, "r");
  }
  else if (strchr(mode, 'w') != NULL)
  {
    strcpy(realmode, "w");
  }
  else
  {
    return ( NULL );
  }

  /*
   * Open the
   * byte-stream file
   */

  if (strcmp(name, "-") == 0)
  {
    realfile = stdin;
  }
  else if (( realfile = fopen(name, realmode)) == NULL)
  {
    perror(name);
    return ( NULL );
  }

  /*
   * Allocate and initialize
   * MXBITFILE structure
   */

  mxbitfile = (MXBITFILE *)malloc(sizeof ( MXBITFILE ));
  if (mxbitfile == NULL)
  {
    fprintf(stderr, "Error opening file.  No room to ");
    fprintf(stderr, "allocate mxbitfile structure.\n");
    fclose(realfile);
    return ( NULL );
  }

  memset((char *)mxbitfile, 0, sizeof ( MXBITFILE ));
  mxbitfile->realfile = realfile;
  strcpy(mxbitfile->path, name);
  if (strchr(mode, 't') != NULL)
  {
    mxbitfile->temp_file = 1;
  }

  if (realmode[0] == 'w')
  {
    mxbitfile->write = 1;
  }
  else
  {
    mxbitfile->byte_buffer_pos = 8; /* Forces first getbit call */
                                    /*  to fetch new byte */
    if (!mxbitfile->temp_file)
    {                               /* Might be a tape file or stdin */
      mxbitfile->saved_input_ptr =
        (unsigned char *)malloc(INPUT_BUFFER_SIZE);
      if (mxbitfile->saved_input_ptr == NULL)
      {
        fprintf(stderr, "Error opening file.  No room to ");
        fprintf(stderr, "allocate saved input buffer.\n");
        if (strcmp(mxbitfile->path, "-") != 0)
        {
          fclose(realfile);
        }

        return ( NULL );
      }
#ifdef TAPEBUF

          /*
           * The XPS's tape driver seems to require
           * that the file's buffer size be bigger
           * than the tape_mult_ tape block size,
           * which is 4680. Performance on other
           * systems may be improved by using a big
           * buffer, but on the Sun, use of setvbuf
           * does not seem to work at all for tape files.
           */

      mxbitfile->buffer_ptr = malloc(abs(TAPEBUF));
      if (mxbitfile->buffer_ptr == NULL)
      {
        fprintf(stderr, "Error opening file.  No room to ");
        fprintf(stderr, "allocate tape buffer.\n");
        if (strcmp(mxbitfile->path, "-") != 0)
        {
          fclose(realfile);
        }

        return ( NULL );
      }

      setvbuf(realfile, mxbitfile->buffer_ptr, _IOFBF, abs(TAPEBUF));
#endif /* ifdef TAPEBUF */
      read_tape_label(mxbitfile);
    }
  }

  return ( mxbitfile );
}

#ifdef ANSI_FUNC
void
rewind_mxbit_file (MXBITFILE *mxbitfile, char *mode)
#else
void
rewind_mxbit_file(mxbitfile, mode)

MXBITFILE *mxbitfile;
char *mode;
#endif

{
  FILE *realfile;
  int seek_result = -1;
  MXBITFILE *temp_mxbitfile;
  char temp_path[400];

  /*
   * If file is to be used in same mode as before, just rewind.
   * Otherwise have to do a close and open, making it invisible
   * to the caller by leaving his file pointer the same as
   * before.  Note that this routine was written with the
   * intention of having file openings be for update and always
   * just rewinding in this routine.  However, the sad truth is
   * that files open for update sometimes (or perhaps always)
   * have their file buffering turned off or greatly reduced,
   * resulting in lousy performance for reads and writes.
   * So we live with doing a lot of opens and closes.
   *
   * If we're reading from stdin, real rewinding won't work.
   * So we keep the first INPUT_BUFFER_SIZE bytes of data that's
   * read and "rewind" (in read_rewind) by going back to pull
   * data from the buffer.
   */

  if (strchr(mode, 'w') != NULL && mxbitfile->write)
  {
    mxbitfile->byte_buffer = 0;
    mxbitfile->write = 1;
    seek_result = fseek(mxbitfile->realfile, 0L, SEEK_SET);
  }
  else if (strchr(mode, 'r') != NULL && !mxbitfile->write)
  {
    read_rewind(mxbitfile);
  }
  else
  {
    strcpy(temp_path, mxbitfile->path);
    close_mxbit_file(mxbitfile);
    temp_mxbitfile = open_mxbit_file(temp_path, mode);
    memcpy(mxbitfile, temp_mxbitfile, sizeof ( MXBITFILE ));
    free(temp_mxbitfile);
  }
}

/*
 * Note that caller is responsible for
 * freeing the MXBITFILE structure
 */

#ifdef ANSI_FUNC
void
close_mxbit_file (MXBITFILE *mxbitfile)
#else
void
close_mxbit_file(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  if (mxbitfile->write)
  {

        /*
         * Write out any bits left
         * in the byte buffer
         */

    if (( mxbitfile->byte_buffer_pos >= 8 )
        || ( mxbitfile->byte_buffer_pos > 0 &&
             mxbitfile->byte_buffer != 0 ))
    {
      putc(mxbitfile->byte_buffer, mxbitfile->realfile);
    }
  }

  if (strcmp(mxbitfile->path, "-") != 0)
  {
    fclose(mxbitfile->realfile);
  }

  if (mxbitfile->buffer_ptr != NULL)
  {
    free(mxbitfile->buffer_ptr);
  }

  if (mxbitfile->saved_input_ptr != NULL)
  {
    free(mxbitfile->saved_input_ptr);
  }
}

/*
 * Returns number of bits actually
 * read, which is less than count if
 * end-of-file is reached or an error
 * occurs.
 */

#ifdef ANSI_FUNC
long
get_mxbits (MXBITFILE *mxbitfile, long count, unsigned char *string)
#else
long
get_mxbits(mxbitfile, count, string)

MXBITFILE *mxbitfile;
long count;
unsigned char *string;
#endif

{
  int i;
  int next_bit;
  unsigned int n_bytes_to_get;
  int n_extra_bits_to_get;
  int j;
  int next_byte;
  unsigned int n_read;

  /*
   * First read full
   * bytes, if any
   */

  n_bytes_to_get = count / 8L;
  if (n_bytes_to_get != 0)
  {

        /*
         * If we are positioned at a byte
         * boundary, get whole bytes
         */

    if (mxbitfile->byte_buffer_pos == 8)
    {

          /*
           * Special case tapes,
           * fighting for optimization.
           */

      if (mxbitfile->reading_tape_data)
      {

                /*
                 * If we are at end of one tape
                 * block jump to beginning of next
                 */

        if (mxbitfile->n_bytes_left_in_tape_block == 0
            && mxbitfile->reading_tape_data)
        {
          read_tape_record_trailer(mxbitfile);
          read_tape_record_header(mxbitfile);
        }

        if (n_bytes_to_get <= mxbitfile->n_bytes_left_in_tape_block)
        {
          n_read = freadwrapper((char *)string, 1, n_bytes_to_get, mxbitfile);
          if (n_read != n_bytes_to_get)
          {
            fprintf(
              stderr,
              "Premature end of file reading %s.\n",
              mxbitfile->path);
            return ( n_read * 9L );
          }

          mxbitfile->n_bytes_left_in_tape_block -= n_bytes_to_get;
        }
        else
        {
          for (j = 0; j < n_bytes_to_get; ++j)
          {
            *( string + j ) = next_byte = next_tape_byte(mxbitfile);
          }

          if (next_byte == EOF)
          {
            return ( j * 9L );
          }
        }
      }
      else
      {
        n_read = freadwrapper((char *)string, 1, n_bytes_to_get, mxbitfile);
        if (n_read != n_bytes_to_get)
        {
          if (!mxbitfile->temp_file)
          {
            fprintf(
              stderr,
              "Premature end of file reading %s.\n",
              mxbitfile->path);
          }

          return ( n_read * 9L );
        }
      }
    }
    else
    {

          /*
           * Not at byte boundary, must
           * stuff bit by bit into bytes
           */

      for (j = 0; j < n_bytes_to_get; ++j)
      {
        *( string + j ) = 0;
        for (i = 0; i < 8; ++i)
        {
          if ( ( next_bit = getbit(mxbitfile) ) )
          {
            *( string + j ) |= 0x80 >> i;
          }
        }
      }
    }
  }

  /*
   * Get the extra bits
   * in last byte
   */

  n_extra_bits_to_get = count % 8;
  if (n_extra_bits_to_get != 0)
  {
    string[n_bytes_to_get] = 0;
    for (i = 0; i < n_extra_bits_to_get; ++i)
    {
      next_bit = getbit(mxbitfile);
      if (next_bit == EOF)
      {
        return ( n_bytes_to_get * 9L + i );
      }

      if (next_bit)
      {
        string[n_bytes_to_get] |= 0x80 >> i;
      }
    }
  }

  return ( count );
}

 /*
  * Returns number of bits actually
  * written, which is less than count
  * if an error occurs.
  */

#ifdef ANSI_FUNC
int
put_mxbits (MXBITFILE *mxbitfile, int count, char *string)
#else
int
put_mxbits(mxbitfile, count, string)

MXBITFILE *mxbitfile;
int count;
char *string;
#endif

{
  int i;
  int next_bit;
  int byteN;
  int n_bytes_to_put;
  int n_extra_bits_to_put;
  int n_bits_written;
  int bit_written;

  n_bytes_to_put = count / 8;
  n_extra_bits_to_put = count % 8;
  n_bits_written = 0;
  for (byteN = 0; byteN < n_bytes_to_put; ++byteN)
  {

        /*
         * Write next 8 bits
         * from a byte
         */

    if (mxbitfile->byte_buffer_pos == 0) /* If at beginning of file, */
                                         /* can handle whole byte at once */
    {
      putc(string[byteN], mxbitfile->realfile);
    }
    else if (mxbitfile->byte_buffer_pos >= 8) /* If at byte boundary, can */
                                              /* handle full byte at once */
    {
      putc(mxbitfile->byte_buffer, mxbitfile->realfile);
      mxbitfile->byte_buffer = string[byteN];
    }
    else /* Have to do it bit-by-bit */
    {
      for (i = 0; i < 8; ++i)
      {
        next_bit = (( string[byteN] & ( 0x80 >> i )) != 0 );
        bit_written = putbit(mxbitfile, next_bit);
        if (!bit_written)
        {
          return ( n_bits_written );
        }
        else
        {
          ++n_bits_written;
        }
      }
    }
  }

  /*
   * Write the extra bits
   * in last byte
   */

  for (i = 0; i < n_extra_bits_to_put; ++i)
  {
    next_bit = string[byteN] & ( 0x80 >> i );
    bit_written = putbit(mxbitfile, next_bit);
    if (!bit_written)
    {
      return ( n_bits_written );
    }
    else
    {
      ++n_bits_written;
    }
  }

  return ( count );
}

#ifdef ANSI_FUNC
static int
putbit (MXBITFILE *mxbitfile, int bit)
#else
static int
putbit(mxbitfile, bit)

MXBITFILE *mxbitfile;
int bit;
#endif

{
  if (mxbitfile->byte_buffer_pos >= 8)
  {
    if (putc(mxbitfile->byte_buffer, mxbitfile->realfile) == EOF)
    {
      return ( 0 );
    }

    mxbitfile->byte_buffer = '\0';
    mxbitfile->byte_buffer_pos = 0;
  }

  if (bit)
  {
    mxbitfile->byte_buffer |= ( 0x80 >> ( mxbitfile->byte_buffer_pos ));
  }

  ++( mxbitfile->byte_buffer_pos );

  return ( 1 );
}

#ifdef ANSI_FUNC
int
get_mxstring (MXBITFILE *mxbitfile, char *string, int len)
#else
int
get_mxstring(mxbitfile, string, len)

MXBITFILE *mxbitfile;
char *string;
int len;
#endif

{
  unsigned char buffer[10];
  int i;
  int last_nonblank;
  int n_72_bit_chunks;
  int n_read;

  n_72_bit_chunks = len / 8;
  i = 0;
  n_read = 72;
  while (n_72_bit_chunks-- > 0)
  {
    n_read = get_mxbits(mxbitfile, 72L, buffer);
    string[i] = buffer[0] << 1 | buffer[1] >> 7;
    string[i + 1] = buffer[1] << 2 | buffer[2] >> 6;
    string[i + 2] = buffer[2] << 3 | buffer[3] >> 5;
    string[i + 3] = buffer[3] << 4 | buffer[4] >> 4;
    string[i + 4] = buffer[4] << 5 | buffer[5] >> 3;
    string[i + 5] = buffer[5] << 6 | buffer[6] >> 2;
    string[i + 6] = buffer[6] << 7 | buffer[7] >> 1;
    string[i + 7] = buffer[8];
    i += 8;
  }
  if (n_read != 72)
  {
    return ( EOF );
  }

       /*
        * And read to the end of the string
        * using bit i/o routines to pick
        * remaining bytes apart
        */

  n_read = 8;
  while (i < len)
  {
    skip_mxbits(mxbitfile, 1L);
    n_read = get_mxbits(mxbitfile, 8L, buffer);
    string[i] = buffer[0];
    ++i;
  }
  if (n_read != 8)
  {
    return ( EOF );
  }

  last_nonblank = -1;
  for (i = 0; i < len; ++i)
  {
    if (string[i] == '\0')
    {
      string[i] = '-';
    }
    else if (string[i] != ' ')
    {
      last_nonblank = i;
    }
  }

  string[last_nonblank + 1] = '\0';
  return ( len );
}

#ifdef ANSI_FUNC
int
mxbit_pos (MXBITFILE *mxbitfile, long pos)
#else
int
mxbit_pos(mxbitfile, pos)

MXBITFILE *mxbitfile;
long pos;
#endif

{
  int seek_result = -1;

  if (mxbitfile->write)
  {
    seek_result = fseek(mxbitfile->realfile, 0L, SEEK_SET);
    mxbitfile->byte_buffer_pos = 0;
    mxbitfile->byte_buffer = '\0';
  }
  else
  {
    read_rewind(mxbitfile);
  }

  return ( skip_mxbits(mxbitfile, pos));
}

#ifdef ANSI_FUNC
int
get_9_mxbit_integer (MXBITFILE *mxbitfile)
#else
int
get_9_mxbit_integer(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  int i;
  int result;
  int next_bit;

  /*
   * First bit goes in high-order
   * byte of integer result
   */

  next_bit = getbit(mxbitfile);
  if (next_bit == EOF)
  {
    return ( EOF );
  }

  if (next_bit)
  {
    result = 0x0100;
  }
  else
  {
    result = 0;
  }

  /*
   * Stuff next 8 bits into
   * lower-order byte
   */

  for (i = 0; i < 8; ++i)
  {
    next_bit = getbit(mxbitfile);
    if (next_bit == EOF)
    {
      return ( EOF );
    }

    if (next_bit)
    {
      result |= 0x80 >> i;
    }
  }

  return ( result );
}

/*
 * Read a Multics 36-bit integer and
 * plug it into an array of 2 longs,
 * returning a non-zero value if
 * top 4 bits are non-zero
 */

#ifdef ANSI_FUNC
int
get_36_mxbit_integer (MXBITFILE *mxbitfile,
                unsigned long value[2])
#else
int
get_36_mxbit_integer(mxbitfile, value)

MXBITFILE *mxbitfile;
unsigned long value[2];
#endif

{
  unsigned char buffer[5];
  int high_order_bits_non_zero;

  get_mxbits(mxbitfile, 36L, buffer);
  high_order_bits_non_zero = value[0] = buffer[0] >> 4;
  value[1] = (unsigned long)buffer[0] << 28 | \
             (unsigned long)buffer[1] << 20 | \
             (unsigned long)buffer[2] << 12 | \
               buffer[3] << 4 | buffer[4] >> 4;
  return ( high_order_bits_non_zero );
}

 /*
  * Read a Multics 18-bit integer
  * in a C long integer (32 bits)
  */

#ifdef ANSI_FUNC
long
get_18_mxbit_integer (MXBITFILE *mxbitfile)
#else
long
get_18_mxbit_integer(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  unsigned char buffer[4];

  get_mxbits(mxbitfile, 18L, buffer);
  return ((long)buffer[0] << 10 | \
                buffer[1] << 2  | \
                buffer[2] >> 6 );
}

 /*
  * Read a Multics 24-bit integer
  * in a C long integer (32 bits)
  */

#ifdef ANSI_FUNC
long
get_24_mxbit_integer (MXBITFILE *mxbitfile)
#else
long
get_24_mxbit_integer(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  unsigned char buffer[4];

  get_mxbits(mxbitfile, 24L, buffer);
  return ((long)buffer[0] << 16 | \
          (long)buffer[1] << 8  | \
                buffer[2] );
}

#ifdef ANSI_FUNC
int
skip_mxbits (MXBITFILE *mxbitfile, long n_bits)
#else
int
skip_mxbits(mxbitfile, n_bits)

MXBITFILE *mxbitfile;
long n_bits;
#endif

{
  long n_bytes_to_skip;
  int n_extra_bits_to_skip;
  int status;

  while (mxbitfile->byte_buffer_pos <= 7 && n_bits > 0)
  {
    getbit(mxbitfile);
    --n_bits;
  }

  n_bytes_to_skip = n_bits / 8;
  n_extra_bits_to_skip = n_bits % 8;

  /*
   * We want to detect an attempt to
   * skip past EOF.  Since fseek will
   * position past EOF without
   * complaining, we fix things up so
   * that the last part of repositioning
   * is done by reading, not seeking.
   */

  if (n_extra_bits_to_skip == 0 && n_bytes_to_skip > 0)
  {
    n_extra_bits_to_skip = 8;
    --n_bytes_to_skip;
  }

  if (mxbitfile->reading_tape_data)
  {

        /*
         * Do not skip over header and
         * trailer data for tape file
         */

    while (n_bytes_to_skip > mxbitfile->n_bytes_left_in_tape_block)
    {
      status = fseekwrapper(
        mxbitfile,
        (long)mxbitfile->n_bytes_left_in_tape_block);
      if (status != 0 && !mxbitfile->temp_file)
      {
        fprintf(stderr,
          "Error reading tape file %s.\n", mxbitfile->path);
        return ( EOF );
      }

      n_bytes_to_skip -= mxbitfile->n_bytes_left_in_tape_block;
      read_tape_record_trailer(mxbitfile);
      read_tape_record_header(mxbitfile);
      if (mxbitfile->end_of_reel_reached)
      {
        return ( EOF );
      }
    }
  }

  status = fseekwrapper(mxbitfile, n_bytes_to_skip);
  if (status != 0 && !mxbitfile->temp_file)
  {
    fprintf(stderr,
      "Error performing seek on file %s.\n", mxbitfile->path);
    return ( EOF );
  }

  mxbitfile->n_bytes_left_in_tape_block -= n_bytes_to_skip;

  while (n_extra_bits_to_skip-- > 0)
  {
    if (getbit(mxbitfile) == EOF)
    {
      return ( EOF );
    }
  }

  return ( 0 );
}

#ifdef ANSI_FUNC
static int
first_bit_next_byte (MXBITFILE *mxbitfile)
#else
static int
first_bit_next_byte(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  register int next_byte;

  if (mxbitfile->reading_tape_data)
  {
    if (( next_byte = next_tape_byte(mxbitfile)) == EOF)
    {
      return ( EOF );
    }
  }
  else
  {
    if (( next_byte = getcwrapper(mxbitfile)) == EOF)
    {
      return ( EOF );
    }
  }

  mxbitfile->byte_buffer = next_byte;
  mxbitfile->byte_buffer_pos = 1;
  return ( mxbitfile->byte_buffer & 0x80 );
}

#ifdef ANSI_FUNC
static int
next_tape_byte (MXBITFILE *mxbitfile)
#else
static int
next_tape_byte(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  int next_byte;

  /*
   * For tape file, check for
   * end-of-reel or end-of-block
   */

  if (mxbitfile->reading_tape_data)
  {
    if (--( mxbitfile->n_bytes_left_in_tape_block ) < 0)
    {
      read_tape_record_trailer(mxbitfile);
      read_tape_record_header(mxbitfile);

          /*
           * Subtract byte we're
           * about to read
           */

      --( mxbitfile->n_bytes_left_in_tape_block );
    }
  }

  next_byte = getcwrapper(mxbitfile);
  if (next_byte == EOF)
  {
    if (ferror(mxbitfile->realfile))
    {
      fprintf(stderr, "Error reading %s.\n", mxbitfile->path);
    }
    else if (!mxbitfile->temp_file
             && !( mxbitfile->reading_tape_data == 0
                   && mxbitfile->tape_mult_sw ))
    {
      fprintf(
        stderr,
        "Premature end of file reading %s.\n",
        mxbitfile->path);
      return ( EOF );
    }
  }

  return ( next_byte );
}

#ifdef ANSI_FUNC
int
eof_reached (MXBITFILE *mxbitfile)
#else
int
eof_reached(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  int next_byte;

  if (mxbitfile->end_of_reel_reached)
  {
    return ( 1 );
  }

  /*
   * For tape file, check for
   * end-of-reel or end-of-block
   */

  if (mxbitfile->reading_tape_data)
  {
    if (mxbitfile->end_of_reel_reached)
    {
      return ( 1 );
    }

    if (mxbitfile->n_bytes_left_in_tape_block == 0)
    {
      read_tape_record_trailer(mxbitfile);
      read_tape_record_header(mxbitfile);
      if (mxbitfile->end_of_reel_reached)
      {
        return ( 1 );
      }
    }
  }

  next_byte = getc(mxbitfile->realfile);
  if (next_byte == EOF)
  {
    if (ferror(mxbitfile->realfile))
    {
      perror("");
      fprintf(stderr, "Error reading %s.\n", mxbitfile->path);
    }

    return ( 1 );
  }

  ungetc(next_byte, mxbitfile->realfile);
  return ( 0 );
}

#ifdef ANSI_FUNC
int
freadwrapper (char *ptr, int size, int nitems, MXBITFILE *mxbitfile)
#else
int
freadwrapper(ptr, size, nitems, mxbitfile)

unsigned char *ptr;
int size;
int nitems;
MXBITFILE *mxbitfile;
#endif

{
  unsigned int n_read;
  int i;
  int n_to_copy;
  int n_saved;

  if (mxbitfile->reading_buffer_to_pos > mxbitfile->input_file_pos)
  {
    n_to_copy = nitems * size;
    n_saved = mxbitfile->reading_buffer_to_pos - mxbitfile->input_file_pos;
    if (n_to_copy > n_saved)
    {
      memcpy(
        ptr,
        mxbitfile->saved_input_ptr + mxbitfile->input_file_pos,
        n_saved);
      mxbitfile->input_file_pos += n_saved;
      freadwrapper(ptr + n_saved, 1, n_to_copy - n_saved, mxbitfile);
    }
    else
    {
      memcpy(
        ptr,
        mxbitfile->saved_input_ptr + mxbitfile->input_file_pos,
        n_to_copy);
    }

    mxbitfile->input_file_pos += nitems * size;
    return ( nitems );
  }

  n_read = fread(ptr, size, nitems, mxbitfile->realfile);
  if (mxbitfile->saved_input_ptr != NULL)
  {
    if (mxbitfile->input_file_pos + n_read * size < INPUT_BUFFER_SIZE)
    {
      memcpy(
        mxbitfile->saved_input_ptr + mxbitfile->input_file_pos,
        ptr,
        n_read * size);
    }
    else
    {

          /*
           * We've overflowed the buffer
           * and won't be able to "rewind"
           * by using the buffer.
           */

      free(mxbitfile->saved_input_ptr);
      mxbitfile->saved_input_ptr = NULL;
    }
  }

  mxbitfile->input_file_pos += n_read * size;
  return ( n_read );
}

#ifdef ANSI_FUNC
int
getcwrapper (MXBITFILE *mxbitfile)
#else
int
getcwrapper(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  int c;

  if (mxbitfile->reading_buffer_to_pos > mxbitfile->input_file_pos)
  {
    c = mxbitfile->saved_input_ptr[mxbitfile->input_file_pos];
    ++mxbitfile->input_file_pos;
    return ( c );
  }

  c = getc(mxbitfile->realfile);

  if (mxbitfile->saved_input_ptr != NULL
      && mxbitfile->input_file_pos < INPUT_BUFFER_SIZE)
  {
    mxbitfile->saved_input_ptr[mxbitfile->input_file_pos] = c;
  }

  ++mxbitfile->input_file_pos;
  return ( c );
}

#ifdef ANSI_FUNC
int
fseekwrapper (MXBITFILE *mxbitfile, int n)
#else
int
fseekwrapper(mxbitfile, n)

MXBITFILE *mxbitfile;
int n;
#endif

{
  int status;
  char skipbuf[512];
  int n_left_to_skip;
  int n_to_read;
  int n_read;

  while (n > 0 &&
         mxbitfile->reading_buffer_to_pos > mxbitfile->input_file_pos)
  {
    --n;
    ++( mxbitfile->input_file_pos );
  }

  for (n_left_to_skip = n; n_left_to_skip >= 0; n_left_to_skip -= 512)
  {
    n_to_read = n_left_to_skip;
    if (n_to_read > 512)
    {
      n_to_read = 512;
    }

    n_read = fread(skipbuf, 1, n_to_read, mxbitfile->realfile);
    if (n_read != n_to_read)
    {
      return ( -1 );
    }
  }

  mxbitfile->input_file_pos += n;
  return ( 0 );
}

#ifdef ANSI_FUNC
int
read_rewind (MXBITFILE *mxbitfile)
#else
int
read_rewind(mxbitfile)

MXBITFILE *mxbitfile;
#endif

{
  int seek_result = -1;

  if (mxbitfile->saved_input_ptr != NULL
      && mxbitfile->input_file_pos < INPUT_BUFFER_SIZE)
  {
    mxbitfile->reading_buffer_to_pos = mxbitfile->input_file_pos;
  }
  else
  {
    seek_result = fseek(mxbitfile->realfile, 0L, SEEK_SET);
    if (seek_result != 0)
    {
      fprintf(stderr, "Error rewinding %s.\n", mxbitfile->path);
    }
  }

  mxbitfile->input_file_pos = 0;
  mxbitfile->byte_buffer = 0;
  mxbitfile->byte_buffer_pos = 8;            /* Forces first getbit call */
                                             /*  to fetch new byte       */
  mxbitfile->n_bytes_left_in_tape_block = 0;
  mxbitfile->n_pad_bytes_in_tape_block = 0;
  mxbitfile->next_tape_block_number = 0;
  mxbitfile->end_of_reel_reached = 0;
  mxbitfile->tape_mult_sw = 0;
  mxbitfile->reading_tape_data = 0;

  return ( seek_result );
}
