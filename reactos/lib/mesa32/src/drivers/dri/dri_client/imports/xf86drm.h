/**
 * \file xf86drm.h 
 * OS-independent header for DRM user-level library interface.
 *
 * \author Rickard E. (Rik) Faith <faith@valinux.com>
 */
 
/*
 * Copyright 1999, 2000 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86drm.h,v 1.26 2003/08/16 19:26:37 dawes Exp $ */

#ifndef _XF86DRM_H_
#define _XF86DRM_H_

#include <drm.h>

				/* Defaults, if nothing set in xf86config */
#define DRM_DEV_UID	 0
#define DRM_DEV_GID	 0
/* Default /dev/dri directory permissions 0755 */
#define DRM_DEV_DIRMODE	 	\
	(S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
#define DRM_DEV_MODE	 (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)

#define DRM_DIR_NAME  "/dev/dri"
#define DRM_DEV_NAME  "%s/card%d"
#define DRM_PROC_NAME "/proc/dri/" /* For backward Linux compatibility */

#define DRM_ERR_NO_DEVICE  (-1001)
#define DRM_ERR_NO_ACCESS  (-1002)
#define DRM_ERR_NOT_ROOT   (-1003)
#define DRM_ERR_INVALID    (-1004)
#define DRM_ERR_NO_FD      (-1005)

#define DRM_AGP_NO_HANDLE 0

typedef unsigned int  drmSize,     *drmSizePtr;	    /**< For mapped regions */
typedef void          *drmAddress, **drmAddressPtr; /**< For mapped regions */

/**
 * Driver version information.
 *
 * \sa drmGetVersion() and drmSetVersion().
 */
typedef struct _drmVersion {
    int     version_major;        /**< Major version */
    int     version_minor;        /**< Minor version */
    int     version_patchlevel;   /**< Patch level */
    int     name_len; 	          /**< Length of name buffer */
    char    *name;	          /**< Name of driver */
    int     date_len;             /**< Length of date buffer */
    char    *date;                /**< User-space buffer to hold date */
    int     desc_len;	          /**< Length of desc buffer */
    char    *desc;                /**< User-space buffer to hold desc */
} drmVersion, *drmVersionPtr;

typedef struct _drmStats {
    unsigned long count;	     /**< Number of data */
    struct {
	unsigned long value;	     /**< Value from kernel */
	const char    *long_format;  /**< Suggested format for long_name */
	const char    *long_name;    /**< Long name for value */
	const char    *rate_format;  /**< Suggested format for rate_name */
	const char    *rate_name;    /**< Short name for value per second */
	int           isvalue;       /**< True if value (vs. counter) */
	const char    *mult_names;   /**< Multiplier names (e.g., "KGM") */
	int           mult;          /**< Multiplier value (e.g., 1024) */
	int           verbose;       /**< Suggest only in verbose output */
    } data[15];
} drmStatsT;


				/* All of these enums *MUST* match with the
                                   kernel implementation -- so do *NOT*
                                   change them!  (The drmlib implementation
                                   will just copy the flags instead of
                                   translating them.) */
typedef enum {
    DRM_FRAME_BUFFER    = 0,      /**< WC, no caching, no core dump */
    DRM_REGISTERS       = 1,      /**< no caching, no core dump */
    DRM_SHM             = 2,      /**< shared, cached */
    DRM_AGP             = 3,	  /**< AGP/GART */
    DRM_SCATTER_GATHER  = 4	  /**< PCI scatter/gather */
} drmMapType;

typedef enum {
    DRM_RESTRICTED      = 0x0001, /**< Cannot be mapped to client-virtual */
    DRM_READ_ONLY       = 0x0002, /**< Read-only in client-virtual */
    DRM_LOCKED          = 0x0004, /**< Physical pages locked */
    DRM_KERNEL          = 0x0008, /**< Kernel requires access */
    DRM_WRITE_COMBINING = 0x0010, /**< Use write-combining, if available */
    DRM_CONTAINS_LOCK   = 0x0020, /**< SHM page that contains lock */
    DRM_REMOVABLE	= 0x0040  /**< Removable mapping */
} drmMapFlags;

/**
 * \warning These values *MUST* match drm.h
 */
typedef enum {
    /** \name Flags for DMA buffer dispatch */
    /*@{*/
    DRM_DMA_BLOCK        = 0x01, /**< 
				  * Block until buffer dispatched.
				  * 
				  * \note the buffer may not yet have been
				  * processed by the hardware -- getting a
				  * hardware lock with the hardware quiescent
				  * will ensure that the buffer has been
				  * processed.
				  */
    DRM_DMA_WHILE_LOCKED = 0x02, /**< Dispatch while lock held */
    DRM_DMA_PRIORITY     = 0x04, /**< High priority dispatch */
    /*@}*/

    /** \name Flags for DMA buffer request */
    /*@{*/
    DRM_DMA_WAIT         = 0x10, /**< Wait for free buffers */
    DRM_DMA_SMALLER_OK   = 0x20, /**< Smaller-than-requested buffers OK */
    DRM_DMA_LARGER_OK    = 0x40  /**< Larger-than-requested buffers OK */
    /*@}*/
} drmDMAFlags;

typedef enum {
    DRM_PAGE_ALIGN       = 0x01,
    DRM_AGP_BUFFER       = 0x02,
    DRM_SG_BUFFER        = 0x04
} drmBufDescFlags;

typedef enum {
    DRM_LOCK_READY      = 0x01, /**< Wait until hardware is ready for DMA */
    DRM_LOCK_QUIESCENT  = 0x02, /**< Wait until hardware quiescent */
    DRM_LOCK_FLUSH      = 0x04, /**< Flush this context's DMA queue first */
    DRM_LOCK_FLUSH_ALL  = 0x08, /**< Flush all DMA queues first */
				/* These *HALT* flags aren't supported yet
                                   -- they will be used to support the
                                   full-screen DGA-like mode. */
    DRM_HALT_ALL_QUEUES = 0x10, /**< Halt all current and future queues */
    DRM_HALT_CUR_QUEUES = 0x20  /**< Halt all current queues */
} drmLockFlags;

typedef enum {
    DRM_CONTEXT_PRESERVED = 0x01, /**< This context is preserved and
				     never swapped. */
    DRM_CONTEXT_2DONLY    = 0x02  /**< This context is for 2D rendering only. */
} drm_context_tFlags, *drm_context_tFlagsPtr;

typedef struct _drmBufDesc {
    int              count;	  /**< Number of buffers of this size */
    int              size;	  /**< Size in bytes */
    int              low_mark;	  /**< Low water mark */
    int              high_mark;	  /**< High water mark */
} drmBufDesc, *drmBufDescPtr;

typedef struct _drmBufInfo {
    int              count;	  /**< Number of buffers described in list */
    drmBufDescPtr    list;	  /**< List of buffer descriptions */
} drmBufInfo, *drmBufInfoPtr;

typedef struct _drmBuf {
    int              idx;	  /**< Index into the master buffer list */
    int              total;	  /**< Buffer size */
    int              used;	  /**< Amount of buffer in use (for DMA) */
    drmAddress       address;	  /**< Address */
} drmBuf, *drmBufPtr;

/**
 * Buffer mapping information.
 *
 * Used by drmMapBufs() and drmUnmapBufs() to store information about the
 * mapped buffers.
 */
typedef struct _drmBufMap {
    int              count;	  /**< Number of buffers mapped */
    drmBufPtr        list;	  /**< Buffers */
} drmBufMap, *drmBufMapPtr;

typedef struct _drmLock {
    volatile unsigned int lock;
    char                      padding[60];
    /* This is big enough for most current (and future?) architectures:
       DEC Alpha:              32 bytes
       Intel Merced:           ?
       Intel P5/PPro/PII/PIII: 32 bytes
       Intel StrongARM:        32 bytes
       Intel i386/i486:        16 bytes
       MIPS:                   32 bytes (?)
       Motorola 68k:           16 bytes
       Motorola PowerPC:       32 bytes
       Sun SPARC:              32 bytes
    */
} drmLock, *drmLockPtr;

/**
 * Indices here refer to the offset into
 * list in drmBufInfo
 */
typedef struct _drmDMAReq {
    drm_context_t    context;  	  /**< Context handle */
    int           send_count;     /**< Number of buffers to send */
    int           *send_list;     /**< List of handles to buffers */
    int           *send_sizes;    /**< Lengths of data to send, in bytes */
    drmDMAFlags   flags;          /**< Flags */
    int           request_count;  /**< Number of buffers requested */
    int           request_size;	  /**< Desired size of buffers requested */
    int           *request_list;  /**< Buffer information */
    int           *request_sizes; /**< Minimum acceptable sizes */
    int           granted_count;  /**< Number of buffers granted at this size */
} drmDMAReq, *drmDMAReqPtr;

typedef struct _drmRegion {
    drm_handle_t     handle;
    unsigned int  offset;
    drmSize       size;
    drmAddress    map;
} drmRegion, *drmRegionPtr;

typedef struct _drmTextureRegion {
    unsigned char next;
    unsigned char prev;
    unsigned char in_use;
    unsigned char padding;	/**< Explicitly pad this out */
    unsigned int  age;
} drmTextureRegion, *drmTextureRegionPtr;


typedef enum {
    DRM_VBLANK_ABSOLUTE = 0x0,	/**< Wait for specific vblank sequence number */
    DRM_VBLANK_RELATIVE = 0x1,	/**< Wait for given number of vblanks */
    DRM_VBLANK_SIGNAL   = 0x40000000	/* Send signal instead of blocking */
} drmVBlankSeqType;

typedef struct _drmVBlankReq {
	drmVBlankSeqType type;
	unsigned int sequence;
	unsigned long signal;
} drmVBlankReq, *drmVBlankReqPtr;

typedef struct _drmVBlankReply {
	drmVBlankSeqType type;
	unsigned int sequence;
	long tval_sec;
	long tval_usec;
} drmVBlankReply, *drmVBlankReplyPtr;

typedef union _drmVBlank {
	drmVBlankReq request;
	drmVBlankReply reply;
} drmVBlank, *drmVBlankPtr;

typedef struct _drmSetVersion {
	int drm_di_major;
	int drm_di_minor;
	int drm_dd_major;
	int drm_dd_minor;
} drmSetVersion, *drmSetVersionPtr;


#define __drm_dummy_lock(lock) (*(__volatile__ unsigned int *)lock)

#define DRM_LOCK_HELD  0x80000000 /**< Hardware lock is held */
#define DRM_LOCK_CONT  0x40000000 /**< Hardware lock is contended */

#if defined(__GNUC__) && (__GNUC__ >= 2)
# if defined(__i386) || defined(__amd64__)
				/* Reflect changes here to drmP.h */
#define DRM_CAS(lock,old,new,__ret)                                    \
	do {                                                           \
                int __dummy;	/* Can't mark eax as clobbered */      \
		__asm__ __volatile__(                                  \
			"lock ; cmpxchg %4,%1\n\t"                     \
                        "setnz %0"                                     \
			: "=d" (__ret),                                \
   			  "=m" (__drm_dummy_lock(lock)),               \
                          "=a" (__dummy)                               \
			: "2" (old),                                   \
			  "r" (new));                                  \
	} while (0)

#elif defined(__alpha__)

#define	DRM_CAS(lock, old, new, ret) 		\
 	do {					\
 		int old32;                      \
 		int cur32;			\
 		__asm__ __volatile__(		\
 		"       mb\n"			\
 		"       zap   %4, 0xF0, %0\n"   \
 		"       ldl_l %1, %2\n"		\
 		"       zap   %1, 0xF0, %1\n"   \
                "       cmpeq %0, %1, %1\n"	\
                "       beq   %1, 1f\n"		\
 		"       bis   %5, %5, %1\n"	\
                "       stl_c %1, %2\n"		\
                "1:     xor   %1, 1, %1\n"	\
                "       stl   %1, %3"		\
                : "+r" (old32),                 \
		  "+&r" (cur32),		\
                   "=m" (__drm_dummy_lock(lock)),\
                   "=m" (ret)			\
 		: "r" (old),			\
 		  "r" (new));			\
 	} while(0)

#elif defined(__sparc__)

#define DRM_CAS(lock,old,new,__ret)				\
do {	register unsigned int __old __asm("o0");		\
	register unsigned int __new __asm("o1");		\
	register volatile unsigned int *__lock __asm("o2");	\
	__old = old;						\
	__new = new;						\
	__lock = (volatile unsigned int *)lock;			\
	__asm__ __volatile__(					\
		/*"cas [%2], %3, %0"*/				\
		".word 0xd3e29008\n\t"				\
		/*"membar #StoreStore | #StoreLoad"*/		\
		".word 0x8143e00a"				\
		: "=&r" (__new)					\
		: "0" (__new),					\
		  "r" (__lock),					\
		  "r" (__old)					\
		: "memory");					\
	__ret = (__new != __old);				\
} while(0)

#elif defined(__ia64__)

#ifdef __INTEL_COMPILER
/* this currently generates bad code (missing stop bits)... */
#include <ia64intrin.h>

#define DRM_CAS(lock,old,new,__ret)					      \
	do {								      \
		unsigned long __result, __old = (old) & 0xffffffff;		\
		__mf();							      	\
		__result = _InterlockedCompareExchange_acq(&__drm_dummy_lock(lock), (new), __old);\
		__ret = (__result) != (__old);					\
/*		__ret = (__sync_val_compare_and_swap(&__drm_dummy_lock(lock), \
						     (old), (new))	      \
			 != (old));					      */\
	} while (0)

#else
#define DRM_CAS(lock,old,new,__ret)					  \
	do {								  \
		unsigned int __result, __old = (old);			  \
		__asm__ __volatile__(					  \
			"mf\n"						  \
			"mov ar.ccv=%2\n"				  \
			";;\n"						  \
			"cmpxchg4.acq %0=%1,%3,ar.ccv"			  \
			: "=r" (__result), "=m" (__drm_dummy_lock(lock))  \
			: "r" ((unsigned long)__old), "r" (new)			  \
			: "memory");					  \
		__ret = (__result) != (__old);				  \
	} while (0)

#endif

#elif defined(__powerpc__)

#define DRM_CAS(lock,old,new,__ret)			\
	do {						\
		__asm__ __volatile__(			\
			"sync;"				\
			"0:    lwarx %0,0,%1;"		\
			"      xor. %0,%3,%0;"		\
			"      bne 1f;"			\
			"      stwcx. %2,0,%1;"		\
			"      bne- 0b;"		\
			"1:    "			\
			"sync;"				\
		: "=&r"(__ret)				\
		: "r"(lock), "r"(new), "r"(old)		\
		: "cr0", "memory");			\
	} while (0)

#endif /* architecture */
#endif /* __GNUC__ >= 2 */

#ifndef DRM_CAS
#define DRM_CAS(lock,old,new,ret) do { ret=1; } while (0) /* FAST LOCK FAILS */
#endif

#if defined(__alpha__) || defined(__powerpc__)
#define DRM_CAS_RESULT(_result)		int _result
#else
#define DRM_CAS_RESULT(_result)		char _result
#endif

#define DRM_LIGHT_LOCK(fd,lock,context)                                \
	do {                                                           \
                DRM_CAS_RESULT(__ret);                                 \
		DRM_CAS(lock,context,DRM_LOCK_HELD|context,__ret);     \
                if (__ret) drmGetLock(fd,context,0);                   \
        } while(0)

				/* This one counts fast locks -- for
                                   benchmarking only. */
#define DRM_LIGHT_LOCK_COUNT(fd,lock,context,count)                    \
	do {                                                           \
                DRM_CAS_RESULT(__ret);                                 \
		DRM_CAS(lock,context,DRM_LOCK_HELD|context,__ret);     \
                if (__ret) drmGetLock(fd,context,0);                   \
                else       ++count;                                    \
        } while(0)

#define DRM_LOCK(fd,lock,context,flags)                                \
	do {                                                           \
		if (flags) drmGetLock(fd,context,flags);               \
		else       DRM_LIGHT_LOCK(fd,lock,context);            \
	} while(0)

#define DRM_UNLOCK(fd,lock,context)                                    \
	do {                                                           \
                DRM_CAS_RESULT(__ret);                                 \
		DRM_CAS(lock,DRM_LOCK_HELD|context,context,__ret);     \
                if (__ret) drmUnlock(fd,context);                      \
        } while(0)

				/* Simple spin locks */
#define DRM_SPINLOCK(spin,val)                                         \
	do {                                                           \
            DRM_CAS_RESULT(__ret);                                     \
	    do {                                                       \
		DRM_CAS(spin,0,val,__ret);                             \
		if (__ret) while ((spin)->lock);                       \
	    } while (__ret);                                           \
	} while(0)

#define DRM_SPINLOCK_TAKE(spin,val)                                    \
	do {                                                           \
            DRM_CAS_RESULT(__ret);                                     \
            int  cur;                                                  \
	    do {                                                       \
                cur = (*spin).lock;                                    \
		DRM_CAS(spin,cur,val,__ret);                           \
	    } while (__ret);                                           \
	} while(0)

#define DRM_SPINLOCK_COUNT(spin,val,count,__ret)                       \
	do {                                                           \
            int  __i;                                                  \
            __ret = 1;                                                 \
            for (__i = 0; __ret && __i < count; __i++) {               \
		DRM_CAS(spin,0,val,__ret);                             \
		if (__ret) for (;__i < count && (spin)->lock; __i++);  \
	    }                                                          \
	} while(0)

#define DRM_SPINUNLOCK(spin,val)                                       \
	do {                                                           \
            DRM_CAS_RESULT(__ret);                                     \
            if ((*spin).lock == val) { /* else server stole lock */    \
	        do {                                                   \
		    DRM_CAS(spin,val,0,__ret);                         \
	        } while (__ret);                                       \
            }                                                          \
	} while(0)

/* General user-level programmer's API: unprivileged */
extern int           drmAvailable(void);
extern int           drmOpen(const char *name, const char *busid);
extern int           drmClose(int fd);
extern drmVersionPtr drmGetVersion(int fd);
extern drmVersionPtr drmGetLibVersion(int fd);
extern void          drmFreeVersion(drmVersionPtr);
extern int           drmGetMagic(int fd, drm_magic_t * magic);
extern char          *drmGetBusid(int fd);
extern int           drmGetInterruptFromBusID(int fd, int busnum, int devnum,
					      int funcnum);
extern int           drmGetMap(int fd, int idx, drm_handle_t *offset,
			       drmSize *size, drmMapType *type,
			       drmMapFlags *flags, drm_handle_t *handle,
			       int *mtrr);
extern int           drmGetClient(int fd, int idx, int *auth, int *pid,
				  int *uid, unsigned long *magic,
				  unsigned long *iocs);
extern int           drmGetStats(int fd, drmStatsT *stats);
extern int           drmSetInterfaceVersion(int fd, drmSetVersion *version);
extern int           drmCommandNone(int fd, unsigned long drmCommandIndex);
extern int           drmCommandRead(int fd, unsigned long drmCommandIndex,
                                    void *data, unsigned long size);
extern int           drmCommandWrite(int fd, unsigned long drmCommandIndex,
                                     void *data, unsigned long size);
extern int           drmCommandWriteRead(int fd, unsigned long drmCommandIndex,
                                         void *data, unsigned long size);

/* General user-level programmer's API: X server (root) only  */
extern void          drmFreeBusid(const char *busid);
extern int           drmSetBusid(int fd, const char *busid);
extern int           drmAuthMagic(int fd, drm_magic_t magic);
extern int           drmAddMap(int fd,
			       drm_handle_t offset,
			       drmSize size,
			       drmMapType type,
			       drmMapFlags flags,
			       drm_handle_t * handle);
extern int	     drmRmMap(int fd, drm_handle_t handle);
extern int	     drmAddContextPrivateMapping(int fd, drm_context_t ctx_id,
						 drm_handle_t handle);

extern int           drmAddBufs(int fd, int count, int size,
				drmBufDescFlags flags,
				int agp_offset);
extern int           drmMarkBufs(int fd, double low, double high);
extern int           drmCreateContext(int fd, drm_context_t * handle);
extern int           drmSetContextFlags(int fd, drm_context_t context,
					drm_context_tFlags flags);
extern int           drmGetContextFlags(int fd, drm_context_t context,
					drm_context_tFlagsPtr flags);
extern int           drmAddContextTag(int fd, drm_context_t context, void *tag);
extern int           drmDelContextTag(int fd, drm_context_t context);
extern void          *drmGetContextTag(int fd, drm_context_t context);
extern drm_context_t * drmGetReservedContextList(int fd, int *count);
extern void          drmFreeReservedContextList(drm_context_t *);
extern int           drmSwitchToContext(int fd, drm_context_t context);
extern int           drmDestroyContext(int fd, drm_context_t handle);
extern int           drmCreateDrawable(int fd, drm_drawable_t * handle);
extern int           drmDestroyDrawable(int fd, drm_drawable_t handle);
extern int           drmCtlInstHandler(int fd, int irq);
extern int           drmCtlUninstHandler(int fd);
extern int           drmInstallSIGIOHandler(int fd,
					    void (*f)(int fd,
						      void *oldctx,
						      void *newctx));
extern int           drmRemoveSIGIOHandler(int fd);

/* General user-level programmer's API: authenticated client and/or X */
extern int           drmMap(int fd,
			    drm_handle_t handle,
			    drmSize size,
			    drmAddressPtr address);
extern int           drmUnmap(drmAddress address, drmSize size);
extern drmBufInfoPtr drmGetBufInfo(int fd);
extern drmBufMapPtr  drmMapBufs(int fd);
extern int           drmUnmapBufs(drmBufMapPtr bufs);
extern int           drmDMA(int fd, drmDMAReqPtr request);
extern int           drmFreeBufs(int fd, int count, int *list);
extern int           drmGetLock(int fd,
			        drm_context_t context,
			        drmLockFlags flags);
extern int           drmUnlock(int fd, drm_context_t context);
extern int           drmFinish(int fd, int context, drmLockFlags flags);
extern int	     drmGetContextPrivateMapping(int fd, drm_context_t ctx_id, 
						 drm_handle_t * handle);

/* AGP/GART support: X server (root) only */
extern int           drmAgpAcquire(int fd);
extern int           drmAgpRelease(int fd);
extern int           drmAgpEnable(int fd, unsigned long mode);
extern int           drmAgpAlloc(int fd, unsigned long size,
				 unsigned long type, unsigned long *address,
				 unsigned long *handle);
extern int           drmAgpFree(int fd, unsigned long handle);
extern int 	     drmAgpBind(int fd, unsigned long handle,
				unsigned long offset);
extern int           drmAgpUnbind(int fd, unsigned long handle);

/* AGP/GART info: authenticated client and/or X */
extern int           drmAgpVersionMajor(int fd);
extern int           drmAgpVersionMinor(int fd);
extern unsigned long drmAgpGetMode(int fd);
extern unsigned long drmAgpBase(int fd); /* Physical location */
extern unsigned long drmAgpSize(int fd); /* Bytes */
extern unsigned long drmAgpMemoryUsed(int fd);
extern unsigned long drmAgpMemoryAvail(int fd);
extern unsigned int  drmAgpVendorId(int fd);
extern unsigned int  drmAgpDeviceId(int fd);

/* PCI scatter/gather support: X server (root) only */
extern int           drmScatterGatherAlloc(int fd, unsigned long size,
					   unsigned long *handle);
extern int           drmScatterGatherFree(int fd, unsigned long handle);

extern int           drmWaitVBlank(int fd, drmVBlankPtr vbl);

/* Support routines */
extern int           drmError(int err, const char *label);
extern void          *drmMalloc(int size);
extern void          drmFree(void *pt);

/* Hash table routines */
extern void *drmHashCreate(void);
extern int  drmHashDestroy(void *t);
extern int  drmHashLookup(void *t, unsigned long key, void **value);
extern int  drmHashInsert(void *t, unsigned long key, void *value);
extern int  drmHashDelete(void *t, unsigned long key);
extern int  drmHashFirst(void *t, unsigned long *key, void **value);
extern int  drmHashNext(void *t, unsigned long *key, void **value);

/* PRNG routines */
extern void          *drmRandomCreate(unsigned long seed);
extern int           drmRandomDestroy(void *state);
extern unsigned long drmRandom(void *state);
extern double        drmRandomDouble(void *state);

/* Skip list routines */

extern void *drmSLCreate(void);
extern int  drmSLDestroy(void *l);
extern int  drmSLLookup(void *l, unsigned long key, void **value);
extern int  drmSLInsert(void *l, unsigned long key, void *value);
extern int  drmSLDelete(void *l, unsigned long key);
extern int  drmSLNext(void *l, unsigned long *key, void **value);
extern int  drmSLFirst(void *l, unsigned long *key, void **value);
extern void drmSLDump(void *l);
extern int  drmSLLookupNeighbors(void *l, unsigned long key,
				 unsigned long *prev_key, void **prev_value,
				 unsigned long *next_key, void **next_value);

#endif
