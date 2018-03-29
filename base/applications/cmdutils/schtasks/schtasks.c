/*
 * Copyright 2012 Detlef Riekenberg
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
#include "taskschd.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(schtasks);

static const WCHAR change_optW[] = {'/','c','h','a','n','g','e',0};
static const WCHAR create_optW[] = {'/','c','r','e','a','t','e',0};
static const WCHAR delete_optW[] = {'/','d','e','l','e','t','e',0};
static const WCHAR enable_optW[] = {'/','e','n','a','b','l','e',0};
static const WCHAR f_optW[] = {'/','f',0};
static const WCHAR ru_optW[] = {'/','r','u',0};
static const WCHAR tn_optW[] = {'/','t','n',0};
static const WCHAR tr_optW[] = {'/','t','r',0};
static const WCHAR xml_optW[] = {'/','x','m','l',0};

static ITaskFolder *get_tasks_root_folder(void)
{
    ITaskService *service;
    ITaskFolder *root;
    VARIANT empty;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                            &IID_ITaskService, (void**)&service);
    if (FAILED(hres))
        return NULL;

    V_VT(&empty) = VT_EMPTY;
    hres = ITaskService_Connect(service, empty, empty, empty, empty);
    if (FAILED(hres)) {
        FIXME("Connect failed: %08x\n", hres);
        return NULL;
    }

    hres = ITaskService_GetFolder(service, NULL, &root);
    ITaskService_Release(service);
    if (FAILED(hres)) {
        FIXME("GetFolder failed: %08x\n", hres);
        return NULL;
    }

    return root;
}

static IRegisteredTask *get_registered_task(const WCHAR *name)
{
    IRegisteredTask *registered_task;
    ITaskFolder *root;
    BSTR str;
    HRESULT hres;

    root = get_tasks_root_folder();
    if (!root)
        return NULL;

    str = SysAllocString(name);
    hres = ITaskFolder_GetTask(root, str, &registered_task);
    SysFreeString(str);
    ITaskFolder_Release(root);
    if (FAILED(hres)) {
        FIXME("GetTask failed: %08x\n", hres);
        return NULL;
    }

    return registered_task;
}

static BSTR read_file_to_bstr(const WCHAR *file_name)
{
    LARGE_INTEGER file_size;
    DWORD read_size, size;
    unsigned char *data;
    HANDLE file;
    BOOL r = FALSE;
    BSTR ret;

    file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        FIXME("Could not open file\n");
        return NULL;
    }

    if (!GetFileSizeEx(file, &file_size) || !file_size.QuadPart) {
        FIXME("Empty file\n");
        CloseHandle(file);
        return NULL;
    }

    data = HeapAlloc(GetProcessHeap(), 0, file_size.QuadPart);
    if (data)
        r = ReadFile(file, data, file_size.QuadPart, &read_size, NULL);
    CloseHandle(file);
    if (!r) {
        FIXME("Read filed\n");
        HeapFree(GetProcessHeap(), 0, data);
        return NULL;
    }

    if (read_size > 2 && data[0] == 0xff && data[1] == 0xfe) { /* UTF-16 BOM */
        ret = SysAllocStringLen((const WCHAR *)(data + 2), (read_size - 2) / sizeof(WCHAR));
    }else {
        size = MultiByteToWideChar(CP_ACP, 0, (const char *)data, read_size, NULL, 0);
        ret = SysAllocStringLen(NULL, size);
        if (ret)
            MultiByteToWideChar(CP_ACP, 0, (const char *)data, read_size, ret, size);
    }
    HeapFree(GetProcessHeap(), 0, data);

    return ret;
}

static int change_command(int argc, WCHAR *argv[])
{
    BOOL have_option = FALSE, enable = FALSE;
    const WCHAR *task_name = NULL;
    IRegisteredTask *task;
    HRESULT hres;

    while (argc) {
        if(!strcmpiW(argv[0], tn_optW)) {
            if (argc < 2) {
                FIXME("Missing /tn value\n");
                return 1;
            }

            if (task_name) {
                FIXME("Duplicated /tn argument\n");
                return 1;
            }

            task_name = argv[1];
            argc -= 2;
            argv += 2;
        }else if (!strcmpiW(argv[0], enable_optW)) {
            enable = TRUE;
            have_option = TRUE;
            argc--;
            argv++;
        }else if (!strcmpiW(argv[0], tr_optW)) {
            if (argc < 2) {
                FIXME("Missing /tr value\n");
                return 1;
            }

            FIXME("Unsupported /tr option %s\n", debugstr_w(argv[1]));
            have_option = TRUE;
            argc -= 2;
            argv += 2;
        }else {
            FIXME("Unsupported arguments %s\n", debugstr_w(argv[0]));
            return 1;
        }
    }

    if (!task_name) {
        FIXME("Missing /tn option\n");
        return 1;
    }

    if (!have_option) {
        FIXME("Missing change options\n");
        return 1;
    }

    task = get_registered_task(task_name);
    if (!task)
        return 1;

    if (enable) {
        hres = IRegisteredTask_put_Enabled(task, VARIANT_TRUE);
        if (FAILED(hres)) {
            IRegisteredTask_Release(task);
            FIXME("put_Enabled failed: %08x\n", hres);
            return 1;
        }
    }

    IRegisteredTask_Release(task);
    return 0;
}

static int create_command(int argc, WCHAR *argv[])
{
    const WCHAR *task_name = NULL, *xml_file = NULL;
    ITaskFolder *root = NULL;
    LONG flags = TASK_CREATE;
    IRegisteredTask *task;
    VARIANT empty;
    BSTR str, xml;
    HRESULT hres;

    while (argc) {
        if (!strcmpiW(argv[0], xml_optW)) {
            if (argc < 2) {
                FIXME("Missing /xml value\n");
                return 1;
            }

            if (xml_file) {
                FIXME("Duplicated /xml argument\n");
                return 1;
            }

            xml_file = argv[1];
            argc -= 2;
            argv += 2;
        }else if(!strcmpiW(argv[0], tn_optW)) {
            if (argc < 2) {
                FIXME("Missing /tn value\n");
                return 1;
            }

            if (task_name) {
                FIXME("Duplicated /tn argument\n");
                return 1;
            }

            task_name = argv[1];
            argc -= 2;
            argv += 2;
        }else if(!strcmpiW(argv[0], f_optW)) {
            flags = TASK_CREATE_OR_UPDATE;
            argc--;
            argv++;
        }else if (!strcmpiW(argv[0], ru_optW)) {
            if (argc < 2) {
                FIXME("Missing /ru value\n");
                return 1;
            }

            FIXME("Unsupported /ru option %s\n", debugstr_w(argv[1]));
            argc -= 2;
            argv += 2;
        }else {
            FIXME("Unsupported argument %s\n", debugstr_w(argv[0]));
            return 1;
        }
    }

    if (!task_name) {
        FIXME("Missing /tn argument\n");
        return 1;
    }

    if (!xml_file) {
        FIXME("Missing /xml argument\n");
        return E_FAIL;
    }

    xml = read_file_to_bstr(xml_file);
    if (!xml)
        return 1;

    root = get_tasks_root_folder();
    if (!root) {
        SysFreeString(xml);
        return 1;
    }

    V_VT(&empty) = VT_EMPTY;
    str = SysAllocString(task_name);
    hres = ITaskFolder_RegisterTask(root, str, xml, flags, empty, empty,
                                    TASK_LOGON_NONE, empty, &task);
    SysFreeString(str);
    SysFreeString(xml);
    ITaskFolder_Release(root);
    if (FAILED(hres))
        return 1;

    IRegisteredTask_Release(task);
    return 0;
}

static int delete_command(int argc, WCHAR *argv[])
{
    const WCHAR *task_name = NULL;
    ITaskFolder *root = NULL;
    BSTR str;
    HRESULT hres;

    while (argc) {
        if (!strcmpiW(argv[0], f_optW)) {
            TRACE("force opt\n");
            argc--;
            argv++;
        }else if(!strcmpiW(argv[0], tn_optW)) {
            if (argc < 2) {
                FIXME("Missing /tn value\n");
                return 1;
            }

            if (task_name) {
                FIXME("Duplicated /tn argument\n");
                return 1;
            }

            task_name = argv[1];
            argc -= 2;
            argv += 2;
        }else {
            FIXME("Unsupported argument %s\n", debugstr_w(argv[0]));
            return 1;
        }
    }

    if (!task_name) {
        FIXME("Missing /tn argument\n");
        return 1;
    }

    root = get_tasks_root_folder();
    if (!root)
        return 1;

    str = SysAllocString(task_name);
    hres = ITaskFolder_DeleteTask(root, str, 0);
    SysFreeString(str);
    ITaskFolder_Release(root);
    if (FAILED(hres))
        return 1;

    return 0;
}

int wmain(int argc, WCHAR *argv[])
{
    int i, ret = 0;

    for (i = 0; i < argc; i++)
        TRACE(" %s", wine_dbgstr_w(argv[i]));
    TRACE("\n");

    CoInitialize(NULL);

    if (argc < 2)
        FIXME("Print current tasks state\n");
    else if (!strcmpiW(argv[1], change_optW))
        ret = change_command(argc - 2, argv + 2);
    else if (!strcmpiW(argv[1], create_optW))
        ret = create_command(argc - 2, argv + 2);
    else if (!strcmpiW(argv[1], delete_optW))
        ret = delete_command(argc - 2, argv + 2);
    else
        FIXME("Unsupported command %s\n", debugstr_w(argv[1]));

    CoUninitialize();
    return ret;
}
