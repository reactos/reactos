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

/* We need the full structure with all non-volatile */
#define _SEH3$_FRAME_ALL_NONVOLATILES 1

#include <stdarg.h>
#include <windef.h>
#include <winnt.h>

#define ASSERT(exp) if (!(exp)) __int2c();

#include "pseh3.h"
#include "pseh3_asmdef.h"

/* Make sure the asm definitions match the structures */
C_ASSERT(SEH3_REGISTRATION_FRAME_Next == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Next));
C_ASSERT(SEH3_REGISTRATION_FRAME_Handler == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Handler));
C_ASSERT(SEH3_REGISTRATION_FRAME_ScopeTable == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, ScopeTable));
C_ASSERT(SEH3_REGISTRATION_FRAME_TryLevel == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, TryLevel));
C_ASSERT(SEH3_REGISTRATION_FRAME_EndOfChain == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, EndOfChain));
C_ASSERT(SEH3_REGISTRATION_FRAME_ExceptionPointers == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, ExceptionPointers));
C_ASSERT(SEH3_REGISTRATION_FRAME_ExceptionCode == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, ExceptionCode));
C_ASSERT(SEH3_REGISTRATION_FRAME_Esp == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Esp));
C_ASSERT(SEH3_REGISTRATION_FRAME_Ebp == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Ebp));
C_ASSERT(SEH3_REGISTRATION_FRAME_AllocaFrame == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, AllocaFrame));
#ifdef _SEH3$_FRAME_ALL_NONVOLATILES
C_ASSERT(SEH3_REGISTRATION_FRAME_Ebx == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Ebx));
C_ASSERT(SEH3_REGISTRATION_FRAME_Esi == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Esi));
C_ASSERT(SEH3_REGISTRATION_FRAME_Edi == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, Edi));
#endif
#ifdef __clang__
C_ASSERT(SEH3_REGISTRATION_FRAME_ReturnAddress == FIELD_OFFSET(SEH3$_REGISTRATION_FRAME, ReturnAddress));
#endif
C_ASSERT(SEH3_SCOPE_TABLE_Filter == FIELD_OFFSET(SEH3$_SCOPE_TABLE, Filter));
C_ASSERT(SEH3_SCOPE_TABLE_Target == FIELD_OFFSET(SEH3$_SCOPE_TABLE, Target));

enum
{
    _SEH3$_NESTED_HANDLER = 0,
    _SEH3$_CPP_HANDLER = 1,
    _SEH3$_CLANG_HANDLER = 2,
};

void
__attribute__((regparm(1)))
_SEH3$_Unregister(
    volatile SEH3$_REGISTRATION_FRAME *Frame)
{
    volatile SEH3$_REGISTRATION_FRAME *CurrentFrame;

    /* Check if the frame has a handler (then it's the registered frame) */
    if (Frame->Handler != NULL)
    {
        /* There shouldn't be any more nested try-level frames */
        ASSERT(Frame->EndOfChain == Frame);

        /* During unwinding on Windows ExecuteHandler2 installs its own EH frame,
           so there can be one or even multiple frames before our own one and we
           need to search for the link that points to our head frame. */
        CurrentFrame = (SEH3$_REGISTRATION_FRAME*)__readfsdword(0);
        if (Frame == CurrentFrame)
        {
            /* The target frame is the first installed frame, remove it */
            __writefsdword(0, (ULONG)Frame->Next);
            return;
        }

        /* Loop to find the frame that links the target frame */
        while (CurrentFrame->Next != Frame)
        {
            CurrentFrame = CurrentFrame->Next;
        }

        /* Remove the frame from the linked list */
        CurrentFrame->Next = Frame->Next;
    }
    else
    {
        /* Search for the head frame */
        CurrentFrame = Frame->Next;
        while (CurrentFrame->Handler == NULL)
        {
            CurrentFrame = CurrentFrame->Next;
        }

        /* Make sure the head frame points to our frame */
        ASSERT(CurrentFrame->EndOfChain == Frame);

        /* Remove this frame from the internal linked list */
        CurrentFrame->EndOfChain = Frame->Next;
    }
}

static inline
LONG
_SEH3$_InvokeNestedFunctionFilter(
    volatile SEH3$_REGISTRATION_FRAME *RegistrationFrame,
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
        "addl %[RegistrationFrame], %%eax\n\t"

        /* Second call to get the filter result */
        "mov $1, %%ecx\n\t"
        "call *%[Filter]"
        : "=a" (FilterResult)
        : [RegistrationFrame] "m" (RegistrationFrame), [Filter] "m" (Filter)
        : "ecx", "edx");

    return FilterResult;
}

long
__attribute__((regparm(1)))
_SEH3$_InvokeEmbeddedFilter(
    volatile SEH3$_REGISTRATION_FRAME *RegistrationFrame);

long
__attribute__((regparm(1)))
_SEH3$_InvokeEmbeddedFilterFromRegistration(
    volatile SEH3$_REGISTRATION_FRAME *RegistrationFrame);

static inline
LONG
_SEH3$_InvokeFilter(
    volatile SEH3$_REGISTRATION_FRAME *RegistrationFrame,
    PVOID Filter,
    int HandlerType)
{
    LONG FilterResult;

    if (HandlerType == _SEH3$_NESTED_HANDLER)
    {
        return _SEH3$_InvokeNestedFunctionFilter(RegistrationFrame, Filter);
    }
    else if (HandlerType == _SEH3$_CPP_HANDLER)
    {
        /* Call the embedded filter function */
        return _SEH3$_InvokeEmbeddedFilter(RegistrationFrame);
    }
    else if (HandlerType == _SEH3$_CLANG_HANDLER)
    {
        return _SEH3$_InvokeEmbeddedFilterFromRegistration(RegistrationFrame);
    }
    else
    {
        /* Should not happen! Skip this handler */
        FilterResult = EXCEPTION_CONTINUE_SEARCH;
    }

    return FilterResult;
}

void
__attribute__((regparm(1)))
_SEH3$_C_AutoCleanup(
    volatile SEH3$_REGISTRATION_FRAME *Frame)
{
    _SEH3$_Unregister(Frame);

    /* Check for __finally frames */
    if (Frame->ScopeTable->Target == NULL)
    {
#ifdef __clang__
        _SEH3$_InvokeFilter(Frame, Frame->ScopeTable->Filter, _SEH3$_CLANG_HANDLER);
#else
        _SEH3$_InvokeFilter(Frame, Frame->ScopeTable->Filter, _SEH3$_NESTED_HANDLER);
#endif
    }
}

void
__attribute__((regparm(1)))
_SEH3$_CPP_AutoCleanup(
    volatile SEH3$_REGISTRATION_FRAME *Frame)
{
    if (Frame->Handler)
        _SEH3$_UnregisterFrame(Frame);
    else
        _SEH3$_UnregisterTryLevel(Frame);

    /* Check for __finally frames */
    if (Frame->ScopeTable->Target == NULL)
    {
       _SEH3$_InvokeFilter(Frame, Frame->ScopeTable->Filter, _SEH3$_CPP_HANDLER);
    }
}


static inline
LONG
_SEH3$_GetFilterResult(
    PSEH3$_REGISTRATION_FRAME Record,
    int HandlerType)
{
    PVOID Filter = Record->ScopeTable->Filter;
    LONG Result;

    /* Check if we have a constant filter */
    if (((ULONG)Filter & 0xFFFFFF00) == 0)
    {
        /* Lowest 8 bit are sign extended to give the result */
        Result = (LONG)(CHAR)(ULONG)Filter;
    }
    else
    {
        /* Call the filter function */
        Result = _SEH3$_InvokeFilter(Record, Filter, HandlerType);
    }

    /* Normalize the result */
    if (Result < 0) return EXCEPTION_CONTINUE_EXECUTION;
    else if (Result > 0) return EXCEPTION_EXECUTE_HANDLER;
    else return EXCEPTION_CONTINUE_SEARCH;
}

static inline
VOID
_SEH3$_CallFinally(
    PSEH3$_REGISTRATION_FRAME Record,
    int HandlerType)
{
    _SEH3$_InvokeFilter(Record, Record->ScopeTable->Filter, HandlerType);
}

__attribute__((noreturn))
static inline
void
_SEH3$_JumpToTarget(
    PSEH3$_REGISTRATION_FRAME RegistrationFrame)
{
#ifdef __clang__
        asm volatile (
            /* Load the registers */
            "movl 24(%%ecx), %%esp\n\t"
            "movl 28(%%ecx), %%ebp\n\t"

            "movl 36(%%ecx), %%ebx\n\t"
            "movl 40(%%ecx), %%esi\n\t"
            "movl 44(%%ecx), %%edi\n\t"

            /* Stack pointer is 4 off from the call to __SEH3$_RegisterFrame */
            "addl $4, %%esp\n\t"

            /* Jump into the exception handler */
            "jmp *%[Target]"
            : :
            "c" (RegistrationFrame),
            "a" (RegistrationFrame->ScopeTable),
             [Target] "m" (RegistrationFrame->ScopeTable->Target)
        );
#else
        asm volatile (
            /* Load the registers */
            "movl 24(%%ecx), %%esp\n\t"
            "movl 28(%%ecx), %%ebp\n\t"

            /* Stack pointer is 4 off from the call to __SEH3$_RegisterFrame */
            "addl $4, %%esp\n\t"

            /* Jump into the exception handler */
            "jmp *%[Target]"
            : :
            "c" (RegistrationFrame),
            "a" (RegistrationFrame->ScopeTable),
             [Target] "m" (RegistrationFrame->ScopeTable->Target)
        );
#endif

    __builtin_unreachable();
}

void
__fastcall
_SEH3$_CallRtlUnwind(
    PSEH3$_REGISTRATION_FRAME RegistrationFrame);


int // EXCEPTION_DISPOSITION
__cdecl
#ifndef __clang__
__attribute__ ((__target__ ("cld")))
#endif
_SEH3$_common_except_handler(
    struct _EXCEPTION_RECORD * ExceptionRecord,
    PSEH3$_REGISTRATION_FRAME EstablisherFrame,
    struct _CONTEXT * ContextRecord,
    void * DispatcherContext,
    int HandlerType)
{
    PSEH3$_REGISTRATION_FRAME CurrentFrame, TargetFrame;
    SEH3$_EXCEPTION_POINTERS ExceptionPointers;
    LONG FilterResult;

    /* Clear the direction flag. */
    asm volatile ("cld" : : : "memory");

    /* Save the exception pointers on the stack */
    ExceptionPointers.ExceptionRecord = ExceptionRecord;
    ExceptionPointers.ContextRecord = ContextRecord;

    /* Check if this is an unwind */
    if (IS_UNWINDING(ExceptionRecord->ExceptionFlags))
    {
        /* Unwind all local frames */
        TargetFrame = EstablisherFrame->Next;
    }
    else
    {
        /* Loop all frames for this registration */
        CurrentFrame = EstablisherFrame->EndOfChain;
        for (;;)
        {
            /* Check if we have an exception handler */
            if (CurrentFrame->ScopeTable->Target != NULL)
            {
                /* Set exception pointers and code for this frame */
                CurrentFrame->ExceptionPointers = &ExceptionPointers;
                CurrentFrame->ExceptionCode = ExceptionRecord->ExceptionCode;

                /* Get the filter result */
                FilterResult = _SEH3$_GetFilterResult(CurrentFrame, HandlerType);

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
            /* Set exception pointers and code for this frame */
            CurrentFrame->ExceptionPointers = &ExceptionPointers;
            CurrentFrame->ExceptionCode = ExceptionRecord->ExceptionCode;

            /* Call the finally function */
            _SEH3$_CallFinally(CurrentFrame, HandlerType);
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

int // EXCEPTION_DISPOSITION
__cdecl
_SEH3$_C_except_handler(
    struct _EXCEPTION_RECORD* ExceptionRecord,
    PSEH3$_REGISTRATION_FRAME EstablisherFrame,
    struct _CONTEXT* ContextRecord,
    void* DispatcherContext)
{
#ifdef __clang__
    return _SEH3$_common_except_handler(ExceptionRecord, EstablisherFrame, ContextRecord, DispatcherContext, _SEH3$_CLANG_HANDLER);
#else
    return _SEH3$_common_except_handler(ExceptionRecord, EstablisherFrame, ContextRecord, DispatcherContext, _SEH3$_NESTED_HANDLER);
#endif
}

int // EXCEPTION_DISPOSITION
__cdecl
_SEH3$_CPP_except_handler(
    struct _EXCEPTION_RECORD* ExceptionRecord,
    PSEH3$_REGISTRATION_FRAME EstablisherFrame,
    struct _CONTEXT* ContextRecord,
    void* DispatcherContext)
{
    return _SEH3$_common_except_handler(ExceptionRecord, EstablisherFrame, ContextRecord, DispatcherContext, _SEH3$_CPP_HANDLER);
}
