/*
**------------------------------------------------------------------------------
** Module:  NTFS Compression Disk Cleanup cleaner
** File:    cclean.h
**
** Purpose: Defines the 'Compression cleaner' for the IEmptyVolumeCache2
**          interface
** Notes:
** Mod Log: Created by Jason Cobb (2/97)
**          Adapted for NTFS compression clean by DSchott (6/98)
**
** Copyright (c)1997-1998 Microsoft Corporation. All Rights Reserved.
**------------------------------------------------------------------------------
*/
#ifndef COMPCLEN_H
#define COMPCLEN_H

#ifndef COMMON_H
    #include "common.h"
#endif


#ifndef WINNT

//
// Stolen from DEVIOCTL.H
//
#ifndef CTL_CODE
   #define CTL_CODE( DeviceType, Function, Method, Access ) ( \
            ((DeviceType) << 16)  | ((Access) << 14) | ((Function) << 2) | (Method) \
           )
#endif

#ifndef METHOD_BUFFERED
    #define METHOD_BUFFERED 0
#endif

#ifndef FILE_DEVICE_FILE_SYSTEM
   #define FILE_DEVICE_FILE_SYSTEM 0x00000009
#endif

// END Stolen from DEVIOCTL.H

//
// Stolen from NTIOAPI.H
//

#ifndef FILE_ATTRIBUTE_OFFLINE
   #define FILE_ATTRIBUTE_OFFLINE 0x00001000  // winnt
#endif

#ifndef FSCTL_SET_COMPRESSION
   #define FSCTL_SET_COMPRESSION CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 16, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#endif

// END Stolen from NTIOAPI.H

#endif // !winnt

/*
**------------------------------------------------------------------------------
** Project include files 
**------------------------------------------------------------------------------

*/
/*
**------------------------------------------------------------------------------
** Global Defines 
**------------------------------------------------------------------------------
*/

#define COMPCLN_REGPATH TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Compress old files")

#define MAX_DAYS 500           // Settings dialog spin control max
#define MIN_DAYS 1             // Settings dialog spin control min
#define DEFAULT_DAYS 50        // Default #days if no setting in registry

/*
**------------------------------------------------------------------------------
** Global function prototypes
**------------------------------------------------------------------------------
*/

//
// Already defined in common.h
//
// Dll Object count methods
//UINT getDllObjectCount (void);
//
// Dll Lock count methods
//UINT getDllLockCount (void);

/*
**------------------------------------------------------------------------------
** Class Declarations
**------------------------------------------------------------------------------
*/


/*
**------------------------------------------------------------------------------
** Class:   CCompCleanerClassFactory
** Purpose: Manufactures instances of our CCompCleaner object
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**          Adapted for NTFS compression cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/ 
class CCompCleanerClassFactory : public IClassFactory
{
private:
protected:
    //
    // Data
    //
    ULONG   m_cRef;     // Reference count

public:
    //
    // Constructors
    //
    CCompCleanerClassFactory();
    ~CCompCleanerClassFactory();
  
        //
    // IUnknown interface members
    //
        STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
        STDMETHODIMP_(ULONG)    AddRef();
        STDMETHODIMP_(ULONG)    Release();

    //   
    // IClassFactory interface members
        //
        STDMETHODIMP            CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
        STDMETHODIMP            LockServer(BOOL);
};

typedef CCompCleanerClassFactory *LPCCOMPCLEANERCLASSFACTORY;

/*
**------------------------------------------------------------------------------
** Class:   CCompCleaner
** Purpose: This is the actual Compression Cleaner Class
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**          Adapted for NTFS compression cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/ 
class CCompCleaner : public IEmptyVolumeCache2
{
private:
protected:
    //
    // Data
    //
    ULONG               m_cRef;                 // reference count
    LPDATAOBJECT        m_lpdObject;            // Last Data object

    ULARGE_INTEGER      cbSpaceUsed;
    ULARGE_INTEGER      cbSpaceFreed;

    FILETIME            ftMinLastAccessTime;
    DWORD               dwDaysForCurrentLAD;
    DWORD               dwDaysForCurrentList;

    TCHAR               szVolume[MAX_PATH];
    TCHAR               szFolder[MAX_PATH];
    TCHAR               filelist[MAX_PATH];
    BOOL                bPurged;                // TRUE if Purge() method was run
    BOOL                bSettingsMode;          // TRUE if currently in settings mode

    PCLEANFILESTRUCT    head;                   // head of the linked list of files

    BOOL WalkForUsedSpace(PTCHAR lpPath, IEmptyVolumeCacheCallBack *picb);
    BOOL AddFileToList(PTCHAR lpFile, ULARGE_INTEGER filesize, BOOL bDirectory);
    void PurgeFiles(IEmptyVolumeCacheCallBack *picb, BOOL bDoDirectories);
    void FreeList(PCLEANFILESTRUCT pCleanFile);
    void BuildList(IEmptyVolumeCacheCallBack *picb);
    void CalcLADFileTime();
    BOOL bLastAccessisOK(FILETIME ftFileLastAccess);

public:
    //
    // Constructors
    //
    CCompCleaner (void);
    ~CCompCleaner (void);

    //
    // IUnknown interface members
    //
    STDMETHODIMP            QueryInterface (REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);
    
    //
    // IEmptyVolumeCache2 interface members
    //
    STDMETHODIMP    Initialize(
                HKEY hRegKey,
                LPCWSTR pszVolume,
                LPWSTR *ppszDisplayName,
                LPWSTR *ppszDescription,
                DWORD *pdwFlags
                );


    STDMETHODIMP    GetSpaceUsed(
                DWORDLONG *pdwSpaceUsed,
                IEmptyVolumeCacheCallBack *picb
                );
                
    STDMETHODIMP    Purge(
                DWORDLONG dwSpaceToFree,
                IEmptyVolumeCacheCallBack *picb
                );
                
    STDMETHODIMP    ShowProperties(
                HWND hwnd
                );
                
    STDMETHODIMP    Deactivate(
                DWORD *pdwFlags
                );                                                                                                                                

    STDMETHODIMP    InitializeEx(
                HKEY hRegKey,
                LPCWSTR pcwszVolume,
                LPCWSTR pcwszKeyName,
                LPWSTR *ppwszDisplayName,
                LPWSTR *ppwszDescription,
                LPWSTR *ppwszBtnText,
                DWORD *pdwFlags
                );

};

typedef CCompCleaner *LPCCOMPCLEANER;


#endif // CCLEAN_H
/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/
