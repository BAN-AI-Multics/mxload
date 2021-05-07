 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

 /* CVPATH */

 /*
  * Program to convert a file pathname into a valid one for the system
  * at hand.
  *
  * Input path has been converted to Unix or MSDOS format by replacing
  * '>' characters by '/' (Unix) or '\' (MSDOS), and by replacing all
  * NULLs and '/' or '\' characters by '-'. Double quotes are changed
  * here to dashes. In Unix names single quotes are changed to dashes.
  * In MS-DOS names colons are changed to dashes. The add_chars parameter
  * specifies an identifying string to be added to the next-to-last
  * component of the file name, with a '#' character preceding it.
  *
  * First find out if all dirs in path exist. For those that don't exist,
  * convert the entryname to the appropriate format (UNIX, BSD, MSDOS) and
  * create them.  For the name of the file itself, convert it to the
  * appropriate format and see if something already exists with that name.
  * If so, try again after changing the add_chars string. For cases where
  * a file exists already with the name needed for a directory, handle
  * like filename conflicts.
  *
  * If path cannot be created, print error message and return non-zero
  * code.
  */

#include "mxload.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "cvpath.h"
#include "dirsep.h"
#include "mxlopts.h"

static void get_existing_dir();
static  int make_dirs();
static void change_names();
static void make_msdos_name();
static void make_cms_name();
static void make_bsd_name();
static void make_sysv_name();
static void tack_on_chars();
static void make_unique_entryname();
static  int make_or_find_renamed_dir();
static void add_entryname();

#ifdef ANSI_FUNC
int 
cvpath (char *path, char *add_chars, char *type, char *new_path)
#else
int
cvpath(path, add_chars, type, new_path)

char *path;
char *add_chars;
char *type;
char *new_path;
#endif

{
  char *p;
  char temp_path1[400];
  char temp_path2[400];
   int make_dirs_result;
  char entryname[34];
  char unique_entryname[40];

  /*
   * Convert all names in path to 
   * appropriate format for system type
   */

  change_names(path, type, temp_path1);

  /*
   * Create (or find already existing)
   * directory to contain file
   */

  make_dirs_result = make_dirs(temp_path1, temp_path2, type);
  if (make_dirs_result != 0)
  {
    return ( make_dirs_result );
  }

  /*
   * Add extra characters to file name and
   * change name of file to make it unique
   */

  get_directory(temp_path2, new_path);
  get_entryname(temp_path2, entryname);

  make_unique_entryname(
    new_path,
    entryname,
    unique_entryname,
    add_chars,
    type);
  add_entryname(new_path, unique_entryname);
  return ( 0 );
}

 /*
  * Determine new path and
  * create directories.
  */

#ifdef ANSI_FUNC
int 
make_new_path (char *dname, char *ename,
                struct MXLOPTS *retrieval_options_ptr, char *add_chars,
                char *name_type, char *new_path)
#else
int
make_new_path(dname, ename, retrieval_options_ptr, add_chars, name_type,
                  new_path)

char *dname;
char *ename;
struct MXLOPTS *retrieval_options_ptr;
char *add_chars;
char *name_type;
char *new_path;
#endif

{
  char add_to_path[170];
  int i;
  int path_len;
  char raw_path[170];

  /*
   * For reloads of a single file, just
   * put the file in the file indicated
   * by new_path
   */

  if (strcmp(retrieval_options_ptr->path_type, "file") == 0)
  {
    add_to_path[0] = '\0';
  }

  /*
   * For flat reloads, put all files in
   * directory indicated by new_path.
   */

  else if (strcmp(retrieval_options_ptr->reload, "flat") == 0)
  {
    add_to_path[0] = '\0';
    if (retrieval_options_ptr->new_path[0] != '\0')
    {
      strcat(add_to_path, ">");
    }

    strcat(add_to_path, ename);
  }

  /*
   * Non-flat reloads are
   * more complicated.
   */

  else
  {

        /*
         * Next get part that has to be added to the new_path,
         * e.g.when retrieving subtree ">user_dir_dir>Multics"
         * into "/Multics", a file in
         * ">user_dir_dir>Multics>Homan>lib" has to have
         * ">Homan>lib>" tacked onto "/Multics".
         */

    if (retrieval_options_ptr->new_path[0] != '\0')
    {
      strcpy(add_to_path, ">");
    }
    else
    {
      add_to_path[0] = '\0';
    }

    if (( path_len = strlen(retrieval_options_ptr->path)) < strlen(dname))
    {

          /*
           * Special case
           * retrieval of >
           */

      if (strcmp(retrieval_options_ptr->path, ">") == 0)
      {
        strcat(add_to_path, dname + path_len);
      }
      else
      {
        strcat(add_to_path, dname + path_len + 1);
      }

      strcat(add_to_path, ">");
    }

    strcat(add_to_path, ename);
  }

  /*
   * Fix up pathname by translating the Multics
   * directory separators to slashes. At the same
   * time, any slash characters within the Multics
   * path get translated to dashes.
   */

  for (i = 0; add_to_path[i] != '\0'; ++i)
  {
    if (add_to_path[i] == '>')
    {
      add_to_path[i] = DIR_SEPARATOR;
    }
    else if (add_to_path[i] == DIR_SEPARATOR)
    {
      add_to_path[i] = '-';
    }
  }

  /*
   * Put the part to be added to the path onto
   * the new_path, and call cvpath to get
   * entrynames converted to proper format, name
   * conflicts resolved, and directories created.
   */

  strcpy(raw_path, retrieval_options_ptr->new_path);
  strcat(raw_path, add_to_path);
  return ( cvpath(raw_path, add_chars, name_type, new_path));
}

/*
 * Find the deepest existing directory, then do a
 * mkdir for each non-existent directory in pathname,
 * starting from the last existent directory. If a file
 * is found with name conflicting with a directory, fix
 * by renaming directory or deleting file.
 */

#ifdef ANSI_FUNC
static int 
make_dirs (char *path, char *newpath, char *type)
#else
static int
make_dirs(path, newpath, type)

char *path;
char *newpath;
char *type;
#endif

{
  int inpath_idx;
  char entryname[34];
  char changed_entryname[36];
  char existing_dir[400];
  int mkdir_result;
  char corrected_path[400];
  int dir_found;
  int child;

  get_existing_dir(path, corrected_path);
  inpath_idx = strlen(corrected_path);
  while (path[++inpath_idx] != '\0')
  {
    if (path[inpath_idx] == DIR_SEPARATOR)
    {
      path[inpath_idx] = '\0';
      get_entryname(path, entryname);
      path[inpath_idx] = DIR_SEPARATOR;
      dir_found = make_or_find_renamed_dir(
        corrected_path,
        entryname,
        changed_entryname,
        type);
      add_entryname(corrected_path, changed_entryname);
      if (!dir_found)
      {
#ifdef SVR2
        child = fork();
        if (child == 0)
        {
          execl("/bin/mkdir", "mkdir", corrected_path, 0);
          exit(-1);
        }
        else if (wait(&mkdir_result) != child)
        {
          mkdir_result = -1;
        }

#else  /* ifdef SVR2 */
        mkdir_result = mkdir(corrected_path, 0777);
#endif /* ifdef SVR2 */
        if (mkdir_result != 0)
        {
          strcpy(newpath, corrected_path);
          fprintf(
            stderr,
            "Unable to create directory %s for file %s\n",
            newpath,
            path);
          return ( mkdir_result );
        }
      }
    }
  }
  get_entryname(path, entryname);
  add_entryname(corrected_path, entryname);
  strcpy(newpath, corrected_path);
  return ( 0 );
}

 /*
  * Return path of deepest
  * existing directory in path
  */

#ifdef ANSI_FUNC
static void 
get_existing_dir (char *path, char *deepest_dir)
#else
static void
get_existing_dir(path, deepest_dir)

char *path;
char *deepest_dir;
#endif

{
  int i;
  int stat_result;
  struct stat statinfo;

  if (path[0] == DIR_SEPARATOR)
  {
    i = 0;
    strcpy(deepest_dir, DIR_SEPARATOR_STRING);
  }

  else
  {
    i = -1;
    deepest_dir[0] = '\0';
  }

  while (path[++i] != '\0')
  {
    if (path[i] == DIR_SEPARATOR)
    {
      path[i] = '\0';
      stat_result = stat(path, &statinfo);
      if (stat_result == 0 && ( statinfo.st_mode & S_IFDIR ))
      {
        strcpy(deepest_dir, path);
      }
      else
      {
        path[i] = DIR_SEPARATOR;
        return;
      }

      path[i] = DIR_SEPARATOR;
    }
  }
}

/*
 * Change all names in path to
 * proper form for system type
 */

#ifdef ANSI_FUNC
static void 
change_names (char *path, char *type, char *corrected_path)
#else
static void
change_names(path, type, corrected_path)

char *path;
char *type;
char *corrected_path;
#endif

{
  int i;
  char entryname[34];
  int entryname_len;
  char corrected_entryname[34];

  if (strcmp(type, "BSD") == 0)
  {
    strcpy(corrected_path, path);
    return;
  }

  i = 0;

  while (path[i] == DIR_SEPARATOR || path[i] == '.')
  {
    corrected_path[i] = path[i];
    ++i;
  }

  corrected_path[i] = '\0';

  entryname[entryname_len = 0] = '\0';
  while (1)
  {
    if (path[i] == DIR_SEPARATOR || path[i] == '\0')
    {
      entryname[entryname_len++] = '\0';
      if (strcmp(type, "MSDOS") == 0)
      {
        make_msdos_name(entryname, corrected_entryname, "");
      }
      else if (strcmp(type, "SysV") == 0)
      {
        make_sysv_name(entryname, corrected_entryname, "");
      }
      else if (strcmp(type, "BSD") == 0)
      {
        make_bsd_name(entryname, corrected_entryname, "");
      }
      else if (strcmp(type, "CMS") == 0)
      {
        make_cms_name(entryname, corrected_entryname, "");
      }

      strcat(corrected_path, corrected_entryname);
      if (path[i] == '\0')
      {
        return;
      }

      strcat(corrected_path, DIR_SEPARATOR_STRING);
      entryname[entryname_len = 0] = '\0';
    }
    else
    {
      entryname[entryname_len++] = path[i];
    }

    ++i;
  }
}

 /* 
  * Convert to 8+3 character
  * MS-DOS entryname
  */

#ifdef ANSI_FUNC
static void 
make_msdos_name (char *entryname, char *corrected_entryname, char *addchars)
#else
static void
make_msdos_name(entryname, corrected_entryname, addchars)

char *entryname;
char *corrected_entryname;
char *addchars;
#endif

{
  int i;
  int extension_len;
  char name[9];
  char extension[4];
  char temp_entryname[35];

  strcpy(temp_entryname, entryname);
  strcat(temp_entryname, "..");
  memset(name, 0, sizeof ( name ));
  i = 0;

  /*
   * Take first 8 characters
   * of first component
   */

  while (temp_entryname[i] != '.')
  {
    if (i < 8)
    {
      name[i] = temp_entryname[i];
    }

    ++i;
  }

  /*
   * If first component is empty,
   * make it a dash. This also handles
   * "." and ".."
   */

  if (i == 0)
  {
    name[i++] = '-';
  }

  /* 
   * Put addchars at end of
   * first component
   */

  if (addchars[0] != '\0')
  {
    name[8 - strlen(addchars) - 1] = '\0';
    strcat(name, "#");
    strcat(name, addchars);
  }

  /* 
   * Get 3-letter
   * extenson
   */

  extension_len = 0;
  memset(extension, 0, sizeof ( extension ));
  ++i;
  while (temp_entryname[i] != '.' && extension_len < 3)
  {
    extension[extension_len] = temp_entryname[i];
    ++i;
    ++extension_len;
  }
  strcpy(corrected_entryname, name);
  if (extension_len > 0)
  {
    strcat(corrected_entryname, ".");
    strcat(corrected_entryname, extension);
  }

  /* 
   * Uppercase the final result and
   * get rid of double quotes and colons
   */

  for (i = 0; corrected_entryname[i] != '\0'; ++i)
  {
    if (corrected_entryname[i] == '"' || corrected_entryname[i] == ':')
    {
      corrected_entryname[i] = '-';
    }
    else if (islower(corrected_entryname[i]))
    {
      corrected_entryname[i] = toupper(corrected_entryname[i]);
    }
  }
}

 /*
  * Convert to 8+8 character
  * CMS entryname
  */

#ifdef ANSI_FUNC
static void 
make_cms_name (char *entryname, char *corrected_entryname, char *addchars)
#else
static void
make_cms_name(entryname, corrected_entryname, addchars)

char *entryname;
char *corrected_entryname;
char *addchars;
#endif

{
  int i;
  int extension_len;
  char name[9];
  char extension[4];
  char temp_entryname[35];

  strcpy(temp_entryname, entryname);
  strcat(temp_entryname, "..");
  memset(name, 0, sizeof ( name ));
  i = 0;

  /*
   * Take first 8 characters
   * of first component
   */

  while (temp_entryname[i] != '.')
  {
    if (i < 8)
    {
      name[i] = temp_entryname[i];
    }

    ++i;
  }

  /*
   * Put addchars at end 
   * of first component
   */

  if (addchars[0] != '\0')
  {
    name[8 - strlen(addchars) - 1] = '\0';
    strcat(name, "#");
    strcat(name, addchars);
  }

  /* 
   * Get 8-letter
   * extenson
   */

  extension_len = 0;
  memset(extension, 0, sizeof ( extension ));
  ++i;
  while (temp_entryname[i] != '.' && extension_len < 8)
  {
    extension[extension_len] = temp_entryname[i];
    ++i;
    ++extension_len;
  }
  strcpy(corrected_entryname, name);
  if (extension_len > 0)
  {
    strcat(corrected_entryname, ".");
    strcat(corrected_entryname, extension);
  }

  /*
   * Uppercase the
   * final result
   */

  for (i = 0; corrected_entryname[i] != '\0'; ++i)
  {
    if (islower(corrected_entryname[i]))
    {
      corrected_entryname[i] = toupper(corrected_entryname[i]);
    }
  }
}

/* 
 * Convert to 14-character
 * UNIX entryname
 */

#ifdef ANSI_FUNC
static void 
make_sysv_name (char *entryname, char *corrected_entryname, char *addchars)
#else
static void
make_sysv_name(entryname, corrected_entryname, addchars)

char *entryname;
char *corrected_entryname;
char *addchars;
#endif

{
  int i;

  strcpy(corrected_entryname, entryname);
  if (addchars[0] != '\0')
  {
    corrected_entryname[14 - strlen(addchars) - 1] = '\0';
    strcat(corrected_entryname, "#");
    strcat(corrected_entryname, addchars);
  }

  /* 
   * Get rid
   * of quotes
   */

  for (i = 0; corrected_entryname[i] != '\0'; ++i)
  {
    if (corrected_entryname[i] == '\'' || corrected_entryname[i] == '"')
    {
      corrected_entryname[i] = '-';
    }
  }
}

 /*
  * Convert to BSD UNIX entryname
  * (no real conversion necessary)
  */

#ifdef ANSI_FUNC
static void 
make_bsd_name (char *entryname, char *corrected_entryname, char *addchars)
#else
static void
make_bsd_name(entryname, corrected_entryname, addchars)

char *entryname;
char *corrected_entryname;
char *addchars;
#endif

{
  int i;

  strcpy(corrected_entryname, entryname);
  if (addchars[0] != '\0')
  {
    strcat(corrected_entryname, "#");
    strcat(corrected_entryname, addchars);
  }

  for (i = 0; corrected_entryname[i] != '\0'; ++i)
  {
    if (corrected_entryname[i] == '\'' || corrected_entryname[i] == '"')
    {
      corrected_entryname[i] = '-';
    }
  }
}

#ifdef ANSI_FUNC
void 
get_entryname (char *path, char *entry)
#else
void
get_entryname(path, entry)

char *path;
char *entry;
#endif

{
  int i;

  i = strlen(path) - 1;
  while (i >= 0 && path[i] != DIR_SEPARATOR)
  {
    --i;
  }

  if (i < 0)
  {
    strcpy(entry, path);  /* No dir separators, copy back original */
  }
  else
  {
    strcpy(entry, path + i + 1);
  }
}

#ifdef ANSI_FUNC
void 
get_directory (char *path, char *directory)
#else
void
get_directory(path, directory)

char *path;
char *directory;
#endif

{
  int i;

  i = strlen(path) - 1;
  while (i >= 0 && path[i] != DIR_SEPARATOR)
  {
    --i;
  }
  if (i < 0)
  {
    directory[0] = '\0'; /* No dir separators, directory is current dir */
  }
  else
  {
    path[i] = '\0';
    strcpy(directory, path);
    path[i] = DIR_SEPARATOR;
  }
}

#ifdef ANSI_FUNC
static void 
tack_on_chars (char *name, char *chars, char *type)
#else
static void
tack_on_chars(name, chars, type)

char *name;
char *chars;
char *type;
#endif

{
  char corrected_name[36];

  if (strcmp(type, "MSDOS") == 0)
  {
    make_msdos_name(name, corrected_name, chars);
  }
  else if (strcmp(type, "SysV") == 0)
  {
    make_sysv_name(name, corrected_name, chars);
  }
  else
  {
    make_bsd_name(name, corrected_name, chars);
  }

  strcpy(name, corrected_name);
}

#ifdef ANSI_FUNC
static void 
make_unique_entryname (char *dir, char *entryname, char *unique_entryname,
                char *add_chars, char *type)
#else
static void
make_unique_entryname(dir, entryname, unique_entryname, add_chars, type)

char *dir;
char *entryname;
char *unique_entryname;
char *add_chars;
char *type;
#endif

{
  char path[400];
  struct stat statinfo;
  char number_string[8];
  char temp_add_chars[8];
  int number;
  int stat_result;

  strcpy(unique_entryname, entryname);
  tack_on_chars(unique_entryname, add_chars, type);
  strcpy(path, dir);
  add_entryname(path, unique_entryname);
  stat_result = stat(path, &statinfo);

  number = 0;

  /* 
   * Limit of 999 placed here to handle case
   * of using UNIX or BSD Name_type under MS-DOS,
   * which can result in loop due to fact that
   * changing chararacters beyond end of allowed
   * name won't effectively change the name.
   */

  while (stat_result == 0 && number < 999)
  {
    strcpy(unique_entryname, entryname);
    sprintf(number_string, "%d", ++number);
    strcpy(temp_add_chars, add_chars);
    strcat(temp_add_chars, number_string);
    tack_on_chars(unique_entryname, temp_add_chars, type);
    strcpy(path, dir);
    add_entryname(path, unique_entryname);
    stat_result = stat(path, &statinfo);
  }
}

#ifdef ANSI_FUNC
static int 
make_or_find_renamed_dir (char *dir, char *entryname,
                char *changed_entryname, char *type)
#else
static int
make_or_find_renamed_dir(dir, entryname, changed_entryname, type)

char *dir;
char *entryname;
char *changed_entryname;
char *type;
#endif

{
  char path[400];
  struct stat statinfo;
  char number_string[8];
  int number;
  int stat_result;

  strcpy(changed_entryname, entryname);
  tack_on_chars(changed_entryname, "", type);
  strcpy(path, dir);
  add_entryname(path, changed_entryname);
  stat_result = stat(path, &statinfo);

  number = 0;
  while (stat_result == 0 && ( statinfo.st_mode & S_IFREG ) && number < 999)
  {
    strcpy(changed_entryname, entryname);
    sprintf(number_string, "%d", ++number);
    tack_on_chars(changed_entryname, number_string, type);
    strcpy(path, dir);
    add_entryname(path, changed_entryname);
    stat_result = stat(path, &statinfo);
  }
  if (stat_result == 0 && ( statinfo.st_mode & S_IFDIR ))
  {
    return ( 1 );
  }
  else
  {
    return ( 0 );
  }
}

#ifdef ANSI_FUNC
static void 
add_entryname (char *dir, char *entry)
#else
static void
add_entryname(dir, entry)

char *dir;
char *entry;
#endif

{
  if (strlen(dir) > 0 && dir[strlen(dir) - 1] != DIR_SEPARATOR)
  {
    strcat(dir, DIR_SEPARATOR_STRING);
  }

  strcat(dir, entry);
}
