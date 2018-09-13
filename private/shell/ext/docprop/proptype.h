////////////////////////////////////////////////////////////////////////////////
//
// proptype.h
//
// Base types common to OLE Property Exchange and OLE Property Sets.
// See "OLE 2 Programmer's Reference, Volume 1", Appendix B for the
// published format for Property Sets.  The types defined here
// follow that format.
//
// Notes:
//  All strings in objects are stored in the following format:
//   DWORD size of buffer, DWORD length of string, string data, terminating 0.
//  The size of the buffer is inclusive of the DWORD, the length is not but
//  does include the ending 0.
//
//  EXTREMELY IMPORTANT!  All strings buffers must align on 32-bit boundaries.
//  Whenever one is allocated, the macro CBALIGN32 should be used to add
//  enough bytes to pad it out.
//
// Change history:
//
// Date         Who             What
// --------------------------------------------------------------------------
// 06/01/94     B. Wentz        Created file
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __proptype_h__
#define __proptype_h__

#include "nocrt.h"
#include <objbase.h>
#include <oleauto.h>
#include "offcapi.h"
#include "plex.h"

  // Property Id's for Summary Info, as defined in OLE 2 Prog. Ref.
#define PID_TITLE               0x00000002L
#define PID_SUBJECT             0x00000003L
#define PID_AUTHOR              0x00000004L
#define PID_KEYWORDS            0x00000005L
#define PID_COMMENTS            0x00000006L
#define PID_TEMPLATE            0x00000007L
#define PID_LASTAUTHOR          0x00000008L
#define PID_REVNUMBER           0x00000009L
#define PID_EDITTIME            0x0000000aL
#define PID_LASTPRINTED         0x0000000bL
#define PID_CREATE_DTM          0x0000000cL
#define PID_LASTSAVE_DTM        0x0000000dL
#define PID_PAGECOUNT           0x0000000eL
#define PID_WORDCOUNT           0x0000000fL
#define PID_CHARCOUNT           0x00000010L
#define PID_THUMBNAIL           0x00000011L
#define PID_APPNAME             0x00000012L
#define PID_DOC_SECURITY        0x00000013L

  // Range of PId's we understand
#define PID_SIFIRST             0x00000002L
#define PID_SILAST              0x00000013L
#define NUM_SI_PROPERTIES       (PID_SILAST - PID_SIFIRST + 1)

  // Property Id's for Document Summary Info, as define in OLE Property Exchange spec
#define PID_CATEGORY            0x00000002L
#define PID_PRESFORMAT          0x00000003L
#define PID_BYTECOUNT           0x00000004L
#define PID_LINECOUNT           0x00000005L
#define PID_PARACOUNT           0x00000006L
#define PID_SLIDECOUNT          0x00000007L
#define PID_NOTECOUNT           0x00000008L
#define PID_HIDDENCOUNT         0x00000009L
#define PID_MMCLIPCOUNT         0x0000000aL
#define PID_SCALE               0x0000000bL
#define PID_HEADINGPAIR         0x0000000cL
#define PID_DOCPARTS            0x0000000dL
#define PID_MANAGER             0x0000000eL
#define PID_COMPANY             0x0000000fL
#define PID_LINKSDIRTY          0x00000010L

  // Range of PID's we understand
#define PID_DSIFIRST            0x00000002L
#define PID_DSILAST             0x00000010L
#define NUM_DSI_PROPERTIES      (PID_DSILAST - PID_DSIFIRST + 1)

  // Beginning of the User-Defined range of properties.
#define PID_UDFIRST             0x00000002L

  // Predefined Property Id's in the standard
#define PID_DICT                0x00000000L  /* Property Id for the Property Set Dictionary */
#define PID_DOC_CODEPAGE        0x00000001L  /* Property Id for the Code Page */

  // Property Id masks to identify links and IMonikers
#define PID_LINKMASK            0x01000000L
#define PID_IMONIKERMASK        0x10000000L

  // Predefined Clipboard Format Identifiers in the standard
#define CFID_NONE        0L     /* No format name */
#define CFID_WINDOWS    -1L     /* Windows built-in clipboard format */
#define CFID_MACINTOSH  -2L     /* Macintosh format value */
#define CFID_FMTID      -3L     /* A FMTID */
 

  // Type for linked-lists.
typedef struct _LLIST *LPLLIST;
typedef struct _LLIST
{
  LPLLIST lpllistNext;
  LPLLIST lpllistPrev;
} LLIST;

  // Cache struct for linked-list routines
typedef struct _LLCACHE
{
  DWORD idw;
  LPLLIST lpllist;
} LLCACHE, FAR * LPLLCACHE;

  // Structure to hold the document headings
typedef struct _xheadpart
{
  BOOL fHeading;           // Is this node a heading??
  DWORD dwParts;           // Number of sections for this heading
  DWORD iHeading;          // Which heading does this document part belong to
  LPTSTR lpstz;             // The heading or the document part
} XHEADPART;

DEFPL (PLXHEADPART, XHEADPART, ixheadpartMax, ixheadpartMac, rgxheadpart);
typedef PLXHEADPART *LPPLXHEADPART;
typedef XHEADPART *LPXHEADPART;

  // Structure for linked list of User-defined properties
  // Note: This structure and everything it points to is allocated
  // with CoTaskMemAlloc.

typedef struct _UDPROP *LPUDPROP;
typedef struct _UDPROP
{
  LLIST         llist;
  LPTSTR        lpstzName;
  PROPID        propid;
  LPPROPVARIANT lppropvar;
  LPTSTR        lpstzLink;
  BOOL          fLinkInvalid;
} UDPROP;              

  // Macro to calculate how many bytes needed to round up to a 32-bit boundary.
#define CBALIGN32(cb) ((((cb+3) >> 2) << 2) - cb)

  // Macro to give the buffer size
  // (No longer used because we use PropVariants instead of the serialized
  // property set format)
//#define CBBUF(lpstz) (*(DWORD *) &((lpstz)[0]))

  // Macro to give string length
  // (No longer used because we use PropVariants instead of the serialized
  // property set format)
//#define CCHSTR(lpstz) (*(DWORD *) &(((LPBYTE)(lpstz))[sizeof(DWORD)]))
#define CBTSTR(lpstz) ( ( CchTszLen(lpstz) + 1 ) * sizeof(TCHAR) )

  // Macro to give pointer to beginning of lpsz
  // (No longer used because we use PropVariants instead of the serialized
  // property set format)
#define PSTR(lpstz) (lpstz)

  // Macro to give pointer to beginning of lpstz
  // (No longer used because we use PropVariants instead of the serialized
  // property set format)
#define PSTZ(lpstz) (lpstz)

//
// Our internal data for Summary Info
//
  // Max number of strings we store
#define cSIStringsMax           0x13      // The is actual PID of last string + 1
                                          // makes it easy to lookup string based on PID in array

  // Max number of filetimes we store, the offset to subtract from the index
#define cSIFTMax                0x4       // same as for cSIStringsMax
#define cSIFTOffset             0xa

// These are used to indicate whether a property has been set or not
#define bEditTime  1
#define bLastPrint 2
#define bCreated   4
#define bLastSave  8
#define bPageCount 16
#define bWordCount 32
#define bCharCount 64
#define bSecurity  128

  // Max number of VT_I4's we store
#define cdwSIMax                0x6    // same as for cSIStringsMax
#define cdwSIOffset             0xe

#define ifnSIMax                  4

// Used for OLE Automation
typedef struct _docprop
{
   LPVOID pIDocProp;              // Pointer to a DocumentProperty object
} DOCPROP;

DEFPL (PLDOCPROP, DOCPROP, idocpropMax, idocpropMac, rgdocprop);

// SummaryInformation data.

typedef struct _SINFO
{
  PROPVARIANT rgpropvar[ NUM_SI_PROPERTIES ];         // The actual properties.

  BOOL     fSaveSINail;         // Should we save the thumbnail?
  BOOL     fNoTimeTracking;     // Is time tracking disabled (Germany)

  BOOL (*lpfnFCPConvert)(LPTSTR, DWORD, DWORD, BOOL); // Code page converter
  BOOL (*lpfnFSzToNum)(NUM *, LPTSTR);                // Convert sz to double
  BOOL (*lpfnFNumToSz)(NUM *, LPTSTR, DWORD);         // Convert double to sz
  BOOL (*lpfnFUpdateStats)(HWND, LPSIOBJ, LPDSIOBJ);  // Update stats on stat tab

} SINFO, FAR * LPSINFO;

  // Macro to access the SINFO structure within the OFFICESUMINFO structure.
#define GETSINFO(lpSInfo) ( (LPSINFO) lpSInfo->m_lpData )

  // Indices into SINFO.rgpropvar array.
#define PVSI_TITLE               0x00L
#define PVSI_SUBJECT             0x01L
#define PVSI_AUTHOR              0x02L
#define PVSI_KEYWORDS            0x03L
#define PVSI_COMMENTS            0x04L
#define PVSI_TEMPLATE            0x05L
#define PVSI_LASTAUTHOR          0x06L
#define PVSI_REVNUMBER           0x07L
#define PVSI_EDITTIME            0x08L
#define PVSI_LASTPRINTED         0x09L
#define PVSI_CREATE_DTM          0x0aL
#define PVSI_LASTSAVE_DTM        0x0bL
#define PVSI_PAGECOUNT           0x0cL
#define PVSI_WORDCOUNT           0x0dL
#define PVSI_CHARCOUNT           0x0eL
#define PVSI_THUMBNAIL           0x0fL
#define PVSI_APPNAME             0x10L
#define PVSI_DOC_SECURITY        0x11L


//
// Our internal data for Document Summary Info
//
  // Max number of strings we store.
#define cDSIStringsMax          0x10   // same as for cSIStringsMax

  // Max number of VT_I4's we store
#define cdwDSIMax               0xe    // same as for cSIStringsMax

// These are used to indicate whether a property has been set or not
#define bByteCount   1
#define bLineCount   2
#define bParCount    4
#define bSlideCount  8
#define bNoteCount   16
#define bHiddenCount 32
#define bMMClipCount 64

#define ifnDSIMax                  1    // DSIObj only has one callback

typedef struct _DSINFO
{
  PROPVARIANT   rgpropvar[ NUM_DSI_PROPERTIES ];
  BYTE          bPropSet;

  BOOL (*lpfnFCPConvert)(LPTSTR, DWORD, DWORD, BOOL); // Code page converter

} DSINFO, FAR * LPDSINFO;

  // Macro to access the DSINFO structure within the OFFICESUMINFO structure.
#define GETDSINFO(lpDSInfo) ( (LPDSINFO) lpDSInfo->m_lpData )

  // Indices into ALLOBJS.propvarDocSumInfo array.
#define PVDSI_CATEGORY            0x00L
#define PVDSI_PRESFORMAT          0x01L
#define PVDSI_BYTECOUNT           0x02L
#define PVDSI_LINECOUNT           0x03L
#define PVDSI_PARACOUNT           0x04L
#define PVDSI_SLIDECOUNT          0x05L
#define PVDSI_NOTECOUNT           0x06L
#define PVDSI_HIDDENCOUNT         0x07L
#define PVDSI_MMCLIPCOUNT         0x08L
#define PVDSI_SCALE               0x09L
#define PVDSI_HEADINGPAIR         0x0AL
#define PVDSI_DOCPARTS            0x0BL
#define PVDSI_MANAGER             0x0CL
#define PVDSI_COMPANY             0x0DL
#define PVDSI_LINKSDIRTY          0x0EL


//
// Our internal data for User-defined properties
//

#define ifnUDMax                  ifnMax

  // The prefix for hidden property names
#define HIDDENPREFIX TEXT('_')

  // An iterator for User-defined Properties
typedef struct _UDITER
{
  LPUDPROP lpudp;
} UDITER;

typedef struct _UDINFO
{
    // Real object data
  DWORD     dwcLinks;                   // Number of links
  DWORD     dwcProps;                   // Number of user-defined properties
  LPUDPROP  lpudpHead;                  // Head of list of properties
  LPUDPROP  lpudpCache;
  CLSID     clsid;                      // The ClassID from the property set.

    // Temporary object data
  DWORD     dwcTmpLinks;                // Number of links
  DWORD     dwcTmpProps;                // Number of user-defined properties
  LPUDPROP  lpudpTmpHead;               // Head of list of properties
  LPUDPROP  lpudpTmpCache;

    // Application callback functions
  BOOL (*lpfnFCPConvert)(LPTSTR, DWORD, DWORD, BOOL); // Code page converter
  BOOL (*lpfnFSzToNum)(NUM *, LPTSTR);             // Convert sz to double
  BOOL (*lpfnFNumToSz)(NUM *, LPTSTR, DWORD);      // Convert double to sz

} UDINFO, FAR * LPUDINFO;

  // Macro to access the UDINFO structure within the OFFICESUMINFO structure.
#define GETUDINFO(lpUDInfo) ( (LPUDINFO) lpUDInfo->m_lpData )


  // Number of PIds we do understand.
#define cSIPIDS                   18
//Number of doc sum Pids
#define cDSIPIDS   16

#endif // __proptype_h__
