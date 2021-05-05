#ifdef LINT_ARGS
extern int getopt(int, char **, char *);
#else
extern int getopt();
#endif

extern int optind;
extern char *optarg;
