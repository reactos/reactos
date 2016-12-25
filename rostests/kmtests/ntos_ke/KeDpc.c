/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Deferred Procedure Call test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

//#define NDEBUG
#include <debug.h>

/* TODO: DPC importance */

static volatile LONG DpcCount;
static volatile UCHAR DpcImportance;

static KDEFERRED_ROUTINE DpcHandler;

static
VOID
NTAPI
DpcHandler(
    IN PRKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    ok_irql(DISPATCH_LEVEL);
    InterlockedIncrement(&DpcCount);
    ok(DeferredContext == Dpc, "DeferredContext = %p, Dpc = %p, expected equal\n", DeferredContext, Dpc);
    ok_eq_pointer(SystemArgument1, (PVOID)0xabc123);
    ok_eq_pointer(SystemArgument2, (PVOID)0x5678);

    /* KDPC object contents */
    ok_eq_uint(Dpc->Type, DpcObject);
    ok_eq_uint(Dpc->Importance, DpcImportance);
    ok_eq_uint(Dpc->Number, 0);
    ok(Dpc->DpcListEntry.Blink != NULL, "\n");
    ok(Dpc->DpcListEntry.Blink != &Dpc->DpcListEntry, "\n");
    if (!skip(Dpc->DpcListEntry.Blink != NULL, "DpcListEntry.Blink == NULL\n"))
        ok_eq_pointer(Dpc->DpcListEntry.Flink, Dpc->DpcListEntry.Blink->Flink);

    ok(Dpc->DpcListEntry.Flink != NULL, "\n");
    ok(Dpc->DpcListEntry.Flink != &Dpc->DpcListEntry, "\n");
    if (!skip(Dpc->DpcListEntry.Flink != NULL, "DpcListEntry.Flink == NULL\n"))
        ok_eq_pointer(Dpc->DpcListEntry.Blink, Dpc->DpcListEntry.Flink->Blink);

    ok_eq_pointer(Dpc->DeferredRoutine, DpcHandler);
    ok_eq_pointer(Dpc->DeferredContext, DeferredContext);
    ok_eq_pointer(Dpc->SystemArgument1, SystemArgument1);
    ok_eq_pointer(Dpc->SystemArgument2, SystemArgument2);
    ok_eq_pointer(Dpc->DpcData, NULL);

    ok_eq_uint(Prcb->DpcRoutineActive, 1);
    /* this DPC is not in the list anymore, but it was at the head! */
    ok_eq_pointer(Prcb->DpcData[DPC_NORMAL].DpcListHead.Flink, Dpc->DpcListEntry.Flink);
    ok_eq_pointer(Prcb->DpcData[DPC_NORMAL].DpcListHead.Blink, Dpc->DpcListEntry.Blink);
}

START_TEST(KeDpc)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KDPC Dpc;
    KIRQL Irql, Irql2, Irql3;
    LONG ExpectedDpcCount = 0;
    BOOLEAN Ret;
    int i;

    DpcCount = 0;
    DpcImportance = MediumImportance;

#define ok_dpccount() ok(DpcCount == ExpectedDpcCount, "DpcCount = %ld, expected %ld\n", DpcCount, ExpectedDpcCount);
    trace("Dpc = %p\n", &Dpc);
    memset(&Dpc, 0x55, sizeof Dpc);
    KeInitializeDpc(&Dpc, DpcHandler, &Dpc);
    /* check the Dpc object's fields */
    ok_eq_uint(Dpc.Type, DpcObject);
    ok_eq_uint(Dpc.Importance, DpcImportance);
    ok_eq_uint(Dpc.Number, 0);
    ok_eq_pointer(Dpc.DpcListEntry.Flink, (LIST_ENTRY *)0x5555555555555555LL);
    ok_eq_pointer(Dpc.DpcListEntry.Blink, (LIST_ENTRY *)0x5555555555555555LL);
    ok_eq_pointer(Dpc.DeferredRoutine, DpcHandler);
    ok_eq_pointer(Dpc.DeferredContext, &Dpc);
    ok_eq_pointer(Dpc.SystemArgument1, (PVOID)0x5555555555555555LL);
    ok_eq_pointer(Dpc.SystemArgument2, (PVOID)0x5555555555555555LL);
    ok_eq_pointer(Dpc.DpcData, NULL);

    /* simply run the Dpc a few times */
    for (i = 0; i < 5; ++i)
    {
        ok_dpccount();
        Ret = KeInsertQueueDpc(&Dpc, (PVOID)0xabc123, (PVOID)0x5678);
        ok_bool_true(Ret, "KeInsertQueueDpc returned");
        ++ExpectedDpcCount;
        ok_dpccount();
    }

    /* insert into queue at high irql
     * -> should only run when lowered to APC_LEVEL,
     *    inserting a second time should fail
     */
    KeRaiseIrql(APC_LEVEL, &Irql);
    for (i = 0; i < 5; ++i)
    {
        KeRaiseIrql(DISPATCH_LEVEL, &Irql2);
          ok_dpccount();
          Ret = KeInsertQueueDpc(&Dpc, (PVOID)0xabc123, (PVOID)0x5678);
          ok_bool_true(Ret, "KeInsertQueueDpc returned");
          Ret = KeInsertQueueDpc(&Dpc, (PVOID)0xdef, (PVOID)0x123);
          ok_bool_false(Ret, "KeInsertQueueDpc returned");
          ok_dpccount();
          KeRaiseIrql(HIGH_LEVEL, &Irql3);
            ok_dpccount();
          KeLowerIrql(Irql3);
          ok_dpccount();
        KeLowerIrql(Irql2);
        ++ExpectedDpcCount;
        ok_dpccount();
    }
    KeLowerIrql(Irql);

    /* now test removing from the queue */
    KeRaiseIrql(APC_LEVEL, &Irql);
    for (i = 0; i < 5; ++i)
    {
        KeRaiseIrql(DISPATCH_LEVEL, &Irql2);
          ok_dpccount();
          Ret = KeRemoveQueueDpc(&Dpc);
          ok_bool_false(Ret, "KeRemoveQueueDpc returned");
          Ret = KeInsertQueueDpc(&Dpc, (PVOID)0xabc123, (PVOID)0x5678);
          ok_bool_true(Ret, "KeInsertQueueDpc returned");
          ok_dpccount();
          KeRaiseIrql(HIGH_LEVEL, &Irql3);
            ok_dpccount();
          KeLowerIrql(Irql3);
          ok_dpccount();
          Ret = KeRemoveQueueDpc(&Dpc);
          ok_bool_true(Ret, "KeRemoveQueueDpc returned");
        KeLowerIrql(Irql2);
        ok_dpccount();
    }
    KeLowerIrql(Irql);

    /* parameter checks */
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        KeInitializeDpc(&Dpc, NULL, NULL);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);

    if (!skip(Status == STATUS_SUCCESS, "KeInitializeDpc failed\n"))
    {
        KeRaiseIrql(HIGH_LEVEL, &Irql);
          Ret = KeInsertQueueDpc(&Dpc, NULL, NULL);
          ok_bool_true(Ret, "KeInsertQueueDpc returned");
          Ret = KeRemoveQueueDpc(&Dpc);
          ok_bool_true(Ret, "KeRemoveQueueDpc returned");
        KeLowerIrql(Irql);
    }

    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        KeInitializeDpc(NULL, NULL, NULL);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);

    /* These result in IRQL_NOT_LESS_OR_EQUAL on 2k3 -- IRQLs 0x1f and 0xff (?)
    Ret = KeInsertQueueDpc(NULL, NULL, NULL);
    Ret = KeRemoveQueueDpc(NULL);*/

    ok_dpccount();
    ok_irql(PASSIVE_LEVEL);
    trace("Final Dpc count: %ld, expected %ld\n", DpcCount, ExpectedDpcCount);
}
