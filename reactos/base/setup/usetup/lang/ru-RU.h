#ifndef LANG_RU_RU_H__
#define LANG_RU_RU_H__

static MUI_ENTRY ruRULanguagePageEntries[] =
{
    {
        6,
        8,
        "Select your language:",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue  F3 = Quit",
        TEXT_STATUS
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
        6,
        8,
        "Добро пожаловать в установку ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "На этой стадии установки будут скопированы файлы операционной системы ReactOS",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "на ваш компьютер и подготовлена вторая стадия установки.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Нажмите ВВОД для установки ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Нажмите R для востановления ReactOS.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "\x07  Нажмите L для просмотра лицензионного соглашения ReactOS",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Нажмите F3 для выхода из установки ReactOS.",
        TEXT_NORMAL
    },
    {
        6, 
        23,
        "Для дополнительной информации о ReactOS посетите:",
        TEXT_NORMAL
    },
    {
        6, 
        24, 
        "http://www.reactos.org",
        TEXT_HIGHLIGHT
    },
    {
        0,
        0,
        "   ВВОД = Продолжение  R = Востановление F3 = Выход",
        TEXT_STATUS
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
        " Установка ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6, 
        8, 
        "ReactOS находится в ранней стадии разработки и не поддерживает все",
        TEXT_NORMAL
    },
    {
        6, 
        9,
        "функции для полной совместимости с утанавливаемыми приложениями.",
        TEXT_NORMAL
    },
    {
        6, 
        12,
        "Имеются следующие ограничения:",
        TEXT_NORMAL
    },
    {
        8, 
        13,
        "- Установка возможна только на первичный раздел диска",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- При установке нельзя удалить первичный раздел диска",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  пока имеется расширенный раздел.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- При установке нельзя удалить первый расширенный раздел с диска",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  пока существуют другие расширенные разделы.",
        TEXT_NORMAL
    },
    {
        8, 
        18, 
        "- При установке поддерживается только файловая система FAT.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "- Проверка файловой системы не осуществляется.",
        TEXT_NORMAL
    },
    {
        8, 
        23, 
        "\x07  Нажмите ввод для установки ReactOS.",
        TEXT_NORMAL
    },
    {
        8, 
        25, 
        "\x07  Нажмите F3 для выхода из установки ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0, 
        "   ВВОД = Продолжение   F3 = Выход",
        TEXT_STATUS
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
        6,
        6,
        "Лицензия:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS лицензирована в соответствии с Открытым лицензионным",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "соглашением GNU GPL и содержит компоненты распространяемые",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "с совместимыми лицензиями: X11, BSD и GNU LGPL.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "Все программное обеспечение входящее в систему ReactOS выпущено",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "под Открытым лицензионным соглашением GNU GPL с сохранением",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "первоначальной лицензии.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "Данное программное обеспечение поставляется БЕЗ ГАРАНТИИ и без ограничений",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "в использовании, как в местном, так и международном праве.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "Лицензия ReactOS разрешает передачу продукта третьим лицам.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "Если по каком-либо причинам вы не получили копию Открытого",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "лицензионного соглашения GNU вместе с ReactOS, посетите",
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
        "Гарантии:",
        TEXT_HIGHLIGHT
    },
    {           
        8,
        24,
        "Это свободное программное обеспечение; см. источник для просмотра прав.",
        TEXT_NORMAL
    },
    {           
        8,
        25,
        "НЕТ НИКАКИХ ГАРАНТИЙ; нет гарантии ТОВАРНОГО СОСТОЯНИЯ или",
        TEXT_NORMAL
    },
    {           
        8,
        26,
        "ПРИГОДНОСТИ ДЛЯ КОНКРЕТНЫХ ЦЕЛЕЙ",
        TEXT_NORMAL
    },
    {           
        0,
        0,
        "   ВВОД = Возврат",
        TEXT_STATUS
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
        6, 
        8,
        "В списке ниже приведены устройства и их параметры.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "       Компьютер:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        Экран:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       Клавиатура:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Раскладка клавиатуры:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "         Применить:",
        TEXT_NORMAL
    },
    {
        25, 
        16, "Применить данные параметры устройств",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Вы можете изменить параметры устройств нажимая клавиши ВВЕРХ и ВНИЗ",
        TEXT_NORMAL
    },
    {
        6, 
        20, 
        "для выделения элемента и клавишу ВВОД для выбора других вариантов",
        TEXT_NORMAL
    },
    {
        6, 
        21,
        "параметров.",
        TEXT_NORMAL
    },
    {
        6, 
        23, 
        "Когда все параметры определены, выберите \"Применить данные параметры устройств\"",
        TEXT_NORMAL
    },
    {
        6, 
        24,
        "и нажмите ВВОД.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ВВОД = Продолжение   F3 = Выход",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRURepairPageEntries[] =
{
    {
        6, 
        8,
        "ReactOS находится в ранней стадии разработки и не поддерживает все",
        TEXT_NORMAL
    },
    {
        6, 
        9,
        "функции для полной совместимости с утанавливаемыми приложениями.",
        TEXT_NORMAL
    },
    {
        6, 
        12,
        "Функция востановления в данным момент отсутствует.",
        TEXT_NORMAL
    },
    {
        8, 
        15,
        "\x07  Нажмите U для обновления ОС.",
        TEXT_NORMAL
    },
    {
        8, 
        17,
        "\x07  Нажмите R для запуска консоли востановления.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "\x07  Нажмите ESC для возврата на главную страницу.",
        TEXT_NORMAL
    },
    {
        8, 
        21,
        "\x07  Нажмите ВВОД для перезагрузки компьютера.",
        TEXT_NORMAL
    },
    {
        0, 
        0,
        "   ESC = Главная страница  ВВОД = Перезагрузка",
        TEXT_STATUS
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
        6,
        8,
        "Вы хотите изменить устанавливаемый тип компьютера.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Нажмите клавишу ВВЕРХ или ВНИЗ для выбора предпочтительного типа компьютера.",
        TEXT_NORMAL
    },    
    {
        8,
        11,
        "   Затем нажмите ВВОД.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Нажмите клавишу ESC для возврата к предыдущей странице без изменения",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   типа компьютера.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ВВОД = Продолжение   ESC = Отмена   F3 = Выход",
        TEXT_STATUS
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
        10,
        6,
        "Система проверяет все ли данные записаны на диск",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Это может занять минуту",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "После завершения компьютер будет автоматически перезагружен",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Очистка кеша",
        TEXT_STATUS
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
        10,
        6,
        "ReactOS установлен не полностью",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Извлеките гибкий диск из дисковода A: и",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "все CD-ROM из CD-дисководов.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Нажмите ВВОД для перезагрузки компьютера.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Пожалуйста подождите ...",
        TEXT_STATUS,
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
        6,
        8,
        "Вы хотите изменить устанавливаемый тип экрана.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Нажмите клавиши ВВОРХ или ВНИЗ для выбора типа экрана.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Затем нажмите ВВОД.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Нажмите клавишу ESC для возврата к предыдущей странице без изменения",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   типа экрана.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ВВОД = Продолжение   ESC = Отмена   F3 = Выход",
        TEXT_STATUS
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
        10,
        6,
        "Основные компоненты ReactOS были успешно установлены.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Извлеките гибкий диск из дисковода A: и",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "все CD-ROM из CD-дисководов.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Нажмите ВВОД для перезагрузки компьютера.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ВВОД = Перезагрузить компьютер",
        TEXT_STATUS
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
        6,
        8,
        "Программа установки не смогла установить загрузчик на",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "жесткий диск вашего компьютера.",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Пожалуйста вставте отформатированный гибкий диск в дисковод A: и",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "нажмите ВВОД.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ВВОД = Продолжение   F3 = Выход",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

MUI_PAGE ruRUPages[] =
{
    {
        LANGUAGE_PAGE,
        ruRULanguagePageEntries
    },
    {
       START_PAGE,
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
        REPAIR_INTRO_PAGE,
        ruRURepairPageEntries
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
        -1,
        NULL
    }
};

#endif
