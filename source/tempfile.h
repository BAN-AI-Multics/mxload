#ifdef LINT_ARGS
extern struct mxbitiobuf *get_temp_file(char *mode, char * comment);
extern void release_temp_file(struct mxbitiobuf *tempfile, char * comment);
extern void replace_temp_file(struct mxbitiobuf *tempfile);
extern char * temp_file_name(struct mxbitiobuf *tempfile);
extern void cleanup_temp_files(void);
#else
extern struct mxbitiobuf *get_temp_file();
extern void release_temp_file();
extern void replace_temp_file();
extern char * temp_file_name();
extern void cleanup_temp_files();
#endif
