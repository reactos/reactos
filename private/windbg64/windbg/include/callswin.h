
typedef struct _tagSTACKINFO {
    STACKFRAME64 StkFrame;
    DWORD        FrameNum;
    CHAR         ProcName[MAX_PATH];
    CHAR         Params[MAX_PATH*3];
    CHAR         Context[MAX_PATH];
    CHAR         Module[MAX_PATH];
    DWORDLONG    Displacement;
    ADDR         ProcAddr;
    CXF          Cxf;
    BOOL         fInProlog;
} STACKINFO, *LPSTACKINFO;

#define MAX_FRAMES  1000



#if defined( NEW_WINDOWING_CODE )

LRESULT
CALLBACK
NewCalls_WindowProc(
    HWND   hwnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    );

#else

LRESULT
CALLBACK
CallsWndProc(
    HWND   hwnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    );

#endif


void
OpenCallsWindow(
    int         type,
    LPWININFO   lpWinInfo,
    int         Preference,
    BOOL        bUserActivated
    );

HWND
GetCallsHWND(
    VOID
    );


BOOL
IsCallsInFocus(
    VOID
    );

LPSTR
GetLastFrameFuncName(
    VOID
    );

BOOL
GetCompleteStackTrace(
    DWORD64       FramePointer,
    DWORD64       StackPointer,
    DWORD64       ProgramCounter,
    LPSTACKINFO   StackInfo,
    LPDWORD       lpdwFrames,
    BOOL          fQuick,
    BOOL          fFull
    );

BOOL
GotoFrame(
    int         iCall,
    BOOL        bUserActivated
    );

PCXF
ChangeFrame(
    int iCall
    );

BOOL
IsValidFrameNumber(
    INT FrameNumber
    );
