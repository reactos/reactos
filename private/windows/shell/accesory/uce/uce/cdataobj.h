//**********************************************************************
// File name: cdataobj.h
//
// Definition of CImpIDataObject
// Implements the IDataObject interface required for Data transfer
//
// Copyright (c) 1997-1999 Microsoft Corporation.
//**********************************************************************

#ifndef CIMPIDATAOBJECT_H
#define CIMPIDATAOBJECT_H

class CImpIDataObject : public IDataObject {
private:
    ULONG   m_cRef;     // Reference counting information
    HWND    hWndDlg;    // Dialog handle cached for dumping Rich text
    TCHAR   m_lpszText[2];   // Pointer to text which is to be dragged

public:
    // Constructor
    CImpIDataObject(HWND hWndDlg);

    // IUnknown interface members
    STDMETHODIMP QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDataObject interface members
    STDMETHODIMP GetData(FORMATETC*, STGMEDIUM*);
    STDMETHODIMP GetDataHere(FORMATETC*, STGMEDIUM*);
    STDMETHODIMP QueryGetData(FORMATETC*);
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC*, FORMATETC*);
    STDMETHODIMP SetData(FORMATETC*, STGMEDIUM*, BOOL);
    STDMETHODIMP EnumFormatEtc(DWORD, IEnumFORMATETC**);
    STDMETHODIMP DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*);
    STDMETHODIMP DUnadvise(DWORD);
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA**);

    // Function which transfer data
    HRESULT RenderRTFText(STGMEDIUM* pMedium);
    HRESULT RenderPlainUnicodeText(STGMEDIUM* pMedium);
    HRESULT RenderPlainAnsiText(STGMEDIUM* pMedium);

    int SetText(LPTSTR);
};

typedef CImpIDataObject *PCImpIDataObject;

#endif // CIMPIDATAOBJECT_H
