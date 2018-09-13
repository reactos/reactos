/*****************************************************************************\
*                                                                             *
* nddeapi.h -   Network DDE share manipulation and control functions          *
*                                                                             *
*               Version 1.0                                                   *
*                                                                             *
*               NOTE: windows.h must be #included first                       *
*                                                                             *
*               Copyright (c) 1992, Microsoft Corp.  All rights reserved.     *
*                                                                             *
\*****************************************************************************/

#ifndef          _INC_NDDEAPI
#define          _INC_NDDEAPI

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif    /* __cplusplus */

#ifndef WINAPI          /* If not included with 3.1 headers... */
#define WINAPI          FAR PASCAL
#define CALLBACK        FAR PASCAL
#define LPCSTR          LPSTR
#define UINT            WORD
#define LPARAM          LONG
#define WPARAM          WORD
#define LRESULT         LONG
#define HMODULE         HANDLE
#define HINSTANCE       HANDLE
#define HLOCAL          HANDLE
#define HGLOBAL         HANDLE
#endif  /* WINAPI */

#ifndef CNLEN           /* If not included with netapi header */
#define CNLEN           15                  /* Computer name length     */
#define UNCLEN          (CNLEN+2)           /* UNC computer name length */
#endif /* CNLEN */

/* API error codes  */
#define NDDE_NO_ERROR                  0
#define NDDE_ACCESS_DENIED             1
#define NDDE_BUF_TOO_SMALL             2
#define NDDE_ERROR_MORE_DATA           3
#define NDDE_INVALID_SERVER            4
#define NDDE_INVALID_SHARE             5
#define NDDE_INVALID_PARAMETER         6
#define NDDE_INVALID_LEVEL             7
#define NDDE_INVALID_PASSWORD          8
#define NDDE_INVALID_ITEMNAME          9
#define NDDE_INVALID_TOPIC             10
#define NDDE_INTERNAL_ERROR            11
#define NDDE_OUT_OF_MEMORY             12
#define NDDE_INVALID_APPNAME           13
#define NDDE_NOT_IMPLEMENTED           14
#define NDDE_SHARE_ALREADY_EXIST       15
#define NDDE_SHARE_NOT_EXIST           16
#define NDDE_INVALID_FILENAME          17
#define NDDE_NOT_RUNNING               18
#define NDDE_INVALID_WINDOW            19
#define NDDE_INVALID_SESSION           20

/* string size constants */
#define MAX_NDDESHARENAME       64
#define MAX_PASSWORD            15
#define MAX_USERNAME            15
#define MAX_DOMAINNAME          15
#define MAX_APPNAME             255
#define MAX_TOPICNAME           255
#define MAX_ITEMNAME            255

/* permission mask bits */
#define NDDEACCESS_REQUEST      0x00000001L
#define NDDEACCESS_ADVISE       0x00000002L
#define NDDEACCESS_POKE         0x00000004L
#define NDDEACCESS_EXECUTE      0x00000008L
#define NDDEACCESS_START_APP    0x00000010L

/* connectFlags bits for ndde service affix */
#define NDDEF_NOPASSWORDPROMPT  0x0001
#define NDDEF_NOCACHELOOKUP     0x0002
#define NDDEF_STRIP_NDDE        0x0004


/* NDDESHAREITEMINFO - contains information about item security */

struct NDdeShareItemInfo_tag {
        LPSTR                   lpszItem;
        DWORD                   dwPermissions;
};
typedef struct NDdeShareItemInfo_tag NDDESHAREITEMINFO;
typedef struct NDdeShareItemInfo_tag * PNDDESHAREITEMINFO;
typedef struct NDdeShareItemInfo_tag far * LPNDDESHAREITEMINFO;

/* NDDESHAREINFO - contains information about a NDDE share */

struct NDdeShareInfo_tag {
        char                    szShareName[ MAX_NDDESHARENAME+1 ];
        LPSTR                   lpszTargetApp;
        LPSTR                   lpszTargetTopic;
        LPBYTE                  lpbPassword1;
        DWORD                   cbPassword1;
        DWORD                   dwPermissions1;                                                      
        LPBYTE                  lpbPassword2;
        DWORD                   cbPassword2;
        DWORD                   dwPermissions2;                                                      
        LPSTR                   lpszItem;
        LONG                    cAddItems;
        LPNDDESHAREITEMINFO     lpNDdeShareItemInfo;
};
typedef struct NDdeShareInfo_tag NDDESHAREINFO;
typedef struct NDdeShareInfo_tag * PNDDESHAREINFO;
typedef struct NDdeShareInfo_tag far * LPNDDESHAREINFO;

/* ddesess_Status defines */
#define NDDESESS_CONNECTING_WAIT_NET_INI                1
#define NDDESESS_CONNECTING_WAIT_OTHR_ND                2
#define NDDESESS_CONNECTED                              3
#define NDDESESS_DISCONNECTING                          4       

/* NDDESESSINFO - contains information about a NDDE session */

struct NDdeSessInfo_tag {
                char        szClientName[UNCLEN+1];
                short       Status;
                DWORD       UniqueID;
};
typedef struct NDdeSessInfo_tag NDDESESSINFO;
typedef struct NDdeSessInfo_tag * PNDDESESSINFO;
typedef struct NDdeSessInfo_tag far * LPNDDESESSINFO;

/* ddeconn_Status defines */
#define NDDECONN_WAIT_LOCAL_INIT_ACK    1
#define NDDECONN_WAIT_NET_INIT_ACK      2
#define NDDECONN_OK                     3
#define NDDECONN_TERMINATING            4
#define NDDECONN_WAIT_USER_PASSWORD     5

/* NDDECONNINFO - contains information about a NDDE conversation */

struct NDdeConnInfo_tag {
        LPSTR   lpszShareName;
        short   Status;
        short   pad;
};
typedef struct NDdeConnInfo_tag NDDECONNINFO;
typedef struct NDdeConnInfo_tag * PNDDECONNINFO;
typedef struct NDdeConnInfo_tag far * LPNDDECONNINFO;

UINT WINAPI NDdeShareAdd(LPSTR, UINT, LPBYTE, DWORD );
UINT WINAPI NDdeShareDel(LPSTR, LPSTR, UINT );
UINT WINAPI NDdeShareEnum(LPSTR, UINT, LPBYTE, DWORD, LPDWORD, LPDWORD );
UINT WINAPI NDdeShareGetInfo(LPSTR, LPSTR, UINT, LPBYTE, DWORD, LPDWORD, LPWORD);
UINT WINAPI NDdeShareSetInfo(LPSTR, LPSTR, UINT, LPBYTE, DWORD, WORD);
UINT WINAPI NDdeGetErrorString(UINT, LPSTR, DWORD);
BOOL WINAPI NDdeIsValidShareName(LPSTR);
BOOL WINAPI NDdeIsValidPassword(LPSTR);
BOOL WINAPI NDdeIsValidTopic(LPSTR);
BOOL WINAPI NDdeIsSharingAllowed(VOID);
UINT WINAPI NDdeSessionEnum(LPSTR, UINT, LPBYTE, DWORD, LPDWORD, LPDWORD);
UINT WINAPI NDdeConnectionEnum(LPSTR, LPSTR, DWORD, UINT, LPBYTE, DWORD, LPDWORD, LPDWORD);
UINT WINAPI NDdeSessionClose(LPSTR, LPSTR, DWORD);                      
HWND WINAPI NDdeGetWindow(VOID);
UINT WINAPI NDdeGetClientInfo(HWND, LPSTR, LONG, LPSTR, LONG);
UINT WINAPI NDdeGetNodeName(LPSTR, LONG);

#ifdef __cplusplus
}
#endif    /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif  /* _INC_NDDEAPI */
