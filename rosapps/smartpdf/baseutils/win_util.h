/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#ifndef WIN_UTIL_H_
#define WIN_UTIL_H_
#include <commctrl.h>

/* Utilities to help in common windows programming tasks */

#ifdef __cplusplus
extern "C"
{
#endif

 /* constant to make it easier to return proper LRESULT values when handling
   various windows messages */
#define WM_KILLFOCUS_HANDLED 0
#define WM_SETFOCUS_HANDLED 0
#define WM_KEYDOWN_HANDLED 0
#define WM_KEYUP_HANDLED 0
#define WM_LBUTTONDOWN_HANDLED 0
#define WM_LBUTTONUP_HANDLED 0
#define WM_PAINT_HANDLED 0
#define WM_DRAWITEM_HANDLED TRUE
#define WM_MEASUREITEM_HANDLED TRUE
#define WM_SIZE_HANDLED 0
#define LVN_ITEMACTIVATE_HANDLED 0
#define WM_VKEYTOITEM_HANDLED_FULLY -2
#define WM_VKEYTOITEM_NOT_HANDLED -1
#define WM_CREATE_OK 0
#define WM_CREATE_FAILED -1

#define WIN_COL_RED     RGB(255,0,0)
#define WIN_COL_WHITE   RGB(255,255,255)
#define WIN_COL_BLACK   RGB(0,0,0)
#define WIN_COL_BLUE    RGB(0,0,255)
#define WIN_COL_GREEN   RGB(0,255,0)
#define WIN_COL_GRAY    RGB(215,215,215)

int     rect_dx(RECT *r);
int     rect_dy(RECT *r);
void    rect_set(RECT *r, int x, int y, int dx, int dy);

void    win_set_font(HWND hwnd, HFONT font);

int     win_get_text_len(HWND hwnd);
TCHAR * win_get_text(HWND hwnd);
void    win_set_text(HWND hwnd, const TCHAR *txt);

void win_edit_set_selection(HWND hwnd, DWORD selStart, DWORD selEnd);
void win_edit_select_all(HWND hwnd);

LRESULT lv_delete_all_items(HWND hwnd);
LRESULT lv_set_items_count(HWND hwnd, int items_count);
int     lv_get_items_count(HWND hwnd);
LRESULT lv_insert_column(HWND hwnd, int col, LVCOLUMN *lvc);
LRESULT lv_set_column(HWND hwnd, int col, LVCOLUMN *lvc);
LRESULT lv_set_column_dx(HWND hwnd, int col, int dx);
LRESULT lv_insert_item(HWND hwnd, int row, LVITEM *lvi);
LRESULT lv_insert_item_text(HWND hwnd, int row, const TCHAR *txt);
int     lv_get_selection_pos(HWND hwnd);
LRESULT lb_delete_all_items(HWND hwnd);
#if 0 /* doesn't seem to be supported under wince */
LRESULT lb_set_items_count(HWND hwnd, int items_count);
#endif
LRESULT lb_insert_item_text(HWND hwnd, int row, const TCHAR *txt);
LRESULT lb_append_string_no_sort(HWND hwnd, const TCHAR *txt);
LRESULT lb_get_items_count(HWND hwnd);
LRESULT lb_set_selection(HWND hwnd, int item);
LRESULT lb_get_selection(HWND hwnd);

int     font_get_dy(HWND hwnd, HFONT font);
int     font_get_dy_from_dc(HDC hdc, HFONT font);

void    screen_get_dx_dy(int *dx_out, int *dy_out);
int     screen_get_dx(void);
int     screen_get_dy(void);
int     screen_get_menu_dy(void);
int     screen_get_caption_dy(void);

#ifdef _WIN32_WCE
void    sip_completion_disable(void);
void    sip_completion_enable(void);
#endif

void    launch_url(const TCHAR *url);

TCHAR * get_app_data_folder_path(BOOL f_create);

TCHAR * load_string_dup(int str_id);
const TCHAR *load_string(int str_id);

int     regkey_set_dword(HKEY key_class, TCHAR *key_path, TCHAR *key_name, DWORD key_value);
int     regkey_set_str(HKEY key_class, TCHAR *key_path, TCHAR *key_name, TCHAR *key_value);

void    paint_round_rect_around_hwnd(HDC hdc, HWND hwnd_edit_parent, HWND hwnd_edit, COLORREF col);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class AppBarData {
public:
    AppBarData() {
        m_abd.cbSize = sizeof(m_abd);
        /* default values for the case of SHAppBarMessage() failing
           (shouldn't really happen) */
        RECT rc = {0, 0, 0, 0};
        m_abd.rc = rc;
        m_abd.uEdge = ABE_TOP;
        SHAppBarMessage(ABM_GETTASKBARPOS, &m_abd);
    }
    int dx() { return rect_dx(&m_abd.rc); }
    int dy() { return rect_dy(&m_abd.rc); }
    int x() { return m_abd.rc.left; }
    int y() { return m_abd.rc.top; }
    bool atTop() { return ABE_TOP == m_abd.uEdge; }
    bool atBottom() { return ABE_BOTTOM == m_abd.uEdge; }
    bool atLeft() { return ABE_LEFT == m_abd.uEdge; }
    bool atRight() { return ABE_RIGHT == m_abd.uEdge; }
    bool isHorizontal() { return atLeft() || atRight(); }
    bool isVertical() { return atBottom() || atTop(); }
private:
    APPBARDATA m_abd;
};
#endif

#endif
