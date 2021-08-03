// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once

static MUI_ENTRY ptPTSetupInitPageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "Por favor aguarde enquanto o Instalador do ReactOS inicializa",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "e detecta os seus dispositivos...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Por favor aguarde...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Selec\207\306o do idioma",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Por favor, seleccione o idioma a ser utilizado durante a instala\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  O idioma seleccionado ser\240 o idioma padr\306o do sistema.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "   Pressione ENTER para continuar.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTWelcomePageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Bem-vindo ao assist\210nte de instala\207\306o do ReactOS.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Esta fase da instala\207\306o prepara o ReactOS para ser",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "executado no seu computador.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Pressione ENTER para instalar ou actualizar o ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Para reparar uma instala\207\306o do ReactOS, pressione R.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Para ver os termos e condi\207\344es da licen\207a, pressione L.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Para sair sem instalar o ReactOS, pressione F3.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Para mais informa\207\344es sobre o ReactOS, visite o s\241tio:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "https://reactos.org/",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continuar  R=Reparar  L=Licen\207a  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTIntroPageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS Vers\306o Status",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "O ReactOS est\240 em est\240gio Alpha, o que significa que n\306o est\240 completo",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "e est\240 em forte desenvolvimento. Recomenda-se o seu uso apenas para fins",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "de avalia\207\306o e teste e n\306o como seu sistema operativo de uso di\240rio.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Fa\207a c\242pia de segur\306n\207 dos seus dados ou teste num computador",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "secund\240rio se pretende executar o ReactOS em hardware real.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Pressione ENTER para continuar a instala\207\306o do ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Pressione F3 para terminar sem instalar o ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuar   F3 = Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTLicensePageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Licen\207a:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "O ReactOS est\240 licenciado sob os termos da licen\207a",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL contendo partes de c\242digo licenciados sob outras",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "licen\207as compat\241veis, como X11 ou BSD e GNU LGPL.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Todo o software que faz parte do sistema ReactOS \202 portanto,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "livre sob a licen\207a GNU GPL, bem como mantendo sua licen\207a",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "original.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Este software vem sem NENHUMA GARANTIA ou restri\207\306o de uso",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "excepto as leis locais e internacionais aplicaveis.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "A licen\207a do ReactOS abrange apenas a distribui\207\306o a terceiros.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Se por alguma raz\306o n\306o recebeu uma c\242pia da licen\207a",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "GNU General Public License com o ReactOS por favor visite",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        22,
        "Garantia:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Este \202 um software livre; veja o c\242digo fonte para condi\207\344es de c\242pia.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "N\307O h\240 garantia; nem mesmo para COMERCIALIZA\200\307O ou",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "ADEQUA\200\307O PARA UM PROP\340SITO PARTICULAR",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Voltar",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTDevicePageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A lista seguinte mostra a configura\207\306o de dispositivos actual.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "Computador:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "V\241deo:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Teclado:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Tipo de teclado:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Confirmar:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Aceitar esta configura\207\306o de dispositivos",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para mudar de op\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "Para escolher uma configura\207\306o alternativa, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        22,
        "Quando finalizar os ajustes, seleccione \"Aceitar esta configura\207\306o",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "de dispositivos\" e pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTRepairPageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "O instalador do ReactOS est\240 em fase inicial de desenvolvimento e",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "ainda n\306o suporta todas as fun\207\344es de instala\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "As fun\207\344es de repara\207\306o ainda n\306o foram implementadas.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Para actualizar o sistema operacional, pressione U.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Para abrir a consola de recupera\207\306o, pressione R.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Para voltar para a p\240gina principal, pressione ESC.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Para reiniciar o computador, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC=P\240gina principal  U=Actualizar  R=Recuperar  ENTER=Reiniciar",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTUpgradePageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Com o Instalador do ReactOS pode actualizar uma das instala\207\344es",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "dispon\241veis listadas abaixo, ou, se tem uma instala\207\306o do ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "corrompida, pode tentar a sua recupera\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "As fun\207\344es de repara\207\306o ainda n\306o foram implementadas.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Use SETA A CIMA ou SETA A BAIXO para seleccionar uma instala\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Pressione U para actualizar  a instala\207\306o seleccionada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Pressione ESC para executar uma nova instala\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Pressione F3 para terminar sem instalar o ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Actualizar   ESC = N\343o actualizar   F3 = Terminar",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTComputerPageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de computadores dispon\241veis",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "para instala\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para seleccionar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "um item na lista.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "\x07  Para escolher o item seleccionado, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "\x07  Para cancelar a altera\207\306o, pressione ESC.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continuar  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTFlushPageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "O sistema est\240 a certificar que todos os dados est\306 a ser",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        7,
        "armazenados correctamente no disco.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Esta opera\207\306o pode demorar um minuto.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Quando terminar, o computador ser\240 reiniciado automaticamente.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "A esvaziar o cache",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTQuitPageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "O ReactOS n\306o foi totalmente instalado neste computador.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Se houver alguma disquete na unidade A: ou disco nas unidades",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "de CD-ROM, remova-os.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Para reiniciar o computador, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Por favor, aguarde...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTDisplayPageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de v\241deo dispon\241veis para instala\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
         "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para seleccionar",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        6,
        11,
        "um item na lista.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Para escolher o item seleccionado, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Para cancelar a altera\207\306o, pressione ESC.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continuar  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTSuccessPageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Os componentes b\240sicos do ReactOS foram instalados com sucesso.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Se houver alguma disquete na unidade A: ou disco nas unidades",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "de CD-ROM, remova-os.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Para reiniciar o computador, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Reiniciar",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTBootPageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "O instalador n\306o pode instalar o ger\210nciador de inicializa\207\306o no disco",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "r\241gido do computador.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Por favor insira uma disquete formatada na unidade A: e",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY ptPTSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        7,
        "A lista a seguir mostra as parti\207\344es existentes e os espa\207os",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "n\306o-particionados neste computador.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para seleccionar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "um item na lista.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Para configurar o ReactOS no item seleccionado, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Para criar uma  parti\207\306o prim\240ria, pressione P.",
//        "\x07  Para criar uma parti\207\306o no espa\207o n\306o particionado, pressione C.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Para criar uma parti\207\306o estendida, pressione E.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Para criar uma parti\207\3060 l\242gica, pressione L.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Para excluir a parti\207\306o seleccionada, pressione D.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Por favor, aguarde...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTChangeSystemPartition[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A actual parti\207\306o do sistema no disco do seu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "computador usa um",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "formato n\306o suportado pelo ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "Para instalar o ReactOS com sucesso, o instalador deve alterar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "a actual parti\207\306o para um novo tipo.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "O formato de parti\207\306o sugerido \202:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Para aceitar esta op\207\306o, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  Para alterar manualmente o tipo de parti\207\306o, pressione ESC. para retornar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   \205 lista de sele\207\306o de parti\207\306o e, em seguida, seleccione ou crie um novo tipo",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   de parti\207\306o no disco de sistema.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "No caso de haver outro sistema operativo que dependa da parti\207\306o",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "original, provavelmente precisar\240 de reconfigura-lo para a nova",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "parti\207\306o ou reverter para o tipo de parti\207\306o original",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "ap\242s a instala\207\306o do ReactOS ser concluida.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continue   ESC = Cancel",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Optou por excluir a parti\207\306o do sistema.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "As parti\207\344es do sistema podem conter programas de diagn\242stico,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "configura\207\344es de hardware, programas para iniciar um sistema operativo",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "(ReactOS) ou outro e programas fornecidos pelo fabricante do hardware.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Exclua uma parti\207\306o apenas quando tiver certeza de que n\306o cont\202m",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "programas ou quando desejar exclui-los definitivamente.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Quando exclui uma parti\207\306o, pode n\306o ser capaz de inicializar o",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "computador pelo disco r\241gido at\202 concluir a instala\207\306o do ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Pressione ENTER para excluir a parti\207\306o do sistema. Ser\240 solicitado",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   a confirmar a exclus\306o da parti\207\306o novamente mais tarde.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Pressione ESC para retornar a p\240gina anterior. A parti\207\306o",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   n\306o ser\240 excluida.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continue  ESC=Cancel",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Formatar a parti\207\306o",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "O instalador ir\240 formatar a parti\207\306o. Para continuar, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTCheckFSEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "O instalador est\240 a verificar a parti\207\306o seleccionada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Por favor, aguarde...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "O instalador ir\240 copiar os arquivos para a parti\207\306o seleccionada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Seleccione o direct\242rio onde deseja que o ReactOS seja instalado:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Para mudar o direct\242rio sugerido, pressione a tecla BACKSPACE para apagar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "o texto e escreva o nome do direct\242rio onde deseja que o ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "seja instalado.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continuar  F3 = Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTFileCopyEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "Por favor aguarde enquanto o instalador copia os",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "arquivos do ReactOS para a pasta de instala\207\306o.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Esta opera\207\306o pode demorar um minuto.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Por favor, aguarde...",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTBootLoaderEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "O instalador ir\240 configurar o ger\210nciador de inicializa\207\306o",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "Instalar o ger\210nciador de inicializa\207\306o no disco r\241gido (MBR e VBR)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Instalar o ger\210nciador de inicializa\207\306o no disco r\241gido (apenas VBR)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Instalar o ger\210nciador de inicializa\207\306o numa disquete",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Saltar a instala\207\306o do ger\210nciador de inicializa\207\306o",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS  | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTBootLoaderInstallPageEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "A instalar o programa de arranque no disco de instala\207\306o. Por favor aguarde...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de teclados dispon\241veis para instala\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para seleccionar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "um item na lista.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Para escolher o item seleccionado, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Para cancelar a altera\207\306o, pressione ESC.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continuar  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A lista a seguir mostra os estilos de teclado dispon\241veis",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "para instala\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para seleccionar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "um item na lista.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "\x07  Para escolher o item seleccionado, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "\x07  Para cancelar a altera\207\306o, pressione ESC.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continuar  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ptPTPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "O instalador est\240 a preparar o computador para copiar os arquivos",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "do ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "A montar a lista de arquivos a serem copiados...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ptPTSelectFSEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Seleccione um sistema de arquivos para a nova parti\207\306o na lista abaixo.",
        0
    },
    {
        6,
        17,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para seleccionar o",
        0
    },
    {
        6,
        18,
        "sistema de arquivos de arquivos desejado e pressione ENTER.",
        0
    },
    {
        8,
        20,
        "Se desejar seleccionar uma parti\207\306o diferente, pressione ESC.",
        0
    },
    {
        0,
        0,
        "ENTER=Continuar  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Solicitou a exclus\306o da parti\207\306o",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Para excluir esta parti\207\306o, pressione L",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "CUIDADO: todos os dados da parti\207\306o ser\306o perdidos!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Para retornar \205 ecr\306 anterior sem excluir",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        22,
        "a parti\207\306o, pressione ESC.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L=Excluir  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptPTRegistryEntries[] =
{
    {
        4,
        3,
        " Instala\207\306o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "O instalador est\240 a actualizar a configura\207\306o do sistema.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "A criar a estrutura de registo...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR ptPTErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Sucesso\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "O ReactOS n\306o est\240 completamente instalado no computador.\n"
        "Se sair da instala\207\306o agora, precisar\240 executa-la\n"
        "novamente para instalar o ReactOS.\n"
        "\n"
        "  \x07  Para continuar a instala\207\306o, pressione ENTER.\n"
        "  \x07  Para sair da instala\207\306o, pressione F3.",
        "F3=Sair  ENTER=Continuar"
    },
    {
        // ERROR_NO_HDD
        "N\306o foi poss\241vel localizar um disco r\241digo.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "N\306o foi poss\241vel localizar a unidade de origem.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "N\306o foi poss\241vel carregar o arquivo TXTSETUP.SIF.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "O arquivos TXTSETUP.SIF est\240 corrompido.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "O arquivo TXTSETUP.SIF est\240 com a assinatura incorrecta.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "N\306o foi poss\241vel obter as informa\207\344es sobre o disco do sistema.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_WRITE_BOOT,
        "Erro ao escrever o c\242digo de inicializa\207\306o %S na parti\207\306o do sistema.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "N\306o foi poss\241vel carregar a lista de tipos de computadores.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "N\306o foi poss\241vel carregar a lista de tipos de v\241deo.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "N\306o foi poss\241vel carregar a lista de tipos de teclado.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "N\306o foi poss\241vel carregar a lista de estilos de teclado.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_WARN_PARTITION,
        "O instalador encontrou uma tabela de parti\207\306o incompat\241vel\n"
        "que n\306o pode ser utilizada correctamente!\n"
        "\n"
        "Criar ou excluir parti\207\344es pode destruir a tabela de parti\207\306o.\n"
        "\n"
        "  \x07  Para sair da instala\207\306o, pressione F3.\n"
        "  \x07  Para continuar, pressione ENTER.",
        "F3=Sair  ENTER=Continuar"
    },
    {
        // ERROR_NEW_PARTITION,
        "Pode criar uma parti\207\306o dentro de\n"
        "outra parti\207\306o j\240 existente!\n"
        "\n"
        "  * Pressione qualquer tecla para continuar.",
        NULL
    },
    {
        // ERROR_DELETE_SPACE,
        "Pode excluir um espa\207o n\306o particionado!\n"
        "\n"
        "  * Pressione qualquer tecla para continuar.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Erro ao instalar o c\242digo de inicializa\207\306o %S na parti\207\306o do sistema.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_NO_FLOPPY,
        "N\306o h\240 disco na unidade A:.",
        "ENTER=Continuar"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "N\306o foi poss\241vel actualizar a configura\207\306o de estilo de teclado.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "N\306o foi poss\241vel actualizar a configura\207\306o de v\241deo.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_IMPORT_HIVE,
        "N\306o foi poss\241vel importar o arquivo de estrutura.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_FIND_REGISTRY
        "N\306o foi poss\241vel encontrar os arquivos do registo.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CREATE_HIVE,
        "N\306o foi poss\241vel criar as estruturas do registo.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "N\306o foi poss\241vel inicializar o registo.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "O arquivo cab n\306o cont\202m um arquivo inf v\240lido.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CABINET_MISSING,
        "N\306o foi poss\241vel econtrar o arquivo cab.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "O arquivo cab n\306o cont\202m um script de instala\207\306o.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_COPY_QUEUE,
        "N\306o foi poss\241vel abrir a lista de arquivos para c\242pia.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CREATE_DIR,
        "N\306o foi poss\241vel criar os direct\242rios de instala\207\306o.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "N\306o foi poss\241vel encontrar a sec\207\306o '%S' no\n"
        "arquivo TXTSETUP.SIF.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CABINET_SECTION,
        "N\306o foi poss\241vel encontrar a sec\207\306o '%S' no\n"
        "arquivo cab.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "N\306o foi poss\241vel criar o direct\242rio de instala\207\306o.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_WRITE_PTABLE,
        "N\306o foi poss\241vel escrever a tabela de parti\207\344es.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "N\306o foi poss\241vel adicionar o c\242digo de localidade no registo.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "N\306o foi poss\241vel configurar o idioma do sistema.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "N\306o foi poss\241vel adicionar o tipo do teclado no registo.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_UPDATE_GEOID,
        "N\306o foi poss\241vel configurar a identifica\207\306o geogr\240fica.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Nome de direct\242rio inv\240lido.\n"
        "\n"
        "  * Pressione qualquer tecla para continuar."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "A parti\207\306o selecionada n\306o \202 grande o suficiente para instalar o ReactOS.\n"
        "A parti\207\306o de instala\207\306o deve ter um tamanho de pelo menos %lu MB.\n"
        "\n"
        "  * Pressione qualquer tecla para continuar.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "N\306o pode criar uma nova parti\207\306o prim\240ria ou estendida\n"
        "neste disco, porque a tabela de parti\207\344es est\240 cheia.\n"
        "\n"
        "  * Pressione qualquer tecla para continuar."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "N\306o pode criar mais de uma parti\207\306o estendida por disco.\n"
        "\n"
        "  * Pressione qualquer tecla para continuar."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "A instala\207\306o n\306o consegue formatar a parti\207\306o:\n"
        " %S\n"
        "\n"
        "ENTER = Reiniciar o computador"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE ptPTPages[] =
{
    {
        SETUP_INIT_PAGE,
        ptPTSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        ptPTLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        ptPTWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        ptPTIntroPageEntries
    },
    {
        LICENSE_PAGE,
        ptPTLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        ptPTDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        ptPTRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        ptPTUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        ptPTComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        ptPTDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        ptPTFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        ptPTSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        ptPTChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        ptPTConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        ptPTSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        ptPTFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        ptPTCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        ptPTDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        ptPTInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        ptPTPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        ptPTFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        ptPTKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        ptPTBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        ptPTLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        ptPTQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        ptPTSuccessPageEntries
    },
    {
        BOOT_LOADER_INSTALLATION_PAGE,
        ptPTBootLoaderInstallPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        ptPTBootPageEntries
    },
    {
        REGISTRY_PAGE,
        ptPTRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING ptPTStrings[] =
{
    {STRING_PLEASEWAIT,
    "   Por favor, aguarde..."},
    {STRING_INSTALLCREATEPARTITION,
    "   ENTER=Instalar  P=Criar Part. Prim\240ria E=Criar Part. Estendida  F3=Sair"},
    {STRING_INSTALLCREATELOGICAL,
    "   ENTER = Instalar   L = Criar Parti\207\306o L\242gica   F3 = Sair"},
    {STRING_INSTALLDELETEPARTITION,
    "   ENTER=Instalar  D=Apagar parti\207\306o  F3 = Sair"},
    {STRING_DELETEPARTITION,
     "   D = Apagar Parti\207\306o   F3 = Sair"},
    {STRING_PARTITIONSIZE,
    "Tamanho da nova parti\207\306o:"},
    {STRING_CHOOSENEWPARTITION,
     "Seleccionou criar uma parti\207\306o prim\240ria"},
//    "Voc\210 solicitou a cria\207\306o de uma nova parti\207\306o em"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Seleccionou criar uma parti\207\306o estendida"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Seleccionou criar uma parti\207\306o l\242gica"},
    {STRING_HDDSIZE,
    "Por favor, insira o tamanho da nova parti\207\306o em megabytes (MB)."},
    {STRING_CREATEPARTITION,
    "   ENTER=Criar parti\207\306o  ESC=Cancelar  F3=Sair"},
    {STRING_PARTFORMAT,
    "Esta parti\207\306o ser\240 formatada logo em seguida."},
    {STRING_NONFORMATTEDPART,
    "Solicitou instalar o ReactOS numa parti\207\306o nova ou sem formato."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "A parti\207\306o ainda n\306o est\240 formatada."},
    {STRING_NONFORMATTEDOTHERPART,
    "A nova parti\207\306o ainda n\306o est\240 formatada."},
    {STRING_INSTALLONPART,
    "O instalador instala o ReactOS na parti\207\306o"},
    {STRING_CONTINUE,
    "ENTER=Continuar"},
    {STRING_QUITCONTINUE,
    "F3=Sair  ENTER=Continuar"},
    {STRING_REBOOTCOMPUTER,
    "ENTER=Reiniciar"},
    {STRING_DELETING,
     "   A apagar arquivo: %S"},
    {STRING_MOVING,
     "   A mover arquivo: %S to: %S"},
    {STRING_RENAMING,
     "   A renomear arquivo: %S to: %S"},
    {STRING_COPYING,
    "   A Copiar arquivo: %S"},
    {STRING_SETUPCOPYINGFILES,
    "O instalador est\240 a copiar os arquivos..."},
    {STRING_REGHIVEUPDATE,
    "  A actualizar a estrutura do registo..."},
    {STRING_IMPORTFILE,
    "  A importar %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   A actualizar a configura\207\344o de v\241deo..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   A actualizar as configura\207\344es regionais..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   A actualizar as configura\207\344es do teclado..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   A adicionar as informa\207\344es de localiza\207\306o no registo..."},
    {STRING_DONE,
    "   Pronto..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER=Reiniciar"},
    {STRING_REBOOTPROGRESSBAR,
    " O computador ir\240 reiniciar em %li segundo(s)... "},
    {STRING_CONSOLEFAIL1,
    "N\306o foi poss\241vel abrir o console\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "A causa mais com\243m \202 a utiliza\207\306o de um teclado USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Os teclados USB ainda n\306o s\306o completamente suportados\r\n"},
    {STRING_FORMATTINGDISK,
    "O instalador est\240 a formatar o disco"},
    {STRING_CHECKINGDISK,
    "O instalador est\240 a verificar o disco"},
    {STRING_FORMATDISK1,
    " Formatar a parti\207\306o utilizando o sistema de arquivos %S (R\240pido) "},
    {STRING_FORMATDISK2,
    " Formatar a parti\207\306o utilizando o sistema de arquivos %S "},
    {STRING_KEEPFORMAT,
    " Manter o sistema de arquivos actual (sem altera\207\344es) "},
    {STRING_HDINFOPARTCREATE_1,
    "%s."},
    {STRING_HDINFOPARTDELETE_1,
    "em %s."},
    {STRING_PARTTYPE,
    "Tipo 0x%02x"},
    {STRING_HDDINFO_1,
    // "Disco %lu (%I64u %s), Porta=%hu, Barramento=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Disco %lu (Porta=%hu, Barramento=%hu, Id=%hu) em %wZ [%s]"},
    {STRING_HDDINFO_2,
    // "Disco %lu (%I64u %s), Porta=%hu, Barramento=%hu, Id=%hu [%s]"
    "%I64u %s Disco %lu (Porta=%hu, Barramento=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "O instalador criou uma nova parti\207\306o em"},
    {STRING_UNPSPACE,
    "Espa\207o n\306o particionado"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Parti\207\306 Estendida"},
    {STRING_UNFORMATTED,
    "Novo (sem formato)"},
    {STRING_FORMATUNUSED,
    "Livre"},
    {STRING_FORMATUNKNOWN,
    "desconhecido"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "A adicionar tipos de teclado"},
    {0, 0}
};
