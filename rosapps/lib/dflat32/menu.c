/* ------------- menu.c ------------- */

#include "dflat32/dflat.h"

static struct PopDown *FindCmd(MBAR *mn, int cmd)
{
    MENU *mnu = mn->PullDown;
    while (mnu->Title != (void *)-1)    {
        struct PopDown *pd = mnu->Selections;
        while (pd->SelectionTitle != NULL)    {
            if (pd->ActionId == cmd)
                return pd;
            pd++;
        }
        mnu++;
    }
    return NULL;
}

char *GetCommandText(MBAR *mn, int cmd)
{
    struct PopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        return pd->SelectionTitle;
    return NULL;
}

BOOL isCascadedCommand(MBAR *mn, int cmd)
{
    struct PopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        return pd->Attrib & CASCADED;
    return FALSE;
}

void ActivateCommand(MBAR *mn, int cmd)
{
    struct PopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        pd->Attrib &= ~INACTIVE;
}

void DeactivateCommand(MBAR *mn, int cmd)
{
    struct PopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        pd->Attrib |= INACTIVE;
}

BOOL isActive(MBAR *mn, int cmd)
{
    struct PopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        return !(pd->Attrib & INACTIVE);
    return FALSE;
}

BOOL GetCommandToggle(MBAR *mn, int cmd)
{
    struct PopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        return (pd->Attrib & CHECKED) != 0;
    return FALSE;
}

void SetCommandToggle(MBAR *mn, int cmd)
{
    struct PopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        pd->Attrib |= CHECKED;
}

void ClearCommandToggle(MBAR *mn, int cmd)
{
    struct PopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        pd->Attrib &= ~CHECKED;
}

void InvertCommandToggle(MBAR *mn, int cmd)
{
    struct PopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        pd->Attrib ^= CHECKED;
}
