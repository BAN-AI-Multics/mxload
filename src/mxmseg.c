 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

 /* MXMSEG */

 /*
  * Process Multics message segments,
  * including mailboxes.
  * See mseg_hdr.incl.pl1,
  * mseg_message.incl.pl1,
  * mail_format.incl.pl1.
  */

#include "mxload.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "copybits.h"
#include "dirsep.h"
#include "mapprint.h"
#include "mxbitio.h"
#include "mxlopts.h"
#include "mxmseg.h"
#include "optptr.h"
#include "preamble.h"
#include "tempfile.h"
#include "timestr.h"

static int msegintr();
static int get_message();
static int write_as_9bit();

#ifdef ANSI_FUNC
int
mxmseg (MXBITFILE *mseg_contents_file, struct PREAMBLE *mseg_preamble_ptr,
                struct MXLOPTS *retrieval_opt_ptr,
                struct BRANCH_PREAMBLE *mseg_branch_preamble_ptr, int repack,
                char *seg_type)
#else
int
mxmseg(mseg_contents_file, mseg_preamble_ptr, retrieval_opt_ptr,
           mseg_branch_preamble_ptr, repack, seg_type)

MXBITFILE *mseg_contents_file;
struct PREAMBLE *mseg_preamble_ptr;
struct MXLOPTS *retrieval_opt_ptr;
struct BRANCH_PREAMBLE *mseg_branch_preamble_ptr;
int repack;
char *seg_type;
#endif

{
  MXBITFILE *message_contents_file = 0;
  MXBITFILE *repacked_mbx_file = 0;
  struct PREAMBLE message_preamble;
  struct BRANCH_PREAMBLE message_branch_preamble;
  struct MXLOPTS message_retrieval_opt;
  struct PREAMBLE repacked_mbx_preamble;
  struct BRANCH_PREAMBLE repacked_mbx_branch_preamble;
  int gm_code;
  int message_number;
  int is_mbx;
  char *temp_path;

  message_contents_file = get_temp_file("wt", "message contents");
  if (message_contents_file == NULL)
  {
    return ( -1 );
  }

  if (repack)
  {
    repacked_mbx_file = get_temp_file("wt", "repacked mailbox");
    if (repacked_mbx_file == NULL)
    {
      return ( -1 );
    }
  }

  rewind_mxbit_file(mseg_contents_file, "rt");

  if (repack)
  {
    memcpy(
      &repacked_mbx_preamble,
      mseg_preamble_ptr,
      sizeof ( repacked_mbx_preamble ));
    memcpy(
      &repacked_mbx_branch_preamble,
      mseg_branch_preamble_ptr,
      sizeof ( repacked_mbx_branch_preamble ));
    repacked_mbx_preamble.bitcnt = 0;
  }
  else
  {
    memcpy(
      &message_preamble,
      mseg_preamble_ptr,
      sizeof ( message_preamble ));
    if (message_preamble.dname[0] != '\0')
    {
      strcat(message_preamble.dname, ">");
    }

    strcat(message_preamble.dname, mseg_preamble_ptr->ename);
    memcpy(
      &message_branch_preamble,
      mseg_branch_preamble_ptr,
      sizeof ( message_branch_preamble ));
    message_branch_preamble.naddnames = 0;
  }

  is_mbx = ( strcmp(seg_type, "mbx") == 0 );

  if (repack)
  {
    memcpy(
      &message_retrieval_opt,
      retrieval_opt_ptr,
      sizeof ( message_retrieval_opt ));
    message_retrieval_opt.force_convert = "8bit";
  }
  else
  {

        /*
         * Make the mseg look like a
         * directory within a subtree.
         */

    if (retrieval_opt_ptr != NULL)
    {
      memcpy(
        &message_retrieval_opt,
        retrieval_opt_ptr,
        sizeof ( message_retrieval_opt ));
    }

    message_retrieval_opt.path_type = "subtree";
  }

  message_number = 0;
  while (( gm_code = get_message(
             mseg_contents_file,
             message_contents_file,
             &message_preamble,
             &message_branch_preamble,
             ++message_number,
             is_mbx,
             repack))
         == 0)
  {
    rewind_mxbit_file(message_contents_file, "rt");
    if (repack)
    {
      if (copy_bits(
            message_contents_file,
            repacked_mbx_file,
            message_preamble.bitcnt)
          != message_preamble.bitcnt)
      {
        fprintf(stderr, "Error writing message #%d\n", message_number);
      }

      repacked_mbx_preamble.bitcnt += message_preamble.bitcnt;
    }
    else
    {
      process_seg(
        message_contents_file,
        &message_branch_preamble,
        &message_preamble,
        &message_retrieval_opt,
        0);
    }

    rewind_mxbit_file(message_contents_file, "w");
  }

  if (gm_code != EOF)
  {
    fprintf(stderr, "Message segment format error encountered in mailbox");
    if (mseg_preamble_ptr->ename[0] != '\0')
    {
      fprintf(
        stderr,
        " %s>%s",
        mseg_preamble_ptr->dname,
        mseg_preamble_ptr->ename);
    }

    fprintf(stderr, ".\n");
  }

  if (repack)
  {
    repacked_mbx_preamble.adjusted_bitcnt = repacked_mbx_preamble.bitcnt;
    repacked_mbx_preamble.length = repacked_mbx_preamble.bitcnt / 36L;
    rewind_mxbit_file(repacked_mbx_file, "rt");
    process_seg(
      repacked_mbx_file,
      &repacked_mbx_branch_preamble,
      &repacked_mbx_preamble,
      &message_retrieval_opt,
      0);
  }

  release_temp_file(message_contents_file, "message contents");
  if (repack)
  {
    release_temp_file(repacked_mbx_file, "repacked mailbox");
  }

  if (gm_code != EOF)
  {
    return ( -1 );
  }
  else
  {
    return ( 0 );
  }
}

/*
 * The calls to copy_bits in this routine turn out to be very expensive, but
 * there's not much that can be done.  Most of the blocks of data in message
 * segments are an odd number of words long (31 is the most frequent).  This
 * means that half the time the data cannot beproperly aligned for efficient
 * output.  Fixing write_as_9bit to pad its output to an even number of
 * Multics words makes sure that the data is properly aligned as often as
 * possible (speeding things up by perhaps 30% overall), but further
 * improvements would be difficult or impossible.
 */

#ifdef ANSI_FUNC
static int
get_message (MXBITFILE *mseg_contents_file,
                MXBITFILE *message_contents_file,
                struct PREAMBLE *message_preamble_ptr,
                struct BRANCH_PREAMBLE *message_branch_preamble_ptr,
                int message_number, int is_mbx, int repacking)
#else
static int
get_message(mseg_contents_file, message_contents_file,
                       message_preamble_ptr, message_branch_preamble_ptr,
                       message_number, is_mbx, repacking)

MXBITFILE *mseg_contents_file;
MXBITFILE *message_contents_file;
struct PREAMBLE *message_preamble_ptr;
struct BRANCH_PREAMBLE *message_branch_preamble_ptr;
int message_number;
int is_mbx;
int repacking;
#endif

{
  MXBITFILE *mbx_header_temp_file;
  static long next_message_offset;
  static int block_size;
  static int mseg_version;
  long bits_copied;
  int desc_size;
  long block_offset;
  long next_block_offset;
  long block_bit_length;
  long first_block_offset;
  long cur_message_length;
  long ms_header_lines_length;
  char header_lines[1000];
  long mail_text_len;
  long mbx_header_n_read;
  char print_buffer[1024];
  char ms_sender[128];
  long ms_length;
  unsigned char ms_ring_level;
  long ms_message_id[4];      /* 18 bits stored in each long */
  unsigned long ms_time;
  long ms_authorization[4];   /* 18 bits stored in each long */
  long ms_access_class[4];    /* 18 bits stored in each long */
  unsigned long long_pair[2];
  char access_class_string[128];

  /*
   * If this is first message,
   * pick up its offset from mseg
   * header, else take offset remembered
   * from previous call
   */

  if (message_number == 1)
  {
    mxbit_pos(mseg_contents_file, (long)36L * 15L + 18L);
    mseg_version = get_18_mxbit_integer(mseg_contents_file);
    mxbit_pos(mseg_contents_file, (long)36L * 8L);

    /* mseg_segment.first_message */

    first_block_offset = get_18_mxbit_integer(mseg_contents_file);
    mxbit_pos(mseg_contents_file, (long)36L * 12L);
    skip_mxbits(mseg_contents_file, 18L);

    /* mseg_segment.n_messages */

    skip_mxbits(mseg_contents_file, 36L);

    /* mseg_segment.block_size */

    block_size = (int)get_18_mxbit_integer(mseg_contents_file);
  }
  else
  {
    first_block_offset = next_message_offset;
  }

  if (is_mbx && repacking && message_number > 1)
  {
    strcpy(header_lines, "\f\n\n");
  }
  else
  {
    header_lines[0] = '\0';
  }

  if (first_block_offset == 0)
  {
    return ( EOF );
  }

  block_offset = first_block_offset;

  if (mseg_version == 5)
  {
    desc_size = 22;
  }
  else
  {
    desc_size = 18;
  }

  /*
   * Position to
   * first block
   */

  mxbit_pos(mseg_contents_file, (long)36L * block_offset);

  /*
   * Read message descriptor.  Descriptor is 22 words long (=792 bits),
   * so we find it by going to 22 words before the end of the block, whose
   * size we know from mseg_segment.header.block_size.  (Normally 32
   * words).  (Version 4 message segments have 18 word descriptors)
   */

  skip_mxbits(mseg_contents_file, (long)(( block_size - desc_size ) * 36 ));

  /* message_descriptor.sentinel */

  skip_mxbits(mseg_contents_file, 36L);

  /* message_descriptor.next_message */

  next_message_offset = get_18_mxbit_integer(mseg_contents_file);

  /* message_descriptor.prev_message */

  skip_mxbits(mseg_contents_file, 18L);
  get_mxbits(mseg_contents_file, 8L, &ms_ring_level);
  ms_ring_level >>= 5; /* Strip off message_descriptor.pad */
  sprintf(print_buffer, "MS-Ring-Level: %d\n", ms_ring_level);
  strcat(header_lines, print_buffer);

  skip_mxbits(
    mseg_contents_file,
    18L + 10L);  /* message_descriptor.chain(?) to message_descriptor.pad? */

  ms_message_id[0] = get_18_mxbit_integer(mseg_contents_file);
  ms_message_id[1] = get_18_mxbit_integer(mseg_contents_file);
  ms_message_id[2] = get_18_mxbit_integer(mseg_contents_file);
  ms_message_id[3] = get_18_mxbit_integer(mseg_contents_file);
  sprintf(
    print_buffer,
    "MS-Message-ID: %.6lo%.6lo%.6lo\n",
    ms_message_id[1],
    ms_message_id[2],
    ms_message_id[3]);
  strcat(header_lines, print_buffer);

  /*
   * Backup over message_descriptor.ms_id
   * and read it again as a time.
   */

  /*
   * First back to beginning
   * of descriptor
   */

  mxbit_pos(mseg_contents_file, (long)36L * block_offset);
  skip_mxbits(mseg_contents_file, (long)(( block_size - desc_size ) * 36 ));

  /*
   * And skip up to
   * message_descriptor.ms_id
   */

  skip_mxbits(mseg_contents_file, (long)3L * 36L);

  /*
   * Message_descriptor.ms_id is a 72-bit clock value.
   * We pick off the 36 bits for fstime and convert
   */

  skip_mxbits(mseg_contents_file, 20L);
  get_36_mxbit_integer(mseg_contents_file, long_pair);
  ms_time = cvmxtime(long_pair);
  skip_mxbits(mseg_contents_file, 16L);
  sprintf(print_buffer, "MS-Time: %s\n", timestr(&ms_time));
  strcat(header_lines, print_buffer);

  ms_length = get_24_mxbit_integer(mseg_contents_file);
  skip_mxbits(mseg_contents_file, 12L);  /* Skip pad3 */
  sprintf(print_buffer, "MS-Length: %ld\n", ms_length);
  strcat(header_lines, print_buffer);

  get_mxstring(mseg_contents_file, ms_sender, 32);
  sprintf(print_buffer, "MS-Sender: %s\n", ms_sender);
  strcat(header_lines, print_buffer);

  ms_authorization[0] = get_18_mxbit_integer(mseg_contents_file);
  ms_authorization[1] = get_18_mxbit_integer(mseg_contents_file);
  ms_authorization[2] = get_18_mxbit_integer(mseg_contents_file);
  ms_authorization[3] = get_18_mxbit_integer(mseg_contents_file);

  format_access_class(ms_authorization, access_class_string);
  sprintf(print_buffer, "MS-Authorization: %s\n", access_class_string);
  strcat(header_lines, print_buffer);

  ms_access_class[0] = get_18_mxbit_integer(mseg_contents_file);
  ms_access_class[1] = get_18_mxbit_integer(mseg_contents_file);
  ms_access_class[2] = get_18_mxbit_integer(mseg_contents_file);
  ms_access_class[3] = get_18_mxbit_integer(mseg_contents_file);

  format_access_class(ms_access_class, access_class_string);
  sprintf(print_buffer, "MS-Access: %s\n", access_class_string);
  strcat(header_lines, print_buffer);
  ms_header_lines_length =
    write_as_9bit(message_contents_file, header_lines);

  /*
   * And position back to header
   * of first block
   */

  mxbit_pos(mseg_contents_file, (long)36L * block_offset);

  mbx_header_temp_file = get_temp_file("wt", "mbx header");
  if (mbx_header_temp_file == NULL)
  {
    return ( 1 );
  }

  mbx_header_n_read = 0;
  cur_message_length = 0L;
  while (block_offset != 0)
  {

    /* message_block_header.next_block */

    next_block_offset = get_18_mxbit_integer(mseg_contents_file);

    /* message_block_header.data_lth */

    block_bit_length = get_18_mxbit_integer(mseg_contents_file);

        /*
         * Mask off
         * message_block_header.descriptor_present
         */

    block_bit_length &= 0x0001FFFFL;

        /*
         * For mailboxes, the 468-bit mail format
         * header gets copied into temp file
         */

    if (is_mbx && mbx_header_n_read < 468L)
    {
      if (468L - mbx_header_n_read >= block_bit_length)

          /*
           * Copy part of mail
           * format header
           */

      {

        /* first_message_block.data_space */

        if (copy_bits(
              mseg_contents_file,
              mbx_header_temp_file,
              block_bit_length)
            != block_bit_length)
        {
          return ( 1 );
        }

        mbx_header_n_read += block_bit_length;
      }
      else

          /*
           * Copy last of mail
           * format header
           */

      {
        block_bit_length -= ( 468L - mbx_header_n_read );

        /* first_message_block.data_space */

        if (copy_bits(
              mseg_contents_file,
              mbx_header_temp_file,
              468L - mbx_header_n_read)
            != 468L - mbx_header_n_read)
        {
          return ( 1 );
        }

        mbx_header_n_read = 468L;

        /* first_message_block.data_space */

        if (copy_bits(
              mseg_contents_file,
              message_contents_file,
              block_bit_length)
            != block_bit_length)
        {
          return ( 1 );
        }

        cur_message_length += block_bit_length;
      }
    }
    else
    {

      /* first_message_block.data_space */

      bits_copied = copy_bits(
        mseg_contents_file,
        message_contents_file,
        block_bit_length);
      if (bits_copied != block_bit_length)
      {

       /*
        * If this is the last block of mbx, ignore missing info
        * at the end. Else it's an error. Used to compensate for
        * mysterious missing byte at end of some reloaded mbxs.
        */

        if (next_block_offset != 0 || next_message_offset != 0)
        {
          return ( 1 );
        }
      }

      cur_message_length += bits_copied;
    }

    block_offset = next_block_offset;

        /*
         * Position to
         * next block
         */

    mxbit_pos(mseg_contents_file, (long)36L * block_offset);
  }
  copy_bits(NULL, NULL, 0L);

  sprintf(message_preamble_ptr->ename, "%.4d", message_number);
  strcat(message_preamble_ptr->ename, ".MSG");

  if (is_mbx)
  {
    rewind_mxbit_file(mbx_header_temp_file, "rt");

        /*
         * Skip to
         * mail_format.text_len
         */

    mxbit_pos(mbx_header_temp_file, (long)36L * 10L);
    skip_mxbits(mbx_header_temp_file, 12L);

    /* mail_format.text_len */

    mail_text_len = get_24_mxbit_integer(mbx_header_temp_file);
    cur_message_length = mail_text_len * 9L + ms_header_lines_length * 9L;
  }
  else
  {
    cur_message_length += ms_header_lines_length * 9L;
  }

  message_preamble_ptr->bitcnt = cur_message_length;
  message_preamble_ptr->adjusted_bitcnt = cur_message_length;
  message_preamble_ptr->maximum_bitcnt = cur_message_length;
  message_preamble_ptr->length = ( cur_message_length + 35L ) / 36L;
  message_branch_preamble_ptr->dtu = ms_time;
  message_branch_preamble_ptr->dtm = ms_time;

  release_temp_file(mbx_header_temp_file, "mbx header");
  return ( 0 );
}

#ifdef ANSI_FUNC
static int
write_as_9bit (MXBITFILE *message_contents_file, char *line)
#else
static int
write_as_9bit(message_contents_file, line)

MXBITFILE *message_contents_file;
char *line;
#endif

{
  int i;
  unsigned char buffer[10];
  int n_72_bit_chunks;
  int needed_padding;

  /*
   * Write data as if it were 9-bit ASCII.  In order to leave things
   *    nicely aligned, we pad out the last header line with enough
   *    blanks to leave us (Multics) double-word aligned
   */

  if (line[strlen(line) - 1] == '\n')
  {
    line[strlen(line) - 1] = '\0';
  }

  needed_padding = 8 - strlen(line) % 8;
  while (needed_padding-- > 1)
  {
    strcat(line, " ");
  }
  strcat(line, "\n");

  n_72_bit_chunks = strlen(line) / 8;
  i = 0;
  memset(buffer, 0, sizeof ( buffer ));
  while (n_72_bit_chunks-- > 0)
  {
    buffer[0] = line[i + 0] >> 1;
    buffer[1] = line[i + 0] << 7 | line[i + 1] >> 2;
    buffer[2] = line[i + 1] << 6 | line[i + 2] >> 3;
    buffer[3] = line[i + 2] << 5 | line[i + 3] >> 4;
    buffer[4] = line[i + 3] << 4 | line[i + 4] >> 5;
    buffer[5] = line[i + 4] << 3 | line[i + 5] >> 6;
    buffer[6] = line[i + 5] << 2 | line[i + 6] >> 7;
    buffer[7] = line[i + 6] << 1;
    buffer[8] = line[i + 7];
    put_mxbits(message_contents_file, 72, buffer);
    i += 8;
  }

  return ( strlen(line));
}
