#ifndef _MXTIMER_H_
#define _MXTIMER_H_

#include <ntddk.h>
#include "common/mxgeneral.h"

#define TolerableDelayUnlimited ((ULONG)-1)

typedef
BOOLEAN
(*PFN_KE_SET_COALESCABLE_TIMER) (
    __inout PKTIMER Timer,
    __in LARGE_INTEGER DueTime,
    __in ULONG Period,
    __in ULONG TolerableDelay,
    __in_opt PKDPC Dpc
    );

typedef struct _MdTimer {

    //
    // The timer period
    //
    LONG m_Period;
    
    //
    // Tracks whether the ex timer is being used 
    // 
    BOOLEAN m_IsExtTimer;
    
#pragma warning(push)
#pragma warning( disable: 4201 ) // nonstandard extension used : nameless struct/union
    
    union {
        struct {
            //
            // Callback function to be invoked upon timer expiration 
            //
            MdDeferredRoutine m_TimerCallback;
            KTIMER KernelTimer;
            KDPC TimerDpc;
        };
    };

#pragma warning(pop)
    
    //
    // Context to be passed in to the callback function
    //
    PVOID m_TimerContext;

} MdTimer;

class MxTimer
{
private:
    //
    // Handle to the timer object
    //
    MdTimer m_Timer;

public:

    __inline
    MxTimer(
        VOID
        )
    {
        m_Timer.m_TimerContext = NULL;
        m_Timer.m_TimerCallback = NULL;
        m_Timer.m_Period = 0;
    }

    __inline
    ~MxTimer(
        VOID
        )
    {
        BOOLEAN wasCancelled;
    }

    //CHECK_RETURN_IF_USER_MODE
    __inline
    NTSTATUS
    Initialize(
        __in_opt PVOID TimerContext,
        __in MdDeferredRoutine TimerCallback,
        __in LONG Period
        )
    {
        m_Timer.m_TimerContext = TimerContext;
        m_Timer.m_TimerCallback = TimerCallback;
        m_Timer.m_Period = Period;

        KeInitializeDpc(&(m_Timer.TimerDpc), // Timer DPC
                        m_Timer.m_TimerCallback, // DeferredRoutine
                        m_Timer.m_TimerContext); // DeferredContext

        m_Timer.m_IsExtTimer = FALSE;

        return STATUS_SUCCESS;
    }

    __inline
    VOID
    Start(
        __in LARGE_INTEGER DueTime,
        __in ULONG TolerableDelay = 0
        )
    {
        if (m_Timer.m_IsExtTimer)
        {
            StartWithReturn(DueTime,TolerableDelay);
        }
        else
        {
    	    KeSetTimer(&(m_Timer.KernelTimer),
    		   DueTime,
    		   &(m_Timer.TimerDpc));
        }                                              

        return;
    }

    __inline
    BOOLEAN
    StartWithReturn(
        __in LARGE_INTEGER DueTime,
        __in ULONG TolerableDelay = 0
        )
    {        
    	return KeSetTimer(&(m_Timer.KernelTimer),
    			  DueTime,
    			  &(m_Timer.TimerDpc));            
    }

    _Must_inspect_result_
    __inline
    BOOLEAN
    Stop(
        VOID
        )
    {
        BOOLEAN bRetVal;
    
        bRetVal = KeCancelTimer(&(m_Timer.KernelTimer));

        return bRetVal;
    }

    __inline
    VOID
    FlushQueuedDpcs(
        VOID
        )
    {
        Mx::MxFlushQueuedDpcs();
    }

};

#endif //_MXTIMER_H_
