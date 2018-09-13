/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1994, Microsoft Corporation
 *
 *  WOLE2.C
 *  WOW32 Support for OLE2 stuff
 *
 *  History:
 *  Created 03-May-1994 by Bob Day (bobday)
--*/

#include "precomp.h"
#pragma hdrstop

MODNAME(wole.c);

/*
** Under OLE 2.0, the IMessageFilter interface passes HTASKs/THREADIDs.  It
** passes HTASKs in the 16-bit world, and THREADIDs in the 32-bit world. The
** OLE 2.0 16 <-> 32 interoperability DLLs need a way of converting the
** HTASK into an appropriate THREADID and back.
**
** Really the only place the 16-bit code uses the HTASK is in ole2's BUSY.C
** module, wherein they take the HTASK and use TOOLHELP's TaskFindHandle
** to determine a HINST.  Then they take the HINST and try and find its
** module name, and a top-level window handle.  Using this, they bring up
** a nice dialog describing the task.
**
** In the case when a 32-bit process's THREADID needs to be given into the
** 16-bit world as an htask, we create an htask alias (a GDT selector).
** We check for it in TaskFindHandle and return an HINST of exactly the
** same value (same GDT selector).  We also check for this value in
** GetModuleFileName. Then, later, we make sure that any window operated on
** with GetWindowWord( GWW_HINST, ...) maps to exactly the same value if it
** is from a 32-bit process AND from the process which we created an alias
** for.
**
** I've tried to make these routines general, so that later we might be able
** to really maintain HTASK aliases whenever we see a 32-bit THREADID, but
** it is too late before shipping to be able to test a general fix.
**
** -BobDay
**
*/

#define MAP_SLOT_HTASK(slot)    ((HTASK16)((WORD)0xffe0 - (8 * (slot))))
#define MAP_HTASK_SLOT(htask)   ((UINT)(((WORD)0xffe0 - (htask16))/8))

typedef struct tagHTASKALIAS {
    DWORD       dwThreadID32;
    DWORD       dwProcessID32;
    union {
        FILETIME    ftCreationTime;
        ULONGLONG   ullCreationTime;
    };
} HTASKALIAS;

#define MAX_HTASKALIAS_SIZE  32     // 32 should be plenty

HTASKALIAS *lphtaskalias = NULL;
UINT cHtaskAliasCount = 0;

BOOL GetThreadIDHTASKALIAS(
    DWORD  dwThreadID32,
    HTASKALIAS *ha
) {
    OBJECT_ATTRIBUTES   obja;
    THREAD_BASIC_INFORMATION ThreadInfo;
    HANDLE      hThread;
    NTSTATUS    Status;
    FILETIME    ftDummy;
    CLIENT_ID   cid;

    InitializeObjectAttributes(
            &obja,
            NULL,
            0,
            NULL,
            0 );

    cid.UniqueProcess = 0;      // Don't know it, 0 means any process
    cid.UniqueThread  = (HANDLE)dwThreadID32;

    Status = NtOpenThread(
                &hThread,
                THREAD_QUERY_INFORMATION,
                &obja,
                &cid );

    if ( !NT_SUCCESS(Status) ) {
#if DBG
        DbgPrint("WOW32: Could not get open thread handle\n");
#endif
        return( FALSE );
    }

    Status = NtQueryInformationThread(
        hThread,
        ThreadBasicInformation,
        (PVOID)&ThreadInfo,
        sizeof(THREAD_BASIC_INFORMATION),
        NULL
        );

    ha->dwProcessID32 = (DWORD)ThreadInfo.ClientId.UniqueProcess;
    ha->dwThreadID32  = dwThreadID32;

    GetThreadTimes( hThread,
        &ha->ftCreationTime,
        &ftDummy,
        &ftDummy,
        &ftDummy );

    Status = NtClose( hThread );
    if ( !NT_SUCCESS(Status) ) {
#if DBG
        DbgPrint("WOW32: Could not close thread handle\n");
        DbgBreakPoint();
#endif
        return( FALSE );
    }
    return( TRUE );
}

HTASK16 AddHtaskAlias(
    DWORD   ThreadID32
) {
    UINT        iSlot;
    UINT        iUsable;
    HTASKALIAS  ha;
    ULONGLONG   ullOldest;

    if ( !GetThreadIDHTASKALIAS( ThreadID32, &ha ) ) {
        return( 0 );
    }

    //
    // Need to allocate the alias table?
    //
    if ( lphtaskalias == NULL ) {
        lphtaskalias = (HTASKALIAS *) malloc_w( MAX_HTASKALIAS_SIZE * sizeof(HTASKALIAS) );
        if ( lphtaskalias == NULL ) {
            LOGDEBUG(LOG_ALWAYS,("WOW::AddHtaskAlias : Failed to allocate memory\n"));
            WOW32ASSERT(FALSE);
            return( 0 );
        }
        // Zero them out initially
        memset( lphtaskalias, 0, MAX_HTASKALIAS_SIZE * sizeof(HTASKALIAS) );
    }

    //
    // Now iterate through the alias table, either finding an available slot,
    // or finding the oldest one there to overwrite.
    //
    iSlot = 0;
    iUsable = 0;
    ullOldest = -1;

    while ( iSlot < MAX_HTASKALIAS_SIZE ) {

        //
        // Did we find an available slot?
        //
        if ( lphtaskalias[iSlot].dwThreadID32 == 0 ) {
            cHtaskAliasCount++;     // Using an available slot
            iUsable = iSlot;
            break;
        }

        //
        // Remember the oldest guy
        //
        if ( lphtaskalias[iSlot].ullCreationTime < ullOldest  ) {
            ullOldest = lphtaskalias[iSlot].ullCreationTime;
            iUsable = iSlot;
        }

        iSlot++;
    }

    //
    // If the above loop is exitted due to not enough space, then
    // iUsable will be the oldest one.  If it was exitted because we found
    // an empty slot, then iUsable will be the slot.
    //

    lphtaskalias[iUsable] = ha;

    return( MAP_SLOT_HTASK(iUsable) );
}

HTASK16 FindHtaskAlias(
    DWORD   ThreadID32
) {
    UINT    iSlot;

    if ( lphtaskalias == NULL || ThreadID32 == 0 ) {
        return( 0 );
    }

    iSlot = MAX_HTASKALIAS_SIZE;

    while ( iSlot > 0 ) {
        --iSlot;

        //
        // Did we find the appropriate guy?
        //
        if ( lphtaskalias[iSlot].dwThreadID32 == ThreadID32 ) {

            return( MAP_SLOT_HTASK(iSlot) );
        }
    }
    return( 0 );
}

void RemoveHtaskAlias(
    HTASK16 htask16
) {
    UINT    iSlot;

    //
    // Get out early if we haven't any aliases
    //
    if ( lphtaskalias == NULL || (!htask16)) {
        return;
    }
    iSlot = MAP_HTASK_SLOT(htask16);

    if (iSlot >= MAX_HTASKALIAS_SIZE) {
        LOGDEBUG(LOG_ALWAYS, ("WOW::RemoveHtaskAlias : iSlot >= MAX_TASK_ALIAS_SIZE\n"));
        WOW32ASSERT(FALSE);
        return;
    }

    //
    // Zap the entry from the list
    //

    if (lphtaskalias[iSlot].dwThreadID32) {

        lphtaskalias[iSlot].dwThreadID32 = 0;
        lphtaskalias[iSlot].dwProcessID32 = 0;
        lphtaskalias[iSlot].ullCreationTime = 0;

        --cHtaskAliasCount;
    }
}

DWORD GetHtaskAlias(
    HTASK16 htask16,
    LPDWORD lpProcessID32
) {
    UINT        iSlot;
    DWORD       ThreadID32;
    HTASKALIAS  ha;

    ha.dwProcessID32 = 0;
    ThreadID32 = 0;

    if ( ! ISTASKALIAS(htask16) ) {
        goto Done;
    }

    iSlot = MAP_HTASK_SLOT(htask16);

    if ( iSlot >= MAX_HTASKALIAS_SIZE ) {
        WOW32ASSERTMSGF(FALSE, ("WOW::GetHtaskAlias : iSlot >= MAX_TASK_ALIAS_SIZE\n"));
        goto Done;
    }

    ThreadID32 = lphtaskalias[iSlot].dwThreadID32;

    //
    // Make sure the thread still exists in the system
    //

    if ( ! GetThreadIDHTASKALIAS( ThreadID32, &ha ) ||
         ha.ullCreationTime != lphtaskalias[iSlot].ullCreationTime ||
         ha.dwProcessID32   != lphtaskalias[iSlot].dwProcessID32 ) {

        RemoveHtaskAlias( htask16 );
        ha.dwProcessID32 = 0;
        ThreadID32 = 0;
    }

    if ( lpProcessID32 ) {
        *lpProcessID32 = ha.dwProcessID32;
    }

Done:
    return ThreadID32;
}

UINT GetHtaskAliasProcessName(
    HTASK16 htask16,
    LPSTR   lpNameBuffer,
    UINT    cNameBufferSize
) {
    DWORD   dwThreadID32;
    DWORD   dwProcessID32;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PUCHAR  pucLargeBuffer;
    ULONG   LargeBufferSize = 32*1024;
    NTSTATUS status = STATUS_INFO_LENGTH_MISMATCH;
    ULONG TotalOffset;

    dwThreadID32 = GetHtaskAlias(htask16, &dwProcessID32);

    if (  dwThreadID32 == 0 || 
          cNameBufferSize == 0 || 
          lpNameBuffer == NULL ) {

        return 0;
    }

    while(status == STATUS_INFO_LENGTH_MISMATCH) {

        pucLargeBuffer = VirtualAlloc(NULL, 
                                      LargeBufferSize, 
                                      MEM_COMMIT, 
                                      PAGE_READWRITE);

        if (pucLargeBuffer == NULL) {
            WOW32ASSERTMSGF((FALSE),
                            ("WOW::GetHtaskAliasProcessName: VirtualAlloc(%x) failed %x.\n",
                            LargeBufferSize));
            return 0;
        }
    
        status = NtQuerySystemInformation(SystemProcessInformation,
                                          pucLargeBuffer,
                                          LargeBufferSize,
                                          &TotalOffset);

        if (NT_SUCCESS(status)) {
            break;
        }
        else if (status == STATUS_INFO_LENGTH_MISMATCH) {
            LargeBufferSize += 8192;
            VirtualFree (pucLargeBuffer, 0, MEM_RELEASE);
            pucLargeBuffer = NULL;
        }
        else {

            WOW32ASSERTMSGF((NT_SUCCESS(status)),
                            ("WOW::GetHtaskAliasProcessName: NtQuerySystemInformation failed %x.\n",
                            status));

            if(pucLargeBuffer) {
                VirtualFree (pucLargeBuffer, 0, MEM_RELEASE);
            }
            return 0;
        }
    }

    //
    // Iterate through the returned list of process information structures,
    // trying to find the one with the right process id.
    //
    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pucLargeBuffer;

    while (TRUE) {
        if ( (DWORD)ProcessInfo->UniqueProcessId == dwProcessID32 ) {

            //
            // Found it, return the name.
            //

            if ( ProcessInfo->ImageName.Buffer ) {

                cNameBufferSize = 
                    WideCharToMultiByte(
                        CP_ACP,
                        0,
                        ProcessInfo->ImageName.Buffer,    // src
                        ProcessInfo->ImageName.Length,
                        lpNameBuffer,                     // dest
                        cNameBufferSize,
                        NULL,
                        NULL
                        );

                lpNameBuffer[cNameBufferSize] = '\0';

                return cNameBufferSize;

            } else {

                //
                // Don't let them get the name of a system process
                //

                return 0;
            }
        }
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pucLargeBuffer[TotalOffset];
    }
    return 0;
}

