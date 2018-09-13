//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       propchg.hxx
//
//  Contents:   defines the propnotification holder class
//
//  Classes:    CPropertyChange
//
//  Maintained by Frankman
//
//----------------------------------------------------------------------------


#ifndef _PROPCHG_HXX_
#   define _PROPCHG_HXX_    1


class CPropertyChange
{
public:
    // destruction and initialisation
    CPropertyChange();
    ~CPropertyChange();
    virtual void Passivate(void);

public:
    // operations for manipulating members
    HRESULT AddDispID(DISPID dispid);

    BOOL IsDirty(void) { return ((BOOL) (_DispidArray.Size()  ||
                                         _fDataSourceModified ||
                                         _fRegenerate         ||
                                         _fCreateToFit              )); }

    BOOL IsTemplateModified(void) {return (BOOL) _fTemplateModified; }

    void SetDataSourceModified(BOOL f) { _fDataSourceModified = f;};
    BOOL IsDataSourceModified(void) {return (BOOL) _fDataSourceModified;};

    void SetRegenerate(BOOL f) { _fRegenerate = f; };
    BOOL NeedRegenerate(void) { return (BOOL) _fRegenerate;};

    void SetCreateToFit(BOOL f) { _fCreateToFit = f; };
    BOOL NeedCreateToFit(void) { return (BOOL) _fCreateToFit;};

    void SetLinkInfoChanged(BOOL f) { _fLinkInfoChanged = f; };
    BOOL IsLinkInfoChanged(void) { return (BOOL) _fLinkInfoChanged; };

    void SetNeedToScroll (BOOL f) { _fScroll = f; };
    BOOL NeedToScroll (void) { return (BOOL) _fScroll;};

    void SetNeedToInvalidate (BOOL f) { _fInvalidate = f; };
    BOOL NeedToInvalidate (void) { return (BOOL) _fInvalidate;};

    void SetNeedGridResize(BOOL f) { _fNeedGridResize = f; }
    BOOL NeedGridResize (void) { return (BOOL) _fNeedGridResize ;};


public:
    // enum operations
    virtual void EnumReset(void);
    BOOL    EnumNextDispID(DISPID *pDispid);

protected:
    // property change notification members
    unsigned            _fTemplateModified :1;  // indicates that someone added/deleted controls
    unsigned            _fDataSourceModified :1;// indicates that someone modified the datasource
    unsigned            _fRegenerate:1;         // indicates regeneration is needed
    unsigned            _fCreateToFit:1;        // indicates create to fit is needed
    unsigned            _fLinkInfoChanged:1;    // indicates that either linkmaster or linkchild changed
    unsigned            _fScroll:1;             // indicates that OnScroll is needed.
    unsigned            _fInvalidate:1;         // indicates that Invalidate is needed.
    unsigned            _fNeedGridResize:1;     // indication for the root for pass gridresize flags
    CDataAry<DISPID>    _DispidArray;           // array of DISPIDs of the props that changed
    int                 _iCurrentDispid;

};





class CControlPropertyChange : public CPropertyChange
{
public:
    // destruction and initialisation
    CControlPropertyChange();
    virtual void Passivate(void);

public:
    // operations for manipulating members
    HRESULT AddDispIDAndVariant(DISPID dispid, VARIANT *pvar);

public:
    virtual void EnumReset(void);
    BOOL EnumNextVariant(VARIANT **ppvar);


protected:
    CPtrAry<VARIANT *>  _VariantArray;      // array of variants, if the control does not provide IControlInstance
    int                 _iCurrentVariant;

};




#endif


