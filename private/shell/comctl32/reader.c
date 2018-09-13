
#include "ctlspriv.h"

static struct {
    WPARAM vk1;
    WPARAM vk2;
    int dx;
    int dy;
} arrNumMaps[] = 
{ 
    { VK_NUMPAD1, VK_END,   -RM_SCROLLUNIT, RM_SCROLLUNIT,},
    { VK_NUMPAD2, VK_DOWN,  0,              RM_SCROLLUNIT},
    { VK_NUMPAD3, VK_NEXT,  RM_SCROLLUNIT,  RM_SCROLLUNIT},
    { VK_NUMPAD4, VK_LEFT,  -RM_SCROLLUNIT, 0},
    { VK_NUMPAD5, VK_CLEAR, 0,              0},
    { VK_NUMPAD6, VK_RIGHT, RM_SCROLLUNIT,  0},
    { VK_NUMPAD7, VK_HOME,  -RM_SCROLLUNIT, -RM_SCROLLUNIT},
    { VK_NUMPAD8, VK_UP,    0,              -RM_SCROLLUNIT},
    { VK_NUMPAD9, VK_PRIOR, RM_SCROLLUNIT,  -RM_SCROLLUNIT},
};

// do some keyboard handling...
// this works like USER's arrow keys for resizing 
// bugbug, diagonals don't work right now
void RM_HandleKeyDown(LPRECT prcHot, WPARAM wParam, LPARAM lParam)
{
    int i;
    POINT pt;
    
    GetCursorPos(&pt);
    
    for (i = ARRAYSIZE(arrNumMaps) - 1 ; i >= 0; i--) {
        if (wParam == arrNumMaps[i].vk1 || 
            wParam == arrNumMaps[i].vk2) {
            break;
        }
    }
    
    if (i == -1) {
        ReleaseCapture();
        return;
    }

    // this deals with if the cursor is within the bounds of the rect
    if (pt.x < prcHot->right &&
        pt.x >= prcHot->left && 
        arrNumMaps[i].dx) {
        
        if (arrNumMaps[i].dx > 0)
            pt.x = prcHot->right - 2;
        else 
            pt.x = prcHot->left + 1;
        
    }
    
    if (pt.y < prcHot->bottom &&
        pt.y >= prcHot->top && 
        arrNumMaps[i].dy) {
        
        if (arrNumMaps[i].dy > 0)
            pt.y = prcHot->bottom - 2;
        else 
            pt.y = prcHot->top + 1;
        
    }
    
    pt.x += arrNumMaps[i].dx;
    pt.y += arrNumMaps[i].dy;

    if (!arrNumMaps[i].dx && !arrNumMaps[i].dy) {
        // special case this for centering
        pt.x = (prcHot->right + prcHot->left) / 2;
        pt.y = (prcHot->top + prcHot->bottom) / 2;
    }

    // all we do is move the cursor.. the RM_CheckScroll will do the actual
    // scrolling for us.
    SetCursorPos(pt.x, pt.y);
}

void RM_GetScrollXY(PREADERMODEINFO prmi, LPRECT prcHot, LPINT pdx, LPINT pdy)
{

    POINT pt;
    
    GetCursorPos(&pt);
    
    *pdx = 0;
    *pdy = 0;
    
    if (pt.x <= prcHot->left) {
        *pdx = ((pt.x - prcHot->left) / RM_SCROLLUNIT) - 1;
    } else if (pt.x >= prcHot->right) {
        *pdx = ((pt.x - prcHot->right) / RM_SCROLLUNIT) + 1;
    }
    
    if (pt.y <= prcHot->top) {
        *pdy = ((pt.y - prcHot->top) / RM_SCROLLUNIT) - 1;
    } else if (pt.y >= prcHot->bottom) {
        *pdy = ((pt.y - prcHot->bottom) / RM_SCROLLUNIT) + 1;
    }

    if (prmi->fFlags & RMF_VERTICALONLY)
        *pdx = 0;

    if (prmi->fFlags & RMF_HORIZONTALONLY)
        *pdy = 0;
}

void RM_CheckScroll(PREADERMODEINFO prmi, LPRECT prcHot)
{
    int dx;
    int dy;

    RM_GetScrollXY(prmi, prcHot, &dx, &dy);
    prmi->pfnScroll(prmi, dx, dy);
}

void RM_SetCursor(PREADERMODEINFO prmi, LPRECT prcHot)
{
    int dx;
    int dy;
    LPCTSTR pRes;
    
    RM_GetScrollXY(prmi, prcHot, &dx, &dy);

    // default is center
    if (prmi->fFlags & RMF_VERTICALONLY)
        pRes = IDC_VERTICALONLY;
    else if (prmi->fFlags & RMF_HORIZONTALONLY)
        pRes = IDC_HORIZONTALONLY;
    else
        pRes = IDC_MOVE2D;

    // multiply to figure out if either is zero and also the sign parity
    if (dy * dx) {
        // diagonal case
        if (dy > 0) {
            if (dx > 0)
                pRes = IDC_SOUTHEAST;
            else
                pRes = IDC_SOUTHWEST;
        } else {
            if (dx > 0)
                pRes = IDC_NORTHEAST;
            else
                pRes = IDC_NORTHWEST;
        }
    } else {
        // simple horizontal or vertical case
        if (dy > 0)
            pRes = IDC_SOUTH;
        else if (dy < 0)
            pRes = IDC_NORTH;
        else if (dx < 0)
            pRes = IDC_WEST;
        else if (dx > 0)
            pRes = IDC_EAST;
    }
    
    SetCursor(LoadCursor(HINST_THISDLL, pRes));
    
}

void DoReaderMode(PREADERMODEINFO prmi)
{
    RECT rcHot;
    
    if (!prmi->hwnd || prmi->cbSize != sizeof(*prmi))
        return;
    
    SetCapture(prmi->hwnd);
    
    // if they didn't pass in a rect, then use the window
    if (!prmi->prc) {
        GetWindowRect(prmi->hwnd, &rcHot );
    } else {
        rcHot = *prmi->prc;
        MapWindowPoints(prmi->hwnd, HWND_DESKTOP, (LPPOINT)&rcHot, 2);
    }
    
    
    // set the cursor to the center of the hot rect if they ask us to
    if (prmi->fFlags & RMF_ZEROCURSOR) {
        SetCursorPos((rcHot.left + rcHot.right)/2, 
                     (rcHot.top + rcHot.bottom)/2);
    }
    
    while (GetCapture() == prmi->hwnd) {
        
        BOOL  fMessage;
        MSG32 msg32;
        RM_CheckScroll(prmi, &rcHot);

        // Try to peek keyboard message first, then mouse message,
        // and finally, other message. This is for raid 44392.
        // During scrolling, Trident might generate too many WM_PAINT
        // messages that push keyboard/mouse message (that DoReaderMode()
        // uses to stop auto-scroll mode) down in message pump, and we can
        // not get those messages until we peek and process all these
        // WM_PAINT messages. This is way cuto-scroll mode can be stopped
        // only by moving cursor back to origin circle (Trident does not
        // scroll, so no need to paint). Trident's scroll performance
        // issue will be worked on after RTM (raid 33232).
        //
        fMessage = PeekMessage32(&msg32, NULL, WM_KEYFIRST, WM_KEYLAST,
                        PM_REMOVE, TRUE);
        if (!fMessage)
        {
            fMessage = PeekMessage32(&msg32, NULL, WM_MOUSEFIRST, WM_MOUSELAST,
                            PM_REMOVE, TRUE);
            if (!fMessage)
            {
                fMessage = PeekMessage32(&msg32, NULL, 0, 0, PM_REMOVE, TRUE);
            }
        }

        if (fMessage) {
            if (!prmi->pfnTranslateDispatch || 
                !prmi->pfnTranslateDispatch((LPMSG)&msg32)) {

                if (msg32.message == g_msgMSWheel)
                    goto BailOut;

                switch(msg32.message) {
                case WM_LBUTTONUP:
                case WM_RBUTTONUP:
                case WM_MBUTTONUP:
                case WM_LBUTTONDOWN:
                case WM_RBUTTONDOWN:
                case WM_MBUTTONDOWN:
                case WM_SYSKEYDOWN:
BailOut:
                    ReleaseCapture();
                    break;

                case WM_KEYDOWN:
                    // if it's an arrow key, move the mouse cursor
                    RM_HandleKeyDown(&rcHot, msg32.wParam, msg32.lParam);
                    break;

                case WM_MOUSEMOVE:
                case WM_SETCURSOR:
                    RM_SetCursor(prmi, &rcHot);
                    break;

                default:
                    TranslateMessage32(&msg32, TRUE);
                    DispatchMessage32(&msg32, TRUE);
                }
                
            }
        }
        else WaitMessage();
    }
}
