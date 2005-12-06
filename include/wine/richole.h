/*
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#include <richedit.h>

#include_next <richole.h>

#ifndef WINE_RICHOLE_H_INCLUDED
#define WINE_RICHOLE_H_INCLUDED

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IRichEditOle_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IRichEditOle_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IRichEditOle_Release(p) (p)->lpVtbl->Release(p)
/*** IRichEditOle methods ***/
#define IRichEditOle_GetClientSite(p,a) (p)->lpVtbl->GetClientSite(p,a)
#define IRichEditOle_GetObjectCount(p) (p)->lpVtbl->GetObjectCount(p)
#define IRichEditOle_GetLinkCount(p) (p)->lpVtbl->GetLinkCount(p)
#define IRichEditOle_GetObject(p,a,b,c) (p)->lpVtbl->GetObject(p,a,b,c)
#define IRichEditOle_InsertObject(p,a) (p)->lpVtbl->InsertObject(p,a)
#define IRichEditOle_ConvertObject(p,a,b,c) (p)->lpVtbl->ConvertObject(p,a,b,c)
#define IRichEditOle_ActivateAs(p,a,b) (p)->lpVtbl->ActivateAs(p,a,b)
#define IRichEditOle_SetHostNames(p,a,b) (p)->lpVtbl->SetHostNames(p,a,b)
#define IRichEditOle_SetLinkAvailable(p,a,b) (p)->lpVtbl->SetLinkAvailable(p,a,b)
#define IRichEditOle_SetDvaspect(p,a,b) (p)->lpVtbl->SetDvaspect(p,a,b)
#define IRichEditOle_HandsOffStorage(p,a) (p)->lpVtbl->HandsOffStorage(p,a)
#define IRichEditOle_SaveCompleted(p,a,b) (p)->lpVtbl->SaveCompleted(p,a,b)
#define IRichEditOle_InPlaceDeactivate(p) (p)->lpVtbl->InPlaceDeactivate(p)
#define IRichEditOle_ContextSensitiveHelp(p,a) (p)->lpVtbl->ContextSensitiveHelp(p,a)
#define IRichEditOle_GetClipboardData(p,a,b,c) (p)->lpVtbl->GetClipboardData(p,a,b,c)
#define IRichEditOle_ImportDataObject(p,a,b,c) (p)->lpVtbl->ImportDataObject(p,a,b,c)
#endif

#endif /* WINE_RICHOLE_H_INCLUDED */
