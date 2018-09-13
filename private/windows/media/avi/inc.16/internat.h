/***********************************************************************
// INTERNAT.H
//
//      Copyright (c) 1992, 1993 - Microsoft Corp.
//		All rights reserved.
//		Microsoft Confidential
//
//	Include file for international specific options.
//
// Johnhe 03-10-92
// TABS = 3
***********************************************************************/

#ifndef INTERNAT_INC
#define INTERNAT_INC 1           // INTERNAT.H signature


#ifdef DBCS
	#define	SkipDBCSLeadByte( x )   {if IsDBCLeadByte( szPtr ) szPtr++;}
#else
	#define	SkipDBCSLeadByte( x )
#endif

//**********************************************************************
// BUGBUG - need to move to resource file
//**********************************************************************

#define		AM_PM_STRINGS		{ "A", "AM", "P", "PM", NULL }
#define		PM_CHAR				'P'
#define		AM_CHAR				'A'
#define     AMPM_TRAIL_CHAR 	'M'

#define		DATE_DELIMITER_STR "-/."	// Date seperators ('-' is replaced)

//**********************************************************************
// Command line delimiters
//**********************************************************************

#define CHAR_COMMA          ','
#define CHAR_EOL            '\0'
#define CHAR_PLUS           '+'
#define CHAR_MINUS          '-'
#define CHAR_SIMICOLON      ';'
#define CHAR_SRCHSTR        '"'
#define CHAR_SWITCH         '/'
#define CHAR_QUOTE          '"'
#define CHAR_SPC            ' '
#define CHAR_TAB            '\t'

#define DOUBLE_SLASH        0x2f2f  	// Double slash chars "//"
#define UNC_WORD            0x5c5c      // UNC specifier ("\\\\")

#define CUR_DIR_STR         "."
#define PARENT_DIR_STR      ".."


#define PATH_CHAR_STR       "\\"
#define PATH_CHAR           '\\'
#define	DRIVE_DELIMITER		':'

#define WILD_NAME_CHAR      '*'     	//  All remaining wildcarc
#define WILD_CHAR           '?'     	// Single wildcard character
#define MULTI_WILDCARDS     1       	// Allow multiple wildcards filenames

#define FILE_EXT_CHAR       '.'     	// File extension seperator
#define CHAR_NOT_ATTRIB     '-'     	// Invert attribute switch

//***********************************************************************
//***********************************************************************

#define GET_EXTINFO         1           // Fill in extended info structure
#define GET_CASEMAP         2           // Get ptr to case map table
#define GET_FNCASEMAP       4           // Get ptr to filename case map table
#define GET_FNAME_CHARS     5           // Get ptr to filename character table
#define GET_COLLATE         6           // Get ptr to collate table
#define GET_DBYTE_SET       7           // Get ptr to double-byte char set

#define	DEFAULT_CODEPAGE	0xffff
#define DEFAULT_COUNTRY 	0xffff

#define USA_CNTRY_ID        1       	// Default country ID
#define USA_CODE_PAGE       437     	// Default country code page
		
#define ASCII_LEN           256         	// # of ASCII characters
#define NON_EXT_LEN         (ASCII_LEN / 2)	// # of non-ext ASCII chars
#define EXT_ASCII_LEN       (ASCII_LEN / 2)	// # of ext ASCII chars
#define COLLATE_TABLE_LEN   ASCII_LEN

//***********************************************************************
//	CntryTable_s is a structure which is filled in by DOS function 65xxh
//***********************************************************************

#ifndef	CntryTable_s
typedef struct CntryTable_s
{
    char                IdByte;
    unsigned char far	*fpAddr;
} CNTRY_TABLE;

#endif

//**********************************************************************
//	COUNTRY_INFO is the structure of the data returned by DOS function
//	0x38.
//**********************************************************************

#ifndef	COUNTRY_DEFINED
struct COUNTRY_INFO
{
    char        ccSetCountryInfo;   // SetCountryInfo
    unsigned    ccCountryInfoLen;   // length of country info
    unsigned    ccDosCountry;       // active country code id
    unsigned    ccDosCodePage;      // active code page id

    unsigned    ccDFormat;          // date format
    char        ccCurSymbol[ 5 ];   // 5 byte of (currency symbol+0)
    char        cc1000Sep[ 2 ];     // 2 byte of (1000 sep. + 0)
    char        ccDecSep[ 2 ];      // 2 byte of (Decimal sep. + 0)
    char        ccDateSep[ 2 ];     // 2 byte of (date sep. + 0)
    char        ccTimeSep[ 2 ];     // 2 byte of (time sep. + 0)
    char        ccCFormat;          // currency format flags
    char        ccCSigDigits;       // # of digits in currency
    char        ccTFormat;          // time format
    char far    *ccMono_Ptr;        // monocase routine entry point
    char        ccListSep[ 2 ];     // data list separator
    unsigned    ccReserved_area[ 5 ];   // reserved
};
#define	COUNTRY_DEFINED	1
#endif

//**********************************************************************
//	Global country specific tables.
//**********************************************************************

extern unsigned char near	CollateTable[ ASCII_LEN ];
extern unsigned char near	CaseMap[ ASCII_LEN ];
extern unsigned char near	FnameCharTable[ ASCII_LEN ];
extern unsigned char near	DBCSLeadByteTable[ ASCII_LEN ];
extern struct COUNTRY_INFO near Cntry;     // DOS country info structure

//**********************************************************************
//	Function prototypes.
//**********************************************************************

extern void     GetCountryInfo  ( struct COUNTRY_INFO *Cntry );
extern int      GetExtCountryInfo( unsigned InfoType, unsigned CodePage,
                                   unsigned CountryCode,
                                   struct COUNTRY_INFO *pTable );
extern void		InitCountryInfo	( void );

#ifdef DBCS
int             IsDBCSLeadByte  ( unsigned char c );
int             CheckDBCSTailByte( unsigned char *str, unsigned char *point );
unsigned char   *DBCSstrupr 	( unsigned char *str );
unsigned char	*DBCSstrchr( unsigned char *str, unsigned char c );
#endif


//***************************************************************************
//
//  End of INTERNAT.H
//
//***************************************************************************

#endif  // INTERNAT_INC
