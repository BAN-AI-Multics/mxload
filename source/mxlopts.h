#define N_ATTRS 5
#define N_FILE_TYPES 7
#define N_LIST_TYPES 3

extern char         **file_cv_types[];
extern char         **attr_cv_types[];
extern char         **list_types[];

struct USER_TRANSLATION
     {
     char           multics_name[24];
     char           unix_name[32];      /* Character string representation */
     int            id;                 /* UID or GID */
     int            is_group;           /* As opposed to user/owner */
     struct USER_TRANSLATION *next;
};

struct MXLOPTS {
     struct MXLOPTS *next;
     char           path[170];
     char           *path_type;
     char           new_path[170];
     char           *attr_cv_values[N_ATTRS];
     char           *file_cv_values[N_FILE_TYPES];
     char           *list_values[N_LIST_TYPES];
     char           *force_convert;
     char           *access;
     char           *reload;
     char           *dataend;
     int            n_files_matched;
     int            n_files_loaded;
     struct USER_TRANSLATION *user_translations;
};

