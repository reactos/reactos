
#include "precomp.h"

VOID
Test_Release(VOID)
{
    HANDLE MutantHandle;
    NTSTATUS Status;

    Status = NtCreateMutant(&MutantHandle, 0, NULL, TRUE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    Status = NtReleaseMutant(MutantHandle, NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

}



START_TEST(NtMutant)
{
    Test_Release();
}
