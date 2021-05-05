                  /********************************\
                  *     Copyright (C) 1987 by      *
                  * Cygnus Cybernetics Corporation *
                  *    and Oxford Systems, Inc.    *
                  \********************************/

/* MXARC */

/* Read Multics archives and retrieve components. This is basically a
   lobotomized version of mxload.  */

#include "mxload.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>

#ifdef MSDOS
#include <sys/utime.h>
#include <stdlib.h>
#include <io.h>
#include <direct.h>
#include <malloc.h>
#else
char                 *getenv();
char                 *malloc();
#endif

#include "mxbitio.h"
#include "preamble.h"
#include "parsctl.h"
#include "mxlopts.h"
#include "rdbkrcd.h"
#include "cvpath.h"
#include "dirsep.h"
#include "optptr.h"
#include "gettype.h"
#include "mapprint.h"
#include "copybits.h"
#include "mxdearc.h"
#include "mxmseg.h"
#include "aclmodes.h"
#include "getopt.h"
#include "mxlargs.h"
#include "tempfile.h"

#ifndef MSDOS
#include "utime.h"
#endif

#ifdef LINT_ARGS
static void usage(void);
static int onintr (int);
static int make_new_path_fixed_name(char *dname, char *ename,
       struct MXLOPTS *retrieval_options_ptr,char *name_type,char *new_path);
static void convert_seg(struct mxbitiobuf *contents_filename,
       struct mxbitiobuf *preconverted_contents_file,
       struct PREAMBLE *preamble_ptr,
       struct BRANCH_PREAMBLE *branch_preamble_ptr,char *seg_type,
       char *conversion_type,struct MXLOPTS *retrieval_options_ptr);
static void put_in_place(struct mxbitiobuf *contents_file,char *new_path,
       int is_ascii, long char_count);
static void set_attrs(char *new_path,struct MXLOPTS *retrieval_options_ptr,
       struct PREAMBLE *preamble_ptr,
       struct BRANCH_PREAMBLE *branch_preamble_ptr);
static int find_and_mark_in_list (char **list, int n_entries, char *entry);
#else
static void usage ();
static int onintr ();
static int make_new_path_fixed_name();
static void convert_seg();
static void put_in_place();
static void set_attrs();
static int find_and_mark_in_list ();
#endif

struct MXLARGS      mxlargs;
static char         **components;       /* List of components to extract */
static int          n_components;       /* Count of components to extract */
static char         renamed_archive[400] = "";


void main (argc, argv)

int argc;
char *argv[];

{
     char           *temp_path;
     struct PREAMBLE preamble;
     struct BRANCH_PREAMBLE branch_preamble;
     struct DIRLIST_PREAMBLE dirlist_preamble;
     struct MXLOPTS *retrieval_options_ptr;
     int            option_letter;
     char           *control_files[50];
     int            n_control_files;
     int            unpack_sw;
     int            table_sw;
     int            extract_sw;
     MXBITFILE      *archive_file;
     int            is_ascii;
     static char    archive_header_begin[8] =
                    {014, 012, 012, 012, 017, 012, 011, 011};
     char           comp_header[10];
     int            i;
     int            n_archives;
     int            dearc_result;
     int            rename_result;
     
#ifdef MSDOS
     argv[0] = "mxload";       /* For use by getopt, (under MS-DOS, default
                                  value is full path of command */
#endif

     if (signal (SIGINT, SIG_IGN) != SIG_IGN)
          signal (SIGINT, onintr);

     n_control_files = 0;
     control_files[0] = NULL;
     memset (&mxlargs, 0, sizeof (mxlargs));
     mxlargs.map_file = stdout;
     mxlargs.map_filename = NULL;
     unpack_sw = table_sw = extract_sw = 0;
     while ((option_letter = getopt(argc, argv, "c:ntux")) != EOF)
          {
          switch (option_letter)
               {
               case 'c':                /* Control file */
                         control_files[n_control_files++] = 
                              strcpy (malloc (strlen (optarg) + 1), optarg);
                         break;
               case 'n':                /* No map */
                         mxlargs.no_map = 1;
                         break;
               case 't':                /* Map only */
                         table_sw = 1;
                         mxlargs.map_only = 1;
                         break;
               case 'u':                /* Unpack */
                         unpack_sw = 1;
                         break;
               case 'x':                /* Extract */
                         extract_sw = 1;
                         break;
               case '?':                /* ? or unrecognized option */
                         usage ();
                         mxlexit (4);
          }
     }

     if (unpack_sw + table_sw + extract_sw != 1)
          usage ();

     if (mxlargs.no_map)
#ifdef MSDOS        
          mxlargs.map_file = fopen ("NUL", "w");
#else
          mxlargs.map_file = fopen ("/dev/null", "w");
#endif

     /* Adjust arguments to skip over program name and options,
        putting position at archive file name */
     argc -= optind;
     argv += optind;

     if (argc < 1)
          usage();
     if (extract_sw)
          {
          components = argv+1;
          n_components = argc - 1;
          n_archives = 1;
     }
     else
          {
          n_components = 0;
          n_archives = argc;
     }

     if (table_sw)
        retrieval_options_ptr = NULL;
     else
          {
          /* Get retrieval options.  Returns default options only, i.e. 
             builtin defaults plus result of any global statements in 
             control files. */
          retrieval_options_ptr =
               parsctl (n_control_files, control_files, 0, NULL, 1);
          /* Adjust the retrieval options a bit. */
          if (extract_sw)
               retrieval_options_ptr -> reload = "flat";
     }

     /* Construct the nullest of preambles */
     memset (&preamble, 0, sizeof(preamble));
     preamble.record_type = SEGMENT_RECORD;
     memset (&branch_preamble, 0, sizeof(branch_preamble));

     for (i = 0; i < n_archives; ++i)
          {
          if ((archive_file = open_mxbit_file (argv[i], "rt")) == NULL) 
               {
               fprintf (stderr,
                    "Cannot open Multics archive file %s.\n", argv[i]);
               continue;
          }
          /* Read what should be header of first component to see if we have 
             an already 8bit-ified archive, a raw 9bit archive, or no archive
             at all.  For MS-DOS, the translation of NL to CR-LF means that
             we cannot reliably read an 8bit-ified archive. */
          get_mxstring (archive_file, comp_header, 8);
          if (memcmp (comp_header, archive_header_begin, 8) == 0)
               is_ascii = 0;
          else
               {
#ifdef MSDOS
               fprintf (stderr,
                    "mxarc:  %s does not appear to be a Multics archive.\n",
                    argv[i]);
               fprintf (stderr, "Be sure that the archive was reloaded ");
               fprintf (stderr, "with 9bit conversion.\n");
               continue;
#else     
               mxbit_pos (archive_file, 0L);
               get_mxbits (archive_file, 64L, comp_header);
               if (memcmp (comp_header, archive_header_begin, 8) == 0)
                    is_ascii = 1;
               else
                    {
                    fprintf (stderr, "mxarc:  %s does not appear", argv[i]);
                     fprintf (stderr, "to be a Multics archive.\n");
                    continue;
               }
#endif

          }

          if (i > 0)
               fprintf (mxlargs.map_file, "\n");
          if (n_archives > 1)
               fprintf (mxlargs.map_file, "%s\n", argv[i]);
          if (unpack_sw)
               {
               /* Rename the archive file to make way for a new directory
                    before unpacking. */
#ifdef sun
#define BSD
#endif
#ifdef MSDOS
               cvpath (argv[i], "", "MSDOS", renamed_archive);
#else
#ifdef BSD
               cvpath (argv[i], "", "BSD", renamed_archive);
#else
               cvpath (argv[i], "", "SysV", renamed_archive);
#endif
#endif
#ifdef SVR2
               if ((rename_result = link (argv[i], renamed_archive)) == 0)
                    rename_result = unlink (argv[i]);
#else
               rename_result = rename (argv[i], renamed_archive);
#endif
               if (rename_result != 0)
                    {
                    fprintf (stderr,  "Cannot rename %s to %s.\n", 
                         argv[i], renamed_archive);
                    continue;
               }

               /* Put results in directory with old archive name */
               strcpy (retrieval_options_ptr -> new_path, argv[i]);
          }
          dearc_result = mxdearc (archive_file, &preamble,
               retrieval_options_ptr, &branch_preamble, is_ascii);
               {
               /* When unpacking, delete the original archive */
               if (unpack_sw && rename_result == 0)
                    {
                    if (dearc_result == 0)
                         unlink (renamed_archive);
                    else fprintf (stderr, "Original archive is in %s\n",
                         renamed_archive);
                    renamed_archive[0] = '\0';
               }
          }
     }
     for (i = 0; i < n_components; ++i)
          {
          if (strlen (components[i]) > 0)
               printf ("Component not found: %s\n", components[i]);
     }
     
     mxlexit (0);
}

/* Print usage message */
static void usage ()
{
     fprintf (stderr, "Usage: mxarc -t [options] archive_file ...\n");
     fprintf (stderr, "Or:    mxarc -u [options] archive_file ...\n");
     fprintf (stderr,
          "Or:    mxarc -x [options] archive_file [component ...]\n");
     fprintf (stderr, "\nOptions are:\n");
     fprintf (stderr, "-c control_file\t= control file; ");
     fprintf (stderr, "may be specified more than once\n");
     fprintf (stderr, "-n\t\t= no map\n");
     fprintf (stderr, "-t\t\t= table; just produce a map\n");
     fprintf (stderr, "-u\t\t= unpack; replace archive by ");
     fprintf (stderr, "directory containing components\n");
     fprintf (stderr,
          "-x\t\t= extract; put copies of components in current directory\n");
     mxlexit (4);
}

static int onintr (sig)

int                 sig;
{
     mxlexit (4);
}

/* Exit after cleaning up.  Called by various subrs. */
void mxlexit (status)

int                 status;

{
     if (renamed_archive[0] != '\0')
          fprintf (stderr, "Original archive is in %s\n", renamed_archive);
     cleanup_temp_files();
     exit (status);
}

/* Check conversion type of segment & call convert_seg to do the
   appropriate conversion, except for "discard".

   The conversion type is determined in one of two ways:
        1) Explicitly specified in the control file with a force_convert
           statement.  In this case, it doesn't matter what the segment 
           type is.
        2) Determined from the segment type, which can itself be determined
           in two ways:
                a) From the segment's name, not requiring that the segment
                   be read first.
                b) From the segment's contents, in which case the segment must
                   be read first, and may then not be used if the conversion
                   type turns out to be "discard".
*/

void process_seg (infile, branch_preamble_ptr, preamble_ptr,
     retrieval_options_ptr, is_ascii)

MXBITFILE           *infile;
struct BRANCH_PREAMBLE *branch_preamble_ptr;
struct PREAMBLE     *preamble_ptr;
struct MXLOPTS      *retrieval_options_ptr;
int                 is_ascii;

{              
     int            seg_has_been_read;
     char           *seg_type;
     char           *seg_type_by_name;
     char           *seg_type_by_content;
     char           *conversion_type;
     MXBITFILE      *contents_file;
     MXBITFILE      *preconverted_contents_file;

     if (n_components > 0)
          {
          if (!find_and_mark_in_list (components, n_components,
               preamble_ptr -> ename))
               return;
     }

     if (mxlargs.map_only)
          {
          display_branch_preamble (preamble_ptr, branch_preamble_ptr);
          return;
     }

     preconverted_contents_file =
          get_temp_file ("wt", "preconverted file contents");
     contents_file = get_temp_file ("wt", "raw file contents");
     if (contents_file == NULL || preconverted_contents_file == NULL)
          mxlexit (4);    /* Make sure original archive doesn't get deleted */

     /* If user wants to disregard bitcount, fix up the adjusted bitcount */
     if (strcmp (retrieval_options_ptr -> dataend, "page_boundary") == 0)
          preamble_ptr -> adjusted_bitcnt = preamble_ptr -> maximum_bitcnt;

     seg_has_been_read = 0;
     if (retrieval_options_ptr -> force_convert != NULL)
          {
          conversion_type = retrieval_options_ptr -> force_convert;
          seg_type = "unknown";
     }
     else
          {
          if (is_ascii)
               {
               seg_has_been_read = 1;
               seg_type_by_name = get_type_by_name (preamble_ptr);
               if (seg_type_by_name != NULL && strcmp (seg_type_by_name,
                    "ascii_archive") == 0)
                    seg_type = "ascii_archive";
               else seg_type = "ascii";
          }
          else
               {
               seg_type_by_name = get_type_by_name (preamble_ptr);
               if (seg_type_by_name == NULL ||
                    strcmp (seg_type_by_name, "archive") == 0)
                    {
                    if (rdseg (infile, contents_file, preamble_ptr) != 0)
                         goto EXIT;
                    seg_has_been_read = 1;
                    seg_type_by_content =
                         get_type_by_content (contents_file,
                         preamble_ptr -> adjusted_bitcnt, 
                         preconverted_contents_file);
                    if (seg_type_by_name != NULL &&
                         strcmp (seg_type_by_name, "archive") == 0)
                         {
                         if (strcmp (seg_type_by_content, "ascii") == 0)
                              seg_type = "ascii_archive";
                         else seg_type = "nonascii_archive";
                    }
                    else seg_type = seg_type_by_content;
               }
               else seg_type = seg_type_by_name;
          }
          conversion_type = retrieval_options_ptr -> 
               file_cv_values[get_keyword_values_index (file_cv_types,
               seg_type)];
     }
     if (strcmp (conversion_type, "discard") == 0)
          {
          if (!seg_has_been_read)
               rdseg (infile, NULL, preamble_ptr);  /* If seg has not already
                                                       been read, skip it */
          goto EXIT;
     }

     if (!seg_has_been_read &&
          rdseg (infile, contents_file, preamble_ptr) != 0)
          goto EXIT;

     if (strcmp (conversion_type, "unpack") != 0)
          display_branch_preamble (preamble_ptr, branch_preamble_ptr);
     if (strcmp (seg_type, "ascii") == 0 ||
          strcmp (seg_type, "ascii_archive") == 0)
          {
          if (is_ascii)
               convert_seg (NULL, contents_file, preamble_ptr,
                    branch_preamble_ptr,
               seg_type, conversion_type, retrieval_options_ptr);
          else convert_seg (contents_file, preconverted_contents_file,
               preamble_ptr, branch_preamble_ptr, seg_type, conversion_type,
               retrieval_options_ptr);
     }
     else convert_seg (contents_file, NULL, preamble_ptr, branch_preamble_ptr,
          seg_type, conversion_type, retrieval_options_ptr);
EXIT:
     release_temp_file (preconverted_contents_file,
          "preconverted file contents");
     release_temp_file (contents_file, "raw file contents");
}

/* This function can be called by mxdearc in place of the usual process_seg 
   function, to process preconverted-to-8bit segments.  It must only be called
   for segment types "ascii" and "ascii_archive" (for the *all important*
   recursive archive unpacking)  when the conversion type for "ascii" will
   be "8bit" and for "ascii_archive" it will be "unpack". */

void process_ascii_seg (branch_preamble_ptr, contents_file, preamble_ptr,
     retrieval_options_ptr)

struct BRANCH_PREAMBLE *branch_preamble_ptr;
MXBITFILE           *contents_file;
struct PREAMBLE     *preamble_ptr;
struct MXLOPTS      *retrieval_options_ptr;

{              
     char           *seg_type;
     char           *seg_type_by_name;
     char           *conversion_type;

     if (mxlargs.map_only)
          {
          display_branch_preamble (preamble_ptr, branch_preamble_ptr);
          return;
     }
     seg_type_by_name = get_type_by_name (preamble_ptr);
     if (seg_type_by_name != NULL &&
          strcmp (seg_type_by_name, "ascii_archive") == 0)
          {
          conversion_type = "unpack";
          seg_type = "ascii_archive";
     }
     else 
          {
          conversion_type = "8bit";
          seg_type = "ascii";
     }

     if (strcmp (conversion_type, "unpack") != 0)
          display_branch_preamble (preamble_ptr, branch_preamble_ptr);
     convert_seg (NULL, contents_file, preamble_ptr, branch_preamble_ptr,
          seg_type, conversion_type, retrieval_options_ptr);
}

static int find_and_mark_in_list (list, n_entries, entry)

char                *list[];
int                 n_entries;
char                *entry;

{
     int            i;
     
     for (i = 0; i < n_entries; ++i)
          {
          if (strcmp (list[i], entry) == 0)
               {
               list[i][0] = '\0';                 /* Remove from list */
               return (1);
          }
     }
     return (0);
}

/* Read segment from temp file and write converted form into new path */

static void convert_seg (contents_file, preconverted_contents_file,
       preamble_ptr, branch_preamble_ptr,
       seg_type, conversion_type, retrieval_options_ptr)

MXBITFILE           *contents_file;
struct PREAMBLE     *preamble_ptr;
struct BRANCH_PREAMBLE *branch_preamble_ptr;
char                *seg_type;
char                *conversion_type;
struct MXLOPTS      *retrieval_options_ptr;
MXBITFILE           *preconverted_contents_file;

{
     char           new_path[400];
     char           *name_type;
     static char    last_dir[170] = "";
     
     name_type = retrieval_options_ptr ->
          attr_cv_values[get_keyword_values_index (attr_cv_types,
          "name_type")];
     ++ retrieval_options_ptr -> n_files_loaded;

     if (strcmp (conversion_type, "9bit") == 0)
          {
          if (make_new_path (preamble_ptr -> dname, preamble_ptr -> ename,
               retrieval_options_ptr, "", name_type, new_path) != 0)
          mxlexit (4);                  /* Make sure original archive doesn't 
                                           get deleted */
          display_conversion_info (new_path, seg_type, conversion_type);
          put_in_place (contents_file, new_path, 0,
               (preamble_ptr -> adjusted_bitcnt + 7L) / 8L);
          set_attrs (new_path, retrieval_options_ptr, preamble_ptr,
               branch_preamble_ptr);
     }
     else if (strcmp (conversion_type, "8bit") == 0)
          {
          if (make_new_path (preamble_ptr -> dname, preamble_ptr -> ename,
               retrieval_options_ptr, "", name_type, new_path) != 0)
          mxlexit (4);                  /* Make sure original archive doesn't 
                                           get deleted */
          display_conversion_info (new_path, seg_type, conversion_type);
          if (preconverted_contents_file == NULL)
               copy_8bit (contents_file, new_path,
               preamble_ptr -> adjusted_bitcnt);
          else put_in_place (preconverted_contents_file, new_path, 1,
               preamble_ptr -> adjusted_bitcnt / 9L);
          set_attrs (new_path, retrieval_options_ptr, preamble_ptr,
               branch_preamble_ptr);
     }
     else if (strcmp (conversion_type, "8bit+9bit") == 0)
          {
          if (make_new_path (preamble_ptr -> dname, preamble_ptr -> ename,
               retrieval_options_ptr, "A", name_type, new_path) != 0)
          mxlexit (4);                  /* Make sure original archive doesn't 
                                           get deleted */
          display_conversion_info (new_path, seg_type, conversion_type);
          copy_8bit (contents_file, new_path, preamble_ptr -> adjusted_bitcnt);
          set_attrs (new_path, retrieval_options_ptr, preamble_ptr,
               branch_preamble_ptr);
          if (make_new_path (preamble_ptr -> dname, preamble_ptr -> ename,
               retrieval_options_ptr, "B", name_type, new_path) != 0)
          mxlexit (4);                  /* Make sure original archive doesn't 
                                           get deleted */
          display_conversion_info (new_path, NULL, NULL);
          put_in_place (contents_file, new_path, 0, 
               preamble_ptr -> adjusted_bitcnt / 8L);
          set_attrs (new_path, retrieval_options_ptr, preamble_ptr,
               branch_preamble_ptr);
     }
     else if (strcmp (conversion_type, "unpack") == 0 &&
          strcmp (seg_type, "ascii_archive") == 0)
          {
          /* Normal case for unpacking an ascii archive will be to pass 
             mxdearc the preconverted-to-8bit segment.  Unpacked components
             will be passed to process_ascii_seg.
             However, in the unlikely event that the conversion to be done for
             subsquently unpacked ascii segments is other than 8bit, we must
             go the slow route and use the original 9bit segment so that we
             can get 9bit conversion done on components. */
          if (strcmp ("8bit", retrieval_options_ptr ->
               file_cv_values[get_keyword_values_index (file_cv_types,
               "ascii")]) != 0)
               mxdearc (contents_file, preamble_ptr, retrieval_options_ptr,
                    branch_preamble_ptr, 0);
          else mxdearc (preconverted_contents_file, preamble_ptr,
               retrieval_options_ptr, branch_preamble_ptr, 1);
     }
     else if (strcmp (conversion_type, "unpack") == 0 &&
          strcmp (seg_type, "nonascii_archive") == 0)
          mxdearc (contents_file, preamble_ptr, retrieval_options_ptr,
               branch_preamble_ptr, 0);
     else if (strcmp (conversion_type, "repack") == 0 &&
          (strcmp (seg_type, "mbx") == 0 || strcmp (seg_type, "ms") == 0))
          mxmseg (contents_file, preamble_ptr, retrieval_options_ptr,
               branch_preamble_ptr, 1, seg_type);
     else 
          {
          fprintf (stderr,
               "Conversion type %s is not yet implemented ", conversion_type);
           fprintf (stderr, "for segment type %s.\n", seg_type);
          mxlexit (4);                  /* Make sure original archive doesn't 
                                           get deleted */
     }
     return;
}

/* Renames temporary file into permanent position if possible, otherwise 
   it copies it into position. */

static void put_in_place (contents_file, new_path, is_ascii, charcount)

MXBITFILE           *contents_file;
char                *new_path;
int                 is_ascii;
long                charcount;

{
     int            rename_result;

#ifdef MSDOS
     /* On MSDOS we have to make sure that ascii files go through
        a copying step that will translate newlines to CR-LF. */
     rename_result = -1;
#else
#ifdef SVR2
     if ((rename_result = link (temp_file_name (contents_file), new_path)) == 0)
          rename_result = unlink (temp_file_name (contents_file));
#else
     rename_result = rename (temp_file_name (contents_file), new_path);
#endif    
#endif    
     
     if (rename_result == 0)
          replace_temp_file (contents_file);
     else copy_file (contents_file, new_path, is_ascii, charcount);
}

static void set_attrs (new_path, retrieval_options_ptr, preamble_ptr,
       branch_preamble_ptr)

char                *new_path;
struct MXLOPTS      *retrieval_options_ptr;
struct PREAMBLE *preamble_ptr;
struct BRANCH_PREAMBLE *branch_preamble_ptr;

{
     char           *access_time_action;
     char           *mod_time_action;
     char           *acl_action;     
     char           *addname_action;     
     struct utimbuf utime_struct;
     char           *name_type;
     char           *group_action;
     char           project_id[20];
     char           *owner_action;
     char           person_id[20];

     name_type = retrieval_options_ptr ->
          attr_cv_values[get_keyword_values_index (attr_cv_types,
          "name_type")];

     /* Set the access time if requested */
     access_time_action = retrieval_options_ptr ->
          attr_cv_values[get_keyword_values_index (attr_cv_types,
          "access_time")];
     if (strcmp (access_time_action, "dtu") == 0)
          utime_struct.actime = branch_preamble_ptr -> dtu;
     else utime_struct.actime = time (NULL);

     /* Set the modification time if requested */
     mod_time_action = retrieval_options_ptr ->
          attr_cv_values[get_keyword_values_index (attr_cv_types,
          "modification_time")];
     if (strcmp (mod_time_action, "dtcm") == 0)
          utime_struct.modtime = branch_preamble_ptr -> dtm;
     else utime_struct.modtime = time (NULL);

     utime (new_path, &utime_struct);             /* The actual time-setting */
}
