/*
 * PROJECT:     ReactOS USB 3 stack
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Hierarchical state machine engine shared by the USB 3 drivers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#define SM_QUEUE_DEPTH      32
#define SM_MAX_DEPTH        8
#define SM_TRACE_DEPTH      32
#define SM_TRACE_MASK       (SM_TRACE_DEPTH - 1)
#define SM_NO_EVENT         0xFFFF
#define SM_FRAME_INNERMOST  0xFF

#ifndef SM_BREAK
#define SM_BREAK(Message) NT_ASSERTMSG(Message, FALSE)
#endif

/** How a posted event is queued. */
typedef enum _SM_EVENT_CLASS
{
    SmEventCompletion,  /**< Runs in any state unless it only takes critical events */
    SmEventRequest,     /**< Waits until the current state takes requests */
    SmEventResume,      /**< Only valid while paused, runs immediately */
    SmEventCritical     /**< Runs in any state */
} SM_EVENT_CLASS;

typedef struct _SM_EVENT_INFO
{
    PCSTR Name;
    SM_EVENT_CLASS Class;
    BOOLEAN Coalesce;   /**< Reject the post when the same event is already queued */
    BOOLEAN Replace;    /**< Drop queued copies of the event before queuing it */
} SM_EVENT_INFO, *PSM_EVENT_INFO;

/* State flags, inherited by child states */
#define SM_STATE_TAKES_REQUESTS        0x00000001
#define SM_STATE_NEEDS_PASSIVE         0x00000002
#define SM_STATE_CRITICAL_ONLY         0x00000004
#define SM_STATE_YIELDS_TO_CALLER      0x00000008  /**< Caller may leave it; caller's flags pick events */
#define SM_STATE_STOP_TIMER_ON_EXIT    0x00000010

typedef enum _SM_RESULT_KIND
{
    SmResultUnhandled,
    SmResultHandled,
    SmResultTransition,
    SmResultPaused,
    SmResultCall,
    SmResultReturn
} SM_RESULT_KIND;

typedef struct _SM_RESULT
{
    UCHAR Kind;
    UCHAR Frame;
    USHORT Event;
    const VOID* Target;
} SM_RESULT;

typedef enum _SM_TRACE_KIND
{
    SmTracePosted,
    SmTraceRejected,
    SmTraceDispatched,
    SmTraceEntered,
    SmTraceUnhandled,
    SmTracePaused,
    SmTraceResumed,
    SmTraceWaitPassive,
    SmTraceCalled,
    SmTraceReturned,
    SmTraceDiscarded,
    SmTraceWaitTimer
} SM_TRACE_KIND;

typedef struct _SM_TRACE
{
    PCSTR State;
    USHORT Event;
    UCHAR Kind;
    UCHAR Depth;
} SM_TRACE, *PSM_TRACE;

/**
 * @brief
 * One state. Handler gets every event the state sees.
 */
template <typename Machine, typename Event>
struct SmState
{
    const SmState* Parent;
    PCSTR Name;
    ULONG Flags;
    SM_RESULT (Machine::*Handler)(_In_ Event Ev);
    SM_RESULT (Machine::*Entry)();
    const Event* Discards;
};

/**
 * @brief
 * Run to completion engine. The first thread to post into an idle machine
 * drains the queue; everyone else just enqueues. Handlers, entries and hooks
 * run without the lock held.
 *
 * Machine must provide a static EventInfo[] table indexed by Event. It may
 * hide SmAccepts, SmReference, SmDereference, SmQueuePassive, SmOnDequeue,
 * SmStateChanged, SmCancelTimer, SmCompletionsFirst and SmTimerEventId.
 */
template <typename Machine, typename Event>
class SmMachine
{
public:
    typedef SmState<Machine, Event> State;

    VOID
    SmInitialize(
        _In_ const State* Initial);

    BOOLEAN
    SmPost(
        _In_ Event Ev);

    VOID
    SmContinueOnPassive();

    const State*
    SmCurrent() const
    {
        return m_Frames[m_Depth];
    }

    ULONG
    SmDepth() const
    {
        return m_Depth;
    }

    const State*
    SmFrame(
        _In_ ULONG Index) const
    {
        return m_Frames[Index];
    }

    BOOLEAN
    SmIsIn(
        _In_ const State* Ancestor) const;

    BOOLEAN
    SmIsPaused() const
    {
        return m_Paused;
    }

    BOOLEAN
    SmIsWaitingForTimer() const
    {
        return m_WaitingForTimer;
    }

    ULONG
    SmUnexpectedCount() const
    {
        return m_UnexpectedCount;
    }

protected:
    static SM_RESULT
    SmMakeResult(
        _In_ SM_RESULT_KIND Kind,
        _In_ USHORT Ev,
        _In_opt_ const VOID* Target)
    {
        SM_RESULT result = { static_cast<UCHAR>(Kind), SM_FRAME_INNERMOST, Ev, Target };
        return result;
    }

    static SM_RESULT
    SmHandled()
    {
        return SmMakeResult(SmResultHandled, SM_NO_EVENT, NULL);
    }

    static SM_RESULT
    SmUnhandled()
    {
        return SmMakeResult(SmResultUnhandled, SM_NO_EVENT, NULL);
    }

    static SM_RESULT
    SmTransition(
        _In_ const State* Target)
    {
        return SmMakeResult(SmResultTransition, SM_NO_EVENT, Target);
    }

    /* Return this right after the call that lets the resume event be posted */
    static SM_RESULT
    SmPaused()
    {
        return SmMakeResult(SmResultPaused, SM_NO_EVENT, NULL);
    }

    /* Enter Start in a new frame on top of the current state */
    static SM_RESULT
    SmCall(
        _In_ const State* Start)
    {
        return SmMakeResult(SmResultCall, SM_NO_EVENT, Start);
    }

    /* Leave the innermost frame and hand Ev to the state that called it */
    static SM_RESULT
    SmReturn(
        _In_ Event Ev)
    {
        return SmMakeResult(SmResultReturn, static_cast<USHORT>(Ev), NULL);
    }

    VOID
    SmBeginPause();

    /* For transient code that stands in for a state with a discard list */
    VOID
    SmDiscardQueued(
        _In_ Event Ev);

    /* Default hooks, hidden by the machine when it needs them */
    static const BOOLEAN SmCompletionsFirst = TRUE;
    static const USHORT SmTimerEventId = SM_NO_EVENT;

    BOOLEAN
    SmAccepts(
        _In_ Event Ev)
    {
        UNREFERENCED_PARAMETER(Ev);
        return TRUE;
    }

    VOID
    SmReference()
    {
    }

    VOID
    SmDereference()
    {
    }

    VOID
    SmQueuePassive()
    {
        SM_BREAK("State needs passive level but the machine has no work item");
    }

    VOID
    SmOnDequeue(
        _In_ Event Ev)
    {
        UNREFERENCED_PARAMETER(Ev);
    }

    VOID
    SmStateChanged()
    {
    }

    /* FALSE when the timer already fired or is about to */
    BOOLEAN
    SmCancelTimer()
    {
        return TRUE;
    }

private:
    Machine*
    Self()
    {
        return static_cast<Machine*>(this);
    }

    static BOOLEAN
    HasFlag(
        _In_ const State* Target,
        _In_ ULONG Flag);

    static BOOLEAN
    Discards(
        _In_ const State* Target,
        _In_ Event Ev);

    const State*
    EffectiveState() const;

    BOOLEAN
    IsQueued(
        _In_ Event Ev) const;

    BOOLEAN
    RemoveQueued(
        _In_ Event Ev);

    BOOLEAN
    Dequeue(
        _Out_ Event* Ev);

    VOID
    Purge(
        _In_ const State* Entered,
        _Inout_ KIRQL* Irql);

    VOID
    Trace(
        _In_ SM_TRACE_KIND Kind,
        _In_ USHORT Ev);

    SM_RESULT
    Dispatch(
        _In_ Event Ev);

    BOOLEAN
    Run(
        _In_ KIRQL CallerIrql,
        _In_ USHORT LastEvent,
        _In_ SM_RESULT Result,
        _In_opt_ const State* Entered);

    KSPIN_LOCK m_Lock;
    const State* m_Frames[SM_MAX_DEPTH];
    Event m_Queue[SM_QUEUE_DEPTH];
    UCHAR m_Depth;  /* Index of the innermost frame in m_Frames */
    UCHAR m_Count;  /* Number of events in m_Queue */
    BOOLEAN m_Running;
    BOOLEAN m_Paused;
    BOOLEAN m_WaitingForPassive;
    BOOLEAN m_WaitingForTimer;
    UCHAR m_TraceIndex;
    ULONG m_UnexpectedCount;
    SM_TRACE m_Trace[SM_TRACE_DEPTH];
};

/* IMPLEMENTATION *************************************************************/

template <typename Machine, typename Event>
VOID
SmMachine<Machine, Event>::SmInitialize(
    _In_ const State* Initial)
{
    NT_ASSERT(Initial->Entry == NULL);

    KeInitializeSpinLock(&m_Lock);
    m_Frames[0] = Initial;
    m_Depth = 0;
    m_Count = 0;
    m_Running = FALSE;
    m_Paused = FALSE;
    m_WaitingForPassive = FALSE;
    m_WaitingForTimer = FALSE;
    m_TraceIndex = 0;
    m_UnexpectedCount = 0;
    RtlZeroMemory(m_Trace, sizeof(m_Trace));
    Trace(SmTraceEntered, SM_NO_EVENT);
}

template <typename Machine, typename Event>
BOOLEAN
SmMachine<Machine, Event>::SmPost(
    _In_ Event Ev)
{
    const SM_EVENT_INFO* info = &Machine::EventInfo[static_cast<ULONG>(Ev)];
    const State* current;
    SM_RESULT result;
    KIRQL irql;

    KeAcquireSpinLock(&m_Lock, &irql);

    if (!Self()->SmAccepts(Ev) || (info->Coalesce && IsQueued(Ev)))
    {
        Trace(SmTraceRejected, static_cast<USHORT>(Ev));
        KeReleaseSpinLock(&m_Lock, irql);
        return FALSE;
    }

    Trace(SmTracePosted, static_cast<USHORT>(Ev));

    if (info->Replace && RemoveQueued(Ev))
    {
        KeReleaseSpinLock(&m_Lock, irql);
        Self()->SmOnDequeue(Ev);
        KeAcquireSpinLock(&m_Lock, &irql);
    }

    if (info->Class == SmEventResume)
    {
        if (!m_Paused || !m_Running)
        {
            m_UnexpectedCount++;
            Trace(SmTraceUnhandled, static_cast<USHORT>(Ev));
            KeReleaseSpinLock(&m_Lock, irql);
            SM_BREAK("Resume event posted to a machine that is not paused");
            return TRUE;
        }

        /* The paused drainer already left, this thread takes over */
        m_Paused = FALSE;
        Trace(SmTraceResumed, static_cast<USHORT>(Ev));
        KeReleaseSpinLock(&m_Lock, irql);

        Self()->SmReference();
        result = Dispatch(Ev);
        if (!Run(irql, static_cast<USHORT>(Ev), result, NULL))
            Self()->SmDereference();
        return TRUE;
    }

    if (m_WaitingForTimer)
    {
        /* The drainer stopped after entering a state, the entry runs once the timer is done */
        if (static_cast<USHORT>(Ev) != Machine::SmTimerEventId)
        {
            NT_ASSERTMSG("State machine queue overflow", m_Count < SM_QUEUE_DEPTH);
            m_Queue[m_Count] = Ev;
            m_Count++;
            KeReleaseSpinLock(&m_Lock, irql);
            return TRUE;
        }

        m_WaitingForTimer = FALSE;
        current = SmCurrent();
        KeReleaseSpinLock(&m_Lock, irql);

        result = current->Entry ? (Self()->*current->Entry)() : SmHandled();
        if (!Run(irql, static_cast<USHORT>(Ev), result, current))
            Self()->SmDereference();
        return TRUE;
    }

    NT_ASSERTMSG("State machine queue overflow", m_Count < SM_QUEUE_DEPTH);
    m_Queue[m_Count] = Ev;
    m_Count++;

    if (m_Running)
    {
        KeReleaseSpinLock(&m_Lock, irql);
        return TRUE;
    }

    m_Running = TRUE;
    KeReleaseSpinLock(&m_Lock, irql);

    Self()->SmReference();
    if (!Run(irql, static_cast<USHORT>(Ev), SmHandled(), NULL))
        Self()->SmDereference();
    return TRUE;
}

template <typename Machine, typename Event>
VOID
SmMachine<Machine, Event>::SmContinueOnPassive()
{
    const State* current;
    SM_RESULT result;
    KIRQL irql;

    KeAcquireSpinLock(&m_Lock, &irql);

    if (!m_WaitingForPassive || !m_Running)
    {
        m_UnexpectedCount++;
        KeReleaseSpinLock(&m_Lock, irql);
        SM_BREAK("Passive continuation without a state waiting for it");
        return;
    }

    m_WaitingForPassive = FALSE;
    current = SmCurrent();
    KeReleaseSpinLock(&m_Lock, irql);

    /* The reference taken by the drainer that queued the work item carries over */
    result = current->Entry ? (Self()->*current->Entry)() : SmHandled();
    if (!Run(PASSIVE_LEVEL, SM_NO_EVENT, result, current))
        Self()->SmDereference();
}

template <typename Machine, typename Event>
BOOLEAN
SmMachine<Machine, Event>::SmIsIn(
    _In_ const State* Ancestor) const
{
    const State* walk;
    ULONG frame;

    for (frame = 0; frame <= m_Depth; frame++)
    {
        for (walk = m_Frames[frame]; walk != NULL; walk = walk->Parent)
        {
            if (walk == Ancestor)
                return TRUE;
        }
    }

    return FALSE;
}

template <typename Machine, typename Event>
VOID
SmMachine<Machine, Event>::SmBeginPause()
{
    KIRQL irql;

    KeAcquireSpinLock(&m_Lock, &irql);
    m_Paused = TRUE;
    Trace(SmTracePaused, SM_NO_EVENT);
    KeReleaseSpinLock(&m_Lock, irql);
}

template <typename Machine, typename Event>
VOID
SmMachine<Machine, Event>::SmDiscardQueued(
    _In_ Event Ev)
{
    BOOLEAN removed;
    KIRQL irql;

    KeAcquireSpinLock(&m_Lock, &irql);
    removed = RemoveQueued(Ev);
    if (removed)
        Trace(SmTraceDiscarded, static_cast<USHORT>(Ev));
    KeReleaseSpinLock(&m_Lock, irql);

    if (removed)
        Self()->SmOnDequeue(Ev);
}

template <typename Machine, typename Event>
BOOLEAN
SmMachine<Machine, Event>::HasFlag(
    _In_ const State* Target,
    _In_ ULONG Flag)
{
    const State* walk;

    for (walk = Target; walk != NULL; walk = walk->Parent)
    {
        if (walk->Flags & Flag)
            return TRUE;
    }

    return FALSE;
}

template <typename Machine, typename Event>
BOOLEAN
SmMachine<Machine, Event>::Discards(
    _In_ const State* Target,
    _In_ Event Ev)
{
    const Event* list;

    if (Target->Discards == NULL)
        return FALSE;

    for (list = Target->Discards; *list != Event::Count; list++)
    {
        if (*list == Ev)
            return TRUE;
    }

    return FALSE;
}

/* The state whose flags decide which queued events may run */
template <typename Machine, typename Event>
const typename SmMachine<Machine, Event>::State*
SmMachine<Machine, Event>::EffectiveState() const
{
    ULONG frame = m_Depth;

    while ((frame > 0) && HasFlag(m_Frames[frame], SM_STATE_YIELDS_TO_CALLER))
        frame--;

    return m_Frames[frame];
}

template <typename Machine, typename Event>
BOOLEAN
SmMachine<Machine, Event>::IsQueued(
    _In_ Event Ev) const
{
    ULONG index;

    for (index = 0; index < m_Count; index++)
    {
        if (m_Queue[index] == Ev)
            return TRUE;
    }

    return FALSE;
}

template <typename Machine, typename Event>
BOOLEAN
SmMachine<Machine, Event>::RemoveQueued(
    _In_ Event Ev)
{
    ULONG read;
    ULONG write = 0;

    for (read = 0; read < m_Count; read++)
    {
        if (m_Queue[read] != Ev)
        {
            m_Queue[write] = m_Queue[read];
            write++;
        }
    }

    if (write == m_Count)
        return FALSE;

    m_Count = static_cast<UCHAR>(write);
    return TRUE;
}

template <typename Machine, typename Event>
BOOLEAN
SmMachine<Machine, Event>::Dequeue(
    _Out_ Event* Ev)
{
    const State* effective = EffectiveState();
    BOOLEAN takesRequests = HasFlag(effective, SM_STATE_TAKES_REQUESTS);
    BOOLEAN criticalOnly = HasFlag(effective, SM_STATE_CRITICAL_ONLY);
    ULONG pass;
    ULONG index;
    ULONG found = SM_QUEUE_DEPTH;

    for (pass = 0; (pass < 2) && (found == SM_QUEUE_DEPTH); pass++)
    {
        for (index = 0; index < m_Count; index++)
        {
            SM_EVENT_CLASS eventClass =
                Machine::EventInfo[static_cast<ULONG>(m_Queue[index])].Class;
            BOOLEAN eligible;

            if (eventClass == SmEventCritical)
                eligible = TRUE;
            else if (eventClass == SmEventRequest)
                eligible = takesRequests && (!Machine::SmCompletionsFirst || (pass == 1));
            else
                eligible = !criticalOnly;

            if (eligible)
            {
                found = index;
                break;
            }
        }

        if (!Machine::SmCompletionsFirst)
            break;
    }

    if (found == SM_QUEUE_DEPTH)
        return FALSE;

    *Ev = m_Queue[found];
    for (index = found + 1; index < m_Count; index++)
        m_Queue[index - 1] = m_Queue[index];
    m_Count--;
    return TRUE;
}

/* Drop queued events the entered state discards, called with the lock held */
template <typename Machine, typename Event>
VOID
SmMachine<Machine, Event>::Purge(
    _In_ const State* Entered,
    _Inout_ KIRQL* Irql)
{
    const State* walk;
    const Event* list;

    for (walk = Entered; walk != NULL; walk = walk->Parent)
    {
        if (walk->Discards == NULL)
            continue;

        for (list = walk->Discards; *list != Event::Count; list++)
        {
            if (!RemoveQueued(*list))
                continue;

            Trace(SmTraceDiscarded, static_cast<USHORT>(*list));
            KeReleaseSpinLock(&m_Lock, *Irql);
            Self()->SmOnDequeue(*list);
            KeAcquireSpinLock(&m_Lock, Irql);
        }
    }
}

template <typename Machine, typename Event>
VOID
SmMachine<Machine, Event>::Trace(
    _In_ SM_TRACE_KIND Kind,
    _In_ USHORT Ev)
{
    PSM_TRACE entry = &m_Trace[m_TraceIndex];

    entry->State = SmCurrent()->Name;
    entry->Event = Ev;
    entry->Kind = static_cast<UCHAR>(Kind);
    entry->Depth = m_Depth;
    m_TraceIndex = (m_TraceIndex + 1) & SM_TRACE_MASK;
}

template <typename Machine, typename Event>
SM_RESULT
SmMachine<Machine, Event>::Dispatch(
    _In_ Event Ev)
{
    const State* walk;
    SM_RESULT result;
    ULONG frame = m_Depth;

    /* Only the draining thread changes the frames, so no lock is needed here */
    for (;;)
    {
        for (walk = m_Frames[frame]; walk != NULL; walk = walk->Parent)
        {
            if (Discards(walk, Ev))
                return SmHandled();

            if (walk->Handler == NULL)
                continue;

            result = (Self()->*walk->Handler)(Ev);
            if (result.Kind != SmResultUnhandled)
            {
                result.Frame = static_cast<UCHAR>(frame);
                return result;
            }
        }

        if (frame == 0)
            break;

        frame--;
    }

    return SmUnhandled();
}

/**
 * @brief
 * Applies Result and keeps draining. Called without the lock. Entered is the
 * state whose entry produced Result, if any. Returns TRUE when the machine
 * stopped to wait for passive level or a timer and still owns the reference.
 */
template <typename Machine, typename Event>
BOOLEAN
SmMachine<Machine, Event>::Run(
    _In_ KIRQL CallerIrql,
    _In_ USHORT LastEvent,
    _In_ SM_RESULT Result,
    _In_opt_ const State* Entered)
{
    const State* target;
    const State* previous;
    USHORT traced = LastEvent;
    ULONG frame;
    BOOLEAN canceled;
    Event ev;
    KIRQL irql;

    for (;;)
    {
        /* Whoever posts the resume event owns the machine now, touch nothing */
        if (Result.Kind == SmResultPaused)
            return FALSE;

        KeAcquireSpinLock(&m_Lock, &irql);

        if ((Entered != NULL) && (Result.Kind != SmResultCall))
            Purge(Entered, &irql);
        Entered = NULL;

        if (Result.Kind == SmResultReturn)
        {
            NT_ASSERTMSG("Return from the outermost frame", m_Depth > 0);
            m_Depth--;
            Trace(SmTraceReturned, Result.Event);
            Purge(SmCurrent(), &irql);
            traced = Result.Event;
            KeReleaseSpinLock(&m_Lock, irql);
            Result = Dispatch(static_cast<Event>(Result.Event));
            continue;
        }

        if ((Result.Kind == SmResultTransition) || (Result.Kind == SmResultCall))
        {
            target = static_cast<const State*>(Result.Target);

            if (Result.Kind == SmResultCall)
            {
                NT_ASSERTMSG("State machine frames exhausted", m_Depth + 1 < SM_MAX_DEPTH);
                m_Depth++;
                m_Frames[m_Depth] = target;
                Trace(SmTraceCalled, traced);
            }
            else
            {
                frame = (Result.Frame == SM_FRAME_INNERMOST) ? m_Depth : Result.Frame;
                while (m_Depth > frame)
                {
                    if (!HasFlag(m_Frames[m_Depth], SM_STATE_YIELDS_TO_CALLER))
                        SM_BREAK("Caller left a frame that does not yield to it");
                    m_Depth--;
                }

                previous = m_Frames[frame];
                m_Frames[frame] = target;
                Trace(SmTraceEntered, traced);

                KeReleaseSpinLock(&m_Lock, irql);
                Self()->SmStateChanged();

                if (HasFlag(previous, SM_STATE_STOP_TIMER_ON_EXIT) &&
                    (traced != Machine::SmTimerEventId))
                {
                    canceled = Self()->SmCancelTimer();
                    KeAcquireSpinLock(&m_Lock, &irql);

                    if (!canceled && !RemoveQueued(static_cast<Event>(Machine::SmTimerEventId)))
                    {
                        m_WaitingForTimer = TRUE;
                        Trace(SmTraceWaitTimer, traced);
                        KeReleaseSpinLock(&m_Lock, irql);
                        return TRUE;
                    }
                }
                else
                {
                    KeAcquireSpinLock(&m_Lock, &irql);
                }
            }

            if (HasFlag(target, SM_STATE_NEEDS_PASSIVE) && (CallerIrql > PASSIVE_LEVEL))
            {
                m_WaitingForPassive = TRUE;
                Trace(SmTraceWaitPassive, traced);
                KeReleaseSpinLock(&m_Lock, irql);
                Self()->SmQueuePassive();
                return TRUE;
            }

            KeReleaseSpinLock(&m_Lock, irql);
            Entered = target;
            Result = target->Entry ? (Self()->*target->Entry)() : SmHandled();
            continue;
        }

        if (Result.Kind == SmResultUnhandled)
        {
            m_UnexpectedCount++;
            Trace(SmTraceUnhandled, traced);
            SM_BREAK("State machine got an event no state handles");
        }

        if (!Dequeue(&ev))
        {
            m_Running = FALSE;
            KeReleaseSpinLock(&m_Lock, irql);
            return FALSE;
        }

        traced = static_cast<USHORT>(ev);
        Trace(SmTraceDispatched, traced);
        KeReleaseSpinLock(&m_Lock, irql);

        Self()->SmOnDequeue(ev);
        Result = Dispatch(ev);
    }
}
