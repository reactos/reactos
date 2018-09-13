#error "@@@ This file is nuked"
/*
 *  _UTIL.H
 *
 *  Purpose:
 *      declarations for various useful utility functions
 *
 *  Author:
 *      alexgo (4/25/95)
 */

#ifndef __UTIL_H__
#define __UTIL_H__

HGLOBAL DuplicateHGlobal   (HGLOBAL hglobal);
HGLOBAL TextHGlobalAtoW    (HGLOBAL hglobal);
HGLOBAL TextHGlobalWtoA    (HGLOBAL hglobal);
INT     CountMatchingBits  (const DWORD *a, const DWORD *b, INT total);

//Stuff from OLESTD samples

//Ole clipboard format defines.
#define CF_EMBEDSOURCE      "Embed Source"
#define CF_EMBEDDEDOBJECT   "Embedded Object"
#define CF_LINKSOURCE       "Link Source"
#define CF_OBJECTDESCRIPTOR "Object Descriptor"
#define CF_FILENAME         "FileName"
#define CF_OWNERLINK        "OwnerLink"

    //
    // This builds an HGLOBAL from a unicode html string
    //

HRESULT HtmlStringToSignaturedHGlobal (
    HGLOBAL * phglobal, const TCHAR * pStr, long cch );


#endif // !__UTIL_H__
