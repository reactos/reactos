/*
 * Displays the LanguagePage.
 *
 * Next pages: WelcomePage, QuitPage
 *
 * SIDEEFFECTS
 *  Init SelectedLanguageId
 *  Init USetupData.LanguageId
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
LanguagePage(PINPUT_RECORD Ir)
{
    GENERIC_LIST_UI ListUi;
    PCWSTR NewLanguageId;
    BOOL RefreshPage = FALSE;

    /* Initialize the computer settings list */
    if (USetupData.LanguageList == NULL)
    {
        USetupData.LanguageList = CreateLanguageList(USetupData.SetupInf, DefaultLanguage);
        if (USetupData.LanguageList == NULL)
        {
           PopupError("Setup failed to initialize available translations", NULL, NULL, POPUP_WAIT_NONE);
           return WELCOME_PAGE;
        }
    }

    SelectedLanguageId = DefaultLanguage;
    USetupData.LanguageId = 0;

    /* Load the font */
    SetConsoleCodePage();
    UpdateKBLayout();

    /*
     * If there is no language or just a single one in the list,
     * skip the language selection process altogether.
     */
    if (GetNumberOfListEntries(USetupData.LanguageList) <= 1)
    {
        USetupData.LanguageId = (LANGID)(wcstol(SelectedLanguageId, NULL, 16) & 0xFFFF);
        return WELCOME_PAGE;
    }

    InitGenericListUi(&ListUi, USetupData.LanguageList, GetSettingDescription);
    DrawGenericList(&ListUi,
                    2, 18,
                    xScreen - 3,
                    yScreen - 3);

    ScrollToPositionGenericList(&ListUi, GetDefaultLanguageIndex());

    MUIDisplayPage(LANGUAGE_PAGE);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN))  /* DOWN */
        {
            ScrollDownGenericList(&ListUi);
            RefreshPage = TRUE;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP))  /* UP */
        {
            ScrollUpGenericList(&ListUi);
            RefreshPage = TRUE;
        }
        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_NEXT))  /* PAGE DOWN */
        {
            ScrollPageDownGenericList(&ListUi);
            RefreshPage = TRUE;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_PRIOR))  /* PAGE UP */
        {
            ScrollPageUpGenericList(&ListUi);
            RefreshPage = TRUE;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir))
                return QUIT_PAGE;
            else
                RedrawGenericList(&ListUi);
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)  /* ENTER */
        {
            ASSERT(GetNumberOfListEntries(USetupData.LanguageList) >= 1);

            SelectedLanguageId =
                ((PGENENTRY)GetListEntryData(GetCurrentListEntry(USetupData.LanguageList)))->Id;

            USetupData.LanguageId = (LANGID)(wcstol(SelectedLanguageId, NULL, 16) & 0xFFFF);

            if (wcscmp(SelectedLanguageId, DefaultLanguage))
            {
                UpdateKBLayout();
            }

            /* Load the font */
            SetConsoleCodePage();

            return WELCOME_PAGE;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar > 0x60) && (Ir->Event.KeyEvent.uChar.AsciiChar < 0x7b))
        {
            /* a-z */
            GenericListKeyPress(&ListUi, Ir->Event.KeyEvent.uChar.AsciiChar);
            RefreshPage = TRUE;
        }

        if (RefreshPage)
        {
            ASSERT(GetNumberOfListEntries(USetupData.LanguageList) >= 1);

            NewLanguageId =
                ((PGENENTRY)GetListEntryData(GetCurrentListEntry(USetupData.LanguageList)))->Id;

            if (wcscmp(SelectedLanguageId, NewLanguageId))
            {
                /* Clear the language page */
                MUIClearPage(LANGUAGE_PAGE);

                SelectedLanguageId = NewLanguageId;

                /* Load the font */
                SetConsoleCodePage();

                /* Redraw language selection page in native language */
                MUIDisplayPage(LANGUAGE_PAGE);
            }

            RefreshPage = FALSE;
        }
    }

    return WELCOME_PAGE;
}
