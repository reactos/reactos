// atlcreate.cpp : Implementation of DLL Exports.

#include "shellprv.h"

LCID g_lcidLocale = MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT);


#include <ole2.h>
#include <objbase.h>
#include "unicpp\stdafx.h"

#include "fsearch.h"    // FileSearchBand

extern "C"
STDAPI CFileSearchBand_CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void **ppunk)
{
    return CComCreator< CComObject< CFileSearchBand > >::CreateInstance((LPVOID)pUnkOuter, IID_IUnknown, (LPVOID*)ppunk);
}
