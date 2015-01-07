/*
 * DDEML library
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 * Copyright 1999 Keith Matthews
 * Copyright 2000 Corel
 * Copyright 2001 Eric Pouech
 * Copyright 2003, 2004, 2005 Dmitry Timoshkov
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(ddeml);

static const WCHAR szServerNameClass[] = L"DDEMLMom";
const char WDML_szServerConvClassA[] = "DDEMLAnsiServer";
const WCHAR WDML_szServerConvClassW[] = L"DDEMLUnicodeServer";

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

    TRACE("(%d,%p,%p)\n", idInst, hszTopic, hszItem);

    pInstance = WDML_GetInstance(idInst);

    if (pInstance == NULL)
        return FALSE;

    atom = WDML_MakeAtomFromHsz(hszItem);
    if (!atom) return FALSE;

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

	    if (hDdeData == CBR_BLOCK)
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

		if (!PostMessageW(pConv->hwndClient, WM_DDE_DATA, (WPARAM)pConv->hwndServer,
				  PackDDElParam(WM_DDE_DATA, (UINT_PTR)hItemData, atom)))
		{
		    ERR("post message failed\n");
                    pConv->wStatus &= ~ST_CONNECTED;
                    pConv->instance->lastError = DMLERR_POSTMSG_FAILED;
		    if (!WDML_IsAppOwned(hDdeData))  DdeFreeDataHandle(hDdeData);
		    GlobalFree(hItemData);
		    goto theError;
		}
                if (!WDML_IsAppOwned(hDdeData))  DdeFreeDataHandle(hDdeData);
	    }
	}
    }
    return TRUE;

 theError:
    GlobalDeleteAtom(atom);
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
    HWND 		hwndServer;
    WNDCLASSEXW  	wndclass;

    TRACE("(%d,%p,%p,%x)\n", idInst, hsz1, hsz2, afCmd);

    /*  First check instance
     */
    pInstance = WDML_GetInstance(idInst);
    if  (pInstance == NULL)
    {
	TRACE("Instance not found as initialised\n");
	/*  Nothing has been initialised - exit now ! can return TRUE since effect is the same */
        return NULL;
    }

    if (hsz2 != 0L)
    {
	/*	Illegal, reserved parameter
	 */
	pInstance->lastError = DMLERR_INVALIDPARAMETER;
	WARN("Reserved parameter no-zero !!\n");
        return NULL;
    }
    if (hsz1 == 0 && !(afCmd & DNS_UNREGISTER))
    {
	/*	don't know if we should check this but it makes sense
	 *	why supply REGISTER or filter flags if de-registering all
	 */
	TRACE("General unregister unexpected flags\n");
	pInstance->lastError = DMLERR_INVALIDPARAMETER;
        return NULL;
    }

    switch (afCmd & (DNS_REGISTER | DNS_UNREGISTER))
    {
    case DNS_REGISTER:
	pServer = WDML_FindServer(pInstance, hsz1, 0);
	if (pServer)
	{
	    ERR("Trying to register already registered service!\n");
	    pInstance->lastError = DMLERR_DLL_USAGE;
            return NULL;
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
	wndclass.cbWndExtra    = 2 * sizeof(ULONG_PTR);
	wndclass.hInstance     = 0;
	wndclass.hIcon         = 0;
	wndclass.hCursor       = 0;
	wndclass.hbrBackground = 0;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szServerNameClass;
	wndclass.hIconSm       = 0;

	RegisterClassExW(&wndclass);

	hwndServer = CreateWindowW(szServerNameClass, NULL,
				   WS_POPUP, 0, 0, 0, 0,
				   0, 0, 0, 0);

	SetWindowLongPtrW(hwndServer, GWL_WDML_INSTANCE, (ULONG_PTR)pInstance);
	SetWindowLongPtrW(hwndServer, GWL_WDML_SERVER, (ULONG_PTR)pServer);
	TRACE("Created nameServer=%p for instance=%08x\n", hwndServer, idInst);

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
            return NULL;
	}
	else
	{
	    pServer->filterOn = (afCmd & DNS_FILTERON) != 0;
	}
    }
    return (HDDEDATA)TRUE;
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
        WNDCLASSEXW wndclass;

        wndclass.cbSize        = sizeof(wndclass);
        wndclass.style         = 0;
        wndclass.lpfnWndProc   = WDML_ServerConvProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 2 * sizeof(ULONG_PTR);
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
        WNDCLASSEXA wndclass;

        wndclass.cbSize        = sizeof(wndclass);
        wndclass.style         = 0;
        wndclass.lpfnWndProc   = WDML_ServerConvProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 2 * sizeof(ULONG_PTR);
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

    TRACE("Created convServer=%p (nameServer=%p) for instance=%08x unicode=%d\n",
	  hwndServerConv, hwndServerName, pInstance->instanceID, pInstance->unicode);

    pConv = WDML_AddConv(pInstance, WDML_SERVER_SIDE, hszApp, hszTopic,
			 hwndClient, hwndServerConv);
    if (pConv)
    {
	SetWindowLongPtrW(hwndServerConv, GWL_WDML_INSTANCE, (ULONG_PTR)pInstance);
	SetWindowLongPtrW(hwndServerConv, GWL_WDML_CONVERSATION, (ULONG_PTR)pConv);

	/* this should be the only place using SendMessage for WM_DDE_ACK */
        /* note: sent messages shall not use packing */
	SendMessageW(hwndClient, WM_DDE_ACK, (WPARAM)hwndServerConv,
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

	ERR("WM_DDE_INITIATE message received!\n");
	hwndClient = (HWND)wParam;

	pInstance = WDML_GetInstanceFromWnd(hwndServer);
	if (!pInstance) return 0;
	ERR("idInst=%d, threadID=0x%x\n", pInstance->instanceID, GetCurrentThreadId());

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
                lstrcmpiA(buf, WDML_szClientConvClassA) == 0) ||
               (GetClassNameW(hwndClient, (LPWSTR)buf, sizeof(buf)/sizeof(WCHAR)) &&
                lstrcmpiW((LPWSTR)buf, WDML_szClientConvClassW) == 0))
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
		WDML_SERVER*	pServer = (WDML_SERVER*)GetWindowLongPtrW(hwndServer, GWL_WDML_SERVER);

		/* check filters for name service */
		if (!pServer->filterOn || DdeCmpStringHandles(pServer->hszService, hszApp) == 0)
		{
		    /* pass on to the callback  */
		    hDdeData = WDML_InvokeCallback(pInstance, XTYP_CONNECT,
						   0, 0, hszTop, hszApp, 0, (ULONG_PTR)pcc, self);
		    if ((ULONG_PTR)hDdeData)
		    {
			pConv = WDML_CreateServerConv(pInstance, hwndClient, hwndServer,
						      hszApp, hszTop);
                        if (pConv)
                        {
                            if (pcc) pConv->wStatus |= ST_ISLOCAL;
                            WDML_InvokeCallback(pInstance, XTYP_CONNECT_CONFIRM, 0, (HCONV)pConv,
                                                hszTop, hszApp, 0, (ULONG_PTR)pcc, self);
                        }
		    }
		}
	    }
	    else if (pInstance->servers)
	    {
		/* pass on to the callback  */
		hDdeData = WDML_InvokeCallback(pInstance, XTYP_WILDCONNECT,
					       0, 0, hszTop, hszApp, 0, (ULONG_PTR)pcc, self);

		if (hDdeData == CBR_BLOCK)
		{
		    /* MS doc is not consistent here */
		    FIXME("CBR_BLOCK returned for WILDCONNECT\n");
		}
		else if ((ULONG_PTR)hDdeData != 0)
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
                                                    hszp[i].hszTopic, hszp[i].hszSvc, 0, (ULONG_PTR)pcc, self);
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
    default:
	break;
    }

    return DefWindowProcW(hwndServer, iMsg, wParam, lParam);
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
    BOOL		fAck = TRUE;

    if (!(pConv->instance->CBFflags & CBF_FAIL_REQUESTS))
    {

	hDdeData = WDML_InvokeCallback(pConv->instance, XTYP_REQUEST, pXAct->wFmt, (HCONV)pConv,
				       pConv->hszTopic, pXAct->hszItem, 0, 0, 0);
    }

    switch ((ULONG_PTR)hDdeData)
    {
    case 0:
	TRACE("No data returned from the Callback\n");
	fAck = FALSE;
	break;

    case (ULONG_PTR)CBR_BLOCK:
	return WDML_QS_BLOCK;

    default:
        {
	    HGLOBAL	hMem = WDML_DataHandle2Global(hDdeData, TRUE, FALSE, FALSE, FALSE);
	    if (!PostMessageW(pConv->hwndClient, WM_DDE_DATA, (WPARAM)pConv->hwndServer,
			      ReuseDDElParam(pXAct->lParam, WM_DDE_REQUEST, WM_DDE_DATA,
					     (UINT_PTR)hMem, (UINT_PTR)pXAct->atom)))
	    {
                pConv->instance->lastError = DMLERR_POSTMSG_FAILED;
		DdeFreeDataHandle(hDdeData);
		GlobalFree(hMem);
		fAck = FALSE;
	    }
	}
	break;
    }

    WDML_PostAck(pConv, WDML_SERVER_SIDE, 0, FALSE, fAck, pXAct->atom, pXAct->lParam, WM_DDE_REQUEST);

    WDML_DecHSZ(pConv->instance, pXAct->hszItem);

    return WDML_QS_HANDLED;
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
    HDDEDATA		hDdeData = 0;
    BOOL		fAck = TRUE;

    pDdeAdvise = GlobalLock(pXAct->hMem);
    uType = XTYP_ADVSTART |
	    (pDdeAdvise->fDeferUpd ? XTYPF_NODATA : 0) |
	    (pDdeAdvise->fAckReq ? XTYPF_ACKREQ : 0);

    if (!(pConv->instance->CBFflags & CBF_FAIL_ADVISES))
    {
	hDdeData = WDML_InvokeCallback(pConv->instance, XTYP_ADVSTART, pDdeAdvise->cfFormat,
				       (HCONV)pConv, pConv->hszTopic, pXAct->hszItem, 0, 0, 0);
    }

    switch ((ULONG_PTR)hDdeData)
    {
    case 0:
	TRACE("No data returned from the Callback\n");
	fAck = FALSE;
	break;

    case (ULONG_PTR)CBR_BLOCK:
	return WDML_QS_BLOCK;

    default:
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
	break;
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
	ERR("Couldn't find link for %p, dropping request\n", pXAct->hszItem);
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

static BOOL data_looks_unicode( const WCHAR *data, DWORD size )
{
    DWORD i;

    if (size % sizeof(WCHAR)) return FALSE;
    for (i = 0; i < size / sizeof(WCHAR); i++) if (data[i] > 255) return FALSE;
    return TRUE;
}

/* convert data to Unicode, unless it looks like it's already Unicode */
static HDDEDATA map_A_to_W( DWORD instance, void *ptr, DWORD size )
{
    HDDEDATA ret;
    DWORD len;
    const char *end;

    if (!data_looks_unicode( ptr, size ))
    {
        if ((end = memchr( ptr, 0, size ))) size = end + 1 - (const char *)ptr;
        len = MultiByteToWideChar( CP_ACP, 0, ptr, size, NULL, 0 );
        ret = DdeCreateDataHandle( instance, NULL, len * sizeof(WCHAR), 0, 0, CF_TEXT, 0);
        MultiByteToWideChar( CP_ACP, 0, ptr, size, (WCHAR *)DdeAccessData(ret, NULL), len );
    }
    else ret = DdeCreateDataHandle( instance, ptr, size, 0, 0, CF_TEXT, 0 );

    return ret;
}

/* convert data to ASCII, unless it looks like it's not in Unicode format */
static HDDEDATA map_W_to_A( DWORD instance, void *ptr, DWORD size )
{
    HDDEDATA ret;
    DWORD len;
    const WCHAR *end;

    if (data_looks_unicode( ptr, size ))
    {
        size /= sizeof(WCHAR);
        if ((end = memchrW( ptr, 0, size ))) size = end + 1 - (const WCHAR *)ptr;
        len = WideCharToMultiByte( CP_ACP, 0, ptr, size, NULL, 0, NULL, NULL );
        ret = DdeCreateDataHandle( instance, NULL, len, 0, 0, CF_TEXT, 0);
        WideCharToMultiByte( CP_ACP, 0, ptr, size, (char *)DdeAccessData(ret, NULL), len, NULL, NULL );
    }
    else ret = DdeCreateDataHandle( instance, ptr, size, 0, 0, CF_TEXT, 0 );

    return ret;
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
        DWORD size = GlobalSize(pXAct->hMem);

	if (ptr)
	{
            if (pConv->instance->unicode)  /* Unicode server, try to map A->W */
                hDdeData = map_A_to_W( pConv->instance->instanceID, ptr, size );
            else if (!IsWindowUnicode( pConv->hwndClient )) /* ASCII server and client, try to map W->A */
                hDdeData = map_W_to_A( pConv->instance->instanceID, ptr, size );
            else
                hDdeData = DdeCreateDataHandle(pConv->instance->instanceID, ptr, size, 0, 0, CF_TEXT, 0);
	    GlobalUnlock(pXAct->hMem);
	}
	hDdeData = WDML_InvokeCallback(pConv->instance, XTYP_EXECUTE, 0, (HCONV)pConv,
				       pConv->hszTopic, 0, hDdeData, 0L, 0L);
    }

    switch ((ULONG_PTR)hDdeData)
    {
    case (ULONG_PTR)CBR_BLOCK:
	return WDML_QS_BLOCK;

    case DDE_FACK:
	fAck = TRUE;
	break;
    case DDE_FBUSY:
	fBusy = TRUE;
	break;
    default:
	FIXME("Unsupported returned value %p\n", hDdeData);
	/* fall through */
    case DDE_FNOTPROCESSED:
	break;
    }
    WDML_PostAck(pConv, WDML_SERVER_SIDE, 0, fBusy, fAck, (UINT_PTR)pXAct->hMem, 0, 0);

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

    pDdePoke = GlobalLock(pXAct->hMem);
    if (!pDdePoke)
    {
	return WDML_QS_ERROR;
    }

    if (!(pConv->instance->CBFflags & CBF_FAIL_POKES))
    {
	hDdeData = DdeCreateDataHandle(pConv->instance->instanceID, pDdePoke->Value,
				       GlobalSize(pXAct->hMem) - FIELD_OFFSET(DDEPOKE, Value),
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
    PostMessageW(pConv->hwndClient, WM_DDE_TERMINATE, (WPARAM)pConv->hwndServer, 0);
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

    TRACE("%p %04x %08lx %08lx\n", hwndServer, iMsg, wParam, lParam);

    if (iMsg == WM_DESTROY)
    {
	pConv = WDML_GetConvFromWnd(hwndServer);
	if (pConv && !(pConv->wStatus & ST_TERMINATED))
	{
	    WDML_ServerHandleTerminate(pConv, NULL);
	}
    }
    if (iMsg < WM_DDE_FIRST || iMsg > WM_DDE_LAST)
    {
        return IsWindowUnicode(hwndServer) ? DefWindowProcW(hwndServer, iMsg, wParam, lParam) :
                                             DefWindowProcA(hwndServer, iMsg, wParam, lParam);
    }

    pInstance = WDML_GetInstanceFromWnd(hwndServer);
    pConv = WDML_GetConvFromWnd(hwndServer);

    if (!pConv)
    {
	ERR("Got a message (%x) on a not known conversation, dropping request\n", iMsg);
        return 0;
    }
    if (pConv->hwndClient != WIN_GetFullHandle( (HWND)wParam ) || pConv->hwndServer != hwndServer)
    {
	ERR("mismatch between C/S windows and conversation\n");
        return 0;
    }
    if (pConv->instance != pInstance || pConv->instance == NULL)
    {
	ERR("mismatch in instances\n");
        return 0;
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
        break;
    }

    if (pXAct)
    {
	pXAct->lParam = lParam;

	if ((pConv->wStatus & ST_BLOCKED) || WDML_ServerHandle(pConv, pXAct) == WDML_QS_BLOCK)
	{
            TRACE("Transactions are blocked, add to the queue and exit\n");
	    WDML_QueueTransaction(pConv, pXAct);
	}
	else
	{
	    WDML_FreeTransaction(pInstance, pXAct, TRUE);
	}
    }
    else
        pConv->instance->lastError = DMLERR_MEMORY_ERROR;

    return 0;
}
