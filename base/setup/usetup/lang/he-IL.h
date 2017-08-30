#pragma once

static MUI_ENTRY heILLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " öêóöÑ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ñîô öòâáÅ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ÑêóöÑÑ äåÑéÅ ÑÅ ôéöôöô ÑîôÑ öÄ ÑôóÅÅ òáÅ Äê.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   .ENTER ôóÑ äã òáÄ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  .öâîÖëÑ öãòíéÅ åÉáéÑ öòâòÅ öîô ÑâÑö ÖÜÑ ÑîôÑ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "åàÅ = F3  äôéÑ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ÑêóöÑ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS åô ÑêóöÑÑ öâêãöå ÄÅÑ äÖòÅ",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "ÑåíîÑÑ öãòíé öÄ öÄ óâöíé ÑêóöÑÑ åô ÑÜÑ óåáÑ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "ÑêóöÑÑ åô âêôÑ óåáå èêÖãöéÖ äÅôáéå.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ReactOS öÄ èâóöÑå âÉã ENTER ôóÑ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press R to repair a ReactOS installation using the Recovery Console.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  .ReactOS åô çÖôâòÑ âÄêöÖ âÇôÖé öÄ öÖÄòå âÉã L ôóÑ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  .ReactOS öÄ èâóöÑå âåÅ åàÅå âÉã F3 ôóÑ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        ":Å òóÅ Äê ,ReactOS âÅÇå íÉâé ÉÖíå",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "åàÅ = F3  èÖâôò = L  èóö = R  äôéÑ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ÑêóöÑ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "öãéÖö Äå èââÉí ÄâÑ .çâéÉóÖé áÖöâî ÅåôÅ öÄñéê ReactOS öêóöÑ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        ".ÑÄåé öÖâôÖéâô çí ÑêóöÑ çÖôââ åô öÖâÖòôîÄÑ åãÅ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        ":öÖéôÖâé öÖÄÅÑÑ öÖåÅÇÑÑ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Setup supports FAT file systems only.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- File system checks are not implemented yet.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Press ENTER to install ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Ñéôä   F3 = Åàå",
        TEXT_TYPE_STATUS| TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licensing:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "The ReactOS System is licensed under the terms of the",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL with parts containing code from other compatible",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenses such as the X11 or BSD and GNU LGPL licenses.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "All software that is part of the ReactOS system is",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "therefore released under the GNU GPL as well as maintaining",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "the original license.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "This software comes with NO WARRANTY or restrictions on usage",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "save applicable local and international law. The licensing of",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "ReactOS only covers distribution to third parties.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "If for some reason you did not receive a copy of the",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License with ReactOS please visit",
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
        "Warranty:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "This is free software; see the source for copying conditions.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "There is NO warranty; not even for MERCHANTABILITY or",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "FITNESS FOR A PARTICULAR PURPOSE",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = áÜÖò",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Ñöóêö ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "The list below shows the current device settings.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "éáôÅ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "öñÖÇÑ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "éóåÉö:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "öñÖòö éóåÉö:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "óÅå:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Accept these device settings",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "You can change the hardware settings by pressing the UP or DOWN keys",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "to select an entry. Then press the ENTER key to select alternative",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "ÑÇÉòÖö.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "When all settings are correct, select \"Accept these device settings\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "ÖÄáò ãä Ñóô ENTER",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Ñéôä   F3 = Åàå",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Ñöóêö ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "The ReactOS Setup can upgrade one of the available ReactOS installations",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "listed below, or, if a ReactOS installation is damaged, the Setup program",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "can attempt to repair it.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "The repair functions are not all implemented yet.",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        15,
        "\x07  Press UP or DOWN to select an OS installation.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press U for upgrading the selected OS installation.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Press ESC to continue a new installation without upgrading.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "U = Upgrade   ESC = Do not upgrade   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Ñöóêö ",
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
        "   ÖÄáò ãä Ñóô ENTER.",
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
        "ENTER = Ñéôä   ESC = Åàå   F3 = îòÖô",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Ñöóêö ",
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
        "Flushing cache",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Ñöóêö ",
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
        "Please wait ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Ñöóêö ",
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
        "ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ÑòãâÅâç ÑÅëâëââç ôå ReactOS ÑÖöóêÖ ÅÑñåáÑ.",
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
        "Ñóô ENTER ãÉâ åÑîíâå éáÉô Äö éáôÅä",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Ñîíå éáÉô",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILBootPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Ñöóêö ",
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
        "ENTER = Ñéôä   F3 = Åàå ÑöóêÑ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY heILSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ÑöóêÑ ",
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
        "\x07  Press P to create a primary partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press E to create an extended partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Press L to create a logical partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Press D to delete an existing partition.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "êÄ åáãÖö...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You have chosen to delete the system partition.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "System partitions can contain diagnostic programs, hardware configuration",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "programs, programs to start an operating system (like ReactOS) or other",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "programs provided by the hardware manufacturer.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Delete a system partition only when you are sure that there are no such",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "programs on the partition, or when you are sure you want to delete them.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "When you delete the partition, you might not be able to boot the",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "computer from the harddisk until you finished the ReactOS Setup.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Press ENTER to delete the system partition. You will be asked",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   to confirm the deletion of the partition again later.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  Press ESC to return to the previous page. The partition will",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "   not be deleted.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continue  ESC=Cancel",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ÄöáÖå éáâñÑ",
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
        "ENTER = Ñéôä   F3 = Åàå ÑöóêÑ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY heILInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "ENTER = Ñéôä   F3 = Åàå ÑöóêÑ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Ñöóêö ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Please wait while ReactOS Setup copies files to your ReactOS",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "installation folder.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "This may take several minutes to complete.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 êÄ åáãÖö...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "Install bootloader on the harddisk (MBR and VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Install bootloader on the harddisk (VBR only).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Install bootloader on a floppy disk.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Skip install bootloader.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Ñéôä   F3 = Åàå ÑöóêÑ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "ENTER = Ñéôä   ESC = ÅâàÖå   F3 = Åàå ÑöóêÑ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Please select a layout to be installed by default.",
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
        "ENTER = Ñéôä   ESC = ÅâàÖå   F3 = Åàå ÑöóêÑ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY heILPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "Building the file copy list...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY heILSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "D = Delete Partition   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "Creating registry hives...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR heILErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
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
          "  \x07  Press F3 to quit Setup.\n"
          "  \x07  Press ENTER to continue.",
          "F3 = Quit  ENTER = Continue"
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
        "Setup failed to initialize the registry.",
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
        //ERROR_UPDATE_GEOID,
        "Setup could not set the geo id.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Invalid directory name.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "The selected partition is not large enough to install ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "\n"
        "  * Press any key to continue.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "You can not create a new primary or extended partition in the\n"
        "partition table of this disk because the partition table is full.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "You can not create more than one extended partition per disk.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        //ERROR_FORMATTING_PARTITION,
        "Setup is unable to format the partition:\n"
        " %S\n"
        "\n"
        "ENTER = Reboot computer"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE heILPages[] =
{
    {
        LANGUAGE_PAGE,
        heILLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        heILWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        heILIntroPageEntries
    },
    {
        LICENSE_PAGE,
        heILLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        heILDevicePageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        heILUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        heILComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        heILDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        heILFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        heILSelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        heILConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        heILSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        heILFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        heILDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        heILInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        heILPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        heILFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        heILKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        heILBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        heILLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        heILQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        heILSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        heILBootPageEntries
    },
    {
        REGISTRY_PAGE,
        heILRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING heILStrings[] =
{
    {STRING_PLEASEWAIT,
     "   êÄ åáãÖö..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   P = Create Primary   E = Create Extended   F3 = Quit"},
//     "   ENTER = Ñöóè   C = ñÖò éáâñÑ   F3 = Åàå ÑöóêÑ"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   L = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Ñöóè   D = éáó éáâñÑ   F3 = Åàå ÑöóêÑ"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "ÇÖÉå ÑéáâñÑ ÑáÉôÑ:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a primary partition on"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDDSIZE,
    "Please enter the size of the new partition in megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = ñÖò éáâñÑ   ESC = ÅâàÖå   F3 = Åàå ÑöóêÑ"},
    {STRING_PARTFORMAT,
    "This Partition will be formatted next."},
    {STRING_NONFORMATTEDPART,
    "You chose to install ReactOS on a new or unformatted Partition."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Setup installs ReactOS onto Partition"},
    {STRING_CHECKINGPART,
    "Setup is now checking the selected partition."},
    {STRING_CONTINUE,
    "ENTER = Ñéôä"},
    {STRING_QUITCONTINUE,
    "F3 = Åàå ÑöóêÑ  ENTER = Ñéôä"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Ñîíå éáÉô Äö ÑéáôÅ"},
    {STRING_TXTSETUPFAILED,
    "Setup failed to find the '%S' section\nin TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   éíöâó óÖÅï: %S"},
    {STRING_SETUPCOPYINGFILES,
     "öãêâö ÑÑöóêÑ éíöâóÑ óÅñâç..."},
    {STRING_REGHIVEUPDATE,
    "   Updating registry hives..."},
    {STRING_IMPORTFILE,
    "   éââÅÄ %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Updating display registry settings..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Updating locale settings..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Updating keyboard layout settings..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Adding codepage information to registry..."},
    {STRING_DONE,
    "   ëâÖç..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Ñîíå éáÉô Äö ÑéôÅ"},
    {STRING_CONSOLEFAIL1,
    "Unable to open the console\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "The most common cause of this is using an USB keyboard\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB keyboards are not fully supported yet\r\n"},
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
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "on Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sType %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Setup created a new partition on"},
    {STRING_UNPSPACE,
    "    %sUnpartitioned space%s            %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "New (Unformatted)"},
    {STRING_FORMATUNUSED,
    "Unused"},
    {STRING_FORMATUNKNOWN,
    "Unknown"},
    {STRING_KB,
    "óâåÖÅââà"},
    {STRING_MB,
    "éÇÑÅââà"},
    {STRING_GB,
    "ÇâÇÑÅââà"},
    {STRING_ADDKBLAYOUTS,
    "Adding keyboard layouts"},
    {0, 0}
};
