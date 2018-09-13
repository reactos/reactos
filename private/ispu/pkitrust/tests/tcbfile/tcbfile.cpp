//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       tcbfile.cpp
//
//  Contents:   cbfile tests
//
//  History:    13-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "cbfile.hxx"

CRITICAL_SECTION    MyCriticalSection;

void _DumpIndex(cBFile_ *pcBFile);
void _DumpHeader(cBFile_ *pcBFile);

DWORD                   dwTotal;

extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    WCHAR                   *pwszFile;
    DWORD                   dwExpectedReturn;
    BOOL                    fVerbose;
    int                     iRet;

    cWArgv_                 *pArgs;

    COleDateTime            tStart;
    COleDateTime            tEnd;
    COleDateTimeSpan        tsTotal;

    cBFile_                 *pcBFile;
    BOOL                    fCreatedOK = FALSE;

    pcBFile             = NULL;
    dwTotal             = 0;
    iRet                = 0;
    dwExpectedReturn    = S_OK;
    
    InitializeCriticalSection(&MyCriticalSection);

    if (!(pArgs = new cWArgv_((HINSTANCE)GetModuleHandle(NULL))))
    {
        goto MemoryError;
    }

    pArgs->AddUsageText(IDS_USAGETEXT_USAGE, IDS_USAGETEXT_OPTIONS,
                        IDS_USAGETEXT_CMDFILE, IDS_USAGETEXT_ADD,
                        IDS_USAGETEXT_OPTPARAM);

    pArgs->Add2List(IDS_PARAM_HELP,     IDS_PARAMTEXT_HELP,     WARGV_VALUETYPE_BOOL,   (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_VERBOSE,  IDS_PARAMTEXT_VERBOSE,  WARGV_VALUETYPE_BOOL,   (void *)FALSE);

    pArgs->Fill(argc, wargv);

    if (pArgs->GetValue(IDS_PARAM_HELP))
    {
        wprintf(L"%s\n", pArgs->GetUsageString());
        goto NeededHelp;
    }

    if (!(pwszFile = pArgs->GetFileName()))
    {
        wprintf(L"%s\n", pArgs->GetUsageString());
        goto ParamError;
    }

    fVerbose    = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_VERBOSE));

    if (!(pcBFile = new cBFile_(&MyCriticalSection, L".\\", pwszFile, 1, 1, BFILE_VERSION_1, &fCreatedOK)))
    {
        goto MemoryError;
    }

    if (!fCreatedOK)
    {
        goto DBError;
    }

    if (!(pcBFile->Initialize()))
    {
        goto DBError;
    }

    //
    //  start our timer
    //
    tStart      = COleDateTime::GetCurrentTime();

    //
    // dump the file's index
    //
    _DumpIndex(pcBFile);

    //
    //  dump the file's header
    //
    _DumpHeader(pcBFile);

    tEnd    = COleDateTime::GetCurrentTime();
    tsTotal = tEnd - tStart;

    printf("\n");
    printf("\nProcessing time:      %s", (LPCSTR)tsTotal.Format("%D:%H:%M:%S"));
    printf("\nAverage seconds per:  %f", (double)tsTotal.GetTotalSeconds() / (double)dwTotal);
    printf("\n");

    iRet = 0;

    CommonReturn:
        DeleteCriticalSection(&MyCriticalSection);

        DELETE_OBJECT(pArgs);
        DELETE_OBJECT(pcBFile);

        return(iRet);

    ErrorReturn:
        iRet = 1;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, MemoryError);
    TRACE_ERROR_EX(DBG_SS_APP, ParamError);
    TRACE_ERROR_EX(DBG_SS_APP, NeededHelp);
    TRACE_ERROR_EX(DBG_SS_APP, DBError);
}

void _DumpIndex(cBFile_ *pcBFile)
{
    BYTE    *pb;
    DWORD   dwIdx;
    DWORD   dwRecOffset;
    DWORD   i;

    pb = new BYTE[pcBFile->KeySize()];

    dwIdx = 0;

    while (pcBFile->GetDumpKey(dwIdx, pb, &dwRecOffset))
    {
        printf("\n");

        for (i = 0; i < pcBFile->KeySize(); i++)
        {
            printf("%02.2X", (int)pb[i]);
        }

        printf("  --> %lu", dwRecOffset);

        dwTotal++;

        dwIdx++;
    }

}


void _DumpHeader(cBFile_ *pcBFile)
{
    BFILE_HEADER    sHeader;

    if (pcBFile->GetHeader(&sHeader))
    {
        printf("\n");
        printf("\nHeader:");
        printf("\n   fDirty:        %s", (sHeader.fDirty) ? "TRUE" : "FALSE");
        printf("\n   sVersion:      %lu", (DWORD)sHeader.sVersion);
        printf("\n   sIntVersion:   %lu", (DWORD)sHeader.sIntVersion);
        printf("\n   cbSortedEOF:   %lu", sHeader.cbSortedEOF);
        printf("\n   cbKey:         %lu", sHeader.cbKey);
        printf("\n   cbData:        %lu", sHeader.cbData);
        printf("\n   dwLastRecNum:  %lu", sHeader.dwLastRecNum);
    }
}
