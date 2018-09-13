#include <windows.h>
#include <lzexpand.h>
#include <fcntl.h>

/************************************************************************\
*
* NOTE!!!!
*
* While the 'Diamond' interfaced functions defined in this file are
* multi thread safe, EACH THREAD MUST ONLY HAVE ONE DIAMOND FILE
* OPEN AT A TIME!  (ie. You can not nest InitDiamond()/TermDiamond()
* pairs in one thread of execution.)
*
\************************************************************************/

//
// diamond headers
//
#include <diamondd.h>
#include "mydiam.h"

HINSTANCE hCabinet;
DWORD cCabinetLoad;
typedef HFDI (DIAMONDAPI * tFDICreate) (PFNALLOC pfnalloc,
                                        PFNFREE  pfnfree,
                                        PFNOPEN  pfnopen,
                                        PFNREAD  pfnread,
                                        PFNWRITE pfnwrite,
                                        PFNCLOSE pfnclose,
                                        PFNSEEK  pfnseek,
                                        int      cpuType,
                                        PERF     perf);

typedef BOOL (DIAMONDAPI * tFDIIsCabinet)(HFDI            hfdi,
                                          INT_PTR         hf,
                                          PFDICABINETINFO pfdici);

typedef BOOL (DIAMONDAPI * tFDICopy)(HFDI          hfdi,
                                     char FAR     *pszCabinet,
                                     char FAR     *pszCabPath,
                                     int           flags,
                                     PFNFDINOTIFY  pfnfdin,
                                     PFNFDIDECRYPT pfnfdid,
                                     void FAR     *pvUser);

typedef BOOL (DIAMONDAPI * tFDIDestroy)(HFDI hfdi);

tFDICreate  pFDICreate;
tFDIIsCabinet pFDIIsCabinet;
tFDICopy      pFDICopy;
tFDIDestroy   pFDIDestroy;



INT CopyDateTimeStamp(INT_PTR doshFrom, INT_PTR doshTo)
{
    FILETIME lpCreationTime, lpLastAccessTime, lpLastWriteTime;

    if (!GetFileTime((HANDLE) doshFrom, &lpCreationTime, &lpLastAccessTime,
                     &lpLastWriteTime)) {
        return ((INT)LZERROR_BADINHANDLE);
    }
    if (!SetFileTime((HANDLE) doshTo, &lpCreationTime, &lpLastAccessTime,
                     &lpLastWriteTime)) {
        return ((INT)LZERROR_BADINHANDLE);
    }

    return (TRUE);
}

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

typedef struct _DIAMOND_INFO {

    //
    // A read handle to the source file.
    //
    INT_PTR SourceFileHandle;

    //
    // File names.
    //
    PSTR SourceFileName;
    PSTR TargetFileName;

    //
    // Flag indicating whether to rename the target file.
    //
    BOOL RenameTargetFile;

    //
    // Pointer to LZ information structure.
    // We'll fill in some of the fields to fool expand.
    //
    PLZINFO pLZI;

} DIAMOND_INFO, *PDIAMOND_INFO;


PSTR
StringRevChar(
             IN PSTR String,
             IN CHAR Char
             )
{
    //
    // Although not the most efficient possible algoeithm in each case,
    // this algorithm is correct for unicode, sbcs, or dbcs.
    //
    PCHAR Occurrence,Next;

    //
    // Check each character in the string and remember
    // the most recently encountered occurrence of the desired char.
    //
    for (Occurrence=NULL,Next=CharNextA(String); *String; ) {

        if (!memcmp(String,&Char,(int)((PUCHAR)Next-(PUCHAR)String))) {
            Occurrence = String;
        }

        String = Next;
        Next = CharNextA(Next);
    }

    //
    // Return address of final occurrence of the character
    // (will be NULL if not found at all).
    //
    return (Occurrence);
}


INT_PTR
DIAMONDAPI
DiamondNotifyFunction(
                     IN FDINOTIFICATIONTYPE Operation,
                     IN PFDINOTIFICATION    Parameters
                     )
{
    switch (Operation) {

        case fdintCABINET_INFO:
        case fdintNEXT_CABINET:
        case fdintPARTIAL_FILE:

            //
            // Cabinet management functions which we don't use.
            // Return success.
            //
            return (0);

        case fdintCOPY_FILE:

            //
            // Diamond is asking us whether we want to copy the file.
            //
            {
                PDIAMOND_INFO Info = (PDIAMOND_INFO)Parameters->pv;
                HFILE h;

                //
                // If we need to rename the target file, do that here.
                // The name stored in the cabinet file will be used as
                // the uncompressed name.
                //
                if (Info->RenameTargetFile) {

                    PSTR p,q;

                    //
                    // Find the start of the filename part of the target.
                    //
                    if (p = StringRevChar(Info->TargetFileName,'\\')) {
                        p++;
                    } else {
                        p = Info->TargetFileName;
                    }

                    //
                    // Find the start of the filename part of the name in the cabinet.
                    //
                    if (q = StringRevChar(Parameters->psz1,'\\')) {
                        q++;
                    } else {
                        q = Parameters->psz1;
                    }

                    //
                    // Copy the filename part of the name in the cabinet over
                    // the filename part of the name in the target spec.
                    //
                    lstrcpyA(p,q);
                }

                {
                    // Check they're not the same file

                    CHAR Source[MAX_PATH];
                    CHAR Target[MAX_PATH];
                    PSTR FileName;
                    DWORD PathLenSource;
                    DWORD PathLenTarget;

                    PathLenSource = GetFullPathNameA(Info->SourceFileName,
                                                     MAX_PATH,
                                                     Source,
                                                     &FileName);
                    PathLenTarget = GetFullPathNameA(Info->TargetFileName,
                                                     MAX_PATH,
                                                     Target,
                                                     &FileName);

                    if (PathLenSource == 0 || PathLenSource >= MAX_PATH ||
                        PathLenTarget == 0 || PathLenTarget >= MAX_PATH ||
                        lstrcmpiA(Source, Target) == 0) {
                        return 0;
                    }
                }

                //
                // Remember the uncompressed size and open the file.
                // Returns -1 if an error occurs opening the file.
                //
                Info->pLZI->cblOutSize = Parameters->cb;
                h = _lcreat(Info->TargetFileName,0);
                if (h == HFILE_ERROR) {
                    DiamondLastIoError = LZERROR_BADOUTHANDLE;
                    return (-1);
                }
                return (h);
            }

        case fdintCLOSE_FILE_INFO:

            //
            // Diamond is done with the target file and wants us to close it.
            // (ie, this is the counterpart to fdint_COPY_FILE).
            //
            {
                PDIAMOND_INFO Info = (PDIAMOND_INFO)Parameters->pv;

                CopyDateTimeStamp(Info->SourceFileHandle,Parameters->hf);
                _lclose((HFILE)Parameters->hf);
            }
            return (TRUE);

         default:

            //
            // invalid operation
            //
            return(-1);
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
    return ((PVOID)LocalAlloc(LMEM_FIXED,NumberOfBytes));
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
    LocalFree((HLOCAL)Block);
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

Arguments:

    FileName - supplies name of file to be opened.

    oflag - supplies flags for open.

    pmode - supplies additional flags for open.

Return Value:

    Handle to open file or -1 if error occurs.

--*/

{
    HFILE h;
    int OpenMode;

    if (oflag & _O_WRONLY) {
        OpenMode = OF_WRITE;
    } else {
        if (oflag & _O_RDWR) {
            OpenMode = OF_READWRITE;
        } else {
            OpenMode = OF_READ;
        }
    }

    h = _lopen(FileName,OpenMode | OF_SHARE_DENY_WRITE);

    if (h == HFILE_ERROR) {
        DiamondLastIoError = LZERROR_BADINHANDLE;
        return (-1);
    }

    return ((INT_PTR)h);
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

    rc = _lread((HFILE)Handle,pv,ByteCount);

    if (rc == HFILE_ERROR) {
        rc = (UINT)(-1);
        DiamondLastIoError = LZERROR_READ;
    }

    return (rc);
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

    rc = _lwrite((HFILE)Handle,pv,ByteCount);

    if (rc == HFILE_ERROR) {

        DiamondLastIoError = (GetLastError() == ERROR_DISK_FULL) ? LZERROR_WRITE : LZERROR_BADOUTHANDLE;

    } else {

        if (rc != ByteCount) {
            //
            // let caller interpret return value but record last error just in case
            //
            DiamondLastIoError = LZERROR_WRITE;
        }
    }

    return (rc);
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
    _lclose((HFILE)Handle);
    return (0);
}


LONG
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

    rc = _llseek((HFILE)Handle,Distance,SeekType);

    if (rc == HFILE_ERROR) {
        DiamondLastIoError = LZERROR_BADINHANDLE;
        rc = -1L;
    }

    return (rc);
}


INT
ExpandDiamondFile(
                 IN  PSTR       SourceFileName,      // Note ASCII
                 IN  PTSTR      TargetFileNameT,
                 IN  BOOL       RenameTarget,
                 OUT PLZINFO    pLZI
                 )
{
    BOOL b;
    INT rc;
    INT_PTR h;
    DIAMOND_INFO DiamondInfo;
    CHAR TargetFileName[MAX_PATH];

#ifdef UNICODE
    wsprintfA(TargetFileName, "%ls", TargetFileNameT);
#else
    lstrcpy(TargetFileName, TargetFileNameT);
#endif

    if (!FdiContext) {
        return (LZERROR_BADVALUE);
    }

    DiamondLastIoError = TRUE;

    //
    // Get a handle to the source to use to
    // copy the date and time stamp.
    //
    h = SpdFdiOpen(SourceFileName,_O_RDONLY,0);
    if (h == -1) {
        return (LZERROR_BADINHANDLE);
    }

    pLZI->cblInSize = GetFileSize((HANDLE)h,NULL);
    if (pLZI->cblInSize == -1) {
        SpdFdiClose(h);
        return (LZERROR_BADINHANDLE);
    }

    DiamondInfo.SourceFileHandle = h;
    DiamondInfo.SourceFileName = SourceFileName;
    DiamondInfo.TargetFileName = TargetFileName;
    DiamondInfo.RenameTargetFile = RenameTarget;
    DiamondInfo.pLZI = pLZI;

    b = pFDICopy(
                FdiContext,
                SourceFileName,             // pass the whole path as the name
                "",                         // don't bother with the path part
                0,                          // flags
                DiamondNotifyFunction,
                NULL,                       // no decryption
                &DiamondInfo
                );

    if (b) {

        rc = TRUE;

    } else {

        switch (FdiError.erfOper) {

            case FDIERROR_CORRUPT_CABINET:
            case FDIERROR_UNKNOWN_CABINET_VERSION:
            case FDIERROR_BAD_COMPR_TYPE:
                rc = LZERROR_READ;              // causes SID_FORMAT_ERROR message
                break;

            case FDIERROR_ALLOC_FAIL:
                rc = LZERROR_GLOBALLOC;
                break;

            case FDIERROR_TARGET_FILE:
            case FDIERROR_USER_ABORT:
                rc = DiamondLastIoError;
                break;

            default:
                //
                // The rest of the errors are not handled specially.
                //
                rc = LZERROR_BADVALUE;
                break;
        }

        //
        // Remove the partial target file.
        //
        DeleteFileA(TargetFileName);
    }

    SpdFdiClose(h);

    return (rc);
}


BOOL
IsDiamondFile(
             IN PSTR FileName
             )
{
    FDICABINETINFO CabinetInfo;
    BOOL b;
    INT_PTR h;

    if (!FdiContext) {
        return (FALSE);
    }

    //
    // Open the file such that the handle is valid for use
    // in the diamond context (ie, seek, read routines above).
    //
    h = SpdFdiOpen(FileName,_O_RDONLY,0);
    if (h == -1) {
        return (FALSE);
    }

    b = pFDIIsCabinet(FdiContext,h,&CabinetInfo);

    SpdFdiClose(h);

    return (b);
}


DWORD
InitDiamond(
           VOID
           )
{
    PDIAMOND_CONTEXT pdcx;

    if (!GotDmdTlsSlot())
        return VIF_OUTOFMEMORY;

    if (GotDmdContext())
        return VIF_OUTOFMEMORY;

    pdcx = LocalAlloc(LPTR, sizeof(DIAMOND_CONTEXT));

    if (pdcx == NULL || !TlsSetValue(itlsDiamondContext, pdcx)) {
        /*
         * For some unknown reason, we can't associate
         * our thread storage with the slot, so free
         * it and say we never got one.
         */

        if (pdcx) {
            LocalFree(pdcx);
        }
        return VIF_OUTOFMEMORY;
    }

    if (!cCabinetLoad) {
        hCabinet = LoadLibraryW(L"CABINET.DLL");
        if (!hCabinet) {
            return (VIF_CANNOTLOADCABINET);
        }
        pFDICreate    = (tFDICreate)    GetProcAddress(hCabinet, "FDICreate");
        pFDIDestroy   = (tFDIDestroy)   GetProcAddress(hCabinet, "FDIDestroy");
        pFDIIsCabinet = (tFDIIsCabinet) GetProcAddress(hCabinet, "FDIIsCabinet");
        pFDICopy      = (tFDICopy)      GetProcAddress(hCabinet, "FDICopy");

        if (!(pFDICreate && pFDIDestroy && pFDIIsCabinet && pFDICopy)) {
            FreeLibrary(hCabinet);
            return (VIF_CANNOTLOADCABINET);
        }

        if (InterlockedExchangeAdd(&cCabinetLoad, 1) != 0) {
            // Multiple threads are attempting to LoadLib
            // Free one here.
            FreeLibrary(hCabinet);
        }
    }

    SetFdiContext( pFDICreate(
                             SpdFdiAlloc,
                             SpdFdiFree,
                             SpdFdiOpen,
                             SpdFdiRead,
                             SpdFdiWrite,
                             SpdFdiClose,
                             SpdFdiSeek,
                             cpuUNKNOWN,
                             &FdiError
                             ));

    return ((FdiContext == NULL) ? VIF_CANNOTLOADCABINET : 0);
}


VOID
TermDiamond(
           VOID
           )
{
    if (!GotDmdTlsSlot() || !GotDmdContext())
        return;

    if (FdiContext) {
        pFDIDestroy(FdiContext);
        SetFdiContext( NULL );
    }

    LocalFree( TlsGetValue(itlsDiamondContext) );
    TlsSetValue(itlsDiamondContext, NULL);
}
