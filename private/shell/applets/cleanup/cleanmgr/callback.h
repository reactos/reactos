/*
**------------------------------------------------------------------------------
** Module:  Disk Space Cleanup Property Sheets
** File:    callback.h
**
** Purpose: Defines the IEmptyVoluemCacheCallback interface for 
**          the cleanup manager.
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/
#ifndef CALLBACK_H
#define CALLBACK_H

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



/*
**------------------------------------------------------------------------------
** Defines
**------------------------------------------------------------------------------
*/


/*
**------------------------------------------------------------------------------
** Global function prototypes
**------------------------------------------------------------------------------
*/


/*
**------------------------------------------------------------------------------
** Class declarations
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Class:   CVolumeCacheCallBack
** Purpose: Implements the IEmptyVolumeCacheCallBack interface
** Notes:
** Mod Log: Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
class CVolumeCacheCallBack : public IEmptyVolumeCacheCallBack {
private:
protected:
	//
	// Data
	//
	ULONG       m_cRef;         // Reference count

public:

    //  
    //Constructors
    //
    CVolumeCacheCallBack    (void);
    ~CVolumeCacheCallBack   (void);

	//
    // IUnknown interface members
	//
	STDMETHODIMP            QueryInterface (REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)    AddRef (void);
	STDMETHODIMP_(ULONG)    Release (void);

    //
    //IEmptyVolumeCacheCallBack interface members
    //
    STDMETHODIMP    ScanProgress(
                        DWORDLONG dwSpaceUsed,
                        DWORD dwFlags,
                        LPCWSTR pszStatus
                        );

    STDMETHODIMP    PurgeProgress(
                        DWORDLONG dwSpaceFreed,
                        DWORDLONG dwSpaceToFree,
                        DWORD dwFlags,
                        LPCWSTR pszStatus
                        );

	void SetCleanupMgrInfo(PVOID pVoid);
    void SetCurrentClient(PVOID pVoid);


}; // CVolumeCacheCallBack


typedef CVolumeCacheCallBack *PCVOLUMECACHECALLBACK;

#endif CALLBACK_H
/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/

