#ifdef LINT_ARGS
extern void display_branch_preamble(struct PREAMBLE *preamble_ptr,
       struct BRANCH_PREAMBLE *branch_preamble_ptr);
extern void display_dirlist_preamble(struct PREAMBLE *preamble_ptr,
       struct DIRLIST_PREAMBLE *dirlist_preamble_ptr);
extern void format_access_class (unsigned long *access_class,
       char *access_class_string);
extern void display_conversion_info (char *newpath, char *segtype,
       char *conversion_type);
#else
extern void display_branch_preamble();
extern void display_dirlist_preamble();
extern void format_access_class ();
extern void display_conversion_info ();
#endif
