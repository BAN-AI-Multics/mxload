#ifdef LINT_ARGS
extern char *timestr(long *time);
extern unsigned long cvmxtime(unsigned long *long_pair);
extern long encodetm (struct tm *timeptr);
#else
extern char *timestr ();
extern unsigned long cvmxtime ();
extern long encodetm ();
#endif
