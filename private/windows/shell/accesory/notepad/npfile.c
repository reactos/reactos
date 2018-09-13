/*
 * npfile.c  - Routines for file i/o for notepad
 *   Copyright (C) 1984-1995 Microsoft Inc.
 */

#include "precomp.h"


HANDLE  hFirstMem;
CHAR    BOM_UTF8[3]= {(BYTE) 0xEF, (BYTE) 0xBB, (BYTE)0xBF};



//****************************************************************
//
//   ReverseEndian
//
//   Purpose: copies unicode character from one endian source
//            to another.
//
//            may work on lpDst == lpSrc
//

VOID ReverseEndian( PTCHAR lpDst, PTCHAR lpSrc, DWORD nChars )
{
    DWORD  cnt;

    for( cnt=0; cnt < nChars; cnt++,lpDst++,lpSrc++ )
    {
        *lpDst= (TCHAR) (((*lpSrc<<8) & 0xFF00) + ((*lpSrc>>8)&0xFF));
    }
}

//*****************************************************************
//
//   AnsiWriteFile()
//
//   Purpose     : To simulate the effects of _lwrite() in a Unicode
//                 environment by converting to ANSI buffer and
//                 writing out the ANSI text.
//   Returns     : TRUE is successful, FALSE if not
//                 GetLastError() will have the error code.
//
//*****************************************************************

BOOL AnsiWriteFile(HANDLE  hFile,    // file to write to
                   UINT uCodePage,   // code page to convert unicode to
                   LPVOID lpBuffer,  // unicode buffer
                   DWORD nChars,     // number of unicode chars
                   DWORD nBytes )    // number of ascii chars to produce
{
    LPSTR   lpAnsi;              // pointer to allocate buffer
    BOOL    Done;                // status from write (returned)
    BOOL    fDefCharUsed;        // flag that conversion wasn't perfect
    BOOL*   pfDefCharUsed;       // pointer to flag
    DWORD   nBytesWritten;       // number of bytes written

    lpAnsi= (LPSTR) LocalAlloc( LPTR, nBytes + 1 );
    if( !lpAnsi )
    {
       SetLastError( ERROR_NOT_ENOUGH_MEMORY );
       return (FALSE);
    }

    pfDefCharUsed= NULL;
    if( (uCodePage != CP_UTF8 ) && (uCodePage != CP_UTF7) )
    {
        pfDefCharUsed= &fDefCharUsed;
    }
    WideCharToMultiByte( uCodePage,       // code page
                         0,               // performance and mapping flags
                        (LPWSTR) lpBuffer,// wide char buffer
                         nChars,          // chars in wide char buffer
                         lpAnsi,          // resultant ascii string
                         nBytes,          // size of ascii string buffer
                         NULL,            // char to sub. for unmapped chars
                         pfDefCharUsed);  // flag to set if default char used
                                          

    Done= WriteFile( hFile, lpAnsi, nBytes, &nBytesWritten, NULL );

    LocalFree( lpAnsi );

    return (Done);

} // end of AnsiWriteFile()


// not used anymore ..
#if 0

/* IsNotepadEmpty
 * Check if the edit control is empty.  If it is, put up a messagebox
 * offering to delete the file if it already exists, or just warning
 * that it can't be saved if it doesn't already exist
 *
 * Return value:  TRUE, warned, no further action should take place
 *                FALSE, not warned, or further action is necessary
 * 30 July 1991            Clark Cyr
 */

INT FAR IsNotepadEmpty (HWND hwndParent, TCHAR *szFileSave, BOOL fNoDelete)
{
  unsigned  nChars;
  INT       nRetVal = FALSE;

    nChars = (unsigned) SendMessage (hwndEdit, WM_GETTEXTLENGTH, 0, (LPARAM)0);

    /* If notepad is empty, complain and delete file if necessary. */

    if (!nChars)
    {
       if (fNoDelete)
          nRetVal = MessageBox(hwndParent, szCSEF, szNN,
                             MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
       else if ((nRetVal = AlertBox(hwndNP, szNN, szEFD, szFileSave,
                  MB_APPLMODAL | MB_OKCANCEL | MB_ICONEXCLAMATION)) == IDOK)
       {
          DeleteFile (szFileSave);
          New(FALSE);
       }
    }
    return nRetVal;

}

#endif


#define NOTUSED 0
static DWORD dwStartSel;    // saved start of selection
static DWORD dwEndSel;      // saved end of selection

VOID ClearFmt(VOID) 
{
    DWORD SelStart;
    DWORD SelEnd;

    SendMessage( hwndEdit, EM_GETSEL, (WPARAM) &dwStartSel, (LPARAM) &dwEndSel );

    SendMessage( hwndEdit, EM_SETSEL, (WPARAM) 0, (LPARAM) 0 );   // this is always legal
    // bugbug: should we scrollcaret here?

    SendMessage( hwndEdit, EM_FMTLINES, (WPARAM)FALSE, NOTUSED );   // remove soft EOLs

}

VOID RestoreFmt(VOID)
{
    UINT CharIndex;

    SendMessage( hwndEdit, EM_FMTLINES, (WPARAM)TRUE, NOTUSED );    // restore soft EOLS

    CharIndex= (UINT) SendMessage( hwndEdit, EM_SETSEL, (WPARAM) dwStartSel, (LPARAM) dwEndSel);

}

/* Save notepad file to disk.  szFileSave points to filename.  fSaveAs
   is TRUE iff we are being called from SaveAsDlgProc.  This implies we must
   open file on current directory, whether or not it already exists there
   or somewhere else in our search path.
   Assumes that text exists within hwndEdit.    30 July 1991  Clark Cyr
 */

BOOL FAR SaveFile (HWND hwndParent, TCHAR *szFileSave, BOOL fSaveAs )
{
  LPTSTR    lpch;
  UINT      nChars;
  BOOL      flag;
  BOOL      fNew = FALSE;
  BOOL      fDefCharUsed = FALSE;
  BOOL*     pfDefCharUsed;
  static    WCHAR wchBOM = BYTE_ORDER_MARK;
  static    WCHAR wchRBOM= REVERSE_BYTE_ORDER_MARK;
  HLOCAL    hEText;                // handle to MLE text
  DWORD     nBytesWritten;         // number of bytes written
  DWORD     nAsciiLength;          // length of equivalent ascii file
  UINT      cpTemp= CP_ACP;        // code page to convert to


    /* If saving to an existing file, make sure correct disk is in drive */
    if (!fSaveAs)
    {
       fp= CreateFile( szFileSave,                 // name of file
                       GENERIC_READ|GENERIC_WRITE, // access mode
                       FILE_SHARE_READ,            // share mode
                       NULL,                       // security descriptor
                       OPEN_EXISTING,              // how to create
                       FILE_ATTRIBUTE_NORMAL,      // file attributes
                       NULL);                      // hnd of file with attrs
    }
    else
    {

       // Carefully open the file.  Do not truncate it if it exists.
       // set the fNew flag if it had to be created.
       // We do all this in case of failures later in the process.

       fp= CreateFile( szFileSave,                 // name of file
                       GENERIC_READ|GENERIC_WRITE, // access mode
                       FILE_SHARE_READ|FILE_SHARE_WRITE,  // share mode
                       NULL,                       // security descriptor
                       OPEN_ALWAYS,                // how to create
                       FILE_ATTRIBUTE_NORMAL,      // file attributes
                       NULL);                      // hnd of file with attrs

       if( fp != INVALID_HANDLE_VALUE )
       {
          fNew= (GetLastError() != ERROR_ALREADY_EXISTS );
       }
    }

    if( fp == INVALID_HANDLE_VALUE )
    {
        if (fSaveAs)
          AlertBox( hwndParent, szNN, szCREATEERR, szFileSave,
                    MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }


    // if wordwrap, remove soft carriage returns 
    // Also move the cursor to a safe place to get around MLE bugs
    
    if( fWrap ) 
    {
       ClearFmt();
    }

    /* Must get text length after formatting */
    nChars = (UINT)SendMessage (hwndEdit, WM_GETTEXTLENGTH, 0, (LPARAM)0);

    hEText= (HANDLE) SendMessage( hwndEdit, EM_GETHANDLE, 0,0 );
    if(  !hEText || !(lpch= (LPTSTR) LocalLock(hEText) ))
    {
       AlertUser_FileFail( szFileSave );
       goto CleanUp;
    }



       
    // Determine the SaveAs file type, and write the appropriate BOM.
    // If the filetype is UTF-8 or Ansi, do the conversion.
    switch(g_ftSaveAs)
    {
    case FT_UNICODE:
        WriteFile( fp, &wchBOM, ByteCountOf(1), &nBytesWritten, NULL );
        flag= WriteFile(fp, lpch, ByteCountOf(nChars), &nBytesWritten, NULL);
        break;

    case FT_UNICODEBE:
        WriteFile( fp, &wchRBOM, ByteCountOf(1), &nBytesWritten, NULL );
        ReverseEndian( lpch, lpch,nChars );
        flag= WriteFile(fp, lpch, ByteCountOf(nChars), &nBytesWritten, NULL);
        ReverseEndian( lpch, lpch, nChars );
        break;

    // If it UTF-8, write the BOM (3 bytes), set the code page and fall 
    // through to the default case.
    case FT_UTF8:
        WriteFile( fp, &BOM_UTF8, 3, &nBytesWritten, NULL );        
        cpTemp= CP_UTF8;
        // fall through to convert and write the file

    default:
        pfDefCharUsed= NULL;


        if (g_ftSaveAs != FT_UTF8)
        {
            //
            // Always use the current locale code page to do the translation
            // If the user changes locales, they will need to know what locale
            // this version of the file was saved with.  Since we don't save that
            // information, the user may be screwed.  Unicode would save his bacon.
            //

            cpTemp= GetACP();

            pfDefCharUsed= &fDefCharUsed;
        }
        
        nAsciiLength= WideCharToMultiByte( cpTemp,
                                           0,
                                           (LPWSTR)lpch,
                                           nChars,
                                           NULL,
                                           0,
                                           NULL,
                                           pfDefCharUsed);


        if( fDefCharUsed )
        {
            if ( AlertBox( hwndParent, szNN, szErrUnicode, szFileSave, 
                  MB_APPLMODAL|MB_OKCANCEL|MB_ICONEXCLAMATION) == IDCANCEL)
               goto CleanUp;          
        }
        flag= AnsiWriteFile( fp, cpTemp, lpch, nChars, nAsciiLength );
        break;
    }


    if (!flag)
    {
       SetCursor(hStdCursor);     /* display normal cursor */

       AlertUser_FileFail( szFileSave );
CleanUp:
       SetCursor( hStdCursor );
       CloseHandle (fp); fp=INVALID_HANDLE_VALUE;
       if( hEText )
           LocalUnlock( hEText );
       if (fNew)
          DeleteFile (szFileSave);
       /* if wordwrap, insert soft carriage returns */
       if (fWrap) 
       {
           RestoreFmt();
       }
       return FALSE;
    }
    else
    {
       SetEndOfFile (fp);

       SendMessage (hwndEdit, EM_SETMODIFY, FALSE, 0L);
       SetTitle (szFileSave);
       fUntitled = FALSE;
    }

    CloseHandle (fp); fp=INVALID_HANDLE_VALUE;

    if( hEText )
        LocalUnlock( hEText );

    /* if wordwrap, insert soft carriage returns */
    if (fWrap)
    {
       RestoreFmt();
    }

    /* Display the hour glass cursor */
    SetCursor(hStdCursor);

    return TRUE;

} // end of SaveFile()

/* Read contents of file from disk.
 * Do any conversions required.
 * File is already open, referenced by handle fp
 * Close the file when done.
 * If typeFlag>=0, then use it as filetype, otherwise do automagic guessing.
 */

BOOL FAR LoadFile (TCHAR * sz, INT typeFlag )
{
    UINT      len, i, nChars;
    LPTSTR    lpch=NULL;
    LPTSTR    lpBuf;
    LPSTR     lpBufAfterBOM;
    BOOL      fLog=FALSE;
    TCHAR*    p;
    TCHAR     szSave[MAX_PATH]; /* Private copy of current filename */
    BOOL      bUnicode=FALSE;   /* true if file detected as unicode */
    BOOL      bUTF8=FALSE;      /* true if file detected as UTF-8 */
    DWORD     nBytesRead;       // number of bytes read
    BY_HANDLE_FILE_INFORMATION fiFileInfo;
    BOOL      bStatus;          // boolean status
    HLOCAL    hNewEdit=NULL;    // new handle for edit buffer
    HANDLE    hMap;             // file mapping handle
    TCHAR     szNullFile[2];    // fake null mapped file
    INT       cpTemp = CP_ACP;
    NP_FILETYPE ftOpenedAs=FT_UNKNOWN;


    if( fp == INVALID_HANDLE_VALUE )
    {
       AlertUser_FileFail( sz );
       return (FALSE);
    }

    //
    // Get size of file
    // We use this heavy duty GetFileInformationByHandle API
    // because it finds bugs.  It takes longer, but it only is
    // called at user interaction time.
    //

    bStatus= GetFileInformationByHandle( fp, &fiFileInfo );
    len= (UINT) fiFileInfo.nFileSizeLow;

    // NT may delay giving this status until the file is accessed.
    // i.e. the open succeeds, but operations may fail on damaged files.

    if( !bStatus )
    {
        AlertUser_FileFail( sz );
        CloseHandle( fp ); fp=INVALID_HANDLE_VALUE;
        return( FALSE );
    }

    // If the file is too big, fail now.
    // -1 not valid because we need a zero on the end.

    if( len == -1 || fiFileInfo.nFileSizeHigh != 0 )
    {
       AlertBox( hwndNP, szNN, szErrSpace, sz,
                 MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION );
       CloseHandle (fp); fp=INVALID_HANDLE_VALUE;
       return (FALSE);
    }

    SetCursor(hWaitCursor);                // physical I/O takes time

    //
    // Create a file mapping so we don't page the file to
    // the pagefile.  This is a big win on small ram machines.
    //

    if( len != 0 )
    {
        lpBuf= NULL;

        hMap= CreateFileMapping( fp, NULL, PAGE_READONLY, 0, len, NULL );

        if( hMap )
        {
            lpBuf= MapViewOfFile( hMap, FILE_MAP_READ, 0,0,len);
            CloseHandle( hMap );
        }
    }
    else  // file mapping doesn't work on zero length files
    {
        lpBuf= (LPTSTR) &szNullFile;
        *lpBuf= 0;  // null terminate
    }

    CloseHandle( fp ); fp=INVALID_HANDLE_VALUE;

    if( lpBuf == NULL )
    {
        SetCursor( hStdCursor );
        AlertBox( hwndNP, szNN, szErrSpace, sz,
                  MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION );
        return( FALSE );
    }


    //
    // protect access to the mapped file with a try/except so we
    // can detect I/O errors.
    //

    //
    // WARNING: be very very careful.  This code is pretty fragile.
    // Files across the network, or RSM files (tape) may throw excepts
    // at random points in this code.  Anywhere the code touches the
    // memory mapped file can cause an AV.  Make sure variables are
    // in consistent state if an exception is thrown.  Be very careful
    // with globals.

    __try
    {
    /* Determine the file type and number of characters
     * If the user overrides, use what is specified.
     * Otherwise, we depend on 'IsTextUnicode' getting it right.
     * If it doesn't, bug IsTextUnicode.
     */

    lpBufAfterBOM= (LPSTR) lpBuf;
    if( typeFlag == FT_UNKNOWN )
    {
        switch(*lpBuf)
        {
        case BYTE_ORDER_MARK:
            bUnicode= TRUE;
            ftOpenedAs= FT_UNICODE;

            // don't count the BOM.
            nChars= len / sizeof(TCHAR) -1;
            break;

        case REVERSE_BYTE_ORDER_MARK:
            bUnicode= TRUE;
            ftOpenedAs= FT_UNICODEBE;

            // don't count the BOM.
            nChars= len / sizeof(TCHAR) -1;
            break;

        // UTF bom has 3 bytes; if it doesn't have UTF BOM just fall through ..
        case BOM_UTF8_HALF:            
            if (len > 2 && ((BYTE) *(((LPSTR)lpBuf)+2) == BOM_UTF8_2HALF) )
            {
                bUTF8= TRUE;
                cpTemp= CP_UTF8;
                ftOpenedAs= FT_UTF8;
                // Ignore the first three bytes.
                lpBufAfterBOM= (LPSTR)lpBuf + 3;
                len -= 3;
                break;
            }

        default:

            // Is the file unicode without BOM ?
            if ((bUnicode= IsInputTextUnicode((LPSTR) lpBuf, len)))
            {
                ftOpenedAs= FT_UNICODE;
                nChars= len / sizeof(TCHAR);
            }      
            else
            {
                // Is the file UTF-8 even though it doesn't have UTF-8 BOM.
                if ((bUTF8= IsTextUTF8((LPSTR) lpBuf, len)))
                {
                    ftOpenedAs= FT_UTF8;
                    cpTemp= CP_UTF8;
                }
                // well, not it must be an ansi file!
                else
                {
                    ftOpenedAs= FT_ANSI;
                    cpTemp= CP_ACP;
                }
            }
            break;
        }             
    }

    // find out no. of chars present in the string.
    if (!bUnicode)
    {
        nChars = MultiByteToWideChar (cpTemp,
                                      0,
                                      (LPSTR)lpBufAfterBOM,
                                      len,
                                      NULL,
                                      0);
    }

    //
    // Don't display text until all done.
    //

    SendMessage (hwndEdit, WM_SETREDRAW, (WPARAM)FALSE, (LPARAM)0);

    // Reset selection to 0

    SendMessage(hwndEdit, EM_SETSEL, 0, 0L);
    SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);

    // resize the edit buffer
    // if we can't resize the memory, inform the user

    if (!(hNewEdit= LocalReAlloc(hEdit,ByteCountOf(nChars + 1),LMEM_MOVEABLE)))
    {
      /* Bug 7441: New() causes szFileName to be set to "Untitled".  Save a
       *           copy of the filename to pass to AlertBox.
       *  17 November 1991    Clark R. Cyr
       */
       lstrcpy(szSave, sz);
       New(FALSE);

       /* Display the hour glass cursor */
       SetCursor(hStdCursor);

       AlertBox( hwndNP, szNN, szFTL, szSave,
                 MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
       if( lpBuf != (LPTSTR) &szNullFile )
       {
           UnmapViewOfFile( lpBuf );
       }

       // let user see old text

       SendMessage (hwndEdit, WM_SETREDRAW, (WPARAM)FALSE, (LPARAM)0);
       return FALSE;
    }

    /* Transfer file from temporary buffer to the edit buffer */
    lpch= (LPTSTR) LocalLock(hNewEdit);

    if( bUnicode )
    {
       /* skip the Byte Order Mark */
       if (*lpBuf == BYTE_ORDER_MARK)
       {
          CopyMemory (lpch, lpBuf + 1, ByteCountOf(nChars));
       }
       else if( *lpBuf == REVERSE_BYTE_ORDER_MARK )
       {
          ReverseEndian( lpch, lpBuf+1, nChars );          
       }
       else
       {
          CopyMemory (lpch, lpBuf, ByteCountOf(nChars));
       }
    }
    else
    {      
       nChars = MultiByteToWideChar (cpTemp,
                                     0,
                                     (LPSTR)lpBufAfterBOM,
                                     len,
                                     (LPWSTR)lpch,
                                     nChars);
       
    }

    g_ftOpenedAs= ftOpenedAs;   // got everything; update global safe now

    } 
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        AlertBox( hwndNP, szNN, szDiskError, sz,
            MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION );
        nChars= 0;   // don't deal with it.
    }

    /* Free file mapping */
    if( lpBuf != (LPTSTR) &szNullFile )
    {
        UnmapViewOfFile( lpBuf );
    }


    if( lpch ) 
    {

       // Fix any NUL character that came in from the file to be spaces.

       for (i = 0, p = lpch; i < nChars; i++, p++)
       {
          if( *p == (TCHAR) 0 )
             *p= TEXT(' ');
       }
      
       // null terminate it.  Safe even if nChars==0 because it is 1 TCHAR bigger

       *(lpch+nChars)= (TCHAR) 0;      /* zero terminate the thing */
   
       // Set 'fLog' if first characters in file are ".LOG"
   
       fLog= *lpch++ == TEXT('.') && *lpch++ == TEXT('L') &&
             *lpch++ == TEXT('O') && *lpch == TEXT('G');
    }
   
    if( hNewEdit )
    {
       LocalUnlock( hNewEdit );

       // now it is safe to set the global edit handle

       hEdit= hNewEdit;
    }

    lstrcpy( szFileName, sz );
    SetTitle( sz );
    fUntitled= FALSE;

  /* Pass handle to edit control.  This is more efficient than WM_SETTEXT
   * which would require twice the buffer space.
   */

  /* Bug 7443: If EM_SETHANDLE doesn't have enough memory to complete things,
   * it will send the EN_ERRSPACE message.  If this happens, don't put up the
   * out of memory notification, put up the file to large message instead.
   *  17 November 1991     Clark R. Cyr
   */
    dwEmSetHandle = SETHANDLEINPROGRESS;
    SendMessage (hwndEdit, EM_SETHANDLE, (WPARAM)hEdit, (LPARAM)0);
    if (dwEmSetHandle == SETHANDLEFAILED)
    {
       SetCursor(hStdCursor);

       dwEmSetHandle = 0;
       AlertBox( hwndNP, szNN, szFTL, sz,MB_APPLMODAL|MB_OK|MB_ICONEXCLAMATION);
       New (FALSE);
       SendMessage (hwndEdit, WM_SETREDRAW, (WPARAM)TRUE, (LPARAM)0);
       return (FALSE);
    }
    dwEmSetHandle = 0;

    PostMessage (hwndEdit, EM_LIMITTEXT, (WPARAM)CCHNPMAX, 0L);

    /* If file starts with ".LOG" go to end and stamp date time */
    if (fLog)
    {
       SendMessage( hwndEdit, EM_SETSEL, (WPARAM)nChars, (LPARAM)nChars);
       SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0);
       InsertDateTime(TRUE);
    }

    /* Move vertical thumb to correct position */
    SetScrollPos (hwndNP,
                  SB_VERT,
                  (int) SendMessage (hwndEdit, WM_VSCROLL, EM_GETTHUMB, 0L),
                  TRUE);

    /* Now display text */
    SendMessage( hwndEdit, WM_SETREDRAW, (WPARAM)TRUE, (LPARAM)0 );
    InvalidateRect( hwndEdit, (LPRECT)NULL, TRUE );
    UpdateWindow( hwndEdit );

    SetCursor(hStdCursor);

    return( TRUE );

} // end of LoadFile()

/* New Command - reset everything
 */

void FAR New (BOOL  fCheck)
{
    HANDLE hTemp;
    TCHAR* pSz;

    if (!fCheck || CheckSave (FALSE))
    {
       SendMessage( hwndEdit, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT("") );
       fUntitled= TRUE;
       lstrcpy( szFileName, szUntitled );
       SetTitle(szFileName );
       SendMessage( hwndEdit, EM_SETSEL, 0, 0L );
       SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0 );

       // resize of 1 NULL character i.e. zero length

       hTemp= LocalReAlloc( hEdit, sizeof(TCHAR), LMEM_MOVEABLE );
       if( hTemp )
       {
          hEdit= hTemp;
       }

       // null terminate the buffer.  LocalReAlloc won't do it
       // because in all cases it is not growing which is the
       // only time it would zero out anything.

       pSz= LocalLock( hEdit );
       *pSz= TEXT('\0');
       LocalUnlock( hEdit );

       SendMessage (hwndEdit, EM_SETHANDLE, (WPARAM)hEdit, 0L);
       szSearch[0] = (TCHAR) 0;
    }

} // end of New()

/* If sz does not have extension, append ".txt"
 * This function is useful for getting to undecorated filenames
 * that setup apps use.  DO NOT CHANGE the extension.  Too many setup
 * apps depend on this functionality.
 */

void FAR AddExt( TCHAR* sz )
{
    TCHAR*   pch1;
    int      ch;
    DWORD    dwSize;

    dwSize= lstrlen(sz);

    pch1= sz + dwSize;   // point to end

    ch= *pch1;
    while( ch != TEXT('.') && ch != TEXT('\\') && ch != TEXT(':') && pch1 > sz)
    {
        //
        // backup one character.  Do NOT use CharPrev because
        // it sometimes doesn't actually backup.  Some Thai
        // tone marks fit this category but there seems to be others.
        // This is safe since it will stop at the beginning of the
        // string or on delimiters listed above.  bug# 139374 2/13/98
        //
        // pch1= (TCHAR*)CharPrev (sz, pch1);
        pch1--;  // back up
        ch= *pch1;
    }

    if( *pch1 != TEXT('.') )
    {
       if( dwSize + sizeof(".txt") <= MAX_PATH ) {  // avoid buffer overruns
           lstrcat( sz, TEXT(".txt") );
       }
    }

}


/* AlertUser_FileFail( LPTSTR szFileName )
 *
 * szFileName is the name of file that was attempted to open.
 * Some sort of failure on file open.  Alert the user
 * with some monologue box.  At least give him decent
 * error messages.
 */

VOID FAR AlertUser_FileFail( LPTSTR szFileName )
{
    TCHAR msg[256];     // buffer to format message into
    DWORD dwStatus;     // status from FormatMessage
    UINT  style= MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION;

    // Check GetLastError to see why we failed
    dwStatus=
    FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   GetLastError(),
                   GetUserDefaultLangID(),
                   msg,  // where message will end up
                   CharSizeOf(msg), NULL );
    if( dwStatus )
    {
          MessageBox( hwndNP, msg, szNN, style );
    }
    else
    {
        AlertBox( hwndNP, szNN, szDiskError, szFileName, style );
    }
}
