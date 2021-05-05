#ifdef LINT_ARGS
extern int mxdearc(struct mxbitiobuf *contents_file,
       struct PREAMBLE *arc_preamble_ptr,
       struct MXLOPTS *retrieval_options_ptr,
       struct BRANCH_PREAMBLE *arc_branch_preamble_ptr, int is_ascii);
#else
extern  int mxdearc();
#endif
