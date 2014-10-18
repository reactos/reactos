/*
 * PROJECT:         MSVC runtime check support library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Provides support functions for MSVC runtime checks
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <rtcapi.h>

unsigned long
__cdecl
DbgPrint(
    const char *fmt, ...);

void
_RTC_InitBase(void)
{
    __debugbreak();
}

void
_RTC_Shutdown(void)
{
    __debugbreak();
}

void
__cdecl
_RTC_Failure(
    void* retaddr,
    int errnum)
{
    __debugbreak();
}

void
__cdecl
_RTC_UninitUse(
    const char *_Varname)
{
    __debugbreak();
}

void
__fastcall
_RTC_CheckStackVars(
    void *_Esp,
    _RTC_framedesc *_Fd)
{
    int i, *guard1, *guard2;

    /* Loop all variables in the descriptor */
    for (i = 0; i < _Fd->varCount; i++)
    {
        /* Get the 2 guards below and above the variable */
        guard1 = (int*)((char*)_Esp + _Fd->variables[i].addr - sizeof(*guard1));
        guard2 = (int*)((char*)_Esp + _Fd->variables[i].addr +_Fd->variables[i].size);

        /* Check if they contain the guard bytes */
        if ((*guard1 != 0xCCCCCCCC) || (*guard2 != 0xCCCCCCCC))
        {
            __debugbreak();
        }
    }
}

void
__fastcall
_RTC_CheckStackVars2(
    void *_Esp,
    _RTC_framedesc *_Fd,
    _RTC_ALLOCA_NODE *_AllocaList)
{
    _RTC_ALLOCA_NODE *current;
    int *guard;

    /* Process normal variables */
    _RTC_CheckStackVars(_Esp, _Fd);

    /* Process the alloca list */
    for (current = _AllocaList; current != 0; current = current->next)
    {
        /* Get the upper guard */
        guard = (int*)((char*)current + current->allocaSize - sizeof(*guard));

        /* Check if all guard locations are still ok */
        if ((current->guard1 != 0xCCCCCCCC) ||
            (current->guard2[0] != 0xCCCCCCCC) ||
            (current->guard2[1] != 0xCCCCCCCC) ||
            (current->guard2[2] != 0xCCCCCCCC) ||
            (*guard != 0xCCCCCCCC))
        {
            __debugbreak();
        }
    }
}

void
__fastcall
_RTC_AllocaHelper(
    _RTC_ALLOCA_NODE *_PAllocaBase,
    size_t _CbSize,
    _RTC_ALLOCA_NODE **_PAllocaInfoList)
{
    unsigned long i;

    /* Check if we got any allocation */
    if ((_PAllocaBase != 0) &&
        (_CbSize != 0) &&
        (_PAllocaInfoList != 0))
    {
        /* Mark the whole range */
        char *guard = (char*)_PAllocaBase;
        for (i = 0; i < _CbSize; i++)
        {
            guard[i] = 0xCC;
        }

        /* Initialize the alloca base frame */
        _PAllocaBase->allocaSize = _CbSize;

        /* Insert this frame into the alloca list */
        _PAllocaBase->next = *_PAllocaInfoList;
        *_PAllocaInfoList = _PAllocaBase;
    }
}
