/*****************************************************************************

  Mutex

*****************************************************************************/


#ifndef MUTEX_H
#define MUTEX_H


#include "Unfrag.h"


class Mutex
{
public:
    Mutex ()
    {
    	// NT only code begin ... Win9x disregards this part
    	SECURITY_ATTRIBUTES MutexAttribs;

    	memset (&MutexAttribs, 0, sizeof (SECURITY_ATTRIBUTES));

    	MutexAttribs.bInheritHandle = false;
    	MutexAttribs.nLength = sizeof (SECURITY_ATTRIBUTES);
    	MutexAttribs.lpSecurityDescriptor = NULL;
    	// NT only code end

    	Locked      = false;
    	LockCount   = 0;
    	MutexHandle = CreateMutex (&MutexAttribs, Locked, NULL);

        return;
    }

    ~Mutex ()
    {
    	Lock ();
    	CloseHandle (MutexHandle);
    }

    void Lock (void)
    {
    	Locked = true;
    	WaitForSingleObject (MutexHandle, INFINITE);
    	LockCount += 1;
        return;
    }


    void Unlock   (void)
    {
	    LockCount -= 1;
	    if (LockCount <= 0)
	    {
		    LockCount = 0;
		    Locked = false;
    	}

    	ReleaseMutex (MutexHandle);
        return;
    }


    bool IsLocked (void)
    {
        return (Locked);
    }

protected:
    uint32 LockCount;
    HANDLE MutexHandle;
    bool   Locked;
};


#endif // MUTEX_H
