/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Mutant/Mutex test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

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

static
VOID
TestMutant(VOID)
{
    KMUTANT Mutant;
    LONG State;

    RtlFillMemory(&Mutant, sizeof(Mutant), 0x55);
    KeInitializeMutant(&Mutant, FALSE);
    ok_irql(PASSIVE_LEVEL);
    CheckMutex(&Mutant, FALSE, TRUE, 0);

    RtlFillMemory(&Mutant, sizeof(Mutant), 0x55);
    KeInitializeMutant(&Mutant, TRUE);
    ok_irql(PASSIVE_LEVEL);
    CheckMutex(&Mutant, TRUE, TRUE, 0);
    State = KeReleaseMutant(&Mutant, 1, FALSE, FALSE);
    ok_eq_long(State, 0);
    ok_irql(PASSIVE_LEVEL);
    CheckMutex(&Mutant, FALSE, FALSE, 0);
}

static
VOID
TestMutex(VOID)
{
    KMUTEX Mutex;

    RtlFillMemory(&Mutex, sizeof(Mutex), 0x55);
    KeInitializeMutex(&Mutex, 0);
    ok_irql(PASSIVE_LEVEL);
    CheckMutex(&Mutex, FALSE, TRUE, 1);

    RtlFillMemory(&Mutex, sizeof(Mutex), 0x55);
    KeInitializeMutex(&Mutex, 123);
    ok_irql(PASSIVE_LEVEL);
    CheckMutex(&Mutex, FALSE, TRUE, 1);
}

START_TEST(KeMutex)
{
    TestMutant();
    TestMutex();
}
