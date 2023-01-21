 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

 /* GETOPT */

#include "mxload.h"
#include <stdio.h>
#include <string.h>
#include "mxbitio.h"
#include "mxlopts.h"
#include "optptr.h"
#include "preamble.h"

 /*
  * Find a file, directory or subtree control
  * file statement that matches the pathname
  * of the segment read from tape.  If found,
  * return a pointer to its retrieval options.
  * If not found, return NULL.
  */

#ifdef ANSI_FUNC
struct MXLOPTS *
get_options_ptr (struct PREAMBLE *preamble_ptr,
                struct MXLOPTS *retrieval_list_ptr)
#else
struct MXLOPTS *
get_options_ptr(preamble_ptr, retrieval_list_ptr)

struct MXLOPTS *retrieval_list_ptr;
struct PREAMBLE *preamble_ptr;
#endif

{
  char full_multics_path[170];

  strcpy(full_multics_path, preamble_ptr->dname);
  if (preamble_ptr->ename[0] != '\0')
  {
    if (full_multics_path[1] != '\0') /* If it's not the ROOT */
    {
      strcat(full_multics_path, ">");
    }

    strcat(full_multics_path, preamble_ptr->ename);
  }

  while (retrieval_list_ptr != NULL)
  {
    if (strcmp(retrieval_list_ptr->path_type, "file") == 0
        && strcmp(retrieval_list_ptr->path, full_multics_path) == 0)
    {
      return ( retrieval_list_ptr );
    }
    else if (strcmp(retrieval_list_ptr->path_type, "directory") == 0
             && strcmp(retrieval_list_ptr->path, preamble_ptr->dname) == 0)
    {
      return ( retrieval_list_ptr );
    }
    else if (strcmp(retrieval_list_ptr->path_type, "subtree") == 0
             && strncmp(
               retrieval_list_ptr->path,
               preamble_ptr->dname,
               strlen(retrieval_list_ptr->path))
             == 0)
    {
      return ( retrieval_list_ptr );
    }

    retrieval_list_ptr = retrieval_list_ptr->next;
  }
  return ( NULL );
}
