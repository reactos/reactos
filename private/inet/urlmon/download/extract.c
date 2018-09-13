/*
 *  EXTRACT.C borrowed from TWEX\wextract.c
 *
 *  Has the CAB extraction capabilty for Code Downloader; uses FDI.LIB
 */

#include <urlmon.h>
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include "cdl.h"

#ifdef unix
#include "unixfile.h"
#endif /* unix */

//
// single theaded access to the FDI lib
static BOOL fCritCreated = FALSE;
CRITICAL_SECTION g_mxsFDI;

    
/*
 * W i n 3 2 O p e n ( )
 *
 * Routine:     Win32Open()
 *              
 * Purpose:     Translate a C-Runtime _open() call into appropriate Win32
 *              CreateFile()
 *
 * Returns:     Handle to file              on success
 *              INVALID_HANDLE_VALUE        on failure
 *
 *
 * BUGBUG: Doesn't fully implement C-Runtime _open() capability but it
 * BUGBUG: currently supports all callbacks that FDI will give us
 */

HANDLE
Win32Open(char *pszFile, int oflag, int pmode )
{
    HANDLE  FileHandle = INVALID_HANDLE_VALUE;
    BOOL    fExists     = FALSE;
    DWORD   fAccess;
    DWORD   fCreate; 


    ASSERT( pszFile );

        // BUGBUG: No Append Mode Support
    if (oflag & _O_APPEND)
        return( INVALID_HANDLE_VALUE );

        // Set Read-Write Access
    if ((oflag & _O_RDWR) || (oflag & _O_WRONLY))
        fAccess = GENERIC_WRITE;
    else
        fAccess = GENERIC_READ;

        // Set Create Flags
    if (oflag & _O_CREAT)  {
        if (oflag & _O_EXCL)
            fCreate = CREATE_NEW;
        else if (oflag & _O_TRUNC)
            fCreate = CREATE_ALWAYS;
        else 
            fCreate = OPEN_ALWAYS;
    } else {
        if (oflag & _O_TRUNC)  
            fCreate = TRUNCATE_EXISTING;
        else
            fCreate = OPEN_EXISTING;
    }

#ifdef unix
    UnixEnsureDir(pszFile);
#endif /* unix */

    //BUGBUG: seterrormode to no crit errors and then catch sharing violations
    // and access denied

    // Call Win32
    FileHandle = CreateFile(
                        pszFile, fAccess, FILE_SHARE_READ, NULL, fCreate,
                        FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE
                       );

    if (FileHandle == INVALID_HANDLE_VALUE && 
        SetFileAttributes(pszFile, FILE_ATTRIBUTE_NORMAL))
        FileHandle = CreateFile(
                            pszFile, fAccess, FILE_SHARE_READ, NULL, fCreate,
                            FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE
                           );
    return( FileHandle );
}



        

/*
 * O p e n F u n c ( )
 *
 * Routine:     OpenFunc()
 *
 * Purpose:     Open File Callback from FDI
 *
 * Returns:     File Handle (small integer index into file table)
 *              -1 on failure
 *
 */

int FAR DIAMONDAPI openfunc(char FAR *pszFile, int oflag, int pmode )
{
    int     rc;
    HANDLE hf;


    ASSERT( pszFile );

            //BUGBUG Spill File Support for Quantum?
    if ((*pszFile == '*') && (*(pszFile+1) != 'M'))  {
        DEBUGTRAP("Spill File Support for Quantum Not Supported");
    }


    hf = Win32Open(pszFile, oflag, pmode );
    if (hf != INVALID_HANDLE_VALUE)  {
        // SUNDOWN: typecast problem
        rc = PtrToLong(hf);
    } else {
        rc = -1;
    }

    return( rc );
}







/*
 * R E A D F U N C ( )
 *
 * Routine:     readfunc()
 *
 * Purpose:     FDI read() callback
 *
 */

UINT FAR DIAMONDAPI readfunc(int hf, void FAR *pv, UINT cb)
{
    int     rc;


    ASSERT( pv );
            
    if (! ReadFile((HANDLE)hf, pv, cb, (DWORD *) &cb, NULL))
        rc = -1;
    else
        rc = cb;

    return( rc );
}





/*
 *  W r i t e F u n c ( )
 *
 * Routine:     WriteFunc()
 *
 * Purpose:     FDI Write() callback
 *
 */

UINT FAR DIAMONDAPI
writefunc(int hf, void FAR *pv, UINT cb)
{
    int rc;

    ASSERT( pv );
    
    if (! WriteFile((HANDLE)hf, pv, cb, (DWORD *) &cb, NULL)) 
        rc = -1;
    else
        rc = cb;


    // BUGBUG: implement OnProgress notification

    return( rc );
}




/*
 * C l o s e F u n c ( )
 *
 * Routine:     CloseFunc()
 *
 * Purpose:     FDI Close File Callback
 *
 */

int FAR DIAMONDAPI closefunc( int hf )
{
    int rc;


    if (CloseHandle( (HANDLE)hf ))  {
        rc = 0;
    } else {
        rc = -1;
    }

    return( rc );
}





/*
 * S e e k F u n c ( )
 *
 * Routine:     seekfunc()
 *
 * Purpose:     FDI Seek Callback
 */
 
long FAR DIAMONDAPI seekfunc( int hf, long dist, int seektype )
{
    long    rc;
    DWORD   W32seektype;


        switch (seektype) {
            case SEEK_SET:
                W32seektype = FILE_BEGIN;
                break;
            case SEEK_CUR:
                W32seektype = FILE_CURRENT;
                break;
            case SEEK_END:
                W32seektype = FILE_END;
                break;
        }

        rc = SetFilePointer((HANDLE)hf, dist, NULL, W32seektype);
        if (rc == 0xffffffff)
            rc = -1;            

    return( rc );
}   



/*
 * A d j u s t F i l e T i m e ( )
 *
 * Routine:     AdjustFileTime()
 *
 * Purpose:     Change the time info for a file
 */

BOOL
AdjustFileTime(INT_PTR hf, USHORT date, USHORT time )
{
    FILETIME    ft;
    FILETIME    ftUTC;


    if (! DosDateTimeToFileTime( date, time, &ft ))
        return( FALSE );

    if (! LocalFileTimeToFileTime(&ft, &ftUTC))
        return( FALSE );

    if (! SetFileTime((HANDLE)hf,&ftUTC,&ftUTC,&ftUTC))
        return( FALSE );

    return( TRUE );
}



/*
 * A t t r 3 2 F r o m A t t r F A T ( )
 *
 * Translate FAT attributes to Win32 Attributes
 */
 
DWORD Attr32FromAttrFAT(WORD attrMSDOS)
{
    //** Quick out for normal file special case
    if (attrMSDOS == _A_NORMAL) {
        return FILE_ATTRIBUTE_NORMAL;
    }

    //** Otherwise, mask off read-only, hidden, system, and archive bits
    //   NOTE: These bits are in the same places in MS-DOS and Win32!
    //
    return attrMSDOS & ~(_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
}





/*
 * A l l o c F u n c ( )
 *
 * FDI Memory Allocation Callback
 */
        
FNALLOC(allocfunc)
{
    void *pv;

    pv = (void *) CoTaskMemAlloc( cb );
    DEBUGMSG("%d = ALLOC( %d )", (DWORD_PTR) pv, cb );
    return( pv );
}




/*
 * F r e e F u n c ( )
 *
 * FDI Memory Deallocation Callback
 *      XXX Return Value?
 */
 
FNFREE(freefunc)
{
    ASSERT(pv);

    DEBUGMSG("FREE( %d )", pv );
    CoTaskMemFree( pv );
}






/*
 * D O  G E T  N E X T  C A B ( )
 *
 * Routine:     doGetNextCab()
 *
 * Purpose:     Get Next Cabinet in chain
 *
 * Returns:     -1
 *
 * BUGBUG: CLEANUP: STUB THIS OUT
 * BUGBUG: STUBBED OUT IN WEXTRACT - CHAINED CABINETS NOT SUPPORTED
 */

FNFDINOTIFY(doGetNextCab)
{
    return( -1 );
}


/***    updateCabinetInfo - update history of cabinets seen
 *
 *  Entry:
 *      psess - Session
 *      pfdin - FDI info structurue
 *
 *  Exit:
 *      Returns 0;
 */

int updateCabinetInfo(PSESSION psess, PFDINOTIFICATION pfdin)
{

    ASSERT(psess);


    // Don't need any of this!

    //** Save cabinet info
    //lstrcpy(psess->acab.achCabPath     ,pfdin->psz3);
    //lstrcpy(psess->acab.achCabFilename ,pfdin->psz1);
    //lstrcpy(psess->acab.achDiskName    ,pfdin->psz2);
    //psess->acab.setID    = pfdin->setID;
    //psess->acab.iCabinet = pfdin->iCabinet;

    return 0;
}





/*
 * A P P E N D  P A T H  S E P A R A T O R ( )
 *
 * Routine: appendPathSeparator()
 *
 * Purpose: Append a path separator only if necessary
 *
 * Returns: TRUE -     Path Separator Added
 *          FALSE      No Path Separator added
 */

BOOL 
appendPathSeparator(char *pszPathEnd)
{
    //** Add path separator if necessary
    if ((*pszPathEnd != '\0')        && // Path is not empty
        (*pszPathEnd != chPATH_SEP1) && // Not a path separator
        (*pszPathEnd != chPATH_SEP2) && // Not a path separator
        (*pszPathEnd != chDRIVE_SEP) ) { // Not a drive separator
        #ifdef unix
        *(++pszPathEnd) = chPATH_SEP2; // Add Unix path separator
        #else
        *(++pszPathEnd) = chPATH_SEP1; // Add path separator
        #endif /* !unix */
        *(++pszPathEnd) = '\0';     // Terminate path
        return TRUE;                   // Account for path separator
    }
    //** No separator added
    return FALSE;
}




/*
 * C A T  D I R  A N D  F I L E ( )
 *
 * Routine: catDirAndFile()
 *
 * Purpose: Concatenate a directory with a filename!
 *
 */

BOOL 
catDirAndFile(  char    *pszResult, 
                int     cbResult, 
                char    *pszDir,
                char    *pszFile
             )
{
    int     cch = 0;


        //** Handle directory
    pszResult[0] = '\0';                // No filespec, yet

    if (pszDir)
        cch = lstrlen(pszDir);              // Get length of dir

    if (cch != 0) {                     // Have to concatenate path
        lstrcpy(pszResult,pszDir);      // Copy destination dir to buffer
        cbResult -= cch;                // Account for dir
           //** Add path separator if necessary, adjust remaining size
        cbResult -= appendPathSeparator(&(pszResult[cch-1]));
        if (cbResult <= 0) {
            return FALSE;
        }
    }

        //** Append file name, using default if primary one not supplied
    if (*pszFile == '\0') {
        return( FALSE );
    }
    
    lstrcat(pszResult,pszFile);              // Append file name
    cbResult -= lstrlen(pszFile);            // Update remaining size
    if (cbResult <= 0) {
        return FALSE;
    }

        //** Success
    return TRUE;
}

/*
 * IsExtracted
 *
 *  Look for pszName in psess->pFileList and see if extracted 
 *  
 *  Returns:
 *      Success: TRUE, failure: FALSE
 */

static
BOOL
IsExtracted( PSESSION ps, LPCSTR pszName)
{
    PFNAME CurName = ps->pFileList;

    ASSERT(pszName);
    ASSERT(CurName); // atleast one file needed

    // search for filename in list of files in this CAB
    do {
        if (lstrcmpi(pszName, CurName->pszFilename) == 0) {
            if (CurName->status == SFNAME_EXTRACTED)
                return TRUE;
            else
                return FALSE;
        }

    } while (CurName = CurName->pNextName);

    ASSERT(TRUE); // if here not found in list!

    return FALSE;
}


/*
 * NeedFile
 *
 *  search for pszName in psess->pFilesToExtract (list of PFNAMEs)
 *  Returns:
 *          TRUE -  need file, extract it
 *          FALSE - don't need file, skip it
 *
 */
static
BOOL
NeedFile( PSESSION ps, LPCSTR pszName)
{
    PFNAME CurName;

    ASSERT(pszName);

    if (IsExtracted(ps, pszName) )
        return FALSE;

    if ( ps->flags & SESSION_FLAG_EXTRACT_ALL ) 
        return TRUE;

    // search for filename in list of files needed
    for (CurName = ps->pFilesToExtract; CurName; CurName = CurName->pNextName){

        ASSERT(CurName->pszFilename);

        if (lstrcmpi(CurName->pszFilename, pszName) == 0)
            return TRUE;

    }

    return FALSE;
}

/*
 * MarkExtracted
 *
 *  Look for pszName in psess->pFileList and mark status = status_passed_in
 *      really can be use to mark status as anything else as well (not just
 *      extracted.)
 *  
 *  Returns:
 *      Success: TRUE, failure: FALSE
 */

static
BOOL
MarkExtracted( PSESSION ps, LPCSTR pszName , DWORD status)
{
    PFNAME CurName = ps->pFileList;

    ASSERT(pszName);
    ASSERT(CurName); // atleast one file needed

    // search for filename in list of files in this CAB
    do {
        if (lstrcmpi(pszName, CurName->pszFilename) == 0) {
            CurName->status = status;
            return TRUE;
        }

    } while (CurName = CurName->pNextName);

    ASSERT(TRUE); // if here not found in list!

    return FALSE;
}


/*
 * A d d F i l e ( )
 *
 * Add a file to the list of files we have in the CAB file
 *
 * Singly linked list - items added at front
 */

static
BOOL
AddFile( PSESSION ps, LPCSTR pszName , long cb)
{
    PFNAME NewName;

    ASSERT(pszName);

    if (!(ps->flags & SESSION_FLAG_ENUMERATE))
        return TRUE;

        // Allocate Node
    NewName = (PFNAME) CoTaskMemAlloc(sizeof(FNAME) );
    if (NewName == NULL)  {
        DEBUGMSG("AddFile(): Memory Allocation of structure failed");
        return( FALSE );
    }

        // Allocate String Space
    NewName->pszFilename = (LPSTR) CoTaskMemAlloc(lstrlen(pszName) + 1);
    if (NewName->pszFilename == NULL)  {
        DEBUGMSG("AddFile(): Memory Allocation of name failed");
        return( FALSE );
    }
    NewName->status = SFNAME_INIT;

        // Copy Filename
    lstrcpy( (char *)NewName->pszFilename, pszName );

        // Link into list
    NewName->pNextName = ps->pFileList;
    ps->pFileList = NewName;

    ps->cFiles++;

    ps->cbCabSize += cb;

    return( TRUE );
}


/* 
 * f d i N o t i f y  E x t r a c t()
 *
 * Routine:     fdiNotifyExtract()
 *
 * Purpose:     Principle FDI Callback in file extraction
 *              
 *
 */

FNFDINOTIFY(fdiNotifyExtract)
{
    int         fh;
    PSESSION    psess;


    psess = (PSESSION) pfdin->pv;


    switch (fdint)  {
        case fdintCABINET_INFO:
            return updateCabinetInfo(psess,pfdin);


        case fdintCOPY_FILE:
            // BUGBUG: implement OnProgress?

#ifdef unix
            UnixifyFileName(pfdin->psz1);
#endif /* unix */

            if (!catDirAndFile(psess->achFile, // Buffer for output filespec
                               sizeof(psess->achFile), // Size of output buffer
                               psess->achLocation,  // Output directory
                               pfdin->psz1)) {
                return -1;                  // Abort with error;
            }

            // always add the file (enumeration)
            if (! AddFile(psess, pfdin->psz1, pfdin->cb))
                return( -1 );

            // check if this is the file we are looking for if any
            if (!NeedFile(psess, pfdin->psz1))
                return( 0 );

            if (StrStrA(pfdin->psz1, "\\")) {
                // cab contains dir struct for this file
                // create struct on dest dir as well.

                char *pBaseFileName = NULL;
                char szDir[MAX_PATH];
                LPSTR pchStart;
                LPSTR pchSlash = NULL;

                lstrcpy(szDir, psess->achFile);
                pchStart = szDir + lstrlen(psess->achLocation) + 1;

                while (*pchStart &&  (pchSlash = StrStrA(pchStart, "\\"))) {

                    *pchSlash = '\0';

                    // don't care if this fails. may even already exist!
                    CreateDirectory(szDir, NULL);

                    *pchSlash = '\\';

                    pchStart = pchSlash +1;
                }


            }

            //** Do overwrite processing
            fh = openfunc( psess->achFile, _O_BINARY | _O_TRUNC | _O_RDWR |
                                                                _O_CREAT, 0 );

            return(fh); // -1 if error on open

        case fdintCLOSE_FILE_INFO:

            if (!catDirAndFile(psess->achFile, // Buffer for output filespec
                               sizeof(psess->achFile), // Size of output buffer
                               psess->achLocation,  // Output directory
                               pfdin->psz1))  {
                return -1;                  // Abort with error;
            }
            if (! AdjustFileTime( pfdin->hf, pfdin->date, pfdin->time ))  {
                return( -1 );
            }
            closefunc( (int) pfdin->hf );
            if (! SetFileAttributes(psess->achFile, Attr32FromAttrFAT(pfdin->attribs)))  
                return( -1 );

            MarkExtracted(psess, pfdin->psz1, SFNAME_EXTRACTED);
            return(TRUE);

        case fdintPARTIAL_FILE:
            return( 0 );


        case fdintNEXT_CABINET:
            return doGetNextCab(fdint,pfdin);

        default:
            DEBUGTRAP("fdiNotifyExtract(): Unknown Callback Type");
            break;
    }
    return( 0 );
}





#ifdef DEBUG
/*
 * V E R I F Y  C A B I N E T ( )
 *
 * Routine: VerifyCabinet()
 *
 * Purpose: Check that cabinet is properly formed
 *
 * Returns: TRUE -  Cabinet OK
 *          FALSE - Cabinet invalid
 */

BOOL
VerifyCabinet( PSESSION psess, LPCSTR lpCabName )
{
    HFDI            hfdi;
    ERF             erf;
    FDICABINETINFO  cabinfo;
    INT             fh;


    hfdi = FDICreate(allocfunc,freefunc,openfunc,readfunc,writefunc,closefunc,seekfunc,cpu80386,&erf);
    if (hfdi == NULL)  {
        DEBUGMSG("VerifyCabinet(): FDICreate() Failed");
            //BUGBUG Error Handling?
        return( FALSE );
    }

    fh = openfunc((char FAR *)lpCabName, _O_BINARY | _O_RDONLY, 0 );
    if (fh == -1)  {
        DEBUGMSG("VerifyCabinet(): Open of Memory File Failed");
        return( FALSE );
    }

    if (FDIIsCabinet(hfdi, fh, &cabinfo ) == FALSE)  {
        DEBUGMSG("VerifyCabinet(): FDIIsCabinet() Returned FALSE");
        return( FALSE );
    }
    
    if (cabinfo.cbCabinet != (long) psess->cbCabSize)  {
        DEBUGMSG("VerifyCabinet(): cabinfo.cbCabinet != cbCabSize");
        return( FALSE );
    }

    if (cabinfo.hasprev || cabinfo.hasnext)  {
        DEBUGMSG("VerifyCabinet(): Cabinet is chained");
        return( FALSE );
    }
    
    if (closefunc( fh ) == -1)   {
        DEBUGMSG("VerifyCabinet(): Closefunc() Failed");
        return( FALSE );
    }
        
    if (FDIDestroy(hfdi) == FALSE)  {
        DEBUGMSG("VerifyCabinet(): FDIDestroy() Failed");
        return( FALSE );
    }

    return( TRUE );
}
#endif /* DEBUG */



/*
 * E X T R A C T ( )
 *
 * Routine: Extract()
 *
 * Parameters:
 *
 *      PSESSION ps = session information tied to this extract session
 *
 *          IN params
 *              ps->pFilesToExtract = linked list of PFNAMEs that point to
 *                                    upper case filenames that need extraction
 *                          
 *              ps->flags SESSION_FLAG_ENUMERATE = whether need to enumerate
 *                                  files in CAB (ie. create a pFileList
 *              ps->flags SESSION_FLAG_EXTRACTALL =  all
 *
 *          OUT params
 *              ps->pFileList = global alloced list of files in CAB
 *                              caller needs to call DeleteExtractedFiles
 *                              to free memory and temp files
 *                  
 *
 *      LPCSTR lpCabName = name of cab file
 *
 *          
 * Returns:
 *          S_OK: sucesss
 *
 *
 */
 
HRESULT
Extract(PSESSION ps, LPCSTR lpCabName )
{
    HFDI        hfdi;
    BOOL        fExtractResult = FALSE;
    HRESULT hr = S_OK;


    if (ps->flags & SESSION_FLAG_EXTRACTED_ALL) {
        // already extracted all files in this CAB
        // nothing to do!
        return S_OK;
    }

    memset(&(ps->erf), 0, sizeof(ERF));

    if (ps->flags & SESSION_FLAG_ENUMERATE) {
        ps->cbCabSize = 0;
    }

    // don't enumerate when pFileList already pre-populated
    ASSERT( (!(ps->flags & SESSION_FLAG_ENUMERATE)) ||  (!ps->pFileList));

    {
        HRESULT hrOut = NOERROR;

        if (fCritCreated == FALSE)
        {
            fCritCreated = TRUE;
            InitializeCriticalSection(&g_mxsFDI);
        }
        EnterCriticalSection(&g_mxsFDI);
        
            // Extract the files
        hfdi = FDICreate(allocfunc,freefunc,openfunc,readfunc,writefunc,closefunc,seekfunc,cpu80386, &(ps->erf));
        if (hfdi == NULL)  {
            // Error value will be retrieved from ps->erf
            hrOut = STG_E_UNKNOWN;
            goto done;
        }

        fExtractResult = FDICopy(hfdi, (char FAR *)lpCabName, "", 0, fdiNotifyExtract, NULL, (void *) ps );
    
        if (FDIDestroy(hfdi) == FALSE)  {
            // Error value will be retrieved from ps->erf
            hrOut = STG_E_UNKNOWN;
        }
        
    done:
        LeaveCriticalSection(&g_mxsFDI);
        // leave now if this failed!
        if (hrOut != NOERROR)
        {
            return hrOut;
        }
    }


    if (fExtractResult && (!ps->erf.fError))
        return S_OK;

    hr = HRESULT_FROM_WIN32(GetLastError());

    if (SUCCEEDED(hr)) {
        // not a win32 failure but a cabinet failure

        // convert CABINET failure to disk full or STG_E_UNKNOWN.
        // On win95 writefile failing with disk full is not 
        // setting the last error correctly

        if (ps->erf.fError && (ps->erf.erfOper == FDIERROR_TARGET_FILE))
            hr = HRESULT_FROM_WIN32(ERROR_DISK_FULL);
        else
            hr = STG_E_UNKNOWN;
    }

    return hr;
}   



/*
 * D E L E T E  E X T R A C T E D  F I L E S ( )
 *
 * Routine: DeleteExtractedFiles()
 *
 * Purpose: Delete the files that were extracted
 *          into the temporary directory
 *          FREE all the memory in pFileList
 *          make pFileList = NULL.
 *
 * Paramaters:
 *      psess - Pointer to Session Structure containing
 *              all state about this extraction session
 *
 * Returns: None
 */

VOID
DeleteExtractedFiles(PSESSION psess)
{
    PFNAME      rover = psess->pFileList;
    PFNAME      roverprev;
    char szBuf[MAX_PATH];

    ASSERT(psess);
    DEBUGMSG("Deleting Extracted Files");


    while (rover != NULL)  {

        // skip if this is not a tmp file
        if ( rover->status == SFNAME_EXTRACTED) {

            // Get full filename
            if (catDirAndFile(szBuf, MAX_PATH, psess->achLocation,
                               rover->pszFilename)) {

                if (SetFileAttributes(szBuf, FILE_ATTRIBUTE_NORMAL))
                    DeleteFile(szBuf);
            }
        }

        CoTaskMemFree(rover->pszFilename);

        roverprev = rover;  // save for free'ing current rover below
        rover = rover->pNextName;

        CoTaskMemFree(roverprev);

    }

    psess->pFileList = NULL; // prevent use after deletion!
}
