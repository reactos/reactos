/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "msi.h"
#include "msipriv.h"
#include "msidefs.h"
#include "ocidl.h"
#include "olectl.h"
#include "richedit.h"
#include "commctrl.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);


extern HINSTANCE msi_hInstance;

struct msi_control_tag;
typedef struct msi_control_tag msi_control;
typedef UINT (*msi_handler)( msi_dialog *, msi_control *, WPARAM );

struct msi_control_tag
{
    struct list entry;
    HWND hwnd;
    msi_handler handler;
    LPWSTR property;
    LPWSTR value;
    HBITMAP hBitmap;
    HICON hIcon;
    LPWSTR tabnext;
    LPWSTR type;
    HMODULE hDll;
    float progress_current;
    float progress_max;
    DWORD attributes;
    WCHAR name[1];
};

typedef struct msi_font_tag
{
    struct msi_font_tag *next;
    HFONT hfont;
    COLORREF color;
    WCHAR name[1];
} msi_font;

struct msi_dialog_tag
{
    MSIPACKAGE *package;
    msi_dialog *parent;
    msi_dialog_event_handler event_handler;
    BOOL finished;
    INT scale;
    DWORD attributes;
    SIZE size;
    HWND hwnd;
    LPWSTR default_font;
    msi_font *font_list;
    struct list controls;
    HWND hWndFocus;
    LPWSTR control_default;
    LPWSTR control_cancel;
    WCHAR name[1];
};

typedef UINT (*msi_dialog_control_func)( msi_dialog *dialog, MSIRECORD *rec );
struct control_handler 
{
    LPCWSTR control_type;
    msi_dialog_control_func func;
};

typedef struct
{
    msi_dialog* dialog;
    msi_control *parent;
    DWORD       attributes;
    LPWSTR      propval;
} radio_button_group_descr;

static const WCHAR szMsiDialogClass[] = {
    'M','s','i','D','i','a','l','o','g','C','l','o','s','e','C','l','a','s','s',0
};
static const WCHAR szMsiHiddenWindow[] = {
    'M','s','i','H','i','d','d','e','n','W','i','n','d','o','w',0 };
static const WCHAR szStatic[] = { 'S','t','a','t','i','c',0 };
static const WCHAR szButton[] = { 'B','U','T','T','O','N', 0 };
static const WCHAR szButtonData[] = { 'M','S','I','D','A','T','A',0 };
static const WCHAR szProgress[] = { 'P','r','o','g','r','e','s','s',0 };
static const WCHAR szText[] = { 'T','e','x','t',0 };
static const WCHAR szPushButton[] = { 'P','u','s','h','B','u','t','t','o','n',0 };
static const WCHAR szLine[] = { 'L','i','n','e',0 };
static const WCHAR szBitmap[] = { 'B','i','t','m','a','p',0 };
static const WCHAR szCheckBox[] = { 'C','h','e','c','k','B','o','x',0 };
static const WCHAR szScrollableText[] = {
    'S','c','r','o','l','l','a','b','l','e','T','e','x','t',0 };
static const WCHAR szComboBox[] = { 'C','o','m','b','o','B','o','x',0 };
static const WCHAR szEdit[] = { 'E','d','i','t',0 };
static const WCHAR szMaskedEdit[] = { 'M','a','s','k','e','d','E','d','i','t',0 };
static const WCHAR szPathEdit[] = { 'P','a','t','h','E','d','i','t',0 };
static const WCHAR szProgressBar[] = {
     'P','r','o','g','r','e','s','s','B','a','r',0 };
static const WCHAR szRadioButtonGroup[] = { 
    'R','a','d','i','o','B','u','t','t','o','n','G','r','o','u','p',0 };
static const WCHAR szIcon[] = { 'I','c','o','n',0 };
static const WCHAR szSelectionTree[] = {
    'S','e','l','e','c','t','i','o','n','T','r','e','e',0 };
static const WCHAR szGroupBox[] = { 'G','r','o','u','p','B','o','x',0 };
static const WCHAR szListBox[] = { 'L','i','s','t','B','o','x',0 };
static const WCHAR szDirectoryCombo[] = { 'D','i','r','e','c','t','o','r','y','C','o','m','b','o',0 };
static const WCHAR szDirectoryList[] = { 'D','i','r','e','c','t','o','r','y','L','i','s','t',0 };
static const WCHAR szVolumeCostList[] = { 'V','o','l','u','m','e','C','o','s','t','L','i','s','t',0 };
static const WCHAR szSelectionDescription[] = {'S','e','l','e','c','t','i','o','n','D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR szSelectionPath[] = {'S','e','l','e','c','t','i','o','n','P','a','t','h',0};
static const WCHAR szProperty[] = {'P','r','o','p','e','r','t','y',0};

static UINT msi_dialog_checkbox_handler( msi_dialog *, msi_control *, WPARAM );
static void msi_dialog_checkbox_sync_state( msi_dialog *, msi_control * );
static UINT msi_dialog_button_handler( msi_dialog *, msi_control *, WPARAM );
static UINT msi_dialog_edit_handler( msi_dialog *, msi_control *, WPARAM );
static UINT msi_dialog_radiogroup_handler( msi_dialog *, msi_control *, WPARAM param );
static UINT msi_dialog_evaluate_control_conditions( msi_dialog *dialog );
static LRESULT WINAPI MSIRadioGroup_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static MSIFEATURE *msi_seltree_get_selected_feature( msi_control *control );

/* dialog sequencing */

#define WM_MSI_DIALOG_CREATE  (WM_USER+0x100)
#define WM_MSI_DIALOG_DESTROY (WM_USER+0x101)

static DWORD uiThreadId;
static HWND hMsiHiddenWindow;

static INT msi_dialog_scale_unit( msi_dialog *dialog, INT val )
{
    return (dialog->scale * val + 5) / 10;
}

static msi_control *msi_dialog_find_control( msi_dialog *dialog, LPCWSTR name )
{
    msi_control *control;

    if( !name )
        return NULL;
    LIST_FOR_EACH_ENTRY( control, &dialog->controls, msi_control, entry )
        if( !strcmpW( control->name, name ) ) /* FIXME: case sensitive? */
            return control;
    return NULL;
}

static msi_control *msi_dialog_find_control_by_type( msi_dialog *dialog, LPCWSTR type )
{
    msi_control *control;

    if( !type )
        return NULL;
    LIST_FOR_EACH_ENTRY( control, &dialog->controls, msi_control, entry )
        if( !strcmpW( control->type, type ) ) /* FIXME: case sensitive? */
            return control;
    return NULL;
}

static msi_control *msi_dialog_find_control_by_hwnd( msi_dialog *dialog, HWND hwnd )
{
    msi_control *control;

    LIST_FOR_EACH_ENTRY( control, &dialog->controls, msi_control, entry )
        if( hwnd == control->hwnd )
            return control;
    return NULL;
}

static LPWSTR msi_get_deformatted_field( MSIPACKAGE *package, MSIRECORD *rec, int field )
{
    LPCWSTR str = MSI_RecordGetString( rec, field );
    LPWSTR ret = NULL;

    if (str)
        deformat_string( package, str, &ret );
    return ret;
}

static LPWSTR msi_dialog_dup_property( msi_dialog *dialog, LPCWSTR property, BOOL indirect )
{
    LPWSTR prop = NULL;

    if (!property)
        return NULL;

    if (indirect)
        prop = msi_dup_property( dialog->package, property );

    if (!prop)
        prop = strdupW( property );

    return prop;
}

msi_dialog *msi_dialog_get_parent( msi_dialog *dialog )
{
    return dialog->parent;
}

LPWSTR msi_dialog_get_name( msi_dialog *dialog )
{
    return dialog->name;
}

/*
 * msi_dialog_get_style
 *
 * Extract the {\style} string from the front of the text to display and
 * update the pointer.  Only the last style in a list is applied.
 */
static LPWSTR msi_dialog_get_style( LPCWSTR p, LPCWSTR *rest )
{
    LPWSTR ret;
    LPCWSTR q, i, first;
    DWORD len;

    q = NULL;
    *rest = p;
    if( !p )
        return NULL;

    while ((first = strchrW( p, '{' )) && (q = strchrW( first + 1, '}' )))
    {
        p = first + 1;
        if( *p == '\\' || *p == '&' )
            p++;

        /* little bit of sanity checking to stop us getting confused with RTF */
        for( i=p; i<q; i++ )
            if( *i == '}' || *i == '\\' )
                return NULL;
    }

    if (!p || !q)
        return NULL;

    *rest = ++q;
    len = q - p;

    ret = msi_alloc( len*sizeof(WCHAR) );
    if( !ret )
        return ret;
    memcpy( ret, p, len*sizeof(WCHAR) );
    ret[len-1] = 0;
    return ret;
}

static UINT msi_dialog_add_font( MSIRECORD *rec, LPVOID param )
{
    msi_dialog *dialog = param;
    msi_font *font;
    LPCWSTR face, name;
    LOGFONTW lf;
    INT style;
    HDC hdc;

    /* create a font and add it to the list */
    name = MSI_RecordGetString( rec, 1 );
    font = msi_alloc( sizeof *font + strlenW( name )*sizeof (WCHAR) );
    strcpyW( font->name, name );
    font->next = dialog->font_list;
    dialog->font_list = font;

    font->color = MSI_RecordGetInteger( rec, 4 );

    memset( &lf, 0, sizeof lf );
    face = MSI_RecordGetString( rec, 2 );
    lf.lfHeight = MSI_RecordGetInteger( rec, 3 );
    style = MSI_RecordGetInteger( rec, 5 );
    if( style & msidbTextStyleStyleBitsBold )
        lf.lfWeight = FW_BOLD;
    if( style & msidbTextStyleStyleBitsItalic )
        lf.lfItalic = TRUE;
    if( style & msidbTextStyleStyleBitsUnderline )
        lf.lfUnderline = TRUE;
    if( style & msidbTextStyleStyleBitsStrike )
        lf.lfStrikeOut = TRUE;
    lstrcpynW( lf.lfFaceName, face, LF_FACESIZE );

    /* adjust the height */
    hdc = GetDC( dialog->hwnd );
    if (hdc)
    {
        lf.lfHeight = -MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC( dialog->hwnd, hdc );
    }

    font->hfont = CreateFontIndirectW( &lf );

    TRACE("Adding font style %s\n", debugstr_w(font->name) );

    return ERROR_SUCCESS;
}

static msi_font *msi_dialog_find_font( msi_dialog *dialog, LPCWSTR name )
{
    msi_font *font;

    for( font = dialog->font_list; font; font = font->next )
        if( !strcmpW( font->name, name ) )  /* FIXME: case sensitive? */
            break;

    return font;
}

static UINT msi_dialog_set_font( msi_dialog *dialog, HWND hwnd, LPCWSTR name )
{
    msi_font *font;

    font = msi_dialog_find_font( dialog, name );
    if( font )
        SendMessageW( hwnd, WM_SETFONT, (WPARAM) font->hfont, TRUE );
    else
        ERR("No font entry for %s\n", debugstr_w(name));
    return ERROR_SUCCESS;
}

static UINT msi_dialog_build_font_list( msi_dialog *dialog )
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','*',' ',
      'F','R','O','M',' ','`','T','e','x','t','S','t','y','l','e','`',' ',0
    };
    UINT r;
    MSIQUERY *view = NULL;

    TRACE("dialog %p\n", dialog );

    r = MSI_OpenQuery( dialog->package->db, &view, query );
    if( r != ERROR_SUCCESS )
        return r;

    r = MSI_IterateRecords( view, NULL, msi_dialog_add_font, dialog );
    msiobj_release( &view->hdr );

    return r;
}

static msi_control *msi_dialog_create_window( msi_dialog *dialog,
                MSIRECORD *rec, DWORD exstyle, LPCWSTR szCls, LPCWSTR name, LPCWSTR text,
                DWORD style, HWND parent )
{
    DWORD x, y, width, height;
    LPWSTR font = NULL, title_font = NULL;
    LPCWSTR title = NULL;
    msi_control *control;

    style |= WS_CHILD;

    control = msi_alloc( sizeof *control + strlenW(name)*sizeof(WCHAR) );
    strcpyW( control->name, name );
    list_add_head( &dialog->controls, &control->entry );
    control->handler = NULL;
    control->property = NULL;
    control->value = NULL;
    control->hBitmap = NULL;
    control->hIcon = NULL;
    control->hDll = NULL;
    control->tabnext = strdupW( MSI_RecordGetString( rec, 11) );
    control->type = strdupW( MSI_RecordGetString( rec, 3 ) );
    control->progress_current = 0;
    control->progress_max = 100;

    x = MSI_RecordGetInteger( rec, 4 );
    y = MSI_RecordGetInteger( rec, 5 );
    width = MSI_RecordGetInteger( rec, 6 );
    height = MSI_RecordGetInteger( rec, 7 );

    x = msi_dialog_scale_unit( dialog, x );
    y = msi_dialog_scale_unit( dialog, y );
    width = msi_dialog_scale_unit( dialog, width );
    height = msi_dialog_scale_unit( dialog, height );

    if( text )
    {
        deformat_string( dialog->package, text, &title_font );
        font = msi_dialog_get_style( title_font, &title );
    }

    control->hwnd = CreateWindowExW( exstyle, szCls, title, style,
                          x, y, width, height, parent, NULL, NULL, NULL );

    TRACE("Dialog %s control %s hwnd %p\n",
           debugstr_w(dialog->name), debugstr_w(text), control->hwnd );

    msi_dialog_set_font( dialog, control->hwnd,
                         font ? font : dialog->default_font );

    msi_free( title_font );
    msi_free( font );

    return control;
}

static LPWSTR msi_dialog_get_uitext( msi_dialog *dialog, LPCWSTR key )
{
    MSIRECORD *rec;
    LPWSTR text;

    static const WCHAR query[] = {
        's','e','l','e','c','t',' ','*',' ',
        'f','r','o','m',' ','`','U','I','T','e','x','t','`',' ',
        'w','h','e','r','e',' ','`','K','e','y','`',' ','=',' ','\'','%','s','\'',0
    };

    rec = MSI_QueryGetRecord( dialog->package->db, query, key );
    if (!rec) return NULL;
    text = strdupW( MSI_RecordGetString( rec, 2 ) );
    msiobj_release( &rec->hdr );
    return text;
}

static MSIRECORD *msi_get_binary_record( MSIDATABASE *db, LPCWSTR name )
{
    static const WCHAR query[] = {
        's','e','l','e','c','t',' ','*',' ',
        'f','r','o','m',' ','B','i','n','a','r','y',' ',
        'w','h','e','r','e',' ',
            '`','N','a','m','e','`',' ','=',' ','\'','%','s','\'',0
    };

    return MSI_QueryGetRecord( db, query, name );
}

static LPWSTR msi_create_tmp_path(void)
{
    WCHAR tmp[MAX_PATH];
    LPWSTR path = NULL;
    static const WCHAR prefix[] = { 'm','s','i',0 };
    DWORD len, r;

    r = GetTempPathW( MAX_PATH, tmp );
    if( !r )
        return path;
    len = lstrlenW( tmp ) + 20;
    path = msi_alloc( len * sizeof (WCHAR) );
    if( path )
    {
        r = GetTempFileNameW( tmp, prefix, 0, path );
        if (!r)
        {
            msi_free( path );
            path = NULL;
        }
    }
    return path;
}


static HANDLE msi_load_image( MSIDATABASE *db, LPCWSTR name, UINT type,
                              UINT cx, UINT cy, UINT flags )
{
    MSIRECORD *rec = NULL;
    HANDLE himage = NULL;
    LPWSTR tmp;
    UINT r;

    TRACE("%p %s %u %u %08x\n", db, debugstr_w(name), cx, cy, flags);

    tmp = msi_create_tmp_path();
    if( !tmp )
        return himage;

    rec = msi_get_binary_record( db, name );
    if( rec )
    {
        r = MSI_RecordStreamToFile( rec, 2, tmp );
        if( r == ERROR_SUCCESS )
        {
            himage = LoadImageW( 0, tmp, type, cx, cy, flags );
            DeleteFileW( tmp );
        }
        msiobj_release( &rec->hdr );
    }

    msi_free( tmp );
    return himage;
}

static HICON msi_load_icon( MSIDATABASE *db, LPCWSTR text, UINT attributes )
{
    DWORD cx = 0, cy = 0, flags;

    flags = LR_LOADFROMFILE | LR_DEFAULTSIZE;
    if( attributes & msidbControlAttributesFixedSize )
    {
        flags &= ~LR_DEFAULTSIZE;
        if( attributes & msidbControlAttributesIconSize16 )
        {
            cx += 16;
            cy += 16;
        }
        if( attributes & msidbControlAttributesIconSize32 )
        {
            cx += 32;
            cy += 32;
        }
        /* msidbControlAttributesIconSize48 handled by above logic */
    }
    return msi_load_image( db, text, IMAGE_ICON, cx, cy, flags );
}


/* called from the Control Event subscription code */
void msi_dialog_handle_event( msi_dialog* dialog, LPCWSTR control, 
                              LPCWSTR attribute, MSIRECORD *rec )
{
    msi_control* ctrl;
    LPCWSTR font_text, text = NULL;
    LPWSTR font;

    static const WCHAR empty[] = {0};

    ctrl = msi_dialog_find_control( dialog, control );
    if (!ctrl)
        return;
    if( !lstrcmpW(attribute, szText) )
    {
        font_text = MSI_RecordGetString( rec , 1 );
        font = msi_dialog_get_style( font_text, &text );
        if (!text) text = empty;
        SetWindowTextW( ctrl->hwnd, text );
        msi_free( font );
        msi_dialog_check_messages( NULL );
    }
    else if( !lstrcmpW(attribute, szProgress) )
    {
        DWORD func, val;

        func = MSI_RecordGetInteger( rec , 1 );
        val = MSI_RecordGetInteger( rec , 2 );

        switch (func)
        {
        case 0: /* init */
            ctrl->progress_max = val;
            ctrl->progress_current = 0;
            SendMessageW(ctrl->hwnd, PBM_SETRANGE, 0, MAKELPARAM(0,100));
            SendMessageW(ctrl->hwnd, PBM_SETPOS, 0, 0);
            break;
        case 1: /* FIXME: not sure what this is supposed to do */
            break;
        case 2: /* move */
            ctrl->progress_current += val;
            SendMessageW(ctrl->hwnd, PBM_SETPOS, 100*(ctrl->progress_current/ctrl->progress_max), 0);
            break;
        default:
            ERR("Unknown progress message %d\n", func);
            break;
        }
    }
    else if ( !lstrcmpW(attribute, szProperty) )
    {
        MSIFEATURE *feature = msi_seltree_get_selected_feature( ctrl );
        MSI_SetPropertyW( dialog->package, ctrl->property, feature->Directory );
    }
    else
    {
        FIXME("Attribute %s not being set\n", debugstr_w(attribute));
        return;
    }
}

static void msi_dialog_map_events(msi_dialog* dialog, LPCWSTR control)
{
    static const WCHAR Query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','E','v','e','n','t','M','a','p','p','i','n','g','`',' ',
        'W','H','E','R','E',' ',
         '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',' ',
        'A','N','D',' ',
         '`','C','o','n','t','r','o','l','_','`',' ','=',' ','\'','%','s','\'',0
    };
    MSIRECORD *row;
    LPCWSTR event, attribute;

    row = MSI_QueryGetRecord( dialog->package->db, Query, dialog->name, control );
    if (!row)
        return;

    event = MSI_RecordGetString( row, 3 );
    attribute = MSI_RecordGetString( row, 4 );
    ControlEvent_SubscribeToEvent( dialog->package, dialog, event, control, attribute );
    msiobj_release( &row->hdr );
}

/* everything except radio buttons */
static msi_control *msi_dialog_add_control( msi_dialog *dialog,
                MSIRECORD *rec, LPCWSTR szCls, DWORD style )
{
    DWORD attributes;
    LPCWSTR text, name;
    DWORD exstyle = 0;

    name = MSI_RecordGetString( rec, 2 );
    attributes = MSI_RecordGetInteger( rec, 8 );
    text = MSI_RecordGetString( rec, 10 );
    if( attributes & msidbControlAttributesVisible )
        style |= WS_VISIBLE;
    if( ~attributes & msidbControlAttributesEnabled )
        style |= WS_DISABLED;
    if( attributes & msidbControlAttributesSunken )
        exstyle |= WS_EX_CLIENTEDGE;

    msi_dialog_map_events(dialog, name);

    return msi_dialog_create_window( dialog, rec, exstyle, szCls, name,
                                     text, style, dialog->hwnd );
}

struct msi_text_info
{
    msi_font *font;
    WNDPROC oldproc;
    DWORD attributes;
};

/*
 * we don't erase our own background,
 * so we have to make sure that the parent window redraws first
 */
static void msi_text_on_settext( HWND hWnd )
{
    HWND hParent;
    RECT rc;

    hParent = GetParent( hWnd );
    GetWindowRect( hWnd, &rc );
    MapWindowPoints( NULL, hParent, (LPPOINT) &rc, 2 );
    InvalidateRect( hParent, &rc, TRUE );
}

static LRESULT WINAPI
MSIText_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct msi_text_info *info;
    LRESULT r = 0;

    TRACE("%p %04x %08x %08lx\n", hWnd, msg, wParam, lParam);

    info = GetPropW(hWnd, szButtonData);

    if ( info->font )
        SetTextColor( (HDC)wParam, info->font->color );

    if( msg == WM_CTLCOLORSTATIC &&
       ( info->attributes & msidbControlAttributesTransparent ) )
    {
        SetBkMode( (HDC)wParam, TRANSPARENT );
        return (LRESULT) GetStockObject(NULL_BRUSH);
    }

    r = CallWindowProcW(info->oldproc, hWnd, msg, wParam, lParam);

    switch( msg )
    {
    case WM_SETTEXT:
        msi_text_on_settext( hWnd );
        break;
    case WM_NCDESTROY:
        msi_free( info );
        RemovePropW( hWnd, szButtonData );
        break;
    }

    return r;
}

static UINT msi_dialog_text_control( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    struct msi_text_info *info;
    LPCWSTR text, ptr;
    LPWSTR font_name;

    TRACE("%p %p\n", dialog, rec);

    control = msi_dialog_add_control( dialog, rec, szStatic, SS_LEFT | WS_GROUP );
    if( !control )
        return ERROR_FUNCTION_FAILED;

    info = msi_alloc( sizeof *info );
    if( !info )
        return ERROR_SUCCESS;

    text = MSI_RecordGetString( rec, 10 );
    font_name = msi_dialog_get_style( text, &ptr );
    info->font = ( font_name ) ? msi_dialog_find_font( dialog, font_name ) : NULL;
    msi_free( font_name );

    info->attributes = MSI_RecordGetInteger( rec, 8 );
    if( info->attributes & msidbControlAttributesTransparent )
        SetWindowLongPtrW( control->hwnd, GWL_EXSTYLE, WS_EX_TRANSPARENT );

    info->oldproc = (WNDPROC) SetWindowLongPtrW( control->hwnd, GWLP_WNDPROC,
                                          (LONG_PTR)MSIText_WndProc );
    SetPropW( control->hwnd, szButtonData, info );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_button_control( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    UINT attributes, style;
    LPWSTR text;

    TRACE("%p %p\n", dialog, rec);

    style = WS_TABSTOP;
    attributes = MSI_RecordGetInteger( rec, 8 );
    if( attributes & msidbControlAttributesIcon )
        style |= BS_ICON;

    control = msi_dialog_add_control( dialog, rec, szButton, style );
    if( !control )
        return ERROR_FUNCTION_FAILED;

    control->handler = msi_dialog_button_handler;

    /* set the icon */
    text = msi_get_deformatted_field( dialog->package, rec, 10 );
    control->hIcon = msi_load_icon( dialog->package->db, text, attributes );
    if( attributes & msidbControlAttributesIcon )
        SendMessageW( control->hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM) control->hIcon );
    msi_free( text );

    return ERROR_SUCCESS;
}

static LPWSTR msi_get_checkbox_value( msi_dialog *dialog, LPCWSTR prop )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','`','C','h','e','c','k','B','o','x',' ','`',
        'W','H','E','R','E',' ',
        '`','P','r','o','p','e','r','t','y','`',' ','=',' ',
        '\'','%','s','\'',0
    };
    MSIRECORD *rec = NULL;
    LPWSTR ret = NULL;

    /* find if there is a value associated with the checkbox */
    rec = MSI_QueryGetRecord( dialog->package->db, query, prop );
    if (!rec)
        return ret;

    ret = msi_get_deformatted_field( dialog->package, rec, 2 );
    if( ret && !ret[0] )
    {
        msi_free( ret );
        ret = NULL;
    }
    msiobj_release( &rec->hdr );
    if (ret)
        return ret;

    ret = msi_dup_property( dialog->package, prop );
    if( ret && !ret[0] )
    {
        msi_free( ret );
        ret = NULL;
    }

    return ret;
}

static UINT msi_dialog_checkbox_control( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    LPCWSTR prop;

    TRACE("%p %p\n", dialog, rec);

    control = msi_dialog_add_control( dialog, rec, szButton,
                                BS_CHECKBOX | BS_MULTILINE | WS_TABSTOP );
    control->handler = msi_dialog_checkbox_handler;
    prop = MSI_RecordGetString( rec, 9 );
    if( prop )
    {
        control->property = strdupW( prop );
        control->value = msi_get_checkbox_value( dialog, prop );
        TRACE("control %s value %s\n", debugstr_w(control->property),
              debugstr_w(control->value));
    }
    msi_dialog_checkbox_sync_state( dialog, control );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_line_control( msi_dialog *dialog, MSIRECORD *rec )
{
    TRACE("%p %p\n", dialog, rec);

    /* line is exactly 2 units in height */
    MSI_RecordSetInteger( rec, 7, 2 );

    msi_dialog_add_control( dialog, rec, szStatic, SS_ETCHEDHORZ | SS_SUNKEN );
    return ERROR_SUCCESS;
}

/******************** Scroll Text ********************************************/

struct msi_scrolltext_info
{
    msi_dialog *dialog;
    msi_control *control;
    WNDPROC oldproc;
};

static LRESULT WINAPI
MSIScrollText_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct msi_scrolltext_info *info;
    HRESULT r;

    TRACE("%p %04x %08x %08lx\n", hWnd, msg, wParam, lParam);

    info = GetPropW( hWnd, szButtonData );

    r = CallWindowProcW( info->oldproc, hWnd, msg, wParam, lParam );

    switch( msg )
    {
    case WM_NCDESTROY:
        msi_free( info );
        RemovePropW( hWnd, szButtonData );
        break;
    case WM_PAINT:
        /* native MSI sets a wait cursor here */
        msi_dialog_button_handler( info->dialog, info->control, BN_CLICKED );
        break;
    }
    return r;
}

struct msi_streamin_info
{
    LPSTR string;
    DWORD offset;
    DWORD length;
};

static DWORD CALLBACK
msi_richedit_stream_in( DWORD_PTR arg, LPBYTE buffer, LONG count, LONG *pcb )
{
    struct msi_streamin_info *info = (struct msi_streamin_info*) arg;

    if( (count + info->offset) > info->length )
        count = info->length - info->offset;
    memcpy( buffer, &info->string[ info->offset ], count );
    *pcb = count;
    info->offset += count;

    TRACE("%d/%d\n", info->offset, info->length);

    return 0;
}

static void msi_scrolltext_add_text( msi_control *control, LPCWSTR text )
{
    struct msi_streamin_info info;
    EDITSTREAM es;

    info.string = strdupWtoA( text );
    info.offset = 0;
    info.length = lstrlenA( info.string ) + 1;

    es.dwCookie = (DWORD_PTR) &info;
    es.dwError = 0;
    es.pfnCallback = msi_richedit_stream_in;

    SendMessageW( control->hwnd, EM_STREAMIN, SF_RTF, (LPARAM) &es );

    msi_free( info.string );
}

static UINT msi_dialog_scrolltext_control( msi_dialog *dialog, MSIRECORD *rec )
{
    static const WCHAR szRichEdit20W[] = {
    	'R','i','c','h','E','d','i','t','2','0','W',0
    };
    struct msi_scrolltext_info *info;
    msi_control *control;
    HMODULE hRichedit;
    LPCWSTR text;
    DWORD style;

    info = msi_alloc( sizeof *info );
    if (!info)
        return ERROR_FUNCTION_FAILED;

    hRichedit = LoadLibraryA("riched20");

    style = WS_BORDER | ES_MULTILINE | WS_VSCROLL |
            ES_READONLY | ES_AUTOVSCROLL | WS_TABSTOP;
    control = msi_dialog_add_control( dialog, rec, szRichEdit20W, style );
    if (!control)
    {
        FreeLibrary( hRichedit );
        msi_free( info );
        return ERROR_FUNCTION_FAILED;
    }

    control->hDll = hRichedit;

    info->dialog = dialog;
    info->control = control;

    /* subclass the static control */
    info->oldproc = (WNDPROC) SetWindowLongPtrW( control->hwnd, GWLP_WNDPROC,
                                          (LONG_PTR)MSIScrollText_WndProc );
    SetPropW( control->hwnd, szButtonData, info );

    /* add the text into the richedit */
    text = MSI_RecordGetString( rec, 10 );
    if (text)
        msi_scrolltext_add_text( control, text );

    return ERROR_SUCCESS;
}

static HBITMAP msi_load_picture( MSIDATABASE *db, LPCWSTR name,
                                 INT cx, INT cy, DWORD flags )
{
    HBITMAP hOleBitmap = 0, hBitmap = 0, hOldSrcBitmap, hOldDestBitmap;
    MSIRECORD *rec = NULL;
    IStream *stm = NULL;
    IPicture *pic = NULL;
    HDC srcdc, destdc;
    BITMAP bm;
    UINT r;

    rec = msi_get_binary_record( db, name );
    if( !rec )
        goto end;

    r = MSI_RecordGetIStream( rec, 2, &stm );
    msiobj_release( &rec->hdr );
    if( r != ERROR_SUCCESS )
        goto end;

    r = OleLoadPicture( stm, 0, TRUE, &IID_IPicture, (LPVOID*) &pic );
    IStream_Release( stm );
    if( FAILED( r ) )
    {
        ERR("failed to load picture\n");
        goto end;
    }

    r = IPicture_get_Handle( pic, (OLE_HANDLE*) &hOleBitmap );
    if( FAILED( r ) )
    {
        ERR("failed to get bitmap handle\n");
        goto end;
    }
 
    /* make the bitmap the desired size */
    r = GetObjectW( hOleBitmap, sizeof bm, &bm );
    if (r != sizeof bm )
    {
        ERR("failed to get bitmap size\n");
        goto end;
    }

    if (flags & LR_DEFAULTSIZE)
    {
        cx = bm.bmWidth;
        cy = bm.bmHeight;
    }

    srcdc = CreateCompatibleDC( NULL );
    hOldSrcBitmap = SelectObject( srcdc, hOleBitmap );
    destdc = CreateCompatibleDC( NULL );
    hBitmap = CreateCompatibleBitmap( srcdc, cx, cy );
    hOldDestBitmap = SelectObject( destdc, hBitmap );
    StretchBlt( destdc, 0, 0, cx, cy,
                srcdc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    SelectObject( srcdc, hOldSrcBitmap );
    SelectObject( destdc, hOldDestBitmap );
    DeleteDC( srcdc );
    DeleteDC( destdc );

end:
    if ( pic )
        IPicture_Release( pic );
    return hBitmap;
}

static UINT msi_dialog_bitmap_control( msi_dialog *dialog, MSIRECORD *rec )
{
    UINT cx, cy, flags, style, attributes;
    msi_control *control;
    LPWSTR text;

    flags = LR_LOADFROMFILE;
    style = SS_BITMAP | SS_LEFT | WS_GROUP;

    attributes = MSI_RecordGetInteger( rec, 8 );
    if( attributes & msidbControlAttributesFixedSize )
    {
        flags |= LR_DEFAULTSIZE;
        style |= SS_CENTERIMAGE;
    }

    control = msi_dialog_add_control( dialog, rec, szStatic, style );
    cx = MSI_RecordGetInteger( rec, 6 );
    cy = MSI_RecordGetInteger( rec, 7 );
    cx = msi_dialog_scale_unit( dialog, cx );
    cy = msi_dialog_scale_unit( dialog, cy );

    text = msi_get_deformatted_field( dialog->package, rec, 10 );
    control->hBitmap = msi_load_picture( dialog->package->db, text, cx, cy, flags );
    if( control->hBitmap )
        SendMessageW( control->hwnd, STM_SETIMAGE,
                      IMAGE_BITMAP, (LPARAM) control->hBitmap );
    else
        ERR("Failed to load bitmap %s\n", debugstr_w(text));

    msi_free( text );
    
    return ERROR_SUCCESS;
}

static UINT msi_dialog_icon_control( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    DWORD attributes;
    LPWSTR text;

    TRACE("\n");

    control = msi_dialog_add_control( dialog, rec, szStatic,
                            SS_ICON | SS_CENTERIMAGE | WS_GROUP );
            
    attributes = MSI_RecordGetInteger( rec, 8 );
    text = msi_get_deformatted_field( dialog->package, rec, 10 );
    control->hIcon = msi_load_icon( dialog->package->db, text, attributes );
    if( control->hIcon )
        SendMessageW( control->hwnd, STM_SETICON, (WPARAM) control->hIcon, 0 );
    else
        ERR("Failed to load bitmap %s\n", debugstr_w(text));
    msi_free( text );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_combo_control( msi_dialog *dialog, MSIRECORD *rec )
{
    static const WCHAR szCombo[] = { 'C','O','M','B','O','B','O','X',0 };

    msi_dialog_add_control( dialog, rec, szCombo,
                            SS_BITMAP | SS_LEFT | SS_CENTERIMAGE );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_edit_control( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    LPCWSTR prop;
    LPWSTR val;

    control = msi_dialog_add_control( dialog, rec, szEdit,
                                      WS_BORDER | WS_TABSTOP );
    control->handler = msi_dialog_edit_handler;
    prop = MSI_RecordGetString( rec, 9 );
    if( prop )
        control->property = strdupW( prop );
    val = msi_dup_property( dialog->package, control->property );
    SetWindowTextW( control->hwnd, val );
    msi_free( val );
    return ERROR_SUCCESS;
}

/******************** Masked Edit ********************************************/

#define MASK_MAX_GROUPS 20

struct msi_mask_group
{
    UINT len;
    UINT ofs;
    WCHAR type;
    HWND hwnd;
};

struct msi_maskedit_info
{
    msi_dialog *dialog;
    WNDPROC oldproc;
    HWND hwnd;
    LPWSTR prop;
    UINT num_chars;
    UINT num_groups;
    struct msi_mask_group group[MASK_MAX_GROUPS];
};

static BOOL msi_mask_editable( WCHAR type )
{
    switch (type)
    {
    case '%':
    case '#':
    case '&':
    case '`':
    case '?':
    case '^':
        return TRUE;
    }
    return FALSE;
}

static void msi_mask_control_change( struct msi_maskedit_info *info )
{
    LPWSTR val;
    UINT i, n, r;

    val = msi_alloc( (info->num_chars+1)*sizeof(WCHAR) );
    for( i=0, n=0; i<info->num_groups; i++ )
    {
        if( (info->group[i].len + n) > info->num_chars )
        {
            ERR("can't fit control %d text into template\n",i);
            break;
        }
        if (!msi_mask_editable(info->group[i].type))
        {
            for(r=0; r<info->group[i].len; r++)
                val[n+r] = info->group[i].type;
            val[n+r] = 0;
        }
        else
        {
            r = GetWindowTextW( info->group[i].hwnd, &val[n], info->group[i].len+1 );
            if( r != info->group[i].len )
                break;
        }
        n += r;
    }

    TRACE("%d/%d controls were good\n", i, info->num_groups);

    if( i == info->num_groups )
    {
        TRACE("Set property %s to %s\n",
              debugstr_w(info->prop), debugstr_w(val) );
        CharUpperBuffW( val, info->num_chars );
        MSI_SetPropertyW( info->dialog->package, info->prop, val );
        msi_dialog_evaluate_control_conditions( info->dialog );
    }
    msi_free( val );
}

/* now move to the next control if necessary */
static VOID msi_mask_next_control( struct msi_maskedit_info *info, HWND hWnd )
{
    HWND hWndNext;
    UINT len, i;

    for( i=0; i<info->num_groups; i++ )
        if( info->group[i].hwnd == hWnd )
            break;

    /* don't move from the last control */
    if( i >= (info->num_groups-1) )
        return;

    len = SendMessageW( hWnd, WM_GETTEXTLENGTH, 0, 0 );
    if( len < info->group[i].len )
        return;

    hWndNext = GetNextDlgTabItem( GetParent( hWnd ), hWnd, FALSE );
    SetFocus( hWndNext );
}

static LRESULT WINAPI
MSIMaskedEdit_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct msi_maskedit_info *info;
    HRESULT r;

    TRACE("%p %04x %08x %08lx\n", hWnd, msg, wParam, lParam);

    info = GetPropW(hWnd, szButtonData);

    r = CallWindowProcW(info->oldproc, hWnd, msg, wParam, lParam);

    switch( msg )
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            msi_mask_control_change( info );
            msi_mask_next_control( info, (HWND) lParam );
        }
        break;
    case WM_NCDESTROY:
        msi_free( info->prop );
        msi_free( info );
        RemovePropW( hWnd, szButtonData );
        break;
    }

    return r;
}

/* fish the various bits of the property out and put them in the control */
static void
msi_maskedit_set_text( struct msi_maskedit_info *info, LPCWSTR text )
{
    LPCWSTR p;
    UINT i;

    p = text;
    for( i = 0; i < info->num_groups; i++ )
    {
        if( info->group[i].len < lstrlenW( p ) )
        {
            LPWSTR chunk = strdupW( p );
            chunk[ info->group[i].len ] = 0;
            SetWindowTextW( info->group[i].hwnd, chunk );
            msi_free( chunk );
        }
        else
        {
            SetWindowTextW( info->group[i].hwnd, p );
            break;
        }
        p += info->group[i].len;
    }
}

static struct msi_maskedit_info * msi_dialog_parse_groups( LPCWSTR mask )
{
    struct msi_maskedit_info * info = NULL;
    int i = 0, n = 0, total = 0;
    LPCWSTR p;

    TRACE("masked control, template %s\n", debugstr_w(mask));

    if( !mask )
        return info;

    info = msi_alloc_zero( sizeof *info );
    if( !info )
        return info;

    p = strchrW(mask, '<');
    if( p )
        p++;
    else
        p = mask;

    for( i=0; i<MASK_MAX_GROUPS; i++ )
    {
        /* stop at the end of the string */
        if( p[0] == 0 || p[0] == '>' )
            break;

        /* count the number of the same identifier */
        for( n=0; p[n] == p[0]; n++ )
            ;
        info->group[i].ofs = total;
        info->group[i].type = p[0];
        if( p[n] == '=' )
        {
            n++;
            total++; /* an extra not part of the group */
        }
        info->group[i].len = n;
        total += n;
        p += n;
    }

    TRACE("%d characters in %d groups\n", total, i );
    if( i == MASK_MAX_GROUPS )
        ERR("too many groups in PIDTemplate %s\n", debugstr_w(mask));

    info->num_chars = total;
    info->num_groups = i;

    return info;
}

static void
msi_maskedit_create_children( struct msi_maskedit_info *info, LPCWSTR font )
{
    DWORD width, height, style, wx, ww;
    RECT rect;
    HWND hwnd;
    UINT i;

    style = WS_CHILD | WS_BORDER | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;

    GetClientRect( info->hwnd, &rect );

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    for( i = 0; i < info->num_groups; i++ )
    {
        if (!msi_mask_editable( info->group[i].type ))
            continue;
        wx = (info->group[i].ofs * width) / info->num_chars;
        ww = (info->group[i].len * width) / info->num_chars;

        hwnd = CreateWindowW( szEdit, NULL, style, wx, 0, ww, height,
                              info->hwnd, NULL, NULL, NULL );
        if( !hwnd )
        {
            ERR("failed to create mask edit sub window\n");
            break;
        }

        SendMessageW( hwnd, EM_LIMITTEXT, info->group[i].len, 0 );

        msi_dialog_set_font( info->dialog, hwnd,
                             font?font:info->dialog->default_font );
        info->group[i].hwnd = hwnd;
    }
}

/*
 * office 2003 uses "73931<````=````=````=````=`````>@@@@@"
 * delphi 7 uses "<????-??????-??????-????>" and "<???-???>"
 * filemaker pro 7 uses "<^^^^=^^^^=^^^^=^^^^=^^^^=^^^^=^^^^^>"
 */
static UINT msi_dialog_maskedit_control( msi_dialog *dialog, MSIRECORD *rec )
{
    LPWSTR font_mask, val = NULL, font;
    struct msi_maskedit_info *info = NULL;
    UINT ret = ERROR_SUCCESS;
    msi_control *control;
    LPCWSTR prop, mask;

    TRACE("\n");

    font_mask = msi_get_deformatted_field( dialog->package, rec, 10 );
    font = msi_dialog_get_style( font_mask, &mask );
    if( !mask )
    {
        ERR("mask template is empty\n");
        goto end;
    }

    info = msi_dialog_parse_groups( mask );
    if( !info )
    {
        ERR("template %s is invalid\n", debugstr_w(mask));
        goto end;
    }

    info->dialog = dialog;

    control = msi_dialog_add_control( dialog, rec, szStatic,
                   SS_OWNERDRAW | WS_GROUP | WS_VISIBLE );
    if( !control )
    {
        ERR("Failed to create maskedit container\n");
        ret = ERROR_FUNCTION_FAILED;
        goto end;
    }
    SetWindowLongPtrW( control->hwnd, GWL_EXSTYLE, WS_EX_CONTROLPARENT );

    info->hwnd = control->hwnd;

    /* subclass the static control */
    info->oldproc = (WNDPROC) SetWindowLongPtrW( info->hwnd, GWLP_WNDPROC,
                                          (LONG_PTR)MSIMaskedEdit_WndProc );
    SetPropW( control->hwnd, szButtonData, info );

    prop = MSI_RecordGetString( rec, 9 );
    if( prop )
        info->prop = strdupW( prop );

    msi_maskedit_create_children( info, font );

    if( prop )
    {
        val = msi_dup_property( dialog->package, prop );
        if( val )
        {
            msi_maskedit_set_text( info, val );
            msi_free( val );
        }
    }

end:
    if( ret != ERROR_SUCCESS )
        msi_free( info );
    msi_free( font_mask );
    msi_free( font );
    return ret;
}

/******************** Progress Bar *****************************************/

static UINT msi_dialog_progress_bar( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_dialog_add_control( dialog, rec, PROGRESS_CLASSW, WS_VISIBLE );
    return ERROR_SUCCESS;
}

/******************** Path Edit ********************************************/

struct msi_pathedit_info
{
    msi_dialog *dialog;
    msi_control *control;
    WNDPROC oldproc;
};

static LPWSTR msi_get_window_text( HWND hwnd )
{
    UINT sz, r;
    LPWSTR buf;

    sz = 0x20;
    buf = msi_alloc( sz*sizeof(WCHAR) );
    while ( buf )
    {
        r = GetWindowTextW( hwnd, buf, sz );
        if ( r < (sz - 1) )
            break;
        sz *= 2;
        buf = msi_realloc( buf, sz*sizeof(WCHAR) );
    }

    return buf;
}

static void msi_dialog_update_pathedit( msi_dialog *dialog, msi_control *control )
{
    LPWSTR prop, path;
    BOOL indirect;

    if (!control && !(control = msi_dialog_find_control_by_type( dialog, szPathEdit )))
       return;

    indirect = control->attributes & msidbControlAttributesIndirect;
    prop = msi_dialog_dup_property( dialog, control->property, indirect );
    path = msi_dialog_dup_property( dialog, prop, TRUE );

    SetWindowTextW( control->hwnd, path );
    SendMessageW( control->hwnd, EM_SETSEL, 0, -1 );

    msi_free( path );
    msi_free( prop );
}

/* FIXME: test when this should fail */
static BOOL msi_dialog_verify_path( LPWSTR path )
{
    if ( !lstrlenW( path ) )
        return FALSE;

    if ( PathIsRelativeW( path ) )
        return FALSE;

    return TRUE;
}

/* returns TRUE if the path is valid, FALSE otherwise */
static BOOL msi_dialog_onkillfocus( msi_dialog *dialog, msi_control *control )
{
    LPWSTR buf, prop;
    BOOL indirect;
    BOOL valid;

    indirect = control->attributes & msidbControlAttributesIndirect;
    prop = msi_dialog_dup_property( dialog, control->property, indirect );

    buf = msi_get_window_text( control->hwnd );

    if ( !msi_dialog_verify_path( buf ) )
    {
        /* FIXME: display an error message box */
        ERR("Invalid path %s\n", debugstr_w( buf ));
        valid = FALSE;
        SetFocus( control->hwnd );
    }
    else
    {
        valid = TRUE;
        MSI_SetPropertyW( dialog->package, prop, buf );
    }

    msi_dialog_update_pathedit( dialog, control );

    TRACE("edit %s contents changed, set %s\n", debugstr_w(control->name),
          debugstr_w(prop));

    msi_free( buf );
    msi_free( prop );

    return valid;
}

static LRESULT WINAPI MSIPathEdit_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct msi_pathedit_info *info = GetPropW(hWnd, szButtonData);
    LRESULT r = 0;

    TRACE("%p %04x %08x %08lx\n", hWnd, msg, wParam, lParam);

    if ( msg == WM_KILLFOCUS )
    {
        /* if the path is invalid, don't handle this message */
        if ( !msi_dialog_onkillfocus( info->dialog, info->control ) )
            return 0;
    }

    r = CallWindowProcW(info->oldproc, hWnd, msg, wParam, lParam);

    if ( msg == WM_NCDESTROY )
    {
        msi_free( info );
        RemovePropW( hWnd, szButtonData );
    }

    return r;
}

static UINT msi_dialog_pathedit_control( msi_dialog *dialog, MSIRECORD *rec )
{
    struct msi_pathedit_info *info;
    msi_control *control;
    LPCWSTR prop;

    info = msi_alloc( sizeof *info );
    if (!info)
        return ERROR_FUNCTION_FAILED;

    control = msi_dialog_add_control( dialog, rec, szEdit,
                                      WS_BORDER | WS_TABSTOP );
    control->attributes = MSI_RecordGetInteger( rec, 8 );
    prop = MSI_RecordGetString( rec, 9 );
    control->property = msi_dialog_dup_property( dialog, prop, FALSE );

    info->dialog = dialog;
    info->control = control;
    info->oldproc = (WNDPROC) SetWindowLongPtrW( control->hwnd, GWLP_WNDPROC,
                                                 (LONG_PTR)MSIPathEdit_WndProc );
    SetPropW( control->hwnd, szButtonData, info );

    msi_dialog_update_pathedit( dialog, control );

    return ERROR_SUCCESS;
}

/* radio buttons are a bit different from normal controls */
static UINT msi_dialog_create_radiobutton( MSIRECORD *rec, LPVOID param )
{
    radio_button_group_descr *group = (radio_button_group_descr *)param;
    msi_dialog *dialog = group->dialog;
    msi_control *control;
    LPCWSTR prop, text, name;
    DWORD style, attributes = group->attributes;

    style = WS_CHILD | BS_AUTORADIOBUTTON | BS_MULTILINE | WS_TABSTOP;
    name = MSI_RecordGetString( rec, 3 );
    text = MSI_RecordGetString( rec, 8 );
    if( attributes & 1 )
        style |= WS_VISIBLE;
    if( ~attributes & 2 )
        style |= WS_DISABLED;

    control = msi_dialog_create_window( dialog, rec, 0, szButton, name, text,
                                        style, group->parent->hwnd );
    if (!control)
        return ERROR_FUNCTION_FAILED;
    control->handler = msi_dialog_radiogroup_handler;

    if (!lstrcmpW(control->name, group->propval))
        SendMessageW(control->hwnd, BM_SETCHECK, BST_CHECKED, 0);

    prop = MSI_RecordGetString( rec, 1 );
    if( prop )
        control->property = strdupW( prop );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_radiogroup_control( msi_dialog *dialog, MSIRECORD *rec )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','R','a','d','i','o','B','u','t','t','o','n',' ',
        'W','H','E','R','E',' ',
           '`','P','r','o','p','e','r','t','y','`',' ','=',' ','\'','%','s','\'',0};
    UINT r;
    LPCWSTR prop;
    msi_control *control;
    MSIQUERY *view = NULL;
    radio_button_group_descr group;
    MSIPACKAGE *package = dialog->package;
    WNDPROC oldproc;

    prop = MSI_RecordGetString( rec, 9 );

    TRACE("%p %p %s\n", dialog, rec, debugstr_w( prop ));

    /* Create parent group box to hold radio buttons */
    control = msi_dialog_add_control( dialog, rec, szButton, BS_OWNERDRAW|WS_GROUP );
    if( !control )
        return ERROR_FUNCTION_FAILED;

    oldproc = (WNDPROC) SetWindowLongPtrW( control->hwnd, GWLP_WNDPROC,
                                           (LONG_PTR)MSIRadioGroup_WndProc );
    SetPropW(control->hwnd, szButtonData, oldproc);
    SetWindowLongPtrW( control->hwnd, GWL_EXSTYLE, WS_EX_CONTROLPARENT );

    if( prop )
        control->property = strdupW( prop );

    /* query the Radio Button table for all control in this group */
    r = MSI_OpenQuery( package->db, &view, query, prop );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed for dialog %s radio group %s\n", 
            debugstr_w(dialog->name), debugstr_w(prop));
        return ERROR_INVALID_PARAMETER;
    }

    group.dialog = dialog;
    group.parent = control;
    group.attributes = MSI_RecordGetInteger( rec, 8 );
    group.propval = msi_dup_property( dialog->package, control->property );

    r = MSI_IterateRecords( view, 0, msi_dialog_create_radiobutton, &group );
    msiobj_release( &view->hdr );
    msi_free( group.propval );

    return r;
}

/******************** Selection Tree ***************************************/

struct msi_selection_tree_info
{
    msi_dialog *dialog;
    HWND hwnd;
    WNDPROC oldproc;
    HTREEITEM selected;
};

static void
msi_seltree_sync_item_state( HWND hwnd, MSIFEATURE *feature, HTREEITEM hItem )
{
    TVITEMW tvi;
    DWORD index = feature->Action;

    TRACE("Feature %s -> %d %d %d\n", debugstr_w(feature->Title),
        feature->Installed, feature->Action, feature->ActionRequest);

    if (index == INSTALLSTATE_UNKNOWN)
        index = INSTALLSTATE_ABSENT;

    tvi.mask = TVIF_STATE;
    tvi.hItem = hItem;
    tvi.state = INDEXTOSTATEIMAGEMASK( index );
    tvi.stateMask = TVIS_STATEIMAGEMASK;

    SendMessageW( hwnd, TVM_SETITEMW, 0, (LPARAM) &tvi );
}

static UINT
msi_seltree_popup_menu( HWND hwnd, INT x, INT y )
{
    HMENU hMenu;
    INT r;

    /* create a menu to display */
    hMenu = CreatePopupMenu();

    /* FIXME: load strings from resources */
    AppendMenuA( hMenu, MF_ENABLED, INSTALLSTATE_LOCAL, "Install feature locally");
    AppendMenuA( hMenu, MF_GRAYED, 0x1000, "Install entire feature");
    AppendMenuA( hMenu, MF_ENABLED, INSTALLSTATE_ADVERTISED, "Install on demand");
    AppendMenuA( hMenu, MF_ENABLED, INSTALLSTATE_ABSENT, "Don't install");
    r = TrackPopupMenu( hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
                        x, y, 0, hwnd, NULL );
    DestroyMenu( hMenu );
    return r;
}

static MSIFEATURE *
msi_seltree_feature_from_item( HWND hwnd, HTREEITEM hItem )
{
    TVITEMW tvi;

    /* get the feature from the item */
    memset( &tvi, 0, sizeof tvi );
    tvi.hItem = hItem;
    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
    SendMessageW( hwnd, TVM_GETITEMW, 0, (LPARAM) &tvi );

    return (MSIFEATURE*) tvi.lParam;
}

static LRESULT
msi_seltree_menu( HWND hwnd, HTREEITEM hItem )
{
    struct msi_selection_tree_info *info;
    MSIFEATURE *feature;
    MSIPACKAGE *package;
    union {
        RECT rc;
        POINT pt[2];
        HTREEITEM hItem;
    } u;
    UINT r;

    info = GetPropW(hwnd, szButtonData);
    package = info->dialog->package;

    feature = msi_seltree_feature_from_item( hwnd, hItem );
    if (!feature)
    {
        ERR("item %p feature was NULL\n", hItem);
        return 0;
    }

    /* get the item's rectangle to put the menu just below it */
    u.hItem = hItem;
    SendMessageW( hwnd, TVM_GETITEMRECT, 0, (LPARAM) &u.rc );
    MapWindowPoints( hwnd, NULL, u.pt, 2 );

    r = msi_seltree_popup_menu( hwnd, u.rc.left, u.rc.top );

    switch (r)
    {
    case INSTALLSTATE_LOCAL:
    case INSTALLSTATE_ADVERTISED:
    case INSTALLSTATE_ABSENT:
        msi_feature_set_state( feature, r );
        break;
    default:
        FIXME("select feature and all children\n");
    }

    /* update */
    msi_seltree_sync_item_state( hwnd, feature, hItem );
    ACTION_UpdateComponentStates( package, feature->Feature );

    return 0;
}

static MSIFEATURE *msi_seltree_get_selected_feature( msi_control *control )
{
    struct msi_selection_tree_info *info = GetPropW(control->hwnd, szButtonData);
    return msi_seltree_feature_from_item( control->hwnd, info->selected );
}

static LRESULT WINAPI
MSISelectionTree_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct msi_selection_tree_info *info;
    TVHITTESTINFO tvhti;
    HRESULT r;

    TRACE("%p %04x %08x %08lx\n", hWnd, msg, wParam, lParam);

    info = GetPropW(hWnd, szButtonData);

    switch( msg )
    {
    case WM_LBUTTONDOWN:
        tvhti.pt.x = (short)LOWORD( lParam );
        tvhti.pt.y = (short)HIWORD( lParam );
        tvhti.flags = 0;
        tvhti.hItem = 0;
        r = CallWindowProcW(info->oldproc, hWnd, TVM_HITTEST, 0, (LPARAM) &tvhti );
        if (tvhti.flags & TVHT_ONITEMSTATEICON)
            return msi_seltree_menu( hWnd, tvhti.hItem );
        break;
    }

    r = CallWindowProcW(info->oldproc, hWnd, msg, wParam, lParam);

    switch( msg )
    {
    case WM_NCDESTROY:
        msi_free( info );
        RemovePropW( hWnd, szButtonData );
        break;
    }
    return r;
}

static void
msi_seltree_add_child_features( MSIPACKAGE *package, HWND hwnd,
                                LPCWSTR parent, HTREEITEM hParent )
{
    struct msi_selection_tree_info *info = GetPropW( hwnd, szButtonData );
    MSIFEATURE *feature;
    TVINSERTSTRUCTW tvis;
    HTREEITEM hitem, hfirst = NULL;

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        if ( lstrcmpW( parent, feature->Feature_Parent ) )
            continue;

        if ( !feature->Title )
            continue;

        if ( !feature->Display )
            continue;

        memset( &tvis, 0, sizeof tvis );
        tvis.hParent = hParent;
        tvis.hInsertAfter = TVI_LAST;
        tvis.u.item.mask = TVIF_TEXT | TVIF_PARAM;
        tvis.u.item.pszText = feature->Title;
        tvis.u.item.lParam = (LPARAM) feature;

        hitem = (HTREEITEM) SendMessageW( hwnd, TVM_INSERTITEMW, 0, (LPARAM) &tvis );
        if (!hitem)
            continue;

        if (!hfirst)
            hfirst = hitem;

        msi_seltree_sync_item_state( hwnd, feature, hitem );
        msi_seltree_add_child_features( package, hwnd,
                                        feature->Feature, hitem );

        /* the node is expanded if Display is odd */
        if ( feature->Display % 2 != 0 )
            SendMessageW( hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM) hitem );
    }

    /* select the first item */
    SendMessageW( hwnd, TVM_SELECTITEM, TVGN_CARET | TVGN_DROPHILITE, (LPARAM) hfirst );
    info->selected = hfirst;
}

static void msi_seltree_create_imagelist( HWND hwnd )
{
    const int bm_width = 32, bm_height = 16, bm_count = 3;
    const int bm_resource = 0x1001;
    HIMAGELIST himl;
    int i;
    HBITMAP hbmp;

    himl = ImageList_Create( bm_width, bm_height, FALSE, 4, 0 );
    if (!himl)
    {
        ERR("failed to create image list\n");
        return;
    }

    for (i=0; i<bm_count; i++)
    {
        hbmp = LoadBitmapW( msi_hInstance, MAKEINTRESOURCEW(i+bm_resource) );
        if (!hbmp)
        {
            ERR("failed to load bitmap %d\n", i);
            break;
        }

        /*
         * Add a dummy bitmap at offset zero because the treeview
         * can't use it as a state mask (zero means no user state).
         */
        if (!i)
            ImageList_Add( himl, hbmp, NULL );

        ImageList_Add( himl, hbmp, NULL );
    }

    SendMessageW( hwnd, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)himl );
}

static UINT msi_dialog_seltree_handler( msi_dialog *dialog,
                                        msi_control *control, WPARAM param )
{
    struct msi_selection_tree_info *info = GetPropW( control->hwnd, szButtonData );
    LPNMTREEVIEWW tv = (LPNMTREEVIEWW)param;
    MSIRECORD *row, *rec;
    MSIFOLDER *folder;
    LPCWSTR dir;
    UINT r = ERROR_SUCCESS;

    static const WCHAR select[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','F','e','a','t','u','r','e','`',' ','W','H','E','R','E',' ',
        '`','T','i','t','l','e','`',' ','=',' ','\'','%','s','\'',0
    };

    if (tv->hdr.code != TVN_SELCHANGINGW)
        return ERROR_SUCCESS;

    info->selected = tv->itemNew.hItem;

    row = MSI_QueryGetRecord( dialog->package->db, select, tv->itemNew.pszText );
    if (!row)
        return ERROR_FUNCTION_FAILED;

    rec = MSI_CreateRecord( 1 );

    MSI_RecordSetStringW( rec, 1, MSI_RecordGetString( row, 4 ) );
    ControlEvent_FireSubscribedEvent( dialog->package, szSelectionDescription, rec );

    dir = MSI_RecordGetString( row, 7 );
    folder = get_loaded_folder( dialog->package, dir );
    if (!folder)
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    MSI_RecordSetStringW( rec, 1, folder->ResolvedTarget );
    ControlEvent_FireSubscribedEvent( dialog->package, szSelectionPath, rec );

done:
    msiobj_release(&row->hdr);
    msiobj_release(&rec->hdr);

    return r;
}

static UINT msi_dialog_selection_tree( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    LPCWSTR prop;
    MSIPACKAGE *package = dialog->package;
    DWORD style;
    struct msi_selection_tree_info *info;

    info = msi_alloc( sizeof *info );
    if (!info)
        return ERROR_FUNCTION_FAILED;

    /* create the treeview control */
    style = TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT;
    style |= WS_GROUP | WS_VSCROLL;
    control = msi_dialog_add_control( dialog, rec, WC_TREEVIEWW, style );
    if (!control)
    {
        msi_free(info);
        return ERROR_FUNCTION_FAILED;
    }

    control->handler = msi_dialog_seltree_handler;
    control->attributes = MSI_RecordGetInteger( rec, 8 );
    prop = MSI_RecordGetString( rec, 9 );
    control->property = msi_dialog_dup_property( dialog, prop, FALSE );

    /* subclass */
    info->dialog = dialog;
    info->hwnd = control->hwnd;
    info->oldproc = (WNDPROC) SetWindowLongPtrW( control->hwnd, GWLP_WNDPROC,
                                          (LONG_PTR)MSISelectionTree_WndProc );
    SetPropW( control->hwnd, szButtonData, info );

    ControlEvent_SubscribeToEvent( dialog->package, dialog,
                                   szSelectionPath, control->name, szProperty );

    /* initialize it */
    msi_seltree_create_imagelist( control->hwnd );
    msi_seltree_add_child_features( package, control->hwnd, NULL, NULL );

    return ERROR_SUCCESS;
}

/******************** Group Box ***************************************/

static UINT msi_dialog_group_box( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    DWORD style;

    style = BS_GROUPBOX | WS_CHILD | WS_GROUP;
    control = msi_dialog_add_control( dialog, rec, WC_BUTTONW, style );
    if (!control)
        return ERROR_FUNCTION_FAILED;

    return ERROR_SUCCESS;
}

/******************** List Box ***************************************/

struct msi_listbox_item
{
    LPWSTR property;
    LPWSTR value;
};

struct msi_listbox_info
{
    msi_dialog *dialog;
    HWND hwnd;
    WNDPROC oldproc;
    DWORD num_items;
    struct msi_listbox_item *items;
};

static LRESULT WINAPI MSIListBox_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct msi_listbox_info *info;
    LRESULT r;
    DWORD j;

    TRACE("%p %04x %08x %08lx\n", hWnd, msg, wParam, lParam);

    info = GetPropW( hWnd, szButtonData );
    if (!info)
        return 0;

    r = CallWindowProcW( info->oldproc, hWnd, msg, wParam, lParam );

    switch( msg )
    {
    case WM_NCDESTROY:
        for (j = 0; j < info->num_items; j++)
        {
            msi_free( info->items[j].property );
            msi_free( info->items[j].value );
        }
        msi_free( info->items );
        msi_free( info );
        RemovePropW( hWnd, szButtonData );
        break;
    }

    return r;
}

static UINT msi_listbox_add_item( MSIRECORD *rec, LPVOID param )
{
    struct msi_listbox_info *info = param;
    struct msi_listbox_item *item;
    LPCWSTR property, value, text;
    static int index = 0;

    item = &info->items[index++];
    property = MSI_RecordGetString( rec, 1 );
    value = MSI_RecordGetString( rec, 3 );
    text = MSI_RecordGetString( rec, 4 );

    item->property = strdupW( property );
    item->value = strdupW( value );

    SendMessageW( info->hwnd, LB_ADDSTRING, 0, (LPARAM)text );

    return ERROR_SUCCESS;
}

static UINT msi_listbox_add_items( struct msi_listbox_info *info )
{
    UINT r;
    MSIQUERY *view = NULL;
    DWORD count;

    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','`','L','i','s','t','B','o','x','`',' ',
        'O','R','D','E','R',' ','B','Y',' ','`','O','r','d','e','r','`',0
    };

    r = MSI_OpenQuery( info->dialog->package->db, &view, query );
    if ( r != ERROR_SUCCESS )
        return r;

    /* just get the number of records */
    r = MSI_IterateRecords( view, &count, NULL, NULL );

    info->num_items = count;
    info->items = msi_alloc( sizeof(*info->items) * count );

    r = MSI_IterateRecords( view, NULL, msi_listbox_add_item, info );
    msiobj_release( &view->hdr );

    return r;
}

static UINT msi_dialog_listbox_handler( msi_dialog *dialog,
                                        msi_control *control, WPARAM param )
{
    struct msi_listbox_info *info;
    int index;

    if( HIWORD(param) != LBN_SELCHANGE )
        return ERROR_SUCCESS;

    info = GetPropW( control->hwnd, szButtonData );
    index = SendMessageW( control->hwnd, LB_GETCURSEL, 0, 0 );

    MSI_SetPropertyW( info->dialog->package,
                      info->items[index].property, info->items[index].value );
    msi_dialog_evaluate_control_conditions( info->dialog );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_list_box( msi_dialog *dialog, MSIRECORD *rec )
{
    struct msi_listbox_info *info;
    msi_control *control;
    DWORD style;

    info = msi_alloc( sizeof *info );
    if (!info)
        return ERROR_FUNCTION_FAILED;

    style = WS_TABSTOP | WS_GROUP | WS_CHILD | LBS_STANDARD;
    control = msi_dialog_add_control( dialog, rec, WC_LISTBOXW, style );
    if (!control)
        return ERROR_FUNCTION_FAILED;

    control->handler = msi_dialog_listbox_handler;

    /* subclass */
    info->dialog = dialog;
    info->hwnd = control->hwnd;
    info->items = NULL;
    info->oldproc = (WNDPROC)SetWindowLongPtrW( control->hwnd, GWLP_WNDPROC,
                                                (LONG_PTR)MSIListBox_WndProc );
    SetPropW( control->hwnd, szButtonData, info );

    msi_listbox_add_items( info );

    return ERROR_SUCCESS;
}

/******************** Directory Combo ***************************************/

static void msi_dialog_update_directory_combo( msi_dialog *dialog, msi_control *control )
{
    LPWSTR prop, path;
    BOOL indirect;

    if (!control && !(control = msi_dialog_find_control_by_type( dialog, szDirectoryCombo )))
        return;

    indirect = control->attributes & msidbControlAttributesIndirect;
    prop = msi_dialog_dup_property( dialog, control->property, indirect );
    path = msi_dialog_dup_property( dialog, prop, TRUE );

    PathStripPathW( path );
    PathRemoveBackslashW( path );

    SendMessageW( control->hwnd, CB_INSERTSTRING, 0, (LPARAM)path );
    SendMessageW( control->hwnd, CB_SETCURSEL, 0, 0 );

    msi_free( path );
    msi_free( prop );
}

static UINT msi_dialog_directory_combo( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    LPCWSTR prop;
    DWORD style;

    /* FIXME: use CBS_OWNERDRAWFIXED and add owner draw code */
    style = CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD |
            WS_GROUP | WS_TABSTOP | WS_VSCROLL;
    control = msi_dialog_add_control( dialog, rec, WC_COMBOBOXW, style );
    if (!control)
        return ERROR_FUNCTION_FAILED;

    control->attributes = MSI_RecordGetInteger( rec, 8 );
    prop = MSI_RecordGetString( rec, 9 );
    control->property = msi_dialog_dup_property( dialog, prop, FALSE );

    msi_dialog_update_directory_combo( dialog, control );

    return ERROR_SUCCESS;
}

/******************** Directory List ***************************************/

static void msi_dialog_update_directory_list( msi_dialog *dialog, msi_control *control )
{
    WCHAR dir_spec[MAX_PATH];
    WIN32_FIND_DATAW wfd;
    LPWSTR prop, path;
    BOOL indirect;
    LVITEMW item;
    HANDLE file;

    static const WCHAR asterisk[] = {'*',0};
    static const WCHAR dot[] = {'.',0};
    static const WCHAR dotdot[] = {'.','.',0};

    if (!control && !(control = msi_dialog_find_control_by_type( dialog, szDirectoryList )))
        return;

    /* clear the list-view */
    SendMessageW( control->hwnd, LVM_DELETEALLITEMS, 0, 0 );

    indirect = control->attributes & msidbControlAttributesIndirect;
    prop = msi_dialog_dup_property( dialog, control->property, indirect );
    path = msi_dialog_dup_property( dialog, prop, TRUE );

    lstrcpyW( dir_spec, path );
    lstrcatW( dir_spec, asterisk );

    file = FindFirstFileW( dir_spec, &wfd );
    if ( file == INVALID_HANDLE_VALUE )
        return;

    do
    {
        if ( wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY )
            continue;

        if ( !lstrcmpW( wfd.cFileName, dot ) || !lstrcmpW( wfd.cFileName, dotdot ) )
            continue;

        item.mask = LVIF_TEXT;
        item.cchTextMax = MAX_PATH;
        item.iItem = 0;
        item.iSubItem = 0;
        item.pszText = wfd.cFileName;

        SendMessageW( control->hwnd, LVM_INSERTITEMW, 0, (LPARAM)&item );
    } while ( FindNextFileW( file, &wfd ) );

    msi_free( prop );
    msi_free( path );
    FindClose( file );
}

UINT msi_dialog_directorylist_up( msi_dialog *dialog )
{
    msi_control *control;
    LPWSTR prop, path, ptr;
    BOOL indirect;

    control = msi_dialog_find_control_by_type( dialog, szDirectoryList );
    indirect = control->attributes & msidbControlAttributesIndirect;
    prop = msi_dialog_dup_property( dialog, control->property, indirect );
    path = msi_dialog_dup_property( dialog, prop, TRUE );

    /* strip off the last directory */
    ptr = PathFindFileNameW( path );
    if (ptr != path) *(ptr - 1) = '\0';
    PathAddBackslashW( path );

    MSI_SetPropertyW( dialog->package, prop, path );

    msi_dialog_update_directory_list( dialog, NULL );
    msi_dialog_update_directory_combo( dialog, NULL );
    msi_dialog_update_pathedit( dialog, NULL );

    msi_free( path );
    msi_free( prop );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_dirlist_handler( msi_dialog *dialog,
                                        msi_control *control, WPARAM param )
{
    LPNMHDR nmhdr = (LPNMHDR)param;
    WCHAR new_path[MAX_PATH];
    WCHAR text[MAX_PATH];
    LPWSTR path, prop;
    BOOL indirect;
    LVITEMW item;
    int index;

    static const WCHAR backslash[] = {'\\',0};

    if (nmhdr->code != LVN_ITEMACTIVATE)
        return ERROR_SUCCESS;

    index = SendMessageW( control->hwnd, LVM_GETNEXTITEM, -1, LVNI_SELECTED );
    if ( index < 0 )
    {
        ERR("No list-view item selected!\n");
        return ERROR_FUNCTION_FAILED;
    }

    item.iSubItem = 0;
    item.pszText = text;
    item.cchTextMax = MAX_PATH;
    SendMessageW( control->hwnd, LVM_GETITEMTEXTW, index, (LPARAM)&item );

    indirect = control->attributes & msidbControlAttributesIndirect;
    prop = msi_dialog_dup_property( dialog, control->property, indirect );
    path = msi_dialog_dup_property( dialog, prop, TRUE );

    lstrcpyW( new_path, path );
    lstrcatW( new_path, text );
    lstrcatW( new_path, backslash );

    MSI_SetPropertyW( dialog->package, prop, new_path );

    msi_dialog_update_directory_list( dialog, NULL );
    msi_dialog_update_directory_combo( dialog, NULL );
    msi_dialog_update_pathedit( dialog, NULL );

    msi_free( prop );
    msi_free( path );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_directory_list( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    LPCWSTR prop;
    DWORD style;

    style = LVS_LIST | WS_VSCROLL | LVS_SHAREIMAGELISTS |
            LVS_AUTOARRANGE | LVS_SINGLESEL | WS_BORDER |
            LVS_SORTASCENDING | WS_CHILD | WS_GROUP | WS_TABSTOP;
    control = msi_dialog_add_control( dialog, rec, WC_LISTVIEWW, style );
    if (!control)
        return ERROR_FUNCTION_FAILED;

    control->attributes = MSI_RecordGetInteger( rec, 8 );
    control->handler = msi_dialog_dirlist_handler;
    prop = MSI_RecordGetString( rec, 9 );
    control->property = msi_dialog_dup_property( dialog, prop, FALSE );

    /* double click to activate an item in the list */
    SendMessageW( control->hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE,
                  0, LVS_EX_TWOCLICKACTIVATE );

    msi_dialog_update_directory_list( dialog, control );

    return ERROR_SUCCESS;
}

/******************** VolumeCost List ***************************************/

static BOOL str_is_number( LPCWSTR str )
{
    int i;

    for (i = 0; i < lstrlenW( str ); i++)
        if (!isdigitW(str[i]))
            return FALSE;

    return TRUE;
}

WCHAR column_keys[][80] =
{
    {'V','o','l','u','m','e','C','o','s','t','V','o','l','u','m','e',0},
    {'V','o','l','u','m','e','C','o','s','t','S','i','z','e',0},
    {'V','o','l','u','m','e','C','o','s','t','A','v','a','i','l','a','b','l','e',0},
    {'V','o','l','u','m','e','C','o','s','t','R','e','q','u','i','r','e','d',0},
    {'V','o','l','u','m','e','C','o','s','t','D','i','f','f','e','r','e','n','c','e',0}
};

static void msi_dialog_vcl_add_columns( msi_dialog *dialog, msi_control *control, MSIRECORD *rec )
{
    LPCWSTR text = MSI_RecordGetString( rec, 10 );
    LPCWSTR begin = text, end;
    WCHAR num[10];
    LVCOLUMNW lvc;
    DWORD count = 0;

    static const WCHAR zero[] = {'0',0};
    static const WCHAR negative[] = {'-',0};

    while ((begin = strchrW( begin, '{' )) && count < 5)
    {
        if (!(end = strchrW( begin, '}' )))
            return;

        lstrcpynW( num, begin + 1, end - begin );
        begin += end - begin + 1;

        /* empty braces or '0' hides the column */ 
        if ( !num[0] || !lstrcmpW( num, zero ) )
        {
            count++;
            continue;
        }

        /* the width must be a positive number
         * if a width is invalid, all remaining columns are hidden
         */
        if ( !strncmpW( num, negative, 1 ) || !str_is_number( num ) )
            return;

        ZeroMemory( &lvc, sizeof(lvc) );
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.cx = atolW( num );
        lvc.pszText = msi_dialog_get_uitext( dialog, column_keys[count] );

        SendMessageW( control->hwnd,  LVM_INSERTCOLUMNW, count++, (LPARAM)&lvc );
        msi_free( lvc.pszText );
    }
}

static void msi_dialog_vcl_add_drives( msi_dialog *dialog, msi_control *control )
{
    ULARGE_INTEGER total, free;
    WCHAR size_text[MAX_PATH];
    LPWSTR drives, ptr;
    LVITEMW lvitem;
    DWORD size;
    int i = 0;

    size = GetLogicalDriveStringsW( 0, NULL );
    if ( !size ) return;

    drives = msi_alloc( (size + 1) * sizeof(WCHAR) );
    if ( !drives ) return;

    GetLogicalDriveStringsW( size, drives );

    ptr = drives;
    while (*ptr)
    {
        lvitem.mask = LVIF_TEXT;
        lvitem.iItem = i;
        lvitem.iSubItem = 0;
        lvitem.pszText = ptr;
        lvitem.cchTextMax = lstrlenW(ptr) + 1;
        SendMessageW( control->hwnd, LVM_INSERTITEMW, 0, (LPARAM)&lvitem );

        GetDiskFreeSpaceExW(ptr, &free, &total, NULL);

        StrFormatByteSizeW(total.QuadPart, size_text, MAX_PATH);
        lvitem.iSubItem = 1;
        lvitem.pszText = size_text;
        lvitem.cchTextMax = lstrlenW(size_text) + 1;
        SendMessageW( control->hwnd, LVM_SETITEMW, 0, (LPARAM)&lvitem );

        StrFormatByteSizeW(free.QuadPart, size_text, MAX_PATH);
        lvitem.iSubItem = 2;
        lvitem.pszText = size_text;
        lvitem.cchTextMax = lstrlenW(size_text) + 1;
        SendMessageW( control->hwnd, LVM_SETITEMW, 0, (LPARAM)&lvitem );

        ptr += lstrlenW(ptr) + 1;
        i++;
    }

    msi_free( drives );
}

static UINT msi_dialog_volumecost_list( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    DWORD style;

    style = LVS_REPORT | WS_VSCROLL | WS_HSCROLL | LVS_SHAREIMAGELISTS |
            LVS_AUTOARRANGE | LVS_SINGLESEL | WS_BORDER |
            WS_CHILD | WS_TABSTOP | WS_GROUP;
    control = msi_dialog_add_control( dialog, rec, WC_LISTVIEWW, style );
    if (!control)
        return ERROR_FUNCTION_FAILED;

    msi_dialog_vcl_add_columns( dialog, control, rec );
    msi_dialog_vcl_add_drives( dialog, control );

    return ERROR_SUCCESS;
}

static const struct control_handler msi_dialog_handler[] =
{
    { szText, msi_dialog_text_control },
    { szPushButton, msi_dialog_button_control },
    { szLine, msi_dialog_line_control },
    { szBitmap, msi_dialog_bitmap_control },
    { szCheckBox, msi_dialog_checkbox_control },
    { szScrollableText, msi_dialog_scrolltext_control },
    { szComboBox, msi_dialog_combo_control },
    { szEdit, msi_dialog_edit_control },
    { szMaskedEdit, msi_dialog_maskedit_control },
    { szPathEdit, msi_dialog_pathedit_control },
    { szProgressBar, msi_dialog_progress_bar },
    { szRadioButtonGroup, msi_dialog_radiogroup_control },
    { szIcon, msi_dialog_icon_control },
    { szSelectionTree, msi_dialog_selection_tree },
    { szGroupBox, msi_dialog_group_box },
    { szListBox, msi_dialog_list_box },
    { szDirectoryCombo, msi_dialog_directory_combo },
    { szDirectoryList, msi_dialog_directory_list },
    { szVolumeCostList, msi_dialog_volumecost_list },
};

#define NUM_CONTROL_TYPES (sizeof msi_dialog_handler/sizeof msi_dialog_handler[0])

static UINT msi_dialog_create_controls( MSIRECORD *rec, LPVOID param )
{
    msi_dialog *dialog = param;
    LPCWSTR control_type;
    UINT i;

    /* find and call the function that can create this type of control */
    control_type = MSI_RecordGetString( rec, 3 );
    for( i=0; i<NUM_CONTROL_TYPES; i++ )
        if (!strcmpiW( msi_dialog_handler[i].control_type, control_type ))
            break;
    if( i != NUM_CONTROL_TYPES )
        msi_dialog_handler[i].func( dialog, rec );
    else
        ERR("no handler for element type %s\n", debugstr_w(control_type));

    return ERROR_SUCCESS;
}

static UINT msi_dialog_fill_controls( msi_dialog *dialog )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','C','o','n','t','r','o','l',' ',
        'W','H','E','R','E',' ',
           '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',0};
    UINT r;
    MSIQUERY *view = NULL;
    MSIPACKAGE *package = dialog->package;

    TRACE("%p %s\n", dialog, debugstr_w(dialog->name) );

    /* query the Control table for all the elements of the control */
    r = MSI_OpenQuery( package->db, &view, query, dialog->name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed for dialog %s\n", debugstr_w(dialog->name));
        return ERROR_INVALID_PARAMETER;
    }

    r = MSI_IterateRecords( view, 0, msi_dialog_create_controls, dialog );
    msiobj_release( &view->hdr );

    return r;
}

static UINT msi_dialog_set_control_condition( MSIRECORD *rec, LPVOID param )
{
    static const WCHAR szHide[] = { 'H','i','d','e',0 };
    static const WCHAR szShow[] = { 'S','h','o','w',0 };
    static const WCHAR szDisable[] = { 'D','i','s','a','b','l','e',0 };
    static const WCHAR szEnable[] = { 'E','n','a','b','l','e',0 };
    msi_dialog *dialog = param;
    msi_control *control;
    LPCWSTR name, action, condition;
    UINT r;

    name = MSI_RecordGetString( rec, 2 );
    action = MSI_RecordGetString( rec, 3 );
    condition = MSI_RecordGetString( rec, 4 );
    r = MSI_EvaluateConditionW( dialog->package, condition );
    control = msi_dialog_find_control( dialog, name );
    if( r == MSICONDITION_TRUE && control )
    {
        TRACE("%s control %s\n", debugstr_w(action), debugstr_w(name));

        /* FIXME: case sensitive? */
        if(!lstrcmpW(action, szHide))
            ShowWindow(control->hwnd, SW_HIDE);
        else if(!strcmpW(action, szShow))
            ShowWindow(control->hwnd, SW_SHOW);
        else if(!strcmpW(action, szDisable))
            EnableWindow(control->hwnd, FALSE);
        else if(!strcmpW(action, szEnable))
            EnableWindow(control->hwnd, TRUE);
        else
            FIXME("Unhandled action %s\n", debugstr_w(action));
    }

    return ERROR_SUCCESS;
}

static UINT msi_dialog_evaluate_control_conditions( msi_dialog *dialog )
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','*',' ',
      'F','R','O','M',' ',
        'C','o','n','t','r','o','l','C','o','n','d','i','t','i','o','n',' ',
      'W','H','E','R','E',' ',
        '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',0
    };
    UINT r;
    MSIQUERY *view = NULL;
    MSIPACKAGE *package = dialog->package;

    TRACE("%p %s\n", dialog, debugstr_w(dialog->name) );

    /* query the Control table for all the elements of the control */
    r = MSI_OpenQuery( package->db, &view, query, dialog->name );
    if( r != ERROR_SUCCESS )
        return ERROR_SUCCESS;

    r = MSI_IterateRecords( view, 0, msi_dialog_set_control_condition, dialog );
    msiobj_release( &view->hdr );

    return r;
}

UINT msi_dialog_reset( msi_dialog *dialog )
{
    /* FIXME: should restore the original values of any properties we changed */
    return msi_dialog_evaluate_control_conditions( dialog );
}

/* figure out the height of 10 point MS Sans Serif */
static INT msi_dialog_get_sans_serif_height( HWND hwnd )
{
    static const WCHAR szSansSerif[] = {
        'M','S',' ','S','a','n','s',' ','S','e','r','i','f',0 };
    LOGFONTW lf;
    TEXTMETRICW tm;
    BOOL r;
    LONG height = 0;
    HFONT hFont, hOldFont;
    HDC hdc;

    hdc = GetDC( hwnd );
    if (hdc)
    {
        memset( &lf, 0, sizeof lf );
        lf.lfHeight = MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        strcpyW( lf.lfFaceName, szSansSerif );
        hFont = CreateFontIndirectW(&lf);
        if (hFont)
        {
            hOldFont = SelectObject( hdc, hFont );
            r = GetTextMetricsW( hdc, &tm );
            if (r)
                height = tm.tmHeight;
            SelectObject( hdc, hOldFont );
            DeleteObject( hFont );
        }
        ReleaseDC( hwnd, hdc );
    }
    return height;
}

/* fetch the associated record from the Dialog table */
static MSIRECORD *msi_get_dialog_record( msi_dialog *dialog )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','D','i','a','l','o','g',' ',
        'W','H','E','R','E',' ',
           '`','D','i','a','l','o','g','`',' ','=',' ','\'','%','s','\'',0};
    MSIPACKAGE *package = dialog->package;
    MSIRECORD *rec = NULL;

    TRACE("%p %s\n", dialog, debugstr_w(dialog->name) );

    rec = MSI_QueryGetRecord( package->db, query, dialog->name );
    if( !rec )
        ERR("query failed for dialog %s\n", debugstr_w(dialog->name));

    return rec;
}

static void msi_dialog_adjust_dialog_pos( msi_dialog *dialog, MSIRECORD *rec, LPRECT pos )
{
    static const WCHAR szScreenX[] = {'S','c','r','e','e','n','X',0};
    static const WCHAR szScreenY[] = {'S','c','r','e','e','n','Y',0};

    UINT xres, yres;
    POINT center;
    SIZE sz;
    LONG style;

    center.x = MSI_RecordGetInteger( rec, 2 );
    center.y = MSI_RecordGetInteger( rec, 3 );

    sz.cx = MSI_RecordGetInteger( rec, 4 );
    sz.cy = MSI_RecordGetInteger( rec, 5 );

    sz.cx = msi_dialog_scale_unit( dialog, sz.cx );
    sz.cy = msi_dialog_scale_unit( dialog, sz.cy );

    xres = msi_get_property_int( dialog->package, szScreenX, 0 );
    yres = msi_get_property_int( dialog->package, szScreenY, 0 );

    center.x = MulDiv( center.x, xres, 100 );
    center.y = MulDiv( center.y, yres, 100 );

    /* turn the client pos into the window rectangle */
    if (dialog->package->center_x && dialog->package->center_y)
    {
        pos->left = dialog->package->center_x - sz.cx / 2.0;
        pos->right = pos->left + sz.cx;
        pos->top = dialog->package->center_y - sz.cy / 2.0;
        pos->bottom = pos->top + sz.cy;
    }
    else
    {
        pos->left = center.x - sz.cx/2;
        pos->right = pos->left + sz.cx;
        pos->top = center.y - sz.cy/2;
        pos->bottom = pos->top + sz.cy;

        /* save the center */
        dialog->package->center_x = center.x;
        dialog->package->center_y = center.y;
    }

    dialog->size.cx = sz.cx;
    dialog->size.cy = sz.cy;

    TRACE("%u %u %u %u\n", pos->left, pos->top, pos->right, pos->bottom);

    style = GetWindowLongPtrW( dialog->hwnd, GWL_STYLE );
    AdjustWindowRect( pos, style, FALSE );
}

static BOOL msi_control_set_next( msi_control *control, msi_control *next )
{
    return SetWindowPos( next->hwnd, control->hwnd, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOREDRAW |
                         SWP_NOREPOSITION | SWP_NOSENDCHANGING | SWP_NOSIZE );
}

static UINT msi_dialog_set_tab_order( msi_dialog *dialog )
{
    msi_control *control, *tab_next;

    LIST_FOR_EACH_ENTRY( control, &dialog->controls, msi_control, entry )
    {
        tab_next = msi_dialog_find_control( dialog, control->tabnext );
        if( !tab_next )
            continue;
        msi_control_set_next( control, tab_next );
    }

    return ERROR_SUCCESS;
}

static void msi_dialog_set_first_control( msi_dialog* dialog, LPCWSTR name )
{
    msi_control *control;

    control = msi_dialog_find_control( dialog, name );
    if( control )
        dialog->hWndFocus = control->hwnd;
    else
        dialog->hWndFocus = NULL;
}

static LRESULT msi_dialog_oncreate( HWND hwnd, LPCREATESTRUCTW cs )
{
    static const WCHAR df[] = {
        'D','e','f','a','u','l','t','U','I','F','o','n','t',0 };
    static const WCHAR dfv[] = {
        'M','S',' ','S','h','e','l','l',' ','D','l','g',0 };
    msi_dialog *dialog = (msi_dialog*) cs->lpCreateParams;
    MSIRECORD *rec = NULL;
    LPWSTR title = NULL;
    RECT pos;

    TRACE("%p %p\n", dialog, dialog->package);

    dialog->hwnd = hwnd;
    SetWindowLongPtrW( hwnd, GWLP_USERDATA, (LONG_PTR) dialog );

    rec = msi_get_dialog_record( dialog );
    if( !rec )
    {
        TRACE("No record found for dialog %s\n", debugstr_w(dialog->name));
        return -1;
    }

    dialog->scale = msi_dialog_get_sans_serif_height(dialog->hwnd);

    msi_dialog_adjust_dialog_pos( dialog, rec, &pos );

    dialog->attributes = MSI_RecordGetInteger( rec, 6 );

    dialog->default_font = msi_dup_property( dialog->package, df );
    if (!dialog->default_font)
    {
        dialog->default_font = strdupW(dfv);
        if (!dialog->default_font) return -1;
    }

    title = msi_get_deformatted_field( dialog->package, rec, 7 );
    SetWindowTextW( hwnd, title );
    msi_free( title );

    SetWindowPos( hwnd, 0, pos.left, pos.top,
                  pos.right - pos.left, pos.bottom - pos.top,
                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW );

    msi_dialog_build_font_list( dialog );
    msi_dialog_fill_controls( dialog );
    msi_dialog_evaluate_control_conditions( dialog );
    msi_dialog_set_tab_order( dialog );
    msi_dialog_set_first_control( dialog, MSI_RecordGetString( rec, 8 ) );
    msiobj_release( &rec->hdr );

    return 0;
}

static UINT msi_dialog_send_event( msi_dialog *dialog, LPCWSTR event, LPCWSTR arg )
{
    LPWSTR event_fmt = NULL, arg_fmt = NULL;

    TRACE("Sending control event %s %s\n", debugstr_w(event), debugstr_w(arg));

    deformat_string( dialog->package, event, &event_fmt );
    deformat_string( dialog->package, arg, &arg_fmt );

    dialog->event_handler( dialog->package, event_fmt, arg_fmt, dialog );

    msi_free( event_fmt );
    msi_free( arg_fmt );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_set_property( msi_dialog *dialog, LPCWSTR event, LPCWSTR arg )
{
    static const WCHAR szNullArg[] = { '{','}',0 };
    LPWSTR p, prop, arg_fmt = NULL;
    UINT len;

    len = strlenW(event);
    prop = msi_alloc( len*sizeof(WCHAR));
    strcpyW( prop, &event[1] );
    p = strchrW( prop, ']' );
    if( p && p[1] == 0 )
    {
        *p = 0;
        if( strcmpW( szNullArg, arg ) )
            deformat_string( dialog->package, arg, &arg_fmt );
        MSI_SetPropertyW( dialog->package, prop, arg_fmt );
        msi_free( arg_fmt );
    }
    else
        ERR("Badly formatted property string - what happens?\n");
    msi_free( prop );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_control_event( MSIRECORD *rec, LPVOID param )
{
    msi_dialog *dialog = param;
    LPCWSTR condition, event, arg;
    UINT r;

    condition = MSI_RecordGetString( rec, 5 );
    r = MSI_EvaluateConditionW( dialog->package, condition );
    if( r == MSICONDITION_TRUE || r == MSICONDITION_NONE )
    {
        event = MSI_RecordGetString( rec, 3 );
        arg = MSI_RecordGetString( rec, 4 );
        if( event[0] == '[' )
            msi_dialog_set_property( dialog, event, arg );
        else
            msi_dialog_send_event( dialog, event, arg );
    }

    return ERROR_SUCCESS;
}

struct rec_list
{
    struct list entry;
    MSIRECORD *rec;
};

static UINT add_rec_to_list( MSIRECORD *rec, LPVOID param )
{
    struct rec_list *add_rec;
    struct list *records = (struct list *)param;

    msiobj_addref( &rec->hdr );

    add_rec = msi_alloc( sizeof( *add_rec ) );
    if (!add_rec)
    {
        msiobj_release( &rec->hdr );
        return ERROR_OUTOFMEMORY;
    }

    add_rec->rec = rec;
    list_add_tail( records, &add_rec->entry );
    return ERROR_SUCCESS;
}

static inline void remove_rec_from_list( struct rec_list *rec_entry )
{
    msiobj_release( &rec_entry->rec->hdr );
    list_remove( &rec_entry->entry );
    msi_free( rec_entry );
}

static UINT msi_dialog_button_handler( msi_dialog *dialog,
                                       msi_control *control, WPARAM param )
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','*',' ',
      'F','R','O','M',' ','C','o','n','t','r','o','l','E','v','e','n','t',' ',
      'W','H','E','R','E',' ',
         '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',' ',
      'A','N','D',' ',
         '`','C','o','n','t','r','o','l','_','`',' ','=',' ','\'','%','s','\'',' ',
      'O','R','D','E','R',' ','B','Y',' ','`','O','r','d','e','r','i','n','g','`',0
    };
    MSIQUERY *view = NULL;
    struct rec_list *rec_entry, *next;
    struct list events;
    UINT r;

    if( HIWORD(param) != BN_CLICKED )
        return ERROR_SUCCESS;

    list_init( &events );

    r = MSI_OpenQuery( dialog->package->db, &view, query,
                       dialog->name, control->name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed\n");
        return 0;
    }

    r = MSI_IterateRecords( view, 0, add_rec_to_list, &events );
    msiobj_release( &view->hdr );
    if (r != ERROR_SUCCESS)
        goto done;

    /* handle all SetProperty events first */
    LIST_FOR_EACH_ENTRY_SAFE( rec_entry, next, &events, struct rec_list, entry )
    {
        LPCWSTR event = MSI_RecordGetString( rec_entry->rec, 3 );

        if ( event[0] != '[' )
            continue;

        r = msi_dialog_control_event( rec_entry->rec, dialog );
        remove_rec_from_list( rec_entry );

        if ( r != ERROR_SUCCESS )
            goto done;
    }    

    /* handle all other events */
    LIST_FOR_EACH_ENTRY_SAFE( rec_entry, next, &events, struct rec_list, entry )
    {
        r = msi_dialog_control_event( rec_entry->rec, dialog );
        remove_rec_from_list( rec_entry );

        if ( r != ERROR_SUCCESS )
            goto done;
    }

done:
    LIST_FOR_EACH_ENTRY_SAFE( rec_entry, next, &events, struct rec_list, entry )
    {
        remove_rec_from_list( rec_entry );
    }

    return r;
}

static UINT msi_dialog_get_checkbox_state( msi_dialog *dialog,
                msi_control *control )
{
    WCHAR state[2] = { 0 };
    DWORD sz = 2;

    MSI_GetPropertyW( dialog->package, control->property, state, &sz );
    return state[0] ? 1 : 0;
}

static void msi_dialog_set_checkbox_state( msi_dialog *dialog,
                msi_control *control, UINT state )
{
    static const WCHAR szState[] = { '1', 0 };
    LPCWSTR val;

    /* if uncheck then the property is set to NULL */
    if (!state)
    {
        MSI_SetPropertyW( dialog->package, control->property, NULL );
        return;
    }

    /* check for a custom state */
    if (control->value && control->value[0])
        val = control->value;
    else
        val = szState;

    MSI_SetPropertyW( dialog->package, control->property, val );
}

static void msi_dialog_checkbox_sync_state( msi_dialog *dialog,
                msi_control *control )
{
    UINT state;

    state = msi_dialog_get_checkbox_state( dialog, control );
    SendMessageW( control->hwnd, BM_SETCHECK,
                  state ? BST_CHECKED : BST_UNCHECKED, 0 );
}

static UINT msi_dialog_checkbox_handler( msi_dialog *dialog,
                msi_control *control, WPARAM param )
{
    UINT state;

    if( HIWORD(param) != BN_CLICKED )
        return ERROR_SUCCESS;

    TRACE("clicked checkbox %s, set %s\n", debugstr_w(control->name),
          debugstr_w(control->property));

    state = msi_dialog_get_checkbox_state( dialog, control );
    state = state ? 0 : 1;
    msi_dialog_set_checkbox_state( dialog, control, state );
    msi_dialog_checkbox_sync_state( dialog, control );

    return msi_dialog_button_handler( dialog, control, param );
}

static UINT msi_dialog_edit_handler( msi_dialog *dialog,
                msi_control *control, WPARAM param )
{
    LPWSTR buf;

    if( HIWORD(param) != EN_CHANGE )
        return ERROR_SUCCESS;

    TRACE("edit %s contents changed, set %s\n", debugstr_w(control->name),
          debugstr_w(control->property));

    buf = msi_get_window_text( control->hwnd );
    MSI_SetPropertyW( dialog->package, control->property, buf );

    msi_free( buf );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_radiogroup_handler( msi_dialog *dialog,
                msi_control *control, WPARAM param )
{
    if( HIWORD(param) != BN_CLICKED )
        return ERROR_SUCCESS;

    TRACE("clicked radio button %s, set %s\n", debugstr_w(control->name),
          debugstr_w(control->property));

    MSI_SetPropertyW( dialog->package, control->property, control->name );

    return msi_dialog_button_handler( dialog, control, param );
}

static LRESULT msi_dialog_oncommand( msi_dialog *dialog, WPARAM param, HWND hwnd )
{
    msi_control *control = NULL;

    TRACE("%p %p %08x\n", dialog, hwnd, param);

    switch (param)
    {
    case 1: /* enter */
        control = msi_dialog_find_control( dialog, dialog->control_default );
        break;
    case 2: /* escape */
        control = msi_dialog_find_control( dialog, dialog->control_cancel );
        break;
    default: 
        control = msi_dialog_find_control_by_hwnd( dialog, hwnd );
    }

    if( control )
    {
        if( control->handler )
        {
            control->handler( dialog, control, param );
            msi_dialog_evaluate_control_conditions( dialog );
        }
    }
    else
        ERR("button click from nowhere %p %d %p\n", dialog, param, hwnd);
    return 0;
}

static LRESULT msi_dialog_onnotify( msi_dialog *dialog, LPARAM param )
{
    LPNMHDR nmhdr = (LPNMHDR) param;
    msi_control *control = msi_dialog_find_control_by_hwnd( dialog, nmhdr->hwndFrom );

    TRACE("%p %p\n", dialog, nmhdr->hwndFrom);

    if ( control && control->handler )
        control->handler( dialog, control, param );

    return 0;
}

static void msi_dialog_setfocus( msi_dialog *dialog )
{
    HWND hwnd = dialog->hWndFocus;

    hwnd = GetNextDlgTabItem( dialog->hwnd, hwnd, TRUE);
    hwnd = GetNextDlgTabItem( dialog->hwnd, hwnd, FALSE);
    SetFocus( hwnd );
    dialog->hWndFocus = hwnd;
}

static LRESULT WINAPI MSIDialog_WndProc( HWND hwnd, UINT msg,
                WPARAM wParam, LPARAM lParam )
{
    msi_dialog *dialog = (LPVOID) GetWindowLongPtrW( hwnd, GWLP_USERDATA );

    TRACE("0x%04x\n", msg);

    switch (msg)
    {
    case WM_MOVE:
        dialog->package->center_x = LOWORD(lParam) + dialog->size.cx / 2.0;
        dialog->package->center_y = HIWORD(lParam) + dialog->size.cy / 2.0;
        break;
        
    case WM_CREATE:
        return msi_dialog_oncreate( hwnd, (LPCREATESTRUCTW)lParam );

    case WM_COMMAND:
        return msi_dialog_oncommand( dialog, wParam, (HWND)lParam );

    case WM_ACTIVATE:
        if( LOWORD(wParam) == WA_INACTIVE )
            dialog->hWndFocus = GetFocus();
        else
            msi_dialog_setfocus( dialog );
        return 0;

    case WM_SETFOCUS:
        msi_dialog_setfocus( dialog );
        return 0;

    /* bounce back to our subclassed static control */
    case WM_CTLCOLORSTATIC:
        return SendMessageW( (HWND) lParam, WM_CTLCOLORSTATIC, wParam, lParam );

    case WM_DESTROY:
        dialog->hwnd = NULL;
        return 0;
    case WM_NOTIFY:
        return msi_dialog_onnotify( dialog, lParam );
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static BOOL CALLBACK msi_radioground_child_enum( HWND hWnd, LPARAM lParam )
{
    EnableWindow( hWnd, lParam );
    return TRUE;
}

static LRESULT WINAPI MSIRadioGroup_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC) GetPropW(hWnd, szButtonData);
    LRESULT r;

    TRACE("hWnd %p msg %04x wParam 0x%08x lParam 0x%08lx\n", hWnd, msg, wParam, lParam);

    if (msg == WM_COMMAND) /* Forward notifications to dialog */
        SendMessageW(GetParent(hWnd), msg, wParam, lParam);

    r = CallWindowProcW(oldproc, hWnd, msg, wParam, lParam);

    /* make sure the radio buttons show as disabled if the parent is disabled */
    if (msg == WM_ENABLE)
        EnumChildWindows( hWnd, msi_radioground_child_enum, wParam );

    return r;
}

static LRESULT WINAPI MSIHiddenWindowProc( HWND hwnd, UINT msg,
                WPARAM wParam, LPARAM lParam )
{
    msi_dialog *dialog = (msi_dialog*) lParam;

    TRACE("%d %p\n", msg, dialog);

    switch (msg)
    {
    case WM_MSI_DIALOG_CREATE:
        return msi_dialog_run_message_loop( dialog );
    case WM_MSI_DIALOG_DESTROY:
        msi_dialog_destroy( dialog );
        return 0;
    }
    return DefWindowProcW( hwnd, msg, wParam, lParam );
}

/* functions that interface to other modules within MSI */

msi_dialog *msi_dialog_create( MSIPACKAGE* package,
                               LPCWSTR szDialogName, msi_dialog *parent,
                               msi_dialog_event_handler event_handler )
{
    MSIRECORD *rec = NULL;
    msi_dialog *dialog;

    TRACE("%p %s\n", package, debugstr_w(szDialogName));

    /* allocate the structure for the dialog to use */
    dialog = msi_alloc_zero( sizeof *dialog + sizeof(WCHAR)*strlenW(szDialogName) );
    if( !dialog )
        return NULL;
    strcpyW( dialog->name, szDialogName );
    dialog->parent = parent;
    msiobj_addref( &package->hdr );
    dialog->package = package;
    dialog->event_handler = event_handler;
    dialog->finished = 0;
    list_init( &dialog->controls );

    /* verify that the dialog exists */
    rec = msi_get_dialog_record( dialog );
    if( !rec )
    {
        msiobj_release( &package->hdr );
        msi_free( dialog );
        return NULL;
    }
    dialog->attributes = MSI_RecordGetInteger( rec, 6 );
    dialog->control_default = strdupW( MSI_RecordGetString( rec, 9 ) );
    dialog->control_cancel = strdupW( MSI_RecordGetString( rec, 10 ) );
    msiobj_release( &rec->hdr );

    return dialog;
}

static void msi_process_pending_messages( HWND hdlg )
{
    MSG msg;

    while( PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ) )
    {
        if( hdlg && IsDialogMessageW( hdlg, &msg ))
            continue;
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }
}

void msi_dialog_end_dialog( msi_dialog *dialog )
{
    TRACE("%p\n", dialog);
    dialog->finished = 1;
    PostMessageW(dialog->hwnd, WM_NULL, 0, 0);
}

void msi_dialog_check_messages( HANDLE handle )
{
    DWORD r;

    /* in threads other than the UI thread, block */
    if( uiThreadId != GetCurrentThreadId() )
    {
        if( handle )
            WaitForSingleObject( handle, INFINITE );
        return;
    }

    /* there's two choices for the UI thread */
    while (1)
    {
        msi_process_pending_messages( NULL );

        if( !handle )
            break;

        /*
         * block here until somebody creates a new dialog or
         * the handle we're waiting on becomes ready
         */
        r = MsgWaitForMultipleObjects( 1, &handle, 0, INFINITE, QS_ALLINPUT );
        if( r == WAIT_OBJECT_0 )
            break;
    }
}

UINT msi_dialog_run_message_loop( msi_dialog *dialog )
{
    DWORD style;
    HWND hwnd;

    if( uiThreadId != GetCurrentThreadId() )
        return SendMessageW( hMsiHiddenWindow, WM_MSI_DIALOG_CREATE, 0, (LPARAM) dialog );

    /* create the dialog window, don't show it yet */
    style = WS_OVERLAPPED;
    if( dialog->attributes & msidbDialogAttributesVisible )
        style |= WS_VISIBLE;

    hwnd = CreateWindowW( szMsiDialogClass, dialog->name, style,
                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                     NULL, NULL, NULL, dialog );
    if( !hwnd )
    {
        ERR("Failed to create dialog %s\n", debugstr_w( dialog->name ));
        return ERROR_FUNCTION_FAILED;
    }

    ShowWindow( hwnd, SW_SHOW );
    /* UpdateWindow( hwnd ); - and causes the transparent static controls not to paint */

    if( dialog->attributes & msidbDialogAttributesModal )
    {
        while( !dialog->finished )
        {
            MsgWaitForMultipleObjects( 0, NULL, 0, INFINITE, QS_ALLINPUT );
            msi_process_pending_messages( dialog->hwnd );
        }
    }
    else
        return ERROR_IO_PENDING;

    return ERROR_SUCCESS;
}

void msi_dialog_do_preview( msi_dialog *dialog )
{
    TRACE("\n");
    dialog->attributes |= msidbDialogAttributesVisible;
    dialog->attributes &= ~msidbDialogAttributesModal;
    msi_dialog_run_message_loop( dialog );
}

void msi_dialog_destroy( msi_dialog *dialog )
{
    if( uiThreadId != GetCurrentThreadId() )
    {
        SendMessageW( hMsiHiddenWindow, WM_MSI_DIALOG_DESTROY, 0, (LPARAM) dialog );
        return;
    }

    if( dialog->hwnd )
        ShowWindow( dialog->hwnd, SW_HIDE );
    
    if( dialog->hwnd )
        DestroyWindow( dialog->hwnd );

    /* destroy the list of controls */
    while( !list_empty( &dialog->controls ) )
    {
        msi_control *t = LIST_ENTRY( list_head( &dialog->controls ),
                                     msi_control, entry );
        list_remove( &t->entry );
        /* leave dialog->hwnd - destroying parent destroys child windows */
        msi_free( t->property );
        msi_free( t->value );
        if( t->hBitmap )
            DeleteObject( t->hBitmap );
        if( t->hIcon )
            DestroyIcon( t->hIcon );
        msi_free( t->tabnext );
        msi_free( t->type );
        msi_free( t );
        if (t->hDll)
            FreeLibrary( t->hDll );
    }

    /* destroy the list of fonts */
    while( dialog->font_list )
    {
        msi_font *t = dialog->font_list;
        dialog->font_list = t->next;
        DeleteObject( t->hfont );
        msi_free( t );
    }
    msi_free( dialog->default_font );

    msi_free( dialog->control_default );
    msi_free( dialog->control_cancel );
    msiobj_release( &dialog->package->hdr );
    dialog->package = NULL;
    msi_free( dialog );
}

BOOL msi_dialog_register_class( void )
{
    WNDCLASSW cls;

    ZeroMemory( &cls, sizeof cls );
    cls.lpfnWndProc   = MSIDialog_WndProc;
    cls.hInstance     = NULL;
    cls.hIcon         = LoadIconW(0, (LPWSTR)IDI_APPLICATION);
    cls.hCursor       = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    cls.lpszMenuName  = NULL;
    cls.lpszClassName = szMsiDialogClass;

    if( !RegisterClassW( &cls ) )
        return FALSE;

    cls.lpfnWndProc   = MSIHiddenWindowProc;
    cls.lpszClassName = szMsiHiddenWindow;

    if( !RegisterClassW( &cls ) )
        return FALSE;

    uiThreadId = GetCurrentThreadId();

    hMsiHiddenWindow = CreateWindowW( szMsiHiddenWindow, NULL, WS_OVERLAPPED,
                                   0, 0, 100, 100, NULL, NULL, NULL, NULL );
    if( !hMsiHiddenWindow )
        return FALSE;

    return TRUE;
}

void msi_dialog_unregister_class( void )
{
    DestroyWindow( hMsiHiddenWindow );
    hMsiHiddenWindow = NULL;
    UnregisterClassW( szMsiDialogClass, NULL );
    UnregisterClassW( szMsiHiddenWindow, NULL );
    uiThreadId = 0;
}

static UINT error_dialog_handler(MSIPACKAGE *package, LPCWSTR event,
                                 LPCWSTR argument, msi_dialog* dialog)
{
    static const WCHAR end_dialog[] = {'E','n','d','D','i','a','l','o','g',0};
    static const WCHAR error_abort[] = {'E','r','r','o','r','A','b','o','r','t',0};
    static const WCHAR error_cancel[] = {'E','r','r','o','r','C','a','n','c','e','l',0};
    static const WCHAR error_no[] = {'E','r','r','o','r','N','o',0};
    static const WCHAR result_prop[] = {
        'M','S','I','E','r','r','o','r','D','i','a','l','o','g','R','e','s','u','l','t',0
    };

    if ( lstrcmpW( event, end_dialog ) )
        return ERROR_SUCCESS;

    if ( !lstrcmpW( argument, error_abort ) || !lstrcmpW( argument, error_cancel ) ||
         !lstrcmpW( argument, error_no ) )
    {
         MSI_SetPropertyW( package, result_prop, error_abort );
    }

    ControlEvent_CleanupSubscriptions(package);
    msi_dialog_end_dialog( dialog );

    return ERROR_SUCCESS;
}

static UINT msi_error_dialog_set_error( MSIPACKAGE *package, LPWSTR error_dialog, LPWSTR error )
{
    MSIRECORD * row;

    static const WCHAR update[] = 
        {'U','P','D','A','T','E',' ','`','C','o','n','t','r','o','l','`',' ',
         'S','E','T',' ','`','T','e','x','t','`',' ','=',' ','\'','%','s','\'',' ',
         'W','H','E','R','E', ' ','`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',' ',
         'A','N','D',' ','`','C','o','n','t','r','o','l','`',' ','=',' ',
         '\'','E','r','r','o','r','T','e','x','t','\'',0};

    row = MSI_QueryGetRecord( package->db, update, error, error_dialog );
    if (!row)
        return ERROR_FUNCTION_FAILED;

    msiobj_release(&row->hdr);
    return ERROR_SUCCESS;
}

UINT msi_spawn_error_dialog( MSIPACKAGE *package, LPWSTR error_dialog, LPWSTR error )
{
    msi_dialog *dialog;
    WCHAR result[MAX_PATH];
    UINT r = ERROR_SUCCESS;
    DWORD size = MAX_PATH;
    int res;

    static const WCHAR pn_prop[] = {'P','r','o','d','u','c','t','N','a','m','e',0};
    static const WCHAR title_fmt[] = {'%','s',' ','W','a','r','n','i','n','g',0};
    static const WCHAR error_abort[] = {'E','r','r','o','r','A','b','o','r','t',0};
    static const WCHAR result_prop[] = {
        'M','S','I','E','r','r','o','r','D','i','a','l','o','g','R','e','s','u','l','t',0
    };

    if ( !error_dialog )
    {
        LPWSTR product_name = msi_dup_property( package, pn_prop );
        WCHAR title[MAX_PATH];

        sprintfW( title, title_fmt, product_name );
        res = MessageBoxW( NULL, error, title, MB_OKCANCEL | MB_ICONWARNING );

        msi_free( product_name );

        if ( res == IDOK )
            return ERROR_SUCCESS;
        else
            return ERROR_FUNCTION_FAILED;
    }

    r = msi_error_dialog_set_error( package, error_dialog, error );
    if ( r != ERROR_SUCCESS )
        return r;

    dialog = msi_dialog_create( package, error_dialog, package->dialog,
                                error_dialog_handler );
    if ( !dialog )
        return ERROR_FUNCTION_FAILED;

    dialog->finished = FALSE;
    r = msi_dialog_run_message_loop( dialog );
    if ( r != ERROR_SUCCESS )
        goto done;

    r = MSI_GetPropertyW( package, result_prop, result, &size );
    if ( r != ERROR_SUCCESS)
        r = ERROR_SUCCESS;

    if ( !lstrcmpW( result, error_abort ) )
        r = ERROR_FUNCTION_FAILED;

done:
    msi_dialog_destroy( dialog );

    return r;
}
