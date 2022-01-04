/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/freeldr/ui/directui.c
 * PURPOSE:         FreeLDR UI Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */
#ifdef _M_ARM

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

ULONG UiScreenWidth;
ULONG UiScreenHeight;
UCHAR UiMenuFgColor = COLOR_GRAY;
UCHAR UiMenuBgColor = COLOR_BLACK;
UCHAR UiTextColor = COLOR_GRAY;
UCHAR UiSelectedTextColor = COLOR_BLACK;
UCHAR UiSelectedTextBgColor = COLOR_GRAY;
CHAR UiTimeText[260] = "Seconds until highlighted choice will be started automatically:   ";

INT
TuiPrintf(const char *Format,
          ...)
{
    int i;
    int Length;
    va_list ap;
    CHAR Buffer[512];

    va_start(ap, Format);
    Length = _vsnprintf(Buffer, sizeof(Buffer), Format, ap);
    va_end(ap);

    if (Length == -1) Length = sizeof(Buffer);

    for (i = 0; i < Length; i++)
    {
        MachConsPutChar(Buffer[i]);
    }

    return Length;
}

BOOLEAN
UiInitialize(IN BOOLEAN ShowGui)
{
    ULONG Depth;

    /* Nothing to do */
    if (!ShowGui) return TRUE;

    /* Set mode and query size */
    MachVideoSetDisplayMode(NULL, TRUE);
    MachVideoGetDisplaySize(&UiScreenWidth, &UiScreenHeight, &Depth);
    return TRUE;
}

VOID
UiUnInitialize(IN PCSTR BootText)
{
    /* Nothing to do */
    return;
}

VOID
UiDrawBackdrop(VOID)
{
    /* Clear the screen */
    MachVideoClearScreen(ATTR(COLOR_WHITE, COLOR_BLACK));
}

VOID
UiDrawText(IN ULONG X,
           IN ULONG Y,
           IN PCSTR Text,
           IN UCHAR Attr)
{
    ULONG i, j;

    /* Draw the text character by character, but don't exceed the width */
    for (i = X, j = 0; Text[j] && i < UiScreenWidth; i++, j++)
    {
        /* Write the character */
        MachVideoPutChar(Text[j], Attr, i, Y);
    }
}

VOID
UiDrawText2(IN ULONG X,
            IN ULONG Y,
            IN ULONG MaxNumChars,
            IN PCSTR Text,
            IN UCHAR Attr)
{
    ULONG i, j;

    /* Draw the text character by character, but don't exceed the width */
    for (i = X, j = 0; Text[j] && i < UiScreenWidth && (MaxNumChars > 0 ? j < MaxNumChars : TRUE); i++, j++)
    {
        /* Write the character */
        MachVideoPutChar(Text[j], Attr, i, Y);
    }
}

VOID
UiDrawCenteredText(IN ULONG Left,
                   IN ULONG Top,
                   IN ULONG Right,
                   IN ULONG Bottom,
                   IN PCSTR TextString,
                   IN UCHAR Attr)
{
    ULONG TextLength, BoxWidth, BoxHeight, LineBreakCount, Index, LastIndex;
    ULONG RealLeft, RealTop, X, Y;
    CHAR Temp[2];

    /* Query text length */
    TextLength = strlen(TextString);

    /* Count the new lines and the box width */
    LineBreakCount = 0;
    BoxWidth = 0;
    LastIndex = 0;
    for (Index=0; Index < TextLength; Index++)
    {
        /* Scan for new lines */
        if (TextString[Index] == '\n')
        {
            /* Remember the new line */
            LastIndex = Index;
            LineBreakCount++;
        }
        else
        {
            /* Check for new larger box width */
            if ((Index - LastIndex) > BoxWidth)
            {
                /* Update it */
                BoxWidth = (Index - LastIndex);
            }
        }
    }

    /* Base the box height on the number of lines */
    BoxHeight = LineBreakCount + 1;

    /* Create the centered coordinates */
    RealLeft = (((Right - Left) - BoxWidth) / 2) + Left;
    RealTop = (((Bottom - Top) - BoxHeight) / 2) + Top;

    /* Now go for a second scan */
    LastIndex = 0;
    for (Index=0; Index < TextLength; Index++)
    {
        /* Look for new lines again */
        if (TextString[Index] == '\n')
        {
            /* Update where the text should start */
            RealTop++;
            LastIndex = 0;
        }
        else
        {
            /* We've got a line of text to print, do it */
            X = RealLeft + LastIndex;
            Y = RealTop;
            LastIndex++;
            Temp[0] = TextString[Index];
            Temp[1] = 0;
            UiDrawText(X, Y, Temp, Attr);
        }
    }
}

VOID
UiDrawStatusText(IN PCSTR StatusText)
{
    return;
}

VOID
UiInfoBox(IN PCSTR MessageText)
{
    TuiPrintf(MessageText);
}

VOID
UiMessageBox(IN PCSTR MessageText)
{
    TuiPrintf(MessageText);
}

VOID
UiMessageBoxCritical(IN PCSTR MessageText)
{
    TuiPrintf(MessageText);
}

VOID
UiDrawProgressBarCenter(IN ULONG Position,
                        IN ULONG Range,
                        IN PCHAR ProgressText)
{
    ULONG Left, Top, Right, Bottom, Width, Height;

    /* Build the coordinates and sizes */
    Height = 2;
    Width = UiScreenWidth;
    Left = 0;
    Right = (Left + Width) - 1;
    Top = UiScreenHeight - Height - 4;
    Bottom = Top + Height + 1;

    /* Draw the progress bar */
    UiDrawProgressBar(Left, Top, Right, Bottom, Position, Range, ProgressText);
}

VOID
UiDrawProgressBar(IN ULONG Left,
                  IN ULONG Top,
                  IN ULONG Right,
                  IN ULONG Bottom,
                  IN ULONG Position,
                  IN ULONG Range,
                  IN PCHAR ProgressText)
{
    ULONG i, ProgressBarWidth;

    /* Calculate the width of the bar proper */
    ProgressBarWidth = (Right - Left) - 3;

    /* First make sure the progress bar text fits */
    UiTruncateStringEllipsis(ProgressText, ProgressBarWidth - 4);
    if (Position > Range) Position = Range;

    /* Draw the "Loading..." text */
    UiDrawCenteredText(Left + 2, Top + 1, Right - 2, Top + 1, ProgressText, ATTR(7, 0));

    /* Draw the percent complete */
    for (i = 0; i < (Position * ProgressBarWidth) / Range; i++)
    {
        /* Use the fill character */
        UiDrawText(Left + 2 + i, Top + 2, "\xDB", ATTR(UiTextColor, UiMenuBgColor));
    }
}

VOID
UiShowMessageBoxesInSection(
    IN ULONG_PTR SectionId)
{
    return;
}

VOID
UiShowMessageBoxesInArgv(
    IN ULONG Argc,
    IN PCHAR Argv[])
{
    return;
}

VOID
UiTruncateStringEllipsis(IN PCHAR StringText,
                         IN ULONG MaxChars)
{
    /* If it's too large, just add some ellipsis past the maximum */
    if (strlen(StringText) > MaxChars)
        strcpy(&StringText[MaxChars - 3], "...");
}

VOID
UiDrawMenuBox(IN PUI_MENU_INFO MenuInfo)
{
    CHAR MenuLineText[80], TempString[80];
    ULONG i;

    /* If there is a timeout draw the time remaining */
    if (MenuInfo->MenuTimeRemaining >= 0)
    {
        /* Copy the integral time text string, and remove the last 2 chars */
        strcpy(TempString, UiTimeText);
        i = strlen(TempString);
        TempString[i - 2] = 0;

        /* Display the first part of the string and the remaining time */
        strcpy(MenuLineText, TempString);
        _itoa(MenuInfo->MenuTimeRemaining, TempString, 10);
        strcat(MenuLineText, TempString);

        /* Add the last 2 chars */
        strcat(MenuLineText, &UiTimeText[i - 2]);

        /* Display under the menu directly */
        UiDrawText(0,
                   MenuInfo->Bottom + 4,
                   MenuLineText,
                   ATTR(UiMenuFgColor, UiMenuBgColor));
    }
    else
    {
        /* Erase the timeout string with spaces, and 0-terminate for sure */
        for (i=0; i<sizeof(MenuLineText)-1; i++)
        {
            MenuLineText[i] = ' ';
        }
        MenuLineText[sizeof(MenuLineText)-1] = 0;

        /* Draw this "empty" string to erase */
        UiDrawText(0,
                   MenuInfo->Bottom + 4,
                   MenuLineText,
                   ATTR(UiMenuFgColor, UiMenuBgColor));
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
UiDrawMenuItem(IN PUI_MENU_INFO MenuInfo,
               IN ULONG MenuItemNumber)
{
    CHAR MenuLineText[80];
    UCHAR Attribute = ATTR(UiTextColor, UiMenuBgColor);

    /* Simply left-align it */
    MenuLineText[0] = '\0';
    strcat(MenuLineText, "    ");

    /* Now append the text string */
    if (MenuInfo->MenuItemList[MenuItemNumber])
        strcat(MenuLineText, MenuInfo->MenuItemList[MenuItemNumber]);

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
        /*  If this is the selected item, use the selected colors */
        Attribute = ATTR(UiSelectedTextColor, UiSelectedTextBgColor);
    }

    /* Draw the item */
    UiDrawText(MenuInfo->Left + 1,
               MenuInfo->Top + 1 + MenuItemNumber,
               MenuLineText,
               Attribute);
}

VOID
UiDrawMenu(IN PUI_MENU_INFO MenuInfo)
{
    ULONG i;

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

    /* Draw the menu box */
    UiDrawMenuBox(MenuInfo);

    /* Draw each line of the menu */
    for (i = 0; i < MenuInfo->MenuItemCount; i++)
    {
        UiDrawMenuItem(MenuInfo, i);
    }

    /* Display the boot options if needed */
    if (MenuInfo->ShowBootOptions)
    {
        DisplayBootTimeOptions();
    }
}

ULONG
UiProcessMenuKeyboardEvent(IN PUI_MENU_INFO MenuInfo,
                           IN UiMenuKeyPressFilterCallback KeyPressFilter)
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
        UiDrawMenuBox(MenuInfo);
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
        UiDrawMenu(MenuInfo);
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
            UiDrawMenuItem(MenuInfo, Selected);
            Selected--;

            // Skip past any separators
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
            UiDrawMenuItem(MenuInfo, Selected);
        }
        else if ((KeyEvent == KEY_DOWN) && (Selected < Count))
        {
            /* Deselect previous item and go down */
            MenuInfo->SelectedMenuItem++;
            UiDrawMenuItem(MenuInfo, Selected);
            Selected++;

            // Skip past any separators
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
            UiDrawMenuItem(MenuInfo, Selected);
        }

        /* Select new item and update video buffer */
        UiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem);
    }

    /*  Return the pressed key */
    return KeyEvent;
}

VOID
UiCalcMenuBoxSize(IN PUI_MENU_INFO MenuInfo)
{
    ULONG i, Width = 0, Height, Length;

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

    /* Put the menu in the default left-corner position */
    MenuInfo->Left = -1;
    MenuInfo->Top = 4;

    /* The other margins are the same */
    MenuInfo->Right = (MenuInfo->Left) + Width;
    MenuInfo->Bottom = (MenuInfo->Top) + Height;
}

BOOLEAN
UiDisplayMenu(
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

    /* Check if there's no timeout */
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
    UiCalcMenuBoxSize(&MenuInformation);

    /* Draw the menu */
    UiDrawMenu(&MenuInformation);

    /* Get the current second of time */
    LastClockSecond = ArcGetTime()->Second;

    /* Process keys */
    while (TRUE)
    {
        /* Process key presses */
        KeyPress = UiProcessMenuKeyboardEvent(&MenuInformation, KeyPressFilter);

        /* Check for ENTER or ESC */
        if (KeyPress == KEY_ENTER) break;
        if (CanEscape && KeyPress == KEY_ESC) return FALSE;

        /* Check if there is a countdown */
        if (MenuInformation.MenuTimeRemaining > 0)
        {
            /* Get the updated time, seconds only */
            CurrentClockSecond = ArcGetTime()->Second;

            /* Check if more then a second has now elapsed */
            if (CurrentClockSecond != LastClockSecond)
            {
                /* Update the time information */
                LastClockSecond = CurrentClockSecond;
                MenuInformation.MenuTimeRemaining--;

                /* Update the menu */
                UiDrawMenuBox(&MenuInformation);
            }
        }
        else if (MenuInformation.MenuTimeRemaining == 0)
        {
            /* A time out occurred, exit this loop and return default OS */
            break;
        }
    }

    /* Return the selected item */
    if (SelectedMenuItem) *SelectedMenuItem = MenuInformation.SelectedMenuItem;
    return TRUE;
}

#endif
