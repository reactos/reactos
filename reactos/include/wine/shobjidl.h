/* $Id: shobjidl.h,v 1.4 2004/06/29 13:40:40 gvg Exp $
 *
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */


#ifndef __WINE_SHOBJIDL_H
#define __WINE_SHOBJIDL_H

#define CMIC_MASK_ASYNCOK        SEE_MASK_ASYNCOK

/*****************************************************************************
 * IDropTargetHelper interface
 */
DEFINE_GUID(IID_IDropTargetHelper, 0x4657278b, 0x411b, 0x11d2, 0x83,0x9a, 0x00,0xc0,0x4f,0xd9,0x18,0xd0);

/*****************************************************************************
 * IPersistFolder2 interface
 */
DEFINE_GUID(IID_IPersistFolder2, 0x1ac3d9f0, 0x175c, 0x11d1, 0x95,0xbe, 0x00,0x60,0x97,0x97,0xea,0x4f);

/*****************************************************************************
 * IShellFolder2 interface
 */
DEFINE_GUID(IID_IShellFolder2, 0x93f2f68c, 0x1d1b, 0x11d3, 0xa3,0x0e, 0x00,0xc0,0x4f,0x79,0xab,0xd1);

/*****************************************************************************
 * IPersistFolder3 interface
 */

/*****************************************************************************
 * IShellExecuteHookW interface
 */
DEFINE_GUID(IID_IPersistFolder3, 0xcef04fdf, 0xfe72, 0x11d2, 0x87,0xa5, 0x00,0xc0,0x4f,0x68,0x37,0xcf);
/*** IUnknown methods ***/
#define IShellExecuteHookW_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellExecuteHookW_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IShellExecuteHookW_Release(p) (p)->lpVtbl->Release(p)
/*** IShellExecuteHookW methods ***/
#define IShellExecuteHookW_Execute(p,a) (p)->lpVtbl->Execute(p,a)

#endif /* __WINE_SHOBJIDL_H */
