#include "mktyplib.h"

// INTLSTR.C
//   data that should be internationalized
//   this is the only file that should be altered by localization
//

#define BETA

CHAR * szBanner =
"Microsoft (R) Type Library Generator  Version 2.02"
#if defined(BETA) || defined(DEBUG)
#include "verstamp.h"
#define STRING(x) #x
#define VERSTRINGX(rev) STRING(. ## rev)
#define VERSTRING VERSTRINGX(rup)
    VERSTRING
#endif //BETA || DEBUG
#ifdef DEBUG
    " (Debug)"
#endif //DEBUG
"\nCopyright (c) Microsoft Corp. 1993-1995.  All rights reserved."
;

#ifdef MAC
#define  cPREFIX "-"
#define  cALIGN "\t\t "
#define  cALIGN1 "\t\t "
#define  cINDENT "\t\t "
#define  cWRAP " "
#else	//!MAC
#ifdef WIN16
#define  cPREFIX "/"
#define  cALIGN "\t "
#define  cALIGN1 "\t "
#define  cINDENT "\t\t "
#define  cWRAP "\n" cINDENT 
#else //WIN16
#define  cPREFIX "/"
#define  cALIGN "\t "
#define  cALIGN1 " "
#define  cINDENT "\t\t "
#define  cWRAP " "
#endif //WIN16
#endif //!MAC

CHAR * szUsage =
"Usage: MKTYPLIB <options> [inputfile]\n"
"Valid options are:\n"
cPREFIX "help or " cPREFIX "?" cALIGN "Displays usage.\n"
cPREFIX "tlb <filename>" cALIGN "Specifies type library output"
cWRAP   "filename.  Defaults to input name\n"
cINDENT "with extension replaced by \".tlb\".\n"
cPREFIX "h [filename]" cALIGN "Specifies .H file output filename.\n"
cPREFIX "<system>" cALIGN "Specifies kind of type library to"
cWRAP   "make (win16, win32, mac, mips, alpha, ppc or ppc32).\n"
#ifdef WIN16
cINDENT "Defaults to win16.\n"
#else
#ifdef WIN32
#ifdef _MIPS_
cINDENT "Defaults to mips.\n"
//UNDONE: SYS_MAC_PPC?
#else //_MIPS_
#ifdef _ALPHA_
cINDENT "Defaults to alpha.\n"
#else //_ALPHA_
cINDENT "Defaults to win32.\n"
#endif //_ALPHA_
#endif //_MIPS_
#else
cINDENT "Defaults to mac.\n"
#endif
#endif
cPREFIX "align <#>" cALIGN "Override default alignment setting.\n"
cPREFIX "o filename" cALIGN "Redirects output from screen to"
cWRAP   "specified file.\n"
cPREFIX "nologo\t" cALIGN "Don't display the copyright banner.\n"
cPREFIX "w0\t" cALIGN "Disable warnings.\n"
#if FV_CPP
cPREFIX "nocpp\t" cALIGN "Don't spawn the C pre-processor.\n"
cPREFIX "cpp_cmd <path>" cALIGN "Specifies path for C pre-processor.\n"
cINDENT "Defaults to CL.EXE.\n"
cPREFIX "cpp_opt \"<opt>\"" cALIGN1 "Specifies options for C"
cWRAP   "pre-processor.  Defaults to:\n"
cINDENT "\"/C /E /D__MKTYPLIB__\".\n"
cPREFIX "Ddefine[=value]" cALIGN1 "Defines value for C pre-processor.\n"
cPREFIX "I includepath" cALIGN "Specifies path for include files.\n"
#endif	//FV_CPP
;

// for titles of message boxes
CHAR * szAppTitle = "MkTypLib";


// Error message strings.
// WARNING -- must be the same order as items in ERRORS.H !!!
CHAR * rgszErr[] = {
	// Parser/lexer errors.  These have line # and column # information.
	"unterminated comment block",	// PERR_UNTERMINATED_COMMENT
	"unexpected end-of-file",	// PERR_UNEXP_EOF
	"error reading input file",	// PERR_READ_ERROR
	"undefined interface/dispinterface",	 // PERR_UNDEF_INTER
	"unknown type",			// PERR_UNKNOWN_TYPE
	"duplicate definition",		// PERR_DUP_DEF
	"duplicate 'uuid' attribute",	// PERR_DUP_UUID
	"duplicate 'id' attribute in type",  // PERR_DUP_ID
	"attributes inconsistent with this type", // PERR_INV_COMBO
	"missing 'id' attribute", 	// PERR_ID_REQ
	"missing 'uuid' attribute",	// PERR_UUID_REQ
	"missing 'in' and/or 'out' attribute",	 // PERR_IN_OUT_REQ
	"missing 'dllname' attribute",	 // PERR_DLLNAME_REQ
	"missing 'entry' attribute",	 // PERR_ENTRY_REQ
	"missing 'odl' attribute",	// PERR_ODL_REQ
	"'importlib' sections must be first",	// PERR_IMPLIB_NOTFIRST
	"invalid use of 'void'",	// PERR_VOID_INV
	"numeric value out of range",	// PERR_NUMBER_OV
	"invalid attribute combination", // PERR_INV_ATTR_COMBO
	"invalid attribute for this item", // PERR_INV_ATTR
	"invalid numeric literal",	// PERR_INV_NUMBER
	"invalid string literal",	// PERR_INV_STRING
	"invalid UUID literal",		// PERR_INV_UUID
	"invalid identifier",		// PERR_INV_IDENTIFIER
	"invalid constant definition",	// PERR_INV_CONSTANT
	"specified id is out of range",	// PERR_INV_ID
	"invalid use of 'lcid' attribute", // PERR_INV_LCID_USE
	"invalid use of 'retval' attribute", // PERR_INV_RETVAL_USE
	"invalid use of 'vararg' attribute", // PERR_INV_VARARG_USE
	"unsupported keyword",		// PERR_UNSUPP_KEYWORD
	"expected: ",			// PERR_EXPECTED
	"expected: end-of-file",	// PERR_EXP_EOF
	"expected: identifier",		// PERR_EXP_IDENTIFIER
	"expected: keyword",		// PERR_EXP_KEYWORD
	"expected: attribute",		// PERR_EXP_ATTRIBUTE
	"expected: operator",		// PERR_EXP_OPERATOR
	"unsupported operator",		// PERR_UNSUPPORTED_OP
	"invalid numeric expression",	// PERR_INV_EXPRESION
	"expected: numeric expression",	// PERR_EXP_NUMBER
	"expected: string",		// PERR_EXP_STRING
	"expected: interface or dispinterface",	 // PERR_EXP_INTER
	"no more than one dispinterface allowed in a coclass",	 // PERR_TWO_DISPINTER
	"specified calling convention invalid here", // PERR_INV_CALLCONV
	"invalid array declaration",		// PERR_INV_ARRAY_DECL
#ifdef WIN32
	"missing definition of IDispatch. STDOLE32.TLB must be imported.",  // PERR_NO_IDISPATCH
	"missing definition of IUnknown. STDOLE32.TLB must be imported.",  // PERR_NO_IDISPATCH
#else //WIN32
	"missing definition of IDispatch. STDOLE.TLB must be imported.",  // PERR_NO_IDISPATCH
	"missing definition of IUnknown. STDOLE.TLB must be imported.",  // PERR_NO_IDISPATCH
#endif //WIN32
	"attributes must follow 'typedef' keyword",  // PERR_TYPEDEF_ATTR
	"property put function must have at least 1 argument and must have exactly one argument after any LCID argument",  // PERR_INV_PROPPUT
	"return type inconsistent with property type",  // PERR_INV_PROPFUNC
        "unknown LCID",                 // PERR_INV_LCID
        "source attribute only valid on objects and VARIANTs", // PERR_INV_SOURCE_ATTR
        "'out' parameter must be a pointer", // PERR_INV_OUT_PARAM
        "Base interface of Dual interface must be IDispatch, or an interface that derives from IDispatch", // PERR_INV_DUAL_BASE
        "Base interface of OleAutomation interface must be IUnknown, or an interface that derives from IUnknown", // PERR_INV_OA_BASE
        "Type is not OleAutomation-compatible", // PERR_INV_OA_TYPE
        "Invalid return type for OleAutomation-compatible interface", // PERR_INV_OA_FUNC_TYPE

        "references to this type not allowed", // PERR_INV_REFERENCE
	"specified type is not supported by IDispatch::Invoke",  // PWARN_INV_IDISPATCHTYPE

	// output errors (these also have the name of the current item)
	"forward declaration but no definition", // OERR_NO_DEF

	#define	TYPELIBERR(name,string)  string
	#include "typelib.err"		// TYPELIB.DLL error strings
	#undef	TYPELIBERR

	// This shouldn't ever be seen if the list in TYPELIB.ERR is complete.
	"TYPELIB.DLL returned an error",// OERR_TYPEINFO

	// general errors (no line/column # information)
	"out of memory",		// ERR_OM
	"unable to open input file",	// ERR_CANT_OPEN_INPUTFILE
#if FV_CPP
	"unable to pre-process input file",	// ERR_CPP
#endif	//FV_CPP
	"unable to open .H output file",// ERR_CANT_OPEN_HFILE
	"error writing .H output file",	// ERR_WRITING_HFILE

        // general warnings (no line/column info)
        "using non-standard alignment - some structs in the .h file may need to be padded",    // WARN_STRANGE_ALIGNMENT
	""
};

// strings for error display/formattting
CHAR * szFmtSuccess = "Successfully generated type library '%s'.";
CHAR * szFmtErrFileLineCol = "%s (%ld) : fatal error M0001: Syntax error near line %ld column %d:  %s %s";
CHAR * szFmtWarnFileLineCol = "%s (%ld) : warning M0002: Warning near line %ld column %d:  %s %s";
CHAR * szFmtErrOutput = "%s : fatal error M0003: Error creating type library while processing item '%s': %s.";
CHAR * szFmtErrImportlib = "%s : fatal error M0004: Error processing type library '%s': %s.";

// This shouldn't ever be seen if the list in TYPELIB.ERR is complete.
CHAR * szFmtErrUnknown = "%s : fatal error M0005: Error creating type library while processing item '%s': %s (SCODE = 0x%lX).";

CHAR * szFmtErrGeneral = "fatal error M0006: %s";
CHAR * szFmtWarnGeneral = "warning M0007: %s";


// strings for header file output
CHAR * szHeadFile = "/* This header file machine-generated by mktyplib.exe */\n"
		    "/* Interface to type library: ";

CHAR * szHeadModule = "\n/* Functions defined in module: ";

CHAR * szHeadInter = "\n/* Definition of interface: ";
CHAR * szHeadDispinter = "\n/* Definition of dispatch interface: ";
CHAR * szHeadMethods = "/* You must describe methods for this interface here */\n";
CHAR * szHeadDispatchable = "\n/* Capable of dispatching all the methods of interface ";
