
#ifndef __AUTO_MUTEX_LOCK_H
#define __AUTO_MUTEX_LOCK_H


class AutoLock
{
    private:
        HANDLE m_hMutex;

    public:
        AutoLock(HANDLE hMutex)
            : m_hMutex(hMutex)
            { 
                Assert(NULL != m_hMutex);
                WaitForSingleObject(m_hMutex, INFINITE); 
            }

        ~AutoLock(VOID)
            { ReleaseMutex(m_hMutex); }
};


#endif // __AUTO_MUTEX_LOCK_H

