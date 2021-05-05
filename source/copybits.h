#ifdef LINT_ARGS
extern int copy_to_empty_file(struct mxbitiobuf *infile,
       struct mxbitiobuf *outfile,long n_bits);
extern int copy_bits(struct mxbitiobuf *infile,
       struct mxbitiobuf *outfile,long n_bits);
extern void copy_8bit(struct mxbitiobuf *contents_file,
       char *new_path, unsigned long bitcount);
extern void copy_8bit_to_file(struct mxbitiobuf *contents_file,
       struct _iobuf *outfile, unsigned long bitcount);
extern void copy_file(struct mxbitiobuf *contents_file,
       char *new_path, int is_ascii, long bit_count);
#else
extern int copy_to_empty_file();
extern int copy_bits();
extern void copy_8bit();
extern void copy_8bit_to_file();
extern void copy_file();
#endif
