/* $Id: stubs.c,v 1.2 2001/10/01 16:31:12 rex Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/oldaut32/stubs.c
 * PURPOSE:         Stubbed exports
 * PROGRAMMER:      Rex Jolliff (rex@lvcablemodem.com)
 */

#include <ddk/ntddk.h>
#include <windows.h>

#define NDEBUG
#include <debug.h>

#define  FIXME  \
  do  \
  {   \
    DbgPrint ("%s(%d):%s not implemented\n", __FILE__, __LINE__, __FUNCTION__);  \
  }   \
  while (0)

VOID
DllGetClassObject (VOID)
{
  FIXME;
}

VOID STDCALL
SysAllocString (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SysReAllocString (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
SysAllocStringLen (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
SysReAllocStringLen (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SysFreeString (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SysStringLen (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
VariantInit (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
VariantClear (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
VariantCopy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VariantCopyInd (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VariantChangeType (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VariantTimeToDosDateTime (VOID)
{
  FIXME;
}

VOID STDCALL
DosDateTimeToVariantTime (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SafeArrayCreate (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SafeArrayDestroy (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SafeArrayGetDim (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SafeArrayGetElemsize (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SafeArrayGetUBound (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SafeArrayGetLBound (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SafeArrayLock (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SafeArrayUnlock (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SafeArrayAccessData (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
SafeArrayUnaccessData (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SafeArrayGetElement (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SafeArrayPutElement (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SafeArrayCopy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
DispGetParam (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  FIXME;
}

VOID STDCALL
DispGetIDsOfNames (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
DispInvoke (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7, DWORD Unknown8)
{
  FIXME;
}

VOID STDCALL
CreateDispTypeInfo (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
CreateStdDispatch (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
RegisterActiveObject (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
RevokeActiveObject (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
GetActiveObject (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SafeArrayAllocDescriptor (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
SafeArrayAllocData (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SafeArrayDestroyDescriptor (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SafeArrayDestroyData (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SafeArrayRedim (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
OACreateTypeLib2 (VOID)
{
  FIXME;
}

VOID STDCALL
VarParseNumFromStr (VOID)
{
  FIXME;
}

VOID STDCALL
VarNumFromParseNum (VOID)
{
  FIXME;
}

VOID STDCALL
VarI2FromUI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI2FromI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI2FromR4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI2FromR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI2FromCy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI2FromDate (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI2FromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarI2FromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarI2FromBool (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI4FromUI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI4FromI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI4FromR4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI4FromR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI4FromCy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI4FromDate (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI4FromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarI4FromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarI4FromBool (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR4FromUI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR4FromI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR4FromI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR4FromR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR4FromCy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR4FromDate (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR4FromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarR4FromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarR4FromBool (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
SafeArrayGetVarType (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR8FromUI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR8FromI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR8FromI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR8FromR4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR8FromCy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR8FromDate (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR8FromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarR8FromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarR8FromBool (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDateFromUI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDateFromI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDateFromI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDateFromR4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDateFromR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDateFromCy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDateFromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarDateFromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarDateFromBool (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarCyFromUI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarCyFromI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarCyFromI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarCyFromR4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarCyFromR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarCyFromDate (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarCyFromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarCyFromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyFromBool (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBstrFromUI1 (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBstrFromI2 (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBstrFromI4 (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBstrFromR4 (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBstrFromR8 (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBstrFromCy (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBstrFromDate (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBstrFromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarBstrFromBool (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBoolFromUI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBoolFromI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBoolFromI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBoolFromR4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBoolFromR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBoolFromDate (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBoolFromCy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBoolFromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBoolFromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarUI1FromI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI1FromI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI1FromR4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI1FromR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI1FromCy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI1FromDate (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI1FromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarUI1FromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarUI1FromBool (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
DispCallFunc (VOID)
{
  FIXME;
}

VOID STDCALL
VariantChangeTypeEx (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  FIXME;
}

VOID STDCALL
SafeArrayPtrOfIndex (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SysStringByteLen (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
SysAllocStringByteLen (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
CreateTypeLib (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
LoadTypeLib (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
LoadRegTypeLib (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  FIXME;
}

VOID STDCALL
RegisterTypeLib (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
QueryPathOfRegTypeLib (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  FIXME;
}

VOID STDCALL
LHashValOfNameSys (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
LHashValOfNameSysA (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
OaBuildVersion (VOID)
{
  FIXME;
}

VOID STDCALL
ClearCustData (VOID)
{
  FIXME;
}

VOID STDCALL
CreateTypeLib2 (VOID)
{
  FIXME;
}

VOID STDCALL
LoadTypeLibEx (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SystemTimeToVariantTime (VOID)
{
  FIXME;
}

VOID STDCALL
VariantTimeToSystemTime (VOID)
{
  FIXME;
}

VOID STDCALL
UnRegisterTypeLib (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  FIXME;
}

VOID STDCALL
VarDecFromUI1 (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromI2 (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromI4 (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromR4 (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromR8 (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromDate (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromCy (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromStr (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromBool (VOID)
{
  FIXME;
}

VOID STDCALL
VarI2FromI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI2FromUI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI2FromUI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI2FromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarI4FromI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI4FromUI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI4FromUI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI4FromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarR4FromI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR4FromUI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR4FromUI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR4FromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarR8FromI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR8FromUI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR8FromUI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarR8FromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarDateFromI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDateFromUI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDateFromUI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDateFromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyFromI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarCyFromUI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarCyFromUI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarCyFromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarBstrFromI1 (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBstrFromUI2 (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBstrFromUI4 (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarBstrFromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarBoolFromI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBoolFromUI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBoolFromUI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarBoolFromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarUI1FromI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI1FromUI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI1FromUI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI1FromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromI1 (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromUI2 (VOID)
{
  FIXME;
}

VOID STDCALL
VarDecFromUI4 (VOID)
{
  FIXME;
}

VOID STDCALL
VarI1FromUI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI1FromI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI1FromI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI1FromR4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI1FromR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI1FromDate (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI1FromCy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI1FromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarI1FromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarI1FromBool (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI1FromUI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI1FromUI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarI1FromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarUI2FromUI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI2FromI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI2FromI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI2FromR4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI2FromR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI2FromDate (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI2FromCy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI2FromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarUI2FromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarUI2FromBool (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI2FromI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI2FromUI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI2FromDec (VOID)
{
  FIXME;
}

VOID STDCALL
VarUI4FromUI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI4FromI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI4FromI4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI4FromR4 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI4FromR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI4FromDate (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI4FromCy (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI4FromStr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarUI4FromDisp (VOID)
{
  FIXME;
}

VOID STDCALL
VarUI4FromBool (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI4FromI1 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI4FromUI2 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarUI4FromDec (VOID)
{
  FIXME;
}

VOID STDCALL
BSTR_UserSize (VOID)
{
  FIXME;
}

VOID STDCALL
BSTR_UserMarshal (VOID)
{
  FIXME;
}

VOID STDCALL
BSTR_UserUnmarshal (VOID)
{
  FIXME;
}

VOID STDCALL
BSTR_UserFree (VOID)
{
  FIXME;
}

VOID STDCALL
VARIANT_UserSize (VOID)
{
  FIXME;
}

VOID STDCALL
VARIANT_UserMarshal (VOID)
{
  FIXME;
}

VOID STDCALL
VARIANT_UserUnmarshal (VOID)
{
  FIXME;
}

VOID STDCALL
VARIANT_UserFree (VOID)
{
  FIXME;
}

VOID STDCALL
LPSAFEARRAY_UserSize (VOID)
{
  FIXME;
}

VOID STDCALL
LPSAFEARRAY_UserMarshal (VOID)
{
  FIXME;
}

VOID STDCALL
LPSAFEARRAY_UserUnmarshal (VOID)
{
  FIXME;
}

VOID STDCALL
LPSAFEARRAY_UserFree (VOID)
{
  FIXME;
}

VOID STDCALL
LPSAFEARRAY_Size (VOID)
{
  FIXME;
}

VOID STDCALL
LPSAFEARRAY_Marshal (VOID)
{
  FIXME;
}

VOID STDCALL
LPSAFEARRAY_Unmarshal (VOID)
{
  FIXME;
}

VOID STDCALL
DllRegisterServer (VOID)
{
  FIXME;
}

VOID STDCALL
DllUnregisterServer (VOID)
{
  FIXME;
}

VOID STDCALL
VarDateFromUdate (VOID)
{
  FIXME;
}

VOID STDCALL
VarUdateFromDate (VOID)
{
  FIXME;
}

VOID STDCALL
GetAltMonthNames (VOID)
{
  FIXME;
}

VOID STDCALL
UserHWND_from_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserHWND_to_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserHWND_free_inst (VOID)
{
  FIXME;
}

VOID STDCALL
UserHWND_free_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserBSTR_from_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserBSTR_to_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserBSTR_free_inst (VOID)
{
  FIXME;
}

VOID STDCALL
UserBSTR_free_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserVARIANT_from_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserVARIANT_to_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserVARIANT_free_inst (VOID)
{
  FIXME;
}

VOID STDCALL
UserVARIANT_free_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserEXCEPINFO_from_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserEXCEPINFO_to_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserEXCEPINFO_free_inst (VOID)
{
  FIXME;
}

VOID STDCALL
UserEXCEPINFO_free_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserMSG_from_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserMSG_to_local (VOID)
{
  FIXME;
}

VOID STDCALL
UserMSG_free_inst (VOID)
{
  FIXME;
}

VOID STDCALL
UserMSG_free_local (VOID)
{
  FIXME;
}

VOID STDCALL
DllCanUnloadNow (VOID)
{
  FIXME;
}

VOID STDCALL
SafeArrayCreateVector (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
SafeArrayCopyData (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VectorFromBstr (VOID)
{
  FIXME;
}

VOID STDCALL
BstrFromVector (VOID)
{
  FIXME;
}

VOID STDCALL
OleIconToCursor (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
OleCreatePropertyFrameIndirect (DWORD Unknown1)
{
  FIXME;
}

VOID STDCALL
OleCreatePropertyFrame (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, 
		DWORD Unknown5, DWORD Unknown6, DWORD Unknown7, DWORD Unknown8, 
		DWORD Unknown9, DWORD Unknown10, DWORD Unknown11)
{
  FIXME;
}

VOID STDCALL
OleLoadPicture (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  FIXME;
}

VOID STDCALL
OleCreatePictureIndirect (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
OleCreateFontIndirect (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
OleTranslateColor (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
OleLoadPictureFile (VOID)
{
  FIXME;
}

VOID STDCALL
OleSavePictureFile (VOID)
{
  FIXME;
}

VOID STDCALL
OleLoadPicturePath (VOID)
{
  FIXME;
}

VOID STDCALL
OleLoadPictureEx (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7, DWORD Unknown8)
{
  FIXME;
}

VOID STDCALL
GetRecordInfoFromGuids (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  FIXME;
}

VOID STDCALL
GetRecordInfoFromTypeInfo (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
OleLoadPictureFileEx (VOID)
{
  FIXME;
}

VOID STDCALL
SafeArrayAllocDescriptorEx (VOID)
{
  FIXME;
}

VOID STDCALL
SafeArrayCreateEx (VOID)
{
  FIXME;
}

VOID STDCALL
SafeArrayCreateVectorEx (VOID)
{
  FIXME;
}

VOID STDCALL
SafeArrayGetIID (VOID)
{
  FIXME;
}

VOID STDCALL
SafeArrayGetRecordInfo (VOID)
{
  FIXME;
}

VOID STDCALL
SafeArraySetIID (VOID)
{
  FIXME;
}

VOID STDCALL
SafeArraySetRecordInfo (VOID)
{
  FIXME;
}

VOID STDCALL
VarAbs (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarAdd (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarAnd (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarBstrCat (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarBstrCmp (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarCat (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarCmp (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarCyAbs (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyAdd (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyCmp (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyCmpR8 (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyFix (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyInt (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyMul (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyMulI4 (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyNeg (VOID)
{
  FIXME;
}

VOID STDCALL
VarCyRound (VOID)
{
  FIXME;
}

VOID STDCALL
VarCySub (VOID)
{
  FIXME;
}

VOID STDCALL
VarDateFromUdateEx (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarDecAbs (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDecAdd (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarDecCmp (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDecCmpR8 (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDecDiv (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarDecFix (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDecInt (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDecMul (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarDecNeg (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarDecRound (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarDecSub (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarDiv (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarEqv (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarFix (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarFormat (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  FIXME;
}

VOID STDCALL
VarFormatCurrency (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7)
{
  FIXME;
}

VOID STDCALL
VarFormatDateTime (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarFormatFromTokens (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  FIXME;
}

VOID STDCALL
VarFormatNumber (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7)
{
  FIXME;
}

VOID STDCALL
VarFormatPercent (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7)
{
  FIXME;
}

VOID STDCALL
VarIdiv (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarImp (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarInt (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarMod (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarMonthName (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  FIXME;
}

VOID STDCALL
VarMul (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarNeg (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarNot (DWORD Unknown1, DWORD Unknown2)
{
  FIXME;
}

VOID STDCALL
VarOr (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarPow (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarR4CmpR8 (VOID)
{
  FIXME;
}

VOID STDCALL
VarR8Pow (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarR8Round (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarRound (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarSub (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
VarTokenizeFormatString (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7)
{
  FIXME;
}

VOID STDCALL
VarWeekdayName (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  FIXME;
}

VOID STDCALL
VarXor (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  FIXME;
}

VOID STDCALL
UnknownOrdStub (VOID)
{
  FIXME;
}

