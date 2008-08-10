/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "msi.h"
#include "msipriv.h"
#include "msiserver.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static void MSI_ClosePreview( MSIOBJECTHDR *arg )
{
    MSIPREVIEW *preview = (MSIPREVIEW *) arg;

    msiobj_release( &preview->package->hdr );
}

MSIPREVIEW *MSI_EnableUIPreview( MSIDATABASE *db )
{
    MSIPREVIEW *preview = NULL;
    MSIPACKAGE *package;

    package = MSI_CreatePackage( db, NULL );
    if( package )
    {
        preview = alloc_msiobject( MSIHANDLETYPE_PREVIEW, sizeof (MSIPREVIEW),
                               MSI_ClosePreview );
        if( preview )
        {
            preview->package = package;
            preview->dialog = 0;
            msiobj_addref( &package->hdr );
        }
        msiobj_release( &package->hdr );
    }
    return preview;
}

UINT WINAPI MsiEnableUIPreview( MSIHANDLE hdb, MSIHANDLE* phPreview )
{
    MSIDATABASE *db;
    MSIPREVIEW *preview;
    UINT r = ERROR_FUNCTION_FAILED;

    TRACE("%ld %p\n", hdb, phPreview);

    db = msihandle2msiinfo( hdb, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        IWineMsiRemoteDatabase *remote_database;

        remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( hdb );
        if ( !remote_database )
            return ERROR_INVALID_HANDLE;

        *phPreview = 0;

        IWineMsiRemoteDatabase_Release( remote_database );
        WARN("MsiEnableUIPreview not allowed during a custom action!\n");

        return ERROR_FUNCTION_FAILED;
    }

    preview = MSI_EnableUIPreview( db );
    if( preview )
    {
        *phPreview = alloc_msihandle( &preview->hdr );
        msiobj_release( &preview->hdr );
        r = ERROR_SUCCESS;
        if (! *phPreview)
            r = ERROR_NOT_ENOUGH_MEMORY;
    }
    msiobj_release( &db->hdr );

    return r;
}

static UINT preview_event_handler( MSIPACKAGE *package, LPCWSTR event,
                                   LPCWSTR argument, msi_dialog *dialog )
{
    MESSAGE("Preview dialog event '%s' (arg='%s')\n",
            debugstr_w( event ), debugstr_w( argument ));
    return ERROR_SUCCESS;
}

UINT MSI_PreviewDialogW( MSIPREVIEW *preview, LPCWSTR szDialogName )
{
    msi_dialog *dialog = NULL;
    UINT r = ERROR_SUCCESS;

    if( preview->dialog )
        msi_dialog_destroy( preview->dialog );

    /* an empty name means we should just destroy the current preview dialog */
    if( szDialogName )
    {
        dialog = msi_dialog_create( preview->package, szDialogName, NULL,
                                    preview_event_handler );
        if( dialog )
            msi_dialog_do_preview( dialog );
        else
            r = ERROR_FUNCTION_FAILED;
    }
    preview->dialog = dialog;

    return r;
}

UINT WINAPI MsiPreviewDialogW( MSIHANDLE hPreview, LPCWSTR szDialogName )
{
    MSIPREVIEW *preview;
    UINT r;

    TRACE("%ld %s\n", hPreview, debugstr_w(szDialogName));

    preview = msihandle2msiinfo( hPreview, MSIHANDLETYPE_PREVIEW );
    if( !preview )
        return ERROR_INVALID_HANDLE;

    r = MSI_PreviewDialogW( preview, szDialogName );

    msiobj_release( &preview->hdr );

    return r;
}

UINT WINAPI MsiPreviewDialogA( MSIHANDLE hPreview, LPCSTR szDialogName )
{
    UINT r;
    LPWSTR strW = NULL;

    TRACE("%ld %s\n", hPreview, debugstr_a(szDialogName));

    if( szDialogName )
    {
        strW = strdupAtoW( szDialogName );
        if( !strW )
            return ERROR_OUTOFMEMORY;
    }
    r = MsiPreviewDialogW( hPreview, strW );
    msi_free( strW );
    return r;
}

UINT WINAPI MsiPreviewBillboardW( MSIHANDLE hPreview, LPCWSTR szControlName,
                                  LPCWSTR szBillboard)
{
    FIXME("%ld %s %s\n", hPreview, debugstr_w(szControlName),
          debugstr_w(szBillboard));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiPreviewBillboardA( MSIHANDLE hPreview, LPCSTR szControlName,
                                  LPCSTR szBillboard)
{
    FIXME("%ld %s %s\n", hPreview, debugstr_a(szControlName),
          debugstr_a(szBillboard));
    return ERROR_CALL_NOT_IMPLEMENTED;
}
