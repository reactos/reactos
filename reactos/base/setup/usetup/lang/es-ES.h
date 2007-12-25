#ifndef LANG_ES_ES_H__
#define LANG_ES_ES_H__

static MUI_ENTRY esESLanguagePageEntries[] =
{
    {
        6,
        8,
        "Selección de idioma",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Please choose the language used for the installation process.",
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
        "\x07  This Language will be the default language for the final system.",
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
        "El instalador no pudo instalar el cargador de arranque en el disco",
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
        "La lista inferior muestra las particiones existentes y el espacio libre",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "en el disco para nuevas particiones.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "\x07  Presione las teclas ARRIBA o ABAJO para seleccionar un elemento de la lista.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione ENTER para instalar ReactOS en la partición seleccionada.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Presione C para crear una nueva partición.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Presione D para borrar una partición existente.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Espere por favor ...",
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
        "Formato de la partición",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "El instalador formateará la partición. Presione ENTER para continuar.",
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
        TEXT_NORMAL
    }
};

static MUI_ENTRY esESInstallDirectoryEntries[] =
{
    {
        6,
        8,
        "El programa instalará los archivos de ReactOS en la partición seleccionada. Seleccione un",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "directorio donde quiere que sea instalado ReactOS:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "Para cambiar el directorio sugerido, presione RETROCESO para borrar",
        TEXT_NORMAL
    },
    {
        6,
        15,
        "caracteres y escriba el directorio donde desea que ReactOS",
        TEXT_NORMAL
    },
    {
        6,
        16,
        "sea instalado.",
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

static MUI_ENTRY esESFileCopyEntries[] =
{
    {
        11,
        12,
        "Por favor espere mientras el Instalador de ReactOS copia archivos en su ",
        TEXT_NORMAL
    },
    {
        30,
        13,
        "carpeta de instalación de ReactOS.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "Puede durar varios minutos.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Espere por favor...    ",
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
        "El programam está instalando el cargador de arranque",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Instalar cargador de arranque en el disco duro (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Instalar cargador de inicio en un disquete.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Omitir la instalación del cargador de arranque.",
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

static MUI_ENTRY esESKeyboardSettingsEntries[] =
{
    {
        6,
        8,
        "Desea cambiar el tipo de teclado instalado.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Presione las teclas ARRIBA o ABAJO para seleccionar el tipo de teclado.",
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
        "\x07  Presione la tecla ESC para volver a la página anterior sin cambiar",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   el tipo de teclado.",
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

static MUI_ENTRY esESLayoutSettingsEntries[] =
{
    {
        6,
        8,
        "Desea cambiar la disposición del teclado a instalar.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Presione las teclas ARRIBA o ABAJO para select the la disposición del teclado",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "    deseada. Luego presione ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione la tecla ESC para volver a la página anterior sin cambiar",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   la disposición del teclado.",
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
    },

};

static MUI_ENTRY esESPrepareCopyEntries[] =
{
    {
        6,
        8,
        "El programa prepara su computadora para copiar los archivos de ReactOS. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Creando la lista de archivos a copiar...",
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
        "Seleccione un sistema de archivos de la lista inferior.",
        0
    },
    {
        8,
        19,
        "\x07  Presione las teclas ARRIBA o ABAJO para seleccionar el sistema de archivos.",
        0
    },
    {
        8,
        21,
        "\x07  Presione ENTER para formatear partición.",
        0
    },
    {
        8,
        23,
        "\x07  Presione ESC para seleccionar otra partición.",
        0
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

static MUI_ENTRY esESDeletePartitionEntries[] =
{
    {
        6,
        8,
        "Ha elegido borrar la partición",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Presione D para borrar la partición.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "ADVERTENCIA: ¡Se perderán todos los datos de esta partición!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Presione ESC para cancelar.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Borrar Partición   ESC = Cancelar   F3 = Salir",
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
        esESLanguagePageEntries
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


