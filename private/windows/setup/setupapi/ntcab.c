/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    ntcab.c

Abstract:

    NTCab compression support.

Author:

    Ted Miller (tedm) 31-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
    

BOOL
NtCabNotifyFunction(
    IN PNTCAB_ENUM_DATA EnumData,
    IN PVOID            Cntxt    
    )
{
    PNTCABCONTEXT Context = Cntxt;
    BOOL rc;
    DWORD Operation;
    PSTR FileNameA;
    CABINET_INFO CabInfo;
    FILE_IN_CABINET_INFO FileInCab;
    FILETIME FileTime, UtcTime;
    TCHAR NewPath[MAX_PATH];
    PTSTR p;

    

    rc = ((PSP_NTCAB_CALLBACK)Context->MsgHandler)( EnumData, Context, &Operation );

    if (rc == ERROR_REQUEST_ABORTED) {
        //
        // this means stop making callback
        //
        return(FALSE);
    }
#if 0
    switch(Operation) {
       
        case FILEOP_SKIP:
            //
            // do nothing
            //
            ;
            break;

        case FILEOP_DOIT:
            ;
            break;

        default:
            //
            // Abort.
            //
            return(FALSE);

            break;
    }
#endif

    return(TRUE);   
    

}

#ifdef UNICODE

DWORD
NtCabProcessCabinet(
    //IN PVOID  InCabHandle, OPTIONAL
    IN PCTSTR CabinetFile,
    IN DWORD  Flags,
    IN PVOID  MsgHandler,
    IN PVOID  Context,
    IN BOOL   IsMsgHandlerNativeCharWidth
    )

/*++

Routine Description:

    Process an ntcab file, iterating through all files
    contained within it and calling the callback function with
    information about each file.

Arguments:

    CabHandle      - supplies a handle to the cab file, if it already exists,
                     otherwise, a new handle is created
    
    CabinetFile    - supplies name of cabinet file.

    Flags - supplies flags to control behavior of cabinet processing.

    MsgHandler - Supplies a callback routine to be notified
        of various significant events in cabinet processing.

    Context - Supplies a value that is passed to the MsgHandler
        callback function.

Return Value:

    Win32 error code indicating result. If the cabinet was corrupt,
    ERROR_INVALID_DATA is returned.

--*/

{
    BOOL b;
    DWORD rc;
    PWSTR CabCopy, FilePart,PathPart,tmp;
    WCHAR c;
    WCHAR fullcab[MAX_PATH];
    int h;
    PVOID CabHandle;

    NTCABCONTEXT CabContext;

    UNREFERENCED_PARAMETER(Flags);

    //
    // Initialize diamond for this thread if not
    // already initialized.
    //
    //if(!InCabHandle) {
        CabHandle = NtCabInitialize();
        if (!CabHandle) {
            rc = ERROR_INVALID_HANDLE;
            goto c0;
        }
    //} else {
    //    CabHandle = InCabHandle;
    //}

    if (!CabinetFile) {
        rc = ERROR_INVALID_PARAMETER;
        goto c1;
    }

    MYASSERT( CabHandle != NULL );
    MYASSERT( CabinetFile != NULL );
    
    //
    // make a copy because the input is const
    //
    CabCopy = DuplicateString(CabinetFile);
    if (!CabCopy) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c1;
    }

    //
    // Split the cabinet name into path and name.
    // Make separate copies because we want to remember the
    //
    if(FilePart = wcsrchr(CabCopy, L'\\')) {
        FilePart++;
    } else {
        FilePart = CabCopy;
    }
    c = *FilePart;
    *FilePart = 0;
    PathPart = DuplicateString(CabCopy);
    *FilePart = c;
    
    if(!PathPart) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c2;
    }
    FilePart = DuplicateString(FilePart);
    if(!FilePart) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c3;
    }

    MyFree( CabCopy );

    MYASSERT( FilePart != NULL && PathPart != NULL );

    rc = GetFullPathName(CabinetFile,MAX_PATH,fullcab,&tmp);
    if (!rc || rc > MAX_PATH) {
        rc = ERROR_BUFFER_OVERFLOW;
        goto c4;
    } else if (GetFileAttributes(fullcab) == 0xFFFFFFFF) {
        rc = ERROR_FILE_NOT_FOUND;
        goto c4;
    }

    if (!NtCabOpenCabFile(CabHandle,fullcab)) {
        rc = ERROR_INVALID_DATA;
        goto c4;
    }
        
    CabContext.hCab        = CabHandle;
    CabContext.UserContext = Context;
    CabContext.CabFile     = CabinetFile;
    CabContext.FilePart    = FilePart;
    CabContext.PathPart    = PathPart;
    CabContext.IsMsgHandlerNativeCharWidth = IsMsgHandlerNativeCharWidth;
    CabContext.MsgHandler  = MsgHandler;
    CabContext.LastError   = ERROR_SUCCESS;
    CabContext.CurrentTargetFile = NULL;
    
    //CabContext.UserPath[0]  = 0;
    //CabContext.SwitchedCabinets = FALSE ;
    
    
    //
    // call cab enumeration callback
    //
    b = NtCabEnumerateFiles(
            CabHandle,
            (PNTCABFILEENUM)NtCabNotifyFunction,
            (ULONG_PTR)&CabContext);
    if(b && GetLastError()==ERROR_NO_MORE_FILES) {

        //
        // Everything succeeded so we shouldn't have any partially
        // processed files.
        //
        SetLastError(NO_ERROR);
        MYASSERT(!CabContext.CurrentTargetFile);
        rc = NO_ERROR;

    } else {

        rc = CabContext.LastError;
#if 0
        switch(CabContext.LastError) {

        case :
            break;
        default:
            //
            // Cabinet is corrupt or not actually a cabinet, etc.
            //
            rc = ERROR_INVALID_DATA;
            break;
        }
#endif

        if(CabContext.CurrentTargetFile) {
            //
            // Call the callback function to inform it that the last file
            // was not successfully extracted from the cabinet.
            // Also remove the partially copied file.
            //
            DeleteFile(CabContext.CurrentTargetFile);
                        
            CabContext.CurrentTargetFile = NULL;
        }

    }

c4:
    MyFree(FilePart);
c3:
    MyFree(PathPart);
c2:
    MyFree(CabCopy);
c1:
    //if (CabHandle != InCabHandle) {
        NtCabClose( CabHandle );
    //}

c0:
    return(rc);
}

#else 

DWORD
NtCabProcessCabinet(
    //IN PVOID  InCabHandle, OPTIONAL
    IN PCTSTR CabinetFile,
    IN DWORD  Flags,
    IN PVOID  MsgHandler,
    IN PVOID  Context,
    IN BOOL   IsMsgHandlerNativeCharWidth
    )
{
    //UNREFERENCED_PARAMETER(InCabHandle);
    UNREFERENCED_PARAMETER(CabinetFile);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(MsgHandler);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(IsMsgHandlerNativeCharWidth);


    return(ERROR_CALL_NOT_IMPLEMENTED);
}

#endif

#ifdef UNICODE

BOOL
NtCabIsCabinet(
    IN PCWSTR CabinetFile
    )

/*++

Routine Description:

    Determine if a file is a diamond cabinet.

Arguments:

    FileName - supplies name of file to be checked.

Return Value:

    TRUE if file is diamond file. FALSE if not;

--*/

{
    DWORD rc;
    PVOID CabHandle;
    WCHAR fullcab[MAX_PATH];
    PWSTR tmp;

    CabHandle = NtCabInitialize();
    if (!CabHandle) {
        //
        // BugBug better error code
        //
        rc = ERROR_INVALID_DATA;
        goto c0;        
    }
    
    rc = GetFullPathName(CabinetFile,MAX_PATH,fullcab,&tmp);
    if (!rc || rc > MAX_PATH) {
        rc = ERROR_BUFFER_OVERFLOW;
        goto c1;
    } else if (GetFileAttributes(fullcab) == 0xFFFFFFFF) {
        rc = ERROR_FILE_NOT_FOUND;
        goto c1;
    }

    if (!NtCabOpenCabFile(CabHandle,fullcab)) {
        rc = ERROR_INVALID_DATA;
        goto c1;
    }

    rc = ERROR_SUCCESS;

c1:
    NtCabClose(CabHandle);

c0:
    return(rc == ERROR_SUCCESS);
    
}

#else 

BOOL
NtCabIsCabinet(
    IN PCWSTR FileName
    )
{
    UNREFERENCED_PARAMETER(FileName);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}


#endif


PVOID
NtCabAlloc(
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    Callback used by cab callback to allocate memory.

Arguments:

    NumberOfBytes - supplies desired size of block.

Return Value:

    Returns pointer to a block of memory or NULL
    if memory cannot be allocated.

--*/

{
    return(MyMalloc(NumberOfBytes));
}


VOID
NtCabFree(
    IN PVOID Block
    )

/*++

Routine Description:

    Callback used by cab callback to free a memory block.
    The block must have been allocated with NtCabAlloc().

Arguments:

    Block - supplies pointer to block of memory to be freed.

Return Value:

    None.

--*/

{
    MyFree(Block);
}
