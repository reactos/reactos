#ifndef __WINE_OCIDL_H
#define __WINE_OCIDL_H

#include_next <ocidl.h>

DEFINE_GUID(IID_IFont, 0xbef6e002, 0xa874, 0x101a, 0x8b,0xba, 0x00,0xaa,0x00,0x30,0x0c,0xab);
DEFINE_GUID(IID_IFontDisp, 0xbef6e003, 0xa874, 0x101a, 0x8b,0xba, 0x00,0xaa,0x00,0x30,0x0c,0xab);
DEFINE_GUID(IID_IPicture, 0x7bf80980, 0xbf32, 0x101a, 0x8b,0xbb, 0x00,0xaa,0x00,0x30,0x0c,0xab);
DEFINE_GUID(IID_IPictureDisp, 0x7bf80981, 0xbf32, 0x101a, 0x8b,0xbb, 0x00,0xaa,0x00,0x30,0x0c,0xab);
DEFINE_GUID(IID_IOleControl, 0xb196b288, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);
DEFINE_GUID(IID_IOleControlSite, 0xb196b289, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

/*** IUnknown methods ***/
#define IOleControlSite_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleControlSite_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IOleControlSite_Release(p) (p)->lpVtbl->Release(p)
/*** IOleControlSite methods ***/
#define IOleControlSite_OnControlInfoChanged(p) (p)->lpVtbl->OnControlInfoChanged(p)
#define IOleControlSite_LockInPlaceActive(p,a) (p)->lpVtbl->LockInPlaceActive(p,a)
#define IOleControlSite_GetExtendedControl(p,a) (p)->lpVtbl->GetExtendedControl(p,a)
#define IOleControlSite_TransformCoords(p,a,b,c) (p)->lpVtbl->TransformCoords(p,a,b,c)
#define IOleControlSite_TranslateAccelerator(p,a,b) (p)->lpVtbl->TranslateAccelerator(p,a,b)
#define IOleControlSite_OnFocus(p,a) (p)->lpVtbl->OnFocus(p,a)
#define IOleControlSite_ShowPropertyFrame(p) (p)->lpVtbl->ShowPropertyFrame(p)

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

DEFINE_GUID(IID_IOleInPlaceSiteEx, 0x9c2cad80, 0x3424, 0x11cf, 0xb6,0x70, 0x00,0xaa,0x00,0x4c,0xd6,0xd8);
DEFINE_GUID(IID_IOleInPlaceSiteWindowless, 0x922eada0, 0x3424, 0x11cf, 0xb6,0x70, 0x00,0xaa,0x00,0x4c,0xd6,0xd8);
DEFINE_GUID(IID_IOleInPlaceObjectWindowless, 0x1c2056cc, 0x5ef4, 0x101b, 0x8b,0xc8, 0x00,0xaa,0x00,0x3e,0x3b,0x29);
DEFINE_GUID(IID_IClassFactory2, 0xb196b28f, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);
DEFINE_GUID(IID_IViewObjectEx, 0x3af24292, 0x0c96, 0x11ce, 0xa0,0xcf, 0x00,0xaa,0x00,0x60,0x0a,0xb8);
DEFINE_GUID(IID_IProvideClassInfo, 0xb196b283, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);
DEFINE_GUID(IID_IProvideClassInfo2, 0xa6bc3ac0, 0xdbaa, 0x11ce, 0x9d,0xe3, 0x00,0xaa,0x00,0x4b,0xb8,0x51);
DEFINE_GUID(IID_IConnectionPoint, 0xb196b286, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

/*** IUnknown methods ***/
#define IConnectionPoint_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IConnectionPoint_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IConnectionPoint_Release(p) (p)->lpVtbl->Release(p)
/*** IConnectionPoint methods ***/
#define IConnectionPoint_GetConnectionInterface(p,a) (p)->lpVtbl->GetConnectionInterface(p,a)
#define IConnectionPoint_GetConnectionPointContainer(p,a) (p)->lpVtbl->GetConnectionPointContainer(p,a)
#define IConnectionPoint_Advise(p,a,b) (p)->lpVtbl->Advise(p,a,b)
#define IConnectionPoint_Unadvise(p,a) (p)->lpVtbl->Unadvise(p,a)
#define IConnectionPoint_EnumConnections(p,a) (p)->lpVtbl->EnumConnections(p,a)

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

DEFINE_GUID(IID_IConnectionPointContainer, 0xb196b284, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

/*** IUnknown methods ***/
#define IConnectionPointContainer_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IConnectionPointContainer_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IConnectionPointContainer_Release(p) (p)->lpVtbl->Release(p)
/*** IConnectionPointContainer methods ***/
#define IConnectionPointContainer_EnumConnectionPoints(p,a) (p)->lpVtbl->EnumConnectionPoints(p,a)
#define IConnectionPointContainer_FindConnectionPoint(p,a,b) (p)->lpVtbl->FindConnectionPoint(p,a,b)

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

DEFINE_GUID(IID_IEnumConnections, 0xb196b287, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

/*** IUnknown methods ***/
#define IEnumConnections_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumConnections_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IEnumConnections_Release(p) (p)->lpVtbl->Release(p)
/*** IEnumConnections methods ***/
#define IEnumConnections_Next(p,a,b,c) (p)->lpVtbl->Next(p,a,b,c)
#define IEnumConnections_Skip(p,a) (p)->lpVtbl->Skip(p,a)
#define IEnumConnections_Reset(p) (p)->lpVtbl->Reset(p)
#define IEnumConnections_Clone(p,a) (p)->lpVtbl->Clone(p,a)

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

DEFINE_GUID(IID_IEnumConnectionPoints, 0xb196b285, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);
DEFINE_GUID(IID_IPropertyPage, 0xb196b28d, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);
DEFINE_GUID(IID_IPropertyPage2, 0x01e44665, 0x24ac, 0x101b, 0x84,0xed, 0x08,0x00,0x2b,0x2e,0xc7,0x13);
DEFINE_GUID(IID_IPropertyPageSite, 0xb196b28c, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);
DEFINE_GUID(IID_IPropertyNotifySink, 0x9bfbbc02, 0xeff1, 0x101a, 0x84,0xed, 0x00,0xaa,0x00,0x34,0x1d,0x07);

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

/*** IUnknown methods ***/
#define IPropertyNotifySink_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IPropertyNotifySink_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IPropertyNotifySink_Release(p) (p)->lpVtbl->Release(p)
/*** IPropertyNotifySink methods ***/
#define IPropertyNotifySink_OnChanged(p,a) (p)->lpVtbl->OnChanged(p,a)
#define IPropertyNotifySink_OnRequestEdit(p,a) (p)->lpVtbl->OnRequestEdit(p,a)

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

DEFINE_GUID(IID_ISimpleFrameSite, 0x742b0e01, 0x14e6, 0x101b, 0x91,0x4e, 0x00,0xaa,0x00,0x30,0x0c,0xab);
DEFINE_GUID(IID_IPersistStreamInit, 0x7fd52380, 0x4e07, 0x101b, 0xae,0x2d, 0x08,0x00,0x2b,0x2e,0xc7,0x13);
DEFINE_GUID(IID_IPersistMemory, 0xbd1ae5e0, 0xa6ae, 0x11ce, 0xbd,0x37, 0x50,0x42,0x00,0xc1,0x00,0x00);
DEFINE_GUID(IID_IPersistPropertyBag, 0x37d84f60, 0x42cb, 0x11ce, 0x81,0x35, 0x00,0xaa,0x00,0x4b,0xb8,0x51);

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

/*** IUnknown methods ***/
#define IPersistPropertyBag_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IPersistPropertyBag_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IPersistPropertyBag_Release(p) (p)->lpVtbl->Release(p)
/*** IPersist methods ***/
#define IPersistPropertyBag_GetClassID(p,a) (p)->lpVtbl->GetClassID(p,a)
/*** IPersistPropertyBag methods ***/
#define IPersistPropertyBag_InitNew(p) (p)->lpVtbl->InitNew(p)
#define IPersistPropertyBag_Load(p,a,b) (p)->lpVtbl->Load(p,a,b)
#define IPersistPropertyBag_Save(p,a,b,c) (p)->lpVtbl->Save(p,a,b,c)

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

DEFINE_GUID(IID_IPropertyBag2, 0x22f55882, 0x280b, 0x11d0, 0xa8,0xa9, 0x00,0xa0,0xc9,0x0c,0x20,0x04);
DEFINE_GUID(IID_IPersistPropertyBag2, 0x22f55881, 0x280b, 0x11d0, 0xa8,0xa9, 0x00,0xa0,0xc9,0x0c,0x20,0x04);
DEFINE_GUID(IID_ISpecifyPropertyPages, 0xb196b28b, 0xbab4, 0x101a, 0xb6,0x9c, 0x00,0xaa,0x00,0x34,0x1d,0x07);
DEFINE_GUID(IID_IPerPropertyBrowsing, 0x376bd3aa, 0x3845, 0x101b, 0x84,0xed, 0x08,0x00,0x2b,0x2e,0xc7,0x13);
DEFINE_GUID(IID_IAdviseSinkEx, 0x3af24290, 0x0c96, 0x11ce, 0xa0,0xcf, 0x00,0xaa,0x00,0x60,0x0a,0xb8);
DEFINE_GUID(IID_IPointerInactive, 0x55980ba0, 0x35aa, 0x11cf, 0xb6,0x71, 0x00,0xaa,0x00,0x4c,0xd6,0xd8);
DEFINE_GUID(IID_IObjectWithSite, 0xfc4801a3, 0x2ba9, 0x11cf, 0xa2,0x29, 0x00,0xaa,0x00,0x3d,0x73,0x52);
DEFINE_GUID(IID_IOleUndoUnit, 0x894ad3b0, 0xef97, 0x11ce, 0x9b,0xc9, 0x00,0xaa,0x00,0x60,0x8e,0x01);
DEFINE_GUID(IID_IOleParentUndoUnit, 0xa1faf330, 0xef97, 0x11ce, 0x9b,0xc9, 0x00,0xaa,0x00,0x60,0x8e,0x01);
DEFINE_GUID(IID_IEnumOleUndoUnits, 0xb3e7c340, 0xef97, 0x11ce, 0x9b,0xc9, 0x00,0xaa,0x00,0x60,0x8e,0x01);
DEFINE_GUID(IID_IOleUndoManager, 0xd001f200, 0xef97, 0x11ce, 0x9b,0xc9, 0x00,0xaa,0x00,0x60,0x8e,0x01);
DEFINE_GUID(IID_IQuickActivate, 0xcf51ed10, 0x62fe, 0x11cf, 0xbf,0x86, 0x00,0xa0,0xc9,0x03,0x48,0x36);

#endif  /* __WINE_OAIDL_H */
