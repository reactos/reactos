#ifndef __GAMMA_LOCK_H__
#define __GAMMA_LOCK_H__

extern void gammaGetLock( gammaContextPtr gmesa, GLuint flags );

/* Turn DEBUG_LOCKING on to find locking conflicts.
 */
#define DEBUG_LOCKING	0

#if DEBUG_LOCKING
extern char *prevLockFile;
extern int prevLockLine;

#define DEBUG_LOCK()							\
   do {									\
      prevLockFile = (__FILE__);					\
      prevLockLine = (__LINE__);					\
   } while (0)

#define DEBUG_RESET()							\
   do {									\
      prevLockFile = 0;							\
      prevLockLine = 0;							\
   } while (0)

#define DEBUG_CHECK_LOCK()						\
   do {									\
      if ( prevLockFile ) {						\
	 fprintf( stderr,						\
		  "LOCK SET!\n\tPrevious %s:%d\n\tCurrent: %s:%d\n",	\
		  prevLockFile, prevLockLine, __FILE__, __LINE__ );	\
	 exit( 1 );							\
      }									\
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
#define LOCK_HARDWARE( gmesa )						\
   do {									\
      char __ret = 0;							\
      DEBUG_CHECK_LOCK();						\
      DRM_CAS( gmesa->driHwLock, gmesa->hHWContext,			\
	       (DRM_LOCK_HELD | gmesa->hHWContext), __ret );		\
      if ( __ret )							\
	 gammaGetLock( gmesa, 0 );					\
      DEBUG_LOCK();							\
   } while (0)

/* Unlock the hardware.
 */
#define UNLOCK_HARDWARE( gmesa )					\
   do {									\
      DRM_UNLOCK( gmesa->driFd,						\
		  gmesa->driHwLock,					\
		  gmesa->hHWContext );					\
      DEBUG_RESET();							\
   } while (0)

#define GAMMAHW_LOCK( gmesa )						\
   DRM_UNLOCK(gmesa->driFd, gmesa->driHwLock, gmesa->hHWContext);	\
   DRM_SPINLOCK(&gmesa->driScreen->pSAREA->drawable_lock,		\
		 gmesa->driScreen->drawLockID);				\
   VALIDATE_DRAWABLE_INFO_NO_LOCK(gmesa);

#define GAMMAHW_UNLOCK( gmesa )						\
    DRM_SPINUNLOCK(&gmesa->driScreen->pSAREA->drawable_lock,		\
		   gmesa->driScreen->drawLockID);			\
    VALIDATE_DRAWABLE_INFO_NO_LOCK_POST(gmesa);

#endif /* __GAMMA_LOCK_H__ */
