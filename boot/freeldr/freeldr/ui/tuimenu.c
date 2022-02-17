/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeLoader
 * FILE:            boot/freeldr/freeldr/ui/tuimenu.c
 * PURPOSE:         UI Menu Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Brian Palmer (brianp@sginet.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

/* FUNCTIONS *****************************************************************/

static VOID
TuiCalcMenuBoxSize(
    _In_ PUI_MENU_INFO MenuInfo);

static ULONG
TuiProcessMenuKeyboardEvent(
    _In_ PUI_MENU_INFO MenuInfo,
    _In_ UiMenuKeyPressFilterCallback KeyPressFilter);

BOOLEAN
TuiDisplayMenu(
    IN PCSTR MenuHeader,
    IN PCSTR MenuFooter OPTIONAL,
    IN BOOLEAN ShowBootOptions,
    IN PCSTR MenuItemList[],
    IN ULONG MenuItemCount,
    IN ULONG DefaultMenuItem,
    IN LONG MenuTimeOut,
    OUT PULONG SelectedMenuItem,
    IN BOOLEAN CanEscape,
    IN UiMenuKeyPressFilterCallback KeyPressFilter OPTIONAL,
    IN PVOID Context OPTIONAL)
{
    UI_MENU_INFO MenuInformation;
    ULONG LastClockSecond;
    ULONG CurrentClockSecond;
    ULONG KeyPress;

    /*
     * Before taking any default action if there is no timeout,
     * check whether the supplied key filter callback function
     * may handle a specific user keypress. If it does, the
     * timeout is cancelled.
     */
    if (!MenuTimeOut && KeyPressFilter && MachConsKbHit())
    {
        /* Get the key (get the extended key if needed) */
        KeyPress = MachConsGetCh();
        if (KeyPress == KEY_EXTENDED)
            KeyPress = MachConsGetCh();

        /*
         * Call the supplied key filter callback function to see
         * if it is going to handle this keypress.
         */
        if (KeyPressFilter(KeyPress, DefaultMenuItem, Context))
        {
            /* It processed the key character, cancel the timeout */
            MenuTimeOut = -1;
        }
    }

    /* Check if there is no timeout */
    if (!MenuTimeOut)
    {
        /* Return the default selected item */
        if (SelectedMenuItem) *SelectedMenuItem = DefaultMenuItem;
        return TRUE;
    }

    /* Setup the MENU_INFO structure */
    MenuInformation.MenuHeader = MenuHeader;
    MenuInformation.MenuFooter = MenuFooter;
    MenuInformation.ShowBootOptions = ShowBootOptions;
    MenuInformation.MenuItemList = MenuItemList;
    MenuInformation.MenuItemCount = MenuItemCount;
    MenuInformation.MenuTimeRemaining = MenuTimeOut;
    MenuInformation.SelectedMenuItem = DefaultMenuItem;
    MenuInformation.Context = Context;

    /* Calculate the size of the menu box */
    TuiCalcMenuBoxSize(&MenuInformation);

    /* Draw the menu */
#ifdef _M_ARM
    UiDrawMenu(&MenuInformation);
#else
    UiVtbl.DrawMenu(&MenuInformation);
#endif

    /* Get the current second of time */
    LastClockSecond = ArcGetTime()->Second;

    /* Process keys */
    while (TRUE)
    {
        /* Process key presses */
        KeyPress = TuiProcessMenuKeyboardEvent(&MenuInformation, KeyPressFilter);

        /* Check for ENTER or ESC */
        if (KeyPress == KEY_ENTER) break;
        if (CanEscape && KeyPress == KEY_ESC) return FALSE;

#ifndef _M_ARM // FIXME: Theme-specific
        /* Update the date & time */
        TuiUpdateDateTime();
        VideoCopyOffScreenBufferToVRAM();
#endif

        /* Check if there is a countdown */
        if (MenuInformation.MenuTimeRemaining > 0)
        {
            /* Get the updated time, seconds only */
            CurrentClockSecond = ArcGetTime()->Second;

            /* Check if more than a second has now elapsed */
            if (CurrentClockSecond != LastClockSecond)
            {
                /* Update the time information */
                LastClockSecond = CurrentClockSecond;
                MenuInformation.MenuTimeRemaining--;

                /* Update the menu */
                TuiDrawMenuBox(&MenuInformation);
#ifndef _M_ARM
                VideoCopyOffScreenBufferToVRAM();
#endif
            }
        }
        else if (MenuInformation.MenuTimeRemaining == 0)
        {
            /* A time out occurred, exit this loop and return default OS */
            break;
        }

#ifndef _M_ARM
        MachHwIdle();
#endif
    }

    /* Return the selected item */
    if (SelectedMenuItem) *SelectedMenuItem = MenuInformation.SelectedMenuItem;
    return TRUE;
}

static VOID
TuiCalcMenuBoxSize(
    _In_ PUI_MENU_INFO MenuInfo)
{
    ULONG i;
    ULONG Width = 0;
    ULONG Height;
    ULONG Length;

    /* Height is the menu item count plus 2 (top border & bottom border) */
    Height = MenuInfo->MenuItemCount + 2;
    Height -= 1; // Height is zero-based

    /* Loop every item */
    for (i = 0; i < MenuInfo->MenuItemCount; i++)
    {
        /* Get the string length and make it become the new width if necessary */
        if (MenuInfo->MenuItemList[i])
        {
            Length = (ULONG)strlen(MenuInfo->MenuItemList[i]);
            if (Length > Width) Width = Length;
        }
    }

    /* Allow room for left & right borders, plus 8 spaces on each side */
    Width += 18;

#ifndef _M_ARM
    /* Check if we're drawing a centered menu */
    if (UiCenterMenu)
    {
        /* Calculate the menu box area for a centered menu */
        MenuInfo->Left = (UiScreenWidth - Width) / 2;
        MenuInfo->Top = (((UiScreenHeight - TUI_TITLE_BOX_CHAR_HEIGHT) -
                          Height) / 2) + TUI_TITLE_BOX_CHAR_HEIGHT;
    }
    else
#endif
    {
        /* Put the menu in the default left-corner position */
        MenuInfo->Left = -1;
        MenuInfo->Top = 4;
    }

    /* The other margins are the same */
    MenuInfo->Right = MenuInfo->Left + Width;
    MenuInfo->Bottom = MenuInfo->Top + Height;
}

VOID
TuiDrawMenu(
    _In_ PUI_MENU_INFO MenuInfo)
{
    ULONG i;

#ifdef _M_ARM // FIXME: Theme-specific

    /* No GUI status bar text, just minimal text. Show the menu header. */
    if (MenuInfo->MenuHeader)
    {
        UiDrawText(0,
                   MenuInfo->Top - 2,
                   MenuInfo->MenuHeader,
                   ATTR(UiMenuFgColor, UiMenuBgColor));
    }

    /* Now tell the user how to choose */
    UiDrawText(0,
               MenuInfo->Bottom + 1,
               "Use \x18 and \x19 to move the highlight to your choice.",
               ATTR(UiMenuFgColor, UiMenuBgColor));
    UiDrawText(0,
               MenuInfo->Bottom + 2,
               "Press ENTER to choose.",
               ATTR(UiMenuFgColor, UiMenuBgColor));

    /* And show the menu footer */
    if (MenuInfo->MenuFooter)
    {
        UiDrawText(0,
                   UiScreenHeight - 4,
                   MenuInfo->MenuFooter,
                   ATTR(UiMenuFgColor, UiMenuBgColor));
    }

#else

    /* Draw the backdrop */
    UiDrawBackdrop();

    /* Update the status bar */
    UiVtbl.DrawStatusText("Use \x18 and \x19 to select, then press ENTER.");

#endif

    /* Draw the menu box */
    TuiDrawMenuBox(MenuInfo);

    /* Draw each line of the menu */
    for (i = 0; i < MenuInfo->MenuItemCount; i++)
    {
        TuiDrawMenuItem(MenuInfo, i);
    }

    /* Display the boot options if needed */
    if (MenuInfo->ShowBootOptions)
    {
        DisplayBootTimeOptions();
    }

#ifndef _M_ARM
    VideoCopyOffScreenBufferToVRAM();
#endif
}

VOID
TuiDrawMenuBox(
    _In_ PUI_MENU_INFO MenuInfo)
{
    CHAR MenuLineText[80], TempString[80];
    ULONG i;

#ifndef _M_ARM // FIXME: Theme-specific
    /* Draw the menu box if requested */
    if (UiMenuBox)
    {
        UiDrawBox(MenuInfo->Left,
                  MenuInfo->Top,
                  MenuInfo->Right,
                  MenuInfo->Bottom,
                  D_VERT,
                  D_HORZ,
                  FALSE,    // Filled
                  TRUE,     // Shadow
                  ATTR(UiMenuFgColor, UiMenuBgColor));
    }
#endif

    /* If there is a timeout draw the time remaining */
    if (MenuInfo->MenuTimeRemaining >= 0)
    {
        /* Copy the integral time text string, and remove the last 2 chars */
        strcpy(TempString, UiTimeText);
        i = (ULONG)strlen(TempString);
        TempString[i - 2] = 0;

        /* Display the first part of the string and the remaining time */
        strcpy(MenuLineText, TempString);
        _itoa(MenuInfo->MenuTimeRemaining, TempString, 10);
        strcat(MenuLineText, TempString);

        /* Add the last 2 chars */
        strcat(MenuLineText, &UiTimeText[i - 2]);

#ifndef _M_ARM
        /* Check if this is a centered menu */
        if (UiCenterMenu)
        {
            /* Display it in the center of the menu */
            UiDrawText(MenuInfo->Right - (ULONG)strlen(MenuLineText) - 1,
                       MenuInfo->Bottom,
                       MenuLineText,
                       ATTR(UiMenuFgColor, UiMenuBgColor));
        }
        else
#endif
        {
            /* Display under the menu directly */
            UiDrawText(0,
                       MenuInfo->Bottom + 4,
                       MenuLineText,
                       ATTR(UiMenuFgColor, UiMenuBgColor));
        }
    }
    else
    {
        /* Erase the timeout string with spaces, and 0-terminate for sure */
        for (i = 0; i < sizeof(MenuLineText)-1; i++)
        {
            MenuLineText[i] = ' ';
        }
        MenuLineText[sizeof(MenuLineText)-1] = 0;

        /* Draw this "empty" string to erase */
#ifndef _M_ARM
        if (UiCenterMenu)
        {
            UiDrawText(MenuInfo->Right - (ULONG)strlen(MenuLineText) - 1,
                       MenuInfo->Bottom,
                       MenuLineText,
                       ATTR(UiMenuFgColor, UiMenuBgColor));
        }
        else
#endif
        {
            UiDrawText(0,
                       MenuInfo->Bottom + 4,
                       MenuLineText,
                       ATTR(UiMenuFgColor, UiMenuBgColor));
        }
    }

    /* Loop each item */
    for (i = 0; i < MenuInfo->MenuItemCount; i++)
    {
        /* Check if it's a separator */
        if (MenuInfo->MenuItemList[i] == NULL)
        {
            /* Draw the separator line */
            UiDrawText(MenuInfo->Left,
                       MenuInfo->Top + i + 1,
                       "\xC7",
                       ATTR(UiMenuFgColor, UiMenuBgColor));
            UiDrawText(MenuInfo->Right,
                       MenuInfo->Top + i + 1,
                       "\xB6",
                       ATTR(UiMenuFgColor, UiMenuBgColor));
        }
    }
}

VOID
TuiDrawMenuItem(
    _In_ PUI_MENU_INFO MenuInfo,
    _In_ ULONG MenuItemNumber)
{
#ifndef _M_ARM
    ULONG i;
    ULONG SpaceTotal;
    ULONG SpaceLeft;
    ULONG SpaceRight = 0;
#endif
    UCHAR Attribute = ATTR(UiTextColor, UiMenuBgColor);
    CHAR MenuLineText[80];

#ifndef _M_ARM
    /* Check if using centered menu */
    if (UiCenterMenu)
    {
        /*
         * We will want the string centered so calculate
         * how many spaces will be to the left and right.
         */
        SpaceTotal = (MenuInfo->Right - MenuInfo->Left - 2) -
                     (ULONG)(MenuInfo->MenuItemList[MenuItemNumber] ?
                             strlen(MenuInfo->MenuItemList[MenuItemNumber]) : 0);
        SpaceLeft = (SpaceTotal / 2) + 1;
        SpaceRight = (SpaceTotal - SpaceLeft) + 1;

        /* Insert the spaces on the left */
        for (i = 0; i < SpaceLeft; i++)
            MenuLineText[i] = ' ';
        MenuLineText[i] = '\0';
    }
    else
#endif
    {
        /* Simply left-align it */
        MenuLineText[0] = '\0';
        strcat(MenuLineText, "    ");
    }

    /* Now append the text string */
    if (MenuInfo->MenuItemList[MenuItemNumber])
        strcat(MenuLineText, MenuInfo->MenuItemList[MenuItemNumber]);

#ifndef _M_ARM
    /* Check if using centered menu, and add spaces on the right if so */
    if (UiCenterMenu)
    {
        for (i = 0; i < SpaceRight; i++)
            strcat(MenuLineText, " ");
    }
#endif

    /* If it is a separator */
    if (MenuInfo->MenuItemList[MenuItemNumber] == NULL)
    {
        /* Make it a separator line and use menu colors */
        memset(MenuLineText, 0, sizeof(MenuLineText));
        memset(MenuLineText, 0xC4, (MenuInfo->Right - MenuInfo->Left - 1));
        Attribute = ATTR(UiMenuFgColor, UiMenuBgColor);
    }
    else if (MenuItemNumber == MenuInfo->SelectedMenuItem)
    {
        /* If this is the selected item, use the selected colors */
        Attribute = ATTR(UiSelectedTextColor, UiSelectedTextBgColor);
    }

    /* Draw the item */
    UiDrawText(MenuInfo->Left + 1,
               MenuInfo->Top + 1 + MenuItemNumber,
               MenuLineText,
               Attribute);
}

static ULONG
TuiProcessMenuKeyboardEvent(
    _In_ PUI_MENU_INFO MenuInfo,
    _In_ UiMenuKeyPressFilterCallback KeyPressFilter)
{
    ULONG KeyEvent = 0;
    ULONG Selected, Count;

    /* Check for a keypress */
    if (!MachConsKbHit())
        return 0; // None, bail out

    /* Check if the timeout is not already complete */
    if (MenuInfo->MenuTimeRemaining != -1)
    {
        /* Cancel it and remove it */
        MenuInfo->MenuTimeRemaining = -1;
        TuiDrawMenuBox(MenuInfo); // FIXME: Remove for minimal UI too
    }

    /* Get the key (get the extended key if needed) */
    KeyEvent = MachConsGetCh();
    if (KeyEvent == KEY_EXTENDED)
        KeyEvent = MachConsGetCh();

    /*
     * Call the supplied key filter callback function to see
     * if it is going to handle this keypress.
     */
    if (KeyPressFilter &&
        KeyPressFilter(KeyEvent, MenuInfo->SelectedMenuItem, MenuInfo->Context))
    {
        /* It processed the key character, so redraw and exit */
#ifdef _M_ARM
        UiDrawMenu(MenuInfo);
#else
        UiVtbl.DrawMenu(MenuInfo);
#endif
        return 0;
    }

    /* Process the key */
    if ((KeyEvent == KEY_UP  ) || (KeyEvent == KEY_DOWN) ||
        (KeyEvent == KEY_HOME) || (KeyEvent == KEY_END ))
    {
        /* Get the current selected item and count */
        Selected = MenuInfo->SelectedMenuItem;
        Count = MenuInfo->MenuItemCount - 1;

        /* Check the key and change the selected menu item */
        if ((KeyEvent == KEY_UP) && (Selected > 0))
        {
            /* Deselect previous item and go up */
            MenuInfo->SelectedMenuItem--;
            TuiDrawMenuItem(MenuInfo, Selected);
            Selected--;

            /* Skip past any separators */
            if ((Selected > 0) &&
                (MenuInfo->MenuItemList[Selected] == NULL))
            {
                MenuInfo->SelectedMenuItem--;
            }
        }
        else if ( ((KeyEvent == KEY_UP) && (Selected == 0)) ||
                   (KeyEvent == KEY_END) )
        {
            /* Go to the end */
            MenuInfo->SelectedMenuItem = Count;
            TuiDrawMenuItem(MenuInfo, Selected);
        }
        else if ((KeyEvent == KEY_DOWN) && (Selected < Count))
        {
            /* Deselect previous item and go down */
            MenuInfo->SelectedMenuItem++;
            TuiDrawMenuItem(MenuInfo, Selected);
            Selected++;

            /* Skip past any separators */
            if ((Selected < Count) &&
                (MenuInfo->MenuItemList[Selected] == NULL))
            {
                MenuInfo->SelectedMenuItem++;
            }
        }
        else if ( ((KeyEvent == KEY_DOWN) && (Selected == Count)) ||
                   (KeyEvent == KEY_HOME) )
        {
            /* Go to the beginning */
            MenuInfo->SelectedMenuItem = 0;
            TuiDrawMenuItem(MenuInfo, Selected);
        }

        /* Select new item and update video buffer */
        TuiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem);
#ifndef _M_ARM
        VideoCopyOffScreenBufferToVRAM();
#endif
    }

    /* Return the pressed key */
    return KeyEvent;
}
