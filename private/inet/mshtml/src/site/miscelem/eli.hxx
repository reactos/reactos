//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       eli.hxx
//
//  Contents:   CLIElement class
//
//----------------------------------------------------------------------------

#ifndef I_ELI_HXX_
#define I_ELI_HXX_
#pragma INCMSG("--- Beg 'eli.hxx'")

#define _hxx_
#include "li.hdl"

MtExtern(CLIElement)

// The index+version stored on each list item
class CIndexVersion
{
public:
    DWORD _dwVersion;
    LONG  _lValue;
};

// The List value returned by the list code for render purposes
class CListValue
{
public:
    LONG                  _lValue;
    styleListStyleType    _style;
};

class CListElement;

class CLIElement : public CElement
{
    DECLARE_CLASS_TYPES(CLIElement, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLIElement))

    CLIElement (CDoc *pDoc)
      : CElement(ETAG_LI, pDoc){}
    ~CLIElement() {}

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    virtual void  Notify(CNotification *pNF);

    #define _CLIElement_
    #include "li.hdl"
    
    CIndexVersion _ivIndex;
    VOID  GetValidValue(CListValue *pLV, CMarkup *pMarkup, CTreeNode *pLINode, CTreeNode *pNodeListElement, CElement *pElementFL);
    BOOL  IsIndexValid(CListElement *pListElement);
    LONG  FindPreviousValidIndexedElement(CTreeNode *pNodeListIndex,
                                          CTreeNode *pLINode,
                                          CElement  *pElementFL,
                                          CTreeNode **ppNodeLIPrevValid
                                         );
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);
    
protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CLIElement);
};

#pragma INCMSG("--- End 'eli.hxx'")
#else
#pragma INCMSG("*** Dup 'eli.hxx'")
#endif
