#ifndef __WINE_OLEIDL_H
#define __WINE_OLEIDL_H

#include <unknwn.h>
#include_next <oleidl.h>

DEFINE_GUID(IID_IOleWindow, 0x00000114, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IOleWindow_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleWindow_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IOleWindow_Release(p) (p)->lpVtbl->Release(p)
/*** IOleWindow methods ***/
#define IOleWindow_GetWindow(p,a) (p)->lpVtbl->GetWindow(p,a)
#define IOleWindow_ContextSensitiveHelp(p,a) (p)->lpVtbl->ContextSensitiveHelp(p,a)

DEFINE_GUID(IID_IOleInPlaceObject, 0x00000113, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IOleInPlaceActiveObject, 0x00000117, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IOleInPlaceUIWindow, 0x00000115, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IOleInPlaceFrame, 0x00000116, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IOleInPlaceFrame_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleInPlaceFrame_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IOleInPlaceFrame_Release(p) (p)->lpVtbl->Release(p)
/*** IOleWindow methods ***/
#define IOleInPlaceFrame_GetWindow(p,a) (p)->lpVtbl->GetWindow(p,a)
#define IOleInPlaceFrame_ContextSensitiveHelp(p,a) (p)->lpVtbl->ContextSensitiveHelp(p,a)
/*** IOleInPlaceUIWindow methods ***/
#define IOleInPlaceFrame_GetBorder(p,a) (p)->lpVtbl->GetBorder(p,a)
#define IOleInPlaceFrame_RequestBorderSpace(p,a) (p)->lpVtbl->RequestBorderSpace(p,a)
#define IOleInPlaceFrame_SetBorderSpace(p,a) (p)->lpVtbl->SetBorderSpace(p,a)
#define IOleInPlaceFrame_SetActiveObject(p,a,b) (p)->lpVtbl->SetActiveObject(p,a,b)
/*** IOleInPlaceFrame methods ***/
#define IOleInPlaceFrame_InsertMenus(p,a,b) (p)->lpVtbl->InsertMenus(p,a,b)
#define IOleInPlaceFrame_SetMenu(p,a,b,c) (p)->lpVtbl->SetMenu(p,a,b,c)
#define IOleInPlaceFrame_RemoveMenus(p,a) (p)->lpVtbl->RemoveMenus(p,a)
#define IOleInPlaceFrame_SetStatusText(p,a) (p)->lpVtbl->SetStatusText(p,a)
#define IOleInPlaceFrame_EnableModeless(p,a) (p)->lpVtbl->EnableModeless(p,a)
#define IOleInPlaceFrame_TranslateAccelerator(p,a,b) (p)->lpVtbl->TranslateAccelerator(p,a,b)

DEFINE_GUID(IID_IOleInPlaceSite, 0x00000119, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IParseDisplayName, 0x0000011a, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IParseDisplayName_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IParseDisplayName_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IParseDisplayName_Release(p) (p)->lpVtbl->Release(p)
/*** IParseDisplayName methods ***/
#define IParseDisplayName_ParseDisplayName(p,a,b,c,d) (p)->lpVtbl->ParseDisplayName(p,a,b,c,d)

DEFINE_GUID(IID_IOleContainer, 0x0000011b, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IOleItemContainer, 0x0000011c, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IOleItemContainer_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleItemContainer_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IOleItemContainer_Release(p) (p)->lpVtbl->Release(p)
/*** IParseDisplayName methods ***/
#define IOleItemContainer_ParseDisplayName(p,a,b,c,d) (p)->lpVtbl->ParseDisplayName(p,a,b,c,d)
/*** IOleContainer methods ***/
#define IOleItemContainer_EnumObjects(p,a,b) (p)->lpVtbl->EnumObjects(p,a,b)
#define IOleItemContainer_LockContainer(p,a) (p)->lpVtbl->LockContainer(p,a)
/*** IOleItemContainer methods ***/
#define IOleItemContainer_GetObject(p,a,b,c,d,e) (p)->lpVtbl->GetObject(p,a,b,c,d,e)
#define IOleItemContainer_GetObjectStorage(p,a,b,c,d) (p)->lpVtbl->GetObjectStorage(p,a,b,c,d)
#define IOleItemContainer_IsRunning(p,a) (p)->lpVtbl->IsRunning(p,a)

DEFINE_GUID(IID_IOleLink, 0x0000011d, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IOleClientSite, 0x00000118, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IOleClientSite_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleClientSite_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IOleClientSite_Release(p) (p)->lpVtbl->Release(p)
/*** IOleClientSite methods ***/
#define IOleClientSite_SaveObject(p) (p)->lpVtbl->SaveObject(p)
#define IOleClientSite_GetMoniker(p,a,b,c) (p)->lpVtbl->GetMoniker(p,a,b,c)
#define IOleClientSite_GetContainer(p,a) (p)->lpVtbl->GetContainer(p,a)
#define IOleClientSite_ShowObject(p) (p)->lpVtbl->ShowObject(p)
#define IOleClientSite_OnShowWindow(p,a) (p)->lpVtbl->OnShowWindow(p,a)
#define IOleClientSite_RequestNewObjectLayout(p) (p)->lpVtbl->RequestNewObjectLayout(p)

DEFINE_GUID(IID_IOleCache, 0x0000011e, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IOleCache2, 0x00000128, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IOleCacheControl, 0x00000129, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IEnumOLEVERB, 0x00000104, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IOleObject, 0x00000112, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IOleObject_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleObject_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IOleObject_Release(p) (p)->lpVtbl->Release(p)
/*** IOleObject methods ***/
#define IOleObject_SetClientSite(p,a) (p)->lpVtbl->SetClientSite(p,a)
#define IOleObject_GetClientSite(p,a) (p)->lpVtbl->GetClientSite(p,a)
#define IOleObject_SetHostNames(p,a,b) (p)->lpVtbl->SetHostNames(p,a,b)
#define IOleObject_Close(p,a) (p)->lpVtbl->Close(p,a)
#define IOleObject_SetMoniker(p,a,b) (p)->lpVtbl->SetMoniker(p,a,b)
#define IOleObject_GetMoniker(p,a,b,c) (p)->lpVtbl->GetMoniker(p,a,b,c)
#define IOleObject_InitFromData(p,a,b,c) (p)->lpVtbl->InitFromData(p,a,b,c)
#define IOleObject_GetClipboardData(p,a,b) (p)->lpVtbl->GetClipboardData(p,a,b)
#define IOleObject_DoVerb(p,a,b,c,d,e,f) (p)->lpVtbl->DoVerb(p,a,b,c,d,e,f)
#define IOleObject_EnumVerbs(p,a) (p)->lpVtbl->EnumVerbs(p,a)
#define IOleObject_Update(p) (p)->lpVtbl->Update(p)
#define IOleObject_IsUpToDate(p) (p)->lpVtbl->IsUpToDate(p)
#define IOleObject_GetUserClassID(p,a) (p)->lpVtbl->GetUserClassID(p,a)
#define IOleObject_GetUserType(p,a,b) (p)->lpVtbl->GetUserType(p,a,b)
#define IOleObject_SetExtent(p,a,b) (p)->lpVtbl->SetExtent(p,a,b)
#define IOleObject_GetExtent(p,a,b) (p)->lpVtbl->GetExtent(p,a,b)
#define IOleObject_Advise(p,a,b) (p)->lpVtbl->Advise(p,a,b)
#define IOleObject_Unadvise(p,a) (p)->lpVtbl->Unadvise(p,a)
#define IOleObject_EnumAdvise(p,a) (p)->lpVtbl->EnumAdvise(p,a)
#define IOleObject_GetMiscStatus(p,a,b) (p)->lpVtbl->GetMiscStatus(p,a,b)
#define IOleObject_SetColorScheme(p,a) (p)->lpVtbl->SetColorScheme(p,a)

DEFINE_GUID(IID_IOleAdviseHolder, 0x00000111, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IOleAdviseHolder_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleAdviseHolder_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IOleAdviseHolder_Release(p) (p)->lpVtbl->Release(p)
/*** IOleAdviseHolder methods ***/
#define IOleAdviseHolder_Advise(p,a,b) (p)->lpVtbl->Advise(p,a,b)
#define IOleAdviseHolder_Unadvise(p,a) (p)->lpVtbl->Unadvise(p,a)
#define IOleAdviseHolder_EnumAdvise(p,a) (p)->lpVtbl->EnumAdvise(p,a)
#define IOleAdviseHolder_SendOnRename(p,a) (p)->lpVtbl->SendOnRename(p,a)
#define IOleAdviseHolder_SendOnSave(p) (p)->lpVtbl->SendOnSave(p)
#define IOleAdviseHolder_SendOnClose(p) (p)->lpVtbl->SendOnClose(p)

DEFINE_GUID(IID_IContinue, 0x0000012a, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IViewObject, 0x0000010d, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IViewObject_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IViewObject_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IViewObject_Release(p) (p)->lpVtbl->Release(p)
/*** IViewObject methods ***/
#define IViewObject_Draw(p,a,b,c,d,e,f,g,h,i,j) (p)->lpVtbl->Draw(p,a,b,c,d,e,f,g,h,i,j)
#define IViewObject_GetColorSet(p,a,b,c,d,e,f) (p)->lpVtbl->GetColorSet(p,a,b,c,d,e,f)
#define IViewObject_Freeze(p,a,b,c,d) (p)->lpVtbl->Freeze(p,a,b,c,d)
#define IViewObject_Unfreeze(p,a) (p)->lpVtbl->Unfreeze(p,a)
#define IViewObject_SetAdvise(p,a,b,c) (p)->lpVtbl->SetAdvise(p,a,b,c)
#define IViewObject_GetAdvise(p,a,b,c) (p)->lpVtbl->GetAdvise(p,a,b,c)

DEFINE_GUID(IID_IViewObject2, 0x00000127, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IViewObject2_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IViewObject2_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IViewObject2_Release(p) (p)->lpVtbl->Release(p)
/*** IViewObject methods ***/
#define IViewObject2_Draw(p,a,b,c,d,e,f,g,h,i,j) (p)->lpVtbl->Draw(p,a,b,c,d,e,f,g,h,i,j)
#define IViewObject2_GetColorSet(p,a,b,c,d,e,f) (p)->lpVtbl->GetColorSet(p,a,b,c,d,e,f)
#define IViewObject2_Freeze(p,a,b,c,d) (p)->lpVtbl->Freeze(p,a,b,c,d)
#define IViewObject2_Unfreeze(p,a) (p)->lpVtbl->Unfreeze(p,a)
#define IViewObject2_SetAdvise(p,a,b,c) (p)->lpVtbl->SetAdvise(p,a,b,c)
#define IViewObject2_GetAdvise(p,a,b,c) (p)->lpVtbl->GetAdvise(p,a,b,c)
/*** IViewObject2 methods ***/
#define IViewObject2_GetExtent(p,a,b,c,d) (p)->lpVtbl->GetExtent(p,a,b,c,d)

DEFINE_GUID(IID_IDropSource, 0x00000121, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IDropSource_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDropSource_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IDropSource_Release(p) (p)->lpVtbl->Release(p)
/*** IDropSource methods ***/
#define IDropSource_QueryContinueDrag(p,a,b) (p)->lpVtbl->QueryContinueDrag(p,a,b)
#define IDropSource_GiveFeedback(p,a) (p)->lpVtbl->GiveFeedback(p,a)

DEFINE_GUID(IID_IDropTarget, 0x00000122, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IDropTarget_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDropTarget_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IDropTarget_Release(p) (p)->lpVtbl->Release(p)
/*** IDropTarget methods ***/
#define IDropTarget_DragEnter(p,a,b,c,d) (p)->lpVtbl->DragEnter(p,a,b,c,d)
#define IDropTarget_DragOver(p,a,b,c) (p)->lpVtbl->DragOver(p,a,b,c)
#define IDropTarget_DragLeave(p) (p)->lpVtbl->DragLeave(p)
#define IDropTarget_Drop(p,a,b,c,d) (p)->lpVtbl->Drop(p,a,b,c,d)

#ifndef __IOleCacheControl_FWD_DEFINED__
#define __IOleCacheControl_FWD_DEFINED__
typedef struct IOleCacheControl IOleCacheControl;
#endif

typedef IOleCacheControl *LPOLECACHECONTROL;

#endif  /* __WINE_OLEIDL_H */
