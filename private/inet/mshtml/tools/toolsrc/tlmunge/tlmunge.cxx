//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       tlmunge.cxx
//
//  Contents:   Tool to munge forms3.tlb to get methodinfos of event
//              interfaces.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

HRESULT
GetParamTypes(ITypeInfo * pTI,
              FUNCDESC *  pfd,
              LPWSTR      pstrName,
              LPWSTR      pstrParams,
              BOOL        fPseudoName);


BSTR    g_bstrAllNames;
BOOL    g_fDebug = FALSE;

//
// This array stores the real and psuedo names of all the types that appear
// in parameter lists. This is only used when generating the debug version
// of the header files. The information is used to declare a variable of each
// type so that parameter types can be checked for the Event Firing macros.
//
#define MAX_PARAM_TYPES   50
#define MAX_PARAM_SIZE    30

WCHAR   g_achParamTypeStorage[MAX_PARAM_TYPES][2][MAX_PARAM_SIZE];
int     g_cParamTypes = 0;
int     g_cLongestType = 0;

//+------------------------------------------------------------------------
//
//  Function:   ReleaseInterface
//
//  Synopsis:   Releases an interface pointer if it is non-NULL
//
//  Arguments:  [pUnk]
//
//-------------------------------------------------------------------------

void
ReleaseInterface(IUnknown * pUnk)
{
    if (pUnk)
        pUnk->Release();
}



//+------------------------------------------------------------------------
//
//  Function:   GetLastWin32Error
//
//  Synopsis:   Returns the last Win32 error, converted to an HRESULT.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
GetLastWin32Error( )
{
    // Win 95 can return 0, even when there's an error.
    DWORD dw = GetLastError();
    return dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
}


//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Print help.
//
//----------------------------------------------------------------------------

void
Usage()
{
    printf("\nUsage: tlmunge [/D] <typelibrary> <outfilebasename> [/T <outputDir> <inputfilename>]\n");
}



//+---------------------------------------------------------------------------
//
//  Function:   PrintErrorMsg
//
//  Synopsis:   Print an error message.
//
//----------------------------------------------------------------------------

void
PrintErrorMsg(LPSTR pstr)
{
    printf("TLMunge error: %s\n", pstr);
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteParamVerificationVariables
//
//  Synopsis:   Writes out the variable declaration information to the C file
//              and header file for debug-time checking of parameters to the
//              FireXXX macros.
//
//  Arguments:  [pfileHeader] -- File pointer for the header file
//              [pfileCode]   -- File pointer for the cxx file
//
//----------------------------------------------------------------------------

HRESULT
WriteParamVerificationVariables(FILE * pfileHeader, FILE * pfileCode)
{
    int     i;
    int     ret;

    static LPSTR pstrComment = "\n\n//\n"
                               "// Variable declarations for parameter validation on the FireXXX macros.\n"
                               "//\n\n";
    static LPSTR pstrHeader = "extern %-*S g_tldbg%S;\n";
    static LPSTR pstrCode   = "%-*S g_tldbg%S;\n";

    ret = TFAIL(-1, fprintf(
            pfileHeader,
            pstrComment));

    if (ret < 0)
        goto Error;

    ret = TFAIL(-1, fprintf(
            pfileCode,
            pstrComment));

    if (ret < 0)
        goto Error;

    for (i=0; i < g_cParamTypes; i++)
    {
        ret = TFAIL(-1, fprintf(
                pfileHeader,
                pstrHeader,
                g_cLongestType,
                g_achParamTypeStorage[i][1],
                g_achParamTypeStorage[i][2]));

        if (ret < 0)
            goto Error;

        ret = TFAIL(-1, fprintf(
                pfileCode,
                pstrCode,
                g_cLongestType,
                g_achParamTypeStorage[i][1],
                g_achParamTypeStorage[i][2]));

        if (ret < 0)
            goto Error;
    }

    return S_OK;

Error:
    RRETURN(E_FAIL);
}

//+---------------------------------------------------------------------------
//
//  Function:   StoreParamType
//
//  Synopsis:   Checks to see if a given type has already been stored and
//              stores it if not.
//
//  Arguments:  [pstrReal]   -- Pointer to the "real" type name of the param.
//              [pstrPseudo] -- Pointer to the "pseudo" type name of the param.
//
//----------------------------------------------------------------------------

HRESULT
StoreParamType(LPWSTR pstrReal, LPWSTR pstrPseudo)
{
    int i;
    int cLen;

    for (i = g_cParamTypes; i >= 0; i--)
    {
        if (_wcsicmp(pstrReal, g_achParamTypeStorage[i][1]) == 0)
        {
            return S_OK;
        }
    }

    if (g_cParamTypes >= MAX_PARAM_TYPES)
    {
        PrintErrorMsg("Number of parameter types exceeds maximum!");
        RRETURN(E_FAIL);
    }

    cLen = wcslen(pstrReal);
    if ((cLen > MAX_PARAM_SIZE-1) ||
        (wcslen(pstrPseudo) > MAX_PARAM_SIZE-1))
    {
        PrintErrorMsg("Parameter type string is too long!");
        RRETURN(E_FAIL);
    }

    if (cLen > g_cLongestType)
    {
        g_cLongestType = cLen;
    }

    wcscpy(g_achParamTypeStorage[g_cParamTypes][1], pstrReal);
    wcscpy(g_achParamTypeStorage[g_cParamTypes][2], pstrPseudo);
    g_cParamTypes++;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteParameterNames
//
//  Synopsis:   Writes out a list of the parameter names to the file. Also
//              stores the parameter types into the global array.
//
//  Arguments:  [pTI]         -- Pointer to typeinfo
//              [pfd]         -- The FUNCDESC of the function
//              [pfileHeader] -- File to write to
//              [fDebug]      -- TRUE if debug information should be written
//
//----------------------------------------------------------------------------

HRESULT
WriteParameterNames(
        ITypeInfo * pTI,
        FUNCDESC  * pfd,
        FILE *      pfileHeader,
        BOOL        fDebug)
{
    BSTR    abstrNames[30];  // Max 30 parameters
    WCHAR   achTypes[2046];
    UINT    cNames;
    UINT    i;
    int     ret;
    HRESULT hr;
    WCHAR  *pchPseudo = NULL;
    WCHAR  *pchReal   = NULL;

    static LPSTR pstrParam = "%S%S%S%s";
    static LPSTR pstrDebugParam = "(g_tldbg%S=(%S),(%S))%s";

    //
    // First name returned from this function is the method name, the rest are
    // parameter names.
    //
    hr = THR(pTI->GetNames(pfd->memid, abstrNames, 30, &cNames));
    if (hr)
        RRETURN(hr);

    if (fDebug && (cNames > 1))
    {
        hr = THR(GetParamTypes(pTI, pfd, abstrNames[0], achTypes, TRUE));
        if (hr)
            RRETURN(hr);

        pchPseudo = wcstok(achTypes, L",");
        pchReal   = wcstok(NULL, L",");
    }

    for (i=1; i < cNames; i++)
    {
        //
        // If an error occurs during this loop, we continue but stop writing
        // stuff out. This will free all the strings in abstrNames
        //
        if (!hr)
        {
            ret = TFAIL(-1, fprintf(
                    pfileHeader,
                    ((fDebug) ? pstrDebugParam : pstrParam),
                    ((fDebug) ? pchPseudo : L""),
                    abstrNames[i],
                    ((fDebug) ? abstrNames[i] : L""),
                    ((i == cNames-1) ? "" : ", ")));

            if (ret < 0)
            {
                hr = E_FAIL;
            }
        }

        SysFreeString(abstrNames[i]);

        if (!hr && fDebug)
        {
            hr = StoreParamType(pchReal, pchPseudo);

            pchPseudo = wcstok(NULL, L",");
            pchReal   = wcstok(NULL, L",");
        }
    }

    if (cNames)
    {
        SysFreeString(abstrNames[0]);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   WriteMethodInfoWorker
//
//  Synopsis:   Write a methodinfo for each method in an interface, recursing
//              on base classes if necessary.
//
//  Arguments:  [pTI]         -- The interface typeinfo.
//              [pta]         -- The interface TYPEATTR.
//              [bstrName]    -- The interface name.
//              [pfileHeader] -- The header file.
//
//----------------------------------------------------------------------------

HRESULT
WriteMethodInfoWorker(
        ITypeInfo * pTI,
        TYPEATTR *  pta,
        BSTR        bstrName,
        FILE *      pfileHeader)
{
    static LPSTR pstrMethodPrefix = "#define Fire%S_%S(";
    static LPSTR pstrMethodMiddle = ") \\\n          "
                                    "FireEvent(%d, (BYTE *)%s%s";
    static LPSTR pstrMethodEnd    = ")\n";

#define VI(type)   VT_##type, " VTS_"#type, " VTS_P"#type

    struct VARIANT_INFO
    {
        VARTYPE vt;
        LPSTR   pstr;
        LPSTR   pstrRef;
    };

    static VARIANT_INFO avi[] =
    {
        //
        // Note: these are in increasing order of VARIANT value.
        //

        VI(I2),
        VI(I4),
        VI(R4),
        VI(R8),
        VI(CY),
        VI(DATE),
        VI(BSTR),
        VI(DISPATCH),
        VI(ERROR),
        VI(BOOL),
        VI(VARIANT),
        VI(UNKNOWN),
        VI(UI1),
        VT_PTR,         " TLMUNGE ERROR VT_PTR",         " VTS_PI4",
        VT_USERDEFINED, " TLMUNGE ERROR VT_USERDEFINED", " VTS_PI4",
        USHRT_MAX,      " TLMUNGE ERROR USHRT_MAX",      " TLMUNGE ERROR USHRT_MAX 2",
    };

#define COMMENT_COLUMN 53

    HRESULT         hr          = S_OK;
    HREFTYPE        hrt;
    ITypeInfo *     pTIBaseType = NULL;
    TYPEATTR *      ptaBaseType = NULL;
    int             i;
    int             j;
    FUNCDESC *      pfd;
    int             ret;
    BSTR            bstr;
    char            achVTS[1024];
    char            achParams[2408];
    char            achUnknownParamMsg[256];
    ITypeInfo *     pTIUserDefinedOld;
    ITypeInfo *     pTIUserDefined;
    TYPEATTR *      ptaUserDefined;
    TYPEDESC        tdesc;
    BOOL            fIsPtrType;
    VARIANT_INFO *  pvi;
    TYPEKIND        tkind;

    if (pta->cImplTypes > 1)
        RRETURN(E_FAIL);

    if (pta->cImplTypes == 1)
    {
        //
        // Recurse on inherited interfaces so that they appear first
        // in the methodinfo array.
        //

        hr = THR(pTI->GetRefTypeOfImplType(0, &hrt));
        if (hr)
            goto Cleanup;

        hr = THR(pTI->GetRefTypeInfo(hrt, &pTIBaseType));
        if (hr)
            goto Cleanup;

        hr = THR(pTIBaseType->GetTypeAttr(&ptaBaseType));
        if (hr)
            goto Cleanup;

        hr = THR(WriteMethodInfoWorker(pTIBaseType,
                                       ptaBaseType,
                                       bstrName,
                                       pfileHeader));
        if (hr)
            goto Cleanup;
    }

    for (i = 0; i < pta->cFuncs; i++)
    {
        //
        // Write out a macro definition that can be used to fire each event
        // of the form:
        //
        //    Fire<InterfaceName>_<MethodName>(<paramlist>) \
        //       FireEvent(<dispid>, <VTS description of params>, <paramlist>)
        //

        pfd = NULL;
        bstr = NULL;

        hr = THR(pTI->GetFuncDesc(i, &pfd));
        if (hr)
            goto LoopCleanup;

        if (pfd->funckind != FUNC_DISPATCH)
        {
            //
            // Ignore non-dispatch methods.
            //
            goto LoopCleanup;
        }

        hr = THR(pTI->GetDocumentation(pfd->memid, &bstr, NULL, NULL, NULL));
        if (hr)
            goto LoopCleanup;

        //
        // Process each parameter, determining the array of VTS strings and
        // the parameter list.
        //

        achParams[0] = 0;

        if (pfd->cParams == 0)
        {
            strcpy(achVTS, " VTS_NONE");
        }
        else
        {
            achVTS[0] = 0;
        }

        for (j = 0; j < pfd->cParams; j++)
        {
            pTIUserDefinedOld = NULL;

            //
            // Normalize variant type to an IDispatch type.
            //

            if (pfd->lprgelemdescParam[j].tdesc.vt == VT_PTR)
            {
                tdesc = *pfd->lprgelemdescParam[j].tdesc.lptdesc;
                fIsPtrType = TRUE;
            }
            else
            {
                tdesc = pfd->lprgelemdescParam[j].tdesc;
                fIsPtrType = FALSE;
            }

            pTIUserDefinedOld = pTI;
            pTIUserDefinedOld->AddRef();
            while (tdesc.vt == VT_USERDEFINED)
            {
                hr = THR(pTIUserDefinedOld->GetRefTypeInfo(
                        tdesc.hreftype, &pTIUserDefined));
                if (hr)
                    goto Loop2Cleanup;

                hr = THR(pTIUserDefined->GetTypeAttr(&ptaUserDefined));
                if (hr)
                {
                    pTIUserDefined->Release();
                    goto Loop2Cleanup;
                }

                tkind = ptaUserDefined->typekind;
                switch (tkind)
                {
                case TKIND_ALIAS:
                    tdesc = ptaUserDefined->tdescAlias;
                    break;

                case TKIND_ENUM:
                    tdesc.vt = VT_I4;
                    break;

                case TKIND_INTERFACE:
                    tdesc.vt = VT_UNKNOWN;
                    fIsPtrType = FALSE;
                    break;

                case TKIND_COCLASS:
                case TKIND_DISPATCH:
                    tdesc.vt = VT_DISPATCH;
                    fIsPtrType = FALSE;
                    break;

                default:
                    PrintErrorMsg("Unhandled TKIND in parameter type!");
                    hr = E_FAIL;
                    // hr will cause us to drop out one we hit Loop2Cleanup.
                }

                pTIUserDefined->ReleaseTypeAttr(ptaUserDefined);
                pTIUserDefinedOld->Release();
                pTIUserDefinedOld = pTIUserDefined;

                if (tkind != TKIND_ALIAS)
                    break;
            }

            //
            // Get information on the type.
            //

            pvi = avi;
            for (;;)
            {
                if (pvi->vt < tdesc.vt)
                {
                    pvi++;
                }
                else if (pvi->vt == tdesc.vt)
                {
                    break;
                }
                else
                {
                    goto Loop2Error;
                }
            }

            //
            // Update stack size and vts strings.
            //

            if (fIsPtrType)
            {
                strcat(achVTS, pvi->pstrRef);
            }
            else
            {
                strcat(achVTS, pvi->pstr);
            }

Loop2Cleanup:
            ReleaseInterface(pTIUserDefinedOld);
            if (hr)
                goto LoopCleanup;

            continue;

Loop2Error:
            sprintf(achUnknownParamMsg,
                    "Unknown param type in function %S, param %d, type %d.\n",
                    bstr,
                    j + 1,
                    tdesc.vt);

            PrintErrorMsg(achUnknownParamMsg);
            hr = E_FAIL;
            goto Loop2Cleanup;
        }

        // Print macro definition
        ret = TFAIL(-1, fprintf(
                pfileHeader,
                pstrMethodPrefix,
                bstrName,
                bstr));
        if (ret < 0)
            goto LoopError;

        if (pfd->cParams)
        {
            hr = WriteParameterNames(pTI, pfd, pfileHeader, FALSE);
            if (hr)
                goto LoopCleanup;
        }

        ret = TFAIL(-1, fprintf(
                pfileHeader,
                pstrMethodMiddle,
                pfd->memid,
                achVTS,
                (pfd->cParams) ? ", " : ""));
        if (ret < 0)
            goto LoopError;

        if (pfd->cParams)
        {
            hr = WriteParameterNames(pTI, pfd, pfileHeader, g_fDebug);
            if (hr)
                goto LoopCleanup;
        }

        ret = TFAIL(-1, fprintf(
                pfileHeader,
                pstrMethodEnd));
        if (ret < 0)
            goto LoopError;

LoopCleanup:
        SysFreeString(bstr);
        if (pfd)
            pTI->ReleaseFuncDesc(pfd);

        if (hr)
            goto Cleanup;

        continue;

LoopError:
        hr = E_FAIL;
        goto LoopCleanup;
    }

Cleanup:
    if (ptaBaseType)
        pTIBaseType->ReleaseTypeAttr(ptaBaseType);

    ReleaseInterface(pTIBaseType);

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   WriteMethodInfo
//
//  Synopsis:   Write the methodinfo of an interface.
//
//  Arguments:  [pTI]         -- The interface typeinfo.
//              [pta]         -- The interface TYPEATTR.
//              [pfileHeader] -- The header file.
//              [pfileCode]   -- The code file.
//
//----------------------------------------------------------------------------

HRESULT
WriteMethodInfo(
        ITypeInfo * pTI,
        TYPEATTR *  pta,
        FILE *      pfileHeader,
        FILE *      pfileCode)
{
    static LPSTR pstrItfStart  =
        "//\n"
        "// Event Firing Macros for the %S interface\n"
        "//\n"
        ;

    HRESULT hr;
    BSTR    bstrName        = NULL;
    BSTR    bstrNameSpace   = NULL;
    BSTR    bstrAllNamesNew;
    int     ret;

    hr = THR(pTI->GetDocumentation(
            MEMBERID_NIL, &bstrName, NULL, NULL, NULL));
    if (hr)
        goto Cleanup;

    //
    // Skip this interface if it has already been written.
    //

    // Create name with space delimiter.
    bstrNameSpace = SysAllocStringLen(NULL, SysStringLen(bstrName) + 2);
    if (!bstrNameSpace)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    wcscpy(bstrNameSpace, L" ");
    wcscat(bstrNameSpace, bstrName);
    wcscat(bstrNameSpace, L" ");

    if (g_bstrAllNames && wcsstr(g_bstrAllNames, bstrNameSpace))
    {
        hr = S_OK;
        goto Cleanup;
    }

    bstrAllNamesNew = SysAllocStringLen(
            NULL,
            (g_bstrAllNames ? SysStringLen(g_bstrAllNames) : 0) + SysStringLen(bstrNameSpace));
    if (!bstrAllNamesNew)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (g_bstrAllNames)
    {
        wcscpy(bstrAllNamesNew, g_bstrAllNames);
    }
    else
    {
        bstrAllNamesNew[0] = 0;
    }

    wcscat(bstrAllNamesNew, bstrNameSpace);
    g_bstrAllNames = bstrAllNamesNew;

    // Write comment header
    ret = TFAIL(-1, fprintf(pfileHeader, pstrItfStart, bstrName));
    if (ret < 0)
        goto Error;

    // Write methodinfo of each method.
    hr = THR(WriteMethodInfoWorker(pTI, pta, bstrName, pfileHeader));
    if (hr)
        goto Cleanup;

Cleanup:
    SysFreeString(bstrNameSpace);
    SysFreeString(bstrName);
    RRETURN(hr);

Error:
    hr = E_FAIL;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Function:   MungeCoClasses
//
//  Synopsis:   Determine the maximum size of vtables for source interfaces
//              and default dispinterfaces, and write a methodinfo for each
//              source interface.
//
//  Arguments:  [pTL]         -- The typelibrary.
//              [pfileHeader] -- The header file.
//              [pfileCode]   -- The code file.
//
//----------------------------------------------------------------------------

HRESULT
MungeCoClasses(
        ITypeLib *  pTL,
        FILE *      pfileHeader,
        FILE *      pfileCode)
{
    static GUID CLSID_CControl =
    {
        0x909e0ae0,
        0x16dc,
        0x11ce,
        {0x9e, 0x98, 0x00, 0xaa, 0x00, 0x57, 0x4a, 0x4f}
    };

    static GUID CLSID_CDataFrame =
    {
        0x3050f1c4, 
        0x98b5, 
        0x11cf, 
        {0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b}
    };

    static  const GUID * g_apclsidIgnore[] =
    {
        &CLSID_CDataFrame,
        &CLSID_CControl,
    };

    HRESULT     hr = S_OK;
    int         ret;
    int         i;
    int         j;
    TYPEKIND    tk;
    ITypeInfo * pTICoClass;
    TYPEATTR *  ptaCoClass;
    ITypeInfo * pTIImplType;
    TYPEATTR *  ptaImplType;
    int         typeflags;
    HREFTYPE    hrt;
    BOOL        fIgnoreIfaceSize;
    BSTR        bstrDefaultDface = NULL;
    int         cDefaultDfaceMethodsMax;

    cDefaultDfaceMethodsMax = 0;

    //
    // Loop over coclass typeinfos.
    //

    i = pTL->GetTypeInfoCount();
    while (--i >= 0)
    {
        pTICoClass = NULL;
        ptaCoClass = NULL;

        hr = THR(pTL->GetTypeInfoType(i, &tk));
        if (hr || tk != TKIND_COCLASS)
            goto LoopCleanup;

        hr = THR(pTL->GetTypeInfo(i, &pTICoClass));
        if (hr)
            goto LoopCleanup;

        hr = THR(pTICoClass->GetTypeAttr(&ptaCoClass));
        if (hr)
            goto LoopCleanup;

        fIgnoreIfaceSize = FALSE;
        for (j = ARRAY_SIZE(g_apclsidIgnore); --j >= 0;)
        {
            if (*g_apclsidIgnore[j] == ptaCoClass->guid)
            {
                fIgnoreIfaceSize = TRUE;
                break;
            }
        }

        j = ptaCoClass->cImplTypes;
        while (--j >= 0)
        {
            //
            // Loop over source interfaces and default sink dispinterfaces.
            //

            pTIImplType = NULL;
            ptaImplType = NULL;

            hr = THR(pTICoClass->GetImplTypeFlags(j, &typeflags));
            if (hr)
                goto Loop2Cleanup;

            if (!(typeflags & IMPLTYPEFLAG_FSOURCE) &&
                !(typeflags & IMPLTYPEFLAG_FDEFAULT))
                goto Loop2Cleanup;

            hr = THR(pTICoClass->GetRefTypeOfImplType(j, &hrt));
            if (hr)
                goto Loop2Cleanup;

            hr = THR(pTICoClass->GetRefTypeInfo(hrt, &pTIImplType));
            if (hr)
                goto Loop2Cleanup;

            hr = THR(pTIImplType->GetTypeAttr(&ptaImplType));
            if (hr)
                goto Loop2Cleanup;

            if ((typeflags & IMPLTYPEFLAG_FSOURCE) &&
                (ptaImplType->typekind == TKIND_DISPATCH))
            {
                //
                // Write the methodinfo for this interface.
                //

                hr = THR(WriteMethodInfo(
                        pTIImplType, ptaImplType, pfileHeader, pfileCode));
            }
            else
            {
                if (!fIgnoreIfaceSize)
                {
                    //
                    // Count the maximum number of methods in a default interface.
                    //

                    if (ptaImplType->typekind != TKIND_DISPATCH ||
                        !(ptaImplType->wTypeFlags & TYPEFLAG_FDUAL))
                        goto Loop2Cleanup;

                    Assert(ptaImplType->cVars == 0);
                    if ((int) ptaImplType->cFuncs > cDefaultDfaceMethodsMax)
                    {
                        // Remember the name of the interface.
                        FormsFreeString(bstrDefaultDface);
                        bstrDefaultDface = NULL;
                        hr = THR(pTIImplType->GetDocumentation(
                                MEMBERID_NIL, &bstrDefaultDface, NULL, NULL, NULL));

                        if (hr)
                            goto Loop2Cleanup;

                        cDefaultDfaceMethodsMax = (int) ptaImplType->cFuncs;
                    }
                }
            }

Loop2Cleanup:
            if (ptaImplType)
                pTIImplType->ReleaseTypeAttr(ptaImplType);

            ReleaseInterface(pTIImplType);
            if (hr)
                goto LoopCleanup;
        }

LoopCleanup:
        if (ptaCoClass)
            pTICoClass->ReleaseTypeAttr(ptaCoClass);

        ReleaseInterface(pTICoClass);
        if (hr)
            goto Cleanup;
    }

    ret = TFAIL(-1, fprintf(
            pfileHeader,
            "\n\n#define DEFAULT_INTERFACE_METHODS_MAX %d // %S \n",
            cDefaultDfaceMethodsMax, bstrDefaultDface));
    if (ret < 0)
        goto FileError;


Cleanup:
    FormsFreeString(bstrDefaultDface);

    RRETURN(hr);

FileError:
    hr = E_FAIL;
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetInterfaceMethodCount
//
//  Synopsis:   Returns the number of methods in an interface,
//              including base classes.
//
//  Arguments:  [pTL]       -- The type library.
//              [iid]       -- The iid of the interface.
//              [fDual]     -- Is the interface dual?
//              [pcMethods] -- Number of methods in interface.
//
//----------------------------------------------------------------------------

HRESULT
GetInterfaceMethodCount(ITypeLib * pTL, REFIID iid, BOOL fDual, int * pcMethods)
{
    HRESULT     hr;
    ITypeInfo * pTI = NULL;
    TYPEATTR *  pta = NULL;
    ITypeInfo * pTIIface;
    HREFTYPE    href;

    hr = THR(pTL->GetTypeInfoOfGuid(iid, &pTI));
    if (hr)
        goto Cleanup;

    if (fDual)
    {
        hr = THR(pTI->GetRefTypeOfImplType((UINT) -1, &href));
        if (hr)
            goto Cleanup;

        hr = THR(pTI->GetRefTypeInfo(href, &pTIIface));
        if (hr)
            goto Cleanup;

        pTI->Release();
        pTI = pTIIface;
    }

    hr = THR(pTI->GetTypeAttr(&pta));
    if (hr)
        goto Cleanup;

    *pcMethods = (int) (pta->cbSizeVft / sizeof(void (*)()));

Cleanup:
    if (pta)
        pTI->ReleaseTypeAttr(pta);

    ReleaseInterface(pTI);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   MungeInterfaceMethodCounts
//
//  Synopsis:   Generate constants for the sizes of various interfaces.
//
//  Arguments:  [pTL]         -- The typelibrary.
//              [pfileHeader] -- The header file to write to.
//
//----------------------------------------------------------------------------

#ifndef PRODUCT_96P
HRESULT
MungeInterfaceMethodCounts(ITypeLib * pTL, FILE * pfileHeader)
{
    static struct
    {
        const IID * piid;
        BOOL        fDual;
        LPSTR       pstr;
    } g_amci[] =
    {
        &IID_IControl, TRUE, "NUMBER_ICONTROL_METHODS",
        &IID_IForm, TRUE, "NUMBER_IFORM_METHODS",
    };

    HRESULT hr;
    int     ret;
    int     c;
    int     i;

    ret = TFAIL(-1, fprintf(
            pfileHeader,
            "\n#endif // _F3MUNGE_METHODCOUNTSONLY\n"));
    if (ret < 0)
        RRETURN(E_FAIL);

    for (i = ARRAY_SIZE(g_amci); --i >= 0;)
    {
        hr = THR(GetInterfaceMethodCount(
                pTL,
                *g_amci[i].piid,
                g_amci[i].fDual,
                &c));

        if (hr)
            RRETURN(hr);

        ret = TFAIL(-1, fprintf(
                pfileHeader,
                "\n#define %s %d\n",
                g_amci[i].pstr,
                c));
        if (ret < 0)
            RRETURN(E_FAIL);

    }

    return S_OK;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   GetParamTypes
//
//  Synopsis:   Returns a string containing the prototype arguments
//              of a function. The string contains only the parameter types,
//              not the parameter names.
//
//  Arguments:  [pTI]         -- The typeinfo of the function.
//              [pfd]         -- The FUNCDESC of the function.
//              [pstrName]    -- The name of the function in case of error.
//              [pstrParams]  -- Buffer to put the resulting string.
//              [fPseudoName] -- If TRUE, pseudo-names are returned in addition
//                               to the real type names.
//
//----------------------------------------------------------------------------

HRESULT
GetParamTypes(ITypeInfo * pTI,
              FUNCDESC *  pfd,
              LPWSTR      pstrName,
              LPWSTR      pstrParams,
              BOOL        fPseudoName)
{
    static WCHAR    achSeparator[] = L", ";
    static WCHAR    achSeparator2[] = L",";

    HRESULT     hr = S_OK;
    USHORT      u;
    ITypeInfo * pTIUserDef;
    BSTR        bstrName;
    LPWSTR      pstrTypeName;
    LPWSTR      pstrPseudoName;
    int         cPointer;
    int         i;
    TYPEDESC *  ptd;
    TYPEATTR *  ptaUserDef;
    char        achUnknownParamMsg[256];
    WCHAR     * pchSeparator = (fPseudoName) ? achSeparator2 : achSeparator;
    int         cSeparator   = wcslen(pchSeparator);
    BOOL        fBuiltin;

    // Loop over each parameter.
    for (u = 0; u < pfd->cParams; u++)
    {
        pTIUserDef = NULL;
        ptaUserDef = NULL;
        bstrName = NULL;

        // Add comma separator when more than one argument to function.
        if (u > 0)
        {
            wcscpy(pstrParams, pchSeparator);
            pstrParams += cSeparator;
        }

        ptd = &pfd->lprgelemdescParam[u].tdesc;
        Assert((ptd->vt & ~VT_TYPEMASK) == 0);

        // Count number of pointers.
        cPointer = 0;
        while (ptd->vt == VT_PTR)
        {
            cPointer++;
            ptd = ptd->lptdesc;
        }

        // Emit pointers for pseudo names.
        if (fPseudoName && (cPointer != 0))
        {
            for (i = cPointer; i; i--)
            {
                *pstrParams++ = L'p';
            }
        }

        fBuiltin = TRUE;

        if (ptd->vt == VT_USERDEFINED)
        {
            // Get name of user defined type.

            hr = pTI->GetRefTypeInfo(ptd->hreftype, &pTIUserDef);
            if (hr)
                goto LoopCleanup;

            hr = THR(pTIUserDef->GetTypeAttr(&ptaUserDef));
            if (hr)
                goto LoopCleanup;

            if ((ptaUserDef->typekind == TKIND_COCLASS) ||
                ((ptaUserDef->typekind == TKIND_DISPATCH) &&
                     !(ptaUserDef->wTypeFlags & TYPEFLAG_FDUAL)))
            {
                ptd->vt = VT_DISPATCH;
                if (cPointer)
                    cPointer--;
            }
            else
            {
                hr = pTIUserDef->GetDocumentation(
                        MEMBERID_NIL, &bstrName, NULL, NULL, NULL);
                if (hr)
                    goto LoopCleanup;

                wcscpy(pstrParams, bstrName);
                pstrParams += SysStringLen(bstrName);

                if (fPseudoName)
                {
                    wcscpy(pstrParams, pchSeparator);
                    pstrParams += cSeparator;
                    wcscpy(pstrParams, bstrName);
                    pstrParams += SysStringLen(bstrName);
                }

                fBuiltin = FALSE;
            }
        }

        if (fBuiltin)
        {
            // Get name of built-in type.

    #define CASETYPE(vt, str, pseudo) case (vt): \
               pstrPseudoName = (pseudo);        \
               pstrTypeName = (str);             \
               break;

            switch (ptd->vt)
            {
            CASETYPE(VT_I2,      L"short",          L"Short")
            CASETYPE(VT_I4,      L"long",           L"Long")
            CASETYPE(VT_R4,      L"float",          L"Float")
            CASETYPE(VT_R8,      L"double",         L"Double")
            CASETYPE(VT_BSTR,    L"BSTR",           L"BSTR")
            CASETYPE(VT_DISPATCH,L"IDispatch *",    L"pDispatch")
            CASETYPE(VT_ERROR,   L"HRESULT",        L"hr")
            CASETYPE(VT_BOOL,    L"VARIANT_BOOL",   L"VarBool")
            CASETYPE(VT_VARIANT, L"VARIANT",        L"Variant")
            CASETYPE(VT_UNKNOWN, L"IUnknown *",     L"pUnk")
            CASETYPE(VT_I1,      L"signed char",    L"Char")
            CASETYPE(VT_UI1,     L"unsigned char",  L"Byte")
            CASETYPE(VT_UI2,     L"unsigned short", L"UShort")
            CASETYPE(VT_UI4,     L"unsigned long",  L"ULong")
            CASETYPE(VT_INT,     L"int",            L"Int")
            CASETYPE(VT_UINT,    L"unsigned int",   L"UInt")
            CASETYPE(VT_VOID,    L"void",           L"Void")

            default:
                sprintf(achUnknownParamMsg,
                        "Unknown param type in function %S, param %d, type %d.\n",
                        pstrName,
                        u + 1,
                        ptd->vt);
                PrintErrorMsg(achUnknownParamMsg);
                hr = E_FAIL;
                goto LoopCleanup;
            }

            if (fPseudoName)
            {
                wcscpy(pstrParams, pstrPseudoName);
                pstrParams += wcslen(pstrPseudoName);
                wcscpy(pstrParams, pchSeparator);
                pstrParams += cSeparator;
            }
            wcscpy(pstrParams, pstrTypeName);
            pstrParams += wcslen(pstrTypeName);
        }

        // Emit pointers for real type name.
        if (cPointer != 0)
        {
            if (!fPseudoName)
                *pstrParams++ = L' ';

            while (cPointer--)
            {
                *pstrParams++ = L'*';
            }
        }

LoopCleanup:
        if (ptaUserDef)
            pTIUserDef->ReleaseTypeAttr(ptaUserDef);

        if (pTIUserDef)
            pTIUserDef->Release();

        FormsFreeString(bstrName);
        if (hr)
            goto Cleanup;
    }

    *pstrParams = 0;

Cleanup:
    RRETURN(hr);

}



//+---------------------------------------------------------------------------
//
//  Function:   WriteVtableInfo
//
//  Synopsis:   Create a vtable for an interface that is implemented
//              by a class.
//
//  Arguments:  [pTL]        -- The type library.
//              [pfileCode]  -- The file to write to.
//              [pstrMember] -- The string describing each member.
//              [fWriteTearOffs] -- indicates the format of the output string
//
//----------------------------------------------------------------------------
#ifndef PRODUCT_96P
HRESULT
WriteVtableInfo(
        ITypeInfo   *pTIMaster,
        FILE *      pfileCode,
        LPSTR       pstrMember,
        BOOL        fWriteTearOffs)
{
    HRESULT     hr;
    int         ret=0;
    TYPEATTR *  pta = NULL;
    int         i;
    BSTR        bstrName;
    WCHAR       achParams[1024];
    FUNCDESC *  pfd;
    ITypeInfo * pTIBaseType=NULL;
    HREFTYPE    hrt;



    hr = THR(pTIMaster->GetTypeAttr(&pta));
    if (hr)
        goto Cleanup;


    if (fWriteTearOffs && pta->cImplTypes == 1)
    {
        //
        //  BUGBUG: Recursion, we should consider doing this
        //          in general for all interfaces (frankman)
        //

        //
        // Recurse on inherited interfaces so that they appear first
        // in the methodinfo array.
        //

        hr = THR(pTIMaster->GetRefTypeOfImplType(0, &hrt));
        if (hr)
            goto Cleanup;

        hr = THR(pTIMaster->GetRefTypeInfo(hrt, &pTIBaseType));
        if (hr)
            goto Cleanup;

        hr = THR(WriteVtableInfo(pTIBaseType,pfileCode,pstrMember,fWriteTearOffs));
        if (hr)
            goto Cleanup;
    }

    if (fWriteTearOffs && (pta->guid == IID_IUnknown || pta->guid == IID_IDispatch))
    {
        goto Cleanup;
    }

    // Print an entry for each function.
    for (i = 0; i < pta->cFuncs; i++)
    {
        bstrName = NULL;
        pfd = NULL;

        hr = THR(pTIMaster->GetFuncDesc(i, &pfd));
        if (hr)
            goto Cleanup;

        hr = THR(pTIMaster->GetDocumentation(
                pfd->memid, &bstrName, NULL, NULL, NULL));

        if (hr)
            goto LoopCleanup;

        hr = THR(GetParamTypes(pTIMaster, pfd, bstrName, achParams, FALSE));
        if (hr)
            goto LoopCleanup;

        if (fWriteTearOffs)
        {
            ret = TFAIL(-1, fprintf(
                pfileCode,
                pstrMember,
                bstrName,
                achParams));
        }
        else
        {
            ret = TFAIL(-1, fprintf(
                pfileCode,
                pstrMember,
                achParams,
                bstrName));

        }

        if (ret < 0)
        {
            hr = E_FAIL;
            goto LoopCleanup;
        }

LoopCleanup:
        if (pfd)
            pTIMaster->ReleaseFuncDesc(pfd);

        FormsFreeString(bstrName);
        if (hr)
            goto Cleanup;
    }


Cleanup:
    if (pta)
        pTIMaster->ReleaseTypeAttr(pta);

    ReleaseInterface(pTIBaseType);

    RRETURN(hr);
}
#endif




//+---------------------------------------------------------------------------
//
//  Function:   MungeOneVtable
//
//  Synopsis:   Create a vtable for an interface that is implemented
//              by a class.
//
//  Arguments:  [pTL]        -- The type library.
//              [pfileCode]  -- The file to write to.
//              [iid]        -- The interface.
//              [pstrBegin]  -- The header string.
//              [pstrMember] -- The string describing each member.
//
//----------------------------------------------------------------------------
#ifndef PRODUCT_96P
HRESULT
MungeOneVtable(
        ITypeLib *  pTL,
        FILE *      pfileCode,
        REFIID      iid,
        LPSTR       pstrBegin,
        LPSTR       pstrMember)
{
    HRESULT     hr;
    int         ret;
    ITypeInfo * pTIDual = NULL;
    ITypeInfo * pTI = NULL;
    HREFTYPE    hrt;

    // Print beginning of vtable.
    ret = TFAIL(-1, fprintf(pfileCode, pstrBegin));
    if (ret < 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Get typeinfo of dual interface.
    hr = THR(pTL->GetTypeInfoOfGuid(iid, &pTIDual));
    if (hr)
        goto Cleanup;

    hr = THR(pTIDual->GetRefTypeOfImplType((UINT)-1, &hrt));
    if (hr)
        goto Cleanup;

    hr = THR(pTIDual->GetRefTypeInfo(hrt, &pTI));
    if (hr)
        goto Cleanup;


    hr = THR(WriteVtableInfo(pTI, pfileCode, pstrMember, FALSE));
    if (hr)
        goto Cleanup;

    // Print closing brace.
    ret = TFAIL(-1, fprintf(pfileCode, "};\n\n"));
    if (ret < 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pTIDual);
    ReleaseInterface(pTI);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateTearOffTable
//
//  Synopsis:   Creates a tearoff table for a given interface iid
//
//  Arguments:  [pTL]       -- The type library.
//              [pstrInputFile] -- an input file of the following form:
//                  <charBaseClassName> - name for the BaseClass
//                  <charOutPutFile>    - filename for the output file
//                  <charIID>         - char representation of the InterfaceID
//                  the tokens are space seperated
//
//----------------------------------------------------------------------------
HRESULT
CreateTearOffTable(ITypeLib *pTL, LPSTR pstrFileOutput,
                    LPSTR pstrClassname, LPSTR pstrInterfaceName, const GUID &iid)
{
    static LPSTR pstrHeader =
        "//+------------------------------------------------------------------------\n"
        "//\n"
        "//  Microsoft Forms\n"
        "//  Copyright (C) Microsoft Corporation, 1992 - 1996.\n"
        "//\n"
        "//  File:       %s\n"
        "//\n"
        "//  Contents:   Tearoff table for %s\n"
        "//              inheriting interface %s\n"
        "//\n"
        "//-------------------------------------------------------------------------\n"
        "\n"
        "\n"
        ;

    static LPSTR pstrIntro = "BEGIN_TEAROFF_TABLE(%s, %s)\n"
                             "    TEAROFF_METHOD(GetTypeInfoCount, (unsigned int *))\n"
                             "    TEAROFF_METHOD(GetTypeInfo, (unsigned int, unsigned long, ITypeInfo **))\n"
                             "    TEAROFF_METHOD(GetIDsOfNames, (REFIID, LPOLESTR *, unsigned int, LCID, DISPID *))\n"
                             "    TEAROFF_METHOD(Invoke, (DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, unsigned int *))\n";


    static LPSTR pstrMember = "    TEAROFF_METHOD(%S, (%S))\n";

    HRESULT     hr = S_OK;
    FILE        *pfileOutput=0;
    int         ret;
    ITypeInfo   *pTIDual=NULL;
    ITypeInfo * pTI=NULL;
    HREFTYPE    hrt;




    Assert(pTL);
    Assert(pstrClassname);
    Assert(pstrFileOutput);

        // now open the output file...

    pfileOutput = TFAIL(NULL, fopen(pstrFileOutput, "w"));
    if (!pfileOutput)
    {
        PrintErrorMsg("Unable to open outputfile for tearoff tables.");
        goto Error;
    }

    ret = TFAIL(-1,fprintf(pfileOutput, pstrHeader,
                        pstrFileOutput, pstrClassname, pstrInterfaceName));

    if (ret < 0)
    {
        goto Error;
    }


    // Print beginning of vtable.
    ret = TFAIL(-1, fprintf(pfileOutput, pstrIntro, pstrClassname, pstrInterfaceName));
    if (ret < 0)
    {
        goto Error;
    }


    // Get typeinfo of dual interface.
    hr = THR(pTL->GetTypeInfoOfGuid(iid, &pTIDual));
    if (hr)
        goto Cleanup;

    hr = THR(pTIDual->GetRefTypeOfImplType((UINT)-1, &hrt));
    if (hr)
        goto Cleanup;

    hr = THR(pTIDual->GetRefTypeInfo(hrt, &pTI));
    if (hr)
        goto Cleanup;



    hr = THR(WriteVtableInfo(pTI, pfileOutput, pstrMember, TRUE));
    if (hr)
        goto Cleanup;

    // Print closing statement
    ret = TFAIL(-1, fprintf(pfileOutput, "END_TEAROFF_TABLE()\n\n"));
    if (ret < 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    if (pfileOutput)
    {
        fclose(pfileOutput);
    }
    ReleaseInterface(pTIDual);
    ReleaseInterface(pTI);

    RRETURN(hr);

Error:
    hr = E_FAIL;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Function:   MungeTearOffInterfaces
//
//  Synopsis:   Creates Tearoff tables for a set of interfaces
//
//  Arguments:  [pTL]       -- The type library.
//              [pstrInputFile] -- an input file of the following form:
//                  <charBaseClassName> - name for the BaseClass
//                  <charInterfaceName> - name of the interface
//                  <charOutPutFile>    - filename for the output file
//                  <charIID>         - char representation of the InterfaceID
//                  the tokens are space seperated
//              look in tearoff.dat for a sample
//
//----------------------------------------------------------------------------
HRESULT
MungeTearOffInterfaces(ITypeLib *pTL, LPSTR pstrOutPath, LPSTR pstrInputFile)
{
    HRESULT     hr = S_OK;
    FILE *      pfileInput = 0;
    char        achClassName[MAX_PATH];
    char        achFileOut[MAX_PATH];
    char        achInterfaceIID[MAX_PATH];
    char        achInterfaceName[MAX_PATH];
    char        achTemp[MAX_PATH];
    TCHAR       tachIID[MAX_PATH*2];
    GUID         iid;


    pfileInput = TFAIL(NULL, fopen(pstrInputFile, "r"));

    if (!pfileInput)
    {
        hr = E_FAIL;
        PrintErrorMsg("Unable to open inputfile for tearoff tables.");
        goto Error;
    }

    while (fscanf(pfileInput, "%s%s%s%s", achClassName, achInterfaceName, achTemp, achInterfaceIID)==4)
    {
        // first get the line feed character for the next read...
        fgetc(pfileInput);
        // now first convert the iid string

        if (!MultiByteToWideChar(CP_ACP, 0, (const char*)achInterfaceIID, -1, tachIID, MAX_PATH))
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        hr = IIDFromString(tachIID, &iid);
        if (hr)
            goto Error;

        // now open the output file...
        sprintf(achFileOut, "%s\\%s", pstrOutPath, achTemp);

        hr = THR(CreateTearOffTable(pTL, achFileOut, achClassName, achInterfaceName, iid));
        if (hr)
            goto Error;
    }

Error:
    if (pfileInput)
    {
        fclose(pfileInput);
    }
    RRETURN(hr);

}
#endif





//+---------------------------------------------------------------------------
//
//  Function:   MungeVtables
//
//  Synopsis:   Creates vtables implementing IForm on CForm and
//              IControl on CSite.
//
//  Arguments:  [pTL]       -- The type library.
//              [pfileCode] -- The code file.
//
//----------------------------------------------------------------------------

#ifndef PRODUCT_96P
HRESULT
MungeVtables(ITypeLib * pTL, LPSTR pstrOutFile)
{
    static LPSTR pstrIntro =
        "//+------------------------------------------------------------------------\n"
        "//\n"
        "//  Microsoft Forms\n"
        "//  Copyright (C) Microsoft Corporation, 1992 - 1996.\n"
        "//\n"
        "//  File:       fvtbl.cxx\n"
        "//\n"
        "//  Contents:   Vtables for IForm and IControl.\n"
        "//\n"
        "//-------------------------------------------------------------------------\n"
        "\n"
        "#include <headers.hxx>\n"
        "#include \"formkrnl.hxx\"\n"
        "\n"
        ;

    static LPSTR pstrFormBegin =
            "EXTERN_C HRESULT (__stdcall CVoid::*g_apfnIForm[])();\n\n"
            "HRESULT (__stdcall CVoid::*g_apfnIForm[])()=\n"
            "{\n"
            "#ifdef _MAC\n"
            "     NULL,\n"
            "#endif\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)(REFIID, void **))CForm::ThunkQueryInterface,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(ULONG (__stdcall CVoid::*)())CForm::ThunkAddRef,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(ULONG (__stdcall CVoid::*)())CForm::ThunkRelease,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)(UINT *))CForm::GetTypeInfoCount,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)(UINT, LCID, ITypeInfo **))CForm::GetTypeInfo,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)(REFIID, LPOLESTR *, UINT, LCID, DISPID *))CForm::GetIDsOfNames,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)"
            "(DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT*))CForm::Invoke,\n";

    static LPSTR pstrFormMember =
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)(%S))CForm::%S,\n";

    static LPSTR pstrControlBegin =
            "EXTERN_C HRESULT (__stdcall CVoid::*g_apfnIControl[])();\n\n"
            "HRESULT (__stdcall CVoid::*g_apfnIControl[])()=\n"
            "{\n"
            "#ifdef _MAC\n"
            "     NULL,\n"
            "#endif\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)(REFIID, void **))CSite::ThunkQueryInterface,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(ULONG (__stdcall CVoid::*)())CSite::ThunkAddRef,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(ULONG (__stdcall CVoid::*)())CSite::ThunkRelease,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)(UINT *))CSite::GetTypeInfoCount,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)(UINT, LCID, ITypeInfo **))CSite::GetTypeInfo,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)(REFIID, LPOLESTR *, UINT, LCID, DISPID *))CSite::GetIDsOfNames,\n"
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)"
            "(DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT*))CSite::Invoke,\n";

    static LPSTR pstrControlMember =
            "    (HRESULT (__stdcall CVoid::*)())(HRESULT (__stdcall CVoid::*)(%S))CSite::%S,\n";

    HRESULT hr;
    int     ret;
    FILE *  pfile;

    // Open file path\fvtlb.cxx

    strcpy(strrchr(pstrOutFile, '\\') + 1, "fvtbl.cxx");
    pfile = TFAIL(NULL, fopen(pstrOutFile, "w"));
    if (!pfile)
    {
        PrintErrorMsg("Unable to open fvtbl.cxx");
        goto Error;
    }
                   
    // Write file intro.
    ret = TFAIL(-1, fprintf(pfile, pstrIntro));
    if (ret < 0)
        goto Error;

    hr = THR(MungeOneVtable(
            pTL, pfile, IID_IForm, pstrFormBegin, pstrFormMember));
    if (hr)
        goto Cleanup;

    hr = THR(MungeOneVtable(
            pTL, pfile, IID_IControl, pstrControlBegin, pstrControlMember));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pfile)
        Verify(!fclose(pfile));

    RRETURN(hr);

Error:
    hr = E_FAIL;
    goto Cleanup;
}
#endif


//+---------------------------------------------------------------------------
//
//  Function:   MungeTypeLibrary
//
//  Synopsis:   Open typelibrary and write various information to header
//              and code files.
//
//  Arguments:  [pwstrTL]     -- Name of the typelibrary to open.
//              [pstrOutFile] -- Base name of .h and .cxx containing
//                               the methodinfos.
//
//----------------------------------------------------------------------------

HRESULT
MungeTypeLibrary(LPWSTR pwstrTL, LPSTR pstrOutFile, LPSTR pstrOutPath, LPSTR pstrInputFile)
{
    static LPSTR pstrHeaderIntro =
        "//+------------------------------------------------------------------------\n"
        "//\n"
        "//  Microsoft Forms\n"
        "//  Copyright (C) Microsoft Corporation, 1992 - 1996.\n"
        "//\n"
        "//  File:       %s\n"
        "//\n"
        "//  Contents:   Macro definitions used to fire events.\n"
        "//\n"
        "//-------------------------------------------------------------------------\n"
        "\n"
        "#ifndef _F3MUNGE_H_\n"
        "#define _F3MUNGE_H_\n"
        "\n"
        "%s"
        " version of this file should not be used with %s builds!\n"
        "#endif\n\n"
        "#ifndef _F3MUNGE_METHODCOUNTSONLY\n\n"
        ;

    static LPSTR pstrCodeIntro =
        "//+------------------------------------------------------------------------\n"
        "//\n"
        "//  Microsoft Forms\n"
        "//  Copyright (C) Microsoft Corporation, 1992 - 1996.\n"
        "//\n"
        "//  File:       %s\n"
        "//\n"
        "//  Contents:   Contains variable declarations (debug only)\n"
        "//\n"
        "//-------------------------------------------------------------------------\n"
        "\n"
        "#include <headers.hxx>\n"
        "\n"
        "%s"
        " version of this file should not be used with %s builds!\n"
        "#endif\n\n"
        ;

    static LPSTR pstrHeaderTail =
        "\n"
        "#endif   // _F3MUNGE_H_\n"
        ;

    static LPSTR pstrDebugVerify = "#if DBG != 1\n#error The debug";

    static LPSTR pstrShipVerify = "#if DBG == 1\n#error The ship";

    HRESULT     hr;
    char        achFileName[MAX_PATH];
    LPSTR       pstrEndFileName;
    FILE *      pfileHeader = NULL;
    FILE *      pfileCode   = NULL;
    ITypeLib *  pTL         = NULL;
    int         ret;

    // Open header file.
    strcpy(achFileName, pstrOutFile);
    pstrEndFileName = achFileName + strlen(achFileName);
    strcpy(pstrEndFileName, ".h");
    pfileHeader = TFAIL(NULL, fopen(achFileName, "w"));
    if (!pfileHeader)
    {
        PrintErrorMsg("Unable to open .h file.");
        goto Error;
    }

    // Write header intro.
    ret = TFAIL(-1, fprintf(pfileHeader,
                            pstrHeaderIntro,
                            achFileName,
                            ((g_fDebug) ? pstrDebugVerify : pstrShipVerify),
                            ((g_fDebug) ? "ship" : "debug")));
    if (ret < 0)
        goto Error;

    // Open code file.
    strcpy(pstrEndFileName, ".cxx");
    pfileCode = TFAIL(NULL, fopen(achFileName, "w"));
    if (!pfileCode)
    {
        PrintErrorMsg("Unable to open .cxx file.");
        goto Error;
    }

    // Write code intro.
    ret = TFAIL(-1, fprintf(pfileCode,
                            pstrCodeIntro,
                            achFileName,
                            ((g_fDebug) ? pstrDebugVerify : pstrShipVerify),
                            ((g_fDebug) ? "ship" : "debug")));
    if (ret < 0)
        goto Error;

    // Open type library.
    hr = THR(LoadTypeLib(pwstrTL, &pTL));
    if (hr)
    {
        PrintErrorMsg("Unable to open type library.");
        goto Cleanup;
    }

    hr = THR(MungeCoClasses(pTL, pfileHeader, pfileCode));
    if (hr)
        goto Cleanup;

    if (g_fDebug)
    {
        hr = THR(WriteParamVerificationVariables(pfileHeader, pfileCode));
        if (hr)
            goto Cleanup;
    }

#ifndef PRODUCT_96P
    hr = THR(MungeVtables(pTL, pstrOutFile));
    if (hr)
        goto Cleanup;

    if (*pstrOutPath)
    {
        hr = THR(MungeTearOffInterfaces(pTL, pstrOutPath, pstrInputFile));
        if (hr)
            goto Cleanup;
    }

    hr = THR(MungeInterfaceMethodCounts(pTL, pfileHeader));
    if (hr)
        goto Cleanup;
#else
    ret = TFAIL(-1, fprintf(
            pfileHeader,
            "\n#endif // _F3MUNGE_METHODCOUNTSONLY\n"));
    if (ret < 0)
        RRETURN(E_FAIL);
#endif

    // Write header end.
    ret = TFAIL(-1, fprintf(pfileHeader, pstrHeaderTail));
    if (ret < 0)
        goto Error;

Cleanup:
    if (pfileHeader)
        Verify(!fclose(pfileHeader));

    if (pfileCode)
        Verify(!fclose(pfileCode));

    ReleaseInterface(pTL);

    RRETURN(hr);

Error:
    hr = E_FAIL;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Function:   ParseArguments
//
//  Synopsis:   Parse command line arguments.
//              depending on the arguments the method returns:
//              S_OK: normal mungetypelib use
//              S_FALSE: create tearoff interfaces
//              E_FAIL: wrong arguments in commandline
//
//----------------------------------------------------------------------------

HRESULT
ParseArguments(int argc, char ** argv, LPWSTR pwstrTL, LPSTR pstrOutFile, LPSTR pstrOutPath, LPSTR pstrInputFile)
{
    int ret;

    if (argv[1] && strlen(argv[1]) == 2 && argv[1][0] == '/'
        && argv[1][1] == 'D' )
    {
        //
        // Indicates that debug-style information should be generated.
        //
        g_fDebug = TRUE;

        argc--;
        argv++;
    }

    if (argc != 3 && argc != 6)
    {
        goto ShowUsage;
    }

    if (argc == 6)
    {
        if (argv[3] && strlen(argv[3]) == 2 && argv[3][0] == '/'
            && argv[3][1] == 'T' )
        {
            // this is a call to create tear off interface headers....
            strcpy(pstrOutPath, argv[4]);
            strcpy(pstrInputFile, argv[5]);
        }
        else
        {
            goto ShowUsage;
        }
    }
    ret = TW32(0, MultiByteToWideChar(
            CP_ACP, 0, argv[1], -1, pwstrTL, MAX_PATH));
    if (!ret)
        RRETURN(GetLastWin32Error());

    strcpy(pstrOutFile, argv[2]);

    return S_OK;

ShowUsage:
    Usage();
    return E_FAIL;
}



//+---------------------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   Initialize the application.
//
//----------------------------------------------------------------------------

HRESULT
Init()
{
    HRESULT hr;

    hr = THR(OleInitialize(NULL));
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   Deinit
//
//  Synopsis:   Terminate the application.
//
//----------------------------------------------------------------------------

void
Deinit()
{
    SysFreeString(g_bstrAllNames);
    OleUninitialize();
}



//+---------------------------------------------------------------------------
//
//  Function:   main
//
//----------------------------------------------------------------------------

int __cdecl
main(int argc, char ** argv)
{
    HRESULT hr = S_OK;
    WCHAR   awchTL[MAX_PATH];
    char    achOutFile[MAX_PATH];
    char    achInputFile[MAX_PATH];
    char    achOutPath[MAX_PATH];

    __try
    {
        achInputFile[0] = 0;
        achOutPath[0] = 0;

        hr = THR(Init());
        if (hr)
            goto Cleanup;

        hr = THR(ParseArguments(argc, argv, awchTL, achOutFile, achOutPath, achInputFile));

        if (hr)
            goto Cleanup;

        hr = THR(MungeTypeLibrary(awchTL, achOutFile, achOutPath, achInputFile));
        if (hr)
            goto Cleanup;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        printf("Fatal TLMunge exception!");
    }

Cleanup:
    Deinit();

    if (FAILED(hr))
    {
        printf("TLMunge exiting with error %08x\n.", hr);
    }

    RRETURN1(hr, S_FALSE);
}

