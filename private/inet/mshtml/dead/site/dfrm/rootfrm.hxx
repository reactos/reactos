
//+---------------------------------------------------------------------------
//
//  Maintained by: istvanc
//
//  Copyright: (C) Microsoft Corporation, 1994-1995.
//
//  File:       rootfrm.hxx
//
//  Contents:   this file contains the CRootDataFrame class definition.
//
//----------------------------------------------------------------------------

#ifndef _ROOTFRM_HXX_
#   define _ROOTFRM_HXX_ 1

#   ifndef _DATAFRM_HXX_
#       include "datafrm.hxx"
#   endif


class CRootDataFrame : public CDataFrameTemplate
{
typedef CDataFrameTemplate super;

public:
    CRootDataFrame(CDoc * pDoc, CSite * pParent);

    void Detach ();

    HRESULT CreateInstance (CDoc * pDoc,
                               CSite * pParent,
                               CSite **ppFrame,
                               CCreateInfo * pcinfo);
    HRESULT BuildDetail (CDataFrame * pNewInstance, CCreateInfo * pcinfo);

    void DeleteInstances();

    HRESULT Generate (IN CRectl& boundRect, BOOL fAfterLoad=FALSE);

    void ClearSelection(BOOL fResetInstances);

    HRESULT Notify(SITE_NOTIFICATION, DWORD);

#ifdef PRODUCT_97
    virtual HRESULT BUGCALL AfterLoad(DWORD dw);
#endif

#if DBG == 1
    virtual HRESULT SetProposed(CSite * pSite, const CRectl * prcl);
    virtual HRESULT GetProposed(CSite * pSite, CRectl * prcl);
#endif

    virtual CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc;}
    virtual HRESULT UpdatePropertyChanges(UPDATEPROPS updFlag=UpdatePropsPrepareTemplates);
    virtual HRESULT InitSubFrameInstance(CBaseFrame **pNewInstance, CBaseFrame *pTemplate, CCreateInfo * pcinfo);

protected:

#if defined(PRODUCT_97)
    // Allow access to the chapter
    const CDataLayerChapter& getGroupID() const
    {
        return _groupID;
    };
#endif // defined(PRODUCT_97)

    CRectl _rclPropose;

    static CLASSDESC   s_classdesc;

    static const CLSID *s_apclsidPages[];
};


#endif _ROOTFRM_HXX_
