CSE-506 (Spring 2018) Homework Assignment #1
Submitted By:- Dinesh Balani
ID No- 111500275


* TASK:

Create a Linux kernel module (in vanilla 4.6.y Linux that's in your HW1
GIT repository) that, when loaded into Linux, will support a new system call
called

	sys_dedup(const char *infile1, const char *infile2, char *outfile,
		  u_int flags)

where "infile1" and "infile2" are the input files to deduplicate.
If the two files have identical content, and they're owned by the same user,
then you should delete file infile2 and hard-link it to infile


* BACKGROUND:

Deduplication is very useful and important nowadays because it can save
storage space, but many OSs do not support this feature natively (yet).
Your task is to create a new system call that can take two input files, then
deduplicate them if they have identical content.

* FILES INCLUDED:
-Kernel.config
-Header files - utils.h
-Makefile
-xhw1.c
-sys_xdedup.c
-install_module.sh


* COMPILATION COMMANDS:
# make
# sh install_module.sh

* RUNNING COMMANDS:
./xdedup [-npd] outfile.txt file1.txt file2.txt


USER LEVEL CHECKS:-
1. Valid Number of arguments are passed through command line:-
	a. 0 or d flag - Atleast two file names are passed
	b. N flag - Atleast two file names are passed
	c. P flag - Atleast three file names are passed
	d. NP flag - Atleast two file names are passed

2. If Input files exsist and have read permissions.
3. If Output file exsist and have write permission.


KERNEL LEVEL CHECK:-
1. missing arguments passed
2. null arguments where non-null is expected; or vise versa
3. invalid flags or combination of flags and arguments
4. input files cannot be opened or read
5. output file cannot be opened or written
6. input or output files are not regular, or they point to the same file (no
  point in deduplicating the same file)
7. owners of input files are same.
8. If output file exsist maintain the same permissions.


APPROACH and ERROR HANDLING:-
1. Take in the flags and all the arguments and parse them and allocate the memory in kernel space.

A. If None of the flag is on.
	a. After basic checks mentioned above in USER AND KERNEL LEVEL CHECK SECTION
	b. Create a temporary file with name as timestamp of the system and with permissions of file2.
	c. Taken buffer of size 4096, read both the files and if the content is same keep on incrementing the buffer size.
	d. Write all the content of file 2 to temporary file.
	e. If all of the contents matches unlink the temporary file.
		i). Link the temporary file to file2.
		ii). If linking fails, unlink the temporary file. we still have our file2 saved.
		iii). If linking passes, then rename temp file to file2.
	f. If not all content match, just delete the temp file using unlink

B. If N flag is on.
	a. After basic checks mentioned above in USER AND KERNEL LEVEL CHECK SECTION
	b. Taken buffer of size 4096, read both the files and if the content is same keep on incrementing the buffer size.
	c. Doesn't actually dedup.  Just return -errno or #identical bytes if the two files are identical. 

C. If P flag is on
	a. After basic checks mentioned above in USER AND KERNEL LEVEL CHECK SECTION
	b. Ignore if size of two files should be same.
	c. Create a temporary file with name as timestamp of the system.
	d. Taken buffer of size 4096, read both the files and if the content is same keep on incrementing the buffer size.
	e. Keep on writing identical bytes to this file.
	f. If identical content is 0 and output file already exsist don't overwrite the output file. Basically unlink this temporary file.
	g. If identical content is 0 and output file doesn't exsist, create a new file with 0 bytes.
	h. If identical content is more than 0 overwrite the content in output file or create a new output file.

D. If NP flag is on
	a. After basic checks mentioned above in USER AND KERNEL LEVEL CHECK SECTION
	b. Ignore the check of size of two files should be same.
	b. Taken buffer of size 4096, read both the files and if the content is same keep on incrementing the buffer size.
	c. Doesn't actually dedup/partially dedup.  Just return -errno or #identical bytes if the two files are identical. 

E. If D flag is on
	a. Print the debug messages as well with other flags


IMPORTANT FUNCITONS USED:-

1. VFS_READ() - To read data from file of a buffer length
2. VFS_WRITE() - To write data to file of a buffer length
3. MUTEX_LOCKS_NESTED- To acquire locks before critical operations
4. MUTEX_UNLOCK- To release locks after the operation
5. VFS_RENAME() - Renames the file.
6. VFS_UNLINK() - Unlinks the file from address space.
7. VFS_LINK() - Links the file to address space.
8. VFS_STAT() - To get information about the file structre


SOME OF THE ERROR NO USED:-

#define EPERM            1      /* Operation not permitted */
#define EIO              5      /* I/O error */
#define ENOMEM          12      /* Out of memory */
#define EINVAL          22      /* Invalid argument */
And Appropriate errors thrown by various functions mentioned above


REFERENCES
https://elixir.bootlin.com/linux/latest/source/include/linux/fs.h
http://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
http://www3.cs.stonybrook.edu/~ezk/cse506-s18/hw1.txt
http://www3.cs.stonybrook.edu/~ezk/cse506-s18/lectures/10b.txt
http://www3.cs.stonybrook.edu/~ezk/cse506-s18/lectures/10.txt


