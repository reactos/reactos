// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once

static MUI_ENTRY ptBRSetupInitPageEntries[] =
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
        "Please wait while the ReactOS Setup initializes itself",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "and discovers your devices...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Please wait...",
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

static MUI_ENTRY ptBRLanguagePageEntries[] =
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
        "Sele\207\306o do idioma",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Por favor, selecione o idioma a ser utilizado durante a instala\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Ent\306o pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  O idioma selecionado tamb\202m ser\240 o idioma padr\306o do sistema.",
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

static MUI_ENTRY ptBRWelcomePageEntries[] =
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
        "Bem-vindo \205 instala\207\306o do ReactOS.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Esta parte da instala\207\306o prepara o ReactOS para ser",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "executado em seu computador.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press ENTER to install or upgrade ReactOS.",
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
        "Para maiores informa\207\344es sobre o ReactOS, visite o s\241tio:",
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

static MUI_ENTRY ptBRIntroPageEntries[] =
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
        "ReactOS Version Status",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS is in Alpha stage, meaning it is not feature-complete",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "and is under heavy development. It is recommended to use it only for",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "evaluation and testing purposes and not as your daily-usage OS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Backup your data or test on a secondary computer if you attempt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "to run ReactOS on real hardware.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Press ENTER to continue ReactOS Setup.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continue   F3 = Quit",
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

static MUI_ENTRY ptBRLicensePageEntries[] =
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
        "Todo o software que faz parte do ReactOS \202 portanto, liberado",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "sob a licen\207a GNU GPL, bem como a manuten\207\306o da licen\207a",
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
        "exceto onde leis locais e internacionais s\306o aplicaveis. A licen\207a",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "do ReactOS abrange apenas a distribui\207\306o a terceiros.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Se por alguma raz\306o voc\210 n\306o recebeu uma c\242pia da licen\207a",
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

static MUI_ENTRY ptBRDevicePageEntries[] =
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
        "A lista a seguir mostra as configura\207\344es de dispositivos atual.",
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
        "Leiaute teclado:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Aceitar:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Aceitar essas configura\207\344es de dispositivo",
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
        "Quanto finalizar os ajustes, selecione \"Aceitar essas configura\207\344es",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "de dispositivo\" e pressione ENTER.",
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

static MUI_ENTRY ptBRRepairPageEntries[] =
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
        "As fun\207\344es repara\207\306o ainda n\306o foram implementadas.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Para atualizar o sistema operacional, pressione U.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Para abrir o console de recupera\207\306o, pressione R.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Para voltar a p\240gina principal, pressione ESC.",
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
        "ESC=P\240gina principal  U=Atualizar  R=Recuperar  ENTER=Reiniciar",
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

static MUI_ENTRY ptBRUpgradePageEntries[] =
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
        "The ReactOS Setup can upgrade one of the available ReactOS installations",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "listed below, or, if a ReactOS installation is damaged, the Setup program",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "can attempt to repair it.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "The repair functions are not all implemented yet.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press UP or DOWN to select an OS installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Press U for upgrading the selected OS installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Press ESC to continue with a new installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Upgrade   ESC = Do not upgrade   F3 = Quit",
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

static MUI_ENTRY ptBRComputerPageEntries[] =
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
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar",
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
        "\x07  Para escolher o item selecionado, pressione ENTER.",
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

static MUI_ENTRY ptBRFlushPageEntries[] =
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
        "O sistema est\240 agora certificando que todos os dados estejam sendo",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        7,
        "armazenados corretamente no disco.",
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
        "Esvaziando o cache",
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

static MUI_ENTRY ptBRQuitPageEntries[] =
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
        "Se houver algum disquete na unidade A: ou disco nas unidades",
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

static MUI_ENTRY ptBRDisplayPageEntries[] =
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
         "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar",
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
        "\x07  Para escolher o item selecionado, pressione ENTER.",
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

static MUI_ENTRY ptBRSuccessPageEntries[] =
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
        "Se houver algum disquete na unidade A: ou disco nas unidades",
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

static MUI_ENTRY ptBRSelectPartitionEntries[] =
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
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar",
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
        "\x07  Para configurar o ReactOS no item selecionado, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Para criar uma parti\207\306o prim\240ria/l\242gica, pressione C.",
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
        "\x07  Para excluir a parti\207\306o selecionada, pressione D.",
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

static MUI_ENTRY ptBRChangeSystemPartition[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "The current system partition of your computer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "on the system disk",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "uses a format not supported by ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "In order to successfully install ReactOS, the Setup program must change",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "the current system partition to a new one.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "The new candidate system partition is:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  To accept this choice, press ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  To manually change the system partition, press ESC to go back to",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   the partition selection list, then select or create a new system",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   partition on the system disk.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "In case there are other operating systems that depend on the original",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "system partition, you may need to either reconfigure them for the new",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "system partition, or you may need to change the system partition back",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "to the original one after finishing the installation of ReactOS.",
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

static MUI_ENTRY ptBRConfirmDeleteSystemPartitionEntries[] =
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
        "You have chosen to delete the system partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "System partitions can contain diagnostic programs, hardware configuration",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "programs, programs to start an operating system (like ReactOS) or other",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "programs provided by the hardware manufacturer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Delete a system partition only when you are sure that there are no such",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "programs on the partition, or when you are sure you want to delete them.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "When you delete the partition, you might not be able to boot the",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "computer from the harddisk until you finished the ReactOS Setup.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Press ENTER to delete the system partition. You will be asked",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   to confirm the deletion of the partition again later.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Press ESC to return to the previous page. The partition will",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   not be deleted.",
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

static MUI_ENTRY ptBRFormatPartitionEntries[] =
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
        "Formatar parti\207\306o",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "O instalador ir\240 formatar a parti\207\306o. Para continuar, pressione ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS,
        0
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY ptBRCheckFSEntries[] =
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
        "O instalador est\240 verificando a parti\207\306o selecionada.",
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

static MUI_ENTRY ptBRInstallDirectoryEntries[] =
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
        "O instalador ir\240 copiar os arquivos para a parti\207\306o selecionada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Selecione um diret\242rio onde deseja que o ReactOS seja instalado:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Para mudar o diret\242rio sugerido, pressione a tecla BACKSPACE para apagar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "o texto e escreva o nome do diret\242rio onde deseja que o ReactOS",
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

static MUI_ENTRY ptBRFileCopyEntries[] =
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
        "Esta opera\207\306o pode demorar alguns minutos.",
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

static MUI_ENTRY ptBRBootLoaderSelectPageEntries[] =
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
        "Please select where Setup should install the bootloader:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "Instalar o ger\210nciador de inic. no disco r\241gido (MBR e VBR)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Instalar o ger\210nciador de inic. no disco r\241gido (apenas VBR)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Instalar o ger\210nciador de inicializa\207\306o em um disquete",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Pular a instala\207\306o do ger\210nciador de inicializa\207\306o",
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

static MUI_ENTRY ptBRBootLoaderInstallPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "O instalador ir\240 configurar o ger\210nciador de inicializa\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Installing the bootloader onto the media, please wait...",
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

static MUI_ENTRY ptBRBootLoaderRemovableDiskPageEntries[] =
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
        "O instalador ir\240 configurar o ger\210nciador de inicializa\207\306o.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Por favor insira um disquete formatado na unidade A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "e pressione ENTER.",
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

static MUI_ENTRY ptBRKeyboardSettingsEntries[] =
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
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar",
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
        "\x07  Para escolher o item selecionado, pressione ENTER.",
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

static MUI_ENTRY ptBRLayoutSettingsEntries[] =
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
        "A lista a seguir mostra os tipos de leiautes de teclado dispon\241veis",
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
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar",
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
        "\x07  Para escolher o item selecionado, pressione ENTER.",
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

static MUI_ENTRY ptBRPrepareCopyEntries[] =
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
        "O instalador est\240 preparando o computador para copiar os arquivos",
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
        "Montando a lista de arquivos a serem copiados...",
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

static MUI_ENTRY ptBRSelectFSEntries[] =
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
        "Selecione um sistema de arquivos para a nova parti\207\306o na lista abaixo.",
        0
    },
    {
        6,
        17,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar o",
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
        "Se desejar selecionar uma parti\207\306o diferente, pressione ESC.",
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

static MUI_ENTRY ptBRDeletePartitionEntries[] =
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
        "Voc\210 solicitou a exclus\306o da parti\207\306o",
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
        "\x07  Para retornar \205 tela anterior sem excluir",
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

static MUI_ENTRY ptBRRegistryEntries[] =
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
        "O instalador est\240 atualizando a configura\207\306o do sistema.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Criando a estrutura de registro...",
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

MUI_ERROR ptBRErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "O ReactOS n\306o est\240 completamente instalado no computador.\n"
        "Se voc\210 sair da instala\207\306o agora, precisar\240 executa-la\n"
        "novamente para instalar o ReactOS.\n"
        "\n"
        "  \x07  Para continuar a instala\207\306o, pressione ENTER.\n"
        "  \x07  Para sair da instala\207\306o, pressione F3.",
        "F3=Sair  ENTER=Continuar"
    },
    {
        // ERROR_NO_BUILD_PATH
        "Failed to build the installation paths for the ReactOS installation directory!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_PATH
        "You cannot delete the partition containing the installation source!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_DIR
        "You cannot install ReactOS within the installation source directory!\n"
        "ENTER = Reboot computer"
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
        "O arquivo TXTSETUP.SIF est\240 com a assinatura incorreta.\n",
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
        "N\306o foi poss\241vel carregar a lista de leiautes de teclado.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_WARN_PARTITION,
        "O instalador encontrou uma tabela de parti\207\306o incompat\241vel\n"
        "que n\306o pode ser utilizada corretamente!\n"
        "\n"
        "Criar ou excluir parti\207\344es pode destruir a tabela de parti\207\306o.\n"
        "\n"
        "  \x07  Para sair da instala\207\306o, pressione F3.\n"
        "  \x07  Para continuar, pressione ENTER.",
        "F3=Sair  ENTER=Continuar"
    },
    {
        // ERROR_NEW_PARTITION,
        "Voc\210 n\306o pode criar uma parti\207\306o dentro de\n"
        "outra parti\207\306o j\240 existente!\n"
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
        "N\306o foi poss\241vel atualizar a configura\207\306o de leiaute de teclado.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "N\306o foi poss\241vel atualizar a configura\207\306o de v\241deo.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_IMPORT_HIVE,
        "N\306o foi poss\241vel importar o arquivo de estrutura.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_FIND_REGISTRY
        "N\306o foi poss\241vel encontrar os arquivos do registro.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CREATE_HIVE,
        "N\306o foi poss\241vel criar as estruturas do registro.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "N\306o foi poss\241vel inicializar o registro.",
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
        "N\306o foi poss\241vel criar os diret\242rios de instala\207\306o.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "N\306o foi poss\241vel encontrar a se\207\306o '%S' no\n"
        "arquivo TXTSETUP.SIF.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CABINET_SECTION,
        "N\306o foi poss\241vel encontrar a se\207\306o '%S' no\n"
        "arquivo cab.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "N\306o foi poss\241vel criar o diret\242rio de instala\207\306o.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_WRITE_PTABLE,
        "N\306o foi poss\241vel escrever a tabela de parti\207\344es.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "N\306o foi poss\241vel adicionar o c\242digo de localidade no registro.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "N\306o foi poss\241vel configurar o idioma do sistema.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "N\306o foi poss\241vel adicionar o leiaute do teclado no registro.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_UPDATE_GEOID,
        "N\306o foi poss\241vel configurar a identifica\207\306o geogr\240fica.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Invalid directory name.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "The selected partition is not large enough to install ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "\n"
        "  * Pressione qualquer tecla para continuar.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "You cannot create a new primary or extended partition in the\n"
        "partition table of this disk because the partition table is full.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "You cannot create more than one extended partition per disk.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_FORMATTING_PARTITION,
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

MUI_PAGE ptBRPages[] =
{
    {
        SETUP_INIT_PAGE,
        ptBRSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        ptBRLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        ptBRWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        ptBRIntroPageEntries
    },
    {
        LICENSE_PAGE,
        ptBRLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        ptBRDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        ptBRRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        ptBRUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        ptBRComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        ptBRDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        ptBRFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        ptBRSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        ptBRChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        ptBRConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        ptBRSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        ptBRFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        ptBRCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        ptBRDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        ptBRInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        ptBRPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        ptBRFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        ptBRKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        ptBRBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        ptBRLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        ptBRQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        ptBRSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        ptBRBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        ptBRBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        ptBRRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING ptBRStrings[] =
{
    {STRING_PLEASEWAIT,
    "   Por favor, aguarde..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Instalar  C = Criar Prim\240ria  E = Criar Estendida  F3 = Sair"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Instalar  C = Criar parti\207\306o L\242gica  F3 = Sair"},
    {STRING_INSTALLDELETEPARTITION,
    "   ENTER = Instalar  D = Apagar parti\207\306o  F3 = Sair"},
    {STRING_DELETEPARTITION,
     "   D = Apagar parti\207\306o   F3 = Quit"},
    {STRING_PARTITIONSIZE,
    "Tamanho da nova parti\207\306o:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "You have chosen to create a primary partition on"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDPARTSIZE,
    "Por favor, insira o tamanho da nova parti\207\306o em megabytes (MB)."},
    {STRING_CREATEPARTITION,
    "   ENTER=Criar parti\207\306o  ESC=Cancelar  F3=Sair"},
    {STRING_NEWPARTITION,
    "O instalador criou uma nova parti\207\306o em"},
    {STRING_PARTFORMAT,
    "Esta parti\207\306o ser\240 formatada logo em seguida."},
    {STRING_NONFORMATTEDPART,
    "Voc\210 solicitou instalar o ReactOS em uma parti\207\306o nova ou sem formato."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "O instalador instala o ReactOS na parti\207\306o"},
    {STRING_CONTINUE,
    "ENTER=Continuar"},
    {STRING_QUITCONTINUE,
    "F3=Sair  ENTER=Continuar"},
    {STRING_REBOOTCOMPUTER,
    "ENTER=Reiniciar"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
    "   Copiando arquivo: %S"},
    {STRING_SETUPCOPYINGFILES,
    "O instalador est\240 copiando os arquivos..."},
    {STRING_REGHIVEUPDATE,
    "   Atualizando a estrutura do registro..."},
    {STRING_IMPORTFILE,
    "   Importando %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Atualizando as configura\207\344es de v\241deo..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Atualizando as configura\207\344es regionais..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Atualizando as configura\207\344es de leiaute do teclado..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Adicionando as informa\207\344es de localidade..."},
    {STRING_DONE,
    "   Pronto..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER=Reiniciar"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "N\306o foi poss\241vel abrir o console\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "A causa mais com\243m \202 a utiliza\207\306o de um teclado USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Os teclados USB ainda n\306o s\306o completamente suportados\r\n"},
    {STRING_FORMATTINGPART,
    "O instalador est\240 formatando a parti\207\306o..."},
    {STRING_CHECKINGDISK,
    "O instalador est\240 verificando o disco..."},
    {STRING_FORMATDISK1,
    " Formatar a parti\207\306o utilizando o sistema de arquivos %S (R\240pido) "},
    {STRING_FORMATDISK2,
    " Formatar a parti\207\306o utilizando o sistema de arquivos %S "},
    {STRING_KEEPFORMAT,
    " Manter o sistema de arquivos atual (sem altera\207\344es) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "em %s."},
    {STRING_PARTTYPE,
    "Tipo 0x%02x"},
    {STRING_HDDINFO1,
    // "Disco %lu (%I64u %s), Porta=%hu, Barramento=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Disco %lu (Porta=%hu, Barramento=%hu, Id=%hu) em %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Disco %lu (%I64u %s), Porta=%hu, Barramento=%hu, Id=%hu [%s]"
    "%I64u %s Disco %lu (Porta=%hu, Barramento=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Espa\207o n\306o particionado"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
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
    "Adicionando leiautes de teclado"},
    {0, 0}
};
