/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Section Object test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

#define StartSeh() ExceptionStatus = STATUS_SUCCESS; _SEH2_TRY {
#define EndSeh(ExpectedStatus) } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) { ExceptionStatus = _SEH2_GetExceptionCode(); } _SEH2_END; ok_eq_hex(ExceptionStatus, ExpectedStatus)

START_TEST(MmSection)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS ExceptionStatus;
    const PVOID InvalidPointer = (PVOID)0x5555555555555555ULL;
    PVOID SectionObject;
    LARGE_INTEGER MaximumSize;

    StartSeh()
        Status = MmCreateSection(NULL, 0, NULL, NULL, 0, SEC_RESERVE, NULL, NULL);
    EndSeh(STATUS_SUCCESS);
    ok_eq_hex(Status, STATUS_INVALID_PAGE_PROTECTION);

    if (!KmtIsCheckedBuild)
    {
        /* PAGE_NOACCESS and missing SEC_RESERVE/SEC_COMMIT/SEC_IMAGE assert */
        StartSeh()
            Status = MmCreateSection(NULL, 0, NULL, NULL, PAGE_NOACCESS, SEC_RESERVE, NULL, NULL);
        EndSeh(STATUS_ACCESS_VIOLATION);

        StartSeh()
            Status = MmCreateSection(NULL, 0, NULL, NULL, PAGE_NOACCESS, 0, NULL, NULL);
        EndSeh(STATUS_ACCESS_VIOLATION);
    }

    SectionObject = InvalidPointer;
    StartSeh()
        Status = MmCreateSection(&SectionObject, 0, NULL, NULL, 0, SEC_RESERVE, NULL, NULL);
    EndSeh(STATUS_SUCCESS);
    ok_eq_hex(Status, STATUS_INVALID_PAGE_PROTECTION);
    ok_eq_pointer(SectionObject, InvalidPointer);

    if (SectionObject && SectionObject != InvalidPointer)
        ObDereferenceObject(SectionObject);

    StartSeh()
        Status = MmCreateSection(NULL, 0, NULL, NULL, PAGE_READONLY, SEC_RESERVE, NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    SectionObject = InvalidPointer;
    StartSeh()
        Status = MmCreateSection(&SectionObject, 0, NULL, NULL, PAGE_READONLY, SEC_RESERVE, NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_pointer(SectionObject, InvalidPointer);

    if (SectionObject && SectionObject != InvalidPointer)
        ObDereferenceObject(SectionObject);

    SectionObject = InvalidPointer;
    MaximumSize.QuadPart = 0;
    StartSeh()
        Status = MmCreateSection(&SectionObject, 0, NULL, &MaximumSize, PAGE_READONLY, SEC_IMAGE, NULL, NULL);
    EndSeh(STATUS_SUCCESS);
    ok_eq_hex(Status, STATUS_INVALID_FILE_FOR_SECTION);
    ok_eq_longlong(MaximumSize.QuadPart, 0LL);
    ok_eq_pointer(SectionObject, InvalidPointer);

    if (SectionObject && SectionObject != InvalidPointer)
        ObDereferenceObject(SectionObject);

    MaximumSize.QuadPart = 0;
    StartSeh()
        Status = MmCreateSection(NULL, 0, NULL, &MaximumSize, PAGE_READONLY, SEC_RESERVE, NULL, NULL);
    EndSeh(STATUS_SUCCESS);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER_4);
    ok_eq_longlong(MaximumSize.QuadPart, 0LL);

    if (SectionObject && SectionObject != InvalidPointer)
        ObDereferenceObject(SectionObject);

    MaximumSize.QuadPart = 1;
    StartSeh()
        Status = MmCreateSection(NULL, 0, NULL, &MaximumSize, PAGE_READONLY, SEC_RESERVE, NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_longlong(MaximumSize.QuadPart, 1LL);

    if (SectionObject && SectionObject != InvalidPointer)
        ObDereferenceObject(SectionObject);

    SectionObject = InvalidPointer;
    MaximumSize.QuadPart = 0;
    StartSeh()
        Status = MmCreateSection(&SectionObject, 0, NULL, &MaximumSize, PAGE_READONLY, SEC_RESERVE, NULL, NULL);
    EndSeh(STATUS_SUCCESS);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER_4);
    ok_eq_longlong(MaximumSize.QuadPart, 0LL);
    ok_eq_pointer(SectionObject, InvalidPointer);

    if (SectionObject && SectionObject != InvalidPointer)
        ObDereferenceObject(SectionObject);

    SectionObject = InvalidPointer;
    MaximumSize.QuadPart = 1;
    StartSeh()
        Status = MmCreateSection(&SectionObject, 0, NULL, &MaximumSize, PAGE_READONLY, SEC_RESERVE, NULL, NULL);
    EndSeh(STATUS_SUCCESS);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_longlong(MaximumSize.QuadPart, 1LL);
    ok(SectionObject != InvalidPointer, "Section object pointer untouched\n");
    ok(SectionObject != NULL, "Section object pointer NULL\n");

    if (SectionObject && SectionObject != InvalidPointer)
        ObDereferenceObject(SectionObject);
}
