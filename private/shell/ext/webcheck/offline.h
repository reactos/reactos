#ifndef OFFLINE_H
#define OFFLINE_H

#pragma intrinsic(memcpy, memset, memcmp)

typedef enum AGENT_PRIORITY {
    AGENT_PRIORITY_NORMAL = 2
} AGENT_PRIORITY;

typedef enum    {_INIT_FROM_URL, _INIT_FROM_INTSHCUT, _INIT_FROM_CHANNEL} INIT_SRC_ENUM;

#define MAX_NAME_QUICKLINK      MAX_PATH
#define MAX_NAME                MAX_NAME_QUICKLINK
#define MAX_URL                 INTERNET_MAX_URL_LENGTH
#define MAX_USERNAME            127
#define MAX_PASSWORD            63
#define MAX_STATUS              127

#define MAX_PROP_PAGES          10
#define MAX_WC_AGENT_PAGES      2

typedef struct {
    int     templateRCID;
    DLGPROC dlgProc;
} PageType, * LPPageType;
typedef const PageType *CLPPageType;

// PIDL format for this folder...
typedef struct OOEntry
{
    DWORD       dwSize;
    DWORD       dwFlags;
    CFileTime   m_LastUpdated;
    CFileTime   m_NextUpdate;
    LONG        m_SizeLimit;
    LONG        m_ActualSize;
    LONG        m_RecurseLevels;
    LONG        m_RecurseFlags;
    AGENT_PRIORITY  m_Priority;
    BOOL        bDesktop;
    BOOL        bChannel;
    BOOL        bMail;
    BOOL        bGleam;
    BOOL        bChangesOnly;
    BOOL        bNeedPassword;
    TASK_TRIGGER    m_Trigger;
    DWORD       fChannelFlags;
    SUBSCRIPTIONCOOKIE m_Cookie;
    SUBSCRIPTIONCOOKIE groupCookie;
    DWORD       grfTaskTrigger;
    CLSID       clsidDest;
    SCODE       status;
    LPTSTR      username;
    LPTSTR      password;
    LPTSTR      m_URL;
    LPTSTR      m_Name;
    LPTSTR      statusStr;
} OOEntry;

typedef UNALIGNED OOEntry * POOEntry;

#define PSF_NO_SCHEDULED_UPDATES    0x00000001
#define PSF_NO_EDITING_SCHEDULES    0x00000002
#define PSF_NO_AUTO_NAME_SCHEDULE   0x00000004
#define PSF_NO_CHECK_SCHED_CONFLICT 0x00000008
#define PSF_IS_ALREADY_SUBSCRIBED   0x00000010

typedef struct
{ 
    DWORD       dwFlags;
    CFileTime   m_LastUpdated;
    CFileTime   m_NextUpdate;
    LONG        m_SizeLimit;
    LONG        m_ActualSize;
    LONG        m_RecurseLevels;
    LONG        m_RecurseFlags;
    AGENT_PRIORITY  m_Priority;
    BOOL        bDesktop;
    BOOL        bChannel;
    BOOL        bMail;
    BOOL        bGleam;
    BOOL        bChangesOnly;
    BOOL        bNeedPassword;
    TASK_TRIGGER    m_Trigger;
    DWORD       fChannelFlags;
    SUBSCRIPTIONCOOKIE m_Cookie;
    SUBSCRIPTIONCOOKIE groupCookie;
    DWORD       grfTaskTrigger;
    CLSID       clsidDest;
    SCODE       status;
    TCHAR       username[MAX_USERNAME + 1];
    TCHAR       password[MAX_PASSWORD + 1];
    TCHAR       m_URL[MAX_URL + 1];
    TCHAR       m_Name[MAX_NAME + 1];
    TCHAR       statusStr[MAX_STATUS +1];
    DWORD       m_dwPropSheetFlags;     // used internally by propsheets and wizard
    HWND        hwndNewSchedDlg;
} OOEBuf, * POOEBuf;

typedef struct
{
    USHORT  cb;
    USHORT  usSign;
    OOEntry ooe;  //  Should point to the place right after itself.
} MYPIDL;

typedef UNALIGNED MYPIDL *LPMYPIDL;

typedef struct _ColInfoType {
    short int iCol;
    short int ids;        // Id of string for title
    short int cchCol;     // Number of characters wide to make column
    short int iFmt;       // The format of the column;
} ColInfoType;

enum {
    ICOLC_SHORTNAME = 0,
    ICOLC_LAST,
    ICOLC_STATUS, 
    ICOLC_URL,
    ICOLC_ACTUALSIZE
};
  
#define MYPIDL_MAGIC       0x7405

#define RETURN_ON_FAILURE(hr)   if (FAILED(hr)) return hr

#define IS_VALID_MYPIDL(pidl)      ((((LPMYPIDL)pidl)->cb > sizeof(MYPIDL)) && \
                                     (((LPMYPIDL)pidl)->usSign == (USHORT)MYPIDL_MAGIC))

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define NAME(pooe)  (LPTSTR)((LPBYTE)(pooe) + (DWORD_PTR)((pooe)->m_Name))
#define URL(pooe)   (LPTSTR)((LPBYTE)(pooe) + (DWORD_PTR)((pooe)->m_URL))
#define UNAME(pooe) (LPTSTR)((LPBYTE)(pooe) + (DWORD_PTR)((pooe)->username))
#define PASSWD(pooe) (LPTSTR)((LPBYTE)(pooe) + (DWORD_PTR)((pooe)->password))
#define STATUS(pooe) (LPTSTR)((LPBYTE)(pooe) + (DWORD_PTR)((pooe)->statusStr))

#ifdef __cplusplus
extern "C" {
#endif
    
extern HINSTANCE g_hInst;
extern const CHAR c_szOpen[];
extern const CHAR c_szUpdate[];
extern const CHAR c_szDelete[];
extern const CHAR c_szProperties[];
extern const CHAR c_szCopy[];

extern const CLSID CLSID_OfflineFolder;
extern const CLSID CLSID_WebcrawlHelper;
extern const CLSID IID_IOfflineObject;

#ifdef __cplusplus
};
#endif

#include "wizards.h"
#include "utils.h"  // NOTE: must come at end to get all the definitions

#endif // OFFLINE_H
