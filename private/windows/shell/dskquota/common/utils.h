#ifndef _INC_DSKQUOTA_UTILS_H
#define _INC_DSKQUOTA_UTILS_H
///////////////////////////////////////////////////////////////////////////////
/*  File: utils.h

    Description: Header for general utilities module.  It is expected that
        windows.h is included before this header.



    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

#ifndef _INC_DSKQUOTA_PRIVATE_H
#   include "private.h"
#endif

//
// Convert a value to a "bool".
// Lower-case "boolify" is intentional to enforce relationship
// to type "bool".
//
template <class T>
inline bool boolify(const T& x)
{
    return !!x;
}


template <class T>
const T&
MAX(const T& a, const T& b)
{
    return a > b ? a : b;
}

template <class T>
const T&
MIN(const T& a, const T& b)
{
    return a < b ? a : b;
}


template <class T>
void
SWAP(T& a, T& b)
{
    T temp(a);
    a = b;
    b = temp;
}


//
// Trivial class for ensuring window redraw is restored in the case
// of an exception.
//
class CAutoSetRedraw
{
    public:
        CAutoSetRedraw(HWND hwnd)
            : m_hwnd(hwnd) { }

        CAutoSetRedraw(HWND hwnd, bool bSet)
            : m_hwnd(hwnd) { Set(bSet); }

        ~CAutoSetRedraw(void)
            { Set(true); }

        void Set(bool bSet)
            { SendMessage(m_hwnd, WM_SETREDRAW, (WPARAM)bSet, 0); }

    private:
        HWND m_hwnd;
};


//
// Trivial class for ensuring window is enabled in the case
// of an exception.
//
class CAutoWndEnable
{
    public:
        CAutoWndEnable(HWND hwnd)
            : m_hwnd(hwnd) { }

        CAutoWndEnable(HWND hwnd, bool bEnable)
            : m_hwnd(hwnd) { Enable(bEnable); }

        ~CAutoWndEnable(void)
            { Enable(true); }

        void Enable(bool bEnable)
            { EnableWindow(m_hwnd, bEnable); }

    private:
        HWND m_hwnd;
};


//
// Ensure NT handles are exception safe.
//
class CNtHandle
{
    public:
        CNtHandle(HANDLE handle)
            : m_handle(handle) { }

        CNtHandle(void)
            : m_handle(NULL) { }

        ~CNtHandle(void)
            { Close(); }

        void Close(void)
            { if (m_handle) NtClose(m_handle); m_handle = NULL; }

        operator HANDLE() const
            { return m_handle; }

        HANDLE *HandlePtr(void)
            { DBGASSERT((NULL == m_handle)); return &m_handle; }

    private:
        HANDLE m_handle;

        //
        // Prevent copy.
        // This class is only intended for automatic handle cleanup.
        //
        CNtHandle(const CNtHandle& rhs);
        CNtHandle& operator = (const CNtHandle& rhs);
};


//
// Ensure Win32 handles are exception safe.
//
class CWin32Handle
{
    public:
        CWin32Handle(HANDLE handle)
            : m_handle(handle) { }

        CWin32Handle(void)
            : m_handle(NULL) { }

        ~CWin32Handle(void)
            { Close(); }

        void Close(void)
            { if (m_handle) CloseHandle(m_handle); m_handle = NULL; }

        operator HANDLE() const
            { return m_handle; }

        HANDLE *HandlePtr(void)
            { DBGASSERT((NULL == m_handle)); return &m_handle; }

    private:
        HANDLE m_handle;

        //
        // Prevent copy.
        // This class is only intended for automatic handle cleanup.
        //
        CWin32Handle(const CWin32Handle& rhs);
        CWin32Handle& operator = (const CWin32Handle& rhs);
};


//
// Trivial inline class to automate the cleanup of a STGMEDIUM
// structure.
//
class CStgMedium : public STGMEDIUM
{
    public:
        CStgMedium(void)
            { tymed = TYMED_NULL; hGlobal = NULL; pUnkForRelease = NULL; }

        ~CStgMedium(void)
            { ReleaseStgMedium(this); }

        operator LPSTGMEDIUM(void)
            { return (LPSTGMEDIUM)this; }

        operator const STGMEDIUM& (void)
            { return (STGMEDIUM &)*this; }
};


//
// On mounted volumes, the parsing name and display name are different.
// The parsing name contains a GUID which means nothing to the user.
// This class encapsulates both strings into a single class that can
// be passed as a single object to functions requiring a volume
// identifier.
//
// Here is an example of what the strings will contain:
//
//          Mounted volume              Non-mounted volume
//
// Display  Label (C:\FOO)              C:\
// Parsing  \\?\Volume\{GUID}           C:\
// FSPath   C:\FOO                      C:\
//
// I've coded this using only CString object references to leverage
// the reference counting of the CString class and minimize string copying.
//
class CVolumeID
{
    public:
        CVolumeID(void)
            : m_bMountedVol(false) { }

        CVolumeID(
            const CString& strForParsing, 
            const CString& strForDisplay,
            const CString& strFSPath
            ) : m_bMountedVol(false)
            { SetNames(strForParsing, strForDisplay, strFSPath); }

        ~CVolumeID(void) { };

        bool IsMountedVolume(void) const
            { return m_bMountedVol; }

        void SetNames(
            const CString& strForParsing, 
            const CString& strForDisplay,
            const CString& strFSPath)
            { m_strForParsing = strForParsing;
              m_strFSPath     = strFSPath;
              m_strForDisplay = strForDisplay;
              m_bMountedVol = !!(strForParsing != strForDisplay); }

        const CString& ForParsing(void) const
            { return m_strForParsing; }

        void ForParsing(CString *pstr) const
            { *pstr = m_strForParsing; }

        const CString& ForDisplay(void) const
            { return m_strForDisplay; }

        void ForDisplay(CString *pstr) const
            { *pstr = m_strForDisplay; }

        const CString& FSPath(void) const
            { return m_strFSPath; }

        void FSPath(CString *pstr) const
            { *pstr = m_strFSPath; }

    private:
        CString m_strForParsing;
        CString m_strForDisplay;
        CString m_strFSPath;
        bool    m_bMountedVol;
};



//
// Don't want to include dskquota.h for these.
// Including it here places the CLSIDs and IIDs in the precompiled header
// which screws up the declaration/definition of the GUIDs.
//
struct IDiskQuotaUser; // fwd decl.
#define SIDLIST  FILE_GET_QUOTA_INFORMATION
#define PSIDLIST PFILE_GET_QUOTA_INFORMATION

BOOL SidToString(
    PSID pSid,
    LPTSTR pszSid,
    LPDWORD pcchSid);

BOOL SidToString(
    PSID pSid,
    LPTSTR *ppszSid);

HRESULT
CreateSidList(
    PSID *rgpSids,
    DWORD cpSids,
    PSIDLIST *ppSidList,
    LPDWORD pcbSidList);

VOID MessageBoxNYI(VOID);

inline INT DiskQuotaMsgBox(HWND hWndParent,
                           LPCTSTR pszText,
                           LPCTSTR pszTitle,
                           UINT uType);

INT DiskQuotaMsgBox(HWND hWndParent,
                    UINT idMsgText,
                    UINT idMsgTitle,
                    UINT uType);

INT DiskQuotaMsgBox(HWND hWndParent,
                    UINT idMsgText,
                    LPCTSTR pszTitle,
                    UINT uType);

INT DiskQuotaMsgBox(HWND hWndParent,
                    LPCTSTR pszText,
                    UINT idMsgTitle,
                    UINT uType);

LPTSTR StringDup(LPCTSTR pszSource);
PSID SidDup(PSID pSid);
BOOL UserIsAdministrator(IDiskQuotaUser *pUser);
VOID CenterPopupWindow(HWND hwnd, HWND hwndParent = NULL);
HRESULT CallRegInstall(HINSTANCE hInstance, LPSTR szSection);
void GetDlgItemText(HWND hwnd, UINT idCtl, CString *pstrText);



#endif // _INC_DSKQUOTA_UTILS_H

