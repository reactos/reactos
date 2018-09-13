//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1998.
//
//  File:       propapi.h
//
//  Contents:   Stuff needed to make properties build for Nashville and
//              NT... definitions of Nt property api.
//
//
//  History:    07-Aug-95   BillMo      Created.
//              22-Feb-96   MikeHill    Fixed the non-WINNT version of
//                                      PROPASSERTMSG.
//              09-May-96   MikeHill    Update define to allow PropSet names
//                                      to be 255 characters (from 127).
//              31-May-96   MikeHill    Add OSVersion to RtlCreatePropSet.
//              18-Jun-96   MikeHill    Add OleAut32 wrappers to Unicode callouts.
//              15-Jul-96   MikeHill    - Remvd Win32 SEH exception-related code.
//                                      - WCHAR=>OLECHAR where applicable.
//                                      - Added RtlOnMappedStreamEvent
//                                      - Added Mac versions of PROPASSERT
//
//--------------------------------------------------------------------------


#ifndef _PROPAPI_H_
#define _PROPAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// typedef the function prototypes necessary
// for the UNICODECALLOUTS structure.
//

typedef UINT (WINAPI FNGETACP)(VOID);

typedef int (WINAPI FNMULTIBYTETOWIDECHAR)(
    IN UINT CodePage,
    IN DWORD dwFlags,
    IN LPCSTR lpMultiByteStr,
    IN int cchMultiByte,
    OUT LPWSTR lpWideCharStr,
    IN int cchWideChar);

typedef int (WINAPI FNWIDECHARTOMULTIBYTE)(
    IN UINT CodePage,
    IN DWORD dwFlags,
    IN LPCWSTR lpWideCharStr,
    IN int cchWideChar,
    OUT LPSTR lpMultiByteStr,
    IN int cchMultiByte,
    IN LPCSTR lpDefaultChar,
    IN LPBOOL lpUsedDefaultChar);

typedef BSTR FNSYSALLOCSTRING(
    OLECHAR FAR* pwsz);

typedef VOID FNSYSFREESTRING(
    BSTR pwsz);

//
// The UNICODECALLOUTS structure holds function
// pointers for routines needed by the property
// set routines in NTDLL.
//

typedef struct _UNICODECALLOUTS
{
    FNGETACP              *pfnGetACP;
    FNMULTIBYTETOWIDECHAR *pfnMultiByteToWideChar;
    FNWIDECHARTOMULTIBYTE *pfnWideCharToMultiByte;
    FNSYSALLOCSTRING      *pfnSysAllocString;
    FNSYSFREESTRING       *pfnSysFreeString;
} UNICODECALLOUTS;


//
// Define the default UNICODECALLOUTS
// values.
//

STDAPI_(BSTR)
PropSysAllocString(OLECHAR FAR* pwsz);

STDAPI_(VOID)
PropSysFreeString(BSTR bstr);

#define WIN32_UNICODECALLOUTS \
    GetACP,                   \
    MultiByteToWideChar,      \
    WideCharToMultiByte,      \
    PropSysAllocString,       \
    PropSysFreeString


// Is this pure NT (the IProp DLL needs to run on Win95)?
#if defined(WINNT) && !defined(IPROPERTY_DLL)

    // Set the function modifiers
#   define PROPSYSAPI NTSYSAPI
#   define PROPAPI NTAPI

    // How do we free mem allocated in the low-level propset routines?
#   define PropFreeHeap(h, z, p) RtlFreeHeap(h, z, p)

    // Assert implementations
#   define PROPASSERT ASSERT
#   define PROPASSERTMSG ASSERTMSG

    // Generate the default non-simple property stream/storage name
#   define PROPGENPROPERTYNAME(s,n) swprintf ((s), L"prop%lu", (n))

    // Ansi sprintf implementations
#   define PropSprintfA sprintf
#   define PropVsprintfA vsprintf

// Otherwise this is either the IProp DLL (NT, Win95, Mac),
// or it's the Win95 OLE32build.

#else // #if defined(WINNT) && !defined(IPROPERTY_DLL)

    // Set the function modifiers
#   define PROPSYSAPI
#   define PROPAPI

    // How do we free mem allocated in low-level propset routines?
#   define PropFreeHeap(h, z, p) CoTaskMemFree(p)

    // Assert implementations
#   if DBG==1
#       ifdef _MAC_NODOC
#           define PROPASSERT(f)                { if (!(f)) FnAssert(#f, NULL, __FILE__, __LINE__); }
#           define PROPASSERTMSG(szReason, f)   { if (!(f)) FnAssert(#f, szReason, __FILE__, __LINE__); }
#       else
#           define PROPASSERT(f) Win4Assert((f))
#           define PROPASSERTMSG(szReason, f) Win4Assert( (szReason && FALSE) || (f))
#       endif
#   else
#       define PROPASSERT(f)
#       define PROPASSERTMSG(szReason, f)
#   endif // #if DBG==1

    // Generate the default non-simple property stream/storage name
#   define PROPGENPROPERTYNAME(s,n) \
    { \
        memcpy ((s), OLESTR("prop"), sizeof (OLESTR("prop"))); \
        ULTOO  ((n), &(s)[sizeof("prop") - 1], 10); \
    }

    // Ansi sprintf implementations
#   ifdef IPROPERTY_DLL
#       define PropSprintfA sprintf
#       define PropVsprintfA vsprintf
#   else
#       define PropSprintfA wsprintfA
#       define PropVsprintfA wvsprintfA
#   endif	// #ifdef _MAC_NODOC

#endif // #if defined(WINNT) && !defined(IPROPERTY_DLL) ... #else


#ifdef IPROPERTY_DLL
#define MAX_ULONG ((ULONG) -1)
#endif


#define WC_PROPSET0     ((WCHAR) 0x0005)
#define OC_PROPSET0     ((OLECHAR) 0x0005)

#define CBIT_BYTE       8
#define CBIT_GUID       (CBIT_BYTE * sizeof(GUID))
#define CBIT_CHARMASK   5

// Allow for OC_PROPSET0 and a GUID mapped to a 32 character alphabet
#define CCH_PROPSET        (1 + (CBIT_GUID + CBIT_CHARMASK-1)/CBIT_CHARMASK)
#define CCH_PROPSETSZ      (CCH_PROPSET + 1)            // allow null
#define CCH_PROPSETCOLONSZ (1 + CCH_PROPSET + 1)        // allow colon and null

// Define the max property name in units of characters
// (and synonomously in wchars).

#define CCH_MAXPROPNAME    255                          // Matches Shell & Office
#define CCH_MAXPROPNAMESZ  (CCH_MAXPROPNAME + 1)        // allow null
#define CWC_MAXPROPNAME    CCH_MAXPROPNAME
#define CWC_MAXPROPNAMESZ  CCH_MAXPROPNAMESZ

#define MAX_DOCFILE_ENTRY_NAME  31

//+--------------------------------------------------------------------------
// Property Access APIs:
//---------------------------------------------------------------------------

typedef VOID *NTPROP;
typedef VOID *NTMAPPEDSTREAM;
typedef VOID *NTMEMORYALLOCATOR;


VOID PROPSYSAPI PROPAPI
RtlSetUnicodeCallouts(
    IN UNICODECALLOUTS *pUnicodeCallouts);

ULONG PROPSYSAPI PROPAPI
RtlGuidToPropertySetName(
    IN GUID const *pguid,
    OUT OLECHAR aocname[]);

NTSTATUS PROPSYSAPI PROPAPI
RtlPropertySetNameToGuid(
    IN ULONG cwcname,
    IN OLECHAR const aocname[],
    OUT GUID *pguid);

VOID 
PrSetUnicodeCallouts(
    IN UNICODECALLOUTS *pUnicodeCallouts);

ULONG 
PrGuidToPropertySetName(
    IN GUID const *pguid,
    OUT OLECHAR aocname[]);

NTSTATUS 
PrPropertySetNameToGuid(
    IN ULONG cwcname,
    IN OLECHAR const aocname[],
    OUT GUID *pguid);


// RtlCreatePropertySet Flags:

#define CREATEPROP_READ         0x0000 // request read access (must exist)
#define CREATEPROP_WRITE        0x0001 // request write access (must exist)
#define CREATEPROP_CREATE       0x0002 // create (overwrite if exists)
#define CREATEPROP_CREATEIF     0x0003 // create (open existing if exists)
#define CREATEPROP_DELETE       0x0004 // delete
#define CREATEPROP_MODEMASK     0x000f // open mode mask

#define CREATEPROP_NONSIMPLE    0x0010 // Is non-simple propset (in a storage)


// RtlCreateMappedStream Flags:

#define CMS_READONLY      0x00000000    // Opened for read-only
#define CMS_WRITE         0x00000001    // Opened for write access
#define CMS_TRANSACTED    0x00000002    // Is transacted


NTSTATUS PROPSYSAPI PROPAPI
RtlCreatePropertySet(
    IN NTMAPPEDSTREAM ms,       // Nt mapped stream
    IN USHORT Flags,	// NONSIMPLE|*1* of READ/WRITE/CREATE/CREATEIF/DELETE
    OPTIONAL IN GUID const *pguid, // property set guid (create only)
    OPTIONAL IN GUID const *pclsid,// CLASSID of propset code (create only)
    IN NTMEMORYALLOCATOR ma,	// caller's memory allocator
    IN ULONG LocaleId,		// Locale Id (create only)
    OPTIONAL OUT ULONG *pOSVersion,// OS Version field in header.
    IN OUT USHORT *pCodePage,   // IN: CodePage of property set (create only)
                                // OUT: CodePage of property set (always)
    OUT NTPROP *pnp);           // Nt property set context

NTSTATUS PROPSYSAPI PROPAPI
RtlClosePropertySet(
    IN NTPROP np);              // property set context

NTSTATUS 
PrCreatePropertySet(
    IN NTMAPPEDSTREAM ms,       // Nt mapped stream
    IN USHORT Flags,	// NONSIMPLE|*1* of READ/WRITE/CREATE/CREATEIF/DELETE
    OPTIONAL IN GUID const *pguid, // property set guid (create only)
    OPTIONAL IN GUID const *pclsid,// CLASSID of propset code (create only)
    IN NTMEMORYALLOCATOR ma,	// caller's memory allocator
    IN ULONG LocaleId,		// Locale Id (create only)
    OPTIONAL OUT ULONG *pOSVersion,// OS Version field in header.
    IN OUT USHORT *pCodePage,   // IN: CodePage of property set (create only)
                                // OUT: CodePage of property set (always)
    OUT NTPROP *pnp);           // Nt property set context

NTSTATUS 
PrClosePropertySet(
    IN NTPROP np);              // property set context

// *NOTE* RtlOnMappedStreamEvent assumes that the caller has
// already taken the CPropertySetStream::Lock.
#define CBSTM_UNKNOWN   ((ULONG) -1)
NTSTATUS PROPSYSAPI PROPAPI
RtlOnMappedStreamEvent(
    IN VOID *pv,               // property set context (NTPROP)
    IN VOID *pbuf,             // property set buffer
    IN ULONG cbstm );          // size of underlying stream, or CBSTM_UNKNOWN
NTSTATUS 
PrOnMappedStreamEvent(
    IN VOID *pv,               // property set context (NTPROP)
    IN VOID *pbuf,             // property set buffer
    IN ULONG cbstm );          // size of underlying stream, or CBSTM_UNKNOWN

NTSTATUS PROPSYSAPI PROPAPI
RtlFlushPropertySet(
    IN NTPROP np);              // property set context
NTSTATUS 
PrFlushPropertySet(
    IN NTPROP np);              // property set context

typedef struct _INDIRECTPROPERTY        // ip
{
    ULONG       Index;          // Index into Variant and PropId arrays
    LPOLESTR    poszName;       // Old indirect name, RtlSetProperties() only
} INDIRECTPROPERTY;

NTSTATUS PROPSYSAPI PROPAPI
RtlSetProperties(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPID pidNameFirst,     // first PROPID for new named properties
    IN PROPSPEC const aprs[],   // array of property specifiers
    OPTIONAL OUT PROPID apid[], // buffer for array of propids
    OPTIONAL OUT INDIRECTPROPERTY **ppip, // pointer to returned pointer to
                                // MAXULONG terminated array of Indirect
                                // properties w/indexes into aprs & avar
    OPTIONAL IN PROPVARIANT const avar[]);// array of properties with values
NTSTATUS 
PrSetProperties(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPID pidNameFirst,     // first PROPID for new named properties
    IN PROPSPEC const aprs[],   // array of property specifiers
    OPTIONAL OUT PROPID apid[], // buffer for array of propids
    OPTIONAL OUT INDIRECTPROPERTY **ppip, // pointer to returned pointer to
                                // MAXULONG terminated array of Indirect
                                // properties w/indexes into aprs & avar
    OPTIONAL IN PROPVARIANT const avar[]);// array of properties with values

NTSTATUS PROPSYSAPI PROPAPI
RtlQueryProperties(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPSPEC const aprs[],   // array of property specifiers
    OPTIONAL OUT PROPID apid[], // buffer for array of propids
    OPTIONAL OUT INDIRECTPROPERTY **ppip, // pointer to returned pointer to
                                // MAXULONG terminated array of Indirect
                                // properties w/indexes into aprs & avar
    IN OUT PROPVARIANT *avar,   // IN: array of uninitialized PROPVARIANTs,
                                // OUT: may contain pointers to alloc'd memory
    OUT ULONG *pcpropFound);    // count of property values retrieved
NTSTATUS 
PrQueryProperties(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPSPEC const aprs[],   // array of property specifiers
    OPTIONAL OUT PROPID apid[], // buffer for array of propids
    OPTIONAL OUT INDIRECTPROPERTY **ppip, // pointer to returned pointer to
                                // MAXULONG terminated array of Indirect
                                // properties w/indexes into aprs & avar
    IN OUT PROPVARIANT *avar,   // IN: array of uninitialized PROPVARIANTs,
                                // OUT: may contain pointers to alloc'd memory
    OUT ULONG *pcpropFound);    // count of property values retrieved



#define ENUMPROP_NONAMES        0x00000001      // return property IDs only

NTSTATUS PROPSYSAPI PROPAPI
RtlEnumerateProperties(
    IN NTPROP np,               // property set context
    IN ULONG Flags,             // flags: No Names (propids only), etc.
    IN OUT ULONG *pkey,         // bookmark; caller set to 0 before 1st call
    IN OUT ULONG *pcprop,       // pointer to property count
    OPTIONAL OUT PROPSPEC aprs[],// IN: array of uninitialized PROPSPECs
                                // OUT: may contain pointers to alloc'd strings
    OPTIONAL OUT STATPROPSTG asps[]);
                                // IN: array of uninitialized STATPROPSTGs
                                // OUT: may contain pointers to alloc'd strings

NTSTATUS PROPSYSAPI PROPAPI
RtlQueryPropertyNames(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPID const *apid,      // PROPID array
    OUT OLECHAR *aposz[]        // OUT pointers to allocated strings
    );

NTSTATUS PROPSYSAPI PROPAPI
RtlSetPropertyNames(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPID const *apid,      // PROPID array
    IN OLECHAR const * const aposz[] // pointers to property names
    );

NTSTATUS PROPSYSAPI PROPAPI
RtlSetPropertySetClassId(
    IN NTPROP np,               // property set context
    IN GUID const *pclsid       // new CLASSID of propset code
    );

NTSTATUS PROPSYSAPI PROPAPI
RtlQueryPropertySet(
    IN NTPROP np,               // property set context
    OUT STATPROPSETSTG *pspss   // buffer for property set stat information
    );

NTSTATUS PROPSYSAPI PROPAPI
RtlEnumeratePropertySets(
    IN HANDLE hstg,             // structured storage handle
    IN BOOLEAN fRestart,        // restart scan
    IN OUT ULONG *pcspss,       // pointer to count of STATPROPSETSTGs
    IN OUT GUID *pkey,          // bookmark
    OUT STATPROPSETSTG *pspss   // array of STATPROPSETSTGs
    );





NTSTATUS 
PrEnumerateProperties(
    IN NTPROP np,               // property set context
    IN ULONG Flags,             // flags: No Names (propids only), etc.
    IN OUT ULONG *pkey,         // bookmark; caller set to 0 before 1st call
    IN OUT ULONG *pcprop,       // pointer to property count
    OPTIONAL OUT PROPSPEC aprs[],// IN: array of uninitialized PROPSPECs
                                // OUT: may contain pointers to alloc'd strings
    OPTIONAL OUT STATPROPSTG asps[]);
                                // IN: array of uninitialized STATPROPSTGs
                                // OUT: may contain pointers to alloc'd strings

NTSTATUS 
PrQueryPropertyNames(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPID const *apid,      // PROPID array
    OUT OLECHAR *aposz[]        // OUT pointers to allocated strings
    );

NTSTATUS 
PrSetPropertyNames(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPID const *apid,      // PROPID array
    IN OLECHAR const * const aposz[] // pointers to property names
    );

NTSTATUS 
PrSetPropertySetClassId(
    IN NTPROP np,               // property set context
    IN GUID const *pclsid       // new CLASSID of propset code
    );

NTSTATUS 
PrQueryPropertySet(
    IN NTPROP np,               // property set context
    OUT STATPROPSETSTG *pspss   // buffer for property set stat information
    );

NTSTATUS 
PrEnumeratePropertySets(
    IN HANDLE hstg,             // structured storage handle
    IN BOOLEAN fRestart,        // restart scan
    IN OUT ULONG *pcspss,       // pointer to count of STATPROPSETSTGs
    IN OUT GUID *pkey,          // bookmark
    OUT STATPROPSETSTG *pspss   // array of STATPROPSETSTGs
    );



#ifdef __cplusplus
}
#endif

#endif // ifndef _PROPAPI_H_
