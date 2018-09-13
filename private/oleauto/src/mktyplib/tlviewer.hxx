/* -------------------------------------------------------------------------
  Test: 	OSUTIL
 
                Copyright (C) 1991, Microsoft Corporation
 
  Component:	OLE Programmability
 
  Major Area:	Type Information Interface
 
  Sub Area:	ITypeInfo
 
  Test Area:
 
  Keyword:	Win16
 
 ---------------------------------------------------------------------------
  Purpose:	constants for programs that run under Win16
 
  Scenarios:

  Abstract:
 
 ---------------------------------------------------------------------------
  Category:
 
  Product:
 
  Related Files:

  Notes:
 ---------------------------------------------------------------------------
  Revision History:
 
	[ 0]	09-Mar-1993		     Angelach: Created Test
	[ 1]	dd-mmm-yyyy		     changed made, by whom, and why
 ---------------------------------------------------------------------------
*/
 

// routines defined in osutil.cpp

char FAR * osAllocSpaces (WORD) ;
VOID FAR osDeAllocSpaces (char FAR *) ;
GUID FAR * osCreateGuid  (LPSTR) ;
BOOL FAR osRetrieveGuid  (LPSTR, GUID) ;
WORD FAR osGetSize	 (WORD) ;
WORD FAR osGetMemberSize (WORD *, WORD) ;
WORD FAR osGetAlignment  (VOID) ;
int  FAR osStrCmp	 (LPSTR, LPSTR) ;
VOID FAR osStrCpy	 (LPSTR, LPSTR) ;
VOID FAR osStrCat	 (LPSTR, LPSTR) ;

HRESULT FAR osOleInit	 (VOID) ;
VOID	FAR osMessage	 (LPSTR, LPSTR) ;

// external routine

VOID FAR mainEntry	 (LPSTR lpCmd) ;

#define defaultOutput	(LPSTR) "tlviewer.out"
#define cchFilenameMax	256		// max chars in a filename
#define fMaxBuffer	256
#define aMaxName	15
#define MAX_NAMES	65

// name of valid attributes
#define attrAppObj	 (LPSTR) "appobject"
#define attrDefault	 (LPSTR) "default"
#define attrDllName	 (LPSTR) "dllname"
#define attrEntry	 (LPSTR) "entry"
#define attrHelpCont	 (LPSTR) "helpcontext"
#define attrHelpFile	 (LPSTR) "helpfile"
#define attrHelpStr	 (LPSTR) "helpstring"
#define attrId		 (LPSTR) "id"
#define attrIn		 (LPSTR) "in"
#define attrLcid	 (LPSTR) "lcid"
#define attrOption	 (LPSTR) "optional"
#define attrOut 	 (LPSTR) "out"
#define attrPropget	 (LPSTR) "propget"
#define attrPropput	 (LPSTR) "propput"
#define attrProppr	 (LPSTR) "propputref"
#define attrPublic	 (LPSTR) "public"
#define attrRestrict	 (LPSTR) "restricted"
#define attrReadonly	 (LPSTR) "readonly"
#define attrString	 (LPSTR) "string"
#define attrUuid	 (LPSTR) "uuid"
#define attrVar 	 (LPSTR) "vararg"
#define attrVer 	 (LPSTR) "version"

#define noValue 	 0		// attribute has no value
#define numValue	 1		// attribute has numeric value
#define strValue	 2		// attribute has string value


#define fnWrite		"w"		// file attribute - write only

#define szFileHeader	(LPSTR) "/* Textual file generated for type library: "
#define szEndStr	(LPSTR) " */\n\n\n"
#define szInputInvalid	(LPSTR) "     Input file is not a valid type library\n\n"
#define szReadFail	(LPSTR) "     // *** Error in reading the type library: "
#define szBeginAttr	(LPSTR) "["
#define szEndAttr	(LPSTR) "] "
#define szEndAttrNl	(LPSTR) "]\n"
#define szRefDll	(LPSTR) "  // this function referring routine in a dll\n"
#define szImpLib	(LPSTR) "  //  defined in library: "

// global variables
char szDumpInputFile[cchFilenameMax] ;	// name of input library file
char szDumpOutputFile[cchFilenameMax] ;	// name of output text file
char szLibName[cchFilenameMax] ;	// name of the type library

ITypeLib  FAR *ptLib ;			// pointer to the type library
ITypeInfo FAR *ptInfo ;			// pointer to the type definition
TYPEATTR  FAR *lpTypeAttr ;		// pointer to attribute of type def
int	  strFlag = 0 ;			// for string attribute
BOOL	  endAttrFlag = FALSE ;		// if need to end the attribute-list
short 	  cArrFlag = -1 ;		// if it is a c array
BOOL	  isInherit = FALSE ;		// if the interface has a base interface

// prototypes for local functions

VOID  NEAR ProcessInput	(VOID) ;
VOID  NEAR OutToFile	(HRESULT) ;
BOOL  NEAR fOutLibrary	(FILE *) ;
VOID  NEAR tOutEnum	(FILE *, UINT) ;
VOID  NEAR tOutRecord	(FILE *, UINT) ;
VOID  NEAR tOutModule	(FILE *, UINT) ;
VOID  NEAR tOutInterface(FILE *, UINT) ;
VOID  NEAR tOutDispatch	(FILE *, UINT) ;
VOID  NEAR tOutCoclass	(FILE *, UINT) ;
VOID  NEAR tOutAlias	(FILE *, UINT) ;
VOID  NEAR tOutUnion	(FILE *, UINT) ;
VOID  NEAR tOutEncunion (FILE *, UINT) ;
VOID  NEAR tOutName	(FILE *, UINT) ;
VOID  NEAR tOutType	(FILE *, TYPEDESC) ;
VOID  NEAR tOutCDim	(FILE *, TYPEDESC) ;
VOID  NEAR tOutAliasName(FILE *, HREFTYPE) ;
VOID  NEAR tOutValue	(FILE *, BSTR, VARDESC FAR *) ;
VOID  NEAR tOutMember	(FILE *, LONG, BSTR, TYPEDESC) ;
VOID  NEAR tOutVar	(FILE *) ;
VOID  NEAR tOutFuncAttr (FILE *, FUNCDESC FAR *, DWORD, BSTR) ;
VOID  NEAR tOutCallConv (FILE *, FUNCDESC FAR *) ;
VOID  NEAR tOutParams	(FILE *, FUNCDESC FAR *, BSTR) ;
VOID  NEAR tOutFunc	(FILE *) ;
VOID  NEAR tOutUUID	(FILE *, GUID) ;
VOID  NEAR tOutAttr	(FILE *, int) ;
VOID  NEAR tOutMoreAttr (FILE *) ;
VOID  NEAR WriteAttr	(FILE *, LPSTR, LPSTR, int) ;
VOID  NEAR GetVerNumber (WORD w, WORD, char *) ;
VOID  NEAR WriteOut	(FILE *, LPSTR) ;


// prototypes for routines defined in osutil.cpp

char FAR * osAllocSpaces (WORD nSize) ;
VOID FAR osDeAllocSpaces (char FAR *lpsz) ;
BOOL FAR osRetrieveGuid  (LPSTR, GUID) ;
