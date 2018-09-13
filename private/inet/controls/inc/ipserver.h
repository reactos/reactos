//=--------------------------------------------------------------------------=
// IPServer.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// global header file that contains all the windows stuff, etc ...  should
// be pre-compiled to speed things up a little bit.
//
#ifndef _IPSERVER_H_

//#define INC_OLE2
#include <windows.h>
#include <stddef.h>                    // for offsetof()
#include <olectl.h>

// things that -everybody- wants [read: is going to get]
//
#include "Debug.H"

//=--------------------------------------------------------------------------=
// controls can register for thread notifications in their InitializeLibrary()
//
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void (CALLBACK *THRDNFYPROC)(HANDLE, DWORD, void *);
void SetLibraryThreadProc(THRDNFYPROC pfnThreadNotify);

#ifdef __cplusplus
}
#endif // __cplusplus
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// we don't want to use the CRTs, and would like some memory tracking in the
// debug case, so we'll override these guys
//=--------------------------------------------------------------------------=
//
//void * _cdecl operator new(size_t size);
//void  _cdecl operator delete(void *ptr);


//=--------------------------------------------------------------------------=
// Useful macros
//=--------------------------------------------------------------------------=
//
// handy error macros, randing from cleaning up, to returning to clearing
// rich error information as well.
//
#define RETURN_ON_FAILURE(hr) if (FAILED(hr)) return hr
#define RETURN_ON_NULLALLOC(ptr) if (!(ptr)) return E_OUTOFMEMORY
#define CLEANUP_ON_FAILURE(hr) if (FAILED(hr)) goto CleanUp
#define CLEARERRORINFORET(hr) { SetErrorInfo(0, NULL); return hr; }
#define CLEARERRORINFORET_ON_FAILURE(hr) if (FAILED(hr)) { SetErrorInfo(0, NULL); return hr; }

#define CLEANUP_ON_ERROR(l)    if (l != ERROR_SUCCESS) goto CleanUp

// conversions
//
#define BOOL_TO_VARIANTBOOL(f) (f) ? VARIANT_TRUE : VARIANT_FALSE

// Reference counting help.
//
#define RELEASE_OBJECT(ptr)    if (ptr) { IUnknown *pUnk = (ptr); (ptr) = NULL; pUnk->Release(); }
#define QUICK_RELEASE(ptr)     if (ptr) ((IUnknown *)ptr)->Release();
#define ADDREF_OBJECT(ptr)     if (ptr) (ptr)->AddRef()



//=--------------------------------------------------------------------------=
// QueryInterface Optimizations
//=--------------------------------------------------------------------------=
// for optimizing QI's
//
#define DO_GUIDS_MATCH(riid1, riid2) ((riid1.Data1 == riid2.Data1) && (riid1 == riid2))

// Data1_*
//
// the first dword of GUIDs for most of the interesting interfaces.  these are
// used by speed critical versions of QueryInterface
//
#define Data1_IAdviseSink                  0x0000010f
#define Data1_IAdviseSink2                 0x00000125
#define Data1_IAdviseSinkEx                0x3af24290
#define Data1_IBindCtx                     0x0000000e
#define Data1_ICDataDoc                    0xF413E4C0
#define Data1_IClassFactory                0x00000001
#define Data1_IClassFactory2               0xb196b28f
#define Data1_IConnectionPoint             0xb196b286
#define Data1_IConnectionPointContainer    0xb196b284
#define Data1_IControl_95                  0x9a4bbfb5
#define Data1_IControl                     0xa7fddba0
#define Data1_ICreateErrorInfo             0x22f03340
#define Data1_ICreateTypeInfo              0x00020405
#define Data1_ICreateTypeLib               0x00020406
#define Data1_IDataAdviseHolder            0x00000110
#define Data1_IDataFrame                   0x97F254E0
#define Data1_IDataFrameExpert             0x73687490
#define Data1_IDataObject                  0x0000010e
#define Data1_IDispatch                    0x00020400
#define Data1_IDropSource                  0x00000121
#define Data1_IDropTarget                  0x00000122
#define Data1_IEnumCallback                0x00000108
#define Data1_IEnumConnectionPoints        0xb196b285
#define Data1_IEnumConnections             0xb196b287
#define Data1_IEnumFORMATETC               0x00000103
#define Data1_IEnumGeneric                 0x00000106
#define Data1_IEnumHolder                  0x00000107
#define Data1_IEnumMoniker                 0x00000102
#define Data1_IEnumOLEVERB                 0x00000104
#define Data1_IEnumSTATDATA                0x00000105
#define Data1_IEnumSTATSTG                 0x0000000d
#define Data1_IEnumString                  0x00000101
#define Data1_IEnumOleUndoActions          0xb3e7c340
#define Data1_IEnumUnknown                 0x00000100
#define Data1_IEnumVARIANT                 0x00020404
#define Data1_IErrorInfo                   0x1cf2b120
#define Data1_IExternalConnection          0x00000019
#define Data1_IFont                        0xbef6e002
#define Data1_IFontDisp                    0xbef6e003
#define Data1_IFormExpert                  0x5aac7f70
#define Data1_IGangConnectWithDefault      0x6d5140c0
#define Data1_IInternalMoniker             0x00000011
#define Data1_ILockBytes                   0x0000000a
#define Data1_IMalloc                      0x00000002
#define Data1_IMarshal                     0x00000003
#define Data1_IMessageFilter               0x00000016
#define Data1_IMoniker                     0x0000000f
#define Data1_IMsoCommandTarget            0xb722bccb
#define Data1_IMsoDocument                 0xb722bcc5
#define Data1_IOleInPlaceComponent         0x5efc7970
#define Data1_IMsoView                     0xb722bcc6
#define Data1_IOleAdviseHolder             0x00000111
#define Data1_IOleCache                    0x0000011e
#define Data1_IOleCache2                   0x00000128
#define Data1_IOleCacheControl             0x00000129
#define Data1_IOleClientSite               0x00000118
#define Data1_IOleCompoundUndoAction       0xa1faf330
#define Data1_IOleContainer                0x0000011b
#define Data1_IOleControl                  0xb196b288
#define Data1_IOleControlSite              0xb196b289
#define Data1_IOleInPlaceActiveObject      0x00000117
#define Data1_IOleInPlaceFrame             0x00000116
#define Data1_IOleInPlaceObject            0x00000113
#define Data1_IOleInPlaceObjectWindowless  0x1c2056cc
#define Data1_IOleInPlaceSite              0x00000119
#define Data1_IOleInPlaceSiteEx            0x9c2cad80
#define Data1_IOleInPlaceSiteWindowless    0x922eada0
#define Data1_IOleInPlaceUIWindow          0x00000115
#define Data1_IOleItemContainer            0x0000011c
#define Data1_IOleLink                     0x0000011d
#define Data1_IOleManager                  0x0000011f
#define Data1_IOleObject                   0x00000112
#define Data1_IOlePresObj                  0x00000120
#define Data1_IOlePropertyFrame            0xb83bb801
#define Data1_IOleStandardTool             0xd97877c4
#define Data1_IOleUndoAction               0x894ad3b0
#define Data1_IOleUndoActionManager        0xd001f200
#define Data1_IOleWindow                   0x00000114
#define Data1_IPSFactory                   0x00000009
#define Data1_IPSFactoryBuffer             0xd5f569d0
#define Data1_IParseDisplayName            0x0000011a
#define Data1_IPerPropertyBrowsing         0x376bd3aa
#define Data1_IPersist                     0x0000010c
#define Data1_IPersistFile                 0x0000010b
#define Data1_IPersistPropertyBag          0x37D84F60
#define Data1_IPersistStorage              0x0000010a
#define Data1_IPersistStream               0x00000109
#define Data1_IPersistStreamInit           0x7fd52380
#define Data1_IPicture                     0x7bf80980
#define Data1_IPictureDisp                 0x7bf80981
#define Data1_IPointerInactive             0x55980ba0
#define Data1_IPropertyNotifySink          0x9bfbbc02
#define Data1_IPropertyPage                0xb196b28d
#define Data1_IPropertyPage2               0x01e44665
#define Data1_IPropertyPage3               0xb83bb803
#define Data1_IPropertyPageInPlace         0xb83bb802
#define Data1_IPropertyPageSite            0xb196b28c
#define Data1_IPropertyPageSite2           0xb83bb804
#define Data1_IProvideClassInfo            0xb196b283
#define Data1_IProvideDynamicClassInfo     0x6d5140d1
#define Data1_IQuickActivate               0xcf51ed10
#define Data1_IRequireClasses              0x6d5140d0
#define Data1_IRootStorage                 0x00000012
#define Data1_IRunnableObject              0x00000126
#define Data1_IRunningObjectTable          0x00000010
#define Data1_ISelectionContainer          0x6d5140c6
#define Data1_IServiceProvider             0x6d5140c1
#define Data1_ISimpleFrameSite             0x742b0e01
#define Data1_ISpecifyPropertyPages        0xb196b28b
#define Data1_IStdMarshalInfo              0x00000018
#define Data1_IStorage                     0x0000000b
#define Data1_IStream                      0x0000000c
#define Data1_ISupportErrorInfo            0xdf0b3d60
#define Data1_ITypeComp                    0x00020403
#define Data1_ITypeInfo                    0x00020401
#define Data1_ITypeLib                     0x00020402
#define Data1_IUnknown                     0x00000000
#define Data1_IViewObject                  0x0000010d
#define Data1_IViewObject2                 0x00000127
#define Data1_IViewObjectEx                0x3af24292
#define Data1_IWeakRef                     0x0000001a
#define Data1_ICategorizeProperties        0x4d07fc10


#define QI_INHERITS(pObj, itf)              \
    case Data1_##itf:                       \
      if(DO_GUIDS_MATCH(riid, IID_##itf))    \
      {                                     \
        *ppvObjOut = (void *)(itf *)pObj;   \
      }                                     \
      break;

#include "extobj.h"


#define _IPSERVER_H_
#endif // _IPSERVER_H_
