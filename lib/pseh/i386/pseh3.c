/*
 * PROJECT:         ReactOS system libraries
 * LICENSE:         GNU GPL - See COPYING in the top level directory
 * PURPOSE:         Support library for PSEH3
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/*
 * - Naming: To avoid naming conflicts, all internal identifiers are prefixed
 *   with _SEH3$_.
 * - Frame graph: PSEH3 uses the same registration frame for every trylevel.
 *   Only the top trylevel is registered in FS:0, the inner trylevels are linked
 *   to the first trylevel frame. Only the first trylevel frame has the Handler
 *   member set, it's 0 for all others as an identification. The EndOfChain
 *   member of the FS:0 registered frame points to the last internal frame,
 *   which is the frame itself, when only 1 trylevel is present.
 *
 * The registration graph looks like this:
 *
 *              newer handlers
 *             ---------------->
 *
 *                       fs:0             /----------------\
 * |-----------|<-\  |-----------|<-\    / |----------|<-\ \->|----------|
 * |  <Next>   |   \-|  <Next>   |   \--/--|  <Next>  |   \---|  <Next>  |
 * | <Handler> |     | <Handler> |     /   |  <NULL>  |       |  <NULL>  |
 * |-----------|     |-----------|    /    |----------|       |----------|
 *                   |EndOfChain |---/
 *                   |   ...     |
 *                   |-----------|
 */

#include <stdarg.h>
#include <windef.h>
#include <winnt.h>

#include "pseh3.h"
#include "pseh3_asmdef.h"

/* Make sure the asm definitions match the structures */
C_ASSERT(SEH3_REGISTRATION_FRAME_Next == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Next));
C_ASSERT(SEH3_REGISTRATION_FRAME_Handler == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Handler));
C_ASSERT(SEH3_REGISTRATION_FRAME_EndOfChain == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, EndOfChain));
C_ASSERT(SEH3_REGISTRATION_FRAME_ScopeTable == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, ScopeTable));
C_ASSERT(SEH3_REGISTRATION_FRAME_ExceptionPointers == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, ExceptionPointers));
C_ASSERT(SEH3_REGISTRATION_FRAME_Esp == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Esp));
C_ASSERT(SEH3_REGISTRATION_FRAME_Ebp == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Ebp));
C_ASSERT(SEH3_SCOPE_TABLE_Filter == FIELD_OFFSET(SEH3$_SCOPE_TABLE, Filter));
C_ASSERT(SEH3_SCOPE_TABLE_Target == FIELD_OFFSET(SEH3$_SCOPE_TABLE, Target));

static inline
void _SEH3$_Unregister(
    volatile SEH3$_REGISTRATION_FRAME *Frame)
{
    if (Frame->Handler)
        _SEH3$_UnregisterFrame(Frame);
    else
        _SEH3$_UnregisterTryLevel(Frame);
}

static inline
LONG
_SEH3$_InvokeFilter(
    PVOID Record,
    PVOID Filter)
{
    LONG FilterResult;

    asm volatile (
        /* First call with param = 0 to get the frame layout */
        "xorl %%ecx, %%ecx\n\t"
        "xorl %%eax, %%eax\n\t"
        "call *%[Filter]\n\t"

        /* The result is the frame base address that we passed in (0) plus the
           offset to the registration record. */
        "negl %%eax\n\t"
        "addl %[Record], %%eax\n\t"

        /* Second call to get the filter result */
        "mov $1, %%ecx\n\t"
        "call *%[Filter]\n\t"
        : "=a"(FilterResult)
        : [Record] "m" (Record), [Filter] "m" (Filter)
        : "ecx", "edx");

    return FilterResult;
}

static inline
LONG
_SEH3$_GetFilterResult(
    PSEH3$_REGISTRATION_FRAME Record)
{
    PVOID Filter = Record->ScopeTable->Filter;
    LONG Result;

    /* Check for __finally frames */
    if (Record->ScopeTable->Target == NULL)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    /* Check if we have a constant filter */
    if (((ULONG)Filter & 0xFFFFFF00) == 0)
    {
        /* Lowest 8 bit are sign extended to give the result */
        Result = (LONG)(CHAR)(ULONG)Filter;
    }
    else
    {
        /* Call the filter function */
        Result = _SEH3$_InvokeFilter(Record, Filter);
    }

    /* Normalize the result */
    if (Result < 0) return EXCEPTION_CONTINUE_EXECUTION;
    else if (Result > 0) return EXCEPTION_EXECUTE_HANDLER;
    else return EXCEPTION_CONTINUE_SEARCH;
}

static inline
VOID
_SEH3$_CallFinally(
    PSEH3$_REGISTRATION_FRAME Record)
{
    _SEH3$_InvokeFilter(Record, Record->ScopeTable->Filter);
}

__attribute__((noreturn))
static inline
void
_SEH3$_JumpToTarget(
    PSEH3$_REGISTRATION_FRAME RegistrationFrame)
{
    asm volatile (
        /* Load the registers */
        "movl 20(%%ecx), %%esp\n"
        "movl 24(%%ecx), %%ebp\n"

        /* Stack pointer is 4 off from the call to __SEH3$_RegisterFrame */
        "addl $4, %%esp\n"

        /* Jump into the exception handler */
        "jmp *%[Target]\n"
        : :
        "c" (RegistrationFrame),
        "a" (RegistrationFrame->ScopeTable),
         [Target] "m" (RegistrationFrame->ScopeTable->Target)
    );

    __builtin_unreachable();
}

static inline
void
_SEH3$_CallRtlUnwind(
    PSEH3$_REGISTRATION_FRAME RegistrationFrame)
{
    LONG ClobberedEax;

    asm volatile(
        "push %%ebp\n"
        "push $0\n"
        "push $0\n"
        "push $0\n"
        "push %[TargetFrame]\n"
        "call _RtlUnwind@16\n"
        "pop %%ebp\n"
        : "=a" (ClobberedEax)
        : [TargetFrame] "a" (RegistrationFrame)
        : "ebx", "ecx", "edx", "esi",
          "edi", "flags", "memory");
}

EXCEPTION_DISPOSITION
__cdecl
__attribute__ ((__target__ ("cld")))
_SEH3$_except_handler(
    struct _EXCEPTION_RECORD * ExceptionRecord,
    PSEH3$_REGISTRATION_FRAME EstablisherFrame,
    struct _CONTEXT * ContextRecord,
    void * DispatcherContext)
{
    PSEH3$_REGISTRATION_FRAME CurrentFrame, TargetFrame;
    SEH3$_EXCEPTION_POINTERS ExceptionPointers;
    LONG FilterResult;

    /* Clear the direction flag. */
    asm volatile ("cld\n" : : : "memory");

    /* Check if this is an unwind */
    if (ExceptionRecord->ExceptionFlags & EXCEPTION_UNWINDING)
    {
        /* Unwind all local frames */
        TargetFrame = EstablisherFrame->Next;
    }
    else
    {
        /* Save the exception pointers on the stack */
        ExceptionPointers.ExceptionRecord = ExceptionRecord;
        ExceptionPointers.ContextRecord = ContextRecord;

        /* Loop all frames for this registration */
        CurrentFrame = EstablisherFrame->EndOfChain;
        for (;;)
        {
            /* Check if we have an exception handler */
            if (CurrentFrame->ScopeTable->Target != NULL)
            {
                /* Set exception pointers for this frame */
                CurrentFrame->ExceptionPointers = &ExceptionPointers;

                /* Get the filter result */
                FilterResult = _SEH3$_GetFilterResult(CurrentFrame);

                /* Check, if continuuing is requested */
                if (FilterResult == EXCEPTION_CONTINUE_EXECUTION)
                {
                    return ExceptionContinueExecution;
                }

                /* Check if the except handler shall be executed */
                if (FilterResult == EXCEPTION_EXECUTE_HANDLER) break;
            }

            /* Bail out if this is the last handler */
            if (CurrentFrame == EstablisherFrame)
                return ExceptionContinueSearch;

            /* Go to the next frame */
            CurrentFrame = CurrentFrame->Next;
        }

        /* Call RtlUnwind to unwind the frames below this one */
        _SEH3$_CallRtlUnwind(EstablisherFrame);

        /* Do a local unwind up to this frame */
        TargetFrame = CurrentFrame;
    }

    /* Loop frames up to the target frame */
    for (CurrentFrame = EstablisherFrame->EndOfChain;
         CurrentFrame != TargetFrame;
         CurrentFrame = CurrentFrame->Next)
    {
        /* Manually unregister the frame */
        _SEH3$_Unregister(CurrentFrame);

        /* Check if this is an unwind frame */
        if (CurrentFrame->ScopeTable->Target == NULL)
        {
            /* Set exception pointers for this frame */
            CurrentFrame->ExceptionPointers = &ExceptionPointers;

            /* Call the finally function */
            _SEH3$_CallFinally(CurrentFrame);
        }
    }

    /* Check if this was an unwind */
    if (ExceptionRecord->ExceptionFlags & EXCEPTION_UNWINDING)
    {
        return ExceptionContinueSearch;
    }

    /* Unregister the frame. It will be unregistered again at the end of the
       __except block, due to auto cleanup, but that doesn't hurt.
       All we do is set either fs:[0] or EstablisherFrame->EndOfChain to
       CurrentFrame->Next, which will not change it's value. */
    _SEH3$_Unregister(CurrentFrame);

    /* Jump to the __except block (does not return) */
    _SEH3$_JumpToTarget(CurrentFrame);
}

