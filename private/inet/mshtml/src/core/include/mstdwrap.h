//-----------------------------------------------------------------------------
//
// Microsoft Forms
// Copyright: (c) 1994-1995, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// File         MStdWrap.h
//
// Contents     Class definition for Mac Unicode-friendly Standard Wrapper 
//              Interfaces
//
//              
//
// Note:        These subclass definitions are required to convert internal
//              UNICODE strings to ANSI strings before passing them on to
//              the appropriate Mac Forms superclass method. By defining
//              the interface name as our subclass wrapper, the main body of
//              code does not need to concern itself with UNICODE vs ANSI -
//              the code will call the correct method.
//
//	History:	02/07/96    Created by kfl / black diamond.
//
//-----------------------------------------------------------------------------

#ifndef I_MSTDWRAP_HXX_
#define I_MSTDWRAP_HXX_
#pragma INCMSG("--- Beg 'mstdwrap.h'")

#if defined(_MACUNICODE) && !defined(_MAC)

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
interface IForms96BinderDispenserMac : public IForms96BinderDispenser
{
public:
    operator IForms96BinderDispenser* () { return this; }


        virtual HRESULT __stdcall ParseName( 
            /* [in] */ OLECHAR *pszName,
            /* [out] */ IForms96Binder **ppBinder);

        virtual HRESULT __stdcall ParseName( 
            /* [in] */ WCHAR *pszName,
            /* [out] */ IForms96Binder **ppBinder) = 0;


};
#define IForms96BinderDispenser                    IForms96BinderDispenserMac

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
interface ISimpleTabularDataMac : public ISimpleTabularData
{
public:
    operator ISimpleTabularData* () { return this; }

    virtual HRESULT __stdcall GetString( 
            /* [in] */ ULONG iRow,
            /* [in] */ ULONG iColumn,
            /* [in] */ ULONG cchBuf,
            /* [out] */ OLECHAR *pchBuf,
            /* [out] */ ULONG *pcchActual);
        virtual HRESULT __stdcall GetString( 
            /* [in] */ ULONG iRow,
            /* [in] */ ULONG iColumn,
            /* [in] */ ULONG cchBuf,
            /* [out] */ WCHAR *pchBuf,
            /* [out] */ ULONG *pcchActual) = 0;
        
        virtual HRESULT __stdcall SetString( 
            /* [in] */ ULONG iRow,
            /* [in] */ ULONG iColumn,
            /* [in] */ ULONG cchBuf,
            /* [in] */ OLECHAR *pchBuf);
        virtual HRESULT __stdcall SetString( 
            /* [in] */ ULONG iRow,
            /* [in] */ ULONG iColumn,
            /* [in] */ ULONG cchBuf,
            /* [in] */ WCHAR *pchBuf) = 0;

        virtual HRESULT __stdcall FindPrefixString( 
            /* [in] */ ULONG iRowStart,
            /* [in] */ ULONG iColumn,
            /* [in] */ ULONG cchBuf,
            /* [in] */ OLECHAR *pchBuf,
            /* [in] */ DWORD findFlags,
            /* [out] */ STDFIND *foundFlag,
            /* [out] */ ULONG *piRowFound);
        virtual HRESULT __stdcall FindPrefixString( 
            /* [in] */ ULONG iRowStart,
            /* [in] */ ULONG iColumn,
            /* [in] */ ULONG cchBuf,
            /* [in] */ WCHAR *pchBuf,
            /* [in] */ DWORD findFlags,
            /* [out] */ STDFIND *foundFlag,
            /* [out] */ ULONG *piRowFound) = 0;
};
#define ISimpleTabularData                    ISimpleTabularDataMac



#endif // _MACUNICODE

#pragma INCMSG("--- End 'mstdwrap.h'")
#else
#pragma INCMSG("*** Dup 'mstdwrap.h'")
#endif
