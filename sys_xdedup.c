#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/namei.h>
#include "utils.h"


asmlinkage extern long (*sysptr)(void *arg);
static void logMessage(u_int flag,char *logType, char *logMessage);

asmlinkage long xdedup(void *arg)
{
	//Initializing Variables
    //struct inode *in_inode1,*in_inode2,*in;

    mm_segment_t in_oldfs1;
	int in_bytes1,in_bytes2,out_bytes,checkBytes;
	int retval = 0;
	char *in_buf1=NULL;
	char *in_buf2=NULL;
	char tempName[10];
	struct file *out_filp=NULL,*in_filp1,*in_filp2,*temp_filp=NULL;
	struct dentry *lookup_len,*temp_lookup_len,*out_lookup_len;
	struct kstat in_kstat1,in_kstat2,out_kstat;
	int in_vfs_stat1,in_vfs_stat2=0,out_vfs_stat;
    int out_kern_path, temp_kernpath, in_kern_path1,in_kern_path2;
    int rename;
	loff_t pos = 0;
	int write_upto = 0;
	int chunk_no;
    unsigned short mode = 0644;
	struct path path1,path2,out_path,temp_path;
    struct timespec ts;
    struct params *curParams = kmalloc(sizeof(struct params),GFP_KERNEL);
    if(copy_from_user(curParams,arg,sizeof(struct params))){
        logMessage(curParams->flags,"ERROR","Not Enough Memory");
        retval = -ENOMEM;
        goto out;
    }

//	struct params *curParams = (struct params *) arg;
//    curParams = kmalloc(sizeof((struct params *)));
    printk("\n====================================================================\n");
    logMessage(curParams->flags,"INFO","Entered System Call");

//    printk("%d\n",curParams->flags);
//    goto out;
    // Check if file exsists that is file is not null
    logMessage(curParams->flags,"INFO","Checking if input file exsists and are valid");
	if(curParams->in_file1==NULL){
        logMessage(curParams->flags,"ERROR","Input File 1 name is missing");
		retval = -EINVAL;
		goto out;
	}
    in_vfs_stat1 = vfs_stat(curParams->in_file1,&in_kstat1);
    if(in_vfs_stat1){
        logMessage(curParams->flags,"ERROR","Input File 1 not found");
        retval = in_vfs_stat2;
        goto out;
    }

    if(curParams->in_file2==NULL){
        logMessage(curParams->flags,"ERROR","Input File 2 name is missing");
        retval = -EINVAL;
        goto out;
    }
    in_vfs_stat2 = vfs_stat(curParams->in_file2,&in_kstat2);
    if(in_vfs_stat2){
        logMessage(curParams->flags,"ERROR","Input File 2 not found");
        retval = in_vfs_stat2;
        goto out;

    }
    mode = in_kstat2.mode;

    //Check if input files are regular
    logMessage(curParams->flags,"INFO","Checking if files are regular");
    if(!S_ISREG(in_kstat1.mode)){
        logMessage(curParams->flags,"ERROR","Input File 1 is not regular");
        retval = -EINVAL;
        goto out;
    }
    if(!S_ISREG(in_kstat2.mode)){
        logMessage(curParams->flags,"ERROR","Input File 2 is not regular");
        retval = -EINVAL;
        goto out;
    }

    in_kern_path1 = kern_path(curParams->in_file2, LOOKUP_FOLLOW, &path2);
    if(in_kern_path1!=0){
        logMessage(curParams->flags,"ERROR","Input File 1 Path doesn't exsist");
        retval = -EINVAL;
        goto out;
    }
    in_kern_path2 = kern_path(curParams->in_file1, LOOKUP_FOLLOW, &path1);
    if(in_kern_path2!=0){
        logMessage(curParams->flags,"ERROR","Input File 2 Path doesn't exsist");
        retval = -EINVAL;
        goto out;
    }

    //Check if files are not already hard linked
    logMessage(curParams->flags,"INFO","Checking if files are already deduped");
    if(((in_kstat1.ino == in_kstat2.ino) || memcmp(path1.dentry->d_inode->i_sb->s_uuid,path2.dentry->d_inode->i_sb->s_uuid,sizeof(path1.dentry->d_inode->i_sb->s_uuid))))
    {
        logMessage(curParams->flags,"ERROR","Input Files already deduped");
        if((curParams->flags & FLAG_N)){
            retval = in_kstat1.size;
        } else{
            retval = -EPERM;
        }
        goto out;

    }
    if(curParams->flags & FLAG_P && !(curParams->flags & FLAG_N)){
        logMessage(curParams->flags,"INFO","Checking parameters for output files");
        out_vfs_stat = vfs_stat(curParams->out_file,&out_kstat);
        if(out_vfs_stat){
            logMessage(curParams->flags,"INFO","Output file doesnt exist");
        }
        else{
            mode = out_kstat.mode;
            logMessage(curParams->flags,"INFO","Checking if user is allowed to write to Output file");
            if(curParams->flags & FLAG_P && !(out_kstat.mode & S_IWUSR)){
                logMessage(curParams->flags,"ERROR","User is not allowed to write to output file");
                retval = -EPERM;
                goto out;
            }
            if(!S_ISREG(out_kstat.mode)){
                logMessage(curParams->flags,"ERROR","Output file is not regular");
                retval = -EINVAL;
                goto out;
            }
            out_kern_path = kern_path(curParams->out_file, LOOKUP_FOLLOW, &out_path);
            if(out_kern_path!=0){
                logMessage(curParams->flags,"ERROR","Output file path not found");
                retval = -EINVAL;
                goto out;
            }
            if((in_kstat1.ino == out_kstat.ino)||memcmp(path1.dentry->d_inode->i_sb->s_uuid,out_path.dentry->d_inode->i_sb->s_uuid,sizeof(path1.dentry->d_inode->i_sb->s_uuid))){
                logMessage(curParams->flags,"ERROR","Output file is dedup of first input file");
                retval = -EINVAL;
                goto out;
            }
            if((in_kstat2.ino == out_kstat.ino)||memcmp(path1.dentry->d_inode->i_sb->s_uuid,out_path.dentry->d_inode->i_sb->s_uuid,sizeof(path1.dentry->d_inode->i_sb->s_uuid))){
                logMessage(curParams->flags,"ERROR","Output file is dedup of second input file");
                retval = -EINVAL;
                goto out;
            }
        }
    }

    logMessage(curParams->flags,"INFO","Checking if owners of 2 files match");
	if(!(curParams->flags & FLAG_P) && in_kstat1.uid.val!=in_kstat2.uid.val){
        logMessage(curParams->flags,"ERROR","Owners of 2 files do not match");
		retval = -EPERM;
		goto out;
	}
    logMessage(curParams->flags,"INFO","Owners of 2 files match");

	if(!(curParams->flags & FLAG_P) && (in_kstat1.size!=in_kstat2.size)){
        logMessage(curParams->flags,"ERROR","Size of 2 files do not match");
		retval = -EPERM;
		goto out;
	}
    logMessage(curParams->flags,"INFO","Size of 2 files match");

    logMessage(curParams->flags,"INFO","Checking if user is allowed to read Input file 1");
    if (!(in_kstat1.mode & S_IRUSR)){
        logMessage(curParams->flags,"ERROR","User is not allowed to read input file 1");
        retval = -EPERM;
        goto out;
    }

    logMessage(curParams->flags,"INFO","Checking if user is allowed to read Input file 2");
    if (!(in_kstat2.mode & S_IRUSR)){
        logMessage(curParams->flags,"ERROR","User is not allowed to read input file 2");
        retval = -EPERM;
        goto out;
    }




    in_filp1 = filp_open(curParams->in_file1, O_RDONLY, 0);
	if (!in_filp1 || IS_ERR(in_filp1)) {
        logMessage(curParams->flags,"ERROR","Unable to read open Input file 1");
		retval = (int) PTR_ERR(in_filp1); // return appropriate error
		goto out_close1;
	}
	in_filp1->f_pos = 0;  /* start offset */
    logMessage(curParams->flags,"INFO","Opened Input file 1");

	in_filp2 = filp_open(curParams->in_file2, O_RDONLY, 0);
	if (!in_filp2 || IS_ERR(in_filp2)) {
        logMessage(curParams->flags,"ERROR","Unable to read open Input file 2");
		retval = (int) PTR_ERR(in_filp2); // return appropriate error
		goto out_close2;
	}
	in_filp2->f_pos = 0;  /* start offset */
    logMessage(curParams->flags,"INFO","Opened Input file 2");


    if(curParams->flags & FLAG_P || (!(curParams->flags & FLAG_N)&&!(curParams->flags & FLAG_P))) {
        getnstimeofday(&ts);
        snprintf(tempName, 10, "%ld", ts.tv_nsec);
        logMessage(curParams->flags,"INFO","Creating Temporary files");
		temp_filp = filp_open(tempName, O_WRONLY | O_CREAT, mode);
		if (!temp_filp || IS_ERR(temp_filp)) {
            logMessage(curParams->flags,"ERROR","Unable to open Temporary file in Write Mode");
			printk("wrapfs_read_file err %d\n", (int) PTR_ERR(temp_filp));
			retval = (int) PTR_ERR(temp_filp); // return appropriate error
			goto out_closeTemp;
		}
        logMessage(curParams->flags,"INFO","Opened the Temporary file");
    }

	in_oldfs1 = get_fs();
	set_fs(KERNEL_DS);
	retval = 0;

    logMessage(curParams->flags,"INFO","Reading files chunk by chunk");
	for(chunk_no=0;chunk_no<=(in_kstat1.size/4096)+1;chunk_no=chunk_no+1)
	{
		in_buf1=kmalloc(4096,GFP_KERNEL);
		if (in_buf1 == NULL) {
			retval = -ENOMEM;
			goto kfree_in_buf1;
		}

		in_buf2=kmalloc(4096,GFP_KERNEL);
		if (in_buf2 == NULL) {
			retval = -ENOMEM;
			goto kfree_in_buf2;
		}

		in_bytes1 = vfs_read(in_filp1, in_buf1, 4096, &in_filp1->f_pos);
		in_bytes2 = vfs_read(in_filp2, in_buf2, 4096, &in_filp2->f_pos);

        if(in_bytes1<=in_bytes2){
            checkBytes=in_bytes1;
        }else{
            checkBytes=in_bytes2;
        }
        write_upto=0;
		while(*(in_buf1+write_upto)!='\0'&&*(in_buf2+write_upto)!='\0'&&*(in_buf1+write_upto)==*(in_buf2+write_upto)&&(write_upto<checkBytes)) {
			write_upto++;
		}
		if(curParams->flags & FLAG_P &&!(curParams->flags & FLAG_N)){
			out_bytes = vfs_write(temp_filp, in_buf1, write_upto, &pos);
            if(out_bytes<write_upto){
                logMessage(curParams->flags,"ERROR","Error Occured while writing to file");
                retval = -EIO;
                goto out_unlinkTempFile;
            }
            retval += out_bytes;
		}

		if(curParams->flags & FLAG_N){
            if(curParams->flags & FLAG_D){
                printk("INFO : Bytes matched: %d\n",write_upto);
            }
            logMessage(curParams->flags,"INFO","Reading files chunk by chunk");
			retval+=write_upto;
		}
        if(!(curParams->flags & FLAG_N)&&!(curParams->flags & FLAG_P)){
            if(curParams->flags & FLAG_D) {
                printk("INFO : Going to Write content: %d\n", write_upto);
            }
            out_bytes = vfs_write(temp_filp, in_buf2, write_upto, &pos);
            if(out_bytes<write_upto){
                logMessage(curParams->flags,"ERROR","Error Occured while writing to file");
                retval = -EIO;
                goto out_unlinkTempFile;
            }
            retval += out_bytes;
        }

		if(write_upto<4096){
			break;
		}

		kfree(in_buf1);
		kfree(in_buf2);
	}
    set_fs(in_oldfs1);

    if(curParams->flags & FLAG_D) {
        printk("INFO : Total Writen content: %d\n", retval);
        printk("INFO : Size of file 1: %d\n", (int)in_kstat1.size);
        printk("INFO : Size of file 2: %d\n", (int)in_kstat2.size);
    }

    if((retval == in_kstat1.size)&&(retval == in_kstat2.size)){
        logMessage(curParams->flags,"INFO","Files are identical");
        if(curParams->flags & FLAG_N){
            logMessage(curParams->flags,"INFO","Flag N is ON");
            goto out_close2;
        }
        if(curParams->flags & FLAG_P){
            logMessage(curParams->flags,"INFO","Flag P is ON");
            if(retval==0){
                logMessage(curParams->flags,"INFO","Written content is zero");
                out_vfs_stat = vfs_stat(curParams->out_file,&out_kstat);
                if(out_vfs_stat){
                    logMessage(curParams->flags,"INFO","Output file doesn't exsist");
                    logMessage(curParams->flags,"INFO","Create Output file with size zero");
                }
                else{
                    logMessage(curParams->flags,"INFO","Output file exsist. Do Not Replace");
                    retval = -EINVAL;
                    goto out_unlinkTempFile;
                }
            }
        }

    }
    else{
        logMessage(curParams->flags,"INFO","Files are not identical");
        if((curParams->flags & FLAG_N)&&(curParams->flags & FLAG_P)){
            logMessage(curParams->flags,"INFO","Flag P and N are on. Remove the Temp file");
            goto out_unlinkTempFile;
        }
        else if(curParams->flags & FLAG_P){
            logMessage(curParams->flags,"INFO","Flag P is on.");
            if(retval==0){
                logMessage(curParams->flags,"INFO","Written content is zero bytes.");
                out_vfs_stat = vfs_stat(curParams->out_file,&out_kstat);
                if(out_vfs_stat){
                    logMessage(curParams->flags,"INFO","Output file doesn't exsist");
                    logMessage(curParams->flags,"INFO","Create Output file with size zero");
                }
                else{
                    logMessage(curParams->flags,"INFO","Output file exsist. Do Not Replace");
                    retval = -EINVAL;
                    goto out_unlinkTempFile;
                }
            }
        }
        else if(!(curParams->flags & FLAG_N)&&!(curParams->flags & FLAG_P)){
            logMessage(curParams->flags,"INFO","Flag N and P are off. Do not Dedup.");
            retval = -EINVAL;
            goto out_unlinkTempFile;
        }
        else{
            retval = -EINVAL;
            goto out_close2;
        }
    }

    if((curParams->flags & FLAG_P)&&!(curParams->flags & FLAG_N)) {
        logMessage(curParams->flags,"INFO","Flag P is ON. Inside Partial Dedup.");
//        printk("%d\n",retval);
		out_filp = filp_open(curParams->out_file, O_WRONLY | O_CREAT, mode);
		if (!out_filp || IS_ERR(out_filp)) {
            logMessage(curParams->flags,"ERROR","Unable to open Temporary file in Write Mode");
			retval = (int) PTR_ERR(out_filp); // return appropriate error
			goto out_close3;
		}

		temp_kernpath=kern_path(tempName, LOOKUP_FOLLOW, &temp_path);
        if(temp_kernpath!=0 ){
            logMessage(curParams->flags,"ERROR","File Path doesn't exsist");
            retval = -temp_kernpath;
            goto out;
        }
        out_kern_path=kern_path(curParams->out_file, LOOKUP_FOLLOW, &out_path);
        if(out_kern_path!=0){
            logMessage(curParams->flags,"ERROR","File Path doesn't exsist");
            retval = -out_kern_path;
            goto out;
        }
//		mutex_lock_nested(&out_path.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
//		retval = vfs_unlink(out_path.dentry->d_parent->d_inode, out_filp->f_path.dentry, NULL);
//        if(retval<0){
//            retval = -10;
//            printk("VFS Unlinkng error");
//        }
//		mutex_unlock(&out_path.dentry->d_parent->d_inode->i_mutex);
        mutex_lock_nested(&temp_path.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
        temp_lookup_len = lookup_one_len(temp_path.dentry->d_iname,temp_path.dentry->d_parent,strlen(temp_path.dentry->d_iname));
        mutex_unlock(&temp_path.dentry->d_parent->d_inode->i_mutex);
        if(IS_ERR(temp_lookup_len)){
            logMessage(curParams->flags,"ERROR","In Finding Parent of the file lookup_one_len()");
            retval = (int) PTR_ERR(temp_lookup_len);
            goto out_close2;
        }

        mutex_lock_nested(&out_path.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
        out_lookup_len = lookup_one_len(out_path.dentry->d_iname,out_path.dentry->d_parent,strlen(out_path.dentry->d_iname));
        mutex_unlock(&out_path.dentry->d_parent->d_inode->i_mutex);
        if(IS_ERR(temp_lookup_len)){
            logMessage(curParams->flags,"ERROR","In Finding Parent of the file lookup_one_len()");
            retval = (int) PTR_ERR(temp_lookup_len);
            goto out_close2;
        }

        lock_rename(temp_lookup_len->d_parent,out_lookup_len->d_parent);
		rename = vfs_rename(temp_path.dentry->d_parent->d_inode, temp_lookup_len, out_path.dentry->d_parent->d_inode,out_lookup_len, NULL, 0);
        unlock_rename(temp_lookup_len->d_parent,out_lookup_len->d_parent);
        if(rename!=0){
            logMessage(curParams->flags,"ERROR","Error in renaming the file");
            retval = -rename;
            goto out;
        }
	}

    if(!(curParams->flags & FLAG_N)&&!(curParams->flags & FLAG_P)){
        logMessage(curParams->flags,"INFO","Flag N & P is ON.");

        temp_kernpath=kern_path(tempName, LOOKUP_FOLLOW, &temp_path);
        if(temp_kernpath!=0){
            logMessage(curParams->flags,"ERROR","In Finding Parent of the file lookup_one_len()");
            retval = -temp_kernpath;
            goto out_close3;
        }

        logMessage(curParams->flags,"INFO","Unlinking the file.");
        mutex_lock_nested(&temp_path.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
        rename = vfs_unlink(temp_filp->f_path.dentry->d_parent->d_inode,temp_filp->f_path.dentry,NULL);
        mutex_unlock(&temp_path.dentry->d_parent->d_inode->i_mutex);
        if(rename!=0){
            logMessage(curParams->flags,"ERROR","VFS Unlinkng error.");
            goto out_close3;
        }

        mutex_lock_nested(&temp_path.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
        temp_lookup_len = lookup_one_len(temp_path.dentry->d_iname,temp_path.dentry->d_parent,strlen(temp_path.dentry->d_iname));
        mutex_unlock(&temp_path.dentry->d_parent->d_inode->i_mutex);
        if(IS_ERR(temp_lookup_len)){
            logMessage(curParams->flags,"ERROR","In Finding Parent of the file lookup_one_len()");
            retval = (int) PTR_ERR(temp_lookup_len);
            goto out_close3;
        }

        logMessage(curParams->flags,"INFO","Linking the file.");
//       retval = vfs_link(in_filp2->f_path.dentry,in_filp1->f_path.dentry->d_parent->d_inode,in_filp1->f_path.dentry,NULL);
        mutex_lock_nested(&path1.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
        rename = vfs_link (in_filp1->f_path.dentry,in_filp1->f_path.dentry->d_parent->d_inode,temp_lookup_len,NULL);
        mutex_unlock(&path1.dentry->d_parent->d_inode->i_mutex);
        if(rename!=0){
            logMessage(curParams->flags,"ERROR","VFS Unlinkng error.");
            goto out_close3;
        }
        else{
            logMessage(curParams->flags,"INFO","Renaming the file to output file");
            mutex_lock_nested(&path2.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
            lookup_len = lookup_one_len(path2.dentry->d_iname,path2.dentry->d_parent,strlen(path2.dentry->d_iname));
            mutex_unlock(&path2.dentry->d_parent->d_inode->i_mutex);
            if(IS_ERR(lookup_len)){
                logMessage(curParams->flags,"ERROR","In Finding Parent of the file lookup_one_len()");
                retval = (int) PTR_ERR(lookup_len);
                goto out_close2;
            }

            lock_rename(temp_lookup_len->d_parent,lookup_len->d_parent);
            rename = vfs_rename(temp_path.dentry->d_parent->d_inode, temp_lookup_len, path2.dentry->d_parent->d_inode,lookup_len, NULL, 0);
            unlock_rename(temp_lookup_len->d_parent,lookup_len->d_parent);
            if(rename!=0){
                logMessage(curParams->flags,"ERROR","Error in renaming the file");
                retval = -rename;
                goto out;
            }
        }
    }


	goto out_closeTemp;

    out_close3:
        filp_close(out_filp,NULL);
    out_unlinkTempFile:
        temp_kernpath=kern_path(tempName, LOOKUP_FOLLOW, &temp_path);
        if(out_kern_path!=0){
            retval = -EINVAL;
            goto out_close2;
        }
        mutex_lock_nested(&temp_path.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
        vfs_unlink(temp_filp->f_path.dentry->d_parent->d_inode,temp_filp->f_path.dentry,NULL);
        mutex_unlock(&temp_path.dentry->d_parent->d_inode->i_mutex);
    out_closeTemp:
        filp_close(temp_filp,NULL);
	out_close2:
		filp_close(in_filp2,NULL);
	out_close1:
		filp_close(in_filp1,NULL);
    kfree_in_buf2:
        kfree(in_buf2);
    kfree_in_buf1:
        kfree(in_buf1);
	out:
        printk("====================================================================\n");
        kfree(curParams);
		return retval;
}

static void logMessage(u_int flag,char *logType, char *logMessage){
    if(flag & FLAG_D){
        printk("%s : %s\n",logType,logMessage);
    }
    else if(!strcmp(logType,"ERROR")){
        printk("%s : %s\n",logType,logMessage);
    }

}

static int __init init_sys_xdedup(void)
{
	printk("installed new sys_xdedup module\n");
	if (sysptr == NULL)
		sysptr = xdedup;
	return 0;
}
static void  __exit exit_sys_xdedup(void)
{
	if (sysptr != NULL)
		sysptr = NULL;
	printk("removed sys_xdedup module\n");
}
module_init(init_sys_xdedup);
module_exit(exit_sys_xdedup);
MODULE_LICENSE("GPL");

//	in_inode1 = in_filp1->f_path.dentry->d_inode;
//	in_inode2 = in_filp2->f_path.dentry->d_inode;
//	mutex_lock_nested(&in_filp2->f_path.dentry->d_inode->i_mutex, I_MUTEX_PARENT);
//	printk("Unlinking the file");

//    if(!(curParams->flags & FLAG_N)&&!(curParams->flags & FLAG_P)){
//        printk("Deduping files\n");
//        printk("Getting Kern Paths\n");
//        printk("Locking file2 directory for unlinking\n");
//		mutex_lock_nested(&path2.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
//		printk("Unlinking the file\n");
//        retval = vfs_unlink(in_filp2->f_path.dentry->d_parent->d_inode,in_filp2->f_path.dentry,NULL);
//        mutex_unlock(&path2.dentry->d_parent->d_inode->i_mutex);
//        if(retval<0){
//            retval = -10;
//            printk("VFS Unlinkng error");
//        }
////		retval = vfs_unlink(path2.dentry->d_parent->d_inode, in_filp2->f_path.dentry,NULL);
//        printk("Second File path %s \n",curParams->in_file2);
//        printk("Second file name %s for lookup_one_len \n",path2.dentry->d_iname);
//	    mutex_lock_nested(&path2.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
//		lookup_len = lookup_one_len(path2.dentry->d_iname,path2.dentry->d_parent,strlen(path2.dentry->d_iname));
//        mutex_unlock(&path2.dentry->d_parent->d_inode->i_mutex);
//        if(IS_ERR(lookup_len)){
//            printk("Error in lookup one len");
//            retval = (int) PTR_ERR(lookup_len);
//            goto out_close2;
//        }
//		printk("Linking the file\n");
////       retval = vfs_link(in_filp2->f_path.dentry,in_filp1->f_path.dentry->d_parent->d_inode,in_filp1->f_path.dentry,NULL);
//        mutex_lock_nested(&path1.dentry->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
//		retval = vfs_link (in_filp1->f_path.dentry,in_filp1->f_path.dentry->d_parent->d_inode,lookup_len,NULL);
//        mutex_unlock(&path1.dentry->d_parent->d_inode->i_mutex);
//        if(retval<0){
//            retval = -10;
//            printk("VFS Linking error");
//        }
//    }


/* dummy syscall: returns 0 for non null, -EINVAL for NULL */
//	if (arg == NULL)
//		return -EINVAL;
//	else
//		return 0;