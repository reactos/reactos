/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    callback.cpp
**
** Purpose: Defines the IEmptyVoluemCacheCallback interface for 
**          the cleanup manager.
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"
#include "callback.h"
#include "dmgrinfo.h"
#include "dmgrdlg.h"



/*
**------------------------------------------------------------------------------
**	Local variables
**------------------------------------------------------------------------------
*/
static PCLIENTINFO g_pClientInfo;   // Set to the current CLIENTINFO struct

static CleanupMgrInfo *g_pcmi;		


CVolumeCacheCallBack::CVolumeCacheCallBack(
    void
    )
{
	g_pClientInfo = NULL;
	g_pcmi = NULL;
}

CVolumeCacheCallBack::~CVolumeCacheCallBack(
    void
    )
{
	;
}

/*
**------------------------------------------------------------------------------
** CVolumeCacheCallBack::QueryInterface
**
** Purpose:    Part of the IUnknown interface
** Parameters:
**    riid  -  interface ID to query on 
**    ppv   -  pointer to interface if we support it
** Return:     NOERROR on success, E_NOINTERFACE otherwise
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CVolumeCacheCallBack::QueryInterface(
   REFIID      riid, 
   LPVOID FAR *ppv
   )
{
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
        *ppv = (LPUNKNOWN)(LPEMPTYVOLUMECACHECALLBACK) this;
        AddRef();
        return NOERROR;
    }  

    
    //
    //Check for IEmptyVolumeCacheCallBack interface request
    //
    if (IsEqualIID (riid, IID_IEmptyVolumeCacheCallBack))
    {
        // 
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (LPEMPTYVOLUMECACHECALLBACK) this;
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
** CVolumeCacheCallBack::AddRef
**
** Purpose:    ups the reference count to this object
** Notes;
** Return:     current refernce count
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP_(ULONG) CVolumeCacheCallBack::AddRef()
{
    return ++m_cRef;
}

/*
**------------------------------------------------------------------------------
** CVolumeCacheCallBack::Release
**
** Purpose:    downs the reference count to this object
**             and deletes the object if no one is using it
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP_(ULONG) CVolumeCacheCallBack::Release()
{
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
** CVolumeCacheCallBack::ScanProgress
**
** Purpose:    Part of the IUnknown interface
** Parameters:
**    dwSpaceUsed	-  Amount of space that the client can free so far
**    dwFlags		-  IEmptyVolumeCache flags
**	  pszStatus		-  Display string to tell the user what is happening
** Return:     If E_ABORT then this indicates that no more notifications
**			   are required and the client should abort the scan.  S_OK
**			   if the client should continue scanning.
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP
CVolumeCacheCallBack::ScanProgress(
	DWORDLONG dwSpaceUsed,
	DWORD dwFlags,
	LPCWSTR pszStatus
	)
{
	//
	//Update the amount of used disk space for this client
	//
	if (g_pClientInfo)
		g_pClientInfo->dwUsedSpace.QuadPart = dwSpaceUsed;

	//
	//Check the Flags.  If this is the last notification from this client
	//then set g_pClientInfo to NULL
	//
	if (dwFlags & EVCCBF_LASTNOTIFICATION)
		g_pClientInfo = NULL;

	//
	//Has the user aborted the scan?  If so let the cleanup object know
	//so that it can stop scanning
	//
	if (g_pcmi->bAbortScan)
		return E_ABORT;

	else
		return S_OK;
}

/*
**------------------------------------------------------------------------------
** CVolumeCacheCallBack::PurgeProgress
**
** Purpose:    Part of the IUnknown interface
** Parameters:
**    dwSpaceFreed	-  Amount of disk space freed so far.
**	  dwSpaceToFree -  Amount the client was expected to free.
**    dwFlags		-  IEmptyVolumeCache flags
**	  pszStatus		-  Display string to tell the user what is happening
** Return:     If E_ABORT then this indicates that no more notifications
**			   are required and the client should abort the scan.  S_OK
**			   if the client should continue scanning.
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP
CVolumeCacheCallBack::PurgeProgress(
	DWORDLONG dwSpaceFreed,
	DWORDLONG dwSpaceToFree,
	DWORD dwFlags,
	LPCWSTR pszStatus
	)
{
	g_pcmi->cbCurrentClientPurgedSoFar.QuadPart = dwSpaceFreed;

	//
	//Update the progress bar
	//
	PostMessage(g_pcmi->hAbortPurgeWnd, WMAPP_UPDATEPROGRESS, 0, 0);


	//
	//Has the user aborted the purge?  If so let the cleanup object know
	//so that it can stop purging
	//
	if (g_pcmi->bAbortPurge)
		return E_ABORT;

	else
		return S_OK;
}

void
CVolumeCacheCallBack::SetCleanupMgrInfo(
	PVOID pVoid
	)
{
	if (pVoid)
		g_pcmi = (CleanupMgrInfo*)pVoid;
}

void
CVolumeCacheCallBack::SetCurrentClient(
	PVOID pVoid
	)
{
	if (pVoid)
		g_pClientInfo = (PCLIENTINFO)pVoid;
}
