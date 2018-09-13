#include <windows.h>
#include <fcntl.h>

//
// lz headers
//
#include "..\..\libs\common.h"
#include "..\..\libs\buffers.h"
#include "..\..\libs\header.h"

//
// diamond headers
//
#include <diamondd.h>
#include "mydiam.h"


HFDI FdiContext;
ERF FdiError;

INT DiamondLastIoError;

BOOL
PatternMatch(
    IN PCSTR pszString,
    IN PCSTR pszPattern,
    IN BOOL fImplyDotAtEnd
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

    //
    // Expand callback/notification.
    //
    NOTIFYPROC ExpandNotify;

    //
    // Selective extraction file spec, ie, "aic*.sys" or NULL
    //
    PSTR SelectiveFilesSpec;

} DIAMOND_INFO, *PDIAMOND_INFO;


PTSTR
StringRevChar(
    IN PTSTR String,
    IN TCHAR Char
    )
{
    //
    // Although not the most efficient possible algoeithm in each case,
    // this algorithm is correct for unicode, sbcs, or dbcs.
    //
    PTCHAR Occurrence,Next;

    //
    // Check each character in the string and remember
    // the most recently encountered occurrence of the desired char.
    //
    for(Occurrence=NULL,Next=CharNext(String); *String; ) {

        if(!memcmp(String,&Char,(int)((PUCHAR)Next-(PUCHAR)String))) {
            Occurrence = String;
        }

        String = Next;
        Next = CharNext(Next);
    }

    //
    // Return address of final occurrence of the character
    // (will be NULL if not found at all).
    //
    return(Occurrence);
}


#define WILDCARD    '*'     /* zero or more of any character */
#define WILDCHAR    '?'     /* one of any character (does not match END) */
#define END         '\0'    /* terminal character */
#define DOT         '.'     /* may be implied at end ("hosts" matches "*.") */


static int __inline Lower(c)
{
    if ((c >= 'A') && (c <= 'Z'))
    {
        return(c + ('a' - 'A'));
    }
    else
    {
        return(c);
    }
}


static int __inline CharacterMatch(char chCharacter, char chPattern)
{
    if (Lower(chCharacter) == Lower(chPattern))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


BOOL
PatternMatch(
    PCSTR pszString,
    PCSTR pszPattern,
    IN BOOL fImplyDotAtEnd
    )
{
    /* RECURSIVE */

    //
    //  This function does not deal with 8.3 conventions which might
    //  be expected for filename comparisons.  (In an 8.3 environment,
    //  "alongfilename.html" would match "alongfil.htm")
    //
    //  This code is NOT MBCS-enabled
    //

    for ( ; ; )
    {
        switch (*pszPattern)
        {

        case END:

            //
            //  Reached end of pattern, so we're done.  Matched if
            //  end of string, no match if more string remains.
            //

            return(*pszString == END);

        case WILDCHAR:

            //
            //  Next in pattern is a wild character, which matches
            //  anything except end of string.  If we reach the end
            //  of the string, the implied DOT would also match.
            //

            if (*pszString == END)
            {
                if (fImplyDotAtEnd == TRUE)
                {
                    fImplyDotAtEnd = FALSE;
                }
                else
                {
                    return(FALSE);
                }
            }
            else
            {
                pszString++;
            }

            pszPattern++;

            break;

        case WILDCARD:

            //
            //  Next in pattern is a wildcard, which matches anything.
            //  Find the required character that follows the wildcard,
            //  and search the string for it.  At each occurence of the
            //  required character, try to match the remaining pattern.
            //
            //  There are numerous equivalent patterns in which multiple
            //  WILDCARD and WILDCHAR are adjacent.  We deal with these
            //  before our search for the required character.
            //
            //  Each WILDCHAR burns one non-END from the string.  An END
            //  means we have a match.  Additional WILDCARDs are ignored.
            //

            for ( ; ; )
            {
                pszPattern++;

                if (*pszPattern == END)
                {
                    return(TRUE);
                }
                else if (*pszPattern == WILDCHAR)
                {
                    if (*pszString == END)
                    {
                        if (fImplyDotAtEnd == TRUE)
                        {
                            fImplyDotAtEnd = FALSE;
                        }
                        else
                        {
                            return(FALSE);
                        }
                    }
                    else
                    {
                        pszString++;
                    }
                }
                else if (*pszPattern != WILDCARD)
                {
                    break;
                }
            }

            //
            //  Now we have a regular character to search the string for.
            //

            while (*pszString != END)
            {
                //
                //  For each match, use recursion to see if the remainder
                //  of the pattern accepts the remainder of the string.
                //  If it does not, continue looking for other matches.
                //

                if (CharacterMatch(*pszString, *pszPattern) == TRUE)
                {
                    if (PatternMatch(pszString + 1, pszPattern + 1, fImplyDotAtEnd) == TRUE)
                    {
                        return(TRUE);
                    }
                }

                pszString++;
            }

            //
            //  Reached end of string without finding required character
            //  which followed the WILDCARD.  If the required character
            //  is a DOT, consider matching the implied DOT.
            //
            //  Since the remaining string is empty, the only pattern which
            //  could match after the DOT would be zero or more WILDCARDs,
            //  so don't bother with recursion.
            //

            if ((*pszPattern == DOT) && (fImplyDotAtEnd == TRUE))
            {
                pszPattern++;

                while (*pszPattern != END)
                {
                    if (*pszPattern != WILDCARD)
                    {
                        return(FALSE);
                    }

                    pszPattern++;
                }

                return(TRUE);
            }

            //
            //  Reached end of the string without finding required character.
            //

            return(FALSE);
            break;

        default:

            //
            //  Nothing special about the pattern character, so it
            //  must match source character.
            //

            if (CharacterMatch(*pszString, *pszPattern) == FALSE)
            {
                if ((*pszPattern == DOT) &&
                    (*pszString == END) &&
                    (fImplyDotAtEnd == TRUE))
                {
                    fImplyDotAtEnd = FALSE;
                }
                else
                {
                    return(FALSE);
                }
            }

            if (*pszString != END)
            {
                pszString++;
            }

            pszPattern++;
        }
    }
}


INT_PTR
DIAMONDAPI
DiamondNotifyFunction(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    )
{
    switch(Operation) {

    case fdintCABINET_INFO:
    case fdintNEXT_CABINET:
    case fdintPARTIAL_FILE:
    default:

        //
        // Cabinet management functions which we don't use.
        // Return success.
        //
        return(0);

    case fdintCOPY_FILE:

        //
        // Diamond is asking us whether we want to copy the file.
        //
        {
            PDIAMOND_INFO Info = (PDIAMOND_INFO)Parameters->pv;
            HFILE h;

            //
            // If we were given a filespec, see if the name matches.
            //

            if (Info->SelectiveFilesSpec != NULL) {

                //
                //  Call PatternMatch(), fAllowImpliedDot TRUE if
                //  there is no '.' in the file's base name.
                //

                BOOL fAllowImpliedDot = TRUE;
                PSTR p;

                for (p = Parameters->psz1; *p != '\0'; p++) {
                    if (*p == '.') {
                        fAllowImpliedDot = FALSE;
                    } else if (*p == '\\') {
                        fAllowImpliedDot = TRUE;
                    }
                }
                
                if (PatternMatch(
                        Parameters->psz1,
                        Info->SelectiveFilesSpec,
                        fAllowImpliedDot) == FALSE) {

                    return(0);     // skip this file
                }
            }

            //
            // If we need to rename the target file, do that here.
            // The name stored in the cabinet file will be used as
            // the uncompressed name.
            //
            if(Info->RenameTargetFile) {

                PSTR p,q;

                //
                // Find the start of the filename part of the target.
                //
                if(p = StringRevChar(Info->TargetFileName,'\\')) {
                    p++;
                } else {
                    p = Info->TargetFileName;
                }

                //
                // Find the start of the filename part of the name in the cabinet.
                //
                if(q = StringRevChar(Parameters->psz1,'\\')) {
                    q++;
                } else {
                    q = Parameters->psz1;
                }

                //
                // Copy the filename part of the name in the cabinet over
                // the filename part of the name in the target spec.
                //
                lstrcpy(p,q);
            }

            //
            // Inform the expand callback what we are doing.
            //
            if(!Info->ExpandNotify(Info->SourceFileName,Info->TargetFileName,NOTIFY_START_EXPAND)) {
                return(0);  // skip this file.
            }

            //
            // Remember the uncompressed size and open the file.
            // Returns -1 if an error occurs opening the file.
            //
            Info->pLZI->cblOutSize += Parameters->cb;
            h = _lcreat(Info->TargetFileName,0);
            if(h == HFILE_ERROR) {
                DiamondLastIoError = LZERROR_BADOUTHANDLE;
                return(-1);
            }
            return(h);
        }

    case fdintCLOSE_FILE_INFO:

        //
        // Diamond is done with the target file and wants us to close it.
        // (ie, this is the counterpart to fdint_COPY_FILE).
        //
        {
            PDIAMOND_INFO Info = (PDIAMOND_INFO)Parameters->pv;
            HANDLE TargetFileHandle;
            FILETIME ftLocal, ftUTC;

            _lclose((HFILE)Parameters->hf);

            //
            // Set the target file's date/time stamp from the value inside
            // the CAB.
            //
            TargetFileHandle = CreateFile(Info->TargetFileName,
					   GENERIC_READ | GENERIC_WRITE,
					   0,
					   NULL,
					   OPEN_EXISTING,
					   0,
					   NULL);

            if (TargetFileHandle != INVALID_HANDLE_VALUE)
            {
                if (DosDateTimeToFileTime(Parameters->date, Parameters->time, &ftLocal) &&
                    LocalFileTimeToFileTime(&ftLocal, &ftUTC))
                {
                    SetFileTime(TargetFileHandle, NULL, NULL, &ftUTC);
                }

                CloseHandle(TargetFileHandle);
            }
        }
        return(TRUE);    

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
    return((PVOID)LocalAlloc(LMEM_FIXED,NumberOfBytes));
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

    if(oflag & _O_WRONLY) {
        OpenMode = OF_WRITE;
    } else {
        if(oflag & _O_RDWR) {
            OpenMode = OF_READWRITE;
        } else {
            OpenMode = OF_READ;
        }
    }

    h = _lopen(FileName,OpenMode | OF_SHARE_DENY_WRITE);

    if(h == HFILE_ERROR) {
        DiamondLastIoError = LZERROR_BADINHANDLE;
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

    rc = _lread((HFILE)Handle,pv,ByteCount);

    if(rc == HFILE_ERROR) {
        rc = (UINT)(-1);
        DiamondLastIoError = LZERROR_READ;
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

    rc = _lwrite((HFILE)Handle,pv,ByteCount);

    if(rc == HFILE_ERROR) {

        DiamondLastIoError = (GetLastError() == ERROR_DISK_FULL) ? LZERROR_WRITE : LZERROR_BADOUTHANDLE;

    } else {

        if(rc != ByteCount) {
            //
            // let caller interpret return value but record last error just in case
            //
            DiamondLastIoError = LZERROR_WRITE;
        }
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
    _lclose((HFILE)Handle);
    return(0);
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

    if(rc == HFILE_ERROR) {
        DiamondLastIoError = LZERROR_BADINHANDLE;
        rc = -1L;
    }

    return(rc);
}


INT
ExpandDiamondFile(
    IN  NOTIFYPROC ExpandNotify,
    IN  PSTR       SourceFileName,
    IN  PSTR       TargetFileName,
    IN  BOOL       RenameTarget,
    IN  PSTR       SelectiveFilesSpec,
    OUT PLZINFO    pLZI
    )
{
    BOOL b;
    INT rc;
    INT_PTR h;
    DIAMOND_INFO DiamondInfo;

    if(!FdiContext) {
        return(LZERROR_BADVALUE);
    }

    DiamondLastIoError = TRUE;

    //
    // Get a handle to the source to use to
    // copy the date and time stamp.
    //
    h = SpdFdiOpen(SourceFileName,_O_RDONLY,0);
    if(h == -1) {
        return(LZERROR_BADINHANDLE);
    }

    pLZI->cblInSize = GetFileSize((HANDLE)h,NULL);
    if(pLZI->cblInSize == -1) {
        SpdFdiClose(h);
        return(LZERROR_BADINHANDLE);
    }

    DiamondInfo.SourceFileHandle = h;
    DiamondInfo.SourceFileName = SourceFileName;
    DiamondInfo.TargetFileName = TargetFileName;
    DiamondInfo.RenameTargetFile = RenameTarget;
    DiamondInfo.ExpandNotify = ExpandNotify;
    DiamondInfo.SelectiveFilesSpec = SelectiveFilesSpec;
    DiamondInfo.pLZI = pLZI;

    b = FDICopy(
            FdiContext,
            SourceFileName,             // pass the whole path as the name
            "",                         // don't bother with the path part
            0,                          // flags
            DiamondNotifyFunction,
            NULL,                       // no decryption
            &DiamondInfo
            );

    if(b) {

        rc = TRUE;

    } else {

        switch(FdiError.erfOper) {

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
        DeleteFile(TargetFileName);
    }

    SpdFdiClose(h);

    return(rc);
}


BOOL
IsDiamondFile(
    IN PSTR FileName,
    OUT PBOOL ContainsMultipleFiles
    )
{
    FDICABINETINFO CabinetInfo;
    BOOL b;
    INT_PTR h;

    *ContainsMultipleFiles = FALSE;

    if(!FdiContext) {
        return(FALSE);
    }

    //
    // Open the file such that the handle is valid for use
    // in the diamond context (ie, seek, read routines above).
    //
    h = SpdFdiOpen(FileName,_O_RDONLY,0);
    if(h == -1) {
        return(FALSE);
    }

    b = FDIIsCabinet(FdiContext,h,&CabinetInfo);

    SpdFdiClose(h);

    if (b && (CabinetInfo.cFiles > 1)) {
        *ContainsMultipleFiles = TRUE;
    }

    return(b);
}


BOOL
InitDiamond(
    VOID
    )
{
    if(FdiContext == NULL) {

        FdiContext = FDICreate(
                        SpdFdiAlloc,
                        SpdFdiFree,
                        SpdFdiOpen,
                        SpdFdiRead,
                        SpdFdiWrite,
                        SpdFdiClose,
                        SpdFdiSeek,
                        cpuUNKNOWN,
                        &FdiError
                        );
    }

    return(FdiContext != NULL);
}


VOID
TermDiamond(
    VOID
    )
{
    if(FdiContext) {
        FDIDestroy(FdiContext);
        FdiContext = NULL;
    }
}


