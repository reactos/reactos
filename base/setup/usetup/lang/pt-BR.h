#pragma once

static MUI_ENTRY ptBRLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Seleá∆o do idioma",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Por favor, selecione o idioma a ser utilizado durante a instalaá∆o.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Ent∆o pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  O idioma selecionado tambÇm ser† o idioma padr∆o do sistema.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Bem-vindo Ö instalaá∆o do ReactOS.",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Esta parte da instalaá∆o prepara o ReactOS para ser",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "executado em seu computador.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press ENTER to install or upgrade ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Para reparar uma instalaá∆o do ReactOS, pressione R.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Para ver os termos e condiá‰es da licenáa, pressione L.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Para sair sem instalar o ReactOS, pressione F3.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Para maiores informaá‰es sobre o ReactOS, visite o s°tio:",
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
        "ENTER=Continuar  R=Reparar  L=Licenáa  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Version Status",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "ReactOS is in Alpha stage, meaning it is not feature-complete",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "and is under heavy development. It is recommended to use it only for",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "evaluation and testing purposes and not as your daily-usage OS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "Backup your data or test on a secondary computer if you attempt",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "to run ReactOS on real hardware.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Press ENTER to continue ReactOS Setup.",
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
        "ENTER = Continue   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licenáa:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "O ReactOS est† licenciado sob os termos da licenáa",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL contendo partes de c¢digo licenciados sob outras",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenáas compat°veis, como X11 ou BSD e GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Todo o software que faz parte do ReactOS Ç portanto, liberado",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "sob a licenáa GNU GPL, bem como a manutená∆o da licenáa",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "original.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Este software vem sem NENHUMA GARANTIA ou restriá∆o de uso",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "exceto onde leis locais e internacionais s∆o aplicaveis. A licenáa",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "do ReactOS abrange apenas a distribuiá∆o a terceiros.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Se por alguma raz∆o vocà n∆o recebeu uma c¢pia da licenáa",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License com o ReactOS por favor visite",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        22,
        "Garantia:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Este Ç um software livre; veja o c¢digo fonte para condiá‰es de c¢pia.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "N«O h† garantia; nem mesmo para COMERCIALIZAÄ«O ou",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "ADEQUAÄ«O PARA UM PROP‡SITO PARTICULAR",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Voltar",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A lista a seguir mostra as configuraá‰es de dispositivos atual.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Computador:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "V°deo:",
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
        "Leiaute teclado:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Aceitar:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Aceitar essas configuraá‰es de dispositivo",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para mudar de opá∆o.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "Para escolher uma configuraá∆o alternativa, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        22,
        "Quanto finalizar os ajustes, selecione \"Aceitar essas configuraá‰es",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "de dispositivo\" e pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador do ReactOS est† em fase inicial de desenvolvimento e",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ainda n∆o suporta todas as funá‰es de instalaá∆o.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "As funá‰es reparaá∆o ainda n∆o foram implementadas.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Para atualizar o sistema operacional, pressione U.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Para abrir o console de recuperaá∆o, pressione R.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Para voltar a p†gina principal, pressione ESC.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Para reiniciar o computador, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC=P†gina principal  U=Atualizar  R=Recuperar  ENTER=Reiniciar",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "\x07  Press ESC to continue with a new installation.",
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

static MUI_ENTRY ptBRComputerPageEntries[] =
{
    {
        4,
        3,
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de computadores dispon°veis",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "para instalaá∆o.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "um item na lista.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "\x07  Para escolher o item selecionado, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "\x07  Para cancelar a alteraá∆o, pressione ESC.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continuar  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "O sistema est† agora certificando que todos os dados estejam sendo",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        7,
        "armazenados corretamente no disco.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Esta operaá∆o pode demorar um minuto.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Quando terminar, o computador ser† reiniciado automaticamente.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Esvaziando o cache",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "O ReactOS n∆o foi totalmente instalado neste computador.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Se houver algum disquete na unidade A: ou disco nas unidades",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "de CD-ROM, remova-os.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Para reiniciar o computador, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Por favor, aguarde...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de v°deo dispon°veis para instalaá∆o.",
        TEXT_STYLE_NORMAL
    },
    {   6,
        10,
         "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar",
         TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "um item na lista.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Para escolher o item selecionado, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Para cancelar a alteraá∆o, pressione ESC.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continuar  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Os componentes b†sicos do ReactOS foram instalados com sucesso.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Se houver algum disquete na unidade A: ou disco nas unidades",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "de CD-ROM, remova-os.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Para reiniciar o computador, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Reiniciar",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptBRBootPageEntries[] =
{
    {
        4,
        3,
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador n∆o pìde instalar o gerànciador de inicializaá∆o no disco",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "r°gido do computador.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Por favor insira um disquete formatado na unidade A: e",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "pressione ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        7,
        "A lista a seguir mostra as partiá‰es existentes e os espaáos",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        8,
        "n∆o-particionados neste computador.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "um item na lista.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Para configurar o ReactOS no item selecionado, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press P to create a primary partition.",
//        "\x07  Para criar uma partiá∆o no espaáo n∆o particionado, pressione C.",
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
        "\x07  Para excluir a partiá∆o selecionada, pressione D.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Por favor, aguarde...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY ptBRFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Formatar partiá∆o",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "O instalador ir† formatar a partiá∆o. Para continuar, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY ptBRInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador ir† copiar os arquivos para a partiá∆o selecionada.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Selecione um diret¢rio onde deseja que o ReactOS seja instalado:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Para mudar o diret¢rio sugerido, pressione a tecla BACKSPACE para apagar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "o texto e escreva o nome do diret¢rio onde deseja que o ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "seja instalado.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continuar  F3 = Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Por favor aguarde enquanto o instalador copia os",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "arquivos do ReactOS para a pasta de instalaá∆o.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Esta operaá∆o pode demorar alguns minutos.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 Por favor, aguarde...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ptBRBootLoaderEntries[] =
{
    {
        4,
        3,
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador ir† configurar o gerànciador de inicializaá∆o",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Instalar o gerànciador de inic. no disco r°gido (MBR e VBR)",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Instalar o gerànciador de inic. no disco r°gido (apenas VBR)",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Instalar o gerànciador de inicializaá∆o em um disquete",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Pular a instalaá∆o do gerànciador de inicializaá∆o",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continuar  F3=Sair",
        TEXT_TYPE_STATUS  | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de teclados dispon°veis para instalaá∆o.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "um item na lista.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Para escolher o item selecionado, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Para cancelar a alteraá∆o, pressione ESC.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continuar  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de leiautes de teclado dispon°veis",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "para instalaá∆o.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para selecionar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "um item na lista.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "\x07  Para escolher o item selecionado, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "\x07  Para cancelar a alteraá∆o, pressione ESC.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continuar  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador est† preparando o computador para copiar os arquivos",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "do ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Montando a lista de arquivos a serem copiados...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        16,
        "Selecione um sistema de arquivos para a nova partiá∆o na lista abaixo.",
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
        "Se desejar selecionar uma partiá∆o diferente, pressione ESC.",
        0
    },
    {
        0,
        0,
        "ENTER=Continuar  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Vocà solicitou a exclus∆o da partiá∆o",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Para excluir esta partiá∆o, pressione D",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "CUIDADO: todos os dados da partiá∆o ser∆o perdidos!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Para retornar Ö tela anterior sem excluir",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        22,
        "a partiá∆o, pressione ESC.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D=Excluir  ESC=Cancelar  F3=Sair",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " Instalaá∆o do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador est† atualizando a configuraá∆o do sistema.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Criando a estrutura de registro...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        "O ReactOS n∆o est† completamente instalado no computador.\n"
        "Se vocà sair da instalaá∆o agora, precisar† executa-la\n"
        "novamente para instalar o ReactOS.\n"
        "\n"
        "  \x07  Para continuar a instalaá∆o, pressione ENTER.\n"
        "  \x07  Para sair da instalaá∆o, pressione F3.",
        "F3=Sair  ENTER=Continuar"
    },
    {
        // ERROR_NO_HDD
        "N∆o foi poss°vel localizar um disco r°digo.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "N∆o foi poss°vel localizar a unidade de origem.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "N∆o foi poss°vel carregar o arquivo TXTSETUP.SIF.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "O arquivos TXTSETUP.SIF est† corrompido.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "O arquivo TXTSETUP.SIF est† com a assinatura incorreta.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "N∆o foi poss°vel obter as informaá‰es sobre o disco do sistema.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_WRITE_BOOT,
        "Erro ao escrever o c¢digo de inicializaá∆o %S na partiá∆o do sistema.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "N∆o foi poss°vel carregar a lista de tipos de computadores.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "N∆o foi poss°vel carregar a lista de tipos de v°deo.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "N∆o foi poss°vel carregar a lista de tipos de teclado.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "N∆o foi poss°vel carregar a lista de leiautes de teclado.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_WARN_PARTITION,
        "O instalador encontrou uma tabela de partiá∆o incompat°vel\n"
        "que n∆o pode ser utilizada corretamente!\n"
        "\n"
        "Criar ou excluir partiá‰es pode destruir a tabela de partiá∆o.\n"
        "\n"
        "  \x07  Para sair da instalaá∆o, pressione F3.\n"
        "  \x07  Para continuar, pressione ENTER.",
        "F3=Sair  ENTER=Continuar"
    },
    {
        // ERROR_NEW_PARTITION,
        "Vocà n∆o pode criar uma partiá∆o dentro de\n"
        "outra partiá∆o j† existente!\n"
        "\n"
        "  * Pressione qualquer tecla para continuar.",
        NULL
    },
    {
        // ERROR_DELETE_SPACE,
        "Vocà n∆o pode excluir um espaáo n∆o-particionado!\n"
        "\n"
        "  * Pressione qualquer tecla para continuar.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Erro ao instalar o c¢digo de inicializaá∆o %S na partiá∆o do sistema.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_NO_FLOPPY,
        "N∆o h† disco na unidade A:.",
        "ENTER=Continuar"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "N∆o foi poss°vel atualizar a configuraá∆o de leiaute de teclado.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "N∆o foi poss°vel atualizar a configuraá∆o de v°deo.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_IMPORT_HIVE,
        "N∆o foi poss°vel importar o arquivo de estrutura.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_FIND_REGISTRY
        "N∆o foi poss°vel encontrar os arquivos do registro.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CREATE_HIVE,
        "N∆o foi poss°vel criar as estruturas do registro.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "N∆o foi poss°vel inicializar o registro.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "O arquivo cab n∆o contÇm um arquivo inf v†lido.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CABINET_MISSING,
        "N∆o foi poss°vel econtrar o arquivo cab.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "O arquivo cab n∆o contÇm um script de instalaá∆o.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_COPY_QUEUE,
        "N∆o foi poss°vel abrir a lista de arquivos para c¢pia.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CREATE_DIR,
        "N∆o foi poss°vel criar os diret¢rios de instalaá∆o.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "N∆o foi poss°vel encontrar a seá∆o '%S' no\n"
        "arquivo TXTSETUP.SIF.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CABINET_SECTION,
        "N∆o foi poss°vel encontrar a seá∆o '%S' no\n"
        "arquivo cab.\n",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "N∆o foi poss°vel criar o diret¢rio de instalaá∆o.",
        "ENTER=Reiniciar"
    },
    {
        // ERROR_WRITE_PTABLE,
        "N∆o foi poss°vel escrever a tabela de partiá‰es.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "N∆o foi poss°vel adicionar o c¢digo de localidade no registro.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "N∆o foi poss°vel configurar o idioma do sistema.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "N∆o foi poss°vel adicionar o leiaute do teclado no registro.\n"
        "ENTER=Reiniciar"
    },
    {
        // ERROR_UPDATE_GEOID,
        "N∆o foi poss°vel configurar a identificaá∆o geogr†fica.\n"
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
        "You can not create a new primary or extended partition in the\n"
        "partition table of this disk because the partition table is full.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "You can not create more than one extended partition per disk.\n"
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
        BOOT_LOADER_PAGE,
        ptBRBootLoaderEntries
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
        BOOT_LOADER_FLOPPY_PAGE,
        ptBRBootPageEntries
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
     "   ENTER = Install   P = Create Primary   E = Create Extended   F3 = Quit"},
//    "   ENTER=Instalar  C=Criar partiá∆o  F3=Sair"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   L = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
    "   ENTER=Instalar  D=Apagar partiá∆o  F3=Sair"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
    "Tamanho da nova partiá∆o:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a primary partition on"},
//    "Vocà solicitou a criaá∆o de uma nova partiá∆o em"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDDSIZE,
    "Por favor, insira o tamanho da nova partiá∆o em megabytes (MB)."},
    {STRING_CREATEPARTITION,
    "   ENTER=Criar partiá∆o  ESC=Cancelar  F3=Sair"},
    {STRING_PARTFORMAT,
    "Esta partiá∆o ser† formatada logo em seguida."},
    {STRING_NONFORMATTEDPART,
    "Vocà solicitou instalar o ReactOS em uma partiá∆o nova ou sem formato."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "O instalador instala o ReactOS na partiá∆o"},
    {STRING_CHECKINGPART,
    "O instalador est† verificando a partiá∆o selecionada."},
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
    "O instalador est† copiando os arquivos..."},
    {STRING_REGHIVEUPDATE,
    "   Atualizando a estrutura do registro..."},
    {STRING_IMPORTFILE,
    "   Importando %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Atualizando as configuraá‰es de v°deo..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Atualizando as configuraá‰es regionais..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Atualizando as configuraá‰es de leiaute do teclado..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Adicionando as informaá‰es de localidade no registro..."},
    {STRING_DONE,
    "   Pronto..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER=Reiniciar"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "N∆o foi poss°vel abrir o console\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "A causa mais com£m Ç a utilizaá∆o de um teclado USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Os teclados USB ainda n∆o s∆o completamente suportados\r\n"},
    {STRING_FORMATTINGDISK,
    "O instalador est† formatando o disco"},
    {STRING_CHECKINGDISK,
    "O instalador est† verificando o disco"},
    {STRING_FORMATDISK1,
    " Formatar a partiá∆o utilizando o sistema de arquivos %S (R†pido) "},
    {STRING_FORMATDISK2,
    " Formatar a partiá∆o utilizando o sistema de arquivos %S "},
    {STRING_KEEPFORMAT,
    " Manter o sistema de arquivos atual (sem alteraá‰es) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu) em %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Tipo 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "em %I64u %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu) em %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "em %I64u %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "Disco %lu (%I64u %s), Porta=%hu, Barramento=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  Tipo 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "em Disco %lu (%I64u %s), Porta=%hu, Barramento=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sTipo %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu) em %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "O instalador criou uma nova partiá∆o em"},
    {STRING_UNPSPACE,
    "    %sEspaáo n∆o particionado%s            %6lu %s"},
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
