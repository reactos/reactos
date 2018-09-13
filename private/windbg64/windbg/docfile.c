/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    docfile.c

Abstract:

    This file contains some of the routines which deal with the document
    buffer.

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/

#include "precomp.h"
#pragma hdrstop

#define NON_VALID_LEVEL 32000

//Data for Documents Handling
static HANDLE   hFileDoc;
static LPSTR    pszBufferDoc;
static DWORD    dwOffsetDoc;
static DWORD    dwBytesReadDoc;


// BUGBUG - dead code - kcarlos
#if 0
/***    Flush
**
**  Synopsis:
**      bool = Flush(szFileName)
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

BOOL NEAR PASCAL Flush(LPSTR FileName)
{
    int written;

    written = _lwrite (hFileDoc, pszBufferDoc, dwOffsetDoc);
    if (written == -1)
          return ErrorBox(ERR_File_Write, (LPSTR)FileName);;
    if (written < dwOffsetDoc)
          return ErrorBox(ERR_File_Disk_Full, (LPSTR)FileName);
    return TRUE;
}                                       /* Flush() */
#endif

// BUGBUG - dead code - kcarlos
#if 0
/***    PutChar
**
*/

BOOL NEAR PASCAL PutChar(LPSTR FileName, char ch)
{
    if (dwOffsetDoc >= DISK_BLOCK_SIZE) {
        if (!Flush(FileName))
            return FALSE;
        dwOffsetDoc = 0;
    }
    pszBufferDoc[dwOffsetDoc++] = ch;
    return TRUE;
}                                       /* PutChar() */
#endif

/***    LoadChar
**
**  Synopsis:
**      char = LoadChar()
**
**  Entry:
**      none
**
**  Returns:
**
**  Description:
**
*/

char NEAR PASCAL LoadChar()
{
    if (dwOffsetDoc >= dwBytesReadDoc) {
        if (dwBytesReadDoc < DISK_BLOCK_SIZE) {
            return CTRL_Z;
        } else {           
            if (!ReadFile(hFileDoc, pszBufferDoc, DISK_BLOCK_SIZE, 
                &dwBytesReadDoc, NULL)) {

                return -1;

            } else if (dwBytesReadDoc == 0) {
                return CTRL_Z;
            }
            dwOffsetDoc = 0;
        }
    }
    return (char)pszBufferDoc[dwOffsetDoc++];
}                                       /* LoadChar() */


/***    StoreLine
**
**  Synopsis:
**      bool = StoreLine()
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

BOOL
PASCAL
StoreLine(
    char *line,
    int lineLen,
    int *offset,
    BYTE *prevLength,
    LPBLOCKDEF * pCurBlock
    )
{
    LPLINEREC pl;

    if ((*offset + (LHD + lineLen)) > BLOCK_SIZE) {

        //We arrived at the end of the block. Allocates a new one.

        if (!AllocateBlock(*pCurBlock, NULL, pCurBlock))
              return FALSE;

        ((*pCurBlock)->PrevBlock)->NextBlock = *pCurBlock;
        *offset = 0;
        if ((*pCurBlock)->PrevBlock == NULL)
              *prevLength = 0;
    }
    (*pCurBlock)->LastLineOffset = *offset;
    pl = (LPLINEREC)((*pCurBlock)->Data + *offset);
    AssertAligned(pl);

    pl->PrevLength = *prevLength;
    pl->Length = (BYTE)(LHD + lineLen);
    pl->Status = 0;
    memmove((LPSTR)(pl->Text), (LPSTR)line, lineLen);

    *prevLength = (BYTE)(LHD + lineLen);
    *offset += LHD + lineLen;
#ifdef ALIGN
    *offset = (*offset + 3) & ~3;
#endif
    return TRUE;
}                                       /* StoreLine() */

/***    TruncateLine
**
**  Synopsis:
**      bool = TruncateLine(line, nbLines)
**
**  Entry:
**      line
**      nbLines
**
**  Returns:
**
**  Description:
**
*/

BOOL NEAR PASCAL TruncateLine(PSTR line, int nbLines)
{
    if (QuestionBox(ERR_Truncate_Line, MB_OKCANCEL, nbLines + 1) == IDOK) {
        line[MAX_USER_LINE] = CR;
        return TRUE;
    }
    return FALSE;
}                                       /* TruncateLine() */

/***    LoadLine
**
**  Synopsis:
**      w = LoadLine(szLine, pLineLen, nbLines)
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

WORD NEAR PASCAL LoadLine(PSTR line, int *lineLen, int nbLines)
{
    char prevCh = 0;
    register char ch;
    int level = NON_VALID_LEVEL;
    int extLineLen = 0;
    int totLen;

    *lineLen = 0;

    while (TRUE) {

        //Load next char, don't call LoadChar function to go faster

        if (dwOffsetDoc < dwBytesReadDoc) {
            ch = (char)pszBufferDoc[dwOffsetDoc++];
        } else  if (dwBytesReadDoc < DISK_BLOCK_SIZE) {
            ch = CTRL_Z;
        } else {
            if (!ReadFile(hFileDoc, pszBufferDoc, DISK_BLOCK_SIZE, 
                &dwBytesReadDoc, NULL)) {
                  
                return ERR_File_Read;

            } else {
                if (dwBytesReadDoc == 0) {
                    ch = CTRL_Z;
                } else {
                    ch = (char)pszBufferDoc[0];
                }
            }
            dwOffsetDoc = 1;
        }

        switch (ch) {
            
        case CR:
            break;
            
        case LF:
            if (*lineLen > MAX_USER_LINE) {
                if (TruncateLine(line, nbLines)) {
                    (*lineLen)--;
                } else {
                    return END_ABORT;
                }
            }
            return END_OF_LINE;



          case TAB:
            {
                int  i;

                //Calc number of spaces to insert

                totLen = extLineLen;

                i = g_contGlobalPreferences_WkSp.m_nTabSize - (totLen % g_contGlobalPreferences_WkSp.m_nTabSize);

                if (totLen + i >= MAX_USER_LINE) {
                    if (TruncateLine(line, nbLines)) {

                        char k;

                        //Padd blanks until end of line or remove char
                        (*lineLen)--;

                        //Discard rest of line from input file

                        do {
                            k = LoadChar();
                        } while (k != - 1 && k != CTRL_Z && k != LF);
                        switch (k) {
                          case -1:
                            return ERR_File_Read;
                          case LF:
                            return END_OF_LINE;
                          case CTRL_Z:
                            return END_OF_FILE;
                        }

                        return END_OF_LINE;

                    } else
                          return END_ABORT;
                }

                //Skip if we keep the tab

                extLineLen += i;
                goto defaut;

                //Insert spaces otherwise

                while (i--)
                    line[(*lineLen)++] = ' ';
                break;
            }

          case CTRL_Z:
            return END_OF_FILE;

          default:
          defaut:
            totLen = extLineLen;

            if (totLen > MAX_USER_LINE) {
                if (TruncateLine(line, nbLines)) {

                    char k;

                    (*lineLen)--;


                    //Discard rest of line from input file
                    do {
                        k = LoadChar();
                    } while (k != - 1 && k != CTRL_Z && k != LF);
                    switch (k) {
                      case -1:
                        return ERR_File_Read;
                      case LF:
                        return END_OF_LINE;
                      case CTRL_Z:
                        return END_OF_FILE;
                    }
                    return END_OF_LINE;


                } else
                    return END_ABORT;
             }
             line[(*lineLen)++] = (char)ch;
             extLineLen++;
         }
         prevCh = ch;
     }
}                                       /* LoadLine() */

/***    LoadFile
**
**  Synopsis:
**      bool = LoadFile(d)
**
**  Entry:
**      d - Notepad Document record for file to be loaded
**
**  Returns:
**      TRUE if the file was loaded and FALSE otherwise
**
**  Description:
**      Load file in memory and initialize blocks.
*/

BOOL
PASCAL
LoadFile(
    int doc
    )
{
    LPDOCREC d =&Docs[doc];
    char line[MAX_USER_LINE + 1];
    int offset; //Current offset of storage in block
    int lineLen;
    BYTE prevLength; //Previous line size
    HCURSOR hSaveCursor;
    WORD res;
    WORD nbLines = 0;

    //
    // Documents are now always read only. Because WinDbg is no longer and IDE, it
    // is strictly a debugger now.
    //

    hFileDoc = CreateFile(d->szFileName, GENERIC_READ, FILE_SHARE_READ, 
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE == hFileDoc) {
        ErrorBox( ERR_File_Open, (LPSTR)d->szFileName );
        return FALSE;
    }

    d->readOnly = TRUE;

    //Store file date

    if (!GetFileTimeByName( d->szFileName, NULL, NULL, &d->time ) ) {
        GetSystemTimeAsFileTime(&d->time);
    }

    if ((pszBufferDoc = DocAlloc(DISK_BLOCK_SIZE)) == NULL) {
        ErrorBox(SYS_Allocate_Memory);
        goto error1;
    }

    //Alloc first block

    if (!AllocateBlock(NULL, NULL, &d->FirstBlock)) {
        return FALSE;
    }

    //If we are in reload mode, and a message box shows up, a repaint
    //of the document being loaded is done, so force number of lines
    //to a value != 0

    d->NbLines = MAX_LINE_NUMBER;

    d->LastBlock = d->FirstBlock;
    dwBytesReadDoc = DISK_BLOCK_SIZE;
    dwOffsetDoc = DISK_BLOCK_SIZE;
    offset = 0;
    prevLength = 0;

    //Set the Hour glass cursor

    hSaveCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));

    //Read file line after line

    res = LoadLine(line, &lineLen, nbLines);
    while (res == END_OF_LINE) {

        //Truncate a file too large

        if (nbLines >= MAX_LINE_NUMBER - 1) {
            ErrorBox(ERR_Truncate_Doc);
            res = END_OF_FILE;
            break;
        }

        if (!StoreLine(line, lineLen, &offset, &prevLength, &d->LastBlock)) {
            res = END_ABORT;
            break;
        }

        res = LoadLine(line, &lineLen, ++nbLines);
    }

    //Take decisions

    switch (res) {

      case END_OF_FILE:

        //Store last line

        if (StoreLine(line, lineLen, &offset, &prevLength, &d->LastBlock)) {

            nbLines++;

            //Free memory

            if (!DocFree(pszBufferDoc))
                  InternalErrorBox(SYS_Free_Memory);

            //Restore cursor

            SetCursor(hSaveCursor);

            CloseHandle(hFileDoc);
            d->NbLines = nbLines;
            return TRUE;
        } else
              goto abort;

      case ERR_File_Read:
      case ERR_Not_A_Text_File:
        ErrorBox(res, (LPSTR)d->szFileName);
        //Fall through

      case END_ABORT:
        {
            LPBLOCKDEF pB;

          abort:
            while (TRUE) {

                pB = (d->LastBlock)->PrevBlock;
                if (!DocFree((LPSTR)d->LastBlock))
                      InternalErrorBox(SYS_Free_Memory);
                d->LastBlock = pB;
                if (pB == NULL)
                      break;
            }
            SetCursor(hSaveCursor);
            break;
        }

      default:
        Assert(FALSE);
        break;
    }

    //Restore cursor

    SetCursor(hSaveCursor);

    if (!DocFree(pszBufferDoc))
          InternalErrorBox(SYS_Free_Memory);

  error1: {
        CloseHandle(hFileDoc);
    }

    return FALSE;
}                                       /* LoadFile() */


// BUGBUG - dead code - kcarlos
#if 0
/***    MergeFile
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

BOOL FAR PASCAL MergeFile(LPSTR FileName, int view)
{
    HCURSOR hSaveCursor;
    char line[MAX_USER_LINE + 1];
    int offset; //Current offset of storage in block
    int lineLen; // length of line
    BYTE prevLength; //Previous line size
    LPVIEWREC v = &Views[view];
    WORD res;
    LPDOCREC d =&Docs[v->Doc];
    LPLINEREC pCurLine, pLastLine;
    LPBLOCKDEF pCurBlock, pNewBlock;
    long y;

    Assert( v->Doc >= 0);


    if ((hFileDoc = _lopen(FileName, OF_READ)) == HFILE_ERROR)
          return ErrorBox(ERR_File_Open, (LPSTR)FileName);

    if ((pszBufferDoc = DocAlloc(DISK_BLOCK_SIZE)) == NULL) {
        ErrorBox(SYS_Allocate_Memory);
        goto error1;
    }

    dwBytesReadDoc = DISK_BLOCK_SIZE;
    dwOffsetDoc = DISK_BLOCK_SIZE;
    lineLen = 0;

    //Delete selected text if any
    if (v->BlockStatus) {

        long XR, YR;

        GetBlockCoord (view, &(v->X), &(v->Y), &XR, &YR);
        DeleteStream(view, v->X, v->Y, XR, YR, FALSE);
    }

    //Set the Hour glass cursor
          hSaveCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //Insert first line

    res = LoadLine(line, &lineLen, d->NbLines);
    if (res == END_OF_FILE || res == END_OF_LINE) {

        //Add a CR+LF
        if (res == END_OF_LINE) {
            line[lineLen] = CR;
            line[lineLen + 1] = LF;
            lineLen += 2;
        }
        if (!InsertBlock(v->Doc, v->X, v->Y, (WORD) lineLen, line))
            goto error2;
    }
    else {

        if (res == ERR_File_Read || res == ERR_Not_A_Text_File)
              ErrorBox(res, (LPSTR)FileName);
        goto error2;
    }

    if (res != END_OF_FILE) {

        //Get current line status (we just inserted it)

        y = v->Y;
        if (!FirstLine(v->Doc, &pCurLine, &y, &pCurBlock))
              return FALSE;

        //Get next line (just after the line we just inserted)

        if (!NextLine(v->Doc, &pCurLine, &y, &pCurBlock))
              return FALSE;

        //Set offset to StoreLine start
              offset =  (LPSTR)pCurLine - (LPSTR)pCurBlock->Data;
        prevLength = pCurLine->PrevLength;

        //Split block in 2 blocks by first allocating a new block and then
        //copying right side of block in new block

        if (!AllocateBlock(pCurBlock, pCurBlock->NextBlock, &pNewBlock))
              return FALSE;

        pLastLine = (LPLINEREC)(pCurBlock->Data + pCurBlock->LastLineOffset);
        memmove(pNewBlock->Data, (LPSTR)pCurLine,
              (LPSTR)pLastLine - (LPSTR)pCurLine + pLastLine->Length);

        //Set new old block len and new new block len
              pCurBlock->LastLineOffset = (LPSTR)pCurLine - (LPSTR)pCurBlock->Data - pCurLine->PrevLength;
        pNewBlock->LastLineOffset = (LPSTR)pLastLine - (LPSTR)pCurLine;

        //Backward links next block with new one
        if (pCurBlock->NextBlock == NULL)
              d->LastBlock = pNewBlock;
        else
              (pCurBlock->NextBlock)->PrevBlock = pNewBlock;

        //Forward link current block with new block

        pCurBlock->NextBlock = pNewBlock;

        CloseLine(v->Doc, &pCurLine, y, &pCurBlock);

        //Read and store all lines in new blocks

        res = LoadLine(line, &lineLen, d->NbLines);
        while (res == END_OF_LINE) {

            //Truncate a file too large

            if (d->NbLines >= MAX_LINE_NUMBER - 1) {
                ErrorBox(ERR_Truncate_Doc);
                res = END_OF_FILE;
                break;
            }

            if (!StoreLine(line, lineLen, &offset, &prevLength, &pCurBlock)) {
                res = END_ABORT;
                break;
            }

            res = LoadLine(line, &lineLen, ++d->NbLines);
        }

        //Take decisions
        switch (res) {

          case END_OF_FILE:

            //Store last line
            if (StoreLine(line, lineLen, &offset, &prevLength, &pCurBlock)) {

                d->NbLines++;

                //Forward link of last allocated block with new block

                pCurBlock->NextBlock = pNewBlock;

                //Backward link of new block with last allocated block

                pNewBlock->PrevBlock = pCurBlock;
                ((LPLINEREC)(pNewBlock->Data))->PrevLength = (BYTE)(lineLen + LHD);

                //Free memory

                if (!DocFree(pszBufferDoc))
                      InternalErrorBox(SYS_Free_Memory);

                //Restore cursor

                SetCursor(hSaveCursor);

                //Check syntax if C

                if (d->language == C_LANGUAGE) {
                    d->lineTop = 0;
                    d->lineBottom = d->NbLines;
                    CheckSyntax(v->Doc);
                }
                SetVerticalScrollBar(view, TRUE);

                PosXY(view, v->X, v->Y, FALSE);
                InvalidateRect(v->hwndClient, (LPRECT)NULL, FALSE);

                CloseHandle (hFileDoc);
                return TRUE;
            }
            else
                goto abort;

          case ERR_File_Read:
          case ERR_Not_A_Text_File:
            ErrorBox(res, (LPSTR)FileName);
            //Fall through

          case END_ABORT:
            {

              abort:
                SetCursor(hSaveCursor);
                break;
            }

          default:
            Assert(FALSE);
            break;
        }

    } else
          InvalidateRect(v->hwndClient, (LPRECT)NULL, FALSE);

  error2: {
        SetCursor (hSaveCursor);
        if (!DocFree(pszBufferDoc))
              InternalErrorBox(SYS_Free_Memory);
    }

  error1: {
        CloseHandle (hFileDoc);
    }

    return FALSE;
}                                       /* MergeFile() */
#endif

// BUGBUG - dead code - kcarlos
#if 0
/***    CreateUntitled
**
**  Synopsis:
**      void = CreateUntitled(szFileName, i)
**
**  Entry:
**
**  Returns:
**      Nothing
**
**  Description:
**      Create or open a new document
**
*/

void NEAR PASCAL CreateUntitled(PSTR FileName, int i)
{
        char tmp[MAX_MSG_TXT];

        Dbg(LoadString(g_hInst, SYS_Untitled_File, tmp, MAX_MSG_TXT));
        sprintf((LPSTR)FileName, (LPSTR)tmp, i, (LPSTR)(szStarDotC + 1));
}
#endif


int PASCAL
OpenDocument(
    WORD    mode,
    WORD    type,
    int     doc,
    LPSTR   FileName,
    int     dupView,
    int     Preference,
    BOOL    bUserActivated
    )

/*++

Routine Description:

    This routine is used to create a new document or to duplicate
    the view of an existing document.

Arguments:

    mode        - Supplies MODE_DUPLICATE if the document is to be duplicated
                           MODE_RELOAD if the file is to be reloaded
                           MODE_CREATE if the document is to be created
    type        - Supplies the document type
    doc         - Supplies
    FileName    - Supplies a pointer to the name of the file for the document
    dupView     - Supplies the view to be duplicated (mode == MODE_DUPLICATE)
    Preference  - Supplies the view preference (-1 if none)
    bUserActivated - Indicates whether this action was initiated by the
                user or by windbg. The value is to determine the Z order of
                any windows that are opened.

Return Value:

    -1 on failure
    view number on success (>= 0)
    return-value - Description of conditions needed to return value. - or -

--*/

{
    LPLINEREC   pl;
    int         nView = 0;
    BOOL        create;
    int         n;
    int         language;
    LPDOCREC    d;
    LPVIEWREC   views;

    //
    //  In mode duplicate, we create a new nView being a copy of the previous
    //  nView of the same document
    //

    if (mode == MODE_DUPLICATE) {

        //
        // Search a free entry for the nView we will create
        //

        if ( (Preference != -1) && Views[ Preference ].Doc == -1 ) {
            nView = Preference;
        } else {
            for (nView = 0; (nView < MAX_VIEWS) && (Views[nView].Doc != -1); nView++);
        }

        if (nView == MAX_VIEWS) {
            ErrorBox(ERR_Too_Many_Opened_Views);
            return -1;
        }

        //
        // Copy parameters from previous nView
        //

        Assert( Docs[Views[dupView].Doc].FirstView >= 0);

        //
        //      Find the last nView for this document
        //

        n = dupView;
        while (Views[n].NextView != -1) {
            n = Views[n].NextView;
        }

        Assert(n < MAX_VIEWS);

        //
        //     Attach new nView to last one found
        //

        Views[nView] = Views[n];
        Views[n].NextView = nView;
        Views[nView].hwndClient = NULL;

        //
        //
        //

        Views[nView].iYTop = Views[dupView].iYTop;

        //
        //     Enlist nView in window menu
        //

        AddWindowMenuItem(Views[dupView].Doc, nView);

        return nView;

    } else if (mode == MODE_RELOAD) {
        nView = Docs[doc].FirstView;
    }

    //
    // First search a free entry for the document
    //

    if (mode != MODE_RELOAD) {
        for (doc = 0; (doc < MAX_DOCUMENTS) && (Docs[doc].FirstView != -1); doc++);
    }

    if (doc >= MAX_DOCUMENTS) {
        ErrorBox(ERR_Too_Many_Opened_Documents);
        return -1;
    }

    d = &Docs[doc];

    if (type == DOC_WIN) {

        //
        //      Check if file is not already loaded
        //

        if (mode == MODE_RELOAD) {
            DestroyDocument(doc);
            language = SetLanguage(doc);
        } else {
            if (FileName != NULL) {
                TCHAR szAbsolutePath[_MAX_PATH] = {0};

                Assert(sizeof(szAbsolutePath) == sizeof(d->szFileName));

                // We may have a relative path name to a file. Try to get the absolute path.
                if ( _fullpath(szAbsolutePath, FileName, sizeof(szAbsolutePath) ) ) {
                    // success
                    _tcscpy(d->szFileName, szAbsolutePath);
                } else {
                    // error. Use as is.
                    _tcscpy(d->szFileName, FileName);
                }

                for (n = 0; n < MAX_DOCUMENTS; n++) {
                    if (Docs[n].FirstView != -1
                        && _strcmpi(Docs[n].szFileName, d->szFileName) == 0) {

                        SetMessageText_StatusBar(ERR_File_Already_Loaded, STATUS_INFOTEXT, FileName);
                        MessageBeep(0);

                        //
                        //  Reactivate window
                        //

                        //SendMessage(g_hwndMDIClient, WM_MDIACTIVATE, (WPARAM) Views[Docs[n].FirstView].hwndFrame, 0L);
                        ActivateMDIChild(Views[Docs[n].FirstView].hwndFrame, bUserActivated);
                        return -1;
                    }
                }

                language = SetLanguage(doc);

            } else {
                // shouldn't happen anymore
                Assert(0);
// BUGBUG - dead code - kcarlos
#if 0
                //
                //  Initialize document record and first nView
                //

                register int i, j;

                for (i = 0; i < MAX_DOCUMENTS; i++) {
                    CreateUntitled(d->FileName, i + 1);
                    for (j = 0; j < MAX_DOCUMENTS; j++) {
                        if (j != doc &&
                            _strcmpi (d->szFileName, Docs[j].szFileName) == 0) {
                            break;
                        }
                    }
                    if (j >= MAX_DOCUMENTS) {
                        break;
                    }
                }

                language = C_LANGUAGE;
#endif
            }
        }
    } else {

        WORD winTitle;
        char rgch[MAX_MSG_TXT];

        language = NO_LANGUAGE;

        //
        //  Non Document type, Load window title from ressource
        //

        switch (type) {

        case DISASM_WIN:
            winTitle = SYS_DisasmWin_Title;
            break;       
        case COMMAND_WIN:
            winTitle = SYS_CmdWin_Title;
            break;
        case MEMORY_WIN:
            winTitle = SYS_MemoryWin_Title;
            break;
        default:
            Assert(FALSE);
            return -1;
            break;
        }
        Dbg(LoadString(g_hInst, winTitle, rgch, MAX_MSG_TXT));
        RemoveMnemonic(rgch, d->szFileName);

        if (type == MEMORY_WIN) {
            lstrcat (d->szFileName,"(");
            lstrcat (d->szFileName,TempMemWinDesc.szAddress);
            lstrcat (d->szFileName,")");
        }
    }

    //
    //  Then search a free entry for the first nView we will create
    //

    if (mode != MODE_RELOAD) {

        if ( (Preference != -1) && Views[ Preference ].Doc == -1 ) {
            nView = Preference;
        } else {
            for (nView = 0; nView < MAX_VIEWS && Views[nView].Doc != -1; nView++);
        }
        if (nView == MAX_VIEWS) {
            ErrorBox(ERR_Too_Many_Opened_Views);
            return -1;
        }
    }

    //
    // Check if file exist
    //

    if (mode == MODE_CREATE || type != DOC_WIN) {
        create = TRUE;
    } else {
        if (mode == MODE_OPEN || FileExist(FileName)) {
            create = FALSE;
        } else {
            if (mode == MODE_OPENCREATE) {
                // Tell the user that the file does not exist.

                // kcarlos
                // BUGBUG
                // Quick hack. To fix a bug, hopefully this all disappear soon.
                VarMsgBox(hwndFrame, SYS_Does_Not_Exist_Create, MB_OK, FileName);
                return -1;
            } else {
                create = FALSE;
            }
        }
    }

    d->readOnly = FALSE;
    d->docType = type;
    d->language = (WORD) language;
    d->untitled = (FileName == NULL);
    d->ismodified = FALSE;

    if (create) {

        LPBLOCKDEF pb;

        //
        //  Initialize the file with a null-string
        //


        d->LastBlock = d->FirstBlock = (LPBLOCKDEF)DocAlloc(sizeof(BLOCKDEF));
        GetSystemTimeAsFileTime(&d->time);

        if (d->FirstBlock == NULL) {
            ErrorBox(SYS_Allocate_Memory);
            return -1;
        }

        pb = d->FirstBlock;

        //
        //  Initialize first block
        //

        pb->PrevBlock = pb->NextBlock = NULL;
        pb->LastLineOffset = 0;

        //
        // Initialize first line
        //

        pl = (LPLINEREC)pb->Data;
        pl->PrevLength = 0;
        pl->Length = LHD;
        pl->Status = 0;

        d->NbLines = 1;         // We start with one null line

    } else {

        //
        // Load the file and check if it's a valid one
        //

        if (!LoadFile(doc)) {
            return -1;
        }
    }

    //
    // Initialize current pointers
    //

    d->CurrentBlock = d->FirstBlock;
    d->CurrentLine = 0;
    d->CurrentLineOffset = 0;
    pl = (LPLINEREC)(d->FirstBlock->Data);
    ExpandTabs(&pl);

    //
    // Undo/redo part
    //

    d->undo.h = 0;
    d->redo.h = 0;
    d->playCount = REC_CANNOTUNDO;
    if (g_contGlobalPreferences_WkSp.m_dwUndoResize == 0 || type != DOC_WIN) {
        d->recType = REC_STOPPED;
    } else {
        d->recType = REC_UNDO;
    }
    CreateRecBuf(doc, REC_UNDO, g_contGlobalPreferences_WkSp.m_dwUndoResize);

    if (mode == MODE_RELOAD) {
        RefreshWindowsTitle(doc);
    } else {

        //
        //  Initialize nView part
        //

        views = &Views[nView];
        views->NextView = -1;
        views->X = views->Y = 0;
        views->hwndClient = views->hwndFrame = NULL;
        views->hScrollBar = g_contGlobalPreferences_WkSp.m_bHorzScrollBars;
        views->vScrollBar = g_contGlobalPreferences_WkSp.m_bVertScrollBars;
        views->scrollFactor = 0;
        views->iYTop = -1;

        //
        //  Everything is OK, add title in window menu and return the nView
        //

        d->FirstView = nView;
        views->Doc = doc;
        AddWindowMenuItem(doc, nView);
    }

    //
    // Check syntax if C
    //

    if (d->language == C_LANGUAGE) {
        d->lineTop = 0;
        d->lineBottom = d->NbLines;
        CheckSyntax(doc);
    }

    return nView;
}                                       // OpenDocument()


// BUGBUG - dead code - kcarlos
#if 0
/***    SaveDocument
**
*/

//Save document with (possibly) a new file name
BOOL FAR PASCAL SaveDocument(int doc, LPSTR FileName)
{
    LPLINEREC pl;
    LPBLOCKDEF pb;
    long y;
    register WORD i;
    WORD len;
    HCURSOR hSaveCursor;
    LPDOCREC d = &Docs[doc];

    //Set the Hour glass cursor
    hSaveCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //Create file
    if (HFILE_ERROR == (hFileDoc = _lcreat(FileName, 0)) ) {
        ErrorBox(ERR_File_Create, (LPSTR)FileName);
        goto error0;
    }

    //Allocate space for Disk Buffer

    if ((pszBufferDoc = DocAlloc(DISK_BLOCK_SIZE)) == NULL) {
        ErrorBox(SYS_Allocate_Memory);
        goto error1;
    }

    dwOffsetDoc = 0; y = 0;

    //Get first line
    if (!FirstLine (doc, &pl, &y, &pb))
          return FALSE;

    //Save each line of file
    while (TRUE) {

        //Remove trailing blanks
        len = (WORD) (pl->Length - LHD);
        {
            TCHAR *pch1;

            pch1 = pl->Text + (pl->Length - LHD);

            while (pch1 > pl->Text) {
                pch1 = CharPrev(pl->Text, pch1);
                if (*pch1 != ' ' && *pch1 != TAB) {
                    break;
                }
                len --;
            }
        }

        //Write line
        i = 0;
        while (i < len) {

            PutChar(FileName, pl->Text[i]);
            i++;
        }

        if (y >= d->NbLines) {
            if (dwOffsetDoc && !Flush(FileName))
                goto error2;
            break;
        }
        else {
            PutChar (FileName, CR);
            PutChar (FileName, LF);
            if (!NextLine (doc, &pl, &y, &pb))
                  goto error2;
        }
    }

    CloseHandle (hFileDoc);

    //Update internal file date
    GetSystemTimeAsFileTime(&d->time);

    CloseLine(doc, &pl, y, &pb);

    d->ismodified = FALSE;
    RefreshWindowsTitle(doc);

    SetCursor (hSaveCursor);

    return TRUE;

  error2:
    CloseLine(doc, &pl, y, &pb);
    if (!DocFree(pszBufferDoc))
          InternalErrorBox(SYS_Free_Memory);

  error1:
    CloseHandle (hFileDoc);

  error0:
    SetCursor (hSaveCursor);
    return FALSE;
}                                       /* SaveDocument() */
#endif
