/* 
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              ddraw lib
 * PROGRAMMER:           Magnus Olsen
 * UPDATE HISTORY:
 */


 HANDLE STDCALL OsThunkDdCreateDirectDrawObject(HDC hdc);


HRESULT DDRAW_Create(LPGUID lpGUID, LPVOID *lplpDD, LPUNKNOWN pUnkOuter, REFIID iid, BOOL ex); 
