/*
 * @(#)runtime.cxx 1.0 07/09/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * XTL runtime object
 * 
 */

#ifdef UNIX
#include <alloca.h>
#endif // UNIX

#include "core.hxx"
#pragma hdrstop

#include <xmldomdid.h>

#include "oawrap.hxx"

#include "runtime.hxx"

DISPATCHINFO _dispatch<IXTLRuntime, &LIBID_MSXML, &IID_IXTLRuntime>::s_dispatchinfo =
{
    NULL, &IID_IXTLRuntime, &LIBID_MSXML, ORD_MSXML
};

CXTLRuntimeObject::CXTLRuntimeObject()
{
}

CXTLRuntimeObject::~CXTLRuntimeObject()
{
}

HRESULT STDMETHODCALLTYPE 
CXTLRuntimeObject::QueryInterface(REFIID iid, void ** ppv)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr = super::QueryInterface(iid, ppv);

    if (!SUCCEEDED(hr))
    {
        if (iid == IID_IXMLDOMNode)
        {
            *ppv = this;
            AddRef();
            hr = S_OK;
        }
        else if (iid == Node::s_IID)
        {
            hr =  getDOMNode()->QueryInterface(iid, ppv);
        }
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE 
CXTLRuntimeObject::GetIDsOfNames( 
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{
    STACK_ENTRY_IUNKNOWN(this);

    HRESULT hr = super::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
    if (S_OK != hr)
    {
        hr = getDOMNode()->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
    } 

    return hr;
}

HRESULT STDMETHODCALLTYPE 
CXTLRuntimeObject::Invoke( 
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{
    STACK_ENTRY_IUNKNOWN(this);

    HRESULT hr;
    
    if (dispIdMember >= DISPID_XTLRUNTIME && dispIdMember <= DISPID_XTLRUNTIME_FORMATTIME)
    {
        hr = super::Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
    else
    {   
        hr = getDOMNode()->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
    return hr;
}


// Private helpers

Element *
CXTLRuntimeObject::ancestor(String * s, Element * node)
{
    if (node == null)
        return null;
    return ancestor(Name::create(s), node);
}

Element *
CXTLRuntimeObject::ancestor(Name * tag, Element * node)
{
    if (node == null)
        return null;
    for (Element * e = node->getParent(); e != null; e = e->getParent())
    {
        if (e->getTagName() == tag)
        {
            return e;
        }
    }
    return null;
}


// IXTLRuntime implementation
HRESULT 
STDMETHODCALLTYPE 
CXTLRuntimeObject::uniqueID( 
    IXMLDOMNode *pNode,
    long *pID) 
{
    STACK_ENTRY_IUNKNOWN(this);

    HRESULT hr;

    if (NULL == pID)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // return a pointer to the element
    PVOID pTemp;

    if (SUCCEEDED(hr = pNode->QueryInterface(Node::s_IID, &pTemp)))
    {
		// WIN64 Code Review: This is being used as a unique cookie and is not being used as a pointer. 
		// So this should be OK
        *pID = PtrToLong(pTemp);
    }

Cleanup:

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CXTLRuntimeObject::depth( 
    IXMLDOMNode *pNode,
    long *pDepth) 
{
    Element *pElem;
    HRESULT hr;
    long iDepth;

    if (NULL == pNode)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (NULL == pDepth)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (SUCCEEDED(hr = pNode->QueryInterface(Node::s_IID, (LPVOID *)&pElem)))
    {
        // The document counts as a node, so we need to subtract 
        // one to take it into account

        iDepth = -1;
        while (pElem)
        {
            iDepth++;
            pElem = pElem->getParent();
        }

        *pDepth = iDepth;
    }

Cleanup:
    return hr;
}


HRESULT 
STDMETHODCALLTYPE 
CXTLRuntimeObject::childNumber( 
    IXMLDOMNode *pNode,
    long *pNumber)
{
    STACK_ENTRY_IUNKNOWN(this);

    HRESULT hr;
    Element *pElem;
    
    if (NULL == pNode)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (NULL == pNumber)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (SUCCEEDED(hr = pNode->QueryInterface(Node::s_IID, (LPVOID *)&pElem)))
    {
        *pNumber = pElem->getIndex(true) + 1;
    }

    Assert(SUCCEEDED(hr) && "Couldn't get Element from DOMNode");

Cleanup:

    return hr;
}


HRESULT 
STDMETHODCALLTYPE 
CXTLRuntimeObject::ancestorChildNumber( 
    BSTR bstrNodeName,
    IXMLDOMNode *pNode,
    long *pNumber)
{
    HRESULT hr;

    if (NULL == pNode)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
        
    if (NULL == pNumber)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Node *pXMLNode;

    if (SUCCEEDED(hr = pNode->QueryInterface(Node::s_IID, (LPVOID *)&pXMLNode)))
    {
        String *s = String::newString(bstrNodeName);
        Element * e = ancestor(s, pXMLNode);
        if (e != null)
        {
            *pNumber = e->getIndex(true) + 1;
        }
        else
        {
            *pNumber = 0;
        }
    }

Cleanup:
    return hr;
}


HRESULT 
STDMETHODCALLTYPE 
CXTLRuntimeObject::absoluteChildNumber( 
    IXMLDOMNode *pNode,
    long *pNumber)
{
    HRESULT hr;

    // Validate arguments
    if (NULL == pNode)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (NULL == pNumber)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Node *pXMLNode;

    if (SUCCEEDED(hr = pNode->QueryInterface(Node::s_IID, (LPVOID *)&pXMLNode)))
    {
        *pNumber = 1 + pXMLNode->getIndex(false);
    }

Cleanup:
    return hr;
}



TCHAR CXTLRuntimeObject::s_achRomanUpper[] = {TEXT('M'), TEXT('D'), TEXT('C'), TEXT('L'), TEXT('X'), TEXT('V'), TEXT('I')};
TCHAR CXTLRuntimeObject::s_achRomanLower[] = {TEXT('m'), TEXT('d'), TEXT('c'), TEXT('l'), TEXT('x'), TEXT('v'), TEXT('i')};

BOOL 
CXTLRuntimeObject::IntToRoman(
    UINT iNumber, 
    LPTSTR szRoman, 
    UINT cchRoman, 
    BOOL fUpperCase)
{
    if (0 == iNumber)
        return FALSE;

    UINT i = 0;
    LPCTSTR szRomanChars = fUpperCase ? s_achRomanUpper : s_achRomanLower;
    
    UINT iDivisor     = 1000;
    UINT iDivisorChar = ROMAN_1000;
    UINT iSubtractor     = 100;
    UINT iSubtractorChar = ROMAN_100;

    BOOL fRet;

    // If we get a number bigger than 10000, chop it down
    iNumber %= 10000;

    while (iNumber > 0 && i < cchRoman)
    {
        int iDigit = iNumber / iDivisor;

        if(iDigit + i > cchRoman)
        {
            fRet = FALSE;
            goto LRet;      // Buffer is not enough!!!
        }

        // Get the 1000s, 500s or whatever in place
        if (iDigit < 4 || iDivisor == 1000)
        {
            while (iDigit > 0 && i < cchRoman)
            {
                Assert(iDivisorChar < (sizeof(s_achRomanUpper)/sizeof(TCHAR)));
                szRoman[i++] = szRomanChars[iDivisorChar];
                iDigit--;
                iNumber -= iDivisor;
            }
        }

        Assert(iDigit <= 4);
        if(i+2 >= cchRoman)
        {
            fRet = FALSE;
            goto LRet;
        }
        // Do the weird predecessor stuff            
        if (iNumber >= (iDivisor - iSubtractor))
        {
            Assert(iSubtractor > 0);
            szRoman[i++] = szRomanChars[iSubtractorChar];
            szRoman[i++] = szRomanChars[iDivisorChar];
            iNumber -= (iDivisor - iSubtractor);
        }
        
        if (iNumber > 0)
        {
            iDivisorChar++;
    
            if (iDivisorChar & 1 == 1) // Is it even ?
            {
                iDivisor /= 2;
            }
            else
            {
                iDivisor /= 5;

                iSubtractor /= 10;
                iSubtractorChar += 2;
            }
        }

    }

    if (i <= cchRoman)
    {
        // Terminate the string
        szRoman[i] = 0;
        fRet = TRUE;
    }
LRet:
    return fRet;
}

LCID 
CXTLRuntimeObject::LcidFromVariant(
    VARIANT varLocale, 
    BOOL *pfSuccess)
{
    if (ISEMPTYARG(varLocale))
    {
        return GetThreadLocale();
    }
    else
    {
        // BUGBUG: Add code to translate the Rfc1766 locale code to LCID if necessary
        Assert (FALSE && "Need to implement RFC1766 to LCID");
        return GetThreadLocale();
    }
}


UINT 
CXTLRuntimeObject::GetNumDigits(long i)
{
    // Zero is a digit - this makes sure it gets counted as one
    int iDigits = (i > 0) ? 0 : 1;

    // Count the digits.  I could use log10(i), but that would bring in the 
    // ln function.  This is just about as efficient as the most efficient log 
    // approximation algorithm I could find.  (simonb)
    while (i != 0)
    {
        i /= 10;
        iDigits++;

    }

    return iDigits;
}


HRESULT 
STDMETHODCALLTYPE 
CXTLRuntimeObject::formatIndex( 
    long lIndex,
    BSTR bstrFormat,
    BSTR *pbstrFormattedString)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;

    TRY
    {
        if (lstrlen(bstrFormat) > 1 && bstrFormat[0] != TEXT('0'))
        {
            _dispatchImpl::setErrorInfo(MSG_E_FORMATINDEX_BADFORMAT);
            goto ErrorException;
        }

        BOOL fUpperCase = TRUE;

        switch (bstrFormat[0])
        {
        // Regular numbers
        case L'0':
        case L'1':
        {
            int iLeadingZeros = 0; 

            while (bstrFormat[iLeadingZeros] == L'0' && bstrFormat[iLeadingZeros])
                iLeadingZeros++;

            // Were we given a valid formatting string ?  The current char must be one, and the
            // next char must be \0
            if (bstrFormat[iLeadingZeros] != L'1' || bstrFormat[iLeadingZeros + 1] != 0)
            {
                _dispatchImpl::setErrorInfo(MSG_E_FORMATINDEX_BADFORMAT);
                goto ErrorException;
            }

            if (iLeadingZeros > 0)
                iLeadingZeros++; // Add an extra "0" to account for the "1" in the formatting string

            int iNumDigits = GetNumDigits(lIndex);

            if (lIndex < 0)
                iNumDigits--;

            int i;

            // max(Leading zeros, number of digits + 1 for the "1") + possible negative sign + terminating zero
            LPWSTR pchBuffer = new_ne WCHAR[max(iNumDigits, iLeadingZeros) + 2];

            if (NULL != pchBuffer)
            {
                i = 0;

                // Insert the "-" sign
                if (lIndex < 0)
                {
                    pchBuffer[i++] = L'-';
                    lIndex = -1 * lIndex; // Get rid of the sign
                    
                    // If we have leading zeros, we need to offset the start of the actual number by
                    // one, to allow for the "-" sign.  We could use another variable - this saves 
                    // the extra DWORD on the stack
                    if (iLeadingZeros > 0)
                        iLeadingZeros++; 
                }

                // Put the leading zeros into the string
                if (iLeadingZeros > 0)
                {
                    iLeadingZeros -= iNumDigits; 
                
                    for (; i < iLeadingZeros; i++)
                        pchBuffer[i] = L'0';
                }

                // Build up the string manually, in Unicode.  This eliminates the need 
                // for functions like wsprintf.  We do this from right to left

                // Insert the terminating NULL
                i += iNumDigits - 1;
                pchBuffer[i + 1] = 0;

                do 
                {
                    // Compute the digit
                    UINT iDigit = lIndex % 10;
                    lIndex = lIndex / 10;

                    // Put it in the buffer
                    pchBuffer[i--] = L'0' + iDigit;
                } while (lIndex > 0);

                *pbstrFormattedString = SysAllocString(pchBuffer);

                delete pchBuffer;

                if (NULL != *pbstrFormattedString)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            break;
        }

        // Letters
        case L'a':
            fUpperCase = FALSE;
        case L'A':
        {
            if (lIndex <= 0)
            {
                _dispatchImpl::setErrorInfo(MSG_E_FORMATINDEX_BADINDEX);
                goto ErrorException;
            }

            if (bstrFormat[1] != 0)
            {
                _dispatchImpl::setErrorInfo(MSG_E_FORMATINDEX_BADFORMAT);
                goto ErrorException;
            }

            WCHAR achBuffer[32]; 

            int iNumDigits = (lIndex / 26) + ((lIndex % 26) > 0 ? 1 : 0);
            iNumDigits = iNumDigits % 30;
            
            if (0 == iNumDigits)
                iNumDigits += 30;

            WCHAR chBase = (lIndex - 1) % 26 + (fUpperCase ? TEXT('A') : TEXT('a'));

            for (int i=0; i < iNumDigits; i++)
                achBuffer[i] = chBase;

            achBuffer[i] = 0;

            if (NULL == (*pbstrFormattedString = SysAllocString(achBuffer)))
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            hr = S_OK;
                
            break;
        }

        // Roman numerals
        case L'i':
            fUpperCase = FALSE;
        case L'I':
        {
            TCHAR achBuffer[30];

            if (lIndex <= 0)
            {
                _dispatchImpl::setErrorInfo(MSG_E_FORMATINDEX_BADINDEX);
                goto ErrorException;
            }

            if (bstrFormat[1] != 0)
            {
                _dispatchImpl::setErrorInfo(MSG_E_FORMATINDEX_BADFORMAT);
                goto ErrorException;
            }

            if (IntToRoman(lIndex, achBuffer, sizeof(achBuffer), fUpperCase))
            {
                if (NULL == (*pbstrFormattedString = SysAllocString(achBuffer)))
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                hr = S_OK;  
            }
            else
            {
                hr = E_FAIL;
            }

            break;
        }
        

        default:
            hr = E_INVALIDARG;
            break;
        }

    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    goto Cleanup;

ErrorException:

    hr = DISP_E_EXCEPTION;

Cleanup:
    return hr;
}


HRESULT 
STDMETHODCALLTYPE 
CXTLRuntimeObject::formatNumber( 
    double dblNumber,
    BSTR bstrFormat,
    BSTR *pbstrFormattedString)
{
    VARIANT varNumber;
    VariantInit(&varNumber);
    V_VT(&varNumber) = VT_R8;
    V_R8(&varNumber) = dblNumber;

    HRESULT hr = WrapVarFormat(&varNumber, bstrFormat, 0, 0, 0, pbstrFormattedString);

    if (FAILED(hr))
    {
        // We should just put the number in the string - nice and simple
        hr = VariantChangeTypeEx(&varNumber, &varNumber, GetThreadLocale(), 0, VT_BSTR);
        if (SUCCEEDED(hr))
        {
            if (NULL == (*pbstrFormattedString = SysAllocString(varNumber.bstrVal)))
                hr = E_OUTOFMEMORY;
        }
        
        VariantClear(&varNumber);
    }

    return hr;
}


HRESULT 
CXTLRuntimeObject::VariantDateToDateTime(
    VARIANT varDate, 
    LPWSTR szFormat, 
    LCID lcid,
    BSTR *pbstrFormattedString, 
    BOOL fDate)
{
    if (NULL == pbstrFormattedString)
        return E_POINTER;

    HRESULT hr;
    UDATE date;
    char * pchFormat;
    ULONG ulTCHAR = _tcslen(szFormat);
    ULONG ulchar = ulTCHAR * sizeof(WCHAR) + 1;
    pchFormat = (char *)alloca(ulchar);
    int iFormat = WideCharToMultiByte(CP_ACP, 0,
                                szFormat, ulTCHAR,
                                pchFormat, ulchar, NULL, NULL);
    Assert((ULONG)iFormat < ulchar);
    pchFormat[iFormat] = 0;

    *pbstrFormattedString = NULL;

    if (V_VT(&varDate) != VT_DATE)
    {
        // If it's not a VT_BSTR of any sort (ie. no locales to worry about), 
        // sure, let's change it
        if ( (V_VT(&varDate) != VT_BSTR) && 
             (V_VT(&varDate) != (VT_BYREF | VT_BSTR)) && 
             (V_VT(&varDate) != VT_EMPTY) )

        {
            hr = VariantChangeType(&varDate, &varDate, 0, VT_DATE);
        }

        // Doesn't matter that hr might not be initialised for obvious reasons
        if (V_VT(&varDate) != VT_DATE || FAILED(hr))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    if (SUCCEEDED(hr = VarUdateFromDate(varDate.date, 0, &date)))
    {
        DWORD dwResult;
        char * pchFormatted;
        DWORD dwBufferSize;
        
        if (fDate)
        {
            // Get the buffer size
            dwBufferSize = GetDateFormatA(
                lcid,
                0,
                &date.st,
                pchFormat, 
                NULL,
                0);
            
            pchFormatted = (char *)alloca(dwBufferSize + 1);
            
            dwResult = GetDateFormatA(
                lcid,
                0,
                &date.st,
                pchFormat, 
                pchFormatted,
                dwBufferSize);
        }
        else
        {
            // Get the buffer size
            dwBufferSize = GetTimeFormatA(
                lcid,
                0,
                &date.st,
                pchFormat,
                NULL,
                0);
            
            pchFormatted = (char *)alloca(dwBufferSize + 1);
            
            dwResult = GetTimeFormatA(
                lcid,
                0,
                &date.st,
                pchFormat,
                pchFormatted,
                dwBufferSize);
        }
        
        if (0 != dwResult)
        {
            TCHAR * szFormatted = (TCHAR *)alloca(dwBufferSize * sizeof(WCHAR));
            int iFormatted = MultiByteToWideChar(CP_ACP, 0, pchFormatted, dwBufferSize, szFormatted, dwBufferSize);
            Assert((ULONG)iFormatted <= dwBufferSize);
            *pbstrFormattedString = SysAllocStringLen(szFormatted, iFormatted);
            hr = S_OK;
        }
        else
        {
            // Of the 3 results which could come back from GetLastError(), 
            // one is not possible and the other two both imply some problem 
            // with the parameters passed in
            hr = E_INVALIDARG;
        }
    }

Cleanup:

    return hr;
}



HRESULT 
STDMETHODCALLTYPE 
CXTLRuntimeObject::formatDate( 
    VARIANT varDate,
    BSTR bstrFormat,
    VARIANT varDestLocale,
    BSTR *pbstrFormattedString)
{
    return VariantDateToDateTime(varDate, bstrFormat, LcidFromVariant(varDestLocale), pbstrFormattedString, TRUE);
}


HRESULT 
STDMETHODCALLTYPE 
CXTLRuntimeObject::formatTime( 
    VARIANT vaTime,
    BSTR bstrFormat,
    VARIANT varDestLocale,
    BSTR *pbstrFormattedString)
{
    return VariantDateToDateTime(vaTime, bstrFormat, LcidFromVariant(varDestLocale), pbstrFormattedString, FALSE);
}
    

///////////////////////////
// IXMLDOMNode implementation 
//////////////////////////

// Dummy implementation is in the header file.  Note: this object is only used from script and the IDispatch::Invoke directs the
// IXMLDOMNode calls directly to the DOMNode.

// end of file: runtime.cxx

