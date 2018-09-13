#ifndef _ADMIN_H_
#define _ADMIN_H_

#ifdef ADMIN_PINNED_FOLDERS

#define chAdminLogoff   TEXT('1')
#define chAdminLogon    TEXT('2')
#define chAdminIdle     TEXT('3')

// Structure to hang onto Admniistratively Pinned Folders
class CAPFolder
{
private:
    LPTSTR m_pszFolder;
    LPTSTR m_pszFlags;

public:
    CAPFolder() {m_pszFolder=NULL; m_pszFlags=NULL;}
    ~CAPFolder();
    LPCTSTR Path()  { return m_pszFolder; }
    LPCTSTR Flags() { return m_pszFlags;  }
    void Initialize(HKEY hKey, LPTSTR pszValue);
    BOOL IsChild(LPCTSTR);
    BOOL SyncAtLogoff() { return CheckFlag(chAdminLogoff); }
    BOOL SyncAtLogon()  { return CheckFlag(chAdminLogon); }
    BOOL SyncAtIdle()   { return CheckFlag(chAdminIdle); }

private:
    BOOL CheckFlag(TCHAR ch) { return (m_pszFlags && StrChr(m_pszFlags, ch)); }
};

BOOL ReadAPFFromRegistry(CAPFolder ** ppapf, int * pcapf);


#endif  // ADMIN_PINNED_FOLDERS

#endif  // _ADMIN_H_
