/*
 * PROJECT:     VerOS NT
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        tools/qemu-harness/abi.c
 * PURPOSE:     Log NT syscalls and ntoskrnl/hal/wdf calls from a QEMU guest
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <windows.h>

#define QEMU_PLUGIN_VERSION 7

typedef uint64_t qemu_plugin_id_t;

enum qemu_plugin_cb_flags
{
    QEMU_PLUGIN_CB_NO_REGS,
    QEMU_PLUGIN_CB_R_REGS,
    QEMU_PLUGIN_CB_RW_REGS,
    QEMU_PLUGIN_CB_RW_REGS_PC,
};

struct qemu_plugin_tb;
struct qemu_plugin_insn;
struct qemu_plugin_register;

typedef void (*qemu_plugin_vcpu_udata_cb_t)(unsigned int vcpu_index, void *userdata);
typedef void (*qemu_plugin_vcpu_tb_trans_cb_t)(struct qemu_plugin_tb *tb);

typedef struct
{
    void *handle;
    const char *name;
    const char *feature;
    bool is_readonly;
} RegDesc;

typedef struct
{
    char *data;
    unsigned int len;
} GArray;

typedef struct
{
    unsigned char *data;
    unsigned int len;
} GByteArray;

enum
{
    SITE_SYSCALL = 1,
    SITE_EXPORT = 2,
    SITE_RETURN = 3,
};

typedef struct
{
    int kind;
    char symbol[160];
    char insn_name[16];
    uint64_t vaddr;
    size_t size;
    int has_number;
    int has_args;
    int at_tb_start;
    uint32_t number;
} Site;

typedef struct
{
    char name[32];
    uint64_t preferred;
    uint64_t size;
    uint64_t actual;
    int found;
} ImageEnt;

typedef struct
{
    uint64_t address;
    uint64_t preferred;
    int image;
    char symbol[160];
} ExportEnt;

typedef struct
{
    uint64_t ret_pc;
    char symbol[160];
} Pending;

static uint64_t g_last_pc;
static int g_last_kind;
static int g_repeats;

static bool
should_log(uint64_t pc, int kind)
{
    if (pc == g_last_pc && kind == g_last_kind)
    {
        g_repeats++;
        return g_repeats <= 4;
    }
    g_last_pc = pc;
    g_last_kind = kind;
    g_repeats = 1;
    return true;
}
static FILE *g_log;
static ExportEnt g_exports[4096];
static int g_export_count;
static ImageEnt g_images[8];
static int g_image_count;
static int g_located;
static int g_locate_attempts;
static Pending g_pending[32];
static int g_pending_count;

#define WIN_VCPU 16
#define WIN_DEPTH 8
#define WIN_RET_SLOTS 8192
#define WIN_NAME_SLOTS 8192

static int g_windows;
static uint64_t g_log_bytes;
static uint64_t g_log_cap = 2ULL * 1024ULL * 1024ULL * 1024ULL;
static char *g_syscall_by_num[WIN_NAME_SLOTS];
static char g_name_arena[256 * 1024];
static size_t g_name_used;
static void *g_rcx;
static void *g_r8;
static void *g_r9;
static void *g_r10;
static const char *g_mark_path;
static int g_marked;

typedef struct
{
    uint32_t code;
    char name[48];
} StatusEnt;

static StatusEnt g_status[64];
static int g_status_count;

typedef struct
{
    int live;
    uint64_t ret_pc;
    uint32_t number;
    int has_args;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t pid;
    char image[160];
    char path[240];
} WinCall;

static WinCall g_wincall[WIN_VCPU][WIN_DEPTH];
static int g_win_next[WIN_VCPU];
static uint64_t g_ret_hash[WIN_RET_SLOTS];

static bool (*q_read_mem)(uint64_t addr, GByteArray *data, size_t len);

static void *g_eax;
static void *g_edx;
static bool g_regs_ready;

static size_t (*q_tb_n_insns)(const struct qemu_plugin_tb *tb);
static struct qemu_plugin_insn *(*q_tb_get_insn)(const struct qemu_plugin_tb *tb, size_t idx);
static size_t (*q_insn_data)(const struct qemu_plugin_insn *insn, void *dest, size_t len);
static size_t (*q_insn_size)(const struct qemu_plugin_insn *insn);
static uint64_t (*q_insn_vaddr)(const struct qemu_plugin_insn *insn);
static void (*q_register_tb)(qemu_plugin_id_t id, qemu_plugin_vcpu_tb_trans_cb_t cb);
static void (*q_register_exec)(struct qemu_plugin_insn *insn,
                               qemu_plugin_vcpu_udata_cb_t cb,
                               int flags,
                               void *userdata);
static GArray *(*q_get_registers)(void);
static bool (*q_read_register)(struct qemu_plugin_register *handle, GByteArray *buf);
static GByteArray *(*g_byte_array_new)(void);
static void (*g_byte_array_unref)(GByteArray *array);
static void (*g_array_unref)(GArray *array);

static qemu_plugin_id_t g_id;

static void
json_escape(FILE *out, const char *text)
{
    const unsigned char *p;

    fputc('"', out);
    for (p = (const unsigned char *)text; *p != '\0'; p++)
    {
        if (*p == '"' || *p == '\\')
        {
            fputc('\\', out);
            fputc(*p, out);
        }
        else if (*p >= 0x20)
        {
            fputc(*p, out);
        }
    }
    fputc('"', out);
}

static void
load_map(const char *path)
{
    FILE *file;
    char line[256];

    file = fopen(path, "r");
    if (file == NULL)
    {
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL && g_export_count < 4096)
    {
        char symbol[160];
        unsigned long long address;

        if (strncmp(line, "# image ", 8) == 0 && g_image_count < 8)
        {
            char name[32];
            unsigned long long preferred;
            unsigned long long size;

            if (sscanf(line + 8, "%31s 0x%llx 0x%llx", name, &preferred, &size) == 3)
            {
                strncpy(g_images[g_image_count].name, name, 31);
                g_images[g_image_count].name[31] = '\0';
                g_images[g_image_count].preferred = (uint64_t)preferred;
                g_images[g_image_count].size = (uint64_t)size;
                g_images[g_image_count].actual = 0;
                g_images[g_image_count].found = 0;
                g_image_count++;
            }
            continue;
        }
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
        {
            continue;
        }
        if (sscanf(line, "%159s 0x%llx", symbol, &address) == 2 ||
            sscanf(line, "%159s %llx", symbol, &address) == 2)
        {
            char *bang;
            int image;

            image = -1;
            bang = strchr(symbol, '!');
            if (bang != NULL)
            {
                int n;

                for (n = 0; n < g_image_count; n++)
                {
                    size_t len;

                    len = strlen(g_images[n].name);
                    if (strncmp(symbol, g_images[n].name, len) == 0 && symbol[len] == '!')
                    {
                        image = n;
                        break;
                    }
                }
            }
            g_exports[g_export_count].address = (uint64_t)address;
            g_exports[g_export_count].preferred = (uint64_t)address;
            g_exports[g_export_count].image = image;
            strncpy(g_exports[g_export_count].symbol, symbol, 159);
            g_exports[g_export_count].symbol[159] = '\0';
            g_export_count++;
        }
    }
    fclose(file);
}

static const char *
export_at(uint64_t address)
{
    int i;

    for (i = 0; i < g_export_count; i++)
    {
        if (g_exports[i].address == address)
        {
            return g_exports[i].symbol;
        }
    }
    return NULL;
}

static bool read_mem(uint64_t addr, void *dest, size_t len);

static int
pe_export_name(uint64_t base, char *name, size_t name_len)
{
    uint8_t dos[64];
    uint8_t pe[512];
    uint32_t lfanew;
    uint16_t magic;
    uint32_t export_rva;
    uint32_t name_rva;
    uint8_t dir[40];

    if (!read_mem(base, dos, sizeof(dos)) || dos[0] != 'M' || dos[1] != 'Z')
    {
        return 0;
    }
    memcpy(&lfanew, dos + 0x3C, sizeof(lfanew));
    if (lfanew > 0x400 || !read_mem(base + lfanew, pe, sizeof(pe)))
    {
        return 0;
    }
    if (pe[0] != 'P' || pe[1] != 'E')
    {
        return 0;
    }
    memcpy(&magic, pe + 24, sizeof(magic));
    if (magic != 0x20B)
    {
        return 0;
    }
    memcpy(&export_rva, pe + 24 + 112, sizeof(export_rva));
    if (export_rva == 0 || !read_mem(base + export_rva, dir, sizeof(dir)))
    {
        return 0;
    }
    memcpy(&name_rva, dir + 12, sizeof(name_rva));
    if (name_rva == 0 || !read_mem(base + name_rva, name, name_len))
    {
        return 0;
    }
    name[name_len - 1] = '\0';
    return 1;
}

static void
locate_images(void)
{
    uint64_t addr;
    int i;

    if (g_located || g_image_count == 0)
    {
        return;
    }
    for (addr = 0xFFFFF80000000000ULL; addr < 0xFFFFF80008000000ULL; addr += 0x1000ULL)
    {
        char name[32];
        int n;

        if (!pe_export_name(addr, name, sizeof(name)))
        {
            continue;
        }
        for (n = 0; n < g_image_count; n++)
        {
            uint64_t slide;

            if (g_images[n].found || _stricmp(name, g_images[n].name) != 0)
            {
                continue;
            }
            g_images[n].actual = addr;
            g_images[n].found = 1;
            slide = addr - g_images[n].preferred;
            for (i = 0; i < g_export_count; i++)
            {
                if (g_exports[i].image == n)
                {
                    g_exports[i].address = g_exports[i].preferred + slide;
                }
            }
            if (g_log != NULL)
            {
                fprintf(g_log, "{\"kind\":\"image\",\"phase\":\"load\",\"symbol\":");
                json_escape(g_log, g_images[n].name);
                fprintf(g_log, ",\"pc\":\"0x%llx\",\"result\":\"seen\"}\n",
                        (unsigned long long)addr);
                fflush(g_log);
            }
        }
    }
    for (i = 0; i < g_image_count; i++)
    {
        if (g_images[i].found)
        {
            g_located = 1;
            return;
        }
    }
    g_locate_attempts++;
    if (g_locate_attempts >= 4)
    {
        g_located = 1;
        if (g_log != NULL)
        {
            fprintf(g_log, "{\"kind\":\"image\",\"phase\":\"load\",\"symbol\":\"none\",\"pc\":\"0x0\",\"result\":\"seen\"}\n");
            fflush(g_log);
        }
    }
}

static bool
decode_rip_indirect(const uint8_t *bytes, size_t size, uint64_t vaddr, uint64_t *target)
{
    size_t index;
    int32_t disp;
    uint64_t slot;

    index = 0;
    if (size >= 7 && bytes[0] >= 0x40 && bytes[0] <= 0x4F)
    {
        index = 1;
    }
    if (size < index + 6 || bytes[index] != 0xFF)
    {
        return false;
    }
    if (bytes[index + 1] != 0x15 && bytes[index + 1] != 0x25)
    {
        return false;
    }
    memcpy(&disp, bytes + index + 2, sizeof(disp));
    slot = vaddr + index + 6 + (int64_t)disp;
    return read_mem(slot, target, sizeof(*target));
}

static bool
decode_branch(const uint8_t *bytes, size_t size, uint64_t vaddr, uint64_t *target)
{
    if (size == 5 && (bytes[0] == 0xE8 || bytes[0] == 0xE9))
    {
        int32_t rel;

        memcpy(&rel, bytes + 1, sizeof(rel));
        *target = vaddr + size + (int64_t)rel;
        return true;
    }
    if (size == 3 && bytes[0] == 0xE8)
    {
        int16_t rel;

        memcpy(&rel, bytes + 1, sizeof(rel));
        *target = vaddr + size + rel;
        return true;
    }
    return false;
}

static const char *
syscall_name(const uint8_t *bytes, size_t size)
{
    if (size >= 2 && bytes[0] == 0x0F && bytes[1] == 0x34)
    {
        return "sysenter";
    }
    if (size >= 2 && bytes[0] == 0x0F && bytes[1] == 0x05)
    {
        return "syscall";
    }
    if (size >= 2 && bytes[0] == 0xCD && bytes[1] == 0x2E)
    {
        return "int2e";
    }
    return NULL;
}

static void *
bind_symbol(HMODULE module, const char *name)
{
    return (void *)GetProcAddress(module, name);
}

static bool
bind_qemu(void)
{
    HMODULE exe;
    HMODULE glib;

    exe = GetModuleHandleA(NULL);
    glib = LoadLibraryA("libglib-2.0-0.dll");
    if (exe == NULL)
    {
        return false;
    }

    q_tb_n_insns = bind_symbol(exe, "qemu_plugin_tb_n_insns");
    q_tb_get_insn = bind_symbol(exe, "qemu_plugin_tb_get_insn");
    q_insn_data = bind_symbol(exe, "qemu_plugin_insn_data");
    q_insn_size = bind_symbol(exe, "qemu_plugin_insn_size");
    q_insn_vaddr = bind_symbol(exe, "qemu_plugin_insn_vaddr");
    q_register_tb = bind_symbol(exe, "qemu_plugin_register_vcpu_tb_trans_cb");
    q_register_exec = bind_symbol(exe, "qemu_plugin_register_vcpu_insn_exec_cb");
    q_get_registers = bind_symbol(exe, "qemu_plugin_get_registers");
    q_read_register = bind_symbol(exe, "qemu_plugin_read_register");
    q_read_mem = bind_symbol(exe, "qemu_plugin_read_memory_vaddr");
    if (glib != NULL)
    {
        g_byte_array_new = bind_symbol(glib, "g_byte_array_new");
        g_byte_array_unref = bind_symbol(glib, "g_byte_array_unref");
        g_array_unref = bind_symbol(glib, "g_array_unref");
    }

    return q_tb_n_insns && q_tb_get_insn && q_insn_data && q_insn_size &&
           q_insn_vaddr && q_register_tb && q_register_exec;
}

static uint64_t
read_reg(void *handle)
{
    GByteArray *buf;
    uint64_t value;

    if (handle == NULL || q_read_register == NULL || g_byte_array_new == NULL)
    {
        return 0;
    }

    value = 0;
    buf = g_byte_array_new();
    if (q_read_register(handle, buf) && buf != NULL && buf->data != NULL && buf->len >= 4)
    {
        memcpy(&value, buf->data, buf->len >= 8 ? 8 : 4);
    }
    if (g_byte_array_unref != NULL && buf != NULL)
    {
        g_byte_array_unref(buf);
    }
    return value;
}

static void
cache_regs(void)
{
    GArray *list;
    unsigned int i;
    size_t stride;

    if (g_regs_ready || q_get_registers == NULL)
    {
        return;
    }

    stride = sizeof(RegDesc);
    list = q_get_registers();
    if (list == NULL || list->data == NULL)
    {
        return;
    }

    for (i = 0; i < list->len; i++)
    {
        RegDesc *reg;
        const char *name;

        reg = (RegDesc *)(list->data + (i * stride));
        name = reg->name;
        if (name == NULL)
        {
            continue;
        }
        if (_stricmp(name, "eax") == 0 || _stricmp(name, "rax") == 0)
        {
            g_eax = reg->handle;
        }
        else if (_stricmp(name, "ecx") == 0 || _stricmp(name, "rcx") == 0)
        {
            g_rcx = reg->handle;
        }
        else if (_stricmp(name, "edx") == 0 || _stricmp(name, "rdx") == 0)
        {
            g_edx = reg->handle;
        }
        else if (_stricmp(name, "r8") == 0)
        {
            g_r8 = reg->handle;
        }
        else if (_stricmp(name, "r9") == 0)
        {
            g_r9 = reg->handle;
        }
        else if (_stricmp(name, "r10") == 0)
        {
            g_r10 = reg->handle;
        }
    }
    if (g_array_unref != NULL)
    {
        g_array_unref(list);
    }
    g_regs_ready = true;
}

static char *
arena_copy(const char *text)
{
    size_t len;
    char *slot;

    len = strlen(text) + 1;
    if (g_name_used + len > sizeof(g_name_arena))
    {
        return NULL;
    }
    slot = g_name_arena + g_name_used;
    memcpy(slot, text, len);
    g_name_used += len;
    return slot;
}

static void
load_syscalls(const char *path)
{
    FILE *file;
    char line[256];

    file = fopen(path, "r");
    if (file == NULL)
    {
        return;
    }
    while (fgets(line, sizeof(line), file) != NULL)
    {
        unsigned int number;
        char name[128];
        char *copy;

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
        {
            continue;
        }
        if (sscanf(line, "%u %127s", &number, name) != 2)
        {
            continue;
        }
        if (number >= WIN_NAME_SLOTS)
        {
            continue;
        }
        copy = arena_copy(name);
        if (copy != NULL)
        {
            g_syscall_by_num[number] = copy;
        }
    }
    fclose(file);
}

static void
load_status(const char *path)
{
    FILE *file;
    char line[256];

    file = fopen(path, "r");
    if (file == NULL)
    {
        return;
    }
    while (fgets(line, sizeof(line), file) != NULL && g_status_count < 64)
    {
        unsigned int code;
        char name[48];

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
        {
            continue;
        }
        if (sscanf(line, "0x%x %47s", &code, name) != 2)
        {
            continue;
        }
        g_status[g_status_count].code = code;
        strncpy(g_status[g_status_count].name, name, 47);
        g_status[g_status_count].name[47] = '\0';
        g_status_count++;
    }
    fclose(file);
}

static void
status_text(uint32_t code, char *out, size_t len)
{
    int i;

    if (code == 0)
    {
        strncpy(out, "STATUS_SUCCESS", len - 1);
        out[len - 1] = '\0';
        return;
    }
    for (i = 0; i < g_status_count; i++)
    {
        if (g_status[i].code == code)
        {
            strncpy(out, g_status[i].name, len - 1);
            out[len - 1] = '\0';
            return;
        }
    }
    snprintf(out, len, "0x%08X", code);
}

static bool
read_mem(uint64_t addr, void *dest, size_t len)
{
    GByteArray *buf;
    bool ok;

    if (q_read_mem == NULL || g_byte_array_new == NULL || addr < 0x10000 || len == 0)
    {
        return false;
    }
    buf = g_byte_array_new();
    ok = false;
    if (q_read_mem(addr, buf, len) && buf != NULL && buf->data != NULL && buf->len >= len)
    {
        memcpy(dest, buf->data, len);
        ok = true;
    }
    if (g_byte_array_unref != NULL && buf != NULL)
    {
        g_byte_array_unref(buf);
    }
    return ok;
}

static void
utf16_to_ascii(const uint8_t *raw, size_t nbytes, char *out, size_t out_len)
{
    size_t i;
    size_t o;

    o = 0;
    for (i = 0; i + 1 < nbytes && o + 1 < out_len; i += 2)
    {
        if (raw[i] == 0 && raw[i + 1] == 0)
        {
            break;
        }
        if (raw[i + 1] == 0 && raw[i] >= 0x20 && raw[i] < 0x7F)
        {
            out[o++] = (char)raw[i];
        }
    }
    out[o] = '\0';
}

static void
read_unicode(uint64_t string_ptr, char *out, size_t out_len)
{
    uint8_t header[16];
    uint16_t nbytes;
    uint64_t buffer;
    uint8_t raw[400];

    out[0] = '\0';
    if (!read_mem(string_ptr, header, sizeof(header)))
    {
        return;
    }
    memcpy(&nbytes, header, sizeof(nbytes));
    memcpy(&buffer, header + 8, sizeof(buffer));
    if (nbytes < 2 || nbytes > sizeof(raw) || buffer < 0x10000)
    {
        return;
    }
    if (!read_mem(buffer, raw, nbytes))
    {
        return;
    }
    utf16_to_ascii(raw, nbytes, out, out_len);
}

static void
read_object_name(uint64_t object_attributes, char *out, size_t out_len)
{
    uint8_t header[32];
    uint64_t name;

    out[0] = '\0';
    if (!read_mem(object_attributes, header, sizeof(header)))
    {
        return;
    }
    memcpy(&name, header + 16, sizeof(name));
    read_unicode(name, out, out_len);
}

static int
path_kind(const char *name)
{
    if (name == NULL)
    {
        return 0;
    }
    if (strcmp(name, "NtQueryValueKey") == 0 || strcmp(name, "NtSetValueKey") == 0 ||
        strcmp(name, "NtDeleteValueKey") == 0)
    {
        return 2;
    }
    if (strstr(name, "Key") != NULL || strcmp(name, "NtCreateFile") == 0 ||
        strcmp(name, "NtOpenFile") == 0)
    {
        return 1;
    }
    return 0;
}

static void
remember_ret(uint64_t pc)
{
    uint32_t slot;
    int step;

    slot = (uint32_t)((pc >> 2) & (WIN_RET_SLOTS - 1));
    for (step = 0; step < 32; step++)
    {
        uint32_t index;

        index = (slot + (uint32_t)step) & (WIN_RET_SLOTS - 1);
        if (g_ret_hash[index] == 0 || g_ret_hash[index] == pc)
        {
            g_ret_hash[index] = pc;
            return;
        }
    }
}

static bool
is_remembered_ret(uint64_t pc)
{
    uint32_t slot;
    int step;

    slot = (uint32_t)((pc >> 2) & (WIN_RET_SLOTS - 1));
    for (step = 0; step < 32; step++)
    {
        uint32_t index;
        uint64_t found;

        index = (slot + (uint32_t)step) & (WIN_RET_SLOTS - 1);
        found = g_ret_hash[index];
        if (found == 0)
        {
            return false;
        }
        if (found == pc)
        {
            return true;
        }
    }
    return false;
}

static void
emit_windows(const WinCall *call, uint64_t eax)
{
    const char *name;
    char number_name[16];
    char status[48];

    if (g_log == NULL || g_log_bytes >= g_log_cap)
    {
        return;
    }
    snprintf(number_name, sizeof(number_name), "%u", call->number);
    name = number_name;
    if (call->number < WIN_NAME_SLOTS && g_syscall_by_num[call->number] != NULL)
    {
        name = g_syscall_by_num[call->number];
    }
    status_text((uint32_t)eax, status, sizeof(status));
    if (g_mark_path != NULL && !g_marked && (g_log_bytes & 255ULL) == 0 &&
        GetFileAttributesA(g_mark_path) != INVALID_FILE_ATTRIBUTES)
    {
        fputs("{\"phase\":\"workload\"}\n", g_log);
        fflush(g_log);
        g_marked = 1;
    }
    fprintf(g_log, "{\"syscall\":");
    json_escape(g_log, name);
    fprintf(g_log, ",\"status\":");
    json_escape(g_log, status);
    fprintf(g_log, ",\"n\":%u,\"phase\":\"return\"", call->number);
    if (call->has_args)
    {
        fprintf(g_log,
                ",\"a1\":\"0x%llx\",\"a2\":\"0x%llx\",\"a3\":\"0x%llx\",\"a4\":\"0x%llx\"",
                (unsigned long long)call->arg1, (unsigned long long)call->arg2,
                (unsigned long long)call->arg3, (unsigned long long)call->arg4);
    }
    if (call->pid != 0)
    {
        fprintf(g_log, ",\"pid\":%I64u", (unsigned long long)call->pid);
    }
    if (call->image[0] != '\0')
    {
        fprintf(g_log, ",\"image\":");
        json_escape(g_log, call->image);
    }
    if (call->path[0] != '\0')
    {
        fprintf(g_log, ",\"path\":");
        json_escape(g_log, call->path);
    }
    fprintf(g_log, "}\n");
    fflush(g_log);
    if ((g_log_bytes++ & 4095) == 0)
    {
        long pos;

        pos = ftell(g_log);
        if (pos > 0)
        {
            g_log_bytes = (uint64_t)pos;
        }
    }
}

static int stub_before(uint64_t vaddr, uint32_t *imm);

static void
on_windows(unsigned int vcpu, Site *site, uint64_t eax, uint64_t edx, uint64_t r8)
{
    int slot;
    const char *name;
    WinCall *call;

    if (vcpu >= WIN_VCPU)
    {
        vcpu = 0;
    }
    for (slot = WIN_DEPTH - 1; slot >= 0; slot--)
    {
        call = &g_wincall[vcpu][slot];
        if (call->live && call->ret_pc == site->vaddr)
        {
            emit_windows(call, eax);
            call->live = 0;
            break;
        }
    }
    if (site->kind != SITE_SYSCALL)
    {
        return;
    }
    if (!site->has_number)
    {
        uint32_t imm;

        if (!stub_before(site->vaddr, &imm))
        {
            return;
        }
        site->has_number = 1;
        site->number = imm;
        if (site->at_tb_start)
        {
            site->has_args = 1;
        }
    }
    slot = g_win_next[vcpu] % WIN_DEPTH;
    g_win_next[vcpu]++;
    call = &g_wincall[vcpu][slot];
    memset(call, 0, sizeof(*call));
    call->live = 1;
    call->ret_pc = site->vaddr + site->size;
    call->number = site->number;
    call->has_args = site->has_args;
    if (site->has_args)
    {
        /* SYSCALL saves the return address in rcx and passes argument 1 in r10. */
        if (strcmp(site->insn_name, "syscall") == 0)
        {
            call->arg1 = read_reg(g_r10);
        }
        else
        {
            call->arg1 = read_reg(g_rcx);
        }
        call->arg2 = edx;
        call->arg3 = r8;
        call->arg4 = read_reg(g_r9);
    }
    name = NULL;
    if (call->number < WIN_NAME_SLOTS)
    {
        name = g_syscall_by_num[call->number];
    }
    if (path_kind(name) == 1)
    {
        read_object_name(r8, call->path, sizeof(call->path));
    }
    else if (path_kind(name) == 2)
    {
        read_unicode(edx, call->path, sizeof(call->path));
    }
    remember_ret(call->ret_pc);
}

static void
write_event(const char *kind, const char *phase, const char *symbol, const char *insn,
            uint64_t pc, uint64_t eax, uint64_t edx, const char *result)
{
    if (g_log == NULL)
    {
        return;
    }

    fprintf(g_log, "{\"kind\":");
    json_escape(g_log, kind);
    fprintf(g_log, ",\"phase\":");
    json_escape(g_log, phase);
    fprintf(g_log, ",\"symbol\":");
    json_escape(g_log, symbol != NULL ? symbol : "");
    fprintf(g_log, ",\"insn\":");
    json_escape(g_log, insn != NULL ? insn : "");
    fprintf(g_log, ",\"pc\":\"0x%llx\",\"eax\":\"0x%llx\",\"edx\":\"0x%llx\",\"result\":",
            (unsigned long long)pc, (unsigned long long)eax, (unsigned long long)edx);
    json_escape(g_log, result);
    fprintf(g_log, "}\n");
    fflush(g_log);
}

static void
write_syscall(uint64_t pc, const char *insn, uint64_t rax, uint64_t r10, uint64_t rdx,
              uint64_t r8, uint64_t r9)
{
    if (g_log == NULL)
    {
        return;
    }

    fprintf(g_log, "{\"kind\":\"syscall\",\"phase\":\"enter\",\"symbol\":");
    json_escape(g_log, insn != NULL ? insn : "");
    fprintf(g_log, ",\"insn\":");
    json_escape(g_log, insn != NULL ? insn : "");
    fprintf(g_log,
            ",\"pc\":\"0x%llx\",\"rax\":\"0x%llx\",\"r10\":\"0x%llx\",\"rdx\":\"0x%llx\","
            "\"r8\":\"0x%llx\",\"r9\":\"0x%llx\",\"result\":\"seen\"}\n",
            (unsigned long long)pc, (unsigned long long)rax, (unsigned long long)r10,
            (unsigned long long)rdx, (unsigned long long)r8, (unsigned long long)r9);
    fflush(g_log);
}

static void
on_insn(unsigned int vcpu_index, void *userdata)
{
    Site *site;
    uint64_t eax;
    uint64_t edx;
    int i;

    (void)vcpu_index;
    site = (Site *)userdata;
    cache_regs();
    eax = read_reg(g_eax);
    edx = read_reg(g_edx);
    if (g_windows)
    {
        on_windows(vcpu_index, site, eax, edx, read_reg(g_r8));
        return;
    }

    if (!should_log(site->vaddr, site->kind))
    {
        return;
    }

    for (i = 0; i < g_pending_count; i++)
    {
        if (g_pending[i].ret_pc == site->vaddr)
        {
            write_event("export", "return", g_pending[i].symbol, "ret", site->vaddr, eax, edx,
                        (eax >= 0x80000000ULL) ? "error" : "success");
            g_pending[i] = g_pending[g_pending_count - 1];
            g_pending_count--;
            break;
        }
    }

    if (site->kind == SITE_SYSCALL)
    {
        if (strcmp(site->insn_name, "syscall") == 0)
        {
            write_syscall(site->vaddr, site->insn_name, eax, read_reg(g_r10), edx,
                          read_reg(g_r8), read_reg(g_r9));
        }
        else
        {
            write_event("syscall", "enter", site->insn_name, site->insn_name, site->vaddr, eax, edx,
                        "seen");
        }
    }
    else if (site->kind == SITE_EXPORT)
    {
        write_event("export", "enter", site->symbol, "call", site->vaddr, eax, edx, "seen");
        if (g_pending_count < 32)
        {
            g_pending[g_pending_count].ret_pc = site->vaddr + site->size;
            strncpy(g_pending[g_pending_count].symbol, site->symbol, 159);
            g_pending[g_pending_count].symbol[159] = '\0';
            g_pending_count++;
        }
    }
}

static Site *
new_site(int kind, const char *symbol, const char *insn, uint64_t vaddr, size_t size)
{
    Site *site;

    site = (Site *)calloc(1, sizeof(*site));
    if (site == NULL)
    {
        return NULL;
    }
    site->kind = kind;
    site->vaddr = vaddr;
    site->size = size;
    if (symbol != NULL)
    {
        strncpy(site->symbol, symbol, 159);
    }
    if (insn != NULL)
    {
        strncpy(site->insn_name, insn, 15);
    }
    return site;
}

static int
mov_r10_rcx(const uint8_t *bytes, size_t size)
{
    return size >= 3 && bytes[0] == 0x4C && bytes[1] == 0x8B && bytes[2] == 0xD1;
}

static int
stub_before(uint64_t vaddr, uint32_t *imm)
{
    uint8_t raw[20];

    if (vaddr < sizeof(raw) || !read_mem(vaddr - sizeof(raw), raw, sizeof(raw)))
    {
        return 0;
    }
    if (raw[2] == 0x4C && raw[3] == 0x8B && raw[4] == 0xD1 && raw[5] == 0xB8 &&
        raw[10] == 0xF6 && raw[18] == 0x75 && raw[19] == 0x03)
    {
        memcpy(imm, raw + 6, sizeof(*imm));
        return 1;
    }
    if (raw[12] == 0x4C && raw[13] == 0x8B && raw[14] == 0xD1 && raw[15] == 0xB8)
    {
        memcpy(imm, raw + 16, sizeof(*imm));
        return 1;
    }
    return 0;
}

static bool
mov_eax_imm(const uint8_t *bytes, size_t size, uint32_t *imm)
{
    if (size == 5 && bytes[0] == 0xB8)
    {
        memcpy(imm, bytes + 1, sizeof(*imm));
        return true;
    }
    if (size == 6 && bytes[0] == 0x66 && bytes[1] == 0xB8)
    {
        memcpy(imm, bytes + 2, sizeof(*imm));
        return true;
    }
    return false;
}

static void
on_tb(struct qemu_plugin_tb *tb)
{
    size_t count;
    size_t i;
    uint8_t prev[16];
    uint8_t prev2[16];
    size_t prev_size;
    size_t prev2_size;
    int prev_ok;
    int prev2_ok;

    prev_size = 0;
    prev2_size = 0;
    prev_ok = 0;
    prev2_ok = 0;
    count = q_tb_n_insns(tb);
    for (i = 0; i < count; i++)
    {
        struct qemu_plugin_insn *insn;
        uint8_t bytes[16];
        size_t full;
        size_t size;
        uint64_t vaddr;
        const char *sys;
        uint64_t target;
        const char *symbol;
        Site *site;
        int pending;

        insn = q_tb_get_insn(tb, i);
        full = q_insn_size(insn);
        vaddr = q_insn_vaddr(insn);
        size = full > sizeof(bytes) ? sizeof(bytes) : full;
        q_insn_data(insn, bytes, size);
        if (!g_located && vaddr >= 0xFFFFF80000000000ULL)
        {
            locate_images();
        }

        if (g_windows && is_remembered_ret(vaddr))
        {
            site = new_site(SITE_RETURN, "ret", "ret", vaddr, full);
            if (site != NULL)
            {
                q_register_exec(insn, on_insn, QEMU_PLUGIN_CB_R_REGS, site);
            }
        }

        for (pending = 0; pending < g_pending_count; pending++)
        {
            if (g_pending[pending].ret_pc == vaddr)
            {
                site = new_site(SITE_RETURN, g_pending[pending].symbol, "ret", vaddr, full);
                if (site != NULL)
                {
                    q_register_exec(insn, on_insn, QEMU_PLUGIN_CB_R_REGS, site);
                }
                break;
            }
        }

        sys = syscall_name(bytes, size);
        if (sys != NULL)
        {
            uint32_t imm;
            int stub;
            int from_mem;
            int legacy;

            imm = 0;
            from_mem = (bytes[0] == 0x0F) && stub_before(vaddr, &imm);
            stub = from_mem || (prev_ok && prev2_ok && mov_eax_imm(prev, prev_size, &imm) &&
                                mov_r10_rcx(prev2, prev2_size));
            legacy = !stub && bytes[0] == 0xCD && prev_ok && mov_eax_imm(prev, prev_size, &imm);
            if (!g_windows || stub || legacy || bytes[0] == 0x0F)
            {
                site = new_site(SITE_SYSCALL, sys, sys, vaddr, full);
                if (site != NULL)
                {
                    site->at_tb_start = (i == 0);
                    if (stub || legacy)
                    {
                        site->has_number = 1;
                        site->number = imm;
                        site->has_args = stub && (i == 0 || i == 2);
                    }
                    q_register_exec(insn, on_insn, QEMU_PLUGIN_CB_R_REGS, site);
                }
            }
            memcpy(prev2, prev, prev_size);
            prev2_size = prev_size;
            prev2_ok = prev_ok;
            memcpy(prev, bytes, size);
            prev_size = size;
            prev_ok = 1;
            continue;
        }
        memcpy(prev2, prev, prev_size);
        prev2_size = prev_size;
        prev2_ok = prev_ok;
        memcpy(prev, bytes, size);
        prev_size = size;
        prev_ok = 1;

        target = 0;
        symbol = NULL;
        if (decode_branch(bytes, size, vaddr, &target) ||
            decode_rip_indirect(bytes, size, vaddr, &target))
        {
            symbol = export_at(target);
        }
        if (symbol != NULL)
        {
            site = new_site(SITE_EXPORT, symbol, "call", vaddr, full);
            if (site != NULL)
            {
                q_register_exec(insn, on_insn, QEMU_PLUGIN_CB_R_REGS, site);
            }
        }
    }
}

__declspec(dllexport) int qemu_plugin_version = QEMU_PLUGIN_VERSION;

__declspec(dllexport) int
qemu_plugin_install(qemu_plugin_id_t id, const void *info, int argc, char **argv)
{
    const char *log_path;
    int i;

    (void)info;
    g_id = id;
    log_path = "abi.jsonl";

    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], "log=", 4) == 0)
        {
            log_path = argv[i] + 4;
        }
        else if (strncmp(argv[i], "map=", 4) == 0)
        {
            load_map(argv[i] + 4);
        }
        else if (strncmp(argv[i], "mode=", 5) == 0)
        {
            g_windows = strcmp(argv[i] + 5, "windows") == 0;
        }
        else if (strncmp(argv[i], "mark=", 5) == 0)
        {
            g_mark_path = argv[i] + 5;
        }
        else if (strncmp(argv[i], "syscalls=", 9) == 0)
        {
            load_syscalls(argv[i] + 9);
        }
        else if (strncmp(argv[i], "status=", 7) == 0)
        {
            load_status(argv[i] + 7);
        }
        else if (strncmp(argv[i], "cap=", 4) == 0)
        {
            g_log_cap = strtoull(argv[i] + 4, NULL, 10);
        }
    }

    if (!bind_qemu())
    {
        return 1;
    }

    g_log = fopen(log_path, "w");
    if (g_log == NULL)
    {
        return 1;
    }
    if (g_windows)
    {
        setvbuf(g_log, NULL, _IOFBF, 1024 * 1024);
    }

    q_register_tb(id, on_tb);
    return 0;
}
