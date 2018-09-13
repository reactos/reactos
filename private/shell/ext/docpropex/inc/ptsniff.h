//-------------------------------------------------------------------------//
//
//  PTsniff.h - API for quick determination of file type support.
//
//-------------------------------------------------------------------------//

#ifndef __PTSNIFF_H__
#define __PTSNIFF_H__

//-------------------------------------------------------------------------//
//
enum PTSRV_FILECLASS
{
    PTSFCLASS_UNKNOWN = -1,
    PTSFCLASS_UNSUPPORTED,

    PTSFCLASS_OFFICEDOC,
    PTSFCLASS_IMAGE,
    PTSFCLASS_AUDIO,
    PTSFCLASS_VIDEO,
} ;

enum PTSRV_FILETYPE
{
    PTSFTYPE_UNKNOWN = -1,
    PTSFTYPE_UNSUPPORTED,

    PTSFTYPE_DOC,
    PTSFTYPE_XLS,
    PTSFTYPE_PPT,

    PTSFTYPE_BMP,
    PTSFTYPE_EPS,
    PTSFTYPE_FPX,
    PTSFTYPE_GIF,
    PTSFTYPE_JPG,
    PTSFTYPE_PCD,
    PTSFTYPE_PCX,
    PTSFTYPE_PICT,
    PTSFTYPE_PNG,
    PTSFTYPE_TGA,
    PTSFTYPE_TIF,

    PTSFTYPE_AVI,
    PTSFTYPE_WAV,
    PTSFTYPE_MIDI,

    PTSFTYPE_HTML,
    PTSFTYPE_XML,
    PTSFTYPE_LNK,
} ;

//-------------------------------------------------------------------------//
EXTERN_C BOOL WINAPI IsPTsrvKnownFileType( 
    IN LPCTSTR pszPath, 
    OUT OPTIONAL PTSRV_FILETYPE* pType, 
    OUT OPTIONAL PTSRV_FILECLASS* pClass ) ;

EXTERN_C BOOL WINAPI IsPropSetStgFmt( IN LPCTSTR pszPath, ULONG dwStgFmt, OUT OPTIONAL LPBOOL pbWriteAccess ) ;
EXTERN_C BOOL WINAPI IsOfficeDocFile( IN LPCTSTR pszPath ) ;

EXTERN_C STDMETHODIMP GetPropServerClassForFile( IN LPCTSTR pszPath, BOOL bAdvanced, OUT LPCLSID pclsid ) ;
EXTERN_C STDMETHODIMP RegisterPropServerClassForExtension( IN LPCTSTR pszExt, REFCLSID refclsid ) ;



#endif __PTSNIFF_H__