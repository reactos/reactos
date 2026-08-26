/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT.html)
 * PURPOSE:     Tests for the job object UI restrictions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

/*
 * Chromium 109, sandbox/win/src/job.cc. The JobLevel cases there fall through
 * into each other, so each level takes the flags of the ones below it too:
 * kLockdown adds the four here to kLimitedUser's, which comes to all eight.
 */
#define JOB_LIMITEDUSER_UI (JOB_OBJECT_UILIMIT_DISPLAYSETTINGS | \
                            JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS| \
                            JOB_OBJECT_UILIMIT_DESKTOP         | \
                            JOB_OBJECT_UILIMIT_EXITWINDOWS)

#define JOB_LOCKDOWN_UI (JOB_LIMITEDUSER_UI                 | \
                         JOB_OBJECT_UILIMIT_WRITECLIPBOARD  | \
                         JOB_OBJECT_UILIMIT_READCLIPBOARD   | \
                         JOB_OBJECT_UILIMIT_HANDLES         | \
                         JOB_OBJECT_UILIMIT_GLOBALATOMS)

static const ULONG SingleRestrictions[] =
{
    JOB_OBJECT_UILIMIT_HANDLES,
    JOB_OBJECT_UILIMIT_READCLIPBOARD,
    JOB_OBJECT_UILIMIT_WRITECLIPBOARD,
    JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS,
    JOB_OBJECT_UILIMIT_DISPLAYSETTINGS,
    JOB_OBJECT_UILIMIT_GLOBALATOMS,
    JOB_OBJECT_UILIMIT_DESKTOP,
    JOB_OBJECT_UILIMIT_EXITWINDOWS,
};

static
BOOL
SetRestrictions(
    _In_ HANDLE hJob,
    _In_ ULONG Restrictions)
{
    JOBOBJECT_BASIC_UI_RESTRICTIONS Info;

    Info.UIRestrictionsClass = Restrictions;
    return SetInformationJobObject(hJob,
                                   JobObjectBasicUIRestrictions,
                                   &Info,
                                   sizeof(Info));
}

static
void
test_RoundTrip(void)
{
    JOBOBJECT_BASIC_UI_RESTRICTIONS Info;
    HANDLE hJob;
    DWORD Returned;
    BOOL Success;
    ULONG i;

    hJob = CreateJobObjectW(NULL, NULL);
    ok(hJob != NULL, "CreateJobObject failed with %lu\n", GetLastError());
    if (hJob == NULL)
        return;

    /* A fresh job restricts nothing */
    memset(&Info, 0xAA, sizeof(Info));
    Returned = 0;
    SetLastError(0xDEADBEEF);
    Success = QueryInformationJobObject(hJob,
                                        JobObjectBasicUIRestrictions,
                                        &Info,
                                        sizeof(Info),
                                        &Returned);
    ok(Success == TRUE, "QueryInformationJobObject failed with %lu\n", GetLastError());
    ok_long(Info.UIRestrictionsClass, 0);
    ok_long(Returned, sizeof(Info));

    /* Every flag on its own, so one mistake cannot hide behind the others */
    for (i = 0; i < _countof(SingleRestrictions); i++)
    {
        SetLastError(0xDEADBEEF);
        Success = SetRestrictions(hJob, SingleRestrictions[i]);
        ok(Success == TRUE, "Setting 0x%lx failed with %lu\n",
           SingleRestrictions[i], GetLastError());

        memset(&Info, 0xAA, sizeof(Info));
        Returned = 0;
        Success = QueryInformationJobObject(hJob,
                                            JobObjectBasicUIRestrictions,
                                            &Info,
                                            sizeof(Info),
                                            &Returned);
        ok(Success == TRUE, "Querying 0x%lx failed with %lu\n",
           SingleRestrictions[i], GetLastError());
        ok_long(Info.UIRestrictionsClass, SingleRestrictions[i]);
        ok_long(Returned, sizeof(Info));

        /* Back to nothing, which drops the per-job state win32k keeps */
        SetLastError(0xDEADBEEF);
        Success = SetRestrictions(hJob, 0);
        ok(Success == TRUE, "Clearing 0x%lx failed with %lu\n",
           SingleRestrictions[i], GetLastError());

        memset(&Info, 0xAA, sizeof(Info));
        Success = QueryInformationJobObject(hJob,
                                            JobObjectBasicUIRestrictions,
                                            &Info,
                                            sizeof(Info),
                                            NULL);
        ok(Success == TRUE, "QueryInformationJobObject failed with %lu\n", GetLastError());
        ok_long(Info.UIRestrictionsClass, 0);
    }

    /* All of them at once */
    SetLastError(0xDEADBEEF);
    Success = SetRestrictions(hJob, JOB_OBJECT_UILIMIT_ALL);
    ok(Success == TRUE, "Setting JOB_OBJECT_UILIMIT_ALL failed with %lu\n", GetLastError());

    memset(&Info, 0xAA, sizeof(Info));
    Success = QueryInformationJobObject(hJob,
                                        JobObjectBasicUIRestrictions,
                                        &Info,
                                        sizeof(Info),
                                        NULL);
    ok(Success == TRUE, "QueryInformationJobObject failed with %lu\n", GetLastError());
    ok_long(Info.UIRestrictionsClass, JOB_OBJECT_UILIMIT_ALL);

    /* Setting the same value twice is not an error */
    SetLastError(0xDEADBEEF);
    Success = SetRestrictions(hJob, JOB_OBJECT_UILIMIT_ALL);
    ok(Success == TRUE, "Setting the same restrictions again failed with %lu\n",
       GetLastError());

    /* Close it while still restricted, to exercise the delete path */
    CloseHandle(hJob);
}

static
void
test_InvalidParameters(void)
{
    JOBOBJECT_BASIC_UI_RESTRICTIONS Info;
    HANDLE hJob;
    DWORD Returned;
    BOOL Success;

    hJob = CreateJobObjectW(NULL, NULL);
    ok(hJob != NULL, "CreateJobObject failed with %lu\n", GetLastError());
    if (hJob == NULL)
        return;

    /* Undefined bits must be rejected, and must not be stored */
    SetLastError(0xDEADBEEF);
    Success = SetRestrictions(hJob, 0xDEAD0000);
    ok(Success == FALSE, "Setting undefined restrictions succeeded\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xDEADBEEF);
    Success = SetRestrictions(hJob, JOB_OBJECT_UILIMIT_ALL | 0x100);
    ok(Success == FALSE, "Setting one undefined bit succeeded\n");
    ok_err(ERROR_INVALID_PARAMETER);

    memset(&Info, 0xAA, sizeof(Info));
    Success = QueryInformationJobObject(hJob,
                                        JobObjectBasicUIRestrictions,
                                        &Info,
                                        sizeof(Info),
                                        NULL);
    ok(Success == TRUE, "QueryInformationJobObject failed with %lu\n", GetLastError());
    ok_long(Info.UIRestrictionsClass, 0);

    /* The class is fixed length in both directions */
    Info.UIRestrictionsClass = JOB_OBJECT_UILIMIT_HANDLES;
    SetLastError(0xDEADBEEF);
    Success = SetInformationJobObject(hJob,
                                      JobObjectBasicUIRestrictions,
                                      &Info,
                                      sizeof(Info) - 1);
    ok(Success == FALSE, "SetInformationJobObject with a short buffer succeeded\n");
    ok_err(ERROR_BAD_LENGTH);

    SetLastError(0xDEADBEEF);
    Success = SetInformationJobObject(hJob,
                                      JobObjectBasicUIRestrictions,
                                      &Info,
                                      sizeof(Info) + 1);
    ok(Success == FALSE, "SetInformationJobObject with a long buffer succeeded\n");
    ok_err(ERROR_BAD_LENGTH);

    Returned = 0;
    SetLastError(0xDEADBEEF);
    Success = QueryInformationJobObject(hJob,
                                        JobObjectBasicUIRestrictions,
                                        &Info,
                                        sizeof(Info) - 1,
                                        &Returned);
    ok(Success == FALSE, "QueryInformationJobObject with a short buffer succeeded\n");
    ok_err(ERROR_BAD_LENGTH);

    /* An inaccessible buffer must be reported, not raised */
    SetLastError(0xDEADBEEF);
    Success = SetInformationJobObject(hJob,
                                      JobObjectBasicUIRestrictions,
                                      NULL,
                                      sizeof(Info));
    ok(Success == FALSE, "SetInformationJobObject with a NULL buffer succeeded\n");
    ok_err(ERROR_NOACCESS);

    CloseHandle(hJob);
}

/* A handle without JOB_OBJECT_SET_ATTRIBUTES may not change the restrictions */
static
void
test_Access(void)
{
    JOBOBJECT_BASIC_UI_RESTRICTIONS Info;
    HANDLE hJob, hQueryOnly;
    BOOL Success;

    hJob = CreateJobObjectW(NULL, NULL);
    ok(hJob != NULL, "CreateJobObject failed with %lu\n", GetLastError());
    if (hJob == NULL)
        return;

    hQueryOnly = NULL;
    Success = DuplicateHandle(GetCurrentProcess(),
                              hJob,
                              GetCurrentProcess(),
                              &hQueryOnly,
                              JOB_OBJECT_QUERY,
                              FALSE,
                              0);
    ok(Success == TRUE, "DuplicateHandle failed with %lu\n", GetLastError());
    if (Success)
    {
        SetLastError(0xDEADBEEF);
        Success = SetRestrictions(hQueryOnly, JOB_OBJECT_UILIMIT_HANDLES);
        ok(Success == FALSE, "Setting restrictions through a query handle succeeded\n");
        ok_err(ERROR_ACCESS_DENIED);

        memset(&Info, 0xAA, sizeof(Info));
        Success = QueryInformationJobObject(hQueryOnly,
                                            JobObjectBasicUIRestrictions,
                                            &Info,
                                            sizeof(Info),
                                            NULL);
        ok(Success == TRUE, "QueryInformationJobObject failed with %lu\n", GetLastError());
        ok_long(Info.UIRestrictionsClass, 0);

        CloseHandle(hQueryOnly);
    }

    CloseHandle(hJob);
}

/*
 * The two policies the Chromium sandbox builds, in the order it builds them.
 * The GPU one excepts every UI restriction away again, which has to end up
 * clearing them rather than failing.
 */
static
void
test_SandboxPolicies(void)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimit;
    JOBOBJECT_BASIC_UI_RESTRICTIONS Info;
    HANDLE hJob;
    BOOL Success;

    /* sandbox::Job::Init(JOB_LOCKDOWN) */
    hJob = CreateJobObjectW(NULL, NULL);
    ok(hJob != NULL, "CreateJobObject failed with %lu\n", GetLastError());
    if (hJob == NULL)
        return;

    memset(&ExtendedLimit, 0, sizeof(ExtendedLimit));
    ExtendedLimit.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION |
        JOB_OBJECT_LIMIT_ACTIVE_PROCESS |
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    ExtendedLimit.BasicLimitInformation.ActiveProcessLimit = 1;

    SetLastError(0xDEADBEEF);
    Success = SetInformationJobObject(hJob,
                                      JobObjectExtendedLimitInformation,
                                      &ExtendedLimit,
                                      sizeof(ExtendedLimit));
    ok(Success == TRUE, "Setting the lockdown limits failed with %lu\n", GetLastError());

    SetLastError(0xDEADBEEF);
    Success = SetRestrictions(hJob, JOB_LOCKDOWN_UI);
    ok(Success == TRUE, "Setting the lockdown restrictions failed with %lu\n",
       GetLastError());

    memset(&Info, 0xAA, sizeof(Info));
    Success = QueryInformationJobObject(hJob,
                                        JobObjectBasicUIRestrictions,
                                        &Info,
                                        sizeof(Info),
                                        NULL);
    ok(Success == TRUE, "QueryInformationJobObject failed with %lu\n", GetLastError());
    ok_long(Info.UIRestrictionsClass, JOB_LOCKDOWN_UI);

    CloseHandle(hJob);

    /* Job::Init(JobLevel::kLimitedUser), which does not take kLockdown's four */
    hJob = CreateJobObjectW(NULL, NULL);
    ok(hJob != NULL, "CreateJobObject failed with %lu\n", GetLastError());
    if (hJob == NULL)
        return;

    memset(&ExtendedLimit, 0, sizeof(ExtendedLimit));
    ExtendedLimit.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_ACTIVE_PROCESS |
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    ExtendedLimit.BasicLimitInformation.ActiveProcessLimit = 1;

    SetLastError(0xDEADBEEF);
    Success = SetInformationJobObject(hJob,
                                      JobObjectExtendedLimitInformation,
                                      &ExtendedLimit,
                                      sizeof(ExtendedLimit));
    ok(Success == TRUE, "Setting the limited user limits failed with %lu\n", GetLastError());

    SetLastError(0xDEADBEEF);
    Success = SetRestrictions(hJob, JOB_LIMITEDUSER_UI);
    ok(Success == TRUE, "Setting the limited user restrictions failed with %lu\n",
       GetLastError());

    memset(&Info, 0xAA, sizeof(Info));
    Success = QueryInformationJobObject(hJob,
                                        JobObjectBasicUIRestrictions,
                                        &Info,
                                        sizeof(Info),
                                        NULL);
    ok(Success == TRUE, "QueryInformationJobObject failed with %lu\n", GetLastError());
    ok_long(Info.UIRestrictionsClass, JOB_LIMITEDUSER_UI);

    CloseHandle(hJob);

    /*
     * job.cc applies the caller's exceptions as
     * jbur.UIRestrictionsClass &= ~ui_exceptions before its single
     * SetInformationJobObject, so a delegate that excepts everything away asks
     * for a mask of zero. That has to be accepted, not refused.
     */
    hJob = CreateJobObjectW(NULL, NULL);
    ok(hJob != NULL, "CreateJobObject failed with %lu\n", GetLastError());
    if (hJob == NULL)
        return;

    SetLastError(0xDEADBEEF);
    Success = SetRestrictions(hJob, JOB_LOCKDOWN_UI & ~JOB_OBJECT_UILIMIT_ALL);
    ok(Success == TRUE, "Excepting every restriction away failed with %lu\n",
       GetLastError());

    memset(&Info, 0xAA, sizeof(Info));
    Success = QueryInformationJobObject(hJob,
                                        JobObjectBasicUIRestrictions,
                                        &Info,
                                        sizeof(Info),
                                        NULL);
    ok(Success == TRUE, "QueryInformationJobObject failed with %lu\n", GetLastError());
    ok_long(Info.UIRestrictionsClass, 0);

    CloseHandle(hJob);
}

START_TEST(JobObject)
{
    test_RoundTrip();
    test_InvalidParameters();
    test_Access();
    test_SandboxPolicies();
}
