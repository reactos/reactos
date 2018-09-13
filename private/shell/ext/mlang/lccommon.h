/*
 * Declarations common to compiler and detector.
 *
 * Copyright (C) 1996, 1997, Microsoft Corp.  All rights reserved.
 * 
 *  History:    1-Feb-97    BobP      Created
 *              5-Aug-97    BobP      Added Unicode support, and persisting
 *                                    Charmaps in the data file.
 */

#ifndef __INC_LCDCOMP_COMMON_H
#define __INC_LCDCOMP_COMMON_H

/****************************************************************/

// Compiled detection data file, in lcdetect.dll module directory
#define DETECTION_DATA_FILENAME "mlang.dat"

// Limits
#define MAX7BITLANG 30
#define MAX8BITLANG 30
#define MAXUNICODELANG 30
#define MAXSUBLANG 5			// max # of sublanguages or codepages per lang
#define MAXCHARMAPS 10			// max # of Charmaps, overall


// Special case entries for the training script and detector.
// These language IDs are never returned by the detector.

#define LANGID_UNKNOWN		0x400
#define LANGID_LATIN_GROUP	0x401
#define LANGID_CJK_GROUP	0x402

// Value type of a histogram array index.
// This is the output value of the SBCS/DBCS or WCHAR reduction mapping,
// and is used as the index into the n-gram arrays and for the Unicode
// language group IDs.
// 
typedef unsigned char HIdx;
typedef HIdx *PHIdx;
#define HIDX_MAX UCHAR_MAX		// keep consistent w/ HIdx

// Fixed index values for mapped characters
#define HIDX_IGNORE		0
#define HIDX_EXTD		1
#define HIDX_LETTER_A	2
#define HIDX_LETTER_Z	(HIDX_LETTER_A + 25)


// Value type of a histogram element
typedef unsigned char HElt;
typedef HElt *PHElt;
#define HELT_MAX UCHAR_MAX		// keep consistent w/ HElt


#define LANG7_DIM 3				// 7-bit language uses trigrams

// Fixed IDs of the Charmaps
#define CHARMAP_UNICODE  0		// Built from RANGE directives
#define CHARMAP_7BITLANG 1		// Built from CHARMAP 1
#define CHARMAP_8BITLANG 2		// From CHARMAP 2
#define CHARMAP_CODEPAGE 3		// From CHARMAP 3
#define CHARMAP_U27BIT 4		// Built internally for Unicode to 7-bit lang
#define CHARMAP_NINTERNAL 5		// First ID for dynamic subdetection maps


#define DEFAULT_7BIT_EDGESIZE 28
#define DEFAULT_8BIT_EDGESIZE 155


#define UNICODE_DEFAULT_CHAR_SCORE  50

/****************************************************************/

// Compiled file format.

// These declarations directly define the raw file format.
// Be careful making changes here, and be sure to change the
// header version number when appropriate.

#define APP_SIGNATURE 0x5444434C	// "LCDT"
#define APP_VERSION   2

enum SectionTypes {				// for m_dwType below
	SECTION_TYPE_LANGUAGE = 1,	// any language definition
	SECTION_TYPE_HISTOGRAM = 2,	// any histogram
	SECTION_TYPE_MAP = 3		// any character mapping table
};

enum DetectionType {			// SBCS/DBCS detection types
	DETECT_NOTDEFINED = 0, 
	DETECT_7BIT,
	DETECT_8BIT,
	DETECT_UNICODE,
		
	DETECT_NTYPES
};

// FileHeader -- one-time header at start of file

typedef struct FileHeader {
	DWORD	m_dwAppSig;			// 'DTCT'
	DWORD	m_dwVersion;
	DWORD	m_dwHdrSizeBytes;	// byte offset of 1st real section
	DWORD	m_dwN7BitLanguages;
	DWORD	m_dwN8BitLanguages;
	DWORD	m_dwNUnicodeLanguages;
	DWORD	m_dwNCharmaps;
	DWORD	m_dwMin7BitScore;
	DWORD	m_dwMin8BitScore;
	DWORD	m_dwMinUnicodeScore;
	DWORD	m_dwRelativeThreshhold;
	DWORD	m_dwDocPctThreshhold;
	DWORD	m_dwChunkSize;
} FileHeader;
typedef FileHeader *PFileHeader;

// FileSection -- common header that begins each file section

typedef struct FileSection {
	DWORD	m_dwSizeBytes;		// section size incl. header (offset to next)
	DWORD	m_dwType;			// type of entry this section
} FileSection;
typedef FileSection *PFileSection;

// FileLanguageSection -- 1st entry of sequence for an SBCS/DBCS language
//
// Followed by 1 or more histogram sections

typedef struct FileLanguageSection {
	// preceded by struct FileSection
	DWORD	m_dwDetectionType;
	DWORD	m_dwLangID;
	DWORD	m_dwUnicodeRangeID;	// Unicode range mapping value for this lang
	DWORD	m_dwRecordCount;	// # of histograms following this record
} FileLanguageSection;
typedef FileLanguageSection *PFileLanguageSection;

// FileHistogramSection -- entry for one histogram (SBCS/DBCS or WCHAR)

typedef struct FileHistogramSection {
	// preceded by struct FileSection
	union {
		DWORD	m_dwCodePage;	// for 7 or 8-bit, Codepage this indicates
		DWORD	m_dwRangeID;	// for Unicode, the sublanguage group ID
	};
	DWORD	m_dwDimensionality;
	DWORD	m_dwEdgeSize;
	DWORD	m_dwMappingID;		// ID of Charmap to use
	// HElt m_Elts[]
} FileHistogramSection;
typedef struct FileHistogramSection *PFileHistogramSection;

// FileMapSection -- entry for one character map (SBCS/DBCS or WCHAR)

typedef struct FileMapSection {
	// preceded by struct FileSection
	DWORD	m_dwID;				// ID by which hardwired code finds the table
	DWORD	m_dwSize;			// size of table (256 or 65536)
	DWORD	m_dwNUnique;		// # of unique output values
	// HIdx m_map[]
} FileMapSection;
typedef struct FileMapSection *PFileMapSection;

////////////////////////////////////////////////////////////////

// LangNames - lookup table to get from English-localized names to a Win32
// primary language ID.

struct LangNames {
	LPCSTR			pcszName;
	unsigned short	nLangID;
};
LPCSTR GetLangName (int id);
int GetLangID (LPCSTR pcszName);
extern const struct LangNames LangNames[];

////////////////////////////////////////////////////////////////

#endif
