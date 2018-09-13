/*
 *  M A P I C O D E . H
 *
 *  Status Codes returned by MAPI routines
 *
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 */

#ifndef MAPICODE_H
#define MAPICODE_H

#if defined (WIN32) && !defined (_WIN32)
#define _WIN32
#endif

/* Define S_OK and ITF_* */

#ifdef _WIN32
#include <winerror.h>
#endif

/*
 *  MAPI Status codes follow the style of OLE 2.0 sCodes as defined in the
 *  OLE 2.0 Programmer's Reference and header file scode.h (Windows 3.x)
 *  or winerror.h (Windows NT and Windows 95).
 *
 */

/*  On Windows 3.x, status codes have 32-bit values as follows:
 *
 *   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+---------------------+-------+-------------------------------+
 *  |S|       Context       | Facil |               Code            |
 *  +-+---------------------+-------+-------------------------------+
 *
 *  where
 *
 *      S - is the severity code
 *
 *          0 - SEVERITY_SUCCESS
 *          1 - SEVERITY_ERROR
 *
 *      Context - context info
 *
 *      Facility - is the facility code
 *
 *          0x0 - FACILITY_NULL     generally useful errors ([SE]_*)
 *          0x1 - FACILITY_RPC      remote procedure call errors (RPC_E_*)
 *          0x2 - FACILITY_DISPATCH late binding dispatch errors
 *          0x3 - FACILITY_STORAGE  storage errors (STG_E_*)
 *          0x4 - FACILITY_ITF      interface-specific errors
 *
 *      Code - is the facility's status code
 *
 *
 */

/*
 *  On Windows NT 3.5 and Windows 95, scodes are 32-bit values
 *  laid out as follows:
 *  
 *    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+---------------------+-------------------------------+
 *   |S|R|C|N|r|    Facility         |               Code            |
 *   +-+-+-+-+-+---------------------+-------------------------------+
 *  
 *   where
 *  
 *      S - Severity - indicates success/fail
 *  
 *          0 - Success
 *          1 - Fail (COERROR)
 *  
 *      R - reserved portion of the facility code, corresponds to NT's
 *          second severity bit.
 *  
 *      C - reserved portion of the facility code, corresponds to NT's
 *          C field.
 *  
 *      N - reserved portion of the facility code. Used to indicate a
 *          mapped NT status value.
 *  
 *      r - reserved portion of the facility code. Reserved for internal
 *          use. Used to indicate HRESULT values that are not status
 *          values, but are instead message ids for display strings.
 *  
 *      Facility - is the facility code
 *          FACILITY_NULL                    0x0
 *          FACILITY_RPC                     0x1
 *          FACILITY_DISPATCH                0x2
 *          FACILITY_STORAGE                 0x3
 *          FACILITY_ITF                     0x4
 *          FACILITY_WIN32                   0x7
 *          FACILITY_WINDOWS                 0x8
 *  
 *      Code - is the facility's status code
 *  
 */




/*
 *  We can't use OLE 2.0 macros to build sCodes because the definition has
 *  changed and we wish to conform to the new definition.
 */
#define MAKE_MAPI_SCODE(sev,fac,code) \
    ((SCODE) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )

/* The following two macros are used to build OLE 2.0 style sCodes */

#define MAKE_MAPI_E( err )  (MAKE_MAPI_SCODE( 1, FACILITY_ITF, err ))
#define MAKE_MAPI_S( warn ) (MAKE_MAPI_SCODE( 0, FACILITY_ITF, warn ))

#ifdef  SUCCESS_SUCCESS
#undef  SUCCESS_SUCCESS
#endif
#define SUCCESS_SUCCESS     0L

/* General errors (used by more than one MAPI object) */

#define MAPI_E_CALL_FAILED                              E_FAIL
#define MAPI_E_NOT_ENOUGH_MEMORY                        E_OUTOFMEMORY
#define MAPI_E_INVALID_PARAMETER                        E_INVALIDARG
#define MAPI_E_INTERFACE_NOT_SUPPORTED                  E_NOINTERFACE
#define MAPI_E_NO_ACCESS                                E_ACCESSDENIED

#define MAPI_E_NO_SUPPORT                               MAKE_MAPI_E( 0x102 )
#define MAPI_E_BAD_CHARWIDTH                            MAKE_MAPI_E( 0x103 )
#define MAPI_E_STRING_TOO_LONG                          MAKE_MAPI_E( 0x105 )
#define MAPI_E_UNKNOWN_FLAGS                            MAKE_MAPI_E( 0x106 )
#define MAPI_E_INVALID_ENTRYID                          MAKE_MAPI_E( 0x107 )
#define MAPI_E_INVALID_OBJECT                           MAKE_MAPI_E( 0x108 )
#define MAPI_E_OBJECT_CHANGED                           MAKE_MAPI_E( 0x109 )
#define MAPI_E_OBJECT_DELETED                           MAKE_MAPI_E( 0x10A )
#define MAPI_E_BUSY                                     MAKE_MAPI_E( 0x10B )
#define MAPI_E_NOT_ENOUGH_DISK                          MAKE_MAPI_E( 0x10D )
#define MAPI_E_NOT_ENOUGH_RESOURCES                     MAKE_MAPI_E( 0x10E )
#define MAPI_E_NOT_FOUND                                MAKE_MAPI_E( 0x10F )
#define MAPI_E_VERSION                                  MAKE_MAPI_E( 0x110 )
#define MAPI_E_LOGON_FAILED                             MAKE_MAPI_E( 0x111 )
#define MAPI_E_SESSION_LIMIT                            MAKE_MAPI_E( 0x112 )
#define MAPI_E_USER_CANCEL                              MAKE_MAPI_E( 0x113 )
#define MAPI_E_UNABLE_TO_ABORT                          MAKE_MAPI_E( 0x114 )
#define MAPI_E_NETWORK_ERROR                            MAKE_MAPI_E( 0x115 )
#define MAPI_E_DISK_ERROR                               MAKE_MAPI_E( 0x116 )
#define MAPI_E_TOO_COMPLEX                              MAKE_MAPI_E( 0x117 )
#define MAPI_E_BAD_COLUMN                               MAKE_MAPI_E( 0x118 )
#define MAPI_E_EXTENDED_ERROR                           MAKE_MAPI_E( 0x119 )
#define MAPI_E_COMPUTED                                 MAKE_MAPI_E( 0x11A )
#define MAPI_E_CORRUPT_DATA                             MAKE_MAPI_E( 0x11B )
#define MAPI_E_UNCONFIGURED                             MAKE_MAPI_E( 0x11C )
#define MAPI_E_FAILONEPROVIDER                          MAKE_MAPI_E( 0x11D )
#define MAPI_E_UNKNOWN_CPID                             MAKE_MAPI_E( 0x11E )
#define MAPI_E_UNKNOWN_LCID                             MAKE_MAPI_E( 0x11F )

/* Flavors of E_ACCESSDENIED, used at logon */

#define MAPI_E_PASSWORD_CHANGE_REQUIRED                 MAKE_MAPI_E( 0x120 )
#define MAPI_E_PASSWORD_EXPIRED                         MAKE_MAPI_E( 0x121 )
#define MAPI_E_INVALID_WORKSTATION_ACCOUNT              MAKE_MAPI_E( 0x122 )
#define MAPI_E_INVALID_ACCESS_TIME                      MAKE_MAPI_E( 0x123 )
#define MAPI_E_ACCOUNT_DISABLED                         MAKE_MAPI_E( 0x124 )

/* MAPI base function and status object specific errors and warnings */

#define MAPI_E_END_OF_SESSION                           MAKE_MAPI_E( 0x200 )
#define MAPI_E_UNKNOWN_ENTRYID                          MAKE_MAPI_E( 0x201 )
#define MAPI_E_MISSING_REQUIRED_COLUMN                  MAKE_MAPI_E( 0x202 )
#define MAPI_W_NO_SERVICE                               MAKE_MAPI_S( 0x203 )

/* Property specific errors and warnings */

#define MAPI_E_BAD_VALUE                                MAKE_MAPI_E( 0x301 )
#define MAPI_E_INVALID_TYPE                             MAKE_MAPI_E( 0x302 )
#define MAPI_E_TYPE_NO_SUPPORT                          MAKE_MAPI_E( 0x303 )
#define MAPI_E_UNEXPECTED_TYPE                          MAKE_MAPI_E( 0x304 )
#define MAPI_E_TOO_BIG                                  MAKE_MAPI_E( 0x305 )
#define MAPI_E_DECLINE_COPY                             MAKE_MAPI_E( 0x306 )
#define MAPI_E_UNEXPECTED_ID                            MAKE_MAPI_E( 0x307 )

#define MAPI_W_ERRORS_RETURNED                          MAKE_MAPI_S( 0x380 )

/* Table specific errors and warnings */

#define MAPI_E_UNABLE_TO_COMPLETE                       MAKE_MAPI_E( 0x400 )
#define MAPI_E_TIMEOUT                                  MAKE_MAPI_E( 0x401 )
#define MAPI_E_TABLE_EMPTY                              MAKE_MAPI_E( 0x402 )
#define MAPI_E_TABLE_TOO_BIG                            MAKE_MAPI_E( 0x403 )

#define MAPI_E_INVALID_BOOKMARK                         MAKE_MAPI_E( 0x405 )

#define MAPI_W_POSITION_CHANGED                         MAKE_MAPI_S( 0x481 )
#define MAPI_W_APPROX_COUNT                             MAKE_MAPI_S( 0x482 )

/* Transport specific errors and warnings */

#define MAPI_E_WAIT                                     MAKE_MAPI_E( 0x500 )
#define MAPI_E_CANCEL                                   MAKE_MAPI_E( 0x501 )
#define MAPI_E_NOT_ME                                   MAKE_MAPI_E( 0x502 )

#define MAPI_W_CANCEL_MESSAGE                           MAKE_MAPI_S( 0x580 )

/* Message Store, Folder, and Message specific errors and warnings */

#define MAPI_E_CORRUPT_STORE                            MAKE_MAPI_E( 0x600 )
#define MAPI_E_NOT_IN_QUEUE                             MAKE_MAPI_E( 0x601 )
#define MAPI_E_NO_SUPPRESS                              MAKE_MAPI_E( 0x602 )
#define MAPI_E_COLLISION                                MAKE_MAPI_E( 0x604 )
#define MAPI_E_NOT_INITIALIZED                          MAKE_MAPI_E( 0x605 )
#define MAPI_E_NON_STANDARD                             MAKE_MAPI_E( 0x606 )
#define MAPI_E_NO_RECIPIENTS                            MAKE_MAPI_E( 0x607 )
#define MAPI_E_SUBMITTED                                MAKE_MAPI_E( 0x608 )
#define MAPI_E_HAS_FOLDERS                              MAKE_MAPI_E( 0x609 )
#define MAPI_E_HAS_MESSAGES                             MAKE_MAPI_E( 0x60A )
#define MAPI_E_FOLDER_CYCLE                             MAKE_MAPI_E( 0x60B )

#define MAPI_W_PARTIAL_COMPLETION                       MAKE_MAPI_S( 0x680 )

/* Address Book specific errors and warnings */

#define MAPI_E_AMBIGUOUS_RECIP                          MAKE_MAPI_E( 0x700 )

/* The range 0x0800 to 0x08FF is reserved */

/* Obsolete typing shortcut that will go away eventually. */
#ifndef MakeResult
#define MakeResult(_s)  ResultFromScode(_s)
#endif

/* We expect these to eventually be defined by OLE, but for now,
 * here they are.  When OLE defines them they can be much more
 * efficient than these, but these are "proper" and don't make
 * use of any hidden tricks.
 */
#ifndef HR_SUCCEEDED
#define HR_SUCCEEDED(_hr) SUCCEEDED((SCODE)(_hr))
#define HR_FAILED(_hr) FAILED((SCODE)(_hr))
#endif

#endif  /* MAPICODE_H */
