//-------------------------------------------------------------------------//
//
//  PropVar.cpp
//
//-------------------------------------------------------------------------//
#include "pch.h"
#include <stdio.h>
#include "propVar.h"

//-------------------------------------------------------------------------//
static LPCWSTR clipformats[] = 
{
    L"CF_TEXT",           // 1
    L"CF_BITMAP",         // 2
    L"CF_METAFILEPICT",   // 3
    L"CF_SYLK",           // 4
    L"CF_DIF",            // 5
    L"CF_TIFF",           // 6
    L"CF_OEMTEXT",        // 7
    L"CF_DIB",            // 8
    L"CF_PALETTE",        // 9
    L"CF_PENDATA",        // 10
    L"CF_RIFF",           // 11
    L"CF_WAVE",           // 12
    L"CF_UNICODETEXT",    // 13
    L"CF_ENHMETAFILE",    // 14
    L"CF_HDROP",          // 15
    L"CF_LOCALE",         // 16
} ;
#define NUM_CLIPFORMATS sizeof(clipformats)/sizeof(LPCWSTR)

//-------------------------------------------------------------------------//
//  Global PROPVARARIANT utility functions
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  Determins the size of the data contained by a PROPVARIANT instance.
//  The returned value does not included the size of the PROPVARIANT structure
//  itself.
HRESULT PropVariantSize( 
    IN const PROPVARIANT& var, 
    OUT ULONG& cbDataSize, 
    OUT OPTIONAL PROPVARIANT_ALLOCTYPE* pdwAllocType )
{
    HRESULT hr = S_OK ;
    ULONG   i ;
    
    cbDataSize  = 0 ;
    
    PROPVARIANT_ALLOCTYPE dwAllocType ;
    if( !pdwAllocType ) pdwAllocType = &dwAllocType ;
    *pdwAllocType = PVAT_NOALLOC ;

    #define PROPVARIANT_VECTOR_SIZE( member ) \
            (sizeof(var.member) + (var.member.cElems * sizeof(*var.member.pElems)))
    
    switch( var.vt )
    {
        case VT_EMPTY:
        case VT_NULL:
        case VT_ILLEGAL:
            break ;
                        
        case VT_UI1:
            cbDataSize = sizeof(var.bVal) ;
            break ;

        case VT_I2:
            cbDataSize = sizeof(var.iVal) ;
            break ;

        case VT_UI2:
            cbDataSize = sizeof(var.uiVal) ;
            break ;
            
        case VT_BOOL:
            cbDataSize = sizeof(var.boolVal) ;
            break ;

        case VT_I4:
            cbDataSize = sizeof(var.lVal) ;
            break ;
 
        case VT_UI4:
            cbDataSize = sizeof(var.ulVal) ;
            break ;

        case VT_R4:
            cbDataSize = sizeof(var.fltVal) ;
            break ;

        case VT_ERROR:
            cbDataSize = sizeof(var.scode) ;
            break ;

        case VT_I8:
            cbDataSize = sizeof(var.hVal) ;
            break ;

        case VT_UI8:
            cbDataSize = sizeof(var.uhVal) ;

        case VT_R8:
            cbDataSize = sizeof(var.dblVal) ;
            break ;

        case VT_CY:
            cbDataSize = sizeof(var.cyVal) ;
            break ;

        case VT_DATE:
            cbDataSize = sizeof(var.date) ;
            break ;

        case VT_FILETIME:
            cbDataSize = sizeof(var.filetime) ;
            break ;
        
        case VT_CLSID:
            cbDataSize = var.puuid ? sizeof(*var.puuid) : 0 ;
            *pdwAllocType = PVAT_SYSALLOC ;
            break ;

        case VT_BLOB:
            cbDataSize = sizeof(BLOB) + var.blob.cbSize ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_CF :
            cbDataSize = var.pclipdata ? sizeof(CLIPDATA) + CBPCLIPDATA( *var.pclipdata ) : 0 ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_BSTR:
            cbDataSize  = var.bstrVal ? (lstrlenW( var.bstrVal ) + 1) * sizeof(WCHAR) : 0 ;
            *pdwAllocType = PVAT_STRINGALLOC ;
            break ;

        case VT_LPSTR:
            cbDataSize  = var.pszVal ? (lstrlenA( var.pszVal ) + 1) * sizeof(CHAR) : 0 ;
            *pdwAllocType = PVAT_SYSALLOC ;
            break ;
            
        case VT_LPWSTR:
            cbDataSize  = var.pwszVal ? (lstrlenW( var.pwszVal ) + 1) * sizeof(WCHAR) : 0 ;
            *pdwAllocType = PVAT_SYSALLOC ;
            break ;

        case VT_VECTOR | VT_UI1:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( caub ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_I2:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cai ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_UI2:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( caui ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_I4:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cal ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_UI4:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( caul ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_I8:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cah ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_UI8:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cauh ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_R4:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( caflt ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_R8:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cadbl ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_CY:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cacy ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_DATE:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cadate ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_BOOL:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cabool ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_ERROR:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cascode ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_FILETIME:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cafiletime ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_CLSID:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cauuid ) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_BSTR:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( cabstr ) ;
            for( i=0; i < var.cabstr.cElems; i++ )
                cbDataSize += ((var.cabstr.pElems[i]==NULL) ? lstrlenW( var.cabstr.pElems[i] ) + 1 : 0) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_LPSTR:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( calpstr ) ;
            for( i=0; i < var.calpstr.cElems; i++ )
                cbDataSize += ((var.calpstr.pElems[i]==NULL) ? lstrlenA( var.calpstr.pElems[i] ) + 1 : 0) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_LPWSTR:
            cbDataSize = PROPVARIANT_VECTOR_SIZE( calpwstr ) ;
            for( i=0; i < var.calpwstr.cElems; i++ )
                cbDataSize += ((var.calpwstr.pElems[i]==NULL) ? lstrlenW( var.calpwstr.pElems[i] ) + 1 : 0) ;
            *pdwAllocType = PVAT_COMPLEX ;
            break ;

        case VT_VECTOR | VT_CF:
            //caclipdata; 
        case VT_VECTOR | VT_VARIANT:
            //capropvar;  
            *pdwAllocType = PVAT_COMPLEX ;
            hr = E_NOTIMPL ;
            break ;

        case VT_STREAM:
        case VT_STORAGE:
            *pdwAllocType = PVAT_SYSALLOC ;
            hr = E_NOTIMPL ;
            break ;

        default:
            return E_UNEXPECTED ;
    }

    return hr ;
}

//-------------------------------------------------------------------------//
//  Writes the data contained by a PROPVARIANT instance to single, contiguous 
//  memory buffer.
HRESULT PropVariantWriteMem( IN const PROPVARIANT& var, OUT BYTE* pBuf, IN ULONG cbBuf )
{
    HRESULT hr ;
    
    #define PROPVARIANT_TYPECAST_COPY( buf, cbBuf, type, member ) \
                { if( cbBuf < sizeof(type) ) return E_OUTOFMEMORY ; \
                  (*((type*)buf)) = var.member ; \
                  buf   += sizeof(var.member) ; \
                  cbBuf -= sizeof(var.member) ; } 

    ULONG   cbDataSize = 0, 
            cb ;            // scratch
    
    if( !SUCCEEDED( (hr = PropVariantSize( var, cbDataSize, NULL )) ) )
        return hr ;

    //  Ensure cbBuf is large enough to contain the data
    if( cbDataSize + sizeof(VARTYPE) > cbBuf )
    {
        ASSERT( FALSE ) ;
        return E_OUTOFMEMORY ;
    }

    //  Copy vartype
    PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, VARTYPE, vt ) ;
    hr = S_OK ;
    
    switch( var.vt )
    {
        case VT_EMPTY:
        case VT_NULL:
        case VT_ILLEGAL:
            break ;
                        
        case VT_UI1:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, UCHAR, bVal ) ;
            break ;

        case VT_I2:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, SHORT, iVal ) ;
            break ;

        case VT_UI2:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, USHORT, uiVal ) ;
            break ;
            
        case VT_BOOL:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, VARIANT_BOOL, boolVal ) ;
            break ;

        case VT_I4:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, long, lVal) ;
            break ;
 
        case VT_UI4:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, ULONG, ulVal) ;
            break ;

        case VT_R4:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, float, fltVal) ;
            break ;

        case VT_ERROR:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, SCODE, scode) ;
            break ;

        case VT_I8:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, LARGE_INTEGER, hVal) ;
            break ;

        case VT_UI8:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, ULARGE_INTEGER, uhVal) ;

        case VT_R8:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, double, dblVal) ;
            break ;

        case VT_CY:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, CY, cyVal) ;
            break ;

        case VT_DATE:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, DATE, date) ;
            break ;

        case VT_FILETIME:
            PROPVARIANT_TYPECAST_COPY( pBuf, cbBuf, FILETIME, filetime) ;
            break ;
        
        case VT_CLSID:
            cbBuf = min( cbBuf, sizeof(*var.puuid) ) ;
            memcpy( pBuf, var.puuid, cbBuf ) ;
            break ;

        case VT_BSTR:
            cb = min( cbBuf-1, lstrlenW( var.bstrVal )*sizeof(WCHAR) ) ;
            lstrcpynW( (WCHAR*)pBuf, var.bstrVal, cb+1 ) ;
            cbBuf -= (cb + 1) ;
            break ;

        case VT_LPSTR:
            cb = min( cbBuf-1, lstrlenA( var.pszVal )*sizeof(CHAR) ) ;
            lstrcpynA( (CHAR*)pBuf, var.pszVal, cb+1 ) ;
            cbBuf -= (cb + 1) ;
            break ;
            
        case VT_LPWSTR:
            cb = min( cbBuf-1, lstrlenW( var.pwszVal )*sizeof(WCHAR) ) ;
            lstrcpynW( (WCHAR*)pBuf, var.pwszVal, cb+1 ) ;
            cbBuf -= (cb + 1) ;
            break ;

        case VT_BLOB:
        case VT_CF :
        case VT_VECTOR | VT_UI1:
        case VT_VECTOR | VT_I2:
        case VT_VECTOR | VT_UI2:
        case VT_VECTOR | VT_I4:
        case VT_VECTOR | VT_UI4:
        case VT_VECTOR | VT_I8:
        case VT_VECTOR | VT_UI8:
        case VT_VECTOR | VT_R4:
        case VT_VECTOR | VT_R8:
        case VT_VECTOR | VT_CY:
        case VT_VECTOR | VT_DATE:
        case VT_VECTOR | VT_BOOL:
        case VT_VECTOR | VT_ERROR:
        case VT_VECTOR | VT_FILETIME:
        case VT_VECTOR | VT_BSTR:
        case VT_VECTOR | VT_LPSTR:
        case VT_VECTOR | VT_LPWSTR:
        case VT_VECTOR | VT_CLSID:
        case VT_VECTOR | VT_CF:
        case VT_VECTOR | VT_VARIANT:
        case VT_STREAM:
        case VT_STORAGE:
            hr = E_NOTIMPL ;
            break ;

        default:
            hr = E_UNEXPECTED ;
    }

    return hr ;
}

//-------------------------------------------------------------------------//
//  Reads the data contained by a PROPVARIANT instance to single, contiguous 
//  memory buffer.
HRESULT PropVariantReadMem( IN BYTE* pBuf, OUT PROPVARIANT& var )
{
    HRESULT     hr = S_OK ;
    PROPVARIANT varTmp ;
    
    PropVariantInit( &var ) ;
    PropVariantInit( &varTmp ) ;

    if( !pBuf ) return E_POINTER ;

    #define PROPVARIANT_TYPECAST_ASSIGN( buf, type, val ) \
        { varTmp.val = *((type*)buf) ; buf += sizeof(type) ; }

    //  Determine type
    PROPVARIANT_TYPECAST_ASSIGN( pBuf, VARTYPE, vt ) ;

    switch( varTmp.vt )
    {
        case VT_EMPTY:
        case VT_NULL:
        case VT_ILLEGAL:
            break ;
                        
        case VT_UI1:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, UCHAR, bVal ) ;
            break ;

        case VT_I2:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, SHORT, iVal ) ;
            break ;

        case VT_UI2:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, USHORT, uiVal ) ;
            break ;
            
        case VT_BOOL:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, VARIANT_BOOL, boolVal ) ;
            break ;

        case VT_I4:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, long, lVal) ;
            break ;
 
        case VT_UI4:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, ULONG, ulVal) ;
            break ;

        case VT_R4:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, float, fltVal) ;
            break ;

        case VT_ERROR:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, SCODE, scode) ;
            break ;

        case VT_I8:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, LARGE_INTEGER, hVal) ;
            break ;

        case VT_UI8:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, ULARGE_INTEGER, uhVal) ;

        case VT_R8:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, double, dblVal) ;
            break ;

        case VT_CY:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, CY, cyVal) ;
            break ;

        case VT_DATE:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, DATE, date) ;
            break ;

        case VT_FILETIME:
            PROPVARIANT_TYPECAST_ASSIGN( pBuf, FILETIME, filetime) ;
            break ;
        
        case VT_CLSID:
            varTmp.puuid = (CLSID*)pBuf ;
            break ;

        case VT_BSTR:
            varTmp.bstrVal = (BSTR)pBuf ;
            break ;

        case VT_LPSTR:
            varTmp.pszVal = (LPSTR)pBuf ;
            break ;
            
        case VT_LPWSTR:
            varTmp.pwszVal = (LPWSTR)pBuf ;
            break ;

        case VT_BLOB:
        case VT_CF :
        case VT_VECTOR | VT_UI1:
        case VT_VECTOR | VT_I2:
        case VT_VECTOR | VT_UI2:
        case VT_VECTOR | VT_I4:
        case VT_VECTOR | VT_UI4:
        case VT_VECTOR | VT_I8:
        case VT_VECTOR | VT_UI8:
        case VT_VECTOR | VT_R4:
        case VT_VECTOR | VT_R8:
        case VT_VECTOR | VT_CY:
        case VT_VECTOR | VT_DATE:
        case VT_VECTOR | VT_BOOL:
        case VT_VECTOR | VT_ERROR:
        case VT_VECTOR | VT_FILETIME:
        case VT_VECTOR | VT_BSTR:
        case VT_VECTOR | VT_LPSTR:
        case VT_VECTOR | VT_LPWSTR:
        case VT_VECTOR | VT_CLSID:
        case VT_VECTOR | VT_CF:
        case VT_VECTOR | VT_VARIANT:
        case VT_STREAM:
        case VT_STORAGE:
            hr = E_NOTIMPL ;
            break ;

        default:
            hr = E_UNEXPECTED ;
    }

    if( SUCCEEDED( hr ) )
        ::PropVariantCopy( &var, &varTmp ) ;

    return hr ;
}

//-------------------------------------------------------------------------//
HRESULT PropVariantToBstr( const PROPVARIANT* pvar, UINT nCodePage, ULONG dwFlags, LPCTSTR pszFmt, OUT BSTR* pbstrText )
{
    ASSERT( pvar ) ;
    ASSERT( pbstrText ) ;

    HRESULT     hr = E_FAIL ;
    LCID        lcid = GetUserDefaultLCID() ;
    const DWORD cchNumBuf   = 32 ;
    
    UNREFERENCED_PARAMETER( pszFmt ) ;
    USES_CONVERSION ;

    *pbstrText = NULL ;

    switch( pvar->vt )
    {
        case VT_EMPTY:
        case VT_NULL:
        case VT_ILLEGAL:
            *pbstrText = SysAllocString( L"" ) ;
            return S_OK ;   // return blank

        case VT_UI1:
            return VarBstrFromUI1( pvar->bVal, lcid, dwFlags, pbstrText ) ;

        case VT_I2:
            return VarBstrFromI2( pvar->iVal, lcid, dwFlags, pbstrText ) ;

        case VT_UI2:
            return VarBstrFromUI2(  pvar->uiVal, lcid, dwFlags, pbstrText );
            
        case VT_BOOL:
            return VarBstrFromBool( pvar->boolVal, lcid, dwFlags, pbstrText ) ;

        case VT_I4:
            return VarBstrFromI4( pvar->lVal, lcid, dwFlags, pbstrText ) ;
 
        case VT_UI4:
            return VarBstrFromUI4( pvar->ulVal, lcid, dwFlags, pbstrText );

        case VT_R4:
            return VarBstrFromR4( pvar->fltVal, lcid, dwFlags, pbstrText ) ;

        case VT_ERROR:
            return VarBstrFromUI4( pvar->scode, lcid, dwFlags, pbstrText );

        case VT_I8:
        case VT_UI8:
            if( (*pbstrText = SysAllocStringLen( NULL, cchNumBuf ))!=NULL )
            {
                if( pvar->vt==VT_I8 ? _i64tow( pvar->hVal.QuadPart, *pbstrText, 10 )!=NULL :
                                    _ui64tow( pvar->hVal.QuadPart, *pbstrText, 10 )!=NULL )
                    return TRUE ;
                SysFreeString( *pbstrText ) ;
                *pbstrText = NULL ;
            }
            break ; 

        case VT_R8:
            return VarBstrFromR8( pvar->dblVal, lcid, dwFlags, pbstrText ) ;

        case VT_CY:
            return VarBstrFromCy( pvar->cyVal, lcid, dwFlags , pbstrText ) ;

        case VT_DATE:
            return VarBstrFromDate( pvar->date, lcid, VAR_DATEVALUEONLY, pbstrText ) ;

        case VT_FILETIME:
        {
            SYSTEMTIME  st ;
            DATE        d ;

            if( pvar->filetime.dwLowDateTime == 0L &&
                pvar->filetime.dwHighDateTime ==0L )
            {
                *pbstrText = SysAllocString( L"" ) ;
                return S_OK ;
            }

            if( FileTimeToSystemTime( &pvar->filetime, &st ) &&
                SystemTimeToVariantTime( &st, &d ) )
            {
                return VarBstrFromDate( d, lcid, VAR_DATEVALUEONLY, pbstrText ) ;
            }
            break ;
        }
        
        case VT_CLSID:
            if( pvar->puuid )
            {
                LPWSTR pwsz = NULL ;
                //  Note: we can't hand back this directly, because the
                //  caller expects a BSTR that should be freed with SysFreeString.
                if( SUCCEEDED( (hr = StringFromCLSID( *pvar->puuid, &pwsz )) ) )
                {
                    *pbstrText = SysAllocString( pwsz ) ;
                    CoTaskMemFree( pwsz ) ; // free intermediate buffer.
                    if( *pbstrText ) 
                        return S_OK ;
                    break ;
                }
            }
            else
            {
                if( (*pbstrText = SysAllocString( L"" ))!=NULL )
                    return S_OK ;
            }
            break ;

        case VT_BLOB:
            if( pvar->blob.pBlobData != NULL )
                *pbstrText = SysAllocString( L"..." ) ;
            else
                *pbstrText = SysAllocString( L"" ) ;
            return S_OK ;

        case VT_CF :
        {
            long cf = 0 ;
            
            if( pvar->pclipdata ) 
                cf = pvar->pclipdata->ulClipFmt ;
            if( cf > 0 )
            {
                if( (*pbstrText = SysAllocString( clipformats[cf-1] ))!=NULL )
                    return S_OK ;
            }
            break ;
        }

        case VT_BSTR:
            if( (*pbstrText = SysAllocString( pvar->bstrVal ? pvar->bstrVal : L"" ))!=NULL )
                return S_OK ;
            break ;

        case VT_LPSTR:
            if( (*pbstrText = SysAllocString( pvar->pszVal ? A2W( pvar->pszVal ) : L"" )) != NULL )
                return S_OK ;
            break ;
            
        case VT_LPWSTR:
            if( (*pbstrText = SysAllocString( pvar->pwszVal ? pvar->pwszVal : L"" )) != NULL )
                return S_OK ;
            break ;

        case VT_VECTOR | VT_UI1:
            //pvar->caub;     
        case VT_VECTOR | VT_I2:
            //pvar->cai;      
        case VT_VECTOR | VT_UI2:
            //pvar->caui;     
        case VT_VECTOR | VT_I4:
            //pvar->cal;      
        case VT_VECTOR | VT_UI4:
            //pvar->caul;     
        case VT_VECTOR | VT_I8:
            //pvar->cah;      
        case VT_VECTOR | VT_UI8:
            //pvar->cauh;     
        case VT_VECTOR | VT_R4:
            //pvar->caflt;    
        case VT_VECTOR | VT_R8:
            //pvar->cadbl;    
        case VT_VECTOR | VT_CY:
            //pvar->cacy;     
        case VT_VECTOR | VT_DATE:
            //pvar->cadate;   
        case VT_VECTOR | VT_BSTR:
            //pvar->cabstr;   
        case VT_VECTOR | VT_BOOL:
            //pvar->cabool;   
        case VT_VECTOR | VT_ERROR:
            //pvar->cascode;  
        case VT_VECTOR | VT_LPSTR:
            //pvar->calpstr;  
        case VT_VECTOR | VT_LPWSTR:
            //pvar->calpwstr; 
        case VT_VECTOR | VT_FILETIME:
            //pvar->cafiletime; 
        case VT_VECTOR | VT_CLSID:
            //pvar->cauuid;     
        case VT_VECTOR | VT_CF:
            //pvar->caclipdata; 
        case VT_VECTOR | VT_VARIANT:
            //pvar->capropvar;  
            return E_NOTIMPL ;

        //  Unsupported types
        case VT_STREAM:
        case VT_STORAGE:
            return E_NOTIMPL ;

        //  Illegal types for OLE properties
        default:
            return ERROR_INVALID_DATA ;
    }

    return hr ;
}

//-------------------------------------------------------------------------//
HRESULT PropVariantToString( const PROPVARIANT* pvar, UINT nCodePage, ULONG dwFlags, LPCTSTR pszFmt, OUT LPTSTR pszText, IN ULONG cchTextMax )
{
    BSTR    bstrText = NULL ;
    HRESULT hr = PropVariantToBstr( pvar, nCodePage, dwFlags, pszFmt, &bstrText ) ;

    if( SUCCEEDED( hr ) )
    {
        lstrcpyn( pszText, W2T( bstrText ), cchTextMax ) ;
        SysFreeString( bstrText ) ;
    }
    return hr ;
}

//-------------------------------------------------------------------------//
HRESULT PropVariantFromBstr( 
    IN const BSTR bstrText, 
    IN UINT nCodePage, 
    IN ULONG dwFlags, 
    IN LPCTSTR pszFmt, 
    OUT PROPVARIANT* pvar )
{
    UNREFERENCED_PARAMETER( pszFmt ) ;
    ASSERT( pvar ) ;

    HRESULT     hr = E_FAIL ;
    LCID        lcid = GetUserDefaultLCID() ;
    VARTYPE     vtSave = pvar->vt ;

    if( !( bstrText && *bstrText ) )
    {
        //  We're being passed an empty string, so simply zero out
        //  and restore the current VARTYPE.
        PropVariantClear( pvar ) ;
        pvar->vt = vtSave ;
        return S_OK ;
    }
    
    USES_CONVERSION ;

    switch( vtSave )
    {
        case VT_EMPTY:
        case VT_NULL:
        case VT_ILLEGAL:
            // Clear ourselves always.
            PropVariantClear( pvar ) ;
            pvar->vt = vtSave ;
            return S_OK ;   

        case VT_UI1:
            hr = VarUI1FromStr( bstrText, lcid, dwFlags, &pvar->bVal ) ;
            break ;

        case VT_I2:
            hr = VarI2FromStr( bstrText, lcid, dwFlags, &pvar->iVal ) ;
            break ;

        case VT_UI2:
            hr = VarUI2FromStr(  bstrText, lcid, dwFlags, &pvar->uiVal ) ;
            break ;
            
        case VT_BOOL:
            hr = VarBoolFromStr( bstrText, lcid, dwFlags, &pvar->boolVal ) ;
            break ;

        case VT_I4:
            hr = VarI4FromStr( bstrText, lcid, dwFlags, &pvar->lVal ) ;
            break ;
 
        case VT_UI4:
            hr = VarUI4FromStr(  bstrText, lcid, dwFlags, &pvar->ulVal ) ;
            {
            }
            break ;

        case VT_R4:
            if( SUCCEEDED( (hr = VarR4FromStr( bstrText, lcid, dwFlags, &pvar->fltVal )) ) )
            {
            }
            break ;

        case VT_ERROR:
            if( SUCCEEDED( (hr = VarI4FromStr( bstrText, lcid, dwFlags, &pvar->scode )) ) )
            {
            }
            break ;

        //case VT_I8:
        //    return _i64tot( hVal.QuadPart, pszBuf, 10 ) ; 

        //case VT_UI8:
        //    return _ui64tot( hVal.QuadPart, pszBuf, 10 ) ; 

        case VT_R8:
            if( SUCCEEDED( (hr = VarR8FromStr( bstrText, lcid, dwFlags, &pvar->dblVal )) ) )
            {
            }
            break ;

        case VT_CY:
            if( SUCCEEDED( (hr = VarCyFromStr( bstrText, lcid, dwFlags , &pvar->cyVal )) ) )
            {
            }
            break ;

        case VT_DATE:
            if( SUCCEEDED( (hr = VarDateFromStr( bstrText, lcid, VAR_DATEVALUEONLY, &pvar->date )) ) )
            {
            }
            break ;

        case VT_FILETIME:
        {
            FILETIME    ft ;
            SYSTEMTIME  st ;
            DATE        d ;

            if( bstrText && *bstrText )
            {
                if( SUCCEEDED( (hr = VarDateFromStr( bstrText, lcid, VAR_DATEVALUEONLY, &d )) ) &&
                    VariantTimeToSystemTime( d, &st ) &&
                    SystemTimeToFileTime( &st, &ft ) )
                {
                    pvar->filetime = ft ;
                    return S_OK ;
                }
            }
            else
            {
                memset( &pvar->filetime, 0, sizeof(pvar->filetime) ) ;
                return S_OK ;
            }
            break ;
        }
        
        case VT_CLSID:
            {
                CLSID clsid ;
                if( SUCCEEDED( (hr = CLSIDFromString( bstrText, &clsid )) ) )
                {
                    PropVariantClear( pvar ) ;
                    if( NULL == (pvar->puuid = (CLSID*)CoTaskMemAlloc( sizeof(clsid) )) )
                        return E_OUTOFMEMORY ;
                    *pvar->puuid = clsid ;
                    pvar->vt = vtSave ;
                    return S_OK ;
                }
            }
            break ;

        case VT_BSTR:
            PropVariantClear( pvar ) ;
            pvar->bstrVal = SysAllocString( bstrText ) ;
            pvar->vt = vtSave ;
            return S_OK ;

        case VT_LPSTR:
        {
            PropVariantClear( pvar ) ;
            int cb = sizeof(CHAR) * (lstrlenA( W2A(bstrText) ) + 1) ;
            if( NULL == (pvar->pszVal = (LPSTR)CoTaskMemAlloc( cb )) )
                return E_OUTOFMEMORY ;

            memcpy( pvar->pszVal, W2A( bstrText ), cb ) ;
            pvar->vt = vtSave ;
            return S_OK ;
        }
            
        case VT_LPWSTR:
        {
            PropVariantClear( pvar ) ;
            int cb = sizeof(WCHAR) * (lstrlenW( bstrText ) + 1) ;
            if( NULL == (pvar->pwszVal = (LPWSTR)CoTaskMemAlloc( cb )) )
                return E_OUTOFMEMORY ;

            memcpy( pvar->pwszVal, bstrText, cb ) ;
            pvar->vt = vtSave ;
            return S_OK ;
        }

        case VT_VECTOR | VT_UI1:
            //pvar->caub;     
        case VT_VECTOR | VT_I2:
            //pvar->cai;      
        case VT_VECTOR | VT_UI2:
            //pvar->caui;     
        case VT_VECTOR | VT_I4:
            //pvar->cal;      
        case VT_VECTOR | VT_UI4:
            //pvar->caul;     
        case VT_VECTOR | VT_I8:
            //pvar->cah;      
        case VT_VECTOR | VT_UI8:
            //pvar->cauh;     
        case VT_VECTOR | VT_R4:
            //pvar->caflt;    
        case VT_VECTOR | VT_R8:
            //pvar->cadbl;    
        case VT_VECTOR | VT_CY:
            //pvar->cacy;     
        case VT_VECTOR | VT_DATE:
            //pvar->cadate;   
        case VT_VECTOR | VT_BSTR:
            //pvar->cabstr;   
        case VT_VECTOR | VT_BOOL:
            //pvar->cabool;   
        case VT_VECTOR | VT_ERROR:
            //pvar->cascode;  
        case VT_VECTOR | VT_LPSTR:
            //pvar->calpstr;  
        case VT_VECTOR | VT_LPWSTR:
            //pvar->calpwstr; 
        case VT_VECTOR | VT_FILETIME:
            //pvar->cafiletime; 
        case VT_VECTOR | VT_CLSID:
            //pvar->cauuid;     
        case VT_VECTOR | VT_CF:
            //pvar->caclipdata; 
        case VT_VECTOR | VT_VARIANT:
            //pvar->capropvar;  
            return E_NOTIMPL ;

        //  Illegal types for which to assign value from display text.
        case VT_BLOB:
        case VT_CF :
        case VT_STREAM:
        case VT_STORAGE:
            return E_INVALIDARG ;

        //  Illegal types for OLE properties
        default:
            return ERROR_INVALID_DATA ;
    }

    return hr ;
}

//-------------------------------------------------------------------------//
HRESULT PropVariantFromString( 
    IN LPCTSTR pszText, 
    IN UINT nCodePage, 
    IN ULONG dwFlags, 
    IN LPCTSTR pszFmt, 
    OUT PROPVARIANT* pvar )
{
    HRESULT hr = E_FAIL ;
    BSTR    bstrText = NULL ;
    USES_CONVERSION ; 

    if( pszText )
        if( (bstrText = SysAllocString( T2W( (LPTSTR)pszText ) )) == NULL )
            return E_OUTOFMEMORY ;

    hr = PropVariantFromBstr( bstrText, nCodePage, dwFlags, pszFmt, pvar ) ;
    
    if( bstrText )
        SysFreeString( bstrText ) ;
    return hr ;
}

//-------------------------------------------------------------------------//
//  Generates a 32-bit hash value for the contents of a PROPVARIANT instance.
HRESULT PropVariantHash( IN const PROPVARIANT& var, OUT ULONG& dwHash )
{
    HRESULT     hr ;
    ULONG       cbVar = 0 ;
    const BYTE* pVar = NULL ;
    
    dwHash = var.vt ;

    if( !SUCCEEDED( (hr = PropVariantSize( var, cbVar, NULL )) ) )
        return hr ;

    switch( var.vt )
    {
        case VT_EMPTY:
        case VT_NULL:
        case VT_ILLEGAL:
            break ;

        case VT_I1:
        case VT_UI1:
        case VT_I2:
        case VT_UI2:
        case VT_I4:
        case VT_UI4:
        case VT_I8:
        case VT_UI8:
        case VT_R4:
        case VT_R8:
        case VT_CY:
        case VT_DATE:
        case VT_ERROR:
        case VT_BOOL:
        case VT_INT:
        case VT_FILETIME:
            pVar = &var.bVal ; break ;

        case VT_BSTR:
            pVar = (const BYTE*)var.bstrVal ;  break ;
        case VT_CLSID:
            pVar = (const BYTE*)var.puuid ;    break ;
        case VT_LPSTR:
            pVar = (const BYTE*)var.pszVal ;   break ;
        case VT_LPWSTR:
            pVar = (const BYTE*)var.pwszVal ;  break ;

        case VT_CF:
        case VT_BLOB:
        case VT_DISPATCH:
        case VT_STREAM:
        case VT_STORAGE:
        case VT_STREAMED_OBJECT:
        case VT_STORED_OBJECT:
        case VT_BLOB_OBJECT:

        case VT_VECTOR | VT_UI1:
            //CAUB caub;
        case VT_VECTOR | VT_I2:
            //CAI cai;
        case VT_VECTOR | VT_UI2:
            //CAUI caui;
        case VT_VECTOR | VT_BOOL:
            //CABOOL cabool;
        case VT_VECTOR | VT_I4:
            //CAL cal;
        case VT_VECTOR | VT_UI4:
            //CAUL caul;
        case VT_VECTOR | VT_R4:
            //CAFLT caflt;
        case VT_VECTOR | VT_ERROR:
            //CASCODE cascode;
        case VT_VECTOR | VT_I8:
            //CAH cah;
        case VT_VECTOR | VT_UI8:
            //CAUH cauh;
        case VT_VECTOR | VT_R8:
            //CADBL cadbl;
        case VT_VECTOR | VT_CY:
            //CACY cacy;
        case VT_VECTOR | VT_DATE:
            //CADATE cadate;
        case VT_VECTOR | VT_FILETIME:
            //CAFILETIME cafiletime;
        case VT_VECTOR | VT_CLSID:
            //CACLSID cauuid;

        case VT_VECTOR | VT_BSTR:
            //CABSTR cabstr;
        case VT_VECTOR | VT_LPSTR:
            //CALPSTR calpstr;
        case VT_VECTOR | VT_LPWSTR:
            //CALPWSTR calpwstr;
        case VT_VECTOR | VT_CF:
            //CACLIPDATA caclipdata;
        case VT_VECTOR | VT_BLOB:
            //CABSTRBLOB cabstrblob;
        //case VT_VECTOR | :
            //CAPROPVARIANT capropvar;
            return E_NOTIMPL ;
    }
    
    if( pVar )
    {
        for( ULONG i=0; i<cbVar; i++ )
            dwHash += pVar[i] ;
    }
    return S_OK ;
}

//-------------------------------------------------------------------------//
//  Writes the value of a PROPVARIANT to the registry
HRESULT PropVariantWriteRegistry( HKEY hKey, LPCTSTR pszValueName, IN const PROPVARIANT& var )
{
    HRESULT  hr ;
    DWORD    cbVal = 0 ;
    LPBYTE   pBuf = NULL ;
    
    //  Determine size of data
    if( !SUCCEEDED( (hr = ::PropVariantSize( var, cbVal, NULL )) ) )
        return hr ;

    //  Allocate buffer
    cbVal += sizeof(VARTYPE) ;
    if( (pBuf = new BYTE[cbVal])==NULL )
        return E_OUTOFMEMORY ;
    memset( pBuf, 0, cbVal ) ;

    //  Copy to contiguous memory, write to registry.
    if( SUCCEEDED( (hr = PropVariantWriteMem( var, pBuf, cbVal )) ) )
    {
        hr = HRESULT_FROM_WIN32( RegSetValueEx( hKey, pszValueName, 0L, REG_BINARY, 
                                 pBuf, cbVal ) ) ;
    }

    delete [] pBuf ;
    return hr ;
}

//-------------------------------------------------------------------------//
//  Reads the value of a PROPVARIANT from the registry
HRESULT PropVariantReadRegistry( HKEY hKey, LPCTSTR pszValueName, OUT PROPVARIANT& var )
{
    HRESULT hr ;
    ULONG   dwErr ;
    DWORD dwType = REG_BINARY,
          cbVal = sizeof(var.vt) ;

    PropVariantInit( &var ) ;

    dwErr = RegQueryValueEx( hKey, pszValueName, 0L, &dwType, (LPBYTE)&var.vt, &cbVal ) ;

    if( !SUCCEEDED( (hr = HRESULT_FROM_WIN32( dwErr )) ) )
    {
        if( dwErr == ERROR_MORE_DATA )
        {
            LPBYTE pBuf = NULL ;
            if( (pBuf = new BYTE[cbVal+1])==NULL )
                return E_OUTOFMEMORY ;

            dwErr = RegQueryValueEx( hKey, pszValueName, 0L, &dwType, pBuf, &cbVal ) ;
            if( SUCCEEDED( (hr = HRESULT_FROM_WIN32( dwErr )) ) )
                hr = PropVariantReadMem( pBuf, var ) ;

            delete [] pBuf ;
        }
#ifdef TRACE
        else
        {
            TRACE( TEXT("Failed to read PROPVARIANT (VARTYPE %d) from registry (%ld)\n"), var.vt, dwErr ) ;
        }
#endif TRACE
    }
    return hr ;
}                           

//-------------------------------------------------------------------------//
//  Comparison helpers
#define NUMERIC_COMPARE( a, b )      (((a)>(b)) ? 1 : ((a)<(b)) ? -1 : 0)
#define POINTER_COMPARE( a, b, cb )  (((a)&&(b)) ? memcmp( (a), (b), (cb) ) : \
                                      (a) ? 1 : (b) ? -1 : 0)

inline int TEXT_COMPAREW( LPWSTR psz1, LPWSTR psz2, ULONG uFlags )
{
    if( psz1 && psz2 )
        return (uFlags & PVCF_IGNORECASE) ? lstrcmpiW( psz1, psz2 ) :
                                            lstrcmpW( psz1, psz2 );
                         
    else if( psz1 && *psz1 )
        return 1 ;
    else if( psz2 && *psz2 )
        return -1 ;
    return 0 ;
}

inline int TEXT_COMPAREA( LPSTR psz1, LPSTR psz2, ULONG uFlags )
{
    if( psz1 && psz2 )
        return (uFlags & PVCF_IGNORECASE) ? lstrcmpiA( psz1, psz2 ) :
                                            lstrcmpA( psz1, psz2 ) ;    
    else if( psz1 && *psz1 )
        return 1 ;
    else if( psz2 && *psz2 )
        return -1 ;
    return 0 ;
}

int DATETIME_COMPARE( const SYSTEMTIME* pst1, const SYSTEMTIME* pst2, ULONG uFlags )
{
    int nRet = 0;

    if( uFlags & PVCF_IGNOREDATE )
    {
        if( (nRet = NUMERIC_COMPARE( pst1->wHour, pst2->wHour )) != 0 )
            if( (nRet = NUMERIC_COMPARE( pst1->wMinute, pst2->wMinute )) == 0 )
                if( (nRet = NUMERIC_COMPARE( pst1->wSecond, pst2->wSecond )) == 0 )
                    nRet = NUMERIC_COMPARE( pst1->wMilliseconds, pst2->wMilliseconds );
    }
    else if( uFlags & PVCF_IGNORETIME )
    {
        if( (nRet = NUMERIC_COMPARE( pst1->wYear, pst2->wYear )) == 0 )
            if( (nRet = NUMERIC_COMPARE( pst1->wMonth, pst2->wMonth )) == 0 )
                nRet = NUMERIC_COMPARE( pst1->wDay, pst2->wDay );
    }
    else
    {
        nRet = memcmp( pst1, pst2, sizeof(*pst1) );
    }

    return nRet;
}

//-------------------------------------------------------------------------//
//  Compares the values of two PROPVARIANT instances; returns -1 if var1 is 
//  less than var2, 1 if va1 is greater than var2, and 0 if the two values
//  are equal.
int PropVariantCompare( const PROPVARIANT& var1, const PROPVARIANT& var2, ULONG uFlags )
{
    int iRet = 0 ;
    
    //  Compare VARTYPE.
    if( var1.vt != var2.vt ) 
        return NUMERIC_COMPARE( var1.vt, var2.vt ) ;

    //  Compare data
    switch( var1.vt )
    {
        case VT_EMPTY:
        case VT_NULL:
        case VT_ILLEGAL:
            return 0 ;

        case VT_UI1:
            return NUMERIC_COMPARE( var1.bVal, var2.bVal ) ;

        case VT_I2:
            return NUMERIC_COMPARE( var1.iVal, var2.iVal ) ;

        case VT_UI2:
            return NUMERIC_COMPARE( var1.uiVal, var2.uiVal );
            
        case VT_BOOL:
            return NUMERIC_COMPARE( var1.boolVal, var2.boolVal ) ;

        case VT_I4:
            return NUMERIC_COMPARE( var1.lVal, var2.lVal ) ;
 
        case VT_UI4:
            return NUMERIC_COMPARE( var1.ulVal, var2.ulVal );

        case VT_R4:
            return NUMERIC_COMPARE( var1.fltVal, var2.fltVal ) ;

        case VT_ERROR:
            return NUMERIC_COMPARE( var1.scode, var2.scode );

        case VT_I8:
            return NUMERIC_COMPARE( var1.hVal.QuadPart, var2.hVal.QuadPart ) ;

        case VT_UI8:
            return NUMERIC_COMPARE( var1.uhVal.QuadPart, var2.uhVal.QuadPart ) ;

        case VT_R8:
            return NUMERIC_COMPARE( var1.dblVal, var2.dblVal ) ;

        case VT_CY:
            return memcmp( &var1.cyVal, &var2.cyVal, sizeof(CY) ) ;

        case VT_DATE:
        {
            if( 0 == (uFlags & (PVCF_IGNOREDATE|PVCF_IGNORETIME)) )
                return NUMERIC_COMPARE( var1.date, var2.date ) ;

            SYSTEMTIME st1, st2;
            VariantTimeToSystemTime( var1.date, &st1 );
            VariantTimeToSystemTime( var2.date, &st2 );
            return DATETIME_COMPARE( &st1, &st2, uFlags );
        }

        case VT_FILETIME:
        {
            if( 0 == (uFlags & (PVCF_IGNOREDATE|PVCF_IGNORETIME)) )
                return memcmp( &var1.filetime, &var2.filetime, sizeof(FILETIME) ) ;

            SYSTEMTIME st1, st2;
            FileTimeToSystemTime( &var1.filetime, &st1 );
            FileTimeToSystemTime( &var2.filetime, &st2 );

            return DATETIME_COMPARE( &st1, &st2, uFlags );
        }
        
        case VT_CLSID:
            return POINTER_COMPARE( var1.puuid, var2.puuid, sizeof(CLSID) ) ;

        case VT_BLOB:
            if( (iRet = POINTER_COMPARE( var1.blob.pBlobData, var2.blob.pBlobData,
                                        min( var1.blob.cbSize, var2.blob.cbSize ) ))==0 )
                iRet = NUMERIC_COMPARE( var1.blob.cbSize, var2.blob.cbSize ) ;
            return iRet ;

        case VT_CF :
            if( (iRet = NUMERIC_COMPARE( (DWORD_PTR)var1.pclipdata, (DWORD_PTR)var2.pclipdata ))==0 &&
                var1.pclipdata && var2.pclipdata &&
                (iRet = NUMERIC_COMPARE( var1.pclipdata->ulClipFmt, var2.pclipdata->ulClipFmt ))==0 &&
                (iRet = POINTER_COMPARE( var1.pclipdata->pClipData, var2.pclipdata->pClipData,
                                         min( var1.pclipdata->cbSize, var2.pclipdata->cbSize ) ))==0 )
            {                
                iRet = NUMERIC_COMPARE( var1.pclipdata->cbSize, var2.pclipdata->cbSize ) ;
            }
            return iRet ;

        case VT_BSTR:
            return TEXT_COMPAREW( var1.bstrVal, var2.bstrVal, uFlags ) ;

        case VT_LPSTR:
            return TEXT_COMPAREA( var1.pszVal, var2.pszVal, uFlags ) ;
            
        case VT_LPWSTR:
            return TEXT_COMPAREW( var1.pwszVal, var2.pwszVal, uFlags ) ;

        case VT_VECTOR | VT_UI1:
            //caub;     
        case VT_VECTOR | VT_I2:
            //cai;      
        case VT_VECTOR | VT_UI2:
            //caui;     
        case VT_VECTOR | VT_I4:
            //cal;      
        case VT_VECTOR | VT_UI4:
            //caul;     
        case VT_VECTOR | VT_I8:
            //cah;      
        case VT_VECTOR | VT_UI8:
            //cauh;     
        case VT_VECTOR | VT_R4:
            //caflt;    
        case VT_VECTOR | VT_R8:
            //cadbl;    
        case VT_VECTOR | VT_CY:
            //cacy;     
        case VT_VECTOR | VT_DATE:
            //cadate;   
        case VT_VECTOR | VT_BSTR:
            //cabstr;   
        case VT_VECTOR | VT_BOOL:
            //cabool;   
        case VT_VECTOR | VT_ERROR:
            //cascode;  
        case VT_VECTOR | VT_LPSTR:
            //calpstr;  
        case VT_VECTOR | VT_LPWSTR:
            //calpwstr; 
        case VT_VECTOR | VT_FILETIME:
            //cafiletime; 
        case VT_VECTOR | VT_CLSID:
            //cauuid;     
        case VT_VECTOR | VT_CF:
            //caclipdata; 
        case VT_VECTOR | VT_VARIANT:
            //capropvar;  
            break ;  //BUGBUG: Need to implement comparisons of these types.

        case VT_STREAM:
        case VT_STORAGE:
            break ;  //BUGBUG: Need to implement comparisons of these types.
    }

    return iRet ;
}

//-------------------------------------------------------------------------//
// A PROPVARIANT can hold a few more types than a VARIANT can.  We convert the types that are
// only supported by a PROPVARIANT into equivalent VARIANT types.
HRESULT PropVariantToVariant( IN const PROPVARIANT *pPropVar, OUT VARIANT *pVar)
{
    HRESULT hr = E_FAIL ;
    ASSERT(pPropVar && pVar);

    // if pVar isn't empty, this will properly free before overwriting it
    VariantClear(pVar);

    switch(pPropVar->vt)
    {
        case VT_LPSTR: 
        {
            int len = lstrlenA(pPropVar->pszVal);
            pVar->bstrVal = SysAllocStringLen(NULL, len);
            if (pVar->bstrVal)
            {
                pVar->vt = VT_BSTR;
                MultiByteToWideChar(CP_ACP, 0, pPropVar->pszVal, len, pVar->bstrVal, len);
                hr = S_OK ;
            }

            break;
        }

        case VT_LPWSTR:
            pVar->bstrVal = SysAllocString(pPropVar->pwszVal);
            if (pVar->bstrVal)
            {
                pVar->vt = VT_BSTR;
                hr = S_OK ;
            }
            break;

        case VT_FILETIME:
        {
            SYSTEMTIME st;
            pVar->vt = VT_DATE;
            FileTimeToSystemTime(&pPropVar->filetime, &st);
            SystemTimeToVariantTime(&st, &pVar->date); // delay load...
            hr = S_OK ;
            break;
        }

        case VT_UI2:
            if (pPropVar->uiVal < 0x8000)
            {
                pVar->vt = VT_I2;
                pVar->iVal = (signed short) pPropVar->uiVal;
                hr = S_OK ;
            }
            break;

        case VT_UI4:
            if (pPropVar->ulVal < 0x80000000)
            {
                pVar->vt = VT_I4;
                pVar->lVal = (signed int) pPropVar->ulVal;
                hr = S_OK ;
            }
            break;

        case VT_CLSID:
            if( pPropVar->puuid )
            {
                LPOLESTR pwszPuuid ;
                if( SUCCEEDED( (hr = StringFromCLSID( *pPropVar->puuid, &pwszPuuid )) ) )
                {
                    if( NULL == (pVar->bstrVal = SysAllocString( pwszPuuid )) )
                        hr = E_OUTOFMEMORY ;
                    else
                    {
                        pVar->vt = VT_BSTR;
                        hr = S_OK ;
                    }
                    CoTaskMemFree( pwszPuuid ) ;
                }
            }
            break;

        case VT_BLOB:
        case VT_STREAM:
        case VT_STORAGE:
        case VT_BLOB_OBJECT:
        case VT_STREAMED_OBJECT:
        case VT_STORED_OBJECT:
        case VT_CF:
            ASSERT(0); // leave the output cleared
            hr = E_NOTIMPL ;
            break;

        default:
            VariantCopy(pVar, (VARIANT *)pPropVar);
            hr = S_OK ;
            break;
    }
    return hr ;
}

//-------------------------------------------------------------------------//
//  nullifies time fields in a SYSTEMTIME structure.
void SystemTimeMakeTimeless( SYSTEMTIME* pst )
{
    //  note: If the block is non-null, we set the hour to noon to avoid 
    //  date shifting across daylight savings time boundaries.
    pst->wHour = ( pst->wYear || pst->wMonth || pst->wDay ) ? 12 : 0;
    pst->wMinute = pst->wSecond = pst->wMilliseconds = 0;
}

//-------------------------------------------------------------------------//
//  nullifies time fields in a date/time propvariant.
HRESULT PropVariantMakeTimeless( PROPVARIANT* pvar )
{
    ASSERT(pvar);
    
    HRESULT     hr = S_FALSE;
    SYSTEMTIME  st;

    switch( pvar->vt )
    {
        case VT_FILETIME:
        {
            FileTimeToSystemTime( &pvar->filetime, &st );
            SystemTimeMakeTimeless( &st );
            hr = S_OK;
            SystemTimeToFileTime( &st, &pvar->filetime );
            break;
        }

        case VT_DATE:
            VariantTimeToSystemTime( pvar->date, &st );
            SystemTimeMakeTimeless( &st );
            hr = S_OK;
            SystemTimeToVariantTime( &st, &pvar->date );
            break;
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  class CPropVariant : public PROPVARIANT  
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
HRESULT CPropVariant::GetDisplayText( 
    OUT LPTSTR pszText, 
    IN int cchText, 
    IN OPTIONAL LPCTSTR pszFmt,
    IN OPTIONAL ULONG dwFlags ) const
{
    return PropVariantToString( this, GetACP(), dwFlags, pszFmt, pszText, cchText ) ;
}

//-------------------------------------------------------------------------//
HRESULT CPropVariant::GetDisplayText( 
    OUT BSTR& bstrText,
    IN OPTIONAL LPCTSTR pszFmt,
    IN OPTIONAL ULONG dwFlags ) const
{
    return PropVariantToBstr( this, GetACP(), dwFlags, pszFmt, &bstrText ) ;
}

//-------------------------------------------------------------------------//
HRESULT CPropVariant::AssignFromDisplayText( IN LPCTSTR pszText, IN OPTIONAL LPCTSTR pszFmt )
{
    return PropVariantFromString( pszText, GetACP(), 0L, NULL, this ) ;
}

//-------------------------------------------------------------------------//
HRESULT CPropVariant::AssignFromDisplayText( IN const BSTR bstrText, IN OPTIONAL LPCTSTR pszFmt )
{
    return PropVariantFromBstr( bstrText, GetACP(), 0L, NULL, this ) ;
}

//-------------------------------------------------------------------------//
void CPropVariant::Attach( IN OUT PROPVARIANT* pSrc )
{
	ASSERT( pSrc ) ;
    // Initialize ourselves
    Clear();
	// Copy the source contents
	memcpy( (PROPVARIANT*)this, pSrc, sizeof(PROPVARIANT) );
    //  Empty the source and assume control of the data.
	PropVariantInit( pSrc ) ;
}

//-------------------------------------------------------------------------//
void CPropVariant::Detach( OUT PROPVARIANT* pDest )
{
    ASSERT( pDest ) ;

    // Initialize the target
	PropVariantClear( pDest ) ;
	
    // Copy out our contents and revoke control.
	memcpy( pDest, this, sizeof(PROPVARIANT)) ;

    //  Initialize ourselves
    PropVariantInit( this ) ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CPropVariant& src )
{
    Clear() ;
    ::PropVariantCopy( this, &src ) ;   //  BUGBUG: This will leak on free.
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const PROPVARIANT& src )
{
    Clear() ;
    ::PropVariantCopy( this, &src ) ;   //  BUGBUG: This will leak on free.
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const BYTE& src )
{
    Clear() ;
    bVal = src ;
    vt = VT_UI1 ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const SHORT& src )
{
    Clear() ;
    iVal = src ;
    vt = VT_I2 ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const USHORT& src )
{
    Clear() ;
    uiVal = src ;
    vt = VT_UI2 ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const bool& src )
{
    Clear() ;
#pragma warning(disable: 4310) // cast truncates constant value
    boolVal = src ? VARIANT_TRUE : VARIANT_FALSE ;
#pragma warning(default: 4310) // cast truncates constant value
    vt = VT_BOOL ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const LONG& src )
{
    Clear() ;
    lVal = src ;
    vt = VT_I4 ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const ULONG& src )
{
    Clear() ;
    ulVal = src ;
    vt = VT_UI4 ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const FLOAT& src )
{
    Clear() ;
    fltVal = src ;
    vt = VT_R4 ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const LARGE_INTEGER& src )
{
    Clear() ;
    hVal = src ;
    vt = VT_I8 ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const ULARGE_INTEGER& src )
{
    Clear() ;
    uhVal = src ;
    vt = VT_UI8 ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const DOUBLE& src )
{
    Clear() ;
    dblVal = src ;
    vt = VT_R8 ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CY& src )
{
    Clear() ;
    cyVal = src ;
    vt = VT_CY ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const FILETIME& src )
{
    Clear() ;
    filetime = src ;
    vt = VT_FILETIME ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CLSID& src )
{
    Clear() ;
    if( (puuid = (CLSID*)AllocAndCopy( &src, sizeof(src) ))!=NULL )
        vt = VT_CLSID ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const BLOB& src )
{
    Clear() ;
    if( src.pBlobData != NULL && src.cbSize != 0 )
    {
        if( (blob.pBlobData = (LPBYTE)AllocAndCopy( src.pBlobData, src.cbSize ))!=NULL )
        {
            blob.cbSize = src.cbSize ;
            vt = VT_BLOB ;
        }
    }

    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CLIPDATA& src )
{
    Clear() ;
    if( (pclipdata = (CLIPDATA*)AllocAndCopy( &src, sizeof(src) ))!=NULL )
    {
        pclipdata->pClipData = NULL ;
        ULONG cb = src.pClipData ? CBPCLIPDATA( src ) : 0 ;

        if( cb!=0 && (pclipdata->pClipData = (LPBYTE)AllocAndCopy( src.pClipData, cb ))!=NULL )
            vt = VT_CF ;
        else
        {
            CoTaskMemFree( pclipdata ) ;
            pclipdata = NULL ;
        }
    }
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( IStream* src )
{
    Clear() ;
    pStream = src ;
    vt = VT_STREAM ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( IStorage* src )
{
    Clear() ;
    pStorage = src ;
    vt = VT_STORAGE ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const BSTR& src )
{
    Clear() ;
    if( (bstrVal = AllocString( src ))!=NULL )
        vt = VT_BSTR ;
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const LPSTR src )
{
    int cch ;
    
    Clear() ;
    if( src && (cch = lstrlenA( src ) + 1)> 1 )    {
        if( (pszVal = (LPSTR)AllocAndCopy( src, cch * sizeof(CHAR) ))!=NULL )
            vt = VT_LPSTR ;
    }

    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const LPWSTR src )
{
    int cch ;
    
    Clear() ;
    if( src && (cch = lstrlenW( src ) + 1)> 1 )    {
        if( (pwszVal = (LPWSTR)AllocAndCopy( src, cch * sizeof(WCHAR) ))!=NULL )
            vt = VT_LPWSTR ;
    }

    return *this ;
}

//-------------------------------------------------------------------------//
//  Helper template function for vector type assignments
template <class CATYPE>
BOOL PropVectorAssignShallow( OUT CATYPE& caDest, OUT VARTYPE& vt, IN const CATYPE& caSrc, IN VARTYPE vtElems )
{
#pragma warning( disable : 4244 )   // conversion from 'typeA' to 'typeA', possible loss of data
    PVOID*  ppvAlloc = (PVOID*)&caDest.pElems ;
    ULONG   cb = caSrc.cElems * sizeof(*caSrc.pElems) ;

    if( ((*ppvAlloc) = CoTaskMemAlloc( cb ))!=NULL )
    {
        memcpy( caDest.pElems, caSrc.pElems, cb ) ;
        caDest.cElems = caSrc.cElems ;
        vt = VT_VECTOR|vtElems ;
        return TRUE ;
    }
    return FALSE ;
#pragma warning( default : 4244 )   // conversion from 'typeA' to 'typeA', possible loss of data
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CAUB& src )
{
    Clear() ;
    PropVectorAssignShallow( caub, vt, src, VT_UI1 ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CAI& src )
{
    Clear() ;
    PropVectorAssignShallow( cai, vt, src, VT_I2 ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CAUI& src )
{
    Clear() ;
    PropVectorAssignShallow( caui, vt, src, VT_UI2 ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CABOOL& src )
{
    Clear() ;
    PropVectorAssignShallow( cabool, vt, src, VT_BOOL ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CAL& src )
{
    Clear() ;
    PropVectorAssignShallow( cal, vt, src, VT_I4 ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CAUL& src )
{
    Clear() ;
    PropVectorAssignShallow( caul, vt, src, VT_UI4 ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CAFLT& src )
{
    Clear() ;
    PropVectorAssignShallow( caflt, vt, src, VT_R4 ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CASCODE& src )
{
    Clear() ;
    PropVectorAssignShallow( cascode, vt, src, VT_ERROR ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CAH& src )
{
    Clear() ;
    PropVectorAssignShallow( cah, vt, src, VT_I8 ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CAUH& src )
{
    Clear() ;
    PropVectorAssignShallow( cauh, vt, src, VT_UI8 ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CADBL& src )
{
    Clear() ;
    PropVectorAssignShallow( cadbl, vt, src, VT_R8 ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CACY& src )
{
    Clear() ;
    PropVectorAssignShallow( cacy, vt, src, VT_CY ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CADATE& src )
{
    Clear() ;
    PropVectorAssignShallow( cadate, vt, src, VT_DATE ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CAFILETIME& src )
{
    Clear() ;
    PropVectorAssignShallow( cafiletime, vt, src, VT_FILETIME ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CACLSID& src )
{
    Clear() ;
    PropVectorAssignShallow( cauuid, vt, src, VT_CLSID ) ; 
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CACLIPDATA& src )
{
    Clear() ;
    if( PropVectorAssignShallow( caclipdata, vt, src, VT_CF ) )
    {
        for( ULONG i=0, cb; i<src.cElems; i++ )
        {
            caclipdata.pElems[i].cbSize     = 0 ;
            caclipdata.pElems[i].pClipData  = NULL ;
            
            if( (cb = CBPCLIPDATA( src.pElems[i] ))!=0 &&
                src.pElems[i].pClipData!=NULL )
            {
                if( (caclipdata.pElems[i].pClipData = 
                        (BYTE*)AllocAndCopy( src.pElems[i].pClipData, cb ))!=NULL )
                    caclipdata.pElems[i].cbSize = src.pElems[i].cbSize ;
            }
        }
    }
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CABSTR& src )
{
    Clear() ;
    //  Do shallow copy    
    if( PropVectorAssignShallow( cabstr, vt, src, VT_BSTR ) )
    {
        //  Do deep copy
        for( ULONG i=0; i<src.cElems; i++ )
        {
            cabstr.pElems[i] = src.pElems[i]==NULL ? 
                NULL : SysAllocString( src.pElems[i] ) ;
        }
    }
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CALPSTR& src )
{
    Clear() ;
    //  Do shallow copy
    if( PropVectorAssignShallow( calpstr, vt, src, VT_LPSTR ) )
    {
        //  Do deep copy
        for( ULONG i=0; i<src.cElems; i++ )
        {
            if( src.pElems[i]!=NULL )
            {
                UINT cch = lstrlenA( src.pElems[i] )+1 ;
                calpstr.pElems[i] = (LPSTR)AllocAndCopy( src.pElems[i], cch ) ;
            }
        }
    }
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CALPWSTR& src )
{
    Clear() ;
    //  Do shallow copy
    if( PropVectorAssignShallow( calpwstr, vt, src, VT_LPWSTR ) )
    {
        //  Do deep copy
        for( ULONG i=0; i<src.cElems; i++ )
        {
            if( src.pElems[i]!=NULL )
            {
                UINT cch = lstrlenW( src.pElems[i] )+1 ;
                calpwstr.pElems[i] = (LPWSTR)AllocAndCopy( src.pElems[i], cch ) ;
            }
        }
    }
    return *this ;
}

//-------------------------------------------------------------------------//
CPropVariant& CPropVariant::operator=( const CAPROPVARIANT& src )
{
    Clear() ;
    if( PropVectorAssignShallow( capropvar, vt, src, VT_VARIANT ) )
    {
        for( ULONG i=0; i<src.cElems; i++ )
            ::PropVariantCopy( &capropvar.pElems[i], &src.pElems[i] ) ;    
    }
    return *this ;
}

//-------------------------------------------------------------------------//
int CPropVariant::CompareText( 
    const CPropVariant& varOther, 
    ULONG uFlags,
    IN OPTIONAL LPCTSTR pszFmt, 
    IN OPTIONAL ULONG dwFlags ) const
{
    BSTR    bstrThis  = NULL, 
            bstrOther = NULL;
    HRESULT hrThis, 
            hrOther ;
    int     iRet = 0 ;

    hrThis  = GetDisplayText( bstrThis, pszFmt, dwFlags ) ;
    hrOther = varOther.GetDisplayText( bstrOther, pszFmt, dwFlags ) ;

    if( SUCCEEDED( hrThis ) && SUCCEEDED( hrOther ) )
        iRet = uFlags & PVCF_IGNORECASE ? lstrcmpiW( bstrThis, bstrOther ) :
                                          lstrcmpW( bstrThis, bstrOther ) ;
    else if( SUCCEEDED( hrThis ) )
        iRet = 1 ;
    else if( SUCCEEDED( hrOther ) )
        iRet = -1 ;

    SysFreeString( bstrThis ) ;
    SysFreeString( bstrOther ) ;
    
    return iRet ;
}

//-------------------------------------------------------------------------//
//  Write to registry
HRESULT CPropVariant::WriteValue( HKEY hKey, LPCTSTR pszValueName ) const
{
    return PropVariantWriteRegistry( hKey, pszValueName, *this ) ;
}

//-------------------------------------------------------------------------//
//  Read from registry
HRESULT CPropVariant::ReadValue( HKEY hKey, LPCTSTR pszValueName )
{
    Clear() ;
    return PropVariantReadRegistry( hKey, pszValueName, *this ) ;
}                           


//-------------------------------------------------------------------------//
//  class CDisplayPropVariant : public CPropVariant 
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  Generates display text for the object, optionally using the provided
//  formatting string
HRESULT CDisplayPropVariant::CreateDisplayText( 
    IN OPTIONAL LPCTSTR pszFmt,
    IN OPTIONAL ULONG dwFlags )
{
    BSTR    bstrDisplay = NULL ;
    HRESULT hr = GetDisplayText( bstrDisplay, pszFmt, dwFlags ) ;
    if( SUCCEEDED( hr ) )
        SetDisplayText( bstrDisplay ) ;

    return hr ;
}
//-------------------------------------------------------------------------//
void CDisplayPropVariant::SetDisplayTextT( IN LPCTSTR pszDisplay )
{
    BSTR bstrDisplay = NULL ;

    if( pszDisplay )
    {
        USES_CONVERSION ;
        bstrDisplay = SysAllocString( T2W( (LPTSTR)pszDisplay ) ) ;
    }

    SetDisplayText( bstrDisplay ) ;
}
//-------------------------------------------------------------------------//
//  Assigns display text.
void CDisplayPropVariant::SetDisplayText( BSTR bstrDisplay )
{
    if( m_bstrDisplay )
    {
        SysFreeString( m_bstrDisplay ) ;
        m_bstrDisplay = NULL ;
    }
    m_bstrDisplay = SysAllocString( bstrDisplay ) ;
}


//-------------------------------------------------------------------------//
//  class CPropertyUID : public tagPUID
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  hashing method for class CPropertyUID
ULONG CPropertyUID::Hash() const 
{ 
    int   cb = sizeof(fmtid) ; 
    ULONG nHash = 0 ;
    BYTE* p = (BYTE*)&fmtid ;
    while( cb-- )
        nHash += *p++ ;
    nHash += propid ;
    return  nHash ;
}
