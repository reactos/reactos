#include "priv.h"
#include "linkload.h"
#pragma hdrstop

//
// This table is used by linkload.c to list all the modules we are supporting in the
// delay load.  The g_aLinkLoadModules is a sorted list of DLL names and correspending
// export/ordinal tables. 
//
// To simplify our lives we have some macros MODUULETABLEDECL and MODULETABLEREF which
// declare and fill in all the exports we need.
//
// Rules of the house (these are important):
//
// * All DLL names *must* be lowercase, without this we will fail to find the DLL
//   when searching.  For perf reasons we convert the name we get from the 
//   linker to lowercase, and then use StrCmp to compare, therefore avoiding
//   lowercasing on the fly.
//
// * This table must be sorted alphabetically, if not then the search will also
//   fail.  To find a DLL we search the table using a binary search, therefore
//   reducing the number of string compares etc and allowing us to scale nicely.
//

MODULETABLEDECL(activeds);                          // activeds.dll

const MODULETABLE g_aLinkLoadModules[] =    
{
    MODULETABLEREF("activeds.dll", activeds),
};

const INT g_cLinkLoadModules = ARRAYSIZE(g_aLinkLoadModules);
