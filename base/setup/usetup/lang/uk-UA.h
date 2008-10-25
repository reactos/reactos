#ifndef LANG_UK_UA_H__
#define LANG_UK_UA_H__

MUI_LAYOUTS ukUALayouts[] =
{
    { L"0422", L"00000422" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY ukUALanguagePageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Вибiр мови",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Please choose the language used for the installation process.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  This Language will be the default language for the final system.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER=Продовжити  F3=Вихiд",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAWelcomePageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ласкаво просимо до програми установки ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Ця частина установки копiює операцiйну систему ReactOS у Ваш",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "комп'ютер i готує другу частину установки.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Натиснiть <ENTER> щоб устанити ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Натиснiть <R> щоб вiдновити ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Натиснiть <L> щоб переглянути лiцензiйнi умови ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Натиснiть <F3> щоб вийти, не встановлюючи ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Для бiльш конкретної iнформацiї про ReactOS, будь ласка вiдвiдайте:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org/uk/",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "   ENTER=Продовжити  R=Вiдновити F3=Вихiд",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAIntroPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS знаходиться в раннiй стадiї розробки i не пiдтримує всi",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "пiдтримуйте всi функцiї повноцiнного додатку установки.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Є наступнi обмеження:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Установка можлива тiльки на первинний роздiл диска.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- При установцi не можна видалити первинний роздiл диска",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  поки на диску є розширений роздiл.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- При установцi не можна видалити перший розширений роздiл з диска",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  поки на диску iснують iншi розширенi роздiли.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- При установцi пiдтримується тiльки файлова система FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- Перевiрка файлової системи не здiйснюється.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Натиснiть <ENTER> для установки ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Натиснiть <F3> для виходу з установки ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER=Продовжити  F3=Вихiд",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUALicensePageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Лiцензiя:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS лiцензована вiдповiдно до Вiдкритої лiцензiйної",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "угоди GNU GPL i мiстить компоненти, якi поширюються",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "за сумiсними лiцензiями: X11, BSD i GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Все програмне забезпечення, яке входить в систему ReactOS, випущено",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "пiд Вiдкритою лiцензiйною угодою GNU GPL iз збереженням",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "первиннiй лiцензiї.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Дане програмне забезпечення поставляється БЕЗ ГАРАНТiЇ i без обмежень",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "у використаннi, як в мiсцевому, так i мiжнародному правi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "Лiцензiя ReactOS дозволяє передачу продукту третiм особам.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Якщо через будь-якi причини Ви не отримали копiю Вiдкритої",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "лiцензiйної угоди GNU разом з ReactOS, вiдвiдаєте",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        22,
        "Гарантiї:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Це вiльне програмне забезпечення; див. джерело для перегляду прав.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "НЕМАЄ НiЯКИХ ГАРАНТiЙ; немає гарантiї ТОВАРНОГО СТАНУ або",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "ПРИДАТНОСТi ДЛЯ КОНКРЕТНИХ ЦiЛЕЙ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER=Повернутися",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUADevicePageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "У списку нижче приведенi пристрої i їх параметри.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "      Комп'ютер:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "          Екран:",
        TEXT_STYLE_NORMAL,
    },
    {
        8,
        13,
        "     Клавiатура:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Клав. розкладка:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "    Застосувати:",
        TEXT_STYLE_NORMAL
    },
    {
        25,
        16, "Застосувати данi параметри пристроїв",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Ви можете змiнити параметри пристроїв натискаючи клавiшi <ВГОРУ> i <ВНИЗ>",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "для видiлення елементу i клавiшу <ENTER> для вибору iнших варiантiв",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "параметрiв.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Коли всi параметри визначенi, виберiть \"Застосувати данi параметри пристроїв\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "i натиснiть <ENTER>.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER=Продовжити  F3=Вихiд",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUARepairPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Setup is in an early development phase. It does not yet",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "support all the functions of a fully usable setup application.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "The repair functions are not implemented yet.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press U for Updating OS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press R for the Recovery Console.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Press ESC to return to the main page.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Press ENTER to reboot your computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ESC = Main page  ENTER = Reboot",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY ukUAComputerPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You want to change the type of computer to be installed.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired computer type.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   the computer type.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAFlushPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "The system is now making sure all data is stored on your disk",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "This may take a minute",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "When finished, your computer will reboot automatically",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Flushing cache",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAQuitPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS is not completely installed",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Remove floppy disk from Drive A: and",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "all CD-ROMs from CD-Drives.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Press ENTER to reboot your computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Please wait ...",
        TEXT_TYPE_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUADisplayPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You want to change the type of display to be installed.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Press the UP or DOWN key to select the desired display type.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   the display type.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUASuccessPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "The basic components of ReactOS have been installed successfully.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Remove floppy disk from Drive A: and",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "all CD-ROMs from CD-Drive.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Press ENTER to reboot your computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Reboot computer",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUABootPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup cannot install the bootloader on your computers",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "hardisk",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Please insert a formatted floppy disk in drive A: and",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "press ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "   ENTER=Продовжити  F3=Вихiд",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY ukUASelectPartitionEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "The list below shows existing partitions and unused disk",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "space for new partitions.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Press UP or DOWN to select a list entry.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press ENTER to install ReactOS onto the selected partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press C to create a new partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press D to delete an existing partition.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Please wait...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Format partition",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Setup will now format the partition. Press ENTER to continue.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER=Продовжити  F3=Вихiд",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY ukUAInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup installs ReactOS files onto the selected partition. Choose a",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "directory where you want ReactOS to be installed:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "To change the suggested directory, press BACKSPACE to delete",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "characters and then type the directory where you want ReactOS to",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "be installed.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER=Продовжити  F3=Вихiд",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAFileCopyEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        11,
        12,
        "Please wait while ReactOS Setup copies files to your ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        30,
        13,
        "installation folder.",
        TEXT_STYLE_NORMAL
    },
    {
        20,
        14,
        "This may take several minutes to complete.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Please wait...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUABootLoaderEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup is installing the boot loader",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Install bootloader on the harddisk (MBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Install bootloader on a floppy disk.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Skip install bootloader.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER=Продовжити  F3=Вихiд",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You want to change the type of keyboard to be installed.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard type.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   the keyboard type.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUALayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You want to change the keyboard layout to be installed.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    layout. Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   the keyboard layout.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ukUAPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup prepares your computer for copying the ReactOS files. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Building the file copy list...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ukUASelectFSEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Select a file system from the list below.",
        0
    },
    {
        8,
        19,
        "\x07  Press UP or DOWN to select a file system.",
        0
    },
    {
        8,
        21,
        "\x07  Press ENTER to format the partition.",
        0
    },
    {
        8,
        23,
        "\x07  Press ESC to select another partition.",
        0
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUADeletePartitionEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You have chosen to delete the partition",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Press D to delete the partition.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "WARNING: All data on this partition will be lost!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Press ESC to cancel.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   D = Delete Partition   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUARegistryEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup is updating the system configuration. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Creating registry hives...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR ukUAErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS is not completely installed on your\n"
        "computer. If you quit Setup now, you will need to\n"
        "run Setup again to install ReactOS.\n"
        "\n"
        "  \x07  Press ENTER to continue Setup.\n"
        "  \x07  Press F3 to quit Setup.",
        "F3 = Quit  ENTER = Continue"
    },
    {
        //ERROR_NO_HDD
        "Setup could not find a harddisk.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup could not find its source drive.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup failed to load the file TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Setup found a corrupt TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup found an invalid signature in TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup could not retrieve system drive information.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup failed to install FAT bootcode on the system partition.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup failed to load the computer type list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup failed to load the display settings list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup failed to load the keyboard type list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup failed to load the keyboard layout list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_WARN_PARTITION,
          "Setup found that at least one harddisk contains an incompatible\n"
          "partition table that can not be handled properly!\n"
          "\n"
          "Creating or deleting partitions can destroy the partition table.\n"
          "\n"
          "  \x07  Press F3 to quit Setup."
          "  \x07  Press ENTER to continue.",
          "F3= Quit  ENTER = Continue"
    },
    {
        //ERROR_NEW_PARTITION,
        "You can not create a new Partition inside\n"
        "of an already existing Partition!\n"
        "\n"
        "  * Press any key to continue.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "You can not delete unpartitioned disk space!\n"
        "\n"
        "  * Press any key to continue.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the FAT bootcode on the system partition.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_NO_FLOPPY,
        "No disk in drive A:.",
        "ENTER = Continue"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup failed to update keyboard layout settings.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup failed to update display registry settings.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup failed to import a hive file.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup failed to find the registry data files.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup failed to create the registry hives.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet has no valid inf file.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet not found.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet has no setup script.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup failed to open the copy file queue.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup could not create install directories.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in the cabinet.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup could not create the install directory.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Setup failed to find the 'SetupData' section\n"
        "in TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Setup failed to write partition tables.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup failed to add codepage to registry.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Setup failed to add keyboard layouts to registry.\n"
        "ENTER = Reboot computer"
    },
    {
        NULL,
        NULL
    }
};


MUI_PAGE ukUAPages[] =
{
    {
        LANGUAGE_PAGE,
        ukUALanguagePageEntries
    },
    {
        START_PAGE,
        ukUAWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        ukUAIntroPageEntries
    },
    {
        LICENSE_PAGE,
        ukUALicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        ukUADevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        ukUARepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        ukUAComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        ukUADisplayPageEntries
    },
    {
        FLUSH_PAGE,
        ukUAFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        ukUASelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        ukUASelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        ukUAFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        ukUADeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        ukUAInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        ukUAPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        ukUAFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        ukUAKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        ukUABootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        ukUALayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        ukUAQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        ukUASuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        ukUABootPageEntries
    },
    {
        REGISTRY_PAGE,
        ukUARegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING ukUAStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Please wait..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   C = Create Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Install   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "Size of new partition:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a new partition on"},
    {STRING_HDDSIZE,
    "Please enter the size of the new partition in megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Create Partition   ESC = Cancel   F3 = Quit"},
    {STRING_PARTFORMAT,
    "This Partition will be formatted next."},
    {STRING_NONFORMATTEDPART,
    "You chose to install ReactOS on a new or unformatted Partition."},
    {STRING_INSTALLONPART,
    "Setup install ReactOS onto Partition"},
    {STRING_CHECKINGPART,
    "Setup is now checking the selected partition."},
    {STRING_QUITCONTINUE,
    "F3= Quit  ENTER = Continue"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Reboot computer"},
    {STRING_TXTSETUPFAILED,
    "Setup failed to find the '%S' section\nin TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "\xB3 Copying file: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup is copying files..."},
    {STRING_REGHIVEUPDATE,
    "   Updating registry hives..."},
    {STRING_IMPORTFILE,
    "   Importing %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Updating display registry settings..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Updating locale settings..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Updating keyboard layout settings..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Adding codepage information to registry..."},
    {STRING_DONE,
    "   Done..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Reboot computer"},
    {STRING_CONSOLEFAIL1,
    "Unable to open the console\n\n"},
    {STRING_CONSOLEFAIL2,
    "The most common cause of this is using an USB keyboard\n"},
    {STRING_CONSOLEFAIL3,
    "USB keyboards are not fully supported yet\n"},
    {STRING_FORMATTINGDISK,
    "Setup is formatting your disk"},
    {STRING_CHECKINGDISK,
    "Setup is checking your disk"},
    {STRING_FORMATDISK1,
    " Format partition as %S file system (quick format) "},
    {STRING_FORMATDISK2,
    " Format partition as %S file system "},
    {STRING_KEEPFORMAT,
    " Keep current file system (no changes) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK3,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "on Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  Type %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "Setup created a new partition on"},
    {STRING_UNPSPACE,
    "    Unpartitioned space              %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_UNFORMATTED,
    "New (Unformatted)"},
    {STRING_FORMATUNUSED,
    "Unused"},
    {STRING_FORMATUNKNOWN,
    "Unknown"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Adding keyboard layouts"},
    {0, 0}
};

#endif
