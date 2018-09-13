/*

    File: Dispatch.h

    Copyright (c) 1997-1999 Microsoft Corporation.  All Rights Reserved.

    Abstract:
        Dispatch helpers.

*/

//  Defines

#define VTS_I2              "\x02"
#define VTS_I4              "\x03"
#define VTS_BSTR            "\x08"
#define VTS_DISPATCH        "\x09"
#define VTS_BOOL            "\x0b"
#define VTS_VARIANT         "\x0c"
#define VTS_UNKNOWN         "\x0d"

#define VTS_I2_BYREF        "\x42"
#define VTS_I4_BYREF        "\x43"
#define VTS_BSTR_BYREF      "\x48"
#define VTS_DISPATCH_BYREF  "\x49"
#define VTS_BOOL_BYREF      "\x4b"
#define VTS_VARIANT_BYREF   "\x4c"
#define VTS_UNKNOWN_BYREF   "\x4d"

#define VTS_I2_RETURN       "\x82"
#define VTS_I4_RETURN       "\x83"
#define VTS_BSTR_RETURN     "\x88"
#define VTS_DISPATCH_RETURN "\x89"
#define VTS_BOOL_RETURN     "\x8b"
#define VTS_UNKNOWN_RETURN  "\x8d"

#define VTS_BYREF_FLAG      0x40
#define VTS_RETURN_FLAG     0x80


// Functions

HRESULT         CallDispatchMethod(
                        IDispatch * pDisp,
                        DISPID dispid,
                        VARIANT * pvarFirst,
                        char * pstrSig,
                        va_list val);

HRESULT __cdecl CallDispatchMethod(
                        IDispatch * pDisp,
                        DISPID dispid,
                        char * pstrSig,
                        ...);

HRESULT         CallDispatchMethod(
                        IDispatch * pDisp,
                        WCHAR * pstrMethod,
                        VARIANT * pvarFirst, 
                        char * pstrSig,
                        va_list val);

HRESULT __cdecl CallDispatchMethod(
                        IDispatch * pDisp,
                        WCHAR * pstrMethod,
                        char * pstrSig,
                        ...);

HRESULT __cdecl CallDispatchMethod(
                        IDispatch * pDisp,
                        WCHAR * pstrMethod,
                        VARIANT * pvarFirst,
                        char * pstrSig,
                        ...);

HRESULT         GetDispatchProperty(
                        IDispatch * pDisp,
                        WCHAR * pstrProperty,
                        VARENUM vt,
                        void * pv);

HRESULT         GetDispatchProperty(
                        IDispatch * pDisp,
                        DISPID dispidProperty,
                        VARENUM vt,
                        void * pv);

HRESULT         PutDispatchProperty(
                        IDispatch * pDisp,
                        WCHAR * pstrProperty,
                        VARENUM vt,
                        ...);

HRESULT         PutDispatchProperty(
                        IDispatch * pDisp,
                        DISPID dispidProperty,
                        VARENUM vt,
                        ...);
                        