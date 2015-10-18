/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/xmldomparser.cpp
 * PURPOSE:     XML DOM Parser
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

/*
 *
 * MSXML Version    Header File Name    Library File Name    DLL File Name
 * =======================================================================
 *     2.x              msxml.h              msxml.lib         msxml2.dll
 *     3.0              msxml2.h             msxml2.lib        msxml3.dll
 *     4.0              msxml2.h             msxml2.lib        msxml4.dll
 *     6.0              msxml6.h             msxml6.lib        msxml6.dll
 */
// #pragma comment(lib, "msxml2.lib")
#include <initguid.h>
#define _COM_NO_STANDARD_GUIDS_
#include <comdef.h>
// #include <comutil.h> // comdef.h includes comutil.h
#include <ole2.h>
#include <msxml2.h>

//
// About macro while(0) trick : see http://cnicholson.net/2009/03/stupid-c-tricks-dowhile0-and-c4127/
//
// Macro that releases a COM object if not NULL.
#define SAFE_RELEASE(p) \
    do { if ((p)) { (p)->Release(); (p) = NULL; } } while (0)

#if 0
//  IID_PPV_ARGS(ppType)
//      ppType is the variable of type IType that will be filled
//
//      RESULTS in:  IID_IType, ppvType
//      will create a compiler error if wrong level of indirection is used.
//
// See ObjBase.h - Only in Windows 7 SDK.
#ifndef IID_PPV_ARGS
extern "C++"
{
    template<typename T> void** IID_PPV_ARGS_Helper(T** pp) 
    {
        static_cast<IUnknown*>(*pp);    // make sure everyone derives from IUnknown
        return reinterpret_cast<void**>(pp);
    }
}

#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
#endif
#else
// See include/reactos/shellutils.h
#ifdef __cplusplus
#   define IID_PPV_ARG(Itype, ppType) IID_##Itype, reinterpret_cast<void**>((static_cast<Itype**>(ppType)))
#   define IID_NULL_PPV_ARG(Itype, ppType) IID_##Itype, NULL, reinterpret_cast<void**>((static_cast<Itype**>(ppType)))
#else
#   define IID_PPV_ARG(Itype, ppType) IID_##Itype, (void**)(ppType)
#   define IID_NULL_PPV_ARG(Itype, ppType) IID_##Itype, NULL, (void**)(ppType)
#endif
#endif

HRESULT InitXMLDOMParser(void);
void UninitXMLDOMParser(void);

HRESULT CreateAndInitXMLDOMDocument(IXMLDOMDocument** ppDoc);
BOOL LoadXMLDocumentFromResource(IXMLDOMDocument* pXMLDom, LPCWSTR lpszXMLResName);
BOOL LoadXMLDocumentFromFile(IXMLDOMDocument* pXMLDom, LPCWSTR lpszFilename, BOOL bIgnoreErrorsIfNonExistingFile = FALSE);
