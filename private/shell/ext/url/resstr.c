/*
 * resstr.c - Return code to string translation routines.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include <intshcut.h>

#ifdef DEBUG


/* Macros
 *********/

/*
 * macro for simplifying result to string translation, assumes result string
 * pointer pcsz
 */

#define STRING_CASE(val)               case val: pcsz = #val; break


/****************************** Public Functions *****************************/


PUBLIC_CODE PCSTR GetINTString(int n)
{
   static char s_rgchINT[] = "-2147483646";

   wsprintf(s_rgchINT, "%d", n);

   ASSERT(IS_VALID_STRING_PTR(s_rgchINT, CSTR));

   return(s_rgchINT);
}


PUBLIC_CODE PCSTR GetULONGString(ULONG ul)
{
   static char s_rgchULONG[] = "4294967295";

   wsprintf(s_rgchULONG, "%lx", ul);

   ASSERT(IS_VALID_STRING_PTR(s_rgchULONG, CSTR));

   return(s_rgchULONG);
}


PUBLIC_CODE PCSTR GetBOOLString(BOOL bResult)
{
   PCSTR pcsz;

   pcsz = bResult ? "TRUE" : "FALSE";

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}


PUBLIC_CODE PCSTR GetClipboardFormatNameString(UINT ucf)
{
   PCSTR pcsz;
   static char s_szCFName[MAX_PATH_LEN];

   switch (ucf)
   {
      STRING_CASE(CF_TEXT);
      STRING_CASE(CF_BITMAP);
      STRING_CASE(CF_METAFILEPICT);
      STRING_CASE(CF_SYLK);
      STRING_CASE(CF_DIF);
      STRING_CASE(CF_TIFF);
      STRING_CASE(CF_OEMTEXT);
      STRING_CASE(CF_DIB);
      STRING_CASE(CF_PALETTE);
      STRING_CASE(CF_PENDATA);
      STRING_CASE(CF_RIFF);
      STRING_CASE(CF_WAVE);
      STRING_CASE(CF_UNICODETEXT);
      STRING_CASE(CF_ENHMETAFILE);
      STRING_CASE(CF_HDROP);
      STRING_CASE(CF_LOCALE);
      STRING_CASE(CF_MAX);
      STRING_CASE(CF_OWNERDISPLAY);
      STRING_CASE(CF_DSPTEXT);
      STRING_CASE(CF_DSPBITMAP);
      STRING_CASE(CF_DSPMETAFILEPICT);
      STRING_CASE(CF_DSPENHMETAFILE);

      default:
         if (! GetClipboardFormatName(ucf, s_szCFName, sizeof(s_szCFName)))
            lstrcpy(s_szCFName, "UNKNOWN CLIPBOARD FORMAT");
         pcsz = s_szCFName;
         break;
   }

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}


PUBLIC_CODE PCSTR GetCOMPARISONRESULTString(COMPARISONRESULT cr)
{
   PCSTR pcsz;

   switch (cr)
   {
      STRING_CASE(CR_FIRST_SMALLER);
      STRING_CASE(CR_FIRST_LARGER);
      STRING_CASE(CR_EQUAL);

      default:
         ERROR_OUT(("GetCOMPARISONRESULTString() called on unknown COMPARISONRESULT %d.",
                    cr));
         pcsz = "UNKNOWN COMPARISONRESULT";
         break;
   }

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}


#ifdef INC_OLE2

PUBLIC_CODE PCSTR GetHRESULTString(HRESULT hr)
{
   PCSTR pcsz;
   static char s_rgchHRESULT[] = "0x12345678";

   switch (hr)
   {
      STRING_CASE(S_OK);
      STRING_CASE(S_FALSE);

      STRING_CASE(DRAGDROP_S_CANCEL);
      STRING_CASE(DRAGDROP_S_DROP);
      STRING_CASE(DRAGDROP_S_USEDEFAULTCURSORS);

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

      STRING_CASE(DV_E_DVASPECT);
      STRING_CASE(DV_E_LINDEX);
      STRING_CASE(DV_E_TYMED);
      STRING_CASE(DV_E_FORMATETC);

#ifdef __INTSHCUT_H__

      STRING_CASE(E_FLAGS);

      STRING_CASE(URL_E_INVALID_SYNTAX);
      STRING_CASE(URL_E_UNREGISTERED_PROTOCOL);

      STRING_CASE(IS_E_EXEC_FAILED);

      STRING_CASE(E_FILE_NOT_FOUND);
      STRING_CASE(E_PATH_NOT_FOUND);

#endif

      default:
         wsprintf(s_rgchHRESULT, "%#lx", hr);
         pcsz = s_rgchHRESULT;
         break;
   }

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(pcsz);
}

#endif   /* INC_OLE2 */

#endif   /* DEBUG */

