                  /********************************\
                  *     Copyright (C) 1987 by      *
                  * Cygnus Cybernetics Corporation *
                  *    and Oxford Systems, Inc.    *
                  \********************************/

/* GETTYPE */

#include "mxload.h"

#include <stdio.h>
#include <string.h>
#ifdef MSDOS
#include <direct.h>
#include <stdlib.h>
#endif
#include "mxbitio.h"
#include "preamble.h"
#include "mxlopts.h"
#include "gettype.h"

#ifdef LINT_ARGS
static  int is_object_seg(struct mxbitiobuf *infile,long bitcount);
#else
static int is_object_seg ();
#endif


/* Look at suffix of segment's entryname and find if it's an archive, 
   mailbox, or message segment.  Else return NULL. */

char * get_type_by_name (preamble_ptr)

struct PREAMBLE *preamble_ptr;

{
     int            i;
     char           *suffix;
     int            last_dot;

     last_dot = -1;
     i = 0;
     while (preamble_ptr -> ename[i] != '\0')
          {
          if (preamble_ptr -> ename[i] == '.')
               last_dot = i;
          ++i;
     }
     if (last_dot == -1)
          return (NULL);
     suffix = preamble_ptr -> ename + last_dot;
     if (strcmp (suffix, ".archive") == 0)
          return ("archive");
     else if (strcmp (suffix, ".mbx") == 0)
          return ("mbx");
     else if (strcmp (suffix, ".ms") == 0)
          return ("ms");
     else return (NULL);
}

/* Read the file contents to determine its type. "ascii", "nonascii" and
   "object" are the only possible types returned.  We actually go ahead here
   and "preconvert" the segment to 8-bit ASCII.  If the segment turns out to
   be ascii the whole segment will have been read.  Re-reading it to then do
   the conversion is expensive.  If the segment turns out not to be ascii,
   chances are that only a few bytes of the segment will have been read before
   we come across a 9-bit-byte with its high-order bit on, so it's very 
   unlikely that we will do a lot of wasted preconversion. */

char *get_type_by_content (contents_file, bitcount,
     preconverted_contents_file)

MXBITFILE           *contents_file;
long                bitcount;
MXBITFILE           *preconverted_contents_file;


{
     static unsigned char buffer[4608]; /* 1024 Multics words */
     unsigned int   n_read;
     long           n_8bit;
     long           total_chars;
     int            leftover_chars;
     char           *type;
     long           n_72_bit_chunks_left;
     unsigned char  *subbuffer; /* For addressing 72 bits within buffer */
     long           buffer_bits;
     int            n_chunks_this_buffer;
     int            first_chunk;
#ifndef WORDALIGN
     static unsigned long     ninth_bit_mask[2] = {0L, 0L};
     unsigned long      *mask_overlay;
#else
     static unsigned char   ninth_bit_mask[8] = {'\0', '\0', '\0', '\0', 
                                                 '\0', '\0', '\0', '\0'};
     int            byteno;
#endif

     rewind_mxbit_file (contents_file, "rt");
     rewind_mxbit_file (preconverted_contents_file, "wt");

     /* Initialize mask for high-order bit checking.  Done at run time to
        work on byte-reversed and non-byte-reversed machines */

#ifndef WORDALIGN
     if (ninth_bit_mask[0] == 0L)
#else
     if (ninth_bit_mask[0] == '\0')
#endif
       {
           subbuffer = (unsigned char *) ninth_bit_mask;
           subbuffer[0] = 0x80;
           subbuffer[1] = 0x40;
           subbuffer[2] = 0x20;
           subbuffer[3] = 0x10;
           subbuffer[4] = 0x08;
           subbuffer[5] = 0x04;
           subbuffer[6] = 0x02;
           subbuffer[7] = 0x01;
       }


     n_8bit = 0;
     total_chars = bitcount / 9L;
     n_72_bit_chunks_left = bitcount / 72L;
     leftover_chars = total_chars - n_72_bit_chunks_left * 8L;
     first_chunk = 1;
     while (n_72_bit_chunks_left > 0)
          {
          /* Try to read the biggest possible chunk, limited by remaining
             space in tape block if reading from tape, EXCEPT that the first
             chunk should be small in case the segment quickly turns out to
             be non-ascii. */
          if (first_chunk)
               {
               buffer_bits = 288L; /* First chunk is 32 Multics bytes */
               first_chunk = 0;
          }
          else buffer_bits = sizeof (buffer) * 8L;
          if (contents_file -> n_bytes_left_in_tape_block > 0 &&
               buffer_bits > contents_file -> n_bytes_left_in_tape_block * 8)
               buffer_bits = contents_file -> n_bytes_left_in_tape_block * 8L;
          if (n_72_bit_chunks_left * 72 <= buffer_bits)
               buffer_bits = n_72_bit_chunks_left * 72;
          n_read = get_mxbits (contents_file, buffer_bits, buffer);

          n_chunks_this_buffer = n_read / 72;
          n_72_bit_chunks_left -= n_chunks_this_buffer;
          subbuffer = buffer;
          while (n_chunks_this_buffer > 0)
               {
#ifndef WORDALIGN
                 mask_overlay = (unsigned long *) subbuffer;
                 if ((mask_overlay[0] & ninth_bit_mask[0]) != 0L)
                     goto NONASCII;
                 if ((mask_overlay[1] & ninth_bit_mask[1]) != 0L)
                     goto NONASCII;
#else
                 for (byteno = 0 ; byteno < 8 ; byteno++)
                     {
                     if ((subbuffer[byteno] & ninth_bit_mask[byteno]) != '\0')
                          goto NONASCII;
                 }
#endif
               putc (subbuffer[0] << 1 | subbuffer[1] >> 7,
                    preconverted_contents_file -> realfile);   /* Byte 0 */
               putc (subbuffer[1] << 2 | subbuffer[2] >> 6,
                    preconverted_contents_file -> realfile);   /* Byte 1 */
               putc (subbuffer[2] << 3 | subbuffer[3] >> 5,
                    preconverted_contents_file -> realfile);   /* Byte 2 */
               putc (subbuffer[3] << 4 | subbuffer[4] >> 4,
                    preconverted_contents_file -> realfile);   /* Byte 3 */
               putc (subbuffer[4] << 5 | subbuffer[5] >> 3,
                    preconverted_contents_file -> realfile);   /* Byte 4 */
               putc (subbuffer[5] << 6 | subbuffer[6] >> 2,
                    preconverted_contents_file -> realfile);   /* Byte 5 */
               putc (subbuffer[6] << 7 | subbuffer[7] >> 1,
                    preconverted_contents_file -> realfile);   /* Byte 6 */
               putc (subbuffer[8],
                    preconverted_contents_file -> realfile);   /* Byte 7 */
               subbuffer += 9;
               --n_chunks_this_buffer;
          }
     }

     /* And read to the end of the file using bit i/o routines to pick 
        remaining bytes apart */
     while (leftover_chars-- > 0)
          {
          get_mxbits (contents_file, 9L, buffer);
          if (buffer[0] & 0x80)
               goto NONASCII;
          else if (buffer[0] & 0x40)
               ++n_8bit;
          putc (buffer[0] << 1 | buffer[1] >> 7,
               preconverted_contents_file -> realfile);
     }
     if (n_8bit * 10 > total_chars) /* If file is >10% obviously non-ascii */
          {
          if (is_object_seg (contents_file, bitcount))
               type = "object";
          else type = "nonascii";
     }
     else type = "ascii";
     return (type);

NONASCII:
     if (is_object_seg (contents_file, bitcount))
       type = "object";
     else type = "nonascii";
     return (type);
}

/* Determine if file is Multics object segment */

static int is_object_seg (infile, bitcount)

MXBITFILE           *infile;
long                bitcount;

{
     long           object_map_offset;
     char           obj_map_id[9];
     
     if (bitcount % 36L != 0 || bitcount < 36L)
          return (0);            /* Actually suspect they are all 0 mod 72 */
     /* Position to last word of segment */
     mxbit_pos (infile, (long) bitcount - 36L);
     /* Read first half of last word to get object map offset */
     object_map_offset = get_18_mxbit_integer (infile);
     /* Make sure the offset is reasonable */
     if (object_map_offset * 36L > bitcount)
          return (0);
     /* Position to object map */
     mxbit_pos (infile, (long) object_map_offset * 36L);
     /* First word should contain a 2 (the version number) */
     if (get_18_mxbit_integer (infile) != 0 || 
          get_18_mxbit_integer (infile) != 2)
          return (0);
     /* Next 2 words should contain "obj_map " */
     get_mxstring (infile, obj_map_id, 8);
     if (strcmp (obj_map_id, "obj_map") == 0)
          return (1);
     else return (0);     
}
