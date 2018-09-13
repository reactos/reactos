//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       txtslave.hxx
//
//  Contents:   CTxtSlave is the class for thegeneric 'slave' element that can
//              hold the content for any scopeless element.
//
//----------------------------------------------------------------------------

#ifndef I_TXTSLAVE_HXX_
#define I_TXTSLAVE_HXX_
#pragma INCMSG("--- Beg 'inputinr.hxx'")

//+---------------------------------------------------------------------------
//
// CTxtSlave
//
//----------------------------------------------------------------------------

class CTxtSlave : public CElement
{
private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))
    DECLARE_CLASS_TYPES(CTxtSlave, CElement)
public:
    CTxtSlave(ELEMENT_TAG etag, CDoc *pDoc)
       : super(etag, pDoc)
    {
    }

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Init2(CInit2Context * pContext);
    virtual BOOL    IsEnabled() { return MarkupMaster()->IsEnabled(); }

    virtual HRESULT ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget);
    virtual DWORD   GetBorderInfo(  CDocInfo * pdci,
                                    CBorderInfo *pborderinfo,
                                    BOOL fAll);

    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage)
    {
        // Kill the message if the master is dead. That can happen
        // if an event handler nuked the master (#30760)
        if (!IsInMarkup() || !MarkupMaster())
            return S_OK;

        // super will pass to the master
        return super::HandleMessage(pMessage);
    }       

    HRESULT STDMETHODCALLTYPE QueryStatus(
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext)
    {
        if (IsInMarkup())
        {
            CElement * pElemMaster = MarkupMaster();

            if (pElemMaster)
            {
                RRETURN_NOTRACE(pElemMaster->QueryStatus(
                    pguidCmdGroup,
                    cCmds,
                    rgCmds,
                    pcmdtext));
            }
        }

        RRETURN_NOTRACE(super::QueryStatus(
            pguidCmdGroup,
            cCmds,
            rgCmds,
            pcmdtext));
    }

    HRESULT STDMETHODCALLTYPE Exec(
            GUID * pguidCmdGroup,
            DWORD  nCmdID,
            DWORD  nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut)
    {
        if (IsInMarkup())
        {
            CElement * pElemMaster = MarkupMaster();

            if (pElemMaster)
            {
                RRETURN_NOTRACE(pElemMaster->Exec(
                        pguidCmdGroup,
                        nCmdID,
                        nCmdexecopt,
                        pvarargIn,
                        pvarargOut));
            }
        }
        RRETURN_NOTRACE(super::Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut));
    }

    virtual HRESULT CreateLayout() { return S_OK; /* no layout! */ }

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:

};

#pragma INCMSG("--- End 'txtslave.hxx'")
#else
#pragma INCMSG("*** Dup 'txtslave.hxx'")
#endif
