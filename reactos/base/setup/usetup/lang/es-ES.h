#ifndef LANG_ES_ES_H__
#define LANG_ES_ES_H__

static MUI_ENTRY esESWelcomePageEntries[] =
{
    {
        6,
        8,
        "Bienvenido a la instalación de ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Esta parte de la instalación copia ReactOS en su equipo y",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "prepara la segunda parte de la instalación.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Presione ENTER para instalar ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Presione R para reparar ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Presione L para ver las condiciones y términos de licencia",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Presione F3 para salir sin instalar ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Para más información sobre ReactOS, visite por favor:",
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
        "   ENTER = Continuar  R = Reparar F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESIntroPageEntries[] =
{
    {
        4,
        3,
        " Instalación de ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "El instalador de ReactOS se encuentra en una etapa preliminar.",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Aún no posee todas las funciones de un instalador.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Se presentan las siguientes limitaciones:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- El instalador no soporta más de una partición primaria por disco.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- El instalador no puede eliminar una partición primaria de un disco",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  si hay particiones extendidas en el mismo.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- El instalador no puede eliminar la primer partición extendida",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  si existen otras particiones extendidas en el disco.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- El instalador soporta solamente el sistema de archivos FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- El comprobador de integridad del sistema de archivos no está aún implementado.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  Presione ENTER para instalar ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Presione F3 para salir sin instalar ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESLicensePageEntries[] =
{
    {
        6,
        6,
        "Licencia:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS obedece los terminos de la licencia",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "GNU GPL con partes que contienen código de otras",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "licencias compatibles como la X11 o BSD y la GNU LGPL.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "Todo el software que forma parte del sistema ReactOS está",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "por tanto liberado bajo licencia GNU GPL así como manteniendo",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "la licencia original.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "Este software viene SIN GARANTIA o restricciones de uso",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "excepto leyes locales o internacionales aplicables. La licencia",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "de ReactOS cubre solo la distribución a terceras partes.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "Si por algún motivo no recibió una copia de la",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License con ReactOS por favor visite",
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
        "Garantía:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        24,
        "Este es software libre; vea el código para las condiciones de copia.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "NO existe garantía; ni siquiera para MERCANTIBILIDAD",
        TEXT_NORMAL
    },
    {
        8,
        26,
        "o el cumplimiento de algún propósito particular",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Regresar",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESDevicePageEntries[] =
{
    {
        6,
        8,
        "La lista inferior muestra la configuración del dispositivo actual.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "        Equipo:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "       Pantalla:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "        Teclado:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        " Disp. Teclado:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "        Aceptar:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Aceptar la configuración de los dispositivos",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Puede modificar la configuración con las teclas ARRIBA y ABAJO",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "para elegir. Luego presione ENTER para cambiar a una configuración",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "alternativa.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Cuando la configuración sea correcta, elija \"Aceptar la configuración",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "de los dispostivos\" y presione ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESRepairPageEntries[] =
{
    {
        6,
        8,
        "El instalador de ReactOS se encuentra en una etapa preliminar.",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Aún no posee todas las funciones de un instalador.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Las funciones de reparación no han sido aún implementadas.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Presione U para actualizar el SO.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Presione R para la consola de recuperación.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Presione ESC para volver al menú principal.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Presione ENTER para reiniciar su computadora.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "ESC = Menú inicial ENTER = Reiniciar",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY esESComputerPageEntries[] =
{
    {
        6,
        8,
        "Desea modificar el tipo de equipo a instalar.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Presione las teclas ARRIBA y ABAJO para elegir el tipo.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "Luego presione ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione ESC para volver a la página anterior sin cambiar",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "el tipo de equipo.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESFlushPageEntries[] =
{
    {
        10,
        6,
        "El sistema se está asegurando que todos los datos sean salvados",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Esto puede tardar un minuto",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "Cuando haya terminado, su equipo se reiniciará automáticamente",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "Vaciando el cache",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESQuitPageEntries[] =
{
    {
        10,
        6,
        "ReactOS no ha sido instalado completamente",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Quite el diskette de la unidad A: y",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "todos los CD-ROMs de la unidades.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Presione ENTER para reiniciar su equipo.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Espere por favor ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESDisplayPageEntries[] =
{
    {
        6,
        8,
        "Desea modificar el tipo de pantalla a instalar.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Presione las teclas ARRIBA y ABAJO para modificar el tipo.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Luego presione ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione la tecla ESC para volver a la página anterior sin",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   modificar el tipo de pantalla.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESSuccessPageEntries[] =
{
    {
        10,
        6,
        "Los componentes básicos de ReactOS han sido instalados correctamente.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Retire el disco de la unidad A: y",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "todos los CD-ROMs de las unidades.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Presione ENTER para reiniciar su equipo.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Reiniciar su equipo",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESBootPageEntries[] =
{
    {
        6,
        8,
        "El instalador no pudo instalar el bootloader en el disco",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "de su equipo",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Por favor inserte un disco formateado en la unidad A: y",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "presione ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY esESSelectPartitionEntries[] =
{
    {
        6,
        8,
        "The list below shows existing partitions and unused disk",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "space for new partitions.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "\x07  Press UP or DOWN to select a list entry.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press ENTER to install ReactOS onto the selected partition.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Press C to create a new partition.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Press D to delete an existing partition.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Please wait...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESFormatPartitionEntries[] =
{
    {
        6,
        8,
        "Format partition",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "Setup will now format the partition. Press ENTER to continue.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_NORMAL
    }
};

static MUI_ENTRY esESInstallDirectoryEntries[] =
{
    {
        6,
        8,
        "Setup installs ReactOS files onto the selected partition. Choose a",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "directory where you want ReactOS to be installed:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "To change the suggested directory, press BACKSPACE to delete",
        TEXT_NORMAL
    },
    {
        6,
        15,
        "characters and then type the directory where you want ReactOS to",
        TEXT_NORMAL
    },
    {
        6,
        16,
        "be installed.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESFileCopyEntries[] =
{
    {
        11,
        12,
        "Please wait while ReactOS Setup copies files to your ReactOS",
        TEXT_NORMAL
    },
    {
        30,
        13,
        "installation folder.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "This may take several minutes to complete.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Please wait...    ",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESBootLoaderEntries[] =
{
    {
        6,
        8,
        "Setup is installing the boot loader",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Install bootloader on the harddisk (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Install bootloader on a floppy disk.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Skip install bootloader.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESKeyboardSettingsEntries[] =
{
    {
        6,
        8,
        "You want to change the type of keyboard to be installed.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard type.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   the keyboard type.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESLayoutSettingsEntries[] =
{
    {
        6,
        8,
        "You want to change the keyboard layout to be installed.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "    layout. Then press ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   the keyboard layout.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY esESPrepareCopyEntries[] =
{
    {
        6,
        8,
        "Setup prepares your computer for copying the ReactOS files. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Building the file copy list...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY esESSelectFSEntries[] =
{
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
        TEXT_STATUS
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESDeletePartitionEntries[] =
{
    {
        6,
        8,
        "You have chosen to delete the partition",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Press D to delete the partition.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "WARNING: All data on this partition will be lost!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Press ESC to cancel.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Delete Partition   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

MUI_PAGE esESPages[] =
{
    {
        LANGUAGE_PAGE,
        LanguagePageEntries
    },
    {
       START_PAGE,
       esESWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        esESIntroPageEntries
    },
    {
        LICENSE_PAGE,
        esESLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        esESDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        esESRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        esESComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        esESDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        esESFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        esESSelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        esESSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        esESFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        esESDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        esESInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        esESPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        esESFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        esESKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        esESBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        esESLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        esESQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        esESSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        esESBootPageEntries
    },
    {
        -1,
        NULL
    }
};

#endif

