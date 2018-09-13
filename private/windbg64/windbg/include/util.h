/****************************************************************************

    PROTOTYPES DECLARATION FOR UTIL MODULE

****************************************************************************/

#include <commdlg.h>

HWND 
MDIGetActive(
    HWND    hwndParent,
    BOOL   *lpbMaximized
    );

// Function building a pathname derived form the program running
void MakePathNameFromProg(LPSTR extension, LPSTR pathName);
void MakeFileNameFromProg(LPSTR extension, LPSTR fileName);

//Checks to see if a file exists with the path/filename
BOOL FileExist(LPSTR fileName);

//Check if file is Valid
BOOL PASCAL ValidFilename(
    LPSTR lpszName,
    BOOL fWildOkay);

// Opens a standard error Dialog Box
int CDECL ErrorBox(int wErrorFormat, ...);

// Opens a standard error Dialog Box (Parent is hwnd)
int CDECL ErrorBox2(
    HWND hwnd,
    UINT type,
    WORD wErrorFormat,
    ...);

// Opens an Internal error Dialog Box
int CDECL InternalErrorBox(WORD wDescript);

// Opens a standard information Dialog Box
void CDECL InformationBox(int wInfoFormat, ...);

// Opens a message box with the QCWin title
int PASCAL MsgBox(HWND hwndParent, LPSTR szText, UINT wType);

// Opens a message box with the QCWin title
int CDECL VarMsgBox(
    HWND hwndParent,
    WORD wFormat,
    UINT wType,
    ...);

// Opens a standard error Dialog Box and stop execution
void FatalErrorBox(
    WORD line1,
    LPSTR line2);

// Loads and execute dialog box 'rcDlgNb' with 'dlgProc' function
BOOL StartDialog(int rcDlgNb, DLGPROC dlgProc);

// Invalidate the rectangle area of child 'ctlID' in 'hDlg' Dialog Box
void InvDlgCtlIdRect(HWND hDlg, int ctlID);

// Loads a resource string from resource file
void LoadResourceString(
    WORD wStrId,
    LPSTR lpszStrBuffer);

//Opens a standard question box containing combination
//of : Yes, No, Cancel
int CDECL QuestionBox(
    WORD wMsgFormat,
    UINT wType,
    ...);

//Opens a standard question box containing combination
//of : Yes, No, Cancel
int CDECL QuestionBox2(HWND hwnd, WORD wMsgFormat, UINT wType, ...);

//Gets the text associated with the passed environment variable,
//checking against any internally held environment strings first
BOOL PASCAL GetQCQPEnvStr(LPSTR lpszVar, LPSTR lpszDestEnv, WORD wDestSize);

// Scans a text string to a long value, checking limits
// and allowing 0, 0x, and $ representations
BOOL PASCAL fScanAnyLong(
    LPSTR lpszSource,
    WORD wLanguage,
    long *plDest,
    long lMin,
    long lMax);

//Gets sizes of the installed fonts
void GetFontSizes(
    HWND hWnd,
    int currentFont);

//Gets names of the installed fonts
void LoadFonts(
    HWND hWnd);

//Opens Cpu, Watch or Locals Window under MDI
#if defined( NEW_WINDOWING_CODE )

HWND New_OpenDebugWindow(WIN_TYPES winType, BOOL bUserActivated);

#else

int OpenDebugWindow(
    int winType, 
    BOOL bUserActivated
    );

int OpenDebugWindowEx(
    int       winType,
    LPWININFO lpWinInfo,
    int       nViewPreference,
    BOOL      bUserActivated
    );

#endif

int GotoLine(int view, int lineNbr, BOOL fDebugger);
BOOL    QueryLineStatus(int doc, int lineNbr, UINT state);

//Change the status of a given line
void LineStatus(
    int doc,
    int lineNbr,
    WORD state,
    LINESTATUSACTION action,
    BOOL positionInFirstView,
    BOOL redraw);

//Given a file Name, find corresponding doc
typedef BOOL (* FINDDOC)(LPSTR fileName, int *doc, BOOL docOnly);
BOOL FindDoc (LPSTR fileName, int *doc, BOOL docOnly);
BOOL FindDoc1(LPSTR fileName, int *doc, BOOL docOnly);

char *
GetFileName(
    char * szPath);

//Clear all visual info of a doc (Tags or BreakPoints or Compile Errors)
void ClearDocStatus(
    int doc,
    WORD state);

//Clear all visual info of all docs (Tags or BreakPoints or Compile Errors)
void ClearAllDocStatus(
    WORD state);

//Returns height (in lines) of passed list box
WORD VisibleListboxLines(
    HWND hListbox);

//Alloc and lock MOVABLE global memory.
LPSTR Xalloc(UINT bytes);

//Reallocate memory previously allocated through Xalloc
LPSTR Xrealloc(LPSTR curblock, UINT bytes);

//Free global memory.
BOOL Xfree(LPSTR lPtr);

//Hash code for tokens
void Convert(LPSTR tok);

//Adjust Full Path name to fit in a specified len string
void AdjustFullPathName(
    PCSTR fullPath,
    PSTR adjustedPath,
    int len);

// BUGBUG - kcarlos - dead code
#if 0
//Read ini file
BOOL ReadIni(int hFile, LPSTR lpBuffer, int wBytes);

//Write file and add to checksum
BOOL WriteAndSum(int hFile, LPSTR lpBuffer, int wBytes);
#endif

// Load a DLL (qcqp.exe path tried first)
HANDLE PASCAL LoadQCQPLibrary(LPSTR LibName, BOOL DoErrorBox);

// WinExec a prog (qcqp.exe path tried first)
WORD PASCAL WinQCQPExec(LPSTR CmdLine, int nCmdShow, BOOL DoErrorBox);

//Try to lock a global memory handle
BOOL FailGlobalLock(
    HANDLE h,
    LPSTR *p);

//Retrieve in view the word at cursor, or the first line
//of text if selection is active
BOOL GetCurrentText(
    int view,
    BOOL *lookAround,
    LPSTR pText,
    int maxSize,
    LPINT xLeft,
    LPINT xRight);

//Checks if a string contains a DOS wildcard : "*" or "?"
BOOL PASCAL fIsWild(
    PSTR psz);

#define HUNDREDTHS_IN_A_DAY (((24UL*60)*60)*100)

//Returns current time in 100ths of a second
ULONG PASCAL TimeIn100ths(
    void);

//Returns current time + delay time in 100ths of a second
ULONG PASCAL StopTimeIn100ths(
    ULONG DelayIn100ths);

// Process a message received by QCQP
void PASCAL ProcessQCQPMessage(LPMSG lpMsg);

//Initialize files filters for dialog boxes using commonfile DLL
void InitFilterString(WORD id, LPSTR filter, int maxLen);

//Appends a '\' to a string if none found
void AppendBackslashIfNone(
    LPSTR path);

//Get rid of accelerator mark
void RemoveMnemonic(
    LPSTR sWith,
    LPSTR sWithout);

//Check if keyboard hit is NUMLOCK, CAPSLOCK or INSERT
LRESULT EXPORT KeyboardHook( int iCode, WPARAM wParam, LPARAM lParam );

//Returns the handle of one of the debug windows
HWND GetDebugWindowHandle(WORD type);

/****************************************************************************/
// Set this to 1 to include special code to allow automatic test
// suites to run.
#define TESTSUITES  1
#ifdef DEBUGGING
//Makes a printf style command for output on debug console

//#define REMOVEAUXPRINTFS  TRUE
#define REMOVEAUXPRINTFS    FALSE
#if REMOVEAUXPRINTFS
#define AuxPrintf(level, dummy) FALSE
#else
BOOL AuxPrintf(int iDebugLevel, LPSTR text, ...);
#endif //REMOVEAUXPRINTFS

//Swiss Made Chronograph
void ShowElapsedTime(
    void);
void StartTimer(
    void);

void StopTimer(
    void);

//Opens a Dialog box with a title and accepting a printf style for text
int InfoBox(
    LPSTR text,
    ...);

PSTR GetWindebugTxt(long lDbg);

#endif


// Bitwise Operations Macros

// These work on integral operands...
#define SET(i, mask)        ((i) |= (mask))
#define RESET(i, mask)      ((i) &= ~(mask))
#define TOGGLE(i, mask)     ((i) ^= (mask))

// These work on arrays of integral operands...
// bit_idx is the bit number to set in array a so
// SET_ARRAY(37, ucBuffer) does a SET(ucBuffer[4], 0x20), whereas
// SET_ARRAY[37, lBuffer) does a SET(lBuffer[1], 0x20L)
#define SHIFT_OPERAND(bit_idx, a)   ((sizeof(*(a)) > sizeof(int) ? 1L : 1) << (bit_idx%(sizeof(*(a))*8)))
#define SET_ARRAY(bit_idx, a)           SET(a[bit_idx/(sizeof(*(a))*8)], SHIFT_OPERAND(bit_idx, a))
#define RESET_ARRAY(bit_idx, a)     RESET(a[bit_idx/(sizeof(*(a))*8)], SHIFT_OPERAND(bit_idx, a))
#define TOGGLE_ARRAY(bit_idx, a)        TOGGLE(a[bit_idx/(sizeof(*(a))*8)], SHIFT_OPERAND(bit_idx, a))
#define TEST_ARRAY(bit_idx, a)      (a[bit_idx/(sizeof(*(a))*8)] & SHIFT_OPERAND(bit_idx, a))

//To test if a character belong to the C/Pascal Alphanumeric set
#define CHARINALPHASET(c) (IsCharAlphaNumeric(c) || c == '_')

#ifdef JAPAN_HANKAKU_KANA_AS_WORD
  #define CHARINKANASET(c)  ((BYTE)(c) >= 0xa6 && (BYTE)(c) <= (BYTE)0xdf)
#else   // !JAPAN_HANKAKU_KANA_AS_WORD else
  #define CHARINKANASET(c)  (FALSE)
#endif  // !JAPAN_HANKAKU_KANA_AS_WORD end

#define CHARINWORDCHAR(c) (IsDBCSLeadByte(c) || CHARINALPHASET(c)) || CHARINKANASET(c)


void SetVerticalScrollBar(
    int view,
    BOOL propagate);

BOOL FAR PASCAL QueryCloseAllDocs(
    void);

BOOL FAR PASCAL StartFileDlg(HWND hwnd, int titleId, int defExtId,
    int helpId, int templateId, LPSTR fileName, DWORD *pFlags,
    LPOFNHOOKPROC lpfnHook);

//Start the Edit Project dialog box
void PASCAL StartEditProjDlg(
    HWND hParent);

void AddWindowMenuItem(
    int doc,
    int view);

void DeleteWindowMenuItem(
    int view);

void PASCAL OpenProject(
    PSTR ProjectName,
    HWND ParentWnd,
    BOOL TryForWorkspace);

void PASCAL EditProject(
    HWND ParentWnd);

void PASCAL CloseProject(
    void);

int FindWindowMenuId(
    WORD type,
    int viewLimit,
    BOOL sendDocMenuId);

BOOL DestroyView(
    int view);

int SetLanguage(int doc);

BOOL GetWordAtXY(
    int view,
    int x,
    long y,
    BOOL selection,
    BOOL *lookAround,
    BOOL includeRightSpace,
    LPSTR pWord,
    int maxSize,
    LPINT leftCol,
    LPINT rightCol);

void PASCAL EnsureScrollBars(
    int view,
    BOOL force);

void PASCAL NewFontInView(
    int view);

void PASCAL EscapeAmpersands(
    LPSTR AmpStr,
    int MaxLen);

void PASCAL UnescapeAmpersands(
    LPSTR AmpStr,
    int MaxLen);

void PASCAL BuildTitleBar(LPSTR TitleBarStr, UINT MaxLen);

void PASCAL UpdateTitleBar(
    TITLEBARMODE Mode,
    BOOL Repaint);

void PASCAL DlgEnsureTitleBar(void);

// BUGBUG - dead code - kcarlos
#if 0
void FAR PASCAL FileNotSaved(int doc);
#endif

int FAR PASCAL ConvertPosX(int x);

void FAR PASCAL FlushKeyboard(void);

BOOL FindLineStatus(int view, BYTE target, BOOL forward, long *line);

//ANSI/ASCII compatible isspace() test
#define whitespace(c) (c == ' ' || c == '\t' || c == '\n' || c == '\r')

BOOL    SetDriveAndDir(PSTR st);
VOID    GetBaseName( LPSTR , LPSTR  );
LPSTR   AllocateMultiString( DWORD );
BOOL DeallocateMultiString( LPSTR );
BOOL AddToMultiString( LPSTR *, DWORD *, LPSTR );
LPSTR   GetNextStringFromMultiString( LPSTR, DWORD, DWORD * );
BOOL    FindNameOn(PTSTR, UINT, PCTSTR, PCTSTR);
void    Egg(void);
VOID    InvalidateAllWindows(VOID);

BOOL
GetFileTimeByName(
    LPSTR FileName,
    LPFILETIME lpCreationTime,
    LPFILETIME lpLastAccessTime,
    LPFILETIME lpLastWriteTime
    );

VOID GetDBCSCharWidth(HDC hDC, LPTEXTMETRIC ptm, LPVIEWREC lpv);
VOID SetReplaceDBCSFlag(LPDOCREC lpd, BOOL bTwoRec);

LRESULT ImeMoveConvertWin(HWND hwnd, INT x, INT y);
LRESULT ImeSendVkey(HWND hwnd, WORD wVKey);
BOOL ImeSetFont(HWND hwnd, HFONT hFont);
BOOL ImeWINNLSEnableIME(HWND hwnd, BOOL bEnable);
BOOL ProccessIMEString(HWND hwnd, LPARAM lParam);
BOOL ImeInit(void);
BOOL ImeTerm(void);
