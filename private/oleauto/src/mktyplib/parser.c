#include	"mktyplib.h"

#include	<stdio.h>
#include	<string.h>
#include	<malloc.h>

#ifndef WIN32
#include	<ole2.h>		// required for dispatch.h
#include        <olenls.h>              // for CompareStringA
#include	"dispatch.h"
#endif // WIN32

#include	"errors.h"
#include	"tokens.h"
#include	"parser.h"
#include	"fileinfo.h"

// external data:
extern FILE * hFileInput;

// global variables:

TYPLIB  typlib;		// main type structure
TOKEN 	tok;		// current token -- filled in by lexer
SHORT	cArgsMax = 0;	// max # of args that any function has
LCID    g_lcidCompare = 0x0409;     // lcid for compare purposes
BOOL    fSpecifiedInterCC = FALSE;  // TRUE if an interface cc was specified

// private types and data
typedef enum {
    FUNC_MODULE = 0,
    FUNC_DISPINTERFACE,
    FUNC_INTERFACE,
    FUNC_OAINTERFACE,
    C_FUNCTYPES
} FUNCTYPE;

static DWORD fValidFuncAttrs[C_FUNCTYPES] = {
	VALID_MODULE_FUNC_ATTR,
	VALID_DISPINTER_FUNC_ATTR,
	VALID_INTERFACE_FUNC_ATTR,
	VALID_INTERFACE_FUNC_ATTR
};
static DWORD fValidFuncAttrs2[C_FUNCTYPES] = {
	VALID_MODULE_FUNC_ATTR2,
	VALID_DISPINTER_FUNC_ATTR2,
	VALID_INTERFACE_FUNC_ATTR2,
	VALID_INTERFACE_FUNC_ATTR2
};
static DWORD fValidParmAttrs[C_FUNCTYPES] = {
	VALID_MODULE_PARM_ATTR,
	VALID_DISPINTER_PARM_ATTR,
	VALID_INTERFACE_PARM_ATTR,
	VALID_INTERFACE_PARM_ATTR
};
static DWORD fValidParmAttrs2[C_FUNCTYPES] = {
	VALID_MODULE_PARM_ATTR2,
	VALID_DISPINTER_PARM_ATTR2,
	VALID_INTERFACE_PARM_ATTR2,
	VALID_INTERFACE_PARM_ATTR2
};

typedef struct {
    CHAR * szName;
    VARTYPE vt;
} INTRINSIC_DEF;

static INTRINSIC_DEF rgIntrinsics[] = {
// NOTE: table lookup assumes that unsigned versions follow signed versions
    "char", VT_I1,
    "char", VT_UI1,
    "int", VT_INT,
    "int", VT_UINT,
    "short", VT_I2,
    "short", VT_UI2,
    "long", VT_I4,
    "long", VT_UI4,
    "boolean", VT_BOOL,
    "double", VT_R8,
    "float", VT_R4,
    "CURRENCY", VT_CY,
    "DATE", VT_DATE,
    "VARIANT", VT_VARIANT,
    "void", VT_VOID,
    "BSTR", VT_BSTR,
    "HRESULT", VT_HRESULT,
    "SCODE", VT_ERROR,
    // LPSTR is handled in InitIntrinsicTypes()
    // wchar_t and LPWSTR are handled in InitIntrinsicTypes()
    NULL, 0			// end of list
};

static LPTYPE  lpType_LPSTR;
static LPTYPE  lpType_DISPATCH;
static LPTYPE  lpType_UNKNOWN;
static LPTYPE  lpType_LPWSTR;
static LPTYPE  lpType_wchar_t;

static LPENTRY lpEntryPrev;
static LPTYPE  lpTypeIDispatch = NULL;
static LPTYPE  lpTypeIUnknown = NULL;

// bit flags returned by GetTypeCompatibility
#define COMPAT_NONE	 0	// any old thing
#define COMPAT_IDISPATCH 1	// IDispatch-compatible types
#define COMPAT_OA	 2	// OA compatible type
#define COMPAT_DUALBASE	 4	// suitable for use as base of a DUAL interface

// bit to mark a type we're checking for OA compatibility as a parameter.
#define VT_PARAM                VT_VECTOR

// external data
extern SYSKIND SysKind;
extern DWORD	f2DefaultCC;

// external routines
extern VOID FAR ParseInit(CHAR * szFile);
extern VOID FAR ConsumeTok(TOKID id, WORD fAccept);
extern VOID FAR ScanTok(WORD fAccept);
extern LPVOID FAR ParseMalloc(WORD cbAlloc);
extern VOID FAR LoadExtTypeLib(LPIMPORTLIB pImpLib);
extern LPTYPE FAR FindExtType(LPSTR szLibName, LPSTR szTypeName);

// prototypes for exported routines
VOID FAR ParseOdlFile(CHAR * szFile);
INT FAR FCmpCaseIns(LPSTR str1, LPSTR str2);
LPTYPE FAR lpTypePublic(LPTYPE lpType);

// prototypes for internal routines
VOID NEAR ParseLibrary(VOID);
VOID NEAR ParseOptAttr(LPATTR pAttr, WORD wContext);
VOID NEAR ParseNewElem(LPELEM FAR * lplpElem,
		       DWORD validElemAttr,
		       DWORD validElemAttr2,
		       LPTYPE lpType);
VOID NEAR ParseElem(LPELEM lpElem, WORD fAllow, LPTYPE lpType);
VOID NEAR ParseStructEnumUnion(LPENTRY lpEntry, TENTRYKIND tentrykind);
VOID NEAR ParseNewEnumElem(LPELEM FAR * lplpElem, long * pEnumVal);
VOID NEAR ParseTypedef(LPENTRY lpEntry);
VOID NEAR ParseAlias(LPENTRY lpEntry);
LPTYPE NEAR ParseKnownType(LPATTR pAttr, WORD fAllow);
VOID NEAR ParseElemName(LPELEM lpElem, WORD fAllow);
VOID NEAR ParseModule(VOID);
VOID NEAR ParseInterface(VOID);
VOID NEAR ParseDispinterface(VOID);
VOID NEAR ParseCoclass(VOID);
VOID NEAR ParseFunction(LPFUNC lpFunc, FUNCTYPE funcType);
VOID NEAR ParseProperty_Set(VOID);
VOID NEAR ParseImportlib(VOID);
VOID NEAR CheckAttr(LPATTR pAttr, DWORD attrbit, DWORD attrbit2);
VOID NEAR CheckAttrTokLast(LPATTR pAttr, DWORD attrbit, DWORD attrbit2);
VOID NEAR GotAttr(LPATTR pAttr, DWORD attrbit);
VOID NEAR GotAttr2(LPATTR pAttr2, DWORD attrbit2);
VOID NEAR ConsumeRCurlyOptSemi(WORD fAccept);
LPSTR NEAR ConsumeId(VOID);
LPSTR NEAR lpszParseStringExpr();
DWORD NEAR lParseNumericExpr(VOID);
DWORD NEAR lParseNumber(VOID);
VOID NEAR InitIntrinsicTypes(VOID);
LPTYPE NEAR InitIntrinsic(CHAR * szName, VARTYPE vt);
LPTYPE NEAR FindTypeArray(LPTYPE lpTypeBase);
LPTYPE NEAR FindTypeInd(LPTYPE lpTypeBase, short cIndirect);
LPTYPE NEAR FindType(LPSTR lpszName, BOOL fUnsigned, TENTRYKIND tentrykind);
VARTYPE NEAR GetStringType(LPTYPE lpTypeBase);
VOID NEAR EnsureNoDupEntries(LPENTRY lpEntryLast);
VOID NEAR EnsureNoDupElem(LPELEM lpElemLast);
BOOL NEAR IsType(LPTYPE lpType, VARTYPE vt);
BOOL NEAR FHandleForwardDecl(LPENTRY lpEntry, TENTRYKIND tentrykind);
VOID NEAR CheckForwardMatch(LPENTRY lpEntryLast, LPENTRY lpEntry);
VOID NEAR InitNewEntry(LPENTRY FAR * lplpEntry);
VOID NEAR EnsureIDispatchType(LPTYPE lpType);
VOID NEAR EnsureOAType(LPTYPE lpType, BOOL fParam);
WORD NEAR GetTypeCompatibility(LPTYPE lpType, WORD fRecurse);
BOOL NEAR IsObjectType(LPTYPE lpType);
BOOL NEAR VerifyLcid(LCID lcid);
VOID NEAR FindIDispatch();

// main parse routine
VOID FAR ParseOdlFile
(
   CHAR * szFile
)
{

    ParseInit(szFile);

    ParseLibrary();		// parse library description
    if (tok.id != RW_EOF)
	ParseError(PERR_EXP_EOF);

    fclose(hFileInput);
    hFileInput = NULL;		// we've done the close
}


// parse a library clause
VOID NEAR ParseLibrary
(
)
{

    ParseOptAttr(&typlib.attr, cLIB);		// parse attributes, if any

    // ensure attributes valid in this context
    CheckAttrTokLast(&typlib.attr, VALID_LIBRARY_ATTR, VALID_LIBRARY_ATTR2);

    ConsumeTok(RW_LIBRARY, 0);	// consume "library", advance to next token

    // UUID required on type library
    if (!(typlib.attr.fAttr & fUUID))
	ParseErrorTokLast(PERR_UUID_REQ);

    typlib.szLibName = ConsumeId(); // get library name

    InitIntrinsicTypes();	// load type list with the intrinisc types

    ConsumeTok(RW_LCURLY, 0);

    // first allow any number of imported libraries
    while (tok.id == RW_IMPORTLIB)
	ParseImportlib();

    // then allow the other sections
    while (tok.id != RW_RCURLY)
	{

	    lpEntryPrev = typlib.pEntry;	// save previous entry

	    InitNewEntry(&typlib.pEntry);	// create & init a new item
						// in the entry list

	    ParseOptAttr(&(typlib.pEntry->attr), cTYPE); // parse attributes, if any

	    switch (tok.id)
	    {
		case RW_TYPEDEF:
		    ParseTypedef(typlib.pEntry);
		    break;

		case RW_MODULE:
		    ParseModule();
		    break;

		case RW_INTERFACE:
		    ParseInterface();
		    break;

		case RW_DISPINTERFACE:
		    ParseDispinterface();
		    break;

		case RW_COCLASS:
		    ParseCoclass();
		    break;

		case RW_IMPORTLIB:
		    ParseError(PERR_IMPLIB_NOTFIRST);

		default:
		    ParseError(PERR_EXP_KEYWORD);
	    } // switch

	} // while library entries

    ConsumeRCurlyOptSemi(fACCEPT_EOF);
}


VOID NEAR InitNewEntry
(
    LPENTRY FAR * lplpEntryLast
)
{
    LPENTRY lpEntry;
    
    // create a new entry in list of entries
    ListInsert(lplpEntryLast, sizeof(ENTRY));

    lpEntry = *lplpEntryLast;		// deref

#ifdef	DEBUG
    // filled in later by the type output code (hopefully before it is used!)
    lpEntry->lpdtinfo = (ICreateTypeInfo *)0xffffffff;
#endif 	//DEBUG

    lpEntry->type.tdesc.vt = VT_USERDEFINED;
    lpEntry->type.szName = "";		// don't want this type entry to be
					// found until we're done filling it in.
    lpEntry->type.lptinfo = NULL;
    lpEntry->lpEntryForward = NULL;	// assume no previous forward decl
					// for this entry

    lpEntry->attr.fAttr = 0;		// no attributes seen yet
    lpEntry->attr.fAttr2 = 0;
}


// parses optional [<attributes>]
// initializes 'Attr' structure with current attributes.
VOID NEAR ParseOptAttr
(
    LPATTR pAttr,
    WORD wContext
)
{
    TOKID  attrId;
    DWORD lTemp;

    pAttr->fAttr = 0;		// no attributes seen yet
    pAttr->fAttr2 = 0;
    if (tok.id == RW_LBRACKET)
	{
	     ScanTok(fACCEPT_ATTR);	 // consume '['

	     // keep going until we encounter a right bracket
	     while ((attrId = tok.id) != RW_RBRACKET)
		{
		    ScanTok(fACCEPT_ATTR);	// advance to next token
		    switch (attrId)
		    {

			case ATTR_UUID:		// uuid(uuid constant)
			    GotAttr(pAttr, fUUID);
			    ConsumeTok(RW_LPAREN, fACCEPT_UUID);
			    if (tok.id != LIT_UUID)
				ParseError(PERR_INV_UUID);
			    pAttr->lpUuid = tok.lpUuid;
			    ScanTok(0);		// consume UUID
			    ConsumeTok(RW_RPAREN, 0);
			    break;

			case ATTR_LCID:		// lcid(I4 lcid)
			    GotAttr(pAttr, fLCID);
			    // LCID attribute allowed on a param, too
			    if (wContext == cPARAM)
				break;
			    ConsumeTok(RW_LPAREN, fACCEPT_NUMBER);
			    pAttr->lLcid = lParseNumericExpr();
                            // don't change the global if the given lcid==0
			    if (pAttr->lLcid) {
                                // make sure this is a supported lcid
                                if (!VerifyLcid(pAttr->lLcid))
                                    ParseError(PERR_INV_LCID);
                                g_lcidCompare = pAttr->lLcid;
		            }
			    ConsumeTok(RW_RPAREN, 0);
			    break;

			case ATTR_VERSION:	// version(I2 major.I2 minor)
			    GotAttr(pAttr, fVERSION);
			    ConsumeTok(RW_LPAREN, fACCEPT_NUMBER);

			    lTemp = lParseNumericExpr();
			    if (HIWORD(lTemp))
				ParseError(PERR_NUMBER_OV);
			    pAttr->wVerMajor = LOWORD(lTemp);

			    ConsumeTok(RW_PERIOD, fACCEPT_NUMBER);

			    lTemp = lParseNumericExpr();
			    if (HIWORD(lTemp))
				ParseError(PERR_NUMBER_OV);
			    pAttr->wVerMinor = LOWORD(lTemp);

			    ConsumeTok(RW_RPAREN, 0);
			    break;

			case ATTR_ENTRY:	// entry(proc name/I2 ordinal)
			    ConsumeTok(RW_LPAREN, fACCEPT_NUMBER | fACCEPT_STRING);
			    GotAttr(pAttr, fENTRY);
			    if (tok.id == LIT_STRING)
				{
			    	    pAttr->lpszProcName = tok.lpsz;
				    ScanTok(0);		// consume entry string
				}
			    else
				{   // set high word of lpszProcName to 0,
				    // to indicate call-by-ordinal
				    lTemp = lParseNumericExpr();
				    if (HIWORD(lTemp))
					ParseError(PERR_NUMBER_OV);
			    	    pAttr->lpszProcName = (LPSTR)lTemp;
				}

			    ConsumeTok(RW_RPAREN, 0);
			    break;

			case ATTR_ID:		// id(I4 id num)
			    GotAttr(pAttr, fID);
			    ConsumeTok(RW_LPAREN, fACCEPT_NUMBER);
			    pAttr->lId = lParseNumericExpr();

			    // We reserve the top bit for ourselves --
			    // don't let the user pick these id's except for
			    // a few special values like DISPID_NEWENUM (-4)
			    // and DISPID_EVALUATE (-5)
			    // Also allow the special range -999 to -500 for
			    // the controls folks.  This range will be
			    // documented as reserved in the next version.
			    // Also allow the special range 0x80010000 to
			    // 0x8001FFFF for Control containers such as VB4.

#ifndef DISPID_COLLECT	// temporary until everybody upgrades header files
#define DISPID_COLLECT (-8)
#endif
			    //   high bit == 1 ==> special negative value
			    //   next bit == 1 ==> typelib.dll picked this id
			    // 		  == 0 ==> user picked this id
			    if (pAttr->lId & 0x80000000) {
			      switch ((long)pAttr->lId) {
			      case DISPID_NEWENUM:
			      case DISPID_EVALUATE:
			      case DISPID_CONSTRUCTOR:
			      case DISPID_DESTRUCTOR:
			      case DISPID_COLLECT:
				break;
			      default:
			        if ((long)pAttr->lId >= -999 &&
				     (long)pAttr->lId <= -500) {
				  break;
				}
			        if (HIWORD(pAttr->lId) == 0x8001)
				  break;
				ParseErrorTokLast(PERR_INV_ID);
			      }
			    }

			    ConsumeTok(RW_RPAREN, 0);
			    break;

			case ATTR_HELPCONTEXT:	// helpcontext(I4 context num)
			    GotAttr(pAttr, fHELPCONTEXT);
			    ConsumeTok(RW_LPAREN, fACCEPT_NUMBER);
			    pAttr->lHelpContext = lParseNumericExpr();
			    ConsumeTok(RW_RPAREN, 0);
			    break;

			case ATTR_DLLNAME:	// dllname("dllname string")
			    GotAttr(pAttr, fDLLNAME);
			    ConsumeTok(RW_LPAREN, fACCEPT_STRING);
			    pAttr->lpszDllName = lpszParseStringExpr();
			    ConsumeTok(RW_RPAREN, 0);
			    break;

			case ATTR_HELPFILE:	// helpstring("helpfile")
			    GotAttr(pAttr, fHELPFILE);
			    ConsumeTok(RW_LPAREN, fACCEPT_STRING);
			    pAttr->lpszHelpFile = lpszParseStringExpr();
			    ConsumeTok(RW_RPAREN, 0);
			    break;

			case ATTR_HELPSTRING:	// helpstring("help string")
			    GotAttr(pAttr, fHELPSTRING);
			    ConsumeTok(RW_LPAREN, fACCEPT_STRING);
			    pAttr->lpszHelpString = lpszParseStringExpr();
			    ConsumeTok(RW_RPAREN, 0);
			    break;

			case ATTR_IN:
			    GotAttr(pAttr, fIN);
			    break;

			case ATTR_OUT:
			    GotAttr(pAttr, fOUT);
			    break;

			case ATTR_ODL:
			    GotAttr(pAttr, fODL);
			    break;

			case ATTR_OPTIONAL:
			    GotAttr(pAttr, fOPTIONAL);
			    break;

			case ATTR_PUBLIC:
			    GotAttr(pAttr, fPUBLIC);
			    break;

			case ATTR_READONLY:
			    GotAttr(pAttr, fREADONLY);
			    break;

			case ATTR_STRING:
			    GotAttr(pAttr, fSTRING);
			    break;

			case ATTR_VARARG:
			    GotAttr(pAttr, fVARARG);
			    break;

			case ATTR_APPOBJECT:
			    GotAttr(pAttr, fAPPOBJECT);
			    break;

			case ATTR_PROPGET:
			    if (pAttr->fAttr & (fPROPPUT | fPROPPUTREF))
				ParseErrorTokLast(PERR_INV_ATTR_COMBO);
			    GotAttr(pAttr, fPROPGET);
			    break;

			case ATTR_PROPPUT:
			    if (pAttr->fAttr & (fPROPGET | fPROPPUTREF))
				ParseErrorTokLast(PERR_INV_ATTR_COMBO);
			    GotAttr(pAttr, fPROPPUT);
			    break;

			case ATTR_PROPPUTREF:
			    if (pAttr->fAttr & (fPROPGET | fPROPPUT))
				ParseErrorTokLast(PERR_INV_ATTR_COMBO);
			    GotAttr(pAttr, fPROPPUTREF);
			    break;

			case ATTR_RESTRICTED:
			    GotAttr(pAttr, fRESTRICTED);
			    break;

			case ATTR_DEFAULT:
			    GotAttr(pAttr, fDEFAULT);
			    break;

			case ATTR_SOURCE:
			    GotAttr(pAttr, fSOURCE);
			    break;

			case ATTR_BINDABLE:
			    GotAttr(pAttr, fBINDABLE);
			    break;

			case ATTR_REQUESTEDIT:
			    GotAttr(pAttr, fREQUESTEDIT);
			    break;

			case ATTR_DISPLAYBIND:
			    GotAttr(pAttr, fDISPLAYBIND);
			    break;

			case ATTR_DEFAULTBIND:
			    GotAttr(pAttr, fDEFAULTBIND);
			    break;

			case ATTR_LICENSED:
			    GotAttr(pAttr, fLICENSED);
			    break;

			case ATTR_PREDECLID:
			    GotAttr(pAttr, fPREDECLID);
			    break;

			case ATTR_HIDDEN:
			    GotAttr(pAttr, fHIDDEN);
			    break;

			case ATTR_RETVAL:
			    GotAttr(pAttr, fRETVAL);
			    break;

			case ATTR_CONTROL:
			    GotAttr2(pAttr, f2CONTROL);
			    break;

			case ATTR_DUAL:
			    GotAttr2(pAttr, f2DUAL);
			    break;

			case ATTR_NONEXTENSIBLE:
			    GotAttr2(pAttr, f2NONEXTENSIBLE);
			    break;

			case ATTR_OLEAUTOMATION:
			    GotAttr2(pAttr, f2OLEAUTOMATION);
			    break;

			case RW_COMMA:		// must accept blank attributes
			    continue;

			default:
			    // expected attribute or ']'
			    ParseErrorTokLast(PERR_EXP_ATTRIBUTE);
		    }

		    if (tok.id == RW_RBRACKET)
			break;			// all done
		    // eat comma which must separate args
		    ConsumeTok(RW_COMMA, fACCEPT_ATTR);

		} // while

	    ScanTok(0);		// consume ']'

	    // context-insensitive attribute validations can be done here.

	    // 'bindable' must also be set if any other data binding attr given
	    if ((pAttr->fAttr &
		(fREQUESTEDIT | fDISPLAYBIND | fDEFAULTBIND))
	       && !(pAttr->fAttr & fBINDABLE))
		ParseErrorTokLast(PERR_INV_ATTR_COMBO);

	} // if
}

// ******************************************************************
// TYPEDEF-related routines
// ******************************************************************

VOID NEAR InitIntrinsicTypes()
{
    INTRINSIC_DEF * pIntrinsic;

    // load all the intrinsic types into the type structure.
    for (pIntrinsic = rgIntrinsics; pIntrinsic->szName; pIntrinsic++)
       {
	    InitIntrinsic(pIntrinsic->szName, pIntrinsic->vt);
       };

    lpType_LPSTR = InitIntrinsic("LPSTR", VT_LPSTR);
    lpType_DISPATCH = InitIntrinsic("IDispatch *", VT_DISPATCH);
    lpType_UNKNOWN = InitIntrinsic("IUnknown *", VT_UNKNOWN);
    lpType_LPWSTR = InitIntrinsic("LPWSTR", VT_LPWSTR);
    // UNDONE: what VT_xxx constant to use for 'wchar_t'?
    lpType_wchar_t = InitIntrinsic("wchar_t", VT_I2);
}

LPTYPE NEAR InitIntrinsic
(
    CHAR * szName,
    VARTYPE vt
)
{
    // NOTE: only need to allocate space for a TYPE item
    ListInsert(&typlib.pEntry, sizeof(TYPE));
    typlib.pEntry->type.szName = szName;
    typlib.pEntry->type.tdesc.vt = vt;
    typlib.pEntry->type.tentrykind = tINTRINSIC;
    typlib.pEntry->type.lptinfo = NULL;
    typlib.pEntry->type.intr.fUnsigned =
		(vt == VT_UI1 || vt == VT_UINT ||
	    	 vt == VT_UI2 || vt == VT_UI4);

    return (LPTYPE)typlib.pEntry;
}


// Start at front of type list, and find base type of the given name
// (with 0 levels of indirection).   Will also find forward declarations.
LPTYPE NEAR FindType
(
    LPSTR  lpszName,		// name to look for
    BOOL   fUnsigned,		// TRUE if unsigned keyword preceeded this
    TENTRYKIND tentrykind	// kind of thing to look for
)
{
    LPTYPE lpTypeLast = (LPTYPE)ListLast(typlib.pEntry);
    LPTYPE lpType = (LPTYPE)ListFirst(typlib.pEntry);

#pragma warning(disable:4127)
    while (TRUE)
#pragma warning(default:4127)
    {
	if (tentrykind == tSTRUCT || tentrykind == tUNION)
	    {	// if 'struct'/'union' keyword specified, then match tags
	    	if ((lpType->tentrykind & ~tFORWARD) == tentrykind
		    && lpType->structenum.szTag
		    && !FCmpCaseIns(lpType->structenum.szTag, lpszName))
                        return lpType;		// got a match
	    }
	else if (lpType->tentrykind != tREF && !(lpType->tentrykind & tQUAL)) 
            {	// if type definition or intrinsic, check names
                if (!FCmpCaseIns(lpType->szName, lpszName))
		    // if names match, check that signed/unsigned matches
		    if (fUnsigned == (lpType->tentrykind == tINTRINSIC && lpType->intr.fUnsigned))
                        return lpType;		// got a match
            }
        if (lpType == lpTypeLast)	// if end of list
            return NULL;		// return not found
        lpType = lpType->pNext;		// advance to next type
    }

}


// find existing VT_PTR type entry (with the same base type) with same # of
// levels of indirection, or create new entry with given indirection level.
LPTYPE NEAR FindTypeInd
(
    LPTYPE lpTypeBase,		// * to base type
    short  cIndirect		// indirection level to look for
)
{
    LPTYPE lpTypeLast = (LPTYPE)ListLast(typlib.pEntry);
    LPTYPE lpType;
    short  cIndLast;
    TYPEDESC FAR * lptdescPrev;

    lpType = lpTypeBase;	// start looking at base type
    cIndLast = 0;		// highest indirection level seen so far
    // point to real tdesc, ignoring non-public typedef's
    lptdescPrev = &(lpTypePublic(lpType)->tdesc);

#pragma warning(disable:4127)
    while (TRUE)
#pragma warning(default:4127)
    {
        // check for a reference type with the same base type and same
	// level of indirection
        if (lpType->tdesc.vt == VT_PTR && lpType->ref.ptypeBase == lpTypeBase)
	    {	// if base types match, check indirection levels
                if (lpType->ref.cIndirect == cIndirect)
                    // if indirection levels match, we're done
		    return lpType;
		Assert (lpType->ref.cIndirect > cIndLast);
	        cIndLast = lpType->ref.cIndirect;	// save highest
							// indirection level
		// can't have a VT_PTR entry that isn't public.
		Assert(lpTypePublic(lpType) == lpType);
		lptdescPrev = &(lpType->tdesc);
	    }

        if (lpType == lpTypeLast)	// if end of list
	    break;			//   done -- no match found

        lpType = lpType->pNext;		// advance to next type
    }

    // no entry with same base type & # of levels of indirection, make new
    // entries up to and including the current level of indirection.
    for (cIndLast++; cIndLast <= cIndirect; cIndLast++)
        {
	    // allocate new type list item at end of list
	    // NOTE: only need to allocate space for a TYPE item
    	    ListInsert(&typlib.pEntry, sizeof(TYPE));

	    // set up info
	    typlib.pEntry->type.tdesc.vt = VT_PTR;
	    typlib.pEntry->type.tdesc.lptdesc = lptdescPrev;
	    lptdescPrev = &(typlib.pEntry->type.tdesc);	// ready for next time
	    typlib.pEntry->type.szName = lpTypeBase->szName;
	    typlib.pEntry->type.tentrykind = tREF;
	    typlib.pEntry->type.lptinfo = NULL;
	//CONSIDER: maybe eliminate ptypeBase now that we have the tdesc field.
            typlib.pEntry->type.ref.ptypeBase = lpTypeBase;
	    typlib.pEntry->type.ref.cIndirect = cIndLast;
        }

    return &typlib.pEntry->type;
}

// find existing VT_SAFEARRAY type entry (with the same base type),
// create new entry with this base type.
LPTYPE NEAR FindTypeArray
(
    LPTYPE lpTypeBase		// * to base type
)
{
    LPTYPE lpTypeLast = (LPTYPE)ListLast(typlib.pEntry);
    LPTYPE lpType;

    lpType = lpTypeBase;	// start looking at base type

#pragma warning(disable:4127)
    while (TRUE)
#pragma warning(default:4127)
    {
        // check for a reference type with the same base type
        if (lpType->tdesc.vt == VT_SAFEARRAY && lpType->ref.ptypeBase == lpTypeBase)
	    return lpType;

        if (lpType == lpTypeLast)	// if end of list
	    break;			//   done -- no match found

        lpType = lpType->pNext;		// advance to next type
    }

    // no entry with same base type, make new entry.

    // allocate new type list item at end of list
    // NOTE: only need to allocate space for a TYPE item
    ListInsert(&typlib.pEntry, sizeof(TYPE));

    // now create new array reference type
    typlib.pEntry->type.tdesc.vt = VT_SAFEARRAY;

    // link to real tdesc, ignoring non-public typedef's
    typlib.pEntry->type.tdesc.lptdesc = &(lpTypePublic(lpTypeBase)->tdesc);

    typlib.pEntry->type.szName = lpTypeBase->szName;
    typlib.pEntry->type.tentrykind = tREF;
    typlib.pEntry->type.lptinfo = NULL;
//CONSIDER: maybe eliminate ptypeBase now that we have the tdesc field.
    typlib.pEntry->type.ref.ptypeBase = lpTypeBase;

    return &typlib.pEntry->type;
}


//
// parse a type reference of the form:
// [unsigned/struct] <name> {[far] *}
//    OR
// SAFEARRAY(<type reference>) {[far] *}
//
// Also ensures that current attributes are valid in this context
//
LPTYPE NEAR ParseKnownType
(
    LPATTR pAttr,
    WORD   fAllow
)
{
    short  cIndirect = 0;
    LPSTR  szTypeName;
    LPSTR  szLibName = NULL;
    LPTYPE lpType;
    LPTYPE lpTypeBase = NULL;
    BOOL   fUnsigned = FALSE;
    TENTRYKIND tMatchTag = tANY;
    BOOL   fFar;

    if ((fAllow & fAllowSAFEARRAY) && tok.id == RW_SAFEARRAY)
	{
	    ScanTok(0);		// consume "SAFEARRAY"
	    ConsumeTok(RW_LPAREN, 0);
	    lpTypeBase = ParseKnownType(pAttr, fAllow);

	    if (IsType(lpTypeBase, VT_VOID))	// 'void' is no good here
		ParseErrorTokLast(PERR_VOID_INV);

	    ConsumeTok(RW_RPAREN, 0);

	    // Now find an existing type entry that's a safearry with this
	    // base type, or create a new one if not found.
	    lpTypeBase = FindTypeArray(lpTypeBase);
	    Assert (lpTypeBase != NULL);
	}
    else
	{	// not a safearray

	    switch (tok.id)
		{
		    // type of the form: unsigned [int, char, long, short]
		    case RW_UNSIGNED:
		    	fUnsigned = TRUE;
			goto consumeit;

		    // type of the form: struct <structtag>
		    case RW_STRUCT:
		        tMatchTag = tSTRUCT;
			goto consumeit;

		    // type of the form: union <uniontag>
		    case RW_UNION:
		        tMatchTag = tUNION;
consumeit:
			ScanTok(0);	// consume "unsigned"/"struct"/"union"

		    default:
			;
		}

	    // get/consume typename/libraryname/tagname
	    szTypeName = ConsumeId();

	    if (!fUnsigned && tMatchTag == tANY && tok.id == RW_PERIOD)
		{   // if library.typename syntax
		    ScanTok(0);		// consume "."
		    szLibName = szTypeName;	// first id is really a lib name
		    szTypeName = ConsumeId();	// get/consume type name
		}
	}

    // handle indirection (as many "[far] *" as are present)
    while ((fFar = (tok.id == RW_FAR)) || tok.id == RW_POINTER)
        {
            cIndirect++;	// one more level of indirection

	    ScanTok(0);		// consume "*" or "far"

	    if (fFar)		// if "far" specified, then "*" must follow
		ConsumeTok(RW_POINTER, 0);
		   
	    // CONSIDER: impose a limit on # of levels of indirection???
	}

    if (lpTypeBase == NULL)	// if not a SAFEARRAY
	{
	    // CONSIDER: (size) tQUAL entries won't get re-used
	    if (szLibName)
	       	{   // reference to type in a specific external type library
		    Assert(fUnsigned == FALSE);
		    Assert(tMatchTag == tANY);
		    lpTypeBase = FindExtType(szLibName, szTypeName);
		    _ffree(szLibName);		// done with library name
		}
	    else if (tMatchTag != tANY)
  		{   // we're to match tag instead of name
		    Assert(szLibName == NULL);
		    Assert(fUnsigned == FALSE);
		    lpTypeBase = FindType(szTypeName, fUnsigned, tMatchTag);
		}
	    else
		{
		    // First perform any some special mappings before looking
		    // through all the existing types.
		    if (cIndirect)
			{ 
			    // map IDispatch * to VT_DISPATCH
			    if (!FCmpCaseIns(szTypeName, "IDispatch"))
				{
				    lpTypeBase = lpType_DISPATCH;
				    cIndirect--;	// ignore the last *
				    goto FoundBaseType;
				}
			    // map IUnknown * to VT_UNKNOWN
			    if (!FCmpCaseIns(szTypeName, "IUnknown"))
				{
				    lpTypeBase = lpType_UNKNOWN;
				    cIndirect--;	// ignore the last *
				    goto FoundBaseType;
				}
			}
		    // find existing type of this name with 0 levels of
		    // indirection
		    lpTypeBase = FindType(szTypeName, fUnsigned, tANY);
	
		    // if not found here, then search all external type
		    // libraries for this type definition.
		    if (lpTypeBase == NULL && !fUnsigned)
		        lpTypeBase = FindExtType(NULL, szTypeName);
		}

	    if (lpTypeBase == NULL)		// error if still not found
		ParseErrorTokLast(PERR_UNKNOWN_TYPE);

FoundBaseType:
	    _ffree(szTypeName);			// done with type name
	}

    lpType = lpTypeBase;
    // find existing type entry with same # of levels of indirection
    // or create new entry with given indirection level.
    if (cIndirect != 0)
        lpType = FindTypeInd(lpTypeBase, cIndirect);

    Assert (lpType != NULL);

    // If ensure the base type of this item makes sense in this context.
    // Typelib.dll isn't very good about catching all the errors.
    while (lpTypeBase->tentrykind == tTYPEDEF)
	lpTypeBase = lpTypeBase->td.ptypeAlias;

    switch(lpTypeBase->tentrykind & ~(tFORWARD | tIMPORTED | tQUAL)) {
      case tINTERFACE:
      case tDISPINTER:
      case tCOCLASS:
	if (cIndirect == 0 && (fAllow & fAllowInter) == 0) {
	  // interface with no indirection -- give error
	  ParseErrorTokLast(PERR_INV_REFERENCE);
	}
	break;

      case tMODULE:
	if ((fAllow & fAllowMODULE) == 0) {
	  // references to module -- no good in most places
	  ParseErrorTokLast(PERR_INV_REFERENCE);
	}
	break;

      default:
	break;			// ok
    }

    // NOTE: for MIDL compatiblity, we accept the 'string' attribute on
    // types that mean 'char *'.   We translate those type references to
    // the intrinisc type 'LPSTR' (even in the header file).
    // We first must validate that if 'string' is specified, then the type of
    // the data is really a 'char *'.   For types defined in external type
    // libraries, we just accept whatever string attribute the user specified,
    // since we can't easily check to see if it is valid.   The entry in the
    // imported type library should already be a VT_LPSTR or a VT_LPWSTR in
    // that case anyway.

    if (pAttr->fAttr & fSTRING) {

        // try to figure out if it's really a string (char * or wchar_t *)
        lpTypeBase = lpType;
	while (lpTypeBase->tentrykind == tTYPEDEF)
	     lpTypeBase = lpTypeBase->td.ptypeAlias;

	if (!(lpTypeBase->tentrykind & tIMPORTED)
	    && lpTypeBase->tdesc.vt != VT_LPSTR
	    && lpTypeBase->tdesc.vt != VT_LPWSTR
		)
	    {	// if not already a string, or an imported type -- it had
		// better equate to "char *"
		switch(GetStringType(lpTypeBase)) {
		    case VT_LPSTR:
		        return lpType_LPSTR;
		    case VT_LPWSTR:
		        return lpType_LPWSTR;
		    default:
		        // 'string' attr set but not char * or wchar_t *
		        ParseErrorTokLast(PERR_INV_COMBO);
		}
	    }
    }

    return lpType;
}


/* returns the appropriate string type that this is, or VT_EMPTY if not a
   string */
VARTYPE NEAR GetStringType(LPTYPE lpTypeBase)
{
    if (lpTypeBase->tdesc.vt == VT_PTR && lpTypeBase->ref.cIndirect == 1) {
	lpTypeBase = lpTypeBase->ref.ptypeBase;
	while (lpTypeBase->tentrykind == tTYPEDEF)
	    lpTypeBase = lpTypeBase->td.ptypeAlias;

	if (lpTypeBase->tentrykind == tINTRINSIC)
	    switch (lpTypeBase->tdesc.vt)
		{
// UNDONE: does MIDL have wchar_t???
		    // UNDONE; what VT_xxx to use here?  If
		    // separate, then we can rip the 'if' stmt.
		    // and lpType_wchar_t
		    case VT_I2:		// wchar_t
			if (lpTypeBase != lpType_wchar_t)
			    break;
			return VT_LPWSTR;
		    case VT_I1:
			return VT_LPSTR;

		    default:
			break;
		}
    }
    return VT_EMPTY;	// not a string
}

// parse an element name (property, struct elem, function parm, etc, handling
// array references)
VOID NEAR ParseElemName
(
    LPELEM lpElem,
    WORD fAllow
)
{
    ARRAYDESC FAR * lpAD;
    WORD  cDims;
    DWORD cElems;

    lpElem->szElemName = ConsumeId();	// get/consume/store elem name

    if ((fAllow & fAllowCARRAY) && tok.id == RW_LBRACKET)
	{

#define MAX_DIMS 64		// arbitrary max # of dimensions

	    // allocate & load the array descriptor
	    lpAD = (ARRAYDESC FAR *)ParseMalloc(sizeof(ARRAYDESC) + ((MAX_DIMS-1) * sizeof(SAFEARRAYBOUND)));
	    cDims = 0;
	    while (tok.id == RW_LBRACKET)
		{
		    if (cDims >= MAX_DIMS)
			ParseError(PERR_INV_ARRAY_DECL);

		    ScanTok(fACCEPT_NUMBER);		// consume "["

	   	    lpAD->rgbounds[cDims].lLbound = 0;	// lbound always 0
#if 0	// arrays of the form "a[]" aren't supported
		    if (cDims == 0 && tok.id == RW_RBRACKET)
			{	// array of the form "x[] or x[][2]"
			    // UN_DONE: verify that this is right
	    		    lpAD->rgbounds[cDims].cElements = 0;
			}
		    else
#endif
			{
	    		    cElems = lParseNumericExpr();
			    if (cElems == 0)
				ParseError(PERR_INV_ARRAY_DECL);
	    		    lpAD->rgbounds[cDims].cElements = cElems;
			}
		    cDims++;			// one more dimension
		    ConsumeTok(RW_RBRACKET, 0);		// consume "]"
		}
	    lpAD->cDims = cDims;

	    // get real tdesc, ignoring non-public typedef's
	    lpAD->tdescElem = lpTypePublic(lpElem->elemType)->tdesc;

	    // allocate new type list item at end of list
	    // NOTE: only need to allocate space for a TYPE item
	    ListInsert(&typlib.pEntry, sizeof(TYPE));

	    // now create new array reference type
	    typlib.pEntry->type.tdesc.vt = VT_CARRAY;
	    typlib.pEntry->type.tdesc.lpadesc = lpAD;
	    typlib.pEntry->type.szName = lpElem->elemType->szName;
	    typlib.pEntry->type.tentrykind = tREF;
	    typlib.pEntry->type.lptinfo = NULL;
	    //CONSIDER: maybe eliminate ptypeBase now that we have tdesc.
	    typlib.pEntry->type.ref.ptypeBase = lpElem->elemType;

	    // change type to point to this new type
	    lpElem->elemType = (LPTYPE)typlib.pEntry;
	}

}


// ensure last element in element list isn't duplicated
// or is in some way inconsistent with other elements
VOID NEAR EnsureNoDupElem
(
    LPELEM lpElemLast
)
{
    LPELEM lpElem;
    LPSTR szElemLast = lpElemLast->szElemName;
    BOOL  fIdLast = ((lpElemLast->attr.fAttr & fID) != 0);
    DWORD idElemLast = lpElemLast->attr.lId;
    DWORD propBitsLast = (lpElemLast->attr.fAttr & fPropBits);
    DWORD propFuncBitsLast = (lpElemLast->attr.fAttr & (fPropFuncBits | fRESTRICTED));
    DWORD propBitsCur;
    BOOL  fDefaultBindLast = ((lpElemLast->attr.fAttr & fDEFAULTBIND) != 0);

    for (lpElem = (LPELEM)ListFirst(lpElemLast); lpElem != lpElemLast; lpElem = lpElem->pNext)
        {
	    // ensure names not duplicated in this element list
            if (!FCmpCaseIns(szElemLast, lpElem->szElemName))
		{   // names match
		    // if names match, reject this if they have the same
		    // propget/put/putref bits (or one of them doesn't have
		    // a property bit set).
		    propBitsCur = (lpElem->attr.fAttr & fPropBits);
		    if (!propBitsLast || !propBitsCur || propBitsLast == propBitsCur)
		    	ParseErrorTokLast(PERR_DUP_DEF);

		    // better both have the same id, if specified
		    if ( (fIdLast != ((lpElem->attr.fAttr & fID) != 0)) ||
		         (fIdLast && idElemLast != lpElem->attr.lId) )
			ParseErrorTokLast(PERR_DUP_DEF);

		    // better have the same attributes
		    if ((lpElem->attr.fAttr & (fPropFuncBits | fRESTRICTED)) != propFuncBitsLast) {
			ParseErrorTokLast(PERR_DUP_DEF);
		    }
		}
	   else	  
		{   // names don't match  
		    // better not both have the same id
		    if (fIdLast && (lpElem->attr.fAttr & fID))
			if (idElemLast == lpElem->attr.lId)
			    ParseErrorTokLast(PERR_DUP_ID);

		    // better not both be marked as 'defaultbind'
		    if (fDefaultBindLast && (lpElem->attr.fAttr & fDEFAULTBIND))
			ParseErrorTokLast(PERR_DUP_DEF);
		}
        }
}

// add and parse a new element to a list of elements.
// if lpType is non-null, disallows elements of this type.
// Assumes that the element can handle Array types.
VOID NEAR ParseNewElem
(
    LPELEM FAR * lplpElem,
    DWORD  validElemAttr,
    DWORD  validElemAttr2,
    LPTYPE  lpType
)
{
    LPELEM lpElem;

    ListInsert(lplpElem, sizeof(ELEM));	// allocate new list item
    lpElem = *lplpElem;

    ParseOptAttr(&lpElem->attr, cVAR); 	// parse attributes, if any

    CheckAttr(&lpElem->attr, validElemAttr, validElemAttr2);

    ParseElem(lpElem, fAllowArray, lpType);
}


VOID NEAR ParseElem
(
    LPELEM lpElem,
    WORD   fAllow,
    LPTYPE  lpType
)
{
    // parse element type
    lpElem->elemType = ParseKnownType(&lpElem->attr, fAllow);
    Assert(lpElem->elemType);

    if (IsType(lpElem->elemType, VT_VOID))	// 'void' is no good here
	ParseErrorTokLast(PERR_VOID_INV);

    // disallow self-referencing types (not including pointers to those types)
    // CONSIDER: This check isn't sufficient now that we have forward declares:
    // CONSIDER: typedef struct foo;
    // CONSIDER: typedef struct foo {
    // CONSIDER:   struct foo bar;		// makes it by this check
    // CONSIDER: } str;
    // CONSIDER: The error DOES get caught during LayOut(), but it would be
    // CONSIDER: nicer to catch it sooner if we could.  The correct check is
    // CONSIDER: is to disallow any non-pointer to a forward declare for which
    // CONSIDER: there is no real definition.
    if (lpElem->elemType == lpType)
	ParseErrorTokLast(PERR_UNKNOWN_TYPE);

    // parse element name, allowing arrays where appropriate
    ParseElemName(lpElem, fAllow);

    EnsureNoDupElem(lpElem);			// ensure no duplicate elements now

}



// given a type, returns a pointer to the underlying public type.
// This is used to ignore non-public Typedef's where desired.
// Any typedef with an attribute is considered to be public.
LPTYPE FAR lpTypePublic
(
    LPTYPE lpType
)
{
    while (lpType->tentrykind == tTYPEDEF
	   && (((LPENTRY)lpType)->attr.fAttr) == 0
	   && (((LPENTRY)lpType)->attr.fAttr2) == 0)
	lpType = lpType->td.ptypeAlias;

    return lpType;
}


// Handle forward declarations.  Ensures that there isn't already a "real"
// definition before this definiton.  Then updates the name/tag in the type
// structure, and sees if this is a forward definition (<name> followed by ';')
// returns TRUE if this is a forward declaration, FALSE otherwise.
BOOL NEAR FHandleForwardDecl
(
    LPENTRY    lpEntry,
    TENTRYKIND tentrykind
)
{
    LPSTR   lpszName;
    LPENTRY pEntryForward = NULL;

    lpEntry->type.tentrykind = tentrykind; // store information in record
    lpszName = ConsumeId();		   // get/consume interface/tag name

    Assert (lpEntry->lpEntryForward == NULL);	// caller should have zero'ed

    // find a previous forward declaration, if any
    if ((pEntryForward = (LPENTRY)FindType(lpszName, FALSE, tentrykind)) != NULL)
	{
	    // no good if entry found isn't a forward declaration
	    if ((pEntryForward->type.tentrykind & tFORWARD) == 0)
		ParseError(PERR_DUP_DEF);
	    lpEntry->lpEntryForward = pEntryForward;	// save * to forward
							// declaration
	    pEntryForward->lpEntryForward = lpEntry;	// also save pointer
							// from forward decl
							// back to real entry
	}
	    
    if (tentrykind == tSTRUCT || tentrykind == tENUM || tentrykind == tUNION)
        lpEntry->type.structenum.szTag = lpszName;  // store tag name
    else
        lpEntry->type.szName = lpszName;	// store interface name

    // important to actually store szTag now, so that we can match type
    // references of the form "struct tagname" from within the struct.
    // (and from later references if this is a forward declaration).

    // see if this a forward declaration
    // NOTE: we don't support forward declares of enums, because there is
    // no syntax that works for referencing them.
    if (tentrykind != tENUM && tok.id == RW_SEMI)
	{
	    //can't have any attributes on forward declarations.
	    if (lpEntry->attr.fAttr || lpEntry->attr.fAttr2)
		ParseError(PERR_INV_ATTR);     //CONSIDER: better error?

	    // mark this as a forward declaration and quit.  We'll fill
	    // in the rest of the info later when we get the real def.
	    lpEntry->type.tentrykind |= tFORWARD;
	    ScanTok(0);			// consume the semicolon

	    EnsureNoDupEntries(lpEntry);	// check for dup def

	    return TRUE;
	}
    return FALSE;
}
// end common code


// parses a TypeDef section (3 forms)
VOID NEAR ParseTypedef
(
    LPENTRY lpEntry
)
{

    // attributes must follow 'typedef' keyword for MIDL compatibility
    if (lpEntry->attr.fAttr || lpEntry->attr.fAttr2)
	ParseErrorTokLast(PERR_TYPEDEF_ATTR);

    Assert(tok.id == RW_TYPEDEF);
    ScanTok(0);		// consume "TypeDef", advance to next token

    ParseOptAttr(&(lpEntry->attr), cTYPE); // parse attrs, if any

    switch (tok.id)
    {
	case RW_ENUM:
	    ParseStructEnumUnion(lpEntry, tENUM);
	    break;

	case RW_STRUCT:
	    ParseStructEnumUnion(lpEntry, tSTRUCT);
	    break;

	case RW_UNION:
	    ParseStructEnumUnion(lpEntry, tUNION);
	    break;

	default:		// a regular typedef
	    ParseAlias(lpEntry);
	    break;
    }
}




// parse typedef <basename> <aliasname>;
VOID NEAR ParseAlias
(
    LPENTRY lpEntry
)
{
    lpEntry->type.tentrykind = tTYPEDEF;

    // ensure attributes valid in this context
    CheckAttr(&lpEntry->attr, VALID_TYPEDEF_ATTR, VALID_TYPEDEF_ATTR2);

    // parse base type
    lpEntry->type.td.ptypeAlias = ParseKnownType(&lpEntry->attr,
						 fAllowInter |
						 fAllowMODULE);

    lpEntry->type.szName = ConsumeId();	// get/consume type name

    EnsureNoDupEntries(lpEntry);	// check for dup def

    ConsumeTok(RW_SEMI, 0);		// ends with a semicolon
}


// ParseStructEnumUnion()
// parses:
//     typedef struct/enum/union [tagname] { <items> } <name>;
//     typedef struct tagname;		// forward declare of a struct
//     typedef union  tagname;		// forward declare of a union
// doesn't support the following syntax:
//     typedef struct/enum/union tagname typedefname;
VOID NEAR ParseStructEnumUnion
(
    LPENTRY lpEntry,
    TENTRYKIND tentrykind
)
{
    long EnumVal = -1L;		// enums start at 0 by default

    // ensure attributes valid in this context
    CheckAttrTokLast(&lpEntry->attr,
		     VALID_STRUCT_ENUM_UNION_ATTR,
		     VALID_STRUCT_ENUM_UNION_ATTR2);

    ScanTok(0);					// consume "struct/enum/union"

    lpEntry->type.tentrykind = tentrykind;
    lpEntry->type.structenum.elemList = NULL;	// no struct/enum/union elements
    lpEntry->type.structenum.szTag = NULL;	// assume no tag name

    if (tok.id == LIT_ID)		// if got an (optional) tag name
	{
	    // Note -- we allow forward declares of structs, enums, & unions
	    if (FHandleForwardDecl(lpEntry, tentrykind))
	        return;			// if THIS is a forward decl
	}

    ConsumeTok(RW_LCURLY, 0);

    if (tentrykind == tENUM)
	{
	    // parse first enum item
	    ParseNewEnumElem(&lpEntry->type.structenum.elemList, &EnumVal);
	    while (tok.id == RW_COMMA)   // enum items are comma-separated
		{
		    ScanTok(0);		// consume the comma
		    if (tok.id == RW_RCURLY)	// OK to end in a comma
		        break;
		    ParseNewEnumElem(&lpEntry->type.structenum.elemList, &EnumVal);
	        }
	}
    else   // a struct or a union
	{
	    do
		{
		    ParseNewElem(&lpEntry->type.structenum.elemList,
				 VALID_STRUCT_UNION_ELEM_ATTR,
				 VALID_STRUCT_UNION_ELEM_ATTR2,
				 &(lpEntry->type));
		    ConsumeTok(RW_SEMI, 0);	// ends with a semicolon
		} while (tok.id != RW_RCURLY);
	}

    ConsumeTok(RW_RCURLY, 0);		// advance to struct/enum/union name

    lpEntry->type.szName = ConsumeId();	// get/consume type name

    EnsureNoDupEntries(lpEntry);	// check for dup def

    ConsumeTok(RW_SEMI, 0);		// ends with a semicolon
}


VOID NEAR ParseNewEnumElem
(
    LPELEM FAR * lplpElem,
    long *  pEnumVal
)
{
    long enumVal;
    LPELEM lpElem;
    VARIANT FAR * lpElemVal;

    ListInsert(lplpElem, sizeof(ELEM));
    lpElem = *lplpElem;

    ParseOptAttr(&lpElem->attr, cVAR);			// parse attributes, if any
    // ensure attributes valid in this context
    CheckAttrTokLast(&lpElem->attr,
		     VALID_ENUM_ELEM_ATTR,
		     VALID_ENUM_ELEM_ATTR2);

    ParseElemName(lpElem, 0);			// parse enum element name

    if (tok.id == RW_ASSIGN)	// got a value
	{
	    ScanTok(fACCEPT_NUMBER);
	    enumVal = (long)lParseNumericExpr();	// enum's are signed
	    // enums in win16 must be in the range of an I2.
	    if (SysKind == SYS_WIN16 && (enumVal > 32767L || enumVal < -32768L))
		ParseErrorTokLast(PERR_NUMBER_OV);
	    *pEnumVal = enumVal;	// update current value
	    lpElem->attr.fAttr2 |= f2GotConstVal;	// value explictly
							// specified
	}
    else
	{
	    enumVal = ++(*pEnumVal);		// one more than the last one
	    // if we just overflowed then give error
	    if (SysKind == SYS_WIN16) {
		if (enumVal == 32768L)
		  ParseErrorTokLast(PERR_NUMBER_OV);
	    } else if (enumVal == 0x80000000L)
		ParseErrorTokLast(PERR_NUMBER_OV);
	}

    // now create a variant that contains the value (typlib generator
    // needs it in the form of a variant).
    lpElemVal = (VARIANT FAR *)ParseMalloc(sizeof(VARIANT));
    lpElem->lpElemVal = lpElemVal;

#ifdef DEBUG
    lpElem->elemType = NULL;	// not referenced -- element type is INT.
#endif

    // type of an enum element's value is I2 in WIN16 typelibs, I4 otherwise
    // can't just always set lVal field, due to MAC byte-swapping issues
    if (SysKind == SYS_WIN16)
	{
	    lpElemVal->vt = VT_I2;
	    lpElemVal->iVal = LOWORD(enumVal);
	}
    else
	{
	    lpElemVal->vt = VT_I4;
	    lpElemVal->lVal = enumVal;
	}
    EnsureNoDupElem(lpElem);			// ensure no duplicate elements now
}


VOID NEAR ParseConstant
(
    LPELEM lpElem
)
{

    LPTYPE lpType;
    long  constVal;
    WORD  vt;
    LPSTR lpszConstVal;
    WORD  cbszConstVal;
    BSTR bstrVal;

    // ensure attributes valid in this context
    CheckAttrTokLast(&lpElem->attr,
		     VALID_MODULE_CONST_ATTR,
		     VALID_MODULE_CONST_ATTR2);

    ParseElem(lpElem, 0, NULL);

    // get the type the user said this is.
    lpType = lpElem->elemType;
    while (lpType->tentrykind == tTYPEDEF)
	lpType = lpType->td.ptypeAlias;

    // we don't support constants of external types
    if (lpType->tentrykind & tIMPORTED)
	ParseError(PERR_INV_CONSTANT);

    // CONSIDER: (V2) enhance this to accept date constants.  Others?
    ConsumeTok(RW_ASSIGN, fACCEPT_NUMBER | fACCEPT_STRING);
    if (tok.id == LIT_STRING) {
	lpszConstVal = tok.lpsz; // save * to string constant
	cbszConstVal = tok.cbsz; // save string length (not including null)
	ScanTok(0);		// consume entry string
	constVal = 0;		// in case of error
    } else {
        constVal = (long)lParseNumericExpr();
	lpszConstVal = NULL;	// not a string
    }

    // now create a variant that contains the value (typlib generator
    // needs it in the form of a variant).
    lpElem->lpElemVal = (VARIANT FAR *)ParseMalloc(sizeof(VARIANT));
    lpElem->attr.fAttr2 |= f2GotConstVal;	// value explictly specified

    // valididate that the type the user says is compatible with the
    // type of the constant's value.

    vt = VT_I2;		// assume variant value is tagged as an I2.
			// the only variant value tags currently supported are
			// VT_I2, VT_I4, VT_BOOL, and VT_ERROR.
    switch (lpType->tdesc.vt)
	{
	    case VT_I1:
		if (constVal < -128 || constVal > 127)
		    ParseErrorTokLast(PERR_NUMBER_OV);
		break;
	    case VT_UI1:
		if ((DWORD)constVal > 255)
		    ParseErrorTokLast(PERR_NUMBER_OV);
		break;

	    case VT_INT:
		if (SysKind != SYS_WIN16)
		    goto GotI4;		// it's an I4

	    case VT_BOOL:
		vt = VT_BOOL;

	    case VT_I2:
		if (constVal < -32768 || constVal > 32767)
		    ParseErrorTokLast(PERR_NUMBER_OV);
		break;

	    case VT_UINT:
		if (SysKind != SYS_WIN16)
		    goto GotI4;		// it's an unsigned I4 -- no overflow

	    case VT_UI2:
		if ((DWORD)constVal > 65535L)
		    ParseErrorTokLast(PERR_NUMBER_OV);
		break;

	    case VT_I4:			// no overflow possible
	    case VT_UI4:
GotI4:
		vt = VT_I4;		// it's an I4
		break;

	    case VT_ERROR:
		vt = VT_ERROR;		// VT_ERROR is a valid variant tag
		break;

	    case VT_LPSTR:
		vt = VT_BSTR;
		break;

#if 0
	    case VT_LPWSTR:	// CONSIDER: allow unicode string literals?
		vt = VT_BSTR;
		break;
#endif //0

	    case VT_PTR:		// ensure "char *"
		// CONSIDER: allow wchar_t *, too?
		if (GetStringType(lpType) == VT_LPSTR) {
		    vt = VT_BSTR;
		    break;
		}
		// fall through to give error

	    // CONSIDER: (V2) enhance this to support constants of other
	    // CONSIDER: (V2) intrinsic types
	    default:
		ParseError(PERR_INV_CONSTANT);
	}
    lpElem->lpElemVal->vt = vt;			// store tag

    if (vt == VT_BSTR) {
	if (lpszConstVal == NULL)
	    ParseError(PERR_EXP_STRING);
#ifdef WIN32 
	bstrVal = SysAllocStringLen(NULL, cbszConstVal);
	if (cbszConstVal) {
	    SideAssert (MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    lpszConstVal,
				    cbszConstVal,
				    bstrVal,
				    cbszConstVal) != 0);
	}
#else //WIN32
	bstrVal = SysAllocStringLen(lpszConstVal, cbszConstVal);
#endif //WIN32

	if (bstrVal == NULL && cbszConstVal != 0)
	    ParseError(ERR_OM);
	lpElem->lpElemVal->bstrVal = bstrVal;
    } else {
	if (lpszConstVal != NULL)
	    ParseError(PERR_EXP_NUMBER);
#ifdef MAC
        // can't just always set lVal field, due to MAC byte-swapping issues
        if (vt == VT_I2 || vt == VT_BOOL)
            lpElem->lpElemVal->iVal = LOWORD(constVal);	// store data
        else
#endif //MAC
            lpElem->lpElemVal->lVal = constVal;		// store data
    }

    ConsumeTok(RW_SEMI, 0);	// ends with a semicolon

}	

// ******************************************************************
// FUNCTION-related routines
// ******************************************************************

VOID NEAR ParseFunction
(
    LPFUNC  lpFunc,	// pointer to where to put this function's info
    FUNCTYPE funcType
)
{
    ATTR attr;		// local attribute buffer
    SHORT cArgs;	// # of args for this function
    SHORT cOptArgs;	// # of optional args for this function
    LPTYPE lpArgType;
    LPTYPE lpArgTypeLast;
    LPTYPE lpTypeRetVal;
    LPSTR szFuncName;
    LPTYPE lpTypeBase;
    DWORD IDLFlagsSeen;
    DWORD IDLFlagsCur;
    SHORT cNeedRHS;

    // if 'bindable' attr specified, must be a property function
    if ((lpFunc->func.attr.fAttr & fBINDABLE)
       && !(lpFunc->func.attr.fAttr & fPropBits))
	ParseErrorTokLast(PERR_INV_ATTR_COMBO);

    // ensure attributes valid in this context
    CheckAttr(&lpFunc->func.attr,
	      fValidFuncAttrs[funcType],
	      fValidFuncAttrs2[funcType]); 

    // parse function return type
    lpFunc->func.elemType = ParseKnownType(&lpFunc->func.attr,
					   fAllowSAFEARRAY);

    lpTypeRetVal = lpFunc->func.elemType;	// store in case no retval parm

    // if PROPGET specified, return type cannot be VOID
    if ((lpFunc->func.attr.fAttr & fPROPGET)
       && IsType(lpTypeRetVal, VT_VOID))
	ParseErrorTokLast(PERR_INV_PROPFUNC);

    if (!IsType(lpTypeRetVal, VT_VOID)) {
	// special case of VOID return type -- allowed everywhere
	if (!IsType(lpTypeRetVal, VT_HRESULT)) {
            // if PROPPUT or PROPPUTREF specified, return type must be VOID/HRESULT
            if (lpFunc->func.attr.fAttr & (fPROPPUT | fPROPPUTREF))
	        ParseErrorTokLast(PERR_INV_PROPFUNC);

   	    // OA functions must return either VOID or HRESULT.
            if (funcType == FUNC_OAINTERFACE)
	        ParseErrorTokLast(PERR_INV_OA_FUNC_TYPE);
	}
	// ensure return type is a valid IDispatch/OA type
        if (funcType == FUNC_DISPINTERFACE)
	    EnsureIDispatchType(lpTypeRetVal);
    }

    // parse optional "far"
    if (tok.id == RW_FAR)
	ScanTok(0);			// consume far or _far if present

    // parse optional calling convention.
    switch (tok.id)
	{
	case RW_CDECL:			// cdecl, _cdecl or __cdecl
	    lpFunc->func.attr.fAttr2 |= f2CDECL;
	    goto CheckCallConv;

	case RW_PASCAL:			// pascal, _pascal or __pascal
	    lpFunc->func.attr.fAttr2 |= f2PASCAL;
	    goto CheckCallConv;

	case RW_STDCALL:		// stdcall, _stdcall or __stdcall
	    lpFunc->func.attr.fAttr2 |= f2STDCALL;
CheckCallConv:
	    ScanTok(0);			// consume it

            // set this flag so the .h file outputs a bunch of macros
            // for the STDMETHODCALLTYPE.
            if (funcType == FUNC_INTERFACE || funcType == FUNC_OAINTERFACE)
                fSpecifiedInterCC = TRUE;

	    // explicit calling convention illegal in dispinterfaces and
	    // oa-compatible interfaces.
	    if (funcType == FUNC_DISPINTERFACE || funcType == FUNC_OAINTERFACE)
		ParseErrorTokLast(PERR_INV_CALLCONV);
	    break;

	default:
	    lpFunc->func.attr.fAttr2 |= (f2DefaultCC | f2CCDEFAULTED);
	    break;
	}

    lpFunc->argList = NULL;		// no args yet

    szFuncName = ConsumeId();		// get/consume function name
    lpFunc->func.szElemName = szFuncName; // store function name

    EnsureNoDupElem((LPELEM)lpFunc);	// ensure no duplicate functions

    ConsumeTok(RW_LPAREN, 0);		// start of parms

    cArgs = 0;
    cOptArgs = 0;
    IDLFlagsSeen = 0;
    cNeedRHS = 0;
    lpArgTypeLast = NULL;		// no last parm yet (in case vararg)
    if (tok.id != RW_RPAREN)		// if any parms are present
	{
#pragma warning(disable:4127)
	    while (TRUE)
#pragma warning(default:4127)
		{  // parse a function parameter

		    // parse & validate parm attributes, if any
    		    ParseOptAttr(&attr, cPARAM);

 		    // ensure attributes valid in this context
		    CheckAttr(&attr,
		    	      fValidParmAttrs[funcType],
		    	      fValidParmAttrs2[funcType]); 

		    IDLFlagsCur = attr.fAttr & (fRETVAL | fLCID);

		    // can't have both lcid & retval on the same arg
		    if (IDLFlagsCur == (fRETVAL | fLCID))
			ParseError(PERR_INV_ATTR_COMBO);

		    // only parm that can come after 'lcid' is 'retval',
                    // unless we're looking for an RHS parameter, but then
                    // we only allow one parameter to follow.
		    if (IDLFlagsSeen & fLCID && !(IDLFlagsCur & fRETVAL) && cNeedRHS != 1)
			ParseError(PERR_INV_LCID_USE);                    

                    // If we need an RHS parameter, we want an IN parameter 
                    // that is not marked as RETVAL or LCID.
                    if (cNeedRHS && (!(attr.fAttr & fIN) || IDLFlagsCur)) 
                        ParseError(PERR_INV_PROPFUNC);

                    // If we're made it this far while looking for an RHS parameter,
                    // we've found it.
                    if (cNeedRHS) 
                        cNeedRHS++;

                    // If we have an LCID parameter and are a property PUT/PUTREF,
                    // we MUST have an RHS parameter next.
                    if (IDLFlagsCur & fLCID 
                        && lpFunc->func.attr.fAttr & (fPROPPUT | fPROPPUTREF))

                        cNeedRHS = 1;

		    // Retval param is supposed to be last.
		    if (IDLFlagsSeen & fRETVAL)
			ParseErrorTokLast(PERR_INV_RETVAL_USE);

                    // No retval allowed on PUT/PUTREF.
                    if (IDLFlagsCur == fRETVAL 
                        && lpFunc->func.attr.fAttr & (fPROPPUT | fPROPPUTREF))

                        ParseErrorTokLast(PERR_INV_PROPFUNC);
                         
	    	    IDLFlagsSeen |= IDLFlagsCur;

		    // parse parm type
		    lpArgType = ParseKnownType(&attr, fAllowArray);
		    if (IDLFlagsSeen == 0) {
			lpArgTypeLast = lpArgType;	// save in case vararg
		    }
                
		    // special case of foo(void) ==> no args in this case.
		    // other uses of 'void' are invalid.
		    if (IsType(lpArgType, VT_VOID))
			{
			    if (tok.id == RW_RPAREN && cArgs == 0 && attr.fAttr == 0)
				break;		// all done
			    else
				ParseErrorTokLast(PERR_VOID_INV);
			}

		    cArgs++;			// got one more arg

		    // one (or both) 'in', 'out' must be specified if we're
		    // not a dispinterface.
		    if (funcType != FUNC_DISPINTERFACE && !(attr.fAttr & (fIN | fOUT)))
			ParseErrorTokLast(PERR_IN_OUT_REQ);

		    if (attr.fAttr & fOPTIONAL)
			{
			    // optional args & 'vararg' don't mix
			    if (lpFunc->func.attr.fAttr & fVARARG) {
			        ParseErrorTokLast(PERR_INV_VARARG_USE);
			    }
			    cOptArgs++;		// one more optional arg
			}
		    else
			{	// can't have optional args in the middle
			if (cOptArgs)
			  if (!IDLFlagsSeen)	// if we're not a LCID or RETVAL
			    ParseErrorTokLast(PERR_INV_ATTR);
			}

		    ListInsert(&lpFunc->argList, sizeof(ELEM));
		    lpFunc->argList->attr = attr;	// store parm attributes
		    lpFunc->argList->elemType = lpArgType; // store parm type

		    // parse parm name
		    ParseElemName(lpFunc->argList, fAllowArray);
		    EnsureNoDupElem(lpFunc->argList);	// ensure no duplicate
							// parm names

		    // ensure this is a valid IDispatch/OA type
		    if (funcType == FUNC_DISPINTERFACE)
			EnsureIDispatchType(lpFunc->argList->elemType);
		    else if (funcType == FUNC_OAINTERFACE)
			EnsureOAType(lpFunc->argList->elemType, TRUE);

		    // if 'out' specified, parameter must be a pointer type
		    if (attr.fAttr & fOUT) {
			lpTypeBase = lpArgType;
			while (lpTypeBase->tentrykind == tTYPEDEF)
			    lpTypeBase = lpTypeBase->td.ptypeAlias;
			if (!(lpTypeBase->tentrykind & tIMPORTED)) {
			  // can't reliably check if an imported type
			  switch (lpTypeBase->tdesc.vt) {
			    case VT_PTR:
			    case VT_LPSTR:
			    case VT_LPWSTR:
			    case VT_SAFEARRAY:
				break;
			    default:
			        ParseErrorTokLast(PERR_INV_OUT_PARAM);
		          }
			}
		    }

		    // 'optional' only allowed on non-array VARIANT args
		    if ((attr.fAttr & fOPTIONAL) && !IsType(lpArgType, VT_VARIANT))
			{	// an optional VARIANT * is also OK
			lpTypeBase = lpArgType;
			while (lpTypeBase->tentrykind == tTYPEDEF)
			    lpTypeBase = lpTypeBase->td.ptypeAlias;
		        if (lpTypeBase->tdesc.vt != VT_PTR ||
				lpTypeBase->ref.cIndirect != 1 ||
			        !IsType(lpTypeBase->ref.ptypeBase, VT_VARIANT))
			    ParseErrorTokLast(PERR_INV_ATTR);
			}

		    // validate 'retval' and 'lcid' usage
		    if (IDLFlagsCur) {
		        if (IDLFlagsCur & fLCID) {
			    // 'lcid' only legal if it's a DWORD type
			    if (!IsType(lpArgType, VT_I4))
			        ParseErrorTokLast(PERR_INV_LCID_USE);
			    // must be 'in', optional not allowed
			    if ((attr.fAttr & (fIN | fOUT | fOPTIONAL)) != fIN)
			        ParseErrorTokLast(PERR_INV_LCID_USE);
			}

		        if (IDLFlagsCur & fRETVAL) {

			    lpTypeRetVal = lpArgType;	// update * return type

		            // 'retval' is only allowed on
		            // hresult-returning functions.
			    if (!IsType(lpFunc->func.elemType,VT_HRESULT))
			        ParseErrorTokLast(PERR_INV_ATTR);

			    // must be 'out', optional not allowed
			    if ((attr.fAttr & (fIN | fOUT | fOPTIONAL)) != fOUT)
			        ParseErrorTokLast(PERR_INV_RETVAL_USE);

			    // 'retval' only legal if it's a pointer type
			    // this is ensured by the check above that 'out'
			    // parms must be pointers

			    // But it can't be an alias type, either (due to
			    // a typelib.dll limitation (bug?) in the munging
			    // code in gdtinfo.cxx -- VBA2 bug #3716).
			    // That code must have a pure pointer type.  We're
			    // too close to Daytona ship to fix it in typelib,
			    // so we impose the restriction here.
			    if (!(lpArgType->tentrykind & tIMPORTED)) {
			      // can't reliably check if an imported type
			      if (lpArgType->tdesc.vt != VT_PTR)
			          ParseErrorTokLast(PERR_INV_RETVAL_USE);
			    }
			}

		    }

		    if (tok.id == RW_RPAREN)	// if all done
			break;

		    ConsumeTok(RW_COMMA, 0);	// must have another parm
		}

	}

    // if 'propput' or 'propputref' is specified, then must have at least
    // one non-lcid argument the comes after any LCID parameter that
    // may exist.
    if ((lpFunc->func.attr.fAttr & (fPROPPUT | fPROPPUTREF))
        && (cArgs == 0 || cArgs == 1 && IDLFlagsSeen & fLCID || cNeedRHS == 1))
	ParseErrorTokLast(PERR_INV_PROPPUT);

    // if SOURCE specfied, must be an object/variant return type (ignore prop
    // put/putref when checking for this.
    if ((lpFunc->func.attr.fAttr & fSOURCE)
	&& !(lpFunc->func.attr.fAttr & (fPROPPUT | fPROPPUTREF))
	&& !IsObjectType(lpTypeRetVal))
	ParseErrorTokLast(PERR_INV_SOURCE_ATTR);

    // if 'vararg' specified, ensure last arg is a SAFEARRAY(VARIANT) or
    // SAFEARRAY (VARIANT) *.   Note that the following code doesn't accept
    // things that are typedef'ed to be a SAFEARRAY or SAFEARRAY *, but
    // I don't think that is important.

    // lpArgTypeLast points to the type of the last arg, not including the
    // lcid or retval parms, if there is an arg that meets this criteria
    if (lpFunc->func.attr.fAttr & fVARARG)
	{
	    // error if no last arg
	    if (lpArgTypeLast == NULL)
		ParseErrorTokLast(PERR_INV_VARARG_USE);

	    // if pointer type, get base type
	    if (lpArgTypeLast->tdesc.vt == VT_PTR && lpArgTypeLast->ref.cIndirect == 1)
		lpArgTypeLast = lpArgTypeLast->ref.ptypeBase;

	    if (lpArgTypeLast->tdesc.vt != VT_SAFEARRAY
	    	|| !IsType(lpArgTypeLast->ref.ptypeBase, VT_VARIANT))
		ParseErrorTokLast(PERR_INV_VARARG_USE);
	    cOptArgs = -1;		// tell type lib generator that VARARG
					// was specified
	}

    if (cArgs > cArgsMax)		// update global max # of args
	cArgsMax = cArgs;
    lpFunc->cArgs = cArgs;		// store cArgs
    lpFunc->cOptArgs = cOptArgs;	// store cOptArgs
    ScanTok(0);				// consume the right paren
    ConsumeTok(RW_SEMI, 0);		// ends with a semicolon

}


BOOL NEAR IsType
(
    LPTYPE lpType,
    VARTYPE vt
)
{
    while (lpType->tentrykind == tTYPEDEF)
	lpType = lpType->td.ptypeAlias;
    return (lpType->tdesc.vt == vt);
}


// Checks to see that a given type is compatible with IDispatch::Invoke,
// or is "Ole Automation compatible".  Gives a warning if it is not.
VOID NEAR EnsureIDispatchType
(
    LPTYPE lpType
)
{
    if ((GetTypeCompatibility(lpType, 0) & COMPAT_IDISPATCH) == 0) {
      // Type not supported by IDispatch::Invoke -- give a WARNING
      ParseErrorTokLast(PWARN_INV_IDISPATCHTYPE);
    }
}

// Checks to see that a given type is "Ole Automation compatible".
// Gives an error if it is not.
VOID NEAR EnsureOAType
(
    LPTYPE lpType,
    BOOL fParam
)
{
    if ((GetTypeCompatibility(lpType, (WORD)(fParam ? VT_PARAM : 0)) & COMPAT_OA) == 0) {
      ParseErrorTokLast(PERR_INV_OA_TYPE);	// Type not a valid OA type
    }
}

// Returns info about whether or not a given type is compatible with
// IDispatch::Invoke, or is "Ole Automation compatible".
//
// In the case where VT_VOID and VT_HRESULT are allowed (on a function return
// value), we depend on our caller to not call us.
//
// Types allowed for dispinterfaces:
//    - All basic types (<= VT_UI1)
//      | VT_USERDEFINED/tTYPEDEF (external)
//    - the above with one level of indirection.
//
// Types allowed for OA compatibility:
//
//    - All basic types (<= VT_UI1)
//      | VT_USERDEFINED/tTYPEDEF,tENUM
//      | VT_PTR -> VT_USERDEFINED/tINTERFACE,tDISPATCH,tCOCLASS
//      | VT_INT
//    - the above with one additional level of indirection.
//
WORD NEAR GetTypeCompatibility
(
    LPTYPE lpType,
    WORD fRecurse		// 0 -- not being called recursively
				// VT_BYREF -- being called for a pointer
				// VT_ARRAY -- being called for an array
                                // VT_PARAM -- being called for a parameter
)
{
    WORD wCompat;
    UINT cInd;

    // Treat an array as just another level of indirection.
    BOOL fBYREF = fRecurse & VT_BYREF;
    BOOL fARRAY = fRecurse & VT_ARRAY;
    BOOL fPARAM = fRecurse & VT_PARAM;
    BOOL fSIMPPARAM = fPARAM || !(fBYREF || fARRAY);

    while (lpType->tentrykind == tTYPEDEF)
	lpType = lpType->td.ptypeAlias;

    cInd = lpType->ref.cIndirect;

    // IDispatch and OA always allow the below types.
    if (lpType->tdesc.vt <= VT_UNKNOWN || lpType->tdesc.vt == VT_UI1) {
      // The contents of a safearray may not containg a reference, unless
      // they're pointing to a UDT.  This is not the case, so if we have
      // both a reference AND a safearray, return NONE.
      //
      if (fBYREF && fARRAY) {
        return COMPAT_NONE;
      }
      else {
        return COMPAT_IDISPATCH | COMPAT_OA;
      }
    }

    switch (lpType->tdesc.vt) {
	case VT_PTR:
            // Can't even consider more than 2 levels of indirection.
            if (cInd > 2) {
              return COMPAT_NONE;
            }

            // Let VT_USERDEFINED know if we're dealing with
            // 2 levels of indirection (for ENUM).
            //
            if (!fPARAM || fARRAY || cInd == 2) {
              fRecurse |= VT_BYREF;
            }

	    wCompat = GetTypeCompatibility(lpType->ref.ptypeBase, fRecurse);

            // If the base type is VT_PTR, this isn't valid for IDispatch.
            if (cInd > 1) {
              wCompat &= ~COMPAT_IDISPATCH;
            }

            // Legal values:
            //   cInd = 1, fPARAM = 0, VT == VT_UDT
            //   cInd = 2, fPARAM = 0, error
            //   cInd = 1, fPARAM = 1, fARRAY = 0, VT == ANY
            //   cInd = 1, fPARAM = 1, fARRAY = 1, VT == VT_USERDEFINED
            //   cInd = 2, fPARAM = 1, fARRAY = 0, VT == VT_USERDEFINED
            //   cInd = 2, fPARAM = 1, fARRAY = 1, error
            //
            if (lpType->ref.ptypeBase->tdesc.vt != VT_USERDEFINED
                && (cInd == 1 && (!fPARAM || fARRAY) || cInd == 2 && fPARAM)
                || (cInd == 2 && (!fPARAM || fARRAY))) { 

              wCompat &= ~COMPAT_OA;
            }

	    return wCompat;

	case VT_SAFEARRAY:
            // Only one array.
            if (fARRAY) {
              return COMPAT_NONE;
            }

            // We allow SAFEARRAY(type)* but not SAFEARRAY (type *), unless
            // type is a UDT.  So if the fBYREF flag has been set, clear it
            // so we can property check for byrefs in the array.
            //
	    return GetTypeCompatibility(lpType->ref.ptypeBase, 
                                        (WORD)(fRecurse & ~VT_BYREF | VT_ARRAY));

        // VT_INT is only allowed in OLE automation.
	case VT_INT:
	    return COMPAT_OA;
        
	case VT_USERDEFINED:
          switch (lpType->tentrykind & ~(tFORWARD | tIMPORTED | tQUAL)) {
	    case tENUM:
                // fBYREF is set if we're dealing with 2 levels of
                // indirection.
                //
                if (fBYREF) {
                  return COMPAT_NONE;
                }

		return COMPAT_OA;

            // Cases of the following being used without a level of indirection
            // are caught elsewhere.
            //
	    case tINTERFACE:
		switch(lpType->tentrykind & ~tQUAL) {
		  case tINTERFACE:
		    if (((LPENTRY)lpType)->attr.fAttr2 & f2OACompatBits) {
			if (((LPENTRY)lpType)->attr.fAttr2 & f2VALIDDUALBASE) {
		            return COMPAT_OA | COMPAT_DUALBASE;
			} else {
		            return COMPAT_OA;
			}
		    }
		    break;
		  case (tINTERFACE | tIMPORTED):
		    if ((lpType->import.wTypeFlags & TYPEFLAG_FOLEAUTOMATION))
			// assume valid for use as a base of a dual interface
		        return COMPAT_OA | COMPAT_DUALBASE;
		    break;

		  case (tINTERFACE | tFORWARD):
		    if (((LPENTRY)lpType)->lpEntryForward != NULL) {
		        // if we can get at the real definition, we can check
			if ((((LPENTRY)lpType)->lpEntryForward->attr.fAttr2 & f2OACompatBits) == 0) {
			     break;		// not OA-compatible
			}
		    }
		    // If all we have is a forward declare, we have no
		    // means of checking to see if this is any good or not,
		    // so assume it's OK.
		    return COMPAT_OA | COMPAT_DUALBASE;

		  default:
		    Assert(FALSE);
		}
	        break;			// not OA-compatible

	    case tDISPINTER:
		return COMPAT_IDISPATCH | COMPAT_OA;

	    case tCOCLASS:
		// UNDONE: What are the rules for ensuring COCLASSes 
		// UNDONE: are OA-compatible?  Assume they all are for now.
		return COMPAT_OA;

	    case tTYPEDEF:
		if (lpType->tentrykind & tIMPORTED) {
		    // UNDONE: Assume external typedef's are OA-compatible for
		    // UNDONE: now, rather than walking the typeinfos to find
		    // UNDONE: the true base type & flags.
		    return (COMPAT_IDISPATCH | COMPAT_OA | COMPAT_DUALBASE);
		}
	    default:
		break;
	  }
	  // fall through to return COMPAT_NONE

	default:
	    break;
	  // fall through to return COMPAT_NONE
    }
    return COMPAT_NONE;		
}


// see if this type is (or could be) and object type.
// this means VARIANT, VARIANT *, coclass *, dispinterface *, or interface *.
BOOL NEAR IsObjectType
(
    LPTYPE lpType
)
{
    while (lpType->tentrykind == tTYPEDEF)
	lpType = lpType->td.ptypeAlias;

    switch (lpType->tdesc.vt) {
	case VT_UNKNOWN:
	case VT_DISPATCH:
	case VT_VARIANT:
	  return TRUE;		// variant can contain an object

	case VT_PTR:		// maybe a pointer to an object or variant
	  lpType = lpType->ref.ptypeBase;	// get true base type
	  while (lpType->tentrykind == tTYPEDEF)
	      lpType = lpType->td.ptypeAlias;
          switch(lpType->tentrykind & ~(tFORWARD | tIMPORTED | tQUAL)) {
	    case tINTERFACE:
	    case tDISPINTER:
	    case tCOCLASS:
	      return TRUE;		// pointer to an object is ok

	    case tINTRINSIC:
		 switch (lpType->tdesc.vt) {
		    case VT_UNKNOWN:
		    case VT_DISPATCH:
		    case VT_VARIANT:
		      return TRUE;	// pointer to a variant/object is ok
		 }
	    default:
	      break;
	  }
	  // fall through

	default:
          break;
    }
    return FALSE;		// not an object type
}

// parses a "module" section
VOID NEAR ParseModule
(
)
{
    LPENTRY lpEntry = typlib.pEntry;
    ATTR attr;

    // ensure attributes valid in this context
    CheckAttr(&lpEntry->attr, VALID_MODULE_ATTR, VALID_MODULE_ATTR2);

    // 'dllname' attr required on a module
    if ((lpEntry->attr.fAttr & fDLLNAME) == 0)
	ParseError(PERR_DLLNAME_REQ);

    ScanTok(0);			// consume "module", advance to next token

    lpEntry->type.tentrykind = tMODULE;	// store information in record
    lpEntry->type.szName = ConsumeId();	// get/consume module name
    // no need to set rest of type structure

    lpEntry->module.funcList = NULL;	// no functions initially
    lpEntry->module.constList = NULL;	// no constants initially

    EnsureNoDupEntries(lpEntry);		// check for dup def

    ConsumeTok(RW_LCURLY, 0);

    while (tok.id != RW_RCURLY)
	{
	    ParseOptAttr(&attr, cFUNC);		// parse attributes, if any

	    if (tok.id == RW_CONST)
		{   // a constant
		    ScanTok(0);			// consume "const"
		    ListInsert(&(lpEntry->module.constList), sizeof(ELEM));
		    lpEntry->module.constList->attr = attr;
		    ParseConstant(lpEntry->module.constList);
		}
	    else
		{	// assume function
		    ListInsert(&(lpEntry->module.funcList), sizeof(FUNC));
		    lpEntry->module.funcList->func.attr = attr;	// store attrs

		    // 'entry' attr required on a function in a module
		    if ((attr.fAttr & fENTRY) == 0)
			ParseError(PERR_ENTRY_REQ);

		    ParseFunction(lpEntry->module.funcList, FUNC_MODULE);
		}
	}

    ConsumeRCurlyOptSemi(0);            // consume rcurly and optional ;
}

// parse the interface section
VOID NEAR ParseInterface
(
)
{
    LPENTRY lpEntry = typlib.pEntry;
    LPTYPE lpTypeBase;

    // ensure attributes valid in this context
    CheckAttr(&lpEntry->attr, VALID_INTERFACE_ATTR, VALID_INTERFACE_ATTR2);

    ScanTok(0);			// consume "interface", advance to next token

    if (FHandleForwardDecl(lpEntry, tINTERFACE))
	return;				// if THIS is a forward decl

    // UUID is now required on all interfaces
    if (!(lpEntry->attr.fAttr & fUUID))
	ParseErrorTokLast(PERR_UUID_REQ);

    if (!(lpEntry->attr.fAttr2 & f2OACompatBits)) {
        // 'ODL' attr required on old-style (non-OA) interfaces
        if (!(lpEntry->attr.fAttr & fODL))
	    ParseErrorTokLast(PERR_ODL_REQ);
    }

    if ((lpEntry->attr.fAttr2 & (f2DUAL | f2NONEXTENSIBLE)) == f2NONEXTENSIBLE){
        // 'nonextensible' attr only valid on DUAL interfaces
	ParseErrorTokLast(PERR_INV_ATTR);
    }

    EnsureNoDupEntries(lpEntry);		// check for dup def

    lpEntry->inter.funcList = NULL;		// no functions initially
    lpEntry->type.inter.interList = NULL;	// no base interfaces

    lpTypeBase = NULL;
    if (tok.id == RW_COLON || (lpEntry->attr.fAttr2 & f2OACompatBits))
	{
	    ConsumeTok(RW_COLON, 0);		// consume separating colon
	    if (lpEntry->attr.fAttr2 & f2OACompatBits) {
	        if (lpTypeIDispatch == NULL)
		    FindIDispatch();	// set up lpTypeIDispatch BEFORE
		    			// ParseKnownType (below) so that
					// we can find this.
	    }

	    lpTypeBase = ParseKnownType(&lpEntry->attr, fAllowInter);
	    if ((lpTypeBase->tentrykind & ~(tFORWARD | tIMPORTED | tQUAL)) != tINTERFACE)
		ParseErrorTokLast(PERR_UNDEF_INTER);

	    // if we claim to be "Dual" or "OleAutomation"
	    if (lpEntry->attr.fAttr2 & f2OACompatBits) {
		WORD compatFlags;

		// ok to derive from IDispatch (even though it's
		// not marked as OA-compatible).
		if (lpTypeBase != lpTypeIDispatch) {
		    compatFlags = GetTypeCompatibility(lpTypeBase, VT_BYREF);
		    // ensure the interface we derive from is marked as
		    // Oleautomation compatible, and has the correct base
		    // interface.
	            if (lpEntry->attr.fAttr2 & f2DUAL) {
			// must derive from IDispatch, or another OA interface
			// that derives from IDispatch.
		        if (!(compatFlags & COMPAT_DUALBASE)) {
		            ParseErrorTokLast(PERR_INV_DUAL_BASE);
			}
		    } else {
			// must derive from IUnknown, or another OA interface
		        if (lpTypeBase != lpTypeIUnknown &&
				!(compatFlags & COMPAT_OA)) {
		            ParseErrorTokLast(PERR_INV_OA_BASE);
			}
		    }
		} else {
		    // IDisapatch valid as a base of a dual interface
		    compatFlags = COMPAT_DUALBASE;
		}

		// mark this interface as dual-compatible if base interface
		// is dual-compatible
		if (compatFlags & COMPAT_DUALBASE)
		    lpEntry->attr.fAttr2 |= f2VALIDDUALBASE;
	    }

	    ListInsert(&(lpEntry->type.inter.interList), sizeof(INTER));
	    lpEntry->type.inter.interList->ptypeInter = lpTypeBase;
			// store base interface type
	    lpEntry->type.inter.interList->fAttr = 0;	// no flags
	    lpEntry->type.inter.interList->fAttr2 = 0;
	}

    ConsumeTok(RW_LCURLY, 0);

    while (tok.id != RW_RCURLY)
	{
	    if (tok.id == RW_TYPEDEF)
		{	// a nested TYPEDEF

		    Assert (lpEntryPrev != NULL);

		    Assert(lpEntryPrev->type.pNext == (LPTYPE)lpEntry);

		    // create & init new item in entry list, linked in before
		    // the entry for the interface.  Sets lpEntryPrev to
		    // point to this new entry.
		    InitNewEntry(&lpEntryPrev);

		    // lpEntry still points to Interface entry
		    Assert(lpEntryPrev->type.pNext == (LPTYPE)lpEntry);

		    ParseTypedef(lpEntryPrev);	// parse nested typedef
		}
	    else
		{	// a function

		    ListInsert(&lpEntry->inter.funcList, sizeof(FUNC));

		    // parse attributes, if any
		    ParseOptAttr(&lpEntry->inter.funcList->func.attr, cFUNC);
		    ParseFunction(
			lpEntry->inter.funcList,
	    	        ((lpEntry->attr.fAttr2 & f2OACompatBits)
			    ? FUNC_OAINTERFACE : FUNC_INTERFACE));
		}
	}

    ConsumeRCurlyOptSemi(0);            // consume rcurly and optional ;
}


// parse the dispinterface section
//
// Goes to extra work to parse the properties and methods into a single list,
// and then split the list apart, so that you can't define the same name
// and/or ID in both the properties and methods sections.
VOID NEAR ParseDispinterface
(
)
{
    LPENTRY lpEntry = typlib.pEntry;
    LPELEM elemList = NULL;		// no entries initially
    LPELEM propList;
    LPELEM elemTemp;
    LPTYPE lpTypeBase;

    // ensure attributes valid in this context
    CheckAttr(&lpEntry->attr, VALID_DISPINTER_ATTR, VALID_DISPINTER_ATTR2);

    ScanTok(0);			// consume "dispinterface", advance to next tok

    if (FHandleForwardDecl(lpEntry, tDISPINTER))
	return;				// if THIS is a forward decl

    // UUID required on dispinterface section
    if (!(lpEntry->attr.fAttr & fUUID))
	ParseErrorTokLast(PERR_UUID_REQ);

    ConsumeTok(RW_LCURLY, 0);

    EnsureNoDupEntries(lpEntry);	// check for dup def

    lpEntry->dispinter.propList = NULL;	// assume no properties or methods
    lpEntry->dispinter.methList = NULL;
    lpEntry->type.inter.interList = NULL;	// no base interfaces
    ListInsert(&(lpEntry->type.inter.interList), sizeof(INTER));

    if (lpTypeIDispatch == NULL)
	FindIDispatch();	// set up lpTypeIDispatch

    lpEntry->type.inter.interList->ptypeInter = lpTypeIDispatch;
				// store base interface
    lpEntry->type.inter.interList->fAttr = 0;	// no flags
    lpEntry->type.inter.interList->fAttr2 = 0;	// no flags

    if (tok.id == RW_INTERFACE)
	{	// "interface <interfacename>;
	    ScanTok(0);		// consume "interface", advance to next tok
	    // CONSIDER: this code is pretty much the end of ParseClass()
	    // maybe combine this code into a single routine?
	    lpTypeBase = ParseKnownType(&lpEntry->attr, fAllowInter);

 	    if ((lpTypeBase->tentrykind & ~(tFORWARD | tIMPORTED | tQUAL)) != tINTERFACE)
		ParseErrorTokLast(PERR_UNDEF_INTER);

	    ListInsert(&(lpEntry->type.inter.interList), sizeof(INTER));
	    lpEntry->type.inter.interList->ptypeInter = lpTypeBase;
					// save * to referenced interface
	    lpEntry->type.inter.interList->fAttr = 0;	// no flags
	    lpEntry->type.inter.interList->fAttr2 = 0;

	    ConsumeTok(RW_SEMI, 0);		// ends with a semicolon
	    goto DoneDispinter;
	}

    // Parse the properties (if any).
    ConsumeTok(RW_PROPERTIES, 0);
    ConsumeTok(RW_COLON, 0);

    while (tok.id != RW_METHODS && tok.id != RW_RCURLY)
	{
	    ParseNewElem(&elemList,
			 VALID_DISPINTER_PROP_ATTR,
			 VALID_DISPINTER_PROP_ATTR2,
			 NULL);
	    ConsumeTok(RW_SEMI, 0);	// ends with a semicolon
	    // 'id' attribute is required on dispinterface items
	    if (!(elemList->attr.fAttr & fID))
		ParseErrorTokLast(PERR_ID_REQ);

	    // if SOURCE specfied, must be an object/variant type
	    if ((elemList->attr.fAttr & fSOURCE)
		&& !IsObjectType(elemList->elemType))
		ParseErrorTokLast(PERR_INV_SOURCE_ATTR);

	    // type of this var must be an IDispatch-compatible type
	    EnsureIDispatchType(elemList->elemType);
	}

    lpEntry->dispinter.propList = elemList;	// set property list
    propList = elemList;	// cache

    // Parse the methods (if any).
    ConsumeTok(RW_METHODS, 0);
    ConsumeTok(RW_COLON, 0);

    // now parse the methods (if any)
    while (tok.id != RW_RCURLY)
	{
	    ListInsert(&elemList, sizeof(FUNC));
	    ParseOptAttr(&elemList->attr, cFUNC);
	    
	    ParseFunction((LPFUNC)elemList, FUNC_DISPINTERFACE);
	    if (!(elemList->attr.fAttr & fID))
		ParseErrorTokLast(PERR_ID_REQ);
	}

    // now split the single list into 2 separate lists

    if (elemList != propList)
	{   // if got methods
	    lpEntry->dispinter.methList = (LPFUNC)elemList; // got methods
	    if (propList)
		{   // got both methods & properties -- split into 2 lists.
		    // Swap links in order to link last method to first method,
		    // last property to first property
		    elemTemp = elemList->pNext;
		    elemList->pNext = propList->pNext;
		    propList->pNext = elemTemp;
		}
	}

DoneDispinter:
    ConsumeRCurlyOptSemi(0);            // consume rcurly and optional ;
}

// parse the coclass section
VOID NEAR ParseCoclass
(
)
{

    LPENTRY	lpEntry = typlib.pEntry;
    LPTYPE	lpTypeBase;
    TENTRYKIND	tag;
    WORD	fGotDispinter = 0;
    WORD	fGotDefault = 0;
    WORD	flags;
#define BIT_NOTSOURCE 1	// flags for fGotDispinter and fGotDefault
#define BIT_SOURCE    2
    ATTR	attr;
    LPINTER	lpinter;
    LPINTER	lpinterList;

    // ensure attributes valid in this context
    CheckAttr(&lpEntry->attr, VALID_COCLASS_ATTR, VALID_COCLASS_ATTR2);

    ScanTok(0); 		// consume "coclass", advance to next tok

    // Since a function can now return a COCLASS, we must
    // be able to handle forward declarations to them.
    //
    if (FHandleForwardDecl(lpEntry, tCOCLASS))
	return; 			// if THIS is a forward decl

    // UUID required on coclass section
    if (!(lpEntry->attr.fAttr & fUUID))
	ParseError(PERR_UUID_REQ);

    lpEntry->type.inter.interList = NULL;	// no base interfaces

    EnsureNoDupEntries(lpEntry);	// check for dup def

    ConsumeTok(RW_LCURLY, 0);

    do {
	// parse a COCLASS entry
	ParseOptAttr(&attr, cIMPLTYPE);		// parse attributes, if any

        // ensure attributes valid in this context
        CheckAttrTokLast(&attr, VALID_COCLASS_INTER_ATTR,
			 VALID_COCLASS_INTER_ATTR2);

	flags = (WORD)((attr.fAttr & fSOURCE) ? BIT_SOURCE : BIT_NOTSOURCE);

	if (attr.fAttr & fDEFAULT) {
	    if (attr.fAttr & fRESTRICTED) {
		// default restricted makes no sense
		ParseErrorTokLast(PERR_INV_ATTR_COMBO);
	    }

   	    // figure out if we have too many default's or not.  We're allowed
	    // at most one [default] inter, and one [source, default] inter.
	    if (fGotDefault & flags) {
		ParseError(PERR_DUP_DEF);
	    }
	    fGotDefault |= flags;
	}

        switch (tok.id)
	    {
	    case RW_INTERFACE:
	        tag = tINTERFACE;
	        break;
	    case RW_DISPINTERFACE:
	        tag = tDISPINTER;

   	        // figure out if we have too many dispinterface's or not. 
		// We're allowed at most one non-[source] dispinterface, but
		// as many [source] dispinterfaces as we want.
	        if (fGotDispinter & flags == BIT_NOTSOURCE) {
		    // Error is caught at Layout(), but it's so crypitic I'm
		    // catching it at parse time instead.
		    ParseError(PERR_TWO_DISPINTER);
	        }
	        fGotDispinter |= flags;
	        break;
	    default:
	        ParseError(PERR_EXP_INTER);	// expected dispinterface or interface
	    }
        ScanTok(0);				// consume "interface"/"dispinterface" 

        lpTypeBase = ParseKnownType(&lpEntry->attr, fAllowInter);

        if ((lpTypeBase->tentrykind & ~(tFORWARD | tIMPORTED | tQUAL)) != tag)
	    ParseErrorTokLast(PERR_UNDEF_INTER);

	// insert referenced interface/dispinterface into list of interfaces
	ListInsert(&(lpEntry->type.inter.interList), sizeof(INTER));
	lpEntry->type.inter.interList->ptypeInter = lpTypeBase;
	lpEntry->type.inter.interList->fAttr = attr.fAttr;	// save flags
	lpEntry->type.inter.interList->fAttr2 = attr.fAttr2;

        ConsumeTok(RW_SEMI, 0);		// ends with a semicolon

    } while (tok.id != RW_RCURLY);	// while not end of list

    // If we didn't get interfaces with both source and non-source default
    // attributes, then we need to go through and set the default bit on the
    // first source and first non-source interfaces that aren't restricted.
    lpinterList = lpEntry->type.inter.interList;
    lpinter = (LPINTER)ListFirst(lpinterList);
    while (fGotDefault != (BIT_NOTSOURCE | BIT_SOURCE))
	{
	    flags = (WORD)((lpinter->fAttr & fSOURCE) ? BIT_SOURCE : BIT_NOTSOURCE);
            if (!(fGotDefault & flags) && !(lpinter->fAttr & fRESTRICTED)) {
	 	// if we don't have a default yet for this variety
		// (source/nonsource) of interface, and this interface isn't
		// marked as restricted, then tag it as default.
	        lpinter->fAttr |= fDEFAULT;
		fGotDefault |= flags;
	    }

	    // advance to next entry if not all done
	    if (lpinter == (LPINTER)ListLast(lpinterList))
		break;			// exit if all done
	    lpinter = (LPINTER)lpinter->pNext;
	} // WHILE

    ConsumeRCurlyOptSemi(0);		// consume rcurly and optional ;
}


// parses:  importlib (filename);
VOID NEAR ParseImportlib
(
)
{

    ScanTok(0);			// consume "importlib", advance to next token
    ConsumeTok(RW_LPAREN, fACCEPT_STRING);

    ListInsert(&typlib.pImpLib, sizeof(IMPORTLIB));

    typlib.pImpLib->lptlib = NULL;	// null out pointers in case of error
    typlib.pImpLib->lptcomp = NULL;
    typlib.pImpLib->lptlibattr = NULL;

    typlib.pImpLib->lpszFileName = lpszParseStringExpr();  // parse filename
    					  
    ConsumeTok(RW_RPAREN, 0);

    //now load the type library, and fill in pImpLib->lpszLibName/lptcomp.
    LoadExtTypeLib(typlib.pImpLib);

    ConsumeTok(RW_SEMI, 0);
}

// ****************************************
// Expression support
// ****************************************

LPSTR NEAR lpszParseStringExpr()
{
    LPSTR result;

    // string expressions are not supported -- only accept string literals

    if (tok.id != LIT_STRING)		// better be a string literal
	ParseError(PERR_EXP_STRING);
    if (tok.cbsz == 0)			// empty string not allowed here
	ParseError(PERR_INV_STRING);
    result = tok.lpsz;
    ScanTok(0);				// consume string token
    return result;
}

// supports (minimal) numeric expressions
// Current level of support is:
//	add
//	subtract
//	(numeric literals, parentheses, and unary minus are handled
//	 by lParseNumber)
DWORD NEAR lParseNumericExpr()
{
    DWORD result;
    TOKID idOp;
    DWORD nextNum;

    result = lParseNumber();
    while (tok.id >= OP_MIN && tok.id <= OP_MAX)
	{   // repeat while a binary operator follows this number
	    idOp = tok.id;		// save operator ID
	    ScanTok(fACCEPT_NUMBER);	// consume the operator

	    nextNum = lParseNumber();	// get number after this opcode

	    switch (idOp)
		{
		case OP_ADD:
		   result = result + nextNum;
		   break;

		case OP_SUB:
		   result = result - nextNum;
		   break;

		// CONSIDER: (V2, EXPR) Add more operators.
		// CONSIDER: (V2, EXPR) Probably have to completely re-write this
		// CONSIDER: (V2, EXPR) routine to deal with operator precidence.

		default:
		   // CONSIDER: should probably report error on the operator,
		   // CONSIDER: not on the 2nd number
		   ParseErrorTokLast(PERR_UNSUPPORTED_OP);
		}
	}

    return result;
}

// Parses a single number:
//	one numeric literal,
//	a unary minus followed by a single number
//	a numeric expression enclosed in parentheses
// Rejects anything that doesn't meet the above criteria.
DWORD NEAR lParseNumber()
{
    DWORD result;

    switch (tok.id)
	{
	case RW_LPAREN:
	    ScanTok(fACCEPT_NUMBER);    // consume the '('
	    result = lParseNumericExpr();	// return value of stuff
						// inside the parens
	    ConsumeTok(RW_RPAREN, fACCEPT_OPERATOR);
	    break;

	case RW_HYPHEN:		// treat as unary minus
	    // just assume the expression is signed at this point
	    ScanTok(fACCEPT_NUMBER);    // consume the '-'

	    // if negative numeric literal, ensure in range of signed I4
	    if (tok.id == LIT_NUMBER && tok.number > 0x80000000)
		ParseError(PERR_NUMBER_OV);

	    result = -(long)lParseNumber();
	    break;

	case LIT_NUMBER:
	    result = tok.number;
	    ScanTok(fACCEPT_OPERATOR);		// consume the number
	    break;

	//case LIT_STRING:
	default:
	    ParseError(PERR_INV_EXPRESSION);
	}
   return result;
}


// ****************************************
// Utility routines
// ****************************************

// Calls ParseError if these attributes are invalid in this context
VOID NEAR CheckAttr
(
    LPATTR pAttr,
    DWORD attrbit,
    DWORD attrbit2
)
{
    if (pAttr->fAttr & ~attrbit || pAttr->fAttr2 & ~attrbit2)
	ParseError(PERR_INV_ATTR);		// invalid in this context
}

// Calls ParseErrorTokLast if these attributes are invalid in this context
VOID NEAR CheckAttrTokLast
(
    LPATTR pAttr,
    DWORD attrbit,
    DWORD attrbit2
)
{
    if (pAttr->fAttr & ~attrbit || pAttr->fAttr2 & ~attrbit2)
	ParseErrorTokLast(PERR_INV_ATTR);	// invalid in this context
}

VOID NEAR GotAttr
(
    LPATTR pAttr,
    DWORD attrbit
)
{
    if (pAttr->fAttr & attrbit) // error if already specified
	ParseErrorTokLast(PERR_INV_ATTR_COMBO);
    pAttr->fAttr |= attrbit;
}

VOID NEAR GotAttr2
(
    LPATTR pAttr,
    DWORD attrbit2
)
{
    if (pAttr->fAttr2 & attrbit2) // error if already specified
	ParseErrorTokLast(PERR_INV_ATTR_COMBO);
    pAttr->fAttr2 |= attrbit2;
}

// consumes a close curly followed by an optional semicolon.
VOID NEAR ConsumeRCurlyOptSemi
(
    WORD fAccept
)
{
    ConsumeTok(RW_RCURLY, fAccept);     // consume the close curly
    if (tok.id == RW_SEMI)
       ScanTok(fAccept);                // consume the ';' if present
}


// consumes an identifer token.
LPSTR NEAR ConsumeId()
{
    LPSTR lpszRet;

    if (tok.id != LIT_ID)		// ensure we've got an ID
	ParseError(PERR_EXP_IDENTIFIER); //    error if not
    lpszRet = tok.lpsz;			// save id name for return value
    ScanTok(0);				// consume ID token

    return lpszRet;			// return id name
}


// ensure last element in entry list isn't duplicated
// Also ensures that the uuid defined by this entry (if specified) isn't
// specified by somebody else.
VOID NEAR EnsureNoDupEntries
(
    LPENTRY lpEntryLast
)
{
    LPENTRY lpEntry;
    LPSTR   szNameLast;
    LPSTR   szTagLast;
    GUID FAR * lpUuidLast;

    szNameLast = lpEntryLast->type.szName;	// name of last entry added

    lpUuidLast = NULL;		// assume no UUID
    szTagLast = NULL;		// assume no tag

    // set up szTagLast and lpUuidLast, and ensure that the last entry
    // doesn't conflict with the 'library' section.
    switch (lpEntryLast->type.tentrykind & ~tFORWARD)
    {
	case tINTRINSIC:
	case tREF:
	   break;			// no attr field on these guys

	case tSTRUCT:
	case tENUM:
	case tUNION:
	    szTagLast = lpEntryLast->type.structenum.szTag;	// NULL if none
	    // fall into default processing

	default:
	    if (lpEntryLast->type.tentrykind & tIMPORTED)
		break;			// no attr field on these guys

	    if (lpEntryLast->attr.fAttr & fUUID)	// if UUID specified
		{
		    lpUuidLast = lpEntryLast->attr.lpUuid;	// get * to uuid
		    // ensure this UUID isn't duplicated with the library's uuid
		    if ((typlib.attr.fAttr & fUUID) &&
		       !_fmemcmp(lpUuidLast, typlib.attr.lpUuid, sizeof(GUID)))
       	           ParseErrorTokLast(PERR_DUP_UUID);
		}
    };

    // ensure it's not duplicated by any other entry name/uuid
    for (lpEntry = (LPENTRY)ListFirst(typlib.pEntry); lpEntry != lpEntryLast; lpEntry = (LPENTRY)(lpEntry->type.pNext))
        {
	    if (lpEntry->type.tentrykind & tIMPORTED) {
		// The following check ensures that you can't have a
		// type definition of the same name as an unqualified reference
		// to a type in another library.  While things will work, it's
		// a bit confusing if two id's in the same file refer to
		// to different types just because one comes before the
		// definition of a type.
		// Qualifed references to types in other type libraries don't
		// present any ambiguity.
		if (!(lpEntry->type.tentrykind & tQUAL)) {
		    // ensure name isn't duplicated (CASE-INSENSITIVE)
	            if (!FCmpCaseIns(szNameLast, lpEntry->type.szName))
		        ParseErrorTokLast(PERR_DUP_DEF);	// then error
		}
	    }
	    else {
	    switch (lpEntry->type.tentrykind & ~tFORWARD)
	    {
		case tREF:
		    break;		// no conflicts possible with these guys

		case tINTRINSIC:
		    // ensure intrinsic name isn't duplicated (CASE-SENSITIVE)
	            if (!_fstrcmp(szNameLast, lpEntry->type.szName))
	                ParseErrorTokLast(PERR_DUP_DEF);
		    break;

		case tSTRUCT:
		case tENUM:
		case tUNION:
		    // ensure tag (if given) isn't duplicated (CASE-SENSITIVE)
		    if (szTagLast && lpEntry->type.structenum.szTag &&
	                !_fstrcmp(szTagLast, lpEntry->type.structenum.szTag))
			CheckForwardMatch(lpEntryLast, lpEntry);

		    if (lpEntry->type.tentrykind & tFORWARD)
			break;		// forward declares of these don't have
					// their names filled in yet

		    // fall into default processing to check names

		default:
		    // ensure name isn't duplicated (CASE-INSENSITIVE)
	            if (!FCmpCaseIns(szNameLast, lpEntry->type.szName))
			CheckForwardMatch(lpEntryLast, lpEntry);

		    // ensure this UUID isn't duplicated
		    if (lpUuidLast && (lpEntry->attr.fAttr & fUUID) &&
		        !_fmemcmp(lpUuidLast, lpEntry->attr.lpUuid, sizeof(GUID)))
			ParseErrorTokLast(PERR_DUP_UUID);
	    }
	    }
        }
}


// called when 2 entries have the same name -- ensure these are valid
// combinations of forward declares.   If not -- then dup def error.
VOID NEAR CheckForwardMatch
(
   LPENTRY lpEntryLast,
   LPENTRY lpEntry
)
{
    // if one of these isn't a forward declare
    if ((((lpEntryLast->type.tentrykind | lpEntry->type.tentrykind) & tFORWARD) == 0) ||

    // or if the type's of these forward declares don't match
        ((lpEntryLast->type.tentrykind & ~tFORWARD) != (lpEntry->type.tentrykind & ~tFORWARD)) )

	ParseErrorTokLast(PERR_DUP_DEF);	// then error
}


INT FAR FCmpCaseIns
(
    LPSTR str1,
    LPSTR str2
)
{
    return !(CompareStringA(g_lcidCompare,
                            NORM_IGNOREWIDTH |
                            NORM_IGNOREKANATYPE |
                            NORM_IGNORECASE, 
                            str1,
                            -1,
                            str2,
                            -1)
             == 2); 

}

// *************************************************************************
// Linked list management
// *************************************************************************

// assumes all lists have a pNext field in the same position (usually first)
// This routine will go to ParseError if it runs out of memory
VOID FAR ListInsert
(
    LPVOID ppList,
    WORD cbElem
)
{
    LPTYPE pNewItem;	// any type that has a pNext -- they all have
    LPTYPE pList;		// them in the same positions

    pNewItem = (LPTYPE)ParseMalloc(cbElem);	// new last item
    pList = *(LPTYPE FAR *)ppList;		// deref

    if (pList)			// if list not empty
   	{
	    pNewItem->pNext = pList->pNext;	// set * to head of list
   	    pList->pNext = pNewItem;		// point to new last item
   	}
    else
        {
   	    pNewItem->pNext = pNewItem;		// point back to itself
      	}
    *(LPTYPE FAR *)ppList = pNewItem;	// point to last item
}



/***
* BOOL VerifyLcid(LCID lcid) - ripped out of silver
*
* Purpose: Checks if the passed in lcid is valid.
*
* Inputs:
*   lcid :  LCID that needs to be verified.
*
* Outputs: BOOL :  return TRUE if the passed in lcid is a valid LCID
*          else return FALSE
*
*****************************************************************************/
BOOL NEAR VerifyLcid(LCID lcid)
{
    BOOL fValidLcid;
    INT i;

    // Call the nlsapi function to compare string.  If the compare
    // succeeds then the LCID is valid or else the passed in lcid is
    // invalid.  This is because the only reason the comparision will
    // fail is f the lcid is invalid.

    char rgTest[] = "Test\0";

    fValidLcid = (BOOL) (CompareStringA(lcid, NORM_IGNORECASE | NORM_IGNORENONSPACE,
               rgTest, -1, rgTest, -1)== 2);

    if (fValidLcid) {
	// if DBCS lcid, update lead byte table appropriately

#if defined(WIN32)
	UINT CodePage;
	CHAR szCodePage[6];	// space for an ascii integer

	// Attempt to use IsDBCSLeadByteEx API to compute this.  If this doesn't
	// work, fall into the hardcoded code, to take our best shot at it.
	if (GetLocaleInfoA(lcid,
			   LOCALE_NOUSEROVERRIDE | LOCALE_IDEFAULTANSICODEPAGE,
			   szCodePage,
			   sizeof(szCodePage)) != 0) {
	    CodePage = atoi(szCodePage);

            // start at 128 since there aren't any lead bytes before that
            for(i = 128; i < 256; i++) {
                g_rgchLeadBytes[i] = (char)IsDBCSLeadByteEx(CodePage, (char)i);
            }
	    goto Done;
	}
#endif 	//WIN32
	switch (PRIMARYLANGID(lcid)) {
	case LANG_CHINESE:
	   if (SUBLANGID(lcid) == SUBLANG_CHINESE_TRADITIONAL) {
	      g_rgchLeadBytes[0x80] = 0;
	      for (i = 0x81; i <= 0xFE; i++) {
	        g_rgchLeadBytes[i] = 1;
	      }
	      g_rgchLeadBytes[0xFF] = 0;
	      break;
	   }
	   // fall into LANG_KOREAN for SUBLANG_CHINESE_SIMPLIFIED
	case LANG_KOREAN:
	      for (i = 0x80; i <= 0xA0; i++) {
	        g_rgchLeadBytes[i] = 0;
	      }
	      for (i = 0xA1; i <= 0xFE; i++) {
	        g_rgchLeadBytes[i] = 1;
	      }
	      g_rgchLeadBytes[0xFF] = 0;
	      break;
	   break;
	case LANG_JAPANESE:
	      memset(g_rgchLeadBytes+128, 0, 128);
	      for (i = 0x81; i <= 0x9F; i++) {
	        g_rgchLeadBytes[i] = 1;
	      }
	      for (i = 0xE0; i <= 0xFC; i++) {
	        g_rgchLeadBytes[i] = 1;
	      }
	      break;
	   break;
	default:
	   // not a DBCS LCID
	   memset(g_rgchLeadBytes+128, 0, 128);
	   break;
	}
#if defined(WIN32)
Done:	;
#endif 	//WIN32
    }

    return fValidLcid;
}


VOID NEAR FindIDispatch()
{
    char * szIDispatch = "IDispatch";
    char * szIUnknown = "IUnknown";

    // find existing type of this name with 0 levels of indirection
    lpTypeIDispatch = FindType(szIDispatch, FALSE, tANY);
	
    if (lpTypeIDispatch == NULL) {
        // if not found here, then search all external type libraries for
        // this type definition.
        lpTypeIDispatch = FindExtType(NULL, szIDispatch);
    }
    if (lpTypeIDispatch == NULL) {
	ParseError(PERR_NO_IDISPATCH);
    }

    // now do the same for IUnknown
    // find existing type of this name with 0 levels of indirection
    lpTypeIUnknown = FindType(szIUnknown, FALSE, tANY);
	
    if (lpTypeIUnknown == NULL) {
        // if not found here, then search all external type libraries for
        // this type definition.
        lpTypeIUnknown = FindExtType(NULL, szIUnknown);
    }
    if (lpTypeIUnknown == NULL) {
	ParseError(PERR_NO_IUNKNOWN);
    }
}
