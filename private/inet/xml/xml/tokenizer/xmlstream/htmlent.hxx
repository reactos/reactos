/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _HTMLENT_HXX
#define _HTMLENT_HXX


// Lookup the named entity in the HTML entity table and return the 
// corresponding unicode character, or -1 if not found (Oxffff).
WCHAR LookupBuiltinEntity(const WCHAR* name, ULONG len);


#endif