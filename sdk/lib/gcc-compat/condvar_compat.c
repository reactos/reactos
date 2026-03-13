/*
 * GCC 13's MinGW libgcc pulls in gthr-win32-cond.o, which imports the
 * Vista-era condition-variable entry points from kernel32. For 0x502-targeted
 * images linked against this runtime, provide local stubs that satisfy the
 * linker without widening the runtime API contract.
 */

#include <windef.h>
#include <winbase.h>

static VOID WINAPI CondVar_Initialize(PRTL_CONDITION_VARIABLE ConditionVariable)
{
    ConditionVariable->Ptr = NULL;
}

static VOID WINAPI CondVar_Wake(PRTL_CONDITION_VARIABLE ConditionVariable)
{
    (void)ConditionVariable;
}

static VOID WINAPI CondVar_WakeAll(PRTL_CONDITION_VARIABLE ConditionVariable)
{
    (void)ConditionVariable;
}

static BOOL WINAPI CondVar_SleepCS(
    PRTL_CONDITION_VARIABLE ConditionVariable,
    PRTL_CRITICAL_SECTION CriticalSection,
    DWORD Timeout)
{
    (void)ConditionVariable;
    (void)CriticalSection;
    (void)Timeout;
    return FALSE;
}

/* Provide __imp_ symbols that match the dllimport references in libgcc. */
VOID (WINAPI *__imp_InitializeConditionVariable)(PRTL_CONDITION_VARIABLE) = CondVar_Initialize;
VOID (WINAPI *__imp_WakeConditionVariable)(PRTL_CONDITION_VARIABLE) = CondVar_Wake;
VOID (WINAPI *__imp_WakeAllConditionVariable)(PRTL_CONDITION_VARIABLE) = CondVar_WakeAll;
BOOL (WINAPI *__imp_SleepConditionVariableCS)(PRTL_CONDITION_VARIABLE, PRTL_CRITICAL_SECTION, DWORD) = CondVar_SleepCS;
