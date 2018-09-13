//-------------------------------------------------------------------------//
//  Enum.h
//-------------------------------------------------------------------------//
#ifndef __PTENUM_H__
#define __PTENUM_H__

class CPropertySource ;
class CPropMruStor ;
#include "resource.h"       // resource symbols
#include "dictbase.h"
#include "defprop.h"
#include "propvar.h"

//-------------------------------------------------------------------------//
#define DECLARE_ENUM_UNKNOWN() \
    public: \
    STDMETHOD( QueryInterface )( REFIID, void** ) ; \
    STDMETHOD_( ULONG, AddRef )() ; \
    STDMETHOD_( ULONG, Release )() ; \
    private: ULONG   m_cRef ; static ULONG    m_cObj ;
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// CEnumFolderItem
class CEnumFolderItem : public IEnumPROPFOLDERITEM
//-------------------------------------------------------------------------//
{
    DECLARE_ENUM_UNKNOWN() ;

public:
	CEnumFolderItem( CPropertySource* pSrc ) ;

    HRESULT Create( ULONG dwSrcDescr, const VARIANT& varSrc ) ;
    const CEnumFolderItem& operator=( const CEnumFolderItem& src ) ;


    // IEnumPROPFOLDERITEM methods:
public:
	STDMETHOD( Next ) (/*[in]*/ ULONG cItems, /*[out]*/ PROPFOLDERITEM * rgFolderItem, /*[out]*/ ULONG* pcItemsFetched ) ;
	STDMETHOD( Skip ) (/*[in]*/ ULONG cItems )  { return E_NOTIMPL ; }
	STDMETHOD( Reset )( ) ;
	STDMETHOD( Clone )(/*[out]*/ IEnumPROPFOLDERITEM ** ppEnum ) { return E_NOTIMPL ; }

protected:
    CPropertySource*     m_pSrc ;
    IEnumPROPFOLDERITEM* m_pEnumSpecialized ;
    LONG                 m_iPos,
                         m_cMax ;
};

extern const int nProps ;

//-------------------------------------------------------------------------//
// CEnumPropertyItem
class CEnumPropertyItem : public IEnumPROPERTYITEM
//-------------------------------------------------------------------------//
{
    DECLARE_ENUM_UNKNOWN() ;

public:
	CEnumPropertyItem( CPropertySource* pSrc, const PROPFOLDERITEM *pFolder ) ;
    virtual ~CEnumPropertyItem() ;
    const CEnumPropertyItem& operator= ( const CEnumPropertyItem& src ) ;

    // IEnumPROPERTYITEM methods.
public:
	STDMETHOD( Next ) (/*[in]*/ ULONG cItems, /*[out]*/ PROPERTYITEM * rgPropertyItem, /*[out]*/ ULONG* pcItemsFetched ) ;
	STDMETHOD( Skip ) (/*[in]*/ ULONG cItems ) { return E_NOTIMPL ; }
	STDMETHOD( Reset )( ) ;
	STDMETHOD( Clone )(/*[out]*/ IEnumPROPERTYITEM ** ppEnum ) { return E_NOTIMPL ; }

protected:
    BOOL    Iterate( OUT PROPERTYITEM& propitem ) ;
    CPropertySource* m_pSrc ;
    IEnumPROPERTYITEM* m_pEnumSpecialized ;
    BOOL               m_bEnumSpecialized ;
    HANDLE             m_hIterator ;
    PFID               m_pfid ; // folder ID
};

//-------------------------------------------------------------------------//
// CEnumHardValues
class CEnumHardValues : public IEnumPROPVARIANT_DISPLAY
//-------------------------------------------------------------------------//
{
    DECLARE_ENUM_UNKNOWN() ;

public:
	CEnumHardValues( const DEFVAL* pVals, int cVals ) ;
    ~CEnumHardValues() ;
    const CEnumHardValues& operator= ( const CEnumHardValues& src ) ;

    // IEnumPROPVARIANT_DISPLAY methods
public:
	STDMETHOD( Next  )(/*[in]*/ ULONG celt, /*[out, size_is(celt), length_is(*pCeltFetched)]*/ PROPVARIANT_DISPLAY* rgVar, /*[out]*/ ULONG *pCeltFetched);
	STDMETHOD( Skip  )(/*[in]*/ ULONG celt ) { return E_NOTIMPL ; }
	STDMETHOD( Reset )();
	STDMETHOD( Clone )(/*[out]*/ IEnumPROPVARIANT_DISPLAY ** ppEnum) { return E_NOTIMPL ; }

protected:    
    LONG              m_iPos,
                      m_cMax ;
    const DEFVAL*     m_pVals ;
} ;

//-------------------------------------------------------------------------//
// CEnumMruValues
class CEnumMruValues : public IEnumPROPVARIANT_DISPLAY
//-------------------------------------------------------------------------//
{
    DECLARE_ENUM_UNKNOWN() ;

public:
	CEnumMruValues( const tagPUID& puid ) ;
    ~CEnumMruValues() {}
    const CEnumMruValues& operator= ( const CEnumMruValues& src ) ;

    // IEnumPROPVARIANT_DISPLAY methods
public:
	STDMETHOD( Next  )(/*[in]*/ ULONG celt, /*[out, size_is(celt), length_is(*pCeltFetched)]*/ PROPVARIANT_DISPLAY* rgVar, /*[out]*/ ULONG *pCeltFetched);
	STDMETHOD( Skip  )(/*[in]*/ ULONG celt ) { return E_NOTIMPL ; }
	STDMETHOD( Reset )();
	STDMETHOD( Clone )(/*[out]*/ IEnumPROPVARIANT_DISPLAY ** ppEnum) { return E_NOTIMPL ; }

    //  Attributes
protected:
    CPropertyUID      m_puid ;
    LONG              m_iPos,
                      m_cMax ;
};

#endif __PTENUM_H__