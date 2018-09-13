#ifndef __ENUMVAR_H
#define __ENUMVAR_H

//=======================================================================
//		File:	ENUMVAR.H
//		Date:	4-1-97
//		Name:
//
//		Desc:	This header defines the CEnumVariant object, a VARIANT
//				enumerator that uses a SAFEARRAY to store its elements.
//
//		Notes:	This file was copied from an existing Microsoft Source
//				Code OLE Automation Collection sample.  It was originally
//				written by Microsoft Product Support Services, Windows
//				Developer Support, and was modified as needed.
//
//		Copyright (C) 1997 by Microsoft Corporation.  All rights reserved.
//=======================================================================

//=======================================================================
//		Include Files
//=======================================================================

#include <oleauto.h>



//=======================================================================
//		CEnumVariant Class Definition
//=======================================================================

class FAR CEnumVariant : public IEnumVARIANT
{
public:
    //----  IUnknown methods  ----

    STDMETHOD(QueryInterface)( REFIID riid, LPVOID FAR* ppvObj );
    STDMETHOD_(ULONG, AddRef)( void );
    STDMETHOD_(ULONG, Release)( void );


    //----  IEnumVARIANT methods  ----

	STDMETHOD(Next)( ULONG cVars, VARIANT FAR* rgVar, ULONG FAR* pcVarsFetched );
    STDMETHOD(Skip)( ULONG cVarsToSkip );
    STDMETHOD(Reset)( void );
    STDMETHOD(Clone)( IEnumVARIANT FAR* FAR* ppenum );
    

    //----  Class creation methods  ----

    static HRESULT Create( SAFEARRAY FAR*, ULONG, CEnumVariant FAR* FAR* );


    //----  Constructors & Destructors  ----

    CEnumVariant();
    ~CEnumVariant();
    

private:
    //----  Private data  ----

    ULONG			m_cRef;			// Reference count                                 
    ULONG			m_cElements;	// Number of elements in enumerator. 
    long			m_lLBound;		// Lower bound of index.
    long			m_lCurrent;		// Current index.
    SAFEARRAY FAR*	m_psa;			// Safe array that holds elements.
}; 
 


#endif  /* __ENUMVAR_H */


//----  End of ENUMVAR.H  ----