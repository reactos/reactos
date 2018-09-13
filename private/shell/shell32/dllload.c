#include "shellprv.h"

#define OLEDBVER 0x0250
#include <oledb.h>
#include <cmdtree.h>
#include <oleext.h>
#include <ntquery.h>
#include <urlmon.h>
#include <imagehlp.h>
#include <dbgmem.h>
#include <userenv.h>
#include <activeds.h>
#include <lm.h>
#include <cscapi.h>

#define _DSGETDCAPI_
#include <dsgetdc.h>

#define _NTDSAPI_
#include <ntdsapi.h>
#undef NTDSAPI          // needed as we reference this later

#pragma  hdrstop

#include "uemapp.h"

#include "..\lib\dllload.c"

// -------- SHDOCVW.DLL --------

HMODULE g_hmodShdocvw = NULL;


DELAY_LOAD_BOOL(g_hmodShdocvw, SHDOCVW, DllRegisterWindowClasses,
                (const SHDRC * pshdrc),
                (pshdrc));

DELAY_LOAD_HRESULT(g_hmodShdocvw, SHDOCVW, SHGetIDispatchForFolder,
                (LPCITEMIDLIST pidl, IWebBrowserApp ** ppauto),
                (pidl, ppauto));

DELAY_LOAD_HRESULT(g_hmodShdocvw, SHDOCVW, URLQualifyA,
                ( LPCSTR pszURL, DWORD dwFlags, LPSTR * ppszOut ),
                ( pszURL, dwFlags, ppszOut ));

DELAY_LOAD_HRESULT(g_hmodShdocvw, SHDOCVW, URLQualifyW,
                ( LPCWSTR pszURL, DWORD dwFlags, LPWSTR * ppszOut ),
                ( pszURL, dwFlags, ppszOut ));

DELAY_LOAD_DWORD(g_hmodShdocvw, SHDOCVW, SoftwareUpdateMessageBox,
                ( HWND hWnd, LPCWSTR szDistUnit, DWORD dwFlags, LPSOFTDISTINFO psdi ),
                ( hWnd, szDistUnit, dwFlags, psdi ));

DELAY_LOAD_ORD(g_hmodShdocvw, SHDOCVW, HRESULT, _GetStdLocation, 150,
                ( LPWSTR pwszURL, DWORD cbPathSize, UINT id ),
                ( pwszURL, cbPathSize, id ));

#if !defined(POSTPOSTSPLIT)

//
// Temporary hack for shdocvw/browseui split.  This should be fixed eventually.
//

DELAY_LOAD_ORD(g_hmodShdocvw, SHDOCVW, BOOL,
            ParseURLFromOutsideSourceA, 169,
            (LPCSTR psz, LPSTR pszOut, LPDWORD pcchOut, LPBOOL pbWasSearchURL),
            (psz, pszOut, pcchOut, pbWasSearchURL));

DELAY_LOAD_ORD(g_hmodShdocvw, SHDOCVW, BOOL,
            ParseURLFromOutsideSourceW, 170,
            (LPCWSTR psz, LPWSTR pszOut, LPDWORD pcchOut, LPBOOL pbWasSearchURL),
            (psz, pszOut, pcchOut, pbWasSearchURL));

DELAY_LOAD_ORD_ERR(g_hmodShdocvw, SHDOCVW, HRESULT,
            NavToUrlUsingIEA, 203,
            (LPCSTR wszUrl, BOOL fNewWindow),
            (wszUrl, fNewWindow),
            E_FAIL);

DELAY_LOAD_ORD_ERR(g_hmodShdocvw, SHDOCVW, HRESULT,
            NavToUrlUsingIEW, 204,
            (LPCWSTR wszUrl, BOOL fNewWindow),
            (wszUrl, fNewWindow),
            E_FAIL);

DELAY_LOAD_ORD_ERR(g_hmodShdocvw, SHDOCVW, HRESULT,
            URLSubLoadString, 138,
            (HMODULE hInst, UINT idRes, LPWSTR pszUrlOut,
             DWORD cchSizeOut, DWORD dwSubstitutions),
            (hInst, idRes, pszUrlOut, cchSizeOut, dwSubstitutions),
            E_FAIL);

DELAY_LOAD_ORD(g_hmodShdocvw, SHDOCVW, DWORD,
            SHRestricted2A, 158,
            (RESTRICTIONS rest, LPCSTR pszUrl, DWORD dwReserved),
            (rest, pszUrl, dwReserved));

DELAY_LOAD_ORD(g_hmodShdocvw, SHDOCVW, DWORD,
            SHRestricted2W, 159,
            (RESTRICTIONS rest, LPCWSTR pszUrl, DWORD dwReserved),
            (rest, pszUrl, dwReserved));

#endif

DELAY_LOAD_ORD(g_hmodShdocvw, SHDOCVW, BOOL, GetLeakDetectionFunctionTable, 161,
               (LEAKDETECTFUNCS *pTable), (pTable));


DELAY_LOAD_VOID_ORD(g_hmodShdocvw, SHDOCVW, IEOnFirstBrowserCreation, 195,
                (IUnknown* punk), (punk));

//---------- ACTIVEDS.DLL --------------

HMODULE g_hmodActiveDS = NULL;

DELAY_LOAD_HRESULT(g_hmodActiveDS, ACTIVEDS, ADsOpenObject,
                    (LPCWSTR lpszPathName, LPCWSTR lpszUserName, LPCWSTR lpszPassword, DWORD  dwReserved,
                     REFIID riid, void **ppObject),
                    (lpszPathName, lpszUserName, lpszPassword, dwReserved, riid, ppObject));

//---------- BROWSEUI.DLL --------------

//
//--- delay load browseui functions

HMODULE g_hmodBrowseui = NULL;

DELAY_LOAD_IE_ORD(g_hmodBrowseui, BROWSEUI, BOOL, SHOnCWMCommandLine, 127,
                (LPARAM lParam), (lParam));

DELAY_LOAD_IE_ORD(g_hmodBrowseui, BROWSEUI, BOOL, SHOpenFolderWindow, 102,
                  (IETHREADPARAM* pieiIn),
                  (pieiIn));

DELAY_LOAD_IE_HRESULT(g_hmodBrowseui, BROWSEUI,
                  SHGetSetDefFolderSettings, 107,
                  (DEFFOLDERSETTINGS *pdfs, int cbDfs, UINT flags),
                  (pdfs, cbDfs, flags));

DELAY_LOAD_IE_ORD_VOID(g_hmodBrowseui, BROWSEUI,
                  SHCreateSavedWindows, 105, (), ());

DELAY_LOAD_IE_ORD(g_hmodBrowseui, BROWSEUI, HRESULT,
                  SHCreateBandForPidl, 120,
                  (LPCITEMIDLIST pidl, IUnknown** ppunk, BOOL fAllowBrowserBand),
                  (pidl, ppunk, fAllowBrowserBand));

DELAY_LOAD_IE_ORD(g_hmodBrowseui, BROWSEUI, HRESULT,
                  SHPidlFromDataObject, 121,
                  (IDataObject *pdtobj, LPITEMIDLIST * ppidlTarget, LPWSTR pszDisplayName,DWORD cchDisplayName),
                  (pdtobj, ppidlTarget, pszDisplayName, cchDisplayName));

DELAY_LOAD_IE_ORD(g_hmodBrowseui, BROWSEUI, DWORD,
                  IDataObject_GetDeskBandState, 122,
                  (IDataObject *pdtobj),
                  (pdtobj));

DELAY_LOAD_IE_ORD(g_hmodBrowseui, BROWSEUI, IETHREADPARAM*,
                  SHCreateIETHREADPARAM, 123,
                  (LPCWSTR pszCmdLineIn, int nCmdShowIn, ITravelLog *ptlIn, IEFreeThreadedHandShake* piehsIn),
                  (pszCmdLineIn, nCmdShowIn, ptlIn, piehsIn));

DELAY_LOAD_IE_ORD_VOID(g_hmodBrowseui, BROWSEUI,
                  SHDestroyIETHREADPARAM, 126,
                  (IETHREADPARAM* pieiIn),
                  (pieiIn));

DELAY_LOAD_IE_ORD(g_hmodBrowseui, BROWSEUI, BOOL,
                  SHParseIECommandLine, 125,
                  (LPCWSTR * ppszCmdLine, IETHREADPARAM * piei),
                  (ppszCmdLine, piei));

DELAY_LOAD_IE_ORD(g_hmodBrowseui, BROWSEUI, HRESULT,
                  Channel_QuickLaunch, 133, (void),());


// -------- OLEAUT32.DLL --------
HMODULE g_hmodOLEAUT32 = NULL;

DELAY_LOAD(g_hmodOLEAUT32, OLEAUT32, BSTR, SysAllocString,
    (const WCHAR *pch), (pch));

DELAY_LOAD(g_hmodOLEAUT32, OLEAUT32, BSTR, SysAllocStringLen,
    (const WCHAR *pch, unsigned int i), (pch, i));

DELAY_LOAD_VOID(g_hmodOLEAUT32, OLEAUT32, SysFreeString, (BSTR bs), (bs));

DELAY_LOAD(g_hmodOLEAUT32, OLEAUT32, BSTR, SysAllocStringByteLen,
    (LPCSTR psz, unsigned int len), (psz, len));

DELAY_LOAD_UINT(g_hmodOLEAUT32, OLEAUT32, SysStringLen,
    (BSTR str), (str));

DELAY_LOAD_UINT(g_hmodOLEAUT32, OLEAUT32, SysStringByteLen,
    (BSTR str), (str));

DELAY_LOAD_INT(g_hmodOLEAUT32, OLEAUT32, DosDateTimeToVariantTime,
    (USHORT wDosDate, USHORT wDosTime, DOUBLE * pvtime), (wDosDate, wDosTime, pvtime));

DELAY_LOAD_INT(g_hmodOLEAUT32, OLEAUT32, VariantTimeToDosDateTime,
    (DOUBLE vtime, USHORT *pwDosDate, USHORT *pwDosTime), (vtime, pwDosDate, pwDosTime));

#undef VariantClear

DELAY_LOAD_HRESULT(g_hmodOLEAUT32, OLEAUT32, VariantClear,
    (VARIANTARG *pvarg), (pvarg));

DELAY_LOAD_HRESULT(g_hmodOLEAUT32, OLEAUT32, VariantCopy,
    (VARIANTARG * pvargDest, VARIANTARG * pvargSrc), (pvargDest, pvargSrc));

DELAY_LOAD_HRESULT(g_hmodOLEAUT32, OLEAUT32, VariantChangeType,
    (VARIANTARG * pvargDest, VARIANTARG * pvarSrc, USHORT wFlags, VARTYPE vt), 
    (pvargDest, pvarSrc, wFlags, vt));

DELAY_LOAD_SAFEARRAY(g_hmodOLEAUT32, OLEAUT32, SafeArrayCreateVector,
    (VARTYPE vt, long iBound, ULONG cElements), (vt, iBound, cElements) );

DELAY_LOAD_HRESULT(g_hmodOLEAUT32, OLEAUT32, CreateErrorInfo,
    (ICreateErrorInfo **pperrinfo), (pperrinfo));

DELAY_LOAD_HRESULT(g_hmodOLEAUT32, OLEAUT32, LoadRegTypeLib,
    (REFGUID rguid, unsigned short wVerMajor, unsigned short wVerMinor, LCID lcid, ITypeLib **pptlib), 
    (rguid, wVerMajor, wVerMinor, lcid, pptlib));

DELAY_LOAD_HRESULT(g_hmodOLEAUT32, OLEAUT32, SetErrorInfo,
    (DWORD  dwReserved, IErrorInfo  *perrinfo), (dwReserved, perrinfo));

DELAY_LOAD_HRESULT(g_hmodOLEAUT32, OLEAUT32, RegisterTypeLib,
    (ITypeLib *ptlib, WCHAR *szFullPath, WCHAR *szHelpDir),
    (ptlib, szFullPath, szHelpDir));

DELAY_LOAD_HRESULT(g_hmodOLEAUT32, OLEAUT32, LoadTypeLib,
    (const WCHAR *szFile, ITypeLib **pptlib), (szFile, pptlib));

DELAY_LOAD(g_hmodOLEAUT32, OLEAUT32, INT, SystemTimeToVariantTime,
    (LPSYSTEMTIME pst, double *pvtime), (pst, pvtime));

DELAY_LOAD(g_hmodOLEAUT32, OLEAUT32, INT, VariantTimeToSystemTime,
    (DOUBLE vtime, LPSYSTEMTIME lpSystemTime), (vtime, lpSystemTime));

DELAY_LOAD_HRESULT(g_hmodOLEAUT32, OLEAUT32, VarI4FromStr,
    (OLECHAR * strIn, LCID lcid, DWORD dwFlags, LONG * plOut), (strIn, lcid, dwFlags, plOut));


// -----------ole32.dll---------------
HMODULE g_hmodOLE32 = NULL;

#undef PropVariantClear

DELAY_LOAD_HRESULT( g_hmodOLE32, OLE32, PropVariantClear, (PROPVARIANT * pvar), (pvar));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoGetClassObject,
    (REFCLSID clsid, DWORD dwContext, LPWSTR pszRemote, REFIID riid, void ** ppv), (clsid, dwContext, pszRemote, riid, ppv));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoCreateInstance,
    (REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, void ** ppv), (rclsid, pUnkOuter, dwClsContext, riid, ppv));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleRun, (IUnknown *punk), (punk));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CreateBindCtx, (DWORD dw, LPBC *ppbc), (dw, ppbc));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleInitialize, (void *pv), (pv));
DELAY_LOAD_VOID(g_hmodOLE32, OLE32, OleUninitialize, (void), ());

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoInitialize, (void *pv), (pv));
// BUGBUG -- Win9x doesn't have CoInitializeEx
DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoInitializeEx, (void *pv, DWORD dw), (pv, dw));
DELAY_LOAD_VOID(g_hmodOLE32, OLE32, CoUninitialize, (void), ());
DELAY_LOAD_VOID(g_hmodOLE32, OLE32, CoFreeUnusedLibraries, (void), ());

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CreateStreamOnHGlobal,
                ( HGLOBAL hGlobal, BOOL fRelease, LPSTREAM *ppstm),
                ( hGlobal, fRelease, ppstm ));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CreateDataAdviseHolder,
                (LPDATAADVISEHOLDER *ppDAHolder), (ppDAHolder));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoGetInterfaceAndReleaseStream ,
                   (LPSTREAM pStm, REFIID riid, void ** ppv), (pStm, riid, ppv));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoMarshalInterThreadInterfaceInStream,
                   (REFIID riid, IUnknown *punk, LPSTREAM *ppStm), (riid, punk, ppStm));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, StgCreateDocfile,
                   (const WCHAR *pwcsName, DWORD grfMode, DWORD reserved, IStorage **ppstgOpen), (pwcsName, grfMode, reserved, ppstgOpen));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, StgOpenStorage,
                   (const WCHAR *pwcsName, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstgOpen), (pwcsName, pstgPriority, grfMode, snbExclude, reserved, ppstgOpen));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, StgOpenStorageEx,
                   (const WCHAR *pwcsName, DWORD grfMode, DWORD stgfmt, DWORD grfAttrs, STGOPTIONS * pStgOptions, void * reserved2, REFIID riid, void ** ppObjectOpen), 
                   (pwcsName, grfMode, stgfmt, grfAttrs, pStgOptions, reserved2, riid, ppObjectOpen));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, StgIsStorageFile,
                   (const WCHAR *pwcsName), (pwcsName));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, FmtIdToPropStgName,
                   (const FMTID* pfmtid, LPOLESTR oszName), (pfmtid, oszName));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, StgOpen,
                   (const WCHAR *pwcsName), (pwcsName));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleQueryCreateFromData,
                   (IDataObject *pSrcDataObj), (pSrcDataObj));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleQueryLinkFromData,
                   (IDataObject *pSrcDataObj), (pSrcDataObj));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleRegGetMiscStatus,
                   (REFCLSID clsid, DWORD dwAspect, DWORD *pdwStatus),
                   (clsid, dwAspect, pdwStatus));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, GetClassFile,
                   (const WCHAR *pwcs, CLSID *pclsid), (pwcs, pclsid));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleSetClipboard,
                   (IDataObject *pSrcDataObj), (pSrcDataObj));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleGetClipboard,
                   (IDataObject **ppSrcDataObj), (ppSrcDataObj));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleFlushClipboard,
                   (), ());

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, DoDragDrop,
                   (IDataObject *pdata, IDropSource *pdsrc, DWORD dwEffect, DWORD *pdwEffect), (pdata, pdsrc, dwEffect, pdwEffect));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, RevokeDragDrop,
                   (HWND hwnd), (hwnd));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, RegisterDragDrop,
                   (HWND hwnd, LPDROPTARGET pDropTarget), (hwnd, pDropTarget));

DELAY_LOAD_VOID(g_hmodOLE32, OLE32, ReleaseStgMedium,
                   (LPSTGMEDIUM pmedium), (pmedium));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoGetMalloc,
                   (DWORD dw, IMalloc **ppmem), (dw, ppmem));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleRegGetUserType,
        (REFCLSID clsid, DWORD dwFormOfType, LPWSTR * pszUserType),
        (clsid, dwFormOfType, pszUserType));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleRegEnumVerbs,
    (REFCLSID clsid, LPENUMOLEVERB * ppenum),
    (clsid, ppenum));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, WriteClassStm,
    (LPSTREAM pStm, REFCLSID rclsid),
    (pStm, rclsid));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleLoadFromStream,
    (LPSTREAM pStm, REFIID iidInterface, void ** ppvObj),
    (pStm, iidInterface, ppvObj));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, OleSaveToStream,
    (LPPERSISTSTREAM pPStm, LPSTREAM pStm ),
    (pPStm, pStm ));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoMarshalInterface,
    (LPSTREAM pStm, REFIID riid, LPUNKNOWN pUnk, DWORD dwDestContext,
     void *pvDestContext, DWORD mshlflags),
    (pStm, riid, pUnk, dwDestContext, pvDestContext, mshlflags));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoUnmarshalInterface,
    (LPSTREAM pStm, REFIID riid, void ** ppv),
    (pStm, riid, ppv));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoReleaseMarshalData,
    (LPSTREAM pStm),
    (pStm));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, ProgIDFromCLSID,
    (REFCLSID clsid, LPWSTR * ppszProgID),
    (clsid, ppszProgID));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoRegisterClassObject,
    (REFCLSID rclsid, LPUNKNOWN pUnk, DWORD dwClsContext,
     DWORD flags, LPDWORD lpdwRegister),
    (rclsid, pUnk, dwClsContext, flags, lpdwRegister));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CoRevokeClassObject,
    (DWORD dwRegister),
    (dwRegister));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CreateOleAdviseHolder,
    (LPOLEADVISEHOLDER * ppOAHolder),
    (ppOAHolder));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CreateFileMoniker,
    (LPCOLESTR lpszPathName, LPMONIKER *ppmk),
    (lpszPathName, ppmk));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, StringFromCLSID,
    (IN REFCLSID rclsid, OUT LPOLESTR *lplpsz),
    (rclsid, lplpsz));
    
DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, StgCreateDocfileOnILockBytes,
    (ILockBytes * plkbyt, DWORD grfMode, DWORD reserved, IStorage ** ppstgOpen),
    (plkbyt, grfMode, reserved, ppstgOpen));
    
DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32, CreateILockBytesOnHGlobal,
    (HGLOBAL hGlobal, BOOL fDeleteOnRelease, ILockBytes ** ppLkbyt),
    (hGlobal, fDeleteOnRelease, ppLkbyt));

DELAY_LOAD_LPVOID(g_hmodOLE32, OLE32, CoTaskMemAlloc, (SIZE_T cb), (cb));
DELAY_LOAD_VOID(g_hmodOLE32, OLE32, CoTaskMemFree, (void *pv), (pv));

// -----------oleacc.dll---------------
HMODULE g_hmodOLEACC = NULL;
DELAY_LOAD_LRESULT(g_hmodOLEACC, OLEACC, LresultFromObject, 
                   (REFIID riid, WPARAM wParam, LPUNKNOWN punk),
                   (riid, wParam, punk));
DELAY_LOAD_HRESULT(g_hmodOLEACC, OLEACC, AccessibleObjectFromWindow, 
                   (HWND hwnd, DWORD idObject, REFIID riid, void** ppvObj),
                   (hwnd, idObject, riid, ppvObj));
DELAY_LOAD_HRESULT(g_hmodOLEACC, OLEACC, CreateStdAccessibleObject, 
                   (HWND hwnd, LONG idObject, REFIID riid, void** ppvObj),
                   (hwnd, idObject, riid, ppvObj));


// -------------------- linkinfo.dll ----------------------
HMODULE g_hmodLinkinfo = NULL;

#define DELAY_LINKINFO_BOOL(_fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(g_hmodLinkinfo, LINKINFO, BOOL, _fn, _fni, _args, _nargs, FALSE)
#define DELAY_LINKINFO_VOID(_fn, _args, _nargs) DELAY_LOAD_NAME_VOID(g_hmodLinkinfo, LINKINFO, _fn, _fn, _args, _nargs)

#ifdef UNICODE
DELAY_LINKINFO_BOOL(CreateLinkInfo, CreateLinkInfo, (LPCTSTR psz, PLINKINFO *ppli), (psz, ppli));
DELAY_LINKINFO_BOOL(ResolveLinkInfo, ResolveLinkInfo,
                (PCLINKINFO pli, LPTSTR psz, DWORD dw1, HWND hwnd, PDWORD pdw2, PLINKINFO *ppli),
                (pli, psz, dw1, hwnd, pdw2, ppli));
#else
#undef CreateLinkInfo
#undef ResolveLinkInfo
DELAY_LINKINFO_BOOL(CreateLinkInfoA, CreateLinkInfo, (LPCTSTR psz, PLINKINFO *ppli), (psz, ppli));
DELAY_LINKINFO_BOOL(ResolveLinkInfoA, ResolveLinkInfo,
                (PCLINKINFO pli, LPTSTR psz, DWORD dw1, HWND hwnd, PDWORD pdw2, PLINKINFO *ppli),
                (pli, psz, dw1, hwnd, pdw2, ppli));
#endif

DELAY_LINKINFO_VOID(DestroyLinkInfo, (PLINKINFO pli), (pli));
DELAY_LINKINFO_BOOL(GetLinkInfoData, GetLinkInfoData,
                (PCLINKINFO pli, LINKINFODATATYPE lidt, const VOID **ppv), 
                (pli, lidt, ppv));
                                                                                      
#ifdef WINNT
// -----------query.dll---------------
HMODULE g_hmodQuery = NULL;

DELAY_LOAD_HRESULT(g_hmodQuery, QUERY, CIMakeICommand,
    (ICommand **ppQuery, ULONG cScope, DWORD const *aDepths, WCHAR const * const *awcsScope, WCHAR const * const *awcsCat, WCHAR const * const *awcsMachine),
    (ppQuery, cScope, aDepths, awcsScope, awcsCat, awcsMachine));

DELAY_LOAD_HRESULT(g_hmodQuery, QUERY, CITextToFullTreeEx,
    (WCHAR const *pwszRestriction, ULONG ulDialect, WCHAR const *pwszColumns, WCHAR const *pwszSortColumns, WCHAR const *pwszGroupings, DBCOMMANDTREE **ppTree, ULONG cProperties, CIPROPERTYDEF *pReserved, LCID LocaleID),
    (pwszRestriction, ulDialect, pwszColumns, pwszSortColumns, pwszGroupings, ppTree, cProperties, pReserved, LocaleID));

DELAY_LOAD_HRESULT(g_hmodQuery, QUERY, LocateCatalogsW,
    (WCHAR const *pwszScope, ULONG iBmk, WCHAR *pwszMachine, ULONG *pccMachine, WCHAR *pwszCat, ULONG *pccCat),
    (pwszScope, iBmk, pwszMachine, pccMachine, pwszCat, pccCat));

DELAY_LOAD_HRESULT(g_hmodQuery, QUERY, CIState,
    (WCHAR const *pwszCatalog, WCHAR const *pwszMachine, CI_STATE *pciState),
    (pwszCatalog, pwszMachine, pciState));

// -----------fmifs.dll---------------
HMODULE g_hmodFmifs = NULL;

DELAY_LOAD_BOOLEAN(g_hmodFmifs, FMIFS, QueryFileSystemName,
                (PWSTR NtDriveName, PWSTR FileSystemName, PUCHAR MajorVersion, PUCHAR MinorVersion, PNTSTATUS ErrorCode),
                (NtDriveName, FileSystemName, MajorVersion, MinorVersion, ErrorCode))
                                                                                      
DELAY_LOAD_BOOLEAN(g_hmodFmifs, FMIFS, QueryLatestFileSystemVersion,
                (PWSTR FileSystemName, PUCHAR MajorVersion, PUCHAR MinorVersion),
                (FileSystemName, MajorVersion, MinorVersion))
                                                                                      
DELAY_LOAD_BOOLEAN(g_hmodFmifs, FMIFS, QueryAvailableFileSystemFormat,
                (ULONG Index, PWSTR FileSystemName, PUCHAR MajorVersion, PUCHAR MinorVersion, PBOOLEAN Latest),
                (Index, FileSystemName, MajorVersion, MinorVersion, Latest))
#endif // WINNT                                                                                      
#ifdef UNICODE
// -----------kernel32.dll---------------
// primarily for winnt specific kernel functions that we can't link to
// because it would bind us to NT5.
HMODULE g_hmodAdvApi32 = NULL;
DELAY_LOAD_NAME_ERR(g_hmodAdvApi32, ADVAPI32, BOOL, DelayCreateProcessWithLogon,
                    CreateProcessWithLogonW,
                    (LPCWSTR pszUser,
                     LPCWSTR pszDomain,
                     LPCWSTR pszPassword,
                     DWORD dwLogonFlags,
                     LPCWSTR lpApplicationName,
                     LPCWSTR lpCommandLine,
                     DWORD dwCreationFlags,
                     LPVOID lpEnvironment,
                     LPCWSTR lpCurrentDirectory,
                     LPSTARTUPINFOW lpStartupInfo,
                     LPPROCESS_INFORMATION lpProcessInformation
                    ), 
                    (pszUser,
                     pszDomain,
                     pszPassword,
                     dwLogonFlags,
                     lpApplicationName,
                     lpCommandLine, 
                     dwCreationFlags,
                     lpEnvironment,
                     lpCurrentDirectory,
                     lpStartupInfo,
                     lpProcessInformation
                    ), 
                    FALSE                  
                   );

#endif

// NT5 specific ...
#ifdef WINNT
HMODULE g_hmodKernel32 = NULL;
DELAY_LOAD_NAME_ERR(g_hmodKernel32, KERNEL32, LANGID, DelayGetUserDefaultUILanguage,
    GetUserDefaultUILanguage, 
    (void),(),LANG_USER_DEFAULT);
#endif
// -----------urlmon.dll---------------
HMODULE g_hmodUrlmon = NULL;

// EXTERN_C STDAPI_(HRESULT) CoInternetCreateSecurityManager(IServiceProvider *pSP, IInternetSecurityManager **ppSM, DWORD dwReserved);
DELAY_LOAD_HRESULT(g_hmodUrlmon, URLMON, CoInternetCreateSecurityManager,
    (IServiceProvider *pSP, IInternetSecurityManager **ppSM, DWORD dwReserved),
    (pSP, ppSM, dwReserved));

DELAY_LOAD_HRESULT(g_hmodUrlmon, URLMON, CreateURLMoniker,
    (IMoniker* pMkCtx, LPCWSTR pwsURL, IMoniker ** ppimk), (pMkCtx, pwsURL, ppimk));

DELAY_LOAD_HRESULT(g_hmodUrlmon, URLMON, RegisterBindStatusCallback,
    (LPBC pBC, IBindStatusCallback *pBSCb, IBindStatusCallback**  ppBSCBPrev, DWORD dwReserved),
    (pBC, pBSCb, ppBSCBPrev, dwReserved));

DELAY_LOAD_HRESULT(g_hmodUrlmon, URLMON, RevokeBindStatusCallback,
    (LPBC pBC, IBindStatusCallback *pBSCb),
    (pBC, pBSCb));

DELAY_LOAD_HRESULT(g_hmodUrlmon, URLMON, HlinkNavigateString,
    (IUnknown *pUnk, LPCWSTR szTarget),
    (pUnk, szTarget));


// -----------WININET.dll---------------
HMODULE g_hmodWininet = NULL;

#undef CreateUrlCacheEntryA
// EXTERN_C STDAPI_(BOOL) CreateUrlCacheEntryA(LPCSTR lpszUrlName, DWORD dwExpectedFileSize, LPCSTR lpszFileExtension,
//                                                LPSTR lpszFileName, DWORD dwReserved);
DELAY_LOAD_NAME_ERR(g_hmodWininet, WININET, BOOL, _CreateUrlCacheEntryA, CreateUrlCacheEntryA,
    (LPCSTR lpszUrlName, DWORD dwExpectedFileSize, LPCSTR lpszFileExtension, LPSTR lpszFileName, DWORD dwReserved),
    (lpszUrlName, dwExpectedFileSize, lpszFileExtension, lpszFileName, dwReserved), FALSE);

#undef CreateUrlCacheEntryW
// EXTERN_C STDAPI_(BOOL) CreateUrlCacheEntryW(LPCWSTR lpszUrlName, DWORD dwExpectedFileSize, LPCWSTR lpszFileExtension,
//                                                LPWSTR lpszFileName, DWORD dwReserved);
DELAY_LOAD_NAME_ERR(g_hmodWininet, WININET, BOOL, _CreateUrlCacheEntryW, CreateUrlCacheEntryW,
    (LPCWSTR lpszUrlName, DWORD dwExpectedFileSize, LPCWSTR lpszFileExtension, LPWSTR lpszFileName, DWORD dwReserved),
    (lpszUrlName, dwExpectedFileSize, lpszFileExtension, lpszFileName, dwReserved), FALSE);

#undef CommitUrlCacheEntryA
// EXTERN_C STDAPI_(BOOL) CommitUrlCacheEntryA(LPCSTR lpszUrlName, LPCSTR lpszLocalFileName, FILETIME ExpireTime, FILETIME LastModifiedTime,
//                                             DWORD CacheEntryType, LPSTR lpHeaderInfo, DWORD dwHeaderSize, LPCSTR lpszFileExtension, LPCSTR lpszOriginalUrl);
DELAY_LOAD_NAME_ERR(g_hmodWininet, WININET, BOOL, _CommitUrlCacheEntryA, CommitUrlCacheEntryA,
    (LPCSTR lpszUrlName, LPCSTR lpszLocalFileName, FILETIME ExpireTime, FILETIME LastModifiedTime,
     DWORD CacheEntryType, LPSTR lpHeaderInfo, DWORD dwHeaderSize, LPCSTR lpszFileExtension, LPCSTR lpszOriginalUrl),
    (lpszUrlName, lpszLocalFileName, ExpireTime, LastModifiedTime, CacheEntryType, lpHeaderInfo, dwHeaderSize, lpszFileExtension, lpszOriginalUrl), FALSE);

#undef CommitUrlCacheEntryW
// EXTERN_C STDAPI_(BOOL) CommitUrlCacheEntryW(LPCWSTR lpszUrlName, LPCWSTR lpszLocalFileName, FILETIME ExpireTime, FILETIME LastModifiedTime,
//                                             DWORD CacheEntryType, LPWSTR lpHeaderInfo, DWORD dwHeaderSize, LPCWSTR lpszFileExtension, LPCWSTR lpszOriginalUrl);
DELAY_LOAD_NAME_ERR(g_hmodWininet, WININET, BOOL, _CommitUrlCacheEntryW, CommitUrlCacheEntryW,
    (LPCWSTR lpszUrlName, LPCWSTR lpszLocalFileName, FILETIME ExpireTime, FILETIME LastModifiedTime,
     DWORD CacheEntryType, LPWSTR lpHeaderInfo, DWORD dwHeaderSize, LPCWSTR lpszFileExtension, LPCWSTR lpszOriginalUrl),
    (lpszUrlName, lpszLocalFileName, ExpireTime, LastModifiedTime, CacheEntryType, lpHeaderInfo, dwHeaderSize, lpszFileExtension, lpszOriginalUrl), FALSE);


// -----------commdlg.dll---------------
HMODULE g_hmodCommDlg = NULL;

#ifdef UNICODE
DELAY_LOAD_BOOL(g_hmodCommDlg, comdlg32, GetOpenFileNameW, (LPOPENFILENAMEW pofn), (pofn))
DELAY_LOAD_BOOL(g_hmodCommDlg, comdlg32, GetSaveFileNameW, (LPOPENFILENAMEW pofn), (pofn))
#else
DELAY_LOAD_BOOL(g_hmodCommDlg, comdlg32, GetOpenFileNameA, (LPOPENFILENAMEA pofn), (pofn))
DELAY_LOAD_BOOL(g_hmodCommDlg, comdlg32, GetSaveFileNameA, (LPOPENFILENAMEA pofn), (pofn))
#endif


// -----------sspi.dll---------------
HMODULE g_hmodSECUR32 = NULL;

DELAY_LOAD(g_hmodSECUR32, secur32, BOOLEAN, GetUserNameExW,
           (EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG nSize),
           (NameFormat, lpNameBuffer, nSize)); 


// -----------hhctrl.ocx--------------- 
HMODULE g_hmodHHCtrl = NULL;

#ifdef UNICODE
DELAY_LOAD_EXT(g_hmodHHCtrl, hhctrl, OCX, HWND, HtmlHelpW,
                (HWND hwndCaller, LPCWSTR pwszFile, UINT uCommand, DWORD dwData), 
                (hwndCaller, pwszFile, uCommand, dwData))

#else
DELAY_LOAD_EXT(g_hmodHHCtrl, hhctrl, OCX, HWND, HtmlHelpA,
                (HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD dwData), 
                (hwndCaller, pszFile, uCommand, dwData))

#endif //UNICODE
// -----------version.dll-------------------
HMODULE g_hmodVersion = NULL;

#ifdef UNICODE

DELAY_LOAD_BOOL(g_hmodVersion, version, GetFileVersionInfoW,
                (LPTSTR pszFilename, DWORD dwHandle, DWORD dwLen, void *lpData),
                (pszFilename, dwHandle, dwLen, lpData))

DELAY_LOAD(g_hmodVersion, version, DWORD, GetFileVersionInfoSizeW,
                (LPTSTR pszFilename,  LPDWORD lpdwHandle),
                (pszFilename, lpdwHandle))
 
DELAY_LOAD_BOOL(g_hmodVersion, version, VerQueryValueW,
                (const void *pBlock, LPTSTR lpSubBlock, void **ppBuffer, PUINT puLen),
                (pBlock, lpSubBlock, ppBuffer, puLen))

DELAY_LOAD_DWORD(g_hmodVersion, version, VerLanguageNameW,
                (DWORD wLang, LPTSTR szLang, DWORD nSize),
                (wLang, szLang, nSize))

DELAY_LOAD_BOOL(g_hmodVersion, version, VerQueryValueIndexW,
                (const void *pBlock, LPTSTR lpSubBlock, DWORD dwIndex, void **ppBuffer, void **ppValue, PUINT puLen),
                (pBlock, lpSubBlock, dwIndex, ppBuffer, ppValue, puLen))
#else

DELAY_LOAD_BOOL(g_hmodVersion, version, GetFileVersionInfoA,
                (LPTSTR pszFilename, DWORD dwHandle, DWORD dwLen, void *lpData),
                (pszFilename, dwHandle, dwLen, lpData))

DELAY_LOAD(g_hmodVersion, version, DWORD, GetFileVersionInfoSizeA,
                (LPTSTR pszFilename,  LPDWORD lpdwHandle),
                (pszFilename, lpdwHandle))
 
DELAY_LOAD_BOOL(g_hmodVersion, version, VerQueryValueA,
                (const void *pBlock, LPTSTR lpSubBlock, void **ppBuffer, PUINT puLen),
                (pBlock, lpSubBlock, ppBuffer, puLen))

DELAY_LOAD_DWORD(g_hmodVersion, version, VerLanguageNameA,
                (DWORD wLang, LPTSTR szLang, DWORD nSize),
                (wLang, szLang, nSize))
#endif

// --------- MPR.DLL ---------------

HMODULE g_hmodMPR = NULL;

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetConnectionDialog, (HWND  hwnd, DWORD dwType), (hwnd, dwType));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetCloseEnum, (HANDLE hEnum), (hEnum));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetDisconnectDialog,
        (HWND hwnd, DWORD dwType), (hwnd, dwType));

#ifdef UNICODE

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetEnumResourceW,
       (HANDLE  hEnum, LPDWORD lpcCount, LPVOID  lpBuffer, LPDWORD lpBufferSize),
       (hEnum, lpcCount, lpBuffer, lpBufferSize));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetOpenEnumW,
       (DWORD dwScope, DWORD dwType, DWORD dwUsage, LPNETRESOURCEW lpNetResource, LPHANDLE lphEnum),
       (dwScope, dwType, dwUsage, lpNetResource, lphEnum));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetFormatNetworkNameW,
    (LPCWSTR lpProvider, LPCWSTR lpRemoteName, LPWSTR lpFormattedName,
     LPDWORD lpnLength, DWORD dwFlags, DWORD dwAveCharPerLine),
    (lpProvider, lpRemoteName, lpFormattedName, lpnLength,  dwFlags, dwAveCharPerLine));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetRestoreConnectionW,
       (HWND hwnd, LPCWSTR psz),
       (hwnd, psz));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetCancelConnectionW,
       (LPCTSTR lpName, BOOL fForce),
       (lpName, fForce));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetProviderNameW,
       (DWORD dwNetType,
        LPWSTR lpProvider,
        LPDWORD lpBufferSize),
       (dwNetType,lpProvider,lpBufferSize));

DELAY_LOAD_WNET(g_hmodMPR, MPR, MultinetGetErrorTextW,
       (LPWSTR lpErrorTextBuf,
        LPDWORD lpnErrorBufSize,
        LPWSTR lpProviderNameBuf,
        LPDWORD lpnNameBufSize   ),
       (lpErrorTextBuf, lpnErrorBufSize, lpProviderNameBuf, lpnNameBufSize ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetUserW,
       (LPCWSTR lpName,
        LPWSTR lpUserName,
        LPDWORD lpnLength),
       (lpName,lpUserName,lpnLength));
       
DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetResourceInformationW,
       (LPNETRESOURCE lpNetResource,
        LPVOID lpBuffer,
        LPDWORD lpcbBuffer,
        LPWSTR *lplpSystem ),
       (lpNetResource, lpBuffer, lpcbBuffer, lplpSystem ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetUseConnectionW,
       (HWND hwndOwner,               
        LPNETRESOURCE lpNetResource,  
        LPCWSTR lpUserName,            
        LPCWSTR lpPassword,            
        DWORD dwFlags,                
        LPWSTR lpAccessName,          
        LPDWORD lpBufferSize,         
        LPDWORD lpResult ),
       (hwndOwner, lpNetResource, lpUserName, lpPassword,
        dwFlags, lpAccessName, lpBufferSize, lpResult ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetAddConnection3W,
       (HWND hwndOwner,
        LPNETRESOURCE lpNetResource,
        LPCWSTR lpPassword,
        LPCWSTR lpUserName,
        DWORD dwFlags ),
       (hwndOwner, lpNetResource, lpPassword, lpUserName, dwFlags ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetResourceParentW,
       (LPNETRESOURCE lpNetResource,
        LPVOID lpBuffer,
        LPDWORD lpBufferSize ),
       (lpNetResource, lpBuffer, lpBufferSize ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetCancelConnection2W,
       (LPCWSTR lpName,
        DWORD dwFlags,
        BOOL fForce ),
       (lpName, dwFlags, fForce ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetDisconnectDialog1W,
       (LPDISCDLGSTRUCT lpDiscDlgStruct),
       (lpDiscDlgStruct));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetConnectionDialog1W,
       (LPCONNECTDLGSTRUCT lpConnectDlgStruct),
       (lpConnectDlgStruct));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetLastErrorW,
       (LPDWORD lpError,
        LPWSTR lpErrorBuf,
        DWORD nErrorBufSize,
        LPWSTR lpNameBuf,
        DWORD nNameBufSize ),
       (lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize));

DELAY_LOAD_WNET(g_hmodMPR, MPR, MultinetGetConnectionPerformanceW,
        (LPNETRESOURCE lpNetResource,
        LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct),
        (lpNetResource, lpNetConnectInfoStruct));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetConnectionW,
        (LPCWSTR lpLocalName,
         LPWSTR lpRemoteName,
         LPDWORD lpcbBuffer),
        (lpLocalName, lpRemoteName, lpcbBuffer));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetConnection3W,
        (LPCWSTR lpLocalName,
        LPCWSTR lpProviderName,
        DWORD dwInfoLevel,
        LPVOID lpBuffer,
        LPDWORD lpcbBuffer),
        (lpLocalName, lpProviderName, dwInfoLevel, lpBuffer, lpcbBuffer));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetProviderTypeW,
        (LPCWSTR lpProvider,
        LPDWORD lpdwNetType),
        (lpProvider, lpdwNetType));

#else

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetEnumResourceA,
       (HANDLE  hEnum, LPDWORD lpcCount, LPVOID  lpBuffer, LPDWORD lpBufferSize),
       (hEnum, lpcCount, lpBuffer, lpBufferSize));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetOpenEnumA,
       (DWORD dwScope, DWORD dwType, DWORD dwUsage, LPNETRESOURCEA lpNetResource, LPHANDLE lphEnum),
       (dwScope, dwType, dwUsage, lpNetResource, lphEnum));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetFormatNetworkNameA,
    (LPCSTR lpProvider, LPCSTR lpRemoteName, LPSTR lpFormattedName,
     LPDWORD lpnLength, DWORD dwFlags, DWORD dwAveCharPerLine),
    (lpProvider, lpRemoteName, lpFormattedName, lpnLength,  dwFlags, dwAveCharPerLine));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetRestoreConnectionA,
       (HWND hwnd, LPCSTR psz),
       (hwnd, psz));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetCancelConnectionA,
       (LPCTSTR lpName, BOOL fForce),
       (lpName, fForce));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetProviderNameA,
       (DWORD dwNetType,
        LPSTR lpProvider,
        LPDWORD lpBufferSize),
       (dwNetType,lpProvider,lpBufferSize));

DELAY_LOAD_WNET(g_hmodMPR, MPR, MultinetGetErrorTextA,
       (LPSTR lpErrorTextBuf,
        LPDWORD lpnErrorBufSize,
        LPSTR lpProviderNameBuf,
        LPDWORD lpnNameBufSize   ),
       (lpErrorTextBuf, lpnErrorBufSize, lpProviderNameBuf, lpnNameBufSize ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetLogonA,
       (LPCSTR lpProvider,
        HWND hwndOwner   ),
       (lpProvider, hwndOwner ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetUserA,
       (LPCSTR lpName,
        LPSTR lpUserName,
        LPDWORD lpnLength),
       (lpName,lpUserName,lpnLength));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetResourceInformationA,
       (LPNETRESOURCE lpNetResource,
        LPVOID lpBuffer,
        LPDWORD lpcbBuffer,
        LPSTR *lplpSystem ),
       (lpNetResource, lpBuffer, lpcbBuffer, lplpSystem ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetUseConnectionA,
       (HWND hwndOwner,               
        LPNETRESOURCE lpNetResource,  
        LPCSTR lpUserName,            
        LPCSTR lpPassword,            
        DWORD dwFlags,                
        LPSTR lpAccessName,          
        LPDWORD lpBufferSize,         
        LPDWORD lpResult ),
       (hwndOwner, lpNetResource, lpUserName, lpPassword,
        dwFlags, lpAccessName, lpBufferSize, lpResult ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetAddConnection3A,
       (HWND hwndOwner,
        LPNETRESOURCE lpNetResource,
        LPCSTR lpPassword,
        LPCSTR lpUserName,
        DWORD dwFlags ),
       (hwndOwner, lpNetResource, lpPassword, lpUserName, dwFlags ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetResourceParentA,
       (LPNETRESOURCE lpNetResource,
        LPVOID lpBuffer,
        LPDWORD lpBufferSize ),
       (lpNetResource, lpBuffer, lpBufferSize ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetCancelConnection2A,
       (LPCSTR lpName,
        DWORD dwFlags,
        BOOL fForce ),
       (lpName, dwFlags, fForce ));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetDisconnectDialog1A,
       (LPDISCDLGSTRUCT lpDiscDlgStruct),
       (lpDiscDlgStruct));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetConnectionDialog1A,
       (LPCONNECTDLGSTRUCT lpConnectDlgStruct),
       (lpConnectDlgStruct));

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetLastErrorA,
       (LPDWORD lpError,
        LPSTR lpErrorBuf,
        DWORD nErrorBufSize,
        LPSTR lpNameBuf,
        DWORD nNameBufSize ),
       (lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize));
       
DELAY_LOAD_WNET(g_hmodMPR, MPR, MultinetGetConnectionPerformanceA,
        (LPNETRESOURCE lpNetResource,
        LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct),
        (lpNetResource, lpNetConnectInfoStruct));
        
DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetConnectionA,
        (LPCSTR lpLocalName,
         LPSTR lpRemoteName,
         LPDWORD lpcbBuffer),
        (lpLocalName, lpRemoteName, lpcbBuffer));
        
DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetConnection3A,
        (LPCSTR lpLocalName,
        LPCSTR lpProviderName,
        DWORD dwInfoLevel,
        LPVOID lpBuffer,
        LPDWORD lpcbBuffer),
        (lpLocalName, lpProviderName, dwInfoLevel, lpBuffer, lpcbBuffer));
#endif


// --------- NETAPI32.DLL ---------------

HMODULE g_hmodNetAPI32 = NULL;

DELAY_LOAD_ERR(g_hmodNetAPI32, NETAPI32, NET_API_STATUS, NetGetJoinInformation,
                (LPCWSTR lpServer, LPWSTR *lpNameBuffer, PNETSETUP_JOIN_STATUS BufferType),
                (lpServer, lpNameBuffer, BufferType),
                NERR_NetNotStarted);

DELAY_LOAD_ERR(g_hmodNetAPI32, NETAPI32, NET_API_STATUS, NetApiBufferFree,
                (LPVOID Buffer), 
                (Buffer), 
                NERR_Success);

DELAY_LOAD_ERR(g_hmodNetAPI32, NETAPI32, DWORD, DsGetDcName,
                (IN LPCTSTR ComputerName OPTIONAL,
                IN LPCTSTR DomainName OPTIONAL,
                IN GUID *DomainGuid OPTIONAL,
                IN LPCTSTR SiteName OPTIONAL,
                IN ULONG Flags,
                OUT PDOMAIN_CONTROLLER_INFO *DomainControllerInfo),
                (ComputerName, DomainName, DomainGuid, SiteName, Flags, DomainControllerInfo),
                ERROR_NO_SUCH_DOMAIN);

// --------- NTDSAPI.DLL ---------------
HMODULE g_hmodNTDSAPI = NULL;

DELAY_LOAD_ERR(g_hmodNTDSAPI, NTDSAPI, DWORD, DsCrackNames, 
                (HANDLE          hDS,          
                 DS_NAME_FLAGS   flags,        
                 DS_NAME_FORMAT  formatOffered,
                 DS_NAME_FORMAT  formatDesired,
                 DWORD           cNames,       
                 const LPCTSTR   *rpNames,     
                 PDS_NAME_RESULT *ppResult),
                (hDS, flags, formatOffered, formatDesired, cNames, rpNames, ppResult),
                 ERROR_INVALID_FUNCTION);   

DELAY_LOAD_VOID(g_hmodNTDSAPI, NTDSAPI, DsFreeNameResult, (DS_NAME_RESULT *pResult), (pResult));   

// --------- MLANG.DLL ---------------

HMODULE g_hmodMLANG = NULL;

DELAY_LOAD_HRESULT(g_hmodMLANG, MLANG, ConvertINetMultiByteToUnicode,
            (LPDWORD lpdwMode, DWORD dwEncoding, LPCSTR lpSrcStr, LPINT lpnMultiCharCount, LPWSTR lpDstStr, LPINT lpnWideCharCount),
            (lpdwMode, dwEncoding, lpSrcStr, lpnMultiCharCount, lpDstStr, lpnWideCharCount));

DELAY_LOAD_HRESULT(g_hmodMLANG, MLANG, ConvertINetUnicodeToMultiByte,
            (LPDWORD lpdwMode, DWORD dwEncoding, LPCWSTR lpSrcStr, LPINT lpnWideCharCount, LPSTR lpDstStr, LPINT lpnMultiCharCount),
            (lpdwMode, dwEncoding, lpSrcStr, lpnWideCharCount, lpDstStr, lpnMultiCharCount));

DELAY_LOAD_HRESULT(g_hmodMLANG, MLANG, LcidToRfc1766W,
            (LCID Locale, LPWSTR pszRfc1766, int nChar),
            (Locale, pszRfc1766, nChar));

// --------- CDFVIEW.DLL ---------------
HMODULE g_hmodCDFVIEW = NULL;
DELAY_LOAD_HRESULT(g_hmodCDFVIEW, CDFVIEW, ParseDesktopComponent,
           (HWND hwndOwner, LPWSTR wszURL, COMPONENT *pInfo),
           (hwndOwner, wszURL, pInfo));

// EXTERN_C STDAPI_(HRESULT) SubscribeToCDF(HWND hwndParent, LPCWSTR pwzUrl, DWORD dwCDFTypes);
DELAY_LOAD_HRESULT(g_hmodCDFVIEW, CDFVIEW, SubscribeToCDF,
           (HWND hwndParent, LPCWSTR pwzUrl, DWORD dwCDFTypes),
           (hwndParent, pwzUrl, dwCDFTypes));


// --------- USER32.DLL ----------------
HINSTANCE g_hinstUSER32 = NULL;

DELAY_LOAD_BOOL(g_hinstUSER32, USER32, AllowSetForegroundWindow,
               (DWORD dwProcessId), (dwProcessId));

// --------- USERENV.DLL ----------------
HMODULE g_hmodUserEnv = NULL;

DELAY_LOAD_BOOL(g_hmodUserEnv, USERENV, GetUserProfileDirectoryW,
                (HANDLE hToken, WCHAR *pszDir, DWORD *pdwSize),
                (hToken, pszDir, pdwSize));

DELAY_LOAD_BOOL(g_hmodUserEnv, USERENV, GetAllUsersProfileDirectoryW,
                (WCHAR *pszDir, DWORD *pdwSize),
                (pszDir, pdwSize));

#undef GetDefaultUserProfileDirectoryW

DELAY_LOAD_BOOL(g_hmodUserEnv, USERENV, GetDefaultUserProfileDirectoryW,
                (WCHAR *pszDir, DWORD *pdwSize),
                (pszDir, pdwSize));

#undef ExpandEnvironmentStringsForUserW

DELAY_LOAD_BOOL(g_hmodUserEnv, USERENV, ExpandEnvironmentStringsForUserW,
                (HANDLE hToken, LPCWSTR lpSrc, LPWSTR lpDest, DWORD dwSize),
                (hToken, lpSrc, lpDest, dwSize));

DELAY_LOAD_BOOL(g_hmodUserEnv, USERENV, CreateEnvironmentBlock,
                (LPVOID *lpEnvironment, HANDLE hToken, BOOL bInherit),
                (lpEnvironment, hToken, bInherit));

 DELAY_LOAD_BOOL(g_hmodUserEnv, USERENV, DestroyEnvironmentBlock,
                (LPVOID lpEnvironment),
                (lpEnvironment));
                

// --------- CSCDLL.DLL ----------------
HMODULE g_hmodCSCDLL = NULL;

DELAY_LOAD_BOOL(g_hmodCSCDLL, CSCDLL, CSCIsCSCEnabled, (void), ());

DELAY_LOAD_BOOL(g_hmodCSCDLL, CSCDLL, CSCQueryFileStatusW,
                (LPCWSTR lpszFileName, LPDWORD lpdwStatus, LPDWORD lpdwPinCount, LPDWORD lpdwHintFlags),
                (lpszFileName, lpdwStatus, lpdwPinCount, lpdwHintFlags));

DELAY_LOAD_BOOL(g_hmodCSCDLL, CSCDLL, CSCQueryFileStatusA,
                (LPCSTR lpszFileName, LPDWORD lpdwStatus, LPDWORD lpdwPinCount, LPDWORD lpdwHintFlags),
                (lpszFileName, lpdwStatus, lpdwPinCount, lpdwHintFlags));

// --------- WINSPOOL.DRV ----------------
HINSTANCE g_hinstWINSPOOL_DRV = NULL;

DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, AddPort,
                (LPTSTR pName, HWND hwnd, LPTSTR pMonitorName), 
                (pName, hwnd, pMonitorName));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, ClosePrinter,
                (HANDLE h), 
                (h));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, ConfigurePort,
                (LPTSTR pName, HWND hwnd, LPTSTR pPortName), 
                (pName, hwnd, pPortName));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, DeletePort,
                (LPTSTR pName, HWND hwnd, LPTSTR psz2), 
                (pName, hwnd, psz2));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, DeletePrinter,
                (HANDLE hPrinter), 
                (hPrinter));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, DeletePrinterDriver,
                (LPTSTR pName, LPTSTR pEnvironment, LPTSTR pDriverName), 
                (pName, pEnvironment, pDriverName));
DELAY_LOAD_EXT_WRAP(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, int, _DeviceCapabilities, DeviceCapabilities,
                (LPCTSTR psz1, LPCTSTR psz2, WORD w, LPTSTR psz3, CONST DEVMODE * pDevMode), 
                (psz1, psz2, w, psz3, pDevMode));             
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, EnumJobs,
                (HANDLE h, DWORD dw1, DWORD dw2, DWORD dw3, LPBYTE pb, DWORD dw4, LPDWORD pdw1, LPDWORD pdw2), 
                (h, dw1, dw2, dw3, pb, dw4, pdw1, pdw2));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, EnumMonitors,
                (LPTSTR psz1, DWORD dw1, LPBYTE pb1, DWORD dw2, LPDWORD pdw1, LPDWORD pdw2), 
                (psz1, dw1, pb1, dw2, pdw1, pdw2));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, EnumPorts,
                (LPTSTR psz1, DWORD dw1, LPBYTE pb1, DWORD dw2, LPDWORD pdw1, LPDWORD pdw2), 
                (psz1, dw1, pb1, dw2, pdw1, pdw2));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, EnumPrintProcessorDatatypes,
                (LPTSTR psz1, LPTSTR psz2, DWORD dw1, LPBYTE pb1, DWORD dw2, LPDWORD pdw1, LPDWORD pdw2), 
                (psz1, psz2, dw1, pb1, dw2, pdw1, pdw2));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, EnumPrintProcessors,
                (LPTSTR psz1, LPTSTR psz2, DWORD dw1, LPBYTE pb1, DWORD dw2, LPDWORD pdw1, LPDWORD pdw2), 
                (psz1, psz2, dw1, pb1, dw2, pdw1, pdw2));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, EnumPrinterDrivers,
                (LPTSTR pName, LPTSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned), 
                (pName, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded, pcReturned));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, EnumPrinters,
                (DWORD dwFlags, LPTSTR psz1, DWORD dw1, LPBYTE pb1, DWORD dw2, LPDWORD pdw2, LPDWORD pdw3),
                (dwFlags, psz1, dw1, pb1, dw2, pdw2, pdw3));
DELAY_LOAD_EXT_ORD(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, EnumPrinterPropertySheets, ENUMPRINTERPROPERTYSHEETS_ORD,
                (HANDLE h1, HWND hwnd, LPFNADDPROPSHEETPAGE pfn, LPARAM lParam), 
                (h1, hwnd, pfn, lParam));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, GetPrinter,
                (HANDLE h1, DWORD dw1, LPBYTE pb1, DWORD dw2, LPDWORD pdw1), 
                (h1, dw1, pb1, dw2, pdw1));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, GetPrinterDriver,
                (HANDLE h1, LPTSTR psz1, DWORD dw1, LPBYTE pb1, DWORD dw2, LPDWORD pdw1), 
                (h1, psz1, dw1, pb1, dw2, pdw1));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, OpenPrinter,
                (LPTSTR psz1, LPHANDLE ph1, LPPRINTER_DEFAULTS pDefault), 
                (psz1, ph1, pDefault));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, PrinterProperties,
                (HWND hwnd, HANDLE h1), 
                (hwnd, h1));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, SetJob,
                (HANDLE h1, DWORD dw1, DWORD dw2, LPBYTE pb1, DWORD dw3), 
                (h1, dw1, dw2, pb1, dw3));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, BOOL, SetPrinter,
                (HANDLE h1, DWORD dw1, LPBYTE pb1, DWORD dw2), 
                (h1, dw1, pb1, dw2));
DELAY_LOAD_EXT(g_hinstWINSPOOL_DRV, WINSPOOL, DRV, DWORD, GetPrinterData,
                (HANDLE h1, LPTSTR psz1, LPDWORD pdw1, LPBYTE pb1, DWORD dw1, LPDWORD pdw2), 
                (h1, psz1, pdw1, pb1, dw1, pdw2));
                

// --------- PRINTUI.DLL ----------------
HINSTANCE g_hinstPRINTUI = NULL;

#ifdef WINNT // PRINTQ
DELAY_LOAD_VOID(g_hinstPRINTUI, PRINTUI, vQueueCreate,
    (HWND hwnd1, LPCTSTR psz1, INT n, LPARAM lParam),
    (hwnd1, psz1, n, lParam));
DELAY_LOAD_VOID(g_hinstPRINTUI, PRINTUI, vPrinterPropPages,
    (HWND hwnd1, LPCTSTR psz1, INT n, LPARAM lParam),
    (hwnd1, psz1, n, lParam));
DELAY_LOAD_VOID(g_hinstPRINTUI, PRINTUI, vServerPropPages,
    (HWND hwnd1, LPCTSTR psz1, INT n, LPARAM lParam),
    (hwnd1, psz1, n, lParam));
DELAY_LOAD(g_hinstPRINTUI, PRINTUI, BOOL, bPrinterSetup,
    (HWND hwnd1, UINT ui1, UINT ui2, LPTSTR psz1, PUINT pui, LPCTSTR psz2),
    (hwnd1, ui1, ui2, psz1, pui, psz2));
DELAY_LOAD_VOID(g_hinstPRINTUI, PRINTUI, vDocumentDefaults,
    (HWND hwnd1, LPCTSTR psz1, INT n, LPARAM lParam),
    (hwnd1, psz1, n, lParam));

#ifdef PRN_FOLDERDATA
DELAY_LOAD_HRESULT(g_hinstPRINTUI, PRINTUI, RegisterPrintNotify,
    (LPCTSTR psz1, IFolderNotify * pfn, LPHANDLE pHandle),
    (psz1, pfn, pHandle));
DELAY_LOAD_HRESULT(g_hinstPRINTUI, PRINTUI, UnregisterPrintNotify,
    (LPCTSTR psz1, IFolderNotify * pfn, LPHANDLE pHandle),
    (psz1, pfn, pHandle));
DELAY_LOAD(g_hinstPRINTUI, PRINTUI, HANDLE, hFolderRegister,
    (LPCTSTR psz1, LPCITEMIDLIST pidl),
    (psz1, pidl));
DELAY_LOAD_VOID(g_hinstPRINTUI, PRINTUI, vFolderUnregister,
    (HANDLE h),
    (h));
DELAY_LOAD(g_hinstPRINTUI, PRINTUI, BOOL, bFolderEnumPrinters,
    (HANDLE h, PFOLDER_PRINTER_DATA pFolderPrinterData, DWORD dw1, PDWORD pwd1, PDWORD pwd2),
    (h, pFolderPrinterData, dw1, pwd1, pwd2));
DELAY_LOAD(g_hinstPRINTUI, PRINTUI, BOOL, bFolderRefresh,
    (HANDLE h, PBOOL pb),
    (h, pb));
DELAY_LOAD(g_hinstPRINTUI, PRINTUI, BOOL, bFolderGetPrinter,
    (HANDLE h, LPCTSTR psz1, PFOLDER_PRINTER_DATA pFolderPrinterData, DWORD dw1, PDWORD pdw2),
    (h, psz1, pFolderPrinterData, dw1, pdw2));
#endif // PRN_FOLDERDATA
#endif // PRINTQ

// --------- DEVMGR.DLL ----------------
HINSTANCE g_hinstDEVMGR = NULL;

DELAY_LOAD(g_hinstDEVMGR, DEVMGR, HWND, DeviceCreateHardwarePageEx,
    (HWND hwndParent, const GUID *pguid, int iNumClass, DWORD dwViewMode),
    (hwndParent, pguid, iNumClass,  dwViewMode));

// --------- INETCFG.DLL ----------------
HMODULE g_hmodINETCFG = NULL;
DELAY_LOAD_DWORD(g_hmodINETCFG, INETCFG, IsSmartStart,
           (void),
           ());
// --------- COMCTL32.DLL ----------------
//
//  Wha?  Delay-load comctl32?  Yup, because some setup apps
//  include comctl32.dll in their temporary setup directory,
//  so we end up loading that comctl32 instead of the system
//  comctl32, and ImageList_SetFlags is an API that didn't exist
//  in old versions of Comctl32, so shell32 would fail to load,
//  and the setup app would barf.  Fortunately, setup apps don't
//  use shell32 for much beyond CShellLink and SHChangeNotify,
//  so the mixup of a new shell32 and a downlevel comctl32 won't
//  screw them too badly.
//
HINSTANCE g_hinstCOMCTL32 = NULL;

#undef ImageList_SetFlags
DELAY_LOAD_NAME_BOOL(g_hinstCOMCTL32, COMCTL32,
                     _ImageList_SetFlags, ImageList_SetFlags,
                     (HIMAGELIST himl, UINT flags),
                     (himl, flags));

#undef ImageList_GetFlags
DELAY_LOAD_NAME_UINT(g_hinstCOMCTL32, COMCTL32,
                     _ImageList_GetFlags, ImageList_GetFlags,
                     (HIMAGELIST himl),
                     (himl));

// --------- MSI.DLL ----------------
HMODULE g_hmodMSI = NULL;
DELAY_LOAD_UINT(g_hmodMSI, MSI, MsiDecomposeDescriptorW,
                (LPCWSTR	szDescriptor,
	             LPWSTR     szProductCode,
	             LPWSTR     szFeatureId,
	             LPWSTR     szComponentCode,
	             DWORD*     pcchArgsOffset),
                 (szDescriptor,
                  szProductCode,
                  szFeatureId,
                  szComponentCode,
                  pcchArgsOffset));

DELAY_LOAD_UINT(g_hmodMSI, MSI, MsiDecomposeDescriptorA,
                (LPCSTR	szDescriptor,
	             LPSTR     szProductCode,
	             LPSTR     szFeatureId,
	             LPSTR     szComponentCode,
	             DWORD*     pcchArgsOffset),
                 (szDescriptor,
                  szProductCode,
                  szFeatureId,
                  szComponentCode,
                  pcchArgsOffset));
