#define INPUT_BUFFER_SIZE 10000
#define MXBITFILE struct mxbitiobuf

extern struct mxbitiobuf * open_mxbit_file();
extern void rewind_mxbit_file();
extern void close_mxbit_file();
extern long get_mxbits();
extern int put_mxbits();
extern int get_mxstring();
extern int mxbit_pos();
extern int get_9_mxbit_integer();
extern int get_36_mxbit_integer();
extern long get_18_mxbit_integer();
extern long get_24_mxbit_integer();
extern int skip_mxbits();
extern int eof_reached ();

struct mxbitiobuf
     {
     FILE           *realfile;
     unsigned char  byte_buffer;
     int            byte_buffer_pos;
     long           input_file_pos;
     char           *buffer_ptr;
     unsigned char  *saved_input_ptr;
     int            write;
     int            tape_mult_sw;
     int            reading_tape_data;
     long           n_bytes_left_in_tape_block;
     long           n_pad_bytes_in_tape_block;
     int            next_tape_block_number;
     int            end_of_reel_reached;
     int            temp_file;
     char           path[200];
     long           reading_buffer_to_pos;
};
