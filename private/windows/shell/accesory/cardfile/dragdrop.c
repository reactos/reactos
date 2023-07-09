#include "precomp.h"

#define CBPATHMAX       256

TCHAR    szPackageClass[] = TEXT("Package");

void DoDragDrop(HWND hwnd, HANDLE hdrop, BOOL fCard)
{
    TCHAR szDropFile[CBPATHMAX+1];      /* Path */
    int fLink = FALSE;

    /* Retrieve the file name */
    DragQueryFile(hdrop, 0, szDropFile, CBPATHMAX);
    DragFinish(hdrop);

#if 0
/* Bug 10904:  Nope, don't restore first.  21 August 1991  Clark R. Cyr */
    /* If iconized, restore ourselves first. */
        if (IsIconic(hIndexWnd))
        SendMessage(hIndexWnd,WM_SYSCOMMAND,SC_RESTORE,0);
#endif
    /* We got dropped on, so bring ourselves to the top */
    BringWindowToTop(hIndexWnd);

    if (!fCard) /* did not get dropped on the card, open the file */
    {
        if (!MaybeSaveFile(FALSE))
            return;
        OpenNewFile(szDropFile);
        return;
    }

    /*
     * Ctrl+Shift, no Alt   => link the object
     * no modifiers         => embed the object
     * anything else        => NOP
     */
    if ((GetKeyState(VK_SHIFT) < 0) && (GetKeyState(VK_CONTROL) < 0) &&
        !(GetKeyState(VK_MENU) < 0))
        fLink = TRUE;
    else if (!(GetKeyState(VK_SHIFT) < 0) && !(GetKeyState(VK_CONTROL) < 0) &&
        !(GetKeyState(VK_MENU) < 0))
        fLink = FALSE;
    else
        return;

    if (fOLE && fCard)
        PicCreateFromFile(szPackageClass, szDropFile, fLink);
}
