 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

 /*
  * Routines to pick through preamble
  * stored in a temporrary file. Not
  * as pretty as simply using pointers
  * and big structures in Multics. We
  * could read the temporary file into
  * a workspace and use pointers and
  * structures, except that the elements
  * of the structures would not be aligned
  * at all right, being from a 36-bit world.
  * So it's no worse doing it via I/O and
  * eliminates the need to guess a
  * maximum size for the preamble.
  */

#include "mxload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mxbitio.h"
#include "preamble.h"
#include "timestr.h"

 /* 
  * Read in pathname, seg type,
  * bitcount from structure described
  * in backup_preamble_header.incl.pl1
  */

#ifdef ANSI_FUNC
int 
get_preamble_min (MXBITFILE *preamble_file, struct PREAMBLE *preamble_ptr)
#else
int
get_preamble_min(preamble_file, preamble_ptr)

MXBITFILE *preamble_file;
struct PREAMBLE *preamble_ptr;
#endif

{
  unsigned long long_pair[2];
  int overflow_long;
  int dlen;
  int elen;

  rewind_mxbit_file(preamble_file, "rt");

  /* 
   * Top half of dlen,
   * a fixed bin (17)
   */

  skip_mxbits(preamble_file, 18L);
  dlen = get_18_mxbit_integer(preamble_file);

  get_mxstring(preamble_file, preamble_ptr->dname, dlen);
  preamble_ptr->dname[dlen] = '\0';
  skip_mxbits(preamble_file, (long)(( 168 - dlen ) * 9 ));

  /* 
   * Top half of elen,
   * a fixed bin (17)
   */

  skip_mxbits(preamble_file, 18L);
  elen = get_18_mxbit_integer(preamble_file);

  get_mxstring(preamble_file, preamble_ptr->ename, elen);
  preamble_ptr->ename[elen] = '\0';
  skip_mxbits(preamble_file, (long)(( 32 - elen ) * 9 ));

  overflow_long = get_36_mxbit_integer(preamble_file, long_pair);
  preamble_ptr->bitcnt = long_pair[1];

  /* 
   * Top half of record_type,
   * a fixed bin (17)
   */

  skip_mxbits(preamble_file, 18L);
  preamble_ptr->record_type = get_18_mxbit_integer(preamble_file);

  return ( 0 );
}

 /*
  * Read in everything besides
  * pathname, seg type, & bitcount
  * from structure described in
  * backup_preamble_header.incl.pl1
  */

#ifdef ANSI_FUNC
int 
get_branch_preamble (MXBITFILE *preamble_file, struct BRANCH_PREAMBLE *branch_preamble_ptr, struct PREAMBLE *preamble_ptr)
#else
int
get_branch_preamble(preamble_file, branch_preamble_ptr, preamble_ptr)

MXBITFILE *preamble_file;
struct BRANCH_PREAMBLE *branch_preamble_ptr;
struct PREAMBLE *preamble_ptr;
#endif

{
  unsigned long long_pair[2];
  int i;
  unsigned int namerp; /* Names offset */
  unsigned int bp;     /* Offset of branches */
  unsigned int bc;     /* Branch count */
  unsigned int lc;     /* Link count */
  unsigned int aclp;   /* Offset of ACL */
  unsigned int aclc;   /* ACL count */
  struct ACL *acl_ptr;
  unsigned char switch_in;

  rewind_mxbit_file(preamble_file, "rt");

       /*
        * Skip over h.dlen through
        * h.record_type
        */

  skip_mxbits(preamble_file, 216L * 9L);

  /*
   * Next field is a 72-bit clock value.
   * We pick off the 36 bits for
   * fstime and convert
   */

  skip_mxbits(preamble_file, 20L);
  get_36_mxbit_integer(preamble_file, long_pair);
  skip_mxbits(preamble_file, 16L);
  branch_preamble_ptr->dtd = cvmxtime(long_pair);

  skip_mxbits(preamble_file, 32L * 9L);  /* Skip over dumper ID */

  bp = (int)get_18_mxbit_integer(preamble_file);
  skip_mxbits(preamble_file, 18L);

  skip_mxbits(preamble_file, 18L);
  bc = (int)get_18_mxbit_integer(preamble_file);

  skip_mxbits(preamble_file, 36L);  /* Skip link pointer */

  skip_mxbits(preamble_file, 18L);
  lc = (int)get_18_mxbit_integer(preamble_file);

  aclp = (int)get_18_mxbit_integer(preamble_file);
  skip_mxbits(preamble_file, 18L);

  skip_mxbits(preamble_file, 18L);
  aclc = (int)get_18_mxbit_integer(preamble_file);

  skip_mxbits(preamble_file, 72L);  /* Skip h.actind and h.actime */

  if (preamble_ptr->record_type == DIRECTORY_RECORD)
  {

        /* 
         * Skip h.quota
         * thru h.pad1
         */

    skip_mxbits(preamble_file, 32L * 9L);
    branch_preamble_ptr->author[0] = '\0';
  }
  else
  {
    get_mxstring(preamble_file, branch_preamble_ptr->author, 32);
  }

  skip_mxbits(preamble_file, 36L);  /* Skip h.max_length */
  branch_preamble_ptr->switches = 0;
  get_mxbits(preamble_file, 1L, &switch_in);
  if (switch_in & 0x80)
  {
    branch_preamble_ptr->switches |= SAFETY_SW_MASK;
  }

  get_mxbits(preamble_file, 1L, &switch_in);
  if (switch_in & 0x80)
  {
    branch_preamble_ptr->switches |= ENTRYBD_SW_MASK;
  }

  get_mxbits(preamble_file, 1L, &switch_in);
  if (switch_in & 0x80)
  {
    branch_preamble_ptr->switches |= SOOS_SW_MASK;
  }

  get_mxbits(preamble_file, 1L, &switch_in);
  if (switch_in & 0x80)
  {
    branch_preamble_ptr->switches |= AUDIT_SW_MASK;
  }

  get_mxbits(preamble_file, 1L, &switch_in);
  if (switch_in & 0x80)
  {
    branch_preamble_ptr->switches |= MULTICLASS_SW_MASK;
  }

  skip_mxbits(preamble_file, 2L);  /* Skip h.pad2 */
  get_mxbits(preamble_file, 1L, &switch_in);
  if (switch_in & 0x80)
  {
    branch_preamble_ptr->switches |= MDIR_SW_MASK;
  }

  skip_mxbits(preamble_file, 28L);  /* Skip h.tpd through h.entrypt_bound*/

  branch_preamble_ptr->access_class[0] =
    get_18_mxbit_integer(preamble_file);
  branch_preamble_ptr->access_class[1] =
    get_18_mxbit_integer(preamble_file);
  branch_preamble_ptr->access_class[2] =
    get_18_mxbit_integer(preamble_file);
  branch_preamble_ptr->access_class[3] =
    get_18_mxbit_integer(preamble_file);

  /* 
   * Skip h.spad thru
   * h.dir_inaclc
   */

  skip_mxbits(preamble_file, 36L * 36L);
  get_mxstring(preamble_file, branch_preamble_ptr->bitcount_author, 32);

  /* 
   * Read branch structure from
   * backup_dir_list.incl.pl1
   */

  mxbit_pos(preamble_file, (long)( bp * 36L ));

  /* 
   * Skip br.vtoc_error
   * and pad1
   */

  skip_mxbits(preamble_file, 2L);

  branch_preamble_ptr->uid[0] = get_18_mxbit_integer(preamble_file);
  branch_preamble_ptr->uid[1] = get_18_mxbit_integer(preamble_file);
  skip_mxbits(preamble_file, 34L);  /* Skip rest of br.uid */
  skip_mxbits(preamble_file, 20L);  /* Skip br.pad2 */
  get_36_mxbit_integer(preamble_file, long_pair);
  branch_preamble_ptr->dtu = cvmxtime(long_pair);
  skip_mxbits(preamble_file, 16L);  /* Skip end of br.dtu */
  skip_mxbits(preamble_file, 20L);  /* Skip br.pad3 */
  get_36_mxbit_integer(preamble_file, long_pair);
  branch_preamble_ptr->dtm = cvmxtime(long_pair);
  skip_mxbits(preamble_file, 16L);  /* Skip end of br.dtm */
  skip_mxbits(preamble_file, 20L);  /* Skip br.pad3 */
  get_36_mxbit_integer(preamble_file, long_pair);
  branch_preamble_ptr->dtd = cvmxtime(long_pair);
  skip_mxbits(preamble_file, 16L);  /* Skip end of br.dtd */
  skip_mxbits(preamble_file, 20L);  /* Skip br.pad4 */
  get_36_mxbit_integer(preamble_file, long_pair);
  branch_preamble_ptr->dtbm = cvmxtime(long_pair);
  skip_mxbits(preamble_file, 16L);  /* Skip end of br.dtbm */

  /* 
   * Skip over br.access_class
   * thru br.nomore
   */

  skip_mxbits(preamble_file, 72L + 36L + 18L);
  branch_preamble_ptr->cur_length = get_9_mxbit_integer(preamble_file);
  
  /*
   * Skip over br.ml thru
   * br.pad7
   */

  skip_mxbits(preamble_file, 9L + 2L * 36L);
  for (i = 0; i < 3; ++i)
  {
    get_mxbits(preamble_file, 6L, branch_preamble_ptr->rings + i);
    branch_preamble_ptr->rings[i] >>= 2;
  }

  skip_mxbits(preamble_file, 18L + 18L);  /* Skip over br.pad8 & br.pad9 */
  namerp = (int)get_18_mxbit_integer(preamble_file);
  skip_mxbits(preamble_file, 18L);  /* Skip over br.ix */
  branch_preamble_ptr->naddnames = (int)get_18_mxbit_integer(preamble_file);
  branch_preamble_ptr->naddnames &= 0x7FFF; /* Mask off br.dump_me */
  --( branch_preamble_ptr->naddnames );       /* Don't count primary name */

  /*
   * Read name structure from
   * backup_dir_list.incl.pl1
   */

  if (branch_preamble_ptr->naddnames > 0)
  {
    branch_preamble_ptr->addnames
      = malloc(33 * branch_preamble_ptr->naddnames);
    mxbit_pos(preamble_file, (long)( namerp * 36L ));

    skip_mxbits(preamble_file, 36L + 32L * 9L);  /* Skip primary name */

    for (i = 0; i < branch_preamble_ptr->naddnames; ++i)
    {
      skip_mxbits(preamble_file, 36L);  /* Skip name.size */
      get_mxstring(
        preamble_file,
        branch_preamble_ptr->addnames + 33 * i,
        32);
    }
  }
  else
  {
    branch_preamble_ptr->addnames = NULL;
  }

  /*
   * Read ACL array of structures
   * general_extended_acl from
   * acl_structures.incl.pl1
   */

  branch_preamble_ptr->acl = (struct ACL *)malloc(
    aclc * sizeof ( struct ACL ));
  branch_preamble_ptr->nacl = aclc;
  mxbit_pos(preamble_file, (long)( aclp * 36L ));
  for (i = 0; i < aclc; ++i)
  {
    acl_ptr = branch_preamble_ptr->acl + i;
    get_mxstring(preamble_file, acl_ptr->access_name, 32);
    get_mxbits(preamble_file, 16L, acl_ptr->mode);
    if (preamble_ptr->record_type == SEGMENT_RECORD)
    {
      skip_mxbits(preamble_file, 20L);  /* Skip rest of mode */
      get_mxbits(preamble_file, 16L, acl_ptr->ext_mode);
      skip_mxbits(
        preamble_file,
        20L + 36L);            /* Skip rest of extended_mode + status_code */
    }
    else
    {
      skip_mxbits(
        preamble_file,
        20L + 36L);            /* Skip rest of mode + status_code */
    }
  }

  return ( 0 );
}

/*
 * Read in link information from
 * structure described in
 * backup_preamble_header.incl.pl1
 * for record type 3
 */

#ifdef ANSI_FUNC
int 
get_dirlist_preamble (MXBITFILE *preamble_file, 
                struct DIRLIST_PREAMBLE *dirlist_preamble_ptr, 
                struct PREAMBLE *preamble_ptr)
#else
int
get_dirlist_preamble(preamble_file, dirlist_preamble_ptr, preamble_ptr)

MXBITFILE *preamble_file;
struct DIRLIST_PREAMBLE *dirlist_preamble_ptr;
struct PREAMBLE *preamble_ptr;
#endif

{
  int i;
  int j;
  unsigned long long_pair[2];
  unsigned int lp;   /* Offset of links */
  unsigned long dtd; /* Date/time dumped */
  struct LINK *link_ptr;
  unsigned int *save_link_name_addrs;
  unsigned int *save_link_target_addrs;

  rewind_mxbit_file(preamble_file, "rt");

  /* 
   * Skip over h.dlen through
   * h.record_type
   */

  skip_mxbits(preamble_file, 216L * 9L);

  /* 
   * Next field is a 72-bit clock value.
   * We pick off the 36 bits for
   * fstime and convert
   */

  skip_mxbits(preamble_file, 20L);
  get_36_mxbit_integer(preamble_file, long_pair);
  skip_mxbits(preamble_file, 16L);
  dtd = cvmxtime(long_pair);

  /*
   * Skip over dumper ID,
   * branch pointer and count
   */

  skip_mxbits(preamble_file, 32L * 9L + 72L);

  lp = (int)get_18_mxbit_integer(preamble_file);
  skip_mxbits(preamble_file, 18L);

  skip_mxbits(preamble_file, 18L);
  dirlist_preamble_ptr->nlinks = (int)get_18_mxbit_integer(preamble_file);
  if (dirlist_preamble_ptr->nlinks == 0)
  {
    dirlist_preamble_ptr->links = NULL;
    return ( 0 );
  }

  /* 
   * Read link structure from
   * backup_dir_list.incl.pl1
   */

  mxbit_pos(preamble_file, (long)( lp * 36L ));

  dirlist_preamble_ptr->links = (struct LINK *)malloc(
    sizeof ( struct LINK ) * dirlist_preamble_ptr->nlinks);
  save_link_name_addrs
    = (unsigned int *)malloc(sizeof ( int ) * dirlist_preamble_ptr->nlinks);
  save_link_target_addrs
    = (unsigned int *)malloc(sizeof ( int ) * dirlist_preamble_ptr->nlinks);

  for (i = 0; i < dirlist_preamble_ptr->nlinks; ++i)
  {
    link_ptr = dirlist_preamble_ptr->links + i;

        /* 
         * Time dumped on this tape, saved above.
         * Not DTD from dir entry
         */

    link_ptr->dtd = dtd;

        /*
         * Skip lk.vtoc_error
         * thru lk.pad2
         */

    skip_mxbits(preamble_file, 92L);

    get_36_mxbit_integer(preamble_file, long_pair);
    link_ptr->dtu = cvmxtime(long_pair);
    skip_mxbits(preamble_file, 16L);  /* Skip end of lk.dtu */
    skip_mxbits(preamble_file, 20L);  /* Skip lk.pad3 */
    get_36_mxbit_integer(preamble_file, long_pair);
    link_ptr->dtm = cvmxtime(long_pair);
    skip_mxbits(preamble_file, 16L);  /* Skip end of lk.dtm */
    skip_mxbits(preamble_file, 72L);  /* Skip over lk.pad4 and lk.dtd */

    save_link_target_addrs[i] = (int)get_18_mxbit_integer(preamble_file);
    save_link_name_addrs[i] = (int)get_18_mxbit_integer(preamble_file);

    skip_mxbits(preamble_file, 18L);  /* Skip over lk.ix */

    link_ptr->naddnames = (int)get_18_mxbit_integer(preamble_file);
    link_ptr->naddnames &= 0x7FFF; /* Mask off lk.dump_me */
    --( link_ptr->naddnames );       /* Don't count primary name */
  }

  for (i = 0; i < dirlist_preamble_ptr->nlinks; ++i)
  {
    link_ptr = dirlist_preamble_ptr->links + i;
    mxbit_pos(preamble_file, (long)( save_link_target_addrs[i] * 36L ));

        /*
         * Skip size
         * and author
         */

    skip_mxbits(preamble_file, 36L + 32L * 9L);
    get_mxstring(preamble_file, link_ptr->target, 168);
  }

  for (i = 0; i < dirlist_preamble_ptr->nlinks; ++i)
  {
    link_ptr = dirlist_preamble_ptr->links + i;

        /* 
         * Read name structure from
         * backup_dir_list.incl.pl1
         */

    if (link_ptr->naddnames > 0)
    {
      link_ptr->addnames = malloc(33 * link_ptr->naddnames);
    }

    mxbit_pos(preamble_file, (long)( save_link_name_addrs[i] * 36L ));
    skip_mxbits(preamble_file, 36L);  /* Skip name.size */
    get_mxstring(preamble_file, link_ptr->ename, 32);
    if (link_ptr->naddnames > 0)
    {
      for (j = 0; j < link_ptr->naddnames; ++j)
      {
        skip_mxbits(preamble_file, 36L);  /* Skip name.size */
        get_mxstring(preamble_file, link_ptr->addnames + 33 * j, 32);
      }
    }
    else
    {
      link_ptr->addnames = NULL;
    }
  }

  free(save_link_name_addrs);
  free(save_link_target_addrs);
  return ( 0 );
}
