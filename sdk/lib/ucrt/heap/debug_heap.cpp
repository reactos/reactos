//
// debug_heap.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// The implementation of the CRT Debug Heap.
//
#ifndef _DEBUG
    #error This file is supported only in debug builds
    #define _DEBUG // For design-time support, when editing/viewing CRT sources
#endif

#include <corecrt_internal.h>
#include <malloc.h>
#include <minmax.h>
#include <new.h>
#include <stdio.h>
#include <stdlib.h>



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Constant Data
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define _ALLOCATION_FILE_LINENUM "\nMemory allocated at %hs(%d).\n"

#if _FREE_BLOCK != 0 || _NORMAL_BLOCK != 1 || _CRT_BLOCK != 2 || _IGNORE_BLOCK != 3 || _CLIENT_BLOCK != 4
    #error Block numbers have changed!
#endif

static char const* const block_use_names[_MAX_BLOCKS]
{
    "Free",
    "Normal",
    "CRT",
    "Ignore",
    "Client",
};

// The following values are non-zero, constant, odd, large, and atypical.
// * Non-zero values help find bugs that assume zero-filled data
// * Constant values are good so that memory filling is deterministic (to help
//   make bugs reproducible).  Of course, it is bad if the constant filling of
//   weird values masks a bug.
// * Mathematically odd numbers are good for finding bugs assuming a cleared
//   lower bit (e.g. properly aligned pointers to types other than char are not
//   odd).
// * Large byte values are less typical and are useful for finding bad addresses.
// * Atypical values (i.e., not too often) are good because they typically cause
//   early detection in code.
// * For the case of the no-man's land and free blocks, if you store to any of
//   these locations, the memory integrity checker will detect it.
//
// The align_land_fill was changed from 0xBD to 0xED to ensure that four bytes of
// that value (0xEDEDEDED) would form an inaccessible address outside of the lower
// 3GB of a 32-bit process address space.
static unsigned char const no_mans_land_fill{0xFD}; // Fill unaligned no-man's land
static unsigned char const align_land_fill  {0xED}; // Fill aligned no-man's land
static unsigned char const dead_land_fill   {0xDD}; // Fill free objects with this
static unsigned char const clean_land_fill  {0xCD}; // Fill new objects with this

// The size of the no-man's land used in unaligned and aligned allocations:
static size_t const no_mans_land_size = 4;
static size_t const align_gap_size    = sizeof(void *);

// For _IGNORE_BLOCK blocks, we use these request and line numbers as sentinels.
static long const request_number_for_ignore_blocks{0};
static int  const line_number_for_ignore_blocks   {static_cast<int>(0xFEDCBABC)};



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Types
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// For diagnostic purpose, blocks are allocated in the debug heap with extra
// information and stored in a doubly-linked list.  This makes all blocks
// registered with how big they are, when they were allocated, and what they are
// used for.
struct _CrtMemBlockHeader
{
    _CrtMemBlockHeader* _block_header_next;
    _CrtMemBlockHeader* _block_header_prev;
    char const*         _file_name;
    int                 _line_number;

    int                 _block_use;
    size_t              _data_size;

    long                _request_number;
    unsigned char       _gap[no_mans_land_size];

    // Followed by:
    // unsigned char    _data[_data_size];
    // unsigned char    _another_gap[no_mans_land_size];
};

static_assert(
    sizeof(_CrtMemBlockHeader) % MEMORY_ALLOCATION_ALIGNMENT == 0,
    "Incorrect debug heap block alignment");



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Global Mutable State (Synchronized by the AppCRT Heap Lock)
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These are pointers to the first and last nodes in the debug heap's doubly
// linked list of allocation nodes.
static _CrtMemBlockHeader* __acrt_first_block{nullptr};
static _CrtMemBlockHeader* __acrt_last_block {nullptr};

// These are the statistics for the current state of the debug heap, storing the
// total number of bytes allocated over the life of the process, the total number
// of bytes currently allocated (but not freed), and the maximum number of bytes
// that were ever allocated at once.
static size_t __acrt_total_allocations  {0};
static size_t __acrt_current_allocations{0};
static size_t __acrt_max_allocations    {0};

// These states control the frequency with which _CrtCheckMemory.  The units are
// "calls to allocation functions."
static unsigned __acrt_check_frequency{0};
static unsigned __acrt_check_counter  {0};

// This is the current request number, which is incremented each time a new
// allocation request is made.
static long __acrt_current_request_number{1};



// These three globals had external linkage in older versions of the CRT and may
// be referenced by name in client code.  The first stores the current debug
// heap options (flags).  The second stores the next allocation number on which
// to break.  The third stores the pointer to the dump client to be used when
// dumping heap objects.
#undef _crtDbgFlag
#undef _crtBreakAlloc

extern "C" {
    int              _crtDbgFlag{_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_DEFAULT_DF};
    long             _crtBreakAlloc{-1};
    _CRT_DUMP_CLIENT _pfnDumpClient{nullptr};
}

extern "C" int* __p__crtDbgFlag()
{
    return &_crtDbgFlag;
}

extern "C" long* __p__crtBreakAlloc()
{
    return &_crtBreakAlloc;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Internal Utilities
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
static unsigned char* __cdecl block_from_header(_CrtMemBlockHeader* const header) throw()
{
    return reinterpret_cast<unsigned char*>(header + 1);
}

static _CrtMemBlockHeader* __cdecl header_from_block(void const* const block) throw()
{
    return static_cast<_CrtMemBlockHeader*>(const_cast<void*>(block)) - 1;
}

static bool __cdecl is_block_type_valid(int const block_use) throw()
{
    return _BLOCK_TYPE(block_use) == _CLIENT_BLOCK
        || _BLOCK_TYPE(block_use) == _CRT_BLOCK
        || block_use              == _NORMAL_BLOCK
        || block_use              == _IGNORE_BLOCK;
}

// Tests the array of size bytes starting at first.  Returns true if all of the
// bytes in the array have the given value; returns false otherwise.
static bool __cdecl check_bytes(
    unsigned char const* const first,
    unsigned char        const value,
    size_t               const size
    ) throw()
{
    unsigned char const* const last{first + size};
    for (unsigned char const* it{first}; it != last; ++it)
    {
        if (*it != value)
            return false;
    }

    return true;
}

// Tests whether the size bytes of memory starting at address p can be read from.
// The functionality is similar to that of the Windows API IsBadReadPtr(), which
// is now deprecated.
static bool __cdecl is_bad_read_pointer(void const* const p, size_t const size) throw()
{
    SYSTEM_INFO system_info{};
    GetSystemInfo(&system_info);
    DWORD const page_size{system_info.dwPageSize};

    // If the structure has zero length, then do not probe the structure for
    // read accessibility or alignment.
    if (size == 0)
        return false;

    // A null pointer can never be read from:
    if (!p)
        return true;

    char const* start_address{static_cast<char const*>(p)};
    char const* end_address  {start_address + size - 1   };
    if (end_address < start_address)
        return true;

    __try
    {
        *(volatile char*)start_address;

        long const mask{~(static_cast<long>(page_size) - 1)};

        start_address = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(start_address) & mask);
        end_address   = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(end_address)   & mask);
        while (start_address != end_address)
        {
            start_address = start_address + page_size;
            *reinterpret_cast<const volatile char*>(start_address);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return true;
    }
    __endtry

    return false;
}

static bool __cdecl is_block_an_aligned_allocation(void const* const block) throw()
{
    unsigned char const* const possible_alignment_gap{reinterpret_cast<unsigned char const*>(
        reinterpret_cast<uintptr_t>(block) & (~sizeof(uintptr_t) - 1)) - align_gap_size};

    return check_bytes(possible_alignment_gap, align_land_fill, align_gap_size);
}

// The debug heap can be configured to validate the consistency of the heap at
// regular intervals.  If this behavior is configured, this function controls
// that validation.
static bool heap_validation_pending{false};

static void __cdecl validate_heap_if_required_nolock() throw()
{
    if (__acrt_check_frequency == 0)
    {
        return;
    }

    if (__acrt_check_counter != __acrt_check_frequency - 1)
    {
        ++__acrt_check_counter;
        return;
    }

    if (heap_validation_pending)
    {
        return;
    }

    heap_validation_pending = true;
    __try
    {
        _ASSERTE(_CrtCheckMemory());
    }
    __finally
    {
        heap_validation_pending = false;
    }
    __endtry

    __acrt_check_counter = 0;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Internal Debug Heap APIs
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// Attempts to allocate a block of size bytes from the debug heap.  Returns null
// on failure.
static void* __cdecl heap_alloc_dbg_internal(
    size_t      const size,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    ) throw()
{
    void* block{nullptr};

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        validate_heap_if_required_nolock();

        long const request_number{__acrt_current_request_number};

        // Handle break-on-request and forced failure:
        if (_crtBreakAlloc != -1 && request_number == _crtBreakAlloc)
        {
            _CrtDbgBreak();
        }

        if (_pfnAllocHook && !_pfnAllocHook(
            _HOOK_ALLOC,
            nullptr,
            size,
            block_use,
            request_number,
            reinterpret_cast<unsigned char const*>(file_name),
            line_number))
        {
            if (file_name)
                _RPTN(_CRT_WARN, "Client hook allocation failure at file %hs line %d.\n", file_name, line_number);
            else
                _RPT0(_CRT_WARN, "Client hook allocation failure.\n");

            __leave;
        }

#pragma warning(suppress:__WARNING_UNUSED_ASSIGNMENT) // 28931
        bool const ignore_block{_BLOCK_TYPE(block_use) != _CRT_BLOCK && !(_crtDbgFlag & _CRTDBG_ALLOC_MEM_DF)};

        // Diagnostic memory allocation from this point on...
        if (size > static_cast<size_t>(_HEAP_MAXREQ - no_mans_land_size - sizeof(_CrtMemBlockHeader)))
        {
            errno_t* const global_errno{_errno()};
            if (global_errno)
                *global_errno = ENOMEM;

            __leave;
        }

        if (!is_block_type_valid(block_use))
        {
            _RPT0(_CRT_ERROR, "Error: memory allocation: bad memory block type.\n");
        }

        size_t const block_size{sizeof(_CrtMemBlockHeader) + size + no_mans_land_size};

        _CrtMemBlockHeader* const header{static_cast<_CrtMemBlockHeader*>(HeapAlloc(__acrt_heap, 0, block_size))};
        if (!header)
        {
            errno_t* const global_errno{_errno()};
            if (global_errno)
                *global_errno = ENOMEM;

            __leave;
        }

        // Commit the allocation by linking the block into the global list:
        ++__acrt_current_request_number;

        if (ignore_block)
        {
            header->_block_header_next = nullptr;
            header->_block_header_prev = nullptr;
            header->_file_name         = nullptr;
            header->_line_number       = line_number_for_ignore_blocks;
            header->_data_size         = size;
            header->_block_use         = _IGNORE_BLOCK;
            header->_request_number    = request_number_for_ignore_blocks;
        }
        else
        {
            // Keep track of total amount of memory allocated:
            if (SIZE_MAX - __acrt_total_allocations > size)
            {
                __acrt_total_allocations += size;
            }
            else
            {
                __acrt_total_allocations = SIZE_MAX;
            }

            __acrt_current_allocations += size;

            if (__acrt_current_allocations > __acrt_max_allocations)
                __acrt_max_allocations = __acrt_current_allocations;

            if (__acrt_first_block)
            {
                __acrt_first_block->_block_header_prev = header;
            }
            else
            {
                __acrt_last_block = header;
            }

            header->_block_header_next = __acrt_first_block;
            header->_block_header_prev = nullptr;
            header->_file_name         = file_name;
            header->_line_number       = line_number;
            header->_data_size         = size;
            header->_block_use         = block_use;
            header->_request_number    = request_number;

            __acrt_first_block = header;
        }

        // Fill the gap before and after the data block:
        memset(header->_gap,                     no_mans_land_fill, no_mans_land_size);
        memset(block_from_header(header) + size, no_mans_land_fill, no_mans_land_size);

        // Fill the data block with a silly (but non-zero) value:
        memset(block_from_header(header), clean_land_fill, size);

        block = block_from_header(header);
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry

    return block;
}



// Allocates a block of size bytes from the debug heap, using the new handler if
// it is configured for use with malloc.
static void* __cdecl heap_alloc_dbg(
    size_t      const size,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    ) throw()
{
    bool const should_call_new_handler{_query_new_mode() != 0};
    for (;;)
    {
        void* const block{heap_alloc_dbg_internal(size, block_use, file_name, line_number)};
        if (block)
            return block;

        if (!should_call_new_handler || !_callnewh(size))
        {
            errno_t* const global_errno{_errno()};
            if (global_errno)
                *global_errno = ENOMEM;

            return nullptr;
        }

        // The new handler was successful -- try to allocate again
    }
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Public Debug Heap Allocation APIs
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// These are the allocation functions that allocate, manipulate, and free blocks
// from the debug heap.  They are equivalent to the main allocation functions,
// which deal with blocks from the process heap.  Most of the debug allocation
// functions accept a block use, file name, and/or line number which are used to
// track where allocations originated.
//
// Documentation comments for these functions describe only the material
// differences between them and the corresponding main allocation functions.

// This function must be marked noinline, otherwise malloc and
// _malloc_dbg will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because malloc
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) void* __cdecl _malloc_dbg(
    size_t      const size,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    return heap_alloc_dbg(size, block_use, file_name, line_number);
}


// This function must be marked noinline, otherwise calloc and
// _calloc_dbg will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because calloc
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) void* __cdecl _calloc_dbg(
    size_t      const count,
    size_t      const element_size,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    _VALIDATE_RETURN_NOEXC(count == 0 || (_HEAP_MAXREQ / count) >= element_size, ENOMEM, nullptr);

    size_t const allocation_size{element_size * count};

    // Note that we zero exactly allocation_size bytes.  The _calloc_base
    // function for the main heap may zero more bytes if a larger block is
    // allocated.
    void* const block{heap_alloc_dbg(allocation_size, block_use, file_name, line_number)};
    if (block)
        memset(block, 0, allocation_size);

    return block;
}



// Common debug realloc implementation shared by _realloc_dbg, _recalloc_dbg,
// and _expand_dbg.  If reallocation_is_allowed is true, the expand behavior
// is used; otherwise the realloc behavior is used.
static void * __cdecl realloc_dbg_nolock(
    void*       const block,
    size_t*     const new_size,
    int         const block_use,
    char const* const file_name,
    int         const line_number,
    bool        const reallocation_is_allowed
    ) throw()
{
    // realloc(nullptr, size) is equivalent to malloc(size):
    if (!block)
    {
        return _malloc_dbg(*new_size, block_use, file_name, line_number);
    }

    // realloc(block, 0) is equivalent to free(block):
    if (reallocation_is_allowed && *new_size == 0)
    {
        _free_dbg(block, block_use);
        return nullptr;
    }

    validate_heap_if_required_nolock();

    // Handle break-on-request and forced failure:
    long const request_number{__acrt_current_request_number};
    if (_crtBreakAlloc != -1 && request_number == _crtBreakAlloc)
    {
        _CrtDbgBreak();
    }

    if (_pfnAllocHook && !_pfnAllocHook(
        _HOOK_REALLOC,
        block,
        *new_size,
        block_use,
        request_number,
        reinterpret_cast<unsigned char const*>(file_name),
        line_number))
    {
        if (file_name)
            _RPTN(_CRT_WARN, "Client hook re-allocation failure at file %hs line %d.\n", file_name, line_number);
        else
            _RPT0(_CRT_WARN, "Client hook re-allocation failure.\n");

        return nullptr;
    }

    // Ensure the block type matches what is expected and isn't an aligned allocation:
    if (block_use != _NORMAL_BLOCK && _BLOCK_TYPE(block_use) != _CLIENT_BLOCK && _BLOCK_TYPE(block_use) != _CRT_BLOCK)
    {
        if (file_name)
        {
            _RPTN(_CRT_ERROR,
                "Error: memory allocation: bad memory block type.\n" _ALLOCATION_FILE_LINENUM,
                file_name, line_number);
        }
        else
        {
            _RPT0(_CRT_ERROR, "Error: memory allocation: bad memory block type.\n");
        }
    }
    else if (is_block_an_aligned_allocation(block))
    {
        // We don't know (yet) where (file, linenum) block was allocated
        _RPTN(_CRT_ERROR, "The Block at 0x%p was allocated by aligned routines, use _aligned_realloc()", block);
        errno = EINVAL;

        return nullptr;
    }

    // If this assertion fails, a bad pointer has been passed in.  It may be
    // totally bogus, or it may have been allocated from another heap.  The
    // pointer must have been allocated from the debug heap.
    _ASSERTE(_CrtIsValidHeapPointer(block));

    _CrtMemBlockHeader* const old_head{header_from_block(block)};

    bool const is_ignore_block{old_head->_block_use == _IGNORE_BLOCK};
    if (is_ignore_block)
    {
        _ASSERTE(old_head->_line_number == line_number_for_ignore_blocks && old_head->_request_number == request_number_for_ignore_blocks);
    }
    else if (__acrt_total_allocations < old_head->_data_size)
    {
        _RPTN(_CRT_ERROR, "Error: possible heap corruption at or near 0x%p", block);
        errno = EINVAL;
        return nullptr;
    }

    // Ensure the new requested size is not too large:
    if (*new_size > static_cast<size_t>(_HEAP_MAXREQ - no_mans_land_size - sizeof(_CrtMemBlockHeader)))
    {
        errno = ENOMEM;
        return nullptr;
    }

    // Note that all header values will remain valid and the minimum of the old
    // size and the new size of data will remain valid.
    size_t const new_internal_size{sizeof(_CrtMemBlockHeader) + *new_size + no_mans_land_size};

    _CrtMemBlockHeader* new_head{nullptr};
    if (reallocation_is_allowed)
    {
        new_head = static_cast<_CrtMemBlockHeader*>(_realloc_base(old_head, new_internal_size));
        if (!new_head)
            return nullptr;
    }
    else
    {
        new_head = static_cast<_CrtMemBlockHeader*>(_expand_base(old_head, new_internal_size));
        if (!new_head)
            return nullptr;

        // On Win64, the heap does not try to resize the block if it is shrinking
        // because of the use of the low-fragmentation heap.  It just returns the
        // original block.  We make sure that our own header tracks that properly:
        #ifdef _WIN64
        *new_size = static_cast<size_t>(HeapSize(__acrt_heap, 0, new_head)
            - sizeof(_CrtMemBlockHeader)
            - no_mans_land_size);
        #endif
    }

    _Analysis_assume_(new_head->_data_size == old_head->_data_size);

    // Account for the current allocation and track the total amount of memory
    // that is currently allocated:
    ++__acrt_current_request_number;
    if (!is_ignore_block)
    {
        if (__acrt_total_allocations < SIZE_MAX)
        {
            __acrt_total_allocations -= new_head->_data_size;
            __acrt_total_allocations += SIZE_MAX - __acrt_total_allocations > *new_size
                ? *new_size
                : SIZE_MAX;
        }

        __acrt_current_allocations -= new_head->_data_size;
        __acrt_current_allocations += *new_size;

        if (__acrt_current_allocations > __acrt_max_allocations)
            __acrt_max_allocations = __acrt_current_allocations;
    }

    unsigned char* const new_block{block_from_header(new_head)};

    // If the block grew, fill the "extension" with the land fill value:
    if (*new_size > new_head->_data_size)
    {
        memset(new_block + new_head->_data_size, clean_land_fill, *new_size - new_head->_data_size);
    }

    // Fill in the gap after the client block:
    memset(new_block + *new_size, no_mans_land_fill, no_mans_land_size);

    if (!is_ignore_block)
    {
        new_head->_file_name      = file_name;
        new_head->_line_number    = line_number;
        new_head->_request_number = request_number;
    }

    new_head->_data_size = *new_size;

    _ASSERTE(reallocation_is_allowed || (!reallocation_is_allowed && new_head == old_head));

    // If the block did not move or is ignored, we are done:
    if (new_head == old_head || is_ignore_block)
        return new_block;

    // Swap out the old block from the linked list and link in the new block:
    if (new_head->_block_header_next)
    {
        new_head->_block_header_next->_block_header_prev = new_head->_block_header_prev;
    }
    else
    {
        _ASSERTE(__acrt_last_block == old_head);
        __acrt_last_block = new_head->_block_header_prev;
    }

    if (new_head->_block_header_prev)
    {
        new_head->_block_header_prev->_block_header_next = new_head->_block_header_next;
    }
    else
    {
        _ASSERTE(__acrt_first_block == old_head);
        __acrt_first_block = new_head->_block_header_next;
    }

    if (__acrt_first_block)
    {
        __acrt_first_block->_block_header_prev = new_head;
    }
    else
    {
        __acrt_last_block = new_head;
    }

    new_head->_block_header_next = __acrt_first_block;
    new_head->_block_header_prev = nullptr;
    __acrt_first_block = new_head;

    return new_block;
}


// This function must be marked noinline, otherwise realloc and
// _realloc_dbg will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because realloc
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) void* __cdecl _realloc_dbg(
    void*       const block,
    size_t      const requested_size,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    void* new_block{nullptr};

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        size_t new_size{requested_size};
        new_block = realloc_dbg_nolock(block, &new_size, block_use, file_name, line_number, true);
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry

    return new_block;
}


// This function must be marked noinline, otherwise recalloc and
// _recalloc_dbg will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because recalloc
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) void* __cdecl _recalloc_dbg(
    void*       const block,
    size_t      const count,
    size_t      const element_size,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    _VALIDATE_RETURN_NOEXC(count == 0 || (_HEAP_MAXREQ / count) >= element_size, ENOMEM, nullptr);

    size_t const old_allocation_size{block ? _msize_dbg(block, block_use) : 0};
    size_t const new_allocation_size{element_size * count     };

    void* const new_block{_realloc_dbg(block, new_allocation_size, block_use, file_name, line_number)};
    if (!new_block)
        return nullptr;

    // Zero the "expansion," if the block was expanded:
    if (old_allocation_size < new_allocation_size)
    {
        memset(
            static_cast<unsigned char*>(new_block) + old_allocation_size,
            0,
            new_allocation_size - old_allocation_size);
    }

    return new_block;
}


// This function must be marked noinline, otherwise _expand and
// _expand_dbg will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because _expand
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) void* __cdecl _expand_dbg(
    void*       const block,
    size_t      const requested_size,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    _VALIDATE_RETURN(block != nullptr, EINVAL, nullptr);

    if (requested_size > static_cast<size_t>(_HEAP_MAXREQ - no_mans_land_size - sizeof(_CrtMemBlockHeader)))
    {
        errno = ENOMEM;
        return nullptr;
    }

    void* new_block{nullptr};

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        size_t new_size{requested_size};
        new_block = realloc_dbg_nolock(block, &new_size, block_use, file_name, line_number, false);
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry

    return new_block;
}



static void __cdecl free_dbg_nolock(
    void* const block,
    int   const block_use
    ) throw()
{
    validate_heap_if_required_nolock();

    if (block == nullptr)
        return;

    #if _UCRT_HEAP_MISMATCH_DETECTION && (defined _M_IX86 || defined _M_AMD64)

    if (!_CrtIsValidHeapPointer(block))
    {
        HANDLE const msvcrt_heap_handle = __acrt_get_msvcrt_heap_handle();
        if (msvcrt_heap_handle)
        {
            if (HeapValidate(msvcrt_heap_handle, 0, block))
            {
                _RPT1(_CRT_WARN, "CRTHEAP: ucrt: Attempt to free a pointer (0x%p) that belongs to MSVCRT's private heap, not the process heap.\n", block);

                #if _UCRT_HEAP_MISMATCH_BREAK
                _CrtDbgBreak();
                #endif // _UCRT_HEAP_MISMATCH_BREAK

                #if _UCRT_HEAP_MISMATCH_RECOVERY
                if (HeapFree(msvcrt_heap_handle, 0, block))
                {
                    _RPT1(_CRT_WARN, "CRTHEAP: ucrt: Successfully free'd 0x%p\n", block);
                    return;
                }
                else
                {
                    _RPT1(_CRT_ERROR, "CRTHEAP: ucrt: Unable to free 0x%p\n", block);
                    _CrtDbgBreak(); // Force break.
                }
                #endif // _UCRT_HEAP_MISMATCH_RECOVERY
            }
        }
    }

    #endif // _UCRT_HEAP_MISMATCH_DETECTION && (defined _M_IX86 || defined _M_AMD64)

    // Check to ensure that the block was not allocated by _aligned routines
    if (block_use == _NORMAL_BLOCK && is_block_an_aligned_allocation(block))
    {
        // We don't know (yet) where (file, linenum) block was allocated
        _RPTN(_CRT_ERROR, "The Block at 0x%p was allocated by aligned routines, use _aligned_free()", block);
        errno = EINVAL;
        return;
    }

    // Forced failure handling
    if (_pfnAllocHook && !_pfnAllocHook(_HOOK_FREE, block, 0, block_use, 0, nullptr, 0))
    {
        _RPT0(_CRT_WARN, "Client hook free failure.\n");
        return;
    }

    // If this assertion fails, a bad pointer has been passed in.  It may be
    // totally bogus, or it may have been allocated from another heap.  The
    // pointer must have been allocated from the CRT heap.
    _ASSERTE(_CrtIsValidHeapPointer(block));

    // Get a pointer to memory block header:
    _CrtMemBlockHeader* const header = header_from_block(block);
    _ASSERTE(is_block_type_valid(header->_block_use));

    // If we didn't already check entire heap, at least check this object by
    // verifying that its no-man's land areas have not been trashed:
    if (!(_crtDbgFlag & _CRTDBG_CHECK_ALWAYS_DF))
    {
        if (!check_bytes(header->_gap, no_mans_land_fill, no_mans_land_size))
        {
            if (header->_file_name)
            {
                _RPTN(_CRT_ERROR, "HEAP CORRUPTION DETECTED: before %hs block (#%d) at 0x%p.\n"
                    "CRT detected that the application wrote to memory before start of heap buffer.\n"
                    _ALLOCATION_FILE_LINENUM,
                    block_use_names[_BLOCK_TYPE(header->_block_use)],
                    header->_request_number,
                    block_from_header(header),
                    header->_file_name,
                    header->_line_number);
            }
            else
            {
                _RPTN(_CRT_ERROR, "HEAP CORRUPTION DETECTED: before %hs block (#%d) at 0x%p.\n"
                    "CRT detected that the application wrote to memory before start of heap buffer.\n",
                    block_use_names[_BLOCK_TYPE(header->_block_use)],
                    header->_request_number,
                    block_from_header(header));
            }
        }

        if (!check_bytes(block_from_header(header) + header->_data_size, no_mans_land_fill, no_mans_land_size))
        {
            if (header->_file_name)
            {
                _RPTN(_CRT_ERROR, "HEAP CORRUPTION DETECTED: after %hs block (#%d) at 0x%p.\n"
                    "CRT detected that the application wrote to memory after end of heap buffer.\n"
                    _ALLOCATION_FILE_LINENUM,
                    block_use_names[_BLOCK_TYPE(header->_block_use)],
                    header->_request_number,
                    block_from_header(header),
                    header->_file_name,
                    header->_line_number);
            }
            else
            {
                _RPTN(_CRT_ERROR, "HEAP CORRUPTION DETECTED: after %hs block (#%d) at 0x%p.\n"
                    "CRT detected that the application wrote to memory after end of heap buffer.\n",
                    block_use_names[_BLOCK_TYPE(header->_block_use)],
                    header->_request_number,
                    block_from_header(header));
            }
        }
    }

    // If this block was ignored when it was allocated, we can just free it:
    if (header->_block_use == _IGNORE_BLOCK)
    {
        _ASSERTE(header->_line_number == line_number_for_ignore_blocks && header->_request_number == request_number_for_ignore_blocks);
        memset(header, dead_land_fill, sizeof(_CrtMemBlockHeader) + header->_data_size + no_mans_land_size);
        _free_base(header);
        return;
    }

    // Ensure that we were called with the right block use.  CRT blocks can be
    // freed as NORMAL blocks.
    _ASSERTE(header->_block_use == block_use || header->_block_use == _CRT_BLOCK && block_use == _NORMAL_BLOCK);

    __acrt_current_allocations -= header->_data_size;

    // Optionally reclaim memory:
    if ((_crtDbgFlag & _CRTDBG_DELAY_FREE_MEM_DF) == 0)
    {
        // Unlink this allocation from the global linked list:
        if (header->_block_header_next)
        {
            header->_block_header_next->_block_header_prev = header->_block_header_prev;
        }
        else
        {
            _ASSERTE(__acrt_last_block == header);
            __acrt_last_block = header->_block_header_prev;
        }

        if (header->_block_header_prev)
        {
            header->_block_header_prev->_block_header_next = header->_block_header_next;
        }
        else
        {
            _ASSERTE(__acrt_first_block == header);
            __acrt_first_block = header->_block_header_next;
        }

        memset(header, dead_land_fill, sizeof(_CrtMemBlockHeader) + header->_data_size + no_mans_land_size);
        _free_base(header);
    }
    else
    {
        header->_block_use = _FREE_BLOCK;

        // Keep memory around as dead space:
        memset(block_from_header(header), dead_land_fill, header->_data_size);
    }
}


// This function must be marked noinline, otherwise free and
// _free_dbg will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because free
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) void __cdecl _free_dbg(void* const block, int const block_use)
{
    __acrt_lock(__acrt_heap_lock);
    __try
    {
        // If a block use was provided, use it; if the block use was not known,
        // use the block use stored in the header.  (For example, the block use
        // is not known when this function is called by operator delete because
        // the heap lock must be acquired to access the block header.)
        int const actual_use{block_use == _UNKNOWN_BLOCK && block != nullptr
            ? header_from_block(block)->_block_use
            : block_use};

        free_dbg_nolock(block, actual_use);
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry
}


// This function must be marked noinline, otherwise _msize and
// _msize_dbg will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because _msize
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) size_t __cdecl _msize_dbg(void* const block, int const block_use)
{
    UNREFERENCED_PARAMETER(block_use);

    _VALIDATE_RETURN(block != nullptr, EINVAL, static_cast<size_t>(-1));

    size_t size{0};

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        validate_heap_if_required_nolock();

        // If this assert fails, a bad pointer has been passed in.  It may be
        // totally bogus, or it may have been allocated from another heap.  The
        // pointer must have been allocated from the CRT heap.
        _ASSERTE(_CrtIsValidHeapPointer(block));

        _CrtMemBlockHeader* const header{header_from_block(block)};

        _ASSERTE(is_block_type_valid(header->_block_use));

        size = header->_data_size;
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry

    return size;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Public Debug Heap Control and Status APIs
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// Configures the CRT to break on allocation operation number new_break_alloc.
// Returns the previous break allocation value.
extern "C" long __cdecl _CrtSetBreakAlloc(long const new_break_alloc)
{
    long const old_break_alloc{_crtBreakAlloc};
    _crtBreakAlloc = new_break_alloc;
    return old_break_alloc;
}



// Changes the block use for a block allocated on the debug heap.
extern "C" void __cdecl _CrtSetDbgBlockType(
    void* const block,
    int   const block_use
    )
{
    __acrt_lock(__acrt_heap_lock);
    __try
    {
        if (!_CrtIsValidHeapPointer(block))
            __leave;

        _CrtMemBlockHeader* const header{header_from_block(block)};

        _ASSERTE(is_block_type_valid(header->_block_use));

        header->_block_use = block_use;
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry
}



// These get and set the allocation hook function, which is called for debug
// heap allocation operations.
extern "C" _CRT_ALLOC_HOOK __cdecl _CrtGetAllocHook()
{
    return _pfnAllocHook;
}

extern "C" _CRT_ALLOC_HOOK __cdecl _CrtSetAllocHook(_CRT_ALLOC_HOOK const new_hook)
{
    _CRT_ALLOC_HOOK const old_hook{_pfnAllocHook};
    _pfnAllocHook = new_hook;
    return old_hook;
}



// Checks the integrity of the debug heap.  Returns TRUE if the debug heap (and
// the underlying Windows heap) appears valid; returns FALSE and asserts if the
// heap appears corrupted or otherwise invalid.
static bool __cdecl check_block(_CrtMemBlockHeader* const header) throw()
{
    bool this_block_okay{true};
    char const* block_use{nullptr};

    if (is_block_type_valid(header->_block_use))
    {
        block_use = block_use_names[_BLOCK_TYPE(header->_block_use)];
    }
    else
    {
        block_use = "DAMAGED";
    }

    // Check the no-man's-land gaps:
    if (!check_bytes(header->_gap, no_mans_land_fill, no_mans_land_size))
    {
        if (header->_file_name)
        {
            _RPTN(_CRT_WARN, "HEAP CORRUPTION DETECTED: before %hs block (#%d) at 0x%p.\n"
                "CRT detected that the application wrote to memory before start of heap buffer.\n"
                _ALLOCATION_FILE_LINENUM,
                block_use,
                header->_request_number,
                block_from_header(header),
                header->_file_name,
                header->_line_number);
        }
        else
        {
            _RPTN(_CRT_WARN, "HEAP CORRUPTION DETECTED: before %hs block (#%d) at 0x%p.\n"
                "CRT detected that the application wrote to memory before start of heap buffer.\n",
                block_use, header->_request_number, block_from_header(header));
        }

        this_block_okay = false;
    }

    if (!check_bytes(block_from_header(header) + header->_data_size, no_mans_land_fill, no_mans_land_size))
    {
        if (header->_file_name)
        {
            _RPTN(_CRT_WARN, "HEAP CORRUPTION DETECTED: after %hs block (#%d) at 0x%p.\n"
                "CRT detected that the application wrote to memory after end of heap buffer.\n"
                _ALLOCATION_FILE_LINENUM,
                block_use,
                header->_request_number,
                block_from_header(header),
                header->_file_name,
                header->_line_number);
        }
        else
        {
            _RPTN(_CRT_WARN, "HEAP CORRUPTION DETECTED: after %hs block (#%d) at 0x%p.\n"
                "CRT detected that the application wrote to memory after end of heap buffer.\n",
                block_use, header->_request_number, block_from_header(header));
        }

        this_block_okay = false;
    }

    // Free blocks should remain undisturbed:
    if (header->_block_use == _FREE_BLOCK && !check_bytes(block_from_header(header), dead_land_fill, header->_data_size))
    {
        if (header->_file_name)
        {
            _RPTN(_CRT_WARN, "HEAP CORRUPTION DETECTED: on top of Free block at 0x%p.\n"
                "CRT detected that the application wrote to a heap buffer that was freed.\n"
                _ALLOCATION_FILE_LINENUM,
                block_from_header(header),
                header->_file_name,
                header->_line_number);
        }
        else
        {
            _RPTN(_CRT_WARN, "HEAP CORRUPTION DETECTED: on top of Free block at 0x%p.\n"
                "CRT detected that the application wrote to a heap buffer that was freed.\n",
                block_from_header(header));
        }

        this_block_okay = false;
    }

    if (!this_block_okay)
    {
        // Report statistics about the broken object:
        if (header->_file_name)
        {
            _RPTN(_CRT_WARN,
                "%hs located at 0x%p is %Iu bytes long.\n"
                _ALLOCATION_FILE_LINENUM,
                block_use,
                block_from_header(header),
                header->_data_size,
                header->_file_name,
                header->_line_number);
        }
        else
        {
            _RPTN(_CRT_WARN, "%hs located at 0x%p is %Iu bytes long.\n",
                block_use, block_from_header(header), header->_data_size);
        }
    }

    return this_block_okay;
}

extern "C" int __cdecl _CrtCheckMemory()
{
    if ((_crtDbgFlag & _CRTDBG_ALLOC_MEM_DF) == 0)
    {
        return TRUE;
    }

    bool all_okay{true};

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        // First walk our allocated heap blocks and verify consistency of our
        // internal data structures.  We do this first because we can give
        // better diagnostics to client code than the underlying Windows heap
        // can (we have source file names and line numbers and other metadata).
        // We use Floyd's cycle finding algorithm to detect cycles in the block
        // list.
        _CrtMemBlockHeader* trail_it{__acrt_first_block};
        _CrtMemBlockHeader* lead_it {__acrt_first_block == nullptr ? nullptr : __acrt_first_block->_block_header_next};
        while (trail_it != nullptr)
        {
            all_okay &= check_block(trail_it);

            if (trail_it == lead_it)
            {
                _RPTN(_CRT_WARN,
                    "Cycle in block list detected while processing block located at 0x%p.\n",
                    trail_it);
                all_okay = false;
                break;
            }

            trail_it = trail_it->_block_header_next;

            // Advance the lead iterator twice as fast as the trail iterator:
            if (lead_it != nullptr)
            {
                lead_it = lead_it->_block_header_next == nullptr
                    ? nullptr
                    : lead_it->_block_header_next->_block_header_next;
            }
        }

        // Then check the underlying Windows heap:
        if (!HeapValidate(__acrt_heap, 0, nullptr))
        {
            _RPT0(_CRT_WARN, "Heap validation failed.\n");
            all_okay = false;
            __leave;
        }
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry

    return all_okay ? TRUE : FALSE;
}



// Configures the debug heap behavior flags.  Note that only the flags listed at
// the top of the function definition are valid; any other flags will cause the
// invalid parameter handler to be invoked.  Returns the previous flag state.
extern "C" int __cdecl _CrtSetDbgFlag(int const new_bits)
{
    int const valid_flags{
        _CRTDBG_ALLOC_MEM_DF      |
        _CRTDBG_DELAY_FREE_MEM_DF |
        _CRTDBG_CHECK_ALWAYS_DF   |
        _CRTDBG_CHECK_CRT_DF      |
        _CRTDBG_LEAK_CHECK_DF};

    bool const new_bits_have_only_valid_flags = (new_bits & 0xffff & ~valid_flags) == 0;
    _VALIDATE_RETURN(new_bits == _CRTDBG_REPORT_FLAG || new_bits_have_only_valid_flags, EINVAL, _crtDbgFlag);

    int old_bits{0};

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        old_bits = _crtDbgFlag;

        if (new_bits == _CRTDBG_REPORT_FLAG)
            __leave;

        if (new_bits & _CRTDBG_CHECK_ALWAYS_DF)
            __acrt_check_frequency = 1;
        else
            __acrt_check_frequency = (new_bits >> 16) & 0x0ffff;

        __acrt_check_counter = 0;
        _crtDbgFlag = new_bits;
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry

    return old_bits;
}



// Calls a caller-provided function for each client object in the heap.
extern "C" void __cdecl _CrtDoForAllClientObjects(
    _CrtDoForAllClientObjectsCallback const callback,
    void*                             const context
    )
{
    _VALIDATE_RETURN_VOID(callback != nullptr, EINVAL);

    if ((_crtDbgFlag & _CRTDBG_ALLOC_MEM_DF) == 0)
        return;

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        for (_CrtMemBlockHeader* header{__acrt_first_block}; header != nullptr; header = header->_block_header_next)
        {
            if (_BLOCK_TYPE(header->_block_use) == _CLIENT_BLOCK)
                callback(block_from_header(header), context);
        }
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry
}



// This function is provided for backwards compatibility only.  It just tests
// whether the provided pointer is null.
extern "C" int __cdecl _CrtIsValidPointer(
    void const*  const p,
    unsigned int const size_in_bytes,
    int          const read_write
    )
{
    UNREFERENCED_PARAMETER(size_in_bytes);
    UNREFERENCED_PARAMETER(read_write);

    return p != nullptr;
}



// This function is provided for backwards compatibility only.  It returns TRUE
// if the given block points to an allocation from the OS heap that underlies
// this CRT debug heap.  Back when the CRT used its own OS heap (prior to Dev10),
// this function would thus also tell you whether the block was allocated by this
// debug heap.  Now, it just tells you whether the block was allocated by some
// debug heap.
extern "C" int __cdecl _CrtIsValidHeapPointer(void const* const block)
{
    if (!block)
        return FALSE;

    return HeapValidate(__acrt_heap, 0, header_from_block(block));
}



// Verifies that the provided memory block is a block allocated by this debug
// heap.  Returns TRUE if it is; FALSE otherwise.  If the request_number,
// file_name, and line_number arguments are non-null, and if the block was
// allocated by this debug heap, this function fills in those argments with the
// actual values for the block.
extern "C" int __cdecl _CrtIsMemoryBlock(
    void const* const block,
    unsigned    const size,
    long*       const request_number,
    char**      const file_name,
    int*        const line_number
    )
{
    if (request_number)
        *request_number = 0;

    if (file_name)
        *file_name = nullptr;

    if (line_number)
        *line_number = 0;

    if (!block)
        return FALSE;

    int result{FALSE};

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        _CrtMemBlockHeader* const header{header_from_block(block)};
        if (!is_block_type_valid(header->_block_use))
            __leave;

        if (!_CrtIsValidPointer(block, size, TRUE))
            __leave;

        if (header->_data_size != size)
            __leave;

        if (header->_request_number > __acrt_current_request_number)
            __leave;

        // The block is valid
        if (request_number)
            *request_number = header->_request_number;

        if (file_name)
            *file_name = const_cast<char*>(header->_file_name);

        if (line_number)
            *line_number = header->_line_number;

        result = TRUE;
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry

    return result;
}



// Tests whether the block was allocated by this heap and, if it was, returns
// its block use.  Returns -1 if this block was not allocated by this heap.
extern "C" int _CrtReportBlockType(void const* const block)
{
    if (!_CrtIsValidHeapPointer(block))
        return -1;

    return header_from_block(block)->_block_use;
}



// These get and set the active dump client, which is used when dumping the state
// of the debug heap.
extern "C" _CRT_DUMP_CLIENT __cdecl _CrtGetDumpClient()
{
    return _pfnDumpClient;
}

extern "C" _CRT_DUMP_CLIENT __cdecl _CrtSetDumpClient(_CRT_DUMP_CLIENT const new_client)
{
    _CRT_DUMP_CLIENT const old_client{_pfnDumpClient};
    _pfnDumpClient = new_client;
    return old_client;
}



// Creates a checkpoint for the current state of the debug heap.  Fills in the
// object pointed to by state; state must be non-null.
extern "C" void __cdecl _CrtMemCheckpoint(_CrtMemState* const state)
{
    _VALIDATE_RETURN_VOID(state != nullptr, EINVAL);

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        state->pBlockHeader = __acrt_first_block;

        for (unsigned use{0}; use < _MAX_BLOCKS; ++use)
        {
            state->lCounts[use] = 0;
            state->lSizes [use] = 0;
        }

        for (_CrtMemBlockHeader* header{__acrt_first_block}; header != nullptr; header = header->_block_header_next)
        {
            if (_BLOCK_TYPE(header->_block_use) >= 0 && _BLOCK_TYPE(header->_block_use) < _MAX_BLOCKS)
            {
                ++state->lCounts[_BLOCK_TYPE(header->_block_use)];
                state->lSizes[_BLOCK_TYPE(header->_block_use)] += header->_data_size;
            }
            else if (header->_file_name)
            {
                _RPTN(_CRT_WARN, "Bad memory block found at 0x%p.\n" _ALLOCATION_FILE_LINENUM,
                    header,
                    header->_file_name,
                    header->_line_number);
            }
            else
            {
                _RPTN(_CRT_WARN, "Bad memory block found at 0x%p.\n", header);
            }
        }

        state->lHighWaterCount = __acrt_max_allocations;
        state->lTotalCount     = __acrt_total_allocations;
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry
}



// Computes the difference between two memory states.  The difference between
// the old_state and new_state is stored in the object pointed to by state.
// Returns TRUE if there is a difference; FALSE otherwise.  All three state
// pointers must be valid and non-null.
extern "C" int __cdecl _CrtMemDifference(
    _CrtMemState*       const state,
    _CrtMemState const* const old_state,
    _CrtMemState const* const new_state
    )
{
    _VALIDATE_RETURN(state     != nullptr, EINVAL, FALSE);
    _VALIDATE_RETURN(old_state != nullptr, EINVAL, FALSE);
    _VALIDATE_RETURN(new_state != nullptr, EINVAL, FALSE);

    bool significant_difference_found{false};
    for (int use{0}; use < _MAX_BLOCKS; ++use)
    {
        state->lSizes[use]  = new_state->lSizes[use]  - old_state->lSizes[use];
        state->lCounts[use] = new_state->lCounts[use] - old_state->lCounts[use];

        if (state->lSizes[use] == 0 && state->lCounts[use] == 0)
            continue;

        if (use == _FREE_BLOCK)
            continue;

        if (use == _CRT_BLOCK && (_crtDbgFlag & _CRTDBG_CHECK_CRT_DF) == 0)
            continue;

        significant_difference_found = true;
    }

    state->lHighWaterCount = new_state->lHighWaterCount - old_state->lHighWaterCount;
    state->lTotalCount     = new_state->lTotalCount     - old_state->lTotalCount;
    state->pBlockHeader    = nullptr;

    return significant_difference_found ? 1 : 0;
}



// Prints metadata for a block of memory.
static void __cdecl print_block_data(
    _locale_t           const locale,
    _CrtMemBlockHeader* const header
    ) throw()
{
    _LocaleUpdate locale_update{locale};

    static size_t const max_print = 16;

    char print_buffer[max_print     + 1];
    char value_buffer[max_print * 3 + 1];

    size_t i{0};
    for (; i < min(header->_data_size, max_print); ++i)
    {
        unsigned char const c{block_from_header(header)[i]};

        print_buffer[i] = _isprint_l(c, locale_update.GetLocaleT()) ? c : ' ';
        _ERRCHECK_SPRINTF(sprintf_s(value_buffer + i * 3, _countof(value_buffer) - (i * 3), "%.2X ", c));
    }

    print_buffer[i] = '\0';
    _RPTN(_CRT_WARN, " Data: <%s> %s\n", print_buffer, value_buffer);
}

// Prints metadata for all blocks allocated since the provided state was taken.
static void __cdecl dump_all_object_since_nolock(_CrtMemState const* const state) throw()
{
    _LocaleUpdate locale_update{nullptr};
    _locale_t     locale{locale_update.GetLocaleT()};

    _RPT0(_CRT_WARN, "Dumping objects ->\n");

    _CrtMemBlockHeader* const stop_block{state ? state->pBlockHeader : nullptr};

    for (_CrtMemBlockHeader* header{__acrt_first_block}; header != nullptr && header != stop_block; header = header->_block_header_next)
    {
        if (_BLOCK_TYPE(header->_block_use) == _IGNORE_BLOCK)
            continue;

        if (_BLOCK_TYPE(header->_block_use) == _FREE_BLOCK)
            continue;

        if (_BLOCK_TYPE(header->_block_use) == _CRT_BLOCK && (_crtDbgFlag & _CRTDBG_CHECK_CRT_DF) == 0)
            continue;

        if (header->_file_name != nullptr)
        {
            if (!_CrtIsValidPointer(header->_file_name, 1, FALSE) || is_bad_read_pointer(header->_file_name, 1))
            {
                _RPTN(_CRT_WARN, "#File Error#(%d) : ", header->_line_number);
            }
            else
            {
                _RPTN(_CRT_WARN, "%hs(%d) : ", header->_file_name, header->_line_number);
            }
        }

        _RPTN(_CRT_WARN, "{%ld} ", header->_request_number);

        if (_BLOCK_TYPE(header->_block_use) == _CLIENT_BLOCK)
        {
            _RPTN(_CRT_WARN, "client block at 0x%p, subtype %x, %Iu bytes long.\n",
                block_from_header(header),
                _BLOCK_SUBTYPE(header->_block_use),
                header->_data_size);

            if (_pfnDumpClient && !is_bad_read_pointer(block_from_header(header), header->_data_size))
            {
                _pfnDumpClient(block_from_header(header), header->_data_size);
            }
            else
            {
                print_block_data(locale, header);
            }
        }
        else if (header->_block_use == _NORMAL_BLOCK)
        {
            _RPTN(_CRT_WARN, "normal block at 0x%p, %Iu bytes long.\n",
                block_from_header(header),
                header->_data_size);

            print_block_data(locale, header);
        }
        else if (_BLOCK_TYPE(header->_block_use) == _CRT_BLOCK)
        {
            _RPTN(_CRT_WARN, "crt block at 0x%p, subtype %x, %Iu bytes long.\n",
                block_from_header(header),
                _BLOCK_SUBTYPE(header->_block_use),
                header->_data_size);

            print_block_data(locale, header);
        }
    }
}

// Prints metadata for all blocks allocated since the provided state was taken.
extern "C" void __cdecl _CrtMemDumpAllObjectsSince(_CrtMemState const* const state)
{
    __acrt_lock(__acrt_heap_lock);
    __try
    {
        dump_all_object_since_nolock(state);
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry

    _RPT0(_CRT_WARN, "Object dump complete.\n");
}



// Dumps all currently active allocations in this heap.  Returns TRUE if there
// are memory leaks; false otherwise.  It is assumed that all client allocations
// remaining in the heap are memory leaks.
extern "C" int __cdecl _CrtDumpMemoryLeaks()
{
    _CrtMemState state;
    _CrtMemCheckpoint(&state);

    if (state.lCounts[_CLIENT_BLOCK] != 0 ||
        state.lCounts[_NORMAL_BLOCK] != 0 ||
        (_crtDbgFlag & _CRTDBG_CHECK_CRT_DF && state.lCounts[_CRT_BLOCK] != 0))
    {
        _RPT0(_CRT_WARN, "Detected memory leaks!\n");

        _CrtMemDumpAllObjectsSince(nullptr);
        return TRUE;
    }

    return FALSE;
}



// Prints some brief information about the provided state of the heap.
extern "C" void __cdecl _CrtMemDumpStatistics(_CrtMemState const* const state)
{
    _VALIDATE_RETURN_VOID(state != nullptr, EINVAL);

    for (unsigned use{0}; use < _MAX_BLOCKS; ++use)
    {
        _RPTN(_CRT_WARN, "%Id bytes in %Id %hs Blocks.\n",
            state->lSizes[use],
            state->lCounts[use],
            block_use_names[use]);
    }

    _RPTN(_CRT_WARN, "Largest number used: %Id bytes.\n", state->lHighWaterCount);
    _RPTN(_CRT_WARN, "Total allocations: %Id bytes.\n",   state->lTotalCount);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Aligned Allocation
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions are equivalent to the normal aligned allocation functions in
// alignment.cpp, but these functions (suffixed with _dbg instead of _base) utilize
// the debug heap and accept the file name and line number as arguments.  Consult
// alignment.cpp for more information on the behavior of these functions.
struct _AlignMemBlockHdr
{
    void*         _head;
    unsigned char _gap[align_gap_size];
};



#define IS_2_POW_N(X)   ((X) != 0 && ((X) & ((X) - 1)) == 0)



extern "C" void* __cdecl _aligned_malloc_dbg(
    size_t      const size,
    size_t      const alignment,
    char const* const file_name,
    int         const line_number
    )
{
    return _aligned_offset_malloc_dbg(size, alignment, 0, file_name, line_number);
}

extern "C" void* __cdecl _aligned_offset_malloc_dbg(
    size_t      const size,
    size_t            alignment,
    size_t      const offset,
    char const* const file_name,
    int         const line_number
    )
{
    _VALIDATE_RETURN(IS_2_POW_N(alignment),        EINVAL, nullptr);
    _VALIDATE_RETURN(offset == 0 || offset < size, EINVAL, nullptr);

    alignment = (alignment > sizeof(uintptr_t) ? alignment : sizeof(uintptr_t)) - 1;

    uintptr_t const t_ptr = (0 -offset) & (sizeof(uintptr_t) -1);

    size_t const nonuser_size = t_ptr + alignment + sizeof(_AlignMemBlockHdr); // Cannot overflow
    size_t const block_size = size + nonuser_size;
    _VALIDATE_RETURN_NOEXC(size <= block_size, ENOMEM, nullptr);

    uintptr_t const ptr = reinterpret_cast<uintptr_t>(_malloc_dbg(block_size, _NORMAL_BLOCK, file_name, line_number));
    if (ptr == reinterpret_cast<uintptr_t>(nullptr))
        return nullptr;

    uintptr_t const r_ptr = ((ptr +nonuser_size +offset)&~alignment)-offset;
    _AlignMemBlockHdr* const header_from_block = reinterpret_cast<_AlignMemBlockHdr*>(r_ptr - t_ptr) - 1;
    memset(header_from_block->_gap, align_land_fill, align_gap_size);
    header_from_block->_head = reinterpret_cast<void*>(ptr);
    return reinterpret_cast<void*>(r_ptr);
}

extern "C" void* __cdecl _aligned_realloc_dbg(
    void*       const block,
    size_t      const size,
    size_t      const alignment,
    char const* const file_name,
    int         const line_number
    )
{
    return _aligned_offset_realloc_dbg(block, size, alignment, 0, file_name, line_number);
}

extern "C" void* __cdecl _aligned_recalloc_dbg(
    void*       const block,
    size_t      const count,
    size_t      const size,
    size_t      const alignment,
    char const* const file_name,
    int         const line_number
    )
{
    return _aligned_offset_recalloc_dbg(block, count, size, alignment, 0, file_name, line_number);
}

extern "C" void* __cdecl _aligned_offset_realloc_dbg(
    void * block,
    size_t size,
    size_t alignment,
    size_t offset,
    const char * file_name,
    int line_number
    )
{
    uintptr_t ptr, r_ptr, t_ptr, mov_sz;
    _AlignMemBlockHdr *header_from_block, *s_header_from_block;
    size_t nonuser_size, block_size;

    if (block == nullptr)
    {
        return _aligned_offset_malloc_dbg(size, alignment, offset, file_name, line_number);
    }
    if (size == 0)
    {
        _aligned_free_dbg(block);
        return nullptr;
    }

    s_header_from_block = (_AlignMemBlockHdr *)((uintptr_t)block & ~(sizeof(uintptr_t) -1)) -1;

    if (check_bytes((unsigned char *)block -no_mans_land_size, no_mans_land_fill, no_mans_land_size))
    {
        // We don't know where (file, linenum) block was allocated
        _RPTN(_CRT_ERROR, "The block at 0x%p was not allocated by _aligned routines, use realloc()", block);
        errno = EINVAL;
        return nullptr;
    }

    if(!check_bytes(s_header_from_block->_gap, align_land_fill, align_gap_size))
    {
        // We don't know where (file, linenum) block was allocated
        _RPTN(_CRT_ERROR, "Damage before 0x%p which was allocated by aligned routine\n", block);
    }

    /* validation section */
    _VALIDATE_RETURN(IS_2_POW_N(alignment),        EINVAL, nullptr);
    _VALIDATE_RETURN(offset == 0 || offset < size, EINVAL, nullptr);

    mov_sz = _msize_dbg(s_header_from_block->_head, _NORMAL_BLOCK) - ((uintptr_t)block - (uintptr_t)s_header_from_block->_head);

    alignment = (alignment > sizeof(uintptr_t) ? alignment : sizeof(uintptr_t)) -1;

    t_ptr = (0 -offset) & (sizeof(uintptr_t) - 1);

    nonuser_size = t_ptr + alignment + sizeof(_AlignMemBlockHdr); // Cannot overflow
    block_size = size + nonuser_size;
    _VALIDATE_RETURN_NOEXC(size <= block_size, ENOMEM, nullptr);

    if ((ptr = (uintptr_t)_malloc_dbg(block_size, _NORMAL_BLOCK, file_name, line_number)) == (uintptr_t)nullptr)
        return nullptr;

    r_ptr = ((ptr + nonuser_size + offset) & ~alignment) - offset;
    header_from_block = (_AlignMemBlockHdr*)(r_ptr - t_ptr) - 1;
    memset(header_from_block->_gap, align_land_fill, align_gap_size);
    header_from_block->_head = reinterpret_cast<void*>(ptr);

    memcpy(reinterpret_cast<void*>(r_ptr), block, mov_sz > size ? size : mov_sz);
    _free_dbg(s_header_from_block->_head, _NORMAL_BLOCK);

    return (void *) r_ptr;
}

extern "C" size_t __cdecl _aligned_msize_dbg(
    void*  const block,
    size_t       alignment,
    size_t const offset
    )
{
    size_t header_size = 0; // Size of the header block
    size_t footer_size = 0; // Size of the footer block
    size_t total_size  = 0; // total size of the allocated block
    size_t user_size   = 0; // size of the user block
    uintptr_t gap      = 0; // keep the alignment of the data block
                            // after the sizeof(void*) aligned pointer
                            // to the beginning of the allocated block

    // HEADER SIZE + FOOTER SIZE = GAP + ALIGN + SIZE OF A POINTER
    // HEADER SIZE + USER SIZE + FOOTER SIZE = TOTAL SIZE
    _VALIDATE_RETURN (block != nullptr, EINVAL, static_cast<size_t>(-1));

    _AlignMemBlockHdr* header_from_block = nullptr; // points to the beginning of the allocated block
    header_from_block = (_AlignMemBlockHdr*)((uintptr_t)block & ~(sizeof(uintptr_t) - 1)) - 1;
    total_size = _msize_dbg(header_from_block->_head, _NORMAL_BLOCK);
    header_size = (uintptr_t)block - (uintptr_t)header_from_block->_head;
    gap = (0 - offset) & (sizeof(uintptr_t) - 1);
    // The alignment cannot be smaller than the sizeof(uintptr_t)
    alignment = (alignment > sizeof(uintptr_t) ? alignment : sizeof(uintptr_t)) - 1;
    footer_size = gap + alignment + sizeof(_AlignMemBlockHdr) - header_size;
    user_size = total_size - header_size - footer_size;
    return user_size;
}

extern "C" void* __cdecl _aligned_offset_recalloc_dbg(
    void*       const block,
    size_t      const count,
    size_t      const element_size,
    size_t      const alignment,
    size_t      const offset,
    char const* const file_name,
    int         const line_number
    )
{
    _VALIDATE_RETURN_NOEXC(count == 0 || _HEAP_MAXREQ / count >= element_size, ENOMEM, nullptr);

    size_t const old_allocation_size{block ? _aligned_msize_dbg(block, alignment, offset) : 0};
    size_t const new_allocation_size{element_size * count};

    void* const new_block{_aligned_offset_realloc_dbg(block, new_allocation_size, alignment, offset, file_name, line_number)};
    if (!new_block)
        return nullptr;

    if (old_allocation_size < new_allocation_size)
        memset(static_cast<unsigned char*>(new_block) + old_allocation_size, 0, new_allocation_size - old_allocation_size);

    return new_block;
}

extern "C" void __cdecl _aligned_free_dbg(void* const block)
{
    if (!block)
        return;

    _AlignMemBlockHdr* const header{reinterpret_cast<_AlignMemBlockHdr*>(
        reinterpret_cast<uintptr_t>(block) & ~(sizeof(uintptr_t) - 1)
    ) -1};

    if (check_bytes(static_cast<unsigned char*>(block) - no_mans_land_size, no_mans_land_fill, no_mans_land_size))
    {
        // We don't know where (file, linenum) block was allocated
        _RPTN(_CRT_ERROR, "The block at 0x%p was not allocated by _aligned routines, use free()", block);
        return;
    }

    if (!check_bytes(header->_gap, align_land_fill, align_gap_size))
    {
        // We don't know where (file, linenum) block was allocated
        _RPTN(_CRT_ERROR, "Damage before 0x%p which was allocated by aligned routine\n", block);
    }

    _free_dbg(header->_head, _NORMAL_BLOCK);
}
