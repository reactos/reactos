/*****************************************************************************\
*                                                                             *
* ole.h -       Object Linking and Embedding functions, types, and definitions*
*                                                                             *
*               Version 1.0                                                   *
*                                                                             *
*               NOTE: windows.h must be #included first                       *
*                                                                             *
*               Copyright (c) 1990-1996, Microsoft Corp.  All rights reserved.*
*                                                                             *
\*****************************************************************************/

#ifndef _INC_OLE
#define _INC_OLE

#ifdef WIN16
#include <pshpack1.h>   /* Assume byte packing throughout */
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef WINAPI          /* If not included with 3.1 headers... */
#define WINAPI      FAR PASCAL
#define CALLBACK    FAR PASCAL
#define LPCSTR      LPSTR
#define LRESULT     LONG
#define HGLOBAL     HANDLE
#endif  /* _INC_WINDOWS */


#ifdef STRICT
#define OLE_LPCSTR  LPCSTR
#define OLE_CONST   const
#else   /* STRICT */
#define OLE_LPCSTR  LPSTR
#define OLE_CONST
#endif /* !STRICT */

#define LRESULT     LONG
#define HGLOBAL     HANDLE


/* Object types */
#define OT_LINK             1L
#define OT_EMBEDDED         2L
#define OT_STATIC           3L

/* activate verbs */
#define OLEVERB_PRIMARY     0

/* target device info structure */
typedef struct _OLETARGETDEVICE
{
    USHORT otdDeviceNameOffset;
    USHORT otdDriverNameOffset;
    USHORT otdPortNameOffset;
    USHORT otdExtDevmodeOffset;
    USHORT otdExtDevmodeSize;
    USHORT otdEnvironmentOffset;
    USHORT otdEnvironmentSize;
    BYTE otdData[1];
} OLETARGETDEVICE;
typedef OLETARGETDEVICE FAR* LPOLETARGETDEVICE;

/* flags used in some methods */
#define OF_SET              0x0001
#define OF_GET              0x0002
#define OF_HANDLER          0x0004

/* return codes for OLE functions */
typedef enum
{
    OLE_OK,                     /* 0   Function operated correctly             */

    OLE_WAIT_FOR_RELEASE,       /* 1   Command has been initiated, client      */
                                /*     must wait for release. keep dispatching */
                                /*     messages till OLE_RELESE in callback    */

    OLE_BUSY,                   /* 2   Tried to execute a method while another */
                                /*     method is in progress.                  */

    OLE_ERROR_PROTECT_ONLY,     /* 3   Ole APIs are called in real mode        */
    OLE_ERROR_MEMORY,           /* 4   Could not alloc or lock memory          */
    OLE_ERROR_STREAM,           /* 5  (OLESTREAM) stream error                 */
    OLE_ERROR_STATIC,           /* 6   Non static object expected              */
    OLE_ERROR_BLANK,            /* 7   Critical data missing                   */
    OLE_ERROR_DRAW,             /* 8   Error while drawing                     */
    OLE_ERROR_METAFILE,         /* 9   Invalid metafile                        */
    OLE_ERROR_ABORT,            /* 10  Client chose to abort metafile drawing  */
    OLE_ERROR_CLIPBOARD,        /* 11  Failed to get/set clipboard data        */
    OLE_ERROR_FORMAT,           /* 12  Requested format is not available       */
    OLE_ERROR_OBJECT,           /* 13  Not a valid object                      */
    OLE_ERROR_OPTION,           /* 14  Invalid option(link update / render)    */
    OLE_ERROR_PROTOCOL,         /* 15  Invalid protocol                        */
    OLE_ERROR_ADDRESS,          /* 16  One of the pointers is invalid          */
    OLE_ERROR_NOT_EQUAL,        /* 17  Objects are not equal                   */
    OLE_ERROR_HANDLE,           /* 18  Invalid handle encountered              */
    OLE_ERROR_GENERIC,          /* 19  Some general error                      */
    OLE_ERROR_CLASS,            /* 20  Invalid class                           */
    OLE_ERROR_SYNTAX,           /* 21  Command syntax is invalid               */
    OLE_ERROR_DATATYPE,         /* 22  Data format is not supported            */
    OLE_ERROR_PALETTE,          /* 23  Invalid color palette                   */
    OLE_ERROR_NOT_LINK,         /* 24  Not a linked object                     */
    OLE_ERROR_NOT_EMPTY,        /* 25  Client doc contains objects.            */
    OLE_ERROR_SIZE,             /* 26  Incorrect buffer size passed to the api */
                                /*     that places some string in caller's     */
                                /*     buffer                                  */

    OLE_ERROR_DRIVE,            /* 27  Drive letter in doc name is invalid     */
    OLE_ERROR_NETWORK,          /* 28  Failed to establish connection to a     */
                                /*     network share on which the document     */
                                /*     is located                              */

    OLE_ERROR_NAME,             /* 29  Invalid name(doc name, object name),    */
                                /*     etc.. passed to the APIs                */

    OLE_ERROR_TEMPLATE,         /* 30  Server failed to load template          */
    OLE_ERROR_NEW,              /* 31  Server failed to create new doc         */
    OLE_ERROR_EDIT,             /* 32  Server failed to create embedded        */
                                /*     instance                                */
    OLE_ERROR_OPEN,             /* 33  Server failed to open document,         */
                                /*     possible invalid link                   */

    OLE_ERROR_NOT_OPEN,         /* 34  Object is not open for editing          */
    OLE_ERROR_LAUNCH,           /* 35  Failed to launch server                 */
    OLE_ERROR_COMM,             /* 36  Failed to communicate with server       */
    OLE_ERROR_TERMINATE,        /* 37  Error in termination                    */
    OLE_ERROR_COMMAND,          /* 38  Error in execute                        */
    OLE_ERROR_SHOW,             /* 39  Error in show                           */
    OLE_ERROR_DOVERB,           /* 40  Error in sending do verb, or invalid    */
                                /*     verb                                    */
    OLE_ERROR_ADVISE_NATIVE,    /* 41  Item could be missing                   */
    OLE_ERROR_ADVISE_PICT,      /* 42  Item could be missing or server doesn't */
                                /*     this format.                            */

    OLE_ERROR_ADVISE_RENAME,    /* 43  Server doesn't support rename           */
    OLE_ERROR_POKE_NATIVE,      /* 44  Failure of poking native data to server */
    OLE_ERROR_REQUEST_NATIVE,   /* 45  Server failed to render native data     */
    OLE_ERROR_REQUEST_PICT,     /* 46  Server failed to render presentation    */
                                /*     data                                    */
    OLE_ERROR_SERVER_BLOCKED,   /* 47  Trying to block a blocked server or     */
                                /*     trying to revoke a blocked server       */
                                /*     or document                             */

    OLE_ERROR_REGISTRATION,     /* 48  Server is not registered in regestation */
                                /*     data base                               */
    OLE_ERROR_ALREADY_REGISTERED,/*49  Trying to register same doc multiple    */
                                 /*    times                                   */
    OLE_ERROR_TASK,             /* 50  Server or client task is invalid        */
    OLE_ERROR_OUTOFDATE,        /* 51  Object is out of date                   */
    OLE_ERROR_CANT_UPDATE_CLIENT,/* 52 Embed doc's client doesn't accept       */
                                /*     updates                                 */
    OLE_ERROR_UPDATE,           /* 53  erorr while trying to update            */
    OLE_ERROR_SETDATA_FORMAT,   /* 54  Server app doesn't understand the       */
                                /*     format given to its SetData method      */
    OLE_ERROR_STATIC_FROM_OTHER_OS,/* 55 trying to load a static object created */
                                   /*    on another Operating System           */
    OLE_ERROR_FILE_VER,

    /*  Following are warnings */
    OLE_WARN_DELETE_DATA = 1000 /*     Caller must delete the data when he is  */
                                /*     done with it.                           */
} OLESTATUS;



/* Codes for CallBack events */
typedef enum
{
    OLE_CHANGED,            /* 0                                             */
    OLE_SAVED,              /* 1                                             */
    OLE_CLOSED,             /* 2                                             */
    OLE_RENAMED,            /* 3                                             */
    OLE_QUERY_PAINT,        /* 4  Interruptible paint support                */
    OLE_RELEASE,            /* 5  Object is released(asynchronous operation  */
                            /*    is completed)                              */
    OLE_QUERY_RETRY        /* 6  Query for retry when server sends busy ACK */
} OLE_NOTIFICATION;

typedef enum
{
    OLE_NONE,               /* 0  no method active                           */
    OLE_DELETE,             /* 1  object delete                              */
    OLE_LNKPASTE,           /* 2  PasteLink(auto reconnect)                  */
    OLE_EMBPASTE,           /* 3  paste(and update)                          */
    OLE_SHOW,               /* 4  Show                                       */
    OLE_RUN,                /* 5  Run                                        */
    OLE_ACTIVATE,           /* 6  Activate                                   */
    OLE_UPDATE,             /* 7  Update                                     */
    OLE_CLOSE,              /* 8  Close                                      */
    OLE_RECONNECT,          /* 9  Reconnect                                  */
    OLE_SETUPDATEOPTIONS,   /* 10 setting update options                     */
    OLE_SERVERUNLAUNCH,     /* 11 server is being unlaunched                 */
    OLE_LOADFROMSTREAM,     /* 12 LoadFromStream(auto reconnect)             */
    OLE_SETDATA,            /* 13 OleSetData                                 */
    OLE_REQUESTDATA,        /* 14 OleRequestData                             */
    OLE_OTHER,              /* 15 other misc async operations                */
    OLE_CREATE,             /* 16 create                                     */
    OLE_CREATEFROMTEMPLATE, /* 17 CreatefromTemplate                         */
    OLE_CREATELINKFROMFILE, /* 18 CreateLinkFromFile                         */
    OLE_COPYFROMLNK,        /* 19 CopyFromLink(auto reconnect)               */
    OLE_CREATEFROMFILE,     /* 20 CreateFromFile                             */
    OLE_CREATEINVISIBLE     /* 21 CreateInvisible                            */
} OLE_RELEASE_METHOD;

/* rendering options */
typedef enum
{
    olerender_none,
    olerender_draw,
    olerender_format
} OLEOPT_RENDER;

/* standard clipboard format type */
typedef WORD OLECLIPFORMAT;

/* Link update options */
typedef enum
{
    oleupdate_always,
    oleupdate_onsave,
#ifdef OLE_INTERNAL
    oleupdate_oncall,
    oleupdate_onclose
#else
    oleupdate_oncall
#endif  /* OLE_INTERNAL */
} OLEOPT_UPDATE;

typedef HANDLE  HOBJECT;
typedef LONG    LHSERVER;
typedef LONG    LHCLIENTDOC;
typedef LONG    LHSERVERDOC;

typedef struct _OLEOBJECT FAR*  LPOLEOBJECT;
typedef struct _OLESTREAM FAR*  LPOLESTREAM;
typedef struct _OLECLIENT FAR*  LPOLECLIENT;


/* object method table definitions. */
typedef struct _OLEOBJECTVTBL
{
    void FAR*      (CALLBACK* QueryProtocol)        (LPOLEOBJECT, OLE_LPCSTR);
    OLESTATUS      (CALLBACK* Release)              (LPOLEOBJECT);
    OLESTATUS      (CALLBACK* Show)                 (LPOLEOBJECT, BOOL);
    OLESTATUS      (CALLBACK* DoVerb)               (LPOLEOBJECT, UINT, BOOL, BOOL);
    OLESTATUS      (CALLBACK* GetData)              (LPOLEOBJECT, OLECLIPFORMAT, HANDLE FAR*);
    OLESTATUS      (CALLBACK* SetData)              (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
    OLESTATUS      (CALLBACK* SetTargetDevice)      (LPOLEOBJECT, HGLOBAL);
    OLESTATUS      (CALLBACK* SetBounds)            (LPOLEOBJECT, OLE_CONST RECT FAR*);
    OLECLIPFORMAT  (CALLBACK* EnumFormats)          (LPOLEOBJECT, OLECLIPFORMAT);
    OLESTATUS      (CALLBACK* SetColorScheme)       (LPOLEOBJECT, OLE_CONST LOGPALETTE FAR*);
    /* Server has to implement only the above methods. */

#ifndef SERVERONLY
    /* Extra methods required for client. */
    OLESTATUS      (CALLBACK* Delete)               (LPOLEOBJECT);
    OLESTATUS      (CALLBACK* SetHostNames)         (LPOLEOBJECT, OLE_LPCSTR, OLE_LPCSTR);
    OLESTATUS      (CALLBACK* SaveToStream)         (LPOLEOBJECT, LPOLESTREAM);
    OLESTATUS      (CALLBACK* Clone)                (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR*);
    OLESTATUS      (CALLBACK* CopyFromLink)         (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR*);
    OLESTATUS      (CALLBACK* Equal)                (LPOLEOBJECT, LPOLEOBJECT);
    OLESTATUS      (CALLBACK* CopyToClipboard)      (LPOLEOBJECT);
    OLESTATUS      (CALLBACK* Draw)                 (LPOLEOBJECT, HDC, OLE_CONST RECT FAR*, OLE_CONST RECT FAR*, HDC);
    OLESTATUS      (CALLBACK* Activate)             (LPOLEOBJECT, UINT, BOOL, BOOL, HWND, OLE_CONST RECT FAR*);
    OLESTATUS      (CALLBACK* Execute)              (LPOLEOBJECT, HGLOBAL, UINT);
    OLESTATUS      (CALLBACK* Close)                (LPOLEOBJECT);
    OLESTATUS      (CALLBACK* Update)               (LPOLEOBJECT);
    OLESTATUS      (CALLBACK* Reconnect)            (LPOLEOBJECT);

    OLESTATUS      (CALLBACK* ObjectConvert)        (LPOLEOBJECT, OLE_LPCSTR, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR*);
    OLESTATUS      (CALLBACK* GetLinkUpdateOptions) (LPOLEOBJECT, OLEOPT_UPDATE FAR*);
    OLESTATUS      (CALLBACK* SetLinkUpdateOptions) (LPOLEOBJECT, OLEOPT_UPDATE);

    OLESTATUS      (CALLBACK* Rename)               (LPOLEOBJECT, OLE_LPCSTR);
    OLESTATUS      (CALLBACK* QueryName)            (LPOLEOBJECT, LPSTR, UINT FAR*);

    OLESTATUS      (CALLBACK* QueryType)            (LPOLEOBJECT, LONG FAR*);
    OLESTATUS      (CALLBACK* QueryBounds)          (LPOLEOBJECT, RECT FAR*);
    OLESTATUS      (CALLBACK* QuerySize)            (LPOLEOBJECT, DWORD FAR*);
    OLESTATUS      (CALLBACK* QueryOpen)            (LPOLEOBJECT);
    OLESTATUS      (CALLBACK* QueryOutOfDate)       (LPOLEOBJECT);

    OLESTATUS      (CALLBACK* QueryReleaseStatus)   (LPOLEOBJECT);
    OLESTATUS      (CALLBACK* QueryReleaseError)    (LPOLEOBJECT);
    OLE_RELEASE_METHOD (CALLBACK* QueryReleaseMethod)(LPOLEOBJECT);

    OLESTATUS      (CALLBACK* RequestData)          (LPOLEOBJECT, OLECLIPFORMAT);
    OLESTATUS      (CALLBACK* ObjectLong)           (LPOLEOBJECT, UINT, LONG FAR*);

/* This method is internal only */
    OLESTATUS      (CALLBACK* ChangeData)           (LPOLEOBJECT, HANDLE, LPOLECLIENT, BOOL);
#endif  /* !SERVERONLY */
} OLEOBJECTVTBL;
typedef  OLEOBJECTVTBL FAR* LPOLEOBJECTVTBL;

#ifndef OLE_INTERNAL
typedef struct _OLEOBJECT
{
    LPOLEOBJECTVTBL    lpvtbl;
} OLEOBJECT;
#endif

/* ole client definitions */
typedef struct _OLECLIENTVTBL
{
    int (CALLBACK* CallBack)(LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT);
} OLECLIENTVTBL;

typedef  OLECLIENTVTBL FAR*  LPOLECLIENTVTBL;

typedef struct _OLECLIENT
{
    LPOLECLIENTVTBL   lpvtbl;
} OLECLIENT;

/* Stream definitions */
typedef struct _OLESTREAMVTBL
{
    DWORD (CALLBACK* Get)(LPOLESTREAM, void FAR*, DWORD);
    DWORD (CALLBACK* Put)(LPOLESTREAM, OLE_CONST void FAR*, DWORD);
} OLESTREAMVTBL;
typedef  OLESTREAMVTBL FAR*  LPOLESTREAMVTBL;

typedef struct _OLESTREAM
{
    LPOLESTREAMVTBL      lpstbl;
} OLESTREAM;

/* Public Function Prototypes */
OLESTATUS   WINAPI  OleDelete(LPOLEOBJECT);
OLESTATUS   WINAPI  OleRelease(LPOLEOBJECT);
OLESTATUS   WINAPI  OleSaveToStream(LPOLEOBJECT, LPOLESTREAM);
OLESTATUS   WINAPI  OleEqual(LPOLEOBJECT, LPOLEOBJECT );
OLESTATUS   WINAPI  OleCopyToClipboard(LPOLEOBJECT);
OLESTATUS   WINAPI  OleSetHostNames(LPOLEOBJECT, LPCSTR, LPCSTR);
OLESTATUS   WINAPI  OleSetTargetDevice(LPOLEOBJECT, HGLOBAL);
OLESTATUS   WINAPI  OleSetBounds(LPOLEOBJECT, const RECT FAR*);
OLESTATUS   WINAPI  OleSetColorScheme(LPOLEOBJECT, const LOGPALETTE FAR*);
OLESTATUS   WINAPI  OleQueryBounds(LPOLEOBJECT, RECT FAR*);
OLESTATUS   WINAPI  OleQuerySize(LPOLEOBJECT, DWORD FAR*);
OLESTATUS   WINAPI  OleDraw(LPOLEOBJECT, HDC, const RECT FAR*, const RECT FAR*, HDC);
OLESTATUS   WINAPI  OleQueryOpen(LPOLEOBJECT);
OLESTATUS   WINAPI  OleActivate(LPOLEOBJECT, UINT, BOOL, BOOL, HWND, const RECT FAR*);
OLESTATUS   WINAPI  OleExecute(LPOLEOBJECT, HGLOBAL, UINT);
OLESTATUS   WINAPI  OleClose(LPOLEOBJECT);
OLESTATUS   WINAPI  OleUpdate(LPOLEOBJECT);
OLESTATUS   WINAPI  OleReconnect(LPOLEOBJECT);
OLESTATUS   WINAPI  OleGetLinkUpdateOptions(LPOLEOBJECT, OLEOPT_UPDATE FAR*);
OLESTATUS   WINAPI  OleSetLinkUpdateOptions(LPOLEOBJECT, OLEOPT_UPDATE);
void FAR*   WINAPI  OleQueryProtocol(LPOLEOBJECT, LPCSTR);

/* Routines related to asynchronous operations. */
OLESTATUS   WINAPI  OleQueryReleaseStatus(LPOLEOBJECT);
OLESTATUS   WINAPI  OleQueryReleaseError(LPOLEOBJECT);
OLE_RELEASE_METHOD WINAPI OleQueryReleaseMethod(LPOLEOBJECT);

OLESTATUS   WINAPI  OleQueryType(LPOLEOBJECT, LONG FAR*);

/* LOWORD is major version, HIWORD is minor version */
DWORD       WINAPI  OleQueryClientVersion(void);
DWORD       WINAPI  OleQueryServerVersion(void);

/* Converting to format (as in clipboard): */
OLECLIPFORMAT  WINAPI  OleEnumFormats(LPOLEOBJECT, OLECLIPFORMAT);
OLESTATUS   WINAPI  OleGetData(LPOLEOBJECT, OLECLIPFORMAT, HANDLE FAR*);
OLESTATUS   WINAPI  OleSetData(LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
OLESTATUS   WINAPI  OleQueryOutOfDate(LPOLEOBJECT);
OLESTATUS   WINAPI  OleRequestData(LPOLEOBJECT, OLECLIPFORMAT);

/* Query apis for creation from clipboard */
OLESTATUS   WINAPI  OleQueryLinkFromClip(LPCSTR, OLEOPT_RENDER, OLECLIPFORMAT);
OLESTATUS   WINAPI  OleQueryCreateFromClip(LPCSTR, OLEOPT_RENDER, OLECLIPFORMAT);

/* Object creation functions */
OLESTATUS   WINAPI  OleCreateFromClip(LPCSTR, LPOLECLIENT, LHCLIENTDOC, LPCSTR,  LPOLEOBJECT FAR*, OLEOPT_RENDER, OLECLIPFORMAT);
OLESTATUS   WINAPI  OleCreateLinkFromClip(LPCSTR, LPOLECLIENT, LHCLIENTDOC, LPCSTR, LPOLEOBJECT FAR*, OLEOPT_RENDER, OLECLIPFORMAT);
OLESTATUS   WINAPI  OleCreateFromFile(LPCSTR, LPOLECLIENT, LPCSTR, LPCSTR, LHCLIENTDOC, LPCSTR, LPOLEOBJECT FAR*, OLEOPT_RENDER, OLECLIPFORMAT);
OLESTATUS   WINAPI  OleCreateLinkFromFile(LPCSTR, LPOLECLIENT, LPCSTR, LPCSTR, LPCSTR, LHCLIENTDOC, LPCSTR, LPOLEOBJECT FAR*, OLEOPT_RENDER, OLECLIPFORMAT);
OLESTATUS   WINAPI  OleLoadFromStream(LPOLESTREAM, LPCSTR, LPOLECLIENT, LHCLIENTDOC, LPCSTR, LPOLEOBJECT FAR*);
OLESTATUS   WINAPI  OleCreate(LPCSTR, LPOLECLIENT, LPCSTR, LHCLIENTDOC, LPCSTR, LPOLEOBJECT FAR*, OLEOPT_RENDER, OLECLIPFORMAT);
OLESTATUS   WINAPI  OleCreateInvisible(LPCSTR, LPOLECLIENT, LPCSTR, LHCLIENTDOC, LPCSTR, LPOLEOBJECT FAR*, OLEOPT_RENDER, OLECLIPFORMAT, BOOL);
OLESTATUS   WINAPI  OleCreateFromTemplate(LPCSTR, LPOLECLIENT, LPCSTR, LHCLIENTDOC, LPCSTR, LPOLEOBJECT FAR*, OLEOPT_RENDER, OLECLIPFORMAT);
OLESTATUS   WINAPI  OleClone(LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, LPCSTR, LPOLEOBJECT FAR*);
OLESTATUS   WINAPI  OleCopyFromLink(LPOLEOBJECT, LPCSTR, LPOLECLIENT, LHCLIENTDOC, LPCSTR, LPOLEOBJECT FAR*);
OLESTATUS   WINAPI  OleObjectConvert(LPOLEOBJECT, LPCSTR, LPOLECLIENT, LHCLIENTDOC, LPCSTR, LPOLEOBJECT FAR*);
OLESTATUS   WINAPI  OleRename(LPOLEOBJECT, LPCSTR);
OLESTATUS   WINAPI  OleQueryName(LPOLEOBJECT, LPSTR, UINT FAR*);
OLESTATUS   WINAPI  OleRevokeObject(LPOLECLIENT);
BOOL        WINAPI  OleIsDcMeta(HDC);

/* client document API */
OLESTATUS   WINAPI  OleRegisterClientDoc(LPCSTR, LPCSTR, LONG, LHCLIENTDOC FAR*);
OLESTATUS   WINAPI  OleRevokeClientDoc(LHCLIENTDOC);
OLESTATUS   WINAPI  OleRenameClientDoc(LHCLIENTDOC, LPCSTR);
OLESTATUS   WINAPI  OleRevertClientDoc(LHCLIENTDOC);
OLESTATUS   WINAPI  OleSavedClientDoc(LHCLIENTDOC);
OLESTATUS   WINAPI  OleEnumObjects(LHCLIENTDOC, LPOLEOBJECT FAR*);

/* server usage definitions */
typedef enum {
    OLE_SERVER_MULTI,           /* multiple instances */
    OLE_SERVER_SINGLE           /* single instance(multiple document) */
} OLE_SERVER_USE;

/* Server API */
typedef struct _OLESERVER FAR*  LPOLESERVER;

OLESTATUS   WINAPI  OleRegisterServer(LPCSTR, LPOLESERVER, LHSERVER FAR*, HINSTANCE, OLE_SERVER_USE);
OLESTATUS   WINAPI  OleRevokeServer(LHSERVER);
OLESTATUS   WINAPI  OleBlockServer(LHSERVER);
OLESTATUS   WINAPI  OleUnblockServer(LHSERVER, BOOL FAR*);

/* APIs to keep server open */
OLESTATUS   WINAPI  OleLockServer(LPOLEOBJECT, LHSERVER FAR*);
OLESTATUS   WINAPI  OleUnlockServer(LHSERVER);

/* Server document API */

typedef struct _OLESERVERDOC FAR*  LPOLESERVERDOC;

OLESTATUS   WINAPI  OleRegisterServerDoc(LHSERVER, LPCSTR, LPOLESERVERDOC, LHSERVERDOC FAR*);
OLESTATUS   WINAPI  OleRevokeServerDoc(LHSERVERDOC);
OLESTATUS   WINAPI  OleRenameServerDoc(LHSERVERDOC, LPCSTR);
OLESTATUS   WINAPI  OleRevertServerDoc(LHSERVERDOC);
OLESTATUS   WINAPI  OleSavedServerDoc(LHSERVERDOC);

typedef struct _OLESERVERVTBL
{
    OLESTATUS (CALLBACK* Open)  (LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, LPOLESERVERDOC FAR*);
                                    /* long handle to doc(privtate to DLL)  */
                                    /* lp to OLESERVER                      */
                                    /* document name                        */
                                    /* place holder for returning oledoc.   */

    OLESTATUS (CALLBACK* Create)(LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, OLE_LPCSTR, LPOLESERVERDOC FAR*);
                                    /* long handle to doc(privtate to DLL)  */
                                    /* lp to OLESERVER                      */
                                    /* lp class name                        */
                                    /* lp doc name                          */
                                    /* place holder for returning oledoc.   */

    OLESTATUS (CALLBACK* CreateFromTemplate)(LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, OLE_LPCSTR, OLE_LPCSTR, LPOLESERVERDOC FAR*);
                                    /* long handle to doc(privtate to DLL)  */
                                    /* lp to OLESERVER                      */
                                    /* lp class name                        */
                                    /* lp doc name                          */
                                    /* lp template name                     */
                                    /* place holder for returning oledoc.   */

    OLESTATUS (CALLBACK* Edit)  (LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, OLE_LPCSTR, LPOLESERVERDOC FAR*);
                                    /* long handle to doc(privtate to DLL)  */
                                    /* lp to OLESERVER                      */
                                    /* lp class name                        */
                                    /* lp doc name                          */
                                    /* place holder for returning oledoc.   */

    OLESTATUS (CALLBACK* Exit)  (LPOLESERVER);
                                    /* lp OLESERVER                         */

    OLESTATUS (CALLBACK* Release)  (LPOLESERVER);
                                    /* lp OLESERVER                         */

    OLESTATUS (CALLBACK* Execute)(LPOLESERVER, HGLOBAL);
                                    /* lp OLESERVER                         */
                                    /* handle to command strings            */
} OLESERVERVTBL;
typedef  OLESERVERVTBL FAR*  LPOLESERVERVTBL;

typedef struct _OLESERVER
{
    LPOLESERVERVTBL    lpvtbl;
} OLESERVER;

typedef struct _OLESERVERDOCVTBL
{
    OLESTATUS (CALLBACK* Save)      (LPOLESERVERDOC);
    OLESTATUS (CALLBACK* Close)     (LPOLESERVERDOC);
    OLESTATUS (CALLBACK* SetHostNames)(LPOLESERVERDOC, OLE_LPCSTR, OLE_LPCSTR);
    OLESTATUS (CALLBACK* SetDocDimensions)(LPOLESERVERDOC, OLE_CONST RECT FAR*);
    OLESTATUS (CALLBACK* GetObject) (LPOLESERVERDOC, OLE_LPCSTR, LPOLEOBJECT FAR*, LPOLECLIENT);
    OLESTATUS (CALLBACK* Release)   (LPOLESERVERDOC);
    OLESTATUS (CALLBACK* SetColorScheme)(LPOLESERVERDOC, OLE_CONST LOGPALETTE FAR*);
    OLESTATUS (CALLBACK* Execute)  (LPOLESERVERDOC, HGLOBAL);
} OLESERVERDOCVTBL;
typedef  OLESERVERDOCVTBL FAR*  LPOLESERVERDOCVTBL;

typedef struct _OLESERVERDOC
{
    LPOLESERVERDOCVTBL lpvtbl;
} OLESERVERDOC;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifdef WIN16
#include <poppack.h>
#endif

#endif  /* !_INC_OLE */
