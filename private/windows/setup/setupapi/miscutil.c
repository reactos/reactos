/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    miscutil.c

Abstract:

    Miscellaneous utility functions for Windows NT Setup API dll.

Author:

    Ted Miller (tedm) 20-Jan-1995

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


#if MEM_DBG
#undef DuplicateString          // defined again below
#endif

#if MEM_DBG

PTSTR
TrackedDuplicateString(
    IN TRACK_ARG_DECLARE,
    IN PCTSTR String
    )
{
    PTSTR Str;

    TRACK_PUSH

    Str = DuplicateString (String);

    TRACK_POP

    return Str;
}

#endif


PTSTR
DuplicateString(
    IN PCTSTR String
    )

/*++

Routine Description:

    Create a duplicate copy of a nul-terminated string.
    If the string pointer is not valid an exception is generated.

Arguments:

    String - supplies string to be duplicated.

Return Value:

    NULL if out of memory.
    Caller can free buffer with MyFree().

--*/

{
    PTSTR p;

    //
    // The win32 lstrlen and lstrcpy functions are guarded with
    // try/except (at least on NT). So if we use them and the string
    // is invalid, we may end up 'laundering' it into a valid 0-length
    // string. We don't want that -- we actually want to fault
    // in that case. So use the CRT functions, which we know are
    // unguarded and will generate exceptions with invalid args.
    //
    // Also handle the case where the string is valid when we are
    // taking its length, but becomes invalid before or while we
    // are copying it. If we're not careful this could be a memory
    // leak. A try/finally does exactly what we want -- allowing us
    // to clean up and still 'pass on' the exception.
    //
    if(p = MyMalloc((_tcslen(String)+1)*sizeof(TCHAR))) {
        try {
            //
            // If String is or becomes invalid, this will generate
            // an exception, but before execution leaves this routine
            // we'll hit the termination handler.
            //
            _tcscpy(p,String);
        } finally {
            //
            // If a fault occurred during the copy, free the copy.
            // Execution will then pass to whatever exception handler
            // might exist in the caller, etc.
            //
            if(AbnormalTermination()) {
                MyFree(p);
            }
        }
    }

    return(p);
}

#if MEM_DBG
#define DuplicateString(x)              TrackedDuplicateString(TRACK_ARG_CALL,x)
#endif


PSTR
UnicodeToMultiByte(
    IN PCWSTR UnicodeString,
    IN UINT   Codepage
    )

/*++

Routine Description:

    Convert a string from unicode to ansi.

Arguments:

    UnicodeString - supplies string to be converted.

    Codepage - supplies codepage to be used for the conversion.

Return Value:

    NULL if out of memory or invalid codepage.
    Caller can free buffer with MyFree().

--*/

{
    UINT WideCharCount;
    PSTR String;
    UINT StringBufferSize;
    UINT BytesInString;
    PSTR p;

    WideCharCount = lstrlenW(UnicodeString) + 1;

    //
    // Allocate maximally sized buffer.
    // If every unicode character is a double-byte
    // character, then the buffer needs to be the same size
    // as the unicode string. Otherwise it might be smaller,
    // as some unicode characters will translate to
    // single-byte characters.
    //
    StringBufferSize = WideCharCount * 2;
    String = MyMalloc(StringBufferSize);
    if(String == NULL) {
        return(NULL);
    }

    //
    // Perform the conversion.
    //
    BytesInString = WideCharToMultiByte(
                        Codepage,
                        0,                      // default composite char behavior
                        UnicodeString,
                        WideCharCount,
                        String,
                        StringBufferSize,
                        NULL,
                        NULL
                        );

    if(BytesInString == 0) {
        MyFree(String);
        return(NULL);
    }

    //
    // Resize the string's buffer to its correct size.
    // If the realloc fails for some reason the original
    // buffer is not freed.
    //
    if(p = MyRealloc(String,BytesInString)) {
        String = p;
    }

    return(String);
}


PWSTR
MultiByteToUnicode(
    IN PCSTR String,
    IN UINT  Codepage
    )

/*++

Routine Description:

    Convert a string to unicode.

Arguments:

    String - supplies string to be converted.

    Codepage - supplies codepage to be used for the conversion.

Return Value:

    NULL if string could not be converted (out of memory or invalid cp)
    Caller can free buffer with MyFree().

--*/

{
    UINT BytesIn8BitString;
    UINT CharsInUnicodeString;
    PWSTR UnicodeString;
    PWSTR p;

    BytesIn8BitString = lstrlenA(String) + 1;

    //
    // Allocate maximally sized buffer.
    // If every character is a single-byte character,
    // then the buffer needs to be twice the size
    // as the 8bit string. Otherwise it might be smaller,
    // as some characters are 2 bytes in their unicode and
    // 8bit representations.
    //
    UnicodeString = MyMalloc(BytesIn8BitString * sizeof(WCHAR));
    if(UnicodeString == NULL) {
        return(NULL);
    }

    //
    // Perform the conversion.
    //
    CharsInUnicodeString = MultiByteToWideChar(
                                Codepage,
                                MB_PRECOMPOSED,
                                String,
                                BytesIn8BitString,
                                UnicodeString,
                                BytesIn8BitString
                                );

    if(CharsInUnicodeString == 0) {
        MyFree(UnicodeString);
        return(NULL);
    }

    //
    // Resize the unicode string's buffer to its correct size.
    // If the realloc fails for some reason the original
    // buffer is not freed.
    //
    if(p = MyRealloc(UnicodeString,CharsInUnicodeString*sizeof(WCHAR))) {
        UnicodeString = p;
    }

    return(UnicodeString);
}


#ifdef UNICODE

DWORD
CaptureAndConvertAnsiArg(
    IN  PCSTR   AnsiString,
    OUT PCWSTR *UnicodeString
    )

/*++

Routine Description:

    Capture an ANSI string whose validity is suspect and convert it
    into a Unicode string. The conversion is completely guarded and thus
    won't fault, leak memory in the error case, etc.

Arguments:

    AnsiString - supplies string to be converted.

    UnicodeString - if successful, receives pointer to unicode equivalent
        of AnsiString. Caller must free with MyFree(). If not successful,
        receives NULL. This parameter is NOT validated so be careful.

Return Value:

    Win32 error code indicating outcome.

    NO_ERROR - success, UnicodeString filled in.
    ERROR_NOT_ENOUGH_MEMORY - insufficient memory for conversion.
    ERROR_INVALID_PARAMETER - AnsiString was invalid.

--*/

{
    PSTR ansiString;
    DWORD d;

    //
    // Capture the string first. We do this because MultiByteToUnicode
    // won't fault if AnsiString were to become invalid, meaning we could
    // 'launder' a bogus argument into a valid one. Be careful not to
    // leak memory in the error case, etc (see comments in DuplicateString()).
    // Do NOT use Win32 string functions here; we rely on faults occuring
    // when pointers are invalid!
    //
    *UnicodeString = NULL;
    d = NO_ERROR;
    try {
        ansiString = MyMalloc(strlen(AnsiString)+1);
        if(!ansiString) {
            d = ERROR_NOT_ENOUGH_MEMORY;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If we get here, strlen faulted and ansiString
        // was not allocated.
        //
        d = ERROR_INVALID_PARAMETER;
    }

    if(d == NO_ERROR) {
        try {
            strcpy(ansiString,AnsiString);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            d = ERROR_INVALID_PARAMETER;
            MyFree(ansiString);
        }
    }

    if(d == NO_ERROR) {
        //
        // Now we have a local copy of the string; don't worry
        // about faults any more.
        //
        *UnicodeString = MultiByteToUnicode(ansiString,CP_ACP);
        if(*UnicodeString == NULL) {
            d = ERROR_NOT_ENOUGH_MEMORY;
        }

        MyFree(ansiString);
    }

    return(d);
}

#else

DWORD
CaptureAndConvertAnsiArg(
    IN  PCSTR   AnsiString,
    OUT PCWSTR *UnicodeString
    )
{
    //
    // Stub so the dll will link.
    //
    UNREFERENCED_PARAMETER(AnsiString);
    UNREFERENCED_PARAMETER(UnicodeString);
    return(ERROR_CALL_NOT_IMPLEMENTED);
}

#endif


DWORD
CaptureStringArg(
    IN  PCTSTR  String,
    OUT PCTSTR *CapturedString
    )

/*++

Routine Description:

    Capture a string whose validity is suspect.
    This operation is completely guarded and thus won't fault,
    leak memory in the error case, etc.

Arguments:

    String - supplies string to be captured.

    CapturedString - if successful, receives pointer to captured equivalent
        of String. Caller must free with MyFree(). If not successful,
        receives NULL. This parameter is NOT validated so be careful.

Return Value:

    Win32 error code indicating outcome.

    NO_ERROR - success, CapturedString filled in.
    ERROR_NOT_ENOUGH_MEMORY - insufficient memory for conversion.
    ERROR_INVALID_PARAMETER - String was invalid.

--*/

{
    DWORD d;

    try {
        //
        // DuplicateString is guaranteed to generate a fault
        // if the string is invalid. Otherwise if it is non-NULL
        // the it succeeded.
        //
        *CapturedString = DuplicateString(String);
        d = (*CapturedString == NULL) ? ERROR_NOT_ENOUGH_MEMORY : NO_ERROR;

    } except(EXCEPTION_EXECUTE_HANDLER) {

        d = ERROR_INVALID_PARAMETER;
        *CapturedString = NULL;
    }

    return(d);
}


BOOL
ConcatenatePaths(
    IN OUT PTSTR  Target,
    IN     PCTSTR Path,
    IN     UINT   TargetBufferSize,
    OUT    PUINT  RequiredSize          OPTIONAL
    )

/*++

Routine Description:

    Concatenate 2 paths, ensuring that one, and only one,
    path separator character is introduced at the junction point.

Arguments:

    Target - supplies first part of path. Path is appended to this.

    Path - supplies path to be concatenated to Target.

    TargetBufferSize - supplies the size of the Target buffer,
        in characters.

    RequiredSize - if specified, receives the number of characters
        required to hold the fully concatenated path, including
        the terminating nul.

Return Value:

    TRUE if the full path fit in Target buffer. Otherwise the path
    will have been truncated.

--*/

{
    UINT TargetLength,PathLength;
    BOOL TrailingBackslash,LeadingBackslash;
    UINT EndingLength;

    TargetLength = lstrlen(Target);
    PathLength = lstrlen(Path);

    //
    // See whether the target has a trailing backslash.
    //
#ifdef UNICODE
    if(TargetLength && (Target[TargetLength-1] == TEXT('\\'))) {
        TrailingBackslash = TRUE;
        TargetLength--;
    } else {
        TrailingBackslash = FALSE;
    }
#else
    if(TargetLength && (*CharPrev(Target,Target+TargetLength) == TEXT('\\'))) {
        TrailingBackslash = TRUE;
        TargetLength--;
    } else {
        TrailingBackslash = FALSE;
    }
#endif    
    //
    // See whether the path has a leading backshash.
    //
    if(Path[0] == TEXT('\\')) {
        LeadingBackslash = TRUE;
        PathLength--;
    } else {
        LeadingBackslash = FALSE;
    }

    //
    // Calculate the ending length, which is equal to the sum of
    // the length of the two strings modulo leading/trailing
    // backslashes, plus one path separator, plus a nul.
    //
    EndingLength = TargetLength + PathLength + 2;
    if(RequiredSize) {
        *RequiredSize = EndingLength;
    }

    if(!LeadingBackslash && (TargetLength < TargetBufferSize)) {
        Target[TargetLength++] = TEXT('\\');
    }

    if(TargetBufferSize > TargetLength) {
        lstrcpyn(Target+TargetLength,Path,TargetBufferSize-TargetLength);
    }

    //
    // Make sure the buffer is nul terminated in all cases.
    //
    if (TargetBufferSize) {
        Target[TargetBufferSize-1] = 0;
    }

    return(EndingLength <= TargetBufferSize);
}


PCTSTR
MyGetFileTitle(
    IN PCTSTR FilePath
    )

/*++

Routine Description:

    This routine returns a pointer to the first character in the
    filename part of the supplied path.  If only a filename was given,
    then this will be a pointer to the first character in the string
    (i.e., the same as what was passed in).

    To find the filename part, the routine returns the last component of
    the string, beginning with the character immediately following the
    last '\', '/' or ':'. (NB NT treats '/' as equivalent to '\' )

Arguments:

    FilePath - Supplies the file path from which to retrieve the filename
        portion.

Return Value:

    A pointer to the beginning of the filename portion of the path.

--*/

{
    PCTSTR LastComponent = FilePath;
    TCHAR  CurChar;

    while(CurChar = *FilePath) {
        FilePath = CharNext(FilePath);
        if((CurChar == TEXT('\\')) || (CurChar == TEXT('/')) || (CurChar == TEXT(':'))) {
            LastComponent = FilePath;
        }
    }

    return LastComponent;
}


DWORD
DelimStringToMultiSz(
    IN PTSTR String,
    IN DWORD StringLen,
    IN TCHAR Delim
    )

/*++

Routine Description:

    Converts a string containing a list of items delimited by
    'Delim' into a MultiSz buffer.  The conversion is done in-place.
    Leading and trailing whitespace is removed from each constituent
    string.  Delimiters inside of double-quotes (") are ignored.  The
    quotation marks are removed during processing, and any trailing
    whitespace is trimmed from each string (whether or not the
    whitespace was originally enclosed in quotes.  This is consistent
    with the way LFNs are treated by the file system (i.e., you can
    create a filename with preceding whitespace, but not with trailing
    whitespace.

    NOTE:  The buffer containing the string must be 1 character longer
    than the string itself (including NULL terminator).  An extra
    character is required when there's only 1 string, and no whitespace
    to trim, e.g.:  'ABC\0' (len=4) becomes 'ABC\0\0' (len=5).

Arguments:

    String - Supplies the address of the string to be converted.

    StringLen - Supplies the length, in characters, of the String
        (may include terminating NULL).

    Delim - Specifies the delimiter character.

Return Value:

    This routine returns the number of strings in the resulting multi-sz
    buffer.

--*/

{
    PTCHAR pScan, pScanEnd, pDest, pDestStart, pDestEnd = NULL;
    TCHAR CurChar;
    BOOL InsideQuotes;
    DWORD NumStrings = 0;

    //
    // Truncate any leading whitespace.
    //
    pScanEnd = (pDestStart = String) + StringLen;

    for(pScan = String; pScan < pScanEnd; pScan++) {
        if(!(*pScan)) {
            //
            // We hit a NULL terminator without ever hitting a non-whitespace
            // character.
            //
            goto clean0;

        } else if(!IsWhitespace(pScan)) {
            break;
        }
    }

    for(pDest = pDestStart, InsideQuotes = FALSE; pScan < pScanEnd; pScan++) {

        if((CurChar = *pScan) == TEXT('\"')) {
            InsideQuotes = !InsideQuotes;
        } else if(CurChar && (InsideQuotes || (CurChar != Delim))) {
            if(!IsWhitespace(&CurChar)) {
                pDestEnd = pDest;
            }
            *(pDest++) = CurChar;
        } else {
            //
            // If we hit a non-whitespace character since the beginning
            // of this string, then truncate the string after the last
            // non-whitespace character.
            //
            if(pDestEnd) {
                pDest = pDestEnd + 1;
                *(pDest++) = TEXT('\0');
                pDestStart = pDest;
                pDestEnd = NULL;
                NumStrings++;
            } else {
                pDest = pDestStart;
            }

            if(CurChar) {
                //
                // Then we haven't hit a NULL terminator yet. We need to strip
                // off any leading whitespace from the next string, and keep
                // going.
                //
                for(pScan++; pScan < pScanEnd; pScan++) {
                    if(!(CurChar = *pScan)) {
                        break;
                    } else if(!IsWhitespace(&CurChar)) {
                        //
                        // We need to be at the position immediately preceding
                        // this character.
                        //
                        pScan--;
                        break;
                    }
                }
            }

            if((pScan >= pScanEnd) || !CurChar) {
                //
                // We reached the end of the buffer or hit a NULL terminator.
                //
                break;
            }
        }
    }

clean0:

    if(pDestEnd) {
        //
        // Then we have another string at the end we need to terminate.
        //
        pDestStart = pDestEnd + 1;
        *(pDestStart++) = TEXT('\0');
        NumStrings++;

    } else if(pDestStart == String) {
        //
        // Then no strings were found, so create a single empty string.
        //
        *(pDestStart++) = TEXT('\0');
        NumStrings++;
    }

    //
    // Write out an additional NULL to terminate the string list.
    //
    *pDestStart = TEXT('\0');

    return NumStrings;
}


BOOL
LookUpStringInTable(
    IN  PSTRING_TO_DATA Table,
    IN  PCTSTR          String,
    OUT PUINT           Data
    )

/*++

Routine Description:

    Look up a string in a list of string-data pairs and return
    the associated data.

Arguments:

    Table - supplies an array of string-data pairs. The list is terminated
        when a String member of this array is NULL.

    String - supplies a string to be looked up in the table.

    Data - receives the assoicated data if the string is founf in the table.

Return Value:

    TRUE if the string was found in the given table, FALSE if not.

--*/

{
    UINT i;

    for(i=0; Table[i].String; i++) {
        if(!lstrcmpi(Table[i].String,String)) {
            *Data = Table[i].Data;
            return(TRUE);
        }
    }

    return(FALSE);
}


BOOL
InitializeSynchronizedAccess(
    OUT PMYLOCK Lock
    )

/*++

Routine Description:

    Initialize a lock structure to be used with Synchronization routines.

Arguments:

    Lock - supplies structure to be initialized. This routine creates
        the locking event and mutex and places handles in this structure.

Return Value:

    TRUE if the lock structure was successfully initialized. FALSE if not.

--*/

{
    if(Lock->Handles[TABLE_DESTROYED_EVENT] = CreateEvent(NULL,TRUE,FALSE,NULL)) {
        if(Lock->Handles[TABLE_ACCESS_MUTEX] = CreateMutex(NULL,FALSE,NULL)) {
            return(TRUE);
        }
        CloseHandle(Lock->Handles[TABLE_DESTROYED_EVENT]);
    }
    return(FALSE);
}


VOID
DestroySynchronizedAccess(
    IN OUT PMYLOCK Lock
    )

/*++

Routine Description:

    Tears down a lock structure created by InitializeSynchronizedAccess.
    ASSUMES THAT THE CALLING ROUTINE HAS ALREADY ACQUIRED THE LOCK!

Arguments:

    Lock - supplies structure to be torn down. The structure itself
        is not freed.

Return Value:

    None.

--*/

{
    HANDLE h1,h2;

    h1 = Lock->Handles[TABLE_DESTROYED_EVENT];
    h2 = Lock->Handles[TABLE_ACCESS_MUTEX];

    Lock->Handles[TABLE_DESTROYED_EVENT] = NULL;
    Lock->Handles[TABLE_ACCESS_MUTEX] = NULL;

    CloseHandle(h2);

    SetEvent(h1);
    CloseHandle(h1);
}


BOOL
WaitForPostedThreadMessage(
    OUT LPMSG MessageData,
    IN  UINT  Message
    )
/*++

Routine Description:

    This routine waits for the specified message to arrive in the input queue.
    When the posted message arrives, it is removed from the queue, and returned
    to the caller.  All other messages are left undisturbed.

Arguments:

    MessageData - Supplies the address of a MSG structure that receives the
        message data.

    Message - Specifies the message to wait for.

Return Value:

    If successful, the return value is TRUE, otherwise, it is FALSE.

--*/
{
    while(WaitMessage()) {
        if(PeekMessage(MessageData, (HWND)-1, Message, Message, PM_REMOVE)) {
            return TRUE;
        }
    }

    return FALSE;
}


#ifdef _X86_
BOOL
IsNEC98(
    VOID
    )
{
    static BOOL Checked = FALSE;
    static BOOL Is98;

    if(!Checked) {

        Is98 = ((GetKeyboardType(0) == 7) && ((GetKeyboardType(1) & 0xff00) == 0x0d00));

        Checked = TRUE;
    }

    return(Is98);
}
#endif


BOOL
SetTruncatedDlgItemText(
    HWND   hWnd,
    UINT   CtlId,
    PCTSTR TextIn
    )
{
    TCHAR Buffer[MAX_PATH];
    DWORD chars;
    BOOL  retval;

    lstrcpy(Buffer, TextIn);
    chars = ExtraChars(GetDlgItem(hWnd,CtlId),Buffer);
    if (chars) {
        LPTSTR ShorterText = CompactFileName(Buffer,chars);
        if (ShorterText) {
            retval = SetDlgItemText(hWnd,CtlId,ShorterText);
            MyFree(ShorterText);
        } else {
            retval = SetDlgItemText(hWnd,CtlId,Buffer);
        }
    } else {
        retval = SetDlgItemText(hWnd,CtlId,Buffer);                
    }
    
    return(retval);

}

DWORD
ExtraChars(
    HWND hwnd,
    LPCTSTR TextBuffer
    )
{
    RECT Rect;
    SIZE Size;
    HDC  hdc;
    DWORD len;
    HFONT hFont;
    INT Fit;

    hdc = GetDC( hwnd );
    GetWindowRect( hwnd, &Rect );
    hFont = (HFONT)SendMessage( hwnd, WM_GETFONT, 0, 0 );
    if (hFont != NULL) {
        SelectObject( hdc, hFont );
    }

    len = lstrlen( TextBuffer );

    if (!GetTextExtentExPoint(
        hdc,
        TextBuffer,
        len,
        Rect.right - Rect.left,
        &Fit,
        NULL,
        &Size
        )) {

        //
        // can't determine the text extents so we return zero
        //

        Fit = len;
    }

    ReleaseDC( hwnd, hdc );

    if (Fit < (INT)len) {
        return len - Fit;
    }

    return 0;
}


LPTSTR
CompactFileName(
    LPCTSTR FileNameIn,
    DWORD CharsToRemove
    )
{
    LPTSTR start;
    LPTSTR FileName;
    DWORD  FileNameLen;
    LPTSTR lastPart;
    DWORD  lastPartLen;
    DWORD  lastPartPos;
    LPTSTR midPart;
    DWORD  midPartPos;

    if (! FileNameIn) {
       return NULL;
    }

    FileName = MyMalloc( (lstrlen( FileNameIn ) + 16) * sizeof(TCHAR) );
    if (! FileName) {
       return NULL;
    }

    lstrcpy( FileName, FileNameIn );

    FileNameLen = lstrlen(FileName);

    if (FileNameLen < CharsToRemove + 3) {
       // nothing to remove
       return FileName;
    }

    lastPart = _tcsrchr(FileName, TEXT('\\') );
    if (! lastPart) {
       // nothing to remove
       return FileName;
    }

    lastPartLen = lstrlen(lastPart);

    // temporary null-terminate FileName
    lastPartPos = (DWORD) (lastPart - FileName);
    FileName[lastPartPos] = TEXT('\0');


    midPart = _tcsrchr(FileName, TEXT('\\') );

    // restore
    FileName[lastPartPos] = TEXT('\\');

    if (!midPart) {
       // nothing to remove
       return FileName;
    }

    midPartPos = (DWORD) (midPart - FileName);


    if ( ((DWORD) (lastPart - midPart) ) >= (CharsToRemove + 3) ) {
       // found
       start = midPart+1;
       start[0] = start[1] = start[2] = TEXT('.');
       start += 3;
       _tcscpy(start, lastPart);
       start[lastPartLen] = TEXT('\0');

       return FileName;
    }



    do {
       FileName[midPartPos] = TEXT('\0');

       midPart = _tcsrchr(FileName, TEXT('\\') );

       // restore
       FileName[midPartPos] = TEXT('\\');

       if (!midPart) {
          // nothing to remove
          return FileName;
       }

       midPartPos = (DWORD) (midPart - FileName);

       if ( (DWORD) ((lastPart - midPart) ) >= (CharsToRemove + 3) ) {
          // found
          start = midPart+1;
          start[0] = start[1] = start[2] = TEXT('.');
          start += 3;
          lstrcpy(start, lastPart);
          start[lastPartLen] = TEXT('\0');

          return FileName;
       }

    } while ( 1 );

}
