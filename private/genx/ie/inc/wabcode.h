/*
 *  WABCODE.H
 *
 *  Status Codes returned by WAB routines
 *
 *  Copyright 1993-1998 Microsoft Corporation. All Rights Reserved.
 */

#if !defined(MAPICODE_H) && !defined(WABCODE_H)
#define WABCODE_H

/* Define S_OK and ITF_* */

#ifdef WIN32
#include <objerror.h>
#endif

/*
 *  WAB Status codes follow the style of OLE 2.0 sCodes as defined in the
 *  OLE 2.0 Programmer's Reference and header file scode.h (Windows 3.x)
 *  or objerror.h (Windows NT 3.5 and Windows 95).
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

/* General errors (used by more than one WAB object) */

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

/* WAB base function and status object specific errors and warnings */

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

#define MAPI_W_PARTIAL_COMPLETION						 MAKE_MAPI_S( 0x680 )

/* Address Book specific errors and warnings */

#define MAPI_E_AMBIGUOUS_RECIP                          MAKE_MAPI_E( 0x700 )


/* Miscellaneous errors */
#define MAPI_E_COLLISION                                MAKE_MAPI_E( 0x604 )
#define MAPI_E_NOT_INITIALIZED                          MAKE_MAPI_E( 0x605 )
#define MAPI_E_FOLDER_CYCLE                             MAKE_MAPI_E( 0x60B )

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

#endif  /* WABCODE_H */
