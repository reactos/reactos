/*
 * COPYRIGHT:       See COPYING in the top level directory
		    Addition copyrights might be specified in LGPL.c
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/nls/wctomb.c
 * PURPOSE:         National language support functions
 * PROGRAMMER:      Boudewijn ( ariadne@xs4all.nl)
 * UPDATE HISTORY:  Modified from Onno Hovers wfc. ( 08/02/99 )
 *                  
 */

/*
 * nls/wctomb.c
 * Copyright (c) 1996, Onno Hovers, All rights reserved
 */
 
#include <windows.h>

#include <kernel32/nls.h>
#include <kernel32/thread.h>
#include <wchar.h>

#define NDEBUG
#include <kernel32/kernel32.h>

extern PLOCALE	__TebLocale;

#define GetTebLocale() __TebLocale
 
INT 
STDCALL
WideCharToMultiByte(UINT cpid, DWORD flags, LPCWSTR src, int srclen,
                        LPSTR dest, int destlen, LPCSTR pdefchar,
                        LPBOOL pdefused )
{
   PCODEPAGE pcodepage = __CPFirst;
   BOOL      defused=FALSE;   
   INT       copylen;
   INT       retlen;  
   CHAR      ***utoa;
   CHAR	     defchar='?';   
   CHAR      d;
   WCHAR     c;  
  
   DPRINT("WideCharToMultiByte()\n");
    
   /* get codepage */
   switch(cpid)
   {
      case CP_ACP:   pcodepage=GetTebLocale()->OemCodePage;  break;
      case CP_OEMCP: pcodepage=GetTebLocale()->AnsiCodePage; break;
      case CP_MACCP: pcodepage=&__CP10000;   break;
      default:
         pcodepage=__CPFirst;
         while((pcodepage)&&(pcodepage->Id!=cpid))
            pcodepage=pcodepage->Next;
   } 
   if(pcodepage==NULL)
      { SetLastError(ERROR_INVALID_PARAMETER); return 0; }
      
   /* get conversion table */
   utoa=pcodepage->FromUnicode;
   
   /* get default char */   
   if(pdefchar)
      defchar=*pdefchar;
   else
      defchar=pcodepage->Info->DefaultChar[0];
   
   
   if(destlen!=0)
   {
      /* how long are we ?? */   
      if(srclen==0)
         srclen=wcslen(src);
      
      copylen=min(srclen,destlen);
      retlen=copylen;
      
      /* XXX: maybe some inline assembly to speed things up, here ??? */
      while(copylen>0)
      {
         c=*src;
                 
         d=utoa[c>>10][(c>>5)&0x1F][(c)&0x1F];
         if((!d)&&(c))
         {
            d=defchar;
            defused=TRUE;
         }
         *dest=d;
         src++;
         dest++;
         copylen--;       
      }
   }
   else
   {
      /* XXXX: composites etc. */
      retlen = wcslen(src);
   }
   return retlen;
}                         