//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       perftest.cpp
//
//  Contents:   performance suite
//
//  History:    04-Dec-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

DWORD                   dwTotal             = 0;
DWORD                   dwExpectedError     = 0;
DWORD                   cPasses             = 1;
BOOL                    fCheckExpectedError = FALSE;
BOOL                    fVerbose            = FALSE;
WCHAR                   *pwszInFile         = NULL;

GUID                    gAuthCode       = WINTRUST_ACTION_GENERIC_VERIFY_V2;
GUID                    gDriver         = DRIVER_ACTION_VERIFY;
GUID                    gCertProvider   = WINTRUST_ACTION_GENERIC_CERT_VERIFY;

HANDLE                  *pahThreads         = NULL;

DWORD                   cThreads            = 1;
ThreadData              *pasThreads         = NULL;

extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    int                     iRet;
    cWArgv_                 *pArgs;
    COleDateTime            tStart;
    COleDateTime            tEnd;
    COleDateTimeSpan        tsTotal;

    PFN_TEST                pfnTest;
    DWORD                   i;

    iRet = 0;

    pfnTest = NULL;

    if (!(pArgs = new cWArgv_((HINSTANCE)GetModuleHandle(NULL))))
    {
        goto MemoryError;
    }

    pArgs->AddUsageText(IDS_USAGETEXT_USAGE, IDS_USAGETEXT_OPTIONS,
                        IDS_USAGETEXT_CMDFILE, IDS_USAGETEXT_ADD,
                        IDS_USAGETEXT_OPTPARAM);

    pArgs->Add2List(IDS_PARAM_HELP,     IDS_PARAMTEXT_HELP,     WARGV_VALUETYPE_BOOL,   (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_VERBOSE,  IDS_PARAMTEXT_VERBOSE,  WARGV_VALUETYPE_BOOL,   (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_EXPERROR, IDS_PARAMTEXT_EXPERROR, WARGV_VALUETYPE_DWORDH, (void *)0);

    pArgs->Add2List(IDS_PARAM_NOTHREADS,IDS_PARAMTEXT_NOTHREADS,WARGV_VALUETYPE_DWORDD, (void *)1);
    pArgs->Add2List(IDS_PARAM_NOPASSES, IDS_PARAMTEXT_NOPASSES, WARGV_VALUETYPE_DWORDD, (void *)2);

    pArgs->Add2List(IDS_PARAM_WVTCAT,   IDS_PARAMTEXT_WVTCAT,   WARGV_VALUETYPE_BOOL,   (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_WVTCERT,  IDS_PARAMTEXT_WVTCERT,  WARGV_VALUETYPE_BOOL,   (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_WVTFILE,  IDS_PARAMTEXT_WVTFILE,  WARGV_VALUETYPE_BOOL,   (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_CATADD,   IDS_PARAMTEXT_CATADD,   WARGV_VALUETYPE_BOOL,   (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_CRYPTHASH,IDS_PARAMTEXT_CRYPTHASH,WARGV_VALUETYPE_BOOL,   (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_HASHSHA1, IDS_PARAMTEXT_HASHSHA1, WARGV_VALUETYPE_BOOL,   (void *)FALSE);

    pArgs->Fill(argc, wargv);

    if (!(pArgs->Fill(argc, wargv)) ||
        (pArgs->GetValue(IDS_PARAM_HELP)))
    {
        wprintf(L"%s\n", pArgs->GetUsageString());
        goto NeededHelp;
    }

    pwszInFile  = pArgs->GetFileName();
    fVerbose    = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_VERBOSE));
    cThreads    = (DWORD)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_NOTHREADS));
    cPasses     = (DWORD)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_NOPASSES));

    if (!(pwszInFile))
    {
        pwszInFile = L"*.*";
    }

    if (pArgs->IsSet(IDS_PARAM_EXPERROR))
    {
        dwExpectedError     = (DWORD)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_EXPERROR));
        fCheckExpectedError = TRUE;
    }

    if (cThreads < 1)
    {
        wprintf(L"%s\n", pArgs->GetUsageString());
        goto NeededHelp;
    }

    if (!(pasThreads = new ThreadData[cThreads]))
    {
        goto MemoryError;
    }

    if (!(pahThreads = new HANDLE[cThreads]))
    {
        goto MemoryError;
    }

    memset(pasThreads, 0x00, sizeof(ThreadData) * cThreads);


    if (pArgs->GetValue(IDS_PARAM_WVTCAT))
    {
        pfnTest = TestWVTCat;
    }
    else if (pArgs->GetValue(IDS_PARAM_WVTCERT))
    {
        pfnTest = TestWVTCert;
    }
    else if (pArgs->GetValue(IDS_PARAM_WVTFILE))
    {
        pfnTest = TestWVTFile;
    }
    else if (pArgs->GetValue(IDS_PARAM_CATADD))
    {
        pfnTest = TestCatAdd;
    }
    else if (pArgs->GetValue(IDS_PARAM_CRYPTHASH))
    {
        pfnTest = TestCryptHash;

        if (pArgs->GetValue(IDS_PARAM_HASHSHA1))
        {
            for (i = 0; i < cThreads; i++)
            {
                pasThreads[i].dwPassThrough = PASSTHROUGH_SHA1;
            }
        }
    }

    if (!(pfnTest))
    {
        wprintf(L"%s\n", pArgs->GetUsageString());
        goto NeededHelp;
    }

    for (i = 0; i < cThreads; i++)
    {
        pasThreads[i].hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pfnTest,
                                             &pasThreads[i], CREATE_SUSPENDED, &pasThreads[i].dwId);

        if (!(pasThreads[i].hThread))
        {
            goto CreateThreadFailed;
        }

        pahThreads[i] = pasThreads[i].hThread;
    }

    //
    //  start our timer
    //
    tStart      = COleDateTime::GetCurrentTime();

    for (i = 0; i < cThreads; i++)
    {
        ResumeThread(pasThreads[i].hThread);
    }

    //
    //  wait to finish
    //
    WaitForMultipleObjects(cThreads, pahThreads, TRUE, INFINITE);

    //
    //  stop our timer
    //
    tEnd    = COleDateTime::GetCurrentTime();
    tsTotal = tEnd - tStart;

    for (i = 0; i < cThreads; i++)
    {
        dwTotal += pasThreads[i].dwTotalProcessed;

        printf("\nThread #%d:", i + 1);
        printf("\n  Processing time:        %s", (LPCSTR)pasThreads[i].tsTotal.Format("%D:%H:%M:%S"));
        printf("\n  Total processed:        %ld", pasThreads[i].dwTotalProcessed);
        printf("\n  Average seconds per:    %f", (double)pasThreads[i].tsTotal.GetTotalSeconds() /
                                                 (double)pasThreads[i].dwTotalProcessed);
    }

    printf("\nOverall:");
    printf("\n  Processing time:        %s", (LPCSTR)tsTotal.Format("%D:%H:%M:%S"));
    printf("\n  Total processed:        %ld", dwTotal);
    printf("\n  Average seconds per:    %f", (double)tsTotal.GetTotalSeconds() / (double)dwTotal);
    printf("\n");

    iRet = 0;

CommonReturn:

    if (pasThreads)
    {
        for (i = 0; i < cThreads; i++)
        {
            if (pasThreads[i].hThread)
            {
                CloseHandle(pasThreads[i].hThread);
            }
        }

        delete pasThreads;
    }

    DELETE_OBJECT(pArgs);
    DELETE_OBJECT(pahThreads);

    return(iRet);

ErrorReturn:
    iRet = 1;
    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, MemoryError);
    TRACE_ERROR_EX(DBG_SS_APP, NeededHelp);
    TRACE_ERROR_EX(DBG_SS_APP, CreateThreadFailed);
}

