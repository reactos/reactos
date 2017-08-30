#pragma once

static MUI_ENTRY bgBGLanguagePageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Избор на език",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Изберете език, който да изполвате при слагането.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   натиснете ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Езикът ще бъде подразбираният за крайната уредба.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване  F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGWelcomePageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "РеактОС ви приветства!",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Тази част от настройката записва работната уредба РеактОС",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "на компютъра ви и подготвя втората част на настройката.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Натиснете ENTER за слагане на РеактОС.",
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
        "\x07  Натиснете L, за да видите разрешителните (лицензните)",
        TEXT_STYLE_NORMAL
    },
        {
        8,
        20,
        "   изисквания и условия на РеактОС",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        22,
        "\x07  Натиснете F3 за изход без слагане на РеактОС.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "За повече сведения за РеактОС, посетете:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        25,
        "http://www.reactos.org",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "   ENTER = Продължаване   R = Поправка   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGIntroPageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Настройвачът на РеактОС е в ранна степен на разработка. Все още",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "няма всички възможности на напълно използваемо настройващо приложение.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Съществуват следните ограничения:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Настройвачът поддържа само FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Проверката на файловата уредба все още не е готова.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Натиснете ENTER за слагане на РеактОС.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Натиснете F3 за изход без слагане на РеактОС.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGLicensePageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Лицензиране:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "Уредбата РеактОС е лицензирана при условията на GNU GPL",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "с части, съдържащи код от други съвместими лицензи като",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "X11, BSD или GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Следователно всяко осигуряване, което е част от",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "РеактОС, се обнародва под GNU GPL, с поддържането на",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "на оригиналния лиценз.",
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
        "Ако поради някаква причина, заедно с РеактОС не сте",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "получили копие на GNU General Public License, посетете",
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
        "Гаранция:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Това е свободен софтуер; вижте изходния код за условията на възпроизвеждане.",
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
        "   ENTER = Връщане",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGDevicePageEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Списъкът по- долу показва текущите настройки на устройствата.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "       Компютър:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "          Екран:",
        TEXT_STYLE_NORMAL,
    },
    {
        3,
        13,
        "          Клавиатура:",
        TEXT_STYLE_NORMAL
    },
    {
        3,
        14,
        "Клавиатурна подредба:",
        TEXT_STYLE_NORMAL
    },
    {
        3,
        16,
        "            Приемане:",
        TEXT_STYLE_NORMAL
    },
    {
        25,
        16, "Приемане на настройките",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "За да промените настройките на оборудването, използвайте стрелките",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "НАГОРЕ и НАДОЛУ. След това натиснете ENTER, за да изберете",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "заместващи настройки.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Когато направите всички настройки, изберете 'Приемане на настройките'",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "и натиснете ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGUpgradePageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
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

static MUI_ENTRY bgBGComputerPageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Решили сте да смените вида на компютъра.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Изберете вида на екрана със стрелките нагоре и надолу и ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   натиснете ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснете ESC, за да се върнете към предходната страница, без да",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   сменяте вида на компютъра.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   ESC = Отказ   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGFlushPageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Уредбата проверява, дали всички данни са съхранени на диска ви.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Това ще отнеме минутка.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Компютърът ви ще се презапусне сам, когато приключи.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Изчистване на склада",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGQuitPageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Слагането на РеактОС не е завършило.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Извадете дискетата от устройство А: и",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "всички носители от КД и DVD устройствата.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Натиснете ENTER, за да презапуснете компютъра.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Изчакайте...",
        TEXT_TYPE_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGDisplayPageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Решили сте да смените вида на екрана.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Изберете вида на екрана със стрелките нагоре и надолу и ",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   натиснете ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснете ESC, за да се върнете към предходната страница, без да",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   сменяте вида на екрана.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   ESC = Отказ   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGSuccessPageEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Основните съставки на РеактОС са сложени успешно.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Извадете дискетата от устройство А: и",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "всички носители от оптичните устройства (КД/DVD)",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Натиснете ENTER, за да презапуснете компютъра.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Презапускане на компютъра",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGBootPageEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Слагането на зареждача (bootloader) на диска на компютъра ви",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "бе неуспешно.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Сложете форматирана дискета в устройство A:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "и натиснете ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY bgBGSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Списъкът по- долу съдържа съществуващите дялове и празното",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "място за нови дялове",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Използвайте стрелките за избор от списъка.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснете ENTER за слагане на РеактОС на избрания дял.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press P to create a primary partition.",
//        "\x07  Натиснете C за създаване на нов дял.",
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
        "\x07  Натиснете D за изтриване на съществуващ дял.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Почакайте...",  /* Редът да не се превежда, защото списъкът с дяловете ще се размести */
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
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

static MUI_ENTRY bgBGFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Форматиране на дял",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Дялът ще бъде форматиран. Натиснете ENTER за продължаване.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY bgBGInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Файловете на РеактОС ще бъдат сложени на избрания дял. Изберете",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "папка, в която да бъде сложен РеактОС:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "За смяна на предложената папка натиснете BACKSPACE, за да",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "изтриете знаците и тогава напишете папката, в която да бъде",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "сложен РеактОС.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGFileCopyEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        11,
        12,
        "Изчакайте да приключи записът на файловете в избраната папка.",
        TEXT_STYLE_NORMAL
    },
    {
        30,
        13,
       // "избраната папка.",
       "",
        TEXT_STYLE_NORMAL
    },
    {
        20,
        14,
        "Това може да отнеме няколко минути.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Почакайте...      ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGBootLoaderEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Протича слагането на зареждача.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Слагане на зареждач на твърдия диск (MBR и VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Слагане на зареждач на твърдия диск (само VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Слагане на зареждач на дискета.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Да не се слага зареждач.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Искате да смените вида на клавиатурата.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Използвайте стрелките, за да изберете вида на клавиатура.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   натиснете ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснете ESC, за да се върнете към предходната страница, без да",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   сменяте вида на клавиатурата.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   ESC = Отказ   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Изберете подразбирана клавиатурна подредба.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Използвайте стрелките, за да изберете желаната клавиатурна",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    подредба и после натиснете ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснете ESC, за да се върнете към предходната страница, без да",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   сменяте клавиатурната подредба.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   ESC = Отказ   F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY bgBGPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Компютърът се подготвя за запис на файловете на РеактОС. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Съставяне на списъка от файлове за запис...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY bgBGSelectFSEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "За да изберете файлова система от долния списък:",
        0
    },
    {
        8,
        19,
        "\x07  Изберете файлова система със стрелките.",
        0
    },
    {
        8,
        21,
        "\x07  Натиснете ENTER, за да форматирате дяла.",
        0
    },
    {
        8,
        23,
        "\x07  Натиснете ESC, за да изберете друг дял.",
        0
    },
    {
        0,
        0,
        "   ENTER = Продължаване   ESC = Отказ   F3 = Изход",
        TEXT_TYPE_STATUS
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Избрали сте да изтриете дял",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Натиснете D, за да изтриете дяла.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ВНИМАНИЕ: Всички данни на този дял ще бъдат унищожени!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Натиснете ESC, за да се откажете.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   D = Изтриване на дяла, ESC = Отказ    F3 = Изход",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGRegistryEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Протича обновяване на системните настройки. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Създаване на регистърните роеве...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR bgBGErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "РеактОС не е напълно сложен на компютъра\n"
        "ви. Ако сега излезете от слаганете, ще трябва\n"
        "да пуснете настройката отново, за да инсталирате РеактОС.\n"
        "\n"
        "  \x07  За да продължи слагането, натиснете ENTER.\n"
        "  \x07  За изход натиснете F3.",
        "F3 = Изход ENTER = Продължаване"
    },
    {
        //ERROR_NO_HDD
        "Настройвачът не намери твърд диск.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Настройвачът не намери изходното си устройство.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Настройвачът не успя да намери файл TXTSETUP.SIF.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Настройвачът намери повреден файл TXTSETUP.SIF.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Настройвачът намери нереден подпис в TXTSETUP.SIF.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Настройвачът не успя да открие сведенията на системното ви устройство.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_WRITE_BOOT,
        "Неуспешно слагане на означаваш запис (bootcode) за FAT в системния дял.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Настройвачът не успя да зареди списъка с видовете компютри.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Настройвачът не успя да зареди списъка с натройки за монитори.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Настройвачът не успя да зареди списъка с видовете клавиатури.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Настройвачът не успя да зареди списъка с клавиатурните подредби.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_WARN_PARTITION,
          "Настройвачът установи, че поне един твърд диск съдържа несъвместима\n"
          "дялова таблица, с която не може да се работи правилно!\n"
          "\n"
          "Създаването или изтриването на дялове може да унищожи дяловата таблица.\n"
          "\n"
          "  \x07  За изход натиснете F3.\n"
          "  \x07  Натиснете ENTER за продължаване.",
          "F3 = Изход ENTER = Продължаване"
    },
    {
        //ERROR_NEW_PARTITION,
        "Не можете да създадете нов дял в дял,\n"
        "който вече съществува!\n"
        "\n"
        "  * Натиснете клавиш, за да продължите.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Не можете за изтриете неразпределеното дисково място!\n"
        "\n"
        "  * Натиснете клавиш, за да продължите.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        //"Setup failed to install the FAT bootcode on the system partition.",
        "Неуспешно слагане на обуващия код за FAT на системния дял.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_NO_FLOPPY,
        "В устройство A: няма носител.",
        "ENTER = Продължаване"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Неуспешно обновяване на настройките на клавиатурната подредба.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Неуспешно обновяване на регистърни настройки за монитора.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Неуспешно внасяне на роевия файл.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_FIND_REGISTRY
        "Не бяха открити файловете с регистърни данни.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_CREATE_HIVE,
        "Настройвачът не успя да създаде регистърните роеве.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        //There is something wrong with this line.
        "Неуспешно задаване на начални стойности на регистъра.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cab файлът няма правилен inf файл.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cab файлът не е открит.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cab файлът няма настроечно писание.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_COPY_QUEUE,
        "Неуспешно отваряне на опашката от файлове за запис.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_CREATE_DIR,
        "Неуспешно създаване на папките за слагане.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Разделът 'Directories' не бе открит\n"
        "в TXTSETUP.SIF.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_CABINET_SECTION,
        "Разделът 'Directories' не бе открит\n"
        "в cab файла.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Неуспешно създаване на папката за слагане.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Разделът 'SetupData' не бе открит\n"
        "в TXTSETUP.SIF.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Неуспешно записване на дяловите таблици.\n"
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Неуспешно добавяне на знаковия набор в регистъра.\n"
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Неуспешно установяване на местните настройки.\n"
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Неуспешно добавяне на клавиатурните подредби в регистъра.\n"
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Настройката не можа да установи означителя на географското положение.\n"
        "ENTER = Презапускане на компютъра"
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
        "  * Натиснете клавиш, за да продължите.",
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

MUI_PAGE bgBGPages[] =
{
    {
        LANGUAGE_PAGE,
        bgBGLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        bgBGWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        bgBGIntroPageEntries
    },
    {
        LICENSE_PAGE,
        bgBGLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        bgBGDevicePageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        bgBGUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        bgBGComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        bgBGDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        bgBGFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        bgBGSelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        bgBGConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        bgBGSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        bgBGFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        bgBGDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        bgBGInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        bgBGPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        bgBGFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        bgBGKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        bgBGBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        bgBGLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        bgBGQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        bgBGSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        bgBGBootPageEntries
    },
    {
        REGISTRY_PAGE,
        bgBGRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING bgBGStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Почакайте..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   P = Create Primary   E = Create Extended   F3 = Quit"},
//     "   ENTER = Слагане   C = Създаване на дял   F3 = Изход"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   L = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Слагане   D = Изтриване на дял   F3 = Изход"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "Размер на новия дял:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a primary partition on"},
//     "Избрали сте да създадете нов дял на"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDDSIZE,
    "Въведете размера на новия дял (в мегабайти)."},
    {STRING_CREATEPARTITION,
     "   ENTER = Създаване на дял   ESC = Отказ   F3 = Изход"},
    {STRING_PARTFORMAT,
    "Предстои форматиране на дяла."},
    {STRING_NONFORMATTEDPART,
    "Избрали сте да сложите РеактОС на нов или неразпределен дял."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Слагане на РеактОС върху дял"},
    {STRING_CHECKINGPART,
    "Тече проверка на избрания дял."},
    {STRING_CONTINUE,
    "ENTER = Продължаване"},
    {STRING_QUITCONTINUE,
    "F3 = Изход  ENTER = Продължаване"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Презапускане на компютъра"},
    {STRING_TXTSETUPFAILED,
    "Не бе намерен раздел '%S'\nв TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Запис на файл: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Файловете се записват..."},
    {STRING_REGHIVEUPDATE,
    "   Осъвременяване на регистърните роеве..."},
    {STRING_IMPORTFILE,
    "   Внасяне на %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Осъвременяване регистровите настройки на екрана..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Осъвременяване на местните настройки..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Осъвременяване настройките на клавиатурните подредби..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Добавяне в регистъра на сведения за знаковия набор..."},
    {STRING_DONE,
    "   Готово..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Презапускане на компютъра"},
    {STRING_CONSOLEFAIL1,
    "Отварянето на конзолата е невъзможно\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Това се случва най- често при употреба на USB клавиатура\r\n"},
    {STRING_CONSOLEFAIL3,
    "Поддръжката на USB е все още непълна\r\n"},
    {STRING_FORMATTINGDISK,
    "Тече форматиране на диска"},
    {STRING_CHECKINGDISK,
    "Тече проверка на диска"},
    {STRING_FORMATDISK1,
    " Форматиране на дяла като %S файлова уредба (бързо форматиране) "},
    {STRING_FORMATDISK2,
    " Форматиране на дяла като %S файлова уредба "},
    {STRING_KEEPFORMAT,
    " Запазване на файловата уредба (без промени) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  твърд диск %lu  (Извод=%hu, Шина=%hu, ОУ=%hu) на %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  твърд диск %lu  (Извод=%hu, Шина=%hu, ОУ=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  вид 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "на %I64u %s  твърд диск %lu  (Извод=%hu, Шина=%hu, ОУ=%hu) на %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "на %I64u %s  твърд диск %lu  (Извод=%hu, Шина=%hu, ОУ=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "твърд диск %lu (%I64u %s), Извод=%hu, Шина=%hu, ОУ=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  вид 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "на твърд диск %lu (%I64u %s), Извод=%hu, Шина=%hu, ОУ=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sвид %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  твърд диск %lu  (Извод=%hu, Шина=%hu, ОУ=%hu) на %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  твърд диск %lu  (Извод=%hu, Шина=%hu, ОУ=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Бе създаден нов дял на"},
    {STRING_UNPSPACE,
    "    %sНеразпределено място%s            %6lu %s"},
    {STRING_MAXSIZE,
    "МБ (до %lu МБ)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "Нов (Неформатиран)"},
    {STRING_FORMATUNUSED,
    "Неизползван"},
    {STRING_FORMATUNKNOWN,
    "Неизвестен"},
    {STRING_KB,
    "КБ"},
    {STRING_MB,
    "МБ"},
    {STRING_GB,
    "ГБ"},
    {STRING_ADDKBLAYOUTS,
    "Добавяне на клавиатурни подредби"},
    {0, 0}
};
