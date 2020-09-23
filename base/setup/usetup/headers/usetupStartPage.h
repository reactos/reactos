/*
 * Start page
 *
 * Next pages:
 *  LanguagePage (at once, default)
 *  InstallIntroPage (at once, if unattended)
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Init Sdi
 *  Init USetupData.SourcePath
 *  Init USetupData.SourceRootPath
 *  Init USetupData.SourceRootDir
 *  Init USetupData.SetupInf
 *  Init USetupData.RequiredPartitionDiskSpace
 *  Init IsUnattendedSetup
 *  If unattended, init *List and sets the Codepage
 *  If unattended, init SelectedLanguageId
 *  If unattended, init USetupData.LanguageId
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
SetupStartPage(PINPUT_RECORD Ir)
{
    ULONG Error;
    PGENERIC_LIST_ENTRY ListEntry;
    PCWSTR LocaleId;

    MUIDisplayPage(SETUP_INIT_PAGE);

    /* Initialize Setup, phase 1 */
    Error = InitializeSetup(&USetupData, 1);
    if (Error != ERROR_SUCCESS)
    {
        MUIDisplayError(Error, Ir, POPUP_WAIT_ENTER);
        return QUIT_PAGE;
    }

    /* Initialize the user-mode PnP manager */
    if (!EnableUserModePnpManager())
        DPRINT1("The user-mode PnP manager could not initialize, expect unavailable devices!\n");

    /* Wait for any immediate pending installations to finish */
    if (WaitNoPendingInstallEvents(NULL) != STATUS_WAIT_0)
        DPRINT1("WaitNoPendingInstallEvents() failed to wait!\n");

    CheckUnattendedSetup(&USetupData);

    if (IsUnattendedSetup)
    {
        // TODO: Read options from inf
        /* Load the hardware, language and keyboard layout lists */

        USetupData.ComputerList = CreateComputerTypeList(USetupData.SetupInf);
        USetupData.DisplayList = CreateDisplayDriverList(USetupData.SetupInf);
        USetupData.KeyboardList = CreateKeyboardDriverList(USetupData.SetupInf);

        USetupData.LanguageList = CreateLanguageList(USetupData.SetupInf, DefaultLanguage);

        /* new part */
        SelectedLanguageId = DefaultLanguage;
        wcscpy(DefaultLanguage, USetupData.LocaleID);
        USetupData.LanguageId = (LANGID)(wcstol(SelectedLanguageId, NULL, 16) & 0xFFFF);

        USetupData.LayoutList = CreateKeyboardLayoutList(USetupData.SetupInf, SelectedLanguageId, DefaultKBLayout);

        /* first we hack LanguageList */
        for (ListEntry = GetFirstListEntry(USetupData.LanguageList); ListEntry;
             ListEntry = GetNextListEntry(ListEntry))
        {
            LocaleId = ((PGENENTRY)GetListEntryData(ListEntry))->Id;
            if (!wcsicmp(USetupData.LocaleID, LocaleId))
            {
                DPRINT("found %S in LanguageList\n", LocaleId);
                SetCurrentListEntry(USetupData.LanguageList, ListEntry);
                break;
            }
        }

        /* now LayoutList */
        for (ListEntry = GetFirstListEntry(USetupData.LayoutList); ListEntry;
             ListEntry = GetNextListEntry(ListEntry))
        {
            LocaleId = ((PGENENTRY)GetListEntryData(ListEntry))->Id;
            if (!wcsicmp(USetupData.LocaleID, LocaleId))
            {
                DPRINT("found %S in LayoutList\n", LocaleId);
                SetCurrentListEntry(USetupData.LayoutList, ListEntry);
                break;
            }
        }

        SetConsoleCodePage();

        return INSTALL_INTRO_PAGE;
    }

    return LANGUAGE_PAGE;
}
