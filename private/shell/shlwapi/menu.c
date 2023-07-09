#include "priv.h"
#pragma  hdrstop


int GetMenuPosFromID(HMENU hmenu, UINT id)
{
    int iPos = -1;
    int cItems = GetMenuItemCount(hmenu);
    int i;

    for (i=0; i<cItems;i++)
    {
        MENUITEMINFO mii;
        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_ID;
        mii.wID = 0;
        if (GetMenuItemInfo(hmenu, i, TRUE, &mii))
        {
            if (mii.wID == id)
            {
                iPos = i;
                break;
            }
        }
    }

    return iPos;
}
