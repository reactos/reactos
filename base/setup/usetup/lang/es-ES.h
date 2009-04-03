#ifndef LANG_ES_ES_H__
#define LANG_ES_ES_H__

MUI_LAYOUTS esESLayouts[] =
{
    { L"040A", L"0000040A" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY esESLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Selecci¢n de idioma",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Por favor, seleccione el idioma a utilizar durante la instalaci¢n.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Luego presione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  El idioma seleccionado ser  tambi‚n el idioma por defecto del sistema.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Bienvenido a la instalaci¢n de ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Esta parte de la instalaci¢n copia ReactOS en su equipo y",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "prepara la segunda parte de la instalaci¢n.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Presione ENTER para instalar ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Presione R para reparar ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Presione L para ver las condiciones y t‚rminos de licencia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Presione F3 para salir sin instalar ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Para m s informaci¢n sobre ReactOS, visite por favor:",
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
        "   ENTER = Continuar  R = Reparar  L = Licencia  F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "El instalador de ReactOS se encuentra en una etapa preliminar.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "A£n no posee todas las funciones de un instalador.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Se presentan las siguientes limitaciones:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- El instalador no soporta m s de una partici¢n primaria por disco.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- El instalador no puede eliminar una partici¢n primaria de un disco",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  si hay particiones extendidas en el mismo.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- El instalador no puede eliminar la primer partici¢n extendida",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  si existen otras particiones extendidas en el disco.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- El instalador soporta solamente el sistema de archivos FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- El comprobador de integridad del sistema de archivos no est  a£n implementado.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Presione ENTER para instalar ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Presione F3 para salir sin instalar ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licencia:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS obedece los terminos de la licencia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL con partes que contienen c¢digo de otras",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licencias compatibles como la X11 o BSD y la GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Todo el software que forma parte del sistema ReactOS est ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "por tanto liberado bajo licencia GNU GPL as¡ como manteniendo",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "la licencia original.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Este software viene SIN GARANTIA o restricciones de uso",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "excepto leyes locales o internacionales aplicables. La licencia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "de ReactOS cubre solo la distribuci¢n a terceras partes.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Si por alg£n motivo no recibi¢ una copia de la",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License con ReactOS por favor visite",
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
        "Garant¡a:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Este es software libre; vea el c¢digo para las condiciones de copia.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "No existe garant¡a; ni siquiera para MERCANTIBILIDAD",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "o el cumplimiento de alg£n prop¢sito particular",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Regresar",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "La lista inferior muestra la configuraci¢n del dispositivo actual.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Equipo:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Pantalla:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Teclado:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Disp. Teclado:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Aceptar:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Aceptar la configuraci¢n de los dispositivos",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Puede modificar la configuraci¢n con las teclas ARRIBA y ABAJO",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "para elegir. Luego presione ENTER para cambiar a una configuraci¢n",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "alternativa.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Cuando la configuraci¢n sea correcta, elija \"Aceptar la configuraci¢n",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "de los dispostivos\" y presione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "El instalador de ReactOS se encuentra en una etapa preliminar.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "A£n no posee todas las funciones de un instalador.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Las funciones de reparaci¢n no han sido a£n implementadas.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Presione U para actualizar el SO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Presione R para la consola de recuperaci¢n.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Presione ESC para volver al men£ principal.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Presione ENTER para reiniciar su equipo.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC = Men£ inicial  U = Actualizar  R = Recuperar  ENTER = Reiniciar",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Desea modificar el tipo de equipo a instalar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Presione ARRIBA y ABAJO para elegir el tipo.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Luego presione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione ESC para volver a la p gina anterior sin cambiar",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "el tipo de equipo.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "El sistema se est  asegurando que todos los datos sean salvados",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Esta operaci¢n puede durar varios minutos",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Cuando haya terminado, su equipo se reiniciar  autom ticamente",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Vaciando la cache",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS no ha sido instalado completamente",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Retire el disco de la unidad A: y",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "todos los CD-ROMs de la unidades.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Presione ENTER para reiniciar su equipo.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Por favor, espere...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Desea modificar el tipo de pantalla a instalar.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Presione ARRIBA y ABAJO para modificar el tipo.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Luego presione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione la tecla ESC para volver a la p gina anterior sin",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   modificar el tipo de pantalla.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Los componentes b sicos de ReactOS han sido instalados correctamente.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Retire el disco de la unidad A: y",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "todos los CD-ROMs de las unidades.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Presione ENTER para reiniciar su equipo.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Reiniciar su equipo",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "El instalador no pudo instalar el cargador de arranque en el disco",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "de su equipo",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Por favor inserte un disco formateado en la unidad A: y",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "presione ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "La lista inferior muestra las particiones existentes y el espacio libre",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "en el disco para nuevas particiones.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Presione ARRIBA o ABAJO para seleccionar un elemento de la lista.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione ENTER para instalar ReactOS en la partici¢n seleccionada.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Presione C para crear una nueva partici¢n.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Presione D para borrar una partici¢n existente.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Por favor, espere...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Formato de la partici¢n",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "El instalador formatear  la partici¢n. Presione ENTER para continuar.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY esESInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "El programa instalar  los archivos en la partici¢n seleccionada. ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Seleccione un directorio donde quiere que sea instalado ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Para cambiar el directorio sugerido, presione RETROCESO para borrar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "caracteres y escriba el directorio donde desea que ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "sea instalado.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Por favor espere mientras el Instalador de ReactOS copia",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "archivos en su carpeta de instalaci¢n de ReactOS.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Esta operaci¢n puede durar varios minutos.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 Por favor, espere...",     
        TEXT_TYPE_STATUS
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "El programa est  instalando el cargador de arranque",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Instalar cargador de arranque en el disco duro (MBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Instalar cargador de inicio en un disquete.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Omitir la instalaci¢n del cargador de arranque.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_TYPE_STATUS  | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Desea cambiar el tipo de teclado instalado.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Presione ARRIBA o ABAJO para seleccionar el tipo de teclado.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Luego presione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione la tecla ESC para volver a la p gina anterior sin cambiar",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   el tipo de teclado.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
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
        "\x07  Presione ARRIBA o ABAJO para seleccionar la disposici¢n del teclado",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    deseada. Luego presione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione la tecla ESC para volver a la p gina anterior sin cambiar",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   la disposici¢n del teclado.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "El programa prepara su equipo para copiar los archivos de ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Creando la lista de archivos a copiar...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Seleccione un sistema de archivos de la lista inferior.",
        0
    },
    {
        8,
        19,
        "\x07  Presione ARRIBA o ABAJO para seleccionar el sistema de archivos.",
        0
    },
    {
        8,
        21,
        "\x07  Presione ENTER para formatear partici¢n.",
        0
    },
    {
        8,
        23,
        "\x07  Presione ESC para seleccionar otra partici¢n.",
        0
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ha elegido borrar la partici¢n",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Presione D para borrar la partici¢n.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ADVERTENCIA: ­Se perder n todos los datos de esta partici¢n!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Presione ESC para cancelar.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   D = Borrar Partici¢n   ESC = Cancelar   F3 = Salir",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESRegistryEntries[] =
{
    {
        4,
        3,
        " Instalaci¢n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "El instalador est  actualizando la configuraci¢n del sistema. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Creando la estructura del registro...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR esESErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS no est  completamente instalado en su\n"
        "equipo. Si cierra ahora el Instalador, necesitar \n"
        "ejecutarlo otra vez para instalar ReactOS.\n"
        "\n"
        "  \x07  Presione ENTER para continuar con el instalador.\n"
        "  \x07  Presione F3 para abandonar el instalador.",
        "F3 = Salir  ENTER = Continuar"
    },
    {
        //ERROR_NO_HDD
        "El instalador no pudo encontrar un disco duro.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "El instalador no pudo encontrar su unidad fuente.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "El instalador fall¢ al cargar el archivo TXTSETUP.SIF.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "El instalador encontr¢ un archivo TXTSETUP.SIF corrupto.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "El instalador encontr¢ una firma incorrecta en TXTSETUP.SIF.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "El instalador no pudo recibir informaci¢n del disco del sistema.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_WRITE_BOOT,
        "El instalador fall¢ al instalar el c¢digo de inicio FAT en la partici¢n del sistema.",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "El instalador fall¢ al cargar la lista de tipos de equipos.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "El instalador fall¢ al cargar la lista de resoluciones de pantalla.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "El instalador fall¢ al cargar la lista de teclados.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "El instalador fall¢ al cargar la lista de disposiciones teclados.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_WARN_PARTITION,
        "­El instalador encontr¢ que al menos un disco duro contiene una tabla\n"
        "de partici¢n incompatible que no puede ser manejada correctamente!\n"
        "\n"
        "Crear o borrar particiones puede destruir la tabla de particiones.\n"
        "\n"
        "  \x07  Presione F3 para salir del instalador."
        "  \x07  Presione ENTER para continuar.",
        "F3= Salir  ENTER = Continuar"
    },
    {
        //ERROR_NEW_PARTITION,
        "­No puede crear una nueva partici¢n dentro\n"
        "de una partici¢n existente!\n"
        "\n"
        "  * Presione cualquier tecla para continuar.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "­No puede borrar un espacio de disco sin particionar!\n"
        "\n"
        "  * Presione cualquier tecla para continuar.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "El instalador fall¢ al instalar el c¢digo de inicio FAT en la partici¢n del sistema.",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_NO_FLOPPY,
        "No hay disco en la unidad A:.",
        "ENTER = Continuar"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "El instalador fall¢ al actualizar la configuraci¢n de disposici¢n del teclado.",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "El instalador fall¢ al actualizar la configuraci¢n de la pantalla.",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_IMPORT_HIVE,
        "El instalador fall¢ al importar un archivo de la estructura.",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_FIND_REGISTRY
        "El instalador fall¢ al buscar los archivos de datos registrados.",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_CREATE_HIVE,
        "El instalador fall¢ al crear el registro de la estructura.",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "El instalador fall¢ al configurar el registro de inicio.",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet no tiene un archivo inf v lido.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet no encontrado.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet no tiene ning£n script de instalaci¢n.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_COPY_QUEUE,
        "El instalador fall¢ al abrir la lista de archivos a copiar.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_CREATE_DIR,
        "El instalador no puede crear los directorios de instalaci¢n.",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "El instalador fall¢ al buscar la secci¢n 'Directorios'\n"
        "en TXTSETUP.SIF.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_CABINET_SECTION,
        "El instalador fall¢ al buscar la secci¢n 'Directorios'\n"
        "en el cabinet.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "El instalador no puede crear el directorio de instalaci¢n.",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "El instalador fall¢ al buscar la secci¢n 'SetupData'\n"
        "en TXTSETUP.SIF.\n",
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_WRITE_PTABLE,
        "El instalador fall¢ al escribir la tabla de particiones.\n"
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "El instalador fall¢ al a¤adir el c¢digo de p ginas al registro.\n"
        "ENTER = Reiniciar el equipo"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "El instalador no pudo configurar el idioma del sistema.\n"
        "ENTER = Reiniciar el equipo"
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
        NULL,
        NULL
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
        REGISTRY_PAGE,
        esESRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING esESStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Por favor, espere..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Instalar   C = Crear Partici¢n   F3 = Salir"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalar   D = Borrar Partici¢n   F3 = Salir"},
    {STRING_PARTITIONSIZE,
     "Tamaño de la nueva partici¢n:"},
    {STRING_CHOOSENEWPARTITION,
     "Ha elegido crear una nueva partici¢n en"},
    {STRING_HDDSIZE,
    "Por favor, introduzca el tama¤o de la nueva partici¢n en megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Crear Partici¢n   ESC = Cancelar   F3 = Salir"},
    {STRING_PARTFORMAT,
    "Esta Partici¢n ser  formateada a continuaci¢n."},
    {STRING_NONFORMATTEDPART,
    "Ha elegido instalar ReactOS en una nueva Partici¢n o en una Partici¢n sin formato."},
    {STRING_INSTALLONPART,
    "El instalador instala ReactOS en la Partici¢n"},
    {STRING_CHECKINGPART,
    "El instalador est  comprobando la partici¢n seleccionada."},
    {STRING_QUITCONTINUE,
    "F3= Salir  ENTER = Continuar"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Reiniciar el equipo"},
    {STRING_TXTSETUPFAILED,
    "El instalador fall¢ al buscar la secci¢n\nin TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "\xB3 Copiando archivo: %S"},
    {STRING_SETUPCOPYINGFILES,
     "El instalador est  copiando archivos..."},
    {STRING_REGHIVEUPDATE,
    "   Actualizando el  rbol del registro..."},
    {STRING_IMPORTFILE,
    "   Importando %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Actualizando configuraci¢n del tipo de pantalla..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Actualizando configuraci¢n regional..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Actualizando configuraci¢n de disposici¢n del teclado..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Añadiendo informaci¢n al registro..."},
    {STRING_DONE,
    "   Realizado..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Reiniciar el equipo"},
    {STRING_CONSOLEFAIL1,
    "Incapaz de abrir la consola\n\n"},
    {STRING_CONSOLEFAIL2,
    "La causa m s com£n es la utilizaci¢n de un teclado USB\n"},
    {STRING_CONSOLEFAIL3,
    "Los teclados USB aun no son soportados completamente\n"},
    {STRING_FORMATTINGDISK,
    "El instalador est  formateando su disco"},
    {STRING_CHECKINGDISK,
    "El instalador est  verificando su disco"},
    {STRING_FORMATDISK1,
    " Formatear la partici¢n con sistema de archivos %S (formato r pido) "},
    {STRING_FORMATDISK2,
    " Formatear la partici¢n con sistema de archivos %S "},
    {STRING_KEEPFORMAT,
    " Mantener el sistema actual de archivos (sin cambios) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Disco duro %lu  (Port=%hu, Bus=%hu, Id=%hu) en %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Disco duro %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Tipo %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "en %I64u %s  Disco duro %lu  (Port=%hu, Bus=%hu, Id=%hu) en %wZ."},
    {STRING_HDDINFOUNK3,
    "en %I64u %s  Disco duro %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Disco duro %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Tipo %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "en Disco duro %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  Tipo %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Disco duro %lu  (Port=%hu, Bus=%hu, Id=%hu) en %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Disco duro %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "El instalador a creado una nueva partici¢n en"},
    {STRING_UNPSPACE,
    "    Espacio sin particionar              %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_UNFORMATTED,
    "Nuevo (sin formato)"},
    {STRING_FORMATUNUSED,
    "Libre"},
    {STRING_FORMATUNKNOWN,
    "desconocido"},
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
