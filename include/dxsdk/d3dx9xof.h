

#include "d3dx9.h"

#if !defined( __D3DX9XOF_H__ )
#define __D3DX9XOF_H__

#if defined( __cplusplus )
extern "C" {
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
DEFINE_GUID( IID_ID3DXFile,             0xcef08CF9, 0x7B4F, 0x4429, 0x96, 0x24, 0x2A, 0x69, 0x0A, 0x93, 0x32, 0x01 );
DEFINE_GUID( IID_ID3DXFileSaveObject,   0xcef08CFA, 0x7B4F, 0x4429, 0x96, 0x24, 0x2A, 0x69, 0x0A, 0x93, 0x32, 0x01 );
DEFINE_GUID( IID_ID3DXFileSaveData,     0xcef08CFB, 0x7B4F, 0x4429, 0x96, 0x24, 0x2A, 0x69, 0x0A, 0x93, 0x32, 0x01 );
DEFINE_GUID( IID_ID3DXFileEnumObject,   0xcef08CFC, 0x7B4F, 0x4429, 0x96, 0x24, 0x2A, 0x69, 0x0A, 0x93, 0x32, 0x01 );
DEFINE_GUID( IID_ID3DXFileData,         0xcef08CFD, 0x7B4F, 0x4429, 0x96, 0x24, 0x2A, 0x69, 0x0A, 0x93, 0x32, 0x01 );
#endif

#define D3DXF_FILEFORMAT_BINARY                     0
#define D3DXF_FILEFORMAT_TEXT                       1
#define D3DXF_FILEFORMAT_COMPRESSED                 2
#define D3DXF_FILESAVE_TOFILE                       0x00
#define D3DXF_FILESAVE_TOWFILE                      0x01
#define D3DXF_FILELOAD_FROMFILE                     0x00
#define D3DXF_FILELOAD_FROMWFILE                    0x01
#define D3DXF_FILELOAD_FROMRESOURCE                 0x02
#define D3DXF_FILELOAD_FROMMEMORY                   0x03
#define _FACD3DXF                                   0x876
#define D3DXFERR_BADOBJECT                          MAKE_HRESULT( 1, _FACD3DXF, 900 )
#define D3DXFERR_BADVALUE                           MAKE_HRESULT( 1, _FACD3DXF, 901 )
#define D3DXFERR_BADTYPE                            MAKE_HRESULT( 1, _FACD3DXF, 902 )
#define D3DXFERR_NOTFOUND                           MAKE_HRESULT( 1, _FACD3DXF, 903 )
#define D3DXFERR_NOTDONEYET                         MAKE_HRESULT( 1, _FACD3DXF, 904 )
#define D3DXFERR_FILENOTFOUND                       MAKE_HRESULT( 1, _FACD3DXF, 905 )
#define D3DXFERR_RESOURCENOTFOUND                   MAKE_HRESULT( 1, _FACD3DXF, 906 )
#define D3DXFERR_BADRESOURCE                        MAKE_HRESULT( 1, _FACD3DXF, 907 )
#define D3DXFERR_BADFILETYPE                        MAKE_HRESULT( 1, _FACD3DXF, 908 )
#define D3DXFERR_BADFILEVERSION                     MAKE_HRESULT( 1, _FACD3DXF, 909 )
#define D3DXFERR_BADFILEFLOATSIZE                   MAKE_HRESULT( 1, _FACD3DXF, 910 )
#define D3DXFERR_BADFILE                            MAKE_HRESULT( 1, _FACD3DXF, 911 )
#define D3DXFERR_PARSEERROR                         MAKE_HRESULT( 1, _FACD3DXF, 912 )
#define D3DXFERR_BADARRAYSIZE                       MAKE_HRESULT( 1, _FACD3DXF, 913 )
#define D3DXFERR_BADDATAREFERENCE                   MAKE_HRESULT( 1, _FACD3DXF, 914 )
#define D3DXFERR_NOMOREOBJECTS                      MAKE_HRESULT( 1, _FACD3DXF, 915 )
#define D3DXFERR_NOMOREDATA                         MAKE_HRESULT( 1, _FACD3DXF, 916 )
#define D3DXFERR_BADCACHEFILE                       MAKE_HRESULT( 1, _FACD3DXF, 917 )

typedef DWORD D3DXF_FILEFORMAT;
typedef DWORD D3DXF_FILESAVEOPTIONS;
typedef DWORD D3DXF_FILELOADOPTIONS;

typedef struct _D3DXF_FILELOADMEMORY
{
  LPCVOID lpMemory;
  SIZE_T dSize;
} D3DXF_FILELOADMEMORY;

typedef struct _D3DXF_FILELOADRESOURCE
{
  HMODULE hModule;
  LPCSTR lpName;
  LPCSTR lpType;
} D3DXF_FILELOADRESOURCE;


#if defined( __cplusplus )
    #if !defined( DECLSPEC_UUID )
        #if _MSC_VER >= 1100
            #define DECLSPEC_UUID( x ) __declspec( uuid( x ) )
        #else 
            #define DECLSPEC_UUID( x )
        #endif
    #endif

    #if defined( _COM_SMARTPTR_TYPEDEF )
        interface DECLSPEC_UUID( "CEF08CF9-7B4F-4429-9624-2A690A933201" ) ID3DXFile;
        interface DECLSPEC_UUID( "CEF08CFA-7B4F-4429-9624-2A690A933201" ) ID3DXFileSaveObject;
        interface DECLSPEC_UUID( "CEF08CFB-7B4F-4429-9624-2A690A933201" ) ID3DXFileSaveData;
        interface DECLSPEC_UUID( "CEF08CFC-7B4F-4429-9624-2A690A933201" ) ID3DXFileEnumObject;
        interface DECLSPEC_UUID( "CEF08CFD-7B4F-4429-9624-2A690A933201" ) ID3DXFileData;
    #endif
#endif

#undef INTERFACE
#define INTERFACE ID3DXFile
DECLARE_INTERFACE_( ID3DXFile, IUnknown )
{
  STDMETHOD( QueryInterface )( THIS_ REFIID, LPVOID* ) PURE;
  STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
  STDMETHOD_( ULONG, Release )( THIS ) PURE;
  STDMETHOD( CreateEnumObject )( THIS_ LPCVOID, D3DXF_FILELOADOPTIONS, ID3DXFileEnumObject** ) PURE;
  STDMETHOD( CreateSaveObject )( THIS_ LPCVOID, D3DXF_FILESAVEOPTIONS, D3DXF_FILEFORMAT, ID3DXFileSaveObject** ) PURE;
  STDMETHOD( RegisterTemplates )( THIS_ LPCVOID, SIZE_T ) PURE;
  STDMETHOD( RegisterEnumTemplates )( THIS_ ID3DXFileEnumObject* ) PURE;
};

#undef INTERFACE
#define INTERFACE ID3DXFileEnumObject
DECLARE_INTERFACE_( ID3DXFileEnumObject, IUnknown )
{
  STDMETHOD( QueryInterface )( THIS_ REFIID, LPVOID* ) PURE;
  STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
  STDMETHOD_( ULONG, Release )( THIS ) PURE;
  STDMETHOD( GetFile )( THIS_ ID3DXFile** ) PURE;
  STDMETHOD( GetChildren )( THIS_ SIZE_T* ) PURE;
  STDMETHOD( GetChild )( THIS_ SIZE_T, ID3DXFileData** ) PURE;
  STDMETHOD( GetDataObjectById )( THIS_ REFGUID, ID3DXFileData** ) PURE;
  STDMETHOD( GetDataObjectByName )( THIS_ LPCSTR, ID3DXFileData** ) PURE;
};


#undef INTERFACE
#define INTERFACE ID3DXFileData

DECLARE_INTERFACE_( ID3DXFileData, IUnknown )
{
  STDMETHOD( QueryInterface )( THIS_ REFIID, LPVOID* ) PURE;
  STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
  STDMETHOD_( ULONG, Release )( THIS ) PURE;
  STDMETHOD( GetEnum )( THIS_ ID3DXFileEnumObject** ) PURE;
  STDMETHOD( GetName )( THIS_ LPSTR, SIZE_T* ) PURE;
  STDMETHOD( GetId )( THIS_ LPGUID ) PURE;
  STDMETHOD( Lock )( THIS_ SIZE_T*, LPCVOID* ) PURE;
  STDMETHOD( Unlock )( THIS ) PURE;
  STDMETHOD( GetType )( THIS_ GUID* ) PURE;
  STDMETHOD_( BOOL, IsReference )( THIS ) PURE;
  STDMETHOD( GetChildren )( THIS_ SIZE_T* ) PURE;
  STDMETHOD( GetChild )( THIS_ SIZE_T, ID3DXFileData** ) PURE;
};

#undef INTERFACE
#define INTERFACE ID3DXFileSaveData
DECLARE_INTERFACE_( ID3DXFileSaveData, IUnknown )
{
  STDMETHOD( QueryInterface )( THIS_ REFIID, LPVOID* ) PURE;
  STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
  STDMETHOD_( ULONG, Release )( THIS ) PURE;
  STDMETHOD( GetSave )( THIS_ ID3DXFileSaveObject** ) PURE;
  STDMETHOD( GetName )( THIS_ LPSTR, SIZE_T* ) PURE;
  STDMETHOD( GetId )( THIS_ LPGUID ) PURE;
  STDMETHOD( GetType )( THIS_ GUID* ) PURE;
  STDMETHOD( AddDataObject )( THIS_ REFGUID, LPCSTR, CONST GUID*, SIZE_T, LPCVOID, ID3DXFileSaveData** ) PURE;
  STDMETHOD( AddDataReference )( THIS_ LPCSTR, CONST GUID* ) PURE;
};

#undef INTERFACE
#define INTERFACE ID3DXFileSaveObject
DECLARE_INTERFACE_( ID3DXFileSaveObject, IUnknown )
{
  STDMETHOD( QueryInterface )( THIS_ REFIID, LPVOID* ) PURE;
  STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
  STDMETHOD_( ULONG, Release )( THIS ) PURE;
  STDMETHOD( GetFile )( THIS_ ID3DXFile** ) PURE;
  STDMETHOD( AddDataObject )( THIS_ REFGUID, LPCSTR, CONST GUID*, SIZE_T, LPCVOID, ID3DXFileSaveData** ) PURE;
  STDMETHOD( Save )( THIS ) PURE;
};

#ifndef WIN_TYPES
#define WIN_TYPES(itype, ptype) typedef interface itype *LP##ptype, **LPLP##ptype
#endif
WIN_TYPES(ID3DXFile,                        D3DXFILE);
WIN_TYPES(ID3DXFileEnumObject,              D3DXFILEENUMOBJECT);
WIN_TYPES(ID3DXFileSaveObject,              D3DXFILESAVEOBJECT);
WIN_TYPES(ID3DXFileData,                    D3DXFILEDATA);
WIN_TYPES(ID3DXFileSaveData,                D3DXFILESAVEDATA);
#if defined( __cplusplus )
}
#endif

#endif


