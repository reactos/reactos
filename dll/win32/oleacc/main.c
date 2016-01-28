/*
 * Implementation of the OLEACC dll
 *
 * Copyright 2003 Mike McCormack for CodeWeavers
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

#include "oleacc_private.h"

#include <commctrl.h>
#include <rpcproxy.h>

#include <wine/unicode.h>

#include "resource.h"

static const WCHAR lresult_atom_prefix[] = {'w','i','n','e','_','o','l','e','a','c','c',':'};

static const WCHAR menuW[] = {'#','3','2','7','6','8',0};
static const WCHAR desktopW[] = {'#','3','2','7','6','9',0};
static const WCHAR dialogW[] = {'#','3','2','7','7','0',0};
static const WCHAR winswitchW[] = {'#','3','2','7','7','1',0};
static const WCHAR mdi_clientW[] = {'M','D','I','C','l','i','e','n','t',0};
static const WCHAR richeditW[] = {'R','I','C','H','E','D','I','T',0};
static const WCHAR richedit20aW[] = {'R','i','c','h','E','d','i','t','2','0','A',0};
static const WCHAR richedit20wW[] = {'R','i','c','h','E','d','i','t','2','0','W',0};

typedef HRESULT (WINAPI *accessible_create)(HWND, const IID*, void**);

extern HRESULT WINAPI OLEACC_DllGetClassObject(REFCLSID, REFIID, void**) DECLSPEC_HIDDEN;
extern BOOL WINAPI OLEACC_DllMain(HINSTANCE, DWORD, void*) DECLSPEC_HIDDEN;
extern HRESULT WINAPI OLEACC_DllRegisterServer(void) DECLSPEC_HIDDEN;
extern HRESULT WINAPI OLEACC_DllUnregisterServer(void) DECLSPEC_HIDDEN;

static struct {
    const WCHAR *name;
    DWORD idx;
    accessible_create create_client;
    accessible_create create_window;
} builtin_classes[] = {
    {WC_LISTBOXW,           0x10000, NULL, NULL},
    {menuW,                 0x10001, NULL, NULL},
    {WC_BUTTONW,            0x10002, NULL, NULL},
    {WC_STATICW,            0x10003, NULL, NULL},
    {WC_EDITW,              0x10004, NULL, NULL},
    {WC_COMBOBOXW,          0x10005, NULL, NULL},
    {dialogW,               0x10006, NULL, NULL},
    {winswitchW,            0x10007, NULL, NULL},
    {mdi_clientW,           0x10008, NULL, NULL},
    {desktopW,              0x10009, NULL, NULL},
    {WC_SCROLLBARW,         0x1000a, NULL, NULL},
    {STATUSCLASSNAMEW,      0x1000b, NULL, NULL},
    {TOOLBARCLASSNAMEW,     0x1000c, NULL, NULL},
    {PROGRESS_CLASSW,       0x1000d, NULL, NULL},
    {ANIMATE_CLASSW,        0x1000e, NULL, NULL},
    {WC_TABCONTROLW,        0x1000f, NULL, NULL},
    {HOTKEY_CLASSW,         0x10010, NULL, NULL},
    {WC_HEADERW,            0x10011, NULL, NULL},
    {TRACKBAR_CLASSW,       0x10012, NULL, NULL},
    {WC_LISTVIEWW,          0x10013, NULL, NULL},
    {UPDOWN_CLASSW,         0x10016, NULL, NULL},
    {TOOLTIPS_CLASSW,       0x10018, NULL, NULL},
    {WC_TREEVIEWW,          0x10019, NULL, NULL},
    {MONTHCAL_CLASSW,       0,       NULL, NULL},
    {DATETIMEPICK_CLASSW,   0,       NULL, NULL},
    {WC_IPADDRESSW,         0,       NULL, NULL},
    {richeditW,             0x1001c, NULL, NULL},
    {richedit20aW,          0,       NULL, NULL},
    {richedit20wW,          0,       NULL, NULL},
};

static HINSTANCE oleacc_handle = 0;

int convert_child_id(VARIANT *v)
{
    switch(V_VT(v)) {
    case VT_I4:
        return V_I4(v);
    default:
        FIXME("unhandled child ID variant type: %d\n", V_VT(v));
        return -1;
    }
}

static accessible_create get_builtin_accessible_obj(HWND hwnd, LONG objid)
{
    WCHAR class_name[64];
    int i, idx;

    if(!RealGetWindowClassW(hwnd, class_name, sizeof(class_name)/sizeof(WCHAR)))
        return NULL;
    TRACE("got window class: %s\n", debugstr_w(class_name));

    for(i=0; i<sizeof(builtin_classes)/sizeof(builtin_classes[0]); i++) {
        if(!strcmpiW(class_name, builtin_classes[i].name)) {
            accessible_create ret;

            ret = (objid==OBJID_CLIENT ?
                    builtin_classes[i].create_client :
                    builtin_classes[i].create_window);
            if(!ret)
                FIXME("unhandled window class: %s\n", debugstr_w(class_name));
            return ret;
        }
    }

    idx = SendMessageW(hwnd, WM_GETOBJECT, 0, OBJID_QUERYCLASSNAMEIDX);
    if(idx) {
        for(i=0; i<sizeof(builtin_classes)/sizeof(builtin_classes[0]); i++) {
            if(idx == builtin_classes[i].idx) {
                accessible_create ret;

                ret = (objid==OBJID_CLIENT ?
                        builtin_classes[i].create_client :
                        builtin_classes[i].create_window);
                if(!ret)
                    FIXME("unhandled class name idx: %x\n", idx);
                return ret;
            }
        }

        WARN("unhandled class name idx: %x\n", idx);
    }

    return NULL;
}

HRESULT WINAPI CreateStdAccessibleObject( HWND hwnd, LONG idObject,
        REFIID riidInterface, void** ppvObject )
{
    accessible_create create;

    TRACE("%p %d %s %p\n", hwnd, idObject,
          debugstr_guid( riidInterface ), ppvObject );

    switch(idObject) {
    case OBJID_CLIENT:
        create = get_builtin_accessible_obj(hwnd, idObject);
        if(create) return create(hwnd, riidInterface, ppvObject);
        return create_client_object(hwnd, riidInterface, ppvObject);
    case OBJID_WINDOW:
        create = get_builtin_accessible_obj(hwnd, idObject);
        if(create) return create(hwnd, riidInterface, ppvObject);
        return create_window_object(hwnd, riidInterface, ppvObject);
    default:
        FIXME("unhandled object id: %d\n", idObject);
        return E_NOTIMPL;
    }
}

HRESULT WINAPI ObjectFromLresult( LRESULT result, REFIID riid, WPARAM wParam, void **ppObject )
{
    WCHAR atom_str[sizeof(lresult_atom_prefix)/sizeof(WCHAR)+3*8+3];
    HANDLE server_proc, server_mapping, mapping;
    DWORD proc_id, size;
    IStream *stream;
    HGLOBAL data;
    void *view;
    HRESULT hr;
    WCHAR *p;

    TRACE("%ld %s %ld %p\n", result, debugstr_guid(riid), wParam, ppObject );

    if(wParam)
        FIXME("unsupported wParam = %lx\n", wParam);

    if(!ppObject)
        return E_INVALIDARG;
    *ppObject = NULL;

    if(result != (ATOM)result)
        return E_FAIL;

    if(!GlobalGetAtomNameW(result, atom_str, sizeof(atom_str)/sizeof(WCHAR)))
        return E_FAIL;
    if(memcmp(atom_str, lresult_atom_prefix, sizeof(lresult_atom_prefix)))
        return E_FAIL;
    p = atom_str + sizeof(lresult_atom_prefix)/sizeof(WCHAR);
    proc_id = strtoulW(p, &p, 16);
    if(*p != ':')
        return E_FAIL;
    server_mapping = ULongToHandle( strtoulW(p+1, &p, 16) );
    if(*p != ':')
        return E_FAIL;
    size = strtoulW(p+1, &p, 16);
    if(*p != 0)
        return E_FAIL;

    server_proc = OpenProcess(PROCESS_DUP_HANDLE, FALSE, proc_id);
    if(!server_proc)
        return E_FAIL;

    if(!DuplicateHandle(server_proc, server_mapping, GetCurrentProcess(), &mapping,
                0, FALSE, DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS))
        return E_FAIL;
    CloseHandle(server_proc);
    GlobalDeleteAtom(result);

    view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);
    if(!view)
        return E_FAIL;

    data = GlobalAlloc(GMEM_FIXED, size);
    if(!data) {
        UnmapViewOfFile(view);
        return E_OUTOFMEMORY;
    }
    memcpy(data, view, size);
    UnmapViewOfFile(view);

    hr = CreateStreamOnHGlobal(data, TRUE, &stream);
    if(FAILED(hr)) {
        GlobalFree(data);
        return hr;
    }

    hr = CoUnmarshalInterface(stream, riid, ppObject);
    IStream_Release(stream);
    return hr;
}

LRESULT WINAPI LresultFromObject( REFIID riid, WPARAM wParam, LPUNKNOWN pAcc )
{
    static const WCHAR atom_fmt[] = {'%','0','8','x',':','%','0','8','x',':','%','0','8','x',0};
    static const LARGE_INTEGER seek_zero = {{0}};

    WCHAR atom_str[sizeof(lresult_atom_prefix)/sizeof(WCHAR)+3*8+3];
    IStream *stream;
    HANDLE mapping;
    STATSTG stat;
    HRESULT hr;
    ATOM atom;
    void *view;

    TRACE("%s %ld %p\n", debugstr_guid(riid), wParam, pAcc);

    if(wParam)
        FIXME("unsupported wParam = %lx\n", wParam);

    if(!pAcc)
        return E_INVALIDARG;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if(FAILED(hr))
        return hr;

    hr = CoMarshalInterface(stream, riid, pAcc, MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
    if(FAILED(hr)) {
        IStream_Release(stream);
        return hr;
    }

    hr = IStream_Seek(stream, seek_zero, STREAM_SEEK_SET, NULL);
    if(FAILED(hr)) {
        IStream_Release(stream);
        return hr;
    }

    hr = IStream_Stat(stream, &stat, STATFLAG_NONAME);
    if(FAILED(hr)) {
        CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return hr;
    }else if(stat.cbSize.u.HighPart) {
        FIXME("stream size to big\n");
        CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return E_NOTIMPL;
    }

    mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
            stat.cbSize.u.HighPart, stat.cbSize.u.LowPart, NULL);
    if(!mapping) {
        CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return hr;
    }

    view = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    if(!view) {
        CloseHandle(mapping);
        CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return E_FAIL;
    }

    hr = IStream_Read(stream, view, stat.cbSize.u.LowPart, NULL);
    UnmapViewOfFile(view);
    if(FAILED(hr)) {
        CloseHandle(mapping);
        hr = IStream_Seek(stream, seek_zero, STREAM_SEEK_SET, NULL);
        if(SUCCEEDED(hr))
            CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return hr;

    }

    memcpy(atom_str, lresult_atom_prefix, sizeof(lresult_atom_prefix));
    sprintfW(atom_str+sizeof(lresult_atom_prefix)/sizeof(WCHAR),
             atom_fmt, GetCurrentProcessId(), HandleToUlong(mapping), stat.cbSize.u.LowPart);
    atom = GlobalAddAtomW(atom_str);
    if(!atom) {
        CloseHandle(mapping);
        hr = IStream_Seek(stream, seek_zero, STREAM_SEEK_SET, NULL);
        if(SUCCEEDED(hr))
            CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return E_FAIL;
    }

    IStream_Release(stream);
    return atom;
}

HRESULT WINAPI AccessibleObjectFromPoint( POINT ptScreen, IAccessible** ppacc, VARIANT* pvarChild )
{
    FIXME("{%d,%d} %p %p: stub\n", ptScreen.x, ptScreen.y, ppacc, pvarChild );
    return E_NOTIMPL;
}

HRESULT WINAPI AccessibleObjectFromWindow( HWND hwnd, DWORD dwObjectID,
                             REFIID riid, void** ppvObject )
{
    TRACE("%p %d %s %p\n", hwnd, dwObjectID,
          debugstr_guid( riid ), ppvObject );

    if(!ppvObject)
        return E_INVALIDARG;
    *ppvObject = NULL;

    if(IsWindow(hwnd)) {
        LRESULT lres;

        lres = SendMessageW(hwnd, WM_GETOBJECT, 0xffffffff, dwObjectID);
        if(FAILED(lres))
            return lres;
        else if(lres)
            return ObjectFromLresult(lres, riid, 0, ppvObject);
    }

    return CreateStdAccessibleObject(hwnd, dwObjectID, riid, ppvObject);
}

HRESULT WINAPI WindowFromAccessibleObject(IAccessible *acc, HWND *phwnd)
{
    IDispatch *parent;
    IOleWindow *ow;
    HRESULT hres;

    TRACE("%p %p\n", acc, phwnd);

    IAccessible_AddRef(acc);
    while(1) {
        hres = IAccessible_QueryInterface(acc, &IID_IOleWindow, (void**)&ow);
        if(SUCCEEDED(hres)) {
            hres = IOleWindow_GetWindow(ow, phwnd);
            IOleWindow_Release(ow);
            IAccessible_Release(acc);
            return hres;
        }

        hres = IAccessible_get_accParent(acc, &parent);
        IAccessible_Release(acc);
        if(FAILED(hres))
            return hres;
        if(hres!=S_OK || !parent) {
            *phwnd = NULL;
            return hres;
        }

        hres = IDispatch_QueryInterface(parent, &IID_IAccessible, (void**)&acc);
        IDispatch_Release(parent);
        if(FAILED(hres))
            return hres;
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,
                    LPVOID lpvReserved)
{
    TRACE("%p, %d, %p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            oleacc_handle = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return OLEACC_DllMain(hinstDLL, fdwReason, lpvReserved);
}

HRESULT WINAPI DllRegisterServer(void)
{
    return OLEACC_DllRegisterServer();
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return OLEACC_DllUnregisterServer();
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, void **ppv)
{
    if(IsEqualGUID(&CLSID_CAccPropServices, rclsid)) {
        TRACE("(CLSID_CAccPropServices %s %p)\n", debugstr_guid(iid), ppv);
        return get_accpropservices_factory(iid, ppv);
    }

    if(IsEqualGUID(&CLSID_PSFactoryBuffer, rclsid)) {
        TRACE("(CLSID_PSFactoryBuffer %s %p)\n", debugstr_guid(iid), ppv);
        return OLEACC_DllGetClassObject(rclsid, iid, ppv);
    }

    FIXME("%s %s %p: stub\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);
    return E_NOTIMPL;
}

void WINAPI GetOleaccVersionInfo(DWORD* pVersion, DWORD* pBuild)
{
#ifdef __REACTOS__
    *pVersion = MAKELONG(2,4); /* Windows XP version of oleacc: 4.2.5406.0 */
    *pBuild = MAKELONG(0,5406);
#else
    *pVersion = MAKELONG(0,7); /* Windows 7 version of oleacc: 7.0.0.0 */
    *pBuild = MAKELONG(0,0);
#endif
}

HANDLE WINAPI GetProcessHandleFromHwnd(HWND hwnd)
{
    DWORD proc_id;

    TRACE("%p\n", hwnd);

    if(!GetWindowThreadProcessId(hwnd, &proc_id))
        return NULL;
    return OpenProcess(PROCESS_DUP_HANDLE | PROCESS_VM_OPERATION |
            PROCESS_VM_READ | PROCESS_VM_WRITE | SYNCHRONIZE, TRUE, proc_id);
}

UINT WINAPI GetRoleTextW(DWORD role, LPWSTR lpRole, UINT rolemax)
{
    INT ret;
    WCHAR *resptr;

    TRACE("%u %p %u\n", role, lpRole, rolemax);

    /* return role text length */
    if(!lpRole)
        return LoadStringW(oleacc_handle, role, (LPWSTR)&resptr, 0);

    ret = LoadStringW(oleacc_handle, role, lpRole, rolemax);
    if(!(ret > 0)){
        if(rolemax > 0) lpRole[0] = '\0';
        return 0;
    }

    return ret;
}

UINT WINAPI GetRoleTextA(DWORD role, LPSTR lpRole, UINT rolemax)
{
    UINT length;
    WCHAR *roletextW;

    TRACE("%u %p %u\n", role, lpRole, rolemax);

    if(lpRole && !rolemax)
        return 0;

    length = GetRoleTextW(role, NULL, 0);
    if(!length) {
        if(lpRole && rolemax)
            lpRole[0] = 0;
        return 0;
    }

    roletextW = HeapAlloc(GetProcessHeap(), 0, (length + 1)*sizeof(WCHAR));
    if(!roletextW)
        return 0;

    GetRoleTextW(role, roletextW, length + 1);

    length = WideCharToMultiByte( CP_ACP, 0, roletextW, -1, NULL, 0, NULL, NULL );

    if(!lpRole){
        HeapFree(GetProcessHeap(), 0, roletextW);
        return length - 1;
    }

    if(rolemax < length) {
        HeapFree(GetProcessHeap(), 0, roletextW);
        lpRole[0] = 0;
        return 0;
    }

    WideCharToMultiByte( CP_ACP, 0, roletextW, -1, lpRole, rolemax, NULL, NULL );

    if(rolemax < length){
        lpRole[rolemax-1] = '\0';
        length = rolemax;
    }

    HeapFree(GetProcessHeap(), 0, roletextW);

    return length - 1;
}

UINT WINAPI GetStateTextW(DWORD state_bit, WCHAR *state_str, UINT state_str_len)
{
    DWORD state_id;

    TRACE("%x %p %u\n", state_bit, state_str, state_str_len);

    if(state_bit & ~(STATE_SYSTEM_VALID | STATE_SYSTEM_HASPOPUP)) {
        if(state_str && state_str_len)
            state_str[0] = 0;
        return 0;
    }

    state_id = IDS_STATE_NORMAL;
    while(state_bit) {
        state_id++;
        state_bit /= 2;
    }

    if(state_str) {
        UINT ret = LoadStringW(oleacc_handle, state_id, state_str, state_str_len);
        if(!ret && state_str_len)
            state_str[0] = 0;
        return ret;
    }else {
        WCHAR *tmp;
        return LoadStringW(oleacc_handle, state_id, (WCHAR*)&tmp, 0);
    }

}

UINT WINAPI GetStateTextA(DWORD state_bit, CHAR *state_str, UINT state_str_len)
{
    DWORD state_id;

    TRACE("%x %p %u\n", state_bit, state_str, state_str_len);

    if(state_str && !state_str_len)
        return 0;

    if(state_bit & ~(STATE_SYSTEM_VALID | STATE_SYSTEM_HASPOPUP)) {
        if(state_str && state_str_len)
            state_str[0] = 0;
        return 0;
    }

    state_id = IDS_STATE_NORMAL;
    while(state_bit) {
        state_id++;
        state_bit /= 2;
    }

    if(state_str) {
        UINT ret = LoadStringA(oleacc_handle, state_id, state_str, state_str_len);
        if(!ret && state_str_len)
            state_str[0] = 0;
        return ret;
    }else {
        CHAR tmp[256];
        return LoadStringA(oleacc_handle, state_id, tmp, sizeof(tmp));
    }
}

HRESULT WINAPI AccessibleChildren(IAccessible *container,
        LONG start, LONG count, VARIANT *children, LONG *children_cnt)
{
    IEnumVARIANT *ev;
    LONG i, child_no;
    HRESULT hr;

    TRACE("%p %d %d %p %p\n", container, start, count, children, children_cnt);

    if(!container || !children || !children_cnt)
        return E_INVALIDARG;

    for(i=0; i<count; i++)
        VariantInit(children+i);

    hr = IAccessible_QueryInterface(container, &IID_IEnumVARIANT, (void**)&ev);
    if(SUCCEEDED(hr)) {
        hr = IEnumVARIANT_Reset(ev);
        if(SUCCEEDED(hr))
            hr = IEnumVARIANT_Skip(ev, start);
        if(SUCCEEDED(hr))
            hr = IEnumVARIANT_Next(ev, count, children, (ULONG*)children_cnt);
        IEnumVARIANT_Release(ev);
        return hr;
    }

    hr = IAccessible_get_accChildCount(container, &child_no);
    if(FAILED(hr))
        return hr;

    for(i=0; i<count && start+i+1<=child_no; i++) {
        IDispatch *disp;

        V_VT(children+i) = VT_I4;
        V_I4(children+i) = start+i+1;

        hr = IAccessible_get_accChild(container, children[i], &disp);
        if(SUCCEEDED(hr) && disp) {
            V_VT(children+i) = VT_DISPATCH;
            V_DISPATCH(children+i) = disp;
        }
    }

    *children_cnt = i;
    return i==count ? S_OK : S_FALSE;
}
