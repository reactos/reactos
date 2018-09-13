#include	"mktyplib.h"

#include	<malloc.h>
#include	<stdio.h>

#ifndef WIN32
#include	<ole2.h>
#include	"dispatch.h"
#endif //!WIN32

#include	"errors.h"
#include	"parser.h"

#include	"fileinfo.h"
#include	"intlstr.h"

#if !FV_UNICODE_OLE
#define LHashValOfNameSysA	LHashValOfNameSys
#endif //FV_UNICODE_OLE


extern "C" {

// external C data
extern TYPLIB typlib;		// structure that holds all file data
extern SYSKIND SysKind;
extern int iAlignMax;
extern WORD cArgsMax;		// max # of args in any function
extern SCODE scodeErrCur;	// SCODE of current error


// external C routines
extern INT FAR FCmpCaseIns(LPSTR str1, LPSTR str2); 
extern LPTYPE FAR lpTypePublic(LPTYPE lpType);

// local data
ELEMDESC FAR* rgFuncArgs;	// array of function args
LPOLESTR FAR* rgszFuncArgNames;	// array of function arg names

LPSTR lpszItemCur;		// current item we're working on

// C prototypes
VOID FAR OutputTyplib(CHAR * szTypeLibFile);
VOID FAR LoadExtTypeLib(LPIMPORTLIB lpImpLib);
LPTYPE FAR FindExeType(LPSTR lpszTypeName);
LPTYPE FAR FindExeTypeInLib(LPSTR lpszLibName, LPSTR lpszTypeName);

VOID NEAR OutputTypeInfos(ICreateTypeLib FAR * lpdtlib);
VOID NEAR OutputFuncs(ICreateTypeInfo FAR* lpdtinfo, LPENTRY pEntry, LPFUNC pFuncList, TYPEKIND tkind);
VOID NEAR OutputElems(ICreateTypeInfo FAR* lpdtinfo, LPELEM pPropList, TYPEKIND tkind);
VOID NEAR OutputAlias(ICreateTypeInfo FAR* lpdtinfo, LPTYPE lpTypeAlias);
VOID NEAR LoadElemDesc(ICreateTypeInfo FAR* lpdtinfo, ELEMDESC FAR* lpElemDesc, LPELEM pElem);
VOID NEAR UpdateHRefType(ICreateTypeInfo FAR* lpdtinfo, LPTYPE lpTypeBase);
VOID NEAR OutputInterface(ICreateTypeInfo FAR* lpdtinfo, LPENTRY pEntry);
VOID NEAR OutputDispinter(ICreateTypeInfo FAR* lpdtinfo, LPENTRY pEntry);
VOID NEAR OutputClass(ICreateTypeInfo FAR* lpdtinfo, LPENTRY pEntry);
VOID NEAR OutputBaseInterfaces(ICreateTypeInfo FAR* lpdtinfo, LPENTRY pEntry, TYPEKIND tkind);
VOID NEAR MethodError(HRESULT res);
    

// error reporting macros
#define	CHECKRESULT(x) if (FAILED((res = (x))) != 0) MethodError(res);
#define SETITEMCUR(x) lpszItemCur = x;
BOOL fDoingOutput = FALSE;		// set to TRUE once we start
					// writing the type library

#if FV_UNICODE_OLE
// Unicode conversion routines

#define CW_BUFFER 1024          // 1K ought to do it
OLECHAR szBufW[CW_BUFFER];      // handy static buffer

OLECHAR FAR * ToW(char FAR* lpszA)
{
    UINT cb;

    cb = strlen(lpszA)+1;
    Assert(cb <= CW_BUFFER);

    SideAssert (MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    lpszA,
				    cb,
				    szBufW,
				    cb) != 0);

    return szBufW;
}

OLECHAR FAR * ToNewW(char FAR* lpszA)
{
    OLECHAR FAR * szW;
    UINT    cb;

    cb = strlen(lpszA)+1;
    szW = (LPOLESTR)_fmalloc(cb * sizeof(OLECHAR));

    SideAssert (MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    lpszA,
				    cb,
				    szW,
				    cb) != 0);

    return szW;
}

char FAR * ToA(OLECHAR FAR* lpszW)
{
    SideAssert (WideCharToMultiByte(CP_ACP,
				    0,
				    lpszW,
				    -1,
				    (char FAR *)szBufW,
				    CW_BUFFER,
                    NULL,
                    NULL) != 0);

    return (CHAR *)szBufW;
}
#else //FV_UNICODE_OLE

// Non-unicode routines are a NOP
#define ToW(lpszA) lpszA
#define ToNewW(lpszA) lpszA
#define ToA(lpszW) lpszW

#endif //FV_UNICODE_OLE


VOID FAR OutputTyplib
(
  CHAR * szTypeLibFile
)
{
    HRESULT res;
    ICreateTypeLib FAR * lpdtlib;
#ifndef MAC
    CHAR szTypeLibFileTmp[128];
#endif //!MAC
    WORD wLibFlags;

    fDoingOutput = TRUE;		// signal to error reporting code
    SETITEMCUR(typlib.szLibName);

#ifndef MAC
    // pre-pend ".\" to filename if unqualified name, to work-around OLE path
    // searching bug.

    if (!XStrChr(szTypeLibFile, '\\') && !XStrChr(szTypeLibFile, '/') &&
	 *(szTypeLibFile+1) != ':')
	{	// if unqualified name
	   strcpy(szTypeLibFileTmp, ".\\");
	   strcpy(szTypeLibFileTmp+2, szTypeLibFile);
	   // this name is used later -- don't free it
	   //free(szTypeLibFile);
	   szTypeLibFile = szTypeLibFileTmp;
	}

#endif

    // 1. Get * to ICreateTypeLib interface
    CHECKRESULT(CreateTypeLib(SysKind, ToW(szTypeLibFile), &lpdtlib));

    // WARNING: SetLCID must be called before the anything in the nammgr is used
    // it needs to be called even if the user doesn't specifiy an lcid
    // (in which case we default to an lcid of 0).
    CHECKRESULT(lpdtlib->SetLcid(
       (typlib.attr.fAttr & fLCID) ? typlib.attr.lLcid : 0
	));

    // 2. Set the library name:
    CHECKRESULT(lpdtlib->SetName(ToW(typlib.szLibName)));
    
    // 3. Set the library attributes, if present:
    if (typlib.attr.fAttr & fHELPSTRING)
	CHECKRESULT(lpdtlib->SetDocString(ToW(typlib.attr.lpszHelpString)));

    if (typlib.attr.fAttr & fHELPCONTEXT)
	CHECKRESULT(lpdtlib->SetHelpContext(typlib.attr.lHelpContext));

    if (typlib.attr.fAttr & fHELPFILE)
	CHECKRESULT(lpdtlib->SetHelpFileName(ToW(typlib.attr.lpszHelpFile)));

    if (typlib.attr.fAttr & fVERSION)
	CHECKRESULT(lpdtlib->SetVersion(typlib.attr.wVerMajor, typlib.attr.wVerMinor));

    if (typlib.attr.fAttr & fUUID)
	CHECKRESULT(lpdtlib->SetGuid(*typlib.attr.lpUuid));

    wLibFlags = 0;
    if (typlib.attr.fAttr & fRESTRICTED)
        wLibFlags |= LIBFLAG_FRESTRICTED;
    if (typlib.attr.fAttr2 & f2CONTROL)
        wLibFlags |= LIBFLAG_FCONTROL;
    if (typlib.attr.fAttr & fHIDDEN)
        wLibFlags |= LIBFLAG_FHIDDEN;
    CHECKRESULT(lpdtlib->SetLibFlags(wLibFlags));

    // 4. Output all the typinfo's into the type library
    if (typlib.pEntry)			// if any entries
        OutputTypeInfos(lpdtlib);

    // 5. Save the typlib to the give file
    SETITEMCUR(typlib.szLibName);

    CHECKRESULT(lpdtlib->SaveAllChanges());

    // 6. Cleanup.  All done -- release release the ICreateTypeLib.
    lpdtlib->Release();		// release the ICreateTypeLib

}


// WARNING: must be kept in ssync with TENTRYKIND enum in FILEINFO.H
TYPEKIND rgtkind[] = {
    TKIND_ALIAS,	// tTYPEDEF
    TKIND_RECORD,	// tSTRUCT
    TKIND_ENUM,		// tENUM
    TKIND_UNION,	// tUNION
    TKIND_MODULE,	// tMODULE
    TKIND_INTERFACE,	// tINTERFACE
    TKIND_DISPATCH,	// tDISPINTER
    TKIND_COCLASS,	// tCOCLASS
//    TKIND_xxx,	// tINTRINISIC
//    TKIND_xxx,	// tREF
//    TKIND_xxx,	// tANY
};

// WARNING: must be kept in ssync with TYPEKIND enum in DISPATCH.H
TENTRYKIND rgtentrykind[] = {
    tENUM,		// TKIND_ENUM
    tSTRUCT,		// TKIND_RECORD
    tMODULE,		// TKIND_MODULE
    tINTERFACE,		// TKIND_INTERFACE
    tDISPINTER,		// TKIND_DISPATCH
    tCOCLASS,		// TKIND_COCLASS
    tTYPEDEF,		// TKIND_ALIAS
    tUNION,		// TKIND_UNION
};

// For each library entry, creates a typeinfo in the typelib,
// and fills it in.
VOID NEAR OutputTypeInfos
(
    ICreateTypeLib FAR * lpdtlib
)
{
    LPENTRY	pEntry;
    TYPEKIND	tkind;
    HRESULT	res;
    ICreateTypeInfo FAR* lpdtinfo;
    WORD	wTypeFlags;

    // First allocate an array of ELEMDESCs to hold the max # of
    // args of any function we need to describe.
    rgFuncArgs = (ELEMDESC FAR*)_fmalloc(cArgsMax * sizeof(ELEMDESC));
    rgszFuncArgNames = (LPOLESTR FAR *)_fmalloc((cArgsMax+1) * sizeof(LPOLESTR));
    if (rgFuncArgs == NULL || rgszFuncArgNames == NULL)
	ParseError(ERR_OM);

    // pass 1 -- create the typeinfo's
    pEntry = (LPENTRY)ListFirst(typlib.pEntry);	// point to first entry
    for (;;)
	{

	    // determine if we are going to create a typeinfo for this guy
	    switch (pEntry->type.tentrykind)
		{
		case tTYPEDEF:
		    if (pEntry->attr.fAttr)	// create lpdtinfo only if
			break;			// this is a 'PUBLIC' typedef

NoTypeInfos:
		    pEntry->lpdtinfo = NULL;	// no lpdtinfo for this

		case tREF:			// no typeinfo's made for these
		case tINTRINSIC:
		    goto Next_Entry1;

		default:
		    // no typeinfo's made for forward declarations
		    if (pEntry->type.tentrykind & tFORWARD)
			goto NoTypeInfos;
		    if (pEntry->type.tentrykind & tIMPORTED)
			goto Next_Entry1;	// nothing for imported types
		    break;
		}

	    // 1. determine kind of typeinfo to create
	    tkind = rgtkind[pEntry->type.tentrykind];

	    // 2. Create type library entry (TypeInfo) of a given name/and
	    //    type, getting a pointer to the ICreateTypeInfo interface
	    SETITEMCUR(pEntry->type.szName);
	    CHECKRESULT(lpdtlib->CreateTypeInfo(ToW(pEntry->type.szName), tkind, &lpdtinfo));
	    pEntry->lpdtinfo = lpdtinfo;

	    // 3. Set the item's attributes:
	    if (pEntry->attr.fAttr & fUUID)
		CHECKRESULT(lpdtinfo->SetGuid(*pEntry->attr.lpUuid));

	    if (pEntry->attr.fAttr & fHELPSTRING)
		CHECKRESULT(lpdtinfo->SetDocString(ToW(pEntry->attr.lpszHelpString)));

	    if (pEntry->attr.fAttr & fHELPCONTEXT)
		CHECKRESULT(lpdtinfo->SetHelpContext(pEntry->attr.lHelpContext));

	    if (pEntry->attr.fAttr & fVERSION)
		CHECKRESULT(lpdtinfo->SetVersion(pEntry->attr.wVerMajor, pEntry->attr.wVerMinor));

	    // 4. save lptinfo for this guy in case somebody references it
	    CHECKRESULT(lpdtinfo->QueryInterface(IID_ITypeInfo, (VOID FAR* FAR*)&pEntry->type.lptinfo));

	    // if this type has a forward declaration
	    if (pEntry->lpEntryForward)
		{
		    // copy "real" type info over top of forward declaration,
		    // because folks have pointers to the forward declaration
		    pEntry->lpEntryForward->type.tdesc = pEntry->type.tdesc;
		    pEntry->lpEntryForward->type.lptinfo = pEntry->type.lptinfo;
		    // Only need to copy these 2 fields from the type (the
		    // others aren't referenced at type creation time.
		}

Next_Entry1:
	    // advance to next entry if not all done
	    if (pEntry == (LPENTRY)ListLast(typlib.pEntry))
		break;			// exit if all done
	    pEntry = (LPENTRY)pEntry->type.pNext;
	}


    // pass 2 -- process each entry
    pEntry = (LPENTRY)ListFirst(typlib.pEntry);	// point to first entry
    for (;;)
	{

	    // 1.  Get our lpdtinfo again if we have one

	    switch (pEntry->type.tentrykind)
		{
		case tREF:
		case tINTRINSIC:
		    goto Next_Entry2;	// these guys don't have lpdtinfo field
		default:
		    if (pEntry->type.tentrykind & tIMPORTED)
			goto Next_Entry2;	// no lpdtinfo field
		    break;
		}

	    lpdtinfo = pEntry->lpdtinfo;
	    if (lpdtinfo == NULL)	// skip if no lpdtinfo created
		goto Next_Entry2;	// (forward decl or non-public typedef)

	    // set up for error reporting
	    SETITEMCUR(pEntry->type.szName);

	    // 2. determine kind of typeinfo we're dealing with
	    tkind = rgtkind[pEntry->type.tentrykind];

	    // 2a. Set the typeinfo flags
	    wTypeFlags = 0;
	    if (tkind == TKIND_COCLASS) {
		// COCLASSs always have FCANCREATE bit set
		wTypeFlags |= TYPEFLAG_FCANCREATE;
		// these are only valid on COCLASSs
	        if (pEntry->attr.fAttr & fAPPOBJECT)
		    wTypeFlags |= (WORD)TYPEFLAG_FAPPOBJECT;
	        if (pEntry->attr.fAttr & fLICENSED)
		    wTypeFlags |= (WORD)TYPEFLAG_FLICENSED;
	        if (pEntry->attr.fAttr & fPREDECLID)
		    wTypeFlags |= (WORD)TYPEFLAG_FPREDECLID;
 	    }
	    if (pEntry->attr.fAttr & fHIDDEN)
		wTypeFlags |= (WORD)TYPEFLAG_FHIDDEN;
	    if (pEntry->attr.fAttr2 & f2CONTROL)
		wTypeFlags |= (WORD)TYPEFLAG_FCONTROL;
	    if (pEntry->attr.fAttr2 & f2NONEXTENSIBLE)
		wTypeFlags |= (WORD)TYPEFLAG_FNONEXTENSIBLE;
	    if (pEntry->attr.fAttr2 & f2DUAL) // DUAL implies OLEAUTOMATION
		wTypeFlags |= (WORD)(TYPEFLAG_FDUAL | TYPEFLAG_FOLEAUTOMATION);
	    if (pEntry->attr.fAttr2 & f2OLEAUTOMATION)
		wTypeFlags |= (WORD)TYPEFLAG_FOLEAUTOMATION;
	    CHECKRESULT(lpdtinfo->SetTypeFlags(wTypeFlags));

	    // 3. now process each kind of entry
	    switch (tkind)
		{
		case TKIND_ALIAS:
	
		    OutputAlias(lpdtinfo, pEntry->type.td.ptypeAlias);
		    break;

		case TKIND_RECORD: 	// struct's, enum's, and union's are
		case TKIND_ENUM:	// all very similar
		case TKIND_UNION:
		    OutputElems(lpdtinfo, pEntry->type.structenum.elemList, tkind);
		    break;

		case TKIND_MODULE:
		    OutputFuncs(lpdtinfo, pEntry, pEntry->module.funcList, tkind);
		    OutputElems(lpdtinfo, pEntry->module.constList, tkind);
		    break;

		case TKIND_INTERFACE:
		    OutputInterface(lpdtinfo, pEntry);
		    break;

		case TKIND_DISPATCH:
		    OutputDispinter(lpdtinfo, pEntry);
		    break;

		case TKIND_COCLASS:
		    OutputClass(lpdtinfo, pEntry);
		    break;

#if FV_PROPSET
		case TKIND_PROPSET:
		    // CONSIDER: (FV_PROPSET) do something with base_propset name
		    OutputElems(lpdtinfo, pEntry->propset.propList, tkind);
		    break;
#endif //FV_PROPSET
		default:
		    Assert(FALSE);
		};

            // 3a. Set the alignment for this TypeInfo.  Must be done before
	    // Layout().
            CHECKRESULT(lpdtinfo->SetAlignment(iAlignMax));

	    // 4. Compile this typeinfo we've just created.
	    SETITEMCUR(pEntry->type.szName);
	    CHECKRESULT(lpdtinfo->LayOut());

	    // 5. Cleanup.  All done with lpdtinfo.
	    lpdtinfo->Release();

Next_Entry2:
	    // advance to next entry if not all done
	    if (pEntry == (LPENTRY)ListLast(typlib.pEntry))
		break;			// exit if all done
	    pEntry = (LPENTRY)pEntry->type.pNext;
	}


    // now clean up everything else
    _ffree(rgFuncArgs);	 	// done with function args we allocated
    _ffree(rgszFuncArgNames);

}

VOID NEAR OutputFuncs
(
    ICreateTypeInfo FAR* lpdtinfo,
    LPENTRY	pEntry,
    LPFUNC	pFuncList,
    TYPEKIND	tkind
)
{

    LPFUNC	pFunc;
    LPELEM	pArg;
    HRESULT	res;
    UINT	iFunc = 0;		// function index
    LPOLESTR	lpszDllName;
    LPSTR	lpszProcName;
    SHORT	i;
    FUNCDESC	FuncDesc;
    LPOLESTR FAR*  lplpszArgName;

    if (pFuncList == NULL)		// just return if no functions to output
	return;

    FuncDesc.lprgelemdescParam = rgFuncArgs;	// set up array of args

    pFunc = (LPFUNC)ListFirst(pFuncList);	// point to first entry
#if FV_UNICODE_OLE
    lpszDllName = (pEntry->attr.fAttr & fDLLNAME ? ToNewW(pEntry->attr.lpszDllName) : NULL);
#else
    lpszDllName = pEntry->attr.lpszDllName;
#endif

    for (;;)
	{
	    // Fill in the FUNCDESC structure with the function's info

	    // set up funckind
	    switch (tkind) {
		case TKIND_MODULE:
		    FuncDesc.funckind = FUNC_STATIC;
		    break;
		case TKIND_INTERFACE:
		    FuncDesc.funckind = FUNC_PUREVIRTUAL;
		    break;
		case TKIND_DISPATCH:
		    FuncDesc.funckind = FUNC_DISPATCH;
		    break;
		default:
		    Assert(FALSE);
	    }

	    // set up invkind
	    FuncDesc.invkind = INVOKE_FUNC;	// default
	    if (pFunc->func.attr.fAttr & fPROPGET)
	        FuncDesc.invkind = INVOKE_PROPERTYGET;
	    else if (pFunc->func.attr.fAttr & fPROPPUT)
	        FuncDesc.invkind = INVOKE_PROPERTYPUT;
	    else if (pFunc->func.attr.fAttr & fPROPPUTREF)
	        FuncDesc.invkind = INVOKE_PROPERTYPUTREF;

	    // set up callconv
	    if (pFunc->func.attr.fAttr2 & f2PASCAL)
		// CC_MACPASCAL if /mac specified, CC_MSCPASCAL otherwise
	        FuncDesc.callconv = (CALLCONV)(SysKind == SYS_MAC ? CC_MACPASCAL: CC_MSCPASCAL);
	    else if (pFunc->func.attr.fAttr2 & f2CDECL)
	        FuncDesc.callconv = CC_CDECL;
	    else if (pFunc->func.attr.fAttr2 & f2STDCALL)
	        FuncDesc.callconv = CC_STDCALL;
#ifdef	DEBUG
	    else Assert(FALSE);
#endif	//DEBUG

	    FuncDesc.wFuncFlags = 0;
	    if (pFunc->func.attr.fAttr & fRESTRICTED)
	        FuncDesc.wFuncFlags |= (WORD)FUNCFLAG_FRESTRICTED;
	    if (pFunc->func.attr.fAttr & fSOURCE)
	        FuncDesc.wFuncFlags |= (WORD)FUNCFLAG_FSOURCE;
	    if (pFunc->func.attr.fAttr & fBINDABLE)
	        FuncDesc.wFuncFlags |= (WORD)FUNCFLAG_FBINDABLE;
	    if (pFunc->func.attr.fAttr & fREQUESTEDIT)
	        FuncDesc.wFuncFlags |= (WORD)FUNCFLAG_FREQUESTEDIT;
	    if (pFunc->func.attr.fAttr & fDISPLAYBIND)
	        FuncDesc.wFuncFlags |= (WORD)FUNCFLAG_FDISPLAYBIND;
	    if (pFunc->func.attr.fAttr & fDEFAULTBIND)
	        FuncDesc.wFuncFlags |= (WORD)FUNCFLAG_FDEFAULTBIND;
	    if (pFunc->func.attr.fAttr & fHIDDEN)
	        FuncDesc.wFuncFlags |= (WORD)FUNCFLAG_FHIDDEN;

	    // set up cParams & cParamsOpt
	    FuncDesc.cParams = pFunc->cArgs;
	    FuncDesc.cParamsOpt = pFunc->cOptArgs;
	    //NOTE: cParamsOpt can be set to -1, to note that last parm is a
	    //NOTE: [safe] array of variant args.  This corresponds to the
	    //NOTE: 'vararg' attribute in the input description.  If this was
	    //NOTE: specified, the parser set pFunc->cOptArgs to -1 for us.
 
	    // set up misc unused stuff
	    FuncDesc.oVft = 0;		// only used for FUNC_VIRTUAL
	    FuncDesc.cScodes = -1;	// list of SCODEs unknown
	    FuncDesc.lprgscode = NULL;

	    // set id field if 'id' attribute specified
	    if (pFunc->func.attr.fAttr & fID)
	        FuncDesc.memid = pFunc->func.attr.lId;
	    else
	        FuncDesc.memid = DISPID_UNKNOWN;

	    // set up elemdescFunc
	    // Contains the ID, name, and return type of the function
	    LoadElemDesc(lpdtinfo, &(FuncDesc.elemdescFunc), &pFunc->func);

	    // save function name
	    lplpszArgName = rgszFuncArgNames;
	    *lplpszArgName++ = ToNewW(pFunc->func.szElemName);
	    SETITEMCUR(pFunc->func.szElemName);

	    // set up the lprgelemdescParam array of info for each parameter
	    pArg = pFunc->argList;	// point to last arg, if any
	    for (i = 0; i < pFunc->cArgs; i++)
		{
		    pArg = pArg->pNext;		// point to next arg (first arg
						// first time through loop)
		    LoadElemDesc(lpdtinfo, &(FuncDesc.lprgelemdescParam[i]), pArg);
		    *lplpszArgName++ = ToNewW(pArg->szElemName); // save arg name
		}

	    // Define the function item:
	    CHECKRESULT(lpdtinfo->AddFuncDesc(iFunc, &FuncDesc));

	    // set the names of the function and the parameters
	    Assert(i == pFunc->cArgs);

	    // don't set the name of the last param for proput/putref functions
	    if (pFunc->func.attr.fAttr & (fPROPPUT | fPROPPUTREF)) {
		i--;
	    }

	    CHECKRESULT(lpdtinfo->SetFuncAndParamNames(iFunc, rgszFuncArgNames, i+1));
#if FV_UNICODE_OLE
            // free the unicode function & param names
	    lplpszArgName = rgszFuncArgNames;
	    for (i = 0; i <= pFunc->cArgs; i++) {
              _ffree(*lplpszArgName);	 	// done with unicode name
	      lplpszArgName++;
            }
#endif //FV_UNICODE_OLE

	    // Set the function item's remaining attributes:
	    if (pFunc->func.attr.fAttr & fHELPSTRING)
		CHECKRESULT(lpdtinfo->SetFuncDocString(iFunc, ToW(pFunc->func.attr.lpszHelpString)));
	    if (pFunc->func.attr.fAttr & fHELPCONTEXT)
		CHECKRESULT(lpdtinfo->SetFuncHelpContext(iFunc, pFunc->func.attr.lHelpContext));
	    // Handle case of a DLL entrypoint
	    if (pFunc->func.attr.fAttr & fENTRY)
		{
		    // If high word of name is zero, then call by ordnal
		    // If high word of name is non-zero, then call by name
		    // (same as GetProcAddress)

		    lpszProcName = pFunc->func.attr.lpszProcName;
#if FV_UNICODE_OLE
		    if (HIWORD(lpszProcName)) {
                        lpszProcName = (LPSTR)ToW(lpszProcName);
		    }
#endif //FV_UNICODE_OLE
		    CHECKRESULT(lpdtinfo->DefineFuncAsDllEntry(iFunc, lpszDllName, (LPOLESTR)lpszProcName));
		}

	    // advance to next entry if not all done
	    if (pFunc == (LPFUNC)ListLast(pFuncList))
		break;			// exit if all done
	    pFunc = (LPFUNC)pFunc->func.pNext;
	    iFunc++;			// advance function counter
	}
#if FV_UNICODE_OLE
    if (lpszDllName)
        _ffree(lpszDllName);	 	// done with unicode name
#endif //FV_UNICODE_OLE
}

VOID NEAR OutputElems
(
    ICreateTypeInfo FAR* lpdtinfo,
    LPELEM	pElemList,
    TYPEKIND	tkind
)
{

    LPELEM	pElem;
    HRESULT	res;
    UINT	iElem = 0;		// element index
    VARDESC	VarDesc;


    if (pElemList == NULL)
	return;

    pElem = (LPELEM)ListFirst(pElemList);	// point to first entry

    for (;;)
	{

	    // First fill in the VARDESC structure with the element's info
	    SETITEMCUR(pElem->szElemName);

	    VarDesc.memid = DISPID_UNKNOWN;	// assume not a dispinterface

	    // Set up varkind
	    switch (tkind)
		{
		case TKIND_ENUM:		// for enum elements
		case TKIND_MODULE:		// for const's
		    VarDesc.varkind =  VAR_CONST;
		    VarDesc.lpvarValue = pElem->lpElemVal;	// * to value
		    if (tkind == TKIND_MODULE)
		        goto DoLoadElemDesc;		// set up the tdesc

		    // For ENUM elements, can't call LoadElemDesc, because
		    // we have no pElem->elemType.  Do the required work here.

		    VarDesc.elemdescVar.tdesc.vt = VT_INT;	// element type
								// is an INT
		    VarDesc.elemdescVar.idldesc.wIDLFlags = 0;	// no IDL info
#ifdef WIN16
		    VarDesc.elemdescVar.idldesc.bstrIDLInfo = NULL;
#else //WIN16
		    VarDesc.elemdescVar.idldesc.dwReserved = 0;
#endif //WIN16
		    break;

		case TKIND_RECORD:
		case TKIND_UNION:
		    VarDesc.varkind =  VAR_PERINSTANCE;
		    goto DoLoadElemDesc;

		default:
#if FV_PROPSET
		    Assert(tkind == TKIND_DISPATCH || tkind == TKIND_PROPSET);
#else //FV_PROPSET
		    Assert(tkind == TKIND_DISPATCH);
#endif //FV_PROPSET
		    VarDesc.varkind =  VAR_DISPATCH;

		    // id' attribute required
		    Assert (pElem->attr.fAttr & fID);
		    VarDesc.memid = pElem->attr.lId;

DoLoadElemDesc:
		    // Set up elemdescVar.  Contains name, and type of item.
		    LoadElemDesc(lpdtinfo, &(VarDesc.elemdescVar), pElem);
		}

	    // VarDesc.oInst is not used when doing AddVarDesc
	    VarDesc.wVarFlags = 0;
	    if (pElem->attr.fAttr & fREADONLY)
	        VarDesc.wVarFlags |= (WORD)VARFLAG_FREADONLY;
	    if (pElem->attr.fAttr & fSOURCE)
	        VarDesc.wVarFlags |= (WORD)VARFLAG_FSOURCE;
	    if (pElem->attr.fAttr & fBINDABLE)
	        VarDesc.wVarFlags |= (WORD)VARFLAG_FBINDABLE;
	    if (pElem->attr.fAttr & fREQUESTEDIT)
	        VarDesc.wVarFlags |= (WORD)VARFLAG_FREQUESTEDIT;
	    if (pElem->attr.fAttr & fDISPLAYBIND)
	        VarDesc.wVarFlags |= (WORD)VARFLAG_FDISPLAYBIND;
	    if (pElem->attr.fAttr & fDEFAULTBIND)
	        VarDesc.wVarFlags |= (WORD)VARFLAG_FDEFAULTBIND;
	    if (pElem->attr.fAttr & fHIDDEN)
	        VarDesc.wVarFlags |= (WORD)VARFLAG_FHIDDEN;

	    // Now define the element
	    CHECKRESULT(lpdtinfo->AddVarDesc(iElem, &VarDesc));

	    // Lastly, set element's remaining attributes:
	    CHECKRESULT(lpdtinfo->SetVarName(iElem, ToW(pElem->szElemName)));

	    if (pElem->attr.fAttr & fHELPSTRING)
		CHECKRESULT(lpdtinfo->SetVarDocString(iElem, ToW(pElem->attr.lpszHelpString)));
	    if (pElem->attr.fAttr & fHELPCONTEXT)
		CHECKRESULT(lpdtinfo->SetVarHelpContext(iElem, pElem->attr.lHelpContext));
	    // advance to next entry if not all done
	    if (pElem == (LPELEM)ListLast(pElemList))
		break;			// exit if all done
	    pElem = pElem->pNext;
	    iElem++;			// advance element counter
	}
}


// update the tdesc.HRefType field of the base type prior to
// using a given type.
VOID NEAR UpdateHRefType
(
    ICreateTypeInfo FAR* lpdtinfo,
    LPTYPE lpTypeBase
)
{

    HRESULT	res;
    HREFTYPE	hreftype;
    LPSTR	szTypeName;
    LPTYPE	lpTypeCArray;

    // get * to real base type, ignoring non-public typedef's
    lpTypeBase = lpTypePublic(lpTypeBase);
    lpTypeCArray = NULL;		// assume not a C array

    // CONSIDER: non-pointer to a forward declare with no def yet should give an
    // CONSIDER: error. This isn't the right check, but it's close.
    // CONSIDER: error currently gets caught by Layout() so it's no big deal.
    //if (lpTypeBase->tdesc.vt == VT_USERDEFINED && (lpTypeBase->tentrykind & ~tFORWARD))
    //	  goto NoDef;


    while (lpTypeBase->tentrykind == tREF)	// for VT_PTR, VT_SAFEARRAY or
						// VT_CARRAY
	{
	    if (lpTypeBase->tdesc.vt == VT_CARRAY)
	        lpTypeCArray = lpTypeBase;
	    else
	        lpTypeCArray = NULL;	// not a C array
	    lpTypeBase = lpTypeBase->ref.ptypeBase;

 	    // get * to real base type, ignoring non-public typedef's
	    lpTypeBase = lpTypePublic(lpTypeBase);
	}


    // update the tdesc.hreftype of the base type if necessary
    if (lpTypeBase->tdesc.vt == VT_USERDEFINED)
	{

	    if (lpTypeBase->lptinfo == NULL)
		{   // we're trying to use a forward declaration to a
		    // type with no real definition
//NoDef:
		    switch (lpTypeBase->tentrykind & ~tFORWARD)
			{
			case tSTRUCT:
			case tENUM:
			case tUNION:
			    szTypeName = lpTypeBase->structenum.szTag;
			    break;
			default:
			    szTypeName = lpTypeBase->szName;
			    break;
			}
		    ItemError(szFmtErrOutput, szTypeName, OERR_NO_DEF);
		}

	    // get reference to given typeinfo
	    CHECKRESULT(lpdtinfo->AddRefTypeInfo(lpTypeBase->lptinfo, &hreftype));

	    // store this in the appropriate tdesc
	    if (lpTypeCArray)	// C arrays have a separate tdesc
	        lpTypeCArray->tdesc.lpadesc->tdescElem.hreftype = hreftype;
	    else
	        lpTypeBase->tdesc.hreftype = hreftype;
	}
}


VOID NEAR OutputAlias
(
    ICreateTypeInfo FAR* lpdtinfo,
    LPTYPE lpTypeAlias
)
{
    HRESULT res;

    // update the tdesc.hreftype field of the base type if it's
    // a user-defined type.
    UpdateHRefType(lpdtinfo, lpTypeAlias);

    // Define the alias:
    CHECKRESULT(lpdtinfo->SetTypeDescAlias(&(lpTypePublic(lpTypeAlias)->tdesc)));
}


// Fills in a ELEMDESC (for enum's, properties, property_set properties,
// function names, function parms)
VOID NEAR LoadElemDesc
(
    ICreateTypeInfo FAR* lpdtinfo,
    ELEMDESC FAR* lpElemDesc,
    LPELEM pElem
)
{
    WORD    wFlags;

    // set up the type description

    // update the tdesc.hreftype field of the element type if it's
    // a user-defined type.
    UpdateHRefType(lpdtinfo, pElem->elemType);

    lpElemDesc->tdesc = lpTypePublic(pElem->elemType)->tdesc; // copy the tdesc

    // set up the idldesc field
    wFlags = 0;
    if (pElem->attr.fAttr & fIN)
        wFlags |= IDLFLAG_FIN;
    if (pElem->attr.fAttr & fOUT)
        wFlags |= IDLFLAG_FOUT;	
    if (pElem->attr.fAttr & fLCID)
        wFlags |= IDLFLAG_FLCID;	
    if (pElem->attr.fAttr & fRETVAL)
        wFlags |= IDLFLAG_FRETVAL;	
    lpElemDesc->idldesc.wIDLFlags = wFlags;
#ifdef WIN16
    lpElemDesc->idldesc.bstrIDLInfo = NULL;	// no additional info
#else //WIN16
    lpElemDesc->idldesc.dwReserved = 0;
#endif //WIN16

}



VOID NEAR OutputInterface
(
    ICreateTypeInfo FAR* lpdtinfo,
    LPENTRY pEntry
)
{

    if (pEntry->type.inter.interList)
	{   // handle inheritance if any base interface(s)
	    OutputBaseInterfaces(lpdtinfo, pEntry, TKIND_INTERFACE);
	}

    OutputFuncs(lpdtinfo, pEntry, pEntry->inter.funcList, TKIND_INTERFACE);
}


VOID NEAR OutputDispinter
(
    ICreateTypeInfo FAR* lpdtinfo,
    LPENTRY pEntry
)
{
    // handle inheritance from IDispatch or some other interface
    OutputBaseInterfaces(lpdtinfo, pEntry, TKIND_DISPATCH);

    OutputFuncs(lpdtinfo, pEntry, pEntry->dispinter.methList, TKIND_DISPATCH);

    OutputElems(lpdtinfo, pEntry->dispinter.propList, TKIND_DISPATCH);
}


VOID NEAR OutputClass
(
    ICreateTypeInfo FAR* lpdtinfo,
    LPENTRY pEntry
)
{
    // tell typelib.dll about the list of implemented interface(s)
    OutputBaseInterfaces(lpdtinfo, pEntry, TKIND_COCLASS);
}


VOID NEAR OutputBaseInterfaces
(
    ICreateTypeInfo FAR* lpdtinfo,
    LPENTRY pEntry,
    TYPEKIND tkind
)
{
    HREFTYPE	hreftype;
    HRESULT	res;
    LPINTER	lpinterList = pEntry->type.inter.interList;
    LPINTER	lpinter;
    SHORT	i;		
    INT		implTypeFlags;

    Assert (lpinterList);	// caller should have checked this for use
    // point to first base interface
    lpinter = (LPINTER)ListFirst(lpinterList);
    for (i=0;;i++)
	{
	    // get index of typeinfo of base interface/dispinterface
	    if (lpinter->ptypeInter->lptinfo == NULL)
		ItemError(szFmtErrOutput, lpinter->ptypeInter->szName, OERR_NO_DEF);

	    CHECKRESULT(lpdtinfo->AddRefTypeInfo(lpinter->ptypeInter->lptinfo, &hreftype));

	    CHECKRESULT(lpdtinfo->AddImplType(i, hreftype));

	    // 
	    if (tkind == TKIND_COCLASS) {
		// need to always set the flags for items in a coclass
	        implTypeFlags = 0;
	        if (lpinter->fAttr & fDEFAULT)
		    implTypeFlags |= IMPLTYPEFLAG_FDEFAULT;
	        if (lpinter->fAttr & fRESTRICTED)
	            implTypeFlags |= IMPLTYPEFLAG_FRESTRICTED;
	        if (lpinter->fAttr & fSOURCE)
	            implTypeFlags |= IMPLTYPEFLAG_FSOURCE;
	        CHECKRESULT(lpdtinfo->SetImplTypeFlags(i, implTypeFlags));
	    }

	    // advance to next entry if not all done
	    if (lpinter == (LPINTER)ListLast(lpinterList))
		break;			// exit if all done
	    lpinter = (LPINTER)lpinter->pNext;
	} // WHILE
}


// ************************************************************************
// Functions for looking up external types
// ************************************************************************

// load an external type library
VOID FAR LoadExtTypeLib
(
    LPIMPORTLIB lpImpLib
)
{
    ITypeLib FAR* lptlib;
    HRESULT res;
    BSTR bstrName;

    SETITEMCUR(lpImpLib->lpszFileName);

    // get * to ITypeLib interface
    CHECKRESULT(LoadTypeLib(ToW(lpImpLib->lpszFileName), &lptlib));
    lpImpLib->lptlib = lptlib;

    // get name of this library
    CHECKRESULT(lptlib->GetDocumentation(-1, &bstrName, NULL, NULL, NULL));
    // copy library name from the BSTR
    lpImpLib->lpszLibName = _fstrdup(ToA(bstrName));

    SysFreeString(bstrName);		// free the BSTR

    // get * to ITypeComp interface
    CHECKRESULT(lptlib->GetTypeComp(&(lpImpLib->lptcomp)));
    
    // get library attributes
    CHECKRESULT(lptlib->GetLibAttr(&(lpImpLib->lptlibattr)));

}


VOID FAR CleanupImportedTypeLibs()
{
    LPIMPORTLIB lpImpLib;
    LPENTRY	pEntry;

    // free up each of the referenced lptinfo's
    if (typlib.pEntry) {
        pEntry = (LPENTRY)ListFirst(typlib.pEntry);	// point to first entry
        for (;;)
	{
	    // release the ITypeInfo
	    if (pEntry->type.lptinfo != NULL &&
		(pEntry->type.tentrykind & tFORWARD) == 0)
	        pEntry->type.lptinfo->Release();

	    // advance to next entry if not all done
	    if (pEntry == ListLast(typlib.pEntry))
		break;			// exit if all done
	    pEntry = (LPENTRY)pEntry->type.pNext;
	}
    }

    if (typlib.pImpLib)
	{	// only if any imported type libraries

	    // point to first imported library entry
	    lpImpLib = (LPIMPORTLIB)ListFirst(typlib.pImpLib);
	    for (;;)
		{
		    if (lpImpLib->lptcomp) {
		        lpImpLib->lptcomp->Release();
		    }

		    if (lpImpLib->lptlibattr) {
		        Assert (lpImpLib->lptlib);
		        lpImpLib->lptlib->ReleaseTLibAttr(lpImpLib->lptlibattr);
		    }
	    
		    if (lpImpLib->lptlib) {
		        lpImpLib->lptlib->Release();
		    }

		    // advance to next entry if not all done
		    if (lpImpLib == (LPIMPORTLIB)ListLast(typlib.pImpLib))
			break;			// exit if all done
		    lpImpLib = lpImpLib->pNext;
		} // WHILE
	}
}


// find type defined in an external type library,
// and add it to the type table if found.
// lpszLibName is NULL if we're to look in ALL external type libraries.
LPTYPE FAR FindExtType
(
    LPSTR lpszLibName,
    LPSTR lpszTypeName
)
{
    LPIMPORTLIB lpImpLib;
    HRESULT	res;
    ULONG	lHashVal;
    ITypeInfo FAR* lptinfo;
    ITypeComp FAR* lptcomp;

    Assert (lpszTypeName != NULL);

    if (typlib.pImpLib != NULL)   // if any imported type libraries
	{
	    // point to first imported library entry
	    lpImpLib = (LPIMPORTLIB)ListFirst(typlib.pImpLib);

	    for (;;)
		{

		    // if we're to look in all libraries, or this specific lib
		    if (lpszLibName == NULL || !FCmpCaseIns(lpszLibName, lpImpLib->lpszLibName))
			{

			    SETITEMCUR(lpImpLib->lpszFileName);
			    lHashVal = LHashValOfNameSysA(lpImpLib->lptlibattr->syskind,
						      lpImpLib->lptlibattr->lcid,
						      lpszTypeName);

			    CHECKRESULT(lpImpLib->lptcomp->BindType(ToW(lpszTypeName), lHashVal, &lptinfo, &lptcomp));
			    if (lptinfo)		// if found
				{
				    // create a type table entry for this guy
				    ListInsert(&typlib.pEntry, sizeof(TYPE));

				    // lpszTypeName will get freed by caller.
				    // We must allocate new memory for it.
				    typlib.pEntry->type.szName = _fstrdup(lpszTypeName);

				    // CONSIDER: do a GetTypeAttr on this guy,
				    // to ensure it's not a 'module' type

				    typlib.pEntry->type.tdesc.vt = VT_USERDEFINED;
				    // init this now in case of error, since
				    // error cleanup code looks at this.
				    typlib.pEntry->type.lptinfo = NULL;

				    LPTYPEATTR ptypeattr;
				    TENTRYKIND tentrykind;

				    CHECKRESULT(lptinfo->GetTypeAttr(&ptypeattr));
				    // Get the interface typeinfo instead of
				    // the Dispinteface version. 
				    if (ptypeattr->wTypeFlags & TYPEFLAG_FDUAL){
					ITypeInfo FAR* lptinfo2;
					HREFTYPE hreftype;
					CHECKRESULT(lptinfo->GetRefTypeOfImplType((unsigned int)-1, &hreftype));
					CHECKRESULT(lptinfo->GetRefTypeInfo(hreftype, &lptinfo2));
					lptinfo->Release();
				        lptinfo->ReleaseTypeAttr(ptypeattr);
					lptinfo = lptinfo2;
				        CHECKRESULT(lptinfo->GetTypeAttr(&ptypeattr));
				    }
				    typlib.pEntry->type.lptinfo = lptinfo;

				    // assume generic imported type
				    tentrykind = (TENTRYKIND)(rgtentrykind[ptypeattr->typekind] | tIMPORTED);
				    if (lpszLibName) {
				      tentrykind = (TENTRYKIND)(tentrykind | tQUAL);
				    }
				    typlib.pEntry->type.tentrykind = tentrykind;
				    typlib.pEntry->type.import.wTypeFlags = ptypeattr->wTypeFlags;

				    lptinfo->ReleaseTypeAttr(ptypeattr);
				    return &(typlib.pEntry->type); // all done
				}
			}
	    
		    // advance to next entry if not all done
		    if (lpImpLib == (LPIMPORTLIB)ListLast(typlib.pImpLib))
			break;			// exit if all done
		    lpImpLib = lpImpLib->pNext;
		} // WHILE
	}
    return (LPTYPE)NULL;	//type not found
}

#define	TYPELIBERR(name,string)  name
static SCODE rgTypelibScodes[] = {
    #include "typelib.err"		// TYPELIB.DLL scodes
    S_FALSE				// end of table
};
#undef	TYPELIBERR

void NEAR MethodError
(
    HRESULT res
)
{
    ERR err;
    int i;
    CHAR * szFormat;

    Assert(FAILED(res));		// should only be called if error
    
    scodeErrCur = GetScode(res);	// get scode from the hresult

    // pick appripriate error format string, depending on whether we are
    // reading an imported type library, or writing one.
    if (fDoingOutput)
        szFormat = szFmtErrOutput;
    else
        szFormat = szFmtErrImportlib;

    // find err constant that matches this scode, if any
    for (i = 0; ;i++)
	{
	    if (rgTypelibScodes[i] == scodeErrCur)
		{   // found error in mapping table.  Add table index to
		    // first TYPELIB.DLL error to get error constant.
		    err = (ERR)(i + OERR_TYPE_E_IOERROR);
		    break;
		}
	    else if (rgTypelibScodes[i] == S_FALSE)
		{   // not in our mapping table -- use general error
		    err = OERR_TYPEINFO;
		    szFormat = szFmtErrUnknown;
		    break;
		}
	}

    ItemError(szFormat, lpszItemCur, err);
}

};	// end of C data
