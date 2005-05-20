/*
 * NTOSKRNL Tag names.
 * License GPL
 *
 * FIXME: Replace with standard GPL Header.
 * FIXME: Add notes as needed
 */

#ifndef _NTOSKRNL_TAG_H
#define _NTOSKRNL_TAG_H

/* formerly located in cc/view.c */
#define TAG_CSEG  TAG('C', 'S', 'E', 'G')
#define TAG_BCB   TAG('B', 'C', 'B', ' ')
#define TAG_IBCB  TAG('i', 'B', 'C', 'B')

/* formerly located in ex/resource.c */
#define TAG_OWNER_TABLE     TAG('R', 'O', 'W', 'N')
#define TAG_EXCLUSIVE_LOCK  TAG('E', 'R', 'E', 'L')
#define TAG_SHARED_SEM      TAG('E', 'R', 'S', 'S')

/* formerly located in fs/notify.c */
#define FSRTL_NOTIFY_TAG TAG('N','O','T','I')

/* formerly located in io/device.c */
#define TAG_DEVICE_EXTENSION   TAG('D', 'E', 'X', 'T')
#define TAG_SHUTDOWN_ENTRY    TAG('S', 'H', 'U', 'T')
#define TAG_IO_TIMER      TAG('I', 'O', 'T', 'M')

/* formerly located in io/driver.c */
#define TAG_DRIVER             TAG('D', 'R', 'V', 'R')
#define TAG_DRIVER_EXTENSION   TAG('D', 'R', 'V', 'E')

/* formerly located in io/file.c */
#define TAG_SYSB        TAG('S', 'Y', 'S', 'B')
#define TAG_LOCK        TAG('F','l','c','k')
#define TAG_FILE_NAME   TAG('F', 'N', 'A', 'M')

/* formerly located in io/fs.c */
#define TAG_FILE_SYSTEM       TAG('F', 'S', 'Y', 'S')
#define TAG_FS_CHANGE_NOTIFY  TAG('F', 'S', 'C', 'N')

/* formerly located in io/iocomp.c */
#define IOC_TAG   TAG('I', 'O', 'C', 'T')

/* formerly located in io/iomgr.c */
#define TAG_DEVICE_TYPE     TAG('D', 'E', 'V', 'T')
#define TAG_FILE_TYPE       TAG('F', 'I', 'L', 'E')
#define TAG_ADAPTER_TYPE    TAG('A', 'D', 'P', 'T')
#define IO_LARGEIRP         TAG('I', 'r', 'p', 'l')
#define IO_SMALLIRP         TAG('I', 'r', 'p', 's')
#define IO_LARGEIRP_CPU     TAG('I', 'r', 'p', 'L')
#define IO_SMALLIRP_CPU     TAG('I', 'r', 'p', 'S')
#define IOC_TAG1             TAG('I', 'p', 'c', ' ')
#define IOC_CPU             TAG('I', 'p', 'c', 'P')

/* formerly located in io/work.c */
#define TAG_IOWI TAG('I', 'O', 'W', 'I')

/* formerly located in io/irp.c */
#define TAG_IRP      TAG('I', 'R', 'P', ' ')
#define TAG_SYS_BUF  TAG('S', 'Y', 'S' , 'B')

/* formerly located in io/irq.c */
#define TAG_KINTERRUPT   TAG('K', 'I', 'S', 'R')


#endif /* _NTOSKRNL_TAG_H */
