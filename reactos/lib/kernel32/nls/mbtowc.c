/*
 * COPYRIGHT:       See COPYING in the top level directory
		    Addition copyrights might be specified in LGPL.c
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/nls/mbtowc.c
 * PURPOSE:         National language support functions
 * PROGRAMMER:      Boudewijn ( ariadne@xs4all.nl)
 * UPDATE HISTORY:  Modified from Onno Hovers wfc. ( 08/02/99 )
 *                  
 */
/*
 * nls/mbtowc.c
 * Copyright (c) 1996, Onno Hovers, All rights reserved
 */
 
#include <windows.h>
#include <kernel32/nls.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>

 
INT MultiByteToWideChar(UINT cpid, DWORD flags, LPCSTR src, int srclen,
                        LPWSTR dest, int destlen)
{
   PCODEPAGE pcodepage;
   INT       copylen;
   INT       retlen;  
   WCHAR     **atou;
   CHAR      c;  
  
   aprintf("MultiByteToWideChar( %u, 0x%lX, %s, %d, 0x%lX, %d )\n", 
           cpid, flags, src, srclen, (ULONG) dest, destlen);
    
   /* get codepage */
   switch(cpid)
   {
      case CP_ACP:   pcodepage=GetThreadLocale()->OemCodePage;  break;
      case CP_OEMCP: pcodepage=GetThreadLocale()->AnsiCodePage; break;
      case CP_MACCP: pcodepage=&__CP10000;   break;
      default:
         pcodepage=__CPFirst;
         while((pcodepage)&&(pcodepage->Id!=cpid))
            pcodepage=pcodepage->Next;
   } 
   if(pcodepage==NULL)
      { SetLastError(ERROR_INVALID_PARAMETER); return 0; }
      
   /* get conversion table */
   atou=pcodepage->ToUnicode;
   
   if(destlen!=0)
   {
      /* how long are we ?? */   
      if(srclen==0)
         srclen=strlen(src);
   
      copylen=min(srclen,destlen);
      retlen=copylen;
      
      /* XXX: maybe some inline assembly to speed things up, here ??? */
      while(copylen>0)
      {
         c=*src;
         *dest=atou [c>>5] [(c)&0x1F];
         src++;
         dest++;
         copylen--;       
      }
   }
   else
   {
      /* XXXX: composites etc. */
      retlen = strlen(src);
   }
   return retlen;
}                         