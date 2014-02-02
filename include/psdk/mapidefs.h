/*
 * Copyright (C) 1998 Justin Bradford
 * Copyright (c) 2009 Owen Rudge for CodeWeavers
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

#ifndef MAPIDEFS_H
#define MAPIDEFS_H

#ifndef __WINESRC__
# include <windows.h>
#endif

#include <winerror.h>
#include <objbase.h>
#include <stddef.h>

/* Some types from other headers */
#ifndef __LHANDLE
#define __LHANDLE
typedef ULONG_PTR LHANDLE, *LPLHANDLE;
#endif

#ifndef _tagCY_DEFINED
#define _tagCY_DEFINED
typedef union tagCY
{
    struct
    {
#ifdef WORDS_BIGENDIAN
        LONG  Hi;
        ULONG Lo;
#else
        ULONG Lo;
        LONG  Hi;
#endif
    } DUMMYSTRUCTNAME;
    LONGLONG int64;
} CY;
typedef CY CURRENCY;
#endif /* _tagCY_DEFINED */


#ifndef _FILETIME_
#define _FILETIME_
typedef struct _FILETIME
{
#ifdef WORDS_BIGENDIAN
    DWORD dwHighDateTime;
    DWORD dwLowDateTime;
#else
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
#endif
} FILETIME, *PFILETIME, *LPFILETIME;
#endif

/* Memory allocation routines */
typedef SCODE (WINAPI ALLOCATEBUFFER)(ULONG,LPVOID*);
typedef SCODE (WINAPI ALLOCATEMORE)(ULONG,LPVOID,LPVOID*);
typedef ULONG (WINAPI FREEBUFFER)(LPVOID);

typedef ALLOCATEBUFFER *LPALLOCATEBUFFER;
typedef ALLOCATEMORE *LPALLOCATEMORE;
typedef FREEBUFFER *LPFREEBUFFER;

/* MAPI exposed interfaces */
typedef const IID *LPCIID;

typedef struct IAddrBook IAddrBook;
typedef IAddrBook *LPADRBOOK;
typedef struct IABContainer IABContainer;
typedef IABContainer *LPABCONT;
typedef struct IAttach *LPATTACH;
typedef struct IDistList IDistList;
typedef IDistList *LPDISTLIST;
typedef struct IMailUser IMailUser;
typedef IMailUser *LPMAILUSER;
typedef struct IMAPIAdviseSink *LPMAPIADVISESINK;
typedef struct IMAPIContainer *LPMAPICONTAINER;
typedef struct IMAPIFolder *LPMAPIFOLDER;
typedef struct IMAPIProgress IMAPIProgress;
typedef IMAPIProgress *LPMAPIPROGRESS;
typedef struct IMAPIStatus IMAPIStatus;
typedef IMAPIStatus *LPMAPISTATUS;
typedef struct IMessage *LPMESSAGE;
typedef struct IProfSect IProfSect;
typedef IProfSect *LPPROFSECT;
typedef struct IProviderAdmin IProviderAdmin;
typedef IProviderAdmin *LPPROVIDERADMIN;

#ifndef MAPI_DIM
# define MAPI_DIM 1 /* Default to one dimension for variable length arrays */
#endif

/* Flags for abFlags[0] */
#define MAPI_NOTRESERVED 0x08
#define MAPI_NOW         0x10
#define MAPI_THISSESSION 0x20
#define MAPI_NOTRECIP    0x40
#define MAPI_SHORTTERM   0x80

/* Flags for abFlags[1]  */
#define MAPI_COMPOUND    0x80

typedef struct _ENTRYID
{
    BYTE abFlags[4];
    BYTE ab[MAPI_DIM];
} ENTRYID, *LPENTRYID;

/* MAPI GUID's */
typedef struct _MAPIUID
{
    BYTE ab[sizeof(GUID)];
} MAPIUID, *LPMAPIUID;

#define IsEqualMAPIUID(pl,pr) (!memcmp((pl),(pr),sizeof(MAPIUID)))

#define MAPI_ONE_OFF_UID { 0x81,0x2b,0x1f,0xa4,0xbe,0xa3,0x10,0x19,0x9d,0x6e, \
                           0x00,0xdd,0x01,0x0f,0x54,0x02 }
#define MAPI_ONE_OFF_UNICODE      0x8000
#define MAPI_ONE_OFF_NO_RICH_INFO 0x0001

/* Object types */
#define MAPI_STORE    1U
#define MAPI_ADDRBOOK 2U
#define MAPI_FOLDER   3U
#define MAPI_ABCONT   4U
#define MAPI_MESSAGE  5U
#define MAPI_MAILUSER 6U
#define MAPI_ATTACH   7U
#define MAPI_DISTLIST 8U
#define MAPI_PROFSECT 9U
#define MAPI_STATUS   10U
#define MAPI_SESSION  11U
#define MAPI_FORMINFO 12U

/* Flags for various calls */
#define MAPI_MODIFY                   0x00000001U /* Object can be modified */
#define MAPI_CREATE                   0x00000002U /* Object can be created */
#define MAPI_ACCESS_MODIFY            MAPI_MODIFY /* Want write access */
#define MAPI_ACCESS_READ              0x00000002U /* Want read access */
#define MAPI_ACCESS_DELETE            0x00000004U /* Want delete access */
#define MAPI_ACCESS_CREATE_HIERARCHY  0x00000008U
#define MAPI_ACCESS_CREATE_CONTENTS   0x00000010U
#define MAPI_ACCESS_CREATE_ASSOCIATED 0x00000020U
#define MAPI_USE_DEFAULT              0x00000040U
#define MAPI_UNICODE                  0x80000000U /* Strings in this call are Unicode */

#if defined (UNICODE) || defined (__WINESRC__)
#define fMapiUnicode MAPI_UNICODE
#else
#define fMapiUnicode 0U
#endif

/* IMAPISession::OpenMessageStore() flags */
#define MDB_NO_DIALOG           0x00000001

/* Types of message receivers */
#ifndef MAPI_ORIG
#define MAPI_ORIG      0          /* The original author */
#define MAPI_TO        1          /* The primary message receiver */
#define MAPI_CC        2          /* A carbon copy receiver */
#define MAPI_BCC       3          /* A blind carbon copy receiver */
#define MAPI_P1        0x10000000 /* A message resend */
#define MAPI_SUBMITTED 0x80000000 /* This message has already been sent */
#endif

#ifndef cchProfileNameMax
#define cchProfileNameMax 64 /* Maximum length of a profile name */
#define cchProfilePassMax 64 /* Maximum length of a profile password */
#endif

/* Properties: The are the contents of cells in MAPI tables, as well as the
 * values returned when object properties are queried.
 */

/* Property types */
#define PT_UNSPECIFIED 0U
#define PT_NULL        1U
#define PT_I2          2U
#define PT_SHORT       PT_I2
#define PT_LONG        3U
#define PT_I4          PT_LONG
#define PT_R4          4U
#define PT_FLOAT       PT_R4
#define PT_DOUBLE      5U
#define PT_R8          PT_DOUBLE
#define PT_CURRENCY    6U
#define PT_APPTIME     7U
#define PT_ERROR       10U
#define PT_BOOLEAN     11U
#define PT_OBJECT      13U
#define PT_I8          20U
#define PT_LONGLONG    PT_I8
#define PT_STRING8     30U
#define PT_UNICODE     31U
#define PT_SYSTIME     64U
#define PT_CLSID       72U
#define PT_BINARY      258U

#define MV_FLAG     0x1000 /* This property type is multi-valued (an array) */
#define MV_INSTANCE 0x2000
#define MVI_FLAG    (MV_FLAG|MV_INSTANCE)
#define MVI_PROP(t) ((t)|MVI_FLAG)

#ifndef WINE_NO_UNICODE_MACROS
# ifdef UNICODE
# define PT_TSTRING      PT_UNICODE
# define PT_MV_TSTRING   (MV_FLAG|PT_UNICODE)
# define LPSZ            lpszW
# define LPPSZ           lppszW
# define MVSZ            MVszW
# else
# define PT_TSTRING      PT_STRING8
# define PT_MV_TSTRING   (MV_FLAG|PT_STRING8)
# define LPSZ            lpszA
# define LPPSZ           lppszA
# define MVSZ            MVszA
# endif
#endif

#define PROP_TYPE_MASK  0xFFFFU
#define PROP_TYPE(t)    ((t) & PROP_TYPE_MASK)
#define PROP_ID(t)      ((t) >> 16)
#define PROP_TAG(t,id)  (((id) << 16) | t)
#define PROP_ID_NULL    0
#define PROP_ID_INVALID 0xFFFF
#define PR_NULL         PROP_TAG(PT_NULL, PROP_ID_NULL)

#define CHANGE_PROP_TYPE(t,typ) ((0xFFFF0000 & t) | typ)

/* Multi-valued property types */
#define PT_MV_I2       (MV_FLAG|PT_I2)
#define PT_MV_SHORT    PT_MV_I2
#define PT_MV_LONG     (MV_FLAG|PT_LONG)
#define PT_MV_I4       PT_MV_LONG
#define PT_MV_R4       (MV_FLAG|PT_R4)
#define PT_MV_FLOAT    PT_MV_R4
#define PT_MV_DOUBLE   (MV_FLAG|PT_DOUBLE)
#define PT_MV_R8       PT_MV_DOUBLE
#define PT_MV_CURRENCY (MV_FLAG|PT_CURRENCY)
#define PT_MV_APPTIME  (MV_FLAG|PT_APPTIME)
#define PT_MV_SYSTIME  (MV_FLAG|PT_SYSTIME)
#define PT_MV_STRING8  (MV_FLAG|PT_STRING8)
#define PT_MV_BINARY   (MV_FLAG|PT_BINARY)
#define PT_MV_UNICODE  (MV_FLAG|PT_UNICODE)
#define PT_MV_CLSID    (MV_FLAG|PT_CLSID)
#define PT_MV_I8       (MV_FLAG|PT_I8)
#define PT_MV_LONGLONG PT_MV_I8


/* The property tag structure. This describes a list of columns */
typedef struct _SPropTagArray
{
    ULONG cValues;              /* Number of elements in aulPropTag */
    ULONG aulPropTag[MAPI_DIM]; /* Property tags */
} SPropTagArray, *LPSPropTagArray;

#define CbNewSPropTagArray(c) (offsetof(SPropTagArray,aulPropTag)+(c)*sizeof(ULONG))
#define CbSPropTagArray(p)    CbNewSPropTagArray((p)->cValues)
#define SizedSPropTagArray(n,id) \
    struct _SPropTagArray_##id { ULONG cValues; ULONG aulPropTag[n]; } id

/* Multi-valued PT_APPTIME property value */
typedef struct _SAppTimeArray
{
    ULONG   cValues; /* Number of doubles in lpat */
    double *lpat;    /* Pointer to double array of length cValues */
} SAppTimeArray;

/* PT_BINARY property value */
typedef struct _SBinary
{
    ULONG  cb;  /* Number of bytes in lpb */
    LPBYTE lpb; /* Pointer to byte array of length cb */
} SBinary, *LPSBinary;

/* Multi-valued PT_BINARY property value */
typedef struct _SBinaryArray
{
    ULONG    cValues; /* Number of SBinarys in lpbin */
    SBinary *lpbin;   /* Pointer to SBinary array of length cValues */
} SBinaryArray;

typedef SBinaryArray ENTRYLIST, *LPENTRYLIST;

/* Multi-valued PT_CY property value */
typedef struct _SCurrencyArray
{
    ULONG  cValues; /* Number of CYs in lpcu */
    CY    *lpcur;   /* Pointer to CY array of length cValues */
} SCurrencyArray;

/* Multi-valued PT_SYSTIME property value */
typedef struct _SDateTimeArray
{
    ULONG     cValues; /* Number of FILETIMEs in lpft */
    FILETIME *lpft;    /* Pointer to FILETIME array of length cValues */
} SDateTimeArray;

/* Multi-valued PT_DOUBLE property value */
typedef struct _SDoubleArray
{
    ULONG   cValues; /* Number of doubles in lpdbl */
    double *lpdbl;   /* Pointer to double array of length cValues */
} SDoubleArray;

/* Multi-valued PT_CLSID property value */
typedef struct _SGuidArray
{
    ULONG cValues; /* Number of GUIDs in lpguid */
    GUID *lpguid;  /* Pointer to GUID array of length cValues */
} SGuidArray;

/* Multi-valued PT_LONGLONG property value */
typedef struct _SLargeIntegerArray
{
    ULONG          cValues; /* Number of long64s in lpli */
    LARGE_INTEGER *lpli;    /* Pointer to long64 array of length cValues */
} SLargeIntegerArray;

/* Multi-valued PT_LONG property value */
typedef struct _SLongArray
{
    ULONG  cValues; /* Number of longs in lpl */
    LONG  *lpl;     /* Pointer to long array of length cValues */
} SLongArray;

/* Multi-valued PT_STRING8 property value */
typedef struct _SLPSTRArray
{
    ULONG  cValues; /* Number of Ascii strings in lppszA */
    LPSTR *lppszA;  /* Pointer to Ascii string array of length cValues */
} SLPSTRArray;

/* Multi-valued PT_FLOAT property value */
typedef struct _SRealArray
{
    ULONG cValues; /* Number of floats in lpflt */
    float *lpflt;  /* Pointer to float array of length cValues */
} SRealArray;

/* Multi-valued PT_SHORT property value */
typedef struct _SShortArray
{
    ULONG      cValues; /* Number of shorts in lpb */
    short int *lpi;     /* Pointer to short array of length cValues */
} SShortArray;

/* Multi-valued PT_UNICODE property value */
typedef struct _SWStringArray
{
    ULONG   cValues; /* Number of Unicode strings in lppszW */
    LPWSTR *lppszW;  /* Pointer to Unicode string array of length cValues */
} SWStringArray;

/* A property value */
typedef union _PV
{
    short int          i;
    LONG               l;
    ULONG              ul;
    float              flt;
    double             dbl;
    unsigned short     b;
    CY                 cur;
    double             at;
    FILETIME           ft;
    LPSTR              lpszA;
    SBinary            bin;
    LPWSTR             lpszW;
    LPGUID             lpguid;
    LARGE_INTEGER      li;
    SShortArray        MVi;
    SLongArray         MVl;
    SRealArray         MVflt;
    SDoubleArray       MVdbl;
    SCurrencyArray     MVcur;
    SAppTimeArray      MVat;
    SDateTimeArray     MVft;
    SBinaryArray       MVbin;
    SLPSTRArray        MVszA;
    SWStringArray      MVszW;
    SGuidArray         MVguid;
    SLargeIntegerArray MVli;
    SCODE              err;
    LONG               x;
} __UPV;

/* Property value structure. This is essentially a mini-Variant */
typedef struct _SPropValue
{
    ULONG     ulPropTag;  /* The property type */
    ULONG     dwAlignPad; /* Alignment, treat as reserved */
    union _PV Value;      /* The property value */
} SPropValue, *LPSPropValue;

/* Structure describing a table row (a collection of property values) */
typedef struct _SRow
{
    ULONG        ulAdrEntryPad; /* Padding, treat as reserved */
    ULONG        cValues;       /* Count of property values in lpProbs */
    LPSPropValue lpProps;       /* Pointer to an array of property values of length cValues */
} SRow, *LPSRow;

/* Structure describing a set of table rows */
typedef struct _SRowSet
{
    ULONG cRows;          /* Count of rows in aRow */
    SRow  aRow[MAPI_DIM]; /* Array of rows of length cRows */
} SRowSet, *LPSRowSet;

#define CbNewSRowSet(c) (offsetof(SRowSet,aRow)+(c)*sizeof(SRow))
#define CbSRowSet(p)    CbNewSRowSet((p)->cRows)
#define SizedSRowSet(n,id) \
    struct _SRowSet_##id { ULONG cRows; SRow aRow[n]; } id

/* Structure describing a problem with a property */
typedef struct _SPropProblem
{
    ULONG ulIndex;   /* Index of the property */
    ULONG ulPropTag; /* Property tag of the property */
    SCODE scode;     /* Error code of the problem */
} SPropProblem, *LPSPropProblem;

/* A collection of property problems */
typedef struct _SPropProblemArray
{
    ULONG        cProblem;           /* Number of problems in aProblem */
    SPropProblem aProblem[MAPI_DIM]; /* Array of problems of length cProblem */
} SPropProblemArray, *LPSPropProblemArray;

/* FPropContainsProp flags */
#define FL_FULLSTRING     ((ULONG)0x00000) /* Exact string match */
#define FL_SUBSTRING      ((ULONG)0x00001) /* Substring match */
#define FL_PREFIX         ((ULONG)0x00002) /* Prefix match */
#define FL_IGNORECASE     ((ULONG)0x10000) /* Case insensitive */
#define FL_IGNORENONSPACE ((ULONG)0x20000) /* Ignore non spacing characters */
#define FL_LOOSE          ((ULONG)0x40000) /* Try very hard to match */


/* Table types returned by IMAPITable_GetStatus() */
#define TBLTYPE_SNAPSHOT 0U /* Table is fixed at creation time and contents do not change */
#define TBLTYPE_KEYSET   1U /* Table has a fixed number of rows, but row values may change */
#define TBLTYPE_DYNAMIC  2U /* Table values and the number of rows may change */

/* Table status returned by IMAPITable_GetStatus() */
#define TBLSTAT_COMPLETE       0U  /* All operations have completed (normal status) */
#define TBLSTAT_QCHANGED       7U  /* Table data has changed as expected */
#define TBLSTAT_SORTING        9U  /* Table is being asynchronously sorted */
#define TBLSTAT_SORT_ERROR     10U /* An error occurred while sorting the table */
#define TBLSTAT_SETTING_COLS   11U /* Table columns are being asynchronously changed */
#define TBLSTAT_SETCOL_ERROR   13U /* An error occurred during column changing */
#define TBLSTAT_RESTRICTING    14U /* Table rows are being asynchronously restricted */
#define TBLSTAT_RESTRICT_ERROR 15U /* An error occurred during row restriction */

/* Flags for IMAPITable operations that can be asynchronous */
#define TBL_NOWAIT 1U         /* Perform the operation asynchronously */
#define TBL_BATCH  2U         /* Perform the operation when the results are needed */
#define TBL_ASYNC  TBL_NOWAIT /* Synonym for TBL_NOWAIT */

/* Flags for IMAPITable_FindRow() */
#define DIR_BACKWARD 1U /* Read rows backwards from the start bookmark */

/* Table bookmarks */
typedef ULONG BOOKMARK;

#define BOOKMARK_BEGINNING ((BOOKMARK)0) /* The first row */
#define BOOKMARK_CURRENT   ((BOOKMARK)1) /* The curent table row */
#define BOOKMARK_END       ((BOOKMARK)2) /* The last row */

/* Row restrictions */
typedef struct _SRestriction* LPSRestriction;

typedef struct _SAndRestriction
{
    ULONG          cRes;
    LPSRestriction lpRes;
} SAndRestriction;

typedef struct _SBitMaskRestriction
{
    ULONG relBMR;
    ULONG ulPropTag;
    ULONG ulMask;
} SBitMaskRestriction;

typedef struct _SCommentRestriction
{
    ULONG          cValues;
    LPSRestriction lpRes;
    LPSPropValue   lpProp;
} SCommentRestriction;

#define RELOP_LT 0U
#define RELOP_LE 1U
#define RELOP_GT 2U
#define RELOP_GE 3U
#define RELOP_EQ 4U
#define RELOP_NE 5U
#define RELOP_RE 6U

typedef struct _SComparePropsRestriction
{
    ULONG relop;
    ULONG ulPropTag1;
    ULONG ulPropTag2;
} SComparePropsRestriction;

typedef struct _SContentRestriction
{
    ULONG        ulFuzzyLevel;
    ULONG        ulPropTag;
    LPSPropValue lpProp;
} SContentRestriction;

typedef struct _SExistRestriction
{
    ULONG ulReserved1;
    ULONG ulPropTag;
    ULONG ulReserved2;
} SExistRestriction;

typedef struct _SNotRestriction
{
    ULONG          ulReserved;
    LPSRestriction lpRes;
} SNotRestriction;

typedef struct _SOrRestriction
{
    ULONG          cRes;
    LPSRestriction lpRes;
} SOrRestriction;

typedef struct _SPropertyRestriction
{
    ULONG        relop;
    ULONG        ulPropTag;
    LPSPropValue lpProp;
} SPropertyRestriction;

typedef struct _SSizeRestriction
{
    ULONG relop;
    ULONG ulPropTag;
    ULONG cb;
} SSizeRestriction;

typedef struct _SSubRestriction
{
    ULONG          ulSubObject;
    LPSRestriction lpRes;
} SSubRestriction;

/* Restriction types */
#define RES_AND            0U
#define RES_OR             1U
#define RES_NOT            2U
#define RES_CONTENT        3U
#define RES_PROPERTY       4U
#define RES_COMPAREPROPS   5U
#define RES_BITMASK        6U
#define RES_SIZE           7U
#define RES_EXIST          8U
#define RES_SUBRESTRICTION 9U
#define RES_COMMENT        10U

typedef struct _SRestriction
{
    ULONG rt;
    union
    {
        SAndRestriction          resAnd;
        SBitMaskRestriction      resBitMask;
        SCommentRestriction      resComment;
        SComparePropsRestriction resCompareProps;
        SContentRestriction      resContent;
        SExistRestriction        resExist;
        SNotRestriction          resNot;
        SOrRestriction           resOr;
        SPropertyRestriction     resProperty;
        SSizeRestriction         resSize;
        SSubRestriction          resSub;
    } res;
} SRestriction;

/* Errors */
typedef struct _MAPIERROR
{
    ULONG  ulVersion;       /* Mapi version */
#if defined (UNICODE) || defined (__WINESRC__)
    LPWSTR lpszError;       /* Error and component strings. These are Ascii */
    LPWSTR lpszComponent;   /* unless the MAPI_UNICODE flag is passed in */
#else
    LPSTR  lpszError;
    LPSTR  lpszComponent;
#endif
    ULONG  ulLowLevelError;
    ULONG  ulContext;
} MAPIERROR, *LPMAPIERROR;

/* Sorting */
#define TABLE_SORT_ASCEND  0U
#define TABLE_SORT_DESCEND 1U
#define TABLE_SORT_COMBINE 2U

typedef struct _SSortOrder
{
    ULONG ulPropTag;
    ULONG ulOrder;
} SSortOrder, *LPSSortOrder;

typedef struct _SSortOrderSet
{
    ULONG      cSorts;
    ULONG      cCategories;
    ULONG      cExpanded;
    SSortOrder aSort[MAPI_DIM];
} SSortOrderSet, * LPSSortOrderSet;

#define MNID_ID     0
#define MNID_STRING 1

typedef struct _MAPINAMEID
{
    LPGUID lpguid;
    ULONG ulKind;
    union
    {
        LONG lID;
        LPWSTR lpwstrName;
    } Kind;
} MAPINAMEID, *LPMAPINAMEID;

/* Desired notification types (bitflags) */
#define fnevCriticalError        ((ULONG)0x00000001)
#define fnevNewMail              ((ULONG)0x00000002)
#define fnevObjectCreated        ((ULONG)0x00000004)
#define fnevObjectDeleted        ((ULONG)0x00000008)
#define fnevObjectModified       ((ULONG)0x00000010)
#define fnevObjectMoved          ((ULONG)0x00000020)
#define fnevObjectCopied         ((ULONG)0x00000040)
#define fnevSearchComplete       ((ULONG)0x00000080)
#define fnevTableModified        ((ULONG)0x00000100)
#define fnevStatusObjectModified ((ULONG)0x00000200)
#define fnevReservedForMapi      ((ULONG)0x40000000)
#define fnevExtended             ((ULONG)0x80000000)

/* Type of notification event */
#define TABLE_CHANGED       1U
#define TABLE_ERROR         2U
#define TABLE_ROW_ADDED     3U
#define TABLE_ROW_DELETED   4U
#define TABLE_ROW_MODIFIED  5U
#define TABLE_SORT_DONE     6U
#define TABLE_RESTRICT_DONE 7U
#define TABLE_SETCOL_DONE   8U
#define TABLE_RELOAD        9U

/* fnevCriticalError notification */
typedef struct _ERROR_NOTIFICATION
{
    ULONG       cbEntryID;
    LPENTRYID   lpEntryID;
    SCODE       scode;
    ULONG       ulFlags;
    LPMAPIERROR lpMAPIError;
} ERROR_NOTIFICATION;

/* fnevNewMail notification */
typedef struct _NEWMAIL_NOTIFICATION
{
    ULONG     cbEntryID;
    LPENTRYID lpEntryID;
    ULONG     cbParentID;
    LPENTRYID lpParentID;
    ULONG     ulFlags;
#if defined (UNICODE) || defined (__WINESRC__)
    LPWSTR    lpszMessageClass;
#else
    LPSTR     lpszMessageClass;
#endif
    ULONG     ulMessageFlags;
} NEWMAIL_NOTIFICATION;

/* fnevObjectCreated/Deleted/Modified/Moved/Copied notification */
typedef struct _OBJECT_NOTIFICATION
{
    ULONG           cbEntryID;
    LPENTRYID       lpEntryID;
    ULONG           ulObjType;
    ULONG           cbParentID;
    LPENTRYID       lpParentID;
    ULONG           cbOldID;
    LPENTRYID       lpOldID;
    ULONG           cbOldParentID;
    LPENTRYID       lpOldParentID;
    LPSPropTagArray lpPropTagArray;
} OBJECT_NOTIFICATION;

/* fnevTableModified notification */
typedef struct _TABLE_NOTIFICATION
{
    ULONG      ulTableEvent;
    HRESULT    hResult;
    SPropValue propIndex;
    SPropValue propPrior;
    SRow       row;
    ULONG      ulPad;
} TABLE_NOTIFICATION;

/* fnevExtended notification */
typedef struct _EXTENDED_NOTIFICATION
{
    ULONG  ulEvent;
    ULONG  cb;
    LPBYTE pbEventParameters;
} EXTENDED_NOTIFICATION;

/* fnevStatusObjectModified notification */
typedef struct
{
    ULONG        cbEntryID;
    LPENTRYID    lpEntryID;
    ULONG        cValues;
    LPSPropValue lpPropVals;
} STATUS_OBJECT_NOTIFICATION;

/* The notification structure passed to advise sinks */
typedef struct _NOTIFICATION
{
    ULONG ulEventType;
    ULONG ulAlignPad;
    union
    {
        ERROR_NOTIFICATION         err;
        NEWMAIL_NOTIFICATION       newmail;
        OBJECT_NOTIFICATION        obj;
        TABLE_NOTIFICATION         tab;
        EXTENDED_NOTIFICATION      ext;
        STATUS_OBJECT_NOTIFICATION statobj;
    } info;
} NOTIFICATION, *LPNOTIFICATION;

typedef LONG (WINAPI NOTIFCALLBACK)(LPVOID,ULONG,LPNOTIFICATION);
typedef NOTIFCALLBACK *LPNOTIFCALLBACK;

/* IMAPIContainer::OpenEntry flags */
#define MAPI_BEST_ACCESS    0x00000010

/*****************************************************************************
 * IMAPITable interface
 *
 * This is the read-only 'view' over an I(MAPI)TableData object.
 */
#define INTERFACE IMAPITable
DECLARE_INTERFACE_(IMAPITable,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMAPITable methods ***/
    STDMETHOD(GetLastError)(THIS_ HRESULT hRes, ULONG ulFlags, LPMAPIERROR *lppError) PURE;
    STDMETHOD(Advise)(THIS_ ULONG ulMask, LPMAPIADVISESINK lpSink, ULONG *lpCxn) PURE;
    STDMETHOD(Unadvise)(THIS_ ULONG ulCxn) PURE;
    STDMETHOD(GetStatus)(THIS_ ULONG *lpStatus, ULONG *lpType) PURE;
    STDMETHOD(SetColumns)(THIS_ LPSPropTagArray lpProps, ULONG ulFlags) PURE;
    STDMETHOD(QueryColumns)(THIS_ ULONG ulFlags, LPSPropTagArray *lpCols) PURE;
    STDMETHOD(GetRowCount)(THIS_ ULONG ulFlags, ULONG *lpCount) PURE;
    STDMETHOD(SeekRow)(THIS_ BOOKMARK lpStart, LONG lRows, LONG *lpSeeked) PURE;
    STDMETHOD(SeekRowApprox)(THIS_ ULONG ulNum, ULONG ulDenom) PURE;
    STDMETHOD(QueryPosition)(THIS_ ULONG *lpRow, ULONG *lpNum, ULONG *lpDenom) PURE;
    STDMETHOD(FindRow)(THIS_ LPSRestriction lpRestrict, BOOKMARK lpOrigin, ULONG ulFlags) PURE;
    STDMETHOD(Restrict)(THIS_ LPSRestriction lpRestrict, ULONG ulFlags) PURE;
    STDMETHOD(CreateBookmark)(THIS_ BOOKMARK *lppPos) PURE;
    STDMETHOD(FreeBookmark)(THIS_ BOOKMARK lpPos) PURE;
    STDMETHOD(SortTable)(THIS_ LPSSortOrderSet lpSortOpts, ULONG ulFlags) PURE;
    STDMETHOD(QuerySortOrder)(THIS_ LPSSortOrderSet *lppSortOpts) PURE;
    STDMETHOD(QueryRows)(THIS_ LONG lRows, ULONG ulFlags, LPSRowSet *lppRows) PURE;
    STDMETHOD(Abort) (THIS) PURE;
    STDMETHOD(ExpandRow)(THIS_ ULONG cbKey, LPBYTE lpKey, ULONG ulRows,
                         ULONG ulFlags, LPSRowSet *lppRows, ULONG *lpMore) PURE;
    STDMETHOD(CollapseRow)(THIS_ ULONG cbKey, LPBYTE lpKey, ULONG ulFlags, ULONG *lpRows) PURE;
    STDMETHOD(WaitForCompletion)(THIS_ ULONG ulFlags, ULONG ulTime, ULONG *lpState) PURE;
    STDMETHOD(GetCollapseState)(THIS_ ULONG ulFlags, ULONG cbKey, LPBYTE lpKey,
                                ULONG *lpStateLen, LPBYTE *lpState) PURE;
    STDMETHOD(SetCollapseState)(THIS_ ULONG ulFlags, ULONG ulLen,
                                LPBYTE lpStart, BOOKMARK *lppWhere) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define IMAPITable_QueryInterface(p,a,b)         (p)->lpVtbl->QueryInterface(p,a,b)
#define IMAPITable_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define IMAPITable_Release(p)                    (p)->lpVtbl->Release(p)
        /*** IMAPITable methods ***/
#define IMAPITable_GetLastError(p,a,b,c)         (p)->lpVtbl->GetLastError(p,a,b,c)
#define IMAPITable_Advise(p,a,b,c)               (p)->lpVtbl->Advise(p,a,b,c)
#define IMAPITable_Unadvise(p,a)                 (p)->lpVtbl->Unadvise(p,a)
#define IMAPITable_GetStatus(p,a,b)              (p)->lpVtbl->GetStatus(p,a,b)
#define IMAPITable_SetColumns(p,a,b)             (p)->lpVtbl->SetColumns(p,a,b)
#define IMAPITable_QueryColumns(p,a,b)           (p)->lpVtbl->QueryColumns(p,a,b)
#define IMAPITable_GetRowCount(p,a,b)            (p)->lpVtbl->GetRowCount(p,a,b)
#define IMAPITable_SeekRow(p,a,b)                (p)->lpVtbl->SeekRow(p,a,b)
#define IMAPITable_SeekRowApprox(p,a,b)          (p)->lpVtbl->SeekRowApprox(p,a,b)
#define IMAPITable_QueryPosition(p,a,b)          (p)->lpVtbl->QueryPosition(p,a,b)
#define IMAPITable_FindRow(p,a,b,c)              (p)->lpVtbl->FindRow(p,a,b,c)
#define IMAPITable_Restrict(p,a,b)               (p)->lpVtbl->Recstrict(p,a,b)
#define IMAPITable_CreateBookmark(p,a)           (p)->lpVtbl->CreateBookmark(p,a)
#define IMAPITable_FreeBookmark(p,a)             (p)->lpVtbl->FreeBookmark(p,a)
#define IMAPITable_SortTable(p,a,b)              (p)->lpVtbl->SortTable(p,a,b)
#define IMAPITable_QuerySortOrder(p,a)           (p)->lpVtbl->QuerySortOrder(p,a)
#define IMAPITable_QueryRows(p,a,b,c)            (p)->lpVtbl->QueryRows(p,a,b,c)
#define IMAPITable_Abort(p)                      (p)->lpVtbl->Abort(p)
#define IMAPITable_ExpandRow(p,a,b,c,d,e,f)      (p)->lpVtbl->ExpandRow(p,a,b,c,d,e,f)
#define IMAPITable_CollapseRow(p,a,b,c,d)        (p)->lpVtbl->CollapseRow(p,a,b,c,d)
#define IMAPITable_WaitForCompletion(p,a,b,c)    (p)->lpVtbl->WaitForCompletion(p,a,b,c)
#define IMAPITable_GetCollapseState(p,a,b,c,d,e) (p)->lpVtbl->GetCollapseState(p,a,b,c,d,e)
#define IMAPITable_SetCollapseState(p,a,b,c,d)   (p)->lpVtbl->SetCollapseState(p,a,b,c,d)
#endif

typedef IMAPITable *LPMAPITABLE;

/*****************************************************************************
 * IMAPIAdviseSink interface
 */
#define INTERFACE IMAPIAdviseSink
DECLARE_INTERFACE_(IMAPIAdviseSink,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMAPIAdviseSink methods ***/
    STDMETHOD(OnNotify)(THIS_ ULONG NumNotif, LPNOTIFICATION lpNotif) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define IMAPIAdviseSink_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IMAPIAdviseSink_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IMAPIAdviseSink_Release(p)            (p)->lpVtbl->Release(p)
        /*** IMAPIAdviseSink methods ***/
#define IMAPIAdviseSink_OnNotify(p,a,b)       (p)->lpVtbl->OnNotify(p,a,b)
#endif

/*****************************************************************************
 * IMAPIProp interface
 */
#define INTERFACE IMAPIProp
DECLARE_INTERFACE_(IMAPIProp,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMAPIProp methods ***/
    STDMETHOD(GetLastError)(THIS_ HRESULT hRes, ULONG ulFlags, LPMAPIERROR *lppErr) PURE;
    STDMETHOD(SaveChanges)(THIS_ ULONG ulFlags) PURE;
    STDMETHOD(GetProps)(THIS_ LPSPropTagArray lpPropTags, ULONG ulFlags, ULONG *lpValues, LPSPropValue *lppProps) PURE;
    STDMETHOD(GetPropList)(THIS_ ULONG  ulFlags, LPSPropTagArray *lppPropTagArray) PURE;
    STDMETHOD(OpenProperty)(THIS_ ULONG ulPropTag, LPCIID lpIid, ULONG ulOpts, ULONG ulFlags, LPUNKNOWN *lppUnk) PURE;
    STDMETHOD(SetProps)(THIS_ ULONG cValues, LPSPropValue lpProps, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(DeleteProps)(THIS_ LPSPropTagArray lpPropTags, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyTo)(THIS_ ULONG ciidExclude, LPCIID lpIid, LPSPropTagArray lpProps, ULONG ulParam,
                      LPMAPIPROGRESS lpProgress, LPCIID lpIface,LPVOID lpDest, ULONG ulFlags,
                      LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyProps)(THIS_ LPSPropTagArray lpIncludeProps, ULONG ulParam, LPMAPIPROGRESS lpProgress,
                         LPCIID lpIid, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray *lppProblems) PURE;
    STDMETHOD(GetNamesFromIDs)(THIS_ LPSPropTagArray *lppPropTags, LPGUID lpIid, ULONG ulFlags, ULONG *lpCount,
                               LPMAPINAMEID **lpppNames) PURE;
    STDMETHOD(GetIDsFromNames)(THIS_ ULONG cPropNames, LPMAPINAMEID *lppNames, ULONG ulFlags, LPSPropTagArray *lppPropTags) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define IMAPIProp_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IMAPIProp_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IMAPIProp_Release(p)                   (p)->lpVtbl->Release(p)
        /*** IMAPIProp methods ***/
#define IMAPIProp_GetLastError(p,a,b,c)        (p)->lpVtbl->GetLastError(p,a,b,c)
#define IMAPIProp_SaveChanges(p,a)             (p)->lpVtbl->SaveChanges(p,a)
#define IMAPIProp_GetProps(p,a,b,c,d)          (p)->lpVtbl->GetProps(p,a,b,c,d)
#define IMAPIProp_GetPropList(p,a,b)           (p)->lpVtbl->GetPropList(p,a,b)
#define IMAPIProp_OpenProperty(p,a,b,c,d,e)    (p)->lpVtbl->OpenProperty(p,a,b,c,d,e)
#define IMAPIProp_SetProps(p,a,b,c)            (p)->lpVtbl->SetProps(p,a,b,c)
#define IMAPIProp_DeleteProps(p,a,b)           (p)->lpVtbl->DeleteProps(p,a,b)
#define IMAPIProp_CopyTo(p,a,b,c,d,e,f,g,h,i)  (p)->lpVtbl->CopyTo(p,a,b,c,d,e,f,g,h,i)
#define IMAPIProp_CopyProps(p,a,b,c,d,e,f,g)   (p)->lpVtbl->CopyProps(p,a,b,c,d,e,f,g)
#define IMAPIProp_GetNamesFromIDs(p,a,b,c,d,e) (p)->lpVtbl->GetNamesFromIDs(p,a,b,c,d,e)
#define IMAPIProp_GetIDsFromNames(p,a,b,c,d)   (p)->lpVtbl->GetIDsFromNames(p,a,b,c,d)
#endif

typedef IMAPIProp *LPMAPIPROP;

#define KEEP_OPEN_READONLY      (0x00000001U)
#define KEEP_OPEN_READWRITE     (0x00000002U)
#define FORCE_SAVE              (0x00000004U)

/*****************************************************************************
 * IMsgStore interface
 */
#define INTERFACE IMsgStore
DECLARE_INTERFACE_(IMsgStore,IMAPIProp)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMAPIProp methods ***/
    STDMETHOD(GetLastError)(THIS_ HRESULT hRes, ULONG ulFlags, LPMAPIERROR *lppErr) PURE;
    STDMETHOD(SaveChanges)(THIS_ ULONG ulFlags) PURE;
    STDMETHOD(GetProps)(THIS_ LPSPropTagArray lpPropTags, ULONG ulFlags, ULONG *lpValues, LPSPropValue *lppProps) PURE;
    STDMETHOD(GetPropList)(THIS_ ULONG  ulFlags, LPSPropTagArray *lppPropTagArray) PURE;
    STDMETHOD(OpenProperty)(THIS_ ULONG ulPropTag, LPCIID lpIid, ULONG ulOpts, ULONG ulFlags, LPUNKNOWN *lppUnk) PURE;
    STDMETHOD(SetProps)(THIS_ ULONG cValues, LPSPropValue lpProps, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(DeleteProps)(THIS_ LPSPropTagArray lpPropTags, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyTo)(THIS_ ULONG ciidExclude, LPCIID lpIid, LPSPropTagArray lpProps, ULONG ulParam,
                      LPMAPIPROGRESS lpProgress, LPCIID lpIface,LPVOID lpDest, ULONG ulFlags,
                      LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyProps)(THIS_ LPSPropTagArray lpIncludeProps, ULONG ulParam, LPMAPIPROGRESS lpProgress,
                         LPCIID lpIid, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray *lppProblems) PURE;
    STDMETHOD(GetNamesFromIDs)(THIS_ LPSPropTagArray *lppPropTags, LPGUID lpIid, ULONG ulFlags, ULONG *lpCount,
                               LPMAPINAMEID **lpppNames) PURE;
    STDMETHOD(GetIDsFromNames)(THIS_ ULONG cPropNames, LPMAPINAMEID *lppNames, ULONG ulFlags, LPSPropTagArray *lppPropTags) PURE;
    /*** IMsgStore methods ***/
    STDMETHOD(Advise)(THIS_ ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulEventMask, LPMAPIADVISESINK lpAdviseSink,
                      ULONG * lpulConnection) PURE;
    STDMETHOD(Unadvise)(THIS_ ULONG ulConnection) PURE;
    STDMETHOD(CompareEntryIDs)(THIS_ ULONG cbEntryID1, LPENTRYID lpEntryID1, ULONG cbEntryID2, LPENTRYID lpEntryID2,
                ULONG ulFlags, ULONG * lpulResult) PURE;
    STDMETHOD(OpenEntry)(THIS_ ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, ULONG ulFlags, ULONG *lpulObjType,
                LPUNKNOWN *lppUnk) PURE;
    STDMETHOD(SetReceiveFolder)(THIS_ LPSTR lpszMessageClass, ULONG ulFlags, ULONG cbEntryID, LPENTRYID lpEntryID) PURE;
    STDMETHOD(GetReceiveFolder)(THIS_ LPSTR lpszMessageClass, ULONG ulFlags, ULONG * lpcbEntryID, LPENTRYID *lppEntryID,
                LPSTR *lppszExplicitClass) PURE;
    STDMETHOD(GetReceiveFolderTable)(THIS_ ULONG ulFlags, LPMAPITABLE * lppTable) PURE;
    STDMETHOD(StoreLogoff)(THIS_ ULONG * lpulFlags) PURE;
    STDMETHOD(AbortSubmit)(THIS_ ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulFlags) PURE;
    STDMETHOD(GetOutgoingQueue)(THIS_ ULONG ulFlags, LPMAPITABLE * lppTable) PURE;
    STDMETHOD(SetLockState)(THIS_ LPMESSAGE lpMessage, ULONG ulLockState) PURE;
    STDMETHOD(FinishedMsg)(THIS_ ULONG ulFlags, ULONG cbEntryID, LPENTRYID lpEntryID) PURE;
    STDMETHOD(NotifyNewMail)(THIS_ LPNOTIFICATION lpNotification) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define IMsgStore_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IMsgStore_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IMsgStore_Release(p)                   (p)->lpVtbl->Release(p)
        /*** IMAPIProp methods ***/
#define IMsgStore_GetLastError(p,a,b,c)        (p)->lpVtbl->GetLastError(p,a,b,c)
#define IMsgStore_SaveChanges(p,a)             (p)->lpVtbl->SaveChanges(p,a)
#define IMsgStore_GetProps(p,a,b,c,d)          (p)->lpVtbl->GetProps(p,a,b,c,d)
#define IMsgStore_GetPropList(p,a,b)           (p)->lpVtbl->GetPropList(p,a,b)
#define IMsgStore_OpenProperty(p,a,b,c,d,e)    (p)->lpVtbl->OpenProperty(p,a,b,c,d,e)
#define IMsgStore_SetProps(p,a,b,c)            (p)->lpVtbl->SetProps(p,a,b,c)
#define IMsgStore_DeleteProps(p,a,b)           (p)->lpVtbl->DeleteProps(p,a,b)
#define IMsgStore_CopyTo(p,a,b,c,d,e,f,g,h,i)  (p)->lpVtbl->CopyTo(p,a,b,c,d,e,f,g,h,i)
#define IMsgStore_CopyProps(p,a,b,c,d,e,f,g)   (p)->lpVtbl->CopyProps(p,a,b,c,d,e,f,g)
#define IMsgStore_GetNamesFromIDs(p,a,b,c,d,e) (p)->lpVtbl->GetNamesFromIDs(p,a,b,c,d,e)
#define IMsgStore_GetIDsFromNames(p,a,b,c,d)   (p)->lpVtbl->GetIDsFromNames(p,a,b,c,d)
        /*** IMsgStore methods ***/
#define IMsgStore_Advise(p,a,b,c,d,e)            (p)->lpVtbl->Advise(p,a,b,c,d,e)
#define IMsgStore_Unadvise(p,a)                  (p)->lpVtbl->Unadvise(p,a)
#define IMsgStore_CompareEntryIDs(p,a,b,c,d,e,f) (p)->lpVtbl->CompareEntryIDs(p,a,b,c,d,e,f)
#define IMsgStore_OpenEntry(p,a,b,c,d,e,f)       (p)->lpVtbl->OpenEntry(p,a,b,c,d,e,f)
#define IMsgStore_SetReceiveFolder(p,a,b,c,d)    (p)->lpVtbl->SetReceiveFolder(p,a,b,c,d)
#define IMsgStore_GetReceiveFolder(p,a,b,c,d,e)  (p)->lpVtbl->GetReceiveFolder(p,a,b,c,d,e)
#define IMsgStore_GetReceiveFolderTable(p,a,b)   (p)->lpVtbl->GetReceiveFolderTable(p,a,b)
#define IMsgStore_StoreLogoff(p,a)               (p)->lpVtbl->StoreLogoff(p,a)
#define IMsgStore_AbortSubmit(p,a,b,c)           (p)->lpVtbl->AbortSubmit(p,a,b,c)
#define IMsgStore_GetOutgoingQueue(p,a,b)        (p)->lpVtbl->GetOutgoingQueue(p,a,b)
#define IMsgStore_SetLockState(p,a,b)            (p)->lpVtbl->SetLockState(p,a,b)
#define IMsgStore_FinishedMsg(p,a,b,c)           (p)->lpVtbl->FinishedMsg(p,a,b,c)
#define IMsgStore_NotifyNewMail(p,a)             (p)->lpVtbl->NotifyNewMail(p,a)

#endif

typedef IMsgStore *LPMDB;

/*****************************************************************************
 * IMAPIContainer interface
 */
#define INTERFACE IMAPIContainer
DECLARE_INTERFACE_(IMAPIContainer,IMAPIProp)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMAPIProp methods ***/
    STDMETHOD(GetLastError)(THIS_ HRESULT hRes, ULONG ulFlags, LPMAPIERROR *lppErr) PURE;
    STDMETHOD(SaveChanges)(THIS_ ULONG ulFlags) PURE;
    STDMETHOD(GetProps)(THIS_ LPSPropTagArray lpPropTags, ULONG ulFlags, ULONG *lpValues, LPSPropValue *lppProps) PURE;
    STDMETHOD(GetPropList)(THIS_ ULONG  ulFlags, LPSPropTagArray *lppPropTagArray) PURE;
    STDMETHOD(OpenProperty)(THIS_ ULONG ulPropTag, LPCIID lpIid, ULONG ulOpts, ULONG ulFlags, LPUNKNOWN *lppUnk) PURE;
    STDMETHOD(SetProps)(THIS_ ULONG cValues, LPSPropValue lpProps, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(DeleteProps)(THIS_ LPSPropTagArray lpPropTags, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyTo)(THIS_ ULONG ciidExclude, LPCIID lpIid, LPSPropTagArray lpProps, ULONG ulParam,
                      LPMAPIPROGRESS lpProgress, LPCIID lpIface,LPVOID lpDest, ULONG ulFlags,
                      LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyProps)(THIS_ LPSPropTagArray lpIncludeProps, ULONG ulParam, LPMAPIPROGRESS lpProgress,
                         LPCIID lpIid, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray *lppProblems) PURE;
    STDMETHOD(GetNamesFromIDs)(THIS_ LPSPropTagArray *lppPropTags, LPGUID lpIid, ULONG ulFlags, ULONG *lpCount,
                               LPMAPINAMEID **lpppNames) PURE;
    STDMETHOD(GetIDsFromNames)(THIS_ ULONG cPropNames, LPMAPINAMEID *lppNames, ULONG ulFlags, LPSPropTagArray *lppPropTags) PURE;
    /*** IMAPIContainer methods ***/
    STDMETHOD(GetContentsTable)(THIS_ ULONG ulFlags, LPMAPITABLE * lppTable) PURE;
    STDMETHOD(GetHierarchyTable)(THIS_ ULONG ulFlags, LPMAPITABLE * lppTable) PURE;
    STDMETHOD(OpenEntry)(THIS_ ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, ULONG ulFlags,
                         ULONG * lpulObjType, LPUNKNOWN * lppUnk) PURE;
    STDMETHOD(SetSearchCriteria)(THIS_ LPSRestriction lpRestriction, LPENTRYLIST lpContainerList, ULONG ulSearchFlags) PURE;
    STDMETHOD(GetSearchCriteria)(THIS_ ULONG ulFlags, LPSRestriction * lppRestriction, LPENTRYLIST * lppContainerList,
                                 ULONG * lpulSearchState) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define IMAPIContainer_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IMAPIContainer_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IMAPIContainer_Release(p)                   (p)->lpVtbl->Release(p)
        /*** IMAPIProp methods ***/
#define IMAPIContainer_GetLastError(p,a,b,c)        (p)->lpVtbl->GetLastError(p,a,b,c)
#define IMAPIContainer_SaveChanges(p,a)             (p)->lpVtbl->SaveChanges(p,a)
#define IMAPIContainer_GetProps(p,a,b,c,d)          (p)->lpVtbl->GetProps(p,a,b,c,d)
#define IMAPIContainer_GetPropList(p,a,b)           (p)->lpVtbl->GetPropList(p,a,b)
#define IMAPIContainer_OpenProperty(p,a,b,c,d,e)    (p)->lpVtbl->OpenProperty(p,a,b,c,d,e)
#define IMAPIContainer_SetProps(p,a,b,c)            (p)->lpVtbl->SetProps(p,a,b,c)
#define IMAPIContainer_DeleteProps(p,a,b)           (p)->lpVtbl->DeleteProps(p,a,b)
#define IMAPIContainer_CopyTo(p,a,b,c,d,e,f,g,h,i)  (p)->lpVtbl->CopyTo(p,a,b,c,d,e,f,g,h,i)
#define IMAPIContainer_CopyProps(p,a,b,c,d,e,f,g)   (p)->lpVtbl->CopyProps(p,a,b,c,d,e,f,g)
#define IMAPIContainer_GetNamesFromIDs(p,a,b,c,d,e) (p)->lpVtbl->GetNamesFromIDs(p,a,b,c,d,e)
#define IMAPIContainer_GetIDsFromNames(p,a,b,c,d)   (p)->lpVtbl->GetIDsFromNames(p,a,b,c,d)
        /*** IMAPIContainer methods ***/
#define IMAPIContainer_GetContentsTable(p,a,b)      (p)->lpVtbl->GetContentsTable(p,a,b)
#define IMAPIContainer_GetHierarchyTable(p,a,b)     (p)->lpVtbl->GetHierarchyTable(p,a,b)
#define IMAPIContainer_OpenEntry(p,a,b,c,d,e,f)     (p)->lpVtbl->OpenEntry(p,a,b,c,d,e,f)
#define IMAPIContainer_SetSearchCriteria(p,a,b,c)   (p)->lpVtbl->SetSearchCriteria(p,a,b,c)
#define IMAPIContainer_GetSearchCriteria(p,a,b,c,d) (p)->lpVtbl->GetSearchCriteria(p,a,b,c,d)

#endif

/*****************************************************************************
 * IMAPIFolder interface
 */
#define INTERFACE IMAPIFolder
DECLARE_INTERFACE_(IMAPIFolder,IMAPIContainer)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMAPIProp methods ***/
    STDMETHOD(GetLastError)(THIS_ HRESULT hRes, ULONG ulFlags, LPMAPIERROR *lppErr) PURE;
    STDMETHOD(SaveChanges)(THIS_ ULONG ulFlags) PURE;
    STDMETHOD(GetProps)(THIS_ LPSPropTagArray lpPropTags, ULONG ulFlags, ULONG *lpValues, LPSPropValue *lppProps) PURE;
    STDMETHOD(GetPropList)(THIS_ ULONG  ulFlags, LPSPropTagArray *lppPropTagArray) PURE;
    STDMETHOD(OpenProperty)(THIS_ ULONG ulPropTag, LPCIID lpIid, ULONG ulOpts, ULONG ulFlags, LPUNKNOWN *lppUnk) PURE;
    STDMETHOD(SetProps)(THIS_ ULONG cValues, LPSPropValue lpProps, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(DeleteProps)(THIS_ LPSPropTagArray lpPropTags, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyTo)(THIS_ ULONG ciidExclude, LPCIID lpIid, LPSPropTagArray lpProps, ULONG ulParam,
                      LPMAPIPROGRESS lpProgress, LPCIID lpIface,LPVOID lpDest, ULONG ulFlags,
                      LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyProps)(THIS_ LPSPropTagArray lpIncludeProps, ULONG ulParam, LPMAPIPROGRESS lpProgress,
                         LPCIID lpIid, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray *lppProblems) PURE;
    STDMETHOD(GetNamesFromIDs)(THIS_ LPSPropTagArray *lppPropTags, LPGUID lpIid, ULONG ulFlags, ULONG *lpCount,
                               LPMAPINAMEID **lpppNames) PURE;
    STDMETHOD(GetIDsFromNames)(THIS_ ULONG cPropNames, LPMAPINAMEID *lppNames, ULONG ulFlags, LPSPropTagArray *lppPropTags) PURE;
    /*** IMAPIContainer methods ***/
    STDMETHOD(GetContentsTable)(THIS_ ULONG ulFlags, LPMAPITABLE * lppTable) PURE;
    STDMETHOD(GetHierarchyTable)(THIS_ ULONG ulFlags, LPMAPITABLE * lppTable) PURE;
    STDMETHOD(OpenEntry)(THIS_ ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, ULONG ulFlags,
                         ULONG * lpulObjType, LPUNKNOWN * lppUnk) PURE;
    STDMETHOD(SetSearchCriteria)(THIS_ LPSRestriction lpRestriction, LPENTRYLIST lpContainerList, ULONG ulSearchFlags) PURE;
    STDMETHOD(GetSearchCriteria)(THIS_ ULONG ulFlags, LPSRestriction * lppRestriction, LPENTRYLIST * lppContainerList,
                                 ULONG * lpulSearchState) PURE;
    /*** IMAPIFolder methods ***/
    STDMETHOD(CreateMessage)(THIS_ LPCIID lpInterface, ULONG ulFlags, LPMESSAGE *lppMessage) PURE;
    STDMETHOD(CopyMessages)(THIS_ LPENTRYLIST lpMsgList, LPCIID lpInterface, LPVOID lpDestFolder, ULONG ulUIParam,
                            LPMAPIPROGRESS lpProgress, ULONG ulFlags) PURE;
    STDMETHOD(DeleteMessages)(THIS_ LPENTRYLIST lpMsgList, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) PURE;
    STDMETHOD(CreateFolder)(THIS_ ULONG ulFolderType, LPSTR lpszFolderName, LPSTR lpszFolderComment, LPCIID lpInterface,
                            ULONG ulFlags, LPMAPIFOLDER lppFolder) PURE;
    STDMETHOD(CopyFolder)(THIS_ ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, LPVOID lpDestFolder,
                          LPSTR lpszNewFolderName, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) PURE;
    STDMETHOD(DeleteFolder)(THIS_ ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulUIParam, LPMAPIPROGRESS lpProgress,
                            ULONG ulFlags) PURE;
    STDMETHOD(SetReadFlags)(THIS_ LPENTRYLIST lpMsgList, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) PURE;
    STDMETHOD(GetMessageStatus)(THIS_ ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulFlags, ULONG * lpulMessageStatus) PURE;
    STDMETHOD(SetMessageStatus)(THIS_ ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulNewStatus,
                                ULONG ulNewStatusMask, ULONG * lpulOldStatus) PURE;
    STDMETHOD(SaveContentsSort)(THIS_ LPSSortOrderSet lpSortCriteria, ULONG ulFlags) PURE;
    STDMETHOD(EmptyFolder) (THIS_ ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define IMAPIFolder_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IMAPIFolder_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IMAPIFolder_Release(p)                   (p)->lpVtbl->Release(p)
        /*** IMAPIProp methods ***/
#define IMAPIFolder_GetLastError(p,a,b,c)        (p)->lpVtbl->GetLastError(p,a,b,c)
#define IMAPIFolder_SaveChanges(p,a)             (p)->lpVtbl->SaveChanges(p,a)
#define IMAPIFolder_GetProps(p,a,b,c,d)          (p)->lpVtbl->GetProps(p,a,b,c,d)
#define IMAPIFolder_GetPropList(p,a,b)           (p)->lpVtbl->GetPropList(p,a,b)
#define IMAPIFolder_OpenProperty(p,a,b,c,d,e)    (p)->lpVtbl->OpenProperty(p,a,b,c,d,e)
#define IMAPIFolder_SetProps(p,a,b,c)            (p)->lpVtbl->SetProps(p,a,b,c)
#define IMAPIFolder_DeleteProps(p,a,b)           (p)->lpVtbl->DeleteProps(p,a,b)
#define IMAPIFolder_CopyTo(p,a,b,c,d,e,f,g,h,i)  (p)->lpVtbl->CopyTo(p,a,b,c,d,e,f,g,h,i)
#define IMAPIFolder_CopyProps(p,a,b,c,d,e,f,g)   (p)->lpVtbl->CopyProps(p,a,b,c,d,e,f,g)
#define IMAPIFolder_GetNamesFromIDs(p,a,b,c,d,e) (p)->lpVtbl->GetNamesFromIDs(p,a,b,c,d,e)
#define IMAPIFolder_GetIDsFromNames(p,a,b,c,d)   (p)->lpVtbl->GetIDsFromNames(p,a,b,c,d)
        /*** IMAPIContainer methods ***/
#define IMAPIFolder_GetContentsTable(p,a,b)      (p)->lpVtbl->GetContentsTable(p,a,b)
#define IMAPIFolder_GetHierarchyTable(p,a,b)     (p)->lpVtbl->GetHierarchyTable(p,a,b)
#define IMAPIFolder_OpenEntry(p,a,b,c,d,e,f)     (p)->lpVtbl->OpenEntry(p,a,b,c,d,e,f)
#define IMAPIFolder_SetSearchCriteria(p,a,b,c)   (p)->lpVtbl->SetSearchCriteria(p,a,b,c)
#define IMAPIFolder_GetSearchCriteria(p,a,b,c,d) (p)->lpVtbl->GetSearchCriteria(p,a,b,c,d)
        /*** IMAPIFolder methods ***/
#define IMAPIFolder_CreateMessage(p,a,b,c)        (p)->lpVtbl->CreateMessage(p,a,b,c)
#define IMAPIFolder_CopyMessages(p,a,b,c,d,e,f)   (p)->lpVtbl->CopyMessages(p,a,b,c,d,e,f)
#define IMAPIFolder_DeleteMessages(p,a,b,c,d)     (p)->lpVtbl->DeleteMessages(p,a,b,c,d)
#define IMAPIFolder_CreateFolder(p,a,b,c,d,e,f)   (p)->lpVtbl->CreateFolder(p,a,b,c,d,e,f)
#define IMAPIFolder_CopyFolder(p,a,b,c,d,e,f,g,h) (p)->lpVtbl->CopyFolder(p,a,b,c,d,e,f,g,h)
#define IMAPIFolder_DeleteFolder(p,a,b,c,d,e)     (p)->lpVtbl->CreateFolder(p,a,b,c,d,e)
#define IMAPIFolder_SetReadFlags(p,a,b,c,d)       (p)->lpVtbl->SetReadFlags(p,a,b,c,d)
#define IMAPIFolder_GetMessageStatus(p,a,b,c,d)   (p)->lpVtbl->GetMessageStatus(p,a,b,c,d)
#define IMAPIFolder_SetMessageStatus(p,a,b,c,d,e) (p)->lpVtbl->SetMessageStatus(p,a,b,c,d,e)
#define IMAPIFolder_SaveContentsSort(p,a,b)       (p)->lpVtbl->SaveContentsSort(p,a,b)
#define IMAPIFolder_EmptyFolder(p,a,b,c)          (p)->lpVtbl->EmptyFolder(p,a,b,c)

#endif

typedef struct
{
    ULONG cb;
    BYTE  abEntry[MAPI_DIM];
} FLATENTRY, *LPFLATENTRY;

typedef struct
{
    ULONG cEntries;
    ULONG cbEntries;
    BYTE  abEntries[MAPI_DIM];
} FLATENTRYLIST, *LPFLATENTRYLIST;

typedef struct
{
    ULONG cb;
    BYTE  ab[MAPI_DIM];
} MTSID, *LPMTSID;

typedef struct
{
    ULONG cMTSIDs;
    ULONG cbMTSIDs;
    BYTE  abMTSIDs[MAPI_DIM];
} FLATMTSIDLIST, *LPFLATMTSIDLIST;

typedef struct _ADRENTRY
{
    ULONG        ulReserved1;
    ULONG        cValues;
    LPSPropValue rgPropVals;
} ADRENTRY, *LPADRENTRY;

typedef struct _ADRLIST
{
    ULONG    cEntries;
    ADRENTRY aEntries[MAPI_DIM];
} ADRLIST, *LPADRLIST;

/*****************************************************************************
 * IMessage interface
 */
#define INTERFACE IMessage
DECLARE_INTERFACE_(IMessage,IMAPIProp)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMAPIProp methods ***/
    STDMETHOD(GetLastError)(THIS_ HRESULT hRes, ULONG ulFlags, LPMAPIERROR *lppErr) PURE;
    STDMETHOD(SaveChanges)(THIS_ ULONG ulFlags) PURE;
    STDMETHOD(GetProps)(THIS_ LPSPropTagArray lpPropTags, ULONG ulFlags, ULONG *lpValues, LPSPropValue *lppProps) PURE;
    STDMETHOD(GetPropList)(THIS_ ULONG  ulFlags, LPSPropTagArray *lppPropTagArray) PURE;
    STDMETHOD(OpenProperty)(THIS_ ULONG ulPropTag, LPCIID lpIid, ULONG ulOpts, ULONG ulFlags, LPUNKNOWN *lppUnk) PURE;
    STDMETHOD(SetProps)(THIS_ ULONG cValues, LPSPropValue lpProps, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(DeleteProps)(THIS_ LPSPropTagArray lpPropTags, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyTo)(THIS_ ULONG ciidExclude, LPCIID lpIid, LPSPropTagArray lpProps, ULONG ulParam,
                      LPMAPIPROGRESS lpProgress, LPCIID lpIface,LPVOID lpDest, ULONG ulFlags,
                      LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyProps)(THIS_ LPSPropTagArray lpIncludeProps, ULONG ulParam, LPMAPIPROGRESS lpProgress,
                         LPCIID lpIid, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray *lppProblems) PURE;
    STDMETHOD(GetNamesFromIDs)(THIS_ LPSPropTagArray *lppPropTags, LPGUID lpIid, ULONG ulFlags, ULONG *lpCount,
                               LPMAPINAMEID **lpppNames) PURE;
    STDMETHOD(GetIDsFromNames)(THIS_ ULONG cPropNames, LPMAPINAMEID *lppNames, ULONG ulFlags, LPSPropTagArray *lppPropTags) PURE;
    /*** IMessage methods ***/
    STDMETHOD(GetAttachmentTable)(THIS_ ULONG ulFlags, LPMAPITABLE *lppTable) PURE;
    STDMETHOD(OpenAttach)(THIS_ ULONG ulAttachmentNum, LPCIID lpInterface, ULONG ulFlags, LPATTACH *lppAttach) PURE;
    STDMETHOD(CreateAttach)(THIS_ LPCIID lpInterface, ULONG ulFlags, ULONG *lpulAttachmentNum, LPATTACH *lppAttach) PURE;
    STDMETHOD(DeleteAttach)(THIS_ ULONG ulAttachmentNum, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) PURE;
    STDMETHOD(GetRecipientTable)(THIS_ ULONG ulFlags, LPMAPITABLE *lppTable) PURE;
    STDMETHOD(ModifyRecipients)(THIS_ ULONG ulFlags, LPADRLIST lpMods) PURE;
    STDMETHOD(SubmitMessage)(THIS_ ULONG ulFlags) PURE;
    STDMETHOD(SetReadFlag)(THIS_ ULONG ulFlags) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define IMessage_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IMessage_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IMessage_Release(p)                   (p)->lpVtbl->Release(p)
        /*** IMAPIProp methods ***/
#define IMessage_GetLastError(p,a,b,c)        (p)->lpVtbl->GetLastError(p,a,b,c)
#define IMessage_SaveChanges(p,a)             (p)->lpVtbl->SaveChanges(p,a)
#define IMessage_GetProps(p,a,b,c,d)          (p)->lpVtbl->GetProps(p,a,b,c,d)
#define IMessage_GetPropList(p,a,b)           (p)->lpVtbl->GetPropList(p,a,b)
#define IMessage_OpenProperty(p,a,b,c,d,e)    (p)->lpVtbl->OpenProperty(p,a,b,c,d,e)
#define IMessage_SetProps(p,a,b,c)            (p)->lpVtbl->SetProps(p,a,b,c)
#define IMessage_DeleteProps(p,a,b)           (p)->lpVtbl->DeleteProps(p,a,b)
#define IMessage_CopyTo(p,a,b,c,d,e,f,g,h,i)  (p)->lpVtbl->CopyTo(p,a,b,c,d,e,f,g,h,i)
#define IMessage_CopyProps(p,a,b,c,d,e,f,g)   (p)->lpVtbl->CopyProps(p,a,b,c,d,e,f,g)
#define IMessage_GetNamesFromIDs(p,a,b,c,d,e) (p)->lpVtbl->GetNamesFromIDs(p,a,b,c,d,e)
#define IMessage_GetIDsFromNames(p,a,b,c,d)   (p)->lpVtbl->GetIDsFromNames(p,a,b,c,d)
        /*** IMessage methods ***/
#define IMessage_GetAttachmentTable(p,a,b)    (p)->lpVtbl->GetAttachmentTable(p,a,b)
#define IMessage_OpenAttach(p,a,b,c,d)        (p)->lpVtbl->OpenAttach(p,a,b,c,d)
#define IMessage_CreateAttach(p,a,b,c,d)      (p)->lpVtbl->CreateAttach(p,a,b,c,d)
#define IMessage_DeleteAttach(p,a,b,c,d)      (p)->lpVtbl->DeleteAttach(p,a,b,c,d)
#define IMessage_GetRecipientTable(p,a,b)     (p)->lpVtbl->GetRecipientTable(p,a,b)
#define IMessage_ModifyRecipients(p,a,b)      (p)->lpVtbl->ModifyRecipients(p,a,b)
#define IMessage_SubmitMessage(p,a)           (p)->lpVtbl->SubmitMessage(p,a)
#define IMessage_SetReadFlag(p,a)             (p)->lpVtbl->SetReadFlag(p,a)

#endif

/* Message flags (PR_MESSAGE_FLAGS) */

#define MSGFLAG_READ         0x00000001U
#define MSGFLAG_UNMODIFIED   0x00000002U
#define MSGFLAG_SUBMIT       0x00000004U
#define MSGFLAG_UNSENT       0x00000008U
#define MSGFLAG_HASATTACH    0x00000010U
#define MSGFLAG_FROMME       0x00000020U

/*****************************************************************************
 * IAttach interface
 */
#define INTERFACE IAttach
DECLARE_INTERFACE_(IAttach,IMAPIProp)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMAPIProp methods ***/
    STDMETHOD(GetLastError)(THIS_ HRESULT hRes, ULONG ulFlags, LPMAPIERROR *lppErr) PURE;
    STDMETHOD(SaveChanges)(THIS_ ULONG ulFlags) PURE;
    STDMETHOD(GetProps)(THIS_ LPSPropTagArray lpPropTags, ULONG ulFlags, ULONG *lpValues, LPSPropValue *lppProps) PURE;
    STDMETHOD(GetPropList)(THIS_ ULONG  ulFlags, LPSPropTagArray *lppPropTagArray) PURE;
    STDMETHOD(OpenProperty)(THIS_ ULONG ulPropTag, LPCIID lpIid, ULONG ulOpts, ULONG ulFlags, LPUNKNOWN *lppUnk) PURE;
    STDMETHOD(SetProps)(THIS_ ULONG cValues, LPSPropValue lpProps, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(DeleteProps)(THIS_ LPSPropTagArray lpPropTags, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyTo)(THIS_ ULONG ciidExclude, LPCIID lpIid, LPSPropTagArray lpProps, ULONG ulParam,
                      LPMAPIPROGRESS lpProgress, LPCIID lpIface,LPVOID lpDest, ULONG ulFlags,
                      LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyProps)(THIS_ LPSPropTagArray lpIncludeProps, ULONG ulParam, LPMAPIPROGRESS lpProgress,
                         LPCIID lpIid, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray *lppProblems) PURE;
    STDMETHOD(GetNamesFromIDs)(THIS_ LPSPropTagArray *lppPropTags, LPGUID lpIid, ULONG ulFlags, ULONG *lpCount,
                               LPMAPINAMEID **lpppNames) PURE;
    STDMETHOD(GetIDsFromNames)(THIS_ ULONG cPropNames, LPMAPINAMEID *lppNames, ULONG ulFlags, LPSPropTagArray *lppPropTags) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define IAttach_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IAttach_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IAttach_Release(p)                   (p)->lpVtbl->Release(p)
        /*** IMAPIProp methods ***/
#define IAttach_GetLastError(p,a,b,c)        (p)->lpVtbl->GetLastError(p,a,b,c)
#define IAttach_SaveChanges(p,a)             (p)->lpVtbl->SaveChanges(p,a)
#define IAttach_GetProps(p,a,b,c,d)          (p)->lpVtbl->GetProps(p,a,b,c,d)
#define IAttach_GetPropList(p,a,b)           (p)->lpVtbl->GetPropList(p,a,b)
#define IAttach_OpenProperty(p,a,b,c,d,e)    (p)->lpVtbl->OpenProperty(p,a,b,c,d,e)
#define IAttach_SetProps(p,a,b,c)            (p)->lpVtbl->SetProps(p,a,b,c)
#define IAttach_DeleteProps(p,a,b)           (p)->lpVtbl->DeleteProps(p,a,b)
#define IAttach_CopyTo(p,a,b,c,d,e,f,g,h,i)  (p)->lpVtbl->CopyTo(p,a,b,c,d,e,f,g,h,i)
#define IAttach_CopyProps(p,a,b,c,d,e,f,g)   (p)->lpVtbl->CopyProps(p,a,b,c,d,e,f,g)
#define IAttach_GetNamesFromIDs(p,a,b,c,d,e) (p)->lpVtbl->GetNamesFromIDs(p,a,b,c,d,e)
#define IAttach_GetIDsFromNames(p,a,b,c,d)   (p)->lpVtbl->GetIDsFromNames(p,a,b,c,d)
#endif

/* Attachment flags */

#define NO_ATTACHMENT        0x00000000U
#define ATTACH_BY_VALUE      0x00000001U

#endif /*MAPIDEFS_H*/
