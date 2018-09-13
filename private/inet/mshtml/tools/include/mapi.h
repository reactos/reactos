/*
 *  M A P I . H
 *
 *  Messaging Applications Programming Interface.
 *
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 *
 *  Purpose:
 *
 *    This file defines the structures and constants used by that
 *    subset of the Messaging Applications Programming Interface
 *    which is supported under Windows by Microsoft Mail for PC
 *    Networks version 3.x.
 */


#ifndef MAPI_H
#define MAPI_H

/*
 *  Types.
 */


#ifdef __cplusplus
extern "C" {
#endif


#ifndef EXPORT
#ifdef WIN16
#define EXPORT __export
#else
/* Additional special definitions here */
#define EXPORT
#endif
#endif

typedef unsigned long FAR * LPULONG;
typedef unsigned long       FLAGS;

#ifndef __LHANDLE
#define __LHANDLE
typedef unsigned long       LHANDLE, FAR * LPLHANDLE;
#endif

typedef unsigned char FAR * LPBYTE;

#define    lhSessionNull    ((LHANDLE)0)

typedef struct
{
    ULONG ulReserved;            /* Reserved for future use (must be 0)     */
    ULONG flFlags;               /* Flags                                   */
    ULONG nPosition;             /* character in text to be replaced by attachment */
    LPSTR lpszPathName;          /* Full path name of attachment file       */
    LPSTR lpszFileName;          /* Original file name (optional)           */
    LPVOID lpFileType;           /* Attachment file type (can be lpMapiFileTagExt) */
} MapiFileDesc, FAR * lpMapiFileDesc;

#define MAPI_OLE                0x00000001
#define MAPI_OLE_STATIC         0x00000002


typedef struct
{
    ULONG ulReserved;           /* Reserved, must be zero.                  */
    ULONG cbTag;                /* Size (in bytes) of                       */
    LPBYTE lpTag;               /* X.400 OID for this attachment type       */
    ULONG cbEncoding;           /* Size (in bytes) of                       */
    LPBYTE lpEncoding;          /* X.400 OID for this attachment's encoding */
} MapiFileTagExt, FAR *lpMapiFileTagExt;


typedef struct
{
    ULONG ulReserved;           /* Reserved for future use                  */
    ULONG ulRecipClass;         /* Recipient class                          */
                                /* MAPI_TO, MAPI_CC, MAPI_BCC, MAPI_ORIG    */
    LPSTR lpszName;             /* Recipient name                           */
    LPSTR lpszAddress;          /* Recipient address (optional)             */
    ULONG ulEIDSize;            /* Count in bytes of size of pEntryID       */
    LPVOID lpEntryID;           /* System-specific recipient reference      */
} MapiRecipDesc, FAR * lpMapiRecipDesc;

#ifndef MAPI_ORIG               /* also defined in mapix.h */
#define MAPI_ORIG   0           /* Recipient is message originator          */
#define MAPI_TO     1           /* Recipient is a primary recipient         */
#define MAPI_CC     2           /* Recipient is a copy recipient            */
#define MAPI_BCC    3           /* Recipient is blind copy recipient        */
#endif

typedef struct
{
    ULONG ulReserved;             /* Reserved for future use (M.B. 0)       */
    LPSTR lpszSubject;            /* Message Subject                        */
    LPSTR lpszNoteText;           /* Message Text                           */
    LPSTR lpszMessageType;        /* Message Class                          */
    LPSTR lpszDateReceived;       /* in YYYY/MM/DD HH:MM format             */
    LPSTR lpszConversationID;     /* conversation thread ID                 */
    FLAGS flFlags;                /* unread,return receipt                  */
    lpMapiRecipDesc lpOriginator; /* Originator descriptor                  */
    ULONG nRecipCount;            /* Number of recipients                   */
    lpMapiRecipDesc lpRecips;     /* Recipient descriptors                  */
    ULONG nFileCount;             /* # of file attachments                  */
    lpMapiFileDesc lpFiles;       /* Attachment descriptors                 */
} MapiMessage, FAR * lpMapiMessage;

#define MAPI_UNREAD             0x00000001
#define MAPI_RECEIPT_REQUESTED  0x00000002
#define MAPI_SENT               0x00000004


/*
 *  Entry points.
 */

/*
 *  flFlags values for Simple MAPI entry points. All documented flags are
 *  shown for each call. Duplicates are commented out but remain present
 *  for every call.
 */

/* MAPILogon() flags.       */

#define MAPI_LOGON_UI           0x00000001  /* Display logon UI             */
#ifndef MAPI_PASSWORD_UI
#define MAPI_PASSWORD_UI        0x00020000  /* prompt for password only     */
#endif
#define MAPI_NEW_SESSION        0x00000002  /* Don't use shared session     */
#define MAPI_FORCE_DOWNLOAD     0x00001000  /* Get new mail before return   */
#define MAPI_EXTENDED           0x00000020  /* Extended MAPI Logon          */

/* MAPISendMail() flags.    */

/* #define MAPI_LOGON_UI        0x00000001     Display logon UI             */
/* #define MAPI_NEW_SESSION     0x00000002     Don't use shared session     */

#ifndef MAPI_DIALOG             /* also defined in property.h */
#define MAPI_DIALOG             0x00000008  /* Display a send note UI       */
#endif
/*# define MAPI_USE_DEFAULT     0x00000040     Use default profile in logon */

/* MAPIFindNext() flags.    */

#define MAPI_UNREAD_ONLY        0x00000020  /* Only unread messages         */
#define MAPI_GUARANTEE_FIFO     0x00000100  /* use date order               */
#define MAPI_LONG_MSGID         0x00004000  /* allow 512 char returned ID   */

/* MAPIReadMail() flags.    */

#define MAPI_PEEK               0x00000080  /* Do not mark as read.         */
#define MAPI_SUPPRESS_ATTACH    0x00000800  /* header + body, no files      */
#define MAPI_ENVELOPE_ONLY      0x00000040  /* Only header information      */
#define MAPI_BODY_AS_FILE       0x00000200

/* MAPISaveMail() flags.    */

/* #define MAPI_LOGON_UI        0x00000001     Display logon UI             */
/* #define MAPI_NEW_SESSION     0x00000002     Don't use shared session     */
/* #define MAPI_LONG_MSGID      0x00004000  /* allow 512 char returned ID   */

/* MAPIAddress() flags.     */

/* #define MAPI_LOGON_UI        0x00000001     Display logon UI             */
/* #define MAPI_NEW_SESSION     0x00000002     Don't use shared session     */

/* MAPIDetails() flags.     */

/* #define MAPI_LOGON_UI        0x00000001     Display logon UI             */
/* #define MAPI_NEW_SESSION     0x00000002     Don't use shared session     */
#define MAPI_AB_NOMODIFY        0x00000400  /* Don't allow mods of AB entries */

/* MAPIResolveName() flags. */

/* #define MAPI_LOGON_UI        0x00000001     Display logon UI             */
/* #define MAPI_NEW_SESSION     0x00000002     Don't use shared session     */
/* #define MAPI_DIALOG          0x00000008     Prompt for choices if ambiguous */
/* #define MAPI_AB_NOMODIFY     0x00000400     Don't allow mods of AB entries */

typedef ULONG (FAR PASCAL MAPILOGON)(
    ULONG ulUIParam,
    LPSTR lpszProfileName,
    LPSTR lpszPassword,
    FLAGS flFlags,
    ULONG ulReserved,
    LPLHANDLE lplhSession
);
typedef MAPILOGON FAR *LPMAPILOGON;
MAPILOGON MAPILogon;

typedef ULONG (FAR PASCAL MAPILOGOFF)(
    LHANDLE lhSession,
    ULONG ulUIParam,
    FLAGS flFlags,
    ULONG ulReserved
);
typedef MAPILOGOFF FAR *LPMAPILOGOFF;
MAPILOGOFF MAPILogoff;


typedef ULONG (FAR PASCAL MAPISENDMAIL)(
    LHANDLE lhSession,
    ULONG ulUIParam,
    lpMapiMessage lpMessage,
    FLAGS flFlags,
    ULONG ulReserved
);
typedef MAPISENDMAIL FAR *LPMAPISENDMAIL;
MAPISENDMAIL MAPISendMail;


typedef ULONG (FAR PASCAL MAPISENDDOCUMENTS)(
    ULONG ulUIParam,
    LPSTR lpszDelimChar,
    LPSTR lpszFilePaths,
    LPSTR lpszFileNames,
    ULONG ulReserved
);
typedef MAPISENDDOCUMENTS FAR *LPMAPISENDDOCUMENTS;
MAPISENDDOCUMENTS MAPISendDocuments;


typedef ULONG (FAR PASCAL MAPIFINDNEXT)(
    LHANDLE lhSession,
    ULONG ulUIParam,
    LPSTR lpszMessageType,
    LPSTR lpszSeedMessageID,
    FLAGS flFlags,
    ULONG ulReserved,
    LPSTR lpszMessageID
);
typedef MAPIFINDNEXT FAR *LPMAPIFINDNEXT;
MAPIFINDNEXT MAPIFindNext;

typedef ULONG (FAR PASCAL MAPIREADMAIL)(
    LHANDLE lhSession,
    ULONG ulUIParam,
    LPSTR lpszMessageID,
    FLAGS flFlags,
    ULONG ulReserved,
    lpMapiMessage FAR *lppMessage
);
typedef MAPIREADMAIL FAR *LPMAPIREADMAIL;
MAPIREADMAIL MAPIReadMail;

typedef ULONG (FAR PASCAL MAPISAVEMAIL)(
    LHANDLE lhSession,
    ULONG ulUIParam,
    lpMapiMessage lpMessage,
    FLAGS flFlags,
    ULONG ulReserved,
    LPSTR lpszMessageID
);
typedef MAPISAVEMAIL FAR *LPMAPISAVEMAIL;
MAPISAVEMAIL MAPISaveMail;

typedef ULONG (FAR PASCAL MAPIDELETEMAIL)(
    LHANDLE lhSession,
    ULONG ulUIParam,
    LPSTR lpszMessageID,
    FLAGS flFlags,
    ULONG ulReserved
);
typedef MAPIDELETEMAIL FAR *LPMAPIDELETEMAIL;
MAPIDELETEMAIL MAPIDeleteMail;

#ifndef _MAC
typedef ULONG (EXPORT FAR PASCAL MAPIFREEBUFFER)(
    LPVOID pv
);
typedef MAPIFREEBUFFER FAR *LPMAPIFREEBUFFER;
MAPIFREEBUFFER MAPIFreeBuffer;
#endif

typedef ULONG (FAR PASCAL MAPIADDRESS)(
    LHANDLE lhSession,
    ULONG ulUIParam,
    LPSTR lpszCaption,
    ULONG nEditFields,
    LPSTR lpszLabels,
    ULONG nRecips,
    lpMapiRecipDesc lpRecips,
    FLAGS flFlags,
    ULONG ulReserved,
    LPULONG lpnNewRecips,
    lpMapiRecipDesc FAR *lppNewRecips
);
typedef MAPIADDRESS FAR *LPMAPIADDRESS;
MAPIADDRESS MAPIAddress;

typedef ULONG (FAR PASCAL MAPIDETAILS)(
    LHANDLE lhSession,
    ULONG ulUIParam,
    lpMapiRecipDesc lpRecip,
    FLAGS flFlags,
    ULONG ulReserved
);
typedef MAPIDETAILS FAR *LPMAPIDETAILS;
MAPIDETAILS MAPIDetails;

typedef ULONG (FAR PASCAL MAPIRESOLVENAME)(
    LHANDLE lhSession,
    ULONG ulUIParam,
    LPSTR lpszName,
    FLAGS flFlags,
    ULONG ulReserved,
    lpMapiRecipDesc FAR *lppRecip
);
typedef MAPIRESOLVENAME FAR *LPMAPIRESOLVENAME;
MAPIRESOLVENAME MAPIResolveName;

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



#ifdef MAPIX_H
/*  Maps from a simple mapi session to a MAPI 1.0 extended mapi session */

STDMETHODIMP_(SCODE)
ScMAPIXFromSMAPI(LHANDLE lhSimpleSession,
                ULONG ulFlags,
                LPCIID lpInterface,
                LPMAPISESSION FAR * lppMAPISession);
#endif /* MAPIX_H */


#ifdef  __cplusplus
}       /*  extern "C" */
#endif

#endif /* MAPI_H */
