/*
 *      translated by Artem Reznikov, Igor Paliychuk, 2010
 *      http://www.reactos.org/uk/
 */

#pragma once

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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
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
        "\x07  Будь-ласка, виберiть мову, яка буде використана пiд час встановлення.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   i натиснiть ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Ця мова буде використовуватись по замовчуванню у встановленiй системi.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продовжити  F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ласкаво просимо до програми встановлення ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "На цьому етапi встановлення вiдбудеться копiювання ReactOS на Ваш",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "комп'ютер i пiдготовка до другого етапу встановлення.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Натиснiть ENTER щоб встановити ReactOS.",
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
        "\x07  Натиснiть L для перегляду лiцензiйних умов ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Натиснiть F3 щоб вийти. не встановлюючи ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Для отримання детальнiшої iнформацiї про ReactOS, будь-ласка вiдвiдайте:",
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
        "ENTER = Продовжити  R = Вiдновити  L = Лiцензiя  F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Встановлювач ReactOS знаходиться в раннiй стадiї розробки i ще не",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "пiдтримує всi функцiї повноцiнної програми встановлення.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Присутнi наступнi обмеження:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Встановлювач пiдтримує лише файлову систему FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Перевiрка файлової системи ще не впроваджена.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Натиснiть ENTER щоб встановити ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Натиснiть F3 щоб вийти, не встановлюючи ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продовжити   F3 = Вийти",
        TEXT_TYPE_STATUS| TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
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
        "ReactOS лiцензовано вiдповiдно до умов",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL. Також ReactOS мiстить компоненти, якi лiцензовано",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "за сумiсними лiцензiями: X11, BSD, GNU LGPL.",
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
        "первинних лiцензiї.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Дане програмне забезпечення поставляється БЕЗ ГАРАНТIЇ i без обмежень",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "у використаннi, як за мiсцевим, так i мiжнародним правом",
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
        "Це є вiльне програмне забезпечення; див. джерело для перегляду прав.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "Не даються НIЯКI гарантiї; нi гарантiї ТОВАРНОГО СТАНУ, нi ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "ПРИДАТНОСТI ДЛЯ КОНКРЕТНИХ ЦIЛЕЙ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Повернутись",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "У списку нижче приведенi поточнi параметри пристроїв.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Комп'ютер:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Екран:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Клавiатура:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Клав. розкладка:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Прийняти:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Застосувати данi параметри пристроїв",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Ви можете змiнити параметри пристроїв натискаючи клавiшi ВГОРУ i ВНИЗ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "для видiлення елементу i клавiшу ENTER для вибору iнших варiантiв",
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
        "Коли всi параметри будуть визначенi, виберiть",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "\"Застосувати данi параметри пристроїв\" i натиснiть ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продовжити   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAUpgradePageEntries[] =
{
    {
        4,
        3,
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY ukUAComputerPageEntries[] =
{
    {
        4,
        3,
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Тут Ви можете змiнити тип Вашого комп'ютера.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Натискайте клавiшi ВВЕРХ та ВНИЗ для вибору типу Вашого комп'ютера",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   i натиснiть ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснiть ESC для повернення до попередньої сторiнки без змiни",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   типу комп'ютера.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продовжити   ESC = Скасувати   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Система перевiряє чи всi данi збережено на диск",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Це може зайняти декiлька хвилин",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Пiсля завершення комп'ютер буде автоматично перезавантажено",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Очищую кеш",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS не встановлено повнiстю",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Витягнiть дискуту з дисководу A: та",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "всi CD-ROM з CD-приводiв.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Натиснiть ENTER щоб перезавантажити комп'ютер.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Будь-ласка зачекайте ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Тут ви можете змiнити тип екрану.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Натискайте клавiшi ВВЕРХ та ВНИЗ для вибору потрiбного типу монiтору",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   i натиснiть ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснiть ESC для повернення до попередньої сторiнки без змiни",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   типу монiтора.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продовжити   ESC = Скасувати   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Основнi компоненти ReactOS були успiшно встановленi.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Витягнiть дискету з дисководу A: та",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "всi CD-ROM з CD-приводiв.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Натиснiть ENTER щоб перезавантажити комп'ютер.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Перезавантажити комп'ютер",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Встановлювач не може встановити bootloader на жорсткий диск",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Вашого комп'ютера",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Будь-ласка вставте вiдформатовану дискету в дивковод A: та",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "натиснiть ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = Продовжити   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Нижче приведений список iснуючих роздiлiв та незайнятого мiсця, де можна",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "створити новi роздiли.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Натискайте клавiшi ВВЕРХ та ВНИЗ для вибору пункту.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснiть ENTER щоб встановити ReactOS на вибраний роздiл.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press P to create a primary partition.",
//        "\x07  Натиснiть C щоб створити новий роздiл.",
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
        "\x07  Натиснiть D щоб видалити iснуючий роздiл.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Please wait...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY ukUAFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Форматування роздiлу",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Зараз встановлювач вiдформатує роздiл. Натиснiть ENTER для продовження.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продовжити   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Встановлювач встановить файли ReactOS на вибраний роздiл. Виберiть",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "директорiю, в яку Ви хочете встановити ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Щоб змiнити директорiю натиснiть BACKSPACE для видалення",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "символiв пiсля чого введiть назву директорiї для",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "встановлення ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продовжити   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Будь-ласка, зачекайте поки встановлювач ReactOS копiює файли до",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "папки призначення.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Це може зайняти декiлька хвилин.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 Будь-ласка зачекайте...    ",
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Встановлювач встановлює завантажувач",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Встановити завантажувач на жорсткий диск (MBR i VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Встановити завантажувач на жорсткий диск (лише VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Встановити завантажувач на дискету.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Не встановлювати завантажувач.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продовжити   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Тут Ви можете змiнити тип клавiатури.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Натискайте клавiшi ВВЕРХ та ВНИЗ для вибору потрiбного типу",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   клавiатури i натиснiть ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснiть ESC для повернення на попередню сторiнку без змiни",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   типу клавiатури.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продовжити   ESC = Скасувати   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Виберiть розкладку, яка буде встановлена яка стандартна.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Натискайте клавiшi ВВЕРХ та ВНИЗ для вибору потрiбної розкладки",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    клавiатури i натиснiть ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Натиснiть ESC для повернення на попередню сторiнку без змiни",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   розкладки клавiатури.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продовжити   ESC = Скасувати   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Встановлювач готує Ваш комп'ютер для копiювання файлiв ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Генерую список файлiв...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Виберiть файлову систему зi списку нижче.",
        0
    },
    {
        8,
        19,
        "\x07  Натискайте клавiшi ВВЕРХ та ВНИЗ для вибору файлової системи.",
        0
    },
    {
        8,
        21,
        "\x07  Натиснiть ENTER щоб вiдформатувати роздiл.",
        0
    },
    {
        8,
        23,
        "\x07  Натиснiть ESC для вибору iншого роздiлу.",
        0
    },
    {
        0,
        0,
        "ENTER = Продовжити   ESC = Скасувати   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ви вибрали видалення роздiлу",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Натиснiть D для видалення роздiлу.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "УВАГА: Всi данi на цьому роздiлi будуть втраченi!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Натиснiть ESC для скасування.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Видалити Роздiл   ESC = Скасувати   F3 = Вийти",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Встановлення ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Встановлювач оновлює конфiгурацiю системи. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Створюю структуру реєстру...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "ReactOS не був повнiстю встановлений на Ваш\n"
        "комп'ютер. Якщо ви вийдете з встановлювача зараз,\n"
        "то Вам буде необхiдно запустити програму встановлення\n"
        "знову, якщо Ви хочете встановити ReactOS,\n"
        "\n"
        "  \x07  Натиснiть ENTER щоб продовжити встановлення.\n"
        "  \x07  Натиснiть F3 для виходу з встановлювача.",
        "F3 = Вийти  ENTER = Продовжити"
    },
    {
        //ERROR_NO_HDD
        "Не вдалось знайти жорсткий диск.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Не вдалось знайти установочний диск.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Не вдалось завантажити файл TXTSETUP.SIF.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Файл TXTSETUP.SIF пошкоджений.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Виявлено некоректний пiдпис в TXTSETUP.SIF.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Не вдалось отримати данi про системний диск.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_WRITE_BOOT,
        "Не вдалось встановити завантажувальний код FAT на ситемний роздiл.",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Не вдалось завантажити список типiв комп'ютера.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Не вдалось завантажити список режимiв екрану.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Не вдалось завантажити список типiв клавiатури.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Не вдалось завантажити список розкладок клавiатури.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_WARN_PARTITION,
          "Знайдено як мiнiмум один жорсткий диск, що мiстить роздiл,\n"
          "який не пiдтримується ReactOS!\n"
          "\n"
          "Створення чи видалення роздiлiв може зруйнувати таблицю роздiлiв.\n"
          "\n"
          "  \x07  Натиснiть F3 для виходу з встановлювача.\n"
          "  \x07  Натиснiть ENTER щоб продовжити.",
          "F3 = Вийти  ENTER = Продовжити"
    },
    {
        //ERROR_NEW_PARTITION,
        "Ви не можете створити новий роздiл на\n"
        "вже iснуючому роздiлi!\n"
        "\n"
        "  * Натиснiть будь-яку клавiшу щоб продовжити.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Не можна видалити нерозмiчену область на диску!\n"
        "\n"
        "  * Натиснiть будь-яку клавiшу щоб продовжити.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Не вдалось встановити завантажувальний код FAT на ситемний роздiл.",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_NO_FLOPPY,
        "Вiдсутня дискета в дисководi A:.",
        "ENTER = Продовжити"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Не вдалось оновити параметри розкладки клавiатури.",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Не вдалось оновити параметри екрану в реєстрi.",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Не вдалось iмпортувати файл кущiв реєстру.",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_FIND_REGISTRY
        "Не вдалось знайти файли даних реєстру.",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_CREATE_HIVE,
        "Не вдалось створити кущi реєстру.",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Не вдалось iнiцiалiзувати реєстр.",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet має некоректний inf-файл.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet не знайдено.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet не має установочного сценарiю.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_COPY_QUEUE,
        "Не вдалось вiдкрити чергу копiювання файлiв.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_CREATE_DIR,
        "Не вдалось створити директорiї для встановлення.",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Не вдалось знайти секцiю 'Directories'\n"
        "в файлi TXTSETUP.SIF.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_CABINET_SECTION,
        "Не вдалось знайти секцiю 'Directories'\n"
        "в cabinet.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Не вдалось створити директорiю для встановлння.",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Не вдалось знайти секцiю 'SetupData'\n"
        "в файлi TXTSETUP.SIF.\n",
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Не вдалось записати таблицi роздiлiв.\n"
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Не вдалось додати параметри кодування в реєстр.\n"
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Не вдалось встановити локаль системи.\n"
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Не вдалось додати розкладки клавiатури до реєстру.\n"
        "ENTER = Перезавантажити комп'ютер"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Не вдалось встановити geo id.\n"
        "ENTER = Перезавантажити комп'ютер"
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
        "  * Натиснiть будь-яку клавiшу для продовження.",
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

MUI_PAGE ukUAPages[] =
{
    {
        LANGUAGE_PAGE,
        ukUALanguagePageEntries
    },
    {
        WELCOME_PAGE,
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
        UPGRADE_REPAIR_PAGE,
        ukUAUpgradePageEntries
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
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        ukUAConfirmDeleteSystemPartitionEntries
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
     "   Будь-ласка, зачекайте..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   P = Create Primary   E = Create Extended   F3 = Quit"},
//     "   ENTER = Встановити   C = Створити Роздiл   F3 = Вийти"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   L = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Встановити   D = Видалити Роздiл   F3 = Вийти"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "Розмiр нового роздiлу:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a primary partition on"},
//     "Ви хочете створити новий роздiл на"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDDSIZE,
    "Будь-ласка, введiть розмiр нового роздiлу в мегабайтах."},
    {STRING_CREATEPARTITION,
     "   ENTER = Створити Роздiл   ESC = Скасувати   F3 = Вийти"},
    {STRING_PARTFORMAT,
    "Цей роздiл буде вiдформатовано."},
    {STRING_NONFORMATTEDPART,
    "Ви вибрали встановлення ReactOS на новий або неформатований роздiл."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "ReactOS встановлюється на роздiл"},
    {STRING_CHECKINGPART,
    "Встановлювач перевiряє вибраний роздiл."},
    {STRING_CONTINUE,
    "ENTER = Продовжити"},
    {STRING_QUITCONTINUE,
    "F3 = Вийти  ENTER = Продовжити"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Перезавантажити комп'ютер"},
    {STRING_TXTSETUPFAILED,
    "Встановлювач не змiг знайти секцiю '%S' \nв файлi TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Копiювання: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Встановлювач копiює файли..."},
    {STRING_REGHIVEUPDATE,
    "   Оновлення кущiв реєстру..."},
    {STRING_IMPORTFILE,
    "   Iмпортування %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Оновлення параметрiв екрану в реєстрi..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Оновлення параметрiв локалi..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Оновлення параметрiв розкладки клавiатури..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Додавання даних про кодову сторiнку в реєстр..."},
    {STRING_DONE,
    "   Готово..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Перезавантажити комп'ютер"},
    {STRING_CONSOLEFAIL1,
    "Не вдалось вiдкрити консоль\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Найбiльш ймовiрна причина цього -  використання USB клавiатури\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB клавiатури ще не пiдтримуються повнiстю\r\n"},
    {STRING_FORMATTINGDISK,
    "Встановлювач форматує ваш диск"},
    {STRING_CHECKINGDISK,
    "Встановлювач перевiряє ваш диск"},
    {STRING_FORMATDISK1,
    " Форматувати роздiл в файловiй системi %S (швидке форматування) "},
    {STRING_FORMATDISK2,
    " Форматувати роздiл в файловiй системi %S  "},
    {STRING_KEEPFORMAT,
    " Залишити iснуючу файлову систему (без змiн) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  Жорсткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) on %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  Жорсткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "на %I64u %s  Жорсткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) on %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "на %I64u %s  Жорсткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "Жорсткий диск %lu (%I64u %s), Порт=%hu, Шина=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "на Жорсткому диску %lu (%I64u %s), Порт=%hu, Шина=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sType %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  Жорсткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) on %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  Жорсткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Встановлювач створив новий роздiл на"},
    {STRING_UNPSPACE,
    "    %sНерозмiчена область%s            %6lu %s"},
    {STRING_MAXSIZE,
    "MB (макс. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "Новий (Неформатований)"},
    {STRING_FORMATUNUSED,
    "Не використано"},
    {STRING_FORMATUNKNOWN,
    "Невiдомо"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Додавання розкладок клавiатури"},
    {0, 0}
};
