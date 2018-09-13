///////////////////////////////////////////////////////////////////////////////
// Declaration of class CParseInf
//
// One instance of this class is created for each OCX being removed.  It stores
// all the files associated with the OCX in a linked list.  It also does the
// jobs of determining if the OCX is uninstallable and of the actual file
// removal.

#ifndef __PARSE_INF__
#define __PARSE_INF__

#include "filenode.h"
#include <pkgmgr.h>

#define REGSTR_COM_BRANCH                       "CLSID"
#define REGSTR_DOWNLOAD_INFORMATION             "DownloadInformation"
#define REGSTR_DLINFO_INF_FILE                  "INF"
#define REGSTR_DLINFO_CODEBASE                  "CODEBASE"
#define REGSTR_PATH_DIST_UNITS                  "Software\\Microsoft\\Code Store Database\\Distribution Units"
#define REGSTR_DU_CONTAINS_FILES                "Contains\\Files"
#define REGSTR_DU_CONTAINS_JAVA                 "Contains\\Java"
#define REGSTR_DU_CONTAINS_DIST_UNITS           "Contains\\Distribution Units"
#define REGSTR_VALUE_INF                        "INF"
#define REGSTR_VALUE_OSD                        "OSD"
#define REGSTR_INSTALLED_VERSION                "InstalledVersion"
#define REGSTR_VALUE_EXPIRE                     "Expire"
#define REGSTR_SHOW_ALL_FILES                   "ShowAllFiles"


#define MAX_REGPATH_LEN                           2048
#define MAX_CONTROL_NAME_LEN                      1024
#define MAX_MSGBOX_STRING_LEN                     2048
#define MAX_MSGBOX_TITLE_LEN                      256

#define BYTES_MAXSIZE                             32

BOOL IsShowAllFilesEnabled();
void ToggleShowAllFiles();

class CParseInf
{
// Construction
public:
    CParseInf();
    ~CParseInf();

// Data members
protected:
    DWORD m_dwTotalFileSize;
    DWORD m_dwFileSizeSaved;
    DWORD m_dwStatus;           // status value from the STATUS_CTRL set in <cleanoc.h>
    int m_nTotalFiles;
    CFileNode *m_pHeadFileList;
    CFileNode *m_pCurFileNode;
    CFileNode *m_pFileRetrievalPtr;
    CPackageNode *m_pHeadPackageList;
    CPackageNode *m_pCurPackageNode;
    CPackageNode *m_pPackageRetrievalPtr;

    TCHAR m_szInf[MAX_PATH];
    TCHAR m_szFileName[MAX_PATH];
    TCHAR m_szCLSID[MAX_CLSID_LEN];
    BOOL m_bIsDistUnit;
    BOOL m_bHasActiveX;
    BOOL m_bHasJava;
    IJavaPackageManager *m_pijpm;
    BOOL m_bCoInit;
    ULONG m_cExpireDays;

// Operations
public:
    virtual HRESULT DoParse(
        LPCTSTR szOCXFileName,
        LPCTSTR szCLSID);
    virtual HRESULT RemoveFiles(
        LPCTSTR lpszTypeLibID = NULL,
        BOOL bForceRemove = FALSE,
        DWORD dwIsDistUnit = FALSE,
        BOOL bSilent=FALSE);
    virtual DWORD GetTotalFileSize() const;
    virtual DWORD GetTotalSizeSaved() const;
    virtual int GetTotalFiles() const;
    virtual CFileNode* GetFirstFile();
    virtual CFileNode* GetNextFile();
    virtual CPackageNode* GetFirstPackage();
    virtual CPackageNode* GetNextPackage();
    virtual HRESULT DoParseDU(LPCTSTR szOCXFileName, LPCTSTR szCLSID);
    virtual void SetIsDistUnit(BOOL bDist);
    virtual BOOL GetIsDistUnit() const;
    virtual DWORD GetStatus() const;
    virtual BOOL GetHasActiveX(void) { return m_bHasActiveX; };
    virtual BOOL GetHasJava(void) { return m_bHasJava; };
    virtual ULONG GetExpireDays(void) { return m_cExpireDays; }

// private helper methods
protected:
    void Init();
    void DestroyFileList();
    void DestroyPackageList();
    HRESULT FindInf(LPTSTR szInf);
    HRESULT EnumSections();
    BOOL IsSectionInINF( LPCSTR lpCurCode);
    HRESULT HandleSatellites(LPCTSTR pszFileName);
    HRESULT GetFilePath(CFileNode* pFileNode);
    HRESULT ParseSetupHook();
    HRESULT ParseConditionalHook();
    HRESULT ParseUninstallSection(LPCTSTR lpszSection);
    HRESULT BuildDUFileList( HKEY hKeyDU );
    HRESULT BuildDUPackageList( HKEY hKeyDU );
    HRESULT BuildNamespacePackageList( HKEY hKeyNS, LPCTSTR szNamespace );
    HRESULT CheckFilesRemovability(void);
    HRESULT CheckLegacyRemovability( LONG *cOldSharedCount);
    HRESULT CheckDURemovability( HKEY hkeyDU, BOOL bSilent=FALSE );
    HRESULT RemoveLegacyControl( LPCTSTR lpszTypeLibID, BOOL bSilent=FALSE );
    HRESULT RemoveDU( LPTSTR szFullName, LPCTSTR lpszTypeLibID, HKEY hkeyDUDB, BOOL bSilent=FALSE );
    HRESULT CheckDUDependencies(HKEY hKeyDUDB, BOOL bSilent=FALSE);
};

///////////////////////////////////////////////////////////////////////////////
// Structure storing information about an ActiveX control.
//
// szName           -- descriptive name of control (eg. "Circle control")
// szFile           -- full filename of the control 
//                     (eg. "C:\WINDOWS\OCCACHE\CIRC3.INF")
// szCLSID          -- CLSID of control, in a string
// szTypeLibID      -- TypeLib ID of the control, in a string
// dwTotalFileSize  -- total size in bytes of all control-related files
// dwTotalSizeSaved -- total size in bytes restored when the control is removed
// cTotalFiles      -- total number of control-related files, including the
//                     control itself
// parseInf         -- pointer to an instance of class CParseInf, which does
//                     all the jobs of parsing the inf file and removing the
//                     control.  Users of this struct should not in anyway
//                     manipulate this pointer.
//
class CCacheItem : public CParseInf
{
public:
    TCHAR     m_szName[LENGTH_NAME];
    TCHAR     m_szFile[MAX_PATH];
    TCHAR     m_szCLSID[MAX_DIST_UNIT_NAME_LEN];
    TCHAR     m_szTypeLibID[MAX_CLSID_LEN];
    TCHAR     m_szCodeBase[INTERNET_MAX_URL_LENGTH];
    TCHAR     m_szVersion[VERSION_MAXSIZE];

    CCacheItem(void) {};
    virtual ~CCacheItem(void) {};

    virtual DWORD ItemType(void) const = 0;
};

class CCacheLegacyControl : public CCacheItem 
{
public:
    CCacheLegacyControl(void) {};
    virtual ~CCacheLegacyControl(void) {};

    static DWORD s_dwType;

    virtual DWORD ItemType(void) const { return s_dwType; };
    virtual HRESULT Init( HKEY hkeyCLSID, LPCTSTR szFile,  LPCTSTR szCLSID );
};

class CCacheDistUnit : public CCacheLegacyControl 
{
public:
    CCacheDistUnit(void) {};
    virtual ~CCacheDistUnit() {};

    static DWORD s_dwType;

    virtual DWORD ItemType(void) const { return s_dwType; };
    virtual HRESULT Init( HKEY hkeyCLSID, LPCTSTR szFile, LPCTSTR szCLSID, HKEY hkeyDist, LPCTSTR szDU );

    // override this - we'll do this work when we DoParseDU
    virtual HRESULT DoParse( LPCTSTR szOCXFileName, LPCTSTR szCLSID ) { return S_OK; };
};

#endif
