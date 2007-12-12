#include "usetup.h"
#include "mui.h"

#include "lang/en-US.h"

static MUI_LANGUAGE lang[] =
{
    {
        "English (USA)",
        enUSPages
    },
    {
        NULL,
        NULL
    }
};

static unsigned sel_lang = 0;

extern
VOID
PopupError(PCHAR Text,
	   PCHAR Status,
	   PINPUT_RECORD Ir,
	   ULONG WaitEvent);


static
MUI_ENTRY *
findMUIEntriesOfPage(int pg, MUI_PAGE * pages)
{
    int index = 0;
    do
    {
        if (pages[index].Number == pg)
        {
            return pages[index].MuiEntry;
        }
        index++;
    }while(pages[index].MuiEntry != NULL);
    return NULL;
}

void MUIDisplayPage(int pg)
{
    MUI_ENTRY * entry;
    int index;
    int flags;

    entry = findMUIEntriesOfPage(pg, lang[sel_lang].MuiPages);
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
