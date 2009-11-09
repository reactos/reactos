/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeLoader
 * FILE:            freeldr/ui/tuimenu.c
 * PURPOSE:         UI Menu Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Brian Palmer (brianp@sginet.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

/* FUNCTIONS *****************************************************************/

BOOLEAN
TuiDisplayMenu(PCSTR MenuItemList[],
               ULONG MenuItemCount,
               ULONG DefaultMenuItem,
               LONG MenuTimeOut,
               ULONG* SelectedMenuItem,
               BOOLEAN CanEscape,
               UiMenuKeyPressFilterCallback KeyPressFilter)
{
    UI_MENU_INFO MenuInformation;
    ULONG LastClockSecond;
    ULONG CurrentClockSecond;
    ULONG KeyPress;

    //
    // Check if there's no timeout
    if (!MenuTimeOut)
    {
        //
        // Return the default selected item
        //
        if (SelectedMenuItem) *SelectedMenuItem = DefaultMenuItem;
        return TRUE;
    }

    //
    // Setup the MENU_INFO structure
    //
    MenuInformation.MenuItemList = MenuItemList;
    MenuInformation.MenuItemCount = MenuItemCount;
    MenuInformation.MenuTimeRemaining = MenuTimeOut;
    MenuInformation.SelectedMenuItem = DefaultMenuItem;

    //
    // Calculate the size of the menu box
    //
    TuiCalcMenuBoxSize(&MenuInformation);

    //
    // Draw the menu
    //
    UiVtbl.DrawMenu(&MenuInformation);

    //
    // Get the current second of time
    //
    LastClockSecond = ArcGetTime()->Second;

    //
    // Process keys
    //
    while (TRUE)
    {
        //
        // Process key presses
        //
        KeyPress = TuiProcessMenuKeyboardEvent(&MenuInformation,
                                               KeyPressFilter);

        //
        // Check for ENTER or ESC
        //
        if (KeyPress == KEY_ENTER) break;
        if (CanEscape && KeyPress == KEY_ESC) return FALSE;

        //
        // Update the date & time
        //
        TuiUpdateDateTime();
        VideoCopyOffScreenBufferToVRAM();

        //
        // Check if there is a countdown
        //
        if (MenuInformation.MenuTimeRemaining)
        {
            //
            // Get the updated time, seconds only
            //
            CurrentClockSecond = ArcGetTime()->Second;

            //
            // Check if more then a second has now elapsed
            //
            if (CurrentClockSecond != LastClockSecond)
            {
                //
                // Update the time information
                //
                LastClockSecond = CurrentClockSecond;
                MenuInformation.MenuTimeRemaining--;

                //
                // Update the menu
                //
                TuiDrawMenuBox(&MenuInformation);
                VideoCopyOffScreenBufferToVRAM();
            }
        }
        else
        {
            //
            // A time out occurred, exit this loop and return default OS
            //
            break;
        }
    }

    //
    // Return the selected item
    //
    if (SelectedMenuItem) *SelectedMenuItem = MenuInformation.SelectedMenuItem;
    return TRUE;
}

VOID
NTAPI
TuiCalcMenuBoxSize(PUI_MENU_INFO MenuInfo)
{
    ULONG i;
    ULONG Width = 0;
    ULONG Height;
    ULONG Length;

    //
    // Height is the menu item count plus 2 (top border & bottom border)
    //
    Height = MenuInfo->MenuItemCount + 2;
    Height -= 1; // Height is zero-based

    //
    // Loop every item
    //
    for(i = 0; i < MenuInfo->MenuItemCount; i++)
    {
        //
        // Get the string length and make it become the new width if necessary
        //
        Length = strlen(MenuInfo->MenuItemList[i]);
        if (Length > Width) Width = Length;
    }

    //
    // Allow room for left & right borders, plus 8 spaces on each side
    //
    Width += 18;

    //
    // Check if we're drawing a centered menu
    //
    if (UiCenterMenu)
    {
        //
        // Calculate the menu box area for a centered menu
        //
        MenuInfo->Left = (UiScreenWidth - Width) / 2;
        MenuInfo->Top = (((UiScreenHeight - TUI_TITLE_BOX_CHAR_HEIGHT) -
                          Height) / 2) + TUI_TITLE_BOX_CHAR_HEIGHT;
    }
    else
    {
        //
        // Put the menu in the default left-corner position
        //
        MenuInfo->Left = -1;
        MenuInfo->Top = 4;
    }

    //
    // The other margins are the same
    //
    MenuInfo->Right = (MenuInfo->Left) + Width;
    MenuInfo->Bottom = (MenuInfo->Top) + Height;
}

VOID
TuiDrawMenu(PUI_MENU_INFO MenuInfo)
{
    ULONG i;

    //
    // Draw the backdrop
    //
    UiDrawBackdrop();

    //
    // Update the status bar
    //
    UiVtbl.DrawStatusText("Use \x18\x19 to select, then press ENTER.");

    //
    // Draw the menu box
    //
    TuiDrawMenuBox(MenuInfo);

    //
    // Draw each line of the menu
    //
    for (i = 0; i < MenuInfo->MenuItemCount; i++) TuiDrawMenuItem(MenuInfo, i);
    VideoCopyOffScreenBufferToVRAM();
}

VOID
NTAPI
TuiDrawMenuBox(PUI_MENU_INFO MenuInfo)
{
    CHAR MenuLineText[80];
    CHAR TempString[80];
    ULONG i;

    //
    // Draw the menu box if requested
    //
    if (UiMenuBox)
    {
        UiDrawBox(MenuInfo->Left,
                  MenuInfo->Top,
                  MenuInfo->Right,
                  MenuInfo->Bottom,
                  D_VERT,
                  D_HORZ,
                  FALSE,        // Filled
                  TRUE,        // Shadow
                  ATTR(UiMenuFgColor, UiMenuBgColor));
    }

    //
    // If there is a timeout draw the time remaining
    //
    if (MenuInfo->MenuTimeRemaining >= 0)
    {
        //
        // Copy the integral time text string, and remove the last 2 chars
        //
        strcpy(TempString, UiTimeText);
        i = strlen(TempString);
        TempString[i - 2] = 0;

        //
        // Display the first part of the string and the remaining time
        //
        strcpy(MenuLineText, TempString);
        _itoa(MenuInfo->MenuTimeRemaining, TempString, 10);
        strcat(MenuLineText, TempString);

        //
        // Add the last 2 chars
        //
        strcat(MenuLineText, &UiTimeText[i - 2]);

        //
        // Check if this is a centered menu
        //
        if (UiCenterMenu)
        {
            //
            // Display it in the center of the menu
            //
            UiDrawText(MenuInfo->Right - strlen(MenuLineText) - 1,
                       MenuInfo->Bottom,
                       MenuLineText,
                       ATTR(UiMenuFgColor, UiMenuBgColor));
        }
        else
        {
            //
            // Display under the menu directly
            //
            UiDrawText(0,
                       MenuInfo->Bottom + 3,
                       MenuLineText,
                       ATTR(UiMenuFgColor, UiMenuBgColor));
        }
    }
    else
    {
        //
        // Erase the timeout string with spaces, and 0-terminate for sure
        //
        for (i=0; i<sizeof(MenuLineText)-1; i++)
        {
            MenuLineText[i] = ' ';
        }
        MenuLineText[sizeof(MenuLineText)-1] = 0;

        //
        // Draw this "empty" string to erase
        //
        if (UiCenterMenu)
        {
            UiDrawText(MenuInfo->Right - strlen(MenuLineText) - 1,
                       MenuInfo->Bottom,
                       MenuLineText,
                       ATTR(UiMenuFgColor, UiMenuBgColor));
        }
        else
        {
            UiDrawText(0,
                       MenuInfo->Bottom + 3,
                       MenuLineText,
                       ATTR(UiMenuFgColor, UiMenuBgColor));
        }
    }

    //
    // Loop each item
    //
    for (i = 0; i < MenuInfo->MenuItemCount; i++)
    {
        //
        // Check if it's a separator
        //
        if (!(_stricmp(MenuInfo->MenuItemList[i], "SEPARATOR")))
        {
            //
            // Draw the separator line
            //
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
NTAPI
TuiDrawMenuItem(PUI_MENU_INFO MenuInfo,
                ULONG MenuItemNumber)
{
    ULONG i;
    CHAR MenuLineText[80];
    ULONG SpaceTotal;
    ULONG SpaceLeft;
    ULONG SpaceRight = 0;
    UCHAR Attribute = ATTR(UiTextColor, UiMenuBgColor);

    //
    // Check if using centered menu
    //
    if (UiCenterMenu)
    {
        //
        // We will want the string centered so calculate
        // how many spaces will be to the left and right
        //
        SpaceTotal = (MenuInfo->Right - MenuInfo->Left - 2) -
                     strlen(MenuInfo->MenuItemList[MenuItemNumber]);
        SpaceLeft = (SpaceTotal / 2) + 1;
        SpaceRight = (SpaceTotal - SpaceLeft) + 1;

        //
        // Insert the spaces on the left
        //
        for (i = 0; i < SpaceLeft; i++) MenuLineText[i] = ' ';
        MenuLineText[i] = '\0';
    }
    else
    {
        //
        // Simply left-align it
        //
        MenuLineText[0] = '\0';
        strcat(MenuLineText, "    ");
    }

    //
    // Now append the text string
    //
    strcat(MenuLineText, MenuInfo->MenuItemList[MenuItemNumber]);

    //
    // Check if using centered menu, and add spaces on the right if so
    //
    if (UiCenterMenu) for (i=0; i < SpaceRight; i++) strcat(MenuLineText, " ");

    //
    // If it is a separator
    //
    if (!(_stricmp(MenuInfo->MenuItemList[MenuItemNumber], "SEPARATOR")))
    {
        //
        // Make it a separator line and use menu colors
        //
        memset(MenuLineText, 0, 80);
        memset(MenuLineText, 0xC4, (MenuInfo->Right - MenuInfo->Left - 1));
        Attribute = ATTR(UiMenuFgColor, UiMenuBgColor);
    }
    else if (MenuItemNumber == MenuInfo->SelectedMenuItem)
    {
        //
        // If this is the selected item, use the selected colors
        //
        Attribute = ATTR(UiSelectedTextColor, UiSelectedTextBgColor);
    }

    //
    // Draw the item
    //
    UiDrawText(MenuInfo->Left + 1,
               MenuInfo->Top + 1 + MenuItemNumber,
               MenuLineText,
               Attribute);
}

ULONG
NTAPI
TuiProcessMenuKeyboardEvent(PUI_MENU_INFO MenuInfo,
                            UiMenuKeyPressFilterCallback KeyPressFilter)
{
    ULONG KeyEvent = 0;
    ULONG Selected, Count;

    //
    // Check for a keypress
    //
    if (MachConsKbHit())
    {
        //
        // Check if the timeout is not already complete
        //
        if (MenuInfo->MenuTimeRemaining != -1)
        {
            //
            // Cancel it and remove it
            //
            MenuInfo->MenuTimeRemaining = -1;
            TuiDrawMenuBox(MenuInfo); // FIXME: Remove for minimal UI too
        }

        //
        // Get the key
        //
        KeyEvent = MachConsGetCh();

        //
        // Is it extended? Then get the extended key
        //
        if (!KeyEvent) KeyEvent = MachConsGetCh();

        //
        // Call the supplied key filter callback function to see
        // if it is going to handle this keypress.
        //
        if ((KeyPressFilter) && (KeyPressFilter(KeyEvent)))
        {
            //
            // It processed the key character, so redraw and exit
            //
            UiVtbl.DrawMenu(MenuInfo);
            return 0;
        }

        //
        // Process the key
        //
        if ((KeyEvent == KEY_UP) || (KeyEvent == KEY_DOWN))
        {
            //
            // Get the current selected item and count
            //
            Selected = MenuInfo->SelectedMenuItem;
            Count = MenuInfo->MenuItemCount - 1;

            //
            // Check if this was a key up and there's a selected menu item
            //
            if ((KeyEvent == KEY_UP) && (Selected))
            {
                //
                // Update the menu (Deselect previous item)
                //
                MenuInfo->SelectedMenuItem--;
                TuiDrawMenuItem(MenuInfo, Selected);
                Selected--;

                // Skip past any separators
                if ((Selected) &&
                    !(_stricmp(MenuInfo->MenuItemList[Selected], "SEPARATOR")))
                {
                    MenuInfo->SelectedMenuItem--;
                }
            }
            else if ((KeyEvent == KEY_DOWN) && (Selected < Count))
            {
                //
                // Update the menu (deselect previous item)
                //
                MenuInfo->SelectedMenuItem++;
                TuiDrawMenuItem(MenuInfo, Selected);
                Selected++;

                // Skip past any separators
                if ((Selected < Count) &&
                    !(_stricmp(MenuInfo->MenuItemList[Selected], "SEPARATOR")))
                {
                    MenuInfo->SelectedMenuItem++;
                }
            }

            //
            // Select new item and update video buffer
            //
            TuiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem);
            VideoCopyOffScreenBufferToVRAM();
        }
    }

    //
    // Return the pressed key
    //
    return KeyEvent;
}
