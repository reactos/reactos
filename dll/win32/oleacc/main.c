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

#define COBJMACROS

#include "initguid.h"
#include "oleacc_private.h"
#include "resource.h"

#include "wine/debug.h"

#ifdef __REACTOS__
#include <wchar.h>
#include <winnls.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(oleacc);

static const WCHAR lresult_atom_prefix[] = {'w','i','n','e','_','o','l','e','a','c','c',':'};

#define NAVDIR_INTERNAL_HWND 10
DEFINE_GUID(SID_AccFromDAWrapper, 0x33f139ee, 0xe509, 0x47f7, 0xbf,0x39, 0x83,0x76,0x44,0xf7,0x45,0x76);

extern HRESULT WINAPI OLEACC_DllGetClassObject(REFCLSID, REFIID, void**);
extern BOOL WINAPI OLEACC_DllMain(HINSTANCE, DWORD, void*);
extern HRESULT WINAPI OLEACC_DllRegisterServer(void);
extern HRESULT WINAPI OLEACC_DllUnregisterServer(void);

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

const struct win_class_data* find_class_data(HWND hwnd, const struct win_class_data *classes)
{
    WCHAR class_name[64];
    int i, idx;

    if(!RealGetWindowClassW(hwnd, class_name, ARRAY_SIZE(class_name)))
        return NULL;
    TRACE("got window class: %s\n", debugstr_w(class_name));

    for(i=0; classes[i].name; i++) {
        if(!wcsicmp(class_name, classes[i].name)) {
            if(classes[i].stub)
                FIXME("unhandled window class: %s\n", debugstr_w(class_name));
            return &classes[i];
        }
    }

    idx = SendMessageW(hwnd, WM_GETOBJECT, 0, OBJID_QUERYCLASSNAMEIDX);
    if(idx) {
        for(i=0; classes[i].name; i++) {
            if(idx == classes[i].idx) {
                if(classes[i].stub)
                    FIXME("unhandled window class: %s\n", debugstr_w(class_name));
                return &classes[i];
            }
        }

        WARN("unhandled class name idx: %x\n", idx);
    }

    return NULL;
}

HRESULT WINAPI CreateStdAccessibleObject( HWND hwnd, LONG idObject,
        REFIID riidInterface, void** ppvObject )
{
    TRACE("%p %ld %s %p\n", hwnd, idObject,
          debugstr_guid( riidInterface ), ppvObject );

    switch(idObject) {
    case OBJID_CLIENT:
        return create_client_object(hwnd, riidInterface, ppvObject);
    case OBJID_WINDOW:
        return create_window_object(hwnd, riidInterface, ppvObject);
    default:
        FIXME("unhandled object id: %ld\n", idObject);
        return E_NOTIMPL;
    }
}

HRESULT WINAPI ObjectFromLresult( LRESULT result, REFIID riid, WPARAM wParam, void **ppObject )
{
    WCHAR atom_str[ARRAY_SIZE(lresult_atom_prefix)+3*8+3];
    HANDLE server_proc, server_mapping, mapping;
    DWORD proc_id, size;
    IStream *stream;
    HGLOBAL data;
    void *view;
    HRESULT hr;
    WCHAR *p;

    TRACE("%Id %s %Id %p\n", result, debugstr_guid(riid), wParam, ppObject );

    if(wParam)
        FIXME("unsupported wParam = %Ix\n", wParam);

    if(!ppObject)
        return E_INVALIDARG;
    *ppObject = NULL;

    if(result != (ATOM)result)
        return E_FAIL;

    if(!GlobalGetAtomNameW(result, atom_str, ARRAY_SIZE(atom_str)))
        return E_FAIL;
    if(memcmp(atom_str, lresult_atom_prefix, sizeof(lresult_atom_prefix)))
        return E_FAIL;
    p = atom_str + ARRAY_SIZE(lresult_atom_prefix);
    proc_id = wcstoul(p, &p, 16);
    if(*p != ':')
        return E_FAIL;
    server_mapping = ULongToHandle( wcstoul(p+1, &p, 16) );
    if(*p != ':')
        return E_FAIL;
    size = wcstoul(p+1, &p, 16);
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
    static const LARGE_INTEGER seek_zero = {{0}};

    WCHAR atom_str[ARRAY_SIZE(lresult_atom_prefix)+3*8+3];
    IStream *stream;
    HANDLE mapping;
    STATSTG stat;
    HRESULT hr;
    ATOM atom;
    void *view;

    TRACE("%s %Id %p\n", debugstr_guid(riid), wParam, pAcc);

    if(wParam)
        FIXME("unsupported wParam = %Ix\n", wParam);

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
    swprintf(atom_str+ARRAY_SIZE(lresult_atom_prefix), 3*8 + 3, L"%08x:%08x:%08x", GetCurrentProcessId(),
             HandleToUlong(mapping), stat.cbSize.u.LowPart);
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

static void variant_init_i4( VARIANT *v, int val )
{
    V_VT(v) = VT_I4;
    V_I4(v) = val;
}

HRESULT WINAPI AccessibleObjectFromPoint( POINT point, IAccessible** acc, VARIANT* child_id )
{
    IAccessible *cur;
    HRESULT hr;
    VARIANT v;
    HWND hwnd;

    TRACE("{%ld,%ld} %p %p\n", point.x, point.y, acc, child_id);

    if (!acc || !child_id)
        return E_INVALIDARG;

    *acc = NULL;
    V_VT(child_id) = VT_EMPTY;

    hwnd = WindowFromPoint(point);
    if (!hwnd)
        return E_FAIL;
    hwnd = GetAncestor(hwnd, GA_ROOT);

    hr = AccessibleObjectFromWindow(hwnd, OBJID_WINDOW, &IID_IAccessible, (void **)&cur);
    if (FAILED(hr))
        return hr;
    if (!cur)
        return E_FAIL;

    V_VT(&v) = VT_EMPTY;
    while (1)
    {
        hr = IAccessible_accHitTest(cur, point.x, point.y, &v);
        if (FAILED(hr))
        {
            IAccessible_Release(cur);
            return hr;
        }

        if (V_VT(&v) == VT_I4)
        {
            *acc = cur;
            variant_init_i4(child_id, V_I4(&v));
            return S_OK;
        }
        else if (V_VT(&v) == VT_DISPATCH)
        {
            IAccessible_Release(cur);

            hr = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IAccessible, (void**)&cur);
            VariantClear(&v);
            if (FAILED(hr))
                return hr;
            if (!cur)
                return E_FAIL;
        }
        else
        {
            VariantClear(&v);
            IAccessible_Release(cur);
            FIXME("unhandled variant type: %d\n", V_VT(&v));
            return E_NOTIMPL;
        }
    }

    return S_OK;
}

HRESULT WINAPI AccessibleObjectFromEvent( HWND hwnd, DWORD object_id, DWORD child_id,
                             IAccessible **acc_out, VARIANT *child_id_out )
{
    VARIANT child_id_variant;
    IAccessible *acc = NULL;
    IDispatch *child = NULL;
    HRESULT hr;

    TRACE("%p %ld %ld %p %p\n", hwnd, object_id, child_id, acc_out, child_id_out);

    if (!acc_out)
        return E_INVALIDARG;
    *acc_out = NULL;
    VariantInit(child_id_out);

    hr = AccessibleObjectFromWindow(hwnd, object_id, &IID_IAccessible, (void **)&acc);
    if (FAILED(hr))
        return hr;

    variant_init_i4(&child_id_variant, child_id);
    hr = IAccessible_get_accChild(acc, child_id_variant, &child);
    if (FAILED(hr))
        TRACE("get_accChild failed with %#lx!\n", hr);

    if (SUCCEEDED(hr) && child)
    {
        IAccessible_Release(acc);
        hr = IDispatch_QueryInterface(child, &IID_IAccessible, (void **)&acc);
        IDispatch_Release(child);
        if (FAILED(hr))
            return hr;

        variant_init_i4(&child_id_variant, CHILDID_SELF);
    }

    *acc_out = acc;
    *child_id_out = child_id_variant;

    return S_OK;
}

HRESULT WINAPI AccessibleObjectFromWindow( HWND hwnd, DWORD dwObjectID,
                             REFIID riid, void** ppvObject )
{
    TRACE("%p %ld %s %p\n", hwnd, dwObjectID,
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
    IServiceProvider *sp;
    IAccessible *acc2;
    IDispatch *parent;
    IOleWindow *ow;
    VARIANT v, cid;
    HRESULT hres;

    TRACE("%p %p\n", acc, phwnd);

    hres = IAccessible_QueryInterface(acc, &IID_IOleWindow, (void**)&ow);
    if(SUCCEEDED(hres)) {
        hres = IOleWindow_GetWindow(ow, phwnd);
        IOleWindow_Release(ow);
        if(SUCCEEDED(hres) && *phwnd)
            return S_OK;
    }

    VariantInit(&v);
    variant_init_i4(&cid, CHILDID_SELF);
    hres = IAccessible_accNavigate(acc, NAVDIR_INTERNAL_HWND, cid, &v);
    if(SUCCEEDED(hres)) {
        if(hres == S_OK && V_VT(&v) == VT_I4 && V_I4(&v)) {
            *phwnd = LongToHandle(V_I4(&v));
            return S_OK;
        }
        /* native leaks v here */
        VariantClear(&v);
    }

    hres = IAccessible_QueryInterface(acc, &IID_IServiceProvider, (void**)&sp);
    if(SUCCEEDED(hres)) {
        hres = IServiceProvider_QueryService(sp, &SID_AccFromDAWrapper,
                &IID_IAccessible, (void**)&acc2);
        IServiceProvider_Release(sp);
    }
    if(FAILED(hres)) {
        acc2 = acc;
        IAccessible_AddRef(acc2);
    }

    while(1) {
        hres = IAccessible_QueryInterface(acc2, &IID_IOleWindow, (void**)&ow);
        if(SUCCEEDED(hres)) {
            hres = IOleWindow_GetWindow(ow, phwnd);
            IOleWindow_Release(ow);
            if(SUCCEEDED(hres)) {
                IAccessible_Release(acc2);
                return hres;
            }
        }

        hres = IAccessible_get_accParent(acc, &parent);
        IAccessible_Release(acc2);
        if(FAILED(hres))
            return hres;
        if(!parent) {
            *phwnd = NULL;
            return hres;
        }

        hres = IDispatch_QueryInterface(parent, &IID_IAccessible, (void**)&acc2);
        IDispatch_Release(parent);
        if(FAILED(hres))
            return hres;
        acc = acc2;
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,
                    LPVOID lpvReserved)
{
    TRACE("%p, %ld, %p\n", hinstDLL, fdwReason, lpvReserved);

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
    *pVersion = MAKELONG(0,7); /* Windows 7 version of oleacc: 7.0.0.0 */
    *pBuild = MAKELONG(0,0);
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

    TRACE("%lu %p %u\n", role, lpRole, rolemax);

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

    TRACE("%lu %p %u\n", role, lpRole, rolemax);

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

    TRACE("%lx %p %u\n", state_bit, state_str, state_str_len);

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

    TRACE("%lx %p %u\n", state_bit, state_str, state_str_len);

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

    TRACE("%p %ld %ld %p %p\n", container, start, count, children, children_cnt);

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
