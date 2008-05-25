/*
******************************************************************************
*
*   Copyright (C) 1997-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* File CMUTEX.C
*
* Modification History:
*
*   Date        Name        Description
*   04/02/97    aliu        Creation.
*   04/07/99    srl         updated
*   05/13/99    stephen     Changed to umutex (from cmutex).
*   11/22/99    aliu        Make non-global mutex autoinitialize [j151]
******************************************************************************
*/

#include "unicode/utypes.h"
#include "uassert.h"
#include "ucln_cmn.h"

#if defined(U_DARWIN)
#include <AvailabilityMacros.h>
#if (ICU_USE_THREADS == 1) && defined(MAC_OS_X_VERSION_10_4) && defined(MAC_OS_X_VERSION_MIN_REQUIRED) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
#include <libkern/OSAtomic.h>
#define USE_MAC_OS_ATOMIC_INCREMENT 1
#endif
#endif

/* Assume POSIX, and modify as necessary below */
#define POSIX

#if defined(U_WINDOWS)
#undef POSIX
#endif
#if defined(macintosh)
#undef POSIX
#endif
#if defined(OS2)
#undef POSIX
#endif

#if defined(POSIX) && (ICU_USE_THREADS==1)
# include <pthread.h> /* must be first, so that we get the multithread versions of things. */

#endif /* POSIX && (ICU_USE_THREADS==1) */

#ifdef U_WINDOWS
# define WIN32_LEAN_AND_MEAN
# define VC_EXTRALEAN
# define NOUSER
# define NOSERVICE
# define NOIME
# define NOMCX
# include <windows.h>
#endif

#include "umutex.h"
#include "cmemory.h"

/*
 * A note on ICU Mutex Initialization and ICU startup:
 *
 *   ICU mutexes, as used through the rest of the ICU code, are self-initializing.
 *   To make this work, ICU uses the _ICU GLobal Mutex_ to synchronize the lazy init
 *   of other ICU mutexes.  For the global mutex itself, we need some other mechanism
 *   to safely initialize it on first use.  This becomes important if two or more
 *   threads were more or less simultaenously the first to use ICU in a process, and
 *   were racing into the mutex initialization code.
 *
 *   The solution for the global mutex init is platform dependent.
 *   On POSIX systems, C-style init can be used on a mutex, with the 
 *   macro PTHREAD_MUTEX_INITIALIZER.  The mutex is then ready for use, without
 *   first calling pthread_mutex_init().
 *
 *   Windows has no equivalent statically initialized mutex or CRITICAL SECION.
 *   InitializeCriticalSection() must be called.  If the global mutex does not
 *   appear to be initialized, a thread will create and initialize a new
 *   CRITICAL_SECTION, then use a Windows InterlockedCompareAndExchange to
 *   avoid problems with race conditions.
 *
 *   If an application has overridden the ICU mutex implementation
 *   by calling u_setMutexFunctions(), the user supplied init function must
 *   be safe in the event that multiple threads concurrently attempt to init
 *   the same mutex.  The first thread should do the init, and the others should
 *   have no effect.
 *
 */ 

#define  MAX_MUTEXES  30
static UMTX              gGlobalMutex          = NULL;
static UMTX              gIncDecMutex          = NULL;       
#if (ICU_USE_THREADS == 1)
static UBool             gMutexPoolInitialized = FALSE;
static char              gMutexesInUse[MAX_MUTEXES];   

#if defined(U_WINDOWS) 
/*-------------------------------------------------------------
 *
 *   WINDOWS  platform variable declarations
 *
 *-------------------------------------------------------------*/
static CRITICAL_SECTION  gMutexes[MAX_MUTEXES];
static CRITICAL_SECTION  gGlobalWinMutex;


/* On WIN32 mutexes are reentrant.  This makes it difficult to debug
 * deadlocking problems that show up on POSIXy platforms, where
 * mutexes deadlock upon reentry.  ICU contains checking code for
 * the global mutex as well as for other mutexes in the pool.
 *
 * This is for debugging purposes.
 *
 * This has no effect on non-WIN32 platforms, non-DEBUG builds, and
 * non-ICU_USE_THREADS builds.
 *
 * Note: The CRITICAL_SECTION structure already has a RecursionCount
 * member that can be used for this purpose, but portability to
 * Win98/NT/2K needs to be tested before use.  Works fine on XP.
 * After portability is confirmed, the built-in RecursionCount can be
 * used, and the gRecursionCountPool can be removed.
 *
 * Note: Non-global mutex checking only happens if there is no custom
 * pMutexLockFn defined.  Use one function, not two (don't use
 * pMutexLockFn and pMutexUnlockFn) so the increment and decrement of
 * the recursion count don't get out of sync.  Users might set just
 * one function, e.g., to perform a custom action, followed by a
 * standard call to EnterCriticalSection.
 */
#if defined(U_DEBUG) && (ICU_USE_THREADS==1)
static int32_t gRecursionCount = 0; /* detect global mutex locking */      
static int32_t gRecursionCountPool[MAX_MUTEXES]; /* ditto for non-global */
#endif


#elif defined(POSIX) 
/*-------------------------------------------------------------
 *
 *   POSIX   platform variable declarations
 *
 *-------------------------------------------------------------*/
static pthread_mutex_t   gMutexes[MAX_MUTEXES] = {
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER
};

#else 
/*-------------------------------------------------------------
 *
 *   UNKNOWN   platform  declarations
 *
 *-------------------------------------------------------------*/
static void *gMutexes[MAX_MUTEXES] = {
    NULL, NULL, NULL,
    NULL, NULL, NULL,
    NULL, NULL, NULL,
    NULL, NULL, NULL,
    NULL, NULL, NULL,
    NULL, NULL, NULL,
    NULL, NULL };

/* Unknown platform.  OK so long as ICU_USE_THREAD is not set.  
                      Note that user can still set mutex functions at run time,
                      and that the global mutex variable is still needed in that case. */
#if (ICU_USE_THREADS == 1)
#error no ICU mutex implementation for this platform
#endif
#endif
#endif /* ICU_USE_THREADS==1 */




/*
 *  User mutex implementation functions.  If non-null, call back to these rather than
 *  directly using the system (Posix or Windows) APIs.
 *    (declarations are in uclean.h)
 */
static UMtxInitFn    *pMutexInitFn    = NULL;
static UMtxFn        *pMutexDestroyFn = NULL;
static UMtxFn        *pMutexLockFn    = NULL;
static UMtxFn        *pMutexUnlockFn  = NULL;
static const void    *gMutexContext   = NULL;



/*
 *   umtx_lock
 */
U_CAPI void  U_EXPORT2
umtx_lock(UMTX *mutex)
{
    if (mutex == NULL) {
        mutex = &gGlobalMutex;
    }

    if (*mutex == NULL) {
        /* Lock of an uninitialized mutex.  Initialize it before proceeding.   */
        umtx_init(mutex);    
    }

    if (pMutexLockFn != NULL) {
        (*pMutexLockFn)(gMutexContext, mutex);
    } else {

#if (ICU_USE_THREADS == 1)
#if defined(U_WINDOWS)
        EnterCriticalSection((CRITICAL_SECTION*) *mutex);
#elif defined(POSIX)
        pthread_mutex_lock((pthread_mutex_t*) *mutex);
#endif   /* cascade of platforms */
#endif /* ICU_USE_THREADS==1 */
    }

#if defined(U_WINDOWS) && defined(U_DEBUG) && (ICU_USE_THREADS==1)
    if (mutex == &gGlobalMutex) {         /* Detect Reentrant locking of the global mutex.      */
        gRecursionCount++;                /* Recursion causes deadlocks on Unixes.              */
        U_ASSERT(gRecursionCount == 1);   /* Detection works on Windows.  Debug problems there. */
    }
    /* This handles gGlobalMutex too, but only if there is no pMutexLockFn */
    else if (pMutexLockFn == NULL) { /* see comments above */
        size_t i = ((CRITICAL_SECTION*)*mutex) - &gMutexes[0];
        U_ASSERT(i >= 0 && i < MAX_MUTEXES);
        ++gRecursionCountPool[i];

        U_ASSERT(gRecursionCountPool[i] == 1); /* !Detect Deadlock! */

        /* This works and is fast, but needs testing on Win98/NT/2K.
           See comments above. [alan]
          U_ASSERT((CRITICAL_SECTION*)*mutex >= &gMutexes[0] &&
                   (CRITICAL_SECTION*)*mutex <= &gMutexes[MAX_MUTEXES]);
          U_ASSERT(((CRITICAL_SECTION*)*mutex)->RecursionCount == 1);
        */
    }
#endif /*U_DEBUG*/
}



/*
 * umtx_unlock
 */
U_CAPI void  U_EXPORT2
umtx_unlock(UMTX* mutex)
{
    if(mutex == NULL) {
        mutex = &gGlobalMutex;
    }

    if(*mutex == NULL)    {
#if (ICU_USE_THREADS == 1)
        U_ASSERT(FALSE);  /* This mutex is not initialized.     */
#endif
        return; 
    }

#if defined (U_WINDOWS) && defined (U_DEBUG) && (ICU_USE_THREADS==1)
    if (mutex == &gGlobalMutex) {
        gRecursionCount--;
        U_ASSERT(gRecursionCount == 0);  /* Detect unlock of an already unlocked mutex */
    }
    /* This handles gGlobalMutex too, but only if there is no pMutexLockFn */
    else if (pMutexLockFn == NULL) { /* see comments above */
        size_t i = ((CRITICAL_SECTION*)*mutex) - &gMutexes[0];
        U_ASSERT(i >= 0 && i < MAX_MUTEXES);
        --gRecursionCountPool[i];

        U_ASSERT(gRecursionCountPool[i] == 0); /* !Detect Deadlock! */

        /* This works and is fast, but needs testing on Win98/NT/2K.
           Note that RecursionCount will be 1, not 0, since we haven't
           left the CRITICAL_SECTION yet.  See comments above. [alan]
          U_ASSERT((CRITICAL_SECTION*)*mutex >= &gMutexes[0] &&
                   (CRITICAL_SECTION*)*mutex <= &gMutexes[MAX_MUTEXES]);
          U_ASSERT(((CRITICAL_SECTION*)*mutex)->RecursionCount == 1);
        */
    }
#endif

    if (pMutexUnlockFn) {
        (*pMutexUnlockFn)(gMutexContext, mutex);
    } else {
#if (ICU_USE_THREADS==1)
#if defined (U_WINDOWS)
        LeaveCriticalSection((CRITICAL_SECTION*)*mutex);
#elif defined (POSIX)
        pthread_mutex_unlock((pthread_mutex_t*)*mutex);
#endif  /* cascade of platforms */
#endif /* ICU_USE_THREADS == 1 */
    }
}




/*
 *   initGlobalMutex    Do the platform specific initialization of the ICU global mutex.
 *                      Separated out from the other mutexes because it is different:
 *                      Mutex storage is static for POSIX, init must be thread safe 
 *                      without the use of another mutex.
 */
static void initGlobalMutex() {
    /*
     * If User Supplied mutex functions are in use
     *    init the icu global mutex using them.  
     */
    if (pMutexInitFn != NULL) {
        if (gGlobalMutex==NULL) {
            UErrorCode status = U_ZERO_ERROR;
            (*pMutexInitFn)(gMutexContext, &gGlobalMutex, &status);
            if (U_FAILURE(status)) {
                /* TODO:  how should errors here be handled? */
                return;
            }
        }
        return;
    }

    /* No user override of mutex functions.
     *   Use default ICU mutex implementations.
     */
#if (ICU_USE_THREADS == 1)
    /*
     *  for Windows, init the pool of critical sections that we
     *    will use as needed for ICU mutexes.
     */
#if defined (U_WINDOWS)
    if (gMutexPoolInitialized == FALSE) {
        int i;
        for (i=0; i<MAX_MUTEXES; i++) {
            InitializeCriticalSection(&gMutexes[i]);
#if defined (U_DEBUG)
            gRecursionCountPool[i] = 0; /* see comments above */
#endif
        }
        gMutexPoolInitialized = TRUE;
    }
#elif defined (POSIX)
    /*  TODO:  experimental code.  Shouldn't need to explicitly init the mutexes. */
    if (gMutexPoolInitialized == FALSE) {
        int i;
        for (i=0; i<MAX_MUTEXES; i++) {
            pthread_mutex_init(&gMutexes[i], NULL);
        }
        gMutexPoolInitialized = TRUE;
    }
#endif 

    /*
     * for both Windows & POSIX, the first mutex in the array is used
     *   for the ICU global mutex.
     */
    gGlobalMutex = &gMutexes[0];
    gMutexesInUse[0] = 1;

#else  /* ICU_USE_THREADS */
    gGlobalMutex = &gGlobalMutex;  /* With no threads, we must still set the mutex to
                                    * some non-null value to make the rest of the
                                    *   (not ifdefed) mutex code think that it is initialized.
                                    */
#endif /* ICU_USE_THREADS */
}





U_CAPI void  U_EXPORT2
umtx_init(UMTX *mutex)
{
    if (mutex == NULL || mutex == &gGlobalMutex) {
        initGlobalMutex();
    } else {
        umtx_lock(NULL);
        if (*mutex != NULL) {
            /* Another thread initialized this mutex first. */
            umtx_unlock(NULL);
            return;
        }

        if (pMutexInitFn != NULL) {
            UErrorCode status = U_ZERO_ERROR;
            (*pMutexInitFn)(gMutexContext, mutex, &status);
            /* TODO:  how to report failure on init? */
            umtx_unlock(NULL);
            return;
        }
        else {
#if (ICU_USE_THREADS == 1)
            /*  Search through our pool of pre-allocated mutexes for one that is not
            *  already in use.    */
            int i;
            for (i=0; i<MAX_MUTEXES; i++) {
                if (gMutexesInUse[i] == 0) {
                    gMutexesInUse[i] = 1;
                    *mutex = &gMutexes[i];
                    break;
                }
            }
#endif
        }
        umtx_unlock(NULL);

#if (ICU_USE_THREADS == 1)
        /* No more mutexes were available from our pre-allocated pool.  */
        /*   TODO:  how best to deal with this?                    */
        U_ASSERT(*mutex != NULL);
#endif
    }
}


/*
 *  umtx_destroy.    Un-initialize a mutex, releasing any underlying resources
 *                   that it may be holding.  Destroying an already destroyed
 *                   mutex has no effect.  Unlike umtx_init(), this function
 *                   is not thread safe;  two threads must not concurrently try to
 *                   destroy the same mutex.
 */                  
U_CAPI void  U_EXPORT2
umtx_destroy(UMTX *mutex) {
    if (mutex == NULL) {  /* destroy the global mutex */
        mutex = &gGlobalMutex;
    }
    
    if (*mutex == NULL) {  /* someone already did it. */
        return;
    }

    /*  The life of the inc/dec mutex is tied to that of the global mutex.  */
    if (mutex == &gGlobalMutex) {
        umtx_destroy(&gIncDecMutex);
    }

    if (pMutexDestroyFn != NULL) {
        /* Mutexes are being managed by the app.  Call back to it for the destroy. */
        (*pMutexDestroyFn)(gMutexContext, mutex);
    }
    else {
#if (ICU_USE_THREADS == 1)
        /* Return this mutex to the pool of available mutexes, if it came from the
         *  pool in the first place.
         */
        /* TODO use pointer math here, instead of iterating! */
        int i;
        for (i=0; i<MAX_MUTEXES; i++)  {
            if (*mutex == &gMutexes[i]) {
                gMutexesInUse[i] = 0;
                break;
            }
        }
#endif
    }

    *mutex = NULL;
}



U_CAPI void U_EXPORT2 
u_setMutexFunctions(const void *context, UMtxInitFn *i, UMtxFn *d, UMtxFn *l, UMtxFn *u,
                    UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }

    /* Can not set a mutex function to a NULL value  */
    if (i==NULL || d==NULL || l==NULL || u==NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    /* If ICU is not in an initial state, disallow this operation. */
    if (cmemory_inUse()) {
        *status = U_INVALID_STATE_ERROR;
        return;
    }

    /* Swap in the mutex function pointers.  */
    pMutexInitFn    = i;
    pMutexDestroyFn = d;
    pMutexLockFn    = l;
    pMutexUnlockFn  = u;
    gMutexContext   = context;
    gGlobalMutex    = NULL;         /* For POSIX, the global mutex will be pre-initialized */
                                    /*   Undo that, force re-initialization when u_init()  */
                                    /*   happens.                                          */
}



/*-----------------------------------------------------------------
 *
 *  Atomic Increment and Decrement
 *     umtx_atomic_inc
 *     umtx_atomic_dec
 *
 *----------------------------------------------------------------*/

/* Pointers to user-supplied inc/dec functions.  Null if no funcs have been set.  */
static UMtxAtomicFn  *pIncFn = NULL;
static UMtxAtomicFn  *pDecFn = NULL;
static const void *gIncDecContext  = NULL;


U_CAPI int32_t U_EXPORT2
umtx_atomic_inc(int32_t *p)  {
    int32_t retVal;
    if (pIncFn) {
        retVal = (*pIncFn)(gIncDecContext, p);
    } else {
        #if defined (U_WINDOWS) && ICU_USE_THREADS == 1
            retVal = InterlockedIncrement((LONG*)p);
        #elif defined(USE_MAC_OS_ATOMIC_INCREMENT)
            retVal = OSAtomicIncrement32Barrier(p);
        #elif defined (POSIX) && ICU_USE_THREADS == 1
            umtx_lock(&gIncDecMutex);
            retVal = ++(*p);
            umtx_unlock(&gIncDecMutex);
        #else
            /* Unknown Platform, or ICU thread support compiled out. */
            retVal = ++(*p);
        #endif
    }
    return retVal;
}

U_CAPI int32_t U_EXPORT2
umtx_atomic_dec(int32_t *p) {
    int32_t retVal;
    if (pDecFn) {
        retVal = (*pDecFn)(gIncDecContext, p);
    } else {
        #if defined (U_WINDOWS) && ICU_USE_THREADS == 1
            retVal = InterlockedDecrement((LONG*)p);
        #elif defined(USE_MAC_OS_ATOMIC_INCREMENT)
            retVal = OSAtomicDecrement32Barrier(p);
        #elif defined (POSIX) && ICU_USE_THREADS == 1
            umtx_lock(&gIncDecMutex);
            retVal = --(*p);
            umtx_unlock(&gIncDecMutex);
        #else
            /* Unknown Platform, or ICU thread support compiled out. */
            retVal = --(*p);
        #endif
    }
    return retVal;
}

/* TODO:  Some POSIXy platforms have atomic inc/dec functions available.  Use them. */





U_CAPI void U_EXPORT2
u_setAtomicIncDecFunctions(const void *context, UMtxAtomicFn *ip, UMtxAtomicFn *dp,
                                UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }
    /* Can not set a mutex function to a NULL value  */
    if (ip==NULL || dp==NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    /* If ICU is not in an initial state, disallow this operation. */
    if (cmemory_inUse()) {
        *status = U_INVALID_STATE_ERROR;
        return;
    }

    pIncFn = ip;
    pDecFn = dp;
    gIncDecContext = context;

#if !U_RELEASE
    {
        int32_t   testInt = 0;
        U_ASSERT(umtx_atomic_inc(&testInt) == 1);     /* Sanity Check.    Do the functions work at all? */
        U_ASSERT(testInt == 1);
        U_ASSERT(umtx_atomic_dec(&testInt) == 0);
        U_ASSERT(testInt == 0);
    }
#endif
}



/*
 *  Mutex Cleanup Function
 *
 *      Destroy the global mutex(es), and reset the mutex function callback pointers.
 */
U_CFUNC UBool umtx_cleanup(void) {
    umtx_destroy(NULL);
    pMutexInitFn    = NULL;
    pMutexDestroyFn = NULL;
    pMutexLockFn    = NULL;
    pMutexUnlockFn  = NULL;
    gMutexContext   = NULL;
    gGlobalMutex    = NULL;
    pIncFn          = NULL;
    pDecFn          = NULL;
    gIncDecContext  = NULL;
    gIncDecMutex    = NULL;

#if (ICU_USE_THREADS == 1)
    if (gMutexPoolInitialized) {
        int i;
        for (i=0; i<MAX_MUTEXES; i++) {
            if (gMutexesInUse[i]) {
#if defined (U_WINDOWS)
                DeleteCriticalSection(&gMutexes[i]);
#elif defined (POSIX)
                pthread_mutex_destroy(&gMutexes[i]);
#endif
                gMutexesInUse[i] = 0;
            }
        }
    }
    gMutexPoolInitialized = FALSE;
#endif

    return TRUE;
}


