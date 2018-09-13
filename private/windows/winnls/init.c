/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    init.c

Abstract:

    This file contains the initialization code for the NLS APIs.

    External Routines found in this file:
      NlsDllInitialize

Revision History:

    05-31-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nls.h"
#include "stdio.h"



//
//  Global Variables.
//

HANDLE                hModule;              // handle to module
RTL_CRITICAL_SECTION  gcsTblPtrs;           // critical section for tbl ptrs

UINT                  gAnsiCodePage;        // Ansi code page value
UINT                  gOemCodePage;         // OEM code page value
UINT                  gMacCodePage;         // MAC code page value
LCID                  gSystemLocale;        // system locale value
LANGID                gSystemInstallLang;   // system's original install language
PLOC_HASH             gpSysLocHashN;        // ptr to system loc hash node
PCP_HASH              gpACPHashN;           // ptr to ACP hash node
PCP_HASH              gpOEMCPHashN;         // ptr to OEMCP hash node
PCP_HASH              gpMACCPHashN;         // ptr to MACCP hash node

HANDLE                hCodePageKey;         // handle to System\Nls\CodePage key
HANDLE                hLocaleKey;           // handle to System\Nls\Locale key
HANDLE                hAltSortsKey;         // handle to Locale\Alternate Sorts key
HANDLE                hLangGroupsKey;       // handle to System\Nls\Language Groups key
PNLS_USER_INFO        pNlsUserInfo;         // ptr to the user info cache
HANDLE                hNlsCacheMutant;      // handle to cache semaphore

RTL_CRITICAL_SECTION  gcsNlsProcessCache;   // critical section for nls process cache



//
//  Forward Declarations.
//

ULONG
NlsServerInitialize(void);

ULONG
NlsProcessInitialize(void);

ULONG
NlsInitializeCacheMutant(void);

void
InitKoreanWeights(void);





//-------------------------------------------------------------------------//
//                           EXTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NlsDllInitialize
//
//  DLL Entry initialization procedure for NLSAPI.  This is called by
//  the base dll initialization.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOLEAN NlsDllInitialize(
    IN PVOID hMod,
    ULONG Reason,
    IN PBASE_STATIC_SERVER_DATA pBaseStaticServerData)
{
    ULONG rc = 0L;                     // return code


    if (Reason == DLL_PROCESS_ATTACH)
    {
        //
        // Let's open the cache mutant here. It's created on the server
        // side (CSRSS).
        //
        rc = NlsInitializeCacheMutant();
        if (rc)
        {
            KdPrint(("NLSAPI: Could NOT open hNlsMutantCache - %lx.\n", rc));
            return (FALSE);
        }

        //
        //  Save module handle for use later.
        //
        hModule = (HANDLE)hMod;

        //
        //  Initialize the cached user info pointer.
        //
        pNlsUserInfo = &(pBaseStaticServerData->NlsUserInfo);

        //
        //  Process attaching, so initialize tables.
        //
        rc = NlsServerInitialize();
        if (rc)
        {
            KdPrint(("NLSAPI: Could NOT initialize Server - %lx.\n", rc));
            return (FALSE);
        }

        rc = NlsProcessInitialize();
        if (rc)
        {
            KdPrint(("NLSAPI: Could NOT initialize Process - %lx.\n", rc));
//
// Don't return.  If we return, we prevent kernel32 from init'ing.  If this
// is for winlogon, the system will not boot.  Unhealthy.
//
// LATER. Record the failure in the log.
            return (FALSE);
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsThreadCleanup
//
//  Cleanup for thread resources when it terminates.
//
//  03-30-99    SamerA    Created.
////////////////////////////////////////////////////////////////////////////

BOOLEAN NlsThreadCleanup(void)
{
    if (NtCurrentTeb()->NlsCache)
    {
        CLOSE_REG_KEY( ((PNLS_LOCAL_CACHE)NtCurrentTeb()->NlsCache)->CurrentUserKeyHandle );
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     NtCurrentTeb()->NlsCache
                   );
    }

    return (TRUE);
}



//-------------------------------------------------------------------------//
//                           INTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////
//
//  NlsInitializeCacheMutant
//
//  Tries to open the named cache mutant already created by CSRSS.
//
//  11-81-98    SamerA    Created.
////////////////////////////////////////////////////////////////////////////

ULONG NlsInitializeCacheMutant(void)
{
    ULONG rc;
    UNICODE_STRING ObNlsCacheMutantName;         // nls cache mutant name
    WCHAR wchNlsMutantName[MAX_PATH];
    OBJECT_ATTRIBUTES NlsMutantCacheObjA;        // nls cache mutant object attributes

    //
    // First let's do the security stuff
    //

    //
    // Construct the cache mutant in the correct session space, in case
    // we are running on Hydra
    //
    wchNlsMutantName[0] = UNICODE_NULL;
    if (SessionId != 0)
    {
        swprintf(wchNlsMutantName, L"\\sessions\\%ld", SessionId);
    }

    swprintf(wchNlsMutantName, L"%ws\\%ws", wchNlsMutantName, NLS_CACHE_MUTANT_NAME);
    RtlInitUnicodeString( &ObNlsCacheMutantName, wchNlsMutantName);

    InitializeObjectAttributes( &NlsMutantCacheObjA,
                                &ObNlsCacheMutantName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    // Try to open the Mutant
    rc = NtOpenMutant( &hNlsCacheMutant,
                       MUTANT_QUERY_STATE,     // do we need all access ?
                       &NlsMutantCacheObjA );

    if (!NT_SUCCESS( rc ))
    {
        KdPrint(("NLSAPI : Could NOT open Nls Cache Mutex - %lx.\n", rc));
        return ( rc );
    }

    return (NO_ERROR);
}

////////////////////////////////////////////////////////////////////////////
//
//  NlsServerInitialize
//
//  Server initialization procedure for NLSAPI.  This is the ONE-TIME
//  initialization code for the NLSAPI DLL.  It simply does the calls
//  to NtCreateSection for the code pages that are currently found in the
//  system.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG NlsServerInitialize(void)
{
    HANDLE hSec = (HANDLE)0;           // section handle
    ULONG rc = 0L;                     // return code

#ifndef DOSWIN32
    PIMAGE_NT_HEADERS NtHeaders;

    //
    //  This is to avoid being initialized again when NTSD dynlinks to
    //  a server to get at its debugger extensions.
    //
    NtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);
    if (NtHeaders->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_NATIVE)
    {
        return (NO_ERROR);
    }
#endif

    //
    //  MultiUser NT (Hydra). SesssionId = 0 is the console CSRSS.
    //  If this is NOT the first server process, then just return success,
    //  since we only want to create the object directories once.
    //
    if (NtCurrentPeb()->SessionId != 0)
    {
        return (NO_ERROR);
    }

    //
    //  Create the NLS object directory.
    //
    //  Must create a separate directory off the root in order to have
    //  CreateSection access on the fly.
    //
    if (rc = CreateNlsObjectDirectory())
    {
        return (rc);
    }

    //
    //  The ACP, OEMCP, and Default Language files are already created
    //  at boot time.  The pointers to the files are stored in the PEB.
    //
    //  Create the section for the following data files:
    //      UNICODE
    //      LOCALE
    //      CTYPE
    //      SORTKEY
    //      SORT TABLES
    //
    //  All other data files will have the sections created only as they
    //  are needed.
    //
    if ((!NT_SUCCESS(rc = CsrBasepNlsCreateSection( NLS_CREATE_SECTION_UNICODE, 0, &hSec))) ||
        (!NT_SUCCESS(rc = CsrBasepNlsCreateSection( NLS_CREATE_SECTION_LOCALE, 0, &hSec)))  ||
        (!NT_SUCCESS(rc = CsrBasepNlsCreateSection( NLS_CREATE_SECTION_CTYPE, 0, &hSec)))   ||
        (!NT_SUCCESS(rc = CsrBasepNlsCreateSection( NLS_CREATE_SECTION_SORTKEY, 0, &hSec))) ||
        (!NT_SUCCESS(rc = CsrBasepNlsCreateSection( NLS_CREATE_SECTION_SORTTBLS, 0, &hSec))))
    {
        return (rc);
    }

    //
    //  Return success.
    //
    return (NO_ERROR);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsProcessInitialize
//
//  Process initialization procedure for NLS API.  This routine sets up all
//  of the tables so that they are accessable from the current process.  If
//  it is unable to allocate the appropriate memory or memory map the
//  appropriate files, an error is returned.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG NlsProcessInitialize(void)
{
    ULONG rc = 0L;                     // return code
    LPWORD pBaseAddr;                  // ptr to base address of section
    LCID UserLocale;                   // user locale id
    PLOC_HASH pUserLocHashN;           // ptr to user locale hash node


    //
    // Initialize the critical section that protects
    // the NLS cache for this process.
    //
    RtlInitializeCriticalSection(&gcsNlsProcessCache);

    //
    //  Initialize the table pointers critical section.
    //  Enter the critical section to set up the tables.
    //
    RtlInitializeCriticalSectionAndSpinCount(&gcsTblPtrs, 4000);
    RtlEnterCriticalSection(&gcsTblPtrs);

    //
    //  Allocate initial tables.
    //
    if (rc = AllocTables())
    {
        KdPrint(("AllocTables failed, rc %lx\n", rc));
        RtlLeaveCriticalSection(&gcsTblPtrs);
        return (rc);
    }

    //
    //  Initialize the handles to the various registry keys to NULL.
    //
    hCodePageKey = NULL;
    hLocaleKey = NULL;
    hAltSortsKey = NULL;
    hLangGroupsKey = NULL;

    //
    //  Get the ANSI code page value.
    //  Create the hash node for the ACP.
    //  Insert the hash node into the global CP hash table.
    //
    //  At this point, the ACP table has already been mapped into
    //  the process, so get the pointer from the PEB.
    //
    pBaseAddr = NtCurrentPeb()->AnsiCodePageData;
    gAnsiCodePage = ((PCP_TABLE)(pBaseAddr + CP_HEADER))->CodePage;
    if (rc = MakeCPHashNode( gAnsiCodePage,
                             pBaseAddr,
                             &gpACPHashN,
                             FALSE,
                             NULL ))
    {
        RtlLeaveCriticalSection(&gcsTblPtrs);
        return (rc);
    }

    //
    //  Get the OEM code page value.
    //  Create the hash node for the OEMCP.
    //  Insert the hash node into the global CP hash table.
    //
    //  At this point, the OEMCP table has already been mapped into
    //  the process, so get the pointer from the PEB.
    //
    pBaseAddr = NtCurrentPeb()->OemCodePageData;
    gOemCodePage = ((PCP_TABLE)(pBaseAddr + CP_HEADER))->CodePage;
    if (gOemCodePage != gAnsiCodePage)
    {
        //
        //  Oem code page is different than the Ansi code page, so
        //  need to create and store the new hash node.
        //
        if (rc = MakeCPHashNode( gOemCodePage,
                                 pBaseAddr,
                                 &gpOEMCPHashN,
                                 FALSE,
                                 NULL ))
        {
            RtlLeaveCriticalSection(&gcsTblPtrs);
            return (rc);
        }
    }
    else
    {
        //
        //  Oem code page is the same as the Ansi code page, so set
        //  the oem cp hash node to be the same as the ansi cp hash node.
        //
        gpOEMCPHashN = gpACPHashN;
    }

    //
    //  Initialize the MAC code page values to 0.
    //  These values will be set the first time they are requested for use.
    //
    gMacCodePage = 0;
    gpMACCPHashN = NULL;

    //
    //  Open and Map a View of the Section for UNICODE.NLS.
    //  Save the pointers to the table information in the table ptrs
    //  structure.
    //
    if (rc = GetUnicodeFileInfo())
    {
        KdPrint(("GetUnicodeFileInfo failed, rc %lx\n", rc));
        RtlLeaveCriticalSection(&gcsTblPtrs);
        return (rc);
    }

    //
    //  Cache the system locale value.
    //
    rc = NtQueryDefaultLocale(FALSE, &gSystemLocale);
    if (!NT_SUCCESS(rc))
    {
        RtlLeaveCriticalSection(&gcsTblPtrs);
        return (rc);
    }

    //
    //  Store the user locale value.
    //
    if ((pNlsUserInfo->fCacheValid) && (pNlsUserInfo->UserLocaleId != 0))
    {
        UserLocale = pNlsUserInfo->UserLocaleId;
    }
    else
    {
        UserLocale = gSystemLocale;
    }

    //
    //  Initialize the system install language to zero.  This will only
    //  be retrieved on an as need basis.
    //
    gSystemInstallLang = 0;

    //
    //  Open and Map a View of the Section for LOCALE.NLS.
    //  Create and insert the hash node into the global Locale hash table
    //  for the system default locale.
    //
    if (rc = GetLocaleFileInfo( gSystemLocale,
                                &gpSysLocHashN,
                                TRUE ))
    {
        //
        //  Change the system locale to be the default (English).
        //
        if (GetLocaleFileInfo( MAKELCID(NLS_DEFAULT_LANGID, SORT_DEFAULT),
                               &gpSysLocHashN,
                               TRUE ))
        {
            KdPrint(("Couldn't do English\n"));
            RtlLeaveCriticalSection(&gcsTblPtrs);
            return (rc);
        }
        else
        {
            //
            //  Registry is corrupt, but allow the English default to
            //  work.  Need to reset the system default.
            //
            gSystemLocale = MAKELCID(NLS_DEFAULT_LANGID, SORT_DEFAULT);
            KdPrint(("NLSAPI: Registry is corrupt - Using Default Locale.\n"));
        }
    }

    //
    //  If the user default locale is different from the system default
    //  locale, then create and insert the hash node into the global
    //  Locale hash table for the user default locale.
    //
    //  NOTE:  The System Default Locale Hash Node should be
    //         created before this call.
    //
    if (UserLocale != gSystemLocale)
    {
        if (rc = GetLocaleFileInfo( UserLocale,
                                    &pUserLocHashN,
                                    TRUE ))
        {
            //
            //  Change the user locale to be equal to the system default.
            //
            UserLocale = gSystemLocale;
            KdPrint(("NLSAPI: Registry is corrupt - User Locale Now Equals System Locale.\n"));
        }
    }

    //
    //  Open and Map a View of the Section for SORTKEY.NLS.
    //  Save the pointers to the semaphore dword and the default sortkey
    //  table in the table ptrs structure.
    //
    if (rc = GetDefaultSortkeyFileInfo())
    {
        KdPrint(("NLSAPI: Initialization, GetDefaultSortkeyFileInfo failed with rc %lx.\n", rc));
//        RtlLeaveCriticalSection(&gcsTblPtrs);
//        return (rc);
    }

    //
    //  Open and Map a View of the Section for SORTTBLS.NLS.
    //  Save the pointers to the sort table information in the
    //  table ptrs structure.
    //
    if (rc = GetDefaultSortTablesFileInfo())
    {
        RtlLeaveCriticalSection(&gcsTblPtrs);
        return (rc);
    }

    //
    //  Get the language information portion of the system locale.
    //
    //  NOTE:  GetDefaultSortkeyFileInfo and GetDefaultSortTablesFileInfo
    //         should be called before this so that the default sorting
    //         tables are already initialized at the time of the call.
    //
    if (rc = GetLanguageFileInfo( gSystemLocale,
                                  &gpSysLocHashN,
                                  FALSE,
                                  0 ))
    {
        RtlLeaveCriticalSection(&gcsTblPtrs);
        return (rc);
    }

    //
    //  If the user default is different from the system default,
    //  get the language information portion of the user default locale.
    //
    //  NOTE:  GetDefaultSortkeyFileInfo and GetDefaultSortTablesFileInfo
    //         should be called before this so that the default sorting
    //         tables are already initialized at the time of the call.
    //
    if (gSystemLocale != UserLocale)
    {
        if (rc = MakeLangHashNode( UserLocale,
                                   NULL,
                                   &pUserLocHashN,
                                   FALSE ))
        {
            RtlLeaveCriticalSection(&gcsTblPtrs);
            return (rc);
        }
    }

    //
    //  Initialize the Korean SMWeight values.
    //
    InitKoreanWeights();

    //
    //  Leave the critical section.
    //
    RtlLeaveCriticalSection(&gcsTblPtrs);

    //
    //  Return success.
    //
    return (NO_ERROR);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitKoreanWeights
//
//  Creates the SMWeight array with the IDEOGRAPH script member sorting
//  before all other script members.
//
//  NOTE: This function assumes we're in a critical section.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void InitKoreanWeights()
{
    DWORD ctr;                                 // loop counter
    BYTE NewScript;                            // new script to store
    LPBYTE pSMWeight = pTblPtrs->SMWeight;     // ptr to script member weights
    PMULTI_WT pMulti;                          // ptr to multi weight
    ULONG rc = 0L;                             // return code


    //
    //  Set the 0 to FIRST_SCRIPT of script structure to its default
    //  value.
    //
    RtlZeroMemory(pSMWeight, NUM_SM);
    for (ctr = 1; ctr < FIRST_SCRIPT; ctr++)
    {
        pSMWeight[ctr] = (BYTE)ctr;
    }

    //
    //  Save the order in the SMWeight array.
    //
    NewScript = FIRST_SCRIPT;
    pSMWeight[IDEOGRAPH] = NewScript;
    NewScript++;

    //
    //  See if the script is part of a multiple weights script.
    //
    pMulti = pTblPtrs->pMultiWeight;
    for (ctr = pTblPtrs->NumMultiWeight; ctr > 0; ctr--, pMulti++)
    {
        if (pMulti->FirstSM == IDEOGRAPH)
        {
            //
            //  Part of multiple weight, so must move entire range
            //  by setting each value in range to NewScript and
            //  then incrementing NewScript.
            //
            //  NOTE:  May use 'ctr' here since it ALWAYS breaks
            //         out of outer for loop.
            //
            for (ctr = 1; ctr < pMulti->NumSM; ctr++)
            {
                pSMWeight[IDEOGRAPH + ctr] = NewScript;
                NewScript++;
            }
            break;
        }
    }

    //
    //  Must set each script member that has not yet been reset to its
    //  new order.
    //
    //  The default ordering is to assign:
    //       Order  =  Script Member Value
    //
    //  Therefore, can simply set each zero entry in order to the end
    //  of the array to the next 'NewScript' value.
    //
    for (ctr = FIRST_SCRIPT; ctr < NUM_SM; ctr++)
    {
        //
        //  If it's a zero value, set it to the next sorting order value.
        //
        if (pSMWeight[ctr] == 0)
        {
            pSMWeight[ctr] = NewScript;
            NewScript++;
        }
    }
}
