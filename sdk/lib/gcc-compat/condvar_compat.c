/*
 * GCC 15's MinGW libgcc pulls in gthr-win32-cond.o, which imports the
 * Vista-era condition-variable entry points from kernel32. For 0x502-targeted
 * images linked against this runtime, provide local stubs that satisfy the
 * linker without widening the runtime API contract.
 */

#include <windef.h>
#include <intrin.h>
#include <winbase.h>

static VOID WINAPI CondVar_Initialize(PRTL_CONDITION_VARIABLE ConditionVariable)
{
    __debugbreak();
    ConditionVariable->Ptr = NULL;
}

static VOID WINAPI CondVar_Wake(PRTL_CONDITION_VARIABLE ConditionVariable)
{
    __debugbreak();
    (void)ConditionVariable;
}

static VOID WINAPI CondVar_WakeAll(PRTL_CONDITION_VARIABLE ConditionVariable)
{
    __debugbreak();
    (void)ConditionVariable;
}

static BOOL WINAPI CondVar_SleepCS(
    PRTL_CONDITION_VARIABLE ConditionVariable,
    PRTL_CRITICAL_SECTION CriticalSection,
    DWORD Timeout)
{
    __debugbreak();
    (void)ConditionVariable;
    (void)CriticalSection;
    (void)Timeout;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#ifdef _M_IX86
VOID (WINAPI *__imp_InitializeConditionVariable)(PRTL_CONDITION_VARIABLE)
    __asm__("__imp__InitializeConditionVariable@4") = CondVar_Initialize;
VOID (WINAPI *__imp_WakeConditionVariable)(PRTL_CONDITION_VARIABLE)
    __asm__("__imp__WakeConditionVariable@4") = CondVar_Wake;
VOID (WINAPI *__imp_WakeAllConditionVariable)(PRTL_CONDITION_VARIABLE)
    __asm__("__imp__WakeAllConditionVariable@4") = CondVar_WakeAll;
BOOL (WINAPI *__imp_SleepConditionVariableCS)(PRTL_CONDITION_VARIABLE, PRTL_CRITICAL_SECTION, DWORD)
    __asm__("__imp__SleepConditionVariableCS@12") = CondVar_SleepCS;
#else
VOID (WINAPI *__imp_InitializeConditionVariable)(PRTL_CONDITION_VARIABLE) = CondVar_Initialize;
VOID (WINAPI *__imp_WakeConditionVariable)(PRTL_CONDITION_VARIABLE) = CondVar_Wake;
VOID (WINAPI *__imp_WakeAllConditionVariable)(PRTL_CONDITION_VARIABLE) = CondVar_WakeAll;
BOOL (WINAPI *__imp_SleepConditionVariableCS)(PRTL_CONDITION_VARIABLE, PRTL_CRITICAL_SECTION, DWORD) = CondVar_SleepCS;
#endif
