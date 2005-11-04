/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#ifndef __S3V_LOCK_H__
#define __S3V_LOCK_H__

#include <sys/ioctl.h>

extern void s3vGetLock( s3vContextPtr vmesa, GLuint flags );

/* Turn DEBUG_LOCKING on to find locking conflicts.
 */
#define DEBUG_LOCKING	0

#if DEBUG_LOCKING
extern char *prevLockFile;
extern int prevLockLine;

#define DEBUG_LOCK() \
   do {	\
      prevLockFile = (__FILE__); \
      prevLockLine = (__LINE__); \
   } while (0)

#define DEBUG_RESET() \
   do {	\
      prevLockFile = 0;	\
      prevLockLine = 0;	\
   } while (0)

#define DEBUG_CHECK_LOCK() \
   do {	\
      if ( prevLockFile ) { \
	 fprintf( stderr, \
		  "LOCK SET!\n\tPrevious %s:%d\n\tCurrent: %s:%d\n", \
		  prevLockFile, prevLockLine, __FILE__, __LINE__ );	\
	 exit(1); \
      }	\
   } while (0)

#else

#define DEBUG_LOCK()
#define DEBUG_RESET()
#define DEBUG_CHECK_LOCK()

#endif

/*
 * !!! We may want to separate locks from locks with validation.  This
 * could be used to improve performance for those things commands that
 * do not do any drawing !!!
 */

/* Lock the hardware and validate our state.
 */
#define LOCK_HARDWARE( vmesa ) \
   do {	\
      char __ret = 0; \
      DEBUG_CHECK_LOCK(); \
      DRM_CAS( vmesa->driHwLock, vmesa->hHWContext, \
	       (DRM_LOCK_HELD | vmesa->hHWContext), __ret ); \
      if ( __ret ) \
         s3vGetLock( vmesa, 0 ); \
      DEBUG_LOCK(); \
   } while (0)

/* Unlock the hardware.
 */
#define UNLOCK_HARDWARE( vmesa ) \
   do { \
      DRM_UNLOCK( vmesa->driFd, \
		  vmesa->driHwLock, \
		  vmesa->hHWContext ); \
      DEBUG_RESET(); \
   } while (0)

#define S3VHW_LOCK( vmesa )	\
   DRM_UNLOCK(vmesa->driFd, vmesa->driHwLock, vmesa->hHWContext); \
   DRM_SPINLOCK(&vmesa->driScreen->pSAREA->drawable_lock, \
		 vmesa->driScreen->drawLockID); \
   /* VALIDATE_DRAWABLE_INFO_NO_LOCK(vmesa); */

#define S3VHW_UNLOCK( vmesa ) \
    DRM_SPINUNLOCK(&vmesa->driScreen->pSAREA->drawable_lock, \
		   vmesa->driScreen->drawLockID); \
    /* VALIDATE_DRAWABLE_INFO_NO_LOCK_POST(vmesa); */

#define S3V_SIMPLE_LOCK( vmesa ) \
	ioctl(vmesa->driFd, 0x4a) 

#define S3V_SIMPLE_FLUSH_LOCK( vmesa ) \
	ioctl(vmesa->driFd, 0x4b) 

#define S3V_SIMPLE_UNLOCK( vmesa ) \
	ioctl(vmesa->driFd, 0x4c) 

#endif /* __S3V_LOCK_H__ */
