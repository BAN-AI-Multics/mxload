struct MXLARGS
     {
     unsigned char  brief;              /* -b (mxload only)         */
     unsigned char  verbose;            /* -v                       */
     unsigned char  extremely_verbose;  /* -x                       */
     unsigned char  no_map;             /* -n (mxload only )        */
     char           *map_filename;      /* -g MAPFILE (mxload only) */
     FILE           *map_file;          /*                          */
     unsigned char  local_map;          /* -l (mxload only)         */
     unsigned char  map_only;           /* -m (mxarc only)          */
};

extern struct MXLARGS mxlargs;
