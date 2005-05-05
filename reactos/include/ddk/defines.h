#ifndef _DDK_DEFINES_H
#define _DDK_DEFINES_H

/* GENERAL DEFINITIONS ****************************************************/

#ifndef __ASM__

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <ddk/kedef.h>
#include <ddk/iodef.h>

/*
 * PURPOSE: Number of a thread priority levels
 */
#define NR_PRIORITY_LEVELS (32)

/*
 * PURPOSE: Object attributes
 */
enum
{
   OBJ_INHERIT = 0x2,
   OBJ_PERMANENT = 0x10,
   OBJ_EXCLUSIVE = 0x20,
   OBJ_CASE_INSENSITIVE = 0x40,
   OBJ_OPENIF = 0x80,
   OBJ_OPENLINK = 0x100,
   OBJ_KERNEL_HANDLE = 0x200,
   OBJ_VALID_ATTRIBUTES = 0x3F2,
};

/*
 * PURPOSE: Timer types
 */
enum
{
   NotificationTimer,
   SynchronizationTimer,
};

/*
 * PURPOSE: Some drivers use these
 */
#ifndef IN
#define IN
#define OUT
#define OPTIONAL
#endif

/*
 * PURPOSE: Arguments to MmProbeAndLockPages
 */
enum
{
   IoReadAccess,
   IoWriteAccess,
   IoModifyAccess,
};

#define MAXIMUM_VOLUME_LABEL_LENGTH (32)

#include <ddk/i386/irql.h>

#define PASSIVE_LEVEL	0		// Passive release level
#define LOW_LEVEL	0		// Lowest interrupt level
#define APC_LEVEL	1		// APC interrupt level
#define DISPATCH_LEVEL	2		// Dispatcher level
/* timer used for profiling */
#define PROFILE_LEVEL	27		
/* Interval clock 1 level - Not used on x86 */
#define CLOCK1_LEVEL	28		
#define CLOCK2_LEVEL	28		// Interval clock 2 level
#define IPI_LEVEL	29		// Interprocessor interrupt level 
#define POWER_LEVEL	30		// Power failure level
#define HIGH_LEVEL	31		// Highest interrupt level
#define SYNCH_LEVEL	(IPI_LEVEL-1)	// synchronization level

#define WINSTA_ACCESSCLIPBOARD	(0x4L)
#define WINSTA_ACCESSGLOBALATOMS	(0x20L)
#define WINSTA_CREATEDESKTOP	(0x8L)
#define WINSTA_ENUMDESKTOPS	(0x1L)
#define WINSTA_ENUMERATE	(0x100L)
#define WINSTA_EXITWINDOWS	(0x40L)
#define WINSTA_READATTRIBUTES	(0x2L)
#define WINSTA_READSCREEN	(0x200L)
#define WINSTA_WRITEATTRIBUTES	(0x10L)

#define DF_ALLOWOTHERACCOUNTHOOK	(0x1L)
#define DESKTOP_CREATEMENU	(0x4L)
#define DESKTOP_CREATEWINDOW	(0x2L)
#define DESKTOP_ENUMERATE	(0x40L)
#define DESKTOP_HOOKCONTROL	(0x8L)
#define DESKTOP_JOURNALPLAYBACK	(0x20L)
#define DESKTOP_JOURNALRECORD	(0x10L)
#define DESKTOP_READOBJECTS	(0x1L)
#define DESKTOP_SWITCHDESKTOP	(0x100L)
#define DESKTOP_WRITEOBJECTS	(0x80L)

#endif /* __ASM__ */

/* Values returned by KeGetPreviousMode() */
#define KernelMode (0)
#define UserMode   (1)

#endif /* ndef _DDK_DEFINES_H */
