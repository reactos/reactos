#include "shellprv.h"
#pragma  hdrstop

#include "drives.h"

#include "datautil.h"

const IDropTargetVtbl c_CDrivesDropTargetVtbl =
{
    CIDLDropTarget_QueryInterface, CIDLDropTarget_AddRef, CIDLDropTarget_Release,
    CDrivesIDLDropTarget_DragEnter,
    CIDLDropTarget_DragOver,
    CIDLDropTarget_DragLeave,
    CDrivesIDLDropTarget_Drop,
};
