// Keys.H

//
// Functions exported from narrhook.dll
//
__declspec(dllexport) BOOL InitKeys(HWND hwnd);
__declspec(dllexport) BOOL UninitKeys(void);
__declspec(dllexport) BOOL InitMSAA(void);
__declspec(dllexport) BOOL UnInitMSAA(void);
__declspec(dllexport) void BackToApplication(void);

// this is in other.cpp it is used to avoid pulling in C runtime
__declspec(dllexport) LPTSTR lstrcatn(LPTSTR pDest, LPTSTR pSrc, int maxDest);

//
// typedefs 
//
typedef void (*FPACTION)(int nOption);

typedef struct tagHOTK
{
    WPARAM  keyVal;    // Key value, like F1
	int status;
    FPACTION lFunction; // address of function to get info
    int nOption;     // Extra data to send to function
} HOTK;


//
// defines
//
#define MAX_TEXT 20000  

#define TIMER_ID 1001

#define MSR_CTRL  1
#define MSR_SHIFT 2
#define MSR_ALT   4

#define MSR_KEYUP		1
#define MSR_KEYDOWN		2
#define MSR_KEYLEFT		3
#define MSR_KEYRIGHT	4

//
// Function Prototypes
//
void ProcessWinEvent(DWORD event, HWND hwndMsg, LONG idObject, 
                     LONG idChild, DWORD idThread, DWORD dwmsEventTime);

// Macros and function prototypes for debugging
#ifdef DEBUG
  #define _DEBUG
#endif
#ifdef _DEBUG
  void FAR CDECL PrintIt(LPTSTR strFmt, ...);
  #define DBPRINTF PrintIt
#else
  #define DBPRINTF        1 ? (void)0 : (void)
#endif

extern DWORD g_tidMain;	// ROBSI: 10-10-99 (defined in keys.cpp)

