#include "common.h"

#include <emptyvc.h>

#ifndef COMPCLEN_H
#include "compclen.h"
#endif

#include <regstr.h>
#include <olectl.h>
#include <tlhelp32.h>


#ifndef RESOURCE_H
#include "resource.h"
#endif

#ifdef DEBUG
#include <stdio.h>
#endif // DEBUG

#define SZSIZEINBYTES(x)        (lstrlen(x)*sizeof(TCHAR)+1)
#define MYMBOK(x) MessageBox(NULL, x, TEXT("Compression Cleaner"), MB_OK | MB_APPLMODAL)

/*
**--------------------------------------------------------------------------------
** Global variables
**--------------------------------------------------------------------------------
*/
BOOL g_bSettingsChange = FALSE;

// WARNING: Keep the MAX_NOCOM and MAX_NOCOMEXT constants in sync with
// the following string arrays.

TCHAR * g_NoCompressFiles[] = { TEXT("NTLDR"),
                                TEXT("OSLOADER.EXE"),
                                TEXT("PAGEFILE.SYS"),
                                TEXT("NTDETECT.COM"),
                                TEXT("EXPLORER.EXE"),
};

TCHAR * g_NoCompressExts[] = { TEXT("PAL") };

#define MAX_NOCOM    5  // Keep in sync with number of files in NoCompressFiles list
#define MAX_NOCOMEXT 1  // Keep in sync with number of exts in NoCompressExts list

/*
**-----------------------------------------------------------------------------------
** Externally defined global variables
**-----------------------------------------------------------------------------------
*/

extern HINSTANCE g_hDllModule;

/*
**-----------------------------------------------------------------------------------
** External function prototypes
**-----------------------------------------------------------------------------------
*/

extern UINT incDllObjectCount(void);
extern UINT decDllObjectCount(void);
extern UINT incDllLockCount(void);
extern UINT decDllLockCount(void);
extern UINT getDllLockCount(void);
extern UINT setDllLockCount(ULONG);


/*
**------------------------------------------------------------------------------
** CCompCleanerClassFactory::CCompCleanerClassFactory
**
** Purpose:        Constructor
** Parameters:
** Notes;
** Mod Log:        Created by Jason Cobb (2/97)
**                 Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
CCompCleanerClassFactory::CCompCleanerClassFactory()
{
    
    MiDebugMsg((0, TEXT("CCCCF::Constructor")));
    //
    //Nobody is using us yet
    //
    m_cRef = 0L;
    
    //
    //Increment our global count of objects in the DLL
    //
    incDllObjectCount();
}
/*
**------------------------------------------------------------------------------
** CCompCleanerClassFactory::~CCompCleanerClassFactory
**
** Purpose:        Destructor
** Parameters:
** Notes;
** Mod Log:        Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CCompCleanerClassFactory::~CCompCleanerClassFactory()                                                
{
    MiDebugMsg((0, TEXT("CCCCF::Destructor")));
    //
    //One less factory to worry about
    //
    decDllObjectCount();
}

/*
**------------------------------------------------------------------------------
** CCompCleanerClassFactory::QueryInterface
**
** Purpose:        Part of the IUnknown interface
** Parameters:
**        riid  -  interface ID to query on 
**        ppv   -  pointer to interface if we support it
** Notes;
** Mod Log:        Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCompCleanerClassFactory::QueryInterface(
                                                      REFIID          riid,
                                                      LPVOID FAR *ppv
                                                      )
{
    MiDebugMsg((0, TEXT("CCCCF::QueryInterface")));
    *ppv = NULL;
    
    //
    //Is it an interface we support
    //
    if (IsEqualIID(riid, IID_IUnknown))
    {
        //
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (LPUNKNOWN)(LPCLASSFACTORY) this;
        
        AddRef();
        
        return NOERROR;
    }
    
    
    if (IsEqualIID(riid, IID_IClassFactory))
    {
        //
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (LPCLASSFACTORY)this;
        
        AddRef();
        
        return NOERROR;
    }
    
    //
    //Error - We don't support the requested interface
    //
    return E_NOINTERFACE;
}           

/*
**------------------------------------------------------------------------------
** CCompCleanerClassFactory::AddRef
**
** Purpose:        ups the reference count to this object
** Notes;
** Return:         current refernce count
** Mod Log:        Created by Jason Cobb (2/97)
**                         Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP_(ULONG) CCompCleanerClassFactory::AddRef()
{
    MiDebugMsg((0, TEXT("CCCCF::Addref")));
    return ++m_cRef;
}

/*
**------------------------------------------------------------------------------
** CCompCleanerClassFactory::Release
**
** Purpose:        downs the reference count to this object
**                         and deletes the object if no one is using it
** Notes;
** Mod Log:        Created by Jason Cobb (2/97)
**                         Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/

STDMETHODIMP_(ULONG) CCompCleanerClassFactory::Release()
{
    MiDebugMsg((0, TEXT("CCCCF::Release")));
    //
    //Decrement and check
    //
    if (--m_cRef)
        return m_cRef;
    
    //  
    //No references left
    //
    delete this;
    
    return 0L;
}

/*
**------------------------------------------------------------------------------
** CCompCleanerClassFactory::CreateInstance
**
** Purpose:        Creates an instance of the requested interface
** Notes;
** Mod Log:        Created by Jason Cobb (2/97)
**                         Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCompCleanerClassFactory::CreateInstance(
                                                      LPUNKNOWN   pUnkOuter,
                                                      REFIID          riid,
                                                      LPVOID *        ppvObj
                                                      )
{
    MiDebugMsg((0, TEXT("CCCCF::CreateInstance")));
    
    *ppvObj = NULL;
    
    //
    //Shell extensions typically don't support aggregation (inheritance)
    //
    if (pUnkOuter)
    {
        //
        //Error - we don't support aggregation
        //
        return ResultFromScode (CLASS_E_NOAGGREGATION);
    }
    
    
    //
    //Create an instance of the 'Comp Cleaner' object
    //
    LPCCOMPCLEANER pCompCleaner = new CCompCleaner();  
    if (NULL == pCompCleaner)
    {
        //Error - not enough memory
        return ResultFromScode (E_OUTOFMEMORY);
    }
    
    //
    //Make sure the new object likes the requested interface
    //
    HRESULT hr = pCompCleaner->QueryInterface (riid, ppvObj);
    if (FAILED(hr))
        delete pCompCleaner;
    
    MiDebugMsg((0, TEXT("CCCCF::CreateInstance")));
    return hr;          
}

/*
**------------------------------------------------------------------------------
** CCompCleanerClassFactory::LockServer
**
** Purpose:        Locks or unlocks the server
** Notes;          Increments/Decrements the DLL lock count
**                         so we know when we can safely remove the DLL
** Mod Log:        Created by Jason Cobb (2/97)
**                         Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCompCleanerClassFactory::LockServer(BOOL fLock)
{
    MiDebugMsg((0, TEXT("CCCCF::LockServer")));
    if (fLock)
        incDllLockCount ();
    else
        decDllLockCount ();
    
    return NOERROR;
}


/*
**------------------------------------------------------------------------------
** CCompCleaner class method definitions
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** CCompCleaner::CCompCleaner
**
** Purpose:        Default constructor
** Notes:
** Mod Log:        Created by Jason Cobb (2/97)
**                         Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
CCompCleaner::CCompCleaner()
{
    //
    //Set to default values
    //
    m_cRef                 = 0L;
    m_lpdObject        = NULL;
    
    cbSpaceUsed.QuadPart = 0;
    cbSpaceFreed.QuadPart = 0;
    szVolume[0] = (TCHAR)NULL;
    szFolder[0] = (TCHAR)NULL;
    filelist[0] = (TCHAR)NULL;
    dwDaysForCurrentLAD = 0;
    dwDaysForCurrentList = 0;
    
    head = NULL;
    
    //
    //increment global object count
    //
    incDllObjectCount();
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::~CCompCleaner
**
** Purpose:        Destructor
** Notes:
** Mod Log:        Created by Jason Cobb (2/97)
**                         Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
CCompCleaner::~CCompCleaner()
{
    MiDebugMsg((0, TEXT("CCompCleaner::Destructor")));
    //
    //Cleanup up associated IDataObject interface
    //
    if (m_lpdObject != NULL)
        m_lpdObject->Release();
    
    m_lpdObject = NULL;
    
    //
    //Free the list of files
    //
    FreeList(head);
    head = NULL;
    dwDaysForCurrentList = 0;
    
    //
    //Decrement global object count
    //
    decDllObjectCount();
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::QueryInterface
**
** Purpose:        Part of the IUnknown interface
** Parameters:
**        riid  -  interface ID to query on 
**        ppv   -  pointer to interface if we support it
** Return:         NOERROR on success, E_NOINTERFACE otherwise
** Notes;
** Mod Log:        Created by Jason Cobb (2/97)
**                         Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCompCleaner::QueryInterface(
                                          REFIID          riid, 
                                          LPVOID FAR *ppv
                                          )
{
    MiDebugMsg((0, TEXT("CCompCleaner::QueryInterface")));
    *ppv = NULL;
    
    //
    //Check for IUnknown interface request
    //
    if (IsEqualIID (riid, IID_IUnknown))
    {
        // 
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (LPUNKNOWN) this;
        AddRef();
        return NOERROR;
    }  
    
    //
    //Check for IEmptyVolumeCache interface request
    //
    if (IsEqualIID (riid, IID_IEmptyVolumeCache))
    {
        // 
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (IEmptyVolumeCache*) this;
        AddRef();
        return NOERROR;
    }  
    
    //
    //Check for IEmptyVolumeCache2 interface request
    //
    if (IsEqualIID (riid, IID_IEmptyVolumeCache2))
    {
        // 
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (IEmptyVolumeCache2*) this;
        AddRef();
        return NOERROR;
    }  
    
    //
    //Error - unsupported interface requested
    //
    return E_NOINTERFACE;
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::AddRef
**
** Purpose:        ups the reference count to this object
** Notes;
** Return:         current refernce count
** Mod Log:        Created by Jason Cobb (2/97)
**                         Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP_(ULONG) CCompCleaner::AddRef()
{
    MiDebugMsg((0, TEXT("CCompCleaner::AddRef Ref is %d"), (m_cRef+1)));
    
    return ++m_cRef;
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::Release
**
** Purpose:        downs the reference count to this object
**                         and deletes the object if no one is using it
** Notes;
** Mod Log:        Created by Jason Cobb (2/97)
**                         Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP_(ULONG) CCompCleaner::Release()
{
    MiDebugMsg((0, TEXT("CCompCleaner::Release Ref is %d"), (m_cRef-1)));
    
    //  
    //Decrement and check
    //
    if (--m_cRef)
        return m_cRef;
    
    //
    //No references left to this object
    //
    delete this;
    
    return 0L;
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::Initialize
**
** Purpose:         Initializes the Compression Cleaner and returns the 
**                          specified IEmptyVolumeCache2 flags to the cache manager.
** Notes;
** Mod Log:         Created by Jason Cobb (2/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCompCleaner::Initialize(
                                      HKEY        hRegKey,
                                      LPCWSTR pszVolume,
                                      LPWSTR  *ppwszDisplayName,
                                      LPWSTR  *ppwszDescription,
                                      DWORD   *pdwFlags
                                      )
{
    TCHAR   acsVolume[MAX_PATH];
    TCHAR   szFileSystemName[MAX_PATH];
    DWORD   fFileSystemFlags;
    OSVERSIONINFO osver;
    
    MiDebugMsg((0, TEXT("CCompCleaner::Initialize")));
    
    //
    // This cleaner only runs under WinNT.  If the platform isn't
    // NT then bail out.
    //
    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
    GetVersionEx(&osver);
    
    if (VER_PLATFORM_WIN32_NT != osver.dwPlatformId)
    {
        MiDebugMsg((0, TEXT("CCompCleaner::Initialize platform is not NT")));
        return S_FALSE;
    }
    else
    {
        MiDebugMsg((0, TEXT("CCompCleaner::Initialize platform is Windows NT")));
    }
    
    bPurged = FALSE;
    
    //
    // Allocate memory for the DisplayName string and load the string.
    // If the allocation fails, then we will return NULL which will cause
    // cleanmgr.exe to read the name from the registry.
    //
    if (*ppwszDisplayName = (LPWSTR)CoTaskMemAlloc(DISPLAYNAME_LENGTH * sizeof(WCHAR)))
    {
#ifndef UNICODE
        CHAR szDisplayA[DISPLAYNAME_LENGTH];
        LoadString(g_hDllModule, IDS_COMPCLEANER_DISP, szDisplayA, DISPLAYNAME_LENGTH);
        MultiByteToWideChar(CP_ACP, 0, szDisplayA, -1, *ppwszDisplayName, DISPLAYNAME_LENGTH);
#else
        LoadString(g_hDllModule, IDS_COMPCLEANER_DISP, *ppwszDisplayName, DISPLAYNAME_LENGTH);
#endif
    }
    
    //
    // Allocate memory for the Description string and load the string.
    // If the allocation fails, then we will return NULL which will cause
    // cleanmgr.exe to read the description from the registry.
    //
    if (*ppwszDescription = (LPWSTR)CoTaskMemAlloc(DESCRIPTION_LENGTH * sizeof(WCHAR)))
    {
#ifndef UNICODE
        CHAR szDescA[DESCRIPTION_LENGTH];
        LoadString(g_hDllModule, IDS_COMPCLEANER_DESC, szDescA, DESCRIPTION_LENGTH);
        MultiByteToWideChar(CP_ACP, 0, szDescA, -1, *ppwszDescription, DESCRIPTION_LENGTH);
#else
        LoadString(g_hDllModule, IDS_COMPCLEANER_DESC, *ppwszDescription, DESCRIPTION_LENGTH);
#endif
    }
    
    //
    //If you want your cleaner to run only when the machine is dangerously low on
    //disk space then return S_FALSE unless the EVCF_OUTOFDISKSPACE flag is set.
    //
    //if (!(*pdwFlags & EVCF_OUTOFDISKSPACE))
    //{
    //          return S_FALSE;
    //}
    
    if (*pdwFlags & EVCF_SETTINGSMODE)
    {
        MiDebugMsg((0, TEXT("CCompCleaner::Initialize: Settings mode")));
        bSettingsMode = TRUE;
    }
    else bSettingsMode = FALSE;
    
    //
    //Tell the cache manager to disable this item by default
    //
    *pdwFlags = 0;
    
    //
    //Tell the Disk Cleanup Manager that we have a Settings button
    //
    *pdwFlags |= EVCF_HASSETTINGS;
    
    //
    //Tell the Disk Cleanup Manager to only show this cleaner if it has some files
    //to cleanup.
    //
    // *pdwFlags |= EVCF_DONTSHOWIFZERO;
    
    
    // 
    // If we're in Settings mode no need to do all this other work
    //
    if (bSettingsMode) return S_OK;
    
    
    ftMinLastAccessTime.dwLowDateTime = 0;
    ftMinLastAccessTime.dwHighDateTime = 0;
    
    
#ifndef UNICODE
    
    //
    //Convert the volume name to ANSI
    //
    
    WideCharToMultiByte(CP_ACP, 0, pszVolume, MAX_PATH, acsVolume,
        MAX_PATH, NULL, NULL);
    
#else
    lstrcpy(acsVolume, (LPTSTR)pszVolume);
#endif // UNICODE
    
    lstrcpy(filelist, TEXT("*.*\0\0"));
    
    MiDebugMsg((0, TEXT("CCompCleaner::Initialize acsVolume is %s"), acsVolume));
    
    if (!GetVolumeInformation(acsVolume,
        NULL,
        0,
        NULL,
        NULL,
        &fFileSystemFlags,
        szFileSystemName,
        MAX_PATH))
    {
        
        //
        // An error occurred getting the volume information.
        //
        MiDebugMsg((0, TEXT("CCompCleaner::Initialize -- Unable to get volume info.")));
        return S_FALSE;
    }
    
    if (lstrcmp(szFileSystemName, TEXT("NTFS")))
        
    {
        
        //
        // This is not an NTFS volume so this cleaner won't work.
        //
        MiDebugMsg((0, TEXT("CCompCleaner::Initialize -- not an NTFS volume")));
        return S_FALSE;
    }
    
    
    if  (!(fFileSystemFlags & FS_FILE_COMPRESSION))
    {
        //
        // Strange... an NTFS volume that doesn't support file compression?
        // This cleaner can't work on volumes that don't support comp.
        //
        MiDebugMsg((0, TEXT("CCompCleaner::Initialize -- volume does not support compression")));
        return S_FALSE;
    }
    
    //
    //Fill in the folder that we want to start the cleaning from.
    //
    
    lstrcpy(szFolder, acsVolume);
    
    //
    // Calculate the last access date filetime
    //
    CalcLADFileTime();
    
    return S_OK;
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::InitializeEx
**
** Purpose:         Initializes the Compression Cleaner and returns the 
**                          specified IEmptyVolumeCache flags to the cache manager.
** Notes:
** Mod Log:         Created by David Schott (9/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCompCleaner::InitializeEx(
                                        HKEY hRegKey,
                                        LPCWSTR pcwszVolume,
                                        LPCWSTR pcwszKeyName,
                                        LPWSTR *ppwszDisplayName,
                                        LPWSTR *ppwszDescription,
                                        LPWSTR *ppwszBtnText,
                                        DWORD *pdwFlags
                                        )
{
    
    MiDebugMsg((0, TEXT("CCompCleaner::InitializeEx")));
    
    //
    // Allocate memory for the ButtonText string and load the string.
    // If we can't allocate the memory, leave the pointer NULL.
    //
    if (*ppwszBtnText = (LPWSTR)CoTaskMemAlloc(BUTTONTEXT_LENGTH * sizeof(WCHAR)))
    {
#ifndef UNICODE
        CHAR szButtonA[BUTTONTEXT_LENGTH];
        LoadString(g_hDllModule, IDS_COMPCLEANER_BUTTON, szButtonA, BUTTONTEXT_LENGTH);
        MultiByteToWideChar(CP_ACP, 0, szButtonA, -1, *ppwszBtnText, BUTTONTEXT_LENGTH);
#else
        LoadString(g_hDllModule, IDS_COMPCLEANER_BUTTON, *ppwszBtnText, BUTTONTEXT_LENGTH);
#endif
    }
    
    //
    // Now let the IEmptyVolumeCache version 1 Init function do the rest
    //
    return(Initialize(hRegKey,
        pcwszVolume,
        ppwszDisplayName,
        ppwszDescription,
        pdwFlags));
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::GetSpaceUsed
**
** Purpose:         Returns the total amount of space that the compression cleaner
**                          can free.
** Notes;
** Mod Log:         Created by Jason Cobb (2/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCompCleaner::GetSpaceUsed(
                                        DWORDLONG                   *pdwSpaceUsed,
                                        IEmptyVolumeCacheCallBack   *picb
                                        )
{
    DWORD i;
    
    MiDebugMsg((0, TEXT("CCompCleaner::GetSpaceUsed")));
    
    cbSpaceUsed.QuadPart  = 0L;
    
    //
    // If the list is already built, free it before refreshing it.
    //
    if (head)
    {
        FreeList(head);
        head = NULL;
        dwDaysForCurrentList = 0;
    }
    BuildList(picb);
    
    picb->ScanProgress(cbSpaceUsed.QuadPart, EVCCBF_LASTNOTIFICATION, NULL);
    
    *pdwSpaceUsed = cbSpaceUsed.QuadPart;
    
    return S_OK;
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::Purge
**
** Purpose:         Purges (deletes) all of the files specified in the "filelist"
**                          portion of the registry.
** Notes;
** Mod Log:         Created by Jason Cobb (2/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCompCleaner::Purge(
                                 DWORDLONG                  dwSpaceToFree,
                                 IEmptyVolumeCacheCallBack  *picb)
                                 
{
    MiDebugMsg((0, TEXT("CCompCleaner::Purge")));
    
    bPurged = TRUE;
    
    //
    // If the Days for the current list is out of sync with
    // the Days for the current LAD, we need to rebuild the
    // the file list with the current setting.
    //
    if (dwDaysForCurrentList != dwDaysForCurrentLAD)
    {
        if (head)
        {
            MiDebugMsg((0, TEXT("CCompCleaner::Purge Current list out of sync, rebuilding")));
            FreeList(head);
            head = NULL;
            dwDaysForCurrentList = 0;
        }
        BuildList(NULL);
    }
    //
    //Delete the files
    //
    PurgeFiles(picb, TRUE);
    
    //
    //Send the last notification to the cleanup manager
    //
    picb->PurgeProgress(cbSpaceFreed.QuadPart,
        (cbSpaceUsed.QuadPart - cbSpaceFreed.QuadPart),
        EVCCBF_LASTNOTIFICATION, NULL);
    
    //
    // Free the list of files and invalidate the dwDaysForCurrentList
    // setting
    //
    FreeList(head);
    head = NULL;
    dwDaysForCurrentList = 0;
    
    return S_OK;
}

/*
**------------------------------------------------------------------------------
** ViewFiles
**
** Purpose:         Dialog box that displays all of the files that will be
**                          compressed by this cleaner.
**
** Mod Log:         Created by Jason Cobb (6/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
**
** NOTE:  Per the specification for the Compression Cleaner we are not
**                providing the view files functionality.  However, I will leave
**                the framework in place just in case we want to use it.
**
**------------------------------------------------------------------------------
*/
INT_PTR CALLBACK 
ViewFilesDlgProc(
                 HWND hDlg, 
                 UINT Msg, 
                 WPARAM wParam, 
                 LPARAM lParam
                 )
{
    HWND hwndList;
    LV_ITEM lviItem;
    PCLEANFILESTRUCT pCleanFile;
    
    switch(Msg) {
        
    case WM_INITDIALOG:
        hwndList = GetDlgItem(hDlg, IDC_COMP_LIST);
        pCleanFile = (PCLEANFILESTRUCT)lParam;
        
        ListView_DeleteAllItems(hwndList);
        
        while(pCleanFile) {
            
            lviItem.mask = LVIF_TEXT | LVIF_IMAGE;
            lviItem.iSubItem = 0;
            lviItem.iItem = 0;
            
            //
            //Only show files
            //
            if (!pCleanFile->bDirectory) {
                
                lviItem.pszText = pCleanFile->file;
                ListView_InsertItem(hwndList, &lviItem);
            }
            
            pCleanFile = pCleanFile->pNext;
            lviItem.iItem++;
        }
        
        break;
        
    case WM_COMMAND:
        
        switch (LOWORD(wParam)) {
            
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;
        }
        break;
        
    default:
        return FALSE;
    }
    
    return TRUE;
}

/*
**------------------------------------------------------------------------------
** Settings
**
** Purpose:         Dialog box that displays the settings for this cleaner.
**
** Mod Log:         Created by David Schott (6/98)
**
**------------------------------------------------------------------------------
*/
INT_PTR CALLBACK 
SettingsDlgProc(
                HWND hDlg, 
                UINT Msg, 
                WPARAM wParam, 
                LPARAM lParam
                )
{
    HKEY hCompClenReg;                          // Handle to our registry path
    DWORD dwDisposition;                        // Foo crap for the reg calls
    DWORD dwByteCount;                          // Ditto
    DWORD dwNumDays = DEFAULT_DAYS; // Number of days setting from registry
    static UINT DaysIn;                         // Number of days initial setting
    UINT DaysOut;                                   // Number of days final setting
#ifdef DEBUG
    static PCLEANFILESTRUCT pCleanFile; // Pointer to our file list
#endif // DEBUG
    
    switch(Msg) {
        
    case WM_INITDIALOG:
        
#ifdef DEBUG
        pCleanFile = (PCLEANFILESTRUCT)lParam;
#endif // DEBUG
        //
        // Set the range for the Days spin control (1 to 500)
        //
        SendDlgItemMessage(hDlg, IDC_COMP_SPIN, UDM_SETRANGE, 0, (LPARAM) MAKELONG ((short) MAX_DAYS, (short) MIN_DAYS));
        
        //
        // Get the current user setting for # days and init
        // the spin control edit box
        //
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
            COMPCLN_REGPATH,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hCompClenReg,
            &dwDisposition))
        {
            dwByteCount = sizeof(dwNumDays);
            
            if (ERROR_SUCCESS == RegQueryValueEx(hCompClenReg,
                TEXT("Days"),
                NULL,
                NULL,
                (LPBYTE) &dwNumDays,
                &dwByteCount))
            {
                //
                // Got day count from registry, make sure it's
                // not too big or too small.
                //
                if (dwNumDays > MAX_DAYS) dwNumDays = MAX_DAYS;
                if (dwNumDays < MIN_DAYS) dwNumDays = MIN_DAYS;
                
                SetDlgItemInt(hDlg, IDC_COMP_EDIT, dwNumDays, FALSE);
            }
            else
            {
                //
                // Failed to get the day count from the registry
                // so just use the default.
                //
                
                SetDlgItemInt(hDlg, IDC_COMP_EDIT, DEFAULT_DAYS, FALSE);
            }
        }
        else
        {
            //
            // Failed to get the day count from the registry
            // so just use the default.
            //
            
            SetDlgItemInt(hDlg, IDC_COMP_EDIT, DEFAULT_DAYS, FALSE);
        }
        
        RegCloseKey(hCompClenReg);
        
        // Track the initial setting so we can figure out
        // if the user has changed the setting on the way
        // out.
        
        DaysIn = GetDlgItemInt(hDlg, IDC_COMP_EDIT, NULL, FALSE);
        
        break;
        
    case WM_COMMAND:
        
        switch (LOWORD(wParam)) {
            
            
#ifdef DEBUG
        case IDC_VIEW:
            DialogBoxParam(g_hDllModule, MAKEINTRESOURCE(IDD_COMP_VIEW), hDlg, ViewFilesDlgProc, (LPARAM)pCleanFile);
            break;
#endif // DEBUG
            
        case IDOK:
            
            //
            // Get the current spin control value and write the
            // setting to the registry.
            //
            
            DaysOut = GetDlgItemInt(hDlg, IDC_COMP_EDIT, NULL, FALSE);
            
            if (DaysOut > MAX_DAYS) DaysOut = MAX_DAYS;
            if (DaysOut < MIN_DAYS) DaysOut = MIN_DAYS;
            
            
            if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                COMPCLN_REGPATH,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hCompClenReg,
                &dwDisposition))
            {
                dwNumDays = (DWORD)DaysOut;
                RegSetValueEx(hCompClenReg,
                    TEXT("Days"),
                    0,
                    REG_DWORD,
                    (LPBYTE) &dwNumDays,
                    sizeof(dwNumDays));
                
                RegCloseKey(hCompClenReg);
            }
            
            // Don't care if this failed -- what can we
            // do about it anyway...
            
            // If the user has changed the setting we need
            // to recalculate the list of files.
            
            if (DaysIn != DaysOut)
            {
                g_bSettingsChange = TRUE;   
            }
            
            // Fall thru to IDCANCEL
            
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;
        }
        break;
        
    default:
        return FALSE;
    }
    
    return TRUE;
}


/*
**------------------------------------------------------------------------------
** CCompCleaner::ShowProperties
**
** Purpose:         Displays all of the files found by the cleaner
**
** Mod Log:         Created by Jason Cobb (6/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCompCleaner::ShowProperties(HWND hwnd)
                                          
{
    MiDebugMsg((0, TEXT("CCompCleaner::ShowProperties")));
    
    g_bSettingsChange = FALSE;
    
    DialogBoxParam(g_hDllModule, MAKEINTRESOURCE(IDD_COMP_SETTINGS), hwnd, SettingsDlgProc, (LPARAM)head);
    
    //
    // If the settings have changed we need to recalculate the
    // LAD Filetime.
    //
    if (g_bSettingsChange)
    {
        CalcLADFileTime();
        return S_OK;                // Tell CleanMgr that settings have changed.
    }
    else
    {
        return S_FALSE;         // Tell CleanMgr no settings changed.
    }
    
    return S_OK; // Shouldn't hit this but just in case.
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::Deactivate
**
** Purpose:         Deactivates the cleaner...this basically 
**                          does nothing.
** Notes;
** Mod Log:         Created by Jason Cobb (2/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCompCleaner::Deactivate(DWORD   *pdwFlags)
{
    MiDebugMsg((0, TEXT("CCompCleaner::Deactivate")));
    
    *pdwFlags = 0;
    
    //
    //If the cleaner you are developing needs to only be run one time then set
    //the EVCF_REMOVEFROMLIST cleaner. 
    //
    //*pdwFlags |= EVCF_REMOVEFROMLIST;
    
    return S_OK;
}

/*
**------------------------------------------------------------------------------
** bLastAccessisOK
**
** Purpose:         This function checks if the file is a specified number of days
**                                                          old. If the file has not been accessed in the
**                                                          specified number of days then it can safely be
**                                                          deleted.  If the file has been accessed in that
**                                                          number of days then the file will not be deleted.
**
** Notes;
** Mod Log:         Created by Jason Cobb (7/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
BOOL
CCompCleaner::bLastAccessisOK(FILETIME ftFileLastAccess)
{
    
    //Is the last access FILETIME for this file less than the current
    //FILETIME minus the number of specified days?
    if (CompareFileTime(&ftFileLastAccess, &ftMinLastAccessTime) == -1)
        return TRUE;
    
    else
        return FALSE;
}

/*
**------------------------------------------------------------------------------
** bIsDontCompressFile
**
** Purpose:         This function checks if the file is in the g_NoCompressFiles
**                                                          file list.  If it is, returns TRUE, else FALSE.
**
** Notes;
** Mod Log:         Created by David Schott (8/98)
**------------------------------------------------------------------------------
*/
BOOL
bIsDontCompressFile(LPTSTR lpFullPath)
{
    DWORD i;
    LPTSTR lpFile;
    LPTSTR lpExt;
    LPTSTR lpCurrent;
    
    //MiDebugMsg((0, TEXT("CCompCleaner::bIsDontCompressFile: %s"), lpFullPath ));
    
    // Take the easy out
    if (!lpFullPath) return FALSE;
    
    
    // Get pointers to just the file and just the extension.
    lpFile = NULL;
    lpExt = NULL;
    
    // Start at the end of lpFullPath and work backwards
    lpCurrent = CharPrev(lpFullPath, lpFullPath + lstrlen(lpFullPath));
    
    while ((lpCurrent > lpFullPath) && (!lpFile))
    {
        if ((*lpCurrent == TEXT('.')) && (!lpExt)) lpExt = CharNext(lpCurrent);
        if ((*lpCurrent == TEXT('\\')) || (*lpCurrent == TEXT(':'))) lpFile = CharNext(lpCurrent);
        lpCurrent = CharPrev(lpFullPath, lpCurrent);
    }
    
    // Did we end that while loop because we found the filename or because
    // we hit the beginning of lpFullPath?  If we hit the beginning of lpFP
    // then lpFullPath must be just a filename -- no path info.
    if (!lpFile) lpFile = lpFullPath;
    
    // See if this file is in the g_NoCompressFiles list.
    // 
    
    for (i = 0; i < MAX_NOCOM; i++)
    {
        if (!lstrcmpi(lpFile, g_NoCompressFiles[i]))
        {
            MiDebugMsg((0, TEXT("File is in No Compress list: %s"), lpFile));
            return TRUE;
        }
    }
    
    // Ok, we made it through all the no compress files and our current
    // file ain't one of them.  Does our current file have a no compress
    // extension?
    
    if (lpExt)
    {
        for (i = 0; i < MAX_NOCOMEXT; i++)
        {
            if (!lstrcmpi(lpExt, g_NoCompressExts[i]))
            {
                MiDebugMsg((0, TEXT("File has No Compress extension: %s"), lpFile));
                return TRUE;
            }
        }
    }
    
    // If we made it here the file must be OK to compress.
    return FALSE;
}


/*
**------------------------------------------------------------------------------
** bIsFileOpen
**
** Purpose:     This function checks if a file is open by doing a CreateFile
**                              with fdwShareMode of 0.  If GetLastError() retuns
**                              ERROR_SHARING_VIOLATION then this function retuns TRUE because
**                              someone has the file open.  Otherwise this function retuns false.
**
** Notes;
** Mod Log:     Created by Jason Cobb (7/97)
**              Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
BOOL
bIsFileOpen(
            LPTSTR lpFile,
            LPFILETIME lpftFileLastAccess)
{
    HANDLE hFile = NULL;
    DWORD  dwResult = 0;
    DWORD  dwAttributes;
    BOOL   bFileWasRO;
    
    //MiDebugMsg((0, TEXT("CCompCleaner::bIsFileOpen: %s"), lpFile ));
    
    // Need to see if we can open file with WRITE access -- if we
    // can't we can't compress it.  Of course if the file has R/O
    // attribute then we won't be able to open for WRITE.  So,
    // we need to remove the R/O attribute long enough to try
    // opening the file then restore the original attributes.
    
    bFileWasRO = FALSE;
    dwAttributes = GetFileAttributes(lpFile);
    
    if ((0xFFFFFFFF != dwAttributes) && (dwAttributes & FILE_ATTRIBUTE_READONLY))
    {
        bFileWasRO = TRUE;
        SetFileAttributes(lpFile, FILE_ATTRIBUTE_NORMAL);
    }
    
    SetLastError(0);
    
    hFile = CreateFile(lpFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwResult = GetLastError();
        
        if ((ERROR_SHARING_VIOLATION == dwResult) || (ERROR_ACCESS_DENIED == dwResult))
        {
            //
            //File is currently open by someone or not accessible to this user.
            //
            MiDebugMsg((0, TEXT("CCompCleaner::bIsFileOpen error opening: %s"), lpFile ));
            
            if (bFileWasRO) SetFileAttributes(lpFile, dwAttributes);
            return TRUE;
        }
    }
    
    //
    //File is not currently open
    //
    SetFileTime(hFile, NULL, lpftFileLastAccess, NULL);
    CloseHandle(hFile);
    if (bFileWasRO) SetFileAttributes(lpFile, dwAttributes);
    return FALSE;
}

/*
**------------------------------------------------------------------------------
** BuildList
**
** Purpose:     This function provides a common entry point for
**              building the linked list of files that will be
**              compressed by this cleaner.
**
** Notes;
** Mod Log:     Created by David Schott (7/98)
**
**------------------------------------------------------------------------------
*/
void CCompCleaner::BuildList(IEmptyVolumeCacheCallBack *picb)
{
    LPTSTR          lpSingleFolder;
    
    MiDebugMsg((0, TEXT("CCompCleaner::BuildList")));
    
    cbSpaceUsed.QuadPart = 0;
    
    //
    // Set the Days count for this list so we'll know later on
    // if the list is out of sync with the current Days setting
    // in the registry (hence, the need to build the list again).
    //
    dwDaysForCurrentList = dwDaysForCurrentLAD;
    
    //
    //Walk all of the folders in the folders list scanning for disk space.
    //
    for (lpSingleFolder = &(szFolder[0]); *lpSingleFolder; lpSingleFolder += lstrlen(lpSingleFolder) + 1)
        WalkForUsedSpace(lpSingleFolder, picb);
    
    return;
}

/*
**------------------------------------------------------------------------------
** CalcLADFileTime
**
** Purpose:     This function gets the current last access days
**              setting from the registry and calculates the magic
**              filetime we're looking for when searching for files
**              to compress.
**
** Notes;
** Mod Log:     Created by David Schott (7/98)
**
**------------------------------------------------------------------------------
*/
void CCompCleaner::CalcLADFileTime()
{
    HKEY hCompClenReg = NULL;     // Handle to our registry path
    DWORD dwDisposition;          // Foo crap for the reg calls
    DWORD dwByteCount;            // Ditto
    DWORD dwDaysLastAccessed = 0; // Day count from the registry setting
    
    MiDebugMsg((0, TEXT("CCompCleaner::CalcLADFileTime")));
    
    //
    // Get the DaysLastAccessed value from the registry.
    //
    
    dwDaysLastAccessed = DEFAULT_DAYS;
    
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
        COMPCLN_REGPATH,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hCompClenReg,
        &dwDisposition))
    {
        dwByteCount = sizeof(dwDaysLastAccessed);
        
        RegQueryValueEx(hCompClenReg,
            TEXT("Days"),
            NULL,
            NULL,
            (LPBYTE) &dwDaysLastAccessed,
            &dwByteCount);
        
        RegCloseKey(hCompClenReg);
    }
    
    //
    // Verify LD setting is within range
    //
    if (dwDaysLastAccessed > MAX_DAYS) dwDaysLastAccessed = MAX_DAYS;
    if (dwDaysLastAccessed < MIN_DAYS) dwDaysLastAccessed = MIN_DAYS;
    
    //
    //Determine the LastAccessedTime 
    //
    if (dwDaysLastAccessed != 0)
    {
        ULARGE_INTEGER  ulTemp, ulLastAccessTime;
        FILETIME        ft;
        
        //Determine the number of days in 100ns units
        ulTemp.LowPart = FILETIME_HOUR_LOW;
        ulTemp.HighPart = FILETIME_HOUR_HIGH;
        
        ulTemp.QuadPart *= dwDaysLastAccessed;
        
        //Get the current FILETIME
        GetSystemTimeAsFileTime(&ft);
        ulLastAccessTime.LowPart = ft.dwLowDateTime;
        ulLastAccessTime.HighPart = ft.dwHighDateTime;
        
        //Subtract the Last Access number of days (in 100ns units) from 
        //the current system time.
        ulLastAccessTime.QuadPart -= ulTemp.QuadPart;
        
        //Save this minimal Last Access time in the FILETIME member variable
        //ftMinLastAccessTime.
        ftMinLastAccessTime.dwLowDateTime = ulLastAccessTime.LowPart;
        ftMinLastAccessTime.dwHighDateTime = ulLastAccessTime.HighPart;
        
        //Set the dwDaysForCurrentLAD to reflect this new filetime setting
        dwDaysForCurrentLAD = dwDaysLastAccessed;
    }
    
    return;
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::WalkForUsedSpace
**
** Purpose:     This function will walk the specified directory and create a 
**                              linked list of files that can be deleted.  It will also
**                              increment the member variable to indicate how much disk space
**                              these files are taking.
**                              It will look at the dwFlags member variable to determine if it
**                              needs to recursively walk the tree or not.
** Notes;
** Mod Log:     Created by Jason Cobb (2/97)
**              Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
BOOL
CCompCleaner::WalkForUsedSpace( PTCHAR  lpPath,
                               IEmptyVolumeCacheCallBack *picb)
{
    BOOL                    bRet = TRUE;
    BOOL                    bFind = TRUE;
    HANDLE                  hFind;
    WIN32_FIND_DATA         wd;
    DWORD                   dwAttributes;
    TCHAR                   szFindPath[MAX_PATH];
    TCHAR                   szAddFile[MAX_PATH];
    ULARGE_INTEGER          dwFileSize;
    static DWORD            dwCount = 0;
    LPTSTR                  lpSingleFile;
    
    //MiDebugMsg((0, TEXT("CCompCleaner::WalkForUsedSpace IN")));
    //
    //If this is a directory then tack a *.* onto the end of the path
    //and recurse through the rest of the directories
    //
    dwAttributes = GetFileAttributes(lpPath);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        //
        //Enum through the MULTI_SZ filelist
        //
        
        for (lpSingleFile = &(filelist[0]); *lpSingleFile; lpSingleFile += lstrlen(lpSingleFile) + 1)
        {
            lstrcpy(szFindPath, lpPath);
            if ( !PathAppend(szFindPath, lpSingleFile) )
            {
                // Failure here means the file name is too long, just ignore that file
                continue;
            }

            bFind = TRUE;
            hFind = FindFirstFile(szFindPath, &wd);
            while (hFind != INVALID_HANDLE_VALUE && bFind)
            {
                lstrcpy(szAddFile, lpPath);
                if ( !PathAppend(szAddFile, wd.cFileName) )
                {
                    // Failure here means the file name is too long, just ignore that file
                    continue;
                }
                
                /*
                #ifdef DEBUG
                
                MiDebugMsg((0, TEXT("File %s"), szAddFile ));

                if (wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    MiDebugMsg((0, TEXT("File is a directory")));
                }
                else
                {
                    if (bIsFileOpen(szAddFile, &wd.ftLastAccessTime))
                    {
                        MiDebugMsg((0, TEXT("File is OPEN")));
                    }

                    if (!bLastAccessisOK(wd.ftLastAccessTime))
                    {
                        MiDebugMsg((0, TEXT("File accessed too recently")));
                    }

                    if (wd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
                    {
                        MiDebugMsg((0, TEXT("File already compressed")));
                    }

                    if (wd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
                    {
                        MiDebugMsg((0, TEXT("File is encrypted")));
                    }

                    if (wd.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)
                    {
                        MiDebugMsg((0, TEXT("File is HSM archived")));
                    }

                    if (bIsDontCompressFile(szAddFile))
                    {
                        MiDebugMsg((0, TEXT("File is in NoCompress list")));
                    }
                }
                #endif // DEBUG
                */  
                
                //
                //Check if this is a subdirectory.
                //
                if (wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    dwFileSize.HighPart = 0;
                    dwFileSize.LowPart = 0;
                    AddFileToList(szAddFile, dwFileSize, TRUE);
                }
                //
                //Else this is a file so check if it is open or if it
                //is already compressed, encrypted, HSM archived, or
                //one of those files we ain't supposed to compress...
                //
                else if ((bIsFileOpen(szAddFile, &wd.ftLastAccessTime) == FALSE) &&
                    (!(wd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)) &&
                    (!(wd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) &&
                    (!(wd.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)) &&
                    (bLastAccessisOK(wd.ftLastAccessTime)) &&
                    (!bIsDontCompressFile(szAddFile)))
                {
                    dwFileSize.HighPart = wd.nFileSizeHigh;
                    dwFileSize.LowPart = wd.nFileSizeLow;
                    AddFileToList(szAddFile, dwFileSize, FALSE);
                }
                
                //
                //CallBack the cleanup Manager to update the UI
                //
                
                if ((dwCount++ % 10) == 0)
                {
                    if (picb)
                    {
                        if (picb->ScanProgress(cbSpaceUsed.QuadPart, 0, NULL) == E_ABORT)
                        {
                            //
                            //User aborted
                            //
                            FindClose(hFind);
                            return FALSE;
                        }
                    }
                }
                
                bFind = FindNextFile(hFind, &wd);
            }
        
            FindClose(hFind);
        
        }
        
        //
        //Recurse through all of the directories
        //
        lstrcpy(szFindPath, lpPath);
        if (PathAppend(szFindPath, TEXT("*.*")))
        {
            bFind = TRUE;
            hFind = FindFirstFile(szFindPath, &wd);
            while (hFind != INVALID_HANDLE_VALUE && bFind)
            {
                //
                //This is a directory
                //
                if ((wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    (lstrcmp(wd.cFileName, TEXT(".")) != 0) &&
                    (lstrcmp(wd.cFileName, TEXT("..")) != 0))
                {
                    lstrcpy(szAddFile, lpPath);
                    if (szAddFile[lstrlen(szAddFile) - 1] != TCHAR('\\'))
                        lstrcat(szAddFile, TEXT("\\"));
                
                    lstrcat(szAddFile, wd.cFileName);
                    if (WalkForUsedSpace(szAddFile, picb) == FALSE)
                    {
                        //
                        //User canceled
                        //
                        FindClose(hFind);
                        return FALSE;
                    }
                }
            
                bFind = FindNextFile(hFind, &wd);
            }
        
            FindClose(hFind);
        }
    }
    else
    {
        MiDebugMsg((0, TEXT("CCompCleaner::WalkForUsedSpace -> %s is NOT a directory!"),
            lpPath));
    }
    
    return bRet;
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::AddFileToList
**
** Purpose:         Adds a file to the linked list of files.
** Notes;
** Mod Log:         Created by Jason Cobb (2/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
BOOL
CCompCleaner::AddFileToList(
                            PTCHAR  lpFile,
                            ULARGE_INTEGER  filesize,
                            BOOL bDirectory)
{
    BOOL                bRet = TRUE;
    PCLEANFILESTRUCT    pNew;
    
    pNew = (PCLEANFILESTRUCT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLEANFILESTRUCT));
    
    if (pNew == NULL)
    {
        MiDebugMsg((0, TEXT("CCompCleaner::AddFileToList -> ERROR HeapAlloc() failed with error %d"),
            GetLastError()));
        return FALSE;
    }
    
    lstrcpy(pNew->file, lpFile);
    pNew->ulFileSize.QuadPart = filesize.QuadPart;
    pNew->bSelected = TRUE;
    pNew->bDirectory = bDirectory;
    
    if (head)
        pNew->pNext = head;
    else
        pNew->pNext = NULL;
    
    head = pNew;
    
    cbSpaceUsed.QuadPart += (filesize.QuadPart * 4 /10);
    
    return bRet;
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::PurgeFiles
**
** Purpose:         Removes the files from the disk.
** Notes;
** Mod Log:         Created by Jason Cobb (6/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
void
CCompCleaner::PurgeFiles(
                         IEmptyVolumeCacheCallBack *picb,
                         BOOL bDoDirectories)
                         
{
    PCLEANFILESTRUCT        pCleanFile = head;
    HANDLE                          hFile = NULL;
    USHORT InBuffer = COMPRESSION_FORMAT_DEFAULT;
    DWORD dwBytesReturned = 0;
    ULARGE_INTEGER ulCompressedSize;
    DWORD dwAttributes;
    BOOL  bFileWasRO;
    
    MiDebugMsg((0, TEXT("CCompCleaner::PurgeFiles")));
    cbSpaceFreed.QuadPart = 0;
    
    while (pCleanFile)
    {
        ulCompressedSize.QuadPart = pCleanFile->ulFileSize.QuadPart;
        
        //
        //Remove a file
        //
        
        if (!pCleanFile->bDirectory)
        {
            
            // If the file is read only, we need to remove the
            // R/O attribute long enough to compress the file.
            
            bFileWasRO = FALSE;
            dwAttributes = GetFileAttributes(pCleanFile->file);
            
            if ((0xFFFFFFFF != dwAttributes) && (dwAttributes & FILE_ATTRIBUTE_READONLY))
            {
                bFileWasRO = TRUE;
                SetFileAttributes(pCleanFile->file, FILE_ATTRIBUTE_NORMAL);
            }
            
	       hFile = CreateFile(pCleanFile->file,
               GENERIC_READ | GENERIC_WRITE,
               FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
               NULL,
               OPEN_EXISTING,
               FILE_ATTRIBUTE_NORMAL,
               NULL);
           
           if (INVALID_HANDLE_VALUE == hFile)
           {
               MiDebugMsg((0, TEXT("CCompCleaner::PurgeFiles CreateFile() %s failed with error %d"),
                   pCleanFile->file, GetLastError()));
           }
           else if (DeviceIoControl(hFile,
               FSCTL_SET_COMPRESSION,
               &InBuffer,
               sizeof(InBuffer),
               NULL,
               0,
               &dwBytesReturned,
               NULL))
           {
                   
#ifdef DEBUG
               MiDebugMsg((0, TEXT("CCompCleaner::PurgeFiles successfully compressed %s"),
                   pCleanFile->file));
#endif // DEBUG
               //
               // Get the compressed file size so we can figure out
               // how much space we gained by compressing.
               ulCompressedSize.LowPart = GetCompressedFileSize(pCleanFile->file,
                   &ulCompressedSize.HighPart);
           }
           else
           {
               MiDebugMsg((0, TEXT("CCompCleaner::PurgeFiles DeviceIoControl %s failed with error %d"),
                   pCleanFile->file, GetLastError()));
           }
               
           if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
           
           // Restore the file attributes if needed
           if (bFileWasRO) SetFileAttributes(pCleanFile->file, dwAttributes);
               
        }
        
        //
        //Adjust the cbSpaceFreed
        //
        // cbSpaceFreed.QuadPart+=pCleanFile->ulFileSize.QuadPart;
        
        cbSpaceFreed.QuadPart = cbSpaceFreed.QuadPart + (pCleanFile->ulFileSize.QuadPart - ulCompressedSize.QuadPart);
        
        //
        //Call back the cleanup manager to update the progress bar
        //
        if (picb->PurgeProgress(cbSpaceFreed.QuadPart, (cbSpaceUsed.QuadPart - cbSpaceFreed.QuadPart),
            0, NULL) == E_ABORT)
        {
            //
            //User aborted so stop removing files
            //
            MiDebugMsg((0, TEXT("CCompCleaner::PurgeFiles User abort")));
            return;
        }
        
        pCleanFile = pCleanFile->pNext;
    }
    
#ifdef DEBUG
    MiDebugMsg((0, TEXT("CompCleaner: Total space freed: %lu"), cbSpaceFreed.LowPart));
#endif // DEBUG
    return;
}

/*
**------------------------------------------------------------------------------
** CCompCleaner::FreeList
**
** Purpose:         Frees the memory allocated by AddFileToList.
** Notes;
** Mod Log:         Created by Jason Cobb (6/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
void
CCompCleaner::FreeList(PCLEANFILESTRUCT pCleanFile)
{
    PCLEANFILESTRUCT pNext;
    
    MiDebugMsg((0, TEXT("CCompCleaner::FreeList")));
    
    while (pCleanFile)
    {
        pNext = pCleanFile->pNext;
        HeapFree(GetProcessHeap(), 0, pCleanFile);
        pCleanFile = pNext;
    }
    
    return;
}

/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/
