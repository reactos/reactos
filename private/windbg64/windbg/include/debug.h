/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
/* This include file contains the functions needed by debuggers which run
 * under windows. 
 */

/* USER functions */
BOOL FAR PASCAL QuerySendMessage(HANDLE h1, HANDLE h2, HANDLE h3, LPMSG lpmsg);
BOOL FAR PASCAL LockInput(HANDLE h1, HWND hwndInput, BOOL fLock);

LONG FAR PASCAL GetSystemDebugState(void);
/* Flags returned by GetSystemDebugState. 
 */
#define SDS_MENU        0x0001
#define SDS_SYSMODAL    0x0002
#define SDS_NOTASKQUEUE 0x0004

/* Kernel procedures */
void FAR PASCAL DirectedYield(HANDLE hTask);
