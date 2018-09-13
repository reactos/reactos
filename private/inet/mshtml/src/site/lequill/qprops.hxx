//+----------------------------------------------------------------------------
//
// File:        qprops.HXX
//
// Contents:    dictionary and access class definition for ITextProperties
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _QPROPS_HXX_
#define _QPROPS_HXX_ 1

#include "quillsite.h"


//
// Class for custom property access
//
class CTridentPropertyAccess : public ITextPropertyAccess
{
protected:
    CParentInfo *m_pParentInfo;
	CFancyFormat *m_pFF;
	CCharFormat *m_pCF;

public:
	inline void put_ParentInfo(const CParentInfo *pParentInfo) 
	{
	    m_pParentInfo = (CParentInfo*)pParentInfo;
	} 
	inline CParentInfo *get_ParentInfo() 
	{
	    return (m_pParentInfo);
	} 

	inline void put_CF(const CCharFormat *pCF) 
	{
	    m_pCF = (CCharFormat*)pCF;
	} 
	inline CCharFormat *get_CF() 
	{
	    return (m_pCF);
	} 
	inline void put_FF(const CFancyFormat *pFF) 
	{
	    m_pFF = (CFancyFormat*)pFF;
	} 
	inline CFancyFormat *get_FF() 
	{
	    return (m_pFF);
	} 

	inline LONG XResolvePercentageValue(CUnitValue *pCUnitValue, LONG lFontHeightTwips = 1)
	{
	    if (pCUnitValue->IsNull())
            return 0;

        return (pCUnitValue->XGetPixelValue(get_ParentInfo(), get_ParentInfo()->_sizeParent.cx, lFontHeightTwips));
	} 
};

//
// Class for custom property access
//
class CCharFormatPropertyAccess : public CTridentPropertyAccess
{
protected:
	CCharFormat *m_pThis;

public:
	inline void put_This(const CCharFormat *pThis) 
	{
    	// REVIEW alexmog: can I do same with a constructor?
	    m_pThis = (CCharFormat *) pThis;
	} 

//// ITextPropertyAccess
public:
	STDMETHODIMP_(void *)get_This();

    STDMETHODIMP FetchProperty(int tag, int nUnit, const void *pvSrc, void *pvDst, int cbDst);
	STDMETHODIMP ApplyProperty(int tag, int nUnit, const void *pvSrc, void *pvDst, int cbDst);
	STDMETHODIMP CompareProperty(int tag, int nUnit, const void *pvSrc, const void *pvDst);
};

class CParaFormatPropertyAccess : public CTridentPropertyAccess
{
protected:
	CParaFormat *m_pThis;

public:
	inline void put_This(const CParaFormat *pThis) 
	{
    	// REVIEW alexmog: can I do same with a constructor?
	    m_pThis = (CParaFormat *) pThis;
	} 

//// ITextPropertyAccess
public:
	STDMETHODIMP_(void *)get_This();

    STDMETHODIMP FetchProperty(int tag, int nUnit, const void *pvSrc, void *pvDst, int cbDst);
	STDMETHODIMP ApplyProperty(int tag, int nUnit, const void *pvSrc, void *pvDst, int cbDst);
	STDMETHODIMP CompareProperty(int tag, int nUnit, const void *pvSrc, const void *pvDst);
};

extern TextPropertyDictionary dictCF;
extern TextPropertyDictionary dictPF;

#endif // _QPROPS_HXX_
