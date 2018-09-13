/*
 * resstr.c - Return code to string translation routines.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#ifdef DEBUG

#pragma data_seg(DATA_SEG_READ_ONLY)

#include "debugstr.h"

#pragma data_seg()

#endif


/* Macros
 *********/

/*
 * macro for simplifying result to string translation, assumes result string
 * pointer pcsz
 */

#define STRING_CASE(val)               case val: pcsz = TEXT(#val); break


/****************************** Public Functions *****************************/


#ifdef DEBUG

/*
** GetINTString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetINTString(int n)
{
#pragma data_seg(DATA_SEG_SHARED)
   static TCHAR SrgchINT[] = TEXT("-2147483646");
#pragma data_seg()

   wsprintf(SrgchINT, TEXT("%d"), n);

   ASSERT(IS_VALID_STRING_PTR(SrgchINT, CSTR));

   return(SrgchINT);
}


/*
** GetULONGString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetULONGString(ULONG ul)
{
#pragma data_seg(DATA_SEG_SHARED)
   static TCHAR SrgchULONG[] = TEXT("4294967295");
#pragma data_seg()

   wsprintf(SrgchULONG, TEXT("%lx"), ul);

   ASSERT(IS_VALID_STRING_PTR(SrgchULONG, CSTR));

   return(SrgchULONG);
}


/*
** GetBOOLString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetBOOLString(BOOL bResult)
{
   LPCTSTR pcsz;

   if (bResult)
      pcsz = TEXT("TRUE");
   else
      pcsz = TEXT("FALSE");

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}


/*
** GetCOMPARISONRESULTString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetCOMPARISONRESULTString(COMPARISONRESULT cr)
{
   LPCTSTR pcsz;

   switch (cr)
   {
      STRING_CASE(CR_FIRST_SMALLER);
      STRING_CASE(CR_FIRST_LARGER);
      STRING_CASE(CR_EQUAL);

      default:
         ERROR_OUT((TEXT("GetCOMPARISONRESULTString() called on unknown COMPARISONRESULT %d."),
                    cr));
         pcsz = TEXT("UNKNOWN COMPARISONRESULT");
         break;
   }

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}


#ifdef INC_OLE2

/*
** GetHRESULTString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetHRESULTString(HRESULT hr)
{
   LPCTSTR pcsz;
#pragma data_seg(DATA_SEG_SHARED)
   static TCHAR SrgchHRESULT[] = TEXT("0x12345678");
#pragma data_seg()

   switch (hr)
   {
      STRING_CASE(S_OK);
      STRING_CASE(S_FALSE);

      STRING_CASE(E_UNEXPECTED);
      STRING_CASE(E_NOTIMPL);
      STRING_CASE(E_OUTOFMEMORY);
      STRING_CASE(E_INVALIDARG);
      STRING_CASE(E_NOINTERFACE);
      STRING_CASE(E_POINTER);
      STRING_CASE(E_HANDLE);
      STRING_CASE(E_ABORT);
      STRING_CASE(E_FAIL);
      STRING_CASE(E_ACCESSDENIED);

      STRING_CASE(CLASS_E_NOAGGREGATION);

      STRING_CASE(CO_E_NOTINITIALIZED);
      STRING_CASE(CO_E_ALREADYINITIALIZED);
      STRING_CASE(CO_E_INIT_ONLY_SINGLE_THREADED);

#ifdef __RECONCIL_H__

      STRING_CASE(REC_S_IDIDTHEUPDATES);
      STRING_CASE(REC_S_NOTCOMPLETE);
      STRING_CASE(REC_S_NOTCOMPLETEBUTPROPAGATE);

      STRING_CASE(REC_E_ABORTED);
      STRING_CASE(REC_E_NOCALLBACK);
      STRING_CASE(REC_E_NORESIDUES);
      STRING_CASE(REC_E_TOODIFFERENT);
      STRING_CASE(REC_E_INEEDTODOTHEUPDATES);

#endif   /* __RECONCIL_H__ */

      default:
         wsprintf(SrgchHRESULT, TEXT("%#lx"), hr);
         pcsz = SrgchHRESULT;
         break;
   }

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}

#endif   /* INC_OLE2 */


#ifdef __SYNCENG_H__

/*
** GetTWINRESULTString()
**
** Returns a pointer to the string name of a TWINRESULT return code.
**
** Arguments:     tr - return code to be translated
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetTWINRESULTString(TWINRESULT tr)
{
   LPCTSTR pcsz;

   ASSERT(tr >= 0);

   if (tr < ARRAY_ELEMENTS(rgcpcszTwinResult))
      pcsz = rgcpcszTwinResult[tr];
   else
   {
      ERROR_OUT((TEXT("GetTWINRESULTString() called on unrecognized TWINRESULT %ld."),
                 tr));
      pcsz = TEXT("UNKNOWN TWINRESULT");
   }

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}


/*
** GetCREATERECLISTPROCMSGString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetCREATERECLISTPROCMSGString(UINT uCreateRecListMsg)
{
   LPCTSTR pcsz;

   ASSERT(uCreateRecListMsg >= 0);

   if (uCreateRecListMsg < ARRAY_ELEMENTS(rgcpcszCreateRecListMsg))
      pcsz = rgcpcszCreateRecListMsg[uCreateRecListMsg];
   else
   {
      ERROR_OUT((TEXT("GetCREATERECLISTPROCMSGString() called on unrecognized RECSTATUSPROC message %u."),
                 uCreateRecListMsg));
      pcsz = TEXT("UNKNOWN RECSTATUSPROC MESSAGE");
   }

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}


/*
** GetRECSTATUSPROCMSGString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetRECSTATUSPROCMSGString(UINT uRecStatusMsg)
{
   LPCTSTR pcsz;

   ASSERT(uRecStatusMsg >= 0);

   if (uRecStatusMsg < ARRAY_ELEMENTS(rgcpcszRecStatusMsg))
      pcsz = rgcpcszRecStatusMsg[uRecStatusMsg];
   else
   {
      ERROR_OUT((TEXT("GetRECSTATUSPROCMSGString() called on unrecognized RECSTATUSPROC message %u."),
                 uRecStatusMsg));
      pcsz = TEXT("UNKNOWN RECSTATUSPROC MESSAGE");
   }

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}


/*
** GetRECNODESTATEString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetRECNODESTATEString(RECNODESTATE rnstate)
{
   LPCTSTR pcsz;

   switch (rnstate)
   {
      STRING_CASE(RNS_NEVER_RECONCILED);
      STRING_CASE(RNS_UNAVAILABLE);
      STRING_CASE(RNS_DOES_NOT_EXIST);
      STRING_CASE(RNS_DELETED);
      STRING_CASE(RNS_NOT_RECONCILED);
      STRING_CASE(RNS_UP_TO_DATE);
      STRING_CASE(RNS_CHANGED);

      default:
         ERROR_OUT((TEXT("GetRECNODESTATEString() called on unknown RECNODESTATE %d."),
                    rnstate));
         pcsz = TEXT("UNKNOWN RECNODESTATE");
         break;
   }

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}


/*
** GetRECNODEACTIONString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetRECNODEACTIONString(RECNODEACTION rnaction)
{
   LPCTSTR pcsz;

   switch (rnaction)
   {
      STRING_CASE(RNA_NOTHING);
      STRING_CASE(RNA_COPY_FROM_ME);
      STRING_CASE(RNA_COPY_TO_ME);
      STRING_CASE(RNA_MERGE_ME);
      STRING_CASE(RNA_DELETE_ME);

      default:
         ERROR_OUT((TEXT("GetRECNODEACTIONString() called on unknown RECNODEACTION %d."),
                    rnaction));
         pcsz = TEXT("UNKNOWN RECNODEACTION");
         break;
   }

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}

#endif   /* __SYNCENG_H__ */

#endif   /* DEBUG */

