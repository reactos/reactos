/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    privateice.c

Abstract:

Environment:

    LINUX 2.2.X/2.4.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    16-Jul-1998:	created
    15-Nov-2000:    general cleanup of source files
    19-Jan-2001:    renamed to privateice.c

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/config.h>
#include <linux/sched.h>
#include <asm/unistd.h>
#include <linux/string.h>
#include "precomp.h"
#include "serial.h" 

////////////////////////////////////////////////////
// GLOBALS
////

// this is for the command line to insmod (pice="...")
MODULE_AUTHOR("Klaus P. Gerlicher");
MODULE_DESCRIPTION("Linux system level symbolic debugger");

BOOLEAN bDeviceAlreadyOpen = FALSE;
int major_device_number;

char tempPICE[1024];

typedef asmlinkage int (*PFNMKNOD)(const char * filename, int mode, dev_t dev);
PFNMKNOD sys_mknod;
typedef asmlinkage int (*PFNUNLINK)(const char * pathname);
PFNUNLINK sys_unlink;

////////////////////////////////////////////////////
// FUNCTIONS
////

//*************************************************************************
// pice_open()
//
//*************************************************************************
static int pice_open(struct inode *inode, 
                         struct file *file)
{
    DPRINT((0,"pice_open(%p)\n", file));
    
    /* We don't want to talk to two processes at the 
    * same time */
    if (bDeviceAlreadyOpen)
        return -EBUSY;
    
    bDeviceAlreadyOpen = TRUE;
    
    MOD_INC_USE_COUNT;
    
    return 0;
}

//*************************************************************************
// pice_close()
//
//*************************************************************************
#if REAL_LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static int pice_close(struct inode *inode, 
                          struct file *file)
#else
static void pice_close(struct inode *inode, 
                           struct file *file)
#endif
{
    DPRINT((0,"device_release(%p,%p)\n", inode, file));
    
    /* We're now ready for our next caller */
    bDeviceAlreadyOpen = FALSE;
    
    MOD_DEC_USE_COUNT;
    
#if REAL_LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
    return 0;
#endif
}


//*************************************************************************
// pice_ioctl()
//
//*************************************************************************
static int pice_ioctl(struct inode *inode,struct file *file,unsigned int ioctl_num,unsigned long ioctl_param)
{
//	char* pFilename = (char*) ioctl_param;

	if(_IOC_TYPE(ioctl_num) != PICE_IOCTL_MAGIC)
        return -EINVAL;

    if(!capable(CAP_SYS_ADMIN))
    {
        Print(OUTPUT_WINDOW,"pICE: sorry, you must have superuser privileges\n");
        return -EINVAL;
    }

	switch(ioctl_num)
	{
		case PICE_IOCTL_LOAD:
            break;
		case PICE_IOCTL_RELOAD:
            if(!ReloadSymbols())
            {
			    PICE_sprintf(tempPICE,"pICE: not able to reload symbols\n");
			    Print(OUTPUT_WINDOW,tempPICE);
            }
			break;
		case PICE_IOCTL_UNLOAD:
            UnloadSymbols();
			break;
		case PICE_IOCTL_BREAK:
			PICE_sprintf(tempPICE,"pICE: forcible break\n");
			Print(OUTPUT_WINDOW,tempPICE);
            __asm__ __volatile("int $3");
            break;
		case PICE_IOCTL_STATUS:
            {
                PDEBUGGER_STATUS_BLOCK ustatus_block_p = (PDEBUGGER_STATUS_BLOCK)ioctl_param;
                DEBUGGER_STATUS_BLOCK kstatus_block;
                int err;

                err = verify_area(VERIFY_WRITE,ustatus_block_p ,sizeof(DEBUGGER_STATUS_BLOCK));
                if(err)return err;

                kstatus_block.Test = 0x12345678;
                copy_to_user(ustatus_block_p, &kstatus_block, sizeof(DEBUGGER_STATUS_BLOCK) );
            }
            break;
        default:
            return -EINVAL;
	}

    return 0;
}

/* This structure will hold the functions to be called 
 * when a process does something to the device we 
 * created. Since a pointer to this structure is kept in 
 * the devices table, it can't be local to
 * init_module. NULL is for unimplemented functions. */
struct file_operations pice_fops = {
  ioctl:   pice_ioctl,                                                           
  open:    pice_open,                                                            
  release: pice_close,                                                           
  /* all others are NULL -> default handler */                                       
};


//*************************************************************************
// RegisterDriver()
//
//*************************************************************************
int RegisterDriver(void)
{
	// register the driver
	major_device_number= register_chrdev(0,"pice",&pice_fops);
	return major_device_number;
}

//*************************************************************************
// UnregisterDriver()
//
//*************************************************************************
void UnregisterDriver(void)
{
	// unregister the driver
	unregister_chrdev(major_device_number,"pice");
}


//*************************************************************************
// init_module()
//
//*************************************************************************
int init_module(void)
{
    int err;

#ifdef DEBUG
    // first we enable output of debug strings to COM port
    DebugSetupSerial(1,115200);
#endif // DEBUG

	DPRINT((0,"init_module()\n"));

	// initialize debugger 
	if(InitPICE())
	{
		mm_segment_t oldfs;

        // tell system we're here
        if(RegisterDriver() < 0)
		{
		    CleanUpPICE();
			return -EFAULT;
		}
        
        sys_mknod = (PFNMKNOD)sys_call_table[__NR_mknod];
        sys_unlink = (PFNUNLINK)sys_call_table[__NR_unlink];
        if(!sys_mknod || !sys_unlink)
        {
            // tell system we're gone
            UnregisterDriver();
            // cleanup
		    CleanUpPICE();
			return -EFAULT;
        }
        // 

        oldfs = get_fs(); set_fs(KERNEL_DS);
        sys_unlink("/dev/pice0");
        if(0 > (err = sys_mknod("/dev/pice0",S_IFCHR|S_IRWXUGO,(major_device_number<<8))) )
        {
    		set_fs(oldfs);
            sprintf(tempPICE,"PICE: couldn't create device node (err = %u)\n",-err);
            Print(OUTPUT_WINDOW,tempPICE);
            // tell system we're gone
            UnregisterDriver();
            // cleanup
		    CleanUpPICE();
			return -EFAULT;
        }
        set_fs(oldfs);

		return 0;
	}

	return -EFAULT;
}

//*************************************************************************
// cleanup_module()
//
//*************************************************************************
void cleanup_module(void)
{
    mm_segment_t oldfs;

    DPRINT((0,"cleanup_module()\n"));

    // remove symbolic link
    if(sys_unlink)
    {
        oldfs = get_fs(); set_fs(KERNEL_DS);
        sys_unlink("/dev/pice0");
        set_fs(oldfs);
    }

    // remove all internal stuff
    CleanUpPICE();

    // tell system we're gone
    UnregisterDriver();
}


