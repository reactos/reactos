/*
 *	OLECLI library
 *
 *	Copyright 1995	Martin von Loewis
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

/*	At the moment, these are only empty stubs.
 */

#include <stdarg.h>

#include "windef.h"
#include "wine/windef16.h"
#include "winbase.h"
#include "wingdi.h"
#include "wownt32.h"
#include "objbase.h"
#include "olecli.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct _OLEOBJECTVTBL {
    void *         (CALLBACK *QueryProtocol)(_LPOLEOBJECT,LPCOLESTR16);
    OLESTATUS      (CALLBACK *Release)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *Show)(_LPOLEOBJECT,BOOL16);
    OLESTATUS      (CALLBACK *DoVerb)(_LPOLEOBJECT,UINT16,BOOL16,BOOL16);
    OLESTATUS      (CALLBACK *GetData)(_LPOLEOBJECT,OLECLIPFORMAT,HANDLE16 *);
    OLESTATUS      (CALLBACK *SetData)(_LPOLEOBJECT,OLECLIPFORMAT,HANDLE16);
    OLESTATUS      (CALLBACK *SetTargetDevice)(_LPOLEOBJECT,HGLOBAL16);
    OLESTATUS      (CALLBACK *SetBounds)(_LPOLEOBJECT,LPRECT16);
    OLESTATUS      (CALLBACK *EnumFormats)(_LPOLEOBJECT,OLECLIPFORMAT);
    OLESTATUS      (CALLBACK *SetColorScheme)(_LPOLEOBJECT,struct tagLOGPALETTE*);
    OLESTATUS      (CALLBACK *Delete)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *SetHostNames)(_LPOLEOBJECT,LPCOLESTR16,LPCOLESTR16);
    OLESTATUS      (CALLBACK *SaveToStream)(_LPOLEOBJECT,struct _OLESTREAM*);
    OLESTATUS      (CALLBACK *Clone)(_LPOLEOBJECT,LPOLECLIENT,LHCLIENTDOC,LPCOLESTR16,_LPOLEOBJECT *);
    OLESTATUS      (CALLBACK *CopyFromLink)(_LPOLEOBJECT,LPOLECLIENT,LHCLIENTDOC,LPCOLESTR16,_LPOLEOBJECT *);
    OLESTATUS      (CALLBACK *Equal)(_LPOLEOBJECT,_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *CopyToClipBoard)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *Draw)(_LPOLEOBJECT,HDC16,LPRECT16,LPRECT16,HDC16);
    OLESTATUS      (CALLBACK *Activate)(_LPOLEOBJECT,UINT16,BOOL16,BOOL16,HWND16,LPRECT16);
    OLESTATUS      (CALLBACK *Execute)(_LPOLEOBJECT,HGLOBAL16,UINT16);
    OLESTATUS      (CALLBACK *Close)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *Update)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *Reconnect)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *ObjectConvert)(_LPOLEOBJECT,LPCOLESTR16,LPOLECLIENT,LHCLIENTDOC,LPCOLESTR16,_LPOLEOBJECT*);
    OLESTATUS      (CALLBACK *GetLinkUpdateOptions)(_LPOLEOBJECT,LPOLEOPT_UPDATE);
    OLESTATUS      (CALLBACK *SetLinkUpdateOptions)(_LPOLEOBJECT,OLEOPT_UPDATE);
    OLESTATUS      (CALLBACK *Rename)(_LPOLEOBJECT,LPCOLESTR16);
    OLESTATUS      (CALLBACK *QueryName)(_LPOLEOBJECT,LPSTR,LPUINT16);
    OLESTATUS      (CALLBACK *QueryType)(_LPOLEOBJECT,LPLONG);
    OLESTATUS      (CALLBACK *QueryBounds)(_LPOLEOBJECT,LPRECT16);
    OLESTATUS      (CALLBACK *QuerySize)(_LPOLEOBJECT,LPDWORD);
    OLESTATUS      (CALLBACK *QueryOpen)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *QueryOutOfDate)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *QueryReleaseStatus)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *QueryReleaseError)(_LPOLEOBJECT);
    OLE_RELEASE_METHOD (CALLBACK *QueryReleaseMethod)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *RequestData)(_LPOLEOBJECT,OLECLIPFORMAT);
    OLESTATUS      (CALLBACK *ObjectLong)(_LPOLEOBJECT,UINT16,LPLONG);
} OLEOBJECTVTBL;
typedef OLEOBJECTVTBL *LPOLEOBJECTVTBL;

typedef struct _OLEOBJECT
{
    const OLEOBJECTVTBL *lpvtbl;
} OLEOBJECT;

static LONG OLE_current_handle;

/******************************************************************************
 *		OleSavedClientDoc	[OLECLI32.45]
 */
OLESTATUS WINAPI OleSavedClientDoc(LHCLIENTDOC hDoc)
{
    FIXME("(%ld: stub\n", hDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleRegisterClientDoc	[OLECLI32.41]
 */
OLESTATUS WINAPI OleRegisterClientDoc(LPCSTR classname, LPCSTR docname,
                                        LONG reserved, LHCLIENTDOC *hRet )
{
    FIXME("(%s,%s,...): stub\n",classname,docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/******************************************************************************
 *		OleRenameClientDoc	[OLECLI32.43]
 */
OLESTATUS WINAPI OleRenameClientDoc(LHCLIENTDOC hDoc, LPCSTR newName)
{
    FIXME("(%ld,%s,...): stub\n",hDoc, newName);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeClientDoc	[OLECLI32.42]
 */
OLESTATUS WINAPI OleRevokeClientDoc(LHCLIENTDOC hServerDoc)
{
    FIXME("(%ld): stub\n",hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 *           OleCreateLinkFromClip	[OLECLI32.11]
 */
OLESTATUS WINAPI OleCreateLinkFromClip(
	LPCSTR name,LPOLECLIENT olecli,LHCLIENTDOC hclientdoc,LPCSTR xname,
	_LPOLEOBJECT *lpoleob,OLEOPT_RENDER render,OLECLIPFORMAT clipformat
) {
	FIXME("(%s,%p,%08lx,%s,%p,%d,%ld): stub!\n",
	      name,olecli,hclientdoc,xname,lpoleob,render,clipformat);
	return OLE_OK;
}

/******************************************************************************
 *           OleQueryLinkFromClip	[OLECLI32.9]
 */
OLESTATUS WINAPI OleQueryLinkFromClip(LPCSTR name,OLEOPT_RENDER render,OLECLIPFORMAT clipformat) {
	FIXME("(%s,%d,%ld): stub!\n",name,render,clipformat);
	return OLE_OK;
}

/******************************************************************************
 *           OleQueryCreateFromClip	[OLECLI32.10]
 */
OLESTATUS WINAPI OleQueryCreateFromClip(LPCSTR name,OLEOPT_RENDER render,OLECLIPFORMAT clipformat) {
	FIXME("(%s,%d,%ld): stub!\n",name,render,clipformat);
	return OLE_OK;
}

/******************************************************************************
 *		OleIsDcMeta	[OLECLI32.60]
 */
BOOL WINAPI OleIsDcMeta(HDC hdc)
{
    TRACE("(%p)\n",hdc);
    return GetObjectType( hdc ) == OBJ_METADC;
}

/******************************************************************************
 *		OleSetHostNames	[OLECLI32.15]
 */
OLESTATUS WINAPI OleSetHostNames(_LPOLEOBJECT oleob,LPCSTR name1,LPCSTR name2) {
	FIXME("(%p,%s,%s): stub\n",oleob,name1,name2);
	return OLE_OK;
}

/******************************************************************************
 *		OleQueryType	[OLECLI32.14]
 */
OLESTATUS WINAPI OleQueryType(_LPOLEOBJECT oleob,LONG*xlong) {
	FIXME("(%p,%p): stub!\n",oleob,xlong);
	if (!oleob)
		return 0x10;
	TRACE("Calling OLEOBJECT.QueryType (%p) (%p,%p)\n",
	      oleob->lpvtbl->QueryType,oleob,xlong);
	return oleob->lpvtbl->QueryType(oleob,xlong);
}

/******************************************************************************
 *		OleCreateFromClip	[OLECLI32.12]
 */
OLESTATUS WINAPI OleCreateFromClip(
	LPCSTR name,LPOLECLIENT olecli,LHCLIENTDOC hclientdoc,LPCSTR xname,
	_LPOLEOBJECT *lpoleob,OLEOPT_RENDER render, OLECLIPFORMAT clipformat
) {
	FIXME("(%s,%p,%08lx,%s,%p,%d,%ld): stub!\n",
	      name,olecli,hclientdoc,xname,lpoleob,render,clipformat);
	/* clipb type, object kreieren entsprechend etc. */
	return OLE_OK;
}
