/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Interrupt Request Level test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#ifndef _M_AMD64
__declspec(dllimport) void __stdcall KeRaiseIrql(unsigned char, unsigned char *);
__declspec(dllimport) void __stdcall KeLowerIrql(unsigned char);
#else
#define CLOCK1_LEVEL CLOCK_LEVEL
#define CLOCK2_LEVEL CLOCK_LEVEL
#endif

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

START_TEST(KeIrql)
{
    KIRQL Irql, Irql2, PrevIrql, SynchIrql;

    /* we should be called at PASSIVE_LEVEL */
    ok_irql(PASSIVE_LEVEL);

    PrevIrql = KeGetCurrentIrql();

    /* SYNCH_LEVEL is different for UP/MP */
    if (KmtIsMultiProcessorBuild)
        SynchIrql = IPI_LEVEL - 2;
    else
        SynchIrql = DISPATCH_LEVEL;

    /* some Irqls MUST work */
    {
    const KIRQL Irqls[] = { LOW_LEVEL, PASSIVE_LEVEL, APC_LEVEL, DISPATCH_LEVEL,
                            CMCI_LEVEL, CLOCK1_LEVEL, CLOCK2_LEVEL, CLOCK_LEVEL,
                            PROFILE_LEVEL, IPI_LEVEL, /*POWER_LEVEL,*/ SynchIrql, HIGH_LEVEL };
    int i;
    for (i = 0; i < sizeof Irqls / sizeof Irqls[0]; ++i)
    {
        KeRaiseIrql(Irqls[i], &Irql2);
        ok_eq_uint(Irql2, PrevIrql);
        ok_irql(Irqls[i]);
        KeLowerIrql(Irql2);
        ok_irql(PrevIrql);
    }
    }

    /* raising/lowering to the current level should have no effect */
    ok_irql(PASSIVE_LEVEL);
    KeRaiseIrql(PASSIVE_LEVEL, &Irql);
    ok_eq_uint(Irql, PASSIVE_LEVEL);
    KeLowerIrql(PASSIVE_LEVEL);
    ok_irql(PASSIVE_LEVEL);

    /* try to raise to each Irql and back */
    for (Irql = PASSIVE_LEVEL; Irql <= HIGH_LEVEL; ++Irql)
    {
        DPRINT("Raising to %u\n", Irql);
        KeRaiseIrql(Irql, &Irql2);
        ok_eq_uint(Irql2, PrevIrql);
        KeLowerIrql(Irql2);
        ok_irql(PrevIrql);
    }

    /* go through all Irqls in order, skip the ones that the system doesn't accept */
    for (Irql = PASSIVE_LEVEL; Irql <= HIGH_LEVEL; ++Irql)
    {
        DPRINT("Raising to %u\n", Irql);
        KeRaiseIrql(Irql, &Irql2);
        ok_eq_uint(Irql2, PrevIrql);
        Irql2 = KeGetCurrentIrql();
        ok(Irql2 <= Irql, "New Irql is %u, expected <= requested value of %u\n", Irql2, Irql);
        PrevIrql = Irql2;
    }

    ok_irql(HIGH_LEVEL);

    /* now go back again, skipping the ones that don't work */
    for (Irql = HIGH_LEVEL; Irql > PASSIVE_LEVEL;)
    {
        DPRINT("Lowering to %u\n", Irql - 1);
        KeLowerIrql(Irql - 1);
        Irql2 = KeGetCurrentIrql();
        ok(Irql2 < Irql, "New Irql is %u, expected <= requested value of %u\n", Irql2, Irql - 1);
        if (Irql2 < Irql)
            Irql = Irql2;
        else
            --Irql;
    }

    /* test KeRaiseIrqlToDpcLevel */
    ok_irql(PASSIVE_LEVEL);
    Irql = KeRaiseIrqlToDpcLevel();
    ok_irql(DISPATCH_LEVEL);
    ok_eq_uint(Irql, PASSIVE_LEVEL);
    Irql = KeRaiseIrqlToDpcLevel();
    ok_irql(DISPATCH_LEVEL);
    ok_eq_uint(Irql, DISPATCH_LEVEL);
    KeLowerIrql(PASSIVE_LEVEL);

    /* test KeRaiseIrqlToSynchLevel */
    ok_irql(PASSIVE_LEVEL);
    Irql = KeRaiseIrqlToSynchLevel();
    ok_irql(SynchIrql);
    ok_eq_uint(Irql, PASSIVE_LEVEL);
    Irql = KeRaiseIrqlToSynchLevel();
    ok_irql(SynchIrql);
    ok_eq_uint(Irql, SynchIrql);
    KeLowerIrql(PASSIVE_LEVEL);

    /* these bugcheck on a checked build but run fine on free! */
    if (!KmtIsCheckedBuild)
    {
        KeRaiseIrql(HIGH_LEVEL, &Irql);
        KeRaiseIrql(APC_LEVEL, &Irql);
        ok_irql(APC_LEVEL);
        KeLowerIrql(HIGH_LEVEL);
        ok_irql(HIGH_LEVEL);
        KeLowerIrql(PASSIVE_LEVEL);
    }

#ifndef _M_AMD64
    /* try the actual exports, not only the fastcall versions */
    ok_irql(PASSIVE_LEVEL);
    (KeRaiseIrql)(HIGH_LEVEL, &Irql);
    ok_irql(HIGH_LEVEL);
    ok_eq_uint(Irql, PASSIVE_LEVEL);
    (KeLowerIrql)(Irql);
    ok_irql(PASSIVE_LEVEL);
#endif

    /* make sure we exit gracefully */
    ok_irql(PASSIVE_LEVEL);
    KeLowerIrql(PASSIVE_LEVEL);
}
