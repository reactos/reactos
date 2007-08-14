#include <precomp.h>

#ifdef _DEBUG_MEM

#define REDZONE_SIZE  32
#define REDZONE_LEFT 0x78
#define REDZONE_RIGHT 0x87

typedef struct
{
    size_t size;
    const char *file;
    int line;
} alloc_info, *palloc_info;

static size_t allocations = 0;
static size_t allocated_memory = 0;

static void *
get_base_ptr(void *ptr)
{
    return (void *)((UINT_PTR)ptr - REDZONE_SIZE - sizeof(alloc_info));
}

static void *
write_redzone(void *ptr, size_t size, const char *file, int line)
{
    void *ret;
    palloc_info info = (palloc_info)ptr;

    info->size = size;
    info->file = file;
    info->line = line;

    ptr = (void *)(info + 1);
    memset(ptr, REDZONE_LEFT, REDZONE_SIZE);
    ret = (void *)((size_t)ptr + REDZONE_SIZE);
    ptr = (void *)((size_t)ret + size);
    memset(ptr, REDZONE_RIGHT, REDZONE_SIZE);
    return ret;
}

static int
check_redzone_region(void *ptr, unsigned char sig, void **newptr)
{
    unsigned char *p, *q;
    int ret = 1;

    p = (unsigned char *)ptr;
    q = p + REDZONE_SIZE;
    while (p != q)
    {
        if (*(p++) != sig)
            ret = 0;
    }

    if (newptr != NULL)
        *newptr = p;
    return ret;
}

static void
redzone_err(const char *msg, palloc_info info, void *ptr, const char *file, int line)
{
    DbgPrint("CMD: %s\n", msg);
    DbgPrint("     Block: 0x%p Size: %lu\n", ptr, info->size);
    DbgPrint("     Allocated from %s:%d\n", info->file, info->line);
    DbgPrint("     Detected at: %s:%d\n", file, line);
    ASSERT(FALSE);
    ExitProcess(1);
}

static void
check_redzone(void *ptr, const char *file, int line)
{
    palloc_info info = (palloc_info)ptr;
    ptr = (void *)(info + 1);
    if (!check_redzone_region(ptr, REDZONE_LEFT, &ptr))
        redzone_err("Detected buffer underflow!", info, ptr, file, line);
    ptr = (void *)((UINT_PTR)ptr + info->size);
    if (!check_redzone_region(ptr, REDZONE_RIGHT, NULL))
        redzone_err("Detected buffer overflow!", info, ptr, file, line);
}

static size_t
calculate_size_with_redzone(size_t size)
{
    return sizeof(alloc_info) + size + (2 * REDZONE_SIZE);
}

void *
cmd_alloc_dbg(size_t size, const char *file, int line)
{
    void *newptr = NULL;

    newptr = malloc(calculate_size_with_redzone(size));
    if (newptr != NULL)
    {
        allocations++;
        allocated_memory += size;
        newptr = write_redzone(newptr, size, file, line);
    }

    return newptr;
}

void *
cmd_realloc_dbg(void *ptr, size_t size, const char *file, int line)
{
    size_t prev_size;
    void *newptr = NULL;

    if (ptr == NULL)
        return cmd_alloc_dbg(size, file, line);
    if (size == 0)
    {
        cmd_free_dbg(ptr, file, line);
        return NULL;
    }

    ptr = get_base_ptr(ptr);
    prev_size = ((palloc_info)ptr)->size;
    check_redzone(ptr, file, line);

    newptr = realloc(ptr, calculate_size_with_redzone(size));
    if (newptr != NULL)
    {
        allocated_memory += size - prev_size;
        newptr = write_redzone(newptr, size, file, line);
    }

    return newptr;
}

void
cmd_free_dbg(void *ptr, const char *file, int line)
{
    if (ptr != NULL)
    {
        ptr = get_base_ptr(ptr);
        check_redzone(ptr, file, line);
        allocations--;
        allocated_memory -= ((palloc_info)ptr)->size;
    }

    free(ptr);
}

void
cmd_checkbuffer_dbg(void *ptr, const char *file, int line)
{
    if (ptr != NULL)
    {
        ptr = get_base_ptr(ptr);
        check_redzone(ptr, file, line);
    }
}

void
cmd_exit(int code)
{
    if (allocations != 0 || allocated_memory != 0)
    {
        DbgPrint("CMD: Leaking %lu bytes of memory in %lu blocks! Exit code: %d\n", allocated_memory, allocations, code);
    }

    ExitProcess(code);
}

#endif /* _DEBUG */
