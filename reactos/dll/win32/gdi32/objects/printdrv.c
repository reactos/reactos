#include "precomp.h"

#define NDEBUG
#include <debug.h>

/*
 * @unimplemented
 */
int
WINAPI
EndDoc(
	HDC	hdc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int
WINAPI
StartDocW(
	HDC		hdc,
	CONST DOCINFOW	*a1
	)
{
	return NtGdiStartDoc ( hdc, (DOCINFOW *)a1, NULL, 0);
}

/*
 * @unimplemented
 */
int
WINAPI
StartDocA(
	HDC		hdc,
	CONST DOCINFOA	*lpdi
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int
WINAPI
StartPage(
	HDC	hdc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int
WINAPI
EndFormPage(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int
WINAPI
EndPage(
	HDC	hdc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
WINAPI
AbortDoc(
	HDC	hdc
	)
{
   PLDC pldc;
   int Ret = SP_ERROR;
   ULONG hType = GDI_HANDLE_GET_TYPE(hdc);

   if (hType == GDILoObjType_LO_DC_TYPE || hType == GDILoObjType_LO_METADC16_TYPE)
   {
      SetLastError(ERROR_INVALID_HANDLE);
      return SP_ERROR;
   }
   pldc = GdiGetLDC(hdc);
   if ( !pldc )
   {
      SetLastError(ERROR_INVALID_HANDLE);
      return SP_ERROR;
   }
   if ( !(pldc->Flags & LDC_INIT_DOCUMENT) ) return 1;

   /* winspool:DocumentEvent printer driver */

   ((PW32CLIENTINFO)NtCurrentTeb()->Win32ClientInfo)->cSpins = 0;

   if ( pldc->Flags & LDC_META_PRINT)
   {
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return Ret;
   }

   if (NtGdiAbortDoc(hdc))
   {
      /* winspool:AbortPrinter driver */
      Ret = 1;
   }
   else
      Ret = SP_ERROR;

   pldc->Flags &= ~(LDC_META_PRINT|LDC_STARTPAGE|LDC_INIT_PAGE|LDC_INIT_DOCUMENT|LDC_SAPCALLBACK);

   return Ret;
}


/*
 * @implemented
 */
int
WINAPI
SetAbortProc(
	HDC hdc,
	ABORTPROC lpAbortProc)
{
   PLDC pldc;
   ULONG hType = GDI_HANDLE_GET_TYPE(hdc);

   if (hType == GDILoObjType_LO_DC_TYPE || hType == GDILoObjType_LO_METADC16_TYPE)
      return SP_ERROR;

   pldc = GdiGetLDC(hdc);
   if ( pldc )
   {
      if ( lpAbortProc )
      {
         if ( pldc->Flags & LDC_INIT_DOCUMENT )
         {
            pldc->Flags |= LDC_SAPCALLBACK;
            pldc->CallBackTick = GetTickCount();
         }
      }
      else
      {
         pldc->Flags &= ~LDC_SAPCALLBACK;
      }
      pldc->pAbortProc = lpAbortProc;
      return 1;
   }
   else
   {
      SetLastError(ERROR_INVALID_HANDLE);
   }
   return SP_ERROR;
}


