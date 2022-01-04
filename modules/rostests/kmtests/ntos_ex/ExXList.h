/* used by ExSingleList and ExSequencedList tests */
static
VOID
TestXListFunctional(
    IN PXLIST_HEADER ListHead,
    IN PXLIST_ENTRY Entries,
    IN PKSPIN_LOCK pSpinLock)
{
    USHORT ExpectedSequence = 0;
    PXLIST_ENTRY Ret;

    Ret = FlushXList(ListHead);
    ok_eq_pointer(Ret, NULL);
    CheckXListHeader(ListHead, NULL, 0);

    Ret = PopXList(ListHead, pSpinLock);
    ok_eq_pointer(Ret, NULL);
    CheckXListHeader(ListHead, NULL, 0);

    Ret = PushXList(ListHead, &Entries[0], pSpinLock);
    ++ExpectedSequence;
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(Entries[0].Next, NULL);
    CheckXListHeader(ListHead, &Entries[0], 1);

    Ret = PushXList(ListHead, &Entries[1], pSpinLock);
    ++ExpectedSequence;
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_eq_pointer(Entries[1].Next, &Entries[0]);
    CheckXListHeader(ListHead, &Entries[1], 2);

    Ret = PopXList(ListHead, pSpinLock);
    ok_eq_pointer(Ret, &Entries[1]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_free_xlist(Entries[1].Next, &Entries[0]);
    CheckXListHeader(ListHead, &Entries[0], 1);

    Ret = PopXList(ListHead, pSpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_free_xlist(Entries[0].Next, NULL);
    ok_free_xlist(Entries[1].Next, &Entries[0]);
    CheckXListHeader(ListHead, NULL, 0);

    Ret = PopXList(ListHead, pSpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_free_xlist(Entries[0].Next, NULL);
    ok_free_xlist(Entries[1].Next, &Entries[0]);
    CheckXListHeader(ListHead, NULL, 0);

    /* add entries again */
    Ret = PushXList(ListHead, &Entries[0], pSpinLock);
    ++ExpectedSequence;
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(Entries[0].Next, NULL);
    CheckXListHeader(ListHead, &Entries[0], 1);

    Ret = PushXList(ListHead, &Entries[1], pSpinLock);
    ++ExpectedSequence;
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_eq_pointer(Entries[1].Next, &Entries[0]);
    CheckXListHeader(ListHead, &Entries[1], 2);

    Ret = PopXList(ListHead, pSpinLock);
    ok_eq_pointer(Ret, &Entries[1]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_free_xlist(Entries[1].Next, &Entries[0]);
    CheckXListHeader(ListHead, &Entries[0], 1);

    Ret = PushXList(ListHead, &Entries[1], pSpinLock);
    ++ExpectedSequence;
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_eq_pointer(Entries[1].Next, &Entries[0]);
    CheckXListHeader(ListHead, &Entries[1], 2);

    Ret = PushXList(ListHead, &Entries[2], pSpinLock);
    ++ExpectedSequence;
    ok_eq_pointer(Ret, &Entries[1]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_eq_pointer(Entries[1].Next, &Entries[0]);
    ok_eq_pointer(Entries[2].Next, &Entries[1]);
    CheckXListHeader(ListHead, &Entries[2], 3);

    Ret = FlushXList(ListHead);
    ok_eq_pointer(Ret, &Entries[2]);
    CheckXListHeader(ListHead, NULL, 0);
}

#undef TestXListFunctional
