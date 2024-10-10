
#define WIN32_NO_STATUS
#include <windef.h>
#include <ndk/rtlfuncs.h>
#include <wine/debug.h>

NTSTATUS NTAPI vDbgPrintExWithPrefix(PCCH, ULONG, ULONG, PCCH, va_list);

static struct
{
    HANDLE thread;
    void* allocations;
} s_alloactions[32];

static int find_thread_slot()
{
    HANDLE thread = NtCurrentTeb()->ClientId.UniqueThread;
    for (int i = 0; i < ARRAYSIZE(s_alloactions); i++)
    {
        if (s_alloactions[i].thread == thread)
        {
            return i;
        }
    }
    return -1;
}

static int get_thread_slot()
{
    int slot = find_thread_slot();
    if (slot != -1)
    {
        return slot;
    }

    HANDLE thread = NtCurrentTeb()->ClientId.UniqueThread;
    for (int i = 0; i < ARRAYSIZE(s_alloactions); i++)
    {
        if (s_alloactions[i].thread == NULL)
        {
            if (InterlockedCompareExchangePointer(&s_alloactions[i].thread, thread, NULL) == NULL)
            {
                return i;
            }
        }
    }

    return -1;
}

static char *alloc_buffer(size_t size)
{
    int slot = get_thread_slot();
    if (slot == -1)
    {
        return NULL;
    }

    void** buffer = (void**)RtlAllocateHeap(RtlGetProcessHeap(), 0, size + sizeof(void*));
    if (buffer == NULL)
    {
        return NULL;
    }

    *buffer = s_alloactions[slot].allocations;
    s_alloactions[slot].allocations = buffer;

    return (char*)(buffer + 1);
}

static void free_buffers(void)
{
    int slot = find_thread_slot();
    if (slot != -1)
    {
        return;
    }

    void* buffer = s_alloactions[slot].allocations;
    while (buffer != NULL)
    {
        void* next = *(void**)buffer;
        RtlFreeHeap(RtlGetProcessHeap(), 0, buffer);
        buffer = next;
    }

    s_alloactions[slot].allocations = NULL;
    s_alloactions[slot].thread = NULL;
}

const char *wine_dbg_vsprintf(const char *format, va_list valist)
{
    char* buffer;
    int len;

    len = vsnprintf(NULL, 0, format, valist);
    buffer = alloc_buffer(len + 1);
    if (buffer == NULL)
    {
        return "<allocation failed>";
    }
    len = vsnprintf(buffer, len, format, valist);
    buffer[len] = 0;
    return buffer;
}

/* printf with temp buffer allocation */
const char *wine_dbg_sprintf( const char *format, ... )
{
    const char *ret;
    va_list valist;

    va_start(valist, format);
    ret = wine_dbg_vsprintf( format, valist );
    va_end(valist);
    return ret;
}

const char *wine_dbgstr_wn( const WCHAR *str, int n )
{
    if (!((ULONG_PTR)str >> 16))
    {
        if (!str) return "(null)";
        return wine_dbg_sprintf("#%04x", LOWORD(str) );
    }
    if (n == -1)
    {
        n = (int)wcslen(str);
    }
    if (n < 0) n = 0;

    return wine_dbg_sprintf("%.*S", n, str);
}

/* From wine/dlls/ntdll/misc.c */
LPCSTR debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn(us->Buffer, us->Length / sizeof(WCHAR));
}

static int default_dbg_vprintf( const char *format, va_list args )
{
    return vDbgPrintExWithPrefix("", -1, 0, format, args);
}

int wine_dbg_printf(const char *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = default_dbg_vprintf(format, valist);
    va_end(valist);
    free_buffers();
    return ret;
}

static int winefmt_default_dbg_vlog( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                                     const char *file, const char *func, const int line, const char *format, va_list args )
{
    int ret = 0;

    ret += wine_dbg_printf("%04x:", HandleToULong(NtCurrentTeb()->ClientId.UniqueProcess) );
    ret += wine_dbg_printf("%04x:", HandleToULong(NtCurrentTeb()->ClientId.UniqueThread) );

    if (format)
        ret += default_dbg_vprintf(format, args);
    return ret;
}

#define __wine_dbg_get_channel_flags(channel) \
    ((channel) ? (channel)->flags : 0)

int ros_dbg_log( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                  const char *file, const char *func, const int line, const char *format, ... )
{
    int ret;
    va_list valist;

    if (!(__wine_dbg_get_channel_flags(channel) & (1 << cls))) return -1;

    va_start(valist, format);
    ret = winefmt_default_dbg_vlog(cls, channel, file, func, line, format, valist);
    va_end(valist);
    return ret;
}
