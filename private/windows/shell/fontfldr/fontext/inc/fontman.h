/***************************************************************************
 * FontMan.h   -- Definintion for the class: CFontManager
 *
 *
 * Copyright (C) 1992-93 ElseWare Corporation.    All rights reserved.
 ***************************************************************************/

#if !defined(__FONTMAN_H__)
#define __FONTMAN_H__

#if !defined(__FSNOTIFY_H__)
#include "fsnotify.h"
#endif

#include "strtab.h"

// The database search capabilities have been extended.
enum {
    kSearchAny = 1,
    kSearchTT,
    kSearchNotTT
};


//*********************************************************************
// Forward declarations
//
class CFontClass;
class CFontList;
class CFontManager;
struct IPANOSEMapper;

DWORD dwResetFamilyFlags(void);

//*********************************************************************
// Class CFontManager
//
class CFontManager
{
public:
    virtual ~CFontManager( );
    
    BOOL  bInit( );          // Any initialization
    BOOL  bLoadFontList( );  // Build the font list
    
    VOID  vReconcileFolder( int iPriority );
    VOID  vDoReconcileFolder();

    CFontClass *   poAddToList( LPTSTR lpName, LPTSTR lpPath, LPTSTR lpCompFile = NULL );
    CFontList  *   poLockFontList( );
    void           vReleaseFontList( );
    
    void           vGetFamily( CFontClass * poFont, CFontList * poList );
    
    int            iSearchFontList( PTSTR pszTarget, BOOL bExact = TRUE, int iType = kSearchAny );
    int            iSearchFontListLHS( PTSTR pszLHS );
    int            iSearchFontListFile( PTSTR pszFile );
    CFontClass *   poSearchFontList( PTSTR pszTarget, BOOL bExact = TRUE, int iType = kSearchAny );
    CFontClass *   poSearchFontListLHS( PTSTR pszLHS );
    CFontClass *   poSearchFontListFile( PTSTR pszFile );
    
    VOID           vResetFamilyFlags( );
    VOID           vDoResetFamilyFlags( );
    BOOL           bWaitOnFamilyReset( );
    BOOL           bLoadFamList( );
    BOOL           bRefresh( BOOL bCheckDup = TRUE );
    
    void           vToBeRemoved( CFontList * poList );
    BOOL           bCheckTBR( );
    void           vUndoTBR( );
 
    void           vVerifyList( );
    
    void           vDeleteFontList( CFontList * poList, BOOL bDelete = TRUE );
    void           vDeleteFont( CFontClass * pFont,  BOOL bRemoveFile );
    void           vDeleteFontFamily( CFontClass * pFont,  BOOL bRemoveFile );
    
    int            iCompare( CFontClass * pFont1, CFontClass * pFont2, CFontClass * pOrigin );
    USHORT         nDiff( CFontClass * pFont1, CFontClass * pFont2 );
    
    int            GetFontsDirectory( LPTSTR lpDir, int iLen );
    BOOL           ShouldAutoInstallFile( PTSTR pstr, DWORD dwAttrib );
    BOOL           bFamiliesNeverReset(void) { return m_bFamiliesNeverReset; }
    
//
//  Members are obsolete.  Superceded by dwWaitForInstallationMutex() and
//  bReleaseInstallationMutex().
//  See comment in header of CFontManager::iSuspendNotify() for details.
//
//  int            iSuspendNotify( );
//  int            iResumeNotify( );

    //
    // Enumerated return values for dwWaitForInstallationMutex().
    //
    enum           { MUTEXWAIT_SUCCESS,  // Got the mutex.
                     MUTEXWAIT_TIMEOUT,  // Wait timed out.
                     MUTEXWAIT_FAILED,   // Wait failed.
                     MUTEXWAIT_WMQUIT    // Rcvd WM_QUIT while waiting.
                   };

    DWORD          dwWaitForInstallationMutex(DWORD dwTimeout = 2000);
    BOOL           bReleaseInstallationMutex(void);

#ifdef WINNT
    BOOL           CheckForType1FontDriver(void);
    BOOL           Type1FontDriverInstalled(void)
                        { return m_bType1FontDriverInstalled; }
#endif

private: // Methods
    CFontManager();
    VOID           ProcessRegKey( HKEY hk, BOOL bCheckDup );
    VOID           ProcessT1RegKey( HKEY hk, BOOL bCheckDup );
    int            GetSection( LPTSTR lpFile,
                               LPTSTR lpSection,
                               LPHANDLE hSection);
    
    HRESULT        GetPanMapper( IPANOSEMapper ** ppMapper );

    CFontList     *  m_poFontList;
    CFontList     *  m_poTempList;
    IPANOSEMapper *  m_poPanMap;
    BOOL             m_bTriedOnce;   // Set to true if an attempt has been made
                                     // to get at the pan mapper.
    
    BOOL             m_bFamiliesNeverReset; // T = family reset never done yet.
    CFontList     *  m_poRemoveList; // List of fonts being dragged out.
    
    NOTIFYWATCH m_Notify;
    HANDLE      m_hNotifyThread;
//
//  Member is obsolete.  See comment in header of CFontManager::iSuspendNotify()
//
//  int         m_iSuspendNotify;    // The count of suspends
    
    HANDLE      m_hReconcileThread;
    HANDLE      m_hResetFamThread;
    
    HANDLE      m_hEventTerminateThreads;
    HANDLE      m_hEventResetFamily;
    HANDLE      m_hMutexResetFamily;
    HANDLE      m_hMutexInstallation;  // Prevent concurrent installation
                                       // by reconciliation and main threads.
    CRITICAL_SECTION  m_cs;
   
    class HiddenFilesList : public StringTable
    {
        public:
            HiddenFilesList(void) { }
            ~HiddenFilesList(void) { }

            DWORD Initialize(void);

    } m_HiddenFontFilesList;

#ifdef WINNT
    BOOL        m_bType1FontDriverInstalled;
#endif    

friend DWORD dwResetFamilyFlags(LPVOID);
friend DWORD dwNotifyWatchProc(LPVOID);
friend DWORD dwReconcileThread(LPVOID);
friend HRESULT GetOrReleaseFontManager(CFontManager **ppoFontManager, bool bGet);

};

//
// Singleton instance management.
//
HRESULT GetFontManager(CFontManager **ppoFontManager);
void ReleaseFontManager(CFontManager **poFontManager);


#endif // __FONTMAN_H__
