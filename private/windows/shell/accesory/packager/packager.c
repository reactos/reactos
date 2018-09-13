/* packager.c - OLE object wrapping application
 *
 * Created by Microsoft Corporation.
 */

#include "packager.h"
#include <shellapi.h>
#include "dialogs.h"
#include <htmlhelp.h>

#define MenuFlag(b)    ((b) ? MF_ENABLED : MF_GRAYED)

/* 4-Oct-93 #2695 v-katsuy */
                         // win31#2174: 12/26/92 : fixing frame window initial position
/* The width of the Packager Frame window is nearly equal to 640.
   This value must be changed, when the design will be changed.
*/
#define JPFRAMEWIDTH 640

// Pointer to function RegisterPenApp()
VOID (CALLBACK *RegPen)(WORD, BOOL) = NULL;


static BOOL gfDirty = FALSE;                // TRUE if file needs to be written
static CHAR szEmbedding[] = "-Embedding";   // Not NLS specific
static CHAR szEmbedding2[] = "/Embedding";  // Not NLS specific
static CHAR szFrameClass[] = "AppClass";    // Not NLS specific
static CHAR szObjectMenu[CBSHORTSTRING];    // "&Object" menu string
static CHAR szEdit[CBSHORTSTRING];          // "Edit" string
static CHAR szHelpFile[] = "PACKAGER.CHM";  // packager.chm

static BOOL InitApplication(VOID);
static BOOL InitInstance(VOID);
static VOID EndInstance(VOID);
static VOID SaveAsNeeded(VOID);
static BOOL WriteToFile(VOID);
static BOOL ReadFromFile(LPSTR lpstrFile);
static OLESTATUS ProcessCmdLine(LPSTR lpCmdLine, INT nCmdShow);
static VOID WaitForAllObjects(VOID);
static VOID UpdateMenu(HMENU hmenu);
static VOID UpdateObjectMenuItem(HMENU hMenu);
static VOID ExecuteVerb(INT iVerb);
INT_PTR CALLBACK fnFailedUpdate(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static VOID SendOleClosed(VOID);
static VOID CreateUntitled(VOID);
static VOID MakePenAware(VOID);
static VOID MakePenUnaware(VOID);
static VOID MakeMenuString(CHAR *szCtrl, CHAR *szMenuStr, CHAR *szVerb,
    CHAR *szClass, CHAR *szObject);


BOOL gbDBCS = FALSE;                 // TRUE if we're running in DBCS mode


/* WinMain() - Main Windows routine
 */
INT WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    INT nCmdShow
    )
{
    MSG msg;
    LCID lcid;

//DebugBreak(); //BUGBUG
    // Store the application instance number
    ghInst = hInstance;

    // Check DBCSness
    lcid = GetThreadLocale();

    gbDBCS = ( (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_JAPANESE) ||
               (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_KOREAN)   ||
               (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_CHINESE)
              );


    // Initialize application global information (window classes)
    if (!hPrevInstance)
    {
        if (!InitApplication())
            return FALSE;
    }

    // Initialize instance-specific information
    if (!InitInstance() || !InitClient())
        goto errRtn;

    if (!(gfServer = InitServer()))
        goto errRtn;

    MakePenAware();

    if (ProcessCmdLine(lpCmdLine, nCmdShow) != OLE_OK)
    {
        DeleteServer(glpsrvr);
        goto errRtn;
    }

    // if blocking happened in SrvrOpen(), then wait for object to be created
    if (gfBlocked)
        WaitForObject(((LPPICT)(glpobj[CONTENT]))->lpObject);

    // Main message loop
    while (TRUE)
    {
        if (gfBlocked && glpsrvr)
        {
            BOOL bMore = TRUE;
            LHSERVER lhsrvr = glpsrvr->lhsrvr;

            gfBlocked = FALSE;
            while (bMore)
            {
                if (OleUnblockServer (lhsrvr, &bMore) != OLE_OK)
                    break;

                if (gfBlocked)
                    break;
            }
        }

        if (!GetMessage(&msg, NULL, 0, 0))
            break;

        if (!TranslateAccelerator(ghwndFrame, ghAccTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        //
        // to support activation of file based object though Ole mechanism
        // we create a linked object out of file and then activate it. But
        // we don't get any notification when server closes the document.
        // Using the following mechanism we find it out and then grab the
        // contents from file
        //
        if (gfEmbObjectOpen)
        {
            LPEMBED lpembed = (LPEMBED)(glpobj[CONTENT]);

            if (lpembed != NULL && OleQueryOpen(lpembed->lpLinkObj) != OLE_OK)
            {
                gfEmbObjectOpen = FALSE;
                EmbRead(lpembed);
                EmbDeleteLinkObject(lpembed);

                if (gfInvisible)
                    PostMessage(ghwndFrame, WM_SYSCOMMAND, SC_CLOSE, 0L);
            }
        }
    }

    goto cleanup;

errRtn:
    if (ghwndFrame)
        DestroyWindow(ghwndFrame);

cleanup:
    EndClient();
    MakePenUnaware();
    EndInstance();

    return FALSE;
}



/* InitApplication() - Do application "global" initialization.
 *
 * This function registers the window classes used by the application.
 * Returns:  TRUE iff successful.
 */
static BOOL
InitApplication(
    VOID
    )
{
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = FrameWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = ghInst;
    wc.hIcon = LoadIcon(ghInst, MAKEINTRESOURCE(ID_APPLICATION));
    wc.hCursor = LoadCursor(ghInst, MAKEINTRESOURCE(SPLIT));
    wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(ID_APPLICATION);
    wc.lpszClassName = szFrameClass;

    if (!RegisterClass(&wc))
        return FALSE;

    return InitPaneClasses();
}



/* InitInstance() - Handles the instance-specific initialization.
 *
 * This function creates the main application window.
 * Returns:  TRUE iff successful.
 */
static BOOL
InitInstance(
    VOID
    )
{
    HDC hDC;

    ghAccTable = LoadAccelerators(ghInst, MAKEINTRESOURCE(ID_APPLICATION));
    ghbrBackground = GetSysColorBrush(COLOR_APPWORKSPACE);
    ghcurWait = LoadCursor(NULL, IDC_WAIT);

    // Load the string resources
    LoadString(ghInst, IDS_APPNAME, szAppName, CBMESSAGEMAX);
    LoadString(ghInst, IDS_UNTITLED, szUntitled, CBMESSAGEMAX);

    // Create the Main Window

    if (gbDBCS)
    {
        /* 4-Oct-93 #2695 v-katsuy */
        // win31#2174: 12/26/92 : fixing frame window initial position
        if (!(ghwndError = ghwndFrame =
            CreateWindow(szFrameClass, szAppName,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
            | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT,
                // Following values are calculated when the window size is changed.
                // Default posiotion of a window is desided here, so dumy values
                // must be set here.
                JPFRAMEWIDTH, JPFRAMEWIDTH  * 7 / 18,
            NULL, NULL, ghInst, NULL)))
            return FALSE;
    }
    else
    {
        if (!(ghwndError = ghwndFrame =
            CreateWindow(szFrameClass, szAppName,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
            | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, ghInst, NULL)))
            return FALSE;
    }


    // Initialize the registration database
    RegInit();

    // Set the correct caption string
    OfnInit();
    glpobj[CONTENT] = glpobj[APPEARANCE] = NULL;
    glpobjUndo[CONTENT] = glpobjUndo[APPEARANCE] = NULL;

    LoadString(ghInst, IDS_EDIT, szEdit, CBSHORTSTRING);
    LoadString(ghInst, IDS_OBJECT_MENU, szObjectMenu, CBSHORTSTRING);
    LoadString(ghInst, IDS_UNDO_MENU, szUndo, CBSHORTSTRING);
    LoadString(ghInst, IDS_GENERIC, szDummy, CBSHORTSTRING);
    LoadString(ghInst, IDS_CONTENT_OBJECT, szContent, CBMESSAGEMAX);
    LoadString(ghInst, IDS_APPEARANCE_OBJECT, szAppearance, CBMESSAGEMAX);

    // Initialize global variables with LOGPIXELSX and LOGPIXELSY
    if (hDC = GetDC (NULL))
    {
        giXppli = GetDeviceCaps(hDC, LOGPIXELSX);
        giYppli = GetDeviceCaps(hDC, LOGPIXELSY);
        ReleaseDC(NULL, hDC);
    }

    return InitPanes();
}



/* EndInstance() - Instance-specific termination code.
 */
static VOID
EndInstance(
    VOID
    )
{
    EndPanes();
}



/* FrameWndProc() - Frame window procedure.
 *
 * This function is the message handler for the application frame window.
 */
LRESULT CALLBACK
FrameWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL fSuccess = FALSE;

    if (SplitterFrame(hwnd, msg, wParam, lParam))
        return DefWindowProc(hwnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_READEMBEDDED:
        if (gpty[CONTENT] == PEMBED)
        {
            EmbRead(glpobj[CONTENT]);

            if (gfInvisible)
                PostMessage(ghwndFrame, WM_SYSCOMMAND, SC_CLOSE, 0L);
        }

        break;

    case WM_INITMENU:
        UpdateMenu((HMENU)wParam);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_NEXTWINDOW:
            // Special trickery works because APP = 0 & CONTENT = 1
            Raise(GetTopWindow(hwnd) != ghwndPane[CONTENT]);
            break;

        case IDM_NEW:
            // Save the current file (if needed)
            SaveAsNeeded();

            // delete the current doc, and create untitled document
            CreateUntitled();
            break;

        case IDM_IMPORT:
            if (!OfnGetName(hwnd, IDM_IMPORT))
                break;

            Hourglass(TRUE);
            DeletePane(CONTENT, TRUE);
            if (!ReadFromFile(gszFileName))
                ErrorMessage(E_FAILED_TO_READ_FILE);
            InvalidateRect(ghwndPane[CONTENT], NULL, TRUE);
            Dirty();

            if (!gpty[APPEARANCE])
            {
                if (glpobj[APPEARANCE] = IconCreateFromFile(gszFileName))
                {
                    gpty[APPEARANCE] = ICON;
                    InvalidateRect(ghwndPane[APPEARANCE], NULL, TRUE);
                }
            }

            Hourglass(FALSE);
            break;

        case IDM_EXPORT:
            if (!OfnGetName(hwnd, IDM_EXPORT))
                return 0L;          /* Operation cancelled */

            Hourglass(TRUE);

            if (!WriteToFile())
                ErrorMessage(E_FAILED_TO_SAVE_FILE);

            Hourglass(FALSE);
            break;

        case IDM_UPDATE:
            {
                OLESTATUS retval;

                if (Error(OleSavedClientDoc(glhcdoc)))
                    ErrorMessage(W_FAILED_TO_NOTIFY);

                if ((retval = OleSavedServerDoc (glpdoc->lhdoc)) == OLE_OK)
                {
                    gfDirty = FALSE;
                }
                else if (retval == OLE_ERROR_CANT_UPDATE_CLIENT)
                {
                    //
                    // The client doesn't take updates on Save. Let the
                    // user explicitly update and exit, or continue with
                    // the editing.
                    //
                    if (!MyDialogBox(DTFAILEDUPDATE, ghwndFrame, fnFailedUpdate))
                    {
                        // update the object and exit
                        gfOleClosed = TRUE;
                        DeregisterDoc();
                        DeleteServer(glpsrvr);
                    }
                }
                else
                {
                    Error(retval);
                }

                break;
            }

        case IDM_EXIT:
            SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
            return 0L;
            break;

        case IDM_COMMAND:
            Raise(CONTENT);
            DeletePane(CONTENT, FALSE);

            if (gptyUndo[CONTENT] != CMDLINK)
                glpobj[CONTENT] = CmlCreate("", FALSE);
            else
                glpobj[CONTENT] = CmlClone(glpobjUndo[CONTENT]);

            if (glpobj[CONTENT])
                gpty[CONTENT] = CMDLINK;

            if (glpobj[CONTENT] && ChangeCmdLine(glpobj[CONTENT]))
            {
                InvalidateRect(ghwndPane[CONTENT], NULL, TRUE);
                Dirty();
            }
            else
            {
                CmlDelete(glpobj[CONTENT]);
                gpty[CONTENT] = NOTHING;
                glpobj[CONTENT] = NULL;
                SendMessage(ghwndPane[CONTENT], WM_COMMAND, IDM_UNDO, 0L);
            }

            break;

        case IDM_INSERTICON:
            PostMessage (ghwndBar[APPEARANCE], WM_COMMAND, IDM_INSERTICON, 0L);
            break;

        case IDM_DESC:
        case IDM_PICT:
            PostMessage(ghwndBar[CONTENT], WM_COMMAND, wParam, 0L);
            break;

        case IDM_LABEL:
            Raise(APPEARANCE);

            if (gpty[APPEARANCE] != ICON)
                break;

            ChangeLabel(glpobj[APPEARANCE]);
            InvalidateRect(ghwndPane[APPEARANCE], NULL, TRUE);
            Dirty();
            break;

        case IDM_COPYPACKAGE:
            if (!CopyObjects())
                ErrorMessage(E_CLIPBOARD_COPY_FAILED);

            break;

        case IDM_PASTE:
            // Check to see if we are pasting a packaged object
            if (IsClipboardFormatAvailable(gcfNative)
                && IsClipboardFormatAvailable(gcfOwnerLink))
            {
                HANDLE hData;
                HANDLE hData2;
                LPSTR lpData;

                OpenClipboard(ghwndFrame);
                hData = GetClipboardData(gcfOwnerLink);

                if (lpData = GlobalLock(hData))
                {
                    // If it's the packager, get the native data
                    if (!lstrcmpi(lpData, gszAppClassName)
                        && (hData2 = GetClipboardData(gcfNative)))
                        fSuccess = PutNative(hData2);

                    // Unlock the clipboard Owner Link data
                    GlobalUnlock(hData);
                }

                CloseClipboard();
            }

            // Did we successfully read the native data?
            if (fSuccess)
                break;

            // ... guess not (maybe not Package!)
            PostMessage(GetTopWindow(hwnd), msg, wParam, lParam);
            break;

        case IDM_OBJECT:
            ExecuteVerb(0);     // Execute the ONLY verb
            break;

        case IDM_INDEX:
            HtmlHelp(ghwndFrame, szHelpFile, HH_DISPLAY_TOPIC, 0L);
            break;

        case IDM_ABOUT:
            ShellAbout(hwnd, szAppName, "",
                LoadIcon(ghInst, MAKEINTRESOURCE(ID_APPLICATION)));
            break;

        default:
            if ((LOWORD(wParam) >= IDM_VERBMIN)
                && (LOWORD(wParam) <= IDM_VERBMAX))
            {
                // An object verb has been selected
                // (Hmm.  Did you know that an 'object verb' was a noun?)
                ExecuteVerb(LOWORD(wParam) - IDM_VERBMIN);
            }
            else
            {
                PostMessage(GetTopWindow(hwnd), msg, wParam, lParam);
            }

            break;
        }

        break;

    case WM_CLOSE:
        //
        // Update if necessary by notifying the server that we are closing
        // down, and revoke the server.
        //
        SaveAsNeeded();
        SendOleClosed();
        DeleteServer(glpsrvr);

        return 0L;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0L;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0L;
}



/* SetTitle() - Sets the window caption to the current filename.
 *
 * If gszFileName is NULL, the caption will be set to "(Untitled)".
 * If DocSetHostNames() is called with a client app name, that name
 * will be prepended.
 *
 * For the Embedded case, the "Embedded #n" string is stored in
 * "Untitled", and is always displayed regardless of the file name.
 */
VOID
SetTitle(
    BOOL fRegistering
    )
{
    CHAR szTitle[CBMESSAGEMAX + CBPATHMAX];

    if (!gfEmbedded)
    {
        wsprintf(szTitle, "%s%s%s - %s", gszClientName,
            (*gszClientName) ? " " : "",
            szAppName, szUntitled);
    }
    else
    {
        CHAR szEmbnameContent[CBSHORTSTRING];

        LoadString(ghInst, IDS_EMBNAME_CONTENT, szEmbnameContent, CBSHORTSTRING);

        if (gbDBCS)
        {
            //#3997: 2/19/93: changed Window title
            wsprintf(szTitle, "%s - %s %s", szAppName, szUntitled,
                             szEmbnameContent);
        }
        else
        {
            wsprintf(szTitle, "%s - %s %s", szAppName, szEmbnameContent,
                 szUntitled);
        }

    }

    // Perform the client document registration
    if (glhcdoc)
    {
        if (Error(OleRenameClientDoc(glhcdoc, szUntitled)))
            ErrorMessage(W_FAILED_TO_NOTIFY);

        if (!fRegistering)
            ChangeDocName(&glpdoc, szUntitled);
    }
    else
    {
        if (Error(OleRegisterClientDoc(gszAppClassName, szUntitled, 0L, &glhcdoc)))
        {
            ErrorMessage(W_FAILED_TO_NOTIFY);
            glhcdoc = 0;
        }

        // New file, so re-register it
        if (!fRegistering)
            glpdoc = InitDoc(glpsrvr, 0, szUntitled);
    }

    if (IsWindow(ghwndFrame))
        SetWindowText(ghwndFrame, szTitle);
}



/* InitFile() - Reinitializes the title bar, etc... when editing a New file.
 */
VOID
InitFile(
    VOID
    )
{
    gfDirty = FALSE;

    // Deregister the edited document, and wipe out the objects.
    DeregisterDoc();

    // Reset the title bar, and register the OLE client document
    SetTitle(FALSE);
}



/* SaveAsNeeded() - Saves the file if it has been modified. It's assumed that
 *                  after this routine is called this document is going to be
 *                  closed. If that's not true, then this routine may have to
 *                  be rewritten.
 */
static VOID
SaveAsNeeded(
    VOID
    )
{
    gfOleClosed = FALSE;

    if (gfDirty && gfEmbedded && (glpobj[APPEARANCE] || glpobj[CONTENT]))
    {
        CHAR sz[CBMESSAGEMAX];
        CHAR sz2[CBMESSAGEMAX + CBPATHMAX];

        if (gfInvisible)
        {
            SendDocChangeMsg(glpdoc, OLE_CLOSED);
            return;
        }

        LoadString(ghInst, gfEmbedded ? IDS_MAYBEUPDATE : IDS_MAYBESAVE, sz,
             CBMESSAGEMAX);
        wsprintf(sz2, sz, (LPSTR)szUntitled);

        // Ask "Do you wish to save your changes?"
        if (MessageBoxAfterBlock(ghwndFrame, sz2, szAppName,
            MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            gfOleClosed = TRUE;
            return;
        }
        // If not saving changes, revert the document
        else if (OleRevertClientDoc(glhcdoc))
        {
            ErrorMessage(W_FAILED_TO_NOTIFY);
        }
    }
}



/* WriteToFile() - Writes the current document to a file.
 *
 * Returns:  TRUE iff successful.
 */
static BOOL
WriteToFile(
    VOID
    )
{
    BOOL fSuccess = FALSE;
    OFSTRUCT reopenbuf;
    INT fh;

    CHAR szDesc[CBSTRINGMAX];
    CHAR szMessage[CBSTRINGMAX + CBPATHMAX];

    if (OpenFile(gszFileName, &reopenbuf, OF_EXIST) != -1)
    {
        // File exists, query for overwrite!
        LoadString(ghInst, IDS_OVERWRITE, szDesc, CharCountOf(szDesc));
        wsprintf(szMessage, szDesc, gszFileName);
        if (MessageBoxAfterBlock(ghwndFrame, szMessage, szAppName,
            MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
            return TRUE;
    }

    // Take care of this earlier?
    if ((fh = _lcreat((LPSTR)gszFileName, 0)) <= 0)
    {
        LoadString(ghInst, IDS_INVALID_FILENAME, szDesc, CharCountOf(szDesc));
        wsprintf(szMessage, szDesc, gszFileName);
        MessageBoxAfterBlock(ghwndFrame, szMessage, szAppName, MB_OK);
        return FALSE;
    }

    Hourglass(TRUE);

    // Go to the top of the file
    _llseek(fh, 0L, 0);

    EmbWriteToFile(glpobj[CONTENT], fh);
    fSuccess = TRUE;

    // Close the file, and return
    _lclose(fh);
    gfDirty = FALSE;
    Hourglass(FALSE);

    return fSuccess;
}



/* ReadFromFile() - Reads OLE objects from a file.
 *
 * Reads as many objects as it can, in upwards order (better error recovery).
 * Returns: TRUE iff successful.
 */
static BOOL
ReadFromFile(
    LPSTR lpstrFile
    )
{
    BOOL fSuccess = FALSE;

    Hourglass(TRUE);

    // Read in each object and get them in the right order
    if (!(glpobj[CONTENT] = EmbCreate(lpstrFile)))
    {
        ErrorMessage(E_FAILED_TO_READ_OBJECT);
        goto Error;
    }

    gpty[CONTENT] = PEMBED;

    fSuccess = TRUE;

Error:
    gfDirty = FALSE;
    Hourglass(FALSE);

    return fSuccess;
}



/* ErrorMessage() - Pops up a message box containing a string table message.
 *
 * Pre:  Assigns "ghwndError" to be its parent, so focus will return properly.
 */
VOID
ErrorMessage(
    UINT id
    )
{
    CHAR sz[300];

    if (IsWindow(ghwndError))
    {
        LoadString(ghInst, id, sz, 300);
        MessageBoxAfterBlock(ghwndError, sz, szAppName,
            MB_OK | MB_ICONEXCLAMATION);
    }
}



/* ProcessMessage() - Spin in a message dispatch loop.
 */
BOOL
ProcessMessage(
    VOID
    )
{
    BOOL fReturn;
    MSG msg;

    if (fReturn = GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(ghwndFrame, ghAccTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return fReturn;
}



/* Contains() - Determines whether a string matches a pattern.
 * This could be more intelligent, but it is scarcely executed.
 *
 * Returns: Non-NULL iff lpPattern is a substring of lpString.
 */
LPSTR
Contains(
    LPSTR lpString,
    LPSTR lpPattern
    )
{
    LPSTR lpSubstr;
    LPSTR lpPat;

    for (;;)
    {
        // Match the first character
        while (*lpString && *lpString != *lpPattern)
            lpString++;

        // We are at the end of the string, fail...
        if (!(*lpString))
            return NULL;

        // If we have a match, try to match the entire pattern string
        lpPat = lpPattern;
        lpSubstr = lpString;
        while (*lpPat && *lpSubstr && *lpPat == *lpSubstr)
        {
            lpPat++;
            lpSubstr++;
        }

        // We are at the end of the pattern, success!  Wipe out the pattern
        if (!(*lpPat))
            return lpString;

        // We are at the end of the string, failure...
        if (!(*lpSubstr))
            return NULL;

        lpString++;
    }
}



/* ProcessCmdLine() - Processes the command line options.
 */
static OLESTATUS
ProcessCmdLine(
    LPSTR lpCmdLine,
    INT nCmdShow
    )
{
    OLESTATUS retval = OLE_OK;

    // Does the command line contain "/Embedding"?
    if (gfEmbeddedFlag = gfInvisible =
        (Contains(lpCmdLine, szEmbedding) || Contains(lpCmdLine, szEmbedding2)))
    {
        // If we have a file name, register it NOW!
        lpCmdLine += lstrlen(szEmbedding);

        while (*lpCmdLine && *lpCmdLine == ' ')
            lpCmdLine++;

        if (*lpCmdLine)
        {
            retval = (glpsrvr->olesrvr.lpvtbl->Open)
                ((LPOLESERVER)glpsrvr, 0, lpCmdLine,
                (LPOLESERVERDOC *)&glpdoc);

            if (retval != OLE_OK)
                return retval;
        }

        gfDirty = FALSE;
        gnCmdShowSave = nCmdShow;

    }
    else
    {
        ShowWindow(ghwndFrame, nCmdShow);
        SendMessage(ghwndFrame, WM_COMMAND, IDM_NEW, 0L);
    }

    return retval;
}



/* Dirty() - This function is called each time the document is soiled.
 */
VOID
Dirty(
    VOID
    )
{
    gfDirty = TRUE;
    SendDocChangeMsg(glpdoc, OLE_CHANGED);
}



/* WaitForAllObjects() - Wait for asynchronous operations to complete.
 *
 * We don't use ProcessMessage() because we want to terminate as quickly
 * as possible, and we don't want to allow any structured user input.
 */
static VOID
WaitForAllObjects(
    VOID
    )
{
    MSG msgWait;

    if (gcOleWait)
    {
        while (gcOleWait)
        {
            if (GetMessage(&msgWait, NULL, 0, 0))
                DispatchMessage(&msgWait);
        }
    }
}



/* DeregisterDoc() - Deregisters the currently edited document.
 */
VOID
DeregisterDoc(
    VOID
    )
{
    gfDocCleared = TRUE;

    SendOleClosed();

    // Destroy all the objects
    DeletePane(APPEARANCE, TRUE);
    DeletePane(CONTENT, TRUE);

    // Wait for the objects to be deleted
    WaitForAllObjects();

    if (glpdoc)
    {
        LHSERVERDOC lhdoc = glpdoc->lhdoc;

        glpdoc = NULL;
        OleRevokeServerDoc(lhdoc);
    }

    // Release the document
    if (glhcdoc)
    {
        if (Error(OleRevokeClientDoc(glhcdoc)))
            ErrorMessage(W_FAILED_TO_NOTIFY);

        glhcdoc = 0;
    }
}



static VOID
UpdateMenu(
    HMENU hmenu
    )
{
    INT iPane;
    INT mf;

    iPane = (GetTopWindow(ghwndFrame) == ghwndPane[CONTENT]);
    EnableMenuItem(hmenu, IDM_EXPORT, MenuFlag(gpty[CONTENT] == PEMBED));
    EnableMenuItem(hmenu, IDM_CLEAR, MenuFlag(gpty[iPane]));
    EnableMenuItem(hmenu, IDM_UNDO, MenuFlag(gptyUndo[iPane]));

    EnableMenuItem(hmenu, IDM_UPDATE, (gfEmbedded ? MF_ENABLED : MF_GRAYED));

    if (((iPane == APPEARANCE) && gpty[iPane]) || (gpty[iPane] == PICTURE))
    {
        EnableMenuItem(hmenu, IDM_CUT, MF_ENABLED);
        EnableMenuItem(hmenu, IDM_COPY, MF_ENABLED);
    }
    else
    {
        EnableMenuItem(hmenu, IDM_CUT, MF_GRAYED);
        EnableMenuItem(hmenu, IDM_COPY, MF_GRAYED);
    }

    if (gpty[iPane] == PICTURE)
    {
        LPPICT lppict = glpobj[iPane];
        DWORD ot;

        mf = MF_GRAYED;
        if (lppict->lpObject)
        {
            OleQueryType(lppict->lpObject, &ot);

            // Enable Links... only if we have a linked object
            mf = MenuFlag(ot == OT_LINK);
        }

        EnableMenuItem(hmenu, IDM_LINKS, mf);
        EnableMenuItem(hmenu, IDM_LABEL, MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hmenu, IDM_LINKS, MF_GRAYED);
        EnableMenuItem(hmenu, IDM_LABEL, MenuFlag(gpty[APPEARANCE] == ICON));
    }

    UpdateObjectMenuItem(GetSubMenu(hmenu, POS_EDITMENU));
    mf = MenuFlag(OleQueryCreateFromClip(gszProtocol, olerender_draw, 0) ==
        OLE_OK
        || OleQueryCreateFromClip(gszSProtocol, olerender_draw, 0) == OLE_OK);
    EnableMenuItem(hmenu, IDM_PASTE, mf);

    if (iPane == CONTENT)
    {
        if (IsClipboardFormatAvailable(gcfFileName)) {
            EnableMenuItem(hmenu, IDM_PASTELINK, MF_ENABLED);
        }
        else
        {
            mf = MenuFlag(OleQueryLinkFromClip(gszProtocol, olerender_draw, 0)
                == OLE_OK);
            EnableMenuItem(hmenu, IDM_PASTELINK, mf);
        }
    }
    else
    {
        EnableMenuItem(hmenu, IDM_PASTELINK, MF_GRAYED);
    }

    mf = MenuFlag(gpty[CONTENT] && gpty[APPEARANCE]);
    EnableMenuItem(hmenu, IDM_COPYPACKAGE, mf);
}



/* UpdateObjectMenuItem - If there are items in the selection, add the
 *                        menu, with a possible popup depending on the
 *                        number of verbs.
 */
static VOID
UpdateObjectMenuItem(
    HMENU hMenu
    )
{
    INT cVerbs = 0;             /* how many verbs in list */
    HWND hwndItem = NULL;
    INT iPane;
    LONG objtype;
    LPPICT lpPict;
    CHAR szWordOrder2[10];
    CHAR szWordOrder3[10];

    if (!hMenu)
        return;

    DeleteMenu(hMenu, POS_OBJECT, MF_BYPOSITION);

    LoadString(ghInst, IDS_POPUPVERBS, szWordOrder2, sizeof(szWordOrder2));
    LoadString(ghInst, IDS_SINGLEVERB, szWordOrder3, sizeof(szWordOrder3));

    //
    // CASES:
    //  object supports 0 verbs          "<Object Class> Object"
    //  object supports 1 verb == edit   "<Object Class> Object"
    //  object supports 1 verb != edit   "<verb> <Object Class> Object"
    //  object supports more than 1 verb "<Object Class> Object" => verbs
    //

    iPane = ((hwndItem = GetTopWindow(ghwndFrame)) == ghwndPane[CONTENT]);
    lpPict = glpobj[iPane];

    if (lpPict
        && OleQueryType(lpPict->lpObject, &objtype) == OLE_OK
        && hwndItem
        && gpty[iPane] == PICTURE
        && objtype != OT_STATIC)
    {
        HANDLE hData = NULL;
        LPSTR lpstrData;

        if (OleGetData(lpPict->lpObject, (OLECLIPFORMAT) (objtype == OT_LINK ?
            gcfLink : gcfOwnerLink), &hData) == OLE_OK)
        {
            // Both link formats are:  "szClass0szDocument0szItem00"
            if (lpstrData = GlobalLock(hData))
            {
                DWORD dwSize = KEYNAMESIZE;
                CHAR szClass[KEYNAMESIZE], szBuffer[200];
                CHAR szVerb[KEYNAMESIZE];
                HANDLE hPopupNew = NULL;

                // get real language class of object in szClass for menu
                if (RegQueryValue(HKEY_CLASSES_ROOT, lpstrData,
                    szClass, &dwSize))
                    lstrcpy(szClass, lpstrData);    /* if above call failed */
                GlobalUnlock(hData);

                // append class key
                for (cVerbs = 0; ; ++cVerbs)
                {
                    dwSize = KEYNAMESIZE;
                    wsprintf(szBuffer,
                        "%s\\protocol\\StdFileEditing\\verb\\%d",
                        lpstrData, cVerbs);

                    if (RegQueryValue(HKEY_CLASSES_ROOT, szBuffer,
                        szVerb, &dwSize))
                        break;

                    if (hPopupNew == NULL)
                        hPopupNew = CreatePopupMenu();

                    InsertMenu(hPopupNew, (UINT)-1, MF_BYPOSITION,
                        IDM_VERBMIN + cVerbs, szVerb);
                }

                if (cVerbs == 0)
                {
                    MakeMenuString(szWordOrder3, szBuffer, szEdit,
                        szClass, szObjectMenu);
                    InsertMenu(hMenu, POS_OBJECT, MF_BYPOSITION,
                        IDM_VERBMIN, szBuffer);
                }
                else if (cVerbs == 1)
                {
                    MakeMenuString(szWordOrder3, szBuffer, szVerb,
                        szClass, szObjectMenu);
                    InsertMenu(hMenu, POS_OBJECT, MF_BYPOSITION,
                        IDM_VERBMIN, szBuffer);
                    DestroyMenu(hPopupNew);
                }
                else
                {
                    // > 1 verbs
                    MakeMenuString(szWordOrder2, szBuffer, NULL,
                        szClass, szObjectMenu);
                    InsertMenu(hMenu, POS_OBJECT, MF_BYPOSITION |
                        MF_POPUP, (UINT_PTR)hPopupNew, szBuffer);
                }

                EnableMenuItem(hMenu, POS_OBJECT,
                    MF_ENABLED | MF_BYPOSITION);

                return;
            }
        }
    }

    // error if got to here
    InsertMenu(hMenu, POS_OBJECT, MF_BYPOSITION, 0, szObjectMenu);
    EnableMenuItem(hMenu, POS_OBJECT, MF_GRAYED | MF_BYPOSITION);
}



/* ExecuteVerb() - Find the proper verb to execute for each selected item
 */
static VOID
    ExecuteVerb(
    INT iVerb
    )
{
    HWND hwndItem;
    INT iPane;
    RECT rc;

    iPane = ((hwndItem = GetTopWindow(ghwndFrame)) == ghwndPane[CONTENT]);

    GetClientRect(hwndItem, (LPRECT) & rc);

    // Execute the correct verb for this object
    if (Error(OleActivate(((LPPICT)(glpobj[iPane]))->lpObject, iVerb, TRUE,
        TRUE, hwndItem, &rc)))
    {
        if (OleQueryReleaseError(((LPPICT)(glpobj[iPane]))->lpObject) == OLE_ERROR_LAUNCH )
            ErrorMessage(E_FAILED_TO_LAUNCH_SERVER);
    }
    else
    {
        LONG ot;

        WaitForObject(((LPPICT)(glpobj[iPane]))->lpObject);
        if (!glpobj[iPane])
            return;

        OleQueryType(((LPPICT)(glpobj[iPane]))->lpObject, &ot);
        if (ot == OT_EMBEDDED)
            Error(OleSetHostNames(((LPPICT)(glpobj[iPane]))->lpObject,
                gszAppClassName,
                (iPane == CONTENT) ? szContent : szAppearance));
    }
}



VOID
Raise(
    INT iPane
    )
{
    if (GetTopWindow(ghwndFrame) != ghwndPane[iPane])
        SendMessage(ghwndPane[iPane], WM_LBUTTONDOWN, 0, 0L);
}



INT_PTR CALLBACK
fnFailedUpdate(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    switch (msg)
    {
        case WM_INITDIALOG:
            {
                CHAR szMsg[200];
                CHAR szStr[100];

                LoadString(ghInst, IDS_FAILEDUPDATE, szStr, sizeof(szStr));
                wsprintf((LPSTR)szMsg, szStr, gszClientName, szAppName);
                SetDlgItemText(hDlg, IDD_TEXT, szMsg);

                return TRUE; // default Push button gets the focus
            }

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                case IDD_CONTINUEEDIT:
                    EndDialog(hDlg, TRUE);
                    break;

                case IDD_UPDATEEXIT:
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    break;
            }

            break;

        default:
            break;
    }

    return FALSE;
}



static VOID
SendOleClosed(
    VOID
    )
{
    // Do this first, so the data can be updated as needed
    if (glpdoc)
    {
        if (gfOleClosed)
        {
            SendDocChangeMsg(glpdoc, OLE_CLOSED);
            gfOleClosed = FALSE;
        }
    }
}



static VOID
CreateUntitled(
    VOID
    )
{
    if (gfEmbedded)      /* Unembed if embedded */
        EndEmbedding();

    if (gvlptempdoc = InitDoc(glpsrvr, 0, szUntitled))
    {
        InitFile();      /* Reset the file */
        glpdoc = gvlptempdoc;
        SetTitle(TRUE);
        gvlptempdoc = NULL;
        gfDocExists  = TRUE;
        gfDocCleared = FALSE;
    }
    else
    {
        ErrorMessage(E_FAILED_TO_REGISTER_DOCUMENT);
    }
}



static VOID
MakePenAware(
    VOID
    )
{

    HANDLE hPenWin = NULL;

    if ((hPenWin = (HANDLE)GetSystemMetrics(SM_PENWINDOWS)) != NULL)
    {
        // We do this fancy GetProcAddress simply because we don't
        // know if we're running Pen Windows.

        if ((RegPen = (VOID (CALLBACK *)(WORD, BOOL))GetProcAddress(hPenWin, "RegisterPenApp")) != NULL)
            (*RegPen)(1, TRUE);
    }

}



static VOID
MakePenUnaware(
    VOID
    )
{
    if (RegPen != NULL)
        (*RegPen)(1, FALSE);
}



INT_PTR MessageBoxAfterBlock(
    HWND hwndParent,
    LPSTR lpText,
    LPSTR lpCaption,
    UINT fuStyle
    )
{
    if (glpsrvr && !gfBlocked && (OleBlockServer(glpsrvr->lhsrvr) == OLE_OK))
        gfBlocked = TRUE;

    return MessageBox((gfInvisible ? NULL : hwndParent), lpText, lpCaption,
         fuStyle);
}



INT_PTR DialogBoxAfterBlock(
    LPCSTR lpTemplate,
    HWND hwndParent,
    DLGPROC lpDialogFunc
    )
{
    if (glpsrvr && !gfBlocked && (OleBlockServer(glpsrvr->lhsrvr) == OLE_OK))
        gfBlocked = TRUE;

    return DialogBox(ghInst, lpTemplate, (gfInvisible ? NULL : hwndParent),
        lpDialogFunc);
}



static VOID
MakeMenuString(
    CHAR *szCtrl,
    CHAR *szMenuStr,
    CHAR *szVerb,
    CHAR *szClass,
    CHAR *szObject
    )
{
    register CHAR c;
    CHAR *pStr;

    while (c = *szCtrl++)
    {
        switch (c)
        {
            case 'c': // class
            case 'C': // class
                pStr = szClass;
                break;

            case 'v': // class
            case 'V': // class
                pStr = szVerb;
                break;

            case 'o': // object
            case 'O': // object
                pStr = szObject;
                break;

            default:
                *szMenuStr++ = c;
                *szMenuStr = '\0'; // just in case
                continue;
        }

        if (pStr) // should always be true
        {
            lstrcpy(szMenuStr, pStr);
            szMenuStr += lstrlen(pStr); // point to '\0'

        }
    }
}
