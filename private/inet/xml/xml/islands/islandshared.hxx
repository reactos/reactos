/*
 * @(#)islandshared.hxx 1.0 3/24/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * Common code for MSXML Data Islands
 * 
 */

#ifndef __ISLANDSHARED_HXX__
#define __ISLANDSHARED_HXX__

///////////////////////////////////
// Constants

// Define the name of the script language etc
#define SCRIPTLANGUAGE       L"XML"
#define TYPESCRIPTLANGUAGE   L"text/XML"
#define ATTRIBUTENAME        L"XMLDocument"

///////////////////////////////////
// Function prototypes

HRESULT 
SetExpandoProperty(
    IHTMLElement *pElement,
    String *pstrName,
    VARIANT *pvarValue
    );

HRESULT
GetExpandoProperty(
    IHTMLElement *pElement,
    String *pstrName,
    VARIANT *pvarValue
    );
#endif // __ISLANDSHARED_HXX__