#include "common/fxsystemthread.h"

//
// This is called to tell the thread to exit.
//
// It must be called from thread context such as
// the driver unload routine since it will wait for the
// thread to exit.
//
BOOLEAN
FxSystemThread::ExitThread()
{
    NTSTATUS Status;
    KIRQL irql;

    Lock(&irql);

    if( !m_Initialized )
    {
        Unlock(irql);
        return TRUE;
    }

    if( m_Exit ) {
        ASSERT(FALSE); // This is not race free, so don't allow it
        Unlock(irql);
        return TRUE;
    }

    // Tell the system thread to exit
    m_Exit = TRUE;

    //
    // The thread could still be spinning up, so we must handle this condition.
    //
    if( m_ThreadPtr == NULL )
    {
        Unlock(irql);

        KeEnterCriticalRegion();

        // Wait for thread to start
        Status = m_InitEvent.WaitFor(Executive, KernelMode, FALSE, NULL);

        KeLeaveCriticalRegion();

        UNREFERENCED_PARAMETER(Status);
        ASSERT(NT_SUCCESS(Status));

        //
        // Now we have a thread, wait for it to go away
        //
        ASSERT(m_ThreadPtr != NULL);
    }
    else
    {
        Unlock(irql);
    }

    m_WorkEvent.Set();

    //
    // We can't be waiting in our own thread for the thread to exit.
    //
    ASSERT(IsCurrentThread() == FALSE);

    KeEnterCriticalRegion();

    // Wait for thread to exit
    Status = KeWaitForSingleObject(
                 m_ThreadPtr,
                 Executive,
                 KernelMode,
                 FALSE,
                 NULL
                 );

    KeLeaveCriticalRegion();

    UNREFERENCED_PARAMETER(Status);
    ASSERT(NT_SUCCESS(Status));

    ObDereferenceObject(m_ThreadPtr);

    //
    // Now safe to unload the driver or object
    // the thread worker is pointing to
    //

    return TRUE;
}