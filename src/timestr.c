 /********************************\
 *     Copyright (C) 1987 by      *
 * Cygnus Cybernetics Corporation *
 *    and Oxford Systems, Inc.    *
 \********************************/

#include "timestr.h"
#include "mxload.h"
#include "tmchart.h"
#include <string.h>
#include <time.h>

char *timestr(time)

long *time;

{
  char *strp;
  static char short_time[20];

  if (*time == 0L)
  {
    strcpy(short_time, "ZERO");
  }
  else
  {
    strp = ctime(time);
    memcpy(short_time, strp + 8, 3);
    memcpy(short_time + 3, strp + 4, 4);
    memcpy(short_time + 7, strp + 22, 2);
    memcpy(short_time + 9, strp + 10, 6);
    short_time[16] = '\0';
  }

  return ( short_time );
}

/*
 * Convert Multics file system time
 * to UNIX file system time
 */

/*
 * Multics time values are derived from the Multics system clock
 * which gives the number of microseconds since 1/1/1901 00:00 gmt.
 * In Unix, times are given as number of seconds since 1/1/1970 00:00 gmt.
 *
 * This program converts the 36-bit Multics fstime (file-system time) to
 * Unix time.
 *
 * Diagram of various Multics time formats:
 * 1         2         3         4         5         6         7
 * 123456789012345678901234567890123456789012345678901234567890123456789012
 * <-------------------------Time stored in 72 bits as usecs-------------->
 * ..................<---------- System clock, 54 bits as usecs ---------->
 * ....................<----- FS time is 36 bits --------->................
 * <---- 20 bits ----->....................................<--- 16 bits -->
 *
 * So....  "fstime units" is usecs divided by 2**16 or usecs / 65536
 * or secs * 10**6 / 2**16.
 *
 * sec = fstime_unit * 2**16 / 10**6
 *
 * In other words, 1 fstime_unit = 1/15.2588 seconds.
 *
 * Using a program on Multics we determine that the Multics fstime at
 * the beginning of Unix time was 33,225,292,968.
 *
 * This can be expressed in binary as
 * 011110111100011000011011110010101000
 * 654321098765432109876543210987654321
 * .     3         2         1
 *
 * Algorithm to convert fstime_units to seconds since 1/1/1970:
 * Take top 32 bits, equivalent to shifting right 4 bits.
 * Subtract 33,225,292,968 >> 4, i.e. 2076580810.
 * Multiply by 16*.065536, i.e. 1.048576.
 */

#ifdef ANSI_FUNC
unsigned long
cvmxtime (unsigned long long_pair[2])
#else
unsigned long
cvmxtime(long_pair)

unsigned long long_pair[2];
#endif

{
  unsigned long secs;
  double secs_float;

  secs = long_pair[1] >> 4 | long_pair[0] << 28; /* Shift right 4 bits */

  if (secs <= 2076580810L)
  {
    return ((unsigned long)0L );
  }

  /*
   * Subtract off 1/1/70 fstime
   * units divided by 16,
   * i.e. shifted right 4
   */

  secs -= 2076580810L;

  secs_float = (double)secs * 1.048576;
  secs = (long)secs_float;

  return ( secs );
}

 /*
  * This subroutine is the equivalent of the ANSI C mktime function, the
  * SunOS timelocal function, or the Multics encode_clock_value_ function.
  * It is provided with mxload because many C libraries do not provide such
  * a function.
  */

#ifdef ANSI_FUNC
long
encodetm (struct tm *timeptr)
#else
long
encodetm(timeptr)

struct tm *timeptr;
#endif

{
  int month;
  long seconds;

  if (timeptr->tm_year < 70)
  {
    return ( 0L );
  }

  month = ( timeptr->tm_year - 70 ) * 12 + timeptr->tm_mon;
  if (month >= 360)
  {
    month = 359;
  }

  if (month_times[month] < 0L)
  {
    return ( 0 );
  }

  return ( month_times[month]             /* 1/1/70 to start of month */
           + ( timeptr->tm_mday - 1 ) * 24L * 60L
           * 60L                          /* days since start of month */
           + timeptr->tm_hour * 60L * 60L /* hours since start of day */
           + timeptr->tm_min * 60L        /* minutes since start of hour */
           + timeptr->tm_sec              /* secs since start of minute */
           );
}
