/*	File: D:\WACKER\tdll\xfer_msc.h (Created: 28-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:41p $
 */

extern HXFER CreateXferHdl(const HSESSION hSession);

// extern INT InitializeXferHdl(const HSESSION hSession);

extern INT InitializeXferHdl(const HSESSION hSession, HXFER hXfer);

extern INT LoadXferHdl(HXFER hXfer);

// extern INT SaveXferHdl(const HSESSION hSession);

extern INT SaveXferHdl(HXFER hXfer);

// extern INT DestroyXferHdl(const HSESSION hSession);

extern INT DestroyXferHdl(HXFER hXfer);

// extern VOID WINAPI xfrSetDataPointer(HSESSION hSession, VOID *pData);

extern VOID WINAPI xfrSetDataPointer(HXFER hXfer, VOID *pData);

// extern VOID WINAPI xfrQueryDataPointer(HSESSION hSession, VOID **ppData);

extern VOID WINAPI xfrQueryDataPointer(HXFER hXfer, VOID **ppData);

// extern INT WINAPI xfrQueryParameters(HSESSION hSession, VOID **ppData);

extern INT WINAPI xfrQueryParameters(HXFER hXfer, VOID **ppData);

// extern void xfrSetParameters(HSESSION hSession, VOID *pData);

extern void xfrSetParameters(HXFER hXfer, VOID *pData);

// extern int WINAPI xfrQueryProtoParams(HSESSION hSession, int nId, VOID **ppData);

extern int WINAPI xfrQueryProtoParams(HXFER hXfer, int nId, VOID **ppData);

// extern void WINAPI xfrSetProtoParams(HSESSION hSession, int nId, VOID *pData);

extern void WINAPI xfrSetProtoParams(HXFER hXfer, int nId, VOID *pData);

// extern int xfrSendAddToList(HSESSION hSession, LPCTSTR pszFile);

extern int xfrSendAddToList(HXFER hXfer, LPCTSTR pszFile);

// extern int xfrSendListSend(HSESSION hSession);

extern int xfrSendListSend(HXFER hXfer);

// extern int xfrRecvStart(HSESSION hSession, LPCTSTR pszDir, LPCTSTR pszName);

extern int xfrRecvStart(HXFER hXfer, LPCTSTR pszDir, LPCTSTR pszName);

// extern int xfrGetEventBase(HSESSION hSession);

extern int xfrGetEventBase(HXFER hXfer);

// extern int xfrGetStatusBase(HSESSION hSession);

extern int xfrGetStatusBase(HXFER hXfer);

// extern int xfrGetXferDspBps(HSESSION hSession);

extern int xfrGetXferDspBps(HXFER hXfer);

// extern int xfrSetXferDspBps(HSESSION hSession, int nBps);

extern int xfrSetXferDspBps(HXFER hXfer, int nBps);

// extern void xfrDoTransfer(HSESSION hSession);

extern void xfrDoTransfer(HXFER hXfer);

// extern void xfrDoAutostart(HSESSION hSession, long lProtocol);

extern void xfrDoAutostart(HXFER hXfer, long lProtocol);

// extern void xfrSetPercentDone(HSESSION hSession, int nPerCent);

extern void xfrSetPercentDone(HXFER hXfer, int nPerCent);

// extern int  xfrGetPercentDone(HSESSION hSession);

extern int  xfrGetPercentDone(HXFER hXfer);

// extern HWND xfrGetDisplayWindow(HSESSION hSession);

extern HWND xfrGetDisplayWindow(HXFER hXfer);

