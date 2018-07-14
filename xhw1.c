#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include "utils.h"

#ifndef __NR_xdedup
#error xdedup system call not defined
#endif


int main(int argc, char * const argv[])
{
	struct params *curParams = malloc(sizeof *curParams);
	int c;
	int rc;

	while( (c=getopt(argc,argv,"npd"))!=-1 )
	{
		switch(c){
		case 'n':curParams->flags |= FLAG_N;break;
		case 'p':curParams->flags |= FLAG_P;break;
		case 'd':curParams->flags |= FLAG_D;break;
            default:printf("Invalid Option");exit(0);
		}
	}

    if(curParams->flags&FLAG_N) {
        if (argc != 4) {
            printf("Inavlid number of Arguments \n");
            exit(0);
        }
    }
    else if(curParams->flags&FLAG_P) {
        if (argc != 5) {
            printf("Inavlid number of Arguments \n");
            exit(0);
        }
    }
    else if(curParams->flags&FLAG_D) {
        if (argc != 4) {
            printf("Inavlid number of Arguments \n");
            exit(0);
        }
    }
    else{
        if (argc != 3) {
            printf("Inavlid number of Arguments \n");
            exit(0);
        }
    }

    if(optind<argc){
        if(curParams->flags&FLAG_P && !(curParams->flags&FLAG_N)) {
//			printf("%s\n",argv[optind]);
			curParams->out_file=argv[optind++];

		}
//        printf("%s\n",argv[optind]);
		curParams->in_file1 = argv[optind++];
//        printf("djndj%s\n",argv[optind]);
		curParams->in_file2 = argv[optind];

	}
    else{
        printf("Inavlid number of Arguments \n");
    }
	if(curParams->in_file1==NULL||curParams->in_file2==NULL){
		printf("Input file names missing\n");
		exit(0);
	}
	if((curParams->flags&FLAG_P) && !(curParams->flags&FLAG_P) &&curParams->out_file==NULL){
		printf("Output file name missing\n");
		exit(0);
	}
    if ((access(curParams->in_file1, F_OK) == -1)){
        printf("File 1 does not exsist\n");
        exit(0);
    } else if ((access(curParams->in_file1, R_OK) == -1)) {
        printf("Permission Denied to Read file 1\n");
    }
    if ((access(curParams->in_file2, F_OK) == -1)){
        printf("File 2 does not exsist\n");
        exit(0);
    } else if ((access(curParams->in_file2, R_OK) == -1)) {
        printf("Permission Denied to Read file 2\n");
    }
    if(curParams->flags&FLAG_P&&!(curParams->flags&FLAG_P)&&(access(curParams->out_file, F_OK ) == -1)){
        printf("Output file does not exsist\n");
    } else if(curParams->flags&FLAG_P&&!(curParams->flags&FLAG_P)&&(access(curParams->out_file, W_OK ) == -1)){
        printf("Permission Denied to Write to output file\n");
        exit(0);
    }
	rc = syscall(__NR_xdedup,(void *)curParams);
	if (rc == 0)
		printf("syscall returned %d\n", rc);
	else
		printf("syscall returned %d (errno=%d)\n", rc, errno);

	/*void *dummy = (void *)argv[1];
  	rc = syscall(__NR_xdedup, dummy);
	if (rc == 0)

		printf("syscall returned %d\n", rc);
	else
		printf("syscall returned %d (errno=%d)\n", rc, errno);
	*/

	exit(0);
}

