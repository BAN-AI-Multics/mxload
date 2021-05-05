struct ACL
     {
     unsigned char  mode[2];
     unsigned char  ext_mode[2];
     char           access_name[34];
};

/* Information from all preambles */

struct PREAMBLE
     {
     char           dname[200];
     char           ename[34];
     unsigned long  bitcnt;
     unsigned long  length;             /* In Multics words */
     unsigned long  adjusted_bitcnt;
     int            record_type;
     unsigned long  maximum_bitcnt;
};


/* Selected information from backup records type 19 and 20 */

struct BRANCH_PREAMBLE
     {
     unsigned int   naddnames;          /* Number of addnames */
     char           *addnames;          /* Pointer to array of addnames */
     unsigned int   nacl;               /* Number of ACL entries */
     struct ACL     *acl;               /* Pointer to ACL */
     char           author[34];         /* Only for segment, not dir */
     char           bitcount_author[34];/* Bit count author */
     unsigned long  dtu;                /* Date/time used */
     unsigned long  dtm;                /* Date/time modified */
     unsigned long  dtd;                /* Date/time dumped */
     unsigned long  dtbm;               /* Date/time entry modified */
     int            switches;
     unsigned char  rings[3];
     unsigned long   uid[2];          /* 18 bits stored in each long */
     unsigned long   access_class[4]; /* 18 bits stored in each long */
     int            cur_length;
};


/* Selected information from backup record type 3 */

struct LINK
     {
     char           ename[34];
     unsigned int   naddnames;          /* Number of addnames */
     char           *addnames;          /* Pointer to array of addnames */
     unsigned long  dtu;                /* Date/time used */
     unsigned long  dtm;                /* Date/time modified */
     unsigned long  dtd;                /* Date/time dumped */
     char           target[170];        
};

struct DIRLIST_PREAMBLE
     {
     unsigned int   nlinks;
     struct LINK    *links;
};


#define  SAFETY_SW_MASK 0x0001
#define  ENTRYBD_SW_MASK 0x0002
#define  SOOS_SW_MASK 0x0004
#define  AUDIT_SW_MASK 0x0008
#define  MULTICLASS_SW_MASK 0x0010
#define  MDIR_SW_MASK 0x0020
#define  COPY_SW_MASK 0x0040      /* Not used */
#define  SYNC_SW_MASK 0x0080      /* Not used */
#define  DAMAGED_SW_MASK 0x0100      /* Not used */
#define  NID_SW_MASK 0x0200      /* Not used */
#define  NCD_SW_MASK 0x0400      /* Not used */

#define DIRLIST_RECORD         3
#define SEGMENT_RECORD        19
#define DIRECTORY_RECORD      20

#ifdef LINT_ARGS
extern int get_preamble_min(struct mxbitiobuf *preamble_file,
       struct PREAMBLE *preamble_ptr);
extern int get_branch_preamble(struct mxbitiobuf *preamble_file,
       struct BRANCH_PREAMBLE *branch_preamble_ptr,
       struct PREAMBLE *preamble_ptr);
extern int get_dirlist_preamble(struct mxbitiobuf *preamble_file,
       struct DIRLIST_PREAMBLE *dirlist_preamble_ptr,
       struct PREAMBLE *preamble_ptr);
#else
extern int get_preamble_min();
extern int get_branch_preamble();
extern int get_dirlist_preamble();

#endif
