#pragma once

MUI_LAYOUTS ptBRLayouts[] =
{
    { L"0416", L"00000416" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY ptBRLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Seleção do idioma",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Por favor, selecione o idioma a ser utilizado durante a instalação.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Então pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  O idioma selecionado também será o idioma padrão do sistema.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Bem-vindo à instalação do ReactOS.",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Esta parte da instalação prepara o ReactOS para ser",
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
        "\x07  Para instalar o ReactOS agora, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Para reparar uma instalação do ReactOS, pressione R.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Para ver os termos e condições da licença, pressione L.",
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
        "Para maiores informações sobre o ReactOS, visite o sítio:",
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
        "ENTER=Continuar  R=Reparar  L=Licença  F3=Sair",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador do ReactOS está em fase inicial de desenvolvimento e",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ainda não suporta todas as funções de instalação.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "As seguintes limitações se aplicam:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- O instalador não suporta mais de uma partição primária por disco.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- O instalador não pode excluir uma partição primária de um disco",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  se houverem partições estendidas no mesmo disco.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- O instalador não pode remover a primeira partição estendida de um",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  disco se existirem outras partições estendidas no mesmo disco.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- O instalador suporta somente o sistema de arquivos FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- O verificador de integridade de sistema de arquivos ainda não está",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "  implementado.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Para continuar a instalação do ReactOS, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        27,
        "\x07  Para sair sem instalar o ReactOS, pressione F3.",
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

static MUI_ENTRY ptBRLicensePageEntries[] =
{
    {
        4,
        3,
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licença:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "O ReactOS está licenciado sob os termos da licença",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL contendo partes de código licenciados sob outras",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenças compatíveis, como X11 ou BSD e GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Todo o software que faz parte do ReactOS é portanto, liberado",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "sob a licença GNU GPL, bem como a manutenção da licença",
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
        "Este software vem sem NENHUMA GARANTIA ou restrição de uso",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "exceto onde leis locais e internacionais são aplicaveis. A licença",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "do ReactOS abrange apenas a distribuição a terceiros.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Se por alguma razão você não recebeu uma cópia da licença",
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
        "Este é um software livre; veja o código fonte para condições de cópia.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "NÃO há garantia; nem mesmo para COMERCIALIZAÇÃO ou",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "ADEQUAÇÃO PARA UM PROPÓSITO PARTICULAR",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A lista a seguir mostra as configurações de dispositivos atual.",
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
        "Vídeo:",
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
        16, "Aceitar essas configurações de dispositivo",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Use as teclas SETA PARA CIMA e SETA PARA BAIXO para mudar de opção.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "Para escolher uma configuração alternativa, pressione ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        22,
        "Quanto finalizar os ajustes, selecione \"Aceitar essas configurações",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador do ReactOS está em fase inicial de desenvolvimento e",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ainda não suporta todas as funções de instalação.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "As funções reparação ainda não foram implementadas.",
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
        "\x07  Para abrir o console de recuperação, pressione R.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Para voltar a página principal, pressione ESC.",
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
        "ESC=Página principal  U=Atualizar  R=Recuperar  ENTER=Reiniciar",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de computadores disponíveis",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "para instalação.",
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
        "\x07  Para cancelar a alteração, pressione ESC.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "O sistema está agora certificando que todos os dados estejam sendo",
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
        "Esta operação pode demorar um minuto.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Quando terminar, o computador será reiniciado automaticamente.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "O ReactOS não foi totalmente instalado neste computador.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de vídeo disponíveis para instalação.",
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
        "\x07  Para cancelar a alteração, pressione ESC.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Os componentes básicos do ReactOS foram instalados com sucesso.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador não pôde instalar o gerênciador de inicialização no disco",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "rígido do computador.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        7,
        "A lista a seguir mostra as partições existentes e os espaços",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        8,
        "não-particionados neste computador.",
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
        "\x07  Para criar uma partição no espaço não particionado, pressione C.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Para excluir a partição selecionada, pressione D.",
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

static MUI_ENTRY ptBRFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Formatar partição",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "O instalador irá formatar a partição. Para continuar, pressione ENTER.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador irá copiar os arquivos para a partição selecionada.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Selecione um diretório onde deseja que o ReactOS seja instalado:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Para mudar o diretório sugerido, pressione a tecla BACKSPACE para apagar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "o texto e escreva o nome do diretório onde deseja que o ReactOS",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
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
        "arquivos do ReactOS para a pasta de instalação.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Esta operação pode demorar alguns minutos.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador irá configurar o gerênciador de inicialização",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Instalar o gerênciador de inic. no disco rígido (MBR e VBR)",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Instalar o gerênciador de inic. no disco rígido (apenas VBR)",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Instalar o gerênciador de inicialização em um disquete",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Pular a instalação do gerênciador de inicialização",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de teclados disponíveis para instalação.",
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
        "\x07  Para cancelar a alteração, pressione ESC.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A lista a seguir mostra os tipos de leiautes de teclado disponíveis",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "para instalação.",
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
        "\x07  Para cancelar a alteração, pressione ESC.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador está preparando o computador para copiar os arquivos",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        16,
        "Selecione um sistema de arquivos para a nova partição na lista abaixo.",
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
        "Se desejar selecionar uma partição diferente, pressione ESC.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Você solicitou a exclusão da partição",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Para excluir esta partição, pressione D",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "CUIDADO: todos os dados da partição serão perdidos!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Para retornar à tela anterior sem excluir",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        22,
        "a partição, pressione ESC.",
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
        " Instalação do ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "O instalador está atualizando a configuração do sistema.",
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
        //ERROR_NOT_INSTALLED
        "O ReactOS não está completamente instalado no computador.\n"
        "Se você sair da instalação agora, precisará executa-la\n"
        "novamente para instalar o ReactOS.\n"
        "\n"
        "  \x07  Para continuar a instalação, pressione ENTER.\n"
        "  \x07  Para sair da instalação, pressione F3.",
        "F3=Sair  ENTER=Continuar"
    },
    {
        //ERROR_NO_HDD
        "Não foi possível localizar um disco rídigo.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Não foi possível localizar a unidade de origem.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Não foi possível carregar o arquivo TXTSETUP.SIF.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "O arquivos TXTSETUP.SIF está corrompido.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "O arquivo TXTSETUP.SIF está com a assinatura incorreta.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Não foi possível obter as informações sobre o disco do sistema.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_WRITE_BOOT,
        "Erro ao escrever o código de inicialização na partição do sistema.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Não foi possível carregar a lista de tipos de computadores.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Não foi possível carregar a lista de tipos de vídeo.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Não foi possível carregar a lista de tipos de teclado.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Não foi possível carregar a lista de leiautes de teclado.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_WARN_PARTITION,
        "O instalador encontrou uma tabela de partição incompatível\n"
        "que não pode ser utilizada corretamente!\n"
        "\n"
        "Criar ou excluir partições pode destruir a tabela de partição.\n"
        "\n"
        "  \x07  Para sair da instalação, pressione F3.\n"
        "  \x07  Para continuar, pressione ENTER.",
        "F3=Sair  ENTER=Continuar"
    },
    {
        //ERROR_NEW_PARTITION,
        "Você não pode criar uma partição dentro de\n"
        "outra partição já existente!\n"
        "\n"
        "  * Pressione qualquer tecla para continuar.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Você não pode excluir um espaço não-particionado!\n"
        "\n"
        "  * Pressione qualquer tecla para continuar.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Erro ao instalar o código de inicialização na partição do sistema.",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_NO_FLOPPY,
        "Não há disco na unidade A:.",
        "ENTER=Continuar"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Não foi possível atualizar a configuração de leiaute de teclado.",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Não foi possível atualizar a configuração de vídeo.",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Não foi possível importar o arquivo de estrutura.",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_FIND_REGISTRY
        "Não foi possível encontrar os arquivos do registro.",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_CREATE_HIVE,
        "Não foi possível criar as estruturas do registro.",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Não foi possível inicializar o registro.",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "O arquivo cab não contém um arquivo inf válido.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_CABINET_MISSING,
        "Não foi possível econtrar o arquivo cab.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "O arquivo cab não contém um script de instalação.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_COPY_QUEUE,
        "Não foi possível abrir a lista de arquivos para cópia.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_CREATE_DIR,
        "Não foi possível criar os diretórios de instalação.",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Não foi possível encontrar a seção 'Directories' no\n"
        "arquivo TXTSETUP.SIF.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_CABINET_SECTION,
        "Não foi possível encontrar a seção 'Directories' no\n"
        "arquivo cab.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Não foi possível criar o diretório de instalação.",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Não foi possível encontrar a seção 'SetupData' no\n"
        "arquivo TXTSETUP.SIF.\n",
        "ENTER=Reiniciar"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Não foi possível escrever a tabela de partições.\n"
        "ENTER=Reiniciar"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Não foi possível adicionar o código de localidade no registro.\n"
        "ENTER=Reiniciar"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Não foi possível configurar o idioma do sistema.\n"
        "ENTER=Reiniciar"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Não foi possível adicionar o leiaute do teclado no registro.\n"
        "ENTER=Reiniciar"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Não foi possível configurar a identificação geográfica.\n"
        "ENTER=Reiniciar"
    },
    {
        //ERROR_INSUFFICIENT_DISKSPACE,
        "Não há espaço suficiente na partição selecionada.\n"
        "  * Pressione qualquer tecla para continuar.",
        NULL
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
        START_PAGE,
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
    "   ENTER=Instalar  C=Criar partição  F3=Sair"},
    {STRING_INSTALLDELETEPARTITION,
    "   ENTER=Instalar  D=Apagar partição  F3=Sair"},
    {STRING_PARTITIONSIZE,
    "Tamanho da nova partição:"},
    {STRING_CHOOSENEWPARTITION,
    "Você solicitou a criação de uma nova partição em"},
    {STRING_HDDSIZE,
    "Por favor, insira o tamanho da nova partição em megabytes (MB)."},
    {STRING_CREATEPARTITION,
    "   ENTER=Criar partição  ESC=Cancelar  F3=Sair"},
    {STRING_PARTFORMAT,
    "Esta partição será formatada logo em seguida."},
    {STRING_NONFORMATTEDPART,
    "Você solicitou instalar o ReactOS em uma partição nova ou sem formato."},
    {STRING_INSTALLONPART,
    "O instalador instala o ReactOS na partição"},
    {STRING_CHECKINGPART,
    "O instalador está verificando a partição selecionada."},
    {STRING_QUITCONTINUE,
    "F3=Sair  ENTER=Continuar"},
    {STRING_REBOOTCOMPUTER,
    "ENTER=Reiniciar"},
    {STRING_TXTSETUPFAILED,
    "Não foi possível econtrar a seção '%S' no\narquivo TXTSETUP.SIF.\n"},
    {STRING_COPYING,
    "   Copiando arquivo: %S"},
    {STRING_SETUPCOPYINGFILES,
    "O instalador está copiando os arquivos..."},
    {STRING_REGHIVEUPDATE,
    "   Atualizando a estrutura do registro..."},
    {STRING_IMPORTFILE,
    "   Importando %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Atualizando as configurações de vídeo..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Atualizando as configurações regionais..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Atualizando as configurações de leiaute do teclado..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Adicionando as informações de localidade no registro..."},
    {STRING_DONE,
    "   Pronto..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER=Reiniciar"},
    {STRING_CONSOLEFAIL1,
    "Não foi possível abrir o console\n\n"},
    {STRING_CONSOLEFAIL2,
    "A causa mais comúm é a utilização de um teclado USB\n"},
    {STRING_CONSOLEFAIL3,
    "Os teclados USB ainda não são completamente suportados\n"},
    {STRING_FORMATTINGDISK,
    "O instalador está formatando o disco"},
    {STRING_CHECKINGDISK,
    "O instalador está verificando o disco"},
    {STRING_FORMATDISK1,
    " Formatar a partição utilizando o sistema de arquivos %S (Rápido) "},
    {STRING_FORMATDISK2,
    " Formatar a partição utilizando o sistema de arquivos %S "},
    {STRING_KEEPFORMAT,
    " Manter o sistema de arquivos atual (sem alterações) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu) em %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Tipo %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "em %I64u %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu) em %wZ."},
    {STRING_HDDINFOUNK3,
    "em %I64u %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Disco %lu (%I64u %s), Porta=%hu, Barramento=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Tipo %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "em Disco %lu (%I64u %s), Porta=%hu, Barramento=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  Tipo %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu) em %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Disco %lu  (Porta=%hu, Barramento=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "O instalador criou uma nova partição em"},
    {STRING_UNPSPACE,
    "    Espaço não particionado              %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
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
