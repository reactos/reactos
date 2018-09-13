/******************************************************************************

   Copyright (C) Microsoft Corporation 1991-1992. All rights reserved.

   Title:   ntaviprt.h - Definitions for the portable win16/32 version of AVI

*****************************************************************************/
#ifndef WIN32
    #define EnterCrit(a)
    #define LeaveCrit(a)
#else

    /*
     * we need to enter critical sections more than once on a thread
     * (eg when handling a message that requires sending another message
     * to the winproc). This is ok - the same thread can get a critical
     * section more than once. BUT - we need to release it the same number
     * of times.
     *
     * Problems occur in mciaviTaskWait when we release the critsec to yield
     * - we don't know how many times to release it and enter it again.
     *
     * Solution: keep a count of how many times we are in the critsec. When
     * entering, if the count is already > 0, increment it once more, and leave
     * the critsec (ensuring that the count is protected, but the critsec is
     * only one level deep). On leaving, only do a leave if the count reaches
     * 0.
     *
     * NB: Critical sections are now defined per device, in the MCIGRAPHIC
     * struct. This is needed to avoid critsec deadlocks when running multiple
     * 16-bit apps (if a WOW thread yields in any way - and there are a lot
     * of ways - while holding the critical section, and another WOW thread
     * tries to get the critical section, WOW will hang, since it won't
     * reschedule).
     */


#define EnterCrit(p)  { EnterCriticalSection(&(p)->CritSec); 	\
			if ((p)->lCritRefCount++ > 0)	\
                        	LeaveCriticalSection(&(p)->CritSec);\
                      }

#define LeaveCrit(p)  { if (--(p)->lCritRefCount <= 0) {	\
				LeaveCriticalSection(&(p)->CritSec);\
                                Sleep(0);               \
                        }				\
                      }

#define IsGDIObject(obj) (GetObjectType((HGDIOBJ)(obj)) != 0)

#endif
