/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: dde.c,v 1.4 2003/05/12 19:30:00 jfilby Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/dde.c
 * PURPOSE:         DDE
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

WINBOOL STDCALL
DdeAbandonTransaction(DWORD idInst,
		      HCONV hConv,
		      DWORD idTransaction)
{
  UNIMPLEMENTED;
  return FALSE;
}

LPBYTE STDCALL
DdeAccessData(HDDEDATA hData,
	      LPDWORD pcbDataSize)
{
  UNIMPLEMENTED;
  return (LPBYTE)0;
}

HDDEDATA STDCALL
DdeAddData(HDDEDATA hData,
	   LPBYTE pSrc,
	   DWORD cb,
	   DWORD cbOff)
{
  UNIMPLEMENTED;
  return (HDDEDATA)0;
}

HDDEDATA STDCALL
DdeClientTransaction(LPBYTE pData,
		     DWORD cbData,
		     HCONV hConv,
		     HSZ hszItem,
		     UINT wFmt,
		     UINT wType,
		     DWORD dwTimeout,
		     LPDWORD pdwResult)
{
  UNIMPLEMENTED;
  return (HDDEDATA)0;
}

int STDCALL
DdeCmpStringHandles(HSZ hsz1,
		    HSZ hsz2)
{
  UNIMPLEMENTED;
  return 0;
}

HCONV STDCALL
DdeConnect(DWORD idInst,
	   HSZ hszService,
	   HSZ hszTopic,
	   PCONVCONTEXT pCC)
{
  UNIMPLEMENTED;
  return (HCONV)0;
}

HCONVLIST STDCALL
DdeConnectList(DWORD idInst,
	       HSZ hszService,
	       HSZ hszTopic,
	       HCONVLIST hConvList,
	       PCONVCONTEXT pCC)
{
  UNIMPLEMENTED;
  return (HCONVLIST)0;
}

HDDEDATA STDCALL
DdeCreateDataHandle(DWORD idInst,
		    LPBYTE pSrc,
		    DWORD cb,
		    DWORD cbOff,
		    HSZ hszItem,
		    UINT wFmt,
		    UINT afCmd)
{
  UNIMPLEMENTED;
  return (HDDEDATA)0;
}

HSZ STDCALL
DdeCreateStringHandleA(DWORD idInst,
		       LPSTR psz,
		       int iCodePage)
{
  UNIMPLEMENTED;
  return (HSZ)0;
}

HSZ STDCALL
DdeCreateStringHandleW(DWORD idInst,
		       LPWSTR psz,
		       int iCodePage)
{
  UNIMPLEMENTED;
  return (HSZ)0;
}

WINBOOL STDCALL
DdeDisconnect(HCONV hConv)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
DdeDisconnectList(HCONVLIST hConvList)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
DdeEnableCallback(DWORD idInst,
		  HCONV hConv,
		  UINT wCmd)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
DdeFreeDataHandle(HDDEDATA hData)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
DdeFreeStringHandle(DWORD idInst,
		    HSZ hsz)
{
  UNIMPLEMENTED;
  return FALSE;
}

DWORD STDCALL
DdeGetData(HDDEDATA hData,
	   LPBYTE pDst,
	   DWORD cbMax,
	   DWORD cbOff)
{
  UNIMPLEMENTED;
  return 0;
}

UINT STDCALL
DdeGetLastError(DWORD idInst)
{
  UNIMPLEMENTED;
  return 0;
}

WINBOOL STDCALL
DdeImpersonateClient(HCONV hConv)
{
  UNIMPLEMENTED;
  return FALSE;
}

UINT STDCALL
DdeInitializeA(LPDWORD pidInst,
	       PFNCALLBACK pfnCallback,
	       DWORD afCmd,
	       DWORD ulRes)
{
  UNIMPLEMENTED;
  return 0;
}

UINT STDCALL
DdeInitializeW(LPDWORD pidInst,
	       PFNCALLBACK pfnCallback,
	       DWORD afCmd,
	       DWORD ulRes)
{
  UNIMPLEMENTED;
  return 0;
}

WINBOOL STDCALL
DdeKeepStringHandle(DWORD idInst,
		    HSZ hsz)
{
  UNIMPLEMENTED;
  return FALSE;
}

HDDEDATA STDCALL
DdeNameService(DWORD idInst,
	       HSZ hsz1,
	       HSZ hsz2,
	       UINT afCmd)
{
  UNIMPLEMENTED;
  return (HDDEDATA)0;
}

WINBOOL STDCALL
DdePostAdvise(DWORD idInst,
	      HSZ hszTopic,
	      HSZ hszItem)
{
  UNIMPLEMENTED;
  return FALSE;
}

UINT STDCALL
DdeQueryConvInfo(HCONV hConv,
		 DWORD idTransaction,
		 PCONVINFO pConvInfo)
{
  UNIMPLEMENTED;
  return 0;
}

HCONV STDCALL
DdeQueryNextServer(HCONVLIST hConvList,
		   HCONV hConvPrev)
{
  UNIMPLEMENTED;
  return (HCONV)0;
}

DWORD STDCALL
DdeQueryStringA(DWORD idInst,
		HSZ hsz,
		LPSTR psz,
		DWORD cchMax,
		int iCodePage)
{
  UNIMPLEMENTED;
  return 0;
}

DWORD STDCALL
DdeQueryStringW(DWORD idInst,
		HSZ hsz,
		LPWSTR psz,
		DWORD cchMax,
		int iCodePage)
{
  UNIMPLEMENTED;
  return 0;
}

HCONV STDCALL
DdeReconnect(HCONV hConv)
{
  UNIMPLEMENTED;
  return (HCONV)0;
}

WINBOOL STDCALL
DdeSetQualityOfService(HWND hwndClient,
		       CONST SECURITY_QUALITY_OF_SERVICE *pqosNew,
		       PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
DdeSetUserHandle(HCONV hConv,
		 DWORD id,
		 DWORD_PTR hUser)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
DdeUnaccessData(HDDEDATA hData)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
DdeUninitialize(DWORD idInst)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
FreeDDElParam(UINT msg,
	      LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
ImpersonateDdeClientWindow(HWND hWndClient,
			   HWND hWndServer)
{
  UNIMPLEMENTED;
  return FALSE;
}

LPARAM STDCALL
PackDDElParam(UINT msg,
	      UINT_PTR uiLo,
	      UINT_PTR uiHi)
{
  UNIMPLEMENTED;
  return (LPARAM)0;
}

LPARAM STDCALL
ReuseDDElParam(LPARAM lParam,
	       UINT msgIn,
	       UINT msgOut,
	       UINT_PTR uiLo,
	       UINT_PTR uiHi)
{
  UNIMPLEMENTED;
  return (LPARAM)0;
}

WINBOOL STDCALL
UnpackDDElParam(UINT msg,
		LPARAM lParam,
		PUINT_PTR puiLo,
		PUINT_PTR puiHi)
{
  UNIMPLEMENTED;
  return FALSE;
}
