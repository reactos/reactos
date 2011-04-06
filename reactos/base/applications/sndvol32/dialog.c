/* $Id: misc.c 43790 2009-10-27 10:34:16Z dgorbachev $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sound Volume Control
 * FILE:        subsys/system/sndvol32/dialog.c
 * PROGRAMMERS: Johannes Anderwald
 */
#include "sndvol32.h"


#define XLEFT (30)
#define XTOP (20)
#define DIALOG_VOLUME_SIZE (150)

LPVOID
LoadDialogResource(
    IN HMODULE hModule,
    IN LPCWSTR ResourceName,
    OUT LPDWORD ResourceLength)
{
    HRSRC hSrc;
    HGLOBAL hRes;
    PVOID Result;

    /* find resource */
    hSrc = FindResourceW(hModule, ResourceName, (LPCWSTR)RT_DIALOG);

    if (!hSrc)
    {
        /* failed to find resource */
        return NULL;
    }

    /* now load the resource */
    hRes = LoadResource(hAppInstance, hSrc);
    if (!hRes)
    {
        /* failed to load resource */
        return NULL;
    }

    /* now lock the resource */
    Result = LockResource(hRes);

    if (!Result)
    {
        /* failed to lock resource */
        return NULL;
    }

    if (ResourceLength)
    {
        /* store output length */
        *ResourceLength = SizeofResource(hAppInstance, hSrc);
    }

    /* done */
    return Result;
}

LPWORD
AddDialogControl(
    IN HWND hwndDialog,
    IN HWND * OutWnd,
    IN LPRECT DialogOffset,
    IN PDLGITEMTEMPLATE DialogItem,
    IN DWORD DialogIdMultiplier,
    IN HFONT hFont)
{
    RECT rect;
    LPWORD Offset;
    LPWSTR ClassName, WindowName = NULL;
    HWND hwnd;
    DWORD wID;

    /* initialize client rectangle */
    rect.left = DialogItem->x + DialogOffset->left;
    rect.top = DialogItem->y + DialogOffset->top;
    rect.right = DialogItem->cx;
    rect.bottom = DialogItem->cy;

    //MapDialogRect(hwndDialog, &rect);

    /* move offset after dialog item */
    Offset = (LPWORD)(DialogItem + 1);

    if (*Offset == 0xFFFF)
    {
        /* class is encoded as type */
        Offset++;

        /* get control type */
        switch(*Offset)
        {
            case 0x80:
                ClassName = L"button";
                WindowName = (LPWSTR)(Offset + 1);
                break ;
            case 0x82:
                ClassName = L"static";
                WindowName = (LPWSTR)(Offset + 1);
                break;
            default:
               /* FIXME */
               assert(0);
               ClassName = 0;
        }
    }
    else
    {
        /* class name is encoded as string */
        ClassName = (LPWSTR)Offset;

        /* adjust offset */
        Offset += wcslen(ClassName) + 1;

        /* get offset */
        WindowName = (LPWSTR)(Offset + 1);
    }

    if (DialogItem->id == MAXWORD)
    {
        /* id is not important */
        wID = DialogItem->id;
    }
    else
    {
        /* calculate id */
        wID = DialogItem->id * (DialogIdMultiplier + 1);

    }
    /* now create the window */
    hwnd = CreateWindowExW(DialogItem->dwExtendedStyle, 
                           ClassName, 
                           WindowName, 
                           DialogItem->style, 
                           rect.left, 
                           rect.top,
                           rect.right,
                           rect.bottom,
                           hwndDialog,
                           (HMENU)(wID),
                           hAppInstance,
                           NULL);

    /* sanity check */
    assert(hwnd);

    /* store window */
    *OutWnd = hwnd;

    /* check if this the track bar */
    if (!wcsicmp(ClassName, L"msctls_trackbar32"))
    {
        /* set up range */
        SendMessage(hwnd, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(0, 5));

        /* set up page size */
        SendMessage(hwnd, TBM_SETPAGESIZE, 0, (LPARAM) 1);

        /* set available range */
        SendMessage(hwnd, TBM_SETSEL, (WPARAM) FALSE, (LPARAM) MAKELONG(0, 5)); 
    }
    else if (!wcsicmp(ClassName, L"static") || !wcsicmp(ClassName, L"button"))
    {
        /* set font */
        SendMessageW(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
    }

    //ShowWindow(hwnd, SW_SHOWNORMAL);

    if (WindowName != NULL)
    {
        /* position offset to start of name */
        Offset++;

        /* move offset past name */
        Offset += wcslen((LPWSTR)Offset) + 1;
    }
    else
    {
        /* no name so just adjust offset */
        Offset++;
    }

    /* check if there is additional data */ 
    if (*Offset == 0)
    {
        /* no additional data */
        Offset++;
    }
    else
    {
        /* add data offset */
        Offset += *Offset;
    }

    /* make sure next template is word-aligned */
    Offset = (LPWORD)(((ULONG_PTR)Offset + 3) & ~3);

    /* done */
    return Offset;
}

VOID
LoadDialogControls(
    IN PMIXER_WINDOW MixerWindow,
    LPRECT DialogOffset,
    LPVOID DlgResource,
    DWORD DialogIdMultiplier)
{
    LPDLGTEMPLATE DialogHeader;
    PDLGITEMTEMPLATE DialogItem;
    LPWORD Offset;
    WORD FontSize;
    WCHAR FontName[100];
    WORD Length, Index;
    HFONT Font;

    /* get dialog header */
    DialogHeader = (LPDLGTEMPLATE)DlgResource;

    /* sanity check */
    assert(DialogHeader->cdit);

    if (MixerWindow->Window)
        MixerWindow->Window = (HWND*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MixerWindow->Window, (MixerWindow->WindowCount + DialogHeader->cdit) * sizeof(HWND));
    else
        MixerWindow->Window = (HWND*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, DialogHeader->cdit * sizeof(HWND));
    if (!MixerWindow->Window)
    {
        /* no memory */
        return;
    }

    /* now walk past the dialog header */
    Offset = (LPWORD)(DialogHeader + 1);

    /* FIXME: support menu */
    assert(*Offset == 0);
    Offset++;

    /* FIXME: support classes */
    assert(*Offset == 0);
    Offset++;

    /* FIXME: support titles */
    assert(*Offset == 0);
    Offset++;

    /* get font size */
    FontSize = *Offset;
    Offset++;

    /* calculate font length */
    Length = wcslen((LPWSTR)Offset) + 1;
    assert(Length < (sizeof(FontName) / sizeof(WCHAR)));

    /* copy font */
    wcscpy(FontName, (LPWSTR)Offset);

    Font = CreateFontW(FontSize+8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, FontName);
    assert(Font);

    /* move offset after font name */
    Offset += Length;

    /* offset is now at first dialog item control */
    DialogItem = (PDLGITEMTEMPLATE)Offset;

    /* enumerate now all controls */
    for(Index = 0; Index < DialogHeader->cdit; Index++)
    {
        /* add controls */
        Offset = AddDialogControl(MixerWindow->hWnd, &MixerWindow->Window[MixerWindow->WindowCount], DialogOffset, DialogItem, DialogIdMultiplier, Font);

        /* sanity check */
        assert(Offset);

        /* move dialog item to new offset */
        DialogItem =(PDLGITEMTEMPLATE)Offset;

        /* increment window count */
        MixerWindow->WindowCount++;
    }
}

VOID
LoadDialog(
    IN HMODULE hModule,
    IN PMIXER_WINDOW MixerWindow,
    IN LPCWSTR DialogResId,
    IN DWORD Index)
{
    LPVOID DlgResource;
    RECT rect;

    /* first load the dialog resource */
    DlgResource = LoadDialogResource(hModule, DialogResId, NULL);

    if (!DlgResource)
    {
        /* failed to load resource */
        return;
    }

    /* get window size */
    GetClientRect(MixerWindow->hWnd, &rect);

    /* adjust client position */
    rect.left += (Index * DIALOG_VOLUME_SIZE);


    /* now add the controls */
    LoadDialogControls(MixerWindow, &rect, DlgResource, Index);

}

BOOL
CALLBACK
EnumConnectionsCallback(
    PSND_MIXER Mixer, 
    DWORD LineID, 
    LPMIXERLINE Line,
    PVOID Context)
{
    WCHAR LineName[MIXER_LONG_NAME_CHARS];
    DWORD Flags;
    DWORD wID;
    RECT rect;
    PPREFERENCES_CONTEXT PrefContext = (PPREFERENCES_CONTEXT)Context;

    if (Line->cControls != 0)
    {
      /* get line name */
      if (SndMixerGetLineName(PrefContext->Mixer, PrefContext->SelectedLine, LineName, MIXER_LONG_NAME_CHARS, FALSE) == -1)
      {
          /* failed to get line name */
          LineName[0] = L'\0';
      }

      /* check if line is found in registry settings */
      if (ReadLineConfig(PrefContext->DeviceName,
                         LineName,
                         Line->szName,
                         &Flags))
      {
          /* is it selected */
          if (Flags != 0x4)
          {
              /* load dialog resource */
              LoadDialog(hAppInstance, PrefContext->MixerWindow, MAKEINTRESOURCE(IDD_VOLUME_CTRL), PrefContext->Count);

              /* get id */
              wID = (PrefContext->Count + 1) * IDC_LINE_NAME;

              /* set line name */
              SetDlgItemTextW(PrefContext->MixerWindow->hWnd, wID, Line->szName);

              /* increment dialog count */
              PrefContext->Count++;

              /* get application rectangle */
              GetWindowRect(PrefContext->MixerWindow->hWnd, &rect);

              /* now move the window */
              MoveWindow(PrefContext->MixerWindow->hWnd, rect.left, rect.top, (PrefContext->Count * DIALOG_VOLUME_SIZE), rect.bottom, TRUE);

          }
      }
    }
    return TRUE;
}

VOID
LoadDialogCtrls(
    PPREFERENCES_CONTEXT PrefContext)
{
    /* set dialog count to one */
    PrefContext->Count = 0;

    /* enumerate controls */
    SndMixerEnumConnections(PrefContext->Mixer, PrefContext->SelectedLine, EnumConnectionsCallback, (PVOID)PrefContext);
}
