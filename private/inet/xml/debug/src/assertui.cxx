//+---------------------------------------------------------------------------
//
//  Microsoft Windows
// Copyright (c) 1992 - 1999 Microsoft Corporation. All rights reserved.*///
//  File:       assertui.cxx
//
//  Contents:   Assert Dialog implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#ifdef WIN16
#define LoadIconA   LoadIcon
#define SendDlgItemMessageA SendDlgItemMessage
#define DS_CENTER 0
#define DS_NOFAILCREATE 0
#define BS_CENTER 0
#define BS_VCENTER 0
#define DS_SETFOREGROUND 0
#define DialogBoxIndirectParamA DialogBoxIndirectParam
#endif

//+---------------------------------------------------------------------------
//
// MYDLGTEMPLATE, MYDLGITEMTEMPLATE
//
// Structures containing additional necessary information for the dialog
// template
//
//----------------------------------------------------------------------------

#include "pshpack2.h" // It provides the pack(2) feature.

struct MYDLGTEMPLATE : DLGTEMPLATE
{
    WCHAR wMenu;       // Must be 0
    WCHAR wClass;      // Must be 0
    WCHAR achTitle[2]; // Must be DWORD aligned
};

struct MYDLGITEMTEMPLATE : DLGITEMTEMPLATE
{
    WCHAR wClassLen;   // Must be -1
    WCHAR wClassType;
    WCHAR achInitText[2];
};

#include "poppack.h" // It provides the pack() feature.

//+---------------------------------------------------------------------------
//
//  DLGITEM_ID
//
//  IDs for all the controls in the dialog
//
//----------------------------------------------------------------------------

enum DLGITEM_ID
{
    DI_PROCESSNAME = 0,
    DI_PROCESSID,
    DI_FILE,
    DI_LINE,
    DI_MESSAGE,
    DI_IGNORE,
    DI_BREAK,
    DI_CLIPBOARD,
    DI_ICON,
    DI_STACKMSG,
    DI_MODULEBASE,
    DI_FUNCBASE   = DI_MODULEBASE + SHORT_SYM_COUNT,
    DI_MAX        = DI_FUNCBASE   + SHORT_SYM_COUNT
};

#define STYLE_STATICDEFAULT WS_VISIBLE | SS_LEFTNOWORDWRAP | SS_NOPREFIX | \
                            WS_CHILD


#define DLGITEM_STATIC 0x0082
#define DLGITEM_BUTTON 0x0080

void StringFromMBOT(MBOT *pmbot, char * pch, BOOL fShortSyms);

//+---------------------------------------------------------------------------
//
// Inline function that returns a DWORD aligned pointer to a MYDLGITEMTEMPLATE
// based on the end of the previous one. [pb] should be the achInitText
// pointer, with the string already set. If [fAddExtra] is TRUE, it accounts
// for the "creation data" element by skipping over it (it must already have
// been set to zero).
//
//----------------------------------------------------------------------------

inline MYDLGITEMTEMPLATE *
GetNextItem(BYTE * pb, BOOL fAddExtra = TRUE)
{
    pb = pb + ((lstrlenW((WCHAR*)pb) + 1) * sizeof(WCHAR)) + ((fAddExtra)
                                                               ? sizeof(WORD)
                                                               : 0);

    // The following DWORD aligns the returned pointer

    return (MYDLGITEMTEMPLATE *)(((ULONG_PTR)pb + 3) & ~(3));
}

//+---------------------------------------------------------------------------
//
// wsprintf function that always returns output in Unicode, even on Win95
//
//----------------------------------------------------------------------------
#ifdef WIN16
#define my_wsprintf wsprintf
#else
void
my_wsprintf(WCHAR *pchBuf, CHAR *pchFmt, ...)
{
    extern BOOL g_fOSIsNT;

    va_list valMarker;

    va_start(valMarker, pchFmt);

    // wvsprintfW is not implemented on Win95 - blaugh!
    if (g_fOSIsNT)
    {
        WCHAR achFmt[100];

        wsprintfW(achFmt, _T("%hs"), pchFmt);

        wvsprintfW(pchBuf, achFmt, valMarker);
    }
    else
    {
        CHAR achBuf[1024];

        wvsprintfA(achBuf, pchFmt, valMarker);

        MultiByteToWideChar(CP_ACP, 0, achBuf, -1, pchBuf, 256);
    }

    va_end(valMarker);
}
#endif //!WIN16
//+---------------------------------------------------------------------------
//
//  Function:   DlgAssert
//
//  Synopsis:   DialogProc for the assert dialog
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK
DlgAssert(HWND hwndDlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            EnsureThreadState();

            TLS(pmbot) = (MBOT *)lparam;

            HICON hIcon = LoadIconA(NULL, (LPSTR)IDI_EXCLAMATION);
            SendDlgItemMessageA(hwndDlg,
                                DI_ICON,
                                STM_SETICON,
                                (WPARAM)hIcon,
                                (LPARAM)0);

#if 0
            for (int i = DI_PROCESSNAME; i < DI_MESSAGE; i++)
            {
                SendDlgItemMessageA(hwndDlg,
                                    i,
                                    WM_SETFONT,
                                    (WPARAM)GetStockObject(SYSTEM_FIXED_FONT),
                                    (LPARAM)0);
            }
#endif

            // This call to PlaySound is too flaky to use - during DllMain
            // calls it fails to return and in general doesn't always work on
            // NT 4.0.
            // PlaySoundA("SystemExclamation", NULL, SND_ALIAS | SND_NODEFAULT);
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wparam, lparam))
        {
        case DI_CLIPBOARD:
            {
                HGLOBAL hGlobal;
                char    achBuf[4096];

                StringFromMBOT(TLS(pmbot), achBuf, FALSE);
                hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                                      strlen(achBuf) + 2);

                if (hGlobal)
                {
                    char * pch = (char *)GlobalLock(hGlobal);
                    strcpy(pch, achBuf);
                    GlobalUnlock(hGlobal);

                    OpenClipboard(hwndDlg);
                    EmptyClipboard();
                    SetClipboardData(CF_TEXT, hGlobal);
                    CloseClipboard();
                }

            }
            break;

        case DI_IGNORE:
            EndDialog(hwndDlg, IDOK);
            break;

        case DI_BREAK:
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoAssertDialog
//
//  Synopsis:   Creates an assert dialog (described below)
//
//  Arguments:  [pmbot] -- Pointer to assert information
//
//  Returns:    IDOK if user selected "Ignore", IDCANCEL for "Break"
//
//  The dialog looks like this:
//
//       ------------------------------------------------------------
//       |                                                          |
//       |  ICON    Process: <exename>            <Process ID Info> |
//       |          File: <filename>              Line: <line #>    |
//       |                                                          |
//       |                          <message>                       |
//       |                                                          |
//       |   Stacktrace: (Not displayed if not present)             |
//       |       <module>  <symbol>                                 |
//       |          .         .                                     |
//       |          .         .                                     |
//       |                                                          |
//       |          [Ignore]       [Copy Text]     [Break]          |
//       |                                                          |
//       ------------------------------------------------------------
//
//     Each section of text is a separate static control, whose ID is taken
//     from the DLGITEM_ID enum. Each control has a corresponding
//     MYDLGITEMTEMPLATE structure, which is filled in with the appropriate
//     initial text, position, and style information. The dialog is dynamically
//     sized to fit the message and stacktrace as appropriate. The Copy Text
//     button causes the text of the assert plus an extended stacktrace to
//     be copied to the clipboard.
//
//----------------------------------------------------------------------------

// The following distance and size values are all in "character widths" or
// "character heights", not dialog units, except as noted.

static int CX_LEFT_EDGE = 7; // Left edge to start of 1st two lines
static int CY_TOP      =  1; // Top to first line
static int CY_DELTA    =  1; // Vertical distance between lines
static int CX_BUFFER   =  3; // Minimum Horiz. spacing between two textboxes.
                             //  Also used to space text from the right edge
                             //  of the dialog.
static int CX_BUTTON   = 42; // Width of the buttons  (in dialog units)
static int CY_BUTTON   = 14; // Height of the buttons (in dialog units)

static int CX_MODULE = (CX_LEFT_EDGE + 3); // Left edge to <module> text
static int CX_SYMBOL = (CX_MODULE + 12);   // Left edge to <symbol> text

// The following is the maximum number of characters we allow on a single line
// for the message displayed in the dialog.
#define CX_MAX_WIDTH  100


int
DoAssertDialog(MBOT *pmbot)
{
    BYTE               abBuf[4096];     // Buffer for storing dialog template
    MYDLGTEMPLATE *    pDlgTmplt;
    MYDLGITEMTEMPLATE *pIT[30] = { 0 }; // Max number of controls in dialog=30
    MYDLGITEMTEMPLATE *pITCur;
    DWORD              dwWidth;
    int                i;

    memset(abBuf, 0, sizeof(abBuf));

    // --- Main Dialog Template

    pDlgTmplt = (MYDLGTEMPLATE*)abBuf;

    pDlgTmplt->style = WS_VISIBLE | WS_POPUP | WS_BORDER | WS_CAPTION | WS_POPUP |
                       DS_CENTER | DS_NOFAILCREATE | //DS_FIXEDSYS |
                       DS_SETFOREGROUND;

    my_wsprintf(pDlgTmplt->achTitle, "%hs", pmbot->szTitle);

    // --- Process Name label

    pITCur = pIT[DI_PROCESSNAME] = GetNextItem((BYTE*)(pDlgTmplt->achTitle), FALSE);

    my_wsprintf(pITCur->achInitText, "Process: %hs", pmbot->achModule);

    pITCur->style      = STYLE_STATICDEFAULT;
    pITCur->x          = (short)CX_LEFT_EDGE;
    pITCur->y          = (short)CY_TOP;
    pITCur->cx         = (short)lstrlenW(pITCur->achInitText) + CX_BUFFER;
    pITCur->cy         = (short)1;
    pITCur->id         = DI_PROCESSNAME;
    pITCur->wClassLen  = 0xFFFF;
    pITCur->wClassType = DLGITEM_STATIC;

    // --- Process and Thread ID label

    pITCur = pIT[DI_PROCESSID] = GetNextItem((BYTE*)(pITCur->achInitText));

    if (pmbot->tid < 0xFFFF)
    {
        my_wsprintf(pITCur->achInitText,
                    "PID: %x  TID: %x",
                    pmbot->pid,
                    pmbot->tid);
    }
    else
    {
        my_wsprintf(pITCur->achInitText, "TID: %x", pmbot->tid);
    }

    pITCur->style      = STYLE_STATICDEFAULT;
    pITCur->y          = (short)CY_TOP;
    pITCur->cx         = (short)lstrlenW(pITCur->achInitText);
    pITCur->cy         = (short)1;
    pITCur->id         = DI_PROCESSID;
    pITCur->wClassLen  = 0xFFFF;
    pITCur->wClassType = DLGITEM_STATIC;

    // --- File Name label

    pITCur = pIT[DI_FILE] = GetNextItem((BYTE*)(pITCur->achInitText));

    my_wsprintf(pITCur->achInitText, "File: %hs", pmbot->szFile);

    pITCur->style      = STYLE_STATICDEFAULT;
    pITCur->x          = (short)CX_LEFT_EDGE;
    pITCur->y          = (short)CY_TOP + CY_DELTA;
    pITCur->cx         = (short)lstrlenW(pITCur->achInitText) + CX_BUFFER;
    pITCur->cy         = (short)1;
    pITCur->id         = DI_FILE;
    pITCur->wClassLen  = 0xFFFF;
    pITCur->wClassType = DLGITEM_STATIC;

    // --- Line Number label

    pITCur = pIT[DI_LINE] = GetNextItem((BYTE*)(pITCur->achInitText));

    my_wsprintf(pITCur->achInitText, "Line: %u", pmbot->dwLine);

    pITCur->style      = STYLE_STATICDEFAULT;
    pITCur->y          = (short)(CY_TOP + CY_DELTA);
    pITCur->cx         = (short)lstrlenW(pITCur->achInitText);
    pITCur->cy         = (short)1;
    pITCur->id         = DI_LINE;
    pITCur->wClassLen  = 0xFFFF;
    pITCur->wClassType = DLGITEM_STATIC;

    // -- Compute width of dialog. This is done by finding the longest line
    //    of the first three lines in the dialog. The message can never
    //    contribute more than CX_MAX_WIDTH characters to this calculation
    //    because if it's longer than that then we wrap it onto multiple lines.

    dwWidth = max(pIT[DI_PROCESSNAME]->cx, pIT[DI_FILE]->cx) +
              max(pIT[DI_PROCESSID]->cx,   pIT[DI_LINE]->cx);

    // Do signed arithmetic to catch a negative result
    i = strlen(pmbot->szMessage) - CX_LEFT_EDGE;

    if ((i > (int)dwWidth) && (i < CX_MAX_WIDTH))
    {
        dwWidth = min(i, CX_MAX_WIDTH);
    }

    // Make sure the buttons fit.
    if ((dwWidth + CX_LEFT_EDGE) < (DWORD)(4 * (CX_BUTTON/4)))
    {
        dwWidth = 4 * (CX_BUTTON/4) + CX_BUFFER;
    }

    // Set the overall dialog width, and position the two controls in the upper
    // right corner.

    pDlgTmplt->cx = (short)(dwWidth + CX_LEFT_EDGE + CX_BUFFER);
    pIT[DI_PROCESSID]->x = (short)(dwWidth + CX_LEFT_EDGE - pIT[DI_PROCESSID]->cx);
    pIT[DI_LINE]->x = pIT[DI_PROCESSID]->x;

    // --- Message label

    pITCur = pIT[DI_MESSAGE] = GetNextItem((BYTE*)(pITCur->achInitText));

    my_wsprintf(pITCur->achInitText, "%hs", pmbot->szMessage);

    pITCur->style      = WS_VISIBLE | WS_CHILD | SS_CENTER | SS_NOPREFIX;
    pITCur->x          = 1;
    pITCur->y          = CY_TOP + (CY_DELTA * 3);
    pITCur->cx         = (short)(dwWidth + CX_LEFT_EDGE);
    pITCur->cy         = (short) (lstrlenW(pITCur->achInitText) / dwWidth + 1);
    pITCur->id         = DI_MESSAGE;
    pITCur->wClassLen  = 0xFFFF;
    pITCur->wClassType = DLGITEM_STATIC;

    // --- Ignore Button (default)
    //
    // The order of these buttons determines the tab order!

    pITCur = pIT[DI_IGNORE] = GetNextItem((BYTE*)(pITCur->achInitText));

    my_wsprintf(pITCur->achInitText, "&Ignore");

    pITCur->style      = WS_VISIBLE | WS_TABSTOP | WS_CHILD | BS_CENTER |
                         BS_VCENTER | BS_DEFPUSHBUTTON;
    pITCur->x          = (short)((pDlgTmplt->cx / 2) - (2 * (CX_BUTTON/4)));
    pITCur->cx         = (short)CX_BUTTON;
    pITCur->cy         = (short)CY_BUTTON;
    pITCur->id         = DI_IGNORE;
    pITCur->wClassLen  = 0xFFFF;
    pITCur->wClassType = DLGITEM_BUTTON;

    // --- Copy To Clipboard Button

    pITCur = pIT[DI_CLIPBOARD] = GetNextItem((BYTE*)(pITCur->achInitText));

    my_wsprintf(pITCur->achInitText, "&Copy Text");

    pITCur->style      = WS_VISIBLE | WS_TABSTOP | WS_CHILD | BS_CENTER |
                         BS_VCENTER | BS_PUSHBUTTON;
    pITCur->x          = (short)((pDlgTmplt->cx / 2) - (CX_BUTTON / 8));
    pITCur->cx         = (short)CX_BUTTON;
    pITCur->cy         = (short)CY_BUTTON;
    pITCur->id         = DI_CLIPBOARD;
    pITCur->wClassLen  = 0xFFFF;
    pITCur->wClassType = DLGITEM_BUTTON;

    // --- Break Button

    pITCur = pIT[DI_BREAK] = GetNextItem((BYTE*)(pITCur->achInitText));

    my_wsprintf(pITCur->achInitText, "&Break");

    pITCur->style      = WS_VISIBLE | WS_TABSTOP | WS_CHILD | BS_CENTER |
                         BS_VCENTER | BS_PUSHBUTTON;
    pITCur->x          = (short)((pDlgTmplt->cx / 2) + (CX_BUTTON/4));
    pITCur->cx         = (short)CX_BUTTON;
    pITCur->cy         = (short)CY_BUTTON;
    pITCur->id         = IDCANCEL; // So escape key works
    pITCur->wClassLen  = 0xFFFF;
    pITCur->wClassType = DLGITEM_BUTTON;

    // --- Icon

    pITCur = pIT[DI_ICON] = GetNextItem((BYTE*)(pITCur->achInitText));

    pITCur->achInitText[0] = L'\0'; // Icon is set in WM_INITDIALOG

    pITCur->style      = WS_VISIBLE | WS_CHILD | SS_ICON;
    pITCur->x          = 2;
    pITCur->y          = 1;
    pITCur->cx         = 0;
    pITCur->cy         = 0;
    pITCur->id         = DI_ICON;
    pITCur->wClassLen  = 0xFFFF;
    pITCur->wClassType = DLGITEM_STATIC;

    pDlgTmplt->cdit = 9; // Total of all controls so far

    //
    // --- Add Stacktrace information if available
    //
    if (pmbot->cSym)
    {
        int cSyms, j, ypos, id;

        // --- Stacktrace label

        pITCur = pIT[DI_STACKMSG] = GetNextItem((BYTE*)(pITCur->achInitText));

        my_wsprintf(pITCur->achInitText, "StackTrace:");

        pITCur->style      = STYLE_STATICDEFAULT;
        pITCur->y          = (short)(pIT[DI_MESSAGE]->y + pIT[DI_MESSAGE]->cy + CY_DELTA);
        pITCur->x          = (short)CX_LEFT_EDGE;
        pITCur->cx         = (short)lstrlenW(pITCur->achInitText);
        pITCur->cy         = (short)1;
        pITCur->id         = DI_STACKMSG;
        pITCur->wClassLen  = 0xFFFF;
        pITCur->wClassType = DLGITEM_STATIC;

        pDlgTmplt->cdit++;

        cSyms = min(pmbot->cSym, SHORT_SYM_COUNT);
        ypos  = pITCur->y + pITCur->cy;

        // --- Modules and symbols

        for (i=0; i < cSyms; i++)
        {
            for (j=0; j < 2; j++)
            {
                id = (j) ? (DI_MODULEBASE+i) : (DI_FUNCBASE+i);

                pITCur = pIT[id] = GetNextItem((BYTE*)(pITCur->achInitText));

                my_wsprintf(pITCur->achInitText,
                            "%hs",
                            (j)
                              ? pmbot->asiSym[i].achModule
                              : pmbot->asiSym[i].achSymbol);

                pITCur->style      = STYLE_STATICDEFAULT;
                pITCur->x          = (short)((j) ? CX_MODULE : CX_SYMBOL);
                pITCur->y          = (short)ypos;
                pITCur->cx         = (short)lstrlenW(pITCur->achInitText);
                pITCur->cy         = (short)1;
                pITCur->id         = (short)id;
                pITCur->wClassLen  = 0xFFFF;
                pITCur->wClassType = DLGITEM_STATIC;

                pDlgTmplt->cdit++;
            }

            ypos += pITCur->cy;
        }
    }

    // --- Convert coordinates from character widths to dialog units (char * 4
    // in X direction, char * 8 in Y direction)

    pDlgTmplt->cx  *= 4;

    for (i = 0; i < DI_MAX; i++)
    {
        pITCur = pIT[i];
        if (pITCur)
        {
            pITCur->x *= 4;
            pITCur->y *= 8;

            if (pITCur->wClassType != DLGITEM_BUTTON)
            {
                // Button cx's and cy's are already in dialog units.
                pITCur->cx *= 4;
                pIT[i]->cy *= 8;
            }
        }
    }

    //
    // Position the buttons vertically now that all text is added
    //
    if (pmbot->cSym)
    {
        i = DI_MODULEBASE + min(pmbot->cSym, SHORT_SYM_COUNT) - 1;
    }
    else
    {
        i = DI_MESSAGE;
    }

    pIT[DI_IGNORE]->y = pIT[i]->y + pIT[i]->cy + (8*CY_DELTA);
    pIT[DI_BREAK]->y  = pIT[DI_CLIPBOARD]->y = pIT[DI_IGNORE]->y;

    // Compute the overall height of the dialog
    pDlgTmplt->cy = pIT[DI_IGNORE]->y + pIT[DI_IGNORE]->cy + (8*CY_DELTA);

    // -- Let's create this thing!
#ifdef WIN16
    HGLOBAL dlgTemplate = GlobalAlloc( GMEM_FIXED, sizeof(abBuf));
    void *ptr = GlobalLock(dlgTemplate);
    memcpy((void *)ptr, (void *)abBuf, sizeof(abBuf));
    GlobalUnlock(dlgTemplate);
#else
    DLGTEMPLATE *dlgTemplate = (DLGTEMPLATE *)abBuf;
#endif

		pmbot->id = (int)DialogBoxIndirectParamA(
                                        g_hinstMain,
                                        dlgTemplate,
                                        NULL,
                                        DlgAssert,
                                        (LPARAM)pmbot);

    return pmbot->id;
}

