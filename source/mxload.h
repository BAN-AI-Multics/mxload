#ifdef LINT_ARGS
extern void mxlexit(int);
extern void process_seg(struct mxbitiobuf *infile,
       struct BRANCH_PREAMBLE *branch_preamble_ptr,
       struct PREAMBLE *preamble_ptr,struct MXLOPTS *retrieval_options_ptr,
       int is_ascii);
#else
extern void mxlexit();
extern void process_seg();
#endif
