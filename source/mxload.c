                  /********************************\
                  *     Copyright (C) 1987 by      *
                  * Cygnus Cybernetics Corporation *
                  *    and Oxford Systems, Inc.    *
                  \********************************/

/* MXLOAD */

/* Read Multics backup file or tape and retrieve segments. */

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
char *getenv();
#include <grp.h>
#include <pwd.h>

struct group        *getgrgid();
struct passwd       *getpwuid();
#endif

#ifdef LINT_ARGS
static void usage(void);
static int onintr (int);
static int make_new_path_fixed_name(char *dname, char *ename,
       struct MXLOPTS *retrieval_options_ptr,char *name_type,char *new_path);
static void convert_seg(struct mxbitiobuf *contents_filename,
       struct mxbitiobuf *preconverted_contents_file,
       struct PREAMBLE *preamble_ptr,
       struct BRANCH_PREAMBLE *branch_preamble_ptr,
       char *seg_type,char *conversion_type,
       struct MXLOPTS *retrieval_options_ptr);
static void put_in_place(struct mxbitiobuf *contents_file,char *new_path,
       int is_ascii, long char_count);
static void set_attrs(char *new_path,struct MXLOPTS *retrieval_options_ptr,
       struct PREAMBLE *preamble_ptr,
       struct BRANCH_PREAMBLE *branch_preamble_ptr);
static void check_file_matches (struct MXLOPTS *retrieval_options_ptr);
static void store_acl (char * new_path, struct MXLOPTS *retrieval_options_ptr,
       struct PREAMBLE *preamble_ptr,
       struct BRANCH_PREAMBLE *branch_preamble_ptr, char *name_type,
       int global_file);
static void store_addnames (char * new_path,
       struct MXLOPTS *retrieval_options_ptr, struct PREAMBLE *preamble_ptr,
       struct BRANCH_PREAMBLE *branch_preamble_ptr, char *name_type,
       int global_file);
static void get_project (char *access_id, char *proj_id);
static void get_person (char *access_id, char *pers_id);
static void file_links (struct PREAMBLE *preamble_ptr,
       struct DIRLIST_PREAMBLE *dirlist_preamble_ptr,
       struct MXLOPTS *retrieval_options_ptr, int global_file);
static void set_owner (char *new_path, struct USER_TRANSLATION *xlation_ptr,
       char *project_id, char *person_id);
static void set_permission_bits (char *new_path,
       struct MXLOPTS *retrieval_options_ptr, char *project_id,
       char *person_id, struct PREAMBLE *preamble_ptr,
       struct BRANCH_PREAMBLE *branch_preamble_ptr);
static void open_local_map (char *dir, struct MXLOPTS *retrieval_options_ptr);
#else
static void usage ();
static int onintr ();
static int make_new_path_fixed_name();
static void convert_seg();
static void put_in_place();
static void set_attrs();
static void check_file_matches ();
static void store_acl ();
static void store_addnames ();
static void get_project ();
static void get_person ();
static void file_links ();
static void set_owner ();
static void set_permission_bits ();
static void open_local_map ();
#endif


struct MXLARGS mxlargs;

void main (argc, argv)

int argc;
char *argv[];

{
     MXBITFILE      *mx_backup_file;
     struct PREAMBLE preamble;
     struct BRANCH_PREAMBLE branch_preamble;
     struct DIRLIST_PREAMBLE dirlist_preamble;
     struct MXLOPTS *retrieval_list_ptr;
     struct MXLOPTS *retrieval_options_ptr;
     int            option_letter;
     char           *control_files[50];
     int            n_control_files;
     MXBITFILE      *preamble_file;
     
#ifdef MSDOS
     argv[0] = "mxload";  /* For use by getopt, (under MSDOS, default value
                             is full path of command ) */
#endif

     if (signal (SIGINT, SIG_IGN) != SIG_IGN)
          signal (SIGINT, onintr);

     n_control_files = 0;
     control_files[0] = NULL;
     memset (&mxlargs, 0, sizeof (mxlargs));
     mxlargs.map_file = NULL;
     mxlargs.map_filename = NULL;
     while ((option_letter = getopt(argc, argv, "bc:g:lnvx")) != EOF)
          {
          switch (option_letter)
               {
               case 'b':                /* Brief: suppress no-match warnings */
                         mxlargs.brief = 1;
                         break;
               case 'c':                /* Control file */
                         control_files[n_control_files++] =
                              strcpy (malloc (strlen (optarg) + 1), optarg);
                         break;
               case 'g':                /* Global map file */
                         mxlargs.map_filename =
                              strcpy (malloc (strlen (optarg) + 1), optarg);
                         mxlargs.local_map = 0;   /* Override -l */
                         mxlargs.no_map = 0;      /* Override -n */
                         break;
               case 'l':                /* Local map files: 
                                           mxload.map in each directory */
                         mxlargs.local_map = 1;
                         mxlargs.no_map = 0;      /* Override -n */
                         mxlargs.map_filename = NULL; /* Override -g */
                         break;
               case 'n':                /* No map */
                         mxlargs.no_map = 1;
                         break;
               case 'v':                /* Verbose:  long listing output */
                         mxlargs.verbose = 1;
                         break;
               case 'x':                /* Extremely verbose:
                                           full listing output */
                         mxlargs.extremely_verbose = 1;
                         break;
               case '?':                /* ? or unrecognized option */
                         usage ();
                         mxlexit (4);
          }
     }

     /* Adjust arguments to skip over program name and options,
        putting position at dumpfile name */
     argc -= optind;
     argv += optind;

     if (argc < 1)
          usage();
     if (argc % 2 != 1)
          {
          fprintf (stderr, "mxload:  Missing argument.\n");
          fprintf (stderr, 
#ifdef MSDOS
               "Multics path \"%s\" requires a matching MS-DOS path.\n\n",
#else     
               "Multics path \"%s\" requires a matching Unix path.\n\n",
#endif
               argv[argc - 1]);
          usage ();
     }
     if (n_control_files + (argc - 1) == 0)
          {
          fprintf (stderr, "%s\n\n",
               "mxload:  Either a control file or a subtree must be specified.");
          usage ();
     }

     retrieval_list_ptr =
          parsctl (n_control_files, control_files, argc - 1 , argv + 1, 0);
     if ((mx_backup_file = open_mxbit_file (argv[0], "r")) == NULL) 
          {
          fprintf (stderr, "Cannot open Multics backup file %s.\n", argv[0]);
          mxlexit (4);
     }

     preamble_file = get_temp_file ("wt", "preamble");
     if (preamble_file == NULL)
          mxlexit (4);

     /* Open map file if global */
     
     if (mxlargs.map_filename != NULL)
          {
          mxlargs.map_file = fopen (mxlargs.map_filename, "a");
     }
     else if (mxlargs.no_map)
#ifdef MSDOS        
          mxlargs.map_file = fopen ("NUL", "w");
#else
          mxlargs.map_file = fopen ("/dev/null", "w");
#endif
     else if (!mxlargs.local_map)
          mxlargs.map_file = stdout;

     while (rdbkrcd(mx_backup_file, preamble_file, &preamble) == 0)
          {
          if (preamble.record_type == SEGMENT_RECORD &&
               (retrieval_options_ptr =
                    get_options_ptr (&preamble, retrieval_list_ptr)) != NULL)
               {
               ++ retrieval_options_ptr -> n_files_matched;
               get_branch_preamble (preamble_file, &branch_preamble, &preamble);
               process_seg (mx_backup_file, &branch_preamble, &preamble,
                    retrieval_options_ptr, 0);
               if (branch_preamble.addnames != NULL)
                    free (branch_preamble.addnames);
               if (branch_preamble.acl != NULL)
                    free ((char *) branch_preamble.acl);
          }
          else if (preamble.record_type == DIRLIST_RECORD &&
               (retrieval_options_ptr =
                    get_options_ptr (&preamble, retrieval_list_ptr)) != NULL)
               {    
               if (strcmp (retrieval_options_ptr -> 
                    list_values[get_keyword_values_index (list_types,
                         "link")], "none") != 0)
                    {         
                    get_dirlist_preamble (preamble_file, &dirlist_preamble, &preamble);
                    if (strcmp (retrieval_options_ptr ->
                         list_values[get_keyword_values_index (list_types,
                              "link")], "global") == 0)
                         file_links (&preamble, &dirlist_preamble,
                              retrieval_options_ptr, 1);
                    else file_links (&preamble, &dirlist_preamble,
                         retrieval_options_ptr, 0);
               }
          }
          else
               {
               rdseg (mx_backup_file, NULL, &preamble);
          }
     }
     check_file_matches (retrieval_list_ptr);
     mxlexit (0);
}

static void file_links (preamble_ptr, dirlist_preamble_ptr,
       retrieval_options_ptr, global_file)

struct PREAMBLE *preamble_ptr;
struct DIRLIST_PREAMBLE *dirlist_preamble_ptr;
struct MXLOPTS *retrieval_options_ptr;
int                 global_file;

{
     int            i;
     int            j;
     struct LINK    *link_ptr;
     char           temp_path[400];
     char           temp_str[40];
     char           links_file_path[400];     
     FILE           *links_file;
     char           *name_type;

     if (dirlist_preamble_ptr -> nlinks == 0)
          return;

     /* Print links information */
     display_dirlist_preamble (preamble_ptr, dirlist_preamble_ptr);

     name_type = retrieval_options_ptr ->
          attr_cv_values[get_keyword_values_index (attr_cv_types, "name_type")];

     /* Make pathname for file mxload.links in new directory (if local
        listing)  or working directory if global listing */
     if (global_file)
          links_file = fopen ("mxload.links", "a");
     else
          {
          if (make_new_path_fixed_name (preamble_ptr -> dname, "mxload.links",
               retrieval_options_ptr, name_type, links_file_path) != 0)
               return;
          links_file = fopen (links_file_path, "a");
     }

     /* File links information */
     if (links_file == NULL)
          return;

     /* Write line of form:
        '<multics-name>' '<unix-name>' '<link-target-1>' '<add-name-1>' ...
        with any single quotes transformed into '"'"'.  
        For global file use full pathnames; for flat reload use full
        Multics path, just entry for Unix path; else just entryname. */

     for (i = 0; i < dirlist_preamble_ptr -> nlinks; ++i)
          {
          link_ptr = dirlist_preamble_ptr -> links + i;
          if (global_file || retrieval_options_ptr -> reload[0] == 'f')
               write_quoted_string (links_file,
                    strcat (strcat (strcpy (temp_str, preamble_ptr -> dname),
                         ">"), link_ptr -> ename));
          else write_quoted_string (links_file, link_ptr -> ename);
          fprintf (links_file, " ");
          make_new_path (preamble_ptr -> dname, link_ptr -> ename,
               retrieval_options_ptr, "", name_type, temp_path);
          if (global_file)
               write_quoted_string (links_file, temp_path);
          else
               {
               get_entryname (temp_path, temp_str);
               write_quoted_string (links_file, temp_str);
          }
          fprintf (links_file, " ");
          write_quoted_string (links_file, link_ptr -> target);
          for (j = 0; j < link_ptr -> naddnames; ++j)
               {
               fprintf (links_file, " ");
               write_quoted_string (links_file, link_ptr -> addnames + 33 * j);
          }
          fprintf (links_file, "\n");
     }
     fclose (links_file);

     /* Free link information */
     for (i = 0; i < dirlist_preamble_ptr -> nlinks; ++i)
          {
          if ((dirlist_preamble_ptr -> links + i) -> addnames != NULL)
               free ((dirlist_preamble_ptr -> links + i) -> addnames);
     }
     free ((char *) dirlist_preamble_ptr -> links);
}

/* Print usage message */
static void usage ()
{
     fprintf (stderr, "Usage: mxload [-options] dump_file ");
     fprintf (stderr, 
#ifdef MSDOS     
          "[\"MXpath1\" DOSpath1] [\"MXpath2\" DOSpath2 ...]\n");
#else
          "['Mpath1' Upath1] ['Mpath2' Upath2 ...]\n");
#endif
     fprintf (stderr, "\nOptions are:\n");
     fprintf (stderr, "-b\t\t= brief; suppress no-match warnings\n");
     fprintf (stderr, "-c control_file\t= control file; ");
     fprintf (stderr, "may be specified more than once\n");
     fprintf (stderr,
          "-g map_file\t= global map file; direct map to a file\n");
     fprintf (stderr, "-l\t\t= local map files; ");
     fprintf (stderr, "direct map to files in target directories\n");
     fprintf (stderr, "-n\t\t= no map\n");
     fprintf (stderr, "-v\t\t= verbose; produce long map\n");
     fprintf (stderr, "-x\t\t= extremely verbose; produce full map\n");
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

     preconverted_contents_file =
          get_temp_file ("wt", "preconverted file contents");
     contents_file = get_temp_file ("wt", "raw file contents");
     if (contents_file == NULL || preconverted_contents_file == NULL)
          return;

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
               if (seg_type_by_name != NULL &&
                    strcmp (seg_type_by_name, "ascii_archive") == 0)
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
                    seg_type_by_content = get_type_by_content (contents_file,
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
               rdseg (infile, NULL, preamble_ptr); /* If seg has not already
                                                      been read, skip it */
          goto EXIT;
     }

     if (!seg_has_been_read &&
          rdseg (infile, contents_file, preamble_ptr) != 0)
          {
          goto EXIT;
     }
     if (strcmp (conversion_type, "unpack") != 0 && 
           strcmp (conversion_type, "repack") != 0) {
	  if (mxlargs.local_map)   
		  open_local_map (preamble_ptr -> dname, retrieval_options_ptr);
          display_branch_preamble (preamble_ptr, branch_preamble_ptr);
     }
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
     release_temp_file (preconverted_contents_file, "preconverted file contents");
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

     if (strcmp (conversion_type, "unpack") != 0) {
	  if (mxlargs.local_map)   
		  open_local_map (preamble_ptr -> dname, retrieval_options_ptr);
          display_branch_preamble (preamble_ptr, branch_preamble_ptr);
     }
     convert_seg (NULL, contents_file, preamble_ptr, branch_preamble_ptr,
          seg_type, conversion_type, retrieval_options_ptr);
}

/* Same as make_new_path, but returns fixed entryname, not one guaranteed
   to be unique by tacking #1, etc. on it the way make_new_path does*/

static int make_new_path_fixed_name (dname, ename, retrieval_options_ptr, 
       name_type, new_path)

char                *dname;
char                *ename;
struct MXLOPTS *retrieval_options_ptr;
char                *name_type;
char                *new_path;

{
     char           temp_str[400];

     if (make_new_path (dname, ename, retrieval_options_ptr, "", name_type,
          temp_str) != 0)
          return (-1);
     get_directory (temp_str, new_path);
     strcat (new_path, DIR_SEPARATOR_STRING);
     strcat (new_path, ename);
     return (0);
}

/* Read segment from temp file and write converted form into new path */

static void convert_seg (contents_file, preconverted_contents_file,
       preamble_ptr, branch_preamble_ptr, seg_type, conversion_type,
       retrieval_options_ptr)

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
     int	    unpacker_status;
     
     name_type = retrieval_options_ptr ->
               attr_cv_values[get_keyword_values_index (attr_cv_types,
               "name_type")];
     ++ retrieval_options_ptr -> n_files_loaded;

     if (strcmp (conversion_type, "unpack") == 0 &&
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
               unpacker_status = mxdearc (contents_file, preamble_ptr, retrieval_options_ptr,
                    branch_preamble_ptr, 0);
          else unpacker_status = mxdearc (preconverted_contents_file, preamble_ptr,
               retrieval_options_ptr, branch_preamble_ptr, 1);
	  if (unpacker_status < 0)
	       {
	        fprintf (stderr,
			 "Archive will be stored using 8bit conversion.\n");
		conversion_type = "8bit";
	  }
	  else return;
     }

     if (strcmp (conversion_type, "unpack") == 0 &&
          strcmp (seg_type, "nonascii_archive") == 0)
	  {
          unpacker_status = mxdearc (contents_file, preamble_ptr, retrieval_options_ptr,
               branch_preamble_ptr, 0);
	  if (unpacker_status < 0)
	       {
	        fprintf (stderr,
			 "Archive will be stored using 9bit conversion.\n");
		conversion_type = "9bit";
	  }
	  else return;
     }

     if (strcmp (conversion_type, "unpack") == 0 &&
          (strcmp (seg_type, "mbx") == 0 || strcmp (seg_type, "ms") == 0))
	  {
          unpacker_status = mxmseg (contents_file, preamble_ptr, retrieval_options_ptr,
               branch_preamble_ptr, 0, seg_type);
	  if (unpacker_status < 0)
	       {
	        fprintf (stderr,
			 "Mailbox will be stored using 9bit conversion.\n");
		conversion_type = "9bit";
	  }
	  else return;
     }

     if (strcmp (conversion_type, "repack") == 0 &&
           strcmp (seg_type, "mbx") == 0)
	  {
          unpacker_status = mxmseg (contents_file, preamble_ptr, retrieval_options_ptr,
               branch_preamble_ptr, 1, seg_type);
	  if (unpacker_status < 0)
	       {
	        fprintf (stderr,
			 "Mailbox will be stored using 9bit conversion.\n");
		conversion_type = "9bit";
	  }
	  else return;
     }

     if (strcmp (conversion_type, "9bit") == 0)
          {
          if (make_new_path (preamble_ptr -> dname, preamble_ptr -> ename,
               retrieval_options_ptr, "", name_type, new_path) != 0)
               return;
	  if (mxlargs.local_map)   
               open_local_map (preamble_ptr -> dname, retrieval_options_ptr);
          display_conversion_info (new_path, seg_type, conversion_type);
          put_in_place (contents_file, new_path, 0,
               (preamble_ptr -> adjusted_bitcnt + 7L) / 8L);
          set_attrs (new_path, retrieval_options_ptr, preamble_ptr,
               branch_preamble_ptr);
	  return;
     }

     if (strcmp (conversion_type, "8bit") == 0)
          {
          if (make_new_path (preamble_ptr -> dname, preamble_ptr -> ename,
               retrieval_options_ptr, "", name_type, new_path) != 0)
               return;
	  if (mxlargs.local_map)   
               open_local_map (preamble_ptr -> dname, retrieval_options_ptr);
          display_conversion_info (new_path, seg_type, conversion_type);
          if (preconverted_contents_file == NULL)
               copy_8bit (contents_file, new_path,
                    preamble_ptr -> adjusted_bitcnt);
          else put_in_place (preconverted_contents_file, new_path, 1,
               preamble_ptr -> adjusted_bitcnt / 9L);
          set_attrs (new_path, retrieval_options_ptr, preamble_ptr,
               branch_preamble_ptr);
	  return;
     }

     if (strcmp (conversion_type, "8bit+9bit") == 0)
          {
          if (make_new_path (preamble_ptr -> dname, preamble_ptr -> ename,
               retrieval_options_ptr, "A", name_type, new_path) != 0)
               return;
	  if (mxlargs.local_map)   
               open_local_map (preamble_ptr -> dname, retrieval_options_ptr);
          display_conversion_info (new_path, seg_type, conversion_type);
          if (preconverted_contents_file == NULL)
               copy_8bit (contents_file, new_path,
                    preamble_ptr -> adjusted_bitcnt);
          else put_in_place (preconverted_contents_file, new_path, 1,
               preamble_ptr -> adjusted_bitcnt / 9L);
          set_attrs (new_path, retrieval_options_ptr, preamble_ptr, branch_preamble_ptr);
          if (make_new_path (preamble_ptr -> dname, preamble_ptr -> ename,
               retrieval_options_ptr, "B", name_type, new_path) != 0)
               return;
          display_conversion_info (new_path, NULL, NULL);
          put_in_place (contents_file, new_path, 0,
               preamble_ptr -> adjusted_bitcnt / 8L);
          set_attrs (new_path, retrieval_options_ptr, preamble_ptr,
               branch_preamble_ptr);
	  return;
     }


     fprintf (stderr,
          "Conversion type %s is not yet implemented for segment type %s.\n",
          conversion_type, seg_type);
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
     /* For MSDOS, renaming the file before closing it doesn't work.
        Besides, on MSDOS we have to make sure that ascii files go through
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
          attr_cv_values[get_keyword_values_index (attr_cv_types,"name_type")];

     /* Set the access time if requested */
     access_time_action = retrieval_options_ptr ->
          attr_cv_values[get_keyword_values_index (attr_cv_types,
          "access_time")];
     if (strcmp (access_time_action, "dtu") == 0)
          utime_struct.actime = branch_preamble_ptr -> dtu;
     else utime_struct.actime = time (NULL);

     /* Set the modification time if requested */
     mod_time_action = retrieval_options_ptr -> attr_cv_values[get_keyword_values_index (attr_cv_types, "modification_time")];
     if (strcmp (mod_time_action, "dtcm") == 0)
          utime_struct.modtime = branch_preamble_ptr -> dtm;
     else utime_struct.modtime = time (NULL);

     utime (new_path, &utime_struct);             /* The actual time-setting */
     /* Put the ACL in a file if requested */
     acl_action = retrieval_options_ptr ->
          list_values[get_keyword_values_index (list_types, "acl")];
     if (strcmp (acl_action, "global") == 0)
          store_acl (new_path, retrieval_options_ptr, preamble_ptr,
               branch_preamble_ptr, name_type, 1);
     else if (strcmp (acl_action, "local") == 0)
          store_acl (new_path, retrieval_options_ptr, preamble_ptr,
               branch_preamble_ptr, name_type, 0);

     /* Determine personid.projectid of segments "owner" */
     group_action = retrieval_options_ptr ->
          attr_cv_values[get_keyword_values_index (attr_cv_types, "group")];
     owner_action = retrieval_options_ptr ->
          attr_cv_values[get_keyword_values_index (attr_cv_types, "owner")];
     if (group_action[0] == 'b')
          get_project (branch_preamble_ptr -> bitcount_author, project_id);
     else get_project (branch_preamble_ptr -> author, project_id);
     if (owner_action[0] == 'b')
          get_person (branch_preamble_ptr -> bitcount_author, person_id);
     else get_person (branch_preamble_ptr -> author, person_id);

     /* Set permission bits according to ACL if requested */
     if (strcmp (retrieval_options_ptr -> access, "acl") == 0)
          set_permission_bits (new_path, retrieval_options_ptr, project_id,
          person_id, preamble_ptr, branch_preamble_ptr);

     /* Put the addnames in a file if requested */
     addname_action = retrieval_options_ptr ->
          list_values[get_keyword_values_index (list_types, "addname")];
     if (strcmp (addname_action, "global") == 0)
          store_addnames (new_path, retrieval_options_ptr, preamble_ptr,
          branch_preamble_ptr, name_type, 1);
     else if (strcmp (addname_action, "local") == 0)
          store_addnames (new_path, retrieval_options_ptr, preamble_ptr,
               branch_preamble_ptr, name_type, 0);

#ifndef MSDOS
     /* Translate the owner and group names if requested */
     if (retrieval_options_ptr -> user_translations != NULL)
          set_owner (new_path, retrieval_options_ptr -> user_translations,
          project_id, person_id);
#endif
}

static void set_permission_bits (new_path, retrieval_options_ptr, project_id,
       person_id, preamble_ptr, branch_preamble_ptr)
     
char                *new_path;
struct MXLOPTS      *retrieval_options_ptr;
char                *project_id;
char                *person_id;
struct PREAMBLE     *preamble_ptr;
struct BRANCH_PREAMBLE *branch_preamble_ptr;

{
     unsigned char  owner_mode;
     unsigned char  group_mode;
     unsigned char  world_mode;
     int            unix_mode;
     char           owner_access_name[20];
     char           group_access_name[20];
     int            i;
     struct ACL     *acl_ptr;
     strcpy (group_access_name, "*.");
     strcat (group_access_name, project_id);
     strcat (group_access_name, ".*");

     strcpy (owner_access_name, person_id);
     strcat (owner_access_name, ".");

     owner_mode = group_mode = world_mode = 0;
     for (i = 0; i < branch_preamble_ptr -> nacl; ++i)
          {
          acl_ptr = branch_preamble_ptr -> acl + i;
          if (strncmp (acl_ptr -> access_name, owner_access_name,
               strlen (owner_access_name)) == 0)
               owner_mode |= acl_ptr -> mode[0] >> 5;
          else if (strcmp (acl_ptr -> access_name, group_access_name) == 0)
               group_mode = acl_ptr -> mode[0] >> 5;
          else if (strcmp (acl_ptr -> access_name, "*.*.*") == 0)
               world_mode = acl_ptr -> mode[0] >> 5;
     }
     /* Convert Multics modes in rew order to Unix modes in rwx order */
     owner_mode = (owner_mode & 0x04) | ((owner_mode & 0x02) >> 1) |
          ((owner_mode & 0x01) << 1);
     group_mode = (group_mode & 0x04) | ((group_mode & 0x02) >> 1) | 
          ((group_mode & 0x01) << 1);
     world_mode = (world_mode & 0x04) | ((world_mode & 0x02) >> 1) | 
          ((world_mode & 0x01) << 1);
     /* Bubble up world to group and owner, group to owner */
     group_mode |= world_mode;
     owner_mode |= group_mode;

     unix_mode = owner_mode << 6 | group_mode << 3 | world_mode;
     chmod (new_path, unix_mode);
     if (mxlargs.verbose || mxlargs.extremely_verbose)
          fprintf (mxlargs.map_file,
               "%19sPermission bits: %.3o\n", "", unix_mode);
}
#ifndef MSDOS

static void set_owner (new_path, xlation_ptr, project_id, person_id)

char                *new_path;
struct USER_TRANSLATION *xlation_ptr;
char                *project_id;
char                *person_id;

{
     int            chown_result;
     char           owner_name[24];
     char           group_name[24];
     int            uid;
     int            gid;
     static char    my_name[24] = "unknown";
     static char    my_group[24] = "unknown";
     static int     my_uid = -1;
     static int     my_gid = -1;
     struct passwd  *pwp;
     struct group   *grp;

     if (xlation_ptr == NULL)
          return;

     if (my_uid == -1)                  /* Init info about this process */
          {
          pwp = getpwuid (getuid());
          if (pwp != NULL)
               {
               my_uid = pwp -> pw_uid;
               my_gid = pwp -> pw_gid;
               strcpy (my_name, pwp -> pw_name);
          }
          grp = getgrgid (my_gid);
          if (grp != NULL)
               strcpy (my_group, grp -> gr_name);
     }
     gid = uid = -1;
     group_name[0] = owner_name[0] = '\0';
     while (xlation_ptr != NULL)
          {
          if (xlation_ptr -> is_group)
               {
               if (strcmp (project_id, xlation_ptr -> multics_name) == 0 || 
                    (gid == -1 && strcmp ("(other)",
                    xlation_ptr -> multics_name) == 0))
                    {
                    if (strcmp ("(process)", xlation_ptr -> unix_name) == 0)
                         {
                         strcpy (group_name, my_group);
                         gid = my_gid;
                    }
                    else
                         {
                         strcpy (group_name, xlation_ptr -> unix_name);
                         gid = xlation_ptr -> id;
                    }
               }
          }
          else
               {
               if (strcmp (person_id, xlation_ptr -> multics_name) == 0 || 
                    (uid == -1 &&
                    strcmp ("(other)", xlation_ptr -> multics_name) == 0))
                    {
                    if (strcmp ("(process)", xlation_ptr -> unix_name) == 0)
                         {
                         strcpy (owner_name, my_name);
                         uid = my_uid;
                    }
                    else
                         {
                         strcpy (owner_name, xlation_ptr -> unix_name);
                         uid = xlation_ptr -> id;
                    }
               }
          }
          xlation_ptr = xlation_ptr -> next;
     }
          
     if (uid != -1 || gid != -1)
          chown_result = chown (new_path, uid, gid);
     if (chown_result != 0)
       {
           perror (new_path);
           fprintf (stderr, "Error setting owner/group to %s(%d)/%s(%d).\n",
                      owner_name, uid, group_name, gid);
       }
           
     if (mxlargs.verbose || mxlargs.extremely_verbose)
          {
          if (owner_name[0] =='\0')
               strcpy (owner_name, my_name);
          if (uid == -1)
               uid = my_uid;
          if (group_name[0] == '\0')
               strcpy (group_name, my_group);
          fprintf (mxlargs.map_file,
               "%19sOwner/Group:     %s(%d)/%s(%d)\n", "",
               owner_name, uid, group_name, gid);
     }
}
#endif

static void get_project (access_id, proj_id)

char                *access_id;
char                *proj_id;
{
     int            i;

     i = 0;
     while (access_id[i++] != '.')
          ;
     strcpy (proj_id, access_id+i);
     i = 0;
     while (proj_id[i++] != '.')
          ;
     proj_id[i-1] = '\0';
}


static void get_person (access_id, pers_id)

char                *access_id;
char                *pers_id;
{
     int            i;

     strcpy (pers_id, access_id);
     i = 0;
     while (pers_id[i++] != '.')
          ;
     pers_id[i-1] = '\0';
}

static void store_acl (new_path, retrieval_options_ptr, preamble_ptr,
       branch_preamble_ptr, name_type, global_file)

char                *new_path;
struct MXLOPTS      *retrieval_options_ptr;
struct PREAMBLE     *preamble_ptr;
struct BRANCH_PREAMBLE        *branch_preamble_ptr;
char                *name_type;
int                 global_file;

{
     int            i;
     struct ACL     *acl_ptr;
     char           acl_file_path[400];
     char           temp_str[170];
     static FILE    *acl_file = NULL;
     static char    last_dir[170] = "";
     
     /* Make pathname for file mxload.acl in new directory 
        (if local directory or subtree) or new_path directory */
     
     if (global_file)
          {
          if (acl_file == NULL)
               acl_file = fopen ("mxload.acl", "a");
     }
     else
          {
          if (strcmp (preamble_ptr -> dname, last_dir) != 0)
               {
               if (make_new_path_fixed_name (preamble_ptr -> dname,
                    "mxload.acl", retrieval_options_ptr, name_type,
                    acl_file_path) != 0)
                    return;
               if (acl_file != NULL)
                    fclose (acl_file);
               acl_file = fopen (acl_file_path, "a");
               if (acl_file == NULL)
                    return;
          }
     }
     /* Write line of form:
        '<multics-name>' '<unix-name>' '<ACL-name-1>' <ACL-mode-1> ...
        with any single quotes transformed into '"'"'.  
        For global file use full pathnames; for flat reload use full
        Multics path, just entry for Unix path; else just entryname. */

     if (global_file || retrieval_options_ptr -> reload[0] == 'f')
          write_quoted_string (acl_file,
               strcat (strcat (strcpy (temp_str, preamble_ptr -> dname), ">"),
               preamble_ptr -> ename));
     else write_quoted_string (acl_file, preamble_ptr -> ename);
     fprintf (acl_file, " ");
     if (global_file)
          write_quoted_string (acl_file, new_path);
     else
          {
          get_entryname (new_path, temp_str);
          write_quoted_string (acl_file, temp_str);
     }
     for (i = 0; i < branch_preamble_ptr -> nacl; ++i)
          {
          acl_ptr = branch_preamble_ptr -> acl + i;
          fprintf (acl_file, " %s",
               SHORT_SEG_ACCESS_MODES[acl_ptr -> mode[0] >> 5]);
          fprintf (acl_file, " ");
          write_quoted_string (acl_file, acl_ptr -> access_name);
     }
     fprintf (acl_file, "\n");
}

write_quoted_string (file, string)

FILE                *file;
char                *string;

{
     putc ('\'', file);
     while (*string != '\0')
          {
          if (*string == '\'')
               fprintf (file, "'\"'\"'");
          else putc (*string, file);
          ++string;
     }
     putc ('\'', file);
}

static void store_addnames (new_path, retrieval_options_ptr, preamble_ptr,
       branch_preamble_ptr, name_type, global_file)

char                *new_path;
struct MXLOPTS      *retrieval_options_ptr;
struct PREAMBLE     *preamble_ptr;
struct BRANCH_PREAMBLE        *branch_preamble_ptr;
char                *name_type;
int                 global_file;

{
     int            i;
     char           addname_file_path[400];
     char           temp_str[400];
     static FILE    *addname_file;
     static char    last_dir[170] = "";
     
     if (branch_preamble_ptr -> naddnames == 0)
          return;

     /* Make pathname for file mxload.addname in new directory 
        (if local directory or subtree) or new_path directory */
     
     if (global_file)
          {
          if (addname_file == NULL)
               addname_file = fopen ("mxload.addname", "a");
     }
     else
          {
          if (strcmp (preamble_ptr -> dname, last_dir) != 0)
               {
               if (make_new_path_fixed_name (preamble_ptr -> dname,
                    "mxload.addname", retrieval_options_ptr, name_type,
                    addname_file_path) != 0)
                    return;
               if (addname_file != NULL)
                    fclose (addname_file);
               addname_file = fopen (addname_file_path, "a");
               if (addname_file == NULL)
                    return;
          }
     }
     /* Write line of form:
        '<multics-name>' '<unix-name>' '<add-name-1>' ...
        with any single quotes transformed into '"'"'.  
        For global file use full pathnames; for flat reload use full
        Multics path, just entry for Unix path; else just entryname. */

     if (global_file || retrieval_options_ptr -> reload[0] == 'f')
          write_quoted_string (addname_file,
               strcat (strcat (strcpy (temp_str, preamble_ptr -> dname), ">"),
               preamble_ptr -> ename));
     else write_quoted_string (addname_file, preamble_ptr -> ename);
     fprintf (addname_file, " ");
     if (global_file)
          write_quoted_string (addname_file, new_path);
     else
          {
          get_entryname (new_path, temp_str);
          write_quoted_string (addname_file, temp_str);
     }
     for (i = 0; i < branch_preamble_ptr -> naddnames; ++i)
          {
          fprintf (addname_file, " ");
          write_quoted_string (addname_file,
               branch_preamble_ptr -> addnames + 33 * i);
          }
          fprintf (addname_file, "\n");
}

static void check_file_matches (retrieval_list_ptr)

struct MXLOPTS *retrieval_list_ptr;

{
     int            n_warnings;
     
     n_warnings = 0;
     while (retrieval_list_ptr != NULL)
          {
          if (retrieval_list_ptr -> n_files_matched == 0)
               {
               if (n_warnings++ == 0)
                    fprintf (stderr, "\n");
               fprintf (stderr,
                    "No files found to match statement\n\t%s %s;\n",
               retrieval_list_ptr -> path_type, retrieval_list_ptr -> path);
          }
          retrieval_list_ptr = retrieval_list_ptr -> next;
     }
}

static void open_local_map (dir, retrieval_options_ptr)

char 	*dir;
struct MXLOPTS      *retrieval_options_ptr;

{
     char           local_map_path[400];
     static char    last_dir[170] = "";
     char	    *name_type;

     name_type = retrieval_options_ptr ->
          attr_cv_values[get_keyword_values_index (attr_cv_types, "name_type")];

     if (strcmp (dir, last_dir) != 0)
        {
        make_new_path_fixed_name (dir,
            "mxload.map", retrieval_options_ptr, name_type,
            local_map_path);
        if (mxlargs.map_file != NULL)
            fclose (mxlargs.map_file);
        mxlargs.map_file = fopen (local_map_path, "a");
	if (mxlargs.map_file == NULL)
	    mxlargs.map_file = stdout;
        strcpy (last_dir, dir);              
     }
}
