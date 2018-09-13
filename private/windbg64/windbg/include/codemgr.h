/*++ BUILD Version: 0002    // Increment this if a change has global effects
---*/

BOOL    PASCAL FDebInit(void);
BOOL    PASCAL FDebTerm(void);

#define SRCSTEPPING 0
#define ASMSTEPPING 1


void AsyncStop(void);

void FAR PASCAL DebuggerMessage(LPSTR Msg);

// Single Step
int pascal Step(int Overcalls, int StepMode);

BOOL DebuggeeAlive(void);
BOOL DebuggeeActive(void);

void PASCAL ZapInt3s(void);


BOOL PASCAL DbgFEmLoaded(void);

int  PASCAL GetQueueLength(void);
DWORD64 PASCAL GetQueueItemQuad(void);

BOOL PASCAL GoUntil(PADDR);
//BOOL PASCAL MoveEditorToAddr(PADDR pAddr);
VOID FAR PASCAL SetPTState(PSTATEX pstate, TSTATEX tstate);
void FAR PASCAL EnsureFocusDebugger( void );

BOOL GetRootNameMappings(LPSTR *, DWORD *);
BOOL SetRootNameMappings(LPSTR, DWORD);
BOOL RootSetMapped(LPSTR, LPSTR);
/*
**  Source name mapping routines
*/

int SrcMapSourceFilename(LPSTR, UINT, int, FINDDOC, BOOL bUserActivated);
int SrcBackMapSourceFilename(LPSTR, UINT);
#define SRC_MAP_NONE    0
#define SRC_MAP_OPEN    1
#define SRC_MAP_ONLY    2

typedef enum _DBGSTATE {
    ds_normal = 0,
    ds_init,
    ds_error
} DBGSTATE;
extern DBGSTATE DbgState;

//
// Manipulation techniques
//
HWND GetTopSourceWindow(BOOL AsmOk);
