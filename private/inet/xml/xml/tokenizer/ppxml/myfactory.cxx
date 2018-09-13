/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/

#include "MyFactory.hxx"

#include <stdio.h>


void printIndent(long len)
{
    while (len > 0)
    {
        printf("    ");
        len--;
    }
}

void print(const WCHAR __RPC_FAR *pwcText, ULONG ulLen)
{
    for (ULONG i = 0; i < ulLen; i++)
        putchar((unsigned char)pwcText[i]);
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE MyFactory::NotifyEvent( 
			/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
			/* [in] */ XML_NODEFACTORY_EVENT iEvt)
{
	switch (iEvt)
	{
	case XMLNF_STARTDTDSUBSET:
		printf(" [");
		_fNLPending = true;
		break;
	case XMLNF_ENDDTDSUBSET:
		printf("]");
		_fNLPending = true;
		break;
	}
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MyFactory::BeginChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    switch (pNodeInfo->dwType)
    {
    case XML_ELEMENT:
        printf( ">");
        _fNLPending = true;
        _fMixed = false;
        break;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MyFactory::EndChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ BOOL fEmptyNode,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    _lLevel--;
    if (_fNLPending && ! _fMixed && ! _fCompact)
    {
        _fNLPending = false;
        printf("\n");
        printIndent(_lLevel);
    }
    switch (pNodeInfo->dwType)
    {
    case XML_ELEMENT:                    
        if (!fEmptyNode)
        {
            printf( "</");
            print(pNodeInfo->pwcText, pNodeInfo->ulLen);
            printf( ">");
        }
        else
        {
            printf( "/>");
        }
        _fNLPending = true;
        _fMixed = false;
        break;
    case XML_DTDATTRIBUTE:
    case XML_ATTRIBUTE:
        printf("\"");
        break;
    case XML_XMLDECL:                        
        _fXMLDecl = false;
        // fallthrough
    case XML_PI:
        printf( "?>");
        _fNLPending = true;
        _fMixed = false;
        break;
    case XML_DOCTYPE:
    case XML_ENTITYDECL:
    case XML_ELEMENTDECL:
    case XML_ATTLISTDECL:
    case XML_NOTATION:
        printf( ">");
        _fNLPending = true;
        _fMixed = false;
        break;
    case XML_DTDSUBSET:
        _fNLPending = true;
        break;
    case XML_GROUP:
        printf( ")");
        break;
    case XML_INCLUDESECT:
        printf( "]]>");
        _fNLPending = true;
        _fMixed = false;
        break;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MyFactory::CreateNode( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ PVOID pNode,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
{
    HRESULT hr = S_OK;
    XML_NODE_INFO* pNodeInfo = *apNodeInfo;

    if (_fNLPending && ! _fCompact  &&
        pNodeInfo->dwType != XML_PCDATA &&
        pNodeInfo->dwType != XML_ENTITYREF &&
        pNodeInfo->dwType != XML_WHITESPACE)
    {
        _fNLPending = false;
        printf("\n");
        printIndent(_lLevel);
    }
    switch (pNodeInfo->dwType)
    {
        // ---- container nodes ---
    case XML_ELEMENT:
        _fMixed = false;
        printf( "<");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        break;
    case XML_ATTRIBUTE:
        printf(" ");
        print( pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf( "=\"");
        break;
    case XML_XMLDECL:                        
        _fXMLDecl = true;
    case XML_PI:
        _fMixed = false;
        printf( "<?");                        
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf( " ");
        break;
    case XML_DOCTYPE:
        _fMixed = false;
        printf( "<!DOCTYPE ");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf(" ");
        break;
    case XML_DTDATTRIBUTE:
        printf(" ");
        print( pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf( " \"");
        break;
    case XML_ENTITYDECL:
        _fMixed = false;
        printf( "<!ENTITY ");
        if (pNodeInfo->dwSubType == XML_PENTITYDECL)
        {
            printf("% ");
        }
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf(" ");
        break;
    case XML_ELEMENTDECL:
        _fMixed = false;
        printf( "<!ELEMENT ");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf(" ");
        break;
    case XML_ATTLISTDECL:
        _fMixed = false;
        printf( "<!ATTLIST ");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf(" ");
        break;
    case XML_NOTATION:
        _fMixed = false;
        printf( "<!NOTATION ");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf(" ");
        break;
    case XML_GROUP:
        printf(" (");
        break;
    case XML_INCLUDESECT:
        _fMixed = false;
        printf( "<![");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen); // parameter entities ???
        printf( "[");
        break;        

        // ---- terminal nodes --- 
    case XML_PCDATA: 
        _fMixed = true;
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        break;
    case XML_CDATA:
        _fMixed = false;
        printf( "<![CDATA[");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf( "]]>");
        _fNLPending = true;
        break;
    case XML_IGNORESECT:
        _fMixed = false;
        printf( "<![IGNORE[");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf( "]]>");
        _fNLPending = true;
        break;
    case XML_COMMENT:
        _fMixed = false;
        printf( "<!--");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf( "-->");
        _fNLPending = true;
        break;
    case XML_ENTITYREF:
        _fMixed = true;
        printf( "&");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf( ";");
        break;
    case XML_WHITESPACE:
        // ignore current formatting.
        break;
    case XML_NAME:
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf( " ");
        break;
    case XML_STRING:
        printf( "\"");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf( "\" ");
        break;
    case XML_PEREF:
        _fMixed = true;
        printf( "%");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        printf( ";");
        break;
    case XML_MODEL:
        switch (pNodeInfo->dwSubType)
        {
        case XML_EMPTY:
        case XML_ANY:
            printf(" ");
            print(pNodeInfo->pwcText, pNodeInfo->ulLen);
            break;
        case XML_MIXED:
            printf("#PCDATA");
            break;
        case XML_SEQUENCE:
            printf(",");
            break;
        case XML_CHOICE:
            printf("|");
            break;
        case XML_STAR:
            printf("*");
            break;
        case XML_PLUS:
            printf("+");
            break;
        case XML_QUESTIONMARK:
            printf("?");
            break;
        }
        break;
    case XML_ATTDEF:
    case XML_ATTTYPE:
    case XML_ATTPRESENCE:
        printf(" ");
        print(pNodeInfo->pwcText, pNodeInfo->ulLen);
        break;
    case XML_DTDSUBSET:
        break;
    default: 
        break;
    }
    if (! pNodeInfo->fTerminal )
    {
        _lLevel++;
    }

    // Process the attributes by calling createNode recurrsively as if
    // it was the old node factory interface.
    if (cNumRecs > 1)
    {
        bool fInAttribute = false;
        XML_NODE_INFO* pLastAttr;
        PVOID pNode = pNodeInfo->pNode;
        PVOID pParent = pNode;
        apNodeInfo++;
        for (long i = cNumRecs-1; i > 0; i--)
        {
            XML_NODE_INFO* pNodeInfo = *apNodeInfo;
            if (pNodeInfo->dwType == XML_ATTRIBUTE)
            {
                if (fInAttribute)
                {
                    hr = EndChildren(pSource, FALSE, pLastAttr);
                    if (FAILED(hr)) goto CleanUp;
                }
                pLastAttr = pNodeInfo;
                fInAttribute = true;
                pParent = pNode; // reset parent back to element node
            }
            hr = CreateNode(pSource, pParent, 1, apNodeInfo);

            if (pNodeInfo->dwType == XML_ATTRIBUTE)
            {
                // attributes are also parents of their child PCDATA nodes.
                pParent = pNodeInfo->pNode;
            }
            if (FAILED(hr)) goto CleanUp;
            apNodeInfo++;
        }
        if (fInAttribute)
        {
            hr = EndChildren(pSource, FALSE, pLastAttr);
        }
    }
CleanUp:
    return hr;
}



