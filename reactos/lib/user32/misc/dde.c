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
/* $Id: dde.c,v 1.3 2002/09/08 10:23:10 chorns Exp $
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
  return FALSE;
}

LPBYTE STDCALL
DdeAccessData(HDDEDATA hData,
	      LPDWORD pcbDataSize)
{
  return (LPBYTE)0;
}

HDDEDATA STDCALL
DdeAddData(HDDEDATA hData,
	   LPBYTE pSrc,
	   DWORD cb,
	   DWORD cbOff)
{
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
  return (HDDEDATA)0;
}

int STDCALL
DdeCmpStringHandles(HSZ hsz1,
		    HSZ hsz2)
{
  return 0;
}

HCONV STDCALL
DdeConnect(DWORD idInst,
	   HSZ hszService,
	   HSZ hszTopic,
	   PCONVCONTEXT pCC)
{
  return (HCONV)0;
}

HCONVLIST STDCALL
DdeConnectList(DWORD idInst,
	       HSZ hszService,
	       HSZ hszTopic,
	       HCONVLIST hConvList,
	       PCONVCONTEXT pCC)
{
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
  return (HDDEDATA)0;
}

HSZ STDCALL
DdeCreateStringHandleA(DWORD idInst,
		       LPSTR psz,
		       int iCodePage)
{
  return (HSZ)0;
}

HSZ STDCALL
DdeCreateStringHandleW(DWORD idInst,
		       LPWSTR psz,
		       int iCodePage)
{
  return (HSZ)0;
}

WINBOOL STDCALL
DdeDisconnect(HCONV hConv)
{
  return FALSE;
}

WINBOOL STDCALL
DdeDisconnectList(HCONVLIST hConvList)
{
  return FALSE;
}

WINBOOL STDCALL
DdeEnableCallback(DWORD idInst,
		  HCONV hConv,
		  UINT wCmd)
{
  return FALSE;
}

WINBOOL STDCALL
DdeFreeDataHandle(HDDEDATA hData)
{
  return FALSE;
}

BOOL
DdeFreeStringHandle(DWORD idInst,
		    HSZ hsz)
{
  return FALSE;
}

DWORD STDCALL
DdeGetData(HDDEDATA hData,
	   LPBYTE pDst,
	   DWORD cbMax,
	   DWORD cbOff)
{
  return 0;
}

UINT STDCALL
DdeGetLastError(DWORD idInst)
{
  return 0;
}

WINBOOL STDCALL
DdeImpersonateClient(HCONV hConv)
{
  return FALSE;
}

UINT STDCALL
DdeInitializeA(LPDWORD pidInst,
	       PFNCALLBACK pfnCallback,
	       DWORD afCmd,
	       DWORD ulRes)
{
  return 0;
}

UINT STDCALL
DdeInitializeW(LPDWORD pidInst,
	       PFNCALLBACK pfnCallback,
	       DWORD afCmd,
	       DWORD ulRes)
{
  return 0;
}

WINBOOL STDCALL
DdeKeepStringHandle(DWORD idInst,
		    HSZ hsz)
{
  return FALSE;
}

HDDEDATA STDCALL
DdeNameService(DWORD idInst,
	       HSZ hsz1,
	       HSZ hsz2,
	       UINT afCmd)
{
  return (HDDEDATA)0;
}

WINBOOL STDCALL
DdePostAdvise(DWORD idInst,
	      HSZ hszTopic,
	      HSZ hszItem)
{
  return FALSE;
}

UINT STDCALL
DdeQueryConvInfo(HCONV hConv,
		 DWORD idTransaction,
		 PCONVINFO pConvInfo)
{
  return 0;
}

HCONV STDCALL
DdeQueryNextServer(HCONVLIST hConvList,
		   HCONV hConvPrev)
{
  return (HCONV)0;
}

DWORD STDCALL
DdeQueryStringA(DWORD idInst,
		HSZ hsz,
		LPSTR psz,
		DWORD cchMax,
		int iCodePage)
{
  return 0;
}

DWORD STDCALL
DdeQueryStringW(DWORD idInst,
		HSZ hsz,
		LPWSTR psz,
		DWORD cchMax,
		int iCodePage)
{
  return 0;
}

HCONV STDCALL
DdeReconnect(HCONV hConv)
{
  return (HCONV)0;
}

WINBOOL STDCALL
DdeSetQualityOfService(HWND hwndClient,
		       CONST SECURITY_QUALITY_OF_SERVICE *pqosNew,
		       PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
  return FALSE;
}

WINBOOL STDCALL
DdeSetUserHandle(HCONV hConv,
		 DWORD id,
		 DWORD_PTR hUser)
{
  return FALSE;
}

WINBOOL STDCALL
DdeUnaccessData(HDDEDATA hData)
{
  return FALSE;
}

WINBOOL STDCALL
DdeUninitialize(DWORD idInst)
{
  return FALSE;
}

WINBOOL STDCALL
FreeDDElParam(UINT msg,
	      LPARAM lParam)
{
  return FALSE;
}

WINBOOL STDCALL
ImpersonateDdeClientWindow(HWND hWndClient,
			   HWND hWndServer)
{
  return FALSE;
}

LPARAM STDCALL
PackDDElParam(UINT msg,
	      UINT_PTR uiLo,
	      UINT_PTR uiHi)
{
  return (LPARAM)0;
}

LPARAM STDCALL
ReuseDDElParam(LPARAM lParam,
	       UINT msgIn,
	       UINT msgOut,
	       UINT_PTR uiLo,
	       UINT_PTR uiHi)
{
  return (LPARAM)0;
}

WINBOOL STDCALL
UnpackDDElParam(UINT msg,
		LPARAM lParam,
		PUINT_PTR puiLo,
		PUINT_PTR puiHi)
{
  return FALSE;
}
