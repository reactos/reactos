//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       fmtetc.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCUI_FMTETC_H
#define _INC_CSCUI_FMTETC_H

class CEnumFormatEtc : public IEnumFORMATETC
{
    public:
        CEnumFormatEtc(UINT cFormats, LPFORMATETC prgFormats);
        CEnumFormatEtc(const CEnumFormatEtc& ef);
        ~CEnumFormatEtc(VOID);

        //
        // IUnknown methods.
        //
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvOut);
        STDMETHODIMP_(ULONG) AddRef(VOID);
        STDMETHODIMP_(ULONG) Release(VOID);

        //
        // IEnumFORMATETC methods.
        //
        STDMETHODIMP Next(DWORD, LPFORMATETC, LPDWORD);
        STDMETHODIMP Skip(DWORD);
        STDMETHODIMP Reset(VOID);
        STDMETHODIMP Clone(IEnumFORMATETC **);

        //
        // Called to add formats to the enumerator.  Used by ctors.
        //
        HRESULT AddFormats(UINT cFormats, LPFORMATETC prgFormats);
        //
        // For implementations non-exception throwing clients.
        //
        HRESULT IsValid(void) const
            { return SUCCEEDED(m_hrCtor); }

    private:
        LONG        m_cRef;
        int         m_cFormats;
        int         m_iCurrent;
        LPFORMATETC m_prgFormats;
        HRESULT     m_hrCtor;

        //
        // Prevent assignment.
        //
        void operator = (const CEnumFormatEtc&);
};
        
#endif // _INC_CSCUI_FMTETC_H
