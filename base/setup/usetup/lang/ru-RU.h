#pragma once

static MUI_ENTRY ruRULanguagePageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Выбор языка",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Пожалуйста, выберите язык, используемый при установке.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Затем нажмите ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Этот язык будет использоваться по умолчанию в установленной системе.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить  F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUWelcomePageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Добро пожаловать в программу установки ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "На этой стадии будут скопированы файлы операционной системы ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "на Ваш компьютер и подготовлена вторая стадия установки.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Нажмите ENTER для установки ReactOS.",
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
        "\x07  Нажмите L для просмотра лицензионного соглашения ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Нажмите F3 для выхода из программы установки ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Для дополнительной информации о ReactOS посетите:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.ru",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "ENTER = Продолжить  R = Восстановление  L = Лиц. соглашение  F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUIntroPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS находится в ранней стадии разработки и не поддерживает все",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "функции для полной совместимости с устанавливаемыми приложениями.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Имеются следующие ограничения:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- При установке поддерживается только файловая система FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Проверка файловой системы не осуществляется.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Нажмите ENTER для установки ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Нажмите F3 для выхода из установки ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRULicensePageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Лицензия:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS лицензирована в соответствии с Открытым лицензионным",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "соглашением GNU GPL и содержит компоненты, распространяемые",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "с совместимыми лицензиями: X11, BSD и GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Все программное обеспечение входящее в систему ReactOS выпущено",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "под Открытым лицензионным соглашением GNU GPL с сохранением",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "первоначальной лицензии.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Данное программное обеспечение поставляется БЕЗ ГАРАНТИИ и без",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "ограничений в использовании, как в местном, так и международном праве.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "Лицензия ReactOS разрешает передачу продукта третьим лицам.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Если по каком-либо причинам вы не получили копию Открытого",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "лицензионного соглашения GNU вместе с ReactOS, посетите",
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
        "Гарантии:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Это свободное программное обеспечение; см. источник для просмотра прав.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "НЕТ НИКАКИХ ГАРАНТИЙ; нет гарантии ТОВАРНОГО СОСТОЯНИЯ или",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "ПРИГОДНОСТИ ДЛЯ КОНКРЕТНЫХ ЦЕЛЕЙ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Возврат",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUDevicePageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "В списке ниже приведены устройства и их параметры.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Компьютер:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Экран:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Клавиатура:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Раскладка:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Применить:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Применить данные параметры устройств",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Вы можете изменить параметры устройств, нажимая клавиши ВВЕРХ и ВНИЗ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "для выделения элемента, и клавишу ENTER для выбора других вариантов",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "параметров.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Когда все параметры определены, выберите \"Применить данные параметры",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "устройств\" и нажмите ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUUpgradePageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
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

static MUI_ENTRY ruRUComputerPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Вы хотите изменить устанавливаемый тип компьютера.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Нажмите клавишу ВВЕРХ или ВНИЗ для выбора предпочтительного типа",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   компьютера. Затем нажмите ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Нажмите клавишу ESC для возврата к предыдущей странице без изменения",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   типа компьютера.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить   ESC = Отмена   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUFlushPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Система проверяет, все ли данные записаны на диск",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Это может занять некоторое время.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "После завершения компьютер будет автоматически перезагружен.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Очистка кеша",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUQuitPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS установлен не полностью",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Извлеките гибкий диск из дисковода A: и",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "все CD-ROM из CD-дисководов.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Нажмите ENTER для перезагрузки компьютера.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Пожалуйста, подождите ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUDisplayPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Вы хотите изменить устанавливаемый тип экрана.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Нажмите клавиши ВВЕРХ или ВНИЗ для выбора типа экрана.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Затем нажмите ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Нажмите клавишу ESC для возврата к предыдущей странице без изменения",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   типа экрана.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить   ESC = Отмена   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUSuccessPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Основные компоненты ReactOS были успешно установлены.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Извлеките гибкий диск из дисковода A: и",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "все CD-ROM из CD-дисководов.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Нажмите ENTER для перезагрузки компьютера.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Перезагрузка",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUBootPageEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Программа установки не смогла установить загрузчик на",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "жесткий диск вашего компьютера.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Пожалуйста вставьте отформатированный гибкий диск в дисковод A: и",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "нажмите ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = Продолжить   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY ruRUSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "В списке ниже показаны существующие разделы и неиспользуемое",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "пространство для нового раздела.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Нажмите ВВЕРХ или ВНИЗ для выбора элемента.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Нажмите ENTER для установки ReactOS на выделенный раздел.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Нажмите P для создания первичного раздела.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Нажмите E для создания расширенного раздела.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Нажмите L для создания логического раздела.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Нажмите D для удаления существующего раздела.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Пожалуйста, подождите...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Вы хотите удалить системный раздел.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Системные разделы могут содержать диагностические программы, программы",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "настройки аппаратных средств, программы запуска ОС (подобные ReactOS)",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "или другое ПО, предоставленное изготовителем оборудования.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Удаляйте системный раздел, когда уверены, что на нем нет важных программ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "или когда вы уверены, что они не нужны.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "Когда вы удалите системный раздел, вы не сможете загрузить",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "компьютер с жесткого диска, пока не закончите установку ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Нажмите ENTER чтобы удалить системный раздел. Вы должны будете",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   подтвердить удаление позже снова.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  Нажмите ESC чтобы вернуться к предыдущей странице. Раздел не",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "   будет удален.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Продолжить  ESC=Отмена",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Форматирование раздела",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Для установки раздел будет отформатирован. Нажмите ENTER для продолжения.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY ruRUInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Установка файлов ReactOS на выбранный раздел. Выберите директорию,",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "в которую будет установлена система:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Чтобы изменить выбранную директорию, нажмите BACKSPACE для удаления",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "символов, а за тем наберите новое имя директории для установки ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        " ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUFileCopyEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Пожалуйста, подождите, пока программа установки скопирует файлы",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "ReactOS в установочную директорию.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Это может занять несколько минут.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 Пожалуйста, подождите...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUBootLoaderEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Установка загрузчика ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Установить загрузчик на жесткий диск (MBR и VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Установить загрузчик на жесткий диск (только VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Установить загрузчик на гибкий диск.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Не устанавливать загрузчик.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Изменение типа клавиатуры.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Нажмите ВВЕРХ или ВНИЗ для выбора нужного типа клавиатуры.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Затем нажмите ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Нажмите клавишу ESC для возврата к предыдущей странице без изменения",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   типа клавиатуры.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить   ESC = Отмена   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRULayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Пожалуйста выберите раскладку, которая будет установлена по умолчанию.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Нажмите ВВЕРХ или ВНИЗ для выбора нужной раскладки клавиатуры.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Затем нажмите ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Нажмите клавишу ESC для возврата к предыдущей странице без",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   изменения раскладки клавиатуры.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить   ESC = Отмена   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ruRUPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Подготовка вашего компьютера к копированию файлов ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Подготовка списка копируемых файлов...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ruRUSelectFSEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Выберите файловую систему из списка ниже.",
        0
    },
    {
        8,
        19,
        "\x07  Нажмите ВВЕРХ или ВНИЗ для выбора файловой системы.",
        0
    },
    {
        8,
        21,
        "\x07  Нажмите ENTER для форматирования раздела.",
        0
    },
    {
        8,
        23,
        "\x07  Нажмите ESC для выбора другого раздела.",
        0
    },
    {
        0,
        0,
        "ENTER = Продолжить   ESC = Отмена   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Вы выбрали удаление раздела.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Нажмите D для удаления раздела.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ВНИМАНИЕ: Все данные с этого раздела будут потеряны!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Нажмите ESC для отмены.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Удалить раздел   ESC = Отмена   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRURegistryEntries[] =
{
    {
        4,
        3,
        " Установка ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Программа установки обновляет конфигурацию системы. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Создание кустов системного реестра...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR ruRUErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Успешно\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "ReactOS не был полностью установлен на ваш\n"
        "компьютер. Если вы выйдите из установки сейчас,\n"
        "то вам нужно запустить программу установки снова,\n"
        "если вы хотите установить ReactOS\n"
        "  \x07  Нажмите ENTER для продолжения установки.\n"
        "  \x07  Нажмите F3 выхода из установки.",
        "F3 = Выход  ENTER = Продолжить"
    },
    {
        //ERROR_NO_HDD
        "Не удалось найти жесткий диск.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Не удалось найти установочный диск.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Не удалось загрузить файл TXTSETUP.SIF.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Файл TXTSETUP.SIF поврежден.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Обнаружена некорректная подпись в TXTSETUP.SIF.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Не удалось получить информацию о системном диске.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_WRITE_BOOT,
        "Не удалось установить загрузчик FAT на системный раздел.",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Не удалось загрузить список типов компьютера.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Не удалось загрузить список режимов экрана.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Не удалось загрузить список типов клавиатуры.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Не удалось загрузить список раскладок клавиатуры.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_WARN_PARTITION,
        "Найден по крайней мере один жесткий диск, который содержит раздел\n"
        "неподдерживаемый ReactOS!\n"
        "\n"
        "Создание или удаление разделов может нарушить таблицу разделов.\n"
        "\n"
        "  \x07  Нажмите F3 для выхода из установки.\n"
        "  \x07  Нажмите ENTER для продолжения.",
        "F3 = Выход  ENTER = Продолжить"
    },
    {
        //ERROR_NEW_PARTITION,
        "Вы не можете создать новый раздел диска в\n"
        "уже существующем разделе!\n"
        "\n"
        "  * Нажмите любую клавишу для продолжения.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Вы не можете удалить неразделенное дисковое пространство!\n"
        "\n"
        "  * Нажмите любую клавишу для продолжения.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Не удалось установить загрузчик FAT на системный раздел.",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_NO_FLOPPY,
        "Нет диска в дисководе A:.",
        "ENTER = Продолжить"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Не удалось обновить параметры раскладки клавиатуры.",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Не удалось обновить параметры экрана в реестре.",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Не удалось импортировать файлы кустов реестра.",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_FIND_REGISTRY
        "Не удалось найти файлы системного реестра.",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_CREATE_HIVE,
        "Не удалось создать кусты системного реестра.",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Не удалось инициализировать системный реестр.",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet не получил корректный inf-файл.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet не найден.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet не смог найти установочный скрипт.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_COPY_QUEUE,
        "Не удалось открыть очередь копирования файлов.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_CREATE_DIR,
        "Не удалось создать установочные директории.",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Не удалось найти секцию 'Directories'\n"
        "в файле TXTSETUP.SIF.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_CABINET_SECTION,
        "Не удалось найти секцию 'Directories'\n"
        "в cabinet.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Не удалось создать директорию для установки.",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Не удалось найти секцию 'SetupData'\n"
        "в файле TXTSETUP.SIF.\n",
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Не удалось записать таблицу разделов.\n"
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Не удалось добавить параметры кодировки в реестр.\n"
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Не удалось установить язык системы.\n"
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Не удалось добавить раскладку клавиатуры в реестр.\n"
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Не удалось установить geo id.\n"
        "ENTER = Перезагрузка"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Неверное название директории.\n"
        "\n"
        "  * Нажмите любую клавишу для продолжения."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Выбранный раздел слишком мал для установки ReactOS.\n"
        "Установочный раздел должен иметь по крайней мере %lu MB пространства.\n"
        "\n"
        "  * Нажмите любую клавишу для продолжения.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "Вы не можете создать первичный или расширенный раздел в таблице\n"
        "разделов диска, потому что она заполнена.\n"
        "\n"
        "  * Нажмите любую клавишу для продолжения."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "Вы не можете создать больше одного расширенного раздела на диск.\n"
        "\n"
        "  * Нажмите любую клавишу для продолжения."
    },
    {
        //ERROR_FORMATTING_PARTITION,
        "Не удалось форматировать раздел:\n"
        " %S\n"
        "\n"
        "ENTER = Перезагрузка"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE ruRUPages[] =
{
    {
        LANGUAGE_PAGE,
        ruRULanguagePageEntries
    },
    {
        WELCOME_PAGE,
        ruRUWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        ruRUIntroPageEntries
    },
    {
        LICENSE_PAGE,
        ruRULicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        ruRUDevicePageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        ruRUUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        ruRUComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        ruRUDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        ruRUFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        ruRUSelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        ruRUConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        ruRUSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        ruRUFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        ruRUDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        ruRUInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        ruRUPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        ruRUFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        ruRUKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        ruRUBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        ruRULayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        ruRUQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        ruRUSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        ruRUBootPageEntries
    },
    {
        REGISTRY_PAGE,
        ruRURegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING ruRUStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Пожалуйста, подождите..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Установить   P = Первичный раздел   E = Расширенный   F3 = Выход"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Установить   L = Создать логический раздел   F3 = Выход"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Установить   D = Удалить раздел   F3 = Выход"},
    {STRING_DELETEPARTITION,
     "   D = Удалить раздел   F3 = Выход"},
    {STRING_PARTITIONSIZE,
     "Размер нового раздела:"},
    {STRING_CHOOSENEWPARTITION,
     "Вы хотите создать первичный раздел на"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Вы хотите создать расширенный раздел на"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Вы хотите создать логический раздел на"},
    {STRING_HDDSIZE,
    "Пожалуйста, введите размер нового раздела в мегабайтах."},
    {STRING_CREATEPARTITION,
     "   ENTER = Создать раздел   ESC = Отмена   F3 = Выход"},
    {STRING_PARTFORMAT,
    "Этот раздел будет отформатирован далее."},
    {STRING_NONFORMATTEDPART,
    "Вы выбрали установку ReactOS на новый неотформатированный раздел."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Системный раздел не отформатирован."},
    {STRING_NONFORMATTEDOTHERPART,
    "Новый раздел не отформатирован."},
    {STRING_INSTALLONPART,
    "ReactOS устанавливается на раздел:"},
    {STRING_CHECKINGPART,
    "Программа установки проверяет выбранный раздел."},
    {STRING_CONTINUE,
    "ENTER = Продолжить"},
    {STRING_QUITCONTINUE,
    "F3 = Выход  ENTER = Продолжить"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Перезагрузка"},
    {STRING_TXTSETUPFAILED,
    "Программа установки не смогла найти секцию '%S'\nв файле TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Копирование файла: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Программа установки копирует файлы..."},
    {STRING_REGHIVEUPDATE,
    "   Обновление кустов реестра..."},
    {STRING_IMPORTFILE,
    "   Импортирование %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Обновление параметров экрана в реестре..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Обновление параметров языка..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Обновление параметров раскладки клавиатуры..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Добавление информации о кодовой странице в реестр..."},
    {STRING_DONE,
    "   Завершено..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Перезагрузка"},
    {STRING_CONSOLEFAIL1,
    "Не удалось открыть консоль\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Наиболее вероятная причина этого - использование USB-клавиатуры\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB клавиатуры сейчас поддерживаются не полностью\r\n"},
    {STRING_FORMATTINGDISK,
    "Программа установки форматирует ваш диск"},
    {STRING_CHECKINGDISK,
    "Программа установки проверяет ваш диск"},
    {STRING_FORMATDISK1,
    " Форматирование раздела в файловой системе %S (быстрое) "},
    {STRING_FORMATDISK2,
    " Форматирование раздела в файловой системе %S "},
    {STRING_KEEPFORMAT,
    " Оставить существующую файловую систему (без изменений) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  Жесткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) на %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  Жесткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Запись 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "на %I64u %s  Жесткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) на %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "на %I64u %s  Жесткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "Жесткий диск %lu (%I64u %s), Порт=%hu, Шина=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  Запись 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "на жестком диске %lu (%I64u %s), Порт=%hu, Шина=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sЗапись %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  Жесткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) на %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  Жесткий диск %lu  (Порт=%hu, Шина=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Программа установки создала новый раздел на:"},
    {STRING_UNPSPACE,
    "    %sНеразмеченное пространство%s            %6lu %s"},
    {STRING_MAXSIZE,
    "МБ (макс. %lu МБ)"},
    {STRING_EXTENDED_PARTITION,
    "Расширенный раздел"},
    {STRING_UNFORMATTED,
    "Новый (неотформатированный)"},
    {STRING_FORMATUNUSED,
    "Не использовано"},
    {STRING_FORMATUNKNOWN,
    "Неизвестный"},
    {STRING_KB,
    "КБ"},
    {STRING_MB,
    "МБ"},
    {STRING_GB,
    "ГБ"},
    {STRING_ADDKBLAYOUTS,
    "Добавление раскладки клавиатуры"},
    {0, 0}
};
