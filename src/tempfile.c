 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "dirsep.h"
#include "mxbitio.h"
#include "mxload.h"

char *getenv();

#define MAX_TEMP_FILES 10
static MXBITFILE *tempfiles[MAX_TEMP_FILES] = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL};
static char temppaths[MAX_TEMP_FILES][100];
static char comments[MAX_TEMP_FILES][32];
static int tempfile_open_and_available[MAX_TEMP_FILES] = {
  0};
static char temp_dir[100] = "'";

 /*
  * Return an open file in /tmp or directory indicated by TMP environment
  * variable.  File is taken from a pool of already open files, if possible.
  * Otherwise it is created and opened.
  */

#ifdef ANSI_FUNC
MXBITFILE *
get_temp_file (char *mode, char *comment)
#else
MXBITFILE *
get_temp_file(mode, comment)

char *mode;
char *comment;
#endif

{
  int i;
  char filename[400];
  char *tmp_env_var;

  /* 
   * Initialize temp dir name
   * if this is first call
   */

  if (strcmp(temp_dir, "'") == 0)
  {
    tmp_env_var = getenv("TMP");
    if (tmp_env_var != NULL && strlen(tmp_env_var) > 0)
    {
      strcpy(temp_dir, tmp_env_var);
      if (temp_dir[strlen(temp_dir) - 1] != DIR_SEPARATOR)
      {
        strcat(temp_dir, DIR_SEPARATOR_STRING);
      }
    }

    else
    {
      strcpy(temp_dir, "/tmp/");
    }
  }

  /* 
   * Look for an open
   * and available file
   */

  for (i = 0; i < MAX_TEMP_FILES; ++i)
  {
    if (tempfile_open_and_available[i])
    {
      tempfile_open_and_available[i] = 0;
      strcpy(comments[i], comment);
      rewind_mxbit_file(tempfiles[i], mode);
      return ( tempfiles[i] );
    }
  }

  /*
   * If none were open and available,
   * pick a free slot in tempfiles and
   * open a new file.
   */

  for (i = 0; i < MAX_TEMP_FILES; ++i)
  {
    if (tempfiles[i] == NULL)
    {
      strcpy(filename, temp_dir);
      strcat(filename, "mxXXXXXX");
#ifdef HAVE_MKSTEMP
      tempfiles[i] = open_mxbit_file(mkstemp(filename), mode);
#else /* ifdef HAVE_MKSTEMP */
          mktemp(filename);
          tempfiles[i] = open_mxbit_file(filename, mode);
#endif /* ifdef HAVE_MKSTEMP */
      if (tempfiles[i] == NULL)
      {
        perror("Unable to open temporary file.\n");
      }

      strcpy(temppaths[i], filename);
      strcpy(comments[i], comment);
      return ( tempfiles[i] );
    }
  }

  return( 0 );
}

/* 
 * Put a temporary file
 * back in the pool
 */

#ifdef ANSI_FUNC
void 
release_temp_file (MXBITFILE *tempfile, char *comment)
#else
void
release_temp_file(tempfile, comment)

MXBITFILE *tempfile;
char *comment;
#endif

{
  int i;

  for (i = 0; i < MAX_TEMP_FILES; ++i)
  {
    if (tempfiles[i] == tempfile)
    {
      tempfile_open_and_available[i] = 1;
      return;
    }
  }
}

/*
 * Take a temporary file out of the pool and
 * replace it by another, usually to make the
 * original a permanent segment. The file is
 * closed when this call returns and there is
 * no longer any way to get at it by its
 * old name or file pointer.
 */

#ifdef ANSI_FUNC
void 
replace_temp_file (MXBITFILE *tempfile)
#else
void
replace_temp_file(tempfile)

MXBITFILE *tempfile;
#endif

{
  int i;
  MXBITFILE *newtempfile;
  char new_filename[400];

  for (i = 0; i < MAX_TEMP_FILES; ++i)
  {
    if (tempfiles[i] == tempfile)
    {
      close_mxbit_file(tempfile);
      strcpy(new_filename, temp_dir);
      strcat(new_filename, "mxXXXXXX");
#ifdef HAVE_MKSTEMP
      newtempfile = open_mxbit_file(mkstemp(new_filename), "wt");
#else /* ifdef HAVE_MKSTEMP */
      mktemp(new_filename);
      newtempfile = open_mxbit_file(new_filename, "wt");
#endif /* ifdef HAVE_MKSTEMP */
      strcpy(temppaths[i], new_filename);
      memcpy(tempfile, newtempfile, sizeof ( MXBITFILE ));
      free(newtempfile);
      return;
    }
  }
}

 /* 
  * Returns the actual pathname
  * of a temporary file, but
  * leaves it in the pool.
  */

#ifdef ANSI_FUNC
char *
temp_file_name (MXBITFILE *tempfile)
#else
char *
temp_file_name(tempfile)

MXBITFILE *tempfile;
#endif

{
  int i;

  for (i = 0; i < MAX_TEMP_FILES; ++i)
  {
    if (tempfiles[i] == tempfile)
    {
      return ( temppaths[i] );
    }
  }

  return ( NULL );
}

 /* 
  * Delete all remaining
  * temporary files
  */

#ifdef ANSI_FUNC
void 
cleanup_temp_files (void)
#else
void
cleanup_temp_files()
#endif
{
  int i;

  for (i = 0; i < MAX_TEMP_FILES; ++i)
  {
    if (tempfiles[i] != NULL)
    {
      close_mxbit_file(tempfiles[i]);
      unlink(temppaths[i]);
      tempfiles[i] = NULL;
    }
  }
}
