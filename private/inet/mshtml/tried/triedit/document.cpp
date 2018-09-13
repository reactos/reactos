// Document.cpp : Implementation of CTriEditDocument
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#include "stdafx.h"

#include "triedit.h"
#include "Document.h"
#include "util.h"

#ifdef IE5_SPACING
#include "dispatch.h"
#include <mshtmdid.h>
#include <mshtmcid.h>
#endif //IE5_SPACING

/////////////////////////////////////////////////////////////////////////////
// CTriEditDocument

CTriEditDocument::CTriEditDocument()
{
    m_pUnkTrident = NULL;
    m_pOleObjTrident = NULL;
    m_pCmdTgtTrident = NULL;
    m_pDropTgtTrident = NULL;
#ifdef IE5_SPACING
    m_pTridentPersistStreamInit = NULL;
    m_pMapArray = NULL;
    m_hgMap = NULL;
    m_pspNonDSP = NULL;
    m_hgSpacingNonDSP = NULL;
    m_ichspNonDSPMax = 0;
    m_ichspNonDSP = 0;
#endif //IE5_SPACING

    m_pClientSiteHost = NULL;
    m_pUIHandlerHost = NULL;
    m_pDragDropHandlerHost = NULL;

    m_pUIHandler = NULL;

    m_pTokenizer = NULL;
    m_hwndTrident = NULL;

    m_fUIHandlerSet = FALSE;
    m_fInContextMenu = FALSE;

    m_fDragRectVisible = FALSE;
    m_fConstrain = FALSE;
    m_f2dDropMode = FALSE;
    m_eDirection = CONSTRAIN_NONE;
    m_ptAlign.x = 1;
    m_ptAlign.y = 1;
    m_pihtmlElement = NULL;
    m_pihtmlStyle = NULL;
    m_hbrDragRect = NULL;
    m_fLocked = FALSE;
    m_hgDocRestore = NULL;
}

HRESULT CTriEditDocument::FinalConstruct()
{
    HRESULT hr;
    IUnknown *pUnk = GetControllingUnknown();

    hr = CoCreateInstance(CLSID_HTMLDocument, pUnk, CLSCTX_INPROC_SERVER,
               IID_IUnknown, (void**)&m_pUnkTrident);

    if (SUCCEEDED(hr)) 
    {
        _ASSERTE(NULL != m_pUnkTrident);

                // When we cache Trident pointers, we do a GetControllingUnknown()->Release()
                // since the addref will increment our outer unknown pointer and not Trident
                // We compensate for this by doing a corresponding GetControllingUnknown()->AddRef()
                // in our FinalRelease.  Though these cancel out, it is necessary to do this in order
                // to ensure that our FinalRelease will get called.

        // Cache Trident's IOleObject pointer

        hr = m_pUnkTrident->QueryInterface(IID_IOleObject, (void **)&m_pOleObjTrident);
        _ASSERTE(S_OK == hr && NULL != m_pOleObjTrident);
        pUnk->Release();

        // Cache Trident's IOleCommandTarget pointer

        hr = m_pUnkTrident->QueryInterface(IID_IOleCommandTarget, (void **)&m_pCmdTgtTrident);
        _ASSERTE(S_OK == hr && NULL != m_pCmdTgtTrident);
        pUnk->Release();

        // Allocate UI handler sub-object
        m_pUIHandler = new CTriEditUIHandler(this);
        if (NULL == m_pUIHandler)
            hr = E_OUTOFMEMORY;

#ifdef IE5_SPACING
        // Get IPersistStreamInit
        hr = m_pUnkTrident->QueryInterface(IID_IPersistStreamInit, (void **) &m_pTridentPersistStreamInit);
        _ASSERTE(S_OK == hr && NULL != m_pTridentPersistStreamInit);
        pUnk->Release(); // PuruK - REVIEW Why do we need to do this?
        SetFilterInDone(FALSE);
#endif //IE5_SPACING

        // Allocate buffer for saving document's contents before <BODY> tag
        // Trident replaces all content before <BODY> tag by its own header
        m_hgDocRestore = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, cbHeader);
        if (NULL == m_hgDocRestore)
        {
            delete m_pUIHandler;
            hr = E_OUTOFMEMORY;
        }

    }

    _ASSERTE(SUCCEEDED(hr));

    return hr;
}

void CTriEditDocument::FinalRelease()
{
    IUnknown *pUnk = GetControllingUnknown();

    // Release host interface pointers
    SAFERELEASE(m_pClientSiteHost);
    SAFERELEASE(m_pUIHandlerHost);
    SAFERELEASE(m_pDragDropHandlerHost);

    // Release internal interface pointers
    SAFERELEASE(m_pTokenizer);

    // Release 2d drop related pointers
    ReleaseElement();
    
    // Release Trident interface pointers
    SAFERELEASE(m_pDropTgtTrident);

    pUnk->AddRef();
    SAFERELEASE(m_pOleObjTrident);
    pUnk->AddRef();
    SAFERELEASE(m_pCmdTgtTrident);
#ifdef IE5_SPACING
    pUnk->AddRef(); // REVIEW - PuruK - Why do we need to do this?
    SAFERELEASE(m_pTridentPersistStreamInit);
#endif //IE5_SPACING

    SAFERELEASE(m_pUnkTrident);

    // Delete UI handler sub-object
    if (m_pUIHandler != NULL)
    {
        // Assert that the ref count on the sub-object is 1
        // If this isn't 1, then Trident is holding on to this pointer
        _ASSERTE(m_pUIHandler->m_cRef == 1);
        delete m_pUIHandler;
    }

    if (m_hgDocRestore != NULL)
    {
        GlobalUnlock(m_hgDocRestore);
        GlobalFree(m_hgDocRestore);
    }

#ifdef IE5_SPACING
    if (m_hgMap != NULL)
    {
        GlobalUnlock(m_hgMap);
        GlobalFree(m_hgMap);
        m_hgMap = NULL;
    }
    if (m_hgSpacingNonDSP != NULL)
    {
        GlobalUnlock(m_hgSpacingNonDSP);
        GlobalFree(m_hgSpacingNonDSP);
        m_hgSpacingNonDSP = NULL;
    }
#endif
}

#ifdef IE5_SPACING
void CTriEditDocument::FillUniqueID(BSTR bstrUniqueID, BSTR bstrDspVal, int ichNonDSP, MAPSTRUCT *pMap, int iMapCur, BOOL fLowerCase, int iType)
{
    memcpy((BYTE *)pMap[iMapCur].szUniqueID, (BYTE *)bstrUniqueID, min(wcslen(bstrUniqueID), cchID)*sizeof(WCHAR));
    if (iType == INDEX_DSP)
    {
        memcpy((BYTE *)pMap[iMapCur].szDspID, (BYTE *)bstrDspVal, min(wcslen(bstrDspVal), cchID)*sizeof(WCHAR));
        _ASSERTE(ichNonDSP == -1);
        pMap[iMapCur].ichNonDSP = ichNonDSP;
    }
    else if (iType == INDEX_COMMENT)
    {
        pMap[iMapCur].ichNonDSP = ichNonDSP;
    }
    else if (iType == INDEX_AIMGLINK)
    {
        memcpy((BYTE *)pMap[iMapCur].szDspID, (BYTE *)bstrDspVal, min(wcslen(bstrDspVal), cchID)*sizeof(WCHAR));
        pMap[iMapCur].ichNonDSP = ichNonDSP;
    }
    else if (iType == INDEX_OBJ_COMMENT)
    {
        pMap[iMapCur].ichNonDSP = ichNonDSP;
    }
    else
        _ASSERTE(FALSE);
    pMap[iMapCur].fLowerCase = fLowerCase;
    _ASSERTE(iType >= INDEX_NIL && iType < INDEX_MAX);
    pMap[iMapCur].iType = iType;
}

BOOL CTriEditDocument::FGetSavedDSP(BSTR bstrUniqueID, BSTR *pbstrDspVal, int *pichNonDSP, MAPSTRUCT *pMap, BOOL *pfLowerCase, int *pIndex)
{
    BOOL fRet = FALSE;
    int i;

    // TODO - find a faster way than this linear search...
    for (i = 0; i < m_iMapCur; i++)
    {
        if (0 == _wcsnicmp(pMap[i].szUniqueID, bstrUniqueID, wcslen(bstrUniqueID)))
        {
            fRet = TRUE;
            if (pMap[i].iType == INDEX_DSP)
            {
                *pbstrDspVal = SysAllocString(pMap[i].szDspID);
                *pichNonDSP = -1;
            }
            else if (pMap[i].iType == INDEX_COMMENT)
            {
                *pbstrDspVal = (BSTR)NULL;
                *pichNonDSP = pMap[i].ichNonDSP;
            }
            else if (pMap[i].iType == INDEX_AIMGLINK)
            {
                *pbstrDspVal = SysAllocString(pMap[i].szDspID);
                *pichNonDSP = pMap[i].ichNonDSP;
                _ASSERTE(*pichNonDSP != -1);
            }
            else if (pMap[i].iType == INDEX_OBJ_COMMENT)
            {
                *pbstrDspVal = (BSTR)NULL;
                *pichNonDSP = pMap[i].ichNonDSP;
                _ASSERTE(*pichNonDSP != -1);
            }
            *pfLowerCase = pMap[i].fLowerCase;
            *pIndex = pMap[i].iType;

            goto LRet;
        }
    }

LRet:
    return(fRet);
}

void 
CTriEditDocument::FillNonDSPData(BSTR pOuterTag)
{
    int len = 0;

    _ASSERTE(m_ichspNonDSPMax != -1);
    _ASSERTE(m_ichspNonDSP != -1);
    _ASSERTE(m_hgSpacingNonDSP != NULL);
    _ASSERTE(m_pspNonDSP != NULL);

    // even if pOuterTag is NULL, we still need to store the fact that we have
    // zero bytes of data.
    if (pOuterTag != NULL)
        len = wcslen(pOuterTag);

    if ((int)(m_ichspNonDSP + len + sizeof(int)) > m_ichspNonDSPMax)
    {
        //reallocate & set m_ichspNonDSPMax
        GlobalUnlock(m_hgSpacingNonDSP);
        m_hgSpacingNonDSP = GlobalReAlloc(m_hgSpacingNonDSP, (m_ichspNonDSP + len + sizeof(int)+MIN_SP_NONDSP)*sizeof(WCHAR), GMEM_MOVEABLE|GMEM_ZEROINIT);
        // if this alloc failed, we may still want to continue
        if (m_hgSpacingNonDSP == NULL)
            goto LRet;
        else
        {
            m_pspNonDSP = (WCHAR *)GlobalLock(m_hgSpacingNonDSP);
            _ASSERTE(m_pspNonDSP != NULL);
            m_ichspNonDSPMax = (m_ichspNonDSP + len + sizeof(int)+MIN_SP_NONDSP);
        }
        _ASSERTE(m_ichspNonDSP < m_ichspNonDSPMax);
    }

    memcpy((BYTE *)(m_pspNonDSP+m_ichspNonDSP), (BYTE *)&len, sizeof(int));
    m_ichspNonDSP += sizeof(int)/sizeof(WCHAR);
    memcpy((BYTE *)(m_pspNonDSP+m_ichspNonDSP), (BYTE *)pOuterTag, len*sizeof(WCHAR));
    m_ichspNonDSP += len;

LRet:
    return;

}

void 
CTriEditDocument::ReSetinnerHTMLComment(IHTMLCommentElement *pCommentElement, IHTMLElement* /*pElement*/, int ichspNonDSP)
{
    WCHAR *pStrComment = NULL;
//#ifdef DEBUG
//  CComBSTR bstrOuter, bstrOuterBefore;
//#endif //DEBUG
    int cchComment = 0;

    // get the ich, get the saved comment, set it
    memcpy((BYTE *)&cchComment, (BYTE *)(m_pspNonDSP+ichspNonDSP), sizeof(int));
    _ASSERTE(cchComment > 0);
//#ifdef DEBUG
//  pElement->get_outerHTML(&bstrOuterBefore);
//#endif //DEBUG
    pStrComment = new WCHAR[cchComment + 1];
    memcpy((BYTE *)pStrComment, (BYTE *)(m_pspNonDSP+ichspNonDSP+sizeof(int)/sizeof(WCHAR)), cchComment*sizeof(WCHAR));
    pStrComment[cchComment] = '\0';
    pCommentElement->put_text((BSTR)pStrComment);
//#ifdef DEBUG
//  pElement->get_outerHTML(&bstrOuter);
//#endif //DEBUG
    if (pStrComment)
        delete pStrComment;
//#ifdef DEBUG
//  bstrOuter.Empty();
//  bstrOuterBefore.Empty();
//#endif //DEBUG

}

void 
CTriEditDocument::SetinnerHTMLComment(IHTMLCommentElement *pCommentElement, IHTMLElement* /*pElement*/, BSTR pOuterTag)
{
    WCHAR *pStr = NULL;
    WCHAR *pStrComment = NULL;
    LPCWSTR rgComment[] =
    {
        L"TRIEDITPRECOMMENT-",
        L"-->",
        L"<!--",
    };
//#ifdef DEBUG
//  CComBSTR bstrOuter, bstrInnerBefore, bstrInnerAfter, bstrOuterBefore;
//#endif //DEBUG

    // special case - 
    // send pOuterTag as NULL, if we want to get rid of the comment completely
    if (pOuterTag == NULL)
    {
        pCommentElement->put_text((BSTR)pOuterTag);
        goto LRet;
    }

    //remove the TRIEDITCOMMENT stuff from pOuterTag and set the outerHTML properly
    pStr = wcsstr(pOuterTag, rgComment[0]);
    if (pStr != NULL)
    {
        pStrComment = new WCHAR[wcslen(pOuterTag)-(SAFE_PTR_DIFF_TO_INT(pStr-pOuterTag)+wcslen(rgComment[0]))+wcslen(rgComment[1])+wcslen(rgComment[2])+1];
        if (pStrComment != NULL)
        {
            memcpy( (BYTE *)pStrComment, 
                    (BYTE *)(rgComment[2]),
                    (wcslen(rgComment[2]))*sizeof(WCHAR)
                    );
            memcpy( (BYTE *)(pStrComment+wcslen(rgComment[2])), 
                    (BYTE *)(pStr+wcslen(rgComment[0])),
                    (wcslen(pOuterTag)-(SAFE_PTR_DIFF_TO_INT(pStr-pOuterTag)+wcslen(rgComment[0]))-wcslen(rgComment[1]))*sizeof(WCHAR)
                    );
            memcpy( (BYTE *)(pStrComment+wcslen(rgComment[2])+wcslen(pOuterTag)-(pStr-pOuterTag+wcslen(rgComment[0]))-wcslen(rgComment[1])),
                    (BYTE *)(rgComment[1]),
                    (wcslen(rgComment[1]))*sizeof(WCHAR)
                    );
            pStrComment[wcslen(pOuterTag)-(pStr-pOuterTag+wcslen(rgComment[0]))-wcslen(rgComment[1])+wcslen(rgComment[1])+wcslen(rgComment[2])] = '\0';
//#ifdef DEBUG
//          pElement->get_innerHTML(&bstrInnerBefore);
//          pElement->get_outerHTML(&bstrOuterBefore);
//#endif //DEBUG
            pCommentElement->put_text((BSTR)pStrComment);
//#ifdef DEBUG
//          pElement->get_outerHTML(&bstrOuter);
//          pElement->get_innerHTML(&bstrInnerAfter);
//#endif //DEBUG
            delete pStrComment;
        }
    }
LRet:
//#ifdef DEBUG
//  bstrOuter.Empty(); 
//  bstrInnerBefore.Empty();
//  bstrInnerAfter.Empty();
//  bstrOuterBefore.Empty();
//#endif //DEBUG
    return;
}

void
CTriEditDocument::RemoveEPComment(IHTMLObjectElement *pObjectElement, BSTR bstrAlt, 
                                  int cch, BSTR *pbstrAltComment, BSTR *pbstrAltNew)
{
    int ich = 0;
    WCHAR *pAltNew = NULL;
    WCHAR *pStrAlt = bstrAlt;
    WCHAR *pStr = NULL;
    WCHAR *pStrEnd = NULL;
    WCHAR *pStrComment = NULL;
    LPCWSTR rgComment[] =
    {
        L"<!--ERRORPARAM",
        L"ERRORPARAM-->",
    };

    if (bstrAlt == (BSTR)NULL || pObjectElement == NULL)
        return;

    // look for ERRORPARAM
    pStr = wcsstr(bstrAlt, rgComment[0]);
    pStrEnd = wcsstr(bstrAlt, rgComment[1]);
    if (pStr != NULL && pStrEnd != NULL)
    {
        pStrEnd += wcslen(rgComment[1]);
        pStrComment = new WCHAR[SAFE_PTR_DIFF_TO_INT(pStrEnd-pStr)+1];
        if (pStrComment == NULL)
            goto LRetNull;
        memcpy((BYTE *)pStrComment, (BYTE *)pStr, SAFE_PTR_DIFF_TO_INT(pStrEnd-pStr)*sizeof(WCHAR));
        pStrComment[pStrEnd-pStr] = '\0';
        *pbstrAltComment = SysAllocString(pStrComment);
        delete pStrComment;

        pAltNew = new WCHAR[cch+1]; // max size
        if (pAltNew == NULL)
            goto LRetNull;
        // remove stuff from pStr till pStrEnd & copy into *pbstrAltNew
        if (pStr > pStrAlt)
        {
            memcpy((BYTE *)pAltNew, (BYTE *)pStrAlt, SAFE_PTR_DIFF_TO_INT(pStr-pStrAlt)*sizeof(WCHAR));
            ich += SAFE_PTR_DIFF_TO_INT(pStr-pStrAlt);
        }
        if ((pStrAlt+cch)-pStrEnd > 0)
        {
            memcpy((BYTE *)(pAltNew+ich), (BYTE *)pStrEnd, SAFE_PTR_DIFF_TO_INT((pStrAlt+cch)-pStrEnd)*sizeof(WCHAR));
            ich += SAFE_PTR_DIFF_TO_INT((pStrAlt+cch)-pStrEnd);
        }
        pAltNew[ich] = '\0';
        *pbstrAltNew = SysAllocString(pAltNew);
        delete pAltNew;
    }
    else
    {
LRetNull:
		*pbstrAltNew = (bstrAlt) ? SysAllocString(bstrAlt) : (BSTR)NULL;
        *pbstrAltComment = (BSTR)NULL;
    }

} /* CTriEditDocument::RemoveEPComment() */

HRESULT 
CTriEditDocument::SetObjectComment(IHTMLObjectElement *pObjectElement, BSTR bstrAltNew)
{
    HRESULT hr;

    _ASSERTE(pObjectElement != NULL);
    hr = pObjectElement->put_altHtml(bstrAltNew);
    return(hr);

} /* CTriEditDocument::SetObjectComment() */

void
CTriEditDocument::AppendEPComment(IHTMLObjectElement *pObjectElement, int ichspNonDSP)
{
    CComBSTR bstrAltNew;
    int cch;
    WCHAR *pStrSaved = NULL;
    HRESULT hr;
    
    // get current altHtml from the tree
    hr = pObjectElement->get_altHtml(&bstrAltNew);
    if (hr != S_OK || bstrAltNew == (BSTR)NULL)
        goto LRet;
    
    // get saved altHtml in m_pspNonDSP
    memcpy((BYTE *)&cch, (BYTE *)(m_pspNonDSP+ichspNonDSP), sizeof(int));
    if (cch <= 0)
        goto LRet;
    pStrSaved = new WCHAR[cch + 1];
    if (pStrSaved == NULL)
        goto LRet;
    memcpy((BYTE *)pStrSaved, (BYTE *)(m_pspNonDSP+ichspNonDSP+sizeof(int)/sizeof(WCHAR)), cch*sizeof(WCHAR));
    pStrSaved[cch] = '\0';
    
    // append saved altHtml
    bstrAltNew += pStrSaved;

    // save it back in the tree
    hr = pObjectElement->put_altHtml(bstrAltNew);
    if (pStrSaved)
        delete pStrSaved;

LRet:
    return;
} /* CTriEditDocument::AppendEPComment() */


void 
CTriEditDocument::MapUniqueID(BOOL fGet)
{
    CComPtr<IHTMLDocument2> pHTMLDoc;
    CComPtr<IHTMLElementCollection> pHTMLCollection;
    CComPtr<IDispatch> pDispControl;
    CComPtr<IHTMLElement> pElement;
    CComPtr<IHTMLUniqueName> pUniqueName;
    CComPtr<IHTMLCommentElement> pCommentElement;
    CComPtr<IHTMLObjectElement> pObjectElement;

    HRESULT hr;
    //CComBSTR bstrUniqueID;
    WCHAR *pAttr = NULL;
    WCHAR *pAttrL = NULL;
    WCHAR *pAttrDSU = NULL;
    long len;
    int i;
    LPCWSTR szDSP[] =
    {
        L"DESIGNTIMESP",
        L"designtimesp",
        L"DESIGNTIMEURL",
    };
    LPCWSTR szComment[] =
    {
        L"<!--TRIEDITCOMMENT",
        L"<!--ERRORPARAM",
        L"<!--ERROROBJECT",
    };
    VARIANT var, vaName, vaIndex;

    if (!IsIE5OrBetterInstalled())
        goto LRet;

    pHTMLDoc = NULL;
    hr = m_pUnkTrident->QueryInterface(IID_IHTMLDocument2, (void **) &pHTMLDoc);
    if (hr != S_OK)
        goto LRet;


    pHTMLDoc->get_all(&pHTMLCollection);
    if (hr != S_OK)
        goto LRet;

    pAttr = new WCHAR[wcslen(szDSP[0])+1];
    memcpy((BYTE *)pAttr, (BYTE *)szDSP[0], wcslen(szDSP[0])*sizeof(WCHAR));
    pAttr[wcslen(szDSP[0])] = '\0';
    pAttrL = new WCHAR[wcslen(szDSP[1])+1];
    memcpy((BYTE *)pAttrL, (BYTE *)szDSP[1], wcslen(szDSP[1])*sizeof(WCHAR));
    pAttrL[wcslen(szDSP[1])] = '\0';
    pAttrDSU = new WCHAR[wcslen(szDSP[2])+1];
    memcpy((BYTE *)pAttrDSU, (BYTE *)szDSP[2], wcslen(szDSP[2])*sizeof(WCHAR));
    pAttrDSU[wcslen(szDSP[2])] = '\0';

    pHTMLCollection->get_length(&len);

    if (len == 0)
        goto LRet;

    if (!fGet)
    {
        // now we know that we atleast have one element, lets allocate space for saving
        // UniqueID's & designtimesp's
        if (m_pMapArray == NULL) // this is the first time we are here
        {
            _ASSERTE(m_hgMap == NULL);
            m_hgMap = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, MIN_MAP*sizeof(MAPSTRUCT));
            if (m_hgMap == NULL)
                goto LRet;
            m_cMapMax = MIN_MAP;
        }
        _ASSERTE(m_hgMap != NULL);
        m_pMapArray = (MAPSTRUCT *) GlobalLock(m_hgMap);
        _ASSERTE(m_pMapArray != NULL);
        // even if we allocate the space for m_hgMap here or not, we should start from 0
        m_iMapCur = 0;
        // zeroise the array
        memset((BYTE *)m_pMapArray, 0, m_cMapMax*sizeof(MAPSTRUCT));
        
        if (m_pspNonDSP == NULL) // this is the first time we are here
        {
            _ASSERTE(m_hgSpacingNonDSP == NULL);
            m_hgSpacingNonDSP = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, MIN_SP_NONDSP*sizeof(WCHAR));
            if (m_hgSpacingNonDSP == NULL)
                goto LRet;
            m_ichspNonDSPMax = MIN_SP_NONDSP;
        }
        _ASSERTE(m_hgSpacingNonDSP != NULL);
        m_pspNonDSP = (WCHAR *) GlobalLock(m_hgSpacingNonDSP);
        _ASSERTE(m_pspNonDSP != NULL);
        // even if we allocate the space for m_hgSpacingNonDSP here or not, we should start from 0
        m_ichspNonDSP = 0;
        // zeroise the array
        memset((BYTE *)m_pspNonDSP, 0, m_ichspNonDSPMax*sizeof(WCHAR));
    }
    else // if (fGet)
    {
        if (m_iMapCur < 1) // we don't have any mappings saved
            goto LRet;
        m_pMapArray = (MAPSTRUCT *)GlobalLock(m_hgMap);
        _ASSERTE(m_pMapArray != NULL);

        m_pspNonDSP = (WCHAR *)GlobalLock(m_hgSpacingNonDSP);
        _ASSERTE(m_pspNonDSP != NULL);
    }

    // loop through all the elemets and fill m_pMapArray
    for (i = 0; i < len; i++)
    {
        VARIANT_BOOL fSuccess;

        if (!fGet)
        {
            // reallocate m_hgMap if needed
            if (m_iMapCur == m_cMapMax - 1)
            {
                GlobalUnlock(m_hgMap);
                m_hgMap = GlobalReAlloc(m_hgMap, (m_cMapMax+MIN_MAP)*sizeof(MAPSTRUCT), GMEM_MOVEABLE|GMEM_ZEROINIT);
                // if this alloc failed, we may still want to continue
                if (m_hgMap == NULL)
                    goto LRet;
                else
                {
                    m_pMapArray = (MAPSTRUCT *)GlobalLock(m_hgMap);
                    _ASSERTE(m_pMapArray != NULL);
                    m_cMapMax += MIN_MAP;
                }
            }
            _ASSERTE(m_iMapCur < m_cMapMax);
        }

        VariantInit(&vaName);
        VariantInit(&vaIndex);

        V_VT(&vaName) = VT_ERROR;
        V_ERROR(&vaName) = DISP_E_PARAMNOTFOUND;

        V_VT(&vaIndex) = VT_I4;
        V_I4(&vaIndex) = i;

        pDispControl = NULL;
        hr = pHTMLCollection->item(vaIndex, vaName, &pDispControl);
        VariantClear(&vaName);
        VariantClear(&vaIndex);
        // Trident has a bug that if the object was nested inside <scripts> tags,
        // it returns S_OK with pDispControl as NULL. (See VID BUG 11303)
        if (hr == S_OK && pDispControl != NULL)
        {
            pElement = NULL;
            hr = pDispControl->QueryInterface(IID_IHTMLElement, (void **) &pElement);
            if (hr == S_OK && pElement != NULL)
            {
//#ifdef DEBUG
//              CComBSTR bstrTagName, bstrClsName;
//
//              hr = pElement->get_className(&bstrClsName);
//              hr = pElement->get_tagName(&bstrTagName);
//#endif //DEBUG
                if (!fGet) // saving the data
                {
                    BOOL fLowerCase = FALSE;

                    VariantInit(&var);
                    // KNOWN (and postponed) TRIDENT BUG - ideally, we should be able to look for hr's value here,
                    // but trident returns S_OK even if it can't get the attribute!!!
                    hr = pElement->getAttribute(pAttr, 0, &var); // look for DESIGNTIMESP (upper or lower case'd)
                    if (var.vt == VT_BSTR)
                    {
                        CComVariant varT;

                        hr = pElement->getAttribute(pAttrL, 1, &varT); // look for lowercase designtimesp
                        if (varT.vt == VT_BSTR)
                            fLowerCase = TRUE;
                    }
                    if (var.vt == VT_BSTR && var.bstrVal != NULL)
                    {
                        CComBSTR bstrUniqueID;
                        CComVariant varDSU;
                        int iType = INDEX_DSP; // initial value
                        int ich = -1; // initial value;
//#ifdef DEBUG
//                      CComBSTR pOuterTag;
//#endif //DEBUG

                        pUniqueName = NULL;
                        hr = pDispControl->QueryInterface(IID_IHTMLUniqueName, (void **) &pUniqueName);
                        if (hr == S_OK && pUniqueName != NULL)
                            hr = pUniqueName->get_uniqueID(&bstrUniqueID);
                        if (pUniqueName)
                            pUniqueName.Release();
                        //pHTMLDoc3->get_uniqueID(&bstrUniqueID);

//#ifdef DEBUG
//                      pElement->get_outerHTML(&pOuterTag);
//                      pOuterTag.Empty();
//#endif //DEBUG
                        // at this point, we know that this tag had designtimesp.
                        // It may also have additional triedit attributes like designtimeurl
                        // lets check for those as well
                        hr = pElement->getAttribute(pAttrDSU, 0, &varDSU); // look for DESIGNTIMEURL (upper or lower case'd)
                        if (   hr == S_OK 
                            && varDSU.vt == VT_BSTR 
                            && varDSU.bstrVal != NULL
                            )
                        {
                            // we found 'designtimeurl'
                            iType = INDEX_AIMGLINK;
                            ich = m_ichspNonDSP;

                            FillNonDSPData(varDSU.bstrVal);
//#ifdef DEBUG
//                          pElement->get_outerHTML(&pOuterTag);
//                          pOuterTag.Empty();
//#endif //DEBUG
                            // now remove designtimeurl & its value
                            hr = pElement->removeAttribute(pAttrDSU, 0, &fSuccess);
//#ifdef DEBUG
//                          pElement->get_outerHTML(&pOuterTag);
//                          pOuterTag.Empty();
//#endif //DEBUG
                        }

                        // fill ID mapping structure
                        FillUniqueID(bstrUniqueID, var.bstrVal, ich, m_pMapArray, m_iMapCur, fLowerCase, iType);
                        bstrUniqueID.Empty();
                        m_iMapCur++;
//#ifdef DEBUG
//                      pElement->get_outerHTML(&pOuterTag);
//                      pOuterTag.Empty();
//#endif //DEBUG
                        // Now, remove designtimesp and its value
                        hr = pElement->removeAttribute(pAttr, 0, &fSuccess);
//#ifdef DEBUG
//                      pElement->get_outerHTML(&pOuterTag);
//                      pOuterTag.Empty();
//#endif //DEBUG
                        bstrUniqueID.Empty();
                    }
                    else if (var.vt == VT_NULL)
                    {
                        CComBSTR bstrUniqueID;
                        CComBSTR pOuterTag;
//#ifdef DEBUG
//                      CComBSTR pOuter;
//#endif // DEBUG

                        // see if this is a comment and save it
                        pElement->get_outerHTML(&pOuterTag);
                        if (   pOuterTag != NULL
                            && 0 == _wcsnicmp(pOuterTag, szComment[0], wcslen(szComment[0]))
                            )
                        {
                            pUniqueName = NULL;
                            hr = pDispControl->QueryInterface(IID_IHTMLUniqueName, (void **) &pUniqueName);
                            if (hr == S_OK && pUniqueName != NULL)
                                hr = pUniqueName->get_uniqueID(&bstrUniqueID);
                            if (pUniqueName)
                                pUniqueName.Release();

                            // fill ID mapping structure
                            FillUniqueID(bstrUniqueID, NULL, m_ichspNonDSP, m_pMapArray, m_iMapCur, fLowerCase, INDEX_COMMENT);
                            bstrUniqueID.Empty();
                            m_iMapCur++;

                            FillNonDSPData(pOuterTag);
                            // now, remove the comment spacing stuff and set_outerHTML
                            hr = pDispControl->QueryInterface(IID_IHTMLCommentElement, (void **) &pCommentElement);
                            if (hr == S_OK && pCommentElement != NULL)
                                SetinnerHTMLComment(pCommentElement, pElement, pOuterTag);
//#ifdef DEBUG
//                          pElement->get_outerHTML(&pOuter);
//                          pOuter.Empty();
//#endif //DEBUG
                            if (pCommentElement)
                                pCommentElement.Release();
                        }
                        else if (      S_OK == pDispControl->QueryInterface(IID_IHTMLObjectElement, (void **) &pObjectElement)
                                    && pObjectElement != NULL
                                    )
                        {
                            BSTR bstrAlt, bstrAltNew, bstrAltComment;

                            bstrAlt = bstrAltNew = bstrAltComment = NULL;
                            pUniqueName = NULL;
                            hr = pDispControl->QueryInterface(IID_IHTMLUniqueName, (void **) &pUniqueName);
                            if (hr == S_OK && pUniqueName != NULL)
                                hr = pUniqueName->get_uniqueID(&bstrUniqueID);
                            if (pUniqueName)
                                pUniqueName.Release();

                            // fill ID mapping structure
                            FillUniqueID(bstrUniqueID, NULL, m_ichspNonDSP, m_pMapArray, m_iMapCur, FALSE, INDEX_OBJ_COMMENT);
                            bstrUniqueID.Empty();
                            m_iMapCur++;

                            pObjectElement->get_altHtml(&bstrAlt);
                            // remove <!--ERRORPARAM ...ERRORPARAM-->
                            // ASSUME (FOR NOW) that we won't see TRIEDITCOMMENT or others here
                            RemoveEPComment(pObjectElement, bstrAlt, SysStringLen(bstrAlt), &bstrAltComment, &bstrAltNew);

                            FillNonDSPData(bstrAltComment);
                            SysFreeString(bstrAltComment);

                            hr = SetObjectComment(pObjectElement, bstrAltNew);
                            SysFreeString(bstrAltNew);

                            SysFreeString(bstrAlt);
//#ifdef DEBUG
//                          pObjectElement->get_altHtml(&bstrAlt);
//                          bstrAlt.Empty();
//#endif //DEBUG
                        }
                        if (pObjectElement)
                            pObjectElement.Release();

                        bstrUniqueID.Empty();
                        pOuterTag.Empty();
                    }
                    VariantClear(&var);
                }
                else // if (fGet)
                {
                    BOOL fLowerCase = FALSE;
                    int index, ichNonDSP;
                    CComBSTR bstrUniqueID;

                    pUniqueName = NULL;
                    hr = pDispControl->QueryInterface(IID_IHTMLUniqueName, (void **) &pUniqueName);
                    if (hr == S_OK && pUniqueName != NULL)
                        hr = pUniqueName->get_uniqueID(&bstrUniqueID);
                    if (pUniqueName)
                        pUniqueName.Release();

                    // get the uniqueID
                    //pHTMLDoc3->get_uniqueID(&bstrUniqueID);
                    // see if we have it in m_hgMap, if we do, get corresponding designtimesp ID
                    // if we don't have a DSP for this uniqueID, this is newly inserted element
                    VariantInit(&var);
                    if (FGetSavedDSP(bstrUniqueID, &(var.bstrVal), &ichNonDSP, m_pMapArray, &fLowerCase, &index))
                    {
//#ifdef DEBUG
//                      CComBSTR pOuterTag;
//#endif //DEBUG
                        // insert (correct case) "designtimesp = xxxx" in the tag by setting attribute/value
                        var.vt = VT_BSTR;
#ifdef DEBUG
                        if (index == INDEX_DSP)
                            _ASSERTE(var.bstrVal != NULL && ichNonDSP == -1);
                        else if (index == INDEX_COMMENT)
                            _ASSERTE(var.bstrVal == (BSTR)NULL && ichNonDSP != -1);
                        else if (index == INDEX_AIMGLINK)
                            _ASSERTE(var.bstrVal != NULL && ichNonDSP != -1);
//                      pElement->get_outerHTML(&pOuterTag);
//                      pOuterTag.Empty();
#endif //DEBUG
                        if (index == INDEX_DSP)
                        {
                            if (fLowerCase)
                                hr = pElement->setAttribute(pAttrL, var, 1);
                            else
                                hr = pElement->setAttribute(pAttr, var, 1);
                        }
                        else if (index == INDEX_COMMENT)
                        {
                            hr = pDispControl->QueryInterface(IID_IHTMLCommentElement, (void **) &pCommentElement);
                            if (hr == S_OK && pCommentElement != NULL)
                                ReSetinnerHTMLComment(pCommentElement, pElement, ichNonDSP);
                            if (pCommentElement)
                                pCommentElement.Release();
                        }
                        else if (index == INDEX_AIMGLINK)
                        {
                            CComVariant varDSU;
                            WCHAR *pchDSU;
                            int cchDSU = 0;

                            if (fLowerCase)
                                hr = pElement->setAttribute(pAttrL, var, 1);
                            else
                                hr = pElement->setAttribute(pAttr, var, 1);

                            // put designtimeurl as well
                            // get the data from ichNonDSP and setAttribute
                            _ASSERTE(ichNonDSP != -1);
                            memcpy((BYTE *)&cchDSU, (BYTE *)(m_pspNonDSP+ichNonDSP), sizeof(INT));
                            _ASSERTE(cchDSU > 0);
                            pchDSU = new WCHAR[cchDSU+1];
                            memcpy((BYTE *)pchDSU, (BYTE *)(m_pspNonDSP+ichNonDSP+sizeof(int)/sizeof(WCHAR)), cchDSU*sizeof(WCHAR));
                            pchDSU[cchDSU] = '\0';
                            varDSU.bstrVal = SysAllocString(pchDSU);
                            varDSU.vt = VT_BSTR;
                            hr = pElement->setAttribute(pAttrDSU, varDSU, 1);
                            delete pchDSU;
                        } //else if (index == INDEX_AIMGLINK)
                        else if (index == INDEX_OBJ_COMMENT)
                        {
                            hr = pDispControl->QueryInterface(IID_IHTMLObjectElement, (void **) &pObjectElement);
                            if (pObjectElement != NULL)
                            {
                                AppendEPComment(pObjectElement, ichNonDSP);
                            }
                            else // something is not right, just ignore
                            {
                                _ASSERTE(FALSE);
                            }
                            if (pObjectElement)
                                pObjectElement.Release();
                        }
//#ifdef DEBUG
//                      pElement->get_outerHTML(&pOuterTag);
//                      pOuterTag.Empty();
//#endif //DEBUG
                    } // if (FGetSavedDSP())
                    VariantClear(&var);
                    bstrUniqueID.Empty();
                } // end of else case of 'if (!fGet)'
//#ifdef DEBUG
//              bstrTagName.Empty();
//              bstrClsName.Empty();
//#endif //DEBUG
            } // if (hr == S_OK && pElement != NULL)
            if (pElement)
                pElement.Release();
        } // if (hr == S_OK && pDispControl != NULL)
        if (pDispControl)
            pDispControl.Release();
    } // for (i ...)

LRet:
    if (pAttr != NULL)
        delete pAttr; 
    if (pAttrL != NULL)
        delete pAttrL;
    if (pAttrDSU != NULL)
        delete pAttrDSU;
    if (pHTMLCollection)
        pHTMLCollection.Release();
    if (pHTMLDoc)
        pHTMLDoc.Release();
    if (m_hgMap != NULL)
        GlobalUnlock(m_hgMap);
    if (m_hgSpacingNonDSP != NULL)
        GlobalUnlock(m_hgSpacingNonDSP);


    return;
}

/////////////////////////////////////////////////////////////////////
//
STDMETHODIMP
CTridentEventSink::Invoke(DISPID dispid, REFIID, LCID, USHORT, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*)
{
    switch(dispid)
    {
    case DISPID_HTMLDOCUMENTEVENTS_ONREADYSTATECHANGE:
        {
            CComBSTR p;
            HRESULT hr;
            LPCWSTR szComplete[] =
            {
                L"complete",
            };

            //look for READYSTATE_COMPLETE)
            hr = m_pHTMLDocument2->get_readyState(&p);
            if (   hr == S_OK
                && 0 == _wcsnicmp(p, szComplete[0], wcslen(szComplete[0]))
                && m_pTriEditDocument->FIsFilterInDone()
                )
            {
                CComVariant varDirty;

                // we know that the document is loaded.
                // get pointer to DOM and access all tags
                // create a table that holds mapping from designtimespID to uniqueID
                // save the mapping and remove designtimesp attribute

                // at the time of save, fill in the designtimesp's for each uniqueID

                m_pTriEditDocument->MapUniqueID(/*fGet*/FALSE);
                m_pTriEditDocument->SetFilterInDone(FALSE);
                // set the document to be CLEAN (non-DIRTY), we don't care about hr
                varDirty.bVal = FALSE;
                varDirty.vt = VT_BOOL;
                hr = m_pTriEditDocument->Exec(&CGID_MSHTML, IDM_SETDIRTY, MSOCMDEXECOPT_DODEFAULT, &varDirty, NULL);
            }
            p.Empty();
        }
        break;
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////
//
HRESULT 
CBaseTridentEventSink::Advise(IUnknown* pUnkSource, REFIID riidEventInterface)
{
    HRESULT hr = E_FAIL;
    
    if(NULL == pUnkSource)
    {
        _ASSERTE(FALSE);
        return E_INVALIDARG;
    }

    if(m_dwCookie > 0)
    {
        _ASSERTE(FALSE);
        return E_UNEXPECTED;
    }

    hr = AtlAdvise(pUnkSource, static_cast<IUnknown*>(this), riidEventInterface, &m_dwCookie);
    if(SUCCEEDED(hr) && m_dwCookie > 0)
    {
        m_iidEventInterface = riidEventInterface;

        m_pUnkSource = pUnkSource; // no addref. Advise already addref'ed it
        return S_OK;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
void 
CBaseTridentEventSink::Unadvise(void)
{
    if(0 == m_dwCookie)
        return;

    AtlUnadvise(m_pUnkSource, m_iidEventInterface, m_dwCookie);
    m_dwCookie = 0; 
    m_pUnkSource = NULL;
}






//------------------------------------------------------------------------------
// IPersistStreamInit
//------------------------------------------------------------------------------



STDMETHODIMP CTriEditDocument::Load(LPSTREAM pStm)
{
    ATLTRACE(_T("CTriEditDocument::IPersistStreamInit::Load"));
    _ASSERTE(m_pTridentPersistStreamInit != NULL);      
    return m_pTridentPersistStreamInit->Load(pStm);
}

STDMETHODIMP CTriEditDocument::Save(LPSTREAM pStm, BOOL fClearDirty)
{
    ATLTRACE(_T("CTriEditDocument::IPersistStreamInit::Save"));
    _ASSERTE(m_pTridentPersistStreamInit != NULL);      

    // before we deligate Save to Trident, do the preFiltering stuff
    if (m_hgMap != NULL)
    {
        MapUniqueID(/*fGet*/TRUE);
    }

    return m_pTridentPersistStreamInit->Save(pStm, fClearDirty);
}

STDMETHODIMP CTriEditDocument::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ATLTRACE(_T("CTriEditDocument::IPersistStreamInit::GetSizeMax"));
    _ASSERTE(m_pTridentPersistStreamInit != NULL);      
    return m_pTridentPersistStreamInit->GetSizeMax(pcbSize);
}

STDMETHODIMP CTriEditDocument::IsDirty()
{
    //ATLTRACE(_T("CTriEditDocument::IPersistStreamInit::IsDirty\n"));
    _ASSERTE(m_pTridentPersistStreamInit != NULL);      
    return m_pTridentPersistStreamInit->IsDirty();
}

STDMETHODIMP CTriEditDocument::InitNew()
{
    ATLTRACE(_T("CTriEditDocument::IPersistStreamInit::InitNew\n"));
    _ASSERTE(m_pTridentPersistStreamInit != NULL);      
    return(m_pTridentPersistStreamInit->InitNew());
}

STDMETHODIMP CTriEditDocument::GetClassID(CLSID *pClassID)
{
    ATLTRACE(_T("CTriEditDocument::IPersistStreamInit::GetClassID\n"));
    _ASSERTE(m_pTridentPersistStreamInit != NULL);
    *pClassID = GetObjectCLSID();
    return S_OK;
}
#endif //IE5_SPACING
