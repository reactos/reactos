/* $Id: text.c,v 1.1 2004/02/26 00:33:09 sedwards Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/text.c
 * PURPOSE:         advapi32.dll Text Functions
 * PROGRAMMER:      Steven Edwards
 * UPDATE HISTORY:
 *	20042502
 */
#include <windows.h>

BOOL STDCALL IsTextUnicode(CONST VOID* pbuffer,	int cb,	LPINT lpi)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}
