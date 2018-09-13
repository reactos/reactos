#include	"mktyplib.h"

// .H file output for MKTYPLIB

#include	<stdio.h>

#ifndef WIN32
#include	<ole2.h>		// required for dispatch.h
#include	"dispatch.h"
#endif //WIN32

#include	"errors.h"
#include	"fileinfo.h"
#include	"intlstr.h"		// for szHead* definitions

// external data
extern TYPLIB typlib;
extern FILE *hHFile;
extern SYSKIND SysKind;
extern int iAlignMax;
extern int iAlignDef;
extern BOOL fSpecifiedInterCC;


// external functions
extern INT FAR FCmpCaseIns(LPSTR str1, LPSTR str2); 


// private data
static CHAR *szHeadOlePrefix0 = "#undef INTERFACE\n"
				"#define INTERFACE ";
static CHAR *szHeadOlePrefix1 = "DECLARE_INTERFACE";
static CHAR *szHeadOlePrefix2 = ")\n{\n";
static CHAR *szHeadOlePrefix3 = "\n    /* ";
static CHAR *szHeadOlePrefix4 = " methods */\n";
static CHAR *szHeadOlePrefix5 = " properties:\n";
static CHAR *szHeadOlePrefix6 = " methods:\n";
static CHAR *szHeadOlePrefix7 = "#ifndef NO_BASEINTERFACE_FUNCS\n";
static CHAR *szHeadOleFuncPrefix1 = "    ";		// leading spaces
static CHAR *szHeadOleFuncPrefix2 = "STDMETHOD";
static CHAR *szHeadOleFuncPrefix3 = ", ";		// after return type
static CHAR *szHeadOleArgPrefix1 = ")(THIS";
static CHAR *szHeadOleArgPrefix2 = "_ ";		// only if args
static CHAR *szHeadOleArgSuffix = ") PURE;\n";
static CHAR *szHeadOleSuffix1 = "    */\n";
static CHAR *szHeadOleSuffix2 = "};\n";
static CHAR *szHeadOleSuffix3 = " */\n";
static CHAR *szHeadOleSuffix7 = "#endif\n";

//Hack for profiling
//#define PROFILE

#ifdef PROFILE
static unsigned long cFuncsTotal = 0;
static unsigned long cArgsTotal = 0;
static unsigned long cVarsTotal = 0;
#endif //PROFILE

// stuff dealing with non-default interface calling conventions
static CHAR *szUndefCallType    = "#undef STDMETHODCALLTYPE\n";
static CHAR *szResetCallType    =
"#if defined(WIN32)\n"
"#define STDMETHODCALLTYPE STDMETHODSTDCALL\n"
"#else\n"
"#define STDMETHODCALLTYPE STDMETHODCDECL\n"
"#endif\n";

static CHAR *szDefCallTypePrefix = "#define STDMETHODCALLTYPE STDMETHOD";

static CHAR *rgszCallType[] = {
    "CDECL",
    "PASCAL",
    "STDCALL"
};

typedef enum {
    CALL_CDECL,
    CALL_PASCAL,
    CALL_STDCALL,
    CALL_DEFAULT
} CALLINGCONV;

static CALLINGCONV ccInterCurrent = CALL_DEFAULT;

// user-specified calling conventions for all platforms
static CHAR * szCCHeader = 
"\n"
"/* Macros for redefining the STDMETHOD calling convention\n"
" * (STDMETHODCALLTYPE).\n"
" */\n"
"\n"
"#if defined(_MAC)\n"
"\n"
"#if !defined(_MSC_VER)\n"
"#define STDMETHODCDECL\n"
"#define STDMETHODPASCAL\n"
"#define STDMETHODSTDCALL\n"
"#else\n"
"#define STDMETHODCDECL              FAR CDECL\n"
"#define STDMETHODPASCAL             FAR PASCAL\n"
"#define STDMETHODSTDCALL            FAR STDCALL\n"
"#endif\n"
"\n"
"#elif defined(WIN32)\n"
"\n"
"#define STDMETHODCDECL              EXPORT __cdecl\n"
"#define STDMETHODPASCAL             EXPORT __pascal\n"
"#define STDMETHODSTDCALL            EXPORT __stdcall\n"
"\n"
"#else     /* WIN16 */\n"
"\n"
"#define STDMETHODCDECL              __export FAR CDECL\n"
"#define STDMETHODPASCAL             __export FAR PASCAL\n"
"#define STDMETHODSTDCALL            __export FAR STDCALL\n"
"\n"
"#endif\n"
"\n";


// canned definition of IUnknown
static CHAR * szHeadIUnknown =
"\n"
"    /* IUnknown methods */\n"
"    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;\n"
"    STDMETHOD_(ULONG, AddRef)(THIS) PURE;\n"
"    STDMETHOD_(ULONG, Release)(THIS) PURE;\n";

// canned definition of IDispatch
static CHAR * szHeadIDispatch =
"\n"
"    /* IDispatch methods */\n"
"    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;\n"
"\n"
"    STDMETHOD(GetTypeInfo)(\n"
"      THIS_\n"
"      UINT itinfo,\n"
"      LCID lcid,\n"
"      ITypeInfo FAR* FAR* pptinfo) PURE;\n"
"\n"
"    STDMETHOD(GetIDsOfNames)(\n"
"      THIS_\n"
"      REFIID riid,\n"
"      OLECHAR FAR* FAR* rgszNames,\n"
"      UINT cNames,\n"
"      LCID lcid,\n"
"      DISPID FAR* rgdispid) PURE;\n"
"\n"
"    STDMETHOD(Invoke)(\n"
"      THIS_\n"
"      DISPID dispidMember,\n"
"      REFIID riid,\n"
"      LCID lcid,\n"
"      WORD wFlags,\n"
"      DISPPARAMS FAR* pdispparams,\n"
"      VARIANT FAR* pvarResult,\n"
"      EXCEPINFO FAR* pexcepinfo,\n"
"      UINT FAR* puArgErr) PURE;\n";

static CHAR *szHeadGuidPrefix = "\nDEFINE_GUID(";
static CHAR *szHeadGuidLIBID = "LIBID_";
static CHAR *szHeadGuidCLSID = "CLSID_";
static CHAR *szHeadGuidIID = "IID_";
static CHAR *szHeadGuidDIID = "DIID_";

// prototypes
VOID FAR OutputHFile (CHAR * szHFile);
VOID NEAR HOutTypedef(LPTYPE pType);
VOID NEAR HOutEnum(LPTYPE pType);
VOID NEAR HOutStructUnion(LPTYPE pType);
VOID NEAR HOutModule(LPENTRY pEntry);
VOID NEAR HOutInterface(LPENTRY pEntry);
VOID NEAR HOutCoclass(LPENTRY pEntry);
VOID NEAR HOutBaseInter(LPENTRY pEntry, BOOL fRecurse);

VOID NEAR HOutFuncs(LPFUNC pFuncList, TENTRYKIND tentryKind);
VOID NEAR HOutElems(LPELEM pElemList, CHAR * szPrefix, CHAR * szSep, CHAR * szSepLast, BOOL fEnum);
VOID NEAR HOutType(LPTYPE pType);
VOID NEAR HOutShortNum(SHORT num, BOOL fHex);
VOID NEAR HOutLongNum(LONG num, BOOL fHex);
VOID NEAR HOutPropPrefix(LPFUNC pFunc);
VOID NEAR HOutGuid(LPATTR pAttr, CHAR * szGuidPrefix, LPSTR lpszName);

#ifdef PROFILE
VOID NEAR XOutF(LPSTR lpszData);
VOID NEAR XOut(CHAR * szData);
#else //PROFILE
#define XOutF HOutF
#define XOut HOut
#endif //PROFILE
VOID NEAR HOutF(LPSTR lpszData);
VOID NEAR HOut(CHAR * szData);
VOID NEAR SetCallType(CALLINGCONV cc);


VOID FAR OutputHFile
(
  CHAR * szHFile
)
{
    LPENTRY	pEntry;

#ifdef WIN16
    // convert szHFile in-place to OEM char set
    AnsiToOem(szHFile, szHFile);

    // don't bother converting back since this string is not used again
#endif // WIN16

    // open the file
    hHFile = fopen(szHFile, "w");	// open output file
    if (hHFile == NULL)
	ParseError(ERR_CANT_OPEN_HFILE);

    Assert (SYS_WIN16 == 0 && SYS_WIN32 == 1 && SYS_MAC == 2 && SysKind <= SYS_MAX);

    HOut(szHeadFile);		// output file header
    HOutF(typlib.szLibName);	// output type library name
    HOut(" */\n\n#ifndef _");	// output: #ifndef _<libname>_H_
    HOutF(typlib.szLibName);	// 	   #define _<libname>_H_
    HOut("_H_\n#define _");
    HOutF(typlib.szLibName);
    HOut("_H_\n");

    HOutGuid(&typlib.attr, szHeadGuidLIBID, typlib.szLibName);

    if (fSpecifiedInterCC) {
        HOut(szCCHeader);
    }

    if (typlib.pEntry)
	{
        pEntry = (LPENTRY)ListFirst(typlib.pEntry);	// point to first entry

#pragma warning(disable:4127)
        while (TRUE)
#pragma warning(default:4127)
	    {

	    switch (pEntry->type.tentrykind & ~tFORWARD)
		{
		case tTYPEDEF:
		    HOutTypedef(&pEntry->type);
		    break;

		case tENUM:
		    HOutEnum(&pEntry->type);
		    break;

		case tSTRUCT:
		case tUNION:
		    HOutStructUnion(&pEntry->type);
		    break;

		case tMODULE:
		    HOutModule(pEntry);
		    break;

		case tCOCLASS:
		    HOutCoclass(pEntry);
		    break;

		case tINTERFACE:
		case tDISPINTER:
		    HOutInterface(pEntry);
		    // fall through

		case tINTRINSIC:
		case tREF:
		    break;		// nothing to output

#ifdef	DEBUG
		default:
		    if (pEntry->type.tentrykind & tIMPORTED)
			break;		// noting to output for imported types
		    Assert(FALSE);
#endif //DEBUG
		}

	    // advance to next entry if not all done
	    if (pEntry == (LPENTRY)ListLast(typlib.pEntry))
		break;			// exit if all done
	    pEntry = (LPENTRY)pEntry->type.pNext;

	    } // WHILE
	}

    HOut("\n#endif\n");

    fclose(hHFile);			// done writing .H file
    hHFile = NULL;			// close done
 
    // check for possible alignment problems
    if (iAlignMax != iAlignDef)
        ParseError(WARN_STRANGE_ALIGNMENT);

#ifdef PROFILE
    printf("\n\ntotal functions: %d\n", cFuncsTotal);
    printf("total function args: %d\n", cArgsTotal);
    printf("total variables: %d\n", cVarsTotal);
    ParseError(ERR_OM);		// bogus early quit
#endif //PROFILE

}


VOID NEAR HOutTypedef
(
LPTYPE pType
)
{
    XOut("\ntypedef ");

    // output base type
    HOutType(pType->td.ptypeAlias);
    HOut(" ");

    XOutF(pType->szName);

    Assert(pType->td.ptypeAlias->tdesc.vt != VT_CARRAY);

    XOut(";\n");

}


VOID NEAR HOutEnum
(
LPTYPE pType
)
{

    XOut("\ntypedef enum ");

    if (pType->structenum.szTag)
	{
	    XOutF(pType->structenum.szTag);
	    if (pType->tentrykind & tFORWARD)
		{
		    XOut(";\n");
		    return;
		}
	    XOut(" ");
	}

    XOut("{\n");

    HOutElems(pType->structenum.elemList, "    ", ",\n", "\n", TRUE);

    XOut("} ");
    XOutF(pType->szName);
    XOut(";\n");

}

VOID NEAR HOutStructUnion
(
LPTYPE pType
)
{

    if ((pType->tentrykind & ~tFORWARD) == tSTRUCT)
	XOut("\ntypedef struct ");
    else
	XOut("\ntypedef union ");

    if (pType->structenum.szTag)
	{
	    XOutF(pType->structenum.szTag);
	    if (pType->tentrykind & tFORWARD)
		goto done;

	    XOut(" ");
	}

    XOut("{\n");

    HOutElems(pType->structenum.elemList, "    ", ";\n", ";\n", FALSE);

    XOut("} ");
    XOutF(pType->szName);

done:
    XOut(";\n");

}

VOID NEAR HOutModule
(
LPENTRY pEntry
)
{
    XOut(szHeadModule);
    XOutF(pEntry->type.szName);
    XOut(szHeadOleSuffix3);
    HOutFuncs(pEntry->module.funcList, pEntry->type.tentrykind);
    if (pEntry->module.constList)
        HOutElems(pEntry->module.constList, "    const ", ";\n", ";\n", FALSE);

}

VOID NEAR HOutCoclass
(
LPENTRY     pEntry
)
{
    if ((pEntry->type.tentrykind & tFORWARD) == 0) {
        HOutGuid(&pEntry->attr, szHeadGuidCLSID, pEntry->type.szName);
    }

    // UNDONE: I don't think this will work in C.
    
    HOut("\n#ifdef __cplusplus\nclass ");
    HOutF(pEntry->type.szName);
    HOut(";\n#endif\n");

}


VOID NEAR HOutInterface
(
LPENTRY     pEntry
)
{

LPSTR	    lpszBaseName;
TENTRYKIND  tentrykind;
LPINTER	    lpInterFirst;	// first base interface, if any
LPINTER	    lpInterLast;	// second/last base interface, if any

    tentrykind = pEntry->type.tentrykind;

    if (tentrykind & tFORWARD)
	{
	    // UNDONE: proper OLE format for forward declaration of interface?
	    // UNDONE: I don't think this will work in C.  I think it wants:
	    // UNDONE: typedef interface <interfacename> <interfacename>;
	    HOut("\ninterface ");
	    HOutF(pEntry->type.szName);
	    HOut(";\n");
	    return;
	}

    HOutGuid(&pEntry->attr,
    	     ((tentrykind == tDISPINTER) ? szHeadGuidDIID: szHeadGuidIID),
	     pEntry->type.szName);

    lpszBaseName = NULL;
    lpInterFirst = NULL;
    if (pEntry->type.inter.interList)
	{
	    lpInterFirst = (LPINTER)ListFirst(pEntry->type.inter.interList);
	    lpInterLast = (LPINTER)ListLast(pEntry->type.inter.interList);

	    // We assume there's only single inheritance at this point
	    // But in the case of a dispinterface, we could have the first
	    // base interface be IDispatch, and the 2nd base interface be
	    // the interface that we're capable of dispatching on.  In any
	    // case, there can't be more than 2 interfaces in the list.
	    Assert((LPINTER)lpInterFirst->pNext == lpInterLast);

	    lpszBaseName = lpInterFirst->ptypeInter->szName;
	    Assert(lpszBaseName);
	}

    // first output the header comment
    HOut((tentrykind == tDISPINTER) ? szHeadDispinter: szHeadInter);
    HOutF(pEntry->type.szName);
    HOut(szHeadOleSuffix3);

    // then output the OLE header
    HOut(szHeadOlePrefix0);
    HOutF(pEntry->type.szName);
    HOut("\n\n");

    HOut(szHeadOlePrefix1);
    if (lpszBaseName)
	HOut("_");
    HOut("(");
    HOutF(pEntry->type.szName);
    if (lpszBaseName)			// if this inherits from somebody
	{				//   then add ", <baseinterface>"
	    HOut(", ");
	    HOutF(lpszBaseName);
	}
    HOut(szHeadOlePrefix2);

    if (tentrykind == tDISPINTER)
	{
	    Assert (lpszBaseName);

	    HOut(szHeadOlePrefix7);
	    HOut(szHeadIUnknown);
	    HOut(szHeadIDispatch);
	    HOut(szHeadOleSuffix7);

	    if (lpInterFirst != lpInterLast)
		{   // specifies an interface that is dispatchable
		    HOut(szHeadDispatchable);
		    HOutF(lpInterLast->ptypeInter->szName);
		    HOut(szHeadOleSuffix3);
		}

	    // first output the properties (commented out) in "struct" format
	    if (pEntry->dispinter.propList)
		{
		    XOut(szHeadOlePrefix3);
		    XOutF(pEntry->type.szName);
		    XOut(szHeadOlePrefix5);
		    HOutElems(pEntry->dispinter.propList, "    ", ";\n", ";\n", FALSE);
		    HOut(szHeadOleSuffix1);
		}

	    // then output the methods (commented out) in "normal" format
	    if (pEntry->dispinter.methList)
		{
		    XOut(szHeadOlePrefix3);
		    XOutF(pEntry->type.szName);
		    XOut(szHeadOlePrefix6);
		    HOutFuncs(pEntry->dispinter.methList, tDISPINTER);
		    HOut(szHeadOleSuffix1);
		}
	}
    else
	{   // an interface

	    // output interface functions, and base interface functions (if any)
	    HOutBaseInter(pEntry, FALSE);

	}

    // lastly, output the close curly
    HOut(szHeadOleSuffix2);

}


VOID NEAR HOutBaseInter(LPENTRY pEntry, BOOL fRecurse)
{
    LPINTER lpInterBase;

    // hard-coded descriptions for the 2 most common
    if (!FCmpCaseIns(pEntry->type.szName, "IUnknown"))
	{
	    HOut(szHeadIUnknown);
	}
    else if (!FCmpCaseIns(pEntry->type.szName, "IDispatch"))
	{
	    if (!fRecurse)
	       HOut(szHeadOlePrefix7);
	    HOut(szHeadIUnknown);
	    if (!fRecurse)
	       HOut(szHeadOleSuffix7);

	    HOut(szHeadIDispatch);
	}
    else if ((pEntry->type.tentrykind & ~tFORWARD) != tINTERFACE)
	{
	    // can't deal with imported base interfaces
	    HOut(szHeadOlePrefix3);
	    HOutF(pEntry->type.szName);
	    HOut(szHeadOlePrefix4);
	    HOut(szHeadMethods);
        }
    else
	{
            if (pEntry->type.tentrykind & tFORWARD) {
		// if this is a forward decl, follow pointer back to real
		// interface (since base interfaces and functions aren't
		// stored in the forward declare)
	        pEntry = pEntry->lpEntryForward;
	    }

	    lpInterBase = pEntry->type.inter.interList;
	    if (lpInterBase)	// if this inherits from somebody,
		{		// then first describe the base interface
		    if (!fRecurse)
		       HOut(szHeadOlePrefix7);

		    // HACK -- assumes we can cast LPTYPE to LPENTRY, but
		    // this is always true given our above validation
		    HOutBaseInter((LPENTRY)(lpInterBase->ptypeInter), TRUE);

		    if (!fRecurse)
		       HOut(szHeadOleSuffix7);
		}

	    // output the interface functions in OLE format
	    XOut(szHeadOlePrefix3);
	    XOutF(pEntry->type.szName);
	    XOut(szHeadOlePrefix4);
	    HOutFuncs(pEntry->inter.funcList, tINTERFACE);
	}
}

//***********************************************************************

VOID NEAR HOutFuncs
(
    LPFUNC	pFuncList,
    TENTRYKIND	tentryKind
)
{

    LPFUNC	pFunc;

    if (pFuncList == NULL)		// nothing to output if no functions
	return;

    pFunc = (LPFUNC)ListFirst(pFuncList);	// point to first entry

#pragma warning(disable:4127)
    while (TRUE)
#pragma warning(default:4127)
	{

#ifndef PROFILE
	    if (tentryKind == tINTERFACE)
		{
		    if (fSpecifiedInterCC) {
		        // set up STDMETHODCALLTYPE based on the calling
		        // convention and SYSKIND

			if (pFunc->func.attr.fAttr2 & f2CCDEFAULTED)
			    SetCallType(CALL_DEFAULT);
		        else if (pFunc->func.attr.fAttr2 & f2PASCAL)
			    SetCallType(CALL_PASCAL);
		        else if (pFunc->func.attr.fAttr2 & f2STDCALL)
			    SetCallType(CALL_STDCALL);
		        else
			    {
			        Assert(pFunc->func.attr.fAttr2 & f2CDECL)
			        SetCallType(CALL_CDECL);
			    }
		    }


		    HOut(szHeadOleFuncPrefix1);	// leading spaces
		    HOut(szHeadOleFuncPrefix2);

		    if (pFunc->func.elemType->tdesc.vt == VT_HRESULT)
		        HOut("(");
		    else
			{
			    HOut("_(");
			    // output function return type
			    HOutType(pFunc->func.elemType);
			    HOut(szHeadOleFuncPrefix3);
			}

		    HOutPropPrefix(pFunc);
		    XOutF(pFunc->func.szElemName);

		    HOut(szHeadOleArgPrefix1);
	
		    if (pFunc->cArgs)
			{
			    HOut(szHeadOleArgPrefix2);
			    // output list of variables, separating them by
			    // commas, with nothing after last item
			    HOutElems(pFunc->argList, "", ", ", "", FALSE);
			}
		    XOut(szHeadOleArgSuffix);
		}
	    else
#endif //!PROFILE
		{
		    HOut(szHeadOleFuncPrefix1);	// leading spaces
		    if (tentryKind == tMODULE)
		        HOut ("extern ");

		    // output function return type
		    HOutType(pFunc->func.elemType);
		    HOut(" ");

		    // output calling convention
		    if (!(pFunc->func.attr.fAttr2 & f2CCDEFAULTED)) {
		        if (pFunc->func.attr.fAttr2 & f2PASCAL)
			    HOut("__pascal ");
		        else if (pFunc->func.attr.fAttr2 & f2CDECL)
			    HOut("__cdecl ");
		        else if (pFunc->func.attr.fAttr2 & f2STDCALL)
			    HOut("__stdcall ");
#ifdef	DEBUG
		        else Assert(FALSE);
#endif	//DEBUG
		    }

		    HOutPropPrefix(pFunc);
		    XOutF(pFunc->func.szElemName);

		    Assert(pFunc->func.elemType->tdesc.vt != VT_CARRAY);

		    XOut("(");

#ifdef PROFILE
		    cArgsTotal += pFunc->cArgs;
		    cFuncsTotal++;
#endif //PROFILE
		    if (pFunc->cArgs == 0)
			{
			    HOut("void");
			}
		    else
			{
			    // output list of variables, separating them by
			    // commas, with nothing after last item
			    HOutElems(pFunc->argList, "", ", ", "", FALSE);
#ifdef PROFILE
			    cVarsTotal-= pFunc->cArgs;	// would be counted twice
#endif //PROFILE
			}
		    XOut(");\n");
		}

	    // advance to next entry if not all done
	    if (pFunc == (LPFUNC)ListLast(pFuncList))
		break;			// exit if all done
	    pFunc = (LPFUNC)pFunc->func.pNext;
	}

    if (fSpecifiedInterCC) {
	SetCallType(CALL_DEFAULT);	// reset to default STDMETHODCALLTYPE
    }
}


VOID NEAR SetCallType
(
    CALLINGCONV cc
)
{
    Assert (fSpecifiedInterCC);		// caller should have checked
    if (cc != ccInterCurrent)
	{   // if current different than last
	    HOut(szUndefCallType);	// undefine current STDMETHODCALLTYPE
	    if (cc == CALL_DEFAULT) {
		HOut(szResetCallType);		// reset to default
	    } else {
	        HOut(szDefCallTypePrefix);	// re-define to new value
	        HOut(rgszCallType[cc]); 
                HOut("\n");
	    }

            ccInterCurrent = cc;             // update current value
	}
}


VOID NEAR HOutPropPrefix
(
    LPFUNC pFunc
)
{
    // add a prefix to the function name if this is a property function
    if (pFunc->func.attr.fAttr & fPROPGET)
	HOut("get_");
    else if (pFunc->func.attr.fAttr & fPROPPUT)
	HOut("put_");
    else if (pFunc->func.attr.fAttr & fPROPPUTREF)
	HOut("putref_");
}


VOID NEAR HOutShortNum
(
    SHORT num,
    BOOL fHex
)
{
    CHAR szBuffer[30];			// space to list a number

    sprintf(szBuffer,
	    fHex ? "0x%hX" : "%hd",
	    num);
    HOut(szBuffer);
}


VOID NEAR HOutLongNum
(
    LONG num,
    BOOL fHex
)
{
    CHAR szBuffer[30];			// space to list a number

    // Stupid C will choke if this number is printed in decimal
    if (num == 0x80000000)
	fHex = TRUE;

    sprintf(szBuffer,
	    fHex ? "0x%lX" : "%ld",
	    num);
    HOut(szBuffer);
}

VOID NEAR HOutElems
(
    LPELEM pElemList,
    CHAR * szPrefix,
    CHAR * szSep,
    CHAR * szSepLast,
    BOOL   fEnum
)
{

    LPELEM pElem;
    WORD  cDims;
    ARRAYDESC FAR* lpAD;
    BOOL fHex;
    LPOLESTR lpch;
    CHAR * pch;
    CHAR buf[2];
    UINT cch;

    pElem = (LPELEM)ListFirst(pElemList);	// point to first entry

#pragma warning(disable:4127)
    while (TRUE)
#pragma warning(default:4127)
	{
	    HOut(szPrefix);
	    if (!fEnum)
		{
		    // output elem type, with the right number of "*'s"
		    HOutType(pElem->elemType);
		    HOut(" ");
		}

	    XOutF(pElem->szElemName);
#ifdef PROFILE
	    cVarsTotal++;
#endif //PROFILE

	    if (!fEnum && pElem->elemType->tdesc.vt == VT_CARRAY)
		{   // base type already outputted before name above
		    lpAD = pElem->elemType->tdesc.lpadesc;
		    for (cDims = 0; cDims < lpAD->cDims; cDims++)
			{
			    HOut("[");
#if 0		// arrays of the form "a[]" aren't supported
			    if (lpAD->rgbounds[cDims].cElements)
#endif //0
			        HOutLongNum((long)lpAD->rgbounds[cDims].cElements, FALSE);
			    HOut("]");
			}
			
		}

	    if (pElem->attr.fAttr2 & f2GotConstVal)
		{
		    HOut(" = ");

		    fHex = FALSE;
		    if (!fEnum) {
		      // display all the unsigned constants in Hex form
		      switch (pElem->elemType->tdesc.vt) {
			case VT_UI1:
			case VT_UI2:
			case VT_UI4:
			case VT_UINT:
			case VT_ERROR:
			   fHex = TRUE;
			   break;
			default:
			   break;
		      }
		    }

		    // output the constant element's value
		    switch (pElem->lpElemVal->vt)
			{
			case VT_I2:
			case VT_BOOL:
			    HOutShortNum(pElem->lpElemVal->iVal, fHex);
			    break;
			case VT_I4:
			case VT_ERROR:
			    HOutLongNum(pElem->lpElemVal->lVal, fHex);
			    break;
			case VT_BSTR:
			    HOut("\"");
			    // output 1 char at a time, in order to handle
			    // escape sequences in strings
			    lpch = pElem->lpElemVal->bstrVal;
			    cch = SysStringLen(lpch);
			    while (cch) {
				switch(*lpch) {
				    case 0x0:
					pch = "\\0";
					break;
				    case 0x7:
					pch = "\\a";
					break;
				    case 0x8:
					pch = "\\b";
					break;
				    case 0x9:
					pch = "\\t";
					break;
				    case 0xA:
				        if (SysKind == SYS_MAC)
					    pch = "\\r";
				        else
					    pch = "\\n";
					break;
				    case 0xB:
					pch = "\\v";
					break;
				    case 0xC:
					pch = "\\f";
					break;
				    case 0xD:
				        if (SysKind == SYS_MAC)
					    pch = "\\n";
				        else
					    pch = "\\r";
					break;
				    default:
#ifdef WIN32
					SideAssert (WideCharToMultiByte(CP_ACP,
				    			0,
				    			lpch,
				    			1,
				    			buf,
				    			1,
                    					NULL,
                    					NULL) != 0);
#else //WIN32
				        buf[0] = *lpch;
#endif //WIN32
				        buf[1] = '\0';
				        pch = buf;
					break;
				}
			        HOut(pch);	// output the char
				lpch++;
				cch--;
			    }
			    HOut("\"");
			    break;
			// CONSIDER: support more constant types.
			default:
			    Assert(FALSE);
			}

		}

	    // advance to next entry if not all done
	    if (pElem == (LPELEM)ListLast(pElemList))
		{
		    XOut(szSepLast);
		    break;			// exit if all done
		}
	    XOut(szSep);
	    pElem = pElem->pNext;
	}

}


VOID NEAR HOutType
(
    LPTYPE pType
)
{
    SHORT i;
    CHAR * szPrefix;

    switch (pType->tdesc.vt)
	{
	case VT_PTR:
	    // first output the base type
	    HOutType(pType->ref.ptypeBase);

	    // now output the proper number of "*"'s
	    Assert (pType->ref.cIndirect != 0);
	    for (i = pType->ref.cIndirect; i > 0; i--)
		{
		    // always output "FAR" for constency (same as dispatch.h)
		    HOut(" FAR*");
		}
	    break;

	case VT_CARRAY:
	    // just output the base type -- we'll handle this stuff
	    // after we output the name
	    HOutType(pType->ref.ptypeBase);
	    break;

	case VT_SAFEARRAY:
	    HOut("SAFEARRAY FAR*");
	    break;

	case VT_BOOL:			// special case -- "boolean" no good
	    HOut("VARIANT_BOOL");
	    break;

	case VT_CY:			// special case -- "CURRENCY" no good
	    HOut("CY");
	    break;

	default:
	    // output "unsigned" if necessary
	    if (pType->tentrykind == tINTRINSIC && pType->intr.fUnsigned)
		HOut("unsigned ");

	    switch (pType->tentrykind & ~tFORWARD)
		{
		    case tUNION:
		        szPrefix = "union ";
			goto outputPrefix;
		    
		    case tSTRUCT:
		        szPrefix = "struct ";
outputPrefix:
			if (pType->structenum.szTag)
			    {
				HOut(szPrefix);
				HOutF(pType->structenum.szTag);
				break;
			    }
			// otherwise, fall into default processing

		    default:
		        HOutF(pType->szName);

		}
	    break;
	}

}

VOID NEAR HOutGuid
(
    LPATTR pAttr,
    CHAR * szGuidPrefix,
    LPSTR  lpszName
)
{
    CHAR szBuffer[100];			// space to list a number UNDONE: Tune
    GUID FAR * lpGuid;

    if ((pAttr->fAttr & fUUID) == 0)
	return;		// no guid to output

    lpGuid = pAttr->lpUuid;
    HOut(szHeadGuidPrefix);
    HOut(szGuidPrefix);		// prefix the user's name
    HOutF(lpszName);		// add the user's name
    sprintf(szBuffer, ",0x%.8lX,0x%.4X,0x%.4X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X);\n",
		      lpGuid->Data1,
		      lpGuid->Data2,
		      lpGuid->Data3,
		      lpGuid->Data4[0],
		      lpGuid->Data4[1],
		      lpGuid->Data4[2],
		      lpGuid->Data4[3],
		      lpGuid->Data4[4],
		      lpGuid->Data4[5],
		      lpGuid->Data4[6],
		      lpGuid->Data4[7]
		      );
    HOut(szBuffer);
}

VOID NEAR HOutF
(
    LPSTR lpszData
)
{
    CHAR szBuffer[256];

    _fstrcpy(szBuffer, lpszData);	// copy data near

    HOut(szBuffer);			// output it
}


VOID NEAR HOut
(
    CHAR * szData
)
{
    if (fputs(szData, hHFile) < 0)	// write the data
	ParseError(ERR_WRITING_HFILE);
}

#ifdef PROFILE
VOID NEAR XOutF
(
    LPSTR lpszData
)
{
    CHAR szBuffer[256];

    _fstrcpy(szBuffer, lpszData);	// copy data near

    XOut(szBuffer);			// output it
}


VOID NEAR XOut
(
    CHAR * szData
)
{
    printf(szData);		// output to console
}

#endif //PROFILE
