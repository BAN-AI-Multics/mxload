#include "mxload.h"

#include <stdio.h>

#include "mxbitio.h"
#define _CDECL
#ifdef LINT_ARGS
static  void _CDECL copy_8bit(char *contents_filename,char *new_path);
#else
static void _CDECL copy_8bit ();
#endif


void _CDECL main (argc, argv)

int argc;
char *argv[];

{
     if (argc != 2)
	{
	fprintf (stderr, "usage:  pr8bit file");
	exit (1);
     }

     copy_8bit (argv[1]);

}
static void _CDECL copy_8bit (contents_filename)

char		*contents_filename;

{
     char          	buffer[2];
     MXBITFILE	*infile;
     int		n_read;

     /* Open as temporary file so EOF does not cause error message */
     if ((infile = open_mxbit_file (contents_filename, "rt")) == NULL) {
	fprintf (stderr, "Cannot open temp file %s.\n", contents_filename);
	return;
     }

     skip_mxbits (infile, 1L);
     while (1)
	{
	n_read = get_mxbits (infile, 9, buffer);
	if (n_read != 9)
	     {
	     close_mxbit_file (infile);
	     return;
	}
	putc (buffer[0], stdout);
#ifdef MSDOS
	if (buffer[0] == '\n')
	     putc ('\015', stdout);
#endif	     
     }
}

void _CDECL mxlexit ()

{
     exit (1);
}
