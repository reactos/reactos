// header file for stuff that really should be
// in printer.h, but can't be due to our two different

typedef struct {
    HWND	    hwnd;
    IDataObject *pDataObj;
    IStream *pstmDataObj;       // to marshall the data object
    DWORD	    dwEffect;
    POINT	    ptDrop;
    LPITEMIDLIST    pidl;	// relative pidl of printer printing to
} PRNTHREADPARAM;

void FreePrinterThreadParam(PRNTHREADPARAM *pthp);

DWORD CALLBACK CPrintObj_DropThreadProc(void *pv);
HRESULT PrintObj_DropPrint(IDataObject *pDataObj, HWND hwnd, DWORD dwEffect, LPCITEMIDLIST pidl, LPTHREAD_START_ROUTINE pfn);

STDMETHODIMP CPrintObjs_DragEnter(IDropTarget *pdt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
STDMETHODIMP CPrintObjs_DropCallback(IDropTarget *pdt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect, LPTHREAD_START_ROUTINE lpfn);

