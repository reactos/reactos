//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       purge.h
//
//--------------------------------------------------------------------------

#ifndef __CSCUI_PURGE_H
#define __CSCUI_PURGE_H

//
// Definitions for the dwFlags bits in PurgeCache().
//
#define PURGE_FLAG_NONE      0x00000000
#define PURGE_FLAG_PINNED    0x00000001
#define PURGE_FLAG_UNPINNED  0x00000002
#define PURGE_FLAG_ALL       0x00000003
#define PURGE_IGNORE_ACCESS  0x80000000
//
// Phase identification constants specified in dwPhase argument to PurgeCache
// and returned in PURGECALLBACKINFO.dwPhase.
// 
#define PURGE_PHASE_SCAN     0
#define PURGE_PHASE_DELETE   1

class CCachePurger;  // fwd decl.
class CPath;         // fwd decl.

//
// Purge callback function pointer type.
//
typedef BOOL (CALLBACK * LPFNPURGECALLBACK)(CCachePurger *pPurger);


class CCachePurgerSel
{
    public:
        CCachePurgerSel(void) 
            : m_dwFlags(0),
              m_hdpaShares(DPA_Create(8)),
              m_psidUser(NULL) { };

        ~CCachePurgerSel(void);

        DWORD Flags(void) const
            { return m_dwFlags; }

        BOOL SetUserSid(PSID psid);

        PSID UserSid(void) const
            { return m_psidUser; }

        int ShareCount(void) const
            { return m_hdpaShares ? DPA_GetPtrCount(m_hdpaShares) : 0; }

        LPCTSTR ShareName(int iShare) const
            { return m_hdpaShares ? (LPCTSTR)DPA_GetPtr(m_hdpaShares, iShare) : NULL; }

        DWORD SetFlags(DWORD dwFlags)
            { m_dwFlags = dwFlags; return m_dwFlags; }

        DWORD AddFlags(DWORD dwFlags)
            { m_dwFlags |= dwFlags; return m_dwFlags; }

        BOOL AddShareName(LPCTSTR pszShare);

    private:
        DWORD m_dwFlags;
        HDPA  m_hdpaShares;
        PSID  m_psidUser;

        LPTSTR MyStrDup(LPCTSTR psz);

        //
        // Prevent copy.
        //
        CCachePurgerSel(const CCachePurgerSel& rhs);
        CCachePurgerSel& operator = (const CCachePurgerSel& rhs);
};





class CCachePurger
{
    public:
        CCachePurger(const CCachePurgerSel& desc, 
                     LPFNPURGECALLBACK pfnCbk, 
                     LPVOID pvCbkData);

        ~CCachePurger(void);

        HRESULT Scan(void)
            { return Process(PURGE_PHASE_SCAN); }

        HRESULT Delete(void)
            { return Process(PURGE_PHASE_DELETE); }

        static void AskUserWhatToPurge(HWND hwndParent, CCachePurgerSel *pDesc);

        ULONGLONG BytesToScan(void) const
            { return m_ullBytesToScan; }

        ULONGLONG BytesScanned(void) const
            { return m_ullBytesScanned; }

        ULONGLONG BytesToDelete(void) const
            { return m_ullBytesToDelete; }

        ULONGLONG BytesDeleted(void) const
            { return m_ullBytesDeleted; }

        ULONGLONG FileBytes(void) const
            { return m_ullFileBytes; }

        DWORD Phase(void) const
            { return m_dwPhase; }

        DWORD FilesToScan(void) const
            { return m_cFilesToScan; }

        DWORD FilesToDelete(void) const
            { return m_cFilesToDelete; }

        DWORD FilesScanned(void) const
            { return m_cFilesScanned; }

        DWORD FilesDeleted(void) const
            { return m_cFilesDeleted; }

        DWORD FileOrdinal(void) const
            { return m_iFile; }

        DWORD FileAttributes(void) const
            { return m_dwFileAttributes; }

        DWORD FileDeleteResult(void) const
            { return m_dwResult; }

        LPCTSTR FileName(void) const
            { return m_pszFile; }

        LPVOID CallbackData(void) const
            { return m_pvCbkData; }

        BOOL WillDeleteThisFile(void) const
            { return m_bWillDelete; }

    private:
        //
        // State information to support callback info query functions.
        //
        ULONGLONG m_ullBytesToScan;   // Total bytes to scan.
        ULONGLONG m_ullBytesToDelete; // Total bytes to delete. Known after scanning.
        ULONGLONG m_ullBytesScanned;  // Total bytes scanned.
        ULONGLONG m_ullBytesDeleted;  // Total bytes deleted.
        ULONGLONG m_ullFileBytes;     // Size of this file in bytes.
        DWORD     m_dwPhase;          // PURGE_PHASE_XXXXXX value.
        DWORD     m_cFilesToScan;     // Total files to be scanned.
        DWORD     m_cFilesScanned;    // Total files actually scanned.
        DWORD     m_cFilesToDelete;   // Total files to delete.  Known after scanning.
        DWORD     m_cFilesDeleted;    // Total files actually deleted.
        DWORD     m_iFile;            // "This" file's number [0..(n-1)].
        DWORD     m_dwFileAttributes; // This file's Win32 file attributes.
        DWORD     m_dwResult;         // Win32 result code from CSCDelete()
        HANDLE    m_hmutexPurgeInProg;// Purge-in-progress mutex.
        LPCTSTR   m_pszFile;          // This file's full path.
        LPVOID    m_pvCbkData;        // App data provided in DeleteCacheFiles().
        BOOL      m_bWillDelete;      // 1 == File will be deleted in delete phase.

        DWORD             m_dwFlags;     // PURGE_FLAG_XXXXX flags.
        LPFNPURGECALLBACK m_pfnCbk;      // Ptr to callback function.
        const CCachePurgerSel& m_sel;    // Ref to selection info.
        bool              m_bIsValid;    // Ctor success indicator.
        bool              m_bDelPinned;  // Do dwFlags say delete "pinned"?
        bool              m_bDelUnpinned;// Do dwFlags say delete "non-pinned"?
        bool              m_bUserIsAnAdmin;
        bool              m_bIgnoreAccess;

        HRESULT Process(DWORD dwPhase);
        bool ProcessDirectory(CPath *pstrPath, DWORD dwPhase, bool bShareIsOpen);

        //
        // Prevent copy.
        //
        CCachePurger(const CCachePurger& rhs);
        CCachePurger& operator = (const CCachePurger& rhs);
};


#endif __CSCUI_PURGE_H
