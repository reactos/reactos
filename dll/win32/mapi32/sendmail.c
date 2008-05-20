/*
 * MAPISendMail implementation
 *
 * Copyright 2005 Hans Leidekker
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

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "mapi.h"
#include "winreg.h"
#include "shellapi.h"
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

/**************************************************************************
 *  MAPISendMail	(MAPI32.211)
 *
 * Send a mail.
 *
 * PARAMS
 *  session  [I] Handle to a MAPI session.
 *  uiparam  [I] Parent window handle.
 *  message  [I] Pointer to a MAPIMessage structure.
 *  flags    [I] Flags.
 *  reserved [I] Reserved, pass 0.
 *
 * RETURNS
 *  Success: SUCCESS_SUCCESS
 *  Failure: MAPI_E_FAILURE
 *
 * NOTES
 *  This is a temporary hack. 
 */
ULONG WINAPI MAPISendMail( LHANDLE session, ULONG_PTR uiparam,
    lpMapiMessage message, FLAGS flags, ULONG reserved )
{
    ULONG ret = MAPI_E_FAILURE;
    unsigned int i, to_count = 0, cc_count = 0, bcc_count = 0;
    unsigned int to_size = 0, cc_size = 0, bcc_size = 0, subj_size, body_size;

    char *to = NULL, *cc = NULL, *bcc = NULL;
    const char *address, *subject, *body;
    static const char format[] =
        "mailto:\"%s\"?subject=\"%s\"&cc=\"%s\"&bcc=\"%s\"&body=\"%s\"";
    char *mailto = NULL, *escape = NULL;
    char empty_string[] = "";
    HRESULT res;
    DWORD size;

    TRACE( "(0x%08lx 0x%08lx %p 0x%08lx 0x%08x)\n", session, uiparam,
           message, flags, reserved );

    if (!message) return MAPI_E_FAILURE;

    for (i = 0; i < message->nRecipCount; i++)
    {
        if (!message->lpRecips)
        {
            WARN("No recipients found\n");
            return MAPI_E_FAILURE;
        }

        address = message->lpRecips[i].lpszAddress;
        if (address)
        {
            switch (message->lpRecips[i].ulRecipClass)
            {
            case MAPI_ORIG:
                TRACE( "From: %s\n", debugstr_a(address) );
                break;
            case MAPI_TO:
                TRACE( "To: %s\n", debugstr_a(address) );
                to_size += lstrlenA( address ) + 1;
                break;
            case MAPI_CC:
                TRACE( "Cc: %s\n", debugstr_a(address) );
                cc_size += lstrlenA( address ) + 1;
                break;
            case MAPI_BCC:
                TRACE( "Bcc: %s\n", debugstr_a(address) );
                bcc_size += lstrlenA( address ) + 1;
                break;
            default:
                TRACE( "Unknown recipient class: %d\n",
                       message->lpRecips[i].ulRecipClass );
            }
        }
        else
            FIXME("Name resolution and entry identifiers not supported\n");
    }
    if (message->nFileCount) FIXME("Ignoring attachments\n");

    subject = message->lpszSubject ? message->lpszSubject : "";
    body = message->lpszNoteText ? message->lpszNoteText : "";

    TRACE( "Subject: %s\n", debugstr_a(subject) );
    TRACE( "Body: %s\n", debugstr_a(body) );

    subj_size = lstrlenA( subject );
    body_size = lstrlenA( body );

    ret = MAPI_E_INSUFFICIENT_MEMORY;
    if (to_size)
    {
        to = HeapAlloc( GetProcessHeap(), 0, to_size );
        if (!to) goto exit;
        to[0] = 0;
    }
    if (cc_size)
    {
        cc = HeapAlloc( GetProcessHeap(), 0, cc_size );
        if (!cc) goto exit;
        cc[0] = 0;
    }
    if (bcc_size)
    {
        bcc = HeapAlloc( GetProcessHeap(), 0, bcc_size );
        if (!bcc) goto exit;
        bcc[0] = 0;
    }

    if (message->lpOriginator)
        TRACE( "From: %s\n", debugstr_a(message->lpOriginator->lpszAddress) );

    for (i = 0; i < message->nRecipCount; i++)
    {
        address = message->lpRecips[i].lpszAddress;
        if (address)
        {
            switch (message->lpRecips[i].ulRecipClass)
            {
            case MAPI_TO:
                if (to_count) lstrcatA( to, "," );
                lstrcatA( to, address );
                to_count++;
                break;
            case MAPI_CC:
                if (cc_count) lstrcatA( cc, "," );
                lstrcatA( cc, address );
                cc_count++;
                break;
            case MAPI_BCC:
                if (bcc_count) lstrcatA( bcc, "," );
                lstrcatA( bcc, address );
                bcc_count++;
                break;
            }
        }
    }
    ret = MAPI_E_FAILURE;
    size = sizeof(format) + to_size + cc_size + bcc_size + subj_size + body_size;
    
    mailto = HeapAlloc( GetProcessHeap(), 0, size );
    if (!mailto) goto exit;

    sprintf( mailto, format, to ? to : "", subject, cc ? cc : "", bcc ? bcc : "", body );

    size = 1;
    res = UrlEscapeA( mailto, empty_string, &size, URL_ESCAPE_SPACES_ONLY );
    if (res != E_POINTER) goto exit;

    escape = HeapAlloc( GetProcessHeap(), 0, size );
    if (!escape) goto exit;

    res = UrlEscapeA( mailto, escape, &size, URL_ESCAPE_SPACES_ONLY );
    if (res != S_OK) goto exit;

    if ((UINT_PTR)ShellExecuteA( NULL, "open", escape, NULL, NULL, 0 ) > 32)
        ret = SUCCESS_SUCCESS;

exit:
    HeapFree( GetProcessHeap(), 0, to );
    HeapFree( GetProcessHeap(), 0, cc );
    HeapFree( GetProcessHeap(), 0, bcc );
    HeapFree( GetProcessHeap(), 0, mailto );
    HeapFree( GetProcessHeap(), 0, escape );

    return ret;
}
