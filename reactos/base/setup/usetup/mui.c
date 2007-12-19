#include "usetup.h"
#include "errorcode.h"
#include "mui.h"

#include "lang/en-US.h"
#include "lang/de-DE.h"
#include "lang/es-ES.h"
#include "lang/fr-FR.h"
#include "lang/it-IT.h"
#include "lang/ru-RU.h"
#include "lang/sv-SE.h"
#include "lang/uk-UA.h"

static MUI_LANGUAGE LanguageList[] =
{
    {
        "English (USA)",
        enUSPages
    },
    {
        "French (France)",
        frFRPages
    },
    {
        "German",
        deDEPages
    },
    {
        "Italian",
        itITPages
    },
    {
        "Russian",
        ruRUPages
    },
    {
        "Spanish",
        esESPages
    },
    {
        "Swedish",
        svSEPages
    },
    {
        "Ukrainian",
        ukUAPages
    },
    {
        NULL,
        NULL
    }
};

static ULONG SelectedLanguage = 0;

extern
VOID
PopupError(PCHAR Text,
	   PCHAR Status,
	   PINPUT_RECORD Ir,
	   ULONG WaitEvent);


PGENERIC_LIST
MUICreateLanguageList()
{
    PGENERIC_LIST List;
    ULONG Index;

    List = CreateGenericList();
    if (List == NULL)
    {
        return NULL;
    }

    Index = 0;

    do
    {
        AppendGenericListEntry(List, LanguageList[Index].LanguageDescriptor, (PVOID)Index, (Index == 0 ? TRUE : FALSE));
        Index++;
    }while(LanguageList[Index].MuiPages && LanguageList[Index].LanguageDescriptor);

    return List;
}

BOOLEAN
MUISelectLanguage(ULONG LanguageIndex)
{
    SelectedLanguage = LanguageIndex;
    return TRUE;
}


static
MUI_ENTRY *
findMUIEntriesOfPage(ULONG PageNumber, MUI_PAGE * Pages)
{
    ULONG Index = 0;
    do
    {
        if (Pages[Index].Number == PageNumber)
        {
            return Pages[Index].MuiEntry;
        }
        Index++;
    }while(Pages[Index].MuiEntry != NULL);
    return NULL;
}

VOID
MUIDisplayPage(ULONG pg)
{
    MUI_ENTRY * entry;
    int index;
    int flags;

    entry = findMUIEntriesOfPage(pg, LanguageList[SelectedLanguage].MuiPages);
    if (!entry)
    {
        PopupError("Error: Failed to find translated page",
                   NULL,
                   NULL,
                   POPUP_WAIT_NONE);
        return;
    }

    index = 0;
    do
    {
        flags = entry[index].Flags;
        switch(flags)
        {
            case TEXT_NORMAL:
                CONSOLE_SetTextXY(entry[index].X, entry[index].Y, entry[index].Buffer);
                break;
            case TEXT_HIGHLIGHT:
                CONSOLE_SetHighlightedTextXY(entry[index].X, entry[index].Y, entry[index].Buffer);
                break;
            case TEXT_UNDERLINE:
                CONSOLE_SetUnderlinedTextXY(entry[index].X, entry[index].Y, entry[index].Buffer);
                break;
            case TEXT_STATUS:
                  CONSOLE_SetStatusText(entry[index].Buffer);
                  break;
            default:
                break;
        }
        index++;
    }while(entry[index].Buffer != NULL);
}

VOID
MUIDisplayError(ULONG ErrorNum, PINPUT_RECORD Ir, ULONG WaitEvent)
{
    if (ErrorNum >= ERROR_LAST_ERROR_CODE)
    {
        PopupError("invalid error number provided",
                    "press enter to continue",
                    Ir,
                    POPUP_WAIT_ENTER);

        return;
    }

    PopupError(enUSErrorEntries[ErrorNum].ErrorText,
               enUSErrorEntries[ErrorNum].ErrorStatus,
               Ir,
               WaitEvent);
}
