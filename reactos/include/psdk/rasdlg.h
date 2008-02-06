#ifndef _RASDLG_H_
#define _RASDLG_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <ras.h>

#define RASPBDEVENT_AddEntry    1
#define RASPBDEVENT_EditEntry   2
#define RASPBDEVENT_RemoveEntry 3
#define RASPBDEVENT_DialEntry   4
#define RASPBDEVENT_EditGlobals 5
#define RASPBDEVENT_NoUser      6
#define RASPBDEVENT_NoUserEdit  7

#define RASPBDFLAG_PositionDlg      0x00000001
#define RASPBDFLAG_ForceCloseOnDial 0x00000002
#define RASPBDFLAG_NoUser           0x00000010
#define RASPBDFLAG_UpdateDefaults   0x80000000

#define RASEDFLAG_PositionDlg  1
#define RASEDFLAG_NewEntry     2
#define RASEDFLAG_CloneEntry   4

#define RASDDFLAG_PositionDlg  1

#ifndef RC_INVOKED
#include <pshpack4.h>

typedef struct tagRASENTRYDLGA
{
    DWORD dwSize;
    HWND  hwndOwner;
    DWORD dwFlags;
    LONG  xDlg;
    LONG  yDlg;
    CHAR  szEntry[RAS_MaxEntryName + 1];
    DWORD dwError;
    ULONG_PTR reserved;
    ULONG_PTR reserved2;
} RASENTRYDLGA, *LPRASENTRYDLGA;
typedef struct tagRASENTRYDLGW
{
    DWORD dwSize;
    HWND  hwndOwner;
    DWORD dwFlags;
    LONG  xDlg;
    LONG  yDlg;
    WCHAR szEntry[RAS_MaxEntryName + 1];
    DWORD dwError;
    ULONG_PTR reserved;
    ULONG_PTR reserved2;
} RASENTRYDLGW, *LPRASENTRYDLGW;

typedef struct tagRASDIALDLG
{
    DWORD dwSize;
    HWND  hwndOwner;
    DWORD dwFlags;
    LONG  xDlg;
    LONG  yDlg;
    DWORD dwSubEntry;
    DWORD dwError;
    ULONG_PTR reserved;
    ULONG_PTR reserved2;
} RASDIALDLG, *LPRASDIALDLG;

/* Application-defined callback functions */
typedef VOID (WINAPI* RASPBDLGFUNCW)(DWORD, DWORD, LPWSTR, LPVOID);
typedef VOID (WINAPI* RASPBDLGFUNCA)(DWORD, DWORD, LPSTR, LPVOID);

typedef struct tagRASPBDLGA
{
    DWORD         dwSize;
    HWND          hwndOwner;
    DWORD         dwFlags;
    LONG          xDlg;
    LONG          yDlg;
    ULONG_PTR     dwCallbackId;
    RASPBDLGFUNCA pCallback;
    DWORD         dwError;
    ULONG_PTR     reserved;
    ULONG_PTR     reserved2;
} RASPBDLGA, *LPRASPBDLGA;
typedef struct tagRASPBDLGW
{
    DWORD         dwSize;
    HWND          hwndOwner;
    DWORD         dwFlags;
    LONG          xDlg;
    LONG          yDlg;
    ULONG_PTR     dwCallbackId;
    RASPBDLGFUNCW pCallback;
    DWORD         dwError;
    ULONG_PTR     reserved;
    ULONG_PTR     reserved2;
} RASPBDLGW, *LPRASPBDLGW;

typedef struct tagRASMONITORDLG
{
    DWORD         dwSize;
    HWND          hwndOwner;
    DWORD         dwFlags;
    DWORD         dwStartPage;
    LONG          xDlg;
    LONG          yDlg;
    OUT DWORD     dwError;
    ULONG_PTR     reserved;
    ULONG_PTR     reserved2;
} RASMONITORDLG, *LPRASMONITORDLG;

typedef struct tagRASNOUSERA
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwTimeoutMs;
    CHAR  szUserName[UNLEN + 1];
    CHAR  szPassword[PWLEN + 1];
    CHAR  szDomain[DNLEN + 1];
} RASNOUSERA, *LPRASNOUSERA;
typedef struct tagRASNOUSERW
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwTimeoutMs;
    WCHAR szUserName[UNLEN + 1];
    WCHAR szPassword[PWLEN + 1];
    WCHAR szDomain[DNLEN + 1];
} RASNOUSERW, *LPRASNOUSERW ;

#include <poppack.h>

BOOL APIENTRY RasDialDlgA(LPSTR,LPSTR,LPSTR,LPRASDIALDLG);
BOOL APIENTRY RasDialDlgW(LPWSTR,LPWSTR,LPWSTR,LPRASDIALDLG);
BOOL APIENTRY RasEntryDlgA(LPSTR,LPSTR,LPRASENTRYDLGA);
BOOL APIENTRY RasEntryDlgW(LPWSTR,LPWSTR,LPRASENTRYDLGW);
BOOL APIENTRY RasPhonebookDlgA(LPSTR,LPSTR,LPRASPBDLGA);
BOOL APIENTRY RasPhonebookDlgW(LPWSTR,LPWSTR,LPRASPBDLGW);
#if (WINVER <= 0x500)
BOOL APIENTRY RasMonitorDlgA(LPSTR, LPRASMONITORDLG);
BOOL APIENTRY RasMonitorDlgW(LPWSTR, LPRASMONITORDLG);
#endif /*(WINVER <= 0x500)*/

#ifdef UNICODE
typedef RASENTRYDLGW   RASENTRYDLG, *LPRASENTRYDLG;
typedef RASPBDLGW      RASPBDLG, *LPRASPBDLG;
typedef RASNOUSERW     RASNOUSER, *LPRASNOUSER;
#define RasDialDlg     RasDialDlgW
#define RasEntryDlg    RasEntryDlgW
#if (WINVER <= 0x500)
#define RasMonitorDlg RasMonitorDlgW
#endif /*(WINVER <= 0x500)*/
#define RasPhonebookDlg RasPhonebookDlgW
#else
typedef RASENTRYDLGA   RASENTRYDLG, *LPRASENTRYDLG;
typedef RASPBDLGA      RASPBDLG, *LPRASPBDLG;
typedef RASNOUSERA     RASNOUSER, *LPRASNOUSER;
#define RasDialDlg     RasDialDlgA
#define RasEntryDlg    RasEntryDlgA
#if (WINVER <= 0x500)
#define RasMonitorDlg RasMonitorDlgA
#endif /*(WINVER <= 0x500)*/
#define RasPhonebookDlg RasPhonebookDlgA
#endif /* UNICODE */

#endif /* RC_INVOKED */

#ifdef __cplusplus
}
#endif
#endif
