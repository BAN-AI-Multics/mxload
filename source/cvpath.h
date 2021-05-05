#ifdef LINT_ARGS
extern int cvpath(char *path,char *add_chars,char *type,char *new_path);
extern int make_new_path(char *dname, char *ename,
       struct MXLOPTS *retrieval_options_ptr,char *add_chars,char *name_type,
       char *new_path);
extern void get_directory (char *path, char *directory);
extern void get_entryname (char *path, char *entry);
#else
extern int cvpath();
extern int make_new_path();
extern void get_directory ();
extern void get_entryname ();
#endif
