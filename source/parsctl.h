#ifdef LINT_ARGS
extern struct MXLOPTS * parsctl(int, char * *, int, char * *, int) ;
extern char ** get_keyword_values(char * * *keyword_array,char *keyword);
extern int count_keywords(char * * *keyword_array);
extern int get_keyword_values_index(char * * *keyword_array,char *keyword);
extern char * get_keyword_value(char * *keyword_values,char *value);
#else
extern struct MXLOPTS * parsctl();
extern char ** get_keyword_values();
extern int count_keywords();
extern int get_keyword_values_index();
extern char * get_keyword_value();
#endif
