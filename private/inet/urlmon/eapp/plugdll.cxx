//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       plugdll.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <eapp.h>


#define SZFILTERROOT        "PROTOCOLS\\Filter\\"
#define SZPROTOCOLROOT      "PROTOCOLS\\Handler\\"
#define SZCLASS             "CLSID"
#define SZHANDLER           "HANDLER"

#ifdef EAPP_TEST
const GUID CLSID_ResProtocol        = {0x79eaca00, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b}};
const GUID CLSID_OhServNameSp       = {0x79eaca01, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b}};
const GUID CLSID_MimeHandlerTest1   = {0x79eaca02, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b}};


const GUID CLSID_NotificaitonTest1  = {0xc733e501, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
const GUID CLSID_NotificaitonTest2  = {0xc733e502, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
const GUID CLSID_NotificaitonTest3  = {0xc733e503, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
const GUID CLSID_NotificaitonTest4  = {0xc733e504, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
#endif // EAPP_TEST

//****************************** CLSID for pluggable protocols and filters
const GUID CLSID_StdEncodingFilterFac= {0x54c37cd0, 0xd944, 0x11d0, {0xa9, 0xf4, 0x00, 0x60, 0x97, 0x94, 0x23, 0x11}};
const GUID CLSID_DeCompMimeFilter   = {0x8f6b0360, 0xb80d, 0x11d0, {0xa9, 0xb3, 0x00, 0x60, 0x97, 0x94, 0x23, 0x11}};
const GUID CLSID_CdlProtocol        = {0x3dd53d40, 0x7b8b, 0x11d0, {0xb0, 0x13, 0x00, 0xaa, 0x00, 0x59, 0xce, 0x02}};
const GUID CLSID_ClassInstallFilter = {0x32b533bb, 0xedae, 0x11d0, {0xbd, 0x5a, 0x0, 0xaa, 0x0, 0xb9, 0x2a, 0xf1}};

DECLARE_INFOLEVEL(EProt)
HINSTANCE g_hInst = NULL;

HINSTANCE g_hInst_LZDHtml = NULL;
HINSTANCE g_hInst_Deflate = NULL;
HINSTANCE g_hInst_GZIP = NULL;

// global variables
CRefCount g_cRef(0);        // global dll refcount

#define DLL_NAME      "plugprot.dll"

STDAPI_(BOOL) TlsDllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved);

#ifdef EAPP_TEST
#define HANDLER_RES                            SZPROTOCOLROOT"res"
#define PROTOCOL_RES_CLSID                     "{79eaca00-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_RES_CLSID_REGKEY              "CLSID\\"PROTOCOL_RES_CLSID
#define PROTOCOL_RES_DESCRIP                   "res: Asychronous Pluggable Protocol Handler"

// protocols
//***** PROTOCOL_RES ENTRIES *****
const REGENTRY rgClassesRes[] =
{
    STD_ENTRY(PROTOCOL_RES_CLSID_REGKEY, PROTOCOL_RES_DESCRIP),
    STD_ENTRY(PROTOCOL_RES_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
        { KEYTYPE_STRING, PROTOCOL_RES_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgHandlerRes   [] = { STD_ENTRY(HANDLER_RES  , PROTOCOL_RES_DESCRIP  ), { KEYTYPE_STRING, HANDLER_RES  , "CLSID", REG_SZ, (BYTE*)PROTOCOL_RES_CLSID   } };

#define NAMESPACE_OHSERV                SZNAMESPACEROOT"http\\ohserv"
#define NAMESPACE_OHSERV_DESCRIP       "Asychronous Pluggable NameSpace Handler for http to ohserv"
#define NAMESPACE_OHSERV_CLSID         "{79eaca01-baf9-11ce-8c82-00aa004ba90b}"
#define NAMESPACE_OHSERV_CLSID_REGKEY  "CLSID\\"NAMESPACE_OHSERV_CLSID
#define NAMESPACE_OHSERV_PROTOCOL       NAMESPACE_OHSERV

const REGENTRY rgClassesOhserv[] =
{
    STD_ENTRY(NAMESPACE_OHSERV_CLSID_REGKEY, NAMESPACE_OHSERV_DESCRIP),
    STD_ENTRY(NAMESPACE_OHSERV_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
        { KEYTYPE_STRING, NAMESPACE_OHSERV_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgNameSpaceOhserv  [] = 
{ 
    STD_ENTRY(NAMESPACE_OHSERV , NAMESPACE_OHSERV_DESCRIP  ), 
    { KEYTYPE_STRING, NAMESPACE_OHSERV , "CLSID", REG_SZ, (BYTE*)NAMESPACE_OHSERV_CLSID   } 
};


#define PROT_FILTER_TEST1               SZFILTERROOT"text/html"
#define PROT_FILTER_TEST1_DESCRIP       "Asychronous Pluggable Mime Handler for text/mhtml"
#define PROT_FILTER_TEST1_CLSID         "{79eaca02-baf9-11ce-8c82-00aa004ba90b}"
#define PROT_FILTER_TEST1_CLSID_REGKEY  "CLSID\\"PROT_FILTER_TEST1_CLSID
#define PROT_FILTER_TEST1_PROTOCOL      PROT_FILTER_TEST1

const REGENTRY rgClassesMimeHandlerTest1[] =
{
    STD_ENTRY(PROT_FILTER_TEST1_CLSID_REGKEY, PROT_FILTER_TEST1_DESCRIP),
    STD_ENTRY(PROT_FILTER_TEST1_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
        { KEYTYPE_STRING, PROT_FILTER_TEST1_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};

const REGENTRY rgMimeHandlerTest1  [] = 
{ 
    STD_ENTRY(PROT_FILTER_TEST1 , PROT_FILTER_TEST1_DESCRIP  ), 
    { KEYTYPE_STRING, PROT_FILTER_TEST1 , "CLSID", REG_SZ, (BYTE*)PROT_FILTER_TEST1_CLSID   } 
};
#endif // EAPP_TEST

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
    STD_ENTRY(PROTOCOL_CDL_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
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
    STD_ENTRY(PROT_FILTER_CLASS_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
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
    STD_ENTRY(PROT_FILTER_ENC_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
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
    STD_ENTRY(STD_ENC_FAC_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
        { KEYTYPE_STRING, STD_ENC_FAC_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};




#ifdef EAPP_TEST
// notificaition sink

#define NOTIFICATION_SINK_TEST1_DESCRIP                      "Notification Sink Test1"
#define NOTIFICATION_SINK_TEST1_CLSID                        "{c733e501-576e-11d0-b28c-00c04fd7cd22}"
#define NOTIFICATION_SINK_TEST1_CLSID_REGKEY                 "CLSID\\"NOTIFICATION_SINK_TEST1_CLSID
#define NOTIFICATION_SINK_TEST1_CLSID_REGKEY                 "CLSID\\"NOTIFICATION_SINK_TEST1_CLSID

const REGENTRY rgNotSinkTest1[] =
{
    //***** NOTIFICATION_SINK_TEST1 ENTRIES *****
    STD_ENTRY(NOTIFICATION_SINK_TEST1_CLSID_REGKEY, NOTIFICATION_SINK_TEST1_DESCRIP),
    STD_ENTRY(NOTIFICATION_SINK_TEST1_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
        { KEYTYPE_STRING, NOTIFICATION_SINK_TEST1_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};


#define NOTIFICATION_SINK_TEST2_DESCRIP                      "Notification Sink Test2"
#define NOTIFICATION_SINK_TEST2_CLSID                        "{c733e502-576e-11d0-b28c-00c04fd7cd22}"
#define NOTIFICATION_SINK_TEST2_CLSID_REGKEY                 "CLSID\\"NOTIFICATION_SINK_TEST2_CLSID
#define NOTIFICATION_SINK_TEST2_CLSID_REGKEY                 "CLSID\\"NOTIFICATION_SINK_TEST2_CLSID

const REGENTRY rgNotSinkTest2[] =
{
    //***** NOTIFICATION_SINK_TEST2 ENTRIES *****
    STD_ENTRY(NOTIFICATION_SINK_TEST2_CLSID_REGKEY, NOTIFICATION_SINK_TEST2_DESCRIP),
    STD_ENTRY(NOTIFICATION_SINK_TEST2_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
        { KEYTYPE_STRING, NOTIFICATION_SINK_TEST2_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};


#define NOTIFICATION_SINK_TEST3_DESCRIP                      "Notification Sink Test3"
#define NOTIFICATION_SINK_TEST3_CLSID                        "{c733e503-576e-11d0-b28c-00c04fd7cd22}"
#define NOTIFICATION_SINK_TEST3_CLSID_REGKEY                 "CLSID\\"NOTIFICATION_SINK_TEST3_CLSID
#define NOTIFICATION_SINK_TEST3_CLSID_REGKEY                 "CLSID\\"NOTIFICATION_SINK_TEST3_CLSID

const REGENTRY rgNotSinkTest3[] =
{
    //***** NOTIFICATION_SINK_TEST3 ENTRIES *****
    STD_ENTRY(NOTIFICATION_SINK_TEST3_CLSID_REGKEY, NOTIFICATION_SINK_TEST3_DESCRIP),
    STD_ENTRY(NOTIFICATION_SINK_TEST3_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
        { KEYTYPE_STRING, NOTIFICATION_SINK_TEST3_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};


#define NOTIFICATION_SINK_TEST4_DESCRIP                      "Notification Sink Test4"
#define NOTIFICATION_SINK_TEST4_CLSID                        "{c733e504-576e-11d0-b28c-00c04fd7cd22}"
#define NOTIFICATION_SINK_TEST4_CLSID_REGKEY                 "CLSID\\"NOTIFICATION_SINK_TEST4_CLSID
#define NOTIFICATION_SINK_TEST4_CLSID_REGKEY                 "CLSID\\"NOTIFICATION_SINK_TEST4_CLSID

const REGENTRY rgNotSinkTest4[] =
{
    //***** NOTIFICATION_SINK_TEST4 ENTRIES *****
    STD_ENTRY(NOTIFICATION_SINK_TEST4_CLSID_REGKEY, NOTIFICATION_SINK_TEST4_DESCRIP),
    STD_ENTRY(NOTIFICATION_SINK_TEST4_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
        { KEYTYPE_STRING, NOTIFICATION_SINK_TEST4_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};
#endif // EAPP_TEST



const REGENTRYGROUP rgRegEntryGroups[] = {

#ifdef EAPP_TEST
    { HKEY_CLASSES_ROOT, rgClassesRes,      ARRAYSIZE(rgClassesRes) },
    { HKEY_CLASSES_ROOT, rgHandlerRes  ,    ARRAYSIZE(rgHandlerRes) },

    { HKEY_CLASSES_ROOT, rgClassesOhserv,   ARRAYSIZE(rgClassesOhserv) },
    { HKEY_CLASSES_ROOT, rgNameSpaceOhserv, ARRAYSIZE(rgNameSpaceOhserv) },

    { HKEY_CLASSES_ROOT, rgMimeHandlerTest1,        ARRAYSIZE(rgMimeHandlerTest1) },
    { HKEY_CLASSES_ROOT, rgClassesMimeHandlerTest1, ARRAYSIZE(rgClassesMimeHandlerTest1) },

    { HKEY_CLASSES_ROOT, rgNotSinkTest1  ,  ARRAYSIZE(rgNotSinkTest1) },
    { HKEY_CLASSES_ROOT, rgNotSinkTest2  ,  ARRAYSIZE(rgNotSinkTest2) },
    { HKEY_CLASSES_ROOT, rgNotSinkTest3  ,  ARRAYSIZE(rgNotSinkTest3) },
    { HKEY_CLASSES_ROOT, rgNotSinkTest4  ,  ARRAYSIZE(rgNotSinkTest4) },
#endif //EAPP_TEST

    { HKEY_CLASSES_ROOT, rgMimeHandlerEnc,        ARRAYSIZE(rgMimeHandlerEnc) },
    { HKEY_CLASSES_ROOT, rgClassesMimeHandlerEnc, ARRAYSIZE(rgClassesMimeHandlerEnc) },

    { HKEY_CLASSES_ROOT, rgDeflateEnc,        ARRAYSIZE(rgDeflateEnc) },
    { HKEY_CLASSES_ROOT, rgGZIPEnc,  ARRAYSIZE(rgGZIPEnc) },
    { HKEY_CLASSES_ROOT, rgClassesStdEncFac,  ARRAYSIZE(rgClassesStdEncFac) },
    
    { HKEY_CLASSES_ROOT, rgClassesMimeInstallHandler,      ARRAYSIZE(rgClassesMimeInstallHandler) },
    { HKEY_CLASSES_ROOT, rgMimeInstallHandler,             ARRAYSIZE(rgMimeInstallHandler) },

    { HKEY_CLASSES_ROOT, rgClassesCdl,      ARRAYSIZE(rgClassesCdl) },
    { HKEY_CLASSES_ROOT, rgHandlerCdl  ,    ARRAYSIZE(rgHandlerCdl) },

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
//  History:    11-07-96   JohannP (Johann Posch)   Created
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
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void DllRelease(void)
{
    EProtAssert((g_cRef > 0));
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
//  History:    11-07-96   JohannP (Johann Posch)   Created
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
    if (lpv == NULL)
    {
        return;
    }

    CoTaskMemFree(lpv);
}

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
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//--------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv)
{
    EProtDebugOut((DEB_PLUGPROT, "API _IN DllGetClassObject\n"));

    HRESULT hr = E_FAIL;

#ifdef EAPP_TEST
    if (   (clsid == CLSID_ResProtocol)
        || (clsid == CLSID_OhServNameSp)
        || (clsid == CLSID_NotificaitonTest1)
        || (clsid == CLSID_NotificaitonTest2)
        || (clsid == CLSID_NotificaitonTest3)
        || (clsid == CLSID_NotificaitonTest4)
        || (clsid == CLSID_CdlProtocol)
        || (clsid == CLSID_DeCompMimeFilter)
        || (clsid == CLSID_MimeHandlerTest1)
        || (clsid == CLSID_StdEncodingFilterFac)
        || (clsid == CLSID_ClassInstallFilter) )
#else
    if(    (clsid == CLSID_DeCompMimeFilter)
        || (clsid == CLSID_StdEncodingFilterFac)
        || (clsid == CLSID_CdlProtocol)
        || (clsid == CLSID_ClassInstallFilter) )
#endif // EAPP_TEST
    {
        CUrlClsFact *pCF = NULL;
        hr = CUrlClsFact::Create(clsid, &pCF);
        if (hr == NOERROR)
        {
            EProtAssert((pCF != NULL));
            hr = pCF->QueryInterface(iid, ppv);
            pCF->Release();
        }
    }

    EProtDebugOut((DEB_PLUGPROT, "API OUT DllGetClassObject (hr:%lx, ppv:%p)\n",hr,*ppv));
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
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//--------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID lpvReserved)
{
    BOOL fResult = TRUE;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
#if DBG==1
        {
            EProtInfoLevel = (DWORD) GetProfileIntA("UrlMon","EProt", (DEB_ERROR | DEB_WARN));
        }
#endif //DBG==1
        g_hInst = hInstance;
        //tsaMain.InitApp(NULL);

        //fResult = TlsDllMain(hInstance, dwReason, lpvReserved);
        break;

    case DLL_PROCESS_DETACH:
        if(g_hInst_LZDHtml) 
        {
            FreeLibrary(g_hInst_LZDHtml);
        }
        if(g_hInst_Deflate)
        {
            FreeLibrary(g_hInst_Deflate);
        }
        if(g_hInst_GZIP)
        {
            FreeLibrary(g_hInst_GZIP);
        }

        // Fall through

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        //fResult = TlsDllMain(hInstance, dwReason, lpvReserved);
        break;

    }
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
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
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
//  History:    10-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllRegisterServer()
{
    EProtDebugOut((DEB_PLUGPROT, "API _IN DllRegisterServer\n"));
    HRESULT hr;


    hr = HrDllRegisterServer(rgRegEntryGroups, g_hInst, NULL /*pfnLoadString*/);


    EProtDebugOut((DEB_PLUGPROT, "API OUT DllRegisterServer (hr:%lx)\n",hr));
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
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllUnregisterServer()
{
    EProtDebugOut((DEB_PLUGPROT, "API _IN DllUnregisterServer\n"));
    HRESULT hr;

    hr = HrDllUnregisterServer(rgRegEntryGroups, g_hInst, NULL /*pfnLoadString*/);

    EProtDebugOut((DEB_PLUGPROT, "API OUT DllUnregisterServer (hr:%lx)\n",hr));
    return hr;
}


#if DBG==1

#include <sem.hxx>
CMutexSem   mxs;

IDebugOut *v_pEProtDbgOut = NULL;

LPCWSTR v_gDbgFacilitieNames[] =
{
     L"*/*"
    ,L"EProt"
    ,NULL
};

void EProtUrlSpy(int iOption, const char *pscFormat, ...)
{
    static char szOutBuffer[2048];
    CLock       lck(mxs);
    DWORD tid = GetCurrentThreadId();
    DWORD cbBufLen;
    sprintf(szOutBuffer,"%08x> ", tid );
    cbBufLen = strlen(szOutBuffer);

    va_list args;
    if (iOption & EProtInfoLevel)
    {
        va_start(args, pscFormat);
        wvsprintf(szOutBuffer + cbBufLen, pscFormat, args);
        va_end(args);
        UrlSpySendEntry(v_pEProtDbgOut, szOutBuffer, tid, iOption, 0);
    }
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

        if (   (!wcsicmp(pwzName, L"*/*"))
            || (!wcsicmp(pwzName, L"EProt")) )
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

    }

    return NOERROR;
}



void PerfDbgLogFn(int tag, void * pvObj, const char * pchFmt, ...)
{
    static char szOutBuffer[2048];
    static DWORD * apiLevel[] = { &EProtInfoLevel };
    static IDebugOut ** apDbgOut[] = { &v_pEProtDbgOut };
    int iIndex = min(tag >> DEB_LEVEL_SHIFT, 0);
    int iLevel = *apiLevel[iIndex];

    if ((tag & iLevel) == 0)
        return;

    CLock lck(mxs);
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

#endif //DBG==1



