/*
 * Implementation of some printer driver bits
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 Huw Davies
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winspool.h"
#include "winerror.h"
#include "wine/debug.h"
#include "gdi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(print);

/******************************************************************
 * GdiGetSpoolMessage [GDI32.@]
 *
 */
DWORD WINAPI GdiGetSpoolMessage(LPVOID ptr1, DWORD data2, LPVOID ptr3, DWORD data4)
{
    TRACE("(%p 0x%x %p 0x%x) stub\n", ptr1, data2, ptr3, data4);
    /* avoid 100% cpu usage with spoolsv.exe from w2k
      (spoolsv.exe from xp does Sleep 1000/1500/2000 in a loop) */
    Sleep(500);
    return 0;
}

/******************************************************************
 * GdiInitSpool [GDI32.@]
 *
 */
DWORD WINAPI GdiInitSpool(void)
{
    FIXME("stub\n");
    return TRUE;
}

/******************************************************************
 *                  StartDocW  [GDI32.@]
 *
 * StartDoc calls the STARTDOC Escape with the input data pointing to DocName
 * and the output data (which is used as a second input parameter).pointing at
 * the whole docinfo structure.  This seems to be an undocumented feature of
 * the STARTDOC Escape.
 *
 * Note: we now do it the other way, with the STARTDOC Escape calling StartDoc.
 */
INT WINAPI StartDocW(HDC hdc, const DOCINFOW* doc)
{
    INT ret = 0;
    DC *dc = get_dc_ptr( hdc );

    TRACE("DocName = %s Output = %s Datatype = %s\n",
          debugstr_w(doc->lpszDocName), debugstr_w(doc->lpszOutput),
          debugstr_w(doc->lpszDatatype));

    if(!dc) return SP_ERROR;

    if (dc->pAbortProc && !dc->pAbortProc( hdc, 0 ))
    {
        release_dc_ptr( dc );
        return ret;
    }

    if (dc->funcs->pStartDoc) ret = dc->funcs->pStartDoc( dc->physDev, doc );
    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 *                  StartDocA [GDI32.@]
 *
 */
INT WINAPI StartDocA(HDC hdc, const DOCINFOA* doc)
{
    LPWSTR szDocName = NULL, szOutput = NULL, szDatatype = NULL;
    DOCINFOW docW;
    INT ret, len;

    docW.cbSize = doc->cbSize;
    if (doc->lpszDocName)
    {
        len = MultiByteToWideChar(CP_ACP,0,doc->lpszDocName,-1,NULL,0);
        szDocName = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,doc->lpszDocName,-1,szDocName,len);
    }
    if (doc->lpszOutput)
    {
        len = MultiByteToWideChar(CP_ACP,0,doc->lpszOutput,-1,NULL,0);
        szOutput = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,doc->lpszOutput,-1,szOutput,len);
    }
    if (doc->lpszDatatype)
    {
        len = MultiByteToWideChar(CP_ACP,0,doc->lpszDatatype,-1,NULL,0);
        szDatatype = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,doc->lpszDatatype,-1,szDatatype,len);
    }

    docW.lpszDocName = szDocName;
    docW.lpszOutput = szOutput;
    docW.lpszDatatype = szDatatype;
    docW.fwType = doc->fwType;

    ret = StartDocW(hdc, &docW);

    HeapFree( GetProcessHeap(), 0, szDocName );
    HeapFree( GetProcessHeap(), 0, szOutput );
    HeapFree( GetProcessHeap(), 0, szDatatype );

    return ret;
}


/******************************************************************
 *                  EndDoc  [GDI32.@]
 *
 */
INT WINAPI EndDoc(HDC hdc)
{
    INT ret = 0;
    DC *dc = get_dc_ptr( hdc );
    if(!dc) return SP_ERROR;

    if (dc->funcs->pEndDoc) ret = dc->funcs->pEndDoc( dc->physDev );
    release_dc_ptr( dc );
    return ret;
}


/******************************************************************
 *                  StartPage  [GDI32.@]
 *
 */
INT WINAPI StartPage(HDC hdc)
{
    INT ret = 1;
    DC *dc = get_dc_ptr( hdc );
    if(!dc) return SP_ERROR;

    if(dc->funcs->pStartPage)
        ret = dc->funcs->pStartPage( dc->physDev );
    else
        FIXME("stub\n");
    release_dc_ptr( dc );
    return ret;
}


/******************************************************************
 *                  EndPage  [GDI32.@]
 *
 */
INT WINAPI EndPage(HDC hdc)
{
    INT ret = 0;
    DC *dc = get_dc_ptr( hdc );
    if(!dc) return SP_ERROR;

    if (dc->funcs->pEndPage) ret = dc->funcs->pEndPage( dc->physDev );
    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 *                 AbortDoc  [GDI32.@]
 */
INT WINAPI AbortDoc(HDC hdc)
{
    INT ret = 0;
    DC *dc = get_dc_ptr( hdc );
    if(!dc) return SP_ERROR;

    if (dc->funcs->pAbortDoc) ret = dc->funcs->pAbortDoc( dc->physDev );
    release_dc_ptr( dc );
    return ret;
}


/**********************************************************************
 *           SetAbortProc   (GDI32.@)
 *
 */
INT WINAPI SetAbortProc(HDC hdc, ABORTPROC abrtprc)
{
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;
    dc->pAbortProc = abrtprc;
    release_dc_ptr( dc );
    return TRUE;
}
