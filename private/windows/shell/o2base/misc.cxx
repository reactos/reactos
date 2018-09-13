//+---------------------------------------------------------------------
//
//  File:       misc.cxx
//
//  Contents:   Useful OLE helper and debugging functions
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//#include <rpcferr.h>



//+---------------------------------------------------------------
//
//  Function:   IsCompatibleOleVersion
//
//  Synopsis:   Checks if the installed version of OLE is compatible with
//              the version of OLE for which the software was written.
//
//  Arguments:  [wMaj] -- the major version number
//              [wMin] -- the minor version number
//
//  Returns:    TRUE if the installed version of OLE is compatible
//
//  Notes:      This function combines a call to OleBuildVersion
//              with the proper checking of version numbers.
//              The major and minor values passed in should be
//              the major and minor values returned from
//              OleBuildVersion for the version of OLE current
//              when the software was written.
//
//----------------------------------------------------------------

BOOL
IsCompatibleOleVersion(WORD wMaj, WORD wMin)
{
    // Check our compatibility with the OLE runtime.
    // We are compatible with any later major version,
    // or the same major version with equal or greater minor version.
    DWORD ov = OleBuildVersion();
    return HIWORD(ov) > wMaj || (HIWORD(ov) == wMaj && LOWORD(ov) >= wMin);
}

#if DBG

//+---------------------------------------------------------------
//
//  Function:   AssertSFL
//
//  Synopsis:   Displays "Assertion Failed" message and puts up
//              a message box allowing the user to (1) exit the program,
//              (2) break into the debugger, or (3) ignore
//
//  Arguments:
//              [lpszClause]   -- The assertion clause
//              [lpszFileName] -- File where assertion failed
//              [nLine}        -- Line in file where assertion failed
//
//----------------------------------------------------------------

extern "C" void FAR PASCAL AssertSFL(LPSTR lpszClause,
        LPSTR lpszFileName, int nLine)
{
    static OLECHAR achMessage[] = L"File %hs\n Line %d";
    OLECHAR achTitle[256];
    OLECHAR achFormatBuffer[256];

    wsprintf(achTitle,L"%hs", lpszClause);
    wsprintf(achFormatBuffer, achMessage, lpszFileName, nLine);
    
    DOUT(achFormatBuffer);
    DOUT(L", ");
    DOUT(achTitle);
    DOUT(L"\n\r");
    
retry:
    int nCode = ::MessageBox(NULL, achFormatBuffer, achTitle,
                        MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE);
    if (nCode == IDIGNORE)
        return;     // ignore
    else if (nCode == IDRETRY)
    {
        // break into the debugger (or Dr Watson log)
        DebugBreak();   
        goto retry;
    }                            
    
    // else fall through and exit
    FatalExit(2);
}

#endif // DBG

#if DBG == 1

//+---------------------------------------------------------------
//
//  Function:   TraceIID
//
//  Synopsis:   Outputs the name of the interface to the debugging device
//
//  Arguments:  [riid] -- the interface
//
//  Notes:      This function disappears in retail builds.
//
//----------------------------------------------------------------

void
TraceIID(REFIID riid)
{
    LPWSTR lpstr = L"UNKNOWN INTERFACE";

#define CASE_IID(iid)  \
        if (IsEqualIID(IID_##iid, riid)) lpstr = (LPWSTR)L#iid;

    CASE_IID(IUnknown)
    CASE_IID(IOleLink)
    CASE_IID(IOleCache)
    CASE_IID(IOleManager)
    CASE_IID(IOlePresObj)
    CASE_IID(IDebug)
    CASE_IID(IDebugStream)
    CASE_IID(IAdviseSink2)
    CASE_IID(IDataObject)
    CASE_IID(IViewObject)
    CASE_IID(IOleObject)
    CASE_IID(IOleInPlaceObject)
    CASE_IID(IParseDisplayName)
    CASE_IID(IOleContainer)
    CASE_IID(IOleItemContainer)
    CASE_IID(IOleClientSite)
    CASE_IID(IOleInPlaceSite)
    CASE_IID(IPersist)
    CASE_IID(IPersistStorage)
    CASE_IID(IPersistFile)
    CASE_IID(IPersistStream)
    CASE_IID(IOleClientSite)
    CASE_IID(IOleInPlaceSite)
    CASE_IID(IAdviseSink)
    CASE_IID(IDataAdviseHolder)
    CASE_IID(IOleAdviseHolder)
    CASE_IID(IClassFactory)
    CASE_IID(IOleWindow)
    CASE_IID(IOleInPlaceActiveObject)
    CASE_IID(IOleInPlaceUIWindow)
    CASE_IID(IOleInPlaceFrame)
    CASE_IID(IDropSource)
    CASE_IID(IDropTarget)
    CASE_IID(IBindCtx)
    CASE_IID(IEnumUnknown)
    CASE_IID(IEnumString)
    CASE_IID(IEnumFORMATETC)
    CASE_IID(IEnumSTATDATA)
    CASE_IID(IEnumOLEVERB)
    CASE_IID(IEnumMoniker)
    CASE_IID(IEnumGeneric)
    CASE_IID(IEnumHolder)
    CASE_IID(IEnumCallback)
    CASE_IID(ILockBytes)
    CASE_IID(IStorage)
    CASE_IID(IStream)
    //CASE_IID(IDispatch)
    //CASE_IID(IEnumVARIANT)
    //CASE_IID(ITypeInfo)
    //CASE_IID(ITypeLib)
    //CASE_IID(ITypeComp)
    //CASE_IID(ICreateTypeInfo)
    //CASE_IID(ICreateTypeLib)

#undef CASE_IID

    OLECHAR achTemp[256];
    wsprintf(achTemp, L"%ws", lpstr);
    DOUT( achTemp );
}


//+---------------------------------------------------------------
//
//  Function:   TraceHRESULT
//
//  Synopsis:   Outputs the name of the SCODE to the debugging device
//
//  Arguments:  [scode] -- the status code to report
//
//  Notes:      This function disappears in retail builds.
//
//----------------------------------------------------------------

HRESULT
TraceHRESULT(HRESULT r)
{
    LPWSTR lpstr;

#define CASE_SCODE(sc)  \
        case sc: lpstr = (LPWSTR)L#sc; break;

    switch (r) {
        /* SCODE's defined in SCODE.H */
        CASE_SCODE(S_OK)
 // same value as S_OK      CASE_SCODE(S_TRUE)
        CASE_SCODE(S_FALSE)
        CASE_SCODE(E_UNEXPECTED)
        CASE_SCODE(E_NOTIMPL)
        CASE_SCODE(E_OUTOFMEMORY)
        CASE_SCODE(E_INVALIDARG)
        CASE_SCODE(E_NOINTERFACE)
        CASE_SCODE(E_POINTER)
        CASE_SCODE(E_HANDLE)
        CASE_SCODE(E_ABORT)
        CASE_SCODE(E_FAIL)
        CASE_SCODE(E_ACCESSDENIED)

        /* SCODE's defined in DVOBJ.H */
        CASE_SCODE(DATA_E_FORMATETC)
// same as DATA_E_FORMATETC     CASE_SCODE(DV_E_FORMATETC)
        CASE_SCODE(DATA_S_SAMEFORMATETC)
        CASE_SCODE(VIEW_E_DRAW)
//  same as VIEW_E_DRAW         CASE_SCODE(E_DRAW)
        CASE_SCODE(VIEW_S_ALREADY_FROZEN)
        CASE_SCODE(CACHE_E_NOCACHE_UPDATED)
        CASE_SCODE(CACHE_S_FORMATETC_NOTSUPPORTED)
        CASE_SCODE(CACHE_S_SAMECACHE)
        CASE_SCODE(CACHE_S_SOMECACHES_NOTUPDATED)

        /* SCODE's defined in OLE2.H */
        CASE_SCODE(OLE_E_OLEVERB)
        CASE_SCODE(OLE_E_ADVF)
        CASE_SCODE(OLE_E_ENUM_NOMORE)
        CASE_SCODE(OLE_E_ADVISENOTSUPPORTED)
        CASE_SCODE(OLE_E_NOCONNECTION)
        CASE_SCODE(OLE_E_NOTRUNNING)
        CASE_SCODE(OLE_E_NOCACHE)
        CASE_SCODE(OLE_E_BLANK)
        CASE_SCODE(OLE_E_CLASSDIFF)
        CASE_SCODE(OLE_E_CANT_GETMONIKER)
        CASE_SCODE(OLE_E_CANT_BINDTOSOURCE)
        CASE_SCODE(OLE_E_STATIC)
        CASE_SCODE(OLE_E_PROMPTSAVECANCELLED)
        CASE_SCODE(OLE_E_INVALIDRECT)
        CASE_SCODE(OLE_E_WRONGCOMPOBJ)
        CASE_SCODE(OLE_E_INVALIDHWND)
        CASE_SCODE(DV_E_DVTARGETDEVICE)
        CASE_SCODE(DV_E_STGMEDIUM)
        CASE_SCODE(DV_E_STATDATA)
        CASE_SCODE(DV_E_LINDEX)
        CASE_SCODE(DV_E_TYMED)
        CASE_SCODE(DV_E_CLIPFORMAT)
        CASE_SCODE(DV_E_DVASPECT)
        CASE_SCODE(DV_E_DVTARGETDEVICE_SIZE)
        CASE_SCODE(DV_E_NOIVIEWOBJECT)
        CASE_SCODE(OLE_S_USEREG)
        CASE_SCODE(OLE_S_STATIC)
        CASE_SCODE(OLE_S_MAC_CLIPFORMAT)
        CASE_SCODE(CONVERT10_E_OLESTREAM_GET)
        CASE_SCODE(CONVERT10_E_OLESTREAM_PUT)
        CASE_SCODE(CONVERT10_E_OLESTREAM_FMT)
        CASE_SCODE(CONVERT10_E_OLESTREAM_BITMAP_TO_DIB)
        CASE_SCODE(CONVERT10_E_STG_FMT)
        CASE_SCODE(CONVERT10_E_STG_NO_STD_STREAM)
        CASE_SCODE(CONVERT10_E_STG_DIB_TO_BITMAP)
        CASE_SCODE(CONVERT10_S_NO_PRESENTATION)
        CASE_SCODE(CLIPBRD_E_CANT_OPEN)
        CASE_SCODE(CLIPBRD_E_CANT_EMPTY)
        CASE_SCODE(CLIPBRD_E_CANT_SET)
        CASE_SCODE(CLIPBRD_E_BAD_DATA)
        CASE_SCODE(CLIPBRD_E_CANT_CLOSE)
        CASE_SCODE(DRAGDROP_E_NOTREGISTERED)
        CASE_SCODE(DRAGDROP_E_ALREADYREGISTERED)
        CASE_SCODE(DRAGDROP_E_INVALIDHWND)
        CASE_SCODE(DRAGDROP_S_DROP)
        CASE_SCODE(DRAGDROP_S_CANCEL)
        CASE_SCODE(DRAGDROP_S_USEDEFAULTCURSORS)
        CASE_SCODE(OLEOBJ_E_NOVERBS)
        CASE_SCODE(OLEOBJ_S_INVALIDVERB)
        CASE_SCODE(OLEOBJ_S_CANNOT_DOVERB_NOW)
        CASE_SCODE(OLEOBJ_S_INVALIDHWND)
        CASE_SCODE(INPLACE_E_NOTUNDOABLE)
        CASE_SCODE(INPLACE_E_NOTOOLSPACE)
        CASE_SCODE(INPLACE_S_TRUNCATED)

        /* SCODE's defined in STORAGE.H */
        CASE_SCODE(STG_E_INVALIDFUNCTION)
        CASE_SCODE(STG_E_FILENOTFOUND)
        CASE_SCODE(STG_E_PATHNOTFOUND)
        CASE_SCODE(STG_E_TOOMANYOPENFILES)
        CASE_SCODE(STG_E_ACCESSDENIED)
        CASE_SCODE(STG_E_INVALIDHANDLE)
        CASE_SCODE(STG_E_INSUFFICIENTMEMORY)
        CASE_SCODE(STG_E_INVALIDPOINTER)
        CASE_SCODE(STG_E_NOMOREFILES)
        CASE_SCODE(STG_E_DISKISWRITEPROTECTED)
        CASE_SCODE(STG_E_SEEKERROR)
        CASE_SCODE(STG_E_WRITEFAULT)
        CASE_SCODE(STG_E_READFAULT)
        CASE_SCODE(STG_E_LOCKVIOLATION)
        CASE_SCODE(STG_E_FILEALREADYEXISTS)
        CASE_SCODE(STG_E_INVALIDPARAMETER)
        CASE_SCODE(STG_E_MEDIUMFULL)
        CASE_SCODE(STG_E_ABNORMALAPIEXIT)
        CASE_SCODE(STG_E_INVALIDHEADER)
        CASE_SCODE(STG_E_INVALIDNAME)
        CASE_SCODE(STG_E_UNKNOWN)
        CASE_SCODE(STG_E_UNIMPLEMENTEDFUNCTION)
        CASE_SCODE(STG_E_INVALIDFLAG)
        CASE_SCODE(STG_E_INUSE)
        CASE_SCODE(STG_E_NOTCURRENT)
        CASE_SCODE(STG_E_REVERTED)
        CASE_SCODE(STG_E_CANTSAVE)
        CASE_SCODE(STG_E_OLDFORMAT)
        CASE_SCODE(STG_E_OLDDLL)
        CASE_SCODE(STG_E_SHAREREQUIRED)
        CASE_SCODE(STG_S_CONVERTED)
        //CASE_SCODE(STG_S_BUFFEROVERFLOW)
        //CASE_SCODE(STG_S_TRYOVERWRITE)

        /* SCODE's defined in COMPOBJ.H */
        CASE_SCODE(CO_E_NOTINITIALIZED)
        CASE_SCODE(CO_E_ALREADYINITIALIZED)
        CASE_SCODE(CO_E_CANTDETERMINECLASS)
        CASE_SCODE(CO_E_CLASSSTRING)
        CASE_SCODE(CO_E_IIDSTRING)
        CASE_SCODE(CO_E_APPNOTFOUND)
        CASE_SCODE(CO_E_APPSINGLEUSE)
        CASE_SCODE(CO_E_ERRORINAPP)
        CASE_SCODE(CO_E_DLLNOTFOUND)
        CASE_SCODE(CO_E_ERRORINDLL)
        CASE_SCODE(CO_E_WRONGOSFORAPP)
        CASE_SCODE(CO_E_OBJNOTREG)
        CASE_SCODE(CO_E_OBJISREG)
        CASE_SCODE(CO_E_OBJNOTCONNECTED)
        CASE_SCODE(CO_E_APPDIDNTREG)
        CASE_SCODE(CLASS_E_NOAGGREGATION)
        CASE_SCODE(REGDB_E_READREGDB)
        CASE_SCODE(REGDB_E_WRITEREGDB)
        CASE_SCODE(REGDB_E_KEYMISSING)
        CASE_SCODE(REGDB_E_INVALIDVALUE)
        CASE_SCODE(REGDB_E_CLASSNOTREG)
        CASE_SCODE(REGDB_E_IIDNOTREG)
#if defined(NO_NTOLEBUGS)
        CASE_SCODE(RPC_E_SERVER_DIED)
        CASE_SCODE(RPC_E_CALL_REJECTED)
        CASE_SCODE(RPC_E_CALL_CANCELED)
        CASE_SCODE(RPC_E_CANTPOST_INSENDCALL)
        CASE_SCODE(RPC_E_CANTCALLOUT_INASYNCCALL)
        CASE_SCODE(RPC_E_CANTCALLOUT_INEXTERNALCALL)
        CASE_SCODE(RPC_E_CONNECTION_TERMINATED)
        CASE_SCODE(RPC_E_CLIENT_DIED)
        CASE_SCODE(RPC_E_INVALID_DATAPACKET)
        CASE_SCODE(RPC_E_CANTTRANSMIT_CALL)
        CASE_SCODE(RPC_E_CLIENT_CANTMARSHAL_DATA)
        CASE_SCODE(RPC_E_CLIENT_CANTUNMARSHAL_DATA)
        CASE_SCODE(RPC_E_SERVER_CANTMARSHAL_DATA)
        CASE_SCODE(RPC_E_SERVER_CANTUNMARSHAL_DATA)
        CASE_SCODE(RPC_E_INVALID_DATA)
        CASE_SCODE(RPC_E_INVALID_PARAMETER)
        CASE_SCODE(RPC_E_UNEXPECTED)
#endif // NO_NTOLEBUGS

        /* SCODE's defined in MONIKER.H */
        CASE_SCODE(MK_E_CONNECTMANUALLY)
        CASE_SCODE(MK_E_EXCEEDEDDEADLINE)
        CASE_SCODE(MK_E_NEEDGENERIC)
        CASE_SCODE(MK_E_UNAVAILABLE)
        CASE_SCODE(MK_E_SYNTAX)
        CASE_SCODE(MK_E_NOOBJECT)
        CASE_SCODE(MK_E_INVALIDEXTENSION)
        CASE_SCODE(MK_E_INTERMEDIATEINTERFACENOTSUPPORTED)
        CASE_SCODE(MK_E_NOTBINDABLE)
        CASE_SCODE(MK_E_NOTBOUND)
        CASE_SCODE(MK_E_CANTOPENFILE)
        CASE_SCODE(MK_E_MUSTBOTHERUSER)
        CASE_SCODE(MK_E_NOINVERSE)
        CASE_SCODE(MK_E_NOSTORAGE)
        CASE_SCODE(MK_S_REDUCED_TO_SELF)
        CASE_SCODE(MK_S_ME)
        CASE_SCODE(MK_S_HIM)
        CASE_SCODE(MK_S_US)
#if defined(NO_NTOLEBUGS)
        CASE_SCODE(MK_S_MONIKERALREADYREGISTERED)
#endif //NO_NTOLEBUGS

    default:
        lpstr = L" <UNKNOWN SCODE  0x%lx>\n";
        break;
    }

#undef CASE_SCODE

    OLECHAR achTemp[256];
    wsprintf(achTemp, L"%ws", lpstr);
    DOUT( achTemp );
    return r;
}

#endif  // DBG == 1
