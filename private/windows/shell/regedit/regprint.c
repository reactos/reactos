/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGPRINT.C
*
*  VERSION:     4.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        21 Nov 1993
*
*  Print routines for the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regprint.h"
#include "regcdhk.h"
#include "regresid.h"
#include "regedit.h"
#include "richedit.h"
#include <malloc.h>

const TCHAR s_PrintLineBreak[] = TEXT(",\n  ");

PRINTDLGEX g_PrintDlg;

typedef struct _PRINT_IO {
    BOOL fContinueJob;
    UINT ErrorStringID;
    HWND hRegPrintAbortWnd;
    RECT rcPage;
    RECT rcOutput;
    PTSTR pLineBuffer;
    UINT cch;
    UINT cBufferPos;
}   PRINT_IO;

#define CANCEL_NONE                     0x0000
#define CANCEL_MEMORY_ERROR             0x0001
#define CANCEL_PRINTER_ERROR            0x0002
#define CANCEL_ABORT                    0x0004

#define INITIAL_PRINTBUFFER_SIZE        8192
PRINT_IO s_PrintIo;

BOOL
CALLBACK
RegPrintAbortProc(
    HDC hDC,
    int Error
    );

INT_PTR
CALLBACK
RegPrintAbortDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
PASCAL
PrintBranch(
    HKEY hKey,
    LPTSTR lpFullKeyName
    );

VOID
PASCAL
PrintLiteral(
    LPCTSTR lpLiteral
    );

VOID
PASCAL
PrintBinary(
    CONST BYTE FAR* lpBuffer,
    DWORD cbBytes
    );

BOOL
PASCAL
PrintChar(
    TCHAR Char
    );

/*******************************************************************************
*
*  Implement IPrintDialogCallback
*
*  DESCRIPTION:
*     This interface is necessary to handle messages through PrintDlgEx
*     This interface doesn't need to have all the correct semantics of a COM
*     Object
*
*******************************************************************************/
	
typedef struct
{
    IPrintDialogCallback ipcb;
} CPrintCallback;

#define IMPL(type, pos, ptr) (type*) 

static
HRESULT
CPrintCallback_QueryInterface(IPrintDialogCallback *ppcb, REFIID riid, void **ppv)
{
    CPrintCallback *this = (CPrintCallback*)ppcb;
    if (IsEqualIID (riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IPrintDialogCallback))
        *ppv = &this->ipcb;
    else
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }    

    this->ipcb.lpVtbl->AddRef(&this->ipcb);
    return NOERROR;
}

static
ULONG
CPrintCallback_AddRef(IPrintDialogCallback *ppcb)
{
    CPrintCallback *this = (CPrintCallback*)ppcb;
    return 1;
}

static
ULONG
CPrintCallback_Release(IPrintDialogCallback *ppcb)
{
    CPrintCallback *this = (CPrintCallback*)ppcb;
    return 1;
}

static
HRESULT
CPrintCallback_InitDone(IPrintDialogCallback *ppcb)
{
    return S_OK;
}

static
HRESULT
CPrintCallback_SelectionChange(IPrintDialogCallback *ppcb)
{
    return S_OK;
}

static
HRESULT
CPrintCallback_HandleMessage(
    IPrintDialogCallback *ppcb, 
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT *pResult)
{
    *pResult = RegCommDlgHookProc(hDlg, uMsg, wParam, lParam);
    return S_OK;
}


static IPrintDialogCallbackVtbl vtblPCB =
{
    CPrintCallback_QueryInterface,
    CPrintCallback_AddRef,
    CPrintCallback_Release,
    CPrintCallback_InitDone,
    CPrintCallback_SelectionChange,
    CPrintCallback_HandleMessage
};

CPrintCallback g_callback;

/*******************************************************************************
*
*  RegEdit_OnCommandPrint
*
*  DESCRIPTION:
*     Handles the selection of the "Print" option by the user for the RegEdit
*     dialog box.
*
*  PARAMETERS:
*     hWnd, handle of RegPrint window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnCommandPrint(
    HWND hWnd
    )
{

    LPDEVNAMES lpDevNames;
    HKEY hKey;
    TEXTMETRIC TextMetric;
    DOCINFO DocInfo;
    LOGFONT lf;
    HGLOBAL hDevMode;
    HGLOBAL hDevNames;
    RECT rc;
    HWND hRichEdit;
    FORMATRANGE fr;
    HINSTANCE hInstRichEdit;
    int nOffsetX;
    int nOffsetY;
    
    g_callback.ipcb.lpVtbl = &vtblPCB;

    // We have to completely fill out the PRINTDLGEX structure
    // correctly or the PrintDlgEx function will return an error.
    // The easiest way is to memset it to 0

    hDevMode = g_PrintDlg.hDevMode;
    hDevNames = g_PrintDlg.hDevNames;
    memset(&g_PrintDlg, 0, sizeof(g_PrintDlg));

    g_PrintDlg.lStructSize = sizeof(PRINTDLGEX);
    g_PrintDlg.hwndOwner = hWnd;
    g_PrintDlg.hDevMode = hDevMode;
    g_PrintDlg.hDevNames = hDevNames;
    g_PrintDlg.hDC = NULL;
    g_PrintDlg.Flags = PD_NOPAGENUMS | PD_RETURNDC | PD_ENABLEPRINTTEMPLATE;
    g_PrintDlg.Flags2 = 0;
    g_PrintDlg.ExclusionFlags = 0;
    g_PrintDlg.hInstance = g_hInstance;
    g_PrintDlg.nCopies = 1;
    g_PrintDlg.nStartPage = START_PAGE_GENERAL;
	g_PrintDlg.lpCallback = (IUnknown*) &g_callback.ipcb;
    g_PrintDlg.lpPrintTemplateName = MAKEINTRESOURCE(IDD_REGPRINT);
    g_RegCommDlgDialogTemplate = IDD_REGPRINT;

    if (FAILED(PrintDlgEx(&g_PrintDlg)))
        return;
    if (g_PrintDlg.dwResultAction != PD_RESULT_PRINT)
        return;

    s_PrintIo.ErrorStringID = IDS_PRINTERRNOMEMORY;

    if ((lpDevNames = GlobalLock(g_PrintDlg.hDevNames)) == NULL)
        goto error_ShowDialog;

    if (!g_fRangeAll) {

        if (EditRegistryKey(&hKey, g_SelectedPath, ERK_OPEN) != ERROR_SUCCESS)
            goto error_UnlockDevNames;

    }

    //
    //  For now, assume a page with top and bottom margins of 1/2 inch and
    //  left and right margins of 3/4 inch (the defaults of Notepad).
    //  rcPage and rcOutput are in TWIPS (1/20th of a point)
    //

    rc.left = rc.top = 0;
    rc.bottom = GetDeviceCaps(g_PrintDlg.hDC, PHYSICALHEIGHT);
    rc.right = GetDeviceCaps(g_PrintDlg.hDC, PHYSICALWIDTH);
    nOffsetX = GetDeviceCaps(g_PrintDlg.hDC, PHYSICALOFFSETX);
    nOffsetY = GetDeviceCaps(g_PrintDlg.hDC, PHYSICALOFFSETY);

    s_PrintIo.rcPage.left = s_PrintIo.rcPage.top = 0;
    s_PrintIo.rcPage.right = MulDiv(rc.right, 1440, GetDeviceCaps(g_PrintDlg.hDC, LOGPIXELSX));
    s_PrintIo.rcPage.bottom = MulDiv(rc.bottom, 1440, GetDeviceCaps(g_PrintDlg.hDC, LOGPIXELSY));

    s_PrintIo.rcOutput.left = 1080;
    s_PrintIo.rcOutput.top = 720;
    s_PrintIo.rcOutput.right = s_PrintIo.rcPage.right - 1080;
    s_PrintIo.rcOutput.bottom = s_PrintIo.rcPage.bottom - 720;

    //
    //
    //

    if ((s_PrintIo.pLineBuffer = (PTSTR) LocalAlloc(LPTR, INITIAL_PRINTBUFFER_SIZE*sizeof(TCHAR))) == NULL)
        goto error_DeleteDC;
    s_PrintIo.cch = INITIAL_PRINTBUFFER_SIZE;
    s_PrintIo.cBufferPos = 0;

    if ((s_PrintIo.hRegPrintAbortWnd = CreateDialog(g_hInstance,
        MAKEINTRESOURCE(IDD_REGPRINTABORT), hWnd, RegPrintAbortDlgProc)) ==
        NULL)
        goto error_FreeLineBuffer;

    EnableWindow(hWnd, FALSE);

    //
    //  Prepare the document for printing.
    //

    s_PrintIo.fContinueJob = TRUE;
    SetAbortProc(g_PrintDlg.hDC, RegPrintAbortProc);

    DocInfo.cbSize = sizeof(DOCINFO);
    DocInfo.lpszDocName = LoadDynamicString(IDS_REGEDIT);
    DocInfo.lpszOutput = (LPTSTR) lpDevNames + lpDevNames-> wOutputOffset;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType = 0;

    s_PrintIo.ErrorStringID = 0;

    if (StartDoc(g_PrintDlg.hDC, &DocInfo) <= 0) {

        if (GetLastError() != ERROR_PRINT_CANCELLED)
            s_PrintIo.ErrorStringID = IDS_PRINTERRPRINTER;
        goto error_DeleteDocName;

    }

    //
    //  Print the desired range of the registry.
    //

    if (g_fRangeAll) {

        lstrcpy(g_SelectedPath,
            g_RegistryRoots[INDEX_HKEY_LOCAL_MACHINE].lpKeyName);
        PrintBranch(HKEY_LOCAL_MACHINE, g_SelectedPath);

        lstrcpy(g_SelectedPath,
            g_RegistryRoots[INDEX_HKEY_USERS].lpKeyName);
        PrintBranch(HKEY_USERS, g_SelectedPath);

    }

    else
        PrintBranch(hKey, g_SelectedPath);

    hInstRichEdit = LoadLibrary(TEXT("riched20.dll"));

    hRichEdit = CreateWindowEx(0, RICHEDIT_CLASS, NULL, ES_MULTILINE, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    SendMessage(hRichEdit, WM_SETTEXT, 0, (LPARAM)s_PrintIo.pLineBuffer);

    fr.hdc = g_PrintDlg.hDC;
    fr.hdcTarget = g_PrintDlg.hDC;
    fr.rc = s_PrintIo.rcOutput;
    fr.rcPage = s_PrintIo.rcPage;
    fr.chrg.cpMin = 0;
    fr.chrg.cpMax = -1;

    while (fr.chrg.cpMin < (int) s_PrintIo.cBufferPos) {
        StartPage(g_PrintDlg.hDC);

        // We have to adjust the origin because 0,0 is not at the corner of the paper
        // but is at the corner of the printable region

        SetViewportOrgEx(g_PrintDlg.hDC, -nOffsetX, -nOffsetY, NULL);
        fr.chrg.cpMin = (LONG)SendMessage(hRichEdit, EM_FORMATRANGE, TRUE, (LPARAM)&fr);
        SendMessage(hRichEdit, EM_DISPLAYBAND, 0, (LPARAM)&s_PrintIo.rcOutput);
        EndPage(g_PrintDlg.hDC);
        if (!s_PrintIo.fContinueJob)
            break;
    }
    SendMessage(hRichEdit, EM_FORMATRANGE, FALSE, 0);

    //
    //  End the print job.
    //

    if (s_PrintIo.ErrorStringID == 0 && s_PrintIo.fContinueJob) {

        if (EndDoc(g_PrintDlg.hDC) <= 0) {
            s_PrintIo.ErrorStringID = IDS_PRINTERRPRINTER;
            goto error_AbortDoc;
        }
    }

    //
    //  Either a printer error occurred or the user cancelled the printing, so
    //  abort the print job.
    //

    else {

error_AbortDoc:
        AbortDoc(g_PrintDlg.hDC);

    }

    DestroyWindow(hRichEdit);
    FreeLibrary(hInstRichEdit);

error_DeleteDocName:
    DeleteDynamicString(DocInfo.lpszDocName);

//  error_DestroyRegPrintAbortWnd:
    EnableWindow(hWnd, TRUE);
    DestroyWindow(s_PrintIo.hRegPrintAbortWnd);

error_FreeLineBuffer:
    LocalFree((HLOCAL)s_PrintIo.pLineBuffer);

error_DeleteDC:
    DeleteDC(g_PrintDlg.hDC);
    g_PrintDlg.hDC = NULL;

    if (!g_fRangeAll)
        RegCloseKey(hKey);

error_UnlockDevNames:
    GlobalUnlock(g_PrintDlg.hDevNames);

error_ShowDialog:
    if (s_PrintIo.ErrorStringID != 0)
        InternalMessageBox(g_hInstance, hWnd,
            MAKEINTRESOURCE(s_PrintIo.ErrorStringID),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONERROR | MB_OK);

}

/*******************************************************************************
*
*  RegPrintAbortProc
*
*  DESCRIPTION:
*     Callback procedure to check if the print job should be canceled.
*
*  PARAMETERS:
*     hDC, handle of printer device context.
*     Error, specifies whether an error has occurred.
*     (returns), TRUE to continue the job, else FALSE to cancel the job.
*
*******************************************************************************/

BOOL
CALLBACK
RegPrintAbortProc(
    HDC hDC,
    int Error
    )
{

    while (s_PrintIo.fContinueJob && MessagePump(s_PrintIo.hRegPrintAbortWnd))
        ;

    return s_PrintIo.fContinueJob;

    UNREFERENCED_PARAMETER(hDC);
    UNREFERENCED_PARAMETER(Error);

}

/*******************************************************************************
*
*  RegPrintAbortDlgProc
*
*  DESCRIPTION:
*     Callback procedure for the RegPrintAbort dialog box.
*
*  PARAMETERS:
*     hWnd, handle of RegPrintAbort window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

INT_PTR
CALLBACK
RegPrintAbortDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    switch (Message) {

        case WM_INITDIALOG:
            break;

        case WM_CLOSE:
        case WM_COMMAND:
            s_PrintIo.fContinueJob = FALSE;
            break;

        default:
            return FALSE;

    }

    return TRUE;

}

/*******************************************************************************
*
*  PrintBranch
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
PrintBranch(
    HKEY hKey,
    LPTSTR lpFullKeyName
    )
{

    DWORD EnumIndex;
    DWORD cbValueName;
    DWORD cbValueData;
    DWORD Type;
    LPTSTR lpSubKeyName;
    HKEY hSubKey;
    int nLenFullKey;
    LPTSTR lpTempFullKeyName;

    //
    //  Write out the section header.
    //

    PrintChar(TEXT('['));
    PrintLiteral(lpFullKeyName);
    PrintLiteral(TEXT("]\n"));

    //
    //  Write out all of the value names and their data.
    //

    EnumIndex = 0;

    while (s_PrintIo.fContinueJob) {

        cbValueName = sizeof(g_ValueNameBuffer);
        cbValueData = MAXDATA_LENGTH;

        if (RegEnumValue(hKey, EnumIndex++, g_ValueNameBuffer, &cbValueName,
            NULL, &Type, g_ValueDataBuffer, &cbValueData) != ERROR_SUCCESS)
            break;

        //
        //  If cbValueName is zero, then this is the default value of
        //  the key, or the Windows 3.1 compatible key value.
        //

        if (cbValueName)
            PrintLiteral(g_ValueNameBuffer);

        else
            PrintChar(TEXT('@'));

        PrintChar(TEXT('='));

        switch (Type) {

            case REG_SZ:
                PrintLiteral((LPTSTR)g_ValueDataBuffer);
                break;

            default:
                PrintBinary((LPBYTE) g_ValueDataBuffer, cbValueData);
                break;

        }

        PrintChar(TEXT('\n'));

    }

    PrintChar(TEXT('\n'));

    //
    //  Write out all of the subkeys and recurse into them.
    //


    //copy the existing key into a new buffer with enough room for the next key
    nLenFullKey = lstrlen(lpFullKeyName);
    lpTempFullKeyName = (LPTSTR) alloca( (nLenFullKey+MAXKEYNAME)*sizeof(TCHAR));
    lstrcpy(lpTempFullKeyName, lpFullKeyName);
    lpSubKeyName = lpTempFullKeyName + nLenFullKey;
    *lpSubKeyName++ = TEXT('\\');
    *lpSubKeyName = 0;
    
    EnumIndex = 0;

    while (s_PrintIo.fContinueJob) {

        if (RegEnumKey(hKey, EnumIndex++, lpSubKeyName, MAXKEYNAME) !=
            ERROR_SUCCESS)
            break;

        if(RegOpenKeyEx(hKey,lpSubKeyName,0,KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,&hSubKey) == NO_ERROR) {

            PrintBranch(hSubKey, lpTempFullKeyName);

            RegCloseKey(hSubKey);

        }

        else {

            DbgPrintf(("RegOpenKey failed."));

        }

    }

}

/*******************************************************************************
*
*  PrintLiteral
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
PrintLiteral(
    LPCTSTR lpLiteral
    )
{

    if (s_PrintIo.fContinueJob)
        while (*lpLiteral != 0 && PrintChar(*lpLiteral++));

}

/*******************************************************************************
*
*  PrintBinary
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
PrintBinary(
    CONST BYTE FAR* lpBuffer,
    DWORD cbBytes
    )
{

    BOOL fWriteSeparator;
    BYTE Byte;

    fWriteSeparator = FALSE;

    while (cbBytes--) {
        if (fWriteSeparator) {
            PrintChar(TEXT(','));
            PrintChar(TEXT(' '));
        }
        Byte = *lpBuffer++;
        PrintChar(g_HexConversion[Byte >> 4]);
        PrintChar(g_HexConversion[Byte & 0x0F]);
        fWriteSeparator = TRUE;
    }

}

/*******************************************************************************
*
*  PrintChar
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
PASCAL
PrintChar(
    TCHAR Char
    )
{

    //
    //  Keep track of what column we're currently at.  This is useful in cases
    //  such as writing a large binary registry record.  Instead of writing one
    //  very long line, the other Print* routines can break up their output.
    //

    if (s_PrintIo.cBufferPos == s_PrintIo.cch) {
        PTSTR pNewBuffer = LocalAlloc(LPTR, 2*s_PrintIo.cch*sizeof(TCHAR));
        if (pNewBuffer == NULL)
            return FALSE;
        memcpy(pNewBuffer, s_PrintIo.pLineBuffer, s_PrintIo.cch*sizeof(TCHAR));
        LocalFree(s_PrintIo.pLineBuffer);
        s_PrintIo.pLineBuffer = pNewBuffer;
        s_PrintIo.cch *= 2;
    }

    s_PrintIo.pLineBuffer[s_PrintIo.cBufferPos++] = Char;

    return TRUE;
}
