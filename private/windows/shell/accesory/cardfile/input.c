#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

HWND hDlgFind = NULL;
HANDLE hEditCurs;

int fCanPrint = FALSE;
HANDLE hIndexInstance;
TCHAR szPrinter[128];
TCHAR szDec[2];

#ifndef OLE_20
CHAR szSaveName[] = "SaveName";
#else
TCHAR szSaveName[] = TEXT("SaveName");
#endif

/* OLE libraries */
int cOleWait = 0;

#ifndef OLE_20
CHAR    szPStatic[] = "Static";
CHAR    szPStdFile[] = "StdFileEditing";
#else
TCHAR   szPStatic[] = TEXT("Static");
TCHAR   szPStdFile[] = TEXT("StdFileEditing");
#endif

NOEXPORT BOOL NEAR DisplayBusyMessage(LPOLEOBJECT lpObject);


DWORD IndexInput(
    HWND hWindow,
    int event)
{
    TCHAR *pBuf;
    LPCARDHEADER Cards;
    int i;
    TCHAR buf1[50], szMsg[300];
    RECT rect;
    long ltmp;
    BOOL fIsCard;
    HMENU hMenu;
    LPOLEOBJECT lpObjectSave = NULL;
    TCHAR szFile[PATHMAX];
    WORD fType;                     /* requested file type of target */
#ifndef OLE_20
    CHAR aszBuf[PATHMAX];
#endif

    switch(event)
    {
        case ABOUT:
            if (ShellAbout (hWindow, szCardfile, TEXT(""),
                            LoadIcon (hIndexInstance, (LPTSTR) INDEXICON)) == -1)
                IndexOkError (EINSMEMORY);
            break;

        case EXIT:
            if (CheckForBusyObjects())
                break;
            PostMessage (hWindow, WM_CLOSE, 0, 0L);
            break;

        case NEW:
            if (CheckForBusyObjects())
                break;
            MenuFileNew();
#ifdef JAPAN    //KKBUGFIX     //  added  18 Aug. 1992 by Hiraisi (in Japan)
            fReadOnly = FALSE;
#endif
            break;

        case OPEN:
            if (CheckForBusyObjects())
                break;
            MenuFileOpen();
            break;

        case MERGE:        /* merge in another file */
            if (CheckForBusyObjects())
                break;
            MenuFileMerge();
            break;

        case PAGESETUP:        /* page setup options */
            PutUpDB(DTPAGE);
            break;

        case PRINTSETUP:
            PrinterSetupDlg(hWindow);
            break;

        case PRINT:
            if (CheckForBusyObjects())
                break;
            UpdateEmbObject(&CurCard, MB_YESNO);
            PrintCards(1);
            break;

        case PRINTALL:
            if (CheckForBusyObjects())
                break;
            if (CardPhone == CCARDFILE)
            {
                UpdateEmbObject(&CurCard, MB_YESNO);
                PrintCards(cCards);
            }
            else
                PrintList();
            break;

        case SAVE:
            if (CheckForBusyObjects())
                break;
            if (fReadOnly)
            {
                BuildAndDisplayMsg (E_CANTSAVETOREADONLYFILE, CurIFile);
                goto SaveAs;
            }
            if (CurIFile[0])
            {
                lstrcpy (szFile, CurIFile);
                fType= fFileType;         // Save: is going to use this
                goto Save;
            }

        /* fall through... */

        case SAVEAS:
            if (CheckForBusyObjects())
                break;
SaveAs:
            if (!MyGetSaveFileName(szFile, &fType))
                break;
            if (fReadOnly && lstrcmpi (szFile, CurIFile) == 0)
            {
                /* When a file is readonly(because of sharing/readonly attribute,
                 * allow saving to a different file. But if the user does not
                 * specify a new filename, complain.
                 */
                BuildAndDisplayMsg(E_CANTSAVETOREADONLYFILE, szFile);
                goto SaveAs;
            }
            UpdateWindow(hIndexWnd);
Save:
            SetCursor(hWaitCurs);
            if (CardPhone == CCARDFILE)
                ltmp = SendMessage(hEditWnd, EM_GETSEL, 0, 0L);

            /* If an embedded object is open for editing, try to update */
            if (UpdateEmbObject(&CurCard, MB_YESNOCANCEL) == IDCANCEL)
                break;

            /* Save the current object.
             *
             * We do this so that the server will not be dismissed
             * when the file is saved.  We don't just clone into
             * lpObjectSave because the server will be dismissed
             * if an embedded object is deleted; we must actually
             * maintain the object that is currently being edited.
             */
            if (fOLE && (lpObjectSave = CurCard.lpObject) &&
                OleError (OleClone (lpObjectSave, lpclient,
                                    lhcdoc, szSaveName,
                                    (LPOLEOBJECT FAR *)&(CurCard.lpObject))))
            {
                CurCard.lpObject = lpObjectSave;
                lpObjectSave = NULL;
            }

            /* In phone mode, no need to save the current card
             * In card mode should save the current card. */
            if (CardPhone == PHONEBOOK || SaveCurrentCard(iFirstCard))
            {
                if (fType == OLD_FORMAT)
                {
                    LoadString(hIndexInstance, W_WILLDELETEOBJECTS, szMsg, CharSizeOf(szMsg));

                    if (MessageBox(hIndexWnd, szMsg, szCardfile, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
                        break;
                }

                if (fType != UNICODE_FILE)
                {
                   if (MyIsTextUnicode ())
                   {
                      LoadString(hIndexInstance, E_UNICODETEXT, szMsg, CharSizeOf(szMsg));

                      if (MessageBox(hIndexWnd, szMsg, szWarning, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
                          break;
                   }
                }

#ifndef OLE_20
                WideCharToMultiByte (CP_ACP, 0, szFile, -1, aszBuf, sizeof(aszBuf), NULL, NULL);

                if (fOLE && OLE_OK != OleRenameClientDoc (lhcdoc, aszBuf))
#else
                if (fOLE && OLE_OK != OleRenameClientDoc (lhcdoc, szFile))
#endif
                    ErrorMessage (W_FAILED_TO_NOTIFY);

                if (WriteCardFile (szFile, fType))
                {
                    SetCaption ();

                    if (fOLE && OLE_OK != OleSavedClientDoc (lhcdoc))
                        ErrorMessage(W_FAILED_TO_NOTIFY);

                    Fdelete (TempFile);
                    MakeTempFile ();
                }
                /* if write fails, due to can't create or memfull,
                 * return to saveas dialog box.
                 */
                else
                {
                    /* We failed, rename the document back */
#ifndef OLE_20
                    WideCharToMultiByte (CP_ACP, 0, CurIFile, -1, aszBuf, sizeof(aszBuf), NULL, NULL);

                    if (fOLE && OLE_OK != OleRenameClientDoc (lhcdoc, aszBuf))
#else
                    if (fOLE && OLE_OK != OleRenameClientDoc (lhcdoc, CurIFile))
#endif
                        ErrorMessage (W_FAILED_TO_NOTIFY);

                    if (lpObjectSave)
                    {
                        OleError (OleDelete (CurCard.lpObject));
                        CurCard.lpObject = lpObjectSave;
                    }

                    SetCurCard (iFirstCard);
                    goto SaveAs;
                }

                if (CardPhone == CCARDFILE)
                {
                    SetCurCard(iFirstCard);
                    SendMessage(hEditWnd, EM_SETSEL, LOWORD(ltmp), HIWORD(ltmp));
                }

                if (lpObjectSave)
                {
                    OleError(OleDelete(CurCard.lpObject));
                    CurCard.lpObject = lpObjectSave;
                }
            }
            SetCursor(hArrowCurs);

            break;

        case CCARDFILE:
        case PHONEBOOK:
            if (event == CardPhone)
                break;        /* nothing new */

            /* toggle the menu marks */
            hMenu = GetMenu(hWindow);
            CheckMenuItem(hMenu, CardPhone, MF_UNCHECKED);
            CheckMenuItem(hMenu, event, MF_CHECKED);

            CardPhone = event;    /* keep track of mode we are in */
            fIsCard = (CardPhone == CCARDFILE);

            /*
             * change the main windows background color for the current
             * mode.  if we go into list mode full screen we have to
             * paint the region at the bottom of the screen.  in
             * card mode we set to the same color as the card window
             * to avoid ugly repaint junk.
             */
#if defined( WIN32 )
            SetClassLong(hWindow, GCL_HBRBACKGROUND,
                fIsCard ? COLOR_APPWORKSPACE + 1 : COLOR_WINDOW + 1);
#else
            SetClassWord(hWindow, GCW_HBRBACKGROUND,
                fIsCard ? COLOR_APPWORKSPACE + 1 : COLOR_WINDOW + 1);
#endif

            if (fFullSize)
            {
                GetClientRect(hIndexWnd, &rect);
                rect.top = rect.bottom - CharFixHeight;
                InvalidateRect(hIndexWnd, &rect, TRUE);
            }

            if (fIsCard)
            {
                SetCurCard(iFirstCard);     /* from list mode selection */
                ShowWindow(hListWnd, SW_HIDE);
                ShowWindow(hCardWnd, SW_SHOW);
            }
            else
            {
                ShowWindow(hCardWnd, SW_HIDE);
                SaveCurrentCard(iFirstCard);
                InitPhoneList(hListWnd, iFirstCard);
                SizeListWindow();
                ShowWindow(hListWnd, SW_SHOW);
            }

            SetFocus(fIsCard ? hCardWnd : hListWnd);
            SetWindowText(hLeftWnd, fIsCard ? szCardView : szListView);
            break;

        case UNDO:
            if (EditMode == I_TEXT)
                SendMessage(hEditWnd, EM_UNDO, 0, 0L);
            else
            {
                if (CheckForBusyObjects() || InsertObjectInProgress())
                    break;

                if (lpObjectUndo)
                {
                    PicDelete (&CurCard);
                    CurCard.lpObject = lpObjectUndo;
                    CurCard.otObject = otObjectUndo;
#ifndef OLE_20
                    wsprintfA (szObjectName, szObjFormat, CurCard.idObject + 1);
#else
                    wsprintfW (szObjectName, szObjFormat, CurCard.idObject + 1);
#endif
                    OleRename (CurCard.lpObject, szObjectName);
                    lpObjectUndo = NULL;
                    InvalidateRect(hEditWnd, (LPRECT)&(CurCard.rcObject), TRUE);
                    CurCardHead.flags |= FDIRTY;

                    /* If a link is undone, try to reconnect */
                    if (CurCard.otObject == LINK
                         && OleError (OleReconnect (CurCard.lpObject)))
                        ErrorMessage (E_FAILED_TO_RECONNECT_OBJECT);
                }
            }
            break;

        case HEADER:
            if (CardPhone == PHONEBOOK)
            {
                SetCurCard(iFirstCard);
                SaveCurrentCard(iFirstCard);
            }
            if (pBuf = PutUpDB(DTHEADER))
            {
                /* Preserve the current Index line for restoration later */
                lstrcpy(SavedIndexLine, CurCardHead.line);
                lstrcpy(CurCardHead.line, pBuf);

                /* !!! Should we be destroying the object here? */
                DeleteCard(iFirstCard);     /* take it out of it's current position */
                iFirstCard = AddCurCard();  /* and put it back in the right place */
                fFileDirty = TRUE;
                InvalidateRect(hWindow, NULL, TRUE);
                LocalFree((HANDLE)pBuf);
            }
            break;

        case RESTORE:
            /* Restore the index line from temporary storage */
            Cards = (LPCARDHEADER) GlobalLock(hCards);
            lstrcpy(Cards[iFirstCard].line, SavedIndexLine);
            lstrcpy(CurCardHead.line, SavedIndexLine);
            InvalidateRect(hWindow, NULL, TRUE);
            GlobalUnlock(hCards);
            if (CurCard.lpObject)
                PicDelete(&CurCard);
            SetCurCard(iFirstCard);
            InvalidateRect(hEditWnd, NULL, TRUE);
            break;

        case CUT:
        case COPY:
            if (EditMode == I_OBJECT &&
                    (CheckForBusyObjects() || InsertObjectInProgress()))
                break;
            DoCutCopy(event);
            break;

        case PASTE:
        case PASTELINK:
            if (CheckForBusyObjects() || InsertObjectInProgress())
                break;

            if (CurCard.lpObject && CurCard.otObject == EMBEDDED &&
                OleQueryOpen(CurCard.lpObject) == OLE_OK)
            {
                TCHAR szMsg[300];

                LoadString(hIndexInstance, W_REPLACEOPENOBJECT, szMsg, CharSizeOf(szMsg));

                MessageBox(hIndexWnd, szMsg, szWarning, MB_OK);
                break;
            }

            DoPaste(event);
            break;

        case IDM_PASTESPECIAL:
            if (CheckForBusyObjects() || InsertObjectInProgress())
                break;
            DoPasteSpecial();
            break;

        case IDM_INSERTOBJECT:
            if (CheckForBusyObjects() || InsertObjectInProgress())
                break;
            InsertObject();
            break;

        case I_TEXT:
        case I_OBJECT:
            if (event != EditMode)
            {
#ifdef RIGHT
                /* Should we add a field to Card to be used as the
                 * selection area?  It'd make a lot of sense to
                 * disable/re-enable the selection area if you switch
                 * to and from picture mode.
                 */
                if (event == I_OBJECT)
                {
                    DWORD lSel;

                    /* Remove the selection area if going to picture mode */
                    lSel = SendMessage(hEditWnd, EM_GETSEL, 0, 0L);
                    lSel = (LOWORD(lSel) | (lSel << 8));
                    SendMessage(hEditWnd, EM_SETSEL, 0, lSel);
                }
#endif
                EditMode = event;
#ifdef JAPAN    //KKBUGFIX     // #3082: 02/02/1993:  Disabling IME while Picture mode
                if (fNowFocus)
                {
                    if (event == I_TEXT)
                        EnableIME (TRUE);
                    else if (CardPhone == CCARDFILE)
                        EnableIME (FALSE);
                }
#endif
            }
            break;

        //bugbug - get rid of IDD_FONT resource and lpTemplateName initialization
        case IDM_SETFONT:
        {
            RECT        rect;    // to force re-draw
            CHOOSEFONT  cf;
            HDC         hDisplayDC;  // DC of video
            LOGFONT     NewFontStruct;   // local copy until succeeds

            /* calls the font chooser (in commdlg)
             */
            NewFontStruct= FontStruct;  // initialize our font structure
            cf.lStructSize = sizeof(CHOOSEFONT);
            cf.hwndOwner = hWindow;
            cf.hInstance = hIndexInstance;
            cf.lpTemplateName = (LPTSTR) MAKEINTRESOURCE(IDD_FONT);
            cf.hDC = NULL;
            cf.lpLogFont = &NewFontStruct;
            cf.Flags = CF_SCREENFONTS         |
                       CF_NOVECTORFONTS       |
                       CF_INITTOLOGFONTSTRUCT |
                       0;
            cf.lpfnHook = (LPCFHOOKPROC) NULL;
            if( !(hDisplayDC= GetDC(NULL)) )
            {
               break;
            }
            NewFontStruct.lfHeight= -( (iPointSize * GetDeviceCaps(hDisplayDC,LOGPIXELSY) ) / 720 );
            ReleaseDC( NULL, hDisplayDC );

            if (ChooseFont (&cf))
            {
                HFONT hNewFont;
                hNewFont= CreateFontIndirect( &NewFontStruct );
                if( !hNewFont )
                {
                   break;
                }
                // Update everybody, and write new font data to the registry
                FontStruct= NewFontStruct;
                SetGlobalFont( hNewFont, cf.iPointSize );
                SaveGlobals();
                // windows that need new font

                SendMessage(hEditWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
                SendMessage(hListWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

                 // fake a window move to re-draw everything
                 // also forces re-calc of where cards are
                 // send size message so card positions are re-calced

                 GetWindowRect( hCardWnd, &rect );
                 SendMessage( hCardWnd, WM_SIZE, SIZE_RESTORED,
                              (rect.bottom-rect.top) << 16 + rect.right-rect.left );
            }
            break;
        }

        case ADD:
            if (CheckForBusyObjects())
                break;
            if (pBuf = PutUpDB(DTADD))
            {
                /* Ensure no busy objects for sure */
                if (CheckForBusyObjects())
                    break;
                if (!ExpandHdrs(1))
                    break;
                /* In phone mode, no need to save the current card
                 * In card mode should save the current card. */
                if (CardPhone == PHONEBOOK || SaveCurrentCard(iFirstCard))
                {
                    MakeBlankCard();
                    /* Make sure pBuf has correct length */
                    /* Truncate it if longer.       */
                    if (lstrlen(pBuf) > LINELENGTH)
                        pBuf[LINELENGTH] = 0;
                    lstrcpy(CurCardHead.line, pBuf);
                    CurCardHead.flags |= (FDIRTY + FNEW);
                    iFirstCard = AddCurCard();

                    if (CardPhone == PHONEBOOK)
                        SaveCurrentCard(iFirstCard);
#ifdef JAPAN    //KKBUGFIX     //  added  07 Sep. 1992 by Hiraisi (in Japan)
                    // I referrd to input.c of WIN30J (BuildMessages())
                    // sometimes card number do not update
                    SetNumOfCards();
#endif
                    InvalidateRect(hWindow, NULL, TRUE);
                }
                LocalFree((HANDLE)pBuf);
            }
            break;

        case CARDDELETE:
            if (CheckForBusyObjects())
                break;
            if (CardPhone == PHONEBOOK)
            {
                SetCurCard(iFirstCard);
                SaveCurrentCard(iFirstCard);
            }
            LoadString(hIndexInstance, IDELCURCARD, buf1, CharSizeOf(buf1));
            MergeStrings(buf1, CurCardHead.line, szMsg);
            if (MessageBox(hIndexWnd, szMsg, szWarning, MB_OKCANCEL | MB_ICONEXCLAMATION) != IDOK)
                break;

            if (CurCard.lpObject)
            {
                if (OleQueryOpen(CurCard.lpObject) == OLE_OK)
                {
                    TCHAR szMsg[200];

                    LoadString(hIndexInstance, W_DELETEOPENOBJECT, szMsg, CharSizeOf(szMsg));
                    if (MessageBox(hIndexWnd, szMsg, szWarning, MB_OKCANCEL) == IDCANCEL)
                        break;
                }

                PicDelete(&CurCard);
            }
            if (cCards > 1)
            {
                DeleteCard(iFirstCard);
                if (iFirstCard == cCards)
                    iFirstCard = (CardPhone == CCARDFILE ? 0 : cCards - 1);
                if (CardPhone == CCARDFILE)
                    SetCurCard(iFirstCard);
                /* shrinking, so don't have to check (YOU DO FOR WIN32!!)*/
                hCards = GlobalReAlloc(hCards, cCards * sizeof(CARDHEADER),GMEM_MOVEABLE);
            }
            else
            {
                MakeBlankCard();
                Cards = (LPCARDHEADER) GlobalLock(hCards);
                Cards[0] = CurCardHead;
                GlobalUnlock(hCards);
                if (CardPhone == PHONEBOOK)
                {
                    SaveCurrentCard(iFirstCard);
                    /* update card list */
                    InitPhoneList(hListWnd, 0);
                }
            }

            fFileDirty = TRUE;
#ifdef JAPAN    //KKBUGFIX     //  added  07 Sep. 1992 by Hiraisi (in Japan)
                    // I referrd to input.c of WIN30J (BuildMessages())
                    // sometimes card number do not update
            SetNumOfCards();
#endif
            InvalidateRect(hWindow, (LPRECT)NULL, TRUE);

            break;

        case DUPLICATE:
            if (CheckForBusyObjects())
                break;
            if (!ExpandHdrs(1))
                break;
            /* In phone mode, no need to save the current card
             * In card mode should save the current card. */
            if (CardPhone == PHONEBOOK || SaveCurrentCard(iFirstCard))
            {
                SetCurCard(iFirstCard);
                CurCardHead.flags |= (FDIRTY + FNEW);
                iFirstCard = AddCurCard();
                if (CardPhone == PHONEBOOK)
                {
                    SaveCurrentCard(iFirstCard);
                    GetClientRect(hIndexWnd, (LPRECT)&rect);
                    i = rect.bottom / CharFixHeight;
                    if (!i)
                        i = 1;
                    iTopCard = min(iFirstCard-(i-1)/2, (cCards - i));
                    if (iTopCard < 0)
                        iTopCard = 0;
                }
#ifdef JAPAN    //KKBUGFIX     //  added  07 Sep. 1992 by Hiraisi (in Japan)
                    // I referrd to input.c of WIN30J (BuildMessages())
                    // sometimes card number do not update
                SetNumOfCards();
#endif
                InvalidateRect(hWindow, (LPRECT)NULL, TRUE);
                fFileDirty = TRUE;
            }
            CurCard.idObject = idObjectMax++;
            break;

        case DIAL:
            if (CardPhone == PHONEBOOK)
            {
                SetCurCard(iFirstCard);
                SaveCurrentCard(iFirstCard);
            }
            if (pBuf = PutUpDB(DTDIAL))
            {
                DoDial(pBuf);
                LocalFree((HANDLE)pBuf);
            }
            break;

        case GOTO:
            if (CheckForBusyObjects())
                break;
            if (pBuf = PutUpDB(DTGOTO))
            {
                DoGoto(pBuf);
                LocalFree((HANDLE)pBuf);
            }
            break;

        case FINDNEXT:
            if (szSearch[0])
            {
                fRepeatSearch = TRUE;
                if (fReverse)
                    ReverseSearch();
                else
                    ForwardSearch();
                break;
            }

        case FIND:
            if (CheckForBusyObjects())
                break;
            if (FR.lpstrFindWhat = GlobalLock(hFind))
                hDlgFind = FindText((LPFINDREPLACE)&FR);
            break;

        case ID_USEHELP:
            if(!WinHelp(hIndexWnd, NULL, HELP_HELPONHELP, (DWORD)0))
                IndexOkError(EINSMEMORY);

            break;

        case ID_INDEX:
            if(!WinHelp(hIndexWnd,  szHelpFile, HELP_INDEX, (DWORD)0))
                IndexOkError(EINSMEMORY);

            break;

        case ID_SEARCH:
            if(!WinHelp(hIndexWnd,  szHelpFile, HELP_PARTIALKEY, (DWORD)TEXT("")))
                IndexOkError(EINSMEMORY);

            break;

        default:
            if (fOLE)
                return OleMenu(event);
            break;
    }
    return 0L;
}

TCHAR * PutUpDB(
    int idb)
{
    FARPROC lpdbProc;
    int wResult;

    DBcmd = idb;
    switch(idb)
    {
        case DTLINKS:
            lpdbProc = lpfnLinksDlg;
            break;

        case DTDIAL:
            lpdbProc = lpfnDial;
            break;

        case DTPAGE:
            lpdbProc = lpfnPageDlgProc;
            break;

        default:     /* find, find next, goto. all share this one */
            lpdbProc = lpDlgProc;
            break;
    }

    wResult = DialogBox (hIndexInstance, (LPTSTR) MAKEINTRESOURCE(idb), hIndexWnd, (DLGPROC)lpdbProc);

    if (wResult == -1)
    {
        MessageBox(NULL, NotEnoughMem, szCardfile, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
        wResult = 0;
    }

    return((TCHAR *)wResult);
}

#if defined WIN32
int MapPtToCard(
    MYPOINT pts)
#else
int MapPtToCard(
    MYPOINT pt)
#endif
{
    int idCard;
    int xCur;
    int yCur;
    int i;
    RECT rect;
#if defined(WIN32)
   POINT pt ;

   MYPOINTTOPOINT( pt, pts ) ;
#endif

    yCur = yFirstCard - (cScreenCards - 1)*ySpacing;
    xCur = xFirstCard + (cScreenCards - 1)* (2 * CharFixWidth);
    idCard = (iFirstCard + cScreenCards-1) % cCards;

    for (i = 0; i < cScreenCards; ++i)
    {
        SetRect((LPRECT)&rect, xCur+1, yCur+1, xCur+CardWidth-1, yCur+CharFixHeight+1);
        if (PtInRect((LPRECT)&rect, pt))
            return(idCard);
        SetRect((LPRECT)&rect, rect.right - 2*CharFixWidth + 2, rect.top,rect.right,rect.top+CardHeight-2);
        if (PtInRect((LPRECT)&rect, pt))
            return(idCard);
        xCur -= (2*CharFixWidth);
        yCur += ySpacing;
        idCard--;
        if (idCard < 0)
            idCard = cCards - 1;
    }
    return(-1);
}

DWORD OleMenu(
    int event)
{
    BOOL fTookAction = TRUE;
    WORD fOleErrMsg;
    LPOLEOBJECT lpObject;

    switch (event)
    {
        case PLAY:
            if (CheckForBusyObjects())
                break;
            CurCardHead.flags |= FDIRTY;
            if (OleError(OleActivate(CurCard.lpObject, OLE_PLAY, TRUE, TRUE, hIndexWnd, &(CurCard.rcObject))))
            {
                ErrorMessage(E_FAILED_TO_LAUNCH_SERVER);
            }
            break;

        case EDIT:
            if (CheckForBusyObjects())
                break;
            CurCardHead.flags |= FDIRTY;
            if (OleError(OleActivate(CurCard.lpObject, OLE_EDIT, TRUE, TRUE, hIndexWnd, &(CurCard.rcObject))))
            {
                ErrorMessage(E_FAILED_TO_LAUNCH_SERVER);
            }
            break;

        case UPDATE:
            if (OleError(OleUpdate(CurCard.lpObject)) == FOLEERROR_NOTGIVEN)
                ErrorMessage(E_FAILED_TO_UPDATE);
            break;

        case FREEZE:
            /* Make the object static (using a bogus item name) */
            if (fOleErrMsg = OleError(OleObjectConvert(CurCard.lpObject,
                          szPStatic, lpclient, lhcdoc, szSaveName, &lpObject)))
            {
                if (fOleErrMsg == FOLEERROR_NOTGIVEN)
                    ErrorMessage(E_FAILED_TO_FREEZE);
            }
            else
            {
                WaitForObject (CurCard.lpObject);
                if (OleError(OleDelete(CurCard.lpObject)) == FOLEERROR_NOTGIVEN)
                    ErrorMessage(E_FAILED_TO_DELETE_OBJECT);

#ifndef OLE_20
                wsprintfA (szObjectName, szObjFormat, CurCard.idObject + 1);
#else
                wsprintfW (szObjectName, szObjFormat, CurCard.idObject + 1);
#endif
                OleRename (lpObject, szObjectName);

                CurCard.lpObject = lpObject;
                CurCard.otObject = STATIC;
                CurCardHead.flags |= FDIRTY;
                InvalidateRect (hEditWnd, NULL, TRUE);
            }
            break;

        case LINKSDIALOG:
            if (CheckForBusyObjects())
                break;
            PutUpDB(DTLINKS);
            break;

        default:
            if (event >= OLE_VERB && event <= OLE_VERBMAX)
            {
                if (CheckForBusyObjects())
                    break;
                CurCardHead.flags |= FDIRTY;

                if (CurCard.otObject == EMBEDDED)
                    PicSaveUndo(&CurCard);
                if (OleError(OleActivate(CurCard.lpObject, event-OLE_VERB, TRUE, TRUE, hIndexWnd, &(CurCard.rcObject))))
                {
                    ErrorMessage(E_FAILED_TO_LAUNCH_SERVER);
                }
                break;
            }

            fTookAction = FALSE;
            break;
    }
    return fTookAction;
}

/*
 * this and the ScrollIndexVert() do the scrolling of the Index window
 * when scroll bars are present.  you will note that the position returned
 * by THUMB type messages gets reversed because we move the scroll box
 * opposite of the direction we scroll the window.  See INDEX.C in the
 * WM_SIZE section to see how the scroll limits are defined.
 *
 * note: SB_ENDSCROLL is not supported because it returns ranges that
 * are opposite of those that we work with.
 */
void ScrollIndexHorz(
    HWND hWindow,
    int cmd,
    int pos)
{
    int range_min, range_max;
    int xSave = xFirstCard;

    GetScrollRange(hWindow, SB_HORZ, &range_min, &range_max);

    switch (cmd)
    {
        case SB_LINEUP:
            xFirstCard += CharFixWidth;
            break;

        case SB_LINEDOWN:
            xFirstCard += -CharFixWidth;
            break;

        case SB_PAGEUP:
            xFirstCard += xCardWnd/2;
            break;

        case SB_PAGEDOWN:
            xFirstCard += -xCardWnd/2;
            break;

        case SB_THUMBPOSITION:
            xFirstCard = range_max - (pos - range_min);
            break;

        default:
            break;
    }

    xFirstCard = max(range_min, min(xFirstCard, range_max));

    SetScrollPos(hWindow, SB_HORZ, range_max - (xFirstCard - range_min), TRUE);

    /* this moves the edit window as well */
    ScrollWindow(hWindow, xFirstCard-xSave, 0, NULL, NULL);

    UpdateWindow(hWindow);    /* make sure painting is complete */
    UpdateWindow(hEditWnd);    /* make sure painting is complete */
}

void ScrollIndexVert(
    HWND hWindow,
    int cmd,
    int pos)
{
    int ySave = yFirstCard;
    int range_min, range_max;

    GetScrollRange(hWindow, SB_VERT, &range_min, &range_max);

    switch (cmd)
    {
        case SB_LINEUP:
            yFirstCard += CharFixHeight;
            break;

        case SB_LINEDOWN:
            yFirstCard += -CharFixHeight;
            break;

        case SB_PAGEUP:
            yFirstCard += yCardWnd/2;
            break;

        case SB_PAGEDOWN:
            yFirstCard += -yCardWnd/2;
            break;

        case SB_THUMBPOSITION:
            yFirstCard = range_max - (pos - range_min);
            break;
    }

    yFirstCard = max(range_min, min(yFirstCard, range_max));

    SetScrollPos(hWindow, SB_VERT, range_max - (yFirstCard - range_min), TRUE);

    /* this moves the edit window as well */
    ScrollWindow(hWindow, 0, yFirstCard-ySave, NULL, NULL);

    UpdateWindow(hWindow);    /* make sure painting is complete */
    UpdateWindow(hEditWnd);    /* make sure painting is complete */
}

void MenuFileNew (void)
{
    LPCARDHEADER Cards;
#ifndef OLE_20
    CHAR  aszCardfile[60];
    CHAR  aszUntitled[60];
#endif

    if (fNoTempFile)
        return;
    if (!MaybeSaveFile (FALSE))
        return;
    if (lhcdoc)      /* Wipe out the client document */
        ReleaseClientDoc();
    SetCursor (hWaitCurs);
    *szText = (TCHAR) 0;
    CurIFile[0] = (TCHAR) 0;
    if (fhMain != INVALID_HANDLE_VALUE)
        MyCloseFile (fhMain);
    fhMain = INVALID_HANDLE_VALUE;
    szSearch[0] = (TCHAR) 0;
    SetCaption ();
    Fdelete (TempFile);
    MakeTempFile ();
    /* shrinking or leaving the same size, so this should always work */
    GlobalReAlloc (hCards, sizeof(CARDHEADER),GMEM_MOVEABLE);
    cCards = 1;
    iFirstCard = 0;
    iTopCard = 0;
    idObjectMax = 0;
    MakeBlankCard ();
    Cards = (LPCARDHEADER) GlobalLock (hCards);
    Cards[0] = CurCardHead;
    GlobalUnlock (hCards);

    InvalidateRect (CardPhone == CCARDFILE ? hCardWnd : hListWnd, NULL, TRUE);

    SetCursor (hArrowCurs);

    /* Reset the ListView box. */
    InitPhoneList(hListWnd, iFirstCard);
    SetNumOfCards();

    fFileDirty = FALSE;

    /* Register "Untitled" */
    if (lhcdoc)
        ReleaseClientDoc();

#ifndef OLE_20
    WideCharToMultiByte (CP_ACP, 0, szCardfile, -1, aszCardfile, 60, NULL, NULL);
    WideCharToMultiByte (CP_ACP, 0, szUntitled, -1, aszUntitled, 60, NULL, NULL);

    if (fOLE && OLE_OK != OleRegisterClientDoc (aszCardfile, aszUntitled, 0L, &lhcdoc))
#else
    if (fOLE && OLE_OK != OleRegisterClientDoc (szCardfile, szUntitled, 0L, &lhcdoc))
#endif
    {
        ErrorMessage (W_FAILED_TO_NOTIFY);
        lhcdoc = (LHCLIENTDOC) NULL; // lhb tracks
    }
}

void MakeTempFile (void)
{
    TCHAR    szTemp[PATHMAX];
    HANDLE   fh;


    fNoTempFile = FALSE;

    GetTempPath (PATHMAX - 1, szTemp);
    if (GetTempFileName (szTemp, szFileExtension, 0, TempFile))
    {
        if ((fh = MyOpenFile (TempFile, NULL,
                              OF_SHARE_EXCLUSIVE | OF_CREATE)) != INVALID_HANDLE_VALUE)
        {
            MyCloseFile (fh);
            return;
        }
    }
    IndexOkError (ECANTMAKETEMP);
    fNoTempFile = TRUE;
}


/*
 * set up the list box for the PHONEBOOK mode
 */
void InitPhoneList(
    HWND hWindow,
    int iStartCard)
{
    LPCARDHEADER Cards;
    int i;

    if (CardPhone == CCARDFILE)
        return;

    SendMessage (hWindow, LB_RESETCONTENT, 0, 0L);    /* clear any old data */
    SendMessage (hWindow, WM_SETREDRAW, FALSE, 0L);
    Cards = (LPCARDHEADER) GlobalLock(hCards);

    for (i = 0; i < cCards; i++)
        SendMessage(hWindow, LB_INSERTSTRING, i, (LONG)&Cards[i].line);

    GlobalUnlock(hCards);

    SendMessage(hWindow, WM_SETREDRAW, TRUE, 0L);
    SendMessage(hWindow, LB_SETCURSEL, iStartCard, 0L);
}

/* return TRUE if there are busy OLE objects, FALSE otherwise */
int CheckForBusyObjects(
    void)
{
    BOOL        fBusy = FALSE;
    LPOLEOBJECT lpObject = NULL;

    if (!fOLE) /* for safety */
        return FALSE;

    while ((OleEnumObjects(lhcdoc, &lpObject) == OLE_OK) && lpObject
        && lpObject != lpObjectUndo)
        fBusy |= DisplayBusyMessage(lpObject);

    return fBusy;
}

NOEXPORT BOOL NEAR DisplayBusyMessage (LPOLEOBJECT lpObject)
{
    LONG    objType;
    BOOL    fBusy = FALSE;
    HANDLE  hLink;
    LPTSTR  lpLink;
    PTCHAR  pWLink;         // unicode version of lpLink

    if ((OleQueryType (lpObject, &objType) == OLE_OK) &&
        (objType != OT_STATIC) &&
        (OleQueryReleaseStatus(lpObject) == OLE_BUSY))
    {
        TCHAR szStr[300], szMsg[300];

        fBusy = TRUE;
        LoadString (hIndexInstance, W_FREEBUSYSERVER, szStr, CharSizeOf(szStr));
        if(OleGetData( lpObject,
           (OLECLIPFORMAT) (objType == OT_LINK ? vcfLink : vcfOwnerLink),
                       &hLink) == OLE_OK
           && (lpLink = (LPTSTR) GlobalLock (hLink))
           && ( pWLink = Ole2Native( lpLink,1 ) ) )
        {
            wsprintf (szMsg, szStr, pWLink);
            GlobalUnlock (hLink);
            LocalFree( pWLink );
        }
        else
            wsprintf (szMsg, szStr, TEXT(""));
        MessageBox (hwndError, szMsg, szCardfile, MB_OK | MB_ICONEXCLAMATION);
    }

    return fBusy;
}
