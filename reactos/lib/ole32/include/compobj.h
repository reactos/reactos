#ifndef __WINE_OLE_COMPOBJ_H
#define __WINE_OLE_COMPOBJ_H

/* All private prototype functions used by OLE will be added to this header file */

/* This function initialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_Initialize();

/* This function uninitialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize();

/* This function decomposes a String path to a String Table containing all the elements ("\" or "subDirectory" or "Directory" or "FileName") of the path */
int WINAPI FileMonikerImpl_DecomposePath(LPOLESTR str, LPOLESTR** stringTable);

#endif /* __WINE_OLE_COMPOBJ_H */
