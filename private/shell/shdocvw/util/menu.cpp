//
//  Menu- and menuband- related utility functions
//

#include "local.h"
#include "dochost.h"

// this goes away when the split is done **and** dochost goes away


/*----------------------------------------------------------
Purpose: Replace the contents of hmenuDst with hmenuSrc.  Note any
         submenus in hmenuDst will be deleted.  

         Call Menu_RemoveAllSubMenus if you don't want this to 
         happen.
*/
void Menu_Replace(HMENU hmenuDst, HMENU hmenuSrc)
{
    int cItems = GetMenuItemCount(hmenuDst);
    int i;

    for (i=0; i<cItems; i++)
        DeleteMenu(hmenuDst, 0, MF_BYPOSITION);

    cItems = GetMenuItemCount(hmenuSrc);
    for (i=0; i<cItems; i++)
    {
        MENUITEMINFO mii;
        TCHAR szText[MAX_PATH];
        mii.cbSize = SIZEOF(MENUITEMINFO);
        mii.dwTypeData = szText;
        mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STATE;
        mii.cch = ARRAYSIZE(szText);
        mii.fType = MFT_SEPARATOR;
        mii.hSubMenu = NULL;
        mii.dwItemData = 0;
        if (GetMenuItemInfo(hmenuSrc, i, TRUE, &mii))
        {
            HMENU hMenuOldSub = NULL;
            if (mii.hSubMenu != NULL)
            {
                hMenuOldSub = mii.hSubMenu;
                mii.hSubMenu = CreateMenu();
                Menu_Replace(mii.hSubMenu, hMenuOldSub);
            }
            InsertMenuItem(hmenuDst, i, TRUE, &mii);
        }
    }
}


#ifndef POSTPOSTSPLIT
//----------------------------------------------------------------------
//
// CMenuList
//
//----------------------------------------------------------------------


typedef struct
{
    HMENU   hmenu;
    BITBOOL bObject:1;              // TRUE: menu belongs to object
} MLITEM;       // CMenuList item


CMenuList::CMenuList(void)
{
    ASSERT(NULL == _hdsa);
}


CMenuList::~CMenuList(void)
{
    if (_hdsa)
        DSA_Destroy(_hdsa);
}    


/*----------------------------------------------------------
Purpose: Set the menu list (comparable to HOLEMENU) so we can
         dispatch commands to the frame or the object correctly.
         We do this since menu bands bypass OLE's FrameFilterWndProc.

         We build the menu list by comparing the given hmenuShared
         with hmenuFrame.  Anything in hmenuShared that is not
         in hmenuFrame belongs to the object.

*/
void CMenuList::Set(HMENU hmenuShared, HMENU hmenuFrame)
{
    ASSERT(NULL == hmenuShared || IS_VALID_HANDLE(hmenuShared, MENU));
    ASSERT(NULL == hmenuFrame || IS_VALID_HANDLE(hmenuFrame, MENU));

    if (_hdsa)
    {
        ASSERT(IS_VALID_HANDLE(_hdsa, DSA));

        DSA_DeleteAllItems(_hdsa);
    }
    else
        _hdsa = DSA_Create(sizeof(MLITEM), 10);

    if (_hdsa && hmenuShared && hmenuFrame)
    {
        int i;
        int iFrame = 0;
        int cmenu = GetMenuItemCount(hmenuShared);
        int cmenuFrame = GetMenuItemCount(hmenuFrame);
        BOOL bMatched;
        int iSaveFrame;
        int iHaveFrame = -1;

        TCHAR sz[64];
        TCHAR szFrame[64];
        MENUITEMINFO miiFrame;
        MENUITEMINFO mii;
        MLITEM mlitem;

        miiFrame.cbSize = sizeof(miiFrame);
        mii.cbSize = sizeof(mii);

        for (i = 0; i < cmenu; i++)
        {

            mii.cch = SIZECHARS(sz);
            mii.fMask  = MIIM_SUBMENU | MIIM_TYPE;
            mii.dwTypeData = sz;
            EVAL(GetMenuItemInfo(hmenuShared, i, TRUE, &mii));

            ASSERT(IS_VALID_HANDLE(mii.hSubMenu, MENU));
            
            mlitem.hmenu = mii.hSubMenu;

            iSaveFrame = iFrame;
            bMatched = FALSE;

            //  DocObject might have dropped some of our menus, like edit and view
            //  Need to be able to skip over dropped frame menus
            while (1)
            {
                if (iHaveFrame != iFrame)
                {
                    iHaveFrame = iFrame;
                    if (iFrame < cmenuFrame)
                    {
                        miiFrame.cch = SIZECHARS(szFrame);
                        miiFrame.fMask  = MIIM_SUBMENU | MIIM_TYPE;
                        miiFrame.dwTypeData = szFrame;
                        EVAL(GetMenuItemInfo(hmenuFrame, iFrame, TRUE, &miiFrame));
                    }
                    else
                    {
                        // Make it so it won't compare
                        miiFrame.hSubMenu = NULL;
                        *szFrame = 0;
                    }

                }
                ASSERT(iFrame >= cmenuFrame || IS_VALID_HANDLE(miiFrame.hSubMenu, MENU));
                
                // The browser may have a menu that was not merged into
                // the shared menu because the object put one in with 
                // the same name.  Have we hit this case? Check by comparing
                // sz and szFrame

                if (mii.hSubMenu == miiFrame.hSubMenu || 0 == StrCmp(sz, szFrame))
                {
                    bMatched = TRUE;
                    break;
                }
                else
                {
                    if (iFrame >= cmenuFrame) 
                    {
                        break;
                    }
                    iFrame++;
                }
            }

            // Is this one of our menus?
            mlitem.bObject = (mii.hSubMenu == miiFrame.hSubMenu) ? FALSE:TRUE;
            if (bMatched)
            {
                iFrame++;
            }
            else
            {
                iFrame = iSaveFrame;
            }
            DSA_SetItem(_hdsa, i, &mlitem);
        }
    }
}   


/*----------------------------------------------------------
Purpose: Adds the given hmenu to the list.

*/
void CMenuList::AddMenu(HMENU hmenu)
{
    ASSERT(NULL == hmenu || IS_VALID_HANDLE(hmenu, MENU));

    if (_hdsa && hmenu)
    {
        MLITEM mlitem;

        mlitem.hmenu = hmenu;
        mlitem.bObject = TRUE;
        
        DSA_AppendItem(_hdsa, &mlitem);
    }
}     


/*----------------------------------------------------------
Purpose: Removes the given hmenu from the list.

*/
void CMenuList::RemoveMenu(HMENU hmenu)
{
    ASSERT(NULL == hmenu || IS_VALID_HANDLE(hmenu, MENU));

    if (_hdsa && hmenu)
    {
        int i = DSA_GetItemCount(_hdsa) - 1;

        for (; i >= 0; i--)
        {
            MLITEM * pmlitem = (MLITEM *)DSA_GetItemPtr(_hdsa, i);
            ASSERT(pmlitem);

            if (hmenu == pmlitem->hmenu)
            {
                DSA_DeleteItem(_hdsa, i);
                break;
            }
        }
    }
}     


/*----------------------------------------------------------
Purpose: Returns TRUE if the given hmenu belongs to the object.

*/
BOOL CMenuList::IsObjectMenu(HMENU hmenu)
{
    BOOL bRet = FALSE;

    ASSERT(NULL == hmenu || IS_VALID_HANDLE(hmenu, MENU));

    if (_hdsa && hmenu)
    {
        int i;

        for (i = 0; i < DSA_GetItemCount(_hdsa); i++)
        {
            MLITEM * pmlitem = (MLITEM *)DSA_GetItemPtr(_hdsa, i);
            ASSERT(pmlitem);

            if (hmenu == pmlitem->hmenu)
            {
                bRet = pmlitem->bObject;
                break;
            }
        }
    }
    return bRet;
}     


#ifdef DEBUG

void CMenuList::Dump(LPCTSTR pszMsg)
{
    if (IsFlagSet(g_dwDumpFlags, DF_DEBUGMENU))
    {
        TraceMsg(TF_ALWAYS, "CMenuList: Dumping menus for %#08x %s", (LPVOID)this, pszMsg);
        
        if (_hdsa)
        {
            int i;

            for (i = 0; i < DSA_GetItemCount(_hdsa); i++)
            {
                MLITEM * pmlitem = (MLITEM *)DSA_GetItemPtr(_hdsa, i);
                ASSERT(pmlitem);

                TraceMsg(TF_ALWAYS, "   [%d] = %x", i, pmlitem->hmenu);
            }
        }
    }
}

#endif


#endif
