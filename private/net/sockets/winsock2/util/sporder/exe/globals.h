#include <commctrl.h>


//
// Global variables
//

extern HINSTANCE ghInst;
extern int listTabs[];
extern int gNumRows;


//
// Function prototypes
//

LRESULT CALLBACK EnumDlgProc     (HWND, UINT, WPARAM, LPPROPSHEETPAGE);
LRESULT CALLBACK SortDlgProc     (HWND, UINT, WPARAM, LPPROPSHEETPAGE);
INT_PTR CALLBACK MoreInfoDlgProc (HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK RNRDlgProc      (HWND, UINT, WPARAM, LPARAM);

int CatReadRegistry    (HWND);
int CatDoMoreInfo      (HWND, int);
int CatDoWriteEntries  (HWND);
int CatCheckRegErrCode (HWND, LONG, LPTSTR);
int CatDoUpDown        (HWND, WPARAM);
BOOL CatGetIndex (LPTSTR, LPINT, LPINT);


//
// Constant defines
//

#define DID_HEADERCTL  501
#define DID_LISTCTL    502
#define DID_UP         503
#define DID_DOWN       504
#define DID_MORE       505

#define DID_ENUM       700
#define DID_WSAENUM    701

#define TOTAL_TABS 10

#define MAX_STR 256


//
// Helpful macros
//

//
// In the context of a dialog box with a child listbox id==DID_LISTCTL...
//
#define ADDSTRING(x)   SendMessage (GetDlgItem (hwnd, DID_LISTCTL), LB_ADDSTRING, 0 ,(LPARAM) x)
#define HWNDLISTCTL    GetDlgItem (hwnd, DID_LISTCTL)

#define ASSERT(X,Y) if (!X) MessageBox (NULL, Y, TEXT("ASSERT"), MB_OK);

#define XBORDER   GetSystemMetrics (SM_CXSIZEFRAME) *3/2
#define YBORDER   GetSystemMetrics (SM_CYSIZEFRAME) *3/2
#define HDRHEIGHT GetSystemMetrics (SM_CYMENU)


//
// Simple debug output routine that becomes no-op when ndef DEBUG
//

#ifdef DEBUG
#else
#endif

#if DBG

#define DBGOUT(arg) DbgPrint arg

void
_cdecl
DbgPrint(
    PTCH Format,
    ...
    );

#else

#define DBGOUT(arg)

#endif
