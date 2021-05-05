                  /********************************\
                  *     Copyright (C) 1987 by      *
                  * Cygnus Cybernetics Corporation *
                  *    and Oxford Systems, Inc.    *
                  \********************************/

/* Routine to read an mxload control file and produce a chained list of
   structures describing the files to be loaded and the options that apply 
   to them.  Any errors are reported by writing to stderr and execution
   is aborted by calling mxlexit. 
        
   No claims of prettiness are made for this program.  It would be more
   properly done using yacc, but is done in straight C for portability. */

#include "mxload.h"

#include <stdio.h>
#include <ctype.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#ifdef MSDOS
#include <direct.h>
#include <stdlib.h>
#include <malloc.h>
#else
#include <pwd.h>
#include <grp.h>
char                 *malloc();

struct group        *getgrnam();
struct group        *getgrgid();
struct passwd       *getpwuid();
struct passwd       *getpwnam();
#endif

#include "mxlopts.h"
#include "mxloptsi.h"
#include "parsctl.h"
#include "dirsep.h"

struct token
{
     struct token   *next;
     int            strlen;
     int            is_delimiter;
     char           *str;
};


#ifdef LINT_ARGS
static void new_path_stmt(struct token *headptr,struct MXLOPTS *optptr);
static void path_stmts(struct token *headptr,
        struct MXLOPTS *default_options_ptr,char *stmt_type,
        struct MXLOPTS *file_opt_ptr);
static void copy_options_struct (struct MXLOPTS *to_options_ptr,
       struct MXLOPTS *from_opt_ptr);
static void fake_subtree_and_new_path_stmts (
       struct MXLOPTS *default_options_ptr, struct MXLOPTS *file_opt_ptr,
       char *multics_path, char *unix_path);
static void convert_stmt(struct token *headptr,struct MXLOPTS *optptr);
static void attr_stmt(char *stmt_type,int attr_idx,struct token *headptr,
        struct MXLOPTS *optptr);
static void list_stmt(struct token *headptr,struct MXLOPTS *optptr);
static void force_convert_stmt(struct token *headptr,struct MXLOPTS *optptr);
static void reload_stmt(struct token *headptr,struct MXLOPTS *optptr);
static void access_stmt(struct token *headptr,struct MXLOPTS *optptr);
static void dataend_stmt(struct token *headptr,struct MXLOPTS *optptr);
static void initialize_defaults(struct MXLOPTS *default_options);
static struct token * tokenize_stmt(char *line);
static void free_tokens(struct token *headptr);
static struct token * make_token(char *strptr,int strlen,int is_delim,
        struct token *headptr);
static void statement_must_follow_path_stmt(char *stmt_type);
static void invalid_syntax(char *stmt_type);
static void copy_token_str(char *target_str,int target_str_len,
        struct token *token_ptr);
static int check_tokens(struct token *headptr,int n_tokens_required);
static char * get_next_stmt(struct _iobuf *ctl_file);
static int getc_counting_lines(struct _iobuf *file);
static int ungetc_counting_lines(struct _iobuf *file,int c);
static void display_line_no ();
static void display_statement ();
static struct MXLOPTS * resort_options (struct MXLOPTS * head);
static struct MXLOPTS * add_to_list (struct MXLOPTS * head,
        struct MXLOPTS * opt_ptr);
static struct MXLOPTS * remove_from_list (struct MXLOPTS * head,
        struct MXLOPTS * opt_ptr);
static void user_stmt(struct token *headptr, struct MXLOPTS *optptr,
        char *stmt_type);
#else
static void new_path_stmt();
static void path_stmts ();
static void copy_options_struct ();
static void fake_subtree_and_new_path_stmts ();
static void convert_stmt();
static void attr_stmt();
static void list_stmt();
static void force_convert_stmt ();
static void reload_stmt();
static void access_stmt();
static void dataend_stmt();
static void initialize_defaults ();
static struct token * tokenize_stmt();
static void free_tokens ();
static struct token * make_token ();
static void statement_must_follow_path_stmt ();
static void invalid_syntax ();
static void copy_token_str ();
static int check_tokens ();
static char * get_next_stmt();
static int getc_counting_lines ();
static int ungetc_counting_lines ();
static void display_line_no ();
static void display_statement ();
static struct MXLOPTS * resort_options ();
static struct MXLOPTS *  add_to_list ();
static struct MXLOPTS * remove_from_list ();
static void user_stmt();
#endif


static int          error_count;
static int          cur_line_no;
#define MAX_STMT_LENGTH 200
static char         cur_stmt[MAX_STMT_LENGTH];
static char         *cur_ctl_path;

struct MXLOPTS * parsctl (n_ctl_paths, ctl_paths, n_subtree_paths, subtree_paths, return_defaults)

int                 n_ctl_paths;
char                *ctl_paths[];
int                 n_subtree_paths;
char                *subtree_paths[];
int                 return_defaults; /* Return the default options structure 
                                        instead of structures for the various
                                        paths */

{

     FILE           *ctl_file;
     int            attr_index;
     char           stmt_type[20];
     char           lowercase_stmt_type[20];
     struct MXLOPTS *first_opt_struct;
     struct MXLOPTS *last_opt_struct;
     static struct MXLOPTS default_options; /* Static so it can be passed back
                                               in the return_defaults case */
     struct token   *token_ptr;
     char           *line;
     int            ctl_pathN;

     ctl_pathN = 0;
     ctl_file = NULL;
     initialize_defaults (&default_options);
     error_count = 0;
     first_opt_struct = last_opt_struct = NULL;
     
     while (1)
          {
          line = get_next_stmt (ctl_file);
          if (line == NULL && ctl_pathN < n_ctl_paths)
               {
               if (ctl_file != stdin && ctl_file != (FILE *) NULL)
                    fclose (ctl_file);
               if (strcmp (ctl_paths[ctl_pathN], "-") == 0)
                    ctl_file = stdin;
               else ctl_file = fopen (ctl_paths[ctl_pathN], "r");
               if (ctl_file == NULL)
                    {
                    fprintf (stderr, "Unable to open control file %s.\n",
                         ctl_paths[ctl_pathN]);
                    mxlexit (4);
               }
               cur_ctl_path = ctl_paths[ctl_pathN];
               ++ctl_pathN;
               cur_line_no = 1;
               line = get_next_stmt (ctl_file);
          }
          if (line == NULL)
               break;
          token_ptr = tokenize_stmt (line);
          copy_token_str (stmt_type, sizeof (stmt_type), token_ptr);
          strcpy (lowercase_stmt_type, stmt_type);
          if (isupper (lowercase_stmt_type[0]))
               {
               if (first_opt_struct != NULL)
                    {
                    display_line_no ();
                    fprintf (stderr, "Default statements must precede all ");
                    fprintf (stderr, "directory and subtree statements.\n");
                    display_statement ();
               }
               lowercase_stmt_type[0] = tolower (lowercase_stmt_type[0]);
          }
          if (strcmp (stmt_type, "file") == 0 || 
               strcmp (stmt_type, "directory") == 0 ||
               strcmp (stmt_type, "subtree") == 0)
               {
               if (first_opt_struct == NULL)
                    last_opt_struct = first_opt_struct = 
                         (struct MXLOPTS *) malloc (sizeof (struct MXLOPTS));
               else last_opt_struct = last_opt_struct -> next =
                    (struct MXLOPTS *) malloc (sizeof (struct MXLOPTS));
               memset (last_opt_struct, 0, sizeof (struct MXLOPTS));
               path_stmts (token_ptr, &default_options, stmt_type,
                    last_opt_struct);
          }
          else if (strcmp (lowercase_stmt_type, "convert") == 0)
               convert_stmt (token_ptr,
                    isupper (stmt_type[0]) ? &default_options : last_opt_struct);
          else if (strcmp (lowercase_stmt_type, "list") == 0)
               list_stmt (token_ptr,
                    isupper (stmt_type[0]) ? &default_options : last_opt_struct);
          else if (strcmp (lowercase_stmt_type,
               "person") == 0 || strcmp (lowercase_stmt_type, "project") == 0)
               user_stmt (token_ptr,
                    isupper (stmt_type[0]) ? &default_options : last_opt_struct,
                         lowercase_stmt_type);
          else if (strcmp (lowercase_stmt_type, "new_path") == 0)
               new_path_stmt (token_ptr, last_opt_struct);
          else if (strcmp (lowercase_stmt_type, "reload") == 0)
               reload_stmt (token_ptr,
                    isupper (stmt_type[0]) ? &default_options : last_opt_struct);
          else if (strcmp (lowercase_stmt_type, "dataend") == 0)
               dataend_stmt (token_ptr,
                    isupper (stmt_type[0]) ? &default_options : last_opt_struct);
          else if (strcmp (lowercase_stmt_type, "access") == 0)
               access_stmt (token_ptr,
                    isupper (stmt_type[0]) ? &default_options : last_opt_struct);
          else if (strcmp (lowercase_stmt_type, "force_convert") == 0)
               force_convert_stmt (token_ptr,
                    isupper (stmt_type[0]) ? &default_options : last_opt_struct);
          else if ((attr_index = get_keyword_values_index (attr_cv_types,
               lowercase_stmt_type)) >= 0)
               {
               attr_stmt (stmt_type, attr_index, token_ptr,
                    isupper (stmt_type[0]) ? &default_options : last_opt_struct);
          }
          else 
               {
               display_line_no ();
               fprintf (stderr, "Unrecognized statement type: %s\n", stmt_type);
               display_statement ();
          }
          free_tokens (token_ptr);
     }
     /* Break out when at end of last control file */

     if (return_defaults)
          first_opt_struct = &default_options;
     else 
          {
          /* Do the simulated subtree statements from command-line args */
          while (n_subtree_paths > 0)
               {
               if (first_opt_struct == NULL)
                    last_opt_struct = first_opt_struct =
                         (struct MXLOPTS *) malloc (sizeof (struct MXLOPTS));
               else last_opt_struct = last_opt_struct -> next =
                    (struct MXLOPTS *) malloc (sizeof (struct MXLOPTS));
               memset (last_opt_struct, 0, sizeof (struct MXLOPTS));
               fake_subtree_and_new_path_stmts (&default_options,
                    last_opt_struct, subtree_paths[0], subtree_paths[1]);
               n_subtree_paths -= 2;
               subtree_paths += 2;
          }
          
          if (first_opt_struct == NULL)
               {
               fprintf (stderr,
                    "Error in control file(s).  The control file(s) must\n");
               fprintf (stderr, "contain at least one file, directory or\n");
               fprintf (stderr, "subtree statement.\n");
               ++error_count;
          }
          first_opt_struct = resort_options (first_opt_struct);
     }
     if (error_count > 0)
          mxlexit (4);
     if (ctl_file != (FILE *) NULL)
             fclose (ctl_file);
     return (first_opt_struct);
}

static void new_path_stmt(headptr, optptr)

struct token        *headptr;
struct MXLOPTS      *optptr;

{
     struct token   *path_token_ptr;
     char           temp_path[sizeof (optptr -> new_path)];

     if (optptr == NULL)
          {
          statement_must_follow_path_stmt ("new_path");
          return;
     }

     if (check_tokens (headptr, 3) != 0)
          return;

     path_token_ptr = headptr -> next; /* Point to second token */

     copy_token_str (temp_path, sizeof (temp_path), path_token_ptr);
     strcpy (optptr -> new_path, temp_path);
}

static void path_stmts (headptr, default_options_ptr, stmt_type, file_opt_ptr)

struct token        *headptr;
struct MXLOPTS      *default_options_ptr;
char                *stmt_type;
struct MXLOPTS      *file_opt_ptr;

{
     copy_options_struct (file_opt_ptr, default_options_ptr);

     if (strcmp (stmt_type, "file") == 0)
          file_opt_ptr -> path_type = "file";
     else if (strcmp (stmt_type, "directory") == 0)
          file_opt_ptr -> path_type = "directory";
     else if (strcmp (stmt_type, "subtree") == 0)
          file_opt_ptr -> path_type = "subtree";
     
     if (check_tokens (headptr, 3) != 0)
          return;

     copy_token_str (file_opt_ptr -> path,
          sizeof (file_opt_ptr -> path), headptr -> next);

}

static void fake_subtree_and_new_path_stmts (default_options_ptr,
       file_opt_ptr, multics_path, unix_path)

struct MXLOPTS      *default_options_ptr;
struct MXLOPTS      *file_opt_ptr;
char                *multics_path;
char                *unix_path;

{
     /* Simulate a subtree statement */
     copy_options_struct (file_opt_ptr, default_options_ptr);

     file_opt_ptr -> path_type = "subtree";
     
     /* Simulate a new_path statement */
     strcpy (file_opt_ptr -> path, multics_path);
     strcpy (file_opt_ptr -> new_path, unix_path);
}

static void copy_options_struct (to_options_ptr, from_options_ptr)

struct MXLOPTS      *from_options_ptr;
struct MXLOPTS      *to_options_ptr;


{
     struct USER_TRANSLATION *default_opt_xlation_ptr;
     struct USER_TRANSLATION *file_opt_xlation_ptr;
     struct USER_TRANSLATION *prev_file_opt_xlation_ptr;

     /* Set options structure equal to default options structure, by copying 
        the structure, then the user translations */
     memcpy ((char *) to_options_ptr, (char *) from_options_ptr,
          sizeof (struct MXLOPTS));
     to_options_ptr -> user_translations = NULL;
     default_opt_xlation_ptr = from_options_ptr -> user_translations;
     while (default_opt_xlation_ptr != NULL)
          {
          file_opt_xlation_ptr = (struct USER_TRANSLATION *)
               malloc (sizeof (struct USER_TRANSLATION));
          memcpy ((char *) file_opt_xlation_ptr,
               (char *) default_opt_xlation_ptr,
               sizeof (struct USER_TRANSLATION));
          if (to_options_ptr -> user_translations == NULL)
               to_options_ptr -> user_translations = file_opt_xlation_ptr;
          else prev_file_opt_xlation_ptr -> next = file_opt_xlation_ptr;
          default_opt_xlation_ptr = default_opt_xlation_ptr -> next;
          prev_file_opt_xlation_ptr = file_opt_xlation_ptr;
     }
}

/* Parse convert statements of form 
          "convert segment_type conversion_type;"
   e.g.   "convert archive unpack;"
*/

   
static void convert_stmt(headptr, optptr)

struct token        *headptr;
struct MXLOPTS      *optptr;

{
     struct token   *seg_type_ptr;
     struct token   *conversion_type_ptr;

     char           segment_type[100];
     char           conversion_type[100];
     int            file_idx;
     
     if (optptr == NULL)
          statement_must_follow_path_stmt ("convert");

     if (check_tokens (headptr, 4) != 0)
          return;
     
     seg_type_ptr = headptr -> next; /* Point to second token */

     conversion_type_ptr = seg_type_ptr -> next; /* Point to third token */

     copy_token_str (conversion_type, sizeof (conversion_type),
          conversion_type_ptr);

     if (seg_type_ptr -> strlen >= sizeof (segment_type))
          invalid_syntax ("convert");
     copy_token_str (segment_type, sizeof (segment_type), seg_type_ptr);

     if ((file_idx =
          get_keyword_values_index (file_cv_types, segment_type)) < 0)
          {
          display_line_no ();
          fprintf (stderr, "Invalid file type %s in convert statement.\n",
               segment_type);
          display_statement ();
          return;
     }

     if ((optptr -> file_cv_values[file_idx] =
          get_keyword_value (file_cv_types[file_idx], conversion_type)) == 
          NULL)
          {
          display_line_no ();
          fprintf (stderr, "Invalid conversion type for %s file type:  %s\n",
               segment_type, conversion_type);
          display_statement ();
     }
}

/* Parse list statements of form 
          "list attribute list_type;"
   e.g.   "list acl global;"
*/

   
static void list_stmt(headptr, optptr)

struct token        *headptr;
struct MXLOPTS      *optptr;

{
     struct token   *attr_type_ptr;
     struct token   *list_type_ptr;

     char           attr_type[100];
     char           list_type[100];
     int            list_idx;
     
     if (optptr == NULL)
          statement_must_follow_path_stmt ("list");

     if (check_tokens (headptr, 4) != 0)
          return;
     
     attr_type_ptr = headptr -> next; /* Point to second token */

     list_type_ptr = attr_type_ptr -> next; /* Point to third token */

     copy_token_str (list_type, sizeof (list_type), list_type_ptr);

     if (attr_type_ptr -> strlen >= sizeof (attr_type))
          invalid_syntax ("list");
     copy_token_str (attr_type, sizeof (attr_type), attr_type_ptr);

     if (list_type_ptr -> strlen >= sizeof (list_type))
          invalid_syntax ("list");
     copy_token_str (list_type, sizeof (list_type), list_type_ptr);

     if ((list_idx = get_keyword_values_index (list_types, attr_type)) < 0)
          {
          display_line_no ();
          fprintf (stderr, "Invalid list type %s in list statement.\n",
               attr_type);
          display_statement ();
          return;
     }

     if ((optptr -> list_values[list_idx] =
          get_keyword_value (list_types[list_idx], list_type)) == NULL)
          {
          display_line_no ();
          fprintf (stderr, "Invalid list option:  %s\n", list_type);
          display_statement ();
     }
}

static void attr_stmt(stmt_type, attr_idx, headptr, optptr)

char                *stmt_type;
int                 attr_idx;
struct token        *headptr;
struct MXLOPTS      *optptr;

{
     char           conversion_type[100];

     if (optptr == NULL)
          {
          statement_must_follow_path_stmt (stmt_type);
          return;
     }

     if (check_tokens (headptr, 3) != 0)
          return;

     copy_token_str (conversion_type, sizeof (conversion_type),
          headptr -> next);
     if ((optptr -> attr_cv_values[attr_idx] =
          get_keyword_value (attr_cv_types[attr_idx], conversion_type)) == NULL)
          {
          display_line_no ();
          fprintf (stderr, "Invalid conversion type in %s statement:  %s\n",
               stmt_type, conversion_type);
          display_statement ();
     }
}

static void force_convert_stmt (headptr, optptr)

struct token        *headptr;
struct MXLOPTS      *optptr;

{
     char           force_convert_type[100];

     if (optptr == NULL)
          {
          statement_must_follow_path_stmt ("force_convert");
          return;
     }

     if (check_tokens (headptr, 3) != 0)
          return;

     copy_token_str (force_convert_type, sizeof (force_convert_type),
          headptr -> next);
     if ((optptr -> force_convert =
          get_keyword_value (force_convert_file_cv_types,
          force_convert_type)) == NULL)
          {         
          display_line_no ();
          fprintf (stderr, "Invalid conversion type in force_convert statement:  %s\n",
               force_convert_type);
          display_statement ();
     }
     
}

static void reload_stmt (headptr, optptr)

struct token        *headptr;
struct MXLOPTS      *optptr;

{
     char           reload_type[20];

     if (optptr == NULL)
          {
          statement_must_follow_path_stmt ("reload");
          return;
     }

     if (check_tokens (headptr, 3) != 0)
          return;

     copy_token_str (reload_type, sizeof (reload_type), headptr -> next);
     if (strcmp (reload_type, "heirarchical") == 0)
          optptr -> reload = "heirarchical";
     else if (strcmp (reload_type, "flat") == 0)
          optptr -> reload = "flat";
     else
          {
          display_line_no ();
          fprintf (stderr, "Invalid reload type in reload statement:  %s\n",
               reload_type);
          display_statement ();
     }
}

static void access_stmt (headptr, optptr)

struct token        *headptr;
struct MXLOPTS      *optptr;

{
     char           access_type[20];

     if (optptr == NULL)
          {
          statement_must_follow_path_stmt ("access");
          return;
     }

     if (check_tokens (headptr, 3) != 0)
          return;

     copy_token_str (access_type, sizeof (access_type), headptr -> next);
     if (strcmp (access_type, "acl") == 0)
          optptr -> access = "acl";
     else if (strcmp (access_type, "default") == 0)
          optptr -> access = "default";
     else
          {
          display_line_no ();
          fprintf (stderr, "Invalid access type in access statement:  %s\n",
               access_type);
          display_statement ();
     }
}

static void dataend_stmt (headptr, optptr)

struct token        *headptr;
struct MXLOPTS      *optptr;

{
     char           dataend_type[20];

     if (optptr == NULL)
          {
          statement_must_follow_path_stmt ("dataend");
          return;
     }

     if (check_tokens (headptr, 3) != 0)
          return;

     copy_token_str (dataend_type, sizeof (dataend_type), headptr -> next);
     if (strcmp (dataend_type, "bitcount") == 0)
          optptr -> dataend = "bitcount";
     else if (strcmp (dataend_type, "page_boundary") == 0)
          optptr -> dataend = "page_boundary";
     else
          {
          display_line_no ();
          fprintf (stderr, "Invalid dataend type in dataend statement:  %s\n",
               dataend_type);
          display_statement ();
     }
}

static void user_stmt(headptr, optptr, stmt_type)

struct token        *headptr;
struct MXLOPTS      *optptr;
char                *stmt_type;

{
     struct token   *multics_name_token_ptr;
     struct token   *unix_name_token_ptr;
     struct USER_TRANSLATION *new_user_translation;
     struct USER_TRANSLATION *cur_ptr;
     char           temp_str[40];
#ifndef MSDOS
     struct group   *grp;
     struct passwd  *pwp;
#endif

     if (check_tokens (headptr, 4) != 0)
          return;

     multics_name_token_ptr = headptr -> next; /* Point to second token */
     unix_name_token_ptr = headptr -> next -> next; /* Point to third token */

     new_user_translation = (struct USER_TRANSLATION *) 
          malloc (sizeof (struct USER_TRANSLATION));
     copy_token_str (new_user_translation -> multics_name,
          sizeof (new_user_translation -> multics_name), multics_name_token_ptr);
     new_user_translation -> next = NULL;

     copy_token_str (temp_str, sizeof (temp_str), unix_name_token_ptr);
     if (strspn (temp_str, "0123456789") == strlen (temp_str))
          new_user_translation -> id = atoi (temp_str);
     else
          {
          new_user_translation -> id = -1;
          strcpy (new_user_translation -> unix_name, temp_str);
     }

     if (strcmp (stmt_type, "person") == 0)
          {
          new_user_translation -> is_group = 0;
          if (new_user_translation -> id == -1)
               {
               if (strcmp (new_user_translation -> unix_name,
                    "(process)") != 0)
                    {
#ifdef MSDOS
                    new_user_translation -> id = 0;
#else     
                    pwp = getpwnam (new_user_translation -> unix_name);
                    if (pwp == NULL)
                         {
                         display_line_no ();
                         fprintf (stderr,
                              "User name %s not found in password file.\n",
                              new_user_translation -> unix_name);
                         display_statement ();
                    }
                    else new_user_translation -> id = pwp -> pw_uid;
#endif
               }
          }
          else
               {
#ifdef MSDOS
               strcpy (new_user_translation -> unix_name, "noname");
#else
               pwp = getpwuid (new_user_translation -> id);
               if (pwp == NULL)
                    {
                    display_line_no ();
                    fprintf (stderr, "UID %d not found in password file.\n",
                         new_user_translation -> id);
                    display_statement ();
               }
               else strcpy (new_user_translation -> unix_name, pwp -> pw_name);
#endif
          }
     }
     else           /* "project" statement */
          {
          new_user_translation -> is_group = 1;
          if (new_user_translation -> id == -1)
               {
               if (strcmp (new_user_translation -> unix_name, "(process)") != 0)
                    {
#ifdef MSDOS
                    new_user_translation -> id = 0;
#else
                    grp = getgrnam (new_user_translation -> unix_name);
                    if (grp == NULL)
                         {
                         display_line_no ();
                         fprintf (stderr,
                              "Group name %s not found in group file.\n",
                              new_user_translation -> unix_name);
                         display_statement ();
                    }
                    else new_user_translation -> id = grp -> gr_gid;
#endif
               }
          }
          else
               {
#ifdef MSDOS
               strcpy (new_user_translation -> unix_name, "noname");
#else
               grp = getgrgid (new_user_translation -> id);
               if (grp == NULL)
                    {
                    display_line_no ();
                    fprintf (stderr,
                         "GID %d not found in group file.\n",
                         new_user_translation -> id);
                    display_statement ();
               }
               else strcpy (new_user_translation -> unix_name, grp -> gr_name);
#endif
          }
     }

     /* Thread into chain of translations */
     cur_ptr = optptr -> user_translations;
     if (cur_ptr == NULL)
          optptr -> user_translations = new_user_translation;
     else
          {
          while (cur_ptr -> next != NULL)
               cur_ptr = cur_ptr -> next;
          cur_ptr -> next =  new_user_translation;
     }
}

static void initialize_defaults (default_options)

struct MXLOPTS      *default_options;
{
     int            n_options;
     int            i;
     
     memset ((char *) default_options, 0, sizeof (struct MXLOPTS));

     default_options -> next = NULL;

     n_options = count_keywords (attr_cv_types);
     for (i = 0; i < n_options; ++i)
          default_options -> attr_cv_values[i] = (attr_cv_types[i])[1];

     n_options = count_keywords (list_types);
     for (i = 0; i < n_options; ++i)
          default_options -> list_values[i] = (list_types[i])[1];

     n_options = count_keywords (file_cv_types);
     for (i = 0; i < n_options; ++i)
          default_options -> file_cv_values[i] = (file_cv_types[i])[1];

     default_options -> force_convert = NULL;
     default_options -> access = "default";
     default_options -> reload = "heirarchical";
     default_options -> dataend = "bitcount";
     default_options -> path_type = "subtree";
}

static struct token * tokenize_stmt(line)

char                *line;

{
     int            line_pos;
     int            begin_pos;
     struct token   *first_token;
     
     line_pos = 0;
     first_token = NULL;

     /* White space in front of statement has already been stripped off */

     while (1)
          {
          /* Pick up token */
          begin_pos = line_pos;
          while (isalnum (line[line_pos]) || 
               (ispunct(line[line_pos]) && line[line_pos] != ';'))
               ++line_pos;
     
          if (line_pos != begin_pos)
               first_token = make_token (line+begin_pos,
                    line_pos-begin_pos, 0, first_token);

          /* Skip over white space following token */
          while (isspace (line[line_pos]))
               ++line_pos;

          /* Pick up delimiter, if any */
          if (!isalnum (line[line_pos]) &&
               (!ispunct(line[line_pos]) || line[line_pos] == ';'))
               {
               first_token = make_token (line+line_pos, 1, 1, first_token);
               ++line_pos;
          }
          
          /* Check to see if we're at end of statement */
          if (line[line_pos] == '\0')
               return (first_token);
     }
}

static void free_tokens (headptr)

struct token *headptr;

{
     struct token   *curptr;
     struct token   *nextptr;
     
     curptr = headptr;
     while (curptr != NULL)
          {
          nextptr = curptr -> next;
          free ((char *) curptr);
          curptr = nextptr;
     }
}

static struct token * make_token (strptr, strlen, is_delim, headptr)

char                *strptr;
int                 strlen;
int                 is_delim;
struct token        *headptr;

{
     struct token   *new_token;
     struct token   *curptr;
     
     new_token = (struct token *) malloc (sizeof (struct token));
     new_token -> next = NULL;
     new_token -> strlen = strlen;
     new_token -> is_delimiter = is_delim;
     new_token -> str = strptr;

     if (headptr == NULL)
          return (new_token);

     curptr = headptr;
     while (curptr -> next != NULL)
          curptr = curptr -> next;
     
     curptr -> next = new_token;
     return (headptr);
}

/* Function to search through a keyword_array to find the keyword_values with
   the specified keyword, returning a pointer to the keyword_values. */

char ** get_keyword_values (keyword_array, keyword)

char                **keyword_array[];
char                *keyword;
{

     int            i;
     
     i = 0;
     while (keyword_array[i] != NULL)
          {
          if (strcmp ((keyword_array[i])[0], keyword) == 0)
               return (keyword_array[i]);
          ++i;
     }
     return (NULL);
}

/* Function to search through a keyword_array to find the number of 
   keyword_values */

int count_keywords (keyword_array)

char                **keyword_array[];

{

     int            i;
     
     i = 0;
     while (keyword_array[i++] != NULL)
          ;

     return (i-1);
}

/* Function to search through a keyword_array to find the keyword_values with
   the specified keyword, returning the index of the keyword_values in the
   keyword_array. */

int get_keyword_values_index (keyword_array, keyword)

char                **keyword_array[];
char                *keyword;
{

     int            i;
     
     i = 0;
     while (keyword_array[i] != NULL)
          {
          if (strcmp ((keyword_array[i])[0], keyword) == 0)
               return (i);
          ++i;
     }
     return (-1);
}

/* Function to search through a keyword_values to find the specified value 
   and return a pointer to it, or NULL if it is not found. */

char * get_keyword_value (keyword_values, value)

char                *keyword_values[];
char                *value;
{

     int            i;
     
     i = 1;
     while (keyword_values[i] != NULL)
          {
          if (strcmp (keyword_values[i], value) == 0)
               return (keyword_values[i]);
          ++i;
     }
     return (NULL);
}

static void statement_must_follow_path_stmt (stmt_type)

char                *stmt_type;

{
     display_line_no ();
     fprintf (stderr,
          "%s statement must follow a file, directory or subtree statement.\n",
          stmt_type);
     display_statement ();
}

static void invalid_syntax (stmt_type)

char                *stmt_type;

{
     display_line_no ();
     fprintf (stderr, "Invalid syntax in %s statement.\n", stmt_type);
     display_statement ();
}

static void copy_token_str (target_str, target_str_len, token_ptr)

char                *target_str;
int                 target_str_len;
struct token        *token_ptr;

{
     if (target_str_len <= token_ptr -> strlen)
          token_ptr -> strlen = target_str_len - 1;
     memcpy (target_str, token_ptr -> str, token_ptr -> strlen);
     target_str[token_ptr -> strlen] = '\0';
}

static int check_tokens (headptr, n_tokens_required)

struct token        *headptr;
int                 n_tokens_required;

{
     struct token   *semicolon_ptr;
     int            n_tokens_found;
     char           stmt_type[20];
     
     copy_token_str (stmt_type, sizeof (stmt_type), headptr);
     n_tokens_found = 1;
     semicolon_ptr = headptr;
     while (semicolon_ptr -> next != NULL)
          {
          semicolon_ptr = semicolon_ptr -> next;
          ++n_tokens_found;
     }
     
     if ((n_tokens_found != n_tokens_required) ||
          (semicolon_ptr -> strlen != 1) ||       
          (semicolon_ptr -> str[0] != ';'))
               {
               invalid_syntax (stmt_type);
               return (1);
          }
          
     return (0);
}

/* Gets the next statement.  Ignores whitespace before and after statements,
   as well as comments.
   Returns 0 if line successfully read, non-0 at eof.  Aborts by calling
   mxlexit in case of read error, or file ending in comment. */

static char * get_next_stmt(ctl_file)

FILE                *ctl_file;

{
     int            c;
     int            next_c;
     int            in_statement;
     int            line_pos;
     
     if (ctl_file == NULL)
          return (NULL);
     in_statement = 0;
     line_pos = 0;
     
     while (1)
          {
          c = getc_counting_lines (ctl_file);
          
          /* Test for comment begin and skip to comment end */
          if (c == '/')
               {
               next_c = getc_counting_lines (ctl_file);
               if (next_c == '*')
                    {
                    while (c != '*' || next_c != '/')
                         {
                         c = next_c;
                         next_c = getc_counting_lines (ctl_file);
                         if (next_c == EOF)
                              {
                              display_line_no ();
                              fprintf (stderr,
                                   "Control file ends in a comment.\n");
                              display_statement ();
                         }
                    }
                    c = getc_counting_lines (ctl_file);
               }
               else ungetc_counting_lines (ctl_file, next_c);
          }
          if (isspace (c))
               {
               if (in_statement)
                    cur_stmt[line_pos++] = c;
          }
          else if (c == ';')
               {
               cur_stmt[line_pos++] = c;
               cur_stmt[line_pos] = '\0';
               return (cur_stmt);
          }
          else if (c == EOF)
               {
               cur_stmt[line_pos] = '\0';
               if (in_statement)
                    {
                    display_line_no ();
                    fprintf (stderr,
                         "Last statement does not end with a semicolon.\n");
                    display_statement ();
               }
               return (NULL);

          }
          else 
               {
               cur_stmt[line_pos++] = c;
               in_statement = 1;
          }
          if (line_pos >= MAX_STMT_LENGTH)
               {
               cur_stmt[line_pos-1] = '\0';
               display_line_no ();
               fprintf (stderr, "Control file line is too long.\n");
               mxlexit (4);
          }
     }
}

static int getc_counting_lines (file)

FILE                *file;

{
     int            c;
     
     c = getc (file);
     if (c == '\n')
          ++cur_line_no;
     return (c);
}

static int ungetc_counting_lines (file, c)

FILE                *file;
int                 c;

{
     if (c == '\n')
          --cur_line_no;
     ungetc (c, file);
}

static void display_line_no ()

{
     fprintf (stderr, "Error on line %d of control file %s.\n",
          cur_line_no, cur_ctl_path);
     ++error_count;
}


static void display_statement ()

{
     fprintf (stderr, "\tSOURCE:\t%s\n\n", cur_stmt);
}

/* Put file statements before directory statements
   before subtree statements */

static struct MXLOPTS * resort_options (head)

struct MXLOPTS      *head;

{
     struct MXLOPTS           *cur;
     struct MXLOPTS           *new_head;
     struct MXLOPTS           *next;

     new_head = NULL;

     /* Move file statements to new list */
     cur = head;
     while (cur != NULL)
          {
          next = cur -> next;
          if (cur -> path_type[0] == 'f')
               {
               head = remove_from_list (head, cur);
               new_head = add_to_list (new_head, cur);
          }
          cur = next;
     }

     /* Move directory statements to new list */
     cur = head;
     while (cur != NULL)
          {
          next = cur -> next;
          if (cur -> path_type[0] == 'd')
               {
               head = remove_from_list (head, cur);
               new_head = add_to_list (new_head, cur);
          }
          cur = next;
     }

     /* Move subtree statements to new list */
     cur = head;
     while (cur != NULL)
          {
          next = cur -> next;
          if (cur -> path_type[0] == 's')
               {
               head = remove_from_list (head, cur);
               new_head = add_to_list (new_head, cur);
          }
          cur = next;
     }

     return (new_head);
}

static struct MXLOPTS * remove_from_list (head_ptr, opt_ptr)

struct MXLOPTS                *head_ptr;
struct MXLOPTS                *opt_ptr;

{
     struct MXLOPTS           *cur_ptr;

     if (opt_ptr == head_ptr)
          return (opt_ptr -> next);
     
     cur_ptr = head_ptr;
     while (cur_ptr -> next != NULL)
          {
          if (opt_ptr == cur_ptr -> next)
               {
               cur_ptr -> next = opt_ptr -> next;
               return (head_ptr);
          }
          cur_ptr = cur_ptr -> next;
     }
     return (NULL);
}



static struct MXLOPTS * add_to_list (head_ptr, opt_ptr)

struct MXLOPTS                *head_ptr;
struct MXLOPTS                *opt_ptr;

{
     struct MXLOPTS           *cur_ptr;

     opt_ptr -> next = NULL;
     if (head_ptr == NULL)
          return (opt_ptr);
     
     cur_ptr = head_ptr;
     while (cur_ptr -> next != NULL)
          cur_ptr = cur_ptr -> next;
     cur_ptr -> next = opt_ptr;
     return (head_ptr);
}


