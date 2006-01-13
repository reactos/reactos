#include <precomp.h>

void _endthread(void);

static DWORD WINAPI
_beginthread_start(PVOID lpParameter)
{
    PTHREADDATA ThreadData = (PTHREADDATA)lpParameter;

    if (SetThreadData(ThreadData))
    {
        /* FIXME - wrap start_address in SEH! */
        ThreadData->start_address(ThreadData->arglist);

        _endthread();
    }
    else
    {
        /* couldn't set the thread data, free it before terminating */
        free(ThreadData);
    }

    ExitThread(0);
}


/*
 * @implemented
 *
 * FIXME: the return type should be uintptr_t
 */
unsigned long _beginthread(
    void (__cdecl *start_address)(void*),
    unsigned stack_size,
    void* arglist)
{
    HANDLE hThread;
    PTHREADDATA ThreadData;

    if (start_address == NULL) {
        __set_errno(EINVAL);
        return (unsigned long)-1;
    }

    /* allocate the thread data structure already here instead of allocating the
       thread data structure in the thread itself. this way we can pass an error
       code to the caller in case we don't have sufficient resources */
    ThreadData = malloc(sizeof(THREADDATA));
    if (ThreadData == NULL)
    {
        __set_errno(EAGAIN);
        return (unsigned long)-1;
    }

    ThreadData->start_address = start_address;
    ThreadData->arglist = arglist;

    hThread = CreateThread(NULL,
                           stack_size,
                           _beginthread_start,
                           ThreadData,
                           CREATE_SUSPENDED,
                           NULL);
    if (hThread == NULL)
    {
        free(ThreadData);
        __set_errno(EAGAIN);
        return (unsigned long)-1;
    }

    ThreadData->hThread = hThread;

    if (ResumeThread(hThread) == (DWORD)-1)
    {
        CloseHandle(hThread);

        /* freeing the ThreadData _could_ cause a crash, but only in case someone
           else resumed the thread and it got to free the context before we actually
           get here, but that's _very_ unlikely! */
        free(ThreadData);
        __set_errno(EAGAIN);
        return (unsigned long)-1;
    }

    return (unsigned long)hThread;
}

/*
 * @implemented
 */
void _endthread(void)
{
    PTHREADDATA ThreadData = GetThreadData();

    /* close the thread handle */
    CloseHandle(ThreadData->hThread);

    /* NOTE: the thread data will be freed in the thread detach routine that will
             call FreeThreadData */

    ExitThread(0);
}
