//**********************************************************************
// File name: cdropsrc.h
//
// Definition of CDropSource
// Implements the IDropSource interface required for an application to
// act as a Source in a drag and drop operation
//
// Copyright (c) 1997-1999 Microsoft Corporation.
//**********************************************************************

#ifndef DROPSOURCE_H
#define DROPSOURCE_H

class CDropSource : public IDropSource {
private:
    ULONG   m_cRef;     // Reference counting information

public:
    // Constructor
    CDropSource();

    // IUnknown interface members
    STDMETHODIMP QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDropSource specific members
    STDMETHODIMP QueryContinueDrag(BOOL, DWORD);
    STDMETHODIMP GiveFeedback(DWORD);
};

typedef CDropSource *PCDropSource;

#endif // DROPSOURCE_H
