#ifdef LINT_ARGS
extern int mxmseg(struct mxbitiobuf *contents_file,
       struct PREAMBLE *arc_preamble_ptr,
       struct MXLOPTS *retrieval_options_ptr,
       struct BRANCH_PREAMBLE *arc_branch_preamble_ptr,
       int repack, char *seg_type);
#else
extern int mxmseg();
#endif

