// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       HiResTimer.cxx
//------------------------------------------------------------------------------
#include "Pch.h"

MtDefine(HiResTimer, Utilities, "HiResTimer");

class HiResTimerImpl
    : public HiResTimer
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(HiResTimer))
    
    HiResTimerImpl();

    virtual double GetTime();
    virtual double GetFrequency();
    
    void SetInitialTime(LONG hi, DWORD lo);
    void Reset();

private:
    static double UnmungeTime(LARGE_INTEGER newTime,
                       LARGE_INTEGER initialTime,
                       double frequency);

    LARGE_INTEGER _initialTime;
    double        _clockFrequency;      // in ticks per second
};


HiResTimerImpl::HiResTimerImpl()
{
    LARGE_INTEGER tmpTime;
    LARGE_INTEGER zeroTime = { 0, 0 };

#if (DBG == 1) || defined(_PREFIX_)
    BOOL supported =
#endif
        QueryPerformanceFrequency(&tmpTime);

#if (DBG == 1) || defined(_PREFIX_)
    AssertConstMsgA(supported, "Doesn't have hires timer, using GetTickCount");
#endif
    
    _clockFrequency   = UnmungeTime(tmpTime, zeroTime, 1.0);

    QueryPerformanceCounter(&_initialTime);  // to cut largeTime to 32bit!
}


void
HiResTimerImpl::Reset()
{ 
    LARGE_INTEGER tmpTime;

    QueryPerformanceCounter(&tmpTime);
    _initialTime.HighPart = tmpTime.HighPart;
    _initialTime.LowPart  = tmpTime.LowPart;
}


double
HiResTimerImpl::GetTime()
{
    double result;
    LARGE_INTEGER tmpTime;

    QueryPerformanceCounter(&tmpTime);
    result = UnmungeTime(tmpTime, _initialTime, _clockFrequency);

    return(result);
}


double
HiResTimerImpl::GetFrequency()
{
    return(_clockFrequency);
}


double
HiResTimerImpl::UnmungeTime(LARGE_INTEGER newTime,
                            LARGE_INTEGER initialTime,
                            double frequency)
{
    double whole;

    if (newTime.HighPart == initialTime.HighPart) {
        
        Assert(newTime.LowPart >= initialTime.LowPart);

        return (newTime.LowPart - initialTime.LowPart) / frequency;

    } else if (newTime.LowPart < initialTime.LowPart) {
        
        // Borrow one from the High Part.
        // Assuming unsigned long 32 bits.

        Assert(sizeof(unsigned long) == 4);

        // The compiler needs to do this first, otherwise possible
        // overflow. 
        unsigned long forceSubtract =
            0xFFFFFFFF - initialTime.LowPart;
        newTime.LowPart = 
            forceSubtract + newTime.LowPart + 1;
        newTime.HighPart =
            newTime.HighPart - initialTime.HighPart - 1;
    } else {
        newTime.LowPart = newTime.LowPart - initialTime.LowPart;
        newTime.HighPart -= initialTime.HighPart;
    }

    whole = newTime.LowPart + newTime.HighPart * 4294967296.0;

    return whole/frequency;
}


void
HiResTimerImpl::SetInitialTime(LONG hi, DWORD lo)
{
    _initialTime.HighPart = hi;
    _initialTime.LowPart = lo;
}


class LoResTimer :
    public HiResTimer
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(HiResTimer))

    LoResTimer() : _lastTick(0), _curTime(0.0) { GetTime(); }

    void Reset() { _curTime = 0; }

    virtual double GetTime()
    {
        if (_curTime == 0.0 && _lastTick == 0)
        {
            _lastTick = GetTickCount () ;
        }
        else
        {
            DWORD curtick = GetTickCount () ;

            if (curtick >= _lastTick) {
                _curTime += ((double) (curtick - _lastTick)) / 1000 ;
            } else {
                _curTime +=
                    ((double) (curtick + (0xffffffff - _lastTick))) / 1000 ;
            }

            _lastTick = curtick ;
        }

        return _curTime;
    }
    
    // Don't know what to return.
    virtual double GetFrequency() { return 1.0; }

private:
    DWORD _lastTick;
    double _curTime;
};


HiResTimer * CreateHiResTimer()
{
    LARGE_INTEGER iFreq;

    BOOL bSupported = QueryPerformanceFrequency(&iFreq);

    if (bSupported)
    {
        return new HiResTimerImpl();
    }
    else
    {
        return new LoResTimer();
    }
}



