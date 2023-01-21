 /*
  * This version of getopt is taken from
  * "Proficient C" by Augie Hansen
  */

 /*
  * Copyright (c) 1984, 1985 AT&T
  * All Rights Reserved
  *
  * ----- Author's Note -----
  * getopt() is reproduced with permission
  * of the AT&T UNIX(R) System Toolchest.
  *
  * This is a public domain version of getopt(3)
  * that is distributed to registered Toolchest
  * participants.
  *
  * Defining DOS_MODS alters the code slightly to
  * obtain compatibility with DOS and support
  * libraries provided with most DOS C compilers.
  */

#include <string.h>

 /*
  * 3.0 SID # 1.2
  */

#ifndef NULL
#define NULL 0
#endif /* ifndef NULL */

#define EOF ( -1 )

 /*
  * For this to work under versions of DOS prior
  * to 3.00, argv[0] must be set in main() to
  * point to a valid program name or a reasonable
  * substitute string.  (ARH, 10-8-86)
  */

#define ERR(s, c)                                                           \
  if (opterr)                                                               \
  {                                                                         \
    char errbuf[2];                                                         \
    errbuf[0] = c;                                                          \
    errbuf[1] = '\n';                                                       \
    (void)write( 2, argv[0],    (unsigned)strlen( argv[0] ) );              \
    (void)write( 2,       s,    (unsigned)strlen( s ) );                    \
    (void)write( 2,  errbuf, 2);                                            \
  }

#ifdef DOS_MODS

/*
 * Permit function
 * prototyping under DOS
 */

#include <io.h>
#include <stdlib.h>
#include <string.h>
#else  /* ifdef DOS_MODS */

/*
 * Standard UNIX
 * declarations
 */

extern int strcmp();
extern char *strchr();

 /*
  * The following line was moved here from the
  * ERR definition to prevent a "duplicate
  * definition" error message when the code is
  * compiled under DOS. (ARH, 10-8-86)
  */

/* extern int strlen(); */
extern int write();
#endif /* ifdef DOS_MODS */

int   opterr = 1;
int   optind = 1;
int   optopt;
char *optarg;

#ifdef ANSI_FUNC
int getopt (int argc, char **argv, char *opts)
#else
int
getopt(argc, argv, opts)
int argc;

char **argv, *opts;
#endif
{
  static int sp = 1;
  register int c;
  register char *cp;

  if (sp == 1)
  {
    if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
    {
      return ( EOF );
    }
    else if (strcmp(argv[optind], "--") == '\0' /*NULL*/ )
    {
      optind++;
      return ( EOF );
    }
  }

  optopt = c = argv[optind][sp];
  if (c == ':' || ( cp = strchr(opts, c)) == NULL)
  {
    ERR(": illegal option -- ", c);
    if (argv[optind][++sp] == '\0')
    {
      optind++;
      sp = 1;
    }

    return ( '?' );
  }

  if (*++cp == ':')
  {
    if (argv[optind][sp + 1] != '\0')
    {
      optarg = &argv[optind++][sp + 1];
    }
    else if (++optind >= argc)
    {
      ERR(": option requires an argument -- ", c);
      sp = 1;
      return ( '?' );
    }
    else
    {
      optarg = argv[optind++];
    }

    sp = 1;
  }
  else
  {
    if (argv[optind][++sp] == '\0')
    {
      sp = 1;
      optind++;
    }

    optarg = NULL;
  }

  return ( c );
}
