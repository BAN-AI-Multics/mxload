#ifdef LINT_ARGS
extern int rdbkrcd(struct mxbitiobuf *infile,
       struct mxbitiobuf *preamble_file,struct PREAMBLE *preamble_ptr);
extern int rdseg(struct mxbitiobuf *infile,
       struct mxbitiobuf *contents_file,struct PREAMBLE *preamble_ptr);
#else
extern int rdbkrcd();
extern int rdseg();
#endif
