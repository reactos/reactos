#ifndef LANG_BG_BG_H__
#define LANG_BG_BG_H__

static MUI_ENTRY bgBGLanguagePageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Избор на език",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Изберете език, който да изполвате при слагането.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   натиснете ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Езикът ще бъде подразбираният за крайната уредба.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване  F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "РеактОС ви приветства!",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Тази част от настройката записва работната уредба РеактОС",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "на компютъра ви и подготвя втората част на настройката.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Натиснете ENTER за слагане на РеактОС.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Натиснете R за поправка на РеактОС.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Натиснете L, за да видите разрешителните (лицензните)",
        TEXT_NORMAL
    },
        {
        8,
        20,
        "   изисквания и условия на РеактОС",
        TEXT_NORMAL
    },
    {
        8,
        22,
        "\x07  Натиснете F3 за изход без слагане на РеактОС.",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "За повече сведения за РеактОС, посетете:",
        TEXT_NORMAL
    },
    {
        6,
        25,
        "http://www.reactos.org",
        TEXT_HIGHLIGHT
    },
    {
        0,
        0,
        "   ENTER = Продължаване   R = Поправка   F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Настройвачът на РеактОС е в ранна степен на разработка. Все още",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "няма всички възможности на напълно използваемо настройващо приложение.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Съществуват следните ограничения:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- Настройвачът не може да работи с повече от един дял на диск.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Настройвачът не може да изтриe първичeн дял,",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  ако на диска има разширен дял." ,
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Настройвачът не може да изтрие първия разширен дял от диска",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  ако на диска има и други разширени дялове.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- Настройвачът поддържа само FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- Проверката на файловата уредба все не е готова.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  Натиснете ENTER за слагане на РеактОС.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Натиснете F3 за изход без слагане на РеактОС.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        6,
        "Лицензиране:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        8,
        "Уредбата РеактОС е лицензирана при условията на GNU GPL",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "с части, съдържащи код от други съвместими лицензи като",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "X11, BSD или GNU LGPL.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "Следователно всяко осигуряване, което е част от",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "РеактОС, се обнародва под GNU GPL, с поддържането на",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "на оригиналния лиценз.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "This software comes with NO WARRANTY or restrictions on usage",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "save applicable local and international law. The licensing of",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "ReactOS only covers distribution to third parties.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "Ако поради някаква причина, заедно с РеактОС не сте",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "получили копие на GNU General Public License, посетете",
        TEXT_NORMAL
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_HIGHLIGHT
    },
    {
        8,
        22,
        "Гаранция:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        24,
        "This is free software; see the source for copying conditions.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "There is NO warranty; not even for MERCHANTABILITY or",
        TEXT_NORMAL
    },
    {
        8,
        26,
        "FITNESS FOR A PARTICULAR PURPOSE",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Връщане",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Списъкът по- долу показва текущите настройки на устройствата.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "       Компютър:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "          Екран:",
        TEXT_NORMAL,
    },
    {
        3,
        13,
        "          Клавиатура:",
        TEXT_NORMAL
    },
    {
        3,
        14,
        "Клавиатурна подредба:",
        TEXT_NORMAL
    },
    {
        3,
        16,
        "            Приемане:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Приемане на настройките",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "За да промените настройките на оборудването, използвайте стрелките",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "НАГОРЕ и НАДОЛУ. След това натиснете ENTER, за да изберете",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "заместващи настройки.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Когато направите всички настройки, изберете 'Приемане на настройките'",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "и натиснете ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGRepairPageEntries[] =
{
    {
        4,
        3,
        " Слагане на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Настройвачът на РеактОС е в ранна степен на разработка. Все още",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "няма всички възможности на напълно използваемо настройващо приложение.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Възможността за поправка още не е готова.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  натиснете U за обновяване на операционната система.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Натиснете R за възстановяваща среда (конзола).",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  натсинете ESC за връщане към главната страница.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Натиснете ENTER за презапуск на компютъра.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Главна страница  ENTER = Презапуск",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Решили сте да смените вида на компютъра.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Изберете вида на екрана със стрелките нагоре и надолу и ",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   натиснете ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснете ESC, за да се върнете към предходната страница, без да",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   вида на компютъра.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   ESC = Отказ   F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Уредбата проверява, дали всички данни са съхранени на диска ви.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Това ще отнеме минутка.",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "Компютърът ви ще се презапусне сам, когато приключи.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Изчистване на склада",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Слагането на РеактОС не е завършило.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Извадете дискетата от устройство А: и",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "всички носители от КД и DVD устройствата.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Натиснете ENTER, за да презапуснете компютъра.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Изчакайте...",
        TEXT_STATUS,
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Решили сте да смените вида на екрана.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Изберете вида на екрана със стрелките нагоре и надолу и ",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   натиснете ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснете ESC, за да се върнете към предходната страница, без да",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   вида на екрана.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   ESC = Отказ   F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Основните съставки на РеактОС са сложени успешно.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Извадете дискетата от устройство А: и",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "всички носители от оптичните устройства (КД/DVD)",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Натиснете ENTER, за да презапуснете компютъра.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = презапускане на компютъра",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Слагането на зареждача (bootloader) на диска на компютъра ви",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "бе неуспешно.",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Сложете форматирана дискета в устройство A:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "и натиснете ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Списъкът по- долу съдържа съществуващите дялове и празното",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "място за нови дялове",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "\x07  Изполвайте стрелките за избор от списъка.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснете ENTER за слагане на РеактОС на избрания дял.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Натиснете C за създаване на нов дял.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Натиснете D за изтриване на съществуващ дял.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Почакайте...",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Форматиране на дял",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "Настройката ще форматира дяла. Натиснете ENTER за продължаване.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_NORMAL
    }
};

static MUI_ENTRY bgBGInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Настройка на РеактОС " KERNEL_VERSION_STR " . ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Файловете на РеактОС ще бъдат сложени на избрания дял. Изберете",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "папка, в която да бъде сложен РеактОС:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "За смяна на предложената папка натиснете BACKSPACE, за да",
        TEXT_NORMAL
    },
    {
        6,
        15,
        "изтриете знаците и тогава напишете папката, в която да бъде",
        TEXT_NORMAL
    },
    {
        6,
        16,
        "сложен РеактОС.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        11,
        12,
        "Изчакайте да приключи записът на файловете в избраната папка.",
        TEXT_NORMAL
    },
    {
        30,
        13,
       // "избраната папка.",
       "",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "Това може да отнеме няколко минути.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Почакайте...      ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Протича слагането на зареждача.",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Слагане на зареждач на твърдия диск (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Слагане на зареждач на дискета.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Да не се слага зареждач.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Искате да смените вида на клавиатурата.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Използвайте стрелките, за да изберете вида на клавиатура.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   натиснете ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснете ESC, за да се върнете към предходната страница, без да",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   вида на клавиатурата.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   ESC = Отказ   F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Искате да смените клавиатурната подредба.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Използвайте стрелките, за да изберете желаната клавиатурна",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "    подредба и после натиснете ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснете ESC, за да се върнете към предходната страница, без да",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   клавиатурната подредба.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Продължаване   ESC = Отказ   F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Компютърът се подготвя за запис на файловете на РеактОС. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Съставяне на списъка от файлове за запис...",
        TEXT_STATUS
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
        TEXT_UNDERLINE
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
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Избрали сте да изтриете дял",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Натиснете D, за да изтриете дяла.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "ВНИМАНИЕ: Всички данни на този дял ще бъдат унищожени!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Натиснете ESC за да се откажете.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = изтриване на дяла, ESC = Отказ    F3 = Изход",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Протича обновяване на системните настройки. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Създаване на регистърните роеве...",
        TEXT_STATUS
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
        "Setup failed to install FAT bootcode on the system partition.",
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
          "  \x07  За изход натиснете F3."
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
        "Setup failed to set the initialize the registry.",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet has no valid inf file.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet not found.\n",
        "ENTER = Презапускане на компютъра"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet has no setup script.\n",
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
        "Setup failed to find the 'Directories' section\n"
        "in the cabinet.\n",
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
        "Setup could not set the system locale.\n"
        "ENTER = Презапускане на компютъра"
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
        START_PAGE,
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
        REPAIR_INTRO_PAGE,
        bgBGRepairPageEntries
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
     "                                          \xB3 Copying file: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup is copying files..."},
    {STRING_PAGEDMEM,
     "Paged Memory"},
    {STRING_NONPAGEDMEM,
     "Nonpaged Memory"},
    {STRING_FREEMEM,
     "Free Memory"},
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
    {0, 0}
};

#endif
