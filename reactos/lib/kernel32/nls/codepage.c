/*
 * COPYRIGHT:       See COPYING in the top level directory
		    Addition copyrights might be specified in LGPL.c
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/nls/codepage.c
 * PURPOSE:         National language support functions
 * PROGRAMMER:      Boudewijn ( ariadne@xs4all.nl)
 * UPDATE HISTORY:  Modified from Onno Hovers wfc. ( 08/02/99 )
		    Modified from wine. ( 08/02/99 )
 *                  
 */

/*
 * nls/codepage.c ( Onno Hovers )
 *
 *
 */

/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */
#include <windows.h>
#include <kernel32/nls.h>
#include <kernel32/thread.h>
#include <string.h>


UINT STDCALL GetACP(void)
{
   //aprintf("GetACP()\n");
   
   /* XXX: read from registry, take this as default */
   return GetThreadLocale()->AnsiCodePage->Id;
}

UINT STDCALL GetOEMCP(void)
{
   //aprintf("GetOEMCP()\n");
   /* XXX: read from registry, take this as default */
   return GetThreadLocale()->OemCodePage->Id;
}

WINBOOL STDCALL IsValidCodePage(UINT codepage)
{
   PCODEPAGE pcp;
 
   //aprintf("IsValidCodePage( %u )\n", codepage);  
   
   switch(codepage)
   {
      case CP_ACP:   return TRUE;
      case CP_OEMCP: return TRUE; 
      case CP_MACCP: return TRUE;
      default:
         pcp=__CPFirst;
         while((pcp)&&(pcp->Id!=codepage))
            pcp=pcp->Next;
         return pcp ? TRUE : FALSE;      
   }
}

WINBOOL GetCPInfo(UINT codepage, LPCPINFO pcpinfo)
{
    PCODEPAGE pcp;
   
 //   aprintf("GetCPInfo( %u, 0x%lX )\n", codepage, (ULONG) pcpinfo);
    
    pcp=__CPFirst;
    while((pcp)&&(pcp->Id!=codepage))
            pcp=pcp->Next;  
    if(!pcp) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(!pcp->Info) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
       
    *pcpinfo = *pcp->Info;
    return TRUE;
}



WINBOOL 
STDCALL 
IsDBCSLeadByteEx( UINT codepage, BYTE testchar )
{
    CPINFO cpinfo;
    int i;

    GetCPInfo(codepage, &cpinfo);
    for (i = 0 ; i < sizeof(cpinfo.LeadByte)/sizeof(cpinfo.LeadByte[0]); i+=2)
    {
	if (cpinfo.LeadByte[i] == 0)
            return FALSE;
	if (cpinfo.LeadByte[i] <= testchar && testchar <= cpinfo.LeadByte[i+1])
            return TRUE;
    }
    return FALSE;
}




WINBOOL 
STDCALL 
IsDBCSLeadByte( BYTE testchar )
{
    return IsDBCSLeadByteEx(GetACP(), testchar);
}