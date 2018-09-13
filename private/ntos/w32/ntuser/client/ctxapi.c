
/****************************** Module Header ******************************\
* Module Name: ctxapi.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/**************************************************************
* CtxUserGetWinstationInfo
*
* This functin is called to get the winstation information such as Protocol name,
* Audio driver name etc. .All these information are passed in by client at
* connection time.
*
*
****************************************************************/

BOOL
CtxUserGetWinstationInfo(PWINSTATIONINFO pInfo)
{
   NTSTATUS status;

   status = NtUserRemoteGetWinstationInfo((PWSXINFO)pInfo);

   if (NT_SUCCESS(status)) {
      return TRUE;
   }
   else {
      return FALSE;
   }
}
