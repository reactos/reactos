/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Mutant/Mutex test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

static
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
(NTAPI
*pKeAreAllApcsDisabled)(VOID);

#define ULONGS_PER_POINTER (sizeof(PVOID) / sizeof(ULONG))
#define MUTANT_SIZE (2 + 6 * ULONGS_PER_POINTER)

C_ASSERT(sizeof(DISPATCHER_HEADER) == 8 + 2 * sizeof(PVOID));
C_ASSERT(sizeof(KMUTANT) == sizeof(DISPATCHER_HEADER) + 3 * sizeof(PVOID) + sizeof(PVOID));
C_ASSERT(sizeof(KMUTANT) == MUTANT_SIZE * sizeof(ULONG));

#define CheckMutex(Mutex, Held, New, ExpectedApcDisable) do {                   \
    PKTHREAD Thread = KeGetCurrentThread();                                     \
    ok_eq_uint((Mutex)->Header.Type, MutantObject);                             \
    ok_eq_uint((Mutex)->Header.Abandoned, 0x55);                                \
    ok_eq_uint((Mutex)->Header.Size, MUTANT_SIZE);                              \
    ok_eq_uint((Mutex)->Header.DpcActive, 0x55);                                \
    ok_eq_pointer((Mutex)->Header.WaitListHead.Flink,                           \
                  &(Mutex)->Header.WaitListHead);                               \
    ok_eq_pointer((Mutex)->Header.WaitListHead.Blink,                           \
                  &(Mutex)->Header.WaitListHead);                               \
    if (Held)                                                                   \
    {                                                                           \
        ok_eq_long((Mutex)->Header.SignalState, 0);                             \
        ok_eq_pointer((Mutex)->MutantListEntry.Flink, &Thread->MutantListHead); \
        ok_eq_pointer((Mutex)->MutantListEntry.Blink, &Thread->MutantListHead); \
        ok_eq_pointer(Thread->MutantListHead.Flink, &(Mutex)->MutantListEntry); \
        ok_eq_pointer(Thread->MutantListHead.Blink, &(Mutex)->MutantListEntry); \
        ok_eq_pointer((Mutex)->OwnerThread, Thread);                            \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        ok_eq_long((Mutex)->Header.SignalState, 1);                             \
        if (New)                                                                \
        {                                                                       \
            ok_eq_pointer((Mutex)->MutantListEntry.Flink,                       \
                          (PVOID)0x5555555555555555ULL);                        \
            ok_eq_pointer((Mutex)->MutantListEntry.Blink,                       \
                          (PVOID)0x5555555555555555ULL);                        \
        }                                                                       \
        ok_eq_pointer(Thread->MutantListHead.Flink, &Thread->MutantListHead);   \
        ok_eq_pointer(Thread->MutantListHead.Blink, &Thread->MutantListHead);   \
        ok_eq_pointer((Mutex)->OwnerThread, NULL);                              \
    }                                                                           \
    ok_eq_uint((Mutex)->Abandoned, 0);                                          \
    ok_eq_uint((Mutex)->ApcDisable, ExpectedApcDisable);                        \
} while (0)

#define CheckApcs(KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, Irql) do    \
{                                                                                       \
    ok_eq_bool(KeAreApcsDisabled(), KernelApcsDisabled || SpecialApcsDisabled);         \
    ok_eq_int(Thread->KernelApcDisable, KernelApcsDisabled);                            \
    if (pKeAreAllApcsDisabled)                                                          \
        ok_eq_bool(pKeAreAllApcsDisabled(), AllApcsDisabled);                           \
    ok_eq_int(Thread->SpecialApcDisable, SpecialApcsDisabled);                          \
    ok_irql(Irql);                                                                      \
} while (0)

static
VOID
TestMutant(VOID)
{
    NTSTATUS Status;
    KMUTANT Mutant;
    LONG State;
    PKTHREAD Thread = KeGetCurrentThread();

    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);
    RtlFillMemory(&Mutant, sizeof(Mutant), 0x55);
    KeInitializeMutant(&Mutant, FALSE);
    CheckMutex(&Mutant, FALSE, TRUE, 0);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    RtlFillMemory(&Mutant, sizeof(Mutant), 0x55);
    KeInitializeMutant(&Mutant, TRUE);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);
    CheckMutex(&Mutant, TRUE, TRUE, 0);
    State = KeReleaseMutant(&Mutant, 1, FALSE, FALSE);
    ok_eq_long(State, 0);
    CheckMutex(&Mutant, FALSE, FALSE, 0);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    /* Acquire and release */
    Status = KeWaitForSingleObject(&Mutant,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(&Mutant, TRUE, TRUE, 0);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    State = KeReleaseMutant(&Mutant, 1, FALSE, FALSE);
    ok_eq_long(State, 0);
    CheckMutex(&Mutant, FALSE, FALSE, 0);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);
}

static
VOID
TestMutex(VOID)
{
    NTSTATUS Status;
    KMUTEX Mutex;
    LONG State;
    PKTHREAD Thread = KeGetCurrentThread();

    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);
    RtlFillMemory(&Mutex, sizeof(Mutex), 0x55);
    KeInitializeMutex(&Mutex, 0);
    CheckMutex(&Mutex, FALSE, TRUE, 1);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    RtlFillMemory(&Mutex, sizeof(Mutex), 0x55);
    KeInitializeMutex(&Mutex, 123);
    CheckMutex(&Mutex, FALSE, TRUE, 1);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    Status = KeWaitForSingleObject(&Mutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(&Mutex, TRUE, FALSE, 1);
    CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);

    State = KeReleaseMutex(&Mutex, FALSE);
    ok_eq_long(State, 0);
    CheckMutex(&Mutex, FALSE, FALSE, 1);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);
}

START_TEST(KeMutex)
{
    pKeAreAllApcsDisabled = KmtGetSystemRoutineAddress(L"KeAreAllApcsDisabled");
    if (skip(pKeAreAllApcsDisabled != NULL, "KeAreAllApcsDisabled unavailable\n"))
    {
        /* We can live without this function here */
    }

    TestMutant();
    TestMutex();
}
