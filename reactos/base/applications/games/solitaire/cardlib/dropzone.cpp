//
//    CardLib - DropZone class
//
//    Freeware
//    Copyright J Brown 2001
//
#include <windows.h>

#include "cardlib.h"
#include "cardwindow.h"
#include "dropzone.h"

bool CardWindow::RegisterDropZone(int id, RECT *rect, pDropZoneProc proc)
{
    if(nNumDropZones == MAXDROPZONES)
        return false;

    DropZone *dz = new DropZone(id, rect, proc);
    
    dropzone[nNumDropZones++] = dz;

    return false;
}

DropZone *CardWindow::GetDropZoneFromRect(RECT *rect)
{
    for(int i = 0; i < nNumDropZones; i++)
    {
        RECT inter;
        RECT zone;
                
        //if any part of the drag rectangle falls within a drop zone,
        //let that take priority over any other card stack.
        dropzone[i]->GetZone(&zone);

        if(IntersectRect(&inter, rect, &zone))
        {
            //see if the callback wants us to drop a card on
            //a particular stack
            return dropzone[i];
        }
    }

    return 0;
}

bool CardWindow::DeleteDropZone(int id)
{
    for(int i = 0; i < nNumDropZones; i++)
    {
        if(dropzone[i]->id == id)
        {
            DropZone *dz = dropzone[i];
            
            //shift any after this one backwards
            for(int j = i; j < nNumDropZones - 1; j++)
            {
                dropzone[j] = dropzone[j + 1];
            }

            delete dz;
            nNumDropZones--;
            return true;
        }    
    }

    return false;
}

