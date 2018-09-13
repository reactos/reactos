/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rshx32.h

Abstract:

    Remote administration shell extension.

Author:

    Don Ryan (donryan) 23-May-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _RSHX32_H_
#define _RSHX32_H_

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#define SIZEOF(x)       sizeof(x)
#endif

//
// Avoid bringing in C runtime code for NO reason
//
static inline void * __cdecl operator new(unsigned int size) { return (void *)LocalAlloc(LPTR, size); }
static inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) {return 0;}

DEFINE_GUID(CLSID_RShellExt, 0x1f2e5c40, 0x9550, 
    0x11ce, 0x99, 0xd2, 0x00, 0xaa, 0x00, 0x6e, 0x08, 0x6c);

class CRShellExtCF : public IClassFactory
{
protected:
    ULONG m_cRef;         
    
public:
    CRShellExtCF();
    ~CRShellExtCF();
        
    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};

class CRShellExt : public IShellPropSheetExt, IShellExtInit
{
protected:
    ULONG        m_cRef;  
    LPDATAOBJECT m_lpdobj; // interface passed in by shell

public:
    CRShellExt();
    ~CRShellExt();
    
    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // IShellExtInit method
    STDMETHODIMP Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY);

    // IShellPropSheetExt methods
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE, LPARAM);
    STDMETHODIMP ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM);
};

typedef CRShellExt* PRSHELLEXT;

#define CF_NETRESOURCE          TEXT("Net Resource")
#define MAX_ONE_RESOURCE        1024

typedef struct _RPROPSHEETPAGE {
    PROPSHEETPAGE psp;
    HWND          hDlg;
    DWORD         flags;
    DWORD         cItems;
    LPDATAOBJECT  lpdobj;
    TCHAR         szBuf[MAX_ONE_RESOURCE];
} RPROPSHEETPAGE, *PRPROPSHEETPAGE;

#define DOBJ_CF_HDROP     0x00000001L 
#define DOBJ_CF_HNRES     0x00000002L
#define DOBJ_CF_MASK      0x0000000FL
#define IsFormatHDROP(dw) ((dw)&(DOBJ_CF_HDROP))
#define IsFormatHNRES(dw) ((dw)&(DOBJ_CF_HNRES))

#define DOBJ_FOC_LOCAL    0x00000010L 
#define DOBJ_FOC_REMOTE   0x00000020L
#define DOBJ_FOC_MASK     0x000000F0L
#define IsFocusLocal(dw)  ((dw)&(DOBJ_FOC_LOCAL))
#define IsFocusRemote(dw) ((dw)&(DOBJ_FOC_REMOTE))

#define DOBJ_SRV_NT       0x00000100L 
#define DOBJ_SRV_OS2      0x00000200L
#define DOBJ_SRV_W95      0x00000400L
#define DOBJ_SRV_MASK     0x0000FF00L
#define IsServerNT(dw)    ((dw)&(DOBJ_SRV_NT))
#define IsServerOS2(dw)   ((dw)&(DOBJ_SRV_OS2))
#define IsServerW95(dw)   ((dw)&(DOBJ_SRV_W95))

#define DOBJ_RES_DISK     0x00010000L 
#define DOBJ_RES_PRINT    0x00020000L
#define DOBJ_RES_MASK     0x00FF0000L     
#define IsResDisk(dw)     ((dw)&(DOBJ_RES_DISK)) 
#define IsResPrintQ(dw)   ((dw)&(DOBJ_RES_PRINT))

#define DOBJ_VOL_FAT      0x01000000L 
#define DOBJ_VOL_HPFS     0x02000000L
#define DOBJ_VOL_NTACLS   0x04000000L       // NTFS or OFS
#define DOBJ_VOL_MASK     0xFF000000L
#define IsVolumeFAT(dw)   ((dw)&(DOBJ_VOL_FAT)) 
#define IsVolumeHPFS(dw)  ((dw)&(DOBJ_VOL_HPFS)) 
#define IsVolumeNTACLS(dw)((dw)&(DOBJ_VOL_NTACLS))

#define DLL_SHELL32             TEXT("shell32.dll")
#define EXP_SHDRAGQUERYFILEA    "DragQueryFileA"
#define EXP_SHDRAGQUERYFILEW    "DragQueryFileW"
#define EXP_SHGETNETRESOURCE    (LPSTR)MAKEINTRESOURCE(69)

typedef HANDLE HNRES;
typedef UINT (WINAPI *SHDRAGQUERYFILEA)(HDROP, UINT, LPSTR, UINT);
typedef UINT (WINAPI *SHDRAGQUERYFILEW)(HDROP, UINT, LPWSTR, UINT);
typedef UINT (WINAPI *SHGETNETRESOURCE)(HNRES, UINT, LPNETRESOURCE, UINT);

#define DLL_ACLEDIT             TEXT("acledit.dll")
#define EXP_EDITPERMSINFO       "EditPermissionInfo"
#define EXP_EDITAUDITINFO       "EditAuditInfo"
#define EXP_EDITOWNERINFO       "EditOwnerInfo"
#define EXP_EDITQACL            "EditQACL"

typedef void (APIENTRY *ACLEDITPROC)(HWND);
typedef void (APIENTRY *EDITQACL)(HWND, LPTSTR, BOOL);

#define DLL_PRTQ32              TEXT("prtq32.dll")
#define EXP_EDITQACL2           "EditQACL2"

typedef void (APIENTRY *EDITQACL2)(HWND, LPTSTR, UINT);

#pragma pack(1)

#ifdef WINNT
#include <lm.h>

#define DLL_NET32               TEXT("netapi32.dll")
#define EXP_NETUSEGETINFO       "NetUseGetInfo"
#define EXP_NETSERVERGETINFO    "NetServerGetInfo"
#define EXP_NETAPIBUFFERFREE    "NetApiBufferFree"

typedef NET_API_STATUS (NET_API_FUNCTION *NETUSEGETINFO)(LPTSTR, LPTSTR, DWORD, LPBYTE *);
typedef NET_API_STATUS (NET_API_FUNCTION *NETSERVERGETINFO)(LPTSTR, DWORD, LPBYTE *);
typedef NET_API_STATUS (NET_API_FUNCTION *NETAPIBUFFERFREE)(LPVOID);

#else

#define DLL_NET32               TEXT("msnet32.dll")
#define EXP_NETUSEGETINFO       ((LPSTR)MAKEINTRESOURCE(46))
#define EXP_NETSERVERGETINFO    ((LPSTR)MAKEINTRESOURCE(15))

typedef UINT (APIENTRY *NETUSEGETINFO)(LPSTR, LPSTR, SHORT, LPSTR, USHORT, USHORT*);
typedef UINT (APIENTRY *NETSERVERGETINFO)(LPSTR, SHORT, LPSTR, USHORT, USHORT*); 

#define NERR_Success        0
#define NERR_UseNotFound    2250

#define NT_NOS_MAJOR_VER    3
#define MAJOR_VER_MASK      0x0F   

#define CNLEN               15
#define DEVLEN              8

typedef struct _USE_INFO_1 {
    CHAR    ui1_local[DEVLEN+1];
    CHAR    ui1_pad;
    LPSTR   ui1_remote;
    LPSTR   ui1_password;
    USHORT  ui1_status;
    SHORT   ui1_asg_type;
    USHORT  ui1_refcount;
    USHORT  ui1_usecount;    
} USE_INFO_1, *PUSE_INFO_1;

typedef struct _SERVER_INFO_1 {
    CHAR  sv1_name[CNLEN+1];
    BYTE  sv1_version_major;
    BYTE  sv1_version_minor;
    DWORD sv1_type;
    LPSTR sv1_comment;
} SERVER_INFO_1, *PSERVER_INFO_1;
#endif

#pragma pack()

#define IDD_SECURITY        100

#define IDI_PERMS           200
#define IDI_AUDIT           201
#define IDI_OWNER           202

#define IDC_PERMS           300 // must be first
#define IDC_AUDIT           301 // must be second
#define IDC_OWNER           302 // must be third

#define IDS_SECURITY        400
#define IDS_PERMS           401
#define IDS_AUDIT           402
#define IDS_OWNER           403
#define IDS_PERMS_DEFAULT   404
#define IDS_AUDIT_DEFAULT   405
#define IDS_OWNER_DEFAULT   406

#define IDC_STATIC          -1

#define FMXCLASSNAME        TEXT("RSHXFMXClass")

#endif // _RSHX32_H_
