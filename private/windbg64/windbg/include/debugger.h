/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
#define DBG_SETUPNOTIFICATION WM_USER+20
#define DBG_ENTERHARDMODE       WM_USER+21
#define DBG_ENTERSOFTMODE       WM_USER+22

#define DBG_REFRESH         WM_USER+23
#define DBG_INFO            WM_USER+24


typedef enum
{
    CHK_CONTINUE,
    CHK_ENTERHARDMODE,
    CHK_ENTERSOFTMODE,
    CHK_WAITFORRESULT
}
CHECKRESULT;

typedef enum
{
    CBID_SYSMODALMSGBOX,
    CBID_SKIPBP,
    CBID_REPLACEBPANDGO,
    CBID_HANDLENOTIFICATION
} CALLBACKID;

typedef DWORD (FAR PASCAL *DBGCALLBACK)(
    CALLBACKID callbackId,
    DWORD data);

//Prototypes
FARPROC FAR PASCAL InitDebuggerDLL(
    HWND hwndDebugger,
    int __FAR *pChildPid,
    WORD LSMMsg,
    DBGCALLBACK Callback);

void FAR PASCAL TermDebuggerDLL(
    void);

void FAR PASCAL SetCheckResult(
    CHECKRESULT result);

void FAR PASCAL SetExitHardMode(
    void);

BOOL FAR PASCAL DebuggeeInCallback(
    void);

void FAR PASCAL SetFocusDebuggee(
    void);


// WARNING: Windows 3.0 Internals.
// I found these functions in the Windows 3.0 KERNEL.EXE
// and found out empirically what they did and saw no
// side effects.  Thus there is no guarantee of their
// portability.  All testing was performed with Windows 3.0

// This function takes either a module handle or an instance
// thereof and returns the module handle or 0 otherwise
HANDLE FAR PASCAL WKGetExePtr(HANDLE hInstance);

// This function pulled in from the Windows KERNEL
HANDLE FAR PASCAL WKGetTaskQueue(HANDLE CurrentTask);

// (Un)Lock current task
void FAR PASCAL WULockMyTask(BOOL lock);

// TRUE if current task is locked
BOOL FAR PASCAL WKIsTaskLocked(void);

// Window field offset for users dialog proc in a dialog box
// (This has got to be highly version dependent!)
#define GWL_DIALOGFUNC 4

// END:         Windows 3.0 Internals.
