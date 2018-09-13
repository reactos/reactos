/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _UTILS_HXX
#define _UTILS_HXX

#define TRIDENTHACKS 0

#define SafeRelease(p) \
{ \
    if (p) \
        p->Release();\
    p = NULL;\
}


#define SafeFreeString(p) \
{ \
    if (p) \
    ::SysFreeString(p);\
    p = NULL;\
}

#define SafeDelete(p) \
{ \
    if (p) \
        delete p;\
    p = NULL;\
}

#define CHECKHR(hr) \
    if (!SUCCEEDED(hr)) goto CleanUp;

#define CHECKALLOC(hr, p) \
    if (!p) { (hr) = E_OUTOFMEMORY; goto CleanUp; }

#define CHECKHRPTR(hr, p) \
    CHECKHR(hr);     \
    if (!p) { (hr) = E_POINTER; goto CleanUp; }

#endif