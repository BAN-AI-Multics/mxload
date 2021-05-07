char *ascii_cv_values[] = {
                 "ascii",
                 "8bit",
                 "8bit+9bit",
                 "9bit",
                 "discard",
                 NULL};
char *nonascii_cv_values[] = {
                 "nonascii",
                 "9bit",
                 "8bit",
                 "8bit+9bit",
                 "discard",
                 NULL};
char *object_cv_values[] = {
                 "object",
                 "discard",
                 "8bit",
                 "8bit+9bit",
                 "9bit",
                 NULL};
char *ascii_archive_cv_values[] = {
                 "ascii_archive",
                 "unpack",
                 "8bit",
                 "9bit",
                 "8bit+9bit",
                 "discard",
                 NULL};
char *nonascii_archive_cv_values[] = {
                 "nonascii_archive",
                 "9bit",
                 "unpack",
                 "8bit",
                 "8bit+9bit",
                 "discard",
                 NULL};
char *mbx_cv_values[] = {
                 "mbx",
                 "9bit",
                 "unpack",
                 "repack",
                 "8bit",
                 "8bit+9bit",
                 "discard",
                 NULL};
char *ms_cv_values[] = {
                 "ms",
                 "discard",
                 "8bit",
                 "8bit+9bit",
                 "9bit",
                 "unpack",
                 NULL};

char **file_cv_types[] = {
                 ascii_cv_values,
                 nonascii_cv_values,
             object_cv_values,
         ascii_archive_cv_values,
             nonascii_archive_cv_values,
             mbx_cv_values,
             ms_cv_values,
             NULL};

char *force_convert_file_cv_types[] = {
                 "force_convert",
                 "8bit",
                 "8bit+9bit",
                 "9bit",
                 "discard",
                 NULL};

#ifdef sun
#define BSD
#endif
#ifdef BSD
char *name_type_cv_values[] = {
                 "name_type",
                 "BSD",
                 "SysV",
                 "CMS",
                 "MSDOS",
                 NULL};
#else
char *name_type_cv_values[] = {
                 "name_type",
                 "SysV",
                 "BSD",
                 "CMS",
                 "MSDOS",
                 NULL};
#endif
char *owner_cv_values[] = {
                 "owner",
                 "author",
                 "bit_count_author",
                 NULL};
char *group_cv_values[] = {
             "group",
                 "author",
                 "bit_count_author",
                 NULL};
char *access_time_cv_values[] = {
             "access_time",
                 "dtu",
                 "now",
                 NULL};
char *mod_time_cv_values[] = {
             "modification_time",
                 "dtcm",
                 "now",
                 NULL};
char **attr_cv_types[] = {
             name_type_cv_values,
             owner_cv_values,
             group_cv_values,
             access_time_cv_values,
             mod_time_cv_values,
             NULL};
char *link_list_values[] = {
                "link",
                "none",
                "global",
                "local",
                NULL};
char *acl_list_values[] = {
                "acl",
                "none",
                "global",
                "local",
                NULL};
char *addname_list_values[] = {
                "addname",
                "none",
                "global",
                "local",
                NULL};
char **list_types[] = {
            link_list_values,
            acl_list_values,
        addname_list_values,
        NULL};
