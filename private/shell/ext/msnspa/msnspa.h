/*
 *  msnspa.h
 */

#define STRICT
#include <windows.h>
#include <winsock.h>

#define SECURITY_WIN32
#include <issperr.h>
#include <sspi.h>

#define IDD_MAIN        1
#define IDC_NEWS        16
#define IDC_MAIL        17
#define IDI_MAIN        1

#ifndef RC_INVOKED

#include <windowsx.h>

typedef LPVOID PV;

#define BEGIN_CONST_DATA data_seg(".text", "CODE")
#define END_CONST_DATA data_seg(".data", "DATA")
#define INLINE static __inline

#define INTERNAL WINAPI
#define EXTERNAL WINAPI

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif

/*
 *  Plenty big enough for authorization.
 */
#define PLENTY_BIG          1024

/*
 *  Private structure used to keep track of Win32 authorization state.
 */
typedef struct WIN32AUTHSTATE {
    CredHandle      hCred;
    CtxtHandle      hCtxt;
    ULONG           fContextReq;
    BOOL            fHCredValid;
    BOOL            fHCtxtValid;
    char            szBuffer[PLENTY_BIG];
} WIN32AUTHSTATE, *PWIN32AUTHSTATE;

BOOL INTERNAL
Security_AcquireCredentials(PWIN32AUTHSTATE pwas, LPTSTR ptszPackage);

BOOL INTERNAL Security_GetNegotiation(PWIN32AUTHSTATE pwas);

BOOL INTERNAL
Security_GetResponse(PWIN32AUTHSTATE pwas, LPSTR szChallenge);

void INTERNAL
Security_ReleaseCredentials(PWIN32AUTHSTATE pwas);

/*****************************************************************************/

#define BUFSIZE 1024

/*
 *  Private structure used to keep track of conversation with server.
 */
typedef struct CONNECTIONSTATE {        /* cxs */
    SOCKET          ssfd;               /* Socket to server */
    SOCKET          scfd;               /* Socket to client */
    struct PROXYINFO *pproxy;           /* Info about who we are */
    char            buf[BUFSIZE];       /* Working buffer */
    int             nread;              /* Size of buffer */
} CONNECTIONSTATE, *PCONNECTIONSTATE;

#define EOL TEXT("\r\n")

#ifdef DBG
void __cdecl Squirt(LPCTSTR ptszMsg, ...);
#else
#define Squirt sizeof
#endif

void __cdecl Die(LPCTSTR ptszMsg, ...);

int sendsz(SOCKET s, LPCSTR psz);

//DWORD INTERNAL POP3_Main(LPVOID pvRef);
//void POP3_Negotiate(PCONNECTIONSTATE pcxs);
//DWORD INTERNAL NNTP_Main(LPVOID pvRef);
//void NNTP_Negotiate(PCONNECTIONSTATE pcxs);

HWND INTERNAL UI_Init(void);
void INTERNAL UI_Term(void);
void INTERNAL UI_UpdateCounts(void);

extern HINSTANCE g_hinst;
extern int g_cMailUsers;
extern int g_cNewsUsers;

/*****************************************************************************
 *
 *      proxy.c
 *
 *****************************************************************************/

SOCKET INTERNAL
init_send_socket(SOCKET scfd, LPCSTR pszHost, u_short port, LPCSTR pszErrMsg);

SOCKET INTERNAL
create_listen_socket(u_short port);

/*****************************************************************************
 *
 *      PROXYINFO
 *
 *****************************************************************************/

typedef BOOL (CALLBACK *NEGOTIATE)(SOCKET s);

typedef struct PROXYINFO {
    u_short localport;              /* The port we listen on */
    u_short serverport;             /* The port we talk to */
    LPCSTR pszHost;                 /* The host we proxy to */
    PINT piUsers;                   /* Variable that tracks number of users */
    NEGOTIATE Negotiate;            /* Negotiates security info */
    LPCSTR pszError;                /* Error on failed connect */
    LPCSTR pszErrorPwd;             /* Error on bad password */
    LPCSTR pszResponse;             /* What to respond to ignored cmds */
    char szIgnore1[5];              /* First 4-char command to ignore */
    char szIgnore2[5];              /* Second 4-char command to ignore */

} PROXYINFO, *PPROXYINFO;

extern PROXYINFO g_proxyPop;
extern PROXYINFO g_proxyNntp1;
extern PROXYINFO g_proxyNntp2;

DWORD CALLBACK ProxyThread(LPVOID pvRef);

#endif
