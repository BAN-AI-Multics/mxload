#include "mxload.h"
#include <stdio.h>
#include <stdlib.h>
#include "mxbitio.h"

#define _CDECL
static void _CDECL copy_8bit();

#ifdef ANSI_FUNC
int _CDECL
main (int argc, char *argv[])
#else
int _CDECL
main(argc, argv)

int argc;
char *argv[];
#endif

{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s file\n", argv[0]);
    exit(1);
	return( 1 );
  }

  copy_8bit(argv[1]);
  return( 0 );
}

#ifdef ANSI_FUNC
static void _CDECL
copy_8bit (char *contents_filename)
#else
static void _CDECL
copy_8bit(contents_filename)

char *contents_filename;
#endif

{
  char buffer[2];
  MXBITFILE *infile;
  int n_read;

  /*
   * Open as temporary file so EOF
   * does not cause error message
   */

  if (( infile = open_mxbit_file(contents_filename, "rt")) == NULL)
  {
    fprintf(stderr, "Cannot open temp file %s.\n", contents_filename);
    return;
  }

  skip_mxbits(infile, 1L);
  while (1)
  {
    n_read = get_mxbits(infile, 9, buffer);
    if (n_read != 9)
    {
      close_mxbit_file(infile);
      return;
    }

    putc(buffer[0], stdout);
  }
}

#ifdef ANSI_FUNC
void _CDECL
mxlexit (void)
#else
void _CDECL
mxlexit()
#endif
{
  exit(1);
}
