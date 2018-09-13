//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       urldll.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-25-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <mon.h>
#include "urlcf.hxx"
#include "selfreg.hxx"
#include <delaydll.h>
#include <tls.h>

#ifdef unix
#define DllMain DllMainInternal
#endif /* unix */

COleAutDll  g_OleAutDll;

PerfDbgTag(tagUrlDll, "Urlmon", "Log DllMain", DEB_URLMON);

DECLARE_INFOLEVEL(UrlMk)
DECLARE_INFOLEVEL(Trans)
DECLARE_INFOLEVEL(PProt)
DECLARE_INFOLEVEL(Notf)
DECLARE_INFOLEVEL(EProt)
DECLARE_INFOLEVEL(TNotf)

extern HINSTANCE g_hInst;
extern HMODULE   g_hLibPluginOcx;
extern HMODULE   g_hLibMlang;
extern ULONG     Win4AssertLevel;
extern IEnumFORMATETC *g_pEFmtETC;

DWORD g_dwSettings = 0;
BOOL  g_bCanUseSimpleBinding = TRUE;
BOOL  CanUseSimpleBinding();
BOOL  g_bHasMimeHandlerForTextHtml = TRUE;
DWORD g_cTransLevelHandler = 0;
BOOL  g_bGlobalUTF8Enabled = FALSE;
BOOL  GlobalUTF8Enabled();
BOOL  g_bNT5OrGreater = FALSE;

URLMON_TS* g_pHeadURLMONTSList; 
HRESULT    CleanupTSOnProcessDetach();


// defined in calldatax.c
EXTERN_C HRESULT PrxDllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);
EXTERN_C HRESULT PrxDllRegisterServer();
EXTERN_C HRESULT PrxDllUnregisterServer();
EXTERN_C HRESULT PrxDllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID lpvReserved);

// defined in zoneutil.c
EXTERN_C HRESULT ZonesDllInstall(BOOL bInstall, LPCWSTR pwStr);

// global variables
CRefCount g_cRef(0);        // global dll refcount
CMutexSem g_mxsMedia;       // single access to media holder

LPSTR g_pszUserAgentString = NULL;  // Per-process configurable User Agent string
LPSTR g_pszUAInfoString = NULL;
LPSTR GetUAInfoString(void);

#define URLMON_NAME      "urlmon.dll"

STDAPI_(BOOL) TlsDllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved);

// CLSIDs
#define SOFTDIST_CLSID                          "{B15B8DC0-C7E1-11d0-8680-00AA00BDCB71}"
#define URLMONIKER_CLSID                        "{79eac9e0-baf9-11ce-8c82-00aa004ba90b}"
#define URLBINDCTX_CLSID                        "{79eac9f2-baf9-11ce-8c82-00aa004ba90b}"
#define URLMONIKER_PS_CLSID                     "{79eac9f1-baf9-11ce-8c82-00aa004ba90b}"

#define PROTOCOL_HTTP_CLSID                     "{79eac9e2-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_FTP_CLSID                      "{79eac9e3-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_GOPHER_CLSID                   "{79eac9e4-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_HTTPS_CLSID                    "{79eac9e5-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_MK_CLSID                       "{79eac9e6-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_FILE_CLSID                     "{79eac9e7-baf9-11ce-8c82-00aa004ba90b}"

#define SECMGR_CLSID                            "{7b8a2d94-0ac9-11d1-896c-00c04Fb6bfc4}"
#define ZONEMGR_CLSID                           "{7b8a2d95-0ac9-11d1-896c-00c04Fb6bfc4}"

// Keys
#define SOFTDIST_CLSID_REGKEY                   "CLSID\\"SOFTDIST_CLSID
#define URLMONIKER_CLSID_REGKEY                 "CLSID\\"URLMONIKER_CLSID
#define URLMONIKER_PS_CLSID_REGKEY              "CLSID\\"URLMONIKER_PS_CLSID
#define URLBINDCTX_CLSID_REGKEY                 "CLSID\\"URLBINDCTX_CLSID

#define PROTOCOL_HTTP_CLSID_REGKEY              "CLSID\\"PROTOCOL_HTTP_CLSID
#define PROTOCOL_FTP_CLSID_REGKEY               "CLSID\\"PROTOCOL_FTP_CLSID
#define PROTOCOL_GOPHER_CLSID_REGKEY            "CLSID\\"PROTOCOL_GOPHER_CLSID
#define PROTOCOL_HTTPS_CLSID_REGKEY             "CLSID\\"PROTOCOL_HTTPS_CLSID
#define PROTOCOL_MK_CLSID_REGKEY                "CLSID\\"PROTOCOL_MK_CLSID
#define PROTOCOL_FILE_CLSID_REGKEY              "CLSID\\"PROTOCOL_FILE_CLSID

#define SECMGR_CLSID_REGKEY                     "CLSID\\"SECMGR_CLSID
#define ZONEMGR_CLSID_REGKEY                    "CLSID\\"ZONEMGR_CLSID

// Descriptions
#define URLBINDCTX_DESCRIP                      "Async BindCtx"
#define URLMONIKER_DESCRIP                      "URL Moniker"
#define URLMONIKER_PS_DESCRIP                   "URLMoniker ProxyStub Factory"
#define SOFTDIST_DESCRIP                        "SoftDist"

#define PROTOCOL_HTTP_DESCRIP                   "http: Asychronous Pluggable Protocol Handler"
#define PROTOCOL_FTP_DESCRIP                    "ftp: Asychronous Pluggable Protocol Handler"
#define PROTOCOL_GOPHER_DESCRIP                 "gopher: Asychronous Pluggable Protocol Handler"
#define PROTOCOL_HTTPS_DESCRIP                  "https: Asychronous Pluggable Protocol Handler"
#define PROTOCOL_MK_DESCRIP                     "mk: Asychronous Pluggable Protocol Handler"
#define PROTOCOL_FILE_DESCRIP                   "file:, local: Asychronous Pluggable Protocol Handler"

#define SECMGR_DESCRIP                          "Security Manager"
#define ZONEMGR_DESCRIP                         "URL Zone Manager"

#define STDCOURIER_CLSID                        "{c733e4af-576e-11d0-b28c-00c04fd7cd22}"
#define STDCOURIER_CLSID_REGKEY                 "CLSID\\"STDCOURIER_CLSID
#define STDCOURIER_CLSID_REGKEY                 "CLSID\\"STDCOURIER_CLSID
#define STDCOURIER_DESCRIP                      "Thread NotificationMgr"

const REGENTRY rgStdNotificationMgr[] =
{
    //***** STDCOURIER ENTRIES *****
    STD_ENTRY(STDCOURIER_CLSID_REGKEY, STDCOURIER_DESCRIP),
    STD_ENTRY(STDCOURIER_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, STDCOURIER_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};


///* Registration of urlmon class
// HKEY_CLASSES_ROOT
const REGENTRY rgClassesRoot[] =
{
    //***** URLMONIKER ENTRIES *****
    STD_ENTRY(URLMONIKER_CLSID_REGKEY, URLMONIKER_DESCRIP),
    STD_ENTRY(URLMONIKER_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, URLMONIKER_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgClassesSoftDist[] =
{
    //***** SOFTDIST ENTRIES *****
    STD_ENTRY(SOFTDIST_CLSID_REGKEY, SOFTDIST_DESCRIP),
    STD_ENTRY(SOFTDIST_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, SOFTDIST_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgClassesSecMgr[] =
{
    //***** SECMGR ENTRIES *****
    STD_ENTRY(SECMGR_CLSID_REGKEY, SECMGR_DESCRIP),
    STD_ENTRY(SECMGR_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, SECMGR_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Both" },
};

const REGENTRY rgClassesZoneMgr[] =
{
    //***** ZONEMGR ENTRIES *****
    STD_ENTRY(ZONEMGR_CLSID_REGKEY, ZONEMGR_DESCRIP),
    STD_ENTRY(ZONEMGR_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, ZONEMGR_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Both" },
};

const REGENTRY rgClassesBindCtx[] =
{
    //***** URLBINDCTX ENTRIES *****
    STD_ENTRY(URLBINDCTX_CLSID_REGKEY, URLBINDCTX_DESCRIP),
    STD_ENTRY(URLBINDCTX_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, URLBINDCTX_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

// Registration of proxy/stub class id
const REGENTRY rgPSFactory[] =
{
    //***** URLMONIKER PS ENTRIES *****
    STD_ENTRY(URLMONIKER_PS_CLSID_REGKEY, URLMONIKER_PS_DESCRIP),
    STD_ENTRY(URLMONIKER_PS_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, URLMONIKER_PS_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

// protocols
const REGENTRY rgClassesHttp[] =
{
    //***** PROTOCOL_HTTP ENTRIES *****
    STD_ENTRY(PROTOCOL_HTTP_CLSID_REGKEY, PROTOCOL_HTTP_DESCRIP),
    STD_ENTRY(PROTOCOL_HTTP_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, PROTOCOL_HTTP_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};
const REGENTRY rgClassesFtp[] =
{
    //***** PROTOCOL_FTP ENTRIES *****
    STD_ENTRY(PROTOCOL_FTP_CLSID_REGKEY, PROTOCOL_FTP_DESCRIP),
    STD_ENTRY(PROTOCOL_FTP_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, PROTOCOL_FTP_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgClassesGopher[] =
{
    //***** PROTOCOL_GOPHER ENTRIES *****
    STD_ENTRY(PROTOCOL_GOPHER_CLSID_REGKEY, PROTOCOL_GOPHER_DESCRIP),
    STD_ENTRY(PROTOCOL_GOPHER_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, PROTOCOL_GOPHER_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgClassesHttpS[] =
{
    //***** PROTOCOL_HTTPS ENTRIES *****
    STD_ENTRY(PROTOCOL_HTTPS_CLSID_REGKEY, PROTOCOL_HTTPS_DESCRIP),
    STD_ENTRY(PROTOCOL_HTTPS_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, PROTOCOL_HTTPS_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgClassesMk[] =
{
    //***** PROTOCOL_MK ENTRIES *****
    STD_ENTRY(PROTOCOL_MK_CLSID_REGKEY, PROTOCOL_MK_DESCRIP),
    STD_ENTRY(PROTOCOL_MK_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, PROTOCOL_MK_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgClassesFile[] =
{
    //***** PROTOCOL_FILE ENTRIES *****
    STD_ENTRY(PROTOCOL_FILE_CLSID_REGKEY, PROTOCOL_FILE_DESCRIP),
    STD_ENTRY(PROTOCOL_FILE_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, PROTOCOL_FILE_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};



#define HANDLER_HTTP        SZPROTOCOLROOT"http"
#define HANDLER_FTP         SZPROTOCOLROOT"ftp"
#define HANDLER_GOPHER      SZPROTOCOLROOT"gopher"
#define HANDLER_HTTPS       SZPROTOCOLROOT"https"
#define HANDLER_MK          SZPROTOCOLROOT"mk"
#define HANDLER_FILE        SZPROTOCOLROOT"file"
#define HANDLER_LOCAL       SZPROTOCOLROOT"local"


//const REGENTRY rgHandler[] = { STD_ENTRY(HANDLER_HTTP, PROTOCOL_HTTP_DESCRIP), { KEYTYPE_STRING, HANDLER_HTTP, "CLSID", REG_SZ, (BYTE*)PROTOCOL_HTTP_CLSID } };
const REGENTRY rgHandlerHttp   [] = { STD_ENTRY(HANDLER_HTTP  , PROTOCOL_HTTP_DESCRIP  ), { KEYTYPE_STRING, HANDLER_HTTP  , "CLSID", REG_SZ, (BYTE*)PROTOCOL_HTTP_CLSID   } };
const REGENTRY rgHandlerFtp    [] = { STD_ENTRY(HANDLER_FTP   , PROTOCOL_FTP_DESCRIP   ), { KEYTYPE_STRING, HANDLER_FTP   , "CLSID", REG_SZ, (BYTE*)PROTOCOL_FTP_CLSID    } };
const REGENTRY rgHandlerGopher [] = { STD_ENTRY(HANDLER_GOPHER, PROTOCOL_GOPHER_DESCRIP), { KEYTYPE_STRING, HANDLER_GOPHER, "CLSID", REG_SZ, (BYTE*)PROTOCOL_GOPHER_CLSID } };
const REGENTRY rgHandlerHttpS  [] = { STD_ENTRY(HANDLER_HTTPS , PROTOCOL_HTTPS_DESCRIP ), { KEYTYPE_STRING, HANDLER_HTTPS , "CLSID", REG_SZ, (BYTE*)PROTOCOL_HTTPS_CLSID  } };
const REGENTRY rgHandlerMk     [] = { STD_ENTRY(HANDLER_MK    , PROTOCOL_MK_DESCRIP    ), { KEYTYPE_STRING, HANDLER_MK    , "CLSID", REG_SZ, (BYTE*)PROTOCOL_MK_CLSID     } };
const REGENTRY rgHandlerFile   [] = { STD_ENTRY(HANDLER_FILE  , PROTOCOL_FILE_DESCRIP  ), { KEYTYPE_STRING, HANDLER_FILE  , "CLSID", REG_SZ, (BYTE*)PROTOCOL_FILE_CLSID   } };
const REGENTRY rgHandlerLocal  [] = { STD_ENTRY(HANDLER_LOCAL , PROTOCOL_FILE_DESCRIP  ), { KEYTYPE_STRING, HANDLER_LOCAL , "CLSID", REG_SZ, (BYTE*)PROTOCOL_FILE_CLSID   } };


#ifdef TEST_JOHANNP
#if DBG==1

#define NAMESPACE_TEST1                SZNAMESPACEROOT"http\\testhttp"
#define NAMESPACE_TEST1_DESCRIP       "Asychronous Pluggable NameSpace Handler for http"
#define NAMESPACE_TEST1_CLSID         "{79eac9e2-baf9-11ce-8c82-00aa004ba90b}"
#define NAMESPACE_TEST1_PROTOCOL       NAMESPACE_TEST1

const REGENTRY rgNameSpaceTest1  [] = { STD_ENTRY(NAMESPACE_TEST1 , NAMESPACE_TEST1_DESCRIP  ), { KEYTYPE_STRING, NAMESPACE_TEST1 , "CLSID", REG_SZ, (BYTE*)NAMESPACE_TEST1_CLSID   } };


// test2
#define NAMESPACE_TEST2                SZNAMESPACEROOT"*\\testall"
#define NAMESPACE_TEST2_DESCRIP       "Asychronous Pluggable NameSpace Handler for all protocols"
#define NAMESPACE_TEST2_CLSID         "{79eac9e2-baf9-11ce-8c82-00aa004ba90b}"
#define NAMESPACE_TEST2_PROTOCOL       NAMESPACE_TEST2

const REGENTRY rgNameSpaceTest2  [] = { STD_ENTRY(NAMESPACE_TEST2 , NAMESPACE_TEST2_DESCRIP  ), { KEYTYPE_STRING, NAMESPACE_TEST2 , "CLSID", REG_SZ, (BYTE*)NAMESPACE_TEST2_CLSID   } };

#endif
#endif //TEST_JOHANNP



// From PlugProt.dll (merge)
#define SZFILTERROOT        "PROTOCOLS\\Filter\\"
#define SZPROTOCOLROOT      "PROTOCOLS\\Handler\\"
#define SZCLASS             "CLSID"
#define SZHANDLER           "HANDLER"

//****************************** CLSID for pluggable protocols and filters
//const GUID CLSID_StdEncodingFilterFac;  {0x54c37cd0, 0xd944, 0x11d0, {0xa9, 0xf4, 0x00, 0x60, 0x97, 0x94, 0x23, 0x11}};
//const GUID CLSID_DeCompMimeFilter;    {0x8f6b0360, 0xb80d, 0x11d0, {0xa9, 0xb3, 0x00, 0x60, 0x97, 0x94, 0x23, 0x11}};
//const GUID CLSID_CdlProtocol;   {0x3dd53d40, 0x7b8b, 0x11d0, {0xb0, 0x13, 0x00, 0xaa, 0x00, 0x59, 0xce, 0x02}};
//const GUID CLSID_ClassInstallFilter; {0x32b533bb, 0xedae, 0x11d0, {0xbd, 0x5a, 0x0, 0xaa, 0x0, 0xb9, 0x2a, 0xf1}};
EXTERN_C const GUID CLSID_StdEncodingFilterFac;  
EXTERN_C const GUID CLSID_DeCompMimeFilter;    
EXTERN_C const GUID CLSID_CdlProtocol;   
EXTERN_C const GUID CLSID_ClassInstallFilter; 

//*************************** Registry keys for CDL protocol handler

#define HANDLER_CDL                            SZPROTOCOLROOT"cdl"
#define PROTOCOL_CDL_CLSID                     "{3dd53d40-7b8b-11D0-b013-00aa0059ce02}"
#define PROTOCOL_CDL_CLSID_REGKEY              "CLSID\\"PROTOCOL_CDL_CLSID
#define PROTOCOL_CDL_DESCRIP                   "CDL: Asychronous Pluggable Protocol Handler"

const REGENTRY rgHandlerCdl[] = 
{ 
    STD_ENTRY(HANDLER_CDL  , PROTOCOL_CDL_DESCRIP  ), 
        { KEYTYPE_STRING, HANDLER_CDL  , "CLSID", REG_SZ, (BYTE*)PROTOCOL_CDL_CLSID   } 
};

const REGENTRY rgClassesCdl[] =
{
    STD_ENTRY(PROTOCOL_CDL_CLSID_REGKEY, PROTOCOL_CDL_DESCRIP),
    STD_ENTRY(PROTOCOL_CDL_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, PROTOCOL_CDL_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

//*************************** Registry keys for Class Install Handler protocol filter

#define PROT_FILTER_CLASS               SZFILTERROOT"Class Install Handler"
#define PROT_FILTER_CLASS_DESCRIP       "AP Class Install Handler filter"
#define PROT_FILTER_CLASS_CLSID         "{32B533BB-EDAE-11d0-BD5A-00AA00B92AF1}"
#define PROT_FILTER_CLASS_CLSID_REGKEY  "CLSID\\"PROT_FILTER_CLASS_CLSID
#define PROT_FILTER_CLASS_PROTOCOL      PROT_FILTER_CLASS

const REGENTRY rgClassesMimeInstallHandler[] =
{
    STD_ENTRY(PROT_FILTER_CLASS_CLSID_REGKEY, PROT_FILTER_CLASS_DESCRIP),
    STD_ENTRY(PROT_FILTER_CLASS_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, PROT_FILTER_CLASS_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgMimeInstallHandler[] = 
{ 
    STD_ENTRY(PROT_FILTER_CLASS , PROT_FILTER_CLASS_DESCRIP  ), 
        { KEYTYPE_STRING, PROT_FILTER_CLASS, "CLSID", REG_SZ, (BYTE*)PROT_FILTER_CLASS_CLSID   } 
};

//*************************** Registry keys for ENC & Deflate protocol filters

#define PROT_FILTER_ENC                 SZFILTERROOT"lzdhtml"
#define PROT_FILTER_ENC_DESCRIP         "AP lzdhtml encoding/decoding Filter"
#define PROT_FILTER_ENC_CLSID           "{8f6b0360-b80d-11d0-a9b3-006097942311}"
#define PROT_FILTER_ENC_CLSID_REGKEY    "CLSID\\"PROT_FILTER_ENC_CLSID
#define PROT_FILTER_ENC_PROTOCOL        PROT_FILTER_ENC

const REGENTRY rgClassesMimeHandlerEnc[] =
{
    STD_ENTRY(PROT_FILTER_ENC_CLSID_REGKEY, PROT_FILTER_ENC_DESCRIP),
    STD_ENTRY(PROT_FILTER_ENC_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, PROT_FILTER_ENC_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgMimeHandlerEnc  [] = 
{ 
    STD_ENTRY(PROT_FILTER_ENC , PROT_FILTER_ENC_DESCRIP  ), 
    { KEYTYPE_STRING, PROT_FILTER_ENC , "CLSID", REG_SZ, (BYTE*)PROT_FILTER_ENC_CLSID   } 
};

#define PROT_FILTER_DEFLATE            SZFILTERROOT"deflate"
#define PROT_FILTER_DEFLATE_DESCRIP    "AP Deflate Encoding/Decoding Filter "
#define PROT_FILTER_DEFLATE_CLSID      "{8f6b0360-b80d-11d0-a9b3-006097942311}"

const REGENTRY rgDeflateEnc  [] = 
{ 
    STD_ENTRY(PROT_FILTER_DEFLATE , PROT_FILTER_DEFLATE_DESCRIP  ), 
    { KEYTYPE_STRING, PROT_FILTER_DEFLATE , "CLSID", REG_SZ, (BYTE*)PROT_FILTER_DEFLATE_CLSID   } 
};

#define PROT_FILTER_GZIP               SZFILTERROOT"gzip"
#define PROT_FILTER_GZIP_DESCRIP       "AP GZIP Encoding/Decoding Filter "
#define PROT_FILTER_GZIP_CLSID         "{8f6b0360-b80d-11d0-a9b3-006097942311}"

const REGENTRY rgGZIPEnc  [] = 
{ 
    STD_ENTRY(PROT_FILTER_GZIP , PROT_FILTER_GZIP_DESCRIP  ), 
    { KEYTYPE_STRING, PROT_FILTER_GZIP , "CLSID", REG_SZ, (BYTE*)PROT_FILTER_GZIP_CLSID   } 
};

#define STD_ENC_FAC_CLSID           "{54c37cd0-d944-11d0-a9f4-006097942311}" 
#define STD_ENC_FAC_CLSID_REGKEY    "CLSID\\"STD_ENC_FAC_CLSID
const REGENTRY rgClassesStdEncFac[] =
{
    STD_ENTRY(STD_ENC_FAC_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, STD_ENC_FAC_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};


const REGENTRYGROUP rgRegEntryGroups[] = {
    { HKEY_CLASSES_ROOT, rgStdNotificationMgr,      ARRAYSIZE(rgStdNotificationMgr) },
    { HKEY_CLASSES_ROOT, rgClassesRoot,     ARRAYSIZE(rgClassesRoot) },
    { HKEY_CLASSES_ROOT, rgClassesSoftDist,  ARRAYSIZE(rgClassesSoftDist) },
    { HKEY_CLASSES_ROOT, rgClassesSecMgr,   ARRAYSIZE(rgClassesSecMgr) },
    { HKEY_CLASSES_ROOT, rgClassesZoneMgr,   ARRAYSIZE(rgClassesZoneMgr) },
    { HKEY_CLASSES_ROOT, rgClassesBindCtx,  ARRAYSIZE(rgClassesBindCtx) },
    { HKEY_CLASSES_ROOT, rgPSFactory,       ARRAYSIZE(rgPSFactory) },
    { HKEY_CLASSES_ROOT, rgClassesHttp,     ARRAYSIZE(rgClassesHttp) },
    { HKEY_CLASSES_ROOT, rgClassesFtp,      ARRAYSIZE(rgClassesFtp) },
    { HKEY_CLASSES_ROOT, rgClassesGopher,   ARRAYSIZE(rgClassesGopher) },
    { HKEY_CLASSES_ROOT, rgClassesHttpS,    ARRAYSIZE(rgClassesHttpS) },
    { HKEY_CLASSES_ROOT, rgClassesMk,       ARRAYSIZE(rgClassesMk) },
    { HKEY_CLASSES_ROOT, rgClassesFile,     ARRAYSIZE(rgClassesFile) },

    { HKEY_CLASSES_ROOT, rgHandlerHttp  ,   ARRAYSIZE(rgHandlerHttp  ) },
    { HKEY_CLASSES_ROOT, rgHandlerFtp   ,   ARRAYSIZE(rgHandlerFtp   ) },
    { HKEY_CLASSES_ROOT, rgHandlerGopher,   ARRAYSIZE(rgHandlerGopher) },
    { HKEY_CLASSES_ROOT, rgHandlerHttpS ,   ARRAYSIZE(rgHandlerHttpS ) },
    { HKEY_CLASSES_ROOT, rgHandlerMk    ,   ARRAYSIZE(rgHandlerMk    ) },
    { HKEY_CLASSES_ROOT, rgHandlerFile  ,   ARRAYSIZE(rgHandlerFile  ) },
    { HKEY_CLASSES_ROOT, rgHandlerLocal ,   ARRAYSIZE(rgHandlerLocal ) },
    { HKEY_CLASSES_ROOT, rgMimeHandlerEnc,        ARRAYSIZE(rgMimeHandlerEnc) },
    { HKEY_CLASSES_ROOT, rgClassesMimeHandlerEnc, ARRAYSIZE(rgClassesMimeHandlerEnc) },

    { HKEY_CLASSES_ROOT, rgDeflateEnc,        ARRAYSIZE(rgDeflateEnc) },
    { HKEY_CLASSES_ROOT, rgGZIPEnc,  ARRAYSIZE(rgGZIPEnc) },
    { HKEY_CLASSES_ROOT, rgClassesStdEncFac,  ARRAYSIZE(rgClassesStdEncFac) },
    
    { HKEY_CLASSES_ROOT, rgClassesMimeInstallHandler,      ARRAYSIZE(rgClassesMimeInstallHandler) },
    { HKEY_CLASSES_ROOT, rgMimeInstallHandler,             ARRAYSIZE(rgMimeInstallHandler) },

    { HKEY_CLASSES_ROOT, rgClassesCdl,      ARRAYSIZE(rgClassesCdl) },
    { HKEY_CLASSES_ROOT, rgHandlerCdl  ,    ARRAYSIZE(rgHandlerCdl) },
#ifdef TEST_JOHANNP
#if DBG==1
    { HKEY_CLASSES_ROOT, rgNameSpaceTest1 ,   ARRAYSIZE(rgNameSpaceTest1 ) },
    { HKEY_CLASSES_ROOT, rgNameSpaceTest2 ,   ARRAYSIZE(rgNameSpaceTest2 ) },
#endif // DBG
#endif //TEST_JOHANNP
    { NULL, NULL, 0 }       // terminator
};


const REGENTRYGROUP rgRegEntryGroupsDel[] = {
    { HKEY_CLASSES_ROOT, rgStdNotificationMgr,      ARRAYSIZE(rgStdNotificationMgr) },
    { HKEY_CLASSES_ROOT, rgClassesRoot,     ARRAYSIZE(rgClassesRoot) },
    { HKEY_CLASSES_ROOT, rgClassesSoftDist,  ARRAYSIZE(rgClassesSoftDist) },
    { HKEY_CLASSES_ROOT, rgClassesSecMgr,   ARRAYSIZE(rgClassesSecMgr) },
    { HKEY_CLASSES_ROOT, rgClassesZoneMgr,   ARRAYSIZE(rgClassesZoneMgr) },
    { HKEY_CLASSES_ROOT, rgClassesBindCtx,  ARRAYSIZE(rgClassesBindCtx) },
    { HKEY_CLASSES_ROOT, rgPSFactory,       ARRAYSIZE(rgPSFactory) },
    { HKEY_CLASSES_ROOT, rgClassesHttp,     ARRAYSIZE(rgClassesHttp) },
    { HKEY_CLASSES_ROOT, rgClassesFtp,      ARRAYSIZE(rgClassesFtp) },
    { HKEY_CLASSES_ROOT, rgClassesGopher,   ARRAYSIZE(rgClassesGopher) },
    { HKEY_CLASSES_ROOT, rgClassesHttpS,    ARRAYSIZE(rgClassesHttpS) },
    { HKEY_CLASSES_ROOT, rgClassesMk,       ARRAYSIZE(rgClassesMk) },
    { HKEY_CLASSES_ROOT, rgClassesFile,     ARRAYSIZE(rgClassesFile) },

    { HKEY_CLASSES_ROOT, rgHandlerFtp   ,   ARRAYSIZE(rgHandlerFtp   ) },
    { HKEY_CLASSES_ROOT, rgHandlerGopher,   ARRAYSIZE(rgHandlerGopher) },
    { HKEY_CLASSES_ROOT, rgHandlerMk    ,   ARRAYSIZE(rgHandlerMk    ) },
    { HKEY_CLASSES_ROOT, rgHandlerFile  ,   ARRAYSIZE(rgHandlerFile  ) },
    { HKEY_CLASSES_ROOT, rgHandlerLocal ,   ARRAYSIZE(rgHandlerLocal ) },
    { HKEY_CLASSES_ROOT, rgMimeHandlerEnc,        ARRAYSIZE(rgMimeHandlerEnc) },
    { HKEY_CLASSES_ROOT, rgClassesMimeHandlerEnc, ARRAYSIZE(rgClassesMimeHandlerEnc) },

    { HKEY_CLASSES_ROOT, rgDeflateEnc,        ARRAYSIZE(rgDeflateEnc) },
    { HKEY_CLASSES_ROOT, rgGZIPEnc,  ARRAYSIZE(rgGZIPEnc) },
    { HKEY_CLASSES_ROOT, rgClassesStdEncFac,  ARRAYSIZE(rgClassesStdEncFac) },
    
    { HKEY_CLASSES_ROOT, rgClassesMimeInstallHandler,      ARRAYSIZE(rgClassesMimeInstallHandler) },
    { HKEY_CLASSES_ROOT, rgMimeInstallHandler,             ARRAYSIZE(rgMimeInstallHandler) },

    { HKEY_CLASSES_ROOT, rgClassesCdl,      ARRAYSIZE(rgClassesCdl) },
    { HKEY_CLASSES_ROOT, rgHandlerCdl  ,    ARRAYSIZE(rgHandlerCdl) },
#ifdef TEST_JOHANNP
#if DBG==1
    { HKEY_CLASSES_ROOT, rgNameSpaceTest1 ,   ARRAYSIZE(rgNameSpaceTest1 ) },
    { HKEY_CLASSES_ROOT, rgNameSpaceTest2 ,   ARRAYSIZE(rgNameSpaceTest2 ) },
#endif // DBG
#endif //TEST_JOHANNP
    { NULL, NULL, 0 }       // terminator
};


//+---------------------------------------------------------------------------
//
//  Function:   DllAddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    12-10-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void DllAddRef(void)
{
    g_cRef++;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllRelease
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    12-10-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void DllRelease(void)
{
    UrlMkAssert((g_cRef > 0));
    if (g_cRef > 0)
    {
        g_cRef--;
    }
}

//+---------------------------------------------------------------------------
//
//  Operator:   new
//
//  Synopsis:
//
//  Arguments:  [size] --
//
//  Returns:
//
//  History:    12-10-95   JohannP (Johann Posch)   Created
//
//  Notes:      BUBUG: get and use IMalloc
//
//----------------------------------------------------------------------------
void * _cdecl operator new(size_t size)
{
    void * pBuffer;
    pBuffer = CoTaskMemAlloc(size);
    if (pBuffer)
    {
        memset(pBuffer,0, size);
    }
    return pBuffer;
}

void * _cdecl operator new(size_t sizeEl, ULONG cEl)
{
    void * pBuffer;
    size_t size = sizeEl * cEl;
    pBuffer = CoTaskMemAlloc(size);
    if (pBuffer)
    {
        memset(pBuffer,0, size);
    }
    return pBuffer;
}


//+---------------------------------------------------------------------------
//
//  Operator:   delete
//
//  Synopsis:
//
//  Arguments:  [lpv] --
//
//  Returns:
//
//  History:    2-14-96   JohannP (Johann Posch)   Created
//
//  Notes:      BUBUG: get and use IMalloc
//
//----------------------------------------------------------------------------
void _cdecl operator delete(void *lpv)
{
    //UrlMkAssert((lpv != NULL));
    if (lpv == NULL)
    {
        return;
    }

    CoTaskMemFree(lpv);
}

#ifdef UNUSED
//
int _cdecl _purecall( void )
{
    UrlMkAssert(FALSE);
    return 0;
}

#endif //UNUSED

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Dll entry point
//
//  Arguments:  [clsid] - class id for new class
//              [iid] - interface required of class
//              [ppv] - where to put new interface
//
//  Returns:    S_OK - class object created successfully created.
//
//
//--------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv)
{
    UrlMkDebugOut((DEB_URLMON, "API _IN DllGetClassObject\n"));

    HRESULT hr = E_NOTIMPL;

    if (   (clsid == CLSID_StdURLMoniker)
        || (clsid == CLSID_UrlMkBindCtx)
        || (clsid == CLSID_HttpSProtocol )
        || (clsid == CLSID_HttpProtocol  )
        || (clsid == CLSID_FtpProtocol   )
        || (clsid == CLSID_GopherProtocol)
        || (clsid == CLSID_FileProtocol  )
        || (clsid == CLSID_MkProtocol    )
        || (clsid == CLSID_SoftDistExt    )
        || (clsid == CLSID_InternetSecurityManager     )
        || (clsid == CLSID_InternetZoneManager    )
        || (clsid == CLSID_DeCompMimeFilter)
        || (clsid == CLSID_StdEncodingFilterFac)
        || (clsid == CLSID_CdlProtocol)
        || (clsid == CLSID_ClassInstallFilter) 
       )
    {
        CUrlClsFact *pCF = NULL;
        hr = CUrlClsFact::Create(clsid, &pCF);
        if (hr == NOERROR)
        {
            UrlMkAssert((pCF != NULL));
            hr = pCF->QueryInterface(iid, ppv);
            pCF->Release();
        }
    }
    else
    {
        hr = PrxDllGetClassObject(clsid, iid, ppv);
    }

    DumpIID(clsid);

    UrlMkDebugOut((DEB_URLMON, "API OUT DllGetClassObject (hr:%lx, ppv:%p)\n",hr,*ppv));
    return hr;
}
//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:
//
//  Arguments:  [hDll]          - a handle to the dll instance
//              [dwReason]      - the reason LibMain was called
//              [lpvReserved]   - NULL - called due to FreeLibrary
//                              - non-NULL - called due to process exit
//
//  Returns:    TRUE on success, FALSE otherwise
//
//  Notes:
//
//              The officially approved DLL entrypoint name is DllMain. This
//              entry point will be called by the CRT Init function.
//
//
//--------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID lpvReserved)
{
    BOOL fResult = TRUE;
    UrlMkDebugOut((DEB_DLL,"DllMain:%lx\n", dwReason));

    PrxDllMain(hInstance, dwReason, lpvReserved);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
#if DBG==1
        {
            UrlMkInfoLevel = (DWORD) GetProfileIntA("UrlMon","UrlMk", (DEB_ERROR | DEB_WARN)) & DEB_LEVEL_MASK;
            TransInfoLevel = (DWORD) GetProfileIntA("UrlMon","Trans", (DEB_ERROR | DEB_WARN)) & DEB_LEVEL_MASK;
            PProtInfoLevel = (DWORD) GetProfileIntA("UrlMon","PProt", (DEB_ERROR | DEB_WARN)) & DEB_LEVEL_MASK;
            NotfInfoLevel  = (DWORD) GetProfileIntA("UrlMon","Notf",  (DEB_ERROR | DEB_WARN)) & DEB_LEVEL_MASK;
            EProtInfoLevel = (DWORD) GetProfileIntA("UrlMon","EProt", (DEB_ERROR | DEB_WARN)) & DEB_LEVEL_MASK;
            TNotfInfoLevel = (DWORD) GetProfileIntA("UrlMon","TNotf",  (DEB_ERROR | DEB_WARN)) & DEB_LEVEL_MASK;
            g_dwSettings   = (DWORD) GetProfileIntA("UrlMon","Global", (DEB_ERROR | DEB_WARN));
            Win4AssertLevel= (ULONG) GetProfileIntA("UrlMon","AssertLevel", 0);

        // enable encoding handler
        // g_dwSettings |= 0x00100000;
        }
#endif //DBG==1

        //_asm int 3;
        PerfDbgLog(tagUrlDll, NULL, "+URLMON DLL_PROCESS_ATTACH");
        g_pszUserAgentString = NULL;
        g_pszUAInfoString = GetUAInfoString();
        // We ignore the return code of ZonesInit because other parts of urlmon
        // could still function fine if we can't init zones. 
        ZonesInit( );

        g_bCanUseSimpleBinding = CanUseSimpleBinding();
        g_bGlobalUTF8Enabled = GlobalUTF8Enabled();
        g_pHeadURLMONTSList = NULL; 

        OSVERSIONINFO osvi;

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
        if (GetVersionEx(&osvi))
        {
            g_bNT5OrGreater = (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion >= 5);
        }

        fResult = TlsDllMain(hInstance, dwReason, lpvReserved);
        PerfDbgLog(tagUrlDll, NULL, "-URLMON DLL_PROCESS_ATTACH");
        break;

    case DLL_PROCESS_DETACH:
        if (g_pszUserAgentString != NULL)
        {
            delete g_pszUserAgentString;
            g_pszUserAgentString = NULL;
        }
        if (g_pszUAInfoString != NULL)
        {
            delete g_pszUAInfoString;
            g_pszUAInfoString = NULL;
        }

        if (g_hLibPluginOcx)
        {
            FreeLibrary(g_hLibPluginOcx);
        }

        if (g_hLibMlang)
        {
            FreeLibrary(g_hLibMlang);
        }

        ZonesUnInit( );

        g_bCanUseSimpleBinding = TRUE;

        if (g_pEFmtETC)
        {
            g_pEFmtETC->Release();
        }

        if(g_pHeadURLMONTSList) 
        {
            //
            // if    lpvReserved == NULL   - called due to FreeLibrary
            // else                        - called due to process Terminate
            //
            // Since we are using SendMessage to kill the thread notification 
            // window, we should only do that when FreeLibraray() happens
            //
            if( !lpvReserved )
            {
                CleanupTSOnProcessDetach();   
            }
        }

        // Fall through

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        fResult = TlsDllMain(hInstance, dwReason, lpvReserved);
    }
    UrlMkDebugOut((DEB_DLL,"DllMain: done\n"));

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    //_asm int 3;
    return (g_cRef ? S_FALSE : S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    5-03-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllRegisterServer()
{
    UrlMkDebugOut((DEB_URLMON, "API _IN DllRegisterServer\n"));
    HRESULT hr;

    // don't register the proxies now
    // PrxDllRegisterServer();
    PrxDllRegisterServer();

    hr = HrDllRegisterServer(rgRegEntryGroups, g_hInst, NULL /*pfnLoadString*/);


    UrlMkDebugOut((DEB_URLMON, "API OUT DllRegisterServer (hr:%lx)\n",hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    5-03-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllUnregisterServer()
{
    UrlMkDebugOut((DEB_URLMON, "API _IN DllUnregisterServer\n"));
    HRESULT hr;

    // don't register the proxies now
    //PrxDllUnregisterServer();
    hr = HrDllUnregisterServer(rgRegEntryGroupsDel, g_hInst, NULL /*pfnLoadString*/);

    UrlMkDebugOut((DEB_URLMON, "API OUT DllUnregisterServer (hr:%lx)\n",hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllRegisterServerEx
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    5-03-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllRegisterServerEx()
{
    UrlMkDebugOut((DEB_URLMON, "API _IN DllRegisterServerEx\n"));

    HRESULT hr = E_NOTIMPL;

    UrlMkDebugOut((DEB_URLMON, "API OUT DllRegisterServerEx (hr:%lx)\n",hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllInstall
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    6-17-97   SanjayS (Sanjay Shenoy)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllInstall(BOOL bInstall, LPCWSTR pwStr)
{
    UrlMkDebugOut((DEB_URLMON, "API _IN DllInstall\n"));

#ifdef UNIX
    /*
     * On Unix, regsetup always passes in L"" for pwStr.
     */
    HRESULT hr;

    if (pwStr && !wcscmp(pwStr, L""))
       hr = ZonesDllInstall(bInstall, L"HKCU");
    else
       hr = ZonesDllInstall(bInstall, pwStr);
#else
    HRESULT hr = ZonesDllInstall(bInstall, pwStr);
#endif /* UNIX */

    UrlMkDebugOut((DEB_URLMON, "API OUT DllInstall (hr:%lx)\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoBuildVersion
//
//  Synopsis:   Return build version DWORD
//
//  Returns:    DWORD hiword = 23
//              DWORD loword = build number
//
//  Notes:      The high word must always be constant for a given platform.
//              For Win16 it must be exactly 23 (because that's what 16-bit
//              OLE 2.01 shipped with).  We can choose a different high word
//              for other platforms.  The low word must be greater than 639
//              (also because that's what 16-bit OLE 2.01 shipped with).
//
//--------------------------------------------------------------------------

STDAPI_(DWORD)  UrlMkBuildVersion( VOID )
{
    WORD wLowWord;
    WORD wHighWord;


    wHighWord = 23;
    //wLowWord  = rmm;    //  from ih\verole.h
    wLowWord  = 1;    //  from ih\verole.h

    //Win4Assert(wHighWord == 23 && "CoBuildVersion high word magic number");
    //Win4Assert(wLowWord > 639 && "CoBuildVersion low word not large enough");

    DWORD dwVersion;

    dwVersion = MAKELONG(wLowWord, wHighWord);

    return dwVersion;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetUAInfoString
//
//  Synopsis:   Creates the extra UA header string
//
//  Arguments:
//
//  Returns:    pointer to newly allocated string
//
//  History:    5-27-96   JoeS (Joe Souza)  Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR GetUAInfoString(void)
{
    BOOL    bRet;

    CHAR    szINTEL[] = "x86";
    CHAR    szMIPS[] = "MIPS";
    CHAR    szALPHA[] = "Alpha";
    CHAR    szPPC[] = "PPC";
    CHAR    *pszProcessorString = szINTEL;

    CHAR    szExtraUAInfo[] = "UA-CPU: %s\r\n";
    #define SZUSERAGENTMAX 256
    static char vszBuffer[SZUSERAGENTMAX] = "";

    LPSTR   pszTmp;
    BOOL    fIsNT = FALSE;

    // Get all needed info.
    OSVERSIONINFO   osvi;
    SYSTEM_INFO si;

    {
        memset(&osvi, 0, sizeof(osvi));

        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx(&osvi);
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            // We are running on NT
            fIsNT = TRUE;
            memset(&si, 0, sizeof(si));

            GetSystemInfo(&si);

            switch (si.wProcessorArchitecture)
            {
            case PROCESSOR_ARCHITECTURE_MIPS:
                pszProcessorString = szMIPS;
                break;

            case PROCESSOR_ARCHITECTURE_ALPHA:
                pszProcessorString = szALPHA;
                break;

            case PROCESSOR_ARCHITECTURE_PPC:
                pszProcessorString = szPPC;
                break;
            }
        }
    }

    // Build header string.
    wsprintf(vszBuffer, szExtraUAInfo, pszProcessorString);

    pszTmp = new CHAR [strlen(vszBuffer) + 1];

    if (pszTmp )
    { 
        if(fIsNT && si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL )
            strcpy(pszTmp, vszBuffer);
        else 
            pszTmp[0] = '\0';
    }

    return pszTmp;
}

#if DBG==1

#include <sem.hxx>
CMutexSem   mxs;

IDebugOut *v_pPProtDbgOut = NULL;
IDebugOut *v_pTransDbgOut = NULL;
IDebugOut *v_pUrlMkDbgOut = NULL;
IDebugOut *v_pNotfDbgOut = NULL;
IDebugOut *v_pEProtDbgOut = NULL;
IDebugOut *v_pTNotfDbgOut = NULL;

void UrlSpyFn(int iOption, const char *pscFormat, ...)
{
    CLock lck(mxs);
    
    static char szOutBuffer[2048];
    static DWORD * apiLevel[] = { &UrlMkInfoLevel, &TransInfoLevel, &PProtInfoLevel, &NotfInfoLevel, &EProtInfoLevel, &TNotfInfoLevel };
    static IDebugOut ** apDbgOut[] = { &v_pUrlMkDbgOut, &v_pTransDbgOut, &v_pPProtDbgOut,&v_pNotfDbgOut, &v_pEProtDbgOut, &v_pTNotfDbgOut };
    int iIndex = iOption >> DEB_LEVEL_SHIFT;
    int iLevel = *apiLevel[iIndex];

    if ((iOption & iLevel) == 0)
        return;

    DWORD tid = GetCurrentThreadId();
    DWORD cbBufLen;
    IDebugOut * pDbgOut = *apDbgOut[iIndex];

    wsprintf(szOutBuffer, "%08x> ", tid);
    cbBufLen = strlen(szOutBuffer);

    va_list args;
    va_start(args, pscFormat);
    vsprintf(szOutBuffer + cbBufLen, pscFormat, args);
    va_end(args);
    UrlSpySendEntry(pDbgOut, szOutBuffer, tid, iOption & DEB_LEVEL_MASK, 0);
}

void PerfDbgLogFn(int tag, void * pvObj, const char * pchFmt, ...)
{
    CLock lck(mxs);
    
    static char szOutBuffer[2048];
    static DWORD * apiLevel[] = { &UrlMkInfoLevel, &TransInfoLevel, &PProtInfoLevel, &NotfInfoLevel, &EProtInfoLevel, &TNotfInfoLevel };
    static IDebugOut ** apDbgOut[] = { &v_pUrlMkDbgOut, &v_pTransDbgOut, &v_pPProtDbgOut, &v_pEProtDbgOut };
    int iIndex = min(tag >> DEB_LEVEL_SHIFT, 2);
    int iLevel = *apiLevel[iIndex];

    if ((tag & iLevel) == 0)
        return;

    DWORD tid = GetCurrentThreadId();
    DWORD cbBufLen;
    IDebugOut * pDbgOut = *apDbgOut[iIndex];

    sprintf(szOutBuffer, "%08x> %p %s", tid, pvObj,
        *pchFmt == '+' ? "_IN " : (*pchFmt == '-' ? "OUT " : ""));
    cbBufLen = strlen(szOutBuffer);

    if (*pchFmt == '+' || *pchFmt == '-')
        pchFmt += 1;

    va_list args;
    va_start(args, pchFmt);
    vsprintf(szOutBuffer + cbBufLen, pchFmt, args);
    lstrcat(szOutBuffer, "\n");
    va_end(args);
    UrlSpySendEntry(pDbgOut, szOutBuffer, tid, tag & DEB_LEVEL_MASK, 0);
}

void UrlSpySendEntry(IDebugOut *pDbgOut, LPSTR szOutBuffer, DWORD ThreadId, DWORD dwFlags, DWORD dwReserved)
{
    if (pDbgOut)
    {
        pDbgOut->SendEntry(ThreadId, dwFlags, szOutBuffer, dwReserved);
    }
    else
    {
        OutputDebugString(szOutBuffer);
    }
}

HRESULT RegisterDebugOut(LPCWSTR pwzName, DWORD dwOptions, IDebugOut *pDbgOut, DWORD dwReserved)
{
    if (pwzName)
    {

        if (!wcsicmp(pwzName, L"UrlMk"))
        {
            UrlMkInfoLevel = dwOptions;
            if (v_pUrlMkDbgOut)
            {
                v_pUrlMkDbgOut->Release();
                v_pUrlMkDbgOut = NULL;
            }
            if (pDbgOut)
            {

                v_pUrlMkDbgOut = pDbgOut;
                pDbgOut->AddRef();
            }

        }
        if (!wcsicmp(pwzName, L"Trans"))
        {
            TransInfoLevel = dwOptions;
            if (v_pTransDbgOut)
            {
                v_pTransDbgOut->Release();
                v_pTransDbgOut = NULL;
            }
            if (pDbgOut)
            {

                v_pTransDbgOut = pDbgOut;
                pDbgOut->AddRef();
            }

        }
        if (!wcsicmp(pwzName, L"PProt"))
        {
            PProtInfoLevel = dwOptions;
            if (v_pPProtDbgOut)
            {
                v_pPProtDbgOut->Release();
                v_pPProtDbgOut = NULL;
            }
            if (pDbgOut)
            {

                v_pPProtDbgOut = pDbgOut;
                pDbgOut->AddRef();
            }

        }
        if (!wcsicmp(pwzName, L"Notf"))
        {
            NotfInfoLevel = dwOptions;
            if (v_pNotfDbgOut)
            {
                v_pNotfDbgOut->Release();
                v_pNotfDbgOut = NULL;
            }
            if (pDbgOut)
            {

                v_pNotfDbgOut = pDbgOut;
                pDbgOut->AddRef();
            }

        }
        if (!wcsicmp(pwzName, L"EProt"))
        {
            EProtInfoLevel = dwOptions;
            if (v_pEProtDbgOut)
            {
                v_pEProtDbgOut->Release();
                v_pEProtDbgOut = NULL;
            }
            if (pDbgOut)
            {

                v_pEProtDbgOut = pDbgOut;
                pDbgOut->AddRef();
            }

        }
        if (!wcsicmp(pwzName, L"TNotf"))
        {
            TNotfInfoLevel = dwOptions;
            if (v_pTNotfDbgOut)
            {
                v_pTNotfDbgOut->Release();
                v_pTNotfDbgOut = NULL;
            }
            if (pDbgOut)
            {

                v_pTNotfDbgOut = pDbgOut;
                pDbgOut->AddRef();
            }

        }


    }

    return NOERROR;
}


#endif //DBG==1

/*
const REGENTRY rgClassesRes[] =
{
    //***** PROTOCOL_RES ENTRIES *****
    STD_ENTRY(PROTOCOL_RES_CLSID_REGKEY, PROTOCOL_RES_DESCRIP),
    STD_ENTRY(PROTOCOL_RES_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
        { KEYTYPE_STRING, PROTOCOL_RES_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};
*/

/*
const REGENTRY rgClasses[] =
{
    //***** URLMONIKER ENTRIES *****
        STD_ENTRY(URLMONIKER_CLSID_REGKEY, URLMONIKER_DESCRIP),
        STD_ENTRY(URLMONIKER_CLSID_REGKEY"\\InprocServer32", "%s"URLMON_NAME),
{ KEYTYPE_STRING, URLMONIKER_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};
*/


