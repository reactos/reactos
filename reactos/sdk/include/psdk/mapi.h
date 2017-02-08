/*
 * Copyright (C) 2000 Francois Gouget
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

#ifndef MAPI_H
#define MAPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Some types */

#ifndef __LHANDLE
#define __LHANDLE
typedef ULONG_PTR               LHANDLE, *LPLHANDLE;
#endif
#define lhSessionNull           ((LHANDLE)0)

#ifndef WINE_FLAGS_DEFINED
#define WINE_FLAGS_DEFINED
typedef ULONG                   FLAGS;
#endif
typedef ULONG                  *LPULONG;

typedef struct
{
    ULONG ulReserved;
    ULONG flFlags;
    ULONG nPosition;
    LPSTR lpszPathName;
    LPSTR lpszFileName;
    LPVOID lpFileType;
} MapiFileDesc, *lpMapiFileDesc;

typedef struct
{
    ULONG ulReserved;
    ULONG flFlags;
    ULONG nPosition;
    PWSTR lpszPathName;
    PWSTR lpszFileName;
    PVOID lpFileType;
} MapiFileDescW, *lpMapiFileDescW;

#ifndef MAPI_ORIG
#define MAPI_ORIG   0
#define MAPI_TO     1
#define MAPI_CC     2
#define MAPI_BCC    3
#endif

typedef struct
{
    ULONG ulReserved;
    ULONG ulRecipClass;
    LPSTR lpszName;
    LPSTR lpszAddress;
    ULONG ulEIDSize;
    LPVOID lpEntryID;
} MapiRecipDesc, *lpMapiRecipDesc;

typedef struct
{
    ULONG ulReserved;
    ULONG ulRecipClass;
    PWSTR lpszName;
    PWSTR lpszAddress;
    ULONG ulEIDSize;
    PVOID lpEntryID;
} MapiRecipDescW, *lpMapiRecipDescW;

typedef struct
{
    ULONG ulReserved;
    LPSTR lpszSubject;
    LPSTR lpszNoteText;
    LPSTR lpszMessageType;
    LPSTR lpszDateReceived;
    LPSTR lpszConversationID;
    FLAGS flFlags;
    lpMapiRecipDesc lpOriginator;
    ULONG nRecipCount;
    lpMapiRecipDesc lpRecips;
    ULONG nFileCount;
    lpMapiFileDesc lpFiles;
} MapiMessage, *lpMapiMessage;

typedef struct
{
    ULONG ulReserved;
    PWSTR lpszSubject;
    PWSTR lpszNoteText;
    PWSTR lpszMessageType;
    PWSTR lpszDateReceived;
    PWSTR lpszConversationID;
    FLAGS flFlags;
    lpMapiRecipDescW lpOriginator;
    ULONG nRecipCount;
    lpMapiRecipDescW lpRecips;
    ULONG nFileCount;
    lpMapiFileDescW lpFiles;
} MapiMessageW, *lpMapiMessageW;

/* Error codes */

#ifndef SUCCESS_SUCCESS
#define SUCCESS_SUCCESS                 0
#endif

#define MAPI_USER_ABORT                 1
#define MAPI_E_USER_ABORT               MAPI_USER_ABORT
#define MAPI_E_FAILURE                  2
#define MAPI_E_LOGON_FAILURE            3
#define MAPI_E_LOGIN_FAILURE            MAPI_E_LOGON_FAILURE
#define MAPI_E_DISK_FULL                4
#define MAPI_E_INSUFFICIENT_MEMORY      5
#define MAPI_E_ACCESS_DENIED            6
#define MAPI_E_TOO_MANY_SESSIONS        8
#define MAPI_E_TOO_MANY_FILES           9
#define MAPI_E_TOO_MANY_RECIPIENTS      10
#define MAPI_E_ATTACHMENT_NOT_FOUND     11
#define MAPI_E_ATTACHMENT_OPEN_FAILURE  12
#define MAPI_E_ATTACHMENT_WRITE_FAILURE 13
#define MAPI_E_UNKNOWN_RECIPIENT        14
#define MAPI_E_BAD_RECIPTYPE            15
#define MAPI_E_NO_MESSAGES              16
#define MAPI_E_INVALID_MESSAGE          17
#define MAPI_E_TEXT_TOO_LARGE           18
#define MAPI_E_INVALID_SESSION          19
#define MAPI_E_TYPE_NOT_SUPPORTED       20
#define MAPI_E_AMBIGUOUS_RECIPIENT      21
#define MAPI_E_AMBIG_RECIP              MAPI_E_AMBIGUOUS_RECIPIENT
#define MAPI_E_MESSAGE_IN_USE           22
#define MAPI_E_NETWORK_FAILURE          23
#define MAPI_E_INVALID_EDITFIELDS       24
#define MAPI_E_INVALID_RECIPS           25
#define MAPI_E_NOT_SUPPORTED            26
#define MAPI_E_UNICODE_NOT_SUPPORTED    27


/* MAPILogon */

#ifndef MAPI_LOGON_UI
#define MAPI_LOGON_UI           0x00000001
#endif
#ifndef MAPI_NEW_SESSION
#define MAPI_NEW_SESSION        0x00000002
#endif
#ifndef MAPI_EXTENDED
#define MAPI_EXTENDED           0x00000020
#endif
#ifndef MAPI_FORCE_DOWNLOAD
#define MAPI_FORCE_DOWNLOAD     0x00001000
#endif
#ifndef MAPI_PASSWORD_UI
#define MAPI_PASSWORD_UI        0x00020000
#endif


/* MAPISendMail */

#define MAPI_DIALOG             0x00000008

/* MAPISendMailW */

#define MAPI_FORCE_UNICODE      0x00040000


/* API typedefs and prototypes */

typedef ULONG (WINAPI MAPIADDRESS)(LHANDLE,ULONG_PTR,LPSTR,ULONG,LPSTR,ULONG,lpMapiRecipDesc,FLAGS,ULONG,LPULONG,lpMapiRecipDesc*);
typedef MAPIADDRESS *LPMAPIADDRESS;
MAPIADDRESS MAPIAddress;

typedef ULONG (WINAPI MAPIDELETEMAIL)(LHANDLE,ULONG_PTR,LPSTR,FLAGS,ULONG);
typedef MAPIDELETEMAIL *LPMAPIDELETEMAIL;
MAPIDELETEMAIL MAPIDeleteMail;

typedef ULONG (WINAPI MAPIDETAILS)(LHANDLE,ULONG_PTR,lpMapiRecipDesc,FLAGS,ULONG);
typedef MAPIDETAILS *LPMAPIDETAILS;
MAPIDETAILS MAPIDetails;

typedef ULONG (WINAPI MAPIFINDNEXT)(LHANDLE,ULONG_PTR,LPSTR,LPSTR,FLAGS,ULONG,LPSTR);
typedef MAPIFINDNEXT *LPMAPIFINDNEXT;
MAPIFINDNEXT MAPIFindNext;

#ifndef MAPIFREEBUFFER_DEFINED
#define MAPIFREEBUFFER_DEFINED
typedef ULONG (WINAPI MAPIFREEBUFFER)(LPVOID);
typedef MAPIFREEBUFFER *LPMAPIFREEBUFFER;
MAPIFREEBUFFER MAPIFreeBuffer;
#endif

typedef ULONG (WINAPI MAPILOGOFF)(LHANDLE,ULONG_PTR,FLAGS,ULONG);
typedef MAPILOGOFF *LPMAPILOGOFF;
MAPILOGOFF MAPILogoff;

typedef ULONG (WINAPI MAPILOGON)(ULONG_PTR,LPSTR,LPSTR,FLAGS,ULONG,LPLHANDLE);
typedef MAPILOGON *LPMAPILOGON;
MAPILOGON MAPILogon;

typedef ULONG (WINAPI MAPIREADMAIL)(LHANDLE,ULONG_PTR,LPSTR,FLAGS,ULONG,lpMapiMessage);
typedef MAPIREADMAIL *LPMAPIREADMAIL;
MAPIREADMAIL MAPIReadMail;

typedef ULONG (WINAPI MAPIRESOLVENAME)(LHANDLE,ULONG_PTR,LPSTR,FLAGS,ULONG,lpMapiRecipDesc*);
typedef MAPIRESOLVENAME *LPMAPIRESOLVENAME;
MAPIRESOLVENAME MAPIResolveName;

typedef ULONG (WINAPI MAPISAVEMAIL)(LHANDLE,ULONG_PTR,lpMapiMessage,FLAGS,ULONG,LPSTR);
typedef MAPISAVEMAIL *LPMAPISAVEMAIL;
MAPISAVEMAIL MAPISaveMail;

typedef ULONG (WINAPI MAPISENDDOCUMENTS)(ULONG_PTR,LPSTR,LPSTR,LPSTR,ULONG);
typedef MAPISENDDOCUMENTS *LPMAPISENDDOCUMENTS;
MAPISENDDOCUMENTS MAPISendDocuments;

typedef ULONG (WINAPI MAPISENDMAIL)(LHANDLE,ULONG_PTR,lpMapiMessage,FLAGS,ULONG);
typedef MAPISENDMAIL *LPMAPISENDMAIL;
MAPISENDMAIL MAPISendMail;

typedef ULONG (WINAPI MAPISENDMAILW)(LHANDLE,ULONG_PTR,lpMapiMessageW,FLAGS,ULONG);
typedef MAPISENDMAILW *LPMAPISENDMAILW;
MAPISENDMAILW MAPISendMailW;

#ifdef __cplusplus
}
#endif

#endif /* MAPI_H */
