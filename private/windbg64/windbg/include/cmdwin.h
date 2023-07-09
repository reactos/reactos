/*++ BUILD Version: 0002    // Increment this if a change has global effects
**                                  **
**              CMDWIN.H              **
**                                  **
**  Description:                            **
**  This file contains the prototypes and common declarations   **
**  for the command window                      **
**                                  **
*************************************************************************/

#define MAX_CMDWIN_LINES    30000
#define MAX_CMDWIN_HISTORY  1000

typedef BOOL (WINAPI * CTRLC_HANDLER_PROC)(DWORD);

typedef struct _CTRLC_HANDLER {
    struct _CTRLC_HANDLER * next;
    CTRLC_HANDLER_PROC      pfnFunc;
    DWORD                   dwParam;
} CTRLC_HANDLER, *PCTRLC_HANDLER;

extern BOOL SetAutoRunSuppress(BOOL);
extern BOOL GetAutoRunSuppress(void);
void CmdSetAutoHistOK(BOOL);
BOOL CmdGetAutoHistOK(void);

LRESULT FAR PASCAL EXPORT CmdEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL CmdExecNext(DBC dbcCallback, LPARAM lParam);
BOOL FAR PASCAL DoStopEvent(LPTD);
void FAR PASCAL SetCtrlCTrap( void );
void FAR PASCAL ClearCtrlCTrap( void );
ULONG WDBGAPI CheckCtrlCTrap( void );
BOOL FAR PASCAL CmdSetEatEOLWhitespace( BOOL ff );
VOID FAR PASCAL DispatchCtrlCEvent(VOID);

BOOL CmdHelp ( LPSTR Topic );
