/*
 * DDEML library
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 * Copyright 1999 Keith Matthews
 * Copyright 2000 Corel
 * Copyright 2001 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "dde.h"
#include "ddeml.h"
#include "wine/debug.h"
#include "dde_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddeml);

static const char  szServerNameClassA[] = "DdeServerNameAnsi";
const char  WDML_szServerConvClassA[] = "DdeServerConvAnsi";
const WCHAR WDML_szServerConvClassW[] = {'D','d','e','S','e','r','v','e','r','C','o','n','v','U','n','i','c','o','d','e',0};

static LRESULT CALLBACK WDML_ServerNameProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WDML_ServerConvProc(HWND, UINT, WPARAM, LPARAM);

/******************************************************************************
 * DdePostAdvise [USER32.@]  Send transaction to DDE callback function.
 *
 * PARAMS
 *	idInst	  [I] Instance identifier
 *	hszTopic  [I] Handle to topic name string
 *	hszItem	  [I] Handle to item name string
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DdePostAdvise(DWORD idInst, HSZ hszTopic, HSZ hszItem)
{
    WDML_INSTANCE*	pInstance = NULL;
    WDML_LINK*		pLink = NULL;
    HDDEDATA		hDdeData = 0;
    HGLOBAL             hItemData = 0;
    WDML_CONV*		pConv = NULL;
    ATOM		atom = 0;
    UINT		count;

    TRACE("(%ld,%p,%p)\n", idInst, hszTopic, hszItem);

    EnterCriticalSection(&WDML_CritSect);

    pInstance = WDML_GetInstance(idInst);

    if (pInstance == NULL || pInstance->links == NULL)
    {
	goto theError;
    }

    atom = WDML_MakeAtomFromHsz(hszItem);
    if (!atom) goto theError;

    /* first compute the number of links which will trigger a message */
    count = 0;
    for (pLink = pInstance->links[WDML_SERVER_SIDE]; pLink != NULL; pLink = pLink->next)
    {
	if (DdeCmpStringHandles(hszItem, pLink->hszItem) == 0)
	{
	    count++;
	}
    }
    if (count >= CADV_LATEACK)
    {
	FIXME("too high value for count\n");
	count &= 0xFFFF;
    }

    for (pLink = pInstance->links[WDML_SERVER_SIDE]; pLink != NULL; pLink = pLink->next)
    {
	if (DdeCmpStringHandles(hszItem, pLink->hszItem) == 0)
	{
	    hDdeData = WDML_InvokeCallback(pInstance, XTYP_ADVREQ, pLink->uFmt, pLink->hConv,
					   hszTopic, hszItem, 0, --count, 0);

	    if (hDdeData == (HDDEDATA)CBR_BLOCK)
	    {
		/* MS doc is not consistent here */
		FIXME("CBR_BLOCK returned for ADVREQ\n");
		continue;
	    }
	    if (hDdeData)
	    {
		if (pLink->transactionType & XTYPF_NODATA)
		{
		    TRACE("no data\n");
		    hItemData = 0;
		}
		else
		{
		    TRACE("with data\n");

		    hItemData = WDML_DataHandle2Global(hDdeData, FALSE, FALSE, FALSE, FALSE);
		}

		pConv = WDML_GetConv(pLink->hConv, TRUE);

		if (pConv == NULL)
		{
		    if (!WDML_IsAppOwned(hDdeData))  DdeFreeDataHandle(hDdeData);
		    goto theError;
		}

		if (!PostMessageA(pConv->hwndClient, WM_DDE_DATA, (WPARAM)pConv->hwndServer,
				  PackDDElParam(WM_DDE_DATA, (UINT_PTR)hItemData, atom)))
		{
		    ERR("post message failed\n");
                    pConv->wStatus &= ~ST_CONNECTED;
		    if (!WDML_IsAppOwned(hDdeData))  DdeFreeDataHandle(hDdeData);
		    GlobalFree(hItemData);
		    goto theError;
		}
                if (!WDML_IsAppOwned(hDdeData))  DdeFreeDataHandle(hDdeData);
	    }
	}
    }
    LeaveCriticalSection(&WDML_CritSect);
    return TRUE;
 theError:
    LeaveCriticalSection(&WDML_CritSect);
    if (atom) GlobalDeleteAtom(atom);
    return FALSE;
}


/******************************************************************************
 * DdeNameService [USER32.@]  {Un}registers service name of DDE server
 *
 * PARAMS
 *    idInst [I] Instance identifier
 *    hsz1   [I] Handle to service name string
 *    hsz2   [I] Reserved
 *    afCmd  [I] Service name flags
 *
 * RETURNS
 *    Success: Non-zero
 *    Failure: 0
 */
HDDEDATA WINAPI DdeNameService(DWORD idInst, HSZ hsz1, HSZ hsz2, UINT afCmd)
{
    WDML_SERVER*	pServer;
    WDML_INSTANCE*	pInstance;
    HDDEDATA 		hDdeData;
    HWND 		hwndServer;
    WNDCLASSEXA  	wndclass;

    hDdeData = NULL;

    TRACE("(%ld,%p,%p,%x)\n", idInst, hsz1, hsz2, afCmd);

    EnterCriticalSection(&WDML_CritSect);

    /*  First check instance
     */
    pInstance = WDML_GetInstance(idInst);
    if  (pInstance == NULL)
    {
	TRACE("Instance not found as initialised\n");
	/*  Nothing has been initialised - exit now ! can return TRUE since effect is the same */
	goto theError;
    }

    if (hsz2 != 0L)
    {
	/*	Illegal, reserved parameter
	 */
	pInstance->lastError = DMLERR_INVALIDPARAMETER;
	WARN("Reserved parameter no-zero !!\n");
	goto theError;
    }
    if (hsz1 == 0 && !(afCmd & DNS_UNREGISTER))
    {
	/*	don't know if we should check this but it makes sense
	 *	why supply REGISTER or filter flags if de-registering all
	 */
	TRACE("General unregister unexpected flags\n");
	pInstance->lastError = DMLERR_INVALIDPARAMETER;
	goto theError;
    }

    switch (afCmd & (DNS_REGISTER | DNS_UNREGISTER))
    {
    case DNS_REGISTER:
	pServer = WDML_FindServer(pInstance, hsz1, 0);
	if (pServer)
	{
	    ERR("Trying to register already registered service!\n");
	    pInstance->lastError = DMLERR_DLL_USAGE;
	    goto theError;
	}

	TRACE("Adding service name\n");

	WDML_IncHSZ(pInstance, hsz1);

	pServer = WDML_AddServer(pInstance, hsz1, 0);

	WDML_BroadcastDDEWindows(WDML_szEventClass, WM_WDML_REGISTER,
				 pServer->atomService, pServer->atomServiceSpec);

	wndclass.cbSize        = sizeof(wndclass);
	wndclass.style         = 0;
	wndclass.lpfnWndProc   = WDML_ServerNameProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 2 * sizeof(DWORD);
	wndclass.hInstance     = 0;
	wndclass.hIcon         = 0;
	wndclass.hCursor       = 0;
	wndclass.hbrBackground = 0;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szServerNameClassA;
	wndclass.hIconSm       = 0;

	RegisterClassExA(&wndclass);

	LeaveCriticalSection(&WDML_CritSect);
	hwndServer = CreateWindowA(szServerNameClassA, NULL,
				   WS_POPUP, 0, 0, 0, 0,
				   0, 0, 0, 0);
	EnterCriticalSection(&WDML_CritSect);

	SetWindowLongA(hwndServer, GWL_WDML_INSTANCE, (DWORD)pInstance);
	SetWindowLongA(hwndServer, GWL_WDML_SERVER, (DWORD)pServer);
	TRACE("Created nameServer=%p for instance=%08lx\n", hwndServer, idInst);

	pServer->hwndServer = hwndServer;
	break;

    case DNS_UNREGISTER:
	if (hsz1 == 0L)
	{
	    /* General unregister situation
	     * terminate all server side pending conversations
	     */
	    while (pInstance->servers)
		WDML_RemoveServer(pInstance, pInstance->servers->hszService, 0);
	    pInstance->servers = NULL;
	    TRACE("General de-register - finished\n");
	}
	else
	{
	    WDML_RemoveServer(pInstance, hsz1, 0L);
	}
	break;
    }

    if (afCmd & (DNS_FILTERON | DNS_FILTEROFF))
    {
	/*	Set filter flags on to hold notifications of connection
	 */
	pServer = WDML_FindServer(pInstance, hsz1, 0);
	if (!pServer)
	{
	    /*  trying to filter where no service names !!
	     */
	    pInstance->lastError = DMLERR_DLL_USAGE;
	    goto theError;
	}
	else
	{
	    pServer->filterOn = (afCmd & DNS_FILTERON) != 0;
	}
    }
    LeaveCriticalSection(&WDML_CritSect);
    return (HDDEDATA)TRUE;

 theError:
    LeaveCriticalSection(&WDML_CritSect);
    return FALSE;
}

/******************************************************************
 *		WDML_CreateServerConv
 *
 *
 */
static WDML_CONV* WDML_CreateServerConv(WDML_INSTANCE* pInstance, HWND hwndClient,
					HWND hwndServerName, HSZ hszApp, HSZ hszTopic)
{
    HWND	hwndServerConv;
    WDML_CONV*	pConv;

    if (pInstance->unicode)
    {
	WNDCLASSEXW	wndclass;

	wndclass.cbSize        = sizeof(wndclass);
	wndclass.style         = 0;
	wndclass.lpfnWndProc   = WDML_ServerConvProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 2 * sizeof(DWORD);
	wndclass.hInstance     = 0;
	wndclass.hIcon         = 0;
	wndclass.hCursor       = 0;
	wndclass.hbrBackground = 0;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = WDML_szServerConvClassW;
	wndclass.hIconSm       = 0;

	RegisterClassExW(&wndclass);

	hwndServerConv = CreateWindowW(WDML_szServerConvClassW, 0,
				       WS_CHILD, 0, 0, 0, 0,
				       hwndServerName, 0, 0, 0);
    }
    else
    {
	WNDCLASSEXA	wndclass;

	wndclass.cbSize        = sizeof(wndclass);
	wndclass.style         = 0;
	wndclass.lpfnWndProc   = WDML_ServerConvProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 2 * sizeof(DWORD);
	wndclass.hInstance     = 0;
	wndclass.hIcon         = 0;
	wndclass.hCursor       = 0;
	wndclass.hbrBackground = 0;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = WDML_szServerConvClassA;
	wndclass.hIconSm       = 0;

	RegisterClassExA(&wndclass);

	hwndServerConv = CreateWindowA(WDML_szServerConvClassA, 0,
				       WS_CHILD, 0, 0, 0, 0,
				       hwndServerName, 0, 0, 0);
    }

    TRACE("Created convServer=%p (nameServer=%p) for instance=%08lx\n",
	  hwndServerConv, hwndServerName, pInstance->instanceID);

    pConv = WDML_AddConv(pInstance, WDML_SERVER_SIDE, hszApp, hszTopic,
			 hwndClient, hwndServerConv);
    if (pConv)
    {
	SetWindowLongA(hwndServerConv, GWL_WDML_INSTANCE, (DWORD)pInstance);
	SetWindowLongA(hwndServerConv, GWL_WDML_CONVERSATION, (DWORD)pConv);

	/* this should be the only place using SendMessage for WM_DDE_ACK */
        /* note: sent messages shall not use packing */
	SendMessageA(hwndClient, WM_DDE_ACK, (WPARAM)hwndServerConv,
		     MAKELPARAM(WDML_MakeAtomFromHsz(hszApp), WDML_MakeAtomFromHsz(hszTopic)));
	/* we assume we're connected since we've sent an answer...
	 * I'm not sure what we can do... it doesn't look like the return value
	 * of SendMessage is used... sigh...
	 */
	pConv->wStatus |= ST_CONNECTED;
    }
    else
    {
	DestroyWindow(hwndServerConv);
    }
    return pConv;
}

/******************************************************************
 *		WDML_ServerNameProc
 *
 *
 */
static LRESULT CALLBACK WDML_ServerNameProc(HWND hwndServer, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HWND		hwndClient;
    HSZ			hszApp, hszTop;
    HDDEDATA		hDdeData = 0;
    WDML_INSTANCE*	pInstance;
    UINT_PTR		uiLo, uiHi;

    switch (iMsg)
    {
    case WM_DDE_INITIATE:

	/* wParam         -- sending window handle
	   LOWORD(lParam) -- application atom
	   HIWORD(lParam) -- topic atom */

	TRACE("WM_DDE_INITIATE message received!\n");
	hwndClient = (HWND)wParam;

	pInstance = WDML_GetInstanceFromWnd(hwndServer);
	TRACE("idInst=%ld, threadID=0x%lx\n", pInstance->instanceID, GetCurrentThreadId());
	if (!pInstance) return 0;

	/* don't free DDEParams, since this is a broadcast */
	UnpackDDElParam(WM_DDE_INITIATE, lParam, &uiLo, &uiHi);

	hszApp = WDML_MakeHszFromAtom(pInstance, uiLo);
	hszTop = WDML_MakeHszFromAtom(pInstance, uiHi);

	if (!(pInstance->CBFflags & CBF_FAIL_CONNECTIONS))
	{
	    BOOL 		self = FALSE;
	    CONVCONTEXT		cc;
	    CONVCONTEXT*	pcc = NULL;
	    WDML_CONV*		pConv;
	    char		buf[256];

	    if (GetWindowThreadProcessId(hwndClient, NULL) == GetWindowThreadProcessId(hwndServer, NULL) &&
		WDML_GetInstanceFromWnd(hwndClient) == WDML_GetInstanceFromWnd(hwndServer))
	    {
		self = TRUE;
	    }
	    /* FIXME: so far, we don't grab distant convcontext, so only check if remote is
	     * handled under DDEML, and if so build a default context
	     */
	    if ((GetClassNameA(hwndClient, buf, sizeof(buf)) &&
		 strcmp(buf, WDML_szClientConvClassA) == 0) ||
		(GetClassNameW(hwndClient, (LPWSTR)buf, sizeof(buf)/sizeof(WCHAR)) &&
		 lstrcmpW((LPWSTR)buf, WDML_szClientConvClassW) == 0))
	    {
		pcc = &cc;
		memset(pcc, 0, sizeof(*pcc));
		pcc->cb = sizeof(*pcc);
		pcc->iCodePage = IsWindowUnicode(hwndClient) ? CP_WINUNICODE : CP_WINANSI;
	    }
	    if ((pInstance->CBFflags & CBF_FAIL_SELFCONNECTIONS) && self)
	    {
		TRACE("Don't do self connection as requested\n");
	    }
	    else if (hszApp && hszTop)
	    {
		WDML_SERVER*	pServer = (WDML_SERVER*)GetWindowLongA(hwndServer, GWL_WDML_SERVER);

		/* check filters for name service */
		if (!pServer->filterOn || DdeCmpStringHandles(pServer->hszService, hszApp) == 0)
		{
		    /* pass on to the callback  */
		    hDdeData = WDML_InvokeCallback(pInstance, XTYP_CONNECT,
						   0, 0, hszTop, hszApp, 0, (DWORD)pcc, self);
		    if ((UINT)hDdeData)
		    {
			pConv = WDML_CreateServerConv(pInstance, hwndClient, hwndServer,
						      hszApp, hszTop);
                        if (pConv)
                        {
                            if (pcc) pConv->wStatus |= ST_ISLOCAL;
                            WDML_InvokeCallback(pInstance, XTYP_CONNECT_CONFIRM, 0, (HCONV)pConv,
                                                hszTop, hszApp, 0, (DWORD)pcc, self);
                        }
		    }
		}
	    }
	    else if (pInstance->servers)
	    {
		/* pass on to the callback  */
		hDdeData = WDML_InvokeCallback(pInstance, XTYP_WILDCONNECT,
					       0, 0, hszTop, hszApp, 0, (DWORD)pcc, self);

		if (hDdeData == (HDDEDATA)CBR_BLOCK)
		{
		    /* MS doc is not consistent here */
		    FIXME("CBR_BLOCK returned for WILDCONNECT\n");
		}
		else if ((UINT)hDdeData != 0)
		{
		    HSZPAIR*	hszp;

		    hszp = (HSZPAIR*)DdeAccessData(hDdeData, NULL);
		    if (hszp)
		    {
			int	i;
			for (i = 0; hszp[i].hszSvc && hszp[i].hszTopic; i++)
			{
			    pConv = WDML_CreateServerConv(pInstance, hwndClient, hwndServer,
							  hszp[i].hszSvc, hszp[i].hszTopic);
                            if (pConv)
                            {
                                if (pcc) pConv->wStatus |= ST_ISLOCAL;
                                WDML_InvokeCallback(pInstance, XTYP_CONNECT_CONFIRM, 0, (HCONV)pConv,
                                                    hszp[i].hszTopic, hszp[i].hszSvc, 0, (DWORD)pcc, self);
                            }
			}
			DdeUnaccessData(hDdeData);
		    }
                    if (!WDML_IsAppOwned(hDdeData)) DdeFreeDataHandle(hDdeData);
		}
	    }
	}

	return 0;


    case WM_DDE_REQUEST:
	FIXME("WM_DDE_REQUEST message received!\n");
	return 0;
    case WM_DDE_ADVISE:
	FIXME("WM_DDE_ADVISE message received!\n");
	return 0;
    case WM_DDE_UNADVISE:
	FIXME("WM_DDE_UNADVISE message received!\n");
	return 0;
    case WM_DDE_EXECUTE:
	FIXME("WM_DDE_EXECUTE message received!\n");
	return 0;
    case WM_DDE_POKE:
	FIXME("WM_DDE_POKE message received!\n");
	return 0;
    case WM_DDE_TERMINATE:
	FIXME("WM_DDE_TERMINATE message received!\n");
	return 0;

    }

    return DefWindowProcA(hwndServer, iMsg, wParam, lParam);
}

/******************************************************************
 *		WDML_ServerQueueRequest
 *
 *
 */
static	WDML_XACT*	WDML_ServerQueueRequest(WDML_CONV* pConv, LPARAM lParam)
{
    UINT_PTR		uiLo, uiHi;
    WDML_XACT*		pXAct;

    UnpackDDElParam(WM_DDE_REQUEST, lParam, &uiLo, &uiHi);

    pXAct = WDML_AllocTransaction(pConv->instance, WM_DDE_REQUEST,
				  uiLo, WDML_MakeHszFromAtom(pConv->instance, uiHi));
    if (pXAct) pXAct->atom = uiHi;
    return pXAct;
}

/******************************************************************
 *		WDML_ServerHandleRequest
 *
 *
 */
static	WDML_QUEUE_STATE WDML_ServerHandleRequest(WDML_CONV* pConv, WDML_XACT* pXAct)
{
    HDDEDATA		hDdeData = 0;
    WDML_QUEUE_STATE	ret = WDML_QS_HANDLED;

    if (!(pConv->instance->CBFflags & CBF_FAIL_REQUESTS))
    {

	hDdeData = WDML_InvokeCallback(pConv->instance, XTYP_REQUEST, pXAct->wFmt, (HCONV)pConv,
				       pConv->hszTopic, pXAct->hszItem, 0, 0, 0);
    }

    switch ((ULONG_PTR)hDdeData)
    {
    case 0:
	WDML_PostAck(pConv, WDML_SERVER_SIDE, 0, FALSE, FALSE, pXAct->atom,
                     pXAct->lParam, WM_DDE_REQUEST);
	break;
    case (ULONG_PTR)CBR_BLOCK:
	ret = WDML_QS_BLOCK;
	break;
    default:
        {
	    HGLOBAL	hMem = WDML_DataHandle2Global(hDdeData, TRUE, FALSE, FALSE, FALSE);
	    if (!PostMessageA(pConv->hwndClient, WM_DDE_DATA, (WPARAM)pConv->hwndServer,
			      ReuseDDElParam(pXAct->lParam, WM_DDE_REQUEST, WM_DDE_DATA,
					     (UINT_PTR)hMem, (UINT_PTR)pXAct->atom)))
	    {
		DdeFreeDataHandle(hDdeData);
		GlobalFree(hMem);
	    }
	}
	break;
    }
    WDML_DecHSZ(pConv->instance, pXAct->hszItem);
    return ret;
}

/******************************************************************
 *		WDML_ServerQueueAdvise
 *
 *
 */
static	WDML_XACT*	WDML_ServerQueueAdvise(WDML_CONV* pConv, LPARAM lParam)
{
    UINT_PTR		uiLo, uiHi;
    WDML_XACT*		pXAct;

    /* XTYP_ADVSTART transaction:
       establish link and save link info to InstanceInfoTable */

    if (!UnpackDDElParam(WM_DDE_ADVISE, lParam, &uiLo, &uiHi))
	return NULL;

    pXAct = WDML_AllocTransaction(pConv->instance, WM_DDE_ADVISE,
				  0, WDML_MakeHszFromAtom(pConv->instance, uiHi));
    if (pXAct)
    {
	pXAct->hMem = (HGLOBAL)uiLo;
	pXAct->atom = uiHi;
    }
    return pXAct;
}

/******************************************************************
 *		WDML_ServerHandleAdvise
 *
 *
 */
static	WDML_QUEUE_STATE WDML_ServerHandleAdvise(WDML_CONV* pConv, WDML_XACT* pXAct)
{
    UINT		uType;
    WDML_LINK*		pLink;
    DDEADVISE*		pDdeAdvise;
    HDDEDATA		hDdeData;
    BOOL		fAck;

    pDdeAdvise = (DDEADVISE*)GlobalLock(pXAct->hMem);
    uType = XTYP_ADVSTART |
	    (pDdeAdvise->fDeferUpd ? XTYPF_NODATA : 0) |
	    (pDdeAdvise->fAckReq ? XTYPF_ACKREQ : 0);

    if (!(pConv->instance->CBFflags & CBF_FAIL_ADVISES))
    {
	hDdeData = WDML_InvokeCallback(pConv->instance, XTYP_ADVSTART, pDdeAdvise->cfFormat,
				       (HCONV)pConv, pConv->hszTopic, pXAct->hszItem, 0, 0, 0);
    }
    else
    {
	hDdeData = 0;
    }

    if ((UINT)hDdeData)
    {
	fAck           = TRUE;

	/* billx: first to see if the link is already created. */
	pLink = WDML_FindLink(pConv->instance, (HCONV)pConv, WDML_SERVER_SIDE,
			      pXAct->hszItem, TRUE, pDdeAdvise->cfFormat);

	if (pLink != NULL)
	{
	    /* we found a link, and only need to modify it in case it changes */
	    pLink->transactionType = uType;
	}
	else
	{
	    TRACE("Adding Link with hConv %p\n", pConv);
	    WDML_AddLink(pConv->instance, (HCONV)pConv, WDML_SERVER_SIDE,
			 uType, pXAct->hszItem, pDdeAdvise->cfFormat);
	}
    }
    else
    {
	TRACE("No data returned from the Callback\n");
	fAck = FALSE;
    }

    GlobalUnlock(pXAct->hMem);
    if (fAck)
    {
	GlobalFree(pXAct->hMem);
    }
    pXAct->hMem = 0;

    WDML_PostAck(pConv, WDML_SERVER_SIDE, 0, FALSE, fAck, pXAct->atom, pXAct->lParam, WM_DDE_ADVISE);

    WDML_DecHSZ(pConv->instance, pXAct->hszItem);

    return WDML_QS_HANDLED;
}

/******************************************************************
 *		WDML_ServerQueueUnadvise
 *
 *
 */
static	WDML_XACT* WDML_ServerQueueUnadvise(WDML_CONV* pConv, LPARAM lParam)
{
    UINT_PTR		uiLo, uiHi;
    WDML_XACT*		pXAct;

    UnpackDDElParam(WM_DDE_UNADVISE, lParam, &uiLo, &uiHi);

    pXAct = WDML_AllocTransaction(pConv->instance, WM_DDE_UNADVISE,
				  uiLo, WDML_MakeHszFromAtom(pConv->instance, uiHi));
    if (pXAct) pXAct->atom = uiHi;
    return pXAct;
}

/******************************************************************
 *		WDML_ServerHandleUnadvise
 *
 *
 */
static	WDML_QUEUE_STATE WDML_ServerHandleUnadvise(WDML_CONV* pConv, WDML_XACT* pXAct)
{
    WDML_LINK*	pLink;

    if (pXAct->hszItem == NULL || pXAct->wFmt == 0)
    {
	ERR("Unsupported yet options (null item or clipboard format)\n");
	return WDML_QS_ERROR;
    }

    pLink = WDML_FindLink(pConv->instance, (HCONV)pConv, WDML_SERVER_SIDE,
			  pXAct->hszItem, TRUE, pXAct->wFmt);
    if (pLink == NULL)
    {
	ERR("Couln'd find link for %p, dropping request\n", pXAct->hszItem);
	FreeDDElParam(WM_DDE_UNADVISE, pXAct->lParam);
	return WDML_QS_ERROR;
    }

    if (!(pConv->instance->CBFflags & CBF_FAIL_ADVISES))
    {
	WDML_InvokeCallback(pConv->instance, XTYP_ADVSTOP, pXAct->wFmt, (HCONV)pConv,
			    pConv->hszTopic, pXAct->hszItem, 0, 0, 0);
    }

    WDML_RemoveLink(pConv->instance, (HCONV)pConv, WDML_SERVER_SIDE,
		    pXAct->hszItem, pXAct->wFmt);

    /* send back ack */
    WDML_PostAck(pConv, WDML_SERVER_SIDE, 0, FALSE, TRUE, pXAct->atom,
                 pXAct->lParam, WM_DDE_UNADVISE);

    WDML_DecHSZ(pConv->instance, pXAct->hszItem);

    return WDML_QS_HANDLED;
}

/******************************************************************
 *		WDML_QueueExecute
 *
 *
 */
static	WDML_XACT* WDML_ServerQueueExecute(WDML_CONV* pConv, LPARAM lParam)
{
    WDML_XACT*	pXAct;

    pXAct = WDML_AllocTransaction(pConv->instance, WM_DDE_EXECUTE, 0, 0);
    if (pXAct)
    {
	pXAct->hMem    = (HGLOBAL)lParam;
    }
    return pXAct;
}

 /******************************************************************
 *		WDML_ServerHandleExecute
 *
 *
 */
static	WDML_QUEUE_STATE WDML_ServerHandleExecute(WDML_CONV* pConv, WDML_XACT* pXAct)
{
    HDDEDATA	hDdeData = DDE_FNOTPROCESSED;
    BOOL	fAck = FALSE, fBusy = FALSE;

    if (!(pConv->instance->CBFflags & CBF_FAIL_EXECUTES))
    {
	LPVOID	ptr = GlobalLock(pXAct->hMem);

	if (ptr)
	{
	    hDdeData = DdeCreateDataHandle(0, ptr, GlobalSize(pXAct->hMem),
					   0, 0, CF_TEXT, 0);
	    GlobalUnlock(pXAct->hMem);
	}
	hDdeData = WDML_InvokeCallback(pConv->instance, XTYP_EXECUTE, 0, (HCONV)pConv,
				       pConv->hszTopic, 0, hDdeData, 0L, 0L);
    }

    switch ((UINT)hDdeData)
    {
    case DDE_FACK:
	fAck = TRUE;
	break;
    case DDE_FBUSY:
	fBusy = TRUE;
	break;
    default:
	WARN("Bad result code\n");
	/* fall through */
    case DDE_FNOTPROCESSED:
	break;
    }
    WDML_PostAck(pConv, WDML_SERVER_SIDE, 0, fBusy, fAck, (UINT)pXAct->hMem, 0, 0);

    return WDML_QS_HANDLED;
}

/******************************************************************
 *		WDML_ServerQueuePoke
 *
 *
 */
static	WDML_XACT* WDML_ServerQueuePoke(WDML_CONV* pConv, LPARAM lParam)
{
    UINT_PTR		uiLo, uiHi;
    WDML_XACT*		pXAct;

    UnpackDDElParam(WM_DDE_POKE, lParam, &uiLo, &uiHi);

    pXAct = WDML_AllocTransaction(pConv->instance, WM_DDE_POKE,
				  0, WDML_MakeHszFromAtom(pConv->instance, uiHi));
    if (pXAct)
    {
	pXAct->atom = uiHi;
	pXAct->hMem = (HGLOBAL)uiLo;
    }
    return pXAct;
}

/******************************************************************
 *		WDML_ServerHandlePoke
 *
 *
 */
static	WDML_QUEUE_STATE WDML_ServerHandlePoke(WDML_CONV* pConv, WDML_XACT* pXAct)
{
    DDEPOKE*		pDdePoke;
    HDDEDATA		hDdeData;
    BOOL		fBusy = FALSE, fAck = FALSE;

    pDdePoke = (DDEPOKE*)GlobalLock(pXAct->hMem);
    if (!pDdePoke)
    {
	return WDML_QS_ERROR;
    }

    if (!(pConv->instance->CBFflags & CBF_FAIL_POKES))
    {
	hDdeData = DdeCreateDataHandle(pConv->instance->instanceID, pDdePoke->Value,
				       GlobalSize(pXAct->hMem) - sizeof(DDEPOKE) + 1,
				       0, 0, pDdePoke->cfFormat, 0);
	if (hDdeData)
	{
	    HDDEDATA	hDdeDataOut;

	    hDdeDataOut = WDML_InvokeCallback(pConv->instance, XTYP_POKE, pDdePoke->cfFormat,
					      (HCONV)pConv, pConv->hszTopic, pXAct->hszItem,
					      hDdeData, 0, 0);
	    switch ((ULONG_PTR)hDdeDataOut)
	    {
	    case DDE_FACK:
		fAck = TRUE;
		break;
	    case DDE_FBUSY:
		fBusy = TRUE;
		break;
	    default:
		FIXME("Unsupported returned value %p\n", hDdeDataOut);
		/* fal through */
	    case DDE_FNOTPROCESSED:
		break;
	    }
	    DdeFreeDataHandle(hDdeData);
	}
    }
    GlobalUnlock(pXAct->hMem);

    if (!fAck)
    {
	GlobalFree(pXAct->hMem);
    }
    WDML_PostAck(pConv, WDML_SERVER_SIDE, 0, fBusy, fAck, pXAct->atom, pXAct->lParam, WM_DDE_POKE);

    WDML_DecHSZ(pConv->instance, pXAct->hszItem);

    return WDML_QS_HANDLED;
}

/******************************************************************
 *		WDML_ServerQueueTerminate
 *
 *
 */
static	WDML_XACT*	WDML_ServerQueueTerminate(WDML_CONV* pConv, LPARAM lParam)
{
    WDML_XACT*	pXAct;

    pXAct = WDML_AllocTransaction(pConv->instance, WM_DDE_TERMINATE, 0, 0);
    return pXAct;
}

/******************************************************************
 *		WDML_ServerHandleTerminate
 *
 *
 */
static	WDML_QUEUE_STATE WDML_ServerHandleTerminate(WDML_CONV* pConv, WDML_XACT* pXAct)
{
    /* billx: two things to remove: the conv, and associated links.
     * Respond with another WM_DDE_TERMINATE iMsg.
     */
    if (!(pConv->instance->CBFflags & CBF_SKIP_DISCONNECTS))
    {
	WDML_InvokeCallback(pConv->instance, XTYP_DISCONNECT, 0, (HCONV)pConv, 0, 0,
			    0, 0, (pConv->wStatus & ST_ISSELF) ? 1 : 0);
    }
    PostMessageA(pConv->hwndClient, WM_DDE_TERMINATE, (WPARAM)pConv->hwndServer, 0);
    WDML_RemoveConv(pConv, WDML_SERVER_SIDE);

    return WDML_QS_HANDLED;
}

/******************************************************************
 *		WDML_ServerHandle
 *
 *
 */
WDML_QUEUE_STATE WDML_ServerHandle(WDML_CONV* pConv, WDML_XACT* pXAct)
{
    WDML_QUEUE_STATE	qs = WDML_QS_ERROR;

    switch (pXAct->ddeMsg)
    {
    case WM_DDE_INITIATE:
	FIXME("WM_DDE_INITIATE shouldn't be there!\n");
	break;
    case WM_DDE_REQUEST:
	qs = WDML_ServerHandleRequest(pConv, pXAct);
	break;

    case WM_DDE_ADVISE:
	qs = WDML_ServerHandleAdvise(pConv, pXAct);
	break;

    case WM_DDE_UNADVISE:
	qs = WDML_ServerHandleUnadvise(pConv, pXAct);
	break;

    case WM_DDE_EXECUTE:
	qs = WDML_ServerHandleExecute(pConv, pXAct);
	break;

    case WM_DDE_POKE:
	qs = WDML_ServerHandlePoke(pConv, pXAct);
	break;

    case WM_DDE_TERMINATE:
	qs = WDML_ServerHandleTerminate(pConv, pXAct);
	break;

    case WM_DDE_ACK:
	WARN("Shouldn't receive a ACK message (never requests them). Ignoring it\n");
	break;

    default:
	FIXME("Unsupported message %d\n", pXAct->ddeMsg);
    }
    return qs;
}

/******************************************************************
 *		WDML_ServerConvProc
 *
 *
 */
static LRESULT CALLBACK WDML_ServerConvProc(HWND hwndServer, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    WDML_INSTANCE*	pInstance;
    WDML_CONV*		pConv;
    WDML_XACT*		pXAct = NULL;

    if (iMsg == WM_DESTROY)
    {
	EnterCriticalSection(&WDML_CritSect);
	pConv = WDML_GetConvFromWnd(hwndServer);
	if (pConv && !(pConv->wStatus & ST_TERMINATED))
	{
	    WDML_ServerHandleTerminate(pConv, NULL);
	}
	LeaveCriticalSection(&WDML_CritSect);
    }
    if (iMsg < WM_DDE_FIRST || iMsg > WM_DDE_LAST)
    {
	return IsWindowUnicode(hwndServer) ? DefWindowProcW(hwndServer, iMsg, wParam, lParam) :
                                             DefWindowProcA(hwndServer, iMsg, wParam, lParam);
    }

    EnterCriticalSection(&WDML_CritSect);

    pInstance = WDML_GetInstanceFromWnd(hwndServer);
    pConv = WDML_GetConvFromWnd(hwndServer);

    if (!pConv)
    {
	ERR("Got a message (%x) on a not known conversation, dropping request\n", iMsg);
	goto theError;
    }
    if (pConv->hwndClient != (HWND)wParam || pConv->hwndServer != hwndServer)
    {
	ERR("mismatch between C/S windows and converstation\n");
	goto theError;
    }
    if (pConv->instance != pInstance || pConv->instance == NULL)
    {
	ERR("mismatch in instances\n");
	goto theError;
    }

    switch (iMsg)
    {
    case WM_DDE_INITIATE:
	FIXME("WM_DDE_INITIATE message received!\n");
	break;

    case WM_DDE_REQUEST:
	pXAct = WDML_ServerQueueRequest(pConv, lParam);
	break;

    case WM_DDE_ADVISE:
	pXAct = WDML_ServerQueueAdvise(pConv, lParam);
	break;

    case WM_DDE_UNADVISE:
	pXAct = WDML_ServerQueueUnadvise(pConv, lParam);
	break;

    case WM_DDE_EXECUTE:
	pXAct = WDML_ServerQueueExecute(pConv, lParam);
	break;

    case WM_DDE_POKE:
	pXAct = WDML_ServerQueuePoke(pConv, lParam);
	break;

    case WM_DDE_TERMINATE:
	pXAct = WDML_ServerQueueTerminate(pConv, lParam);
	break;

    case WM_DDE_ACK:
	WARN("Shouldn't receive a ACK message (never requests them). Ignoring it\n");
	break;

    default:
	FIXME("Unsupported message %x\n", iMsg);
    }

    if (pXAct)
    {
	pXAct->lParam = lParam;
	if (WDML_ServerHandle(pConv, pXAct) == WDML_QS_BLOCK)
	{
	    WDML_QueueTransaction(pConv, pXAct);
	}
	else
	{
	    WDML_FreeTransaction(pInstance, pXAct, TRUE);
	}
    }
 theError:
    LeaveCriticalSection(&WDML_CritSect);
    return 0;
}
