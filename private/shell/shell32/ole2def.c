// stubs for OLE routines that we used to delay load or implement ourselves

#include "shellprv.h"
#pragma  hdrstop

STDAPI SHFlushClipboard(void)
{
    return OleFlushClipboard();
}

// we should not use these anymore, just call the OLE32 versions

STDAPI SHRegisterDragDrop(HWND hwnd, IDropTarget *pDropTarget)
{
    return RegisterDragDrop(hwnd, pDropTarget);
}

STDAPI SHRevokeDragDrop(HWND hwnd)
{
    return RevokeDragDrop(hwnd);
}

STDAPI_(void) SHFreeUnusedLibraries()
{
    CoFreeUnusedLibraries();
}

STDAPI SHLoadOLE(LPARAM lParam)
{
    return S_OK;
}

