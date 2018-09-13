//  OLESTUFF.H
//  Prototypes for OLE delay-load stuff needed for the toolbar and tab
//  drop target classes.
//
//  History:
//      8/22/96 -   t-mkim: created
//
#ifndef _OLESTUFF_H
#define _OLESTUFF_H

// This deals with the OLE library module handle
//
HMODULE PrivLoadOleLibrary ();
BOOL    PrivFreeOleLibrary (HMODULE hmodOle);

//  Following functions correspond to CoInitialize, CoUninitialize,
//  RegisterDragDrop, and RevokeDragDrop. All take the HMODULE returned
//  by PrivLoadOleLibrary.
//
HRESULT PrivCoInitialize (HMODULE hmodOle);
void    PrivCoUninitialize (HMODULE hmodOle);
HRESULT PrivRegisterDragDrop (HMODULE hmodOle, HWND hwnd, IDropTarget *pDropTarget);
HRESULT PrivRevokeDragDrop (HMODULE hmodOle, HWND hwnd);

#endif //_OLESTUFF_H
