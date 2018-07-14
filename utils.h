#define FLAG_N  0x01 // decimal 1
#define FLAG_P  0x02 // decimal 2
#define FLAG_D  0x04 // decimal 4


struct params{
    char *out_file;
    char *in_file1;
    char *in_file2;
    u_int flags;
};

