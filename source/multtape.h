#ifdef LING_ARGS
extern void read_tape_label(struct mxbitiobuf *mxbitfile);
extern void read_tape_record_header(struct mxbitiobuf *mxbitfile);
extern void read_tape_record_trailer(struct mxbitiobuf *mxbitfile);
#else
extern void read_tape_label();
extern void read_tape_record_header();
extern void read_tape_record_trailer();
#endif
