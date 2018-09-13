//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1993-1998
//
// File:	propset.h
//
// Contents:	OLE Appendix B property set structure definitions
//
// History:	15-Jul-94       brianb    created
//		15-Aug-94       SethuR    revised
//              22-Feb-96       MikeHill  Changed cb in tagENTRY to cch.
//              28-May-96       MikeHill  Changed OSVER_* to OSKIND_*.
//
//---------------------------------------------------------------------------

#ifndef _PROPSET_H_
#define _PROPSET_H_

// CBMAXPROPSETSTREAM must be a power of 2.
#define CBMAXPROPSETSTREAM	(256 * 1024)

#define IsIndirectVarType(vt)			\
	    ((vt) == VT_STREAM ||		\
	     (vt) == VT_STREAMED_OBJECT ||	\
	     (vt) == VT_STORAGE ||		\
	     (vt) == VT_STORED_OBJECT)


// Defines for the high order WORD of dwOSVer:

#define OSKIND_WINDOWS      0x0000
#define OSKIND_MACINTOSH    0x0001
#define OSKIND_WIN32        0x0002


typedef struct tagFORMATIDOFFSET	// fo
{
    FMTID	fmtid;
    DWORD	dwOffset;
} FORMATIDOFFSET;

#define CB_FORMATIDOFFSET	sizeof(FORMATIDOFFSET)


typedef struct tagPROPERTYSETHEADER	// ph
{
    WORD        wByteOrder;	// Always 0xfffe
    WORD        wFormat;	// Always 0
    DWORD       dwOSVer;	// System version
    CLSID       clsid;		// Application CLSID
    DWORD       reserved;	// reserved (must be at least 1)
} PROPERTYSETHEADER;

#define CB_PROPERTYSETHEADER	sizeof(PROPERTYSETHEADER)
#define PROPSET_BYTEORDER       0xFFFE


typedef struct tagPROPERTYIDOFFSET	// po
{
    DWORD       propid;
    DWORD       dwOffset;
} PROPERTYIDOFFSET;

#define CB_PROPERTYIDOFFSET	sizeof(PROPERTYIDOFFSET)


typedef struct tagPROPERTYSECTIONHEADER	// sh
{
    DWORD       cbSection;
    DWORD       cProperties;
    PROPERTYIDOFFSET rgprop[1];
} PROPERTYSECTIONHEADER;

#define CB_PROPERTYSECTIONHEADER FIELD_OFFSET(PROPERTYSECTIONHEADER, rgprop)


typedef struct tagSERIALIZEDPROPERTYVALUE		// prop
{
    DWORD	dwType;
    BYTE	rgb[1];
} SERIALIZEDPROPERTYVALUE;

#define CB_SERIALIZEDPROPERTYVALUE  FIELD_OFFSET(SERIALIZEDPROPERTYVALUE, rgb)


typedef struct tagENTRY			// ent
{
    DWORD propid;
    DWORD cch;			// Includes trailing '\0' or L'\0'
    char  sz[1];		// WCHAR if UNICODE CodePage
} ENTRY;

#define CB_ENTRY		FIELD_OFFSET(ENTRY, sz)


typedef struct tagDICTIONARY		// dy
{
    DWORD	cEntries;
    ENTRY	rgEntry[1];
} DICTIONARY;

#define CB_DICTIONARY		FIELD_OFFSET(DICTIONARY, rgEntry)

#endif // _PROPSET_H_
