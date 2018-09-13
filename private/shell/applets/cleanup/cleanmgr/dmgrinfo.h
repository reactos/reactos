/*
**------------------------------------------------------------------------------
** Module:  Disk Space Cleanup Property Sheets
** File:    dmgrinfo.h
**
** Purpose: Defines the CleanupMgrInfo class for the property tab
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/
#ifndef DMGRINFO_H
#define DMGRINFO_H

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/

#ifndef COMMON_H
   #include "common.h"
#endif

#ifndef EMPTYVC_H
    #include <emptyvc.h>
#endif

#ifndef DISKUTIL_H
   #include "diskutil.h"
#endif

#ifndef CALLBACK_H
    #include "callback.h"
#endif



/*
**------------------------------------------------------------------------------
** Defines
**------------------------------------------------------------------------------
*/
#define	WMAPP_UPDATEPROGRESS	WM_USER+1
#define WMAPP_UPDATESTATUS		WM_USER+2

#define PROGRESS_DIVISOR		0xFFFF
/*
**------------------------------------------------------------------------------
** Global function prototypes
**------------------------------------------------------------------------------
*/
   // forward references to make compile work
class CleanupMgrInfo;

CleanupMgrInfo * GetCleanupMgrInfoPointer(HWND hDlg);


/*
**------------------------------------------------------------------------------
** Class declarations
**------------------------------------------------------------------------------
*/
typedef struct tag_ClientInfo
{
    HICON               hIcon;
    CLSID               clsid;
    LPEMPTYVOLUMECACHE  pVolumeCache;
    HKEY                hClientKey;
    TCHAR				szRegKeyName[MAX_PATH];
	LPWSTR				wcsDescription;
	LPWSTR				wcsDisplayName;
	LPWSTR				wcsAdvancedButtonText;
    DWORD               dwInitializeFlags;
    DWORD				dwPriority;
    ULARGE_INTEGER      dwUsedSpace;
    BOOL				bShow;
    BOOL                bSelected;
} CLIENTINFO, *PCLIENTINFO;

/*
**------------------------------------------------------------------------------
** Class:   CleanupMgrInfo
** Purpose: Stores useful info for the Disk space cleanup manager drive tab
** Notes:
** Mod Log: Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
class CleanupMgrInfo {
private:
protected:
    static      HINSTANCE   hInstance;
    
    void  init(void);
    void  destroy(void);
    BOOL  initializeClients(void);
    void  deactivateClients(void);
    void  deactivateSingleClient(PCLIENTINFO pSingleClientInfo);
    BOOL  getSpaceUsedByClients(void);
	void calculateSpaceToPurge(void);
    HICON GetClientIcon(LPTSTR, BOOL fIconPath);

public:
    drenum      dre;                        // Drive letter
	HICON		hDriveIcon;					// Drive Icon
    TCHAR       szRoot[MAX_PATH];           // Root
    TCHAR       szVolName[MAX_PATH];        // Volume name
    TCHAR		szFileSystem[MAX_PATH];		// File System name		
    hardware    hwHardware;                 // Hardware Type
    volumetype  vtVolume;                   // Volume Type

    ULARGE_INTEGER   cbDriveFree;           // Free space on drive
    ULARGE_INTEGER   cbDriveUsed;           // Used space on drive
    ULARGE_INTEGER   cbEstCleanupSpace;     // Estimated space that can be cleaned
	ULARGE_INTEGER	 cbLowSpaceThreshold;	// Low disk space threshold (for agressive mode)
	ULARGE_INTEGER	 cbSpaceToPurge;
	ULARGE_INTEGER	 cbProgressDivider;

	DWORD		dwReturnCode;
    DWORD		dwUIFlags;
    ULONG		ulSAGEProfile;				// SAGE Profile
    BOOL		bOutOfDiskSpace;			// Are we in agressive mode?
    BOOL		bPurgeFiles;				// Should we delete the files?

	HANDLE		hAbortScanThread;			// Abort Scan thread Handle
	HWND		hAbortScanWnd;
	HANDLE		hAbortScanEvent;
	DWORD		dwAbortScanThreadID;

	HANDLE		hAbortPurgeThread;
	HWND		hAbortPurgeWnd;
	HANDLE		hAbortPurgeEvent;
	DWORD		dwAbortPurgeThreadID;
	ULARGE_INTEGER	cbTotalPurgedSoFar;
	ULARGE_INTEGER	cbCurrentClientPurgedSoFar;

	static void Register(HINSTANCE hInstance);
	static void Unregister();

    //
    //Volume Cache client information
    //
    int         iNumVolumeCacheClients;
    PCLIENTINFO pClientInfo;

    //
    //IEmptyVolumeCacheCallBack interface
    //
    PCVOLUMECACHECALLBACK    volumeCacheCallBack;
    LPEMPTYVOLUMECACHECALLBACK  pIEmptyVolumeCacheCallBack;
    BOOL                    bAbortScan;    
    BOOL                    bAbortPurge;         
    
    //  
    //Constructors
    //
    CleanupMgrInfo    (void);
    CleanupMgrInfo    (LPTSTR lpDrive, DWORD dwFlags, ULONG ulProfile);
    ~CleanupMgrInfo   (void);

    //   
    //Creation methods
    //
    BOOL isValid   (void)   { return dre != Drive_INV; }
    BOOL create    (LPTSTR lpDrive, DWORD Flags);
	BOOL isAbortScan (void)	{ return bAbortScan; }

	BOOL  purgeClients(void);

}; // CleanupMgrInfo


#endif DMGRINFO_H
/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/
