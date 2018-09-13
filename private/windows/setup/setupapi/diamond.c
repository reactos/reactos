/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    diamond.c

Abstract:

    Diamond MSZIP decompression support.

Author:

    Ted Miller (tedm) 31-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Tls index used by Diamond. This is set up in
// DiamondProcessAttach().
//
DWORD DiamondTlsIndex = (DWORD)(-1);

//
// Per-thread globals.
//
typedef struct _DIAMOND_THREAD_DATA {

    //
    // Boolean value indicating whether the current thread
    // is inside diamond. Diamond doesn't really providee
    // a full context environment so we declare it non-reentrant.
    //
    BOOL InDiamond;

    //
    // Diamond context data
    //
    HFDI FdiContext;
    ERF FdiError;

    //
    // Last encountered error
    //
    DWORD LastError;

    //
    // Name of cabinet as passed to DiamondProcessCabinet,
    // and an ANSI version.
    //
    PCTSTR CabinetFile;
    PCSTR CabinetFileA;

    //
    // Notification callback and context parameter
    //
    PVOID MsgHandler;
    PVOID Context;
    BOOL IsMsgHandlerNativeCharWidth;

    //
    // Full path of the current target file being extracted.
    //
    PTSTR CurrentTargetFile;

    //
    // Flag indicating whether diamond asked us to switch cabinets.
    // If we do switch, then we stop copying when the current file
    // is done. This prevents diamond from happily doing each file
    // in the new cabinet, which would ruin the queue commit routine's
    // ability to allow some files to exist outside the cabinet, etc.
    //
    BOOL SwitchedCabinets;

    //
    // If the source path changes as the result of a prompt for a
    // new cabinet (when a file continues across multiple cabinets),
    // we remember the path the user gave us here.
    //
    TCHAR UserPath[MAX_PATH];

} DIAMOND_THREAD_DATA, *PDIAMOND_THREAD_DATA;



BOOL
DiamondInitialize(
    VOID
    );

INT_PTR
DIAMONDAPI
SpdFdiOpen(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    );

int
DIAMONDAPI
SpdFdiClose(
    IN INT_PTR Handle
    );


UINT
pDiamondNotifyFileDone(
    IN PDIAMOND_THREAD_DATA PerThread,
    IN DWORD                Win32Error
    )
{
    UINT u;
    FILEPATHS FilePaths;

    MYASSERT(PerThread->CurrentTargetFile);

    FilePaths.Source = PerThread->CabinetFile;
    FilePaths.Target = PerThread->CurrentTargetFile;
    FilePaths.Win32Error = Win32Error;

    u = pSetupCallMsgHandler(
            PerThread->MsgHandler,
            PerThread->IsMsgHandlerNativeCharWidth,
            PerThread->Context,
            SPFILENOTIFY_FILEEXTRACTED,
            (UINT_PTR)&FilePaths,
            0
            );

    return(u);
}


INT_PTR
DIAMONDAPI
DiamondNotifyFunction(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    )
{
    PDIAMOND_THREAD_DATA PerThread;
    int h;
    PSTR FileNameA;
    CABINET_INFO CabInfo;
    FILE_IN_CABINET_INFO FileInCab;
    FILETIME FileTime, UtcTime;
    TCHAR NewPath[MAX_PATH];
    PTSTR p;

    PerThread = TlsGetValue(DiamondTlsIndex);
    
    MYASSERT(PerThread);

    switch(Operation) {

    case fdintCABINET_INFO:
        //
        // Tell the callback function, in case it wants to do something
        // with this information.
        //
        h = ERROR_NOT_ENOUGH_MEMORY;

        CabInfo.CabinetFile = NewPortableString(Parameters->psz1);
        if(CabInfo.CabinetFile) {

            CabInfo.DiskName = NewPortableString(Parameters->psz2);
            if(CabInfo.DiskName) {

                CabInfo.CabinetPath = NewPortableString(Parameters->psz3);
                if(CabInfo.CabinetPath) {

                    CabInfo.SetId = Parameters->setID;
                    CabInfo.CabinetNumber = Parameters->iCabinet;

                    h = pSetupCallMsgHandler(
                            PerThread->MsgHandler,
                            PerThread->IsMsgHandlerNativeCharWidth,
                            PerThread->Context,
                            SPFILENOTIFY_CABINETINFO,
                            (UINT_PTR)&CabInfo,
                            0
                            );

                    MyFree(CabInfo.CabinetPath);
                }
                MyFree(CabInfo.DiskName);
            }
            MyFree(CabInfo.CabinetFile);
        }

        if(h != NO_ERROR) {
            PerThread->LastError = h;
        }
        return((h == NO_ERROR) ? 0 : -1);

    case fdintCOPY_FILE:
        //
        // Diamond is asking us whether we want to copy the file.
        // If we switched cabinets, then the answer is no.
        //
        if(PerThread->SwitchedCabinets) {
            PerThread->LastError = NO_ERROR;
            return(-1);
        }

        // Pass the information on to the callback function and
        // let it decide.
        //
        FileInCab.NameInCabinet = NewPortableString(Parameters->psz1);
        FileInCab.FileSize = Parameters->cb;
        FileInCab.DosDate = Parameters->date;
        FileInCab.DosTime = Parameters->time;
        FileInCab.DosAttribs = Parameters->attribs;
        FileInCab.Win32Error = NO_ERROR;

        if(!FileInCab.NameInCabinet) {
            PerThread->LastError = ERROR_NOT_ENOUGH_MEMORY;
            return(-1);
        }

        //
        // Call the callback function.
        //
        h = pSetupCallMsgHandler(
                PerThread->MsgHandler,
                PerThread->IsMsgHandlerNativeCharWidth,
                PerThread->Context,
                SPFILENOTIFY_FILEINCABINET,
                (UINT_PTR)&FileInCab,
                (UINT_PTR)PerThread->CabinetFile
                );

        MyFree (FileInCab.NameInCabinet);

        switch(h) {

        case FILEOP_SKIP:
            h = 0;
            break;

        case FILEOP_DOIT:
            //
            // The callback wants to copy the file. In this case it has
            // provided us the full target pathname to use.
            //
            MYASSERT(PerThread->CurrentTargetFile == NULL);

            if(p = DuplicateString(FileInCab.FullTargetName)) {

                if(FileNameA = NewAnsiString(FileInCab.FullTargetName)) {

                    h = _lcreat(FileNameA,0);
                    if(h == HFILE_ERROR) {
                        PerThread->LastError = GetLastError();
                        h = -1;
                    }

                    MyFree(FileNameA);
                    //
                    // If we get here, a file is being processed.
                    //
                    if(h == -1) {
                        MyFree(p);
                    } else {
                        PerThread->CurrentTargetFile = p;
                    }
                } else {
                    PerThread->LastError = ERROR_NOT_ENOUGH_MEMORY;
                    MyFree(p);
                    h = -1;
                }
            } else {

                PerThread->LastError = ERROR_NOT_ENOUGH_MEMORY;
                h = -1;
            }

            break;

        default:
            //
            // Abort.
            //
            h = -1;
            PerThread->LastError = FileInCab.Win32Error;
            break;
        }

        return(h);

    case fdintCLOSE_FILE_INFO:
        //
        // Diamond is done with the target file and wants us to close it.
        // (ie, this is the counterpart to fdintCOPY_FILE).
        //
        // We want the timestamp to be what is stored in the cabinet.
        // Note that we lose the create and last access times in this case.
        //
        if(DosDateTimeToFileTime(Parameters->date,Parameters->time,&FileTime) &&
            LocalFileTimeToFileTime(&FileTime, &UtcTime)) {

            SetFileTime((HANDLE)Parameters->hf,NULL,NULL,&UtcTime);
        }

        SpdFdiClose(Parameters->hf);

        //
        // Call the callback function to inform it that the file has been
        // successfully extracted from the cabinet.
        //
        MYASSERT(PerThread->CurrentTargetFile);

        h = pDiamondNotifyFileDone(PerThread,NO_ERROR);

        if(h != NO_ERROR) {
            PerThread->LastError = h;
        }

        MyFree(PerThread->CurrentTargetFile);
        PerThread->CurrentTargetFile = NULL;

        return((h == NO_ERROR) ? TRUE : -1);

    case fdintPARTIAL_FILE:
    case fdintENUMERATE:

        //
        // We don't do anything with this.
        //
        return(0);

    case fdintNEXT_CABINET:

        if((Parameters->fdie == FDIERROR_NONE) || (Parameters->fdie == FDIERROR_WRONG_CABINET)) {
            //
            // A file continues into another cabinet.
            // Inform the callback function, who is responsible for
            // making sure the cabinet is accessible when it returns.
            //
            h = ERROR_NOT_ENOUGH_MEMORY;
            CabInfo.SetId = 0;
            CabInfo.CabinetNumber = 0;

            CabInfo.CabinetPath = NewPortableString(Parameters->psz3);
            if(CabInfo.CabinetPath) {

                CabInfo.CabinetFile = NewPortableString(Parameters->psz1);
                if(CabInfo.CabinetFile) {

                    CabInfo.DiskName = NewPortableString(Parameters->psz2);
                    if(CabInfo.DiskName) {

                        h = pSetupCallMsgHandler(
                                PerThread->MsgHandler,
                                PerThread->IsMsgHandlerNativeCharWidth,
                                PerThread->Context,
                                SPFILENOTIFY_NEEDNEWCABINET,
                                (UINT_PTR)&CabInfo,
                                (UINT_PTR)NewPath
                                );

                        if(h == NO_ERROR) {
                            //
                            // See if a new path was specified.
                            //
                            if(NewPath[0]) {
                                lstrcpyn(PerThread->UserPath,NewPath,MAX_PATH);
                            }

                            //
                            // Remember that we switched cabinets.
                            //
                            PerThread->SwitchedCabinets = TRUE;
                        }

                        MyFree(CabInfo.DiskName);
                    }

                    MyFree(CabInfo.CabinetFile);
                }

                MyFree(CabInfo.CabinetPath);
            }

        } else {
            //
            // Some other error we don't understand -- this indicates
            // a bad cabinet.
            //
            h = ERROR_INVALID_DATA;
        }

        if(h != NO_ERROR) {
            PerThread->LastError = h;
        }

        return((h == NO_ERROR) ? 0 : -1);

    default:
        //
        // Unknown notification type. Should never get here.
        //
        MYASSERT(0);
        return(0);
    }
}


PVOID
DIAMONDAPI
SpdFdiAlloc(
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    Callback used by FDICopy to allocate memory.

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
DIAMONDAPI
SpdFdiFree(
    IN PVOID Block
    )

/*++

Routine Description:

    Callback used by FDICopy to free a memory block.
    The block must have been allocated with SpdFdiAlloc().

Arguments:

    Block - supplies pointer to block of memory to be freed.

Return Value:

    None.

--*/

{
    MyFree(Block);
}


INT_PTR
DIAMONDAPI
SpdFdiOpen(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    )

/*++

Routine Description:

    Callback used by FDICopy to open files.

    This routine is capable only of opening existing files.

Arguments:

    FileName - supplies name of file to be opened.

    oflag - supplies flags for open.

    pmode - supplies additional flags for open.

Return Value:

    Handle to open file or -1 if error occurs.

--*/

{
    HFILE h;
    PDIAMOND_THREAD_DATA PerThread;

    UNREFERENCED_PARAMETER(pmode);

    PerThread = TlsGetValue(DiamondTlsIndex);
    
    MYASSERT(PerThread);

    if(oflag & (_O_WRONLY | _O_RDWR | _O_APPEND | _O_CREAT | _O_TRUNC | _O_EXCL)) {
        PerThread->LastError = ERROR_INVALID_PARAMETER;
        return(-1);
    }

    h = _lopen(FileName,OF_READ | OF_SHARE_DENY_WRITE);

    if(h == HFILE_ERROR) {

        PerThread->LastError = GetLastError();
        return(-1);
    }

    return((INT_PTR)h);
}


UINT
DIAMONDAPI
SpdFdiRead(
    IN  INT_PTR Handle,
    OUT PVOID pv,
    IN  UINT  ByteCount
    )

/*++

Routine Description:

    Callback used by FDICopy to read from a file.

Arguments:

    Handle - supplies handle to open file to be read from.

    pv - supplies pointer to buffer to receive bytes we read.

    ByteCount - supplies number of bytes to read.

Return Value:

    Number of bytes read (ByteCount) or -1 if an error occurs.

--*/

{
    UINT rc;
    PDIAMOND_THREAD_DATA PerThread;
    DWORD d;

    rc = _lread((HFILE)Handle,pv,ByteCount);

    if(rc == HFILE_ERROR) {

        rc = (UINT)(-1);
        d = GetLastError();
    
        PerThread = TlsGetValue(DiamondTlsIndex);

        MYASSERT(PerThread);
        PerThread->LastError = d;
    }

    return(rc);
}


UINT
DIAMONDAPI
SpdFdiWrite(
    IN INT_PTR Handle,
    IN PVOID pv,
    IN UINT  ByteCount
    )

/*++

Routine Description:

    Callback used by FDICopy to write to a file.

Arguments:

    Handle - supplies handle to open file to be written to.

    pv - supplies pointer to buffer containing bytes to write.

    ByteCount - supplies number of bytes to write.

Return Value:

    Number of bytes written (ByteCount) or -1 if an error occurs.

--*/

{
    UINT rc;
    PDIAMOND_THREAD_DATA PerThread;
    DWORD d;

    rc = _lwrite((HFILE)Handle,pv,ByteCount);

    if(rc == HFILE_ERROR) {

        rc = (UINT)(-1);
        d = GetLastError();

        PerThread = TlsGetValue(DiamondTlsIndex);
        
        MYASSERT(PerThread);
        PerThread->LastError = d;
    }

    return(rc);
}


int
DIAMONDAPI
SpdFdiClose(
    IN INT_PTR Handle
    )

/*++

Routine Description:

    Callback used by FDICopy to close files.

Arguments:

    Handle - handle of file to close.

Return Value:

    0 (success).

--*/

{
    HFILE h;

    //
    // andrewr temp workaround: diamond is giving us an invalid file handle 
    // actually it gives us the same file handle twice). add this try..except 
    // so that we handle invalid handle exception and keep on going.
    // 
    //
    try {
        h = _lclose((HFILE)Handle);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        h = HFILE_ERROR;
    }

    MYASSERT(h == 0);

    //
    // Always act like we succeeded.
    //
    return 0;
}


long
DIAMONDAPI
SpdFdiSeek(
    IN INT_PTR Handle,
    IN long Distance,
    IN int  SeekType
    )

/*++

Routine Description:

    Callback used by FDICopy to seek files.

Arguments:

    Handle - handle of file to close.

    Distance - supplies distance to seek. Interpretation of this
        parameter depends on the value of SeekType.

    SeekType - supplies a value indicating how Distance is to be
        interpreted; one of SEEK_SET, SEEK_CUR, SEEK_END.

Return Value:

    New file offset or -1 if an error occurs.

--*/

{
    LONG rc;
    DWORD d;
    PDIAMOND_THREAD_DATA PerThread;

    rc = _llseek((HFILE)Handle,Distance,SeekType);

    if(rc == HFILE_ERROR) {

        d = GetLastError();

        PerThread = TlsGetValue(DiamondTlsIndex);
        
        MYASSERT(PerThread);
        PerThread->LastError = d;

        rc = -1L;
    }

    return(rc);
}


DWORD
DiamondProcessCabinet(
    IN PCTSTR CabinetFile,
    IN DWORD  Flags,
    IN PVOID  MsgHandler,
    IN PVOID  Context,
    IN BOOL   IsMsgHandlerNativeCharWidth
    )

/*++

Routine Description:

    Process a diamond cabinet file, iterating through all files
    contained within it and calling the callback function with
    information about each file.

Arguments:

    SourceFileName - supplies name of cabinet file.

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
    PDIAMOND_THREAD_DATA PerThread;
    PSTR CabinetFileA;
    PSTR p,FilePartA,PathPartA;
    CHAR c;
    int h;

    UNREFERENCED_PARAMETER(Flags);

    //
    // Initialize diamond for this thread if not
    // already initialized.
    //
    if(!DiamondInitialize()) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c0;
    }

    //
    // Fetch pointer to per-thread data.
    //
    PerThread = TlsGetValue(DiamondTlsIndex);
    
    MYASSERT(PerThread);
    MYASSERT(PerThread->FdiContext);

    //
    // Because diamond does not really give us a truly comprehensive
    // context mechanism, our diamond support is NOT reentrant.
    // No synchronization is required to check this state because
    // it is stored in per-thread data.
    //
    if(PerThread->InDiamond) {
        rc = ERROR_INVALID_FUNCTION;
        goto c0;
    }

    PerThread->InDiamond = TRUE;

    //
    // Get an ANSI version of the name because diamond is not unicode.
    //
    CabinetFileA = NewAnsiString(CabinetFile);
    if(!CabinetFileA) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c1;
    }

    //
    // Split the cabinet name into path and name.
    // Make separate copies because we want to remember the
    // full ansi version of the fill cabinet filename we were passed,
    // so we'll leave CabinetFileA alone.
    //
    if(FilePartA = _mbsrchr(CabinetFileA,'\\')) {
        FilePartA++;
    } else {
        FilePartA = CabinetFileA;
    }
    c = *FilePartA;
    *FilePartA = 0;
    PathPartA = _strdup(CabinetFileA);
    *FilePartA = c;
    if(!PathPartA) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c2;
    }
    FilePartA = _strdup(FilePartA);
    if(!FilePartA) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c3;
    }

    //
    // Initialize thread globals.
    //
    PerThread->LastError = NO_ERROR;
    PerThread->CabinetFile = CabinetFile;
    PerThread->CabinetFileA = CabinetFileA;

    PerThread->MsgHandler = MsgHandler;
    PerThread->IsMsgHandlerNativeCharWidth = IsMsgHandlerNativeCharWidth;
    PerThread->Context = Context;

    PerThread->SwitchedCabinets = FALSE;
    PerThread->UserPath[0] = 0;

    PerThread->CurrentTargetFile = NULL;

    //
    // Perform the copy.
    //
    b = FDICopy(
            PerThread->FdiContext,
            FilePartA,
            PathPartA,
            0,                          // flags
            DiamondNotifyFunction,
            NULL,                       // no decryption
            NULL                        // don't bother with user-specified data
            );

    if(b) {

        //
        // Everything succeeded so we shouldn't have any partially
        // processed files.
        //
        MYASSERT(!PerThread->CurrentTargetFile);
        rc = NO_ERROR;

    } else {

        switch(PerThread->FdiError.erfOper) {

        case FDIERROR_NONE:
            //
            // We shouldn't see this -- if there was no error
            // then FDICopy should have returned TRUE.
            //
            MYASSERT(PerThread->FdiError.erfOper != FDIERROR_NONE);
            rc = ERROR_INVALID_DATA;
            break;

        case FDIERROR_CABINET_NOT_FOUND:
            rc = ERROR_FILE_NOT_FOUND;
            break;

        case FDIERROR_CORRUPT_CABINET:
            //
            // Read/open/seek error or corrupt cabinet
            //
            rc = PerThread->LastError;
            if(rc == NO_ERROR) {
                rc = ERROR_INVALID_DATA;
            }
            break;

        case FDIERROR_ALLOC_FAIL:
            rc = ERROR_NOT_ENOUGH_MEMORY;
            break;

        case FDIERROR_TARGET_FILE:
        case FDIERROR_USER_ABORT:
            rc = PerThread->LastError;
            break;

        case FDIERROR_NOT_A_CABINET:
        case FDIERROR_UNKNOWN_CABINET_VERSION:
        case FDIERROR_BAD_COMPR_TYPE:
        case FDIERROR_MDI_FAIL:
        case FDIERROR_RESERVE_MISMATCH:
        case FDIERROR_WRONG_CABINET:
        default:
            //
            // Cabinet is corrupt or not actually a cabinet, etc.
            //
            rc = ERROR_INVALID_DATA;
            break;
        }

        if(PerThread->CurrentTargetFile) {
            //
            // Call the callback function to inform it that the last file
            // was not successfully extracted from the cabinet.
            // Also remove the partially copied file.
            //
            DeleteFile(PerThread->CurrentTargetFile);

            pDiamondNotifyFileDone(PerThread,rc);
            MyFree(PerThread->CurrentTargetFile);
            PerThread->CurrentTargetFile = NULL;
        }

    }

    free(FilePartA);
c3:
    free(PathPartA);
c2:
    MyFree(CabinetFileA);
c1:
    PerThread->InDiamond = FALSE;
c0:
    return(rc);
}


BOOL
DiamondIsCabinet(
    IN PCTSTR FileName
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
    FDICABINETINFO CabinetInfo;
    BOOL b;
    INT_PTR h;
    PDIAMOND_THREAD_DATA PerThread;
    PSTR FileNameA;

    b = FALSE;

    //
    // Initialize diamond for this thread if not
    // already initialized.
    //
    if(!DiamondInitialize()) {
        MYASSERT( FALSE && TEXT("DiamondInitialize failed") );
        goto c0;
    }

    if (!FileExists(FileName,NULL)) {
        return FALSE;
    }
    

    PerThread = TlsGetValue(DiamondTlsIndex);
    
    MYASSERT(PerThread);
    MYASSERT(PerThread->FdiContext);

    //
    // Because diamond does not really give us a truly comprehensive
    // context mechanism, our diamond support is NOT reentrant.
    // No synchronization is required to check this state because
    // it is stored in per-thread data.
    //
    if(PerThread->InDiamond) {
        MYASSERT( FALSE && TEXT("PerThread->InDiamond failed") );
        goto c0;
    }

    PerThread->InDiamond = TRUE;

    //
    // Unfortunately the diamond engine is not Unicode.
    // Convert the filename to ANSI.
    //
    FileNameA = NewAnsiString(FileName);
    if(!FileNameA) {
        MYASSERT( FALSE && TEXT("NewAnsiString failed") );
        goto c1;
    }

    //
    // Open the file such that the handle is valid for use
    // in the diamond context (ie, seek, read routines above).
    //
    h = SpdFdiOpen(FileNameA,_O_RDONLY,0);
    if(h == -1) {        
        goto c2;
    }

    SpdFdiSeek(h , 0, SEEK_SET);
    b = FDIIsCabinet(PerThread->FdiContext,h,&CabinetInfo);
    if (!b) {
        PSTR p;
        p = strrchr(FileNameA, '.');
        while (p && *p) {
            if (*p == '_') {
                MYASSERT(FALSE && TEXT("FDIIsCabinetFailed for a file ending in _"));
                SpdFdiSeek(h , 0, SEEK_SET);
                FDIIsCabinet(PerThread->FdiContext,h,&CabinetInfo);
            }
            p++;
        }        
    }

    SpdFdiClose(h);

c2:
    MyFree(FileNameA);
c1:
    PerThread->InDiamond = FALSE;
c0:
    return(b);
}



BOOL
DiamondInitialize(
    VOID
    )

/*++

Routine Description:

    Per-thread initialization routine for Diamond.
    Called internally.

Arguments:

    None.

Return Value:

    Boolean result indicating success or failure.
    Failure can be assumed to be out of memory.

--*/

{
    HFDI FdiContext;
    PDIAMOND_THREAD_DATA PerThread;
    BOOL retval = FALSE;
    
    //
    // See if this thread is already initialized.
    //
    
    PerThread = TlsGetValue(DiamondTlsIndex);
    
    if (PerThread) {
        return(TRUE);
    }
    
    retval = FALSE;
    try {
        
        //
        // Allocate a per-thread data structure
        //
        if(PerThread = MyMalloc(sizeof(DIAMOND_THREAD_DATA))) {
    
            ZeroMemory(PerThread,sizeof(DIAMOND_THREAD_DATA));
    
            //
            // Initialize a diamond context.
            //
            FdiContext = FDICreate(
                            SpdFdiAlloc,
                            SpdFdiFree,
                            SpdFdiOpen,
                            SpdFdiRead,
                            SpdFdiWrite,
                            SpdFdiClose,
                            SpdFdiSeek,
                            cpuUNKNOWN,
                            &PerThread->FdiError
                            );
    
            if(FdiContext) {
    
                if(TlsSetValue(DiamondTlsIndex,PerThread)) {
                    //
                    // success here...but return outside the body of the 
                    // try block for performance reasons
                    //
                    PerThread->FdiContext = FdiContext;
                    retval = TRUE;
                    
                } else {
    
                    FDIDestroy(FdiContext);
                    
                    MyFree(PerThread);

                }
            } else {
            
                MyFree(PerThread);

            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        retval = FALSE;
    }    

    return(retval);
}


VOID
DiamondTerminate(
    VOID
    )

/*++

Routine Description:

    Per-thread termination routine for Diamond.
    Called internally.

Arguments:

    None.

Return Value:

    Boolean result indicating success or failure.
    Failure can be assumed to be out of memory.

--*/

{
    PDIAMOND_THREAD_DATA PerThread;

    if(PerThread = TlsGetValue(DiamondTlsIndex)) {

        FDIDestroy(PerThread->FdiContext);

        MyFree(PerThread);

        TlsSetValue(DiamondTlsIndex,NULL);
            
    }

}


BOOL
DiamondProcessAttach(
    IN BOOL Attach
    )

/*++

Routine Description:

    Process attach routine. Must be called by the DLL entry point routine
    on DLL_PROCESS_ATTACH and DLL_PROCESS_DETACH notifications.

    This routine allocates a Tls index on process attach for
    thread-specific data used by Diamond. The Tls index is freed on
    process detach.

Arguments:

    Attach - TRUE if process is attaching; FALSE if not.

Return Value:

    Boolean result indicating success or failure. Meaningful only if
    Attach is TRUE.

--*/

{
    if(Attach) {

        //
        // Allocate a Tls index.
        //
        DiamondTlsIndex = TlsAlloc();
        return(DiamondTlsIndex != (DWORD)(-1));

    } else {

        //
        // Free the Tls index
        //
        return(TlsFree(DiamondTlsIndex));
    }
}


VOID
DiamondThreadAttach(
    IN BOOL Attach
    )

/*++

Routine Description:

    Thread attach routine. Must be called by the DLL entry point routine
    on DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications.

    The routine initializes per-thread data used by diamond.

Arguments:

    Attach - TRUE if thread is attaching; FALSE if not.

Return Value:

    None.

--*/

{
    PVOID p;

    if(Attach) {
        //
        // There really isn't any reason to do the initialization here
        // because it will get done on entry to externally exposed API.
        // (We have to do that, in case someone uses LoadLibrary and then
        // calls Diamond from a thread that existed before the LoadLibrary.)
        //
        //DiamondInitialize();
    } else {
        DiamondTerminate();
    }
}


///////////////////////////////////////////////////////////////////////////

#if 0
HFCI
DiamondCreateCabinet(
    IN PCTSTR FileName
    )
{
    HFCI hfci;

    //
    // Initialize diamond for this thread if not
    // already initialized.
    //
    if(!DiamondInitialize()) {
        goto c0;
    }

    PerThread = TlsGetValue(DiamondTlsIndex);
    
    MYASSERT(PerThread);

    hfci = FCICreate(
            &PerThread->FdiError,
            SpdFciFilePlaced,
            SpdFdiAlloc,
            SpdFdiFree,
            SpdFciGetTempFile,
            &Cabinfo
            );

    return(hfci);
}
#endif

///////////////////////////////////////////////////////////////////////////


BOOL
_SetupIterateCabinet(
    IN PCTSTR CabinetFile,
    IN DWORD  Flags,
    IN PVOID  MsgHandler,
    IN PVOID  Context,
    IN BOOL   IsMsgHandlerNativeCharWidth
    )
{
    PTSTR cabinetFile;
    DWORD rc;

    //
    // Flags param not used. Make sure it's zero.
    //
    if(Flags) {
        rc = ERROR_INVALID_PARAMETER;
        goto c0;
    }

    //
    // Get a copy of the cabinet file name to validate
    // the caller's buffer.
    //
    try {
        cabinetFile = DuplicateString(CabinetFile);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
        goto c0;
    }

    if(!cabinetFile) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto c0;
    }

    rc = DiamondProcessCabinet(cabinetFile,Flags,MsgHandler,Context,IsMsgHandlerNativeCharWidth);

    MyFree(cabinetFile);

c0:
    SetLastError(rc);
    return(rc == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupIterateCabinetA(
    IN  PCSTR               CabinetFile,
    IN  DWORD               Flags,
    IN  PSP_FILE_CALLBACK_A MsgHandler,
    IN  PVOID               Context
    )
{
    BOOL b;
    DWORD rc;
    PCWSTR cabinetFile;

    rc = CaptureAndConvertAnsiArg(CabinetFile,&cabinetFile);
    if(rc == NO_ERROR) {

        b = _SetupIterateCabinet(cabinetFile,Flags,MsgHandler,Context,FALSE);
        rc = GetLastError();

        MyFree(cabinetFile);

    } else {
        b = FALSE;
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupIterateCabinetW(
    IN  PCWSTR              CabinetFile,
    IN  DWORD               Flags,
    IN  PSP_FILE_CALLBACK_W MsgHandler,
    IN  PVOID               Context
    )
{
    UNREFERENCED_PARAMETER(CabinetFile);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(MsgHandler);
    UNREFERENCED_PARAMETER(Context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupIterateCabinet(
    IN  PCTSTR            CabinetFile,
    IN  DWORD             Flags,
    IN  PSP_FILE_CALLBACK MsgHandler,
    IN  PVOID             Context
    )
{
    return(_SetupIterateCabinet(CabinetFile,Flags,MsgHandler,Context,TRUE));
}
