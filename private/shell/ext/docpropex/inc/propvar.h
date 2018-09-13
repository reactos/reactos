//-------------------------------------------------------------------------//
//
//  PropVar.h
//
//-------------------------------------------------------------------------//

#ifndef __PROPVAR_H__
#define __PROPVAR_H__

#include "PTServer.h"

//-------------------------------------------------------------------------//
enum PROPVARIANT_ALLOCTYPE
{
    PVAT_NOALLOC,
    PVAT_SYSALLOC,
    PVAT_STRINGALLOC,
    PVAT_COMPLEX,
    PVAT_UNKNOWN,
} ;

//  PropVariantCompare flags
#define PVCF_IGNORECASE     0x00000001
#define PVCF_IGNORETIME     0x00000002
#define PVCF_IGNOREDATE     0x00000004

HRESULT  PropVariantSize( IN const PROPVARIANT&, OUT ULONG& cbSize, OUT OPTIONAL PROPVARIANT_ALLOCTYPE* = NULL ) ;
int      PropVariantCompare( IN const PROPVARIANT& var1, IN const PROPVARIANT& var2, ULONG uFlags ) ;
HRESULT  PropVariantHash( IN const PROPVARIANT& var, OUT ULONG& hash ) ;

HRESULT  PropVariantWriteRegistry( HKEY hKey, LPCTSTR pszValueName, IN const PROPVARIANT& var ) ;
HRESULT  PropVariantReadRegistry( HKEY hKey, LPCTSTR pszValueName, OUT PROPVARIANT& var ) ;

HRESULT  PropVariantWriteMem( IN const PROPVARIANT& var, OUT BYTE* pBuf, IN ULONG cbBuf ) ;
HRESULT  PropVariantReadMem( IN BYTE* pBuf, OUT PROPVARIANT& var ) ;
HRESULT  PropVariantToVariant( IN const PROPVARIANT* pvarSrc, OUT VARIANT* pvarDst ) ;

HRESULT  PropVariantToBstr( const PROPVARIANT* pvar, UINT nCodePage, ULONG dwFlags, LPCTSTR pszFmt, OUT BSTR* pbstrText ) ;
HRESULT  PropVariantFromBstr( IN const BSTR bstrText, UINT nCodePage, ULONG dwFlags, LPCTSTR pszFmt, OUT PROPVARIANT* pvar ) ;
HRESULT  PropVariantToString( const PROPVARIANT* pvar, UINT nCodePage, ULONG dwFlags, LPCTSTR pszFmt, OUT LPTSTR pszText, IN ULONG cchTextMax ) ;
HRESULT  PropVariantFromString( IN LPCTSTR pszText, UINT nCodePage, ULONG dwFlags, LPCTSTR pszFmt, OUT PROPVARIANT* pvar ) ;

//  nullifies time fields in a date/time structures:
void     SystemTimeMakeTimeless( SYSTEMTIME* pst );
HRESULT  PropVariantMakeTimeless( PROPVARIANT* pvar );

//-------------------------------------------------------------------------//
typedef class CPropVariant : public PROPVARIANT
//-------------------------------------------------------------------------//
{
//  Constructors
public:
    CPropVariant() ;
    CPropVariant( const CPropVariant& varSrc ) ; // not defined by base
    CPropVariant( const PROPVARIANT& varSrc ) ;  // not defined by base

    CPropVariant( const BYTE&, VARTYPE vt = VT_UI1 ) ;
    CPropVariant( const SHORT& ) ;
    CPropVariant( const USHORT& ) ;
    CPropVariant( const bool& ) ;
    CPropVariant( const LONG&, VARTYPE vt = VT_I4 ) ;
    CPropVariant( const ULONG&, VARTYPE vt = VT_UI4 ) ;
    CPropVariant( const FLOAT& ) ;
    CPropVariant( const LARGE_INTEGER& ) ;
    CPropVariant( const ULARGE_INTEGER& ) ;
    CPropVariant( const DOUBLE&, VARTYPE vt = VT_R8 /*or VT_DATE*/ ) ;
    CPropVariant( const CY& ) ;
    CPropVariant( const DATE& ) ;
    CPropVariant( const FILETIME& ) ;
    CPropVariant( const CLSID& ) ;
    CPropVariant( const BLOB& ) ;
    CPropVariant( const CLIPDATA& ) ;
    CPropVariant( IStream* ) ;
    CPropVariant( IStorage* ) ;
    CPropVariant( const BSTR& ) ;
    CPropVariant( const LPSTR ) ;
    CPropVariant( const LPWSTR ) ;
    CPropVariant( const CAUB& ) ;
    CPropVariant( const CAI& ) ;
    CPropVariant( const CAUI& ) ;
    CPropVariant( const CABOOL& ) ;
    CPropVariant( const CAL& ) ;
    CPropVariant( const CAUL& ) ;
    CPropVariant( const CAFLT& ) ;
    CPropVariant( const CASCODE& ) ;
    CPropVariant( const CAH& ) ;
    CPropVariant( const CAUH& ) ;
    CPropVariant( const CADBL& ) ;
    CPropVariant( const CACY& ) ;
    CPropVariant( const CADATE& ) ;
    CPropVariant( const CAFILETIME& ) ;
    CPropVariant( const CACLSID& ) ;
    CPropVariant( const CACLIPDATA& ) ;
    CPropVariant( const CABSTR& ) ;
    CPropVariant( const CALPSTR& ) ;
    CPropVariant( const CALPWSTR& ) ;
    CPropVariant( const CAPROPVARIANT& ) ;

//  Destructor
public:
    virtual ~CPropVariant() ;

//  Attach/Detach.
    void Attach( IN OUT PROPVARIANT* pSrc ) ;
    void Detach( OUT PROPVARIANT* pDest ) ;


// Assignment Operators
public:
    CPropVariant& operator=( const CPropVariant& ) ;
    CPropVariant& operator=( const PROPVARIANT& ) ;
    CPropVariant& operator=( const BYTE& ) ;
    CPropVariant& operator=( const SHORT& ) ;
    CPropVariant& operator=( const USHORT& ) ;
    CPropVariant& operator=( const bool& ) ;
    CPropVariant& operator=( const LONG& ) ;
    CPropVariant& operator=( const ULONG& ) ;
    CPropVariant& operator=( const FLOAT& ) ;
    CPropVariant& operator=( const LARGE_INTEGER& ) ;
    CPropVariant& operator=( const ULARGE_INTEGER& ) ;
    CPropVariant& operator=( const DOUBLE& ) ;
    CPropVariant& operator=( const CY& ) ;
    CPropVariant& operator=( const FILETIME& ) ;
    CPropVariant& operator=( const CLSID& ) ;
    CPropVariant& operator=( const BLOB& ) ;
    CPropVariant& operator=( const CLIPDATA& ) ;
    CPropVariant& operator=( IStream* ) ;
    CPropVariant& operator=( IStorage* ) ;
    CPropVariant& operator=( const BSTR& ) ;
    CPropVariant& operator=( const LPSTR ) ;
    CPropVariant& operator=( const LPWSTR ) ;
    CPropVariant& operator=( const CAUB& ) ;
    CPropVariant& operator=( const CAI& ) ;
    CPropVariant& operator=( const CAUI& ) ;
    CPropVariant& operator=( const CABOOL& ) ;
    CPropVariant& operator=( const CAL& ) ;
    CPropVariant& operator=( const CAUL& ) ;
    CPropVariant& operator=( const CAFLT& ) ;
    CPropVariant& operator=( const CASCODE& ) ;
    CPropVariant& operator=( const CAH& ) ;
    CPropVariant& operator=( const CAUH& ) ;
    CPropVariant& operator=( const CADBL& ) ;
    CPropVariant& operator=( const CACY& ) ;
    CPropVariant& operator=( const CADATE& ) ;
    CPropVariant& operator=( const CAFILETIME& ) ;
    CPropVariant& operator=( const CACLSID& ) ;
    CPropVariant& operator=( const CACLIPDATA& ) ;
    CPropVariant& operator=( const CABSTR& ) ;
    CPropVariant& operator=( const CALPSTR& ) ;
    CPropVariant& operator=( const CALPWSTR& ) ;
    CPropVariant& operator=( const CAPROPVARIANT& ) ;

public:
    VARTYPE Type() const    { return vt ; }
    void    SetType( VARTYPE vtNew ) ;

    //  Retrieves a display string for the value.  The caller
    //  is responsible for freeing the string with SysFreeString().
    virtual HRESULT GetDisplayText( OUT BSTR& bstrText, IN OPTIONAL LPCTSTR pszFmt = NULL, IN OPTIONAL ULONG dwFlags = 0L ) const ;

    //  Copies the variant's display string to the caller-provided buffer.
    virtual HRESULT GetDisplayText( OUT LPTSTR pszText, IN int cchText, IN OPTIONAL LPCTSTR pszFmt = NULL, IN OPTIONAL ULONG dwFlags = 0L ) const ;

    //  Updates the variant's value given a display string
    HRESULT AssignFromDisplayText( IN const BSTR bstrText, IN OPTIONAL LPCTSTR pszFmt = NULL ) ;
    HRESULT AssignFromDisplayText( IN LPCTSTR pszText, IN OPTIONAL LPCTSTR pszFmt = NULL ) ;

    //  Comparison methods
    int     Compare( const PROPVARIANT& varOther, ULONG uFlags ) const ;
    int     CompareText( const CPropVariant& varOther, ULONG uFlags, IN OPTIONAL LPCTSTR pszFmt = NULL, IN OPTIONAL ULONG dwFlags = 0L ) const ;

    //  Registry functions
    HRESULT WriteValue( HKEY hKey, LPCTSTR pszValueName ) const ;
    HRESULT ReadValue( HKEY hKey, LPCTSTR pszValueName ) ;

    //  Hashing methods
    ULONG   Hash() const ;
    
    //  Release resources and reinitialize.
    void    Clear() ;

protected:
    //  Implementation
    void    CommonConstruct() ;
    PVOID   AllocAndCopy( const VOID* pvSrc, DWORD cb ) const ;
    BSTR    AllocString( LPCWSTR src ) const ;
    BSTR    AllocStringLen( LPCWSTR src, UINT cch ) const ;

} *PCOMPROPVARIANT, *LPCOMPROPVARIANT ;

//-------------------------------------------------------------------------//
//  CPropVariant specialization with built-in maintenance of display text.
typedef class CDisplayPropVariant : public CPropVariant
//-------------------------------------------------------------------------//
{
public:
                CDisplayPropVariant() ;
                CDisplayPropVariant( const PROPVARIANT& p ) ;
    virtual     ~CDisplayPropVariant() ;
    
    HRESULT     CreateDisplayText( IN OPTIONAL LPCTSTR pszFmt = NULL, IN OPTIONAL ULONG dwFlags = 0L ) ;
    void        SetDisplayTextT( IN LPCTSTR pszDisplay ) ;
    void        SetDisplayText( IN BSTR bstrDisplay ) ;
    const BSTR  DisplayText() const ;

protected:
    BSTR        m_bstrDisplay ;
} *PDISPLAYPROPVARIANT, *LPPDISPLAYPROPVARIANT ;

//-------------------------------------------------------------------------//
//  C++ wrap for property unique identifier
class CPropertyUID : public tagPUID
//-------------------------------------------------------------------------//
{
public:
    //  Construction
    CPropertyUID() ;
    CPropertyUID( const struct tagPUID& src ) ;
    
    //  Assignment
    void Set( REFFMTID _fmtid, PROPID _propid, VARTYPE _vt ) ;
    const struct tagPUID& operator= ( const struct tagPUID& src ) ;

    //  Comparison
    BOOL operator==( const struct tagPUID& other ) const ;
    BOOL operator!=( const struct tagPUID& other ) const ;

    //  Hashing
    ULONG Hash() const ;
} ;

//-------------------------------------------------------------------------//
//  Inline implementation, class CPropVariant
inline void CPropVariant::CommonConstruct()    {
    ::PropVariantInit( this ) ;
}
inline CPropVariant::CPropVariant()    {
    CommonConstruct() ;
}
inline CPropVariant::CPropVariant( const CPropVariant& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const PROPVARIANT& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const BYTE& src, VARTYPE vtSrc )   {
    CommonConstruct() ;
    operator=(src) ;
    vt = vtSrc ;
}
inline CPropVariant::CPropVariant( const SHORT& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const USHORT& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const bool& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const LONG& src, VARTYPE vtSrc )   {
    CommonConstruct() ;
    operator=(src) ;
    vt = vtSrc ;
}
inline CPropVariant::CPropVariant( const ULONG& src, VARTYPE vtSrc )   {
    CommonConstruct() ;
    operator=(src) ;
    vt = vtSrc ;
}
inline CPropVariant::CPropVariant( const FLOAT& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const LARGE_INTEGER& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const ULARGE_INTEGER& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const DOUBLE& src, VARTYPE vtSrc )   {
    CommonConstruct() ;
    operator=(src) ;
    vt = vtSrc ;
}
inline CPropVariant::CPropVariant( const CY& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const DATE& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const FILETIME& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CLSID& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const BLOB& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CLIPDATA& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( IStream* pSrc )   {
    CommonConstruct() ;
    operator=(pSrc) ;
}
inline CPropVariant::CPropVariant( IStorage* pSrc )   {
    CommonConstruct() ;
    operator=(pSrc) ;
}
inline CPropVariant::CPropVariant( const BSTR& src )   {
    CommonConstruct() ;
    operator=((BSTR)src) ;
}
inline CPropVariant::CPropVariant( const LPSTR src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const LPWSTR src )    {
    CommonConstruct() ;
    operator=((LPWSTR)src) ;
}
inline CPropVariant::CPropVariant( const CAUB& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CAI& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CAUI& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CABOOL& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CAL& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CAUL& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CAFLT& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CASCODE& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CAH& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CAUH& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CADBL& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CACY& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CADATE& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CAFILETIME& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CACLSID& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CACLIPDATA& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CABSTR& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CALPSTR& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CALPWSTR& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::CPropVariant( const CAPROPVARIANT& src )   {
    CommonConstruct() ;
    operator=(src) ;
}
inline CPropVariant::~CPropVariant()   {
    Clear() ;
}
inline void CPropVariant::SetType( VARTYPE vtNew ){
    vt = vtNew ;
}
inline BSTR CPropVariant::AllocString( LPCWSTR src ) const  {
    return SysAllocString( src ) ;

}
inline BSTR CPropVariant::AllocStringLen( LPCWSTR src, UINT cch ) const {
    return SysAllocStringLen( src, cch ) ;

}
inline PVOID CPropVariant::AllocAndCopy( const VOID* pvSrc, DWORD cb ) const    {
    PVOID pvDest ;
    if( (pvDest = CoTaskMemAlloc( cb ))!=NULL )
        memcpy( pvDest, pvSrc, cb ) ;
    return pvDest ;
}
inline void  CPropVariant::Clear()   {
    ::PropVariantClear( this ) ;
    ::PropVariantInit( this ) ;
}
inline int CPropVariant::Compare( const PROPVARIANT& varOther, ULONG uFlags ) const {
    return ::PropVariantCompare( *this, varOther, uFlags ) ;
}
inline ULONG CPropVariant::Hash() const {
    ULONG dwHash ;
    if( SUCCEEDED( ::PropVariantHash( *this, dwHash ) ) )
        return dwHash ;
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Inline implementation, class CDisplayPropVariant : public CPropVariant
inline CDisplayPropVariant::CDisplayPropVariant() 
    :   m_bstrDisplay(NULL) {
}
inline CDisplayPropVariant::CDisplayPropVariant( const PROPVARIANT& p ) 
    :   CPropVariant( p ), m_bstrDisplay(NULL) {
}
inline CDisplayPropVariant::~CDisplayPropVariant() { 
    SetDisplayText(NULL) ; 
}
inline const BSTR CDisplayPropVariant::DisplayText() const { 
    return m_bstrDisplay ;
}

//-------------------------------------------------------------------------//
//  Inline implementation, class CPropertyUID
inline CPropertyUID::CPropertyUID() {
    propid  = 0 ;
    vt      = 0 ;
    memset( &fmtid, 0, sizeof(fmtid) ) ;
}
inline CPropertyUID::CPropertyUID( const struct tagPUID& src )  {
    operator=( src ) ;
}
inline void CPropertyUID::Set( REFFMTID _fmtid, PROPID _propid, VARTYPE _vt )    {
    fmtid  = _fmtid ;
    propid = _propid ;
    vt     = _vt ;
}
inline const struct tagPUID& CPropertyUID::operator = ( const struct tagPUID& src )  {   
    if( &src==this ) return *this ;
    fmtid = src.fmtid ; propid = src.propid ; vt = src.vt ;
    return *this ;
}
inline BOOL CPropertyUID::operator==( const struct tagPUID& other ) const  {
    return propid == other.propid && 
           memcmp( &fmtid, &other.fmtid, sizeof(fmtid) )==0 ;
}
inline BOOL CPropertyUID::operator!=( const struct tagPUID& other ) const  {
    return !operator==(other) ;
}


#endif __PROPVAR_H__