#include "dflat.h"

void fix_popdown(struct PopDown *popdown)
{
    if (popdown->SelectionTitle != NULL)
        popdown->SelectionTitle = strdup(popdown->SelectionTitle);
    if (popdown->help != NULL)
        popdown->help = strdup(popdown->help);
}

void fix_menu(MENU *menu)
{
    unsigned int x;

    if (menu->Title != NULL)
        menu->Title = strdup(menu->Title);
    if (menu->StatusText != NULL)
        menu->StatusText = strdup(menu->StatusText);
    for (x=0; x<MAXSELECTIONS; x++) {
        if (menu->Title == NULL)
            break;
        else
            fix_popdown(&(menu->Selections[x]));
    }
}

void fix_mbar(MBAR *mbar)
{
    unsigned int x;

    for (x=0; x<MAXPULLDOWNS; x++) {
        if (mbar->PullDown[x].Title == (void *)0xffffffff)
            break;
        else
            fix_menu(&(mbar->PullDown[x]));
    }
}
