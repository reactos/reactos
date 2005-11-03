/* ------------- menu.c ------------- */

#include "dflat.h"

static struct DfPopDown *FindCmd(DF_MBAR *mn, int cmd)
{
    DF_MENU *mnu = mn->PullDown;
    while (mnu->Title != (void *)-1)    {
        struct DfPopDown *pd = mnu->Selections;
        while (pd->SelectionTitle != NULL)    {
            if (pd->ActionId == cmd)
                return pd;
            pd++;
        }
        mnu++;
    }
    return NULL;
}

char *DfGetCommandText(DF_MBAR *mn, int cmd)
{
    struct DfPopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        return pd->SelectionTitle;
    return NULL;
}

BOOL DfIsCascadedCommand(DF_MBAR *mn, int cmd)
{
    struct DfPopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        return pd->Attrib & DF_CASCADED;
    return FALSE;
}

void DfActivateCommand(DF_MBAR *mn, int cmd)
{
    struct DfPopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        pd->Attrib &= ~DF_INACTIVE;
}

void DfDeactivateCommand(DF_MBAR *mn, int cmd)
{
    struct DfPopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        pd->Attrib |= DF_INACTIVE;
}

BOOL isActive(DF_MBAR *mn, int cmd)
{
    struct DfPopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        return !(pd->Attrib & DF_INACTIVE);
    return FALSE;
}

BOOL DfGetCommandToggle(DF_MBAR *mn, int cmd)
{
    struct DfPopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        return (pd->Attrib & DF_CHECKED) != 0;
    return FALSE;
}

void DfSetCommandToggle(DF_MBAR *mn, int cmd)
{
    struct DfPopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        pd->Attrib |= DF_CHECKED;
}

void DfClearCommandToggle(DF_MBAR *mn, int cmd)
{
    struct DfPopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        pd->Attrib &= ~DF_CHECKED;
}

void DfInvertCommandToggle(DF_MBAR *mn, int cmd)
{
    struct DfPopDown *pd = FindCmd(mn, cmd);
    if (pd != NULL)
        pd->Attrib ^= DF_CHECKED;
}
