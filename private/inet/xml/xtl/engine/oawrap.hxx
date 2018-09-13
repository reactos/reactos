/*
 * @(#)oawrap.hxx 1.0 07/09/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * OLE Automation wrapper functions
 * 
 */


/*
    INFORMATION
    -----------

    OLE Automation has introduced new string formatting functions.
    However, these aren't available in all version of OLEAUT32,
    notably in the version which shipped with IE 4.blah.  In order 
    not to introduce a dependency on these functions, we load them 
    dynamically so we can handle failure gracefully

*/

#ifndef __OAWRAP_HXX__
#define __OAWRAP_HXX__

typedef HRESULT (__stdcall *PVARFORMAT) (LPVARIANT pvarIn, LPOLESTR pstrFormat, int iFirstDay, int iFirstWeek, ULONG dwFlags, BSTR *pbstrOut);

HRESULT
WrapVarFormat(
    LPVARIANT pvarIn,  
    LPOLESTR pstrFormat, 
    int iFirstDay, 
    int iFirstWeek, 
    ULONG dwFlags, 
    BSTR *pbstrOut);


#endif // __OAWRAP_HXX__