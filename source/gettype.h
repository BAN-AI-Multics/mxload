#ifdef LINT_ARGS
extern char * get_type_by_name(struct PREAMBLE *preamble_ptr);
extern char * get_type_by_content(struct mxbitiobuf *contents_file,
       long bitcount,struct mxbitiobuf *preamble_contents_file);
#else
extern char * get_type_by_name();
extern char * get_type_by_content();
#endif
