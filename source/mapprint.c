                  /********************************\
                  *     Copyright (C) 1987 by      *
                  * Cygnus Cybernetics Corporation *
                  *    and Oxford Systems, Inc.    *
                  \********************************/

#include "mxload.h"

#include <stdio.h>
#ifdef MSDOS
#include <io.h>
#endif
#include <time.h>
#include <string.h>


#include "preamble.h"
#include "aclmodes.h"
#include "mxlargs.h"
#include "timestr.h"
#include "mapprint.h"

static char         last_dir[170] = {""};

void display_branch_preamble (preamble_ptr, branch_preamble_ptr)

struct PREAMBLE     *preamble_ptr;
struct BRANCH_PREAMBLE *branch_preamble_ptr;

{
     int            i;
     int            j;
     int            n_zeroes;
     struct ACL     *acl_ptr;
     char           access_class_string[50];
     
     if (strcmp (last_dir, preamble_ptr -> dname) != 0)
          {
          fprintf (mxlargs.map_file, "%s", preamble_ptr -> dname);
          strcpy (last_dir, preamble_ptr -> dname);
          fprintf (mxlargs.map_file, "\n");
     }
     fprintf (mxlargs.map_file, " %-32s", preamble_ptr -> ename);
     if (preamble_ptr -> record_type == SEGMENT_RECORD)
          {
          fprintf (mxlargs.map_file,
               "%3d", (preamble_ptr -> length + 1023) / 1024);
          fprintf (mxlargs.map_file, " %s", "SEG");
     }
     else fprintf (mxlargs.map_file, "    %s", "DIR");
     fprintf (mxlargs.map_file,
          "   %s", timestr (&(branch_preamble_ptr -> dtm)));
     fprintf (mxlargs.map_file,
          "   %s", timestr (&(branch_preamble_ptr -> dtu)));
     fprintf (mxlargs.map_file, "\n");
     if (mxlargs.verbose || mxlargs.extremely_verbose)
          {
          for (i = 0; i < branch_preamble_ptr -> naddnames; ++i)
               {
               fprintf (mxlargs.map_file,
                    "          %s", branch_preamble_ptr -> addnames + 33 * i);
               fprintf (mxlargs.map_file, "\n");
          }
     }
     if (mxlargs.verbose || mxlargs.extremely_verbose)
          {
          if (branch_preamble_ptr -> author[0] != '\0')
               {
               fprintf (mxlargs.map_file,
                    "%37sAuthor:          %s", "",
                    branch_preamble_ptr -> author);
               fprintf (mxlargs.map_file, "\n");
          }
          fprintf (mxlargs.map_file,
               "%37sBitcount Author: %s", "",
               branch_preamble_ptr -> bitcount_author);
          fprintf (mxlargs.map_file, "\n");
     }
     if (mxlargs.extremely_verbose)
          {
          if (branch_preamble_ptr -> switches != 0)
               {
               fprintf (mxlargs.map_file, "%37sSwitches:        ", "");
               if (branch_preamble_ptr -> switches & SAFETY_SW_MASK)
                    fprintf (mxlargs.map_file, "safety ");
               if (branch_preamble_ptr -> switches & ENTRYBD_SW_MASK)
                    fprintf (mxlargs.map_file, "gate ");
               if (branch_preamble_ptr -> switches & SOOS_SW_MASK)
                    fprintf (mxlargs.map_file, "soos ");
               if (branch_preamble_ptr -> switches & AUDIT_SW_MASK)
                    fprintf (mxlargs.map_file, "audit ");
               if (branch_preamble_ptr -> switches & MULTICLASS_SW_MASK)
                    fprintf (mxlargs.map_file, "multiclass ");
               if (branch_preamble_ptr -> switches & MDIR_SW_MASK)
                    fprintf (mxlargs.map_file, "mdir ");
               fprintf (mxlargs.map_file, "\n");
          }
     }

     if (mxlargs.extremely_verbose)
          {
          fprintf (mxlargs.map_file,
               "%37sDTBM:            %s\n", "",
               timestr (&(branch_preamble_ptr -> dtbm)));
          fprintf (mxlargs.map_file,
               "%37sDTD:             %s\n", "",
               timestr (&(branch_preamble_ptr -> dtd)));
          fprintf (mxlargs.map_file, "%37sUID:             ", "");
          fprintf (mxlargs.map_file, "%.6lo", branch_preamble_ptr -> uid[0]);
          fprintf (mxlargs.map_file, "%.6lo\n", branch_preamble_ptr -> uid[1]);

          fprintf (mxlargs.map_file, "%37sRing Brackets:   ", "");
          for (i = 0; i < 3; ++i)
               fprintf (mxlargs.map_file, "%d ",
                    branch_preamble_ptr -> rings[i]);
          fprintf (mxlargs.map_file, "\n");

          if (branch_preamble_ptr -> access_class[0] +
               branch_preamble_ptr -> access_class[1] +
               branch_preamble_ptr -> access_class[2] +
               branch_preamble_ptr -> access_class[3] > 0L)
               {
               format_access_class (branch_preamble_ptr -> access_class,
                    access_class_string);
               fprintf (mxlargs.map_file, "%37sAccess Class:    %s", "",
                    access_class_string);
          }
     }
     if (mxlargs.verbose || mxlargs.extremely_verbose)
          fprintf (mxlargs.map_file,
               "%37sBitcount:        %ld (%ld KB)\n", "",
               preamble_ptr -> bitcnt,
               (preamble_ptr -> bitcnt + 1023L * 9L) / (1024L * 9L));
     if (mxlargs.extremely_verbose)
          fprintf (mxlargs.map_file, "%37sCurrent length:  %d\n", "",
               branch_preamble_ptr -> cur_length);

     if (mxlargs.verbose || mxlargs.extremely_verbose)
          {
          for (i = 0; i < branch_preamble_ptr -> nacl; ++i)
               {
               if (i == 0)
                    fprintf (mxlargs.map_file, "%37sACL:             ", "");
               else fprintf (mxlargs.map_file, "%54s", "");
               acl_ptr = branch_preamble_ptr -> acl + i;
               if (preamble_ptr -> record_type == SEGMENT_RECORD)
                    {
                    fprintf (mxlargs.map_file, "%s %s",
                         SEG_ACCESS_MODES[acl_ptr -> mode[0] >> 5],
                         acl_ptr -> access_name);
                    /* Print extended acl term if nonzero,
                       through last non-zero bit */
                    if (acl_ptr -> ext_mode[0] != 0)
                         {
                         fprintf (mxlargs.map_file, " (");
                         n_zeroes = 0;
                         for (j = 0; j < 7; ++j)
                              {
                              if (acl_ptr -> ext_mode[0] & (0x80 >> j))
                                   {
                                   for (;n_zeroes > 0;--n_zeroes)
                                        fprintf (mxlargs.map_file, "0");
                                   fprintf (mxlargs.map_file, "1");
                              }
                              else ++n_zeroes;
                         }
                         fprintf (mxlargs.map_file, ")");
                    }
               }
               else fprintf (mxlargs.map_file, "%s %s",
                    DIR_ACCESS_MODES[acl_ptr -> mode[0] >> 5],
                    acl_ptr -> access_name);
               fprintf (mxlargs.map_file, "\n");
          }
     }
}

void display_dirlist_preamble (preamble_ptr, dirlist_preamble_ptr)

struct PREAMBLE *preamble_ptr;
struct DIRLIST_PREAMBLE *dirlist_preamble_ptr;

{
     int            i;
     int            j;
     struct LINK    *link_ptr;

     if (!mxlargs.verbose)
          return;

     for (i = 0; i < dirlist_preamble_ptr -> nlinks; ++i)
          {
          if (strcmp (last_dir, preamble_ptr -> dname) != 0)
               {
               fprintf (mxlargs.map_file, "%s", preamble_ptr -> dname);
               strcpy (last_dir, preamble_ptr -> dname);
               fprintf (mxlargs.map_file, "\n");
          }
          
          link_ptr = dirlist_preamble_ptr -> links + i;
          fprintf (mxlargs.map_file, " %-32s", link_ptr -> ename);
          fprintf (mxlargs.map_file, "   ");
          fprintf (mxlargs.map_file, " LINK");
          fprintf (mxlargs.map_file, "  %s", timestr (&(link_ptr -> dtm)));
          fprintf (mxlargs.map_file, "\n");
          if (mxlargs.verbose || mxlargs.extremely_verbose)
               {
               for (j = 0; j < link_ptr -> naddnames; ++j)
                    {
                    fprintf (mxlargs.map_file, "          %s",
                         link_ptr -> addnames + 33 * j);
                    fprintf (mxlargs.map_file, "\n");
               }
          }
          fprintf (mxlargs.map_file, "%37sTarget: %s", "",
               link_ptr -> target);
          fprintf (mxlargs.map_file, "\n");
          fprintf (mxlargs.map_file, "\n");
     }
}

void format_access_class (access_class, access_class_string)

unsigned long       access_class[4]; /* 18 bits stored in each long */
char                *access_class_string;

{
     char           temp_str[40];

     sprintf (access_class_string, "%ld:", access_class[2]); /* Level */
     sprintf (temp_str, "%.6lo", access_class[0]); /* Categories */
     strcat (access_class_string, temp_str);
     /* If any supposedly unused bits are non-zero,
        redisplay the whole thing in octal */
     if (access_class[1] != 0 || access_class[2] > 7 ||
          (access_class[3] & 0x7ff) != 0)
          {
          sprintf (temp_str, " (%.6lo%.6lo%.6lo%.6lo)", access_class[0],
               access_class[1], access_class[2], access_class[3]);
          strcat (access_class_string, temp_str);
     }
}

/* Write line on map output giving conversion type and path */

void display_conversion_info (newpath, segtype, conversion_type)

char                *newpath;
char                *segtype;
char                *conversion_type;

{
     if (segtype != NULL && strcmp (segtype, "unknown") != 0)
          fprintf (mxlargs.map_file,
               "%19sSegment Type:    %s\n", "", segtype);

     if (conversion_type != NULL)
          fprintf (mxlargs.map_file,
               "%19sConversion Type: %s\n", "", conversion_type);

     fprintf (mxlargs.map_file, "%19sLoading Into:    %s\n", "", newpath);
}
