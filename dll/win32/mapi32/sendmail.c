/*
 * MAPISendMail implementation
 *
 * Copyright 2005 Hans Leidekker
 * Copyright 2009 Owen Rudge for CodeWeavers
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


#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "objbase.h"
#include "objidl.h"
#include "mapi.h"
#include "mapix.h"
#include "mapiutil.h"
#include "mapidefs.h"
#include "winreg.h"
#include "shellapi.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "util.h"
#include "res.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

#define READ_BUF_SIZE    4096

#define STORE_UNICODE_OK  0x00040000

static LPSTR convert_from_unicode(LPCWSTR wstr)
{
    LPSTR str;
    DWORD len;

    if (!wstr)
        return NULL;

    len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    str = HeapAlloc(GetProcessHeap(), 0, len);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);

    return str;
}

static LPWSTR convert_to_unicode(LPSTR str)
{
    LPWSTR wstr;
    DWORD len;

    if (!str)
        return NULL;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    wstr = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, len);

    return wstr;
}

/*
   Internal function to send a message via Extended MAPI. Wrapper around the Simple
   MAPI function MAPISendMail.
*/
static ULONG sendmail_extended_mapi(LHANDLE mapi_session, ULONG_PTR uiparam, lpMapiMessageW message,
    FLAGS flags)
{
    ULONG tags[] = {1, 0};
    char *subjectA = NULL, *bodyA = NULL;
    ULONG retval = MAPI_E_FAILURE;
    IMAPISession *session = NULL;
    BOOL unicode_aware = FALSE;
    IMAPITable* msg_table;
    LPSRowSet rows = NULL;
    IMsgStore* msg_store;
    IMAPIFolder* folder = NULL, *draft_folder = NULL;
    LPENTRYID entry_id;
    LPSPropValue props;
    ULONG entry_len;
    DWORD obj_type;
    IMessage* msg;
    ULONG values;
    HRESULT ret;

    TRACE("Using Extended MAPI wrapper for MAPISendMail\n");

    /* Attempt to log on via Extended MAPI */

    ret = MAPILogonEx(0, NULL, NULL, MAPI_EXTENDED | MAPI_USE_DEFAULT | MAPI_NEW_SESSION, &session);
    TRACE("MAPILogonEx: %lx\n", ret);

    if (ret != S_OK)
    {
        retval = MAPI_E_LOGIN_FAILURE;
        goto cleanup;
    }

    /* Open the default message store */

    if (IMAPISession_GetMsgStoresTable(session, 0, &msg_table) == S_OK)
    {
        /* We want the default store */
        SizedSPropTagArray(2, columns) = {2, {PR_ENTRYID, PR_DEFAULT_STORE}};

        /* Set the columns we want */
        if (IMAPITable_SetColumns(msg_table, (LPSPropTagArray) &columns, 0) == S_OK)
        {
            while (1)
            {
                if (IMAPITable_QueryRows(msg_table, 1, 0, &rows) != S_OK)
                {
                    MAPIFreeBuffer(rows);
                    rows = NULL;
                }
                else if (rows->cRows != 1)
                {
                    FreeProws(rows);
                    rows = NULL;
                }
                else
                {
                    /* If it's not the default store, try the next row */
                    if (!rows->aRow[0].lpProps[1].Value.b)
                    {
                        FreeProws(rows);
                        continue;
                    }
                }

                break;
            }
        }

        IMAPITable_Release(msg_table);
    }

    /* Did we manage to get the right store? */
    if (!rows)
        goto logoff;

    /* Open the message store */
    IMAPISession_OpenMsgStore(session, 0, rows->aRow[0].lpProps[0].Value.bin.cb,
                              (ENTRYID *) rows->aRow[0].lpProps[0].Value.bin.lpb, NULL,
                              MDB_NO_DIALOG | MAPI_BEST_ACCESS, &msg_store);

    /* We don't need this any more */
    FreeProws(rows);

    /* Check if the message store supports Unicode */
    tags[1] = PR_STORE_SUPPORT_MASK;
    ret = IMsgStore_GetProps(msg_store, (LPSPropTagArray) tags, 0, &values, &props);

    if ((ret == S_OK) && (props[0].Value.l & STORE_UNICODE_OK))
        unicode_aware = TRUE;
    else
    {
        /* Don't convert to ANSI */
        if (flags & MAPI_FORCE_UNICODE)
        {
            WARN("No Unicode-capable mail client, and MAPI_FORCE_UNICODE is specified. MAPISendMail failed.\n");
            retval = MAPI_E_UNICODE_NOT_SUPPORTED;
            IMsgStore_Release(msg_store);
            goto logoff;
        }
    }

    /* First open the inbox, from which the drafts folder can be opened */
    if (IMsgStore_GetReceiveFolder(msg_store, NULL, 0, &entry_len, &entry_id, NULL) == S_OK)
    {
        IMsgStore_OpenEntry(msg_store, entry_len, entry_id, NULL, 0, &obj_type, (LPUNKNOWN*) &folder);
        MAPIFreeBuffer(entry_id);
    }

    tags[1] = PR_IPM_DRAFTS_ENTRYID;

    /* Open the drafts folder, or failing that, try asking the message store for the outbox */
    if ((folder == NULL) || ((ret = IMAPIFolder_GetProps(folder, (LPSPropTagArray) tags, 0, &values, &props)) != S_OK))
    {
        TRACE("Unable to open Drafts folder; opening Outbox instead\n");
        tags[1] = PR_IPM_OUTBOX_ENTRYID;
        ret = IMsgStore_GetProps(msg_store, (LPSPropTagArray) tags, 0, &values, &props);
    }

    if (ret != S_OK)
        goto logoff;

    IMsgStore_OpenEntry(msg_store, props[0].Value.bin.cb, (LPENTRYID) props[0].Value.bin.lpb,
        NULL, MAPI_MODIFY, &obj_type, (LPUNKNOWN *) &draft_folder);

    /* Create a new message */
    if (IMAPIFolder_CreateMessage(draft_folder, NULL, 0, &msg) == S_OK)
    {
        ULONG token;
        SPropValue p;

        /* Define message properties */
        p.ulPropTag = PR_MESSAGE_FLAGS;
        p.Value.l = MSGFLAG_FROMME | MSGFLAG_UNSENT;

        IMessage_SetProps(msg, 1, &p, NULL);

        p.ulPropTag = PR_SENTMAIL_ENTRYID;
        p.Value.bin.cb = props[0].Value.bin.cb;
        p.Value.bin.lpb = props[0].Value.bin.lpb;
        IMessage_SetProps(msg, 1,&p, NULL);

        /* Set message subject */
        if (message->lpszSubject)
        {
            if (unicode_aware)
            {
                p.ulPropTag = PR_SUBJECT_W;
                p.Value.lpszW = message->lpszSubject;
            }
            else
            {
                subjectA = convert_from_unicode(message->lpszSubject);

                p.ulPropTag = PR_SUBJECT_A;
                p.Value.lpszA = subjectA;
            }

            IMessage_SetProps(msg, 1, &p, NULL);
        }

        /* Set message body */
        if (message->lpszNoteText)
        {
            LPSTREAM stream = NULL;

            if (IMessage_OpenProperty(msg, unicode_aware ? PR_BODY_W : PR_BODY_A, &IID_IStream, 0,
                MAPI_MODIFY | MAPI_CREATE, (LPUNKNOWN*) &stream) == S_OK)
            {
                if (unicode_aware)
                    IStream_Write(stream, message->lpszNoteText, (lstrlenW(message->lpszNoteText)+1) * sizeof(WCHAR), NULL);
                else
                {
                    bodyA = convert_from_unicode(message->lpszNoteText);
                    IStream_Write(stream, bodyA, strlen(bodyA)+1, NULL);
                }

                IStream_Release(stream);
            }
        }

        /* Add message attachments */
        if (message->nFileCount > 0)
        {
            ULONG num_attach = 0;
            unsigned int i;

            for (i = 0; i < message->nFileCount; i++)
            {
                IAttach* attachment = NULL;
                char *filenameA = NULL;
                SPropValue prop[4];
                LPCWSTR filename;
                HANDLE file;

                if (!message->lpFiles[i].lpszPathName)
                    continue;

                /* Open the attachment for reading */
                file = CreateFileW(message->lpFiles[i].lpszPathName, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                if (file == INVALID_HANDLE_VALUE)
                    continue;

                /* Check if a display filename has been given; if not, get one ourselves from path name */
                filename = message->lpFiles[i].lpszFileName;

                if (!filename)
                {
                    int j;

                    filename = message->lpFiles[i].lpszPathName;

                    for (j = lstrlenW(message->lpFiles[i].lpszPathName)-1; j >= 0; j--)
                    {
                        if (message->lpFiles[i].lpszPathName[i] == '\\' ||
                            message->lpFiles[i].lpszPathName[i] == '/')
                        {
                            filename = &message->lpFiles[i].lpszPathName[i+1];
                            break;
                        }
                    }
                }

                TRACE("Attachment %u path: '%s'; filename: '%s'\n", i, debugstr_w(message->lpFiles[i].lpszPathName),
                    debugstr_w(filename));

                /* Create the attachment */
                if (IMessage_CreateAttach(msg, NULL, 0, &num_attach, &attachment) != S_OK)
                {
                    TRACE("Unable to create attachment\n");
                    CloseHandle(file);
                    continue;
                }

                /* Set the attachment properties */
                ZeroMemory(prop, sizeof(prop));

                prop[0].ulPropTag = PR_ATTACH_METHOD;
                prop[0].Value.ul = ATTACH_BY_VALUE;

                if (unicode_aware)
                {
                    prop[1].ulPropTag = PR_ATTACH_LONG_FILENAME_W;
                    prop[1].Value.lpszW = (LPWSTR) filename;
                    prop[2].ulPropTag = PR_ATTACH_FILENAME_W;
                    prop[2].Value.lpszW = (LPWSTR) filename;
                }
                else
                {
                    filenameA = convert_from_unicode(filename);

                    prop[1].ulPropTag = PR_ATTACH_LONG_FILENAME_A;
                    prop[1].Value.lpszA = (LPSTR) filenameA;
                    prop[2].ulPropTag = PR_ATTACH_FILENAME_A;
                    prop[2].Value.lpszA = (LPSTR) filenameA;

                }

                prop[3].ulPropTag = PR_RENDERING_POSITION;
                prop[3].Value.l = -1;

                if (IAttach_SetProps(attachment, 4, prop, NULL) == S_OK)
                {
                    LPSTREAM stream = NULL;

                    if (IAttach_OpenProperty(attachment, PR_ATTACH_DATA_BIN, &IID_IStream, 0,
                        MAPI_MODIFY | MAPI_CREATE, (LPUNKNOWN*) &stream) == S_OK)
                    {
                        BYTE data[READ_BUF_SIZE];
                        DWORD size = 0, read, written;

                        while (ReadFile(file, data, READ_BUF_SIZE, &read, NULL) && (read != 0))
                        {
                            IStream_Write(stream, data, read, &written);
                            size += read;
                        }

                        TRACE("%ld bytes written of attachment\n", size);

                        IStream_Commit(stream, STGC_DEFAULT);
                        IStream_Release(stream);

                        prop[0].ulPropTag = PR_ATTACH_SIZE;
                        prop[0].Value.ul = size;
                        IAttach_SetProps(attachment, 1, prop, NULL);

                        IAttach_SaveChanges(attachment, KEEP_OPEN_READONLY);
                        num_attach++;
                    }
                }

                CloseHandle(file);
                IAttach_Release(attachment);

                HeapFree(GetProcessHeap(), 0, filenameA);
            }
        }

        IMessage_SaveChanges(msg, KEEP_OPEN_READWRITE);

        /* Prepare the message form */

        if (IMAPISession_PrepareForm(session, NULL, msg, &token) == S_OK)
        {
            ULONG access = 0, status = 0, message_flags = 0, pc = 0;
            ULONG pT[2] = {1, PR_MSG_STATUS};

            /* Retrieve message status, flags, access rights and class */

            if (IMessage_GetProps(msg, (LPSPropTagArray) pT, 0, &pc, &props) == S_OK)
            {
                status = props->Value.ul;
                MAPIFreeBuffer(props);
            }

            pT[1] = PR_MESSAGE_FLAGS;

            if (IMessage_GetProps(msg, (LPSPropTagArray) pT, 0, &pc, &props) == S_OK)
            {
                message_flags = props->Value.ul;
                MAPIFreeBuffer(props);
            }

            pT[1] = PR_ACCESS;

            if (IMessage_GetProps(msg, (LPSPropTagArray) pT, 0, &pc, &props) == S_OK)
            {
                access = props->Value.ul;
                MAPIFreeBuffer(props);
            }

            pT[1] = PR_MESSAGE_CLASS_A;

            if (IMessage_GetProps(msg, (LPSPropTagArray) pT, 0, &pc, &props) == S_OK)
            {
                /* Show the message form (edit window) */

                ret = IMAPISession_ShowForm(session, 0, msg_store, draft_folder, NULL,
                                            token, NULL, 0, status, message_flags, access,
                                            props->Value.lpszA);

                switch (ret)
                {
                    case S_OK:
                        retval = SUCCESS_SUCCESS;
                        break;

                    case MAPI_E_USER_CANCEL:
                        retval = MAPI_E_USER_ABORT;
                        break;

                    default:
                        TRACE("ShowForm failure: %lx\n", ret);
                        break;
                }
            }
        }

        IMessage_Release(msg);
    }

    /* Free up the resources we've used */
    IMAPIFolder_Release(draft_folder);
    if (folder) IMAPIFolder_Release(folder);
    IMsgStore_Release(msg_store);

    HeapFree(GetProcessHeap(), 0, subjectA);
    HeapFree(GetProcessHeap(), 0, bodyA);

logoff: ;
    IMAPISession_Logoff(session, 0, 0, 0);
    IMAPISession_Release(session);

cleanup: ;
    MAPIUninitialize();
    return retval;
}

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
 */
ULONG WINAPI MAPISendMail( LHANDLE session, ULONG_PTR uiparam,
    lpMapiMessage message, FLAGS flags, ULONG reserved )
{
    WCHAR msg_title[READ_BUF_SIZE], error_msg[READ_BUF_SIZE];

    /* Check to see if we have a Simple MAPI provider loaded */
    if (mapiFunctions.MAPISendMail)
        return mapiFunctions.MAPISendMail(session, uiparam, message, flags, reserved);

    /* Check if we have an Extended MAPI provider - if so, use our wrapper */
    if (MAPIInitialize(NULL) == S_OK)
    {
        MapiMessageW messageW;
        ULONG ret;

        ZeroMemory(&messageW, sizeof(MapiMessageW));

        /* Convert the entries we need to Unicode */
        messageW.lpszSubject = convert_to_unicode(message->lpszSubject);
        messageW.lpszNoteText = convert_to_unicode(message->lpszNoteText);
        messageW.nFileCount = message->nFileCount;

        if (message->nFileCount && message->lpFiles)
        {
            lpMapiFileDescW filesW;
            unsigned int i;

            filesW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MapiFileDescW) * message->nFileCount);

            for (i = 0; i < message->nFileCount; i++)
            {
                filesW[i].lpszPathName = convert_to_unicode(message->lpFiles[i].lpszPathName);
                filesW[i].lpszFileName = convert_to_unicode(message->lpFiles[i].lpszFileName);
            }

            messageW.lpFiles = filesW;
        }

        ret = sendmail_extended_mapi(session, uiparam, &messageW, flags);

        /* Now free everything we allocated */
        if (message->nFileCount && message->lpFiles)
        {
            unsigned int i;

            for (i = 0; i < message->nFileCount; i++)
            {
                HeapFree(GetProcessHeap(), 0, messageW.lpFiles[i].lpszPathName);
                HeapFree(GetProcessHeap(), 0, messageW.lpFiles[i].lpszFileName);
            }

            HeapFree(GetProcessHeap(), 0, messageW.lpFiles);
        }

        HeapFree(GetProcessHeap(), 0, messageW.lpszSubject);
        HeapFree(GetProcessHeap(), 0, messageW.lpszNoteText);

        return ret;
    }

    /* Display an error message since we apparently have no mail clients */
    LoadStringW(hInstMAPI32, IDS_NO_MAPI_CLIENT, error_msg, ARRAY_SIZE(error_msg));
    LoadStringW(hInstMAPI32, IDS_SEND_MAIL, msg_title, ARRAY_SIZE(msg_title));

    MessageBoxW((HWND) uiparam, error_msg, msg_title, MB_ICONEXCLAMATION);

    return MAPI_E_NOT_SUPPORTED;
}

static lpMapiRecipDesc convert_recipient_from_unicode(lpMapiRecipDescW recipW, lpMapiRecipDesc dest)
{
    lpMapiRecipDesc ret;

    if (!recipW)
        return NULL;

    if (dest)
        ret = dest;
    else
        ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MapiRecipDesc));

    ret->ulRecipClass = recipW->ulRecipClass;
    ret->lpszName = convert_from_unicode(recipW->lpszName);
    ret->lpszAddress = convert_from_unicode(recipW->lpszAddress);
    ret->ulEIDSize = recipW->ulEIDSize;
    ret->lpEntryID = recipW->lpEntryID;

    return ret;
}

/**************************************************************************
 *  MAPISendMailW	(MAPI32.256)
 *
 * Send a mail.
 *
 * PARAMS
 *  session  [I] Handle to a MAPI session.
 *  uiparam  [I] Parent window handle.
 *  message  [I] Pointer to a MAPIMessageW structure.
 *  flags    [I] Flags.
 *  reserved [I] Reserved, pass 0.
 *
 * RETURNS
 *  Success: SUCCESS_SUCCESS
 *  Failure: MAPI_E_FAILURE
 *
 */
ULONG WINAPI MAPISendMailW(LHANDLE session, ULONG_PTR uiparam,
    lpMapiMessageW message, FLAGS flags, ULONG reserved)
{
    WCHAR msg_title[READ_BUF_SIZE], error_msg[READ_BUF_SIZE];

    /* Check to see if we have a Simple MAPI provider loaded */
    if (mapiFunctions.MAPISendMailW)
        return mapiFunctions.MAPISendMailW(session, uiparam, message, flags, reserved);

    /* Check if we have an Extended MAPI provider - if so, use our wrapper */
    if (MAPIInitialize(NULL) == S_OK)
        return sendmail_extended_mapi(session, uiparam, message, flags);

    if (mapiFunctions.MAPISendMail)
    {
        MapiMessage messageA;
        ULONG ret;

        if (flags & MAPI_FORCE_UNICODE)
            return MAPI_E_UNICODE_NOT_SUPPORTED;

        /* Convert to ANSI and send to MAPISendMail */
        ZeroMemory(&messageA, sizeof(MapiMessage));

        messageA.lpszSubject = convert_from_unicode(message->lpszSubject);
        messageA.lpszNoteText = convert_from_unicode(message->lpszNoteText);
        messageA.lpszMessageType = convert_from_unicode(message->lpszMessageType);
        messageA.lpszDateReceived = convert_from_unicode(message->lpszDateReceived);
        messageA.lpszConversationID = convert_from_unicode(message->lpszConversationID);
        messageA.flFlags = message->flFlags;
        messageA.lpOriginator = convert_recipient_from_unicode(message->lpOriginator, NULL);
        messageA.nRecipCount = message->nRecipCount;
        messageA.nFileCount = message->nFileCount;

        if (message->nRecipCount && message->lpRecips)
        {
            lpMapiRecipDesc recipsA;
            unsigned int i;

            recipsA = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MapiRecipDesc) * message->nRecipCount);

            for (i = 0; i < message->nRecipCount; i++)
            {
                convert_recipient_from_unicode(&message->lpRecips[i], &recipsA[i]);
            }

            messageA.lpRecips = recipsA;
        }

        if (message->nFileCount && message->lpFiles)
        {
            lpMapiFileDesc filesA;
            unsigned int i;

            filesA = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MapiFileDesc) * message->nFileCount);

            for (i = 0; i < message->nFileCount; i++)
            {
                filesA[i].flFlags = message->lpFiles[i].flFlags;
                filesA[i].nPosition = message->lpFiles[i].nPosition;
                filesA[i].lpszPathName = convert_from_unicode(message->lpFiles[i].lpszPathName);
                filesA[i].lpszFileName = convert_from_unicode(message->lpFiles[i].lpszFileName);
                filesA[i].lpFileType = message->lpFiles[i].lpFileType;
            }

            messageA.lpFiles = filesA;
        }

        ret = mapiFunctions.MAPISendMail(session, uiparam, &messageA, flags, reserved);

        /* Now free everything we allocated */
        if (message->lpOriginator)
        {
            HeapFree(GetProcessHeap(), 0, messageA.lpOriginator->lpszName);
            HeapFree(GetProcessHeap(), 0, messageA.lpOriginator->lpszAddress);
            HeapFree(GetProcessHeap(), 0, messageA.lpOriginator);
        }

        if (message->nRecipCount && message->lpRecips)
        {
            unsigned int i;

            for (i = 0; i < message->nRecipCount; i++)
            {
                HeapFree(GetProcessHeap(), 0, messageA.lpRecips[i].lpszName);
                HeapFree(GetProcessHeap(), 0, messageA.lpRecips[i].lpszAddress);
            }

            HeapFree(GetProcessHeap(), 0, messageA.lpRecips);
        }

        if (message->nFileCount && message->lpFiles)
        {
            unsigned int i;

            for (i = 0; i < message->nFileCount; i++)
            {
                HeapFree(GetProcessHeap(), 0, messageA.lpFiles[i].lpszPathName);
                HeapFree(GetProcessHeap(), 0, messageA.lpFiles[i].lpszFileName);
            }

            HeapFree(GetProcessHeap(), 0, messageA.lpFiles);
        }

        HeapFree(GetProcessHeap(), 0, messageA.lpszSubject);
        HeapFree(GetProcessHeap(), 0, messageA.lpszNoteText);
        HeapFree(GetProcessHeap(), 0, messageA.lpszDateReceived);
        HeapFree(GetProcessHeap(), 0, messageA.lpszConversationID);

        return ret;
    }

    /* Display an error message since we apparently have no mail clients */
    LoadStringW(hInstMAPI32, IDS_NO_MAPI_CLIENT, error_msg, ARRAY_SIZE(error_msg));
    LoadStringW(hInstMAPI32, IDS_SEND_MAIL, msg_title, ARRAY_SIZE(msg_title));

    MessageBoxW((HWND) uiparam, error_msg, msg_title, MB_ICONEXCLAMATION);

    return MAPI_E_NOT_SUPPORTED;
}

ULONG WINAPI MAPISendDocuments(ULONG_PTR uiparam, LPSTR delim, LPSTR paths,
    LPSTR filenames, ULONG reserved)
{
    if (mapiFunctions.MAPISendDocuments)
        return mapiFunctions.MAPISendDocuments(uiparam, delim, paths, filenames, reserved);

    return MAPI_E_NOT_SUPPORTED;
}
