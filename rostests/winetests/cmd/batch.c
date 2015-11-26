/*
 * Copyright 2009 Dan Kegel
 * Copyright 2010 Jacek Caban for CodeWeavers
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

//#include <windows.h>
#include <stdio.h>

#include <wine/test.h>
#include <winnls.h>

static char workdir[MAX_PATH];
static DWORD workdir_len;
static char drive[2];
static const DWORD drive_len = sizeof(drive)/sizeof(drive[0]);
static char path[MAX_PATH];
static DWORD path_len;
static char shortpath[MAX_PATH];
static DWORD shortpath_len;

/* Convert to DOS line endings, and substitute escaped whitespace chars with real ones */
static const char* convert_input_data(const char *data, DWORD size, DWORD *new_size)
{
    static const char escaped_space[] = {'@','s','p','a','c','e','@'};
    static const char escaped_tab[]   = {'@','t','a','b','@'};
    DWORD i, eol_count = 0;
    char *ptr, *new_data;

    for (i = 0; i < size; i++)
        if (data[i] == '\n') eol_count++;

    ptr = new_data = HeapAlloc(GetProcessHeap(), 0, size + eol_count + 1);

    for (i = 0; i < size; i++) {
        switch (data[i]) {
            case '\n':
                if (data[i-1] != '\r')
                    *ptr++ = '\r';
                *ptr++ = '\n';
                break;
            case '@':
                if (data + i + sizeof(escaped_space) - 1 < data + size
                        && !memcmp(data + i, escaped_space, sizeof(escaped_space))) {
                    *ptr++ = ' ';
                    i += sizeof(escaped_space) - 1;
                } else if (data + i + sizeof(escaped_tab) - 1 < data + size
                        && !memcmp(data + i, escaped_tab, sizeof(escaped_tab))) {
                    *ptr++ = '\t';
                    i += sizeof(escaped_tab) - 1;
                } else {
                    *ptr++ = data[i];
                }
                break;
            default:
                *ptr++ = data[i];
        }
    }
    *ptr = '\0';

    *new_size = strlen(new_data);
    return new_data;
}

static BOOL run_cmd(const char *cmd_data, DWORD cmd_size)
{
    SECURITY_ATTRIBUTES sa = {sizeof(sa), 0, TRUE};
    char command[] = "test.cmd";
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    HANDLE file,fileerr;
    DWORD size;
    BOOL bres;

    file = CreateFileA("test.cmd", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;

    bres = WriteFile(file, cmd_data, cmd_size, &size, NULL);
    CloseHandle(file);
    ok(bres, "Could not write to file: %u\n", GetLastError());
    if(!bres)
        return FALSE;

    file = CreateFileA("test.out", GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, &sa, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;

    fileerr = CreateFileA("test.err", GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, &sa, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(fileerr != INVALID_HANDLE_VALUE, "CreateFile stderr failed\n");
    if(fileerr == INVALID_HANDLE_VALUE)
        return FALSE;

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = file;
    si.hStdError = fileerr;
    bres = CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    ok(bres, "CreateProcess failed: %u\n", GetLastError());
    if(!bres) {
        DeleteFileA("test.out");
        return FALSE;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(file);
    CloseHandle(fileerr);
    DeleteFileA("test.cmd");
    return TRUE;
}

static DWORD map_file(const char *file_name, const char **ret)
{
    HANDLE file, map;
    DWORD size;

    file = CreateFileA(file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %08x\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return 0;

    size = GetFileSize(file, NULL);

    map = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    ok(map != NULL, "CreateFileMappingA(%s) failed: %u\n", file_name, GetLastError());
    if(!map)
        return 0;

    *ret = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    ok(*ret != NULL, "MapViewOfFile failed: %u\n", GetLastError());
    CloseHandle(map);
    if(!*ret)
        return 0;

    return size;
}

static const char *compare_line(const char *out_line, const char *out_end, const char *exp_line,
        const char *exp_end)
{
    const char *out_ptr = out_line, *exp_ptr = exp_line;
    const char *err = NULL;

    static const char pwd_cmd[] = {'@','p','w','d','@'};
    static const char drive_cmd[] = {'@','d','r','i','v','e','@'};
    static const char path_cmd[]  = {'@','p','a','t','h','@'};
    static const char shortpath_cmd[]  = {'@','s','h','o','r','t','p','a','t','h','@'};
    static const char space_cmd[] = {'@','s','p','a','c','e','@'};
    static const char tab_cmd[]   = {'@','t','a','b','@'};
    static const char or_broken_cmd[] = {'@','o','r','_','b','r','o','k','e','n','@'};

    while(exp_ptr < exp_end) {
        if(*exp_ptr == '@') {
            if(exp_ptr+sizeof(pwd_cmd) <= exp_end
                    && !memcmp(exp_ptr, pwd_cmd, sizeof(pwd_cmd))) {
                exp_ptr += sizeof(pwd_cmd);
                if(out_end-out_ptr < workdir_len
                   || (CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, out_ptr, workdir_len,
                       workdir, workdir_len) != CSTR_EQUAL)) {
                    err = out_ptr;
                }else {
                    out_ptr += workdir_len;
                    continue;
                }
            } else if(exp_ptr+sizeof(drive_cmd) <= exp_end
                    && !memcmp(exp_ptr, drive_cmd, sizeof(drive_cmd))) {
                exp_ptr += sizeof(drive_cmd);
                if(out_end-out_ptr < drive_len
                   || (CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                      out_ptr, drive_len, drive, drive_len) != CSTR_EQUAL)) {
                    err = out_ptr;
                }else {
                    out_ptr += drive_len;
                    continue;
                }
            } else if(exp_ptr+sizeof(path_cmd) <= exp_end
                    && !memcmp(exp_ptr, path_cmd, sizeof(path_cmd))) {
                exp_ptr += sizeof(path_cmd);
                if(out_end-out_ptr < path_len
                   || (CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                      out_ptr, path_len, path, path_len) != CSTR_EQUAL)) {
                    err = out_ptr;
                }else {
                    out_ptr += path_len;
                    continue;
                }
            } else if(exp_ptr+sizeof(shortpath_cmd) <= exp_end
                    && !memcmp(exp_ptr, shortpath_cmd, sizeof(shortpath_cmd))) {
                exp_ptr += sizeof(shortpath_cmd);
                if(out_end-out_ptr < shortpath_len
                   || (CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                      out_ptr, shortpath_len, shortpath, shortpath_len) != CSTR_EQUAL)) {
                    err = out_ptr;
                }else {
                    out_ptr += shortpath_len;
                    continue;
                }
            }else if(exp_ptr+sizeof(space_cmd) <= exp_end
                    && !memcmp(exp_ptr, space_cmd, sizeof(space_cmd))) {
                exp_ptr += sizeof(space_cmd);
                if(out_ptr < out_end && *out_ptr == ' ') {
                    out_ptr++;
                    continue;
                } else {
                    err = out_end;
                }
            }else if(exp_ptr+sizeof(tab_cmd) <= exp_end
                    && !memcmp(exp_ptr, tab_cmd, sizeof(tab_cmd))) {
                exp_ptr += sizeof(tab_cmd);
                if(out_ptr < out_end && *out_ptr == '\t') {
                    out_ptr++;
                    continue;
                } else {
                    err = out_end;
                }
            }else if(exp_ptr+sizeof(or_broken_cmd) <= exp_end
                     && !memcmp(exp_ptr, or_broken_cmd, sizeof(or_broken_cmd))) {
                if(out_ptr == out_end)
                    return NULL;
                else
                    err = out_ptr;
            }else if(out_ptr == out_end || *out_ptr != *exp_ptr)
                err = out_ptr;
        }else if(out_ptr == out_end || *out_ptr != *exp_ptr) {
            err = out_ptr;
        }

        if(err) {
            if(!broken(1))
                return err;

            while(exp_ptr+sizeof(or_broken_cmd) <= exp_end && memcmp(exp_ptr, or_broken_cmd, sizeof(or_broken_cmd)))
                exp_ptr++;
            if(!exp_ptr)
                return err;

            exp_ptr += sizeof(or_broken_cmd);
            out_ptr = out_line;
            err = NULL;
            continue;
        }

        exp_ptr++;
        out_ptr++;
    }

    if(exp_ptr != exp_end)
        return out_ptr;
    else if(out_ptr != out_end)
        return exp_end;

    return NULL;
}

static void test_output(const char *out_data, DWORD out_size, const char *exp_data, DWORD exp_size)
{
    const char *out_ptr = out_data, *exp_ptr = exp_data, *out_nl, *exp_nl, *err;
    DWORD line = 0;
    static const char todo_wine_cmd[] = {'@','t','o','d','o','_','w','i','n','e','@'};
    static const char resync_cmd[] = {'-','-','-'};
    BOOL is_todo_wine, is_out_resync, is_exp_resync;

    while(out_ptr < out_data+out_size && exp_ptr < exp_data+exp_size) {
        line++;

        for(exp_nl = exp_ptr; exp_nl < exp_data+exp_size && *exp_nl != '\r' && *exp_nl != '\n'; exp_nl++);
        for(out_nl = out_ptr; out_nl < out_data+out_size && *out_nl != '\r' && *out_nl != '\n'; out_nl++);

        is_todo_wine = (exp_ptr+sizeof(todo_wine_cmd) <= exp_nl &&
                        !memcmp(exp_ptr, todo_wine_cmd, sizeof(todo_wine_cmd)));
        if (is_todo_wine) {
            exp_ptr += sizeof(todo_wine_cmd);
            winetest_start_todo("wine");
        }
        is_exp_resync=(exp_ptr+sizeof(resync_cmd) <= exp_nl &&
                       !memcmp(exp_ptr, resync_cmd, sizeof(resync_cmd)));
        is_out_resync=(out_ptr+sizeof(resync_cmd) <= out_nl &&
                       !memcmp(out_ptr, resync_cmd, sizeof(resync_cmd)));

        err = compare_line(out_ptr, out_nl, exp_ptr, exp_nl);
        if(err == out_nl)
            ok(0, "unexpected end of line %d (got '%.*s', wanted '%.*s')\n",
               line, (int)(out_nl-out_ptr), out_ptr, (int)(exp_nl-exp_ptr), exp_ptr);
        else if(err == exp_nl)
            ok(0, "excess characters on line %d (got '%.*s', wanted '%.*s')\n",
               line, (int)(out_nl-out_ptr), out_ptr, (int)(exp_nl-exp_ptr), exp_ptr);
        else if (!err && is_todo_wine && is_out_resync && is_exp_resync)
            /* Consider that the todo_wine was to deal with extra lines,
             * not for the resync line itself
             */
            err = NULL;
        else
            ok(!err, "unexpected char 0x%x position %d in line %d (got '%.*s', wanted '%.*s')\n",
               (err ? *err : 0), (err ? (int)(err-out_ptr) : -1), line, (int)(out_nl-out_ptr), out_ptr, (int)(exp_nl-exp_ptr), exp_ptr);

        if(is_todo_wine) winetest_end_todo("wine");

        if (is_exp_resync && err && is_todo_wine)
        {
            exp_ptr -= sizeof(todo_wine_cmd);
            /* If we rewind to the beginning of the line, don't increment line number */
            line--;
        }
        else if (!is_exp_resync || !err)
        {
            exp_ptr = exp_nl+1;
            if(exp_nl+1 < exp_data+exp_size && exp_nl[0] == '\r' && exp_nl[1] == '\n')
                exp_ptr++;
        }

        if (!is_out_resync || !err)
        {
            out_ptr = out_nl+1;
            if(out_nl+1 < out_data+out_size && out_nl[0] == '\r' && out_nl[1] == '\n')
                out_ptr++;
        }
    }

    ok(exp_ptr >= exp_data+exp_size, "unexpected end of output in line %d, missing %s\n", line, exp_ptr);
    ok(out_ptr >= out_data+out_size, "too long output, got additional %s\n", out_ptr);
}

static void run_test(const char *cmd_data, DWORD cmd_size, const char *exp_data, DWORD exp_size)
{
    const char *out_data, *actual_cmd_data;
    DWORD out_size, actual_cmd_size;

    actual_cmd_data = convert_input_data(cmd_data, cmd_size, &actual_cmd_size);
    if(!actual_cmd_size || !actual_cmd_data)
        goto cleanup;

    if(!run_cmd(actual_cmd_data, actual_cmd_size))
        goto cleanup;

    out_size = map_file("test.out", &out_data);
    if(out_size) {
        test_output(out_data, out_size, exp_data, exp_size);
        UnmapViewOfFile(out_data);
    }
    DeleteFileA("test.out");
    DeleteFileA("test.err");

cleanup:
    HeapFree(GetProcessHeap(), 0, (LPVOID)actual_cmd_data);
}

static void run_from_file(const char *file_name)
{
    char out_name[MAX_PATH];
    const char *test_data, *out_data;
    DWORD test_size, out_size;

    test_size = map_file(file_name, &test_data);
    if(!test_size) {
        ok(0, "Could not map file %s: %u\n", file_name, GetLastError());
        return;
    }

    sprintf(out_name, "%s.exp", file_name);
    out_size = map_file(out_name, &out_data);
    if(!out_size) {
        ok(0, "Could not map file %s: %u\n", out_name, GetLastError());
        UnmapViewOfFile(test_data);
        return;
    }

    run_test(test_data, test_size, out_data, out_size);

    UnmapViewOfFile(test_data);
    UnmapViewOfFile(out_data);
}

static DWORD load_resource(const char *name, const char *type, const char **ret)
{
    const char *res;
    HRSRC src;
    DWORD size;

    src = FindResourceA(NULL, name, type);
    ok(src != NULL, "Could not find resource %s: %u\n", name, GetLastError());
    if(!src)
        return 0;

    res = LoadResource(NULL, src);
    size = SizeofResource(NULL, src);
    while(size && !res[size-1])
        size--;

    *ret = res;
    return size;
}

static BOOL WINAPI test_enum_proc(HMODULE module, LPCSTR type, LPSTR name, LONG_PTR param)
{
    const char *cmd_data, *out_data;
    DWORD cmd_size, out_size;
    char res_name[100];

    trace("running %s test...\n", name);

    cmd_size = load_resource(name, type, &cmd_data);
    if(!cmd_size)
        return TRUE;

    sprintf(res_name, "%s.exp", name);
    out_size = load_resource(res_name, "TESTOUT", &out_data);
    if(!out_size)
        return TRUE;

    run_test(cmd_data, cmd_size, out_data, out_size);
    return TRUE;
}

static int cmd_available(void)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmd[] = "cmd /c exit 0";

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return TRUE;
    }
    return FALSE;
}

START_TEST(batch)
{
    int argc;
    char **argv;

    if (!cmd_available()) {
        win_skip("cmd not installed, skipping cmd tests\n");
        return;
    }

    workdir_len = GetCurrentDirectoryA(sizeof(workdir), workdir);
    drive[0] = workdir[0];
    drive[1] = workdir[1]; /* Should be ':' */
    memcpy(path, workdir + drive_len, (workdir_len - drive_len) * sizeof(drive[0]));

    /* Only add trailing backslash to 'path' for non-root directory */
    if (workdir_len - drive_len > 1) {
        path[workdir_len - drive_len] = '\\';
        path_len = workdir_len - drive_len + 1;
    } else {
        path_len = 1; /* \ */
    }
    shortpath_len = GetShortPathNameA(path, shortpath,
                                      sizeof(shortpath)/sizeof(shortpath[0]));

    argc = winetest_get_mainargs(&argv);
    if(argc > 2)
        run_from_file(argv[2]);
    else
        EnumResourceNamesA(NULL, "TESTCMD", test_enum_proc, 0);
}
