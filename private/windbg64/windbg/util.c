/*--Author:

    Griffith Wm. Kadnier (v-griffk) 01-Aug-1992

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include "direct.h"

LRESULT SendMessageNZ (HWND,UINT,WPARAM,LPARAM);
static BOOL CompareFileName(LPSTR, LPSTR);
static BOOL CompareFileNameWithPath(LPSTR, LPSTR);


#include <ime.h>

typedef BOOL    (WINAPI *LPWINNLSENABLEIME)(HWND, BOOL);
typedef LRESULT (WINAPI *LPSENDIMEMESSAGEEX)(HWND, LPARAM);

static LPWINNLSENABLEIME lpfnWINNLSEnableIME = NULL;
static LPSENDIMEMESSAGEEX lpfnSendIMEMessageEx = NULL;

static HINSTANCE hModUser32 = NULL;



HWND 
MDIGetActive(
    HWND    hwndParent,
    BOOL   *lpbMaximized
    )
/*++
Routine Description:

  Create the command window.

Arguments:

    hwndParent - The parent window to the command window. In an MDI document,
        this is usually the handle to the MDI client window: g_hwndMDIClient

Return Value:

    The return value is the handle to the active MDI child window.

    NULL if no MDI window has been created.

--*/
{
    Assert(IsWindow(hwndParent));
    return (HWND)SendMessage(hwndParent, 
                             WM_MDIGETACTIVE, 
                             0, 
                             (LPARAM)lpbMaximized
                             );
}


/***    FileExist
**
**  Synopsis:
**      bool = FileExist(szFileName)
**
**  Entry:
**      szFileName - Name of file to check for
**
**  Returns:
**      TRUE if file exists and FALSE otherwise
**
**  Description:
**      Checks to see if a file exists with the path/filename
**      described by the string pointed to by 'fileName'.
*/

BOOL
FileExist(
    LPSTR pszFileName
    )
{
    HANDLE hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE == hFile) {
        return FALSE;
    } else {
        CloseHandle(hFile);
        return TRUE;
    }
}                                       /* FileExist() */

/***    hGetBoxParent
**
**  Synopsis:
**      hwnd = hGetBoxParent()
**
**  Entry:
**      none
**
**  Returns:
**
**  Description:
**      Gets a suitable parent window handle for an
**      invocation of a message or dialog box.
**      Helper function to util.c functions so declared
**      near.
**
*/

HWND
hGetBoxParent()
{
    HWND hCurWnd;
    int i=0;

    hCurWnd = GetFocus();
    if (hCurWnd) {
        while (GetWindowLong(hCurWnd, GWL_STYLE) & WS_CHILD) {

            hCurWnd = GetParent(hCurWnd);
            Dbg(++i < 100);
        }
    } else {
        hCurWnd = hwndFrame;
    }

    return hCurWnd;
}                                       /* hGetBoxParent() */

/****************************************************************************

        FUNCTION:   MsgBox

        PURPOSE:    General purpose message box routine which takes
                    a pointer to the message text.  Provides
                    program title as caption.

****************************************************************************/

int
PASCAL
MsgBox(
    HWND hwndParent,
    LPSTR szText,
    UINT wType
    )
/*++

Routine Description:

    Generial purpose message box routine which takes a pointer to a message
    text and prvoides the program title for the caption of the message box.

Arguments:

    hwndParament - Supplies the parent window handle for the message box
    szText      - Supplies a pointer to the message box text.
    wType       - Supplies the message box type (to specify buttons)

Return Value:

    Returns the message box return code

--*/

{
    int MsgBoxRet = IDOK;

    if (NoPopups) {
        //
        // log the string to the command win in case testing
        // or when the remote server is running
        //
        CmdLogFmt ("%s\r\n", szText);
    } else {
        BoxCount++;
        MsgBoxRet = MessageBox(hwndParent, szText, (LPSTR)MainTitleText, wType);
        BoxCount--;
    }

    return MsgBoxRet;
}                               /* MsgBox() */

/****************************************************************************

        FUNCTION:   VarMsgBox

        PURPOSE:    As MsgBox but takes resource id as text and performs
                    a vsprintf on the variable parameters.

****************************************************************************/
int
CDECL
VarMsgBox(
    HWND hwndParent,
    WORD wFormat,
    UINT wType,
    ...)
{
    char szFormat[MAX_MSG_TXT];
    char szText[MAX_VAR_MSG_TXT];   // size is as big as considered necessary
    va_list vargs;

    // load format string from resource file
    Dbg(LoadString(g_hInst, wFormat, (LPSTR)szFormat, MAX_MSG_TXT));

    va_start(vargs, wType);
    vsprintf(szText, szFormat, vargs);
    va_end(vargs);

    return MsgBox(hwndParent, szText, wType);
}                                       /* VarMsgBox() */

/***    ErrorBox
**
**  Synopsis:
**      int = ErrorBox(wErrorFormat, ...)
**
**  Entry:
**      wErrorFormat
**      ...
**
**  Returns:
**      FALSE
**
**  Description:
**      Display an error message box with an "Error" title, an OK
**      button and a Exclamation Icon. First parameter is a
**      reference string in the ressource file.  The string
**      can contain printf formatting chars, the arguments
**      follow from the second parameter onwards.
**
*/

int
CDECL
ErrorBox(
    int wErrorFormat,
    ...
    )
{
    char szErrorFormat[MAX_MSG_TXT];
    char szErrorText[MAX_VAR_MSG_TXT];  // size is as big as considered necessary
    va_list vargs;

    // load format string from resource file
    Dbg(LoadString(g_hInst, wErrorFormat, (LPSTR)szErrorFormat, MAX_MSG_TXT));

    va_start(vargs, wErrorFormat);
    vsprintf(szErrorText, szErrorFormat, vargs);
    va_end(vargs);

    MsgBox(hwndFrame, (LPSTR)szErrorText, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
    return FALSE;   //Keep it always FALSE please
}                                       /* ErrorBox() */

/***    ErrorBox2
**
**  Synopsis:
**      int = ErrorBox2(hwnd, type, wErrorFormat, ...)
**
**  Entry:
**
**  Return:
**      FALSE
**
**  Description:
**      Display an error message box with an "Error" title, an OK
**      button and a Exclamation Icon. First is the window handle
**      of the parent window. Second is the extra types given
**      to the message box.Third parameter is a reference string
**      in the ressource file.  The string can contain printf
**      formatting chars, the arguments follow from the fourth
**      parameter onwards.
**
*/

int
CDECL
ErrorBox2(
    HWND hwnd,
    UINT type,
    WORD wErrorFormat,
    ...
    )
{
    char szErrorFormat[MAX_MSG_TXT];
    char szErrorText[MAX_VAR_MSG_TXT];      // size is as big as considered necessary
    va_list vargs;

    // load format string from resource file
    Dbg(LoadString(g_hInst, wErrorFormat, (LPSTR)szErrorFormat, MAX_MSG_TXT));

    // set up szErrorText from passed parameters
    va_start(vargs, wErrorFormat);
    vsprintf(szErrorText, szErrorFormat, vargs);
    va_end(vargs);

    MsgBox(hwnd, (LPSTR)szErrorText, type | MB_OK | MB_ICONINFORMATION);
    return FALSE; //Keep it always FALSE please
}                                       /* ErrorBox2() */

/***    InternalErrorBox
**
**  Synopsis:
**      int = InternalErrorBox(wDescript,...)
**
**  Entry:
**
**  Returns:
**      FALSE
**
**  Description:
**
*/

int
InternalErrorBox(
    WORD wDescript
    )
{
    char szErrorFormat[MAX_MSG_TXT];
    char szErrorText[MAX_VAR_MSG_TXT];      // size is as big as considered necessary
    char szArgument[MAX_MSG_TXT];

    //Load format and argument strings from resource file
    Dbg(LoadString(g_hInst, ERR_Internal_Error, (LPSTR)szErrorFormat, MAX_MSG_TXT));
    Dbg(LoadString(g_hInst, wDescript, (LPSTR)szArgument, MAX_MSG_TXT));

    sprintf(szErrorText, szErrorFormat, (LPSTR)szArgument);

    MsgBox(hwndFrame, (LPSTR)szErrorText, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);

    DebugBreak();
    return FALSE;
}                                       /* InternalErrorBox() */

/***    InformationBox
**
**  Synopsis:
**      void = InformationBox(wInfoFormat, ...)
**
**  Entry:
**      wInfoFormat     - Resource index of format string
**      ...             - Additional informationto be displayed
**
**  Returns:
**      Nothing
**
**  Description:
**      Display an information message box with an "Information"
**      title, an OK button and an Information Icon. First
**      parameter is a reference string in the ressource file.
**      The string can contain printf formatting chars, the
**      arguments follow from the second parameter onwards.
**
*/

void
CDECL
InformationBox(
    int wInfoFormat,
    ...
    )
{
    char szInfoFormat[MAX_MSG_TXT];
    char szInfoText[MAX_VAR_MSG_TXT];       // size is as big as considered necessary
    va_list vargs;

    // load format string from resource file
    Dbg(LoadString(g_hInst, wInfoFormat, (LPSTR)szInfoFormat, MAX_MSG_TXT));

    // set up szInfoText from passed parameters
    va_start(vargs, wInfoFormat);
    vsprintf(szInfoText, szInfoFormat, vargs);
    va_end(vargs);

    MsgBox(hwndFrame, (LPSTR)szInfoText, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);

    return;
}                                       /* InformationBox() */

/***    QuestionBox
**
**  Synopsis:
**      int = QuestionBox(wCaptionId, wMsgFormat, wType, ...)
**
**  Entry:
**
**  Returns:
**      The result of the message box call
**
**  Description:
**      Display an query box with combination of YES, NO and
**      CANCEL buttons and a question mark Icon.
**      See ErrorBox for discussion.
**
*/

int
CDECL
QuestionBox(
    WORD wMsgFormat,
    UINT wType,
    ...
    )
{
    char szMsgFormat[MAX_MSG_TXT];
    char szMsgText[MAX_VAR_MSG_TXT];
    va_list vargs;

    //Load format string from resource file
    Dbg(LoadString(g_hInst, wMsgFormat, (LPSTR)szMsgFormat, MAX_MSG_TXT));

    //Set up szMsgText from passed parameters
    va_start(vargs, wType);
    vsprintf(szMsgText, szMsgFormat, vargs);
    va_end(vargs);

    return MsgBox(hwndFrame, szMsgText,
        wType | MB_ICONEXCLAMATION | MB_TASKMODAL);
}                                       /* QuestionBox() */

/****************************************************************************

        FUNCTION:   QuestionBox2

        PURPOSE:    Display an query box with combination of YES, NO and
                                        CANCEL buttons and a question mark Icon. The type and
                                        the parent window are adjustable.

        RETURNS:                MessageBox result

****************************************************************************/
int
CDECL
QuestionBox2(
    HWND hwnd,
    WORD wMsgFormat,
    UINT wType,
    ...
    )
{
    char szMsgFormat[MAX_MSG_TXT];
    char szMsgText[MAX_VAR_MSG_TXT];
    va_list vargs;

    //Load format string from resource file
    Dbg(LoadString(g_hInst, wMsgFormat, (LPSTR)szMsgFormat, MAX_MSG_TXT));

    //Set up szMsgText from passed parameters
    va_start(vargs, wType);
    vsprintf(szMsgText, szMsgFormat, vargs);
    va_end(vargs);

    return MsgBox(hwnd, szMsgText, wType | MB_ICONEXCLAMATION);
}                                       /* QuestionBox2() */

/***    FatalErrorBox
**
**  Synopsis:
**      void = FatalErrorBox(iLine, szText)
**
**  Entry:
**      iLine   -
**      szText  -
**
**  Returns:
**      Nothing
**
**  Description:
**      This function will display an error message in a message box
**      which has the title of "Error".  The first paramter is an index
**      of a string in the resource file and the second parameter is
**      a literal which is displayed in the box.
**
*/

typedef struct _febargs {
    LPSTR lpLine2;
    WORD  wLine1;
} FEBARGS;

DWORD
FatalErrorBoxThread(
    LPVOID lpArgs
    )
{
    char text[MAX_MSG_TXT], buffer[MAX_VAR_MSG_TXT];
    FEBARGS *pfa = (FEBARGS *)lpArgs;

    if (pfa->wLine1 == 0) {
        *text = 0;
    } else {
        LoadString(g_hInst, pfa->wLine1, text, MAX_MSG_TXT);
    }

    if (pfa->lpLine2) {
        sprintf(buffer, "%s %s",text, pfa->lpLine2);
    } else {
        sprintf(buffer, "%s",text);
    }

#if DBG
    OutputDebugString(buffer);
    OutputDebugString("\n\r");
#endif

    strcat(buffer, "\r\nOK to ignore, CANCEL to break");

    return MessageBox(NULL,
        buffer,
        MainTitleText,
        MB_OKCANCEL | MB_ICONHAND | MB_TASKMODAL | MB_SETFOREGROUND);
}                                       /* FatalErrorBoxThread() */

void
FatalErrorBox(
    WORD wLine1,
    LPSTR lpLine2
    )
{
    FEBARGS fa;
    HANDLE hThread;
    DWORD dw;

    fa.wLine1 = wLine1;
    fa.lpLine2 = lpLine2;
    hThread = CreateThread(NULL, 0, FatalErrorBoxThread, &fa, 0, &dw);
    WaitForSingleObject(hThread, INFINITE);
    GetExitCodeThread(hThread, &dw);
    CloseHandle(hThread);
    if (dw == IDCANCEL) {
        DebugBreak();
    }
}                                       /* FatalErrorBox() */


/***    ShowAssert
**
**  Synopsis:
**      void = ShowAssert(szCond, iLine, szFile)
**
**  Entry:
**      szCond  - tokenized form of the failed condition
**      iLine   - Line number for the assertion
**      szFile  - File for the assertion
**
**  Returns:
**      void
**
**  Description:
**      Prepare and display a Message Box with szCondition, iLine and
**      szFile as fields.
**
*/

void
ShowAssert(
    LPSTR condition,
    UINT line,
    LPSTR file
    )
{
    char text[MAX_VAR_MSG_TXT];

    //Build line, show assertion and exit program

    sprintf(text, "- Line:%u, File:%Fs, Condition:%Fs",
        (WPARAM) line, file, condition);

    if (!AutoTest) {
        FatalErrorBox(ERR_Assertion_Failed, text);
    } else {
        char szBuffer[_MAX_PATH];
        PSTR pszBuffer;
        PSTR szAssertFile = "assert.qcw";
        HANDLE hFile = NULL;

        hFile = CreateFile(szAssertFile, GENERIC_WRITE, 0,
            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (INVALID_HANDLE_VALUE != hFile) {
            // Write the text out to the file
            DWORD dwBytesWritten = 0;

            Assert(WriteFile(hFile, text, strlen(text), &dwBytesWritten, NULL));
            Assert(strlen(text) == dwBytesWritten);
            CloseHandle(hFile);
        }

        CmdLogFmt( text );
        exit(3);
    }
}                                       /* ShowAssert() */

/***    MakePathNameFromProg
**
**  Synopsis:
**      void = MakePathName(extension, fileName)
**
**  Entry:
**      extension - Pointer to string containing the extension to be used
**      fileName  - pointer to buffer for the resulting file name
**
**  Returns:
**      Nothing
**
**  Description:
**      This function will take the module name, the passed in extension
**      and the current path to construct a full path name.  An example of
**      how this is used is in getting the ini file name.
*/

void
MakePathNameFromProg(
    LPSTR extension,
    LPSTR fileName
    )
{
    LPSTR       pcFileName;
    int         nFileNameLen;

    //
    //  Get the file name for the base module.
    //

    nFileNameLen = GetModuleFileName(g_hInst, fileName, _MAX_PATH);
    pcFileName = fileName + nFileNameLen;

    //
    //  Strip off any extensions found.  Assume that atleast .exe exists
    //
    //  M00BUG -- no extension on file name but a dot in the path.
    //

    while (pcFileName > fileName) {
        pcFileName = CharPrev(fileName, pcFileName);
        if (*pcFileName == '.') {
            *(++pcFileName) = '\0';
            break;
        }
        nFileNameLen -= (IsDBCSLeadByte(*pcFileName) ? 2 : 1);
    }

    //
    //  Make sure that the name is really an ansi string and that
    //  it is not too long.
    //

    OemToAnsi(fileName, fileName);
    if ((nFileNameLen+lstrlen(extension)) < _MAX_PATH) {
        lstrcat(fileName, extension);
    } else {
        ErrorBox(ERR_File_Name_Too_Long, fileName);
    }

    Assert(strlen(fileName) < _MAX_PATH);
}                                       /* MakePathNameFromProg() */

/***    MakeFileNameFromProg
**
**  Synopsis:
**      void = MakeFileNameFromProg(extension, fileName)
**
**  Entry:
**      extension - Pointer to string containing the extension to be used
**      fileName  - pointer to buffer for the resulting file name
**
**  Returns:
**      Nothing
**
**  Description:
**      This function will take the module name, the passed in extension
**      and the current path to construct a full path name.  An example of
**      how this is used is in getting the ini file name.
*/

void
MakeFileNameFromProg(
    LPSTR extension,
    LPSTR fileName
    )
{
    TCHAR   szPath[_MAX_PATH];
    TCHAR   szDrive[_MAX_DRIVE];
    TCHAR   szDir[_MAX_DIR];
    TCHAR   szFName[_MAX_FNAME];
    TCHAR   szExt[_MAX_EXT];
    int     nFileNameLen;

    //
    //  Get the file name for the base module.
    //

    nFileNameLen = GetModuleFileName(g_hInst, szPath, _MAX_PATH);
    _splitpath(szPath, szDrive, szDir, szFName, szExt);


    //
    //  Make sure that the name is really an ansi string and that
    //  it is not too long.
    //

    OemToAnsi(szFName, fileName);
    if ((strlen(fileName)+strlen(extension)) < _MAX_FNAME) {
        _fstrcat(fileName, extension);
    } else {
        ErrorBox(ERR_File_Name_Too_Long, fileName);
    }

    Assert(strlen(fileName) < _MAX_FNAME);
}                                       /* MakePathNameFromProg() */


/***    StartDialog
**
**  Synopsis:
**      bool = StartDialog(rcDlgNb, dlgProc)
**
**  Entry:
**      rcDlgNb - Resource number of dialog to be openned
**      dlgProc - Filter procedure for the dialog
**
**  Returns:
**      Result of the dialog box call
**
**  Description:
**      Loads and execute the dialog box 'rcDlgNb' (ressource
**      file string number) associated with the dialog
**      function 'dlgProc'
**
*/

BOOL
StartDialog(
    int rcDlgNb,
    DLGPROC dlgProc
    )
{
    DLGPROC lpDlgProc;
    LRESULT result;

    //TestRoutine();

    lpDlgProc = (DLGPROC)dlgProc;

    //
    //Execute Dialog Box
    //

    DlgEnsureTitleBar();
    BoxCount++;
    result = DialogBox(g_hInst,
                       MAKEINTRESOURCE(rcDlgNb),
                       hGetBoxParent(),
                       lpDlgProc);
    Assert(result != (LRESULT)-1);
    BoxCount--;

    return (result != 0);
}                                       /* StartDialog() */

/****************************************************************************

        FUNCTION:   InvDlgCtlIdRect

        PURPOSE:    Invalidate the rectangle area of child 'ctlID' in
                                        'hDlg' Dialog Box

****************************************************************************/
void
InvDlgCtlIdRect(
    HWND hDlg,
    int ctlID
    )
{
    HWND win;

    Dbg(win = GetDlgItem(hDlg, ctlID));
    InvalidateRect(win, (LPRECT)NULL, 1);
}                                       /* InvDlgCtlIdRect() */

/****************************************************************************

         FUNCTION:  RemoveMnemonic

         PURPOSE:   Get rid of accelerator mark

****************************************************************************/
void
RemoveMnemonic(
    LPSTR sWith,
    LPSTR sWithout
    )
{
    int k, j = 0;

    for (k = 0; k <= (int)lstrlen(sWith); k++) {
        if (IsDBCSLeadByte(sWith[k]) && sWith[k+1]) {
            sWithout[j++] = sWith[k++];
            sWithout[j++] = sWith[k];
        } else {
            if (sWith[k] != '&') {
                sWithout[j++] = sWith[k];
            } else if (sWith[k+1] == '&') {
                sWithout[j++] = sWith[k++];
            }
        }
    }
}                                       /* RemoveMnomonic() */


/***    fScanAnyLong
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

/****************************************************************************

        FUNCTION:   fScanAnyLong

        PURPOSE:    Converts the text form of a long value in
                    to the binary representation.  The language
                    parameter specifies the expected format of the
                    number.  If it is C then decimal, octal (leading 0)
                    and hex (leading 0x) are supported.  If it is
                    Pascal then decimal and hex (leading $) are
                    handled.  If it is AUTO then C is tried first
                    and Pascal if the conversion is unsuccessful.

        RETURNS:    TRUE if conversion is successful, FALSE if
                    not.  To return TRUE a number must be successfully
                    scanned AND it must fall within the passed max and
                    min values.

****************************************************************************/
BOOL
PASCAL
fScanAnyLong(
    LPSTR lpszSource,
    WORD wLanguage,
    long *plDest,
    long lMin,
    long lMax
    )
{
    long lLong = 0;
    PSTR pszTmp;
    BOOL fGotANumber = FALSE;
    char szSource[80];

    strncpy(szSource, lpszSource, sizeof(szSource)-1);
    szSource[sizeof(szSource)-1] = '\0';

    if ((wLanguage == PASCAL_LANGUAGE) || (wLanguage == AUTO_LANGUAGE)) {
        // look for '$' first
        // skip whitespace
        pszTmp = szSource;
        while (whitespace(*pszTmp)) pszTmp++;
        if (*pszTmp == '$') {
            pszTmp++;
            lLong = strtol(pszTmp, (PSTR *)NULL, 16);
            fGotANumber = TRUE;
        } else if (wLanguage == PASCAL_LANGUAGE) {
            // Try and read a decimal
            lLong = strtol(pszTmp, (PSTR *)NULL, 10);
            fGotANumber = TRUE;
        }
    }

    if (!fGotANumber) {
        lLong = strtol(szSource, (PSTR *)NULL, 0);
    }

    if ((lLong >= lMin) && (lLong <= lMax)) {
        *plDest = lLong;
        return TRUE;
    }

    return FALSE;
}                                       /* fScanAnyLong() */

/***    QCQPEnumFonts
**
**  Synopsis:
**      int = QCQPEnumFonts(lf, mtr, type, data)
**
**  Entry:
**      lf
**      mtr
**      type
**      data
**
**  Returns:
**
**  Description:
**      Enumerates fonts faces names or font chars sizes
**
*/

int
CALLBACK
QCQPEnumFonts(
    const LOGFONT * pLf,
    const TEXTMETRIC * mtr,
    DWORD fontType,
    LPARAM data
    )
{
#define NBOFSIZES 20

    switch (LOWORD(data)) {
    case 0:

        //Just count the fonts, but skip the Decoratives

        if (pLf->lfCharSet != SYMBOL_CHARSET
            && (pLf->lfPitchAndFamily & 0xF0) != FF_DECORATIVE) {

            fontsNb++;
        }
        return 1;

    case 1:
        if (pLf->lfCharSet != SYMBOL_CHARSET
            && (pLf->lfPitchAndFamily & 0xF0) != FF_DECORATIVE) {

            fonts[fontsNb] = *pLf;
            fonts[fontsNb].lfWidth = 0;
            fontsNb++;
        }
        return 1;

    case 2:
        //Just count the sizes for that font

        if (fontType & RASTER_FONTTYPE) {
            fontSizesNb++;
        } else {
            fontSizesNb = NBOFSIZES;
        }
        return 1;

    case 3:
        {
            int i = 0;

            if (fontType & RASTER_FONTTYPE) {

                //Insert the new element sorted in the array

                while (i < fontSizesNb && pLf->lfHeight > fontSizes[i]) {
                    i++;
                }

                if (i < fontSizesNb) {

                    //We don't insert the duplicates

                    if (pLf->lfHeight == fontSizes[i]) {
                        return 1;
                    }

                    //Insert element

                    memmove(fontSizes + i + 1, fontSizes + i,
                        (fontSizesNb - i) * sizeof(*fontSizes));
                    memmove(fontSizesPoint + i + 1, fontSizesPoint + i,
                        (fontSizesNb - i) * sizeof(*fontSizesPoint));
                }

                //Store size in pixels

                fontSizes[i] = pLf->lfHeight;

                //Store pixels converted into points

                fontSizesPoint[i] =
                    (int)((72L * (pLf->lfHeight - mtr->tmInternalLeading)
                    + (HIWORD((DWORD)data) / 2)) / (HIWORD((DWORD)data)));

                fontSizesNb++;
            } else {
                int k = 6;

                fontSizesNb = NBOFSIZES;

                //Put a set of sizes in the array

                for (i = 0 ; i < fontSizesNb; i++) {
                    fontSizesPoint[i] = k;
                    fontSizes[i] = (int)((k * (HIWORD((DWORD)data)) + 72L / 2) / 72L);
                    k +=2;
                }
            }
            return 1;
        }
    }

    // Stop enumerating
    return 0;
}                                       /* QCQPEnumFonts() */

/***    GetFontSizes
**
**  Synopsis:
**      void = GetFontSizes(hWnd, currentFont)
**
**  Entry:
**      hWnd    -
**      currentFont -
**
**  Returns:
**      nothing
**
**  Description
**
*/

void
GetFontSizes(
    HWND hWnd,
    int currentFont
    )
{
    HDC hDC;
    FONTENUMPROC EnumProc;

    hDC = GetDC(hWnd);

    //Dbg((LONG) (EnumProc = (FONTENUMPROC) MakeProcInstance((FARPROC)QCQPEnumFonts, g_hInst)));


    EnumProc = QCQPEnumFonts;

    //First query the number of sizes for this font

    fontSizesNb = 0;
    EnumFonts(hDC, fonts[currentFont].lfFaceName, EnumProc, 2);

    //Allocates the array to store the sizes and query the sizes

    if (fontSizes != NULL) {
        Xfree((LPSTR)fontSizes);
        Xfree((LPSTR)fontSizesPoint);
    }

    fontSizes = (LPINT)Xalloc(fontSizesNb * sizeof(*fontSizes));
    fontSizesPoint = (LPINT)Xalloc(fontSizesNb * sizeof(*fontSizesPoint));
    fontSizesNb = 0;
    EnumFonts(hDC, fonts[currentFont].lfFaceName, EnumProc,
        MAKELONG(3, GetDeviceCaps(hDC, LOGPIXELSY)));
    //FreeProcInstance(EnumProc);
    ReleaseDC(hWnd, hDC);
}                                       /* GetFontSizes() */

/***    LoadFonts
**
**  Synopsis:
**      void = LoadFonts(hWnd)
**
**  Entry:
**      hWnd    -
**
**  Returns:
**      nothing
**
**  Description:
**      Load available fonts
**
*/

void
LoadFonts(
    HWND hWnd
    )
{

    HDC hDC;
    FONTENUMPROC EnumProc;

    hDC = GetDC(hWnd);

    //Dbg((LONG) (EnumProc = (FONTENUMPROC) MakeProcInstance((FARPROC)QCQPEnumFonts, g_hInst)));

    EnumProc = QCQPEnumFonts;


    //First count the number of fonts
    fontsNb = 0;
    EnumFonts(hDC, NULL, EnumProc, 0);

    //Allocates the array to store the fonts and query the fonts
    if (fonts != NULL) {
        Xfree((LPSTR)fonts);
    }
    fonts = (LPLOGFONT)Xalloc(fontsNb * sizeof(*fonts));
    fontsNb = 0;
    EnumFonts(hDC, NULL, EnumProc, 1);
    fontCur = 0;
    //FreeProcInstance(EnumProc);
    ReleaseDC(hWnd, hDC);
}                                       /* LoadFonts() */


#if defined( NEW_WINDOWING_CODE )

HWND
New_OpenDebugWindow(
    WIN_TYPES   winType,
    BOOL        bUserActivated
    )
/*++

Routine Description:

    Opens Cpu, Watch, Locals, Calls, or Memory Window under MDI
    Handles special case for memory win's

Arguments:

    winType - Supplies Type of debug window to be openned
    
    bUserActivated - Indicates whether this action was initiated by the
                user or by windbg. The value is to determine the Z order of
                any windows that are opened.

Return Value:

    Window handle.

    NULL if an error occurs.

--*/
{
    HWND hwndActivate = 0;

    switch (winType) {
    default:
        Assert(!"Invalid window type. Ignorable error.");
        break;

    case WATCH_WINDOW:
        if (g_DebuggerWindows.hwndWatch) {
            hwndActivate = g_DebuggerWindows.hwndWatch;
        } else {
        }
        break;

    case LOCALS_WINDOW:
        if (g_DebuggerWindows.hwndLocals) {
            hwndActivate = g_DebuggerWindows.hwndLocals;
        } else {
        }
        break;

    case CPU_WINDOW:
        if (g_DebuggerWindows.hwndCpu) {
            hwndActivate = g_DebuggerWindows.hwndCpu;
        } else {
        }
        break;

    case DISASM_WINDOW:
        if (!bUserActivated && GetSrcMode_StatusBar()
            && NULL == g_DebuggerWindows.hwndDisasm
            && (g_contWorkspace_WkSp.m_dopDisAsmOpts & dopNeverOpenAutomatically) ) {

            return NULL;
        }

        if (g_DebuggerWindows.hwndDisasm) {
            hwndActivate = g_DebuggerWindows.hwndDisasm;
        } else {
        }
        break;

    case CMD_WINDOW:
        if (g_DebuggerWindows.hwndCmd) {
            hwndActivate = g_DebuggerWindows.hwndCmd;
        } else {
            return NewCmd_CreateWindow(g_hwndMDIClient);
        }
        break;

    case FLOAT_WINDOW:
        if (g_DebuggerWindows.hwndFloat) {
            hwndActivate = g_DebuggerWindows.hwndFloat;
        } else {
        }
        break;


    case MEMORY_WINDOW:
        if (NULL == g_DebuggerWindows.hwndMemory
            if (StartDialog(DLG_MEMORY, DlgMemory) != TRUE) {
                MessageBeep (0);
                return -1;         // No Debuggee or User Cancel out.
            }
        }
        break;


    case QUICKW_WINDOW:
        if (g_DebuggerWindows.hwndQuickW) {
            hwndActivate = g_DebuggerWindows.hwndQuickW;
        } else {
        }
        break;

    case CALLS_WINDOW:
        if (g_DebuggerWindows.hwndCalls) {
            hwndActivate = g_DebuggerWindows.hwndCalls;
        } else {
            OpenCallsWindow(winType, NULL, -1, bUserActivated);
        }
        break;

    }


    if (hwndActivate) {
        if (IsIconic(hwndActivate)) {
            OpenIcon(hwndActivate);
        }

        ActivateMDIChild(hwndActivate, bUserActivated);
    }    

    return hwndActivate;
}
    // New_OpenDebugWindow()

#else // NEW_WINDOWING_CODE

int
OpenDebugWindow(
    int       winType,
    BOOL      bUserActivated
    )
{
    return OpenDebugWindowEx(winType,
                             NULL,
                             -1,
                             bUserActivated
                             );
}

int
OpenDebugWindowEx(
    int       winType,
    LPWININFO lpWinInfo,
    int       nViewPreference,
    BOOL      bUserActivated
    )
/*++

Routine Description:

    Opens Cpu, Watch, Locals, Calls, or Memory Window under MDI
    Handles special case for memory win's

Arguments:

    winType - Supplies Type of debug window to be openned
    
    bUserActivated - Indicates whether this action was initiated by the
    lpWinInfo - window palcement preferences
    
    nViewPreference -  Prefered view position in array
    
                user or by windbg. The value is to determine the Z order of
                any windows that are opened.

Return Value:

    View number on success and -1 on failures

--*/
{
    HWND win = NULL;
    int  view = -1;

    switch (winType) {

    case WATCH_WIN:
    case LOCALS_WIN:
    case CPU_WIN:
    case FLOAT_WIN:
        //
        // Let the Pane Manager handle openning the window (it will
        //      check for prior existance), get the view number and
        //      return without using the document manager.
        //

        OpenPanedWindow(winType, 
                        lpWinInfo, 
                        nViewPreference, 
                        bUserActivated
                        );
        for (view=0; view < MAX_VIEWS; view++) {
            if (Views[view].Doc == -winType) {
                break;
            }
        }
        if (view == MAX_VIEWS) {
            view = -1;
        }
        return view;

    case CALLS_WIN:
        OpenCallsWindow(winType, 
                        lpWinInfo, 
                        nViewPreference, 
                        bUserActivated
                        );
        for (view=0; view < MAX_VIEWS; view++) {
            if (Views[view].Doc == -winType) {
                break;
            }
        }
        if (view == MAX_VIEWS) {
            view = -1;
        }
        return view;

    case DOC_WIN:
    case DISASM_WIN:
    case COMMAND_WIN:
        win = GetDebugWindowHandle((WORD) winType);

        if (DISASM_WIN == winType) {
            if (!bUserActivated && GetSrcMode_StatusBar()
                && NULL == win
                && (g_contWorkspace_WkSp.m_dopDisAsmOpts & dopNeverOpenAutomatically) ) {

                return -1;
            }
        }
        break;

    case MEMORY_WIN:
        if (NULL == lpWinInfo) {
            if (StartDialog(DLG_MEMORY, DlgMemory) != TRUE) {
                MessageBeep (0);
                return -1;         // No Debuggee or User Cancel out.
            }
        }
        break;

    default:
        //Assert(FALSE);
        return -1;
    }


    //
    //  Open the Debug windows to standard size or to previous
    //  size if already opened
    //

    if (win == NULL) {
        view = AddFile(MODE_CREATE, 
                       (WORD) winType, 
                       (LPSTR)szNull, 
                       lpWinInfo,
                       NULL, 
                       FALSE, 
                       -1, 
                       nViewPreference, 
                       bUserActivated
                       );
    } else {

        HWND MDIWin = GetParent(win);

        if (IsIconic(MDIWin)) {
            OpenIcon(MDIWin);
        }

        //SendMessage(g_hwndMDIClient, WM_MDIACTIVATE, (WPARAM) MDIWin, 0L);
        ActivateMDIChild(MDIWin, bUserActivated);

        for (view=0; view < MAX_VIEWS; view++) {
            if (Views[view].Doc < 0) {
                continue;
            }

            if (Docs[Views[view].Doc].docType == winType) {
                break;
            }
        }
        if (view == MAX_VIEWS) {
            view = -1;
        }
    }

    return view;
}
    // OpenDebugWindow()
#endif // NEW_WINDOWING_CODE


int
GotoLine(
    int    view,
    int    lineNbr,
    BOOL   fDebugger
    )
/*++

Routine Description:

    Move to the passed line in the passed view - returns
    line actually hit in case of adjustment.
    (NB lines are passed in range 1 => N)

Arguments:

    view      - Supplies the index of document window to do the goto in
    lineNbr   - Supplies the line number to go to
    fDebugger - Supplies TRUE if debugger is doing the goto

Return Value:

    lineNbr - Actual line number we went to

--*/

{
    Assert( Views[view].Doc >= 0);

    lineNbr = min(max(1, (int)lineNbr), (int)Docs[Views[view].Doc].NbLines) - 1;
    PosXYCenter(view, 0, lineNbr, fDebugger);
    return lineNbr;
}                                       /* GotoLine() */


void
LineStatus(
           int doc,
           int lineNbr,
           WORD state,
           LINESTATUSACTION action,
           BOOL positionInFirstView,
           BOOL redraw
           )

/*++

Routine Description:

    This routine is used to change the highlighting done on a per line
    basis.  This include tags, current line, breakpoints, errors.

Arguments:

    doc         - Supplies the document index
    lineNbr     - Supplies the 0 based line number to be changed
    state       - Supplies the state to be changed.
    action      - Supplies the TRUE/FALSE of the state to be set
    positionInFirstView - Supplies TRUE if the focues is to be changed to
                        the first view for the document in question
    redraw      - Supplies TRUE if a redraw is to be forced now

Return Value:

    None.

--*/

{
    LPLINEREC   pl;
    LPBLOCKDEF  pb;
    long        y;

    y = lineNbr = min(max(1, (int)lineNbr), (int)Docs[doc].NbLines) - 1;

    if (!FirstLine (doc, &pl, &y, &pb)) {
        Assert(FALSE);
        return;
    }

    switch (action) {
    case LINESTATUS_ON:
        SET(pl->Status, state);
        break;

    case LINESTATUS_OFF:
        RESET(pl->Status, state);
        break;

    case LINESTATUS_TOGGLE:
        TOGGLE(pl->Status, state);
        break;
    }

    CloseLine(doc, &pl, y, &pb);

    if (positionInFirstView) {
        PosXY(Docs[doc].FirstView, Views[Docs[doc].FirstView].X, lineNbr, TRUE);
    }

    if (redraw) {
        InvalidateLines(Docs[doc].FirstView, lineNbr, lineNbr, FALSE);

        if (! (IsIconic(GetParent(Views[Docs[doc].FirstView].hwndClient)) ||
            IsZoomed(GetParent(Views[Docs[doc].FirstView].hwndClient)))) {

            ShowWindow (GetParent(Views[Docs[doc].FirstView].hwndClient), SW_SHOWNORMAL);  //re-activate the iconized window gaining
            //current focus
        }


        ClearSelection (Docs[doc].FirstView);


        //  SendMessage (GetParent(Views[Docs[doc].FirstView].hwndClient), WM_MDIACTIVATE,0,0L);
        if (positionInFirstView) {
            ActivateMDIChild(Views[Docs[doc].FirstView].hwndFrame, FALSE);
        }
    }
}                                       /* LineStatus() */

/***    QueryLineStatus
**
**  Synopsis:
**      bool = QueryLineStatus(doc, lineNbr, state)
**
**  Entry:
**      doc     - which document to look in
**      lineNbr - which line to number look at the status for
**      state   - which status flags we are looking at
**
**  Returns:
**      TRUE if any of the flags are set and FALSE otherwise
**
**  Description:
**      This function is used to find out if any status flags are current
**      set on a line
*/

BOOL
QueryLineStatus(
                int doc,
                int lineNbr,
                UINT state
                )
{
    LPLINEREC   pl;
    LPBLOCKDEF  pb;
    long        y;

    //
    //  If we are not looking at a valid line then no status flags
    //          are ever set.
    //

    if ((lineNbr < 1) || (lineNbr > Docs[doc].NbLines-1)) {
        return FALSE;
    }

    y = lineNbr - 1;

    if (!FirstLine (doc, &pl, &y, &pb)) {
        Assert(FALSE);
        return FALSE;
    }

    CloseLine(doc, &pl, y, &pb);

    return (pl->Status & state) ? TRUE : FALSE;
}                                       /* QueryLineStatus() */

/***    FindDoc
**
**  Synopsis:
**      bool = FindDoc(fileName, doc, docOnly)
**
**  Entry:
**
**  Returns:
**      TRUE if document was found FALSE otherwise
**
**  Description:
**      Given a file Name, find doc #, if docOnly == TRUE then
**      the Debug windows are not included
**
**      Updates the location pointed to by doc with the document number
*/

BOOL FindDoc(
             LPSTR fileName,
             int *doc,
             BOOL docOnly)
{
    LPSTR   SrcFile = GetFileName( fileName );

    if ( SrcFile ) {
        if (docOnly) {
            for (*doc = 0; *doc < MAX_DOCUMENTS; (*doc)++) {
                if (Docs[*doc].docType == DOC_WIN && Docs[*doc].FirstView != -1
                    && _stricmp(GetFileName(Docs[*doc].szFileName), SrcFile) == 0) {

                    return TRUE;
                }
            }
        }
        else {
            for (*doc = 0; *doc < MAX_DOCUMENTS; (*doc)++) {
                if (Docs[*doc].FirstView != -1
                    && _stricmp(GetFileName(Docs[*doc].szFileName), SrcFile) == 0) {

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}


/***    FindDoc1
**
**  Synopsis:
**      bool = FindDoc1(fileName, doc, docOnly)
**
**  Entry:
**
**  Returns:
**      TRUE if document was found FALSE otherwise
**
**  Description:
**      Given a file Name, find doc #, if docOnly == TRUE then
**      the Debug windows are not included
**
**      Updates the location pointed to by doc with the document number
*/

BOOL
FindDoc1(
         LPSTR fileName,
         int *doc,
         BOOL docOnly
         )
{
    BOOL    (*lpCompareFileName)(LPSTR, LPSTR);
    LPSTR   SrcFile = GetFileName( fileName );

    if ( SrcFile ) {
#if 1
        if (strlen(SrcFile) == strlen(fileName)) {
            lpCompareFileName = CompareFileName;
        } else {
            lpCompareFileName = CompareFileNameWithPath;
            SrcFile = fileName;
        }
#else
        lpCompareFileName = CompareFileName;
#endif
        if (docOnly) {
            for (*doc = 0; *doc < MAX_DOCUMENTS; (*doc)++) {
                if (Docs[*doc].docType == DOC_WIN && Docs[*doc].FirstView != -1
                    && (*lpCompareFileName)(Docs[*doc].szFileName, SrcFile)) {

                    return TRUE;
                }
            }
        }
        else {
            for (*doc = 0; *doc < MAX_DOCUMENTS; (*doc)++) {
                if (Docs[*doc].FirstView != -1
                    && (*lpCompareFileName)(Docs[*doc].szFileName, SrcFile)) {

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

static
BOOL
CompareFileName(
                LPSTR szName1,
                LPSTR szName2
                )
{
    return (_stricmp(GetFileName(szName1), szName2) == 0);
}

static
BOOL
CompareFileNameWithPath(
                        LPSTR szName1,
                        LPSTR szName2
                        )
{
    return (_stricmp(szName1, szName2) == 0);
}


char *
GetFileName(
            IN  char * szPath
            )

/*++

Routine Description:

    Returns a pointer to the filename portion of a path

Arguments:

    szPath  -   Supplies the path

Return Value:

    char * - Pointer to the filename portion of the path.

--*/
{
    char *p = NULL;

    if ( *szPath ) {

        p = szPath + strlen(szPath );
        while (p > szPath) {
            p = CharPrev(szPath, p);
            if (*p == '\\' || *p == '/') {
                break;
            }
        }
        if ((*p == '\\' ) || (*p == '/' )) {
            p++;
        }
    }

    return p;
}





/****************************************************************************

        FUNCTION:       ClearDocStatus

        PURPOSE:    Given a doc, clear all visual breakpoints, tags or
                                        compile errors in the views in that doc.

****************************************************************************/
void ClearDocStatus(
                    int doc,
                    WORD state)
{
    LPLINEREC pl;
    LPBLOCKDEF      pb;
    long y;

    y = 0;

    if (!FirstLine(doc, &pl, &y, &pb)) {
        return;
    }

    while (TRUE) {
        if (pl->Status & state) {
            pl->Status &= ~state;
            InvalidateLines(Docs[doc].FirstView, y - 1, y - 1, FALSE);
        }
        if (y >= Docs[doc].NbLines) {
            break;
        } else if (!NextLine(doc, &pl, &y, &pb)) {
            return;
        }
    }
    CloseLine(doc, &pl, y, &pb);
}                                       /* ClearDocStatus() */


/****************************************************************************

        FUNCTION:       ClearAllDocStatus

        PURPOSE:    Clear all visual breakpoints, tags or       compile errors
                                        that in all docs.

****************************************************************************/
void
ClearAllDocStatus(
                  WORD state
                  )
{
    int d;

    for (d = 0; d < MAX_DOCUMENTS; d++) {
        if (Docs[d].FirstView != -1) {
            ClearDocStatus(d, state);
        }
    }
}                                       /* ClearAllDocStatus() */


/****************************************************************************

        FUNCTION:       VisibleListboxLines

        PURPOSE:    Return the number of visible lines in the list box
                                associated with the passed handle.
                                Note that this function assumes that each list box item
                                is the same height.

****************************************************************************/
WORD
VisibleListboxLines(
                    HWND hListbox
                    )
{
    RECT ListRect;      // Dims of client area of listbox
    RECT ItemRect;      // Dims of an item within listbox

    GetClientRect(hListbox, (LPRECT)&ListRect);
    Dbg(SendMessage(hListbox, LB_GETITEMRECT, 0, (LPARAM)(LPRECT)&ItemRect)!=LB_ERR);

    return ((WORD)((ListRect.bottom - ListRect.top + 1) /
                          (ItemRect.bottom - ItemRect.top)));
}                                       /* VisibleListboxLines() */

/***    AjustFullPathName
**
**  Synopsis:
**      void = AdjustFullPathName(fullPath, adjustedPath, len
**
**  Entry:
**
**  Returns:
**      nothing
**
**  Description:
**      Adjust Full Path name to fit in a specified len string
**              Priority rules for reduction are :
**                      - FileName.extension
**                      - Drive
**                      - Nth dir
**                      - (N-1)th dir
**                      - ...
**                      - 1st dir
*/

void
AdjustFullPathName(
    PCSTR fullPath,
    PSTR adjustedPath,
    int len
    )
{
    int     remain = len;
    PSTR    cur;
    TCHAR   szDrive[_MAX_DRIVE];    
    TCHAR   szDir[_MAX_DIR];    
    TCHAR   szFName[_MAX_FNAME];    
    TCHAR   szExt[_MAX_EXT];    

    //Assert(len >= MAXFILENAMELEN);

    if ((int)strlen(fullPath) <= len) {
        strcpy(adjustedPath, fullPath);
        return;
    }

    *adjustedPath = '\0';

    _splitpath(fullPath, szDrive, szDir, szFName, szExt);

    remain -= (strlen(szFName) + strlen(szExt));

    //Try to add the drive
    if (szDrive) {
        if (remain >= (int)strlen(szDrive)) {
            strcat(adjustedPath, szDrive);
            remain -= strlen(szDrive);
        } else {

            //No space, try to add some "." templates
            while (remain-- > 0) {
                strcat(adjustedPath, ".");
            }
            goto rebuild;
        }
    }

    //Extract dirs until we have no more space left
    if (szDir) {
        cur = szDir + strlen(szDir) - remain;

        if (cur <= szDir ) {
            //We have space to copy all dirs
            strcat(adjustedPath, szDir);
        } else if (remain < 5) {
            //If we had the drive and there is a dir, better get rid
            //of the drive to avoid to return a wrong pathName.
            if (szDrive && strlen(szDrive) >= 2) {
                strcpy(adjustedPath, "..");
            }
        } else {
            //Add "\..\" template, if we have space left

            strcat(adjustedPath, "\\...");
            remain -= 4;

            if (remain == 1) {
                //Just space to add a "\"
                strcat(adjustedPath, "\\");
            } else {
                //We have no space to copy all dirs, synchronize to next
                //dir beginning and add it to reduced path name
                cur = szDir + strlen(szDir) - remain;
                while (*cur && *cur != '\\') {
                    if (IsDBCSLeadByte(*cur) && *(cur+1)) {
                        cur += 2;
                    } else {
                        cur++;
                    }
                }
                if (*cur) {
                    strcat(adjustedPath, cur);
                } else {
                    strcat(adjustedPath, "\\");
                }
            }
        }
    }

rebuild:

    //Rebuild FileName.extension
    remain = len - strlen(adjustedPath);
    strncat(adjustedPath, szFName, remain);
    remain -= strlen(szFName);
    if (remain > 0) {
        strncat(adjustedPath, szExt, remain);
    }
}                                       /* AdjustFullPathName() */

// BUGBUG - kcarlos - dead code
#if 0
/***    ReadIni
**
**  Synopsis:
**      bool = ReadIni(hFile, lpBuffer, wBytes)
**
**  Entry:
**
**  Returns:
**
**  Description:
**      ReadIni to file and add to checksum
**
*/

BOOL
ReadIni(
        int hFile,
        LPSTR lpBuffer,
        int wBytes
        )
{
    //Read File Buffer

    if (_lread(hFile,  lpBuffer, wBytes) != (UINT) wBytes) {
        return ErrorBox2(hwndFrame, MB_TASKMODAL, ERR_Ini_File_Read, (LPSTR)iniFileName);
    }

    return TRUE;
}                                       /* ReadIni() */

/***    WriteAndSum
**
**  Synopsis:
**      bool = WriteAndSum(hFile, lpBuffer, wBytes)
**
**  Entry:
**      hFile   - Handle to file to do the write on
**      lpBuffer - Buffer containning bytes to be written
**      wBytes  - count of bytes to be written
**
**  Returns:
**      TRUE on a sucessful write and FALSE on failure
**
**  Description:
**      This function will write data out to the file connected to the
**      file handle and to compute a checksum on the data being written
**      out.  The checksum is a sum of all bytes written.
**
*/

BOOL
WriteAndSum(
            int hFile,
            LPSTR lpBuffer,
            int wBytes
            )
{
    register int i;

    //Add buffer bytes to checksum

    for (i = 0; i < wBytes; i++) {
        checkSum += (BYTE)lpBuffer[i];
    }

    //Write File Buffer

    if (_lwrite(hFile,  lpBuffer, wBytes) != (UINT) wBytes) {
        return ErrorBox2(hwndFrame, MB_TASKMODAL, ERR_Ini_File_Write, (LPSTR)iniFileName);
    }

    return TRUE;
}                                       /* WriteAndSum() */
#endif


/***    FailGlobalLock
**
**  Synopsis:
**      bool = FailGlobalLoc(h, p)
**
**  Entry:
**
**  Returns:
**      TRUE if locked and FALSE if failed to lock
**
**  Description:
**      Try to lock a global memory handle
*/

BOOL
FailGlobalLock(
               HANDLE h,
               LPSTR *p
               )
{
    if ((*p = (PSTR) GlobalLock (h)) == 0) {
        Dbg(GlobalFree (h) == NULL);
        return TRUE;
    }
    return FALSE;
}                                       /* FailGlobalLock() */

/***    GetCurrentText
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**      FALSE if there is a problem or nothing to get
**
**  Description:
**      Retrieve in view the word at cursor, or the first line
**      of text if selection is active. If not NULL, beginOffset
**      and endOffset return number of chars relative to cursor
**      position. If a selection is active and is over several
**      lines   and cursor in selection is not above a word and
**      lookAround is TRUE, the word at left is returned and
**      'lookAround' is TRUE, otherwise lookAround is FALSE.
**
*/

BOOL
GetCurrentText(
               int view,
               BOOL *lookAround,
               LPSTR pText,
               int maxSize,
               LPINT xLeft,
               LPINT xRight)
{
    LPVIEWREC v = &Views[view];

    //If selection active
    if (v->BlockStatus) {
        //Returns selected text
        return GetSelectedText(view, lookAround, (LPSTR)pText,
        maxSize, xLeft, xRight);
    } else {
        //No selection, send word at cursor
        return GetWordAtXY(view, v->X, v->Y, FALSE, lookAround, TRUE,
            (LPSTR)pText, maxSize, xLeft, xRight);
    }
}                                       /* GetCurrentText() */


/***    TimeIn100ths
**
**  Synopsis:
**      ulong = TimeIn100ths()
**
**  Entry:
**      None
**
**  Return:
**      the current time in 100ths of a second
**
**  Description:
**      This function will look at the clock and return the current time
**      routined to 100ths of a second
*/

ULONG
PASCAL
TimeIn100ths()
{
#ifdef WIN32
    return GetTickCount();
#else
    struct _dostime_t time;

    _dos_gettime(&time);
    return (((((time.hour*60UL)+time.minute)*60)+time.second)*100)+time.hsecond;
#endif
}                                       /* TimeIn100ths() */


/****************************************************************************

        FUNCTION:       StopTimeIn100ths

        PURPOSE:    Returns the current time + the passed delay in 100ths
                                        of a second.  NB takes into account the time period
                                        crossing midnight.

        RETURNS:    See above.

****************************************************************************/
/***    StopTimeIn100ths
**
**  Synopsis:
**      ulong = StopTimeIn100ths( DelayIn100ths)
**
**  Entry:
**      DelayIn100ths - delay to add to current time
**
**  Return:
**      Current time plus the delay request in 100ths of a second
**
**  Description:
**      This routine will take the current system time and add the requested
**      delay to it.  The result in 100ths of a second will then be returned
*/

ULONG
PASCAL
StopTimeIn100ths(
                 ULONG DelayIn100ths
                 )
{
    return (TimeIn100ths() + DelayIn100ths) % HUNDREDTHS_IN_A_DAY;
}

/***    ProcessQCQPMessage
**
**  Synopsis:
**      void = ProcessQCQPMessage(lpmsg)
**
**  Entry:
**      lpmsg   - The message to be processed and forwarded as appropriate
**
**  Returns:
**      nothing:
**
**  Description:
**      Performs translation and dispatching of a message
**      received by QCQP.  Should be called by anyone that
**      does a Get/Peek etc Message.
**
*/

void
PASCAL
ProcessQCQPMessage(
                   LPMSG lpMsg
                   )
{
    //If a keyboard message is for the MDI , let the MDI client
    //take care of it.  Otherwise, check to see if it's a normal
    //accelerator key (like F3 = find next).  Otherwise, just handle
    //the message as usual.
    if (!TranslateMDISysAccel(g_hwndMDIClient, lpMsg) &&
        !TranslateAccelerator(hwndFrame, hCurrAccTable, lpMsg)) {

        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
    }
}                                       /* ProcessQCQPMessage() */

/***    AppendFilter
**
**
**  Description:
**      Append a filter to an existing filters string.
**
*/

void
AppendFilter(
             WORD filterTextId,
             int filterExtId,
             LPSTR filterString,
             int *len,
             int maxLen
             )
{
    int size;

    //Append filter text

    Dbg(LoadString(g_hInst, filterTextId, (LPSTR)szTmp, MAX_MSG_TXT));
    size = strlen(szTmp) + 1;
    Assert(*len + size <= maxLen);
    memmove(filterString + *len, szTmp, size);
    *len += size;

    //Append filter extension

    Dbg(LoadString(g_hInst, filterExtId, (LPSTR)szTmp, MAX_MSG_TXT));
    size = strlen(szTmp) + 1;
    Assert(*len + size < maxLen);
    memmove(filterString + *len, szTmp, size);
    *len += size;
}                                       /* AppendFilter() */

/***    InitFilterString
**
**  Description:
**      Initialize file filters for file dialog boxes.
*/

void
InitFilterString(
                 WORD titleId,
                 LPSTR filter,
                 int maxLen
                 )
{
    int len = 0;

    switch (titleId) {
    case DLG_Browse_CrashDump_Title:
        AppendFilter(TYP_File_DUMP, DEF_Ext_DUMP, filter, &len, maxLen);
        break;

    case DLG_Browse_Executable_Title:
        AppendFilter(TYP_File_EXE, DEF_Ext_EXE, filter, &len, maxLen);
        break;

    case DLG_Browse_LogFile_Title:
        AppendFilter(TYP_File_LOG, DEF_Ext_LOG, filter, &len, maxLen);
        break;

    case DLG_SaveAs_Filebox_Title:
    case DLG_Open_Filebox_Title:
    case DLG_Merge_Filebox_Title:
    case DLG_Browse_Filebox_Title:
        AppendFilter(TYP_File_SOURCE, DEF_Ext_SOURCE, filter, &len, maxLen);
        AppendFilter(TYP_File_INCLUDE, DEF_Ext_INCLUDE, filter, &len, maxLen);
        AppendFilter(TYP_File_ASMSRC, DEF_Ext_ASMSRC, filter, &len, maxLen);
        AppendFilter(TYP_File_INC, DEF_Ext_INC, filter, &len, maxLen);
        AppendFilter(TYP_File_RC, DEF_Ext_RC, filter, &len, maxLen);
        AppendFilter(TYP_File_DLG, DEF_Ext_DLG, filter, &len, maxLen);
        AppendFilter(TYP_File_DEF, DEF_Ext_DEF, filter, &len, maxLen);
        AppendFilter(TYP_File_MAK, DEF_Ext_MAK, filter, &len, maxLen);
        break ;

    case DLG_Browse_DbugDll_Title:
        AppendFilter(TYP_File_DLL, DEF_Ext_DLL, filter, &len, maxLen);
        break;

    default:
        DAssert(FALSE);
        break;
    }

    AppendFilter(TYP_File_ALL, DEF_Ext_ALL, filter, &len, maxLen);
    filter[len] = '\0';
}                                       /* InitFilterString() */


/***    GetDebugWindowHandle
**
**  Synopsis:
**      hwnd = GetDebugWindowHandle(type)
**
**  Entry:
**      type    - Debug window type to get the handle for
**
**  Returns:
**      The handle for this type of debug window if one exists, otherwise
**      it returns NULL
**
**  Description:
**      This function is used to obtain the window handle for the
**      single instance of the debug window of class type.
**
*/

HWND
GetDebugWindowHandle(
                     WORD type
                     )
{
    int v;

    if ((type == CPU_WIN) || (type == FLOAT_WIN) || (type == WATCH_WIN) ||
        (type == LOCALS_WIN) || (type == CALLS_WIN)) {

        type = -type;
    }

    for (v = 0; v < MAX_VIEWS; v++) {
        if (type < -1) {
            if (Views[v].Doc == (int) type) {
                return Views[v].hwndClient;
            }
        } else if ((Views[v].Doc != -1) &&
            (Docs[Views[v].Doc].docType == type)) {

            return Views[v].hwndClient;
        }
    }
    return NULL;
}                                       /* GetDebugWindowHandle() */

/***    SetVerticalScrollBar
**
**  Synopsis:
**      void = SetScrollBar(view, propagate)
**
**  Entry:
**      view      - document to set scroll bars for
**      propagate - Modify just this view or all other views on same document
**
**  Returns:
**      Nothing
**
**  Description:
**      This function is used to adjust the range of the vertical scroll bar
**      for a document.  It will be set according to the number of lines in
**      the document.  If desired then all views of the same document may have
**      their scroll bars adjusted at the same time.  If so then the cursor
**      for the other windows will be verified as within the current line
**      count.
*/

void
SetVerticalScrollBar(
                     int view,
                     BOOL propagate
                     )
{
    int n = Docs[Views[view].Doc].NbLines - 1;

    DAssert(Views[view].Doc >= 0);

    // Need to work out disasmbler scroll bars later
    if (view == disasmView) {
        return;
    }

    // Only handle multiple views for DOC_WIN

    if (!propagate || Docs[Views[view].Doc].docType != DOC_WIN) {

        SetScrollRange(Views[view].hwndClient, SB_VERT, 0, max(n,1), TRUE);

    } else {

        int k = Docs[Views[view].Doc].FirstView;
        LPVIEWREC v;

        while (k != -1) {
            v = &Views[k];

            //Possibly adjust cursor position

            if (v->Y > n) {
                v->Y = n;
                if (v->BlockYR > n) {
                    v->BlockYR = n;
                }
                if (v->BlockYL > n) {
                    v->BlockYL = n;
                }
            }
            k = v->NextView;
            SetScrollRange(v->hwndClient, SB_VERT, 0, max(n,1), TRUE);

        }
    }
}                                       /* SetVerticalScrollBar() */

/***    AuxPrintf
**
**  Synopsis:
**      bool = AuxPrintf(iReqDebLevel, sz, ...)
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Makes a printf style command for output on debug console
*/

#undef AuxPrintf
BOOL
AuxPrintf(
          int iReqDebLevel,
          LPSTR text,
          ...
          )
{
#if !REMOVEAUXPRINTFS
    char buffer[MAX_MSG_TXT];
    int fdComm;
    va_list vargs;

    if (iReqDebLevel > iDebugLevel)
        return(FALSE);

    va_start(vargs, text);
    vsprintf(buffer, text, vargs);
    va_end(vargs);
    lstrcat(buffer,"\r\n");

    // By-pass OutputDebugString, otherwise we'll get notifications
    // about these things when debugging (DBG_N_InfoAvail)
    //!OutputDebugString(buffer);
    if ((fdComm = _lopen("com1", OF_WRITE)) != -1) {
        _lwrite(fdComm, buffer, strlen(buffer));
        _lclose(fdComm);
    }

    OutputDebugString(buffer);
#endif
    return FALSE;
}                                       /* AuxPrintf() */


BOOL
FindNameOn(
    PSTR     pszDest,
    UINT     cchDest,
    PCTSTR    pszPaths,
    PCTSTR    pszFile
    )
/*++

Routine Description:

    This function will look in each of the directories specified in
    the lszPaths variable for the relative path lszFile.  If the file
    if fond to exist then the resulting name will be returned in
    lszDest and TRUE is returned.

Arguments:

    pszDest - Supplies a buffer to place resulting name
    cchDest - Supplies the number of bytes in buffer
    pszPaths - Supplies a semi-colon seperated list of directories to search
    pszFile -  Supplies a file name to look for

Return Value:

    TRUE if file was found and FALSE otherwise.

--*/
{
    PCSTR   psz1;
    PCSTR   psz2;
    char    ch;
    char    rgchT[MAX_PATH*2];

    psz1 = pszPaths;

    if (psz1 == NULL) {
        return FALSE;
    }

    while (*psz1 != 0) {
        psz2 = psz1;
        while ((*psz2) && (*psz2 != ';')) {
            psz2 = CharNext(psz2);
        }

        strncpy(rgchT, psz1, psz2-psz1);
        rgchT[psz2-psz1] = 0;
        strcat(rgchT, "\\");
        strcat(rgchT, pszFile);

        if (_fullpath(pszDest, rgchT, cchDest) != NULL) {
            if (FileExist(pszDest)) {
                return TRUE;
            }
        }

        psz1 = psz2;

        if (*psz1 == ';') {
            psz1++;
        }
    }

    return FALSE;
}                                       /* FindNameOn() */


BOOL
SetDriveAndDir(
    const TCHAR * const pszTargPath
    )
{
    TCHAR       szTmp[_MAX_PATH] = {0};
    TCHAR       szDrive[_MAX_DRIVE] = {0};
    TCHAR       szDir[_MAX_DIR];
    TCHAR       szFName[_MAX_FNAME];
    TCHAR       szExt[_MAX_EXT];
    int         nDirLen;


    _tsplitpath(pszTargPath, szDrive, szDir, szFName, szExt);

    //Set current drive and dir
    if (szDrive[0] != 0) {
        if (_chdrive((int)(toupper(szDrive[0]) - 'A' + 1)) != 0) {
            return ErrorBox(ERR_Change_Drive, (LPSTR)szDrive);
        }
    }

    nDirLen = strlen(szDir);
    AnsiToOem(szDir, szTmp);
    if (nDirLen > 0) {
        if (_tcslen(szTmp) > 1 && *CharPrev(szTmp, szTmp + nDirLen - 1) == '\\') {
            szTmp[--nDirLen] = 0;
        }
        if (_chdir(szTmp) != 0) {
            OemToAnsi(szTmp, szDir);
            return ErrorBox(ERR_Change_Directory, (LPSTR)szDir);
        }
    }

    return TRUE;
}


BOOL
GetFileTimeByName(
                  LPSTR FileName,
                  LPFILETIME lpCreationTime,
                  LPFILETIME lpLastAccessTime,
                  LPFILETIME lpLastWriteTime
                  )
{
    BOOL Status;
    HANDLE FileHandle;

    FileHandle = CreateFile(
        FileName,
        GENERIC_READ,
        FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
        );

    if (FileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    Status = GetFileTime(
        FileHandle,
        lpCreationTime,
        lpLastAccessTime,
        lpLastWriteTime
        );

    CloseHandle(FileHandle);

    return Status;
}

//*******************************************************************
//*******************************************************************
#ifdef DEBUGGING

ULARGE_INTEGER startTime;
ULARGE_INTEGER stopTime;

void
ShowElapsedTime()
{
    ULONG64 uSecs;
    ULONG64 uMilliSecs;

    // convert from nano (10^-9) to milli (10^-3)
    uMilliSecs = (stopTime.QuadPart - startTime.QuadPart) / 1000000;
    
    // Convert to whole seconds
    uSecs = uMilliSecs / 1000;

    // Calc millisecond remainder
    uMilliSecs %= 1000;
    
    AuxPrintf(1, "%u' %u''", uSecs, uMilliSecs);
}

void
StartTimer()
{
    GetSystemTimeAsFileTime( (PFILETIME) &startTime );
}

void
StopTimer()
{
    GetSystemTimeAsFileTime( (PFILETIME) &stopTime );
}

/****************************************************************************

        FUNCTION:   InfoBox

        PURPOSE:                Opens a Dialog box with a title and accepting
                                        a printf style for text. It's for DEBUGGING USE ONLY

        RETURNS:                MessageBox result

****************************************************************************/
int
InfoBox(
        LPSTR text,
        ...
        )
{
    char buffer[MAX_MSG_TXT];
    va_list vargs;

    va_start(vargs, text);
    vsprintf(buffer, text, vargs);
    va_end(vargs);
    return MsgBox(GetActiveWindow(), buffer, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
}

#endif //debugging


VOID
InvalidateAllWindows()
{
    SendMessageNZ( GetCpuHWND(),   WU_INVALIDATE, 0, 0L);
    SendMessageNZ( GetFloatHWND(), WU_INVALIDATE, 0, 0L);
    SendMessageNZ( GetLocalHWND(), WU_INVALIDATE, 0, 0L);
    SendMessageNZ( GetWatchHWND(), WU_INVALIDATE, 0, 0L);
    SendMessageNZ( GetCallsHWND(), WU_INVALIDATE, 0, 0L);
}


/***************************************************************************



***************************************************************************/
VOID
GetDBCSCharWidth(
                 HDC hDC,
                 LPTEXTMETRIC ptm,
                 LPVIEWREC lpv
                 )
{
    SIZE    Size;

    if (ptm->tmCharSet == SHIFTJIS_CHARSET) {
        if (0 == (ptm->tmPitchAndFamily & TMPF_FIXED_PITCH)
            &&  ptm->tmMaxCharWidth == ptm->tmAveCharWidth * 2) {

            lpv->wViewPitch = VIEW_PITCH_ALL_FIXED;
        } else {
            lpv->wViewPitch = VIEW_PITCH_DBCS_FIXED;
        }
    } else {
        if (0 == (ptm->tmPitchAndFamily & TMPF_FIXED_PITCH)) {
            lpv->wViewPitch = VIEW_PITCH_ALL_FIXED;
        } else {
            lpv->wViewPitch = VIEW_PITCH_VARIABLE;
        }
    }
    GetTextExtentPoint((hDC), DBCS_CHAR, 2, &Size);
    lpv->charWidthDBCS = Size.cx - ptm->tmOverhang;
    lpv->wCharSet = ptm->tmCharSet;
}

/***************************************************************************

    Modify UndoRedo record

***************************************************************************/
VOID
SetReplaceDBCSFlag(
    LPDOCREC lpd,
    BOOL     bTwoRec
    )
{
    STREAMREC *pst;

    if (DOC_WIN != lpd->docType) {
        return;
    }

    //It is bad to update undo information directly...
    pst = (STREAMREC *)((LPSTR)lpd->undo.pRec + lpd->undo.offset);

    if (bTwoRec) {
        // This is because lpd->undo.offset points the undo record
        // for inserted chars. Previous record is for deleted chars.
        pst = (STREAMREC *)((LPSTR)pst - pst->prevLen);
    }
    pst->action |= REPLACEDBCS;
}

/***************************************************************************

    Set position of IME conversion window

***************************************************************************/
LRESULT
ImeMoveConvertWin(
    HWND hwnd,
    INT  x,
    INT  y
    )
{
    HANDLE  hIME;
    LPIMESTRUCT lpIme;
    LRESULT lrRet;

    if (NULL == lpfnSendIMEMessageEx) {
        return FALSE;
    }

    if(!(hIME = GlobalAlloc(GHND | GMEM_SHARE, (DWORD)sizeof(IMESTRUCT)))){
        return FALSE;
    }
    if(!(lpIme = (LPIMESTRUCT)GlobalLock(hIME))){
        return FALSE;
    }

    // Set IME_SETCONVERSIONWINDOW as a sub-function number
    lpIme->fnc = IME_SETCONVERSIONWINDOW;

    // if x == -1 && y == -1 then set default conversion ID
    if (x == -1 && y == -1) {
        lpIme->wParam = MCW_DEFAULT;
    } else {
        RECT    rRect;

        GetClientRect(hwnd, &rRect);
        // Set spot conversion ID, and a position of a conversion window
        lpIme->wParam = MCW_WINDOW | MCW_RECT;
        lpIme->lParam1 = MAKELONG(LOWORD(x), LOWORD(y));
        lpIme->lParam2 = MAKELONG(LOWORD(rRect.left), LOWORD(rRect.top));
        lpIme->lParam3 = MAKELONG(LOWORD(rRect.right), LOWORD(rRect.bottom));
    }
    GlobalUnlock(hIME);

    lrRet = (*lpfnSendIMEMessageEx)(hwnd, (LPARAM)hIME);
    GlobalFree(hIME);
    return lrRet;
}

/***************************************************************************

    Send virtual key message to IME

***************************************************************************/
LRESULT
ImeSendVkey(
            HWND hwnd,
            WORD wVKey
            )
{
    HANDLE  hIME;
    LPIMESTRUCT lpIme;
    LRESULT lrRet;

    if (NULL == lpfnSendIMEMessageEx) {
        return FALSE;
    }

    if(!(hIME = GlobalAlloc(GHND | GMEM_SHARE, (DWORD)sizeof(IMESTRUCT)))){
        return FALSE;
    }
    if(!(lpIme = (LPIMESTRUCT)GlobalLock(hIME))){
        return FALSE;
    }
    lpIme->fnc = IME_SENDVKEY;
    lpIme->wParam = wVKey;
    GlobalUnlock(hIME);

    lrRet = (*lpfnSendIMEMessageEx)(hwnd, (LPARAM)hIME);
    GlobalFree(hIME);
    return lrRet;
}

/****************************************************************************

    FUNCTION   : ImeSetFont

    PURPOSE    : Specify the font which is used in IME conversion window

****************************************************************************/
BOOL
ImeSetFont(
           HWND hwnd,
           HFONT hFont
           )
{
    HANDLE  hIME;
    LPIMESTRUCT lpIme;
    HANDLE      hLF;
    LPLOGFONT   lpLF;

    if (NULL == lpfnSendIMEMessageEx) {
        return FALSE;
    }

    if (!(hIME = GlobalAlloc(GHND | GMEM_SHARE, (DWORD)sizeof(IMESTRUCT)))) {
        return FALSE;
    }
    if (!(lpIme = (LPIMESTRUCT)GlobalLock(hIME))) {
        return FALSE;
    }

    if (!(hLF = GlobalAlloc(GHND | GMEM_SHARE, sizeof(LOGFONT)))) {
        GlobalUnlock(hIME);
        GlobalFree(hIME);
        return FALSE;
    }

    if (!(lpLF = (LPLOGFONT)GlobalLock(hLF))) {
        GlobalFree(hLF);
        GlobalUnlock(hIME);
        GlobalFree(hIME);
        return FALSE;
    }

    if (!GetObject(hFont, sizeof(LOGFONT), lpLF)) {
        GlobalUnlock(hLF);
        GlobalFree(hLF);
        GlobalUnlock(hIME);
        GlobalFree(hIME);
        return FALSE;
    }

    GlobalUnlock(hLF);

    // Set IME sub-function number
    lpIme->fnc = IME_SETCONVERSIONFONTEX;

    lpIme->lParam1 = (LPARAM)hLF;

    GlobalUnlock(hIME);

    (*lpfnSendIMEMessageEx)(hwnd, (LPARAM)hIME);

    GlobalFree(hLF);
    GlobalFree(hIME);

    return TRUE;
}

/****************************************************************************

    FUNCTION   : ImeWINNLSEnableIME

    PURPOSE    :

****************************************************************************/
BOOL
ImeWINNLSEnableIME(
                   HWND hwnd,
                   BOOL bEnable
                   )
{
    if (lpfnWINNLSEnableIME) {
        return (*lpfnWINNLSEnableIME)(hwnd, bEnable);
    } else {
        return FALSE;
    }
}

/***************************************************************************

    Proccess WM_IME_REPORT:IR_STRING message

***************************************************************************/
BOOL
ProccessIMEString(
    HWND hwnd,
    LPARAM lParam
    )
{
    int         view = GetWindowWord(hwnd, GWW_VIEW);
    LPVIEWREC   v = &Views[view];
    LPDOCREC    d = &Docs[v->Doc];
    LPLINEREC   pl;
    LPBLOCKDEF  pb;
    LPTSTR      lpsz;
    int         nLen1;
    BOOL        bRet;

    if (!(lpsz = (PSTR) GlobalLock((HANDLE)lParam))) {
        return FALSE;
    }

    if (!FirstLine(v->Doc, &pl, &(v->Y), &pb)) {
        GlobalUnlock ((HANDLE)lParam);
        return FALSE;
    }
    CloseLine(v->Doc, &pl, v->Y, &pb);
    v->Y--;

    nLen1 = lstrlen(lpsz);

    if ((v->X < elLen - 1)
        && (GetOverType_StatusBar() || d->forcedOvertype) && !v->BlockStatus) {
        /**********************************************/
        /* if over-write mode and no text is selected */
        /**********************************************/
        v->BlockStatus = TRUE;
        v->BlockXL = v->X;
        v->BlockYL = v->Y;
        v->BlockYR = v->Y;

        if (v->X + nLen1 >= elLen) {
            v->BlockXR = elLen - 1;
        } else if (v->bDBCSOverWrite) {
            int i;

            for (i = 0; i < nLen1; i++) {
                if (IsDBCSLeadByte(el[v->X + i])) {
                    i++;
                }
            }
            if (i > nLen1) {
                HGLOBAL hmemTmp;

                GlobalUnlock ((HANDLE)lParam);
                if (!(hmemTmp = GlobalReAlloc((HGLOBAL)lParam,
                    nLen1+2, GMEM_MOVEABLE | GMEM_SHARE))) {
                    return FALSE;
                }
                lParam = (LPARAM) hmemTmp;
                if (!(lpsz = (LPTSTR) GlobalLock(hmemTmp))) {
                    return FALSE;
                }
                lpsz[nLen1] = ' ';
                nLen1++;
            }
            v->BlockXR = v->X + nLen1;
        } else {
            int nNum1;
            int nNum2;
            int i;

            for (i = 0, nNum1 = 0; i < nLen1; i++, nNum1++) {
                if (IsDBCSLeadByte(lpsz[i])) {
                    i++;
                }
            }
            for (i = 0, nNum2 = 0; nNum2 < nNum1; i++, nNum2++) {
                if (IsDBCSLeadByte(el[v->X + i])) {
                    i++;
                }
            }
            v->BlockXR = v->X + i;
        }
    }
    bRet = InsertStream(view, v->X, v->Y, nLen1, lpsz, TRUE);
    GlobalUnlock ((HANDLE)lParam);

    if (bRet) {
        SetReplaceDBCSFlag(d, v->BlockStatus ? TRUE : FALSE);
        PosXY(view, v->X + nLen1, v->Y, TRUE);
    }
    return TRUE;
}

/***************************************************************************

    Initialize pointers of IME APIs

***************************************************************************/
BOOL
ImeInit()
{
    if (NULL == hModUser32) {
        hModUser32 = LoadLibrary("IMM32");
    }
    if (NULL == hModUser32) {
#ifdef DEBUG
        MessageBox(NULL, "Failed to load IMM32.DLL",
            NULL, MB_APPLMODAL | MB_OK);
#endif
        return FALSE;
    }
    if (NULL == lpfnWINNLSEnableIME) {
        lpfnWINNLSEnableIME = (LPWINNLSENABLEIME) GetProcAddress(hModUser32, "ImmWINNLSEnableIME");
#ifdef DEBUG
        if (NULL == lpfnWINNLSEnableIME) {
            MessageBox(NULL, "Failed to get address of ImmWINNLSEnableIME",
                NULL, MB_APPLMODAL | MB_OK);
        }
#endif
    }
    if (NULL == lpfnSendIMEMessageEx) {
        lpfnSendIMEMessageEx = (LPSENDIMEMESSAGEEX) GetProcAddress(hModUser32, "ImmSendIMEMessageExA");
#ifdef DEBUG
        if (NULL == lpfnSendIMEMessageEx) {
            MessageBox(NULL, "Failed to get address of ImmSendIMEMessageExA",
                NULL, MB_APPLMODAL | MB_OK);
        }
#endif
    }

    return TRUE;
}
/***************************************************************************

    Terminate proccess of IME

***************************************************************************/
BOOL
ImeTerm()
{
    if (NULL != hModUser32) {
        if (FreeLibrary(hModUser32)) {
            hModUser32 = NULL;
            lpfnWINNLSEnableIME = NULL;
            lpfnSendIMEMessageEx = NULL;
        }
    }
    return TRUE;
}

