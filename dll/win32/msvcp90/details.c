/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

/* Keep in sync with concrt140/detail.c */

#include <stdarg.h>
#include "msvcp90.h"

#include "wine/debug.h"
#include "wine/exception.h"

#if _MSVCP_VER >= 100

WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

typedef struct _Page
{
    struct _Page *_Next;
    size_t _Mask;
    char data[1];
} _Page;

typedef struct
{
    LONG lock;
    _Page *head;
    _Page *tail;
    size_t head_pos;
    size_t tail_pos;
} threadsafe_queue;

#define QUEUES_NO 8
typedef struct
{
    size_t tail_pos;
    size_t head_pos;
    threadsafe_queue queues[QUEUES_NO];
} queue_data;

typedef struct
{
    const vtable_ptr *vtable;
    queue_data *data; /* queue_data structure is not binary compatible */
    size_t alloc_count;
    size_t item_size;
} _Concurrent_queue_base_v4;

extern const vtable_ptr _Concurrent_queue_base_v4_vtable;
#if _MSVCP_VER == 100
#define call__Concurrent_queue_base_v4__Move_item call__Concurrent_queue_base_v4__Copy_item
#define call__Concurrent_queue_base_v4__Copy_item(this,dst,idx,src) CALL_VTBL_FUNC(this, \
        0, void, (_Concurrent_queue_base_v4*,_Page*,size_t,const void*), (this,dst,idx,src))
#define  call__Concurrent_queue_base_v4__Assign_and_destroy_item(this,dst,src,idx) CALL_VTBL_FUNC(this, \
        4, void, (_Concurrent_queue_base_v4*,void*,_Page*,size_t), (this,dst,src,idx))
#define call__Concurrent_queue_base_v4__Allocate_page(this) CALL_VTBL_FUNC(this, \
        12, _Page*, (_Concurrent_queue_base_v4*), (this))
#define call__Concurrent_queue_base_v4__Deallocate_page(this, page) CALL_VTBL_FUNC(this, \
        16, void, (_Concurrent_queue_base_v4*,_Page*), (this,page))
#else
#define call__Concurrent_queue_base_v4__Move_item(this,dst,idx,src) CALL_VTBL_FUNC(this, \
        0, void, (_Concurrent_queue_base_v4*,_Page*,size_t,void*), (this,dst,idx,src))
#define call__Concurrent_queue_base_v4__Copy_item(this,dst,idx,src) CALL_VTBL_FUNC(this, \
        4, void, (_Concurrent_queue_base_v4*,_Page*,size_t,const void*), (this,dst,idx,src))
#define  call__Concurrent_queue_base_v4__Assign_and_destroy_item(this,dst,src,idx) CALL_VTBL_FUNC(this, \
        8, void, (_Concurrent_queue_base_v4*,void*,_Page*,size_t), (this,dst,src,idx))
#define call__Concurrent_queue_base_v4__Allocate_page(this) CALL_VTBL_FUNC(this, \
        16, _Page*, (_Concurrent_queue_base_v4*), (this))
#define call__Concurrent_queue_base_v4__Deallocate_page(this, page) CALL_VTBL_FUNC(this, \
        20, void, (_Concurrent_queue_base_v4*,_Page*), (this,page))
#endif

/* ?_Internal_throw_exception@_Concurrent_queue_base_v4@details@Concurrency@@IBEXXZ */
/* ?_Internal_throw_exception@_Concurrent_queue_base_v4@details@Concurrency@@IEBAXXZ */
DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4__Internal_throw_exception, 4)
void __thiscall _Concurrent_queue_base_v4__Internal_throw_exception(
        const _Concurrent_queue_base_v4 *this)
{
    TRACE("(%p)\n", this);
    _Xmem();
}

/* ??0_Concurrent_queue_base_v4@details@Concurrency@@IAE@I@Z */
/* ??0_Concurrent_queue_base_v4@details@Concurrency@@IEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4_ctor, 8)
_Concurrent_queue_base_v4* __thiscall _Concurrent_queue_base_v4_ctor(
        _Concurrent_queue_base_v4 *this, size_t size)
{
    TRACE("(%p %Iu)\n", this, size);

    this->data = operator_new(sizeof(*this->data));
    memset(this->data, 0, sizeof(*this->data));

    this->vtable = &_Concurrent_queue_base_v4_vtable;
    this->item_size = size;

    /* alloc_count needs to be power of 2 */
    this->alloc_count =
        size <= 8 ? 32 :
        size <= 16 ? 16 :
        size <= 32 ? 8 :
        size <= 64 ? 4 :
        size <= 128 ? 2 : 1;
    return this;
}

/* ??1_Concurrent_queue_base_v4@details@Concurrency@@MAE@XZ */
/* ??1_Concurrent_queue_base_v4@details@Concurrency@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4_dtor, 4)
void __thiscall _Concurrent_queue_base_v4_dtor(_Concurrent_queue_base_v4 *this)
{
    TRACE("(%p)\n", this);
    operator_delete(this->data);
}

DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4_vector_dtor, 8)
_Concurrent_queue_base_v4* __thiscall _Concurrent_queue_base_v4_vector_dtor(
        _Concurrent_queue_base_v4 *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            _Concurrent_queue_base_v4_dtor(this+i);
        operator_delete(ptr);
    } else {
        if(flags & 1)
            _Concurrent_queue_base_v4_dtor(this);
        operator_delete(this);
    }

    return this;
}

/* ?_Internal_finish_clear@_Concurrent_queue_base_v4@details@Concurrency@@IAEXXZ */
/* ?_Internal_finish_clear@_Concurrent_queue_base_v4@details@Concurrency@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4__Internal_finish_clear, 4)
void __thiscall _Concurrent_queue_base_v4__Internal_finish_clear(
        _Concurrent_queue_base_v4 *this)
{
    int i;

    TRACE("(%p)\n", this);

    for(i=0; i<QUEUES_NO; i++)
    {
        if(this->data->queues[i].tail)
            call__Concurrent_queue_base_v4__Deallocate_page(this, this->data->queues[i].tail);
    }
}

/* ?_Internal_empty@_Concurrent_queue_base_v4@details@Concurrency@@IBE_NXZ */
/* ?_Internal_empty@_Concurrent_queue_base_v4@details@Concurrency@@IEBA_NXZ */
DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4__Internal_empty, 4)
bool __thiscall _Concurrent_queue_base_v4__Internal_empty(
        const _Concurrent_queue_base_v4 *this)
{
    TRACE("(%p)\n", this);
    return this->data->head_pos == this->data->tail_pos;
}

/* ?_Internal_size@_Concurrent_queue_base_v4@details@Concurrency@@IBEIXZ */
/* ?_Internal_size@_Concurrent_queue_base_v4@details@Concurrency@@IEBA_KXZ */
DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4__Internal_size, 4)
size_t __thiscall _Concurrent_queue_base_v4__Internal_size(
        const _Concurrent_queue_base_v4 *this)
{
    TRACE("(%p)\n", this);
    return this->data->tail_pos - this->data->head_pos;
}

static void spin_wait(int *counter)
{
    static int spin_limit = -1;

    if(spin_limit == -1)
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        spin_limit = si.dwNumberOfProcessors>1 ? 4000 : 0;
    }

    if(*counter >= spin_limit)
    {
        *counter = 0;
        Sleep(0);
    }
    else
    {
        (*counter)++;
    }
}

static void CALLBACK queue_push_finally(BOOL normal, void *ctx)
{
    threadsafe_queue *queue = ctx;
    InterlockedIncrementSizeT(&queue->tail_pos);
}

static void threadsafe_queue_push(threadsafe_queue *queue, size_t id,
        void *e, _Concurrent_queue_base_v4 *parent, BOOL copy)
{
    size_t page_id = id & ~(parent->alloc_count-1);
    int spin;
    _Page *p;

    spin = 0;
    while(queue->tail_pos != id)
        spin_wait(&spin);

    if(page_id == id)
    {
        /* TODO: Add exception handling */
        p = call__Concurrent_queue_base_v4__Allocate_page(parent);
        p->_Next = NULL;
        p->_Mask = 0;

        spin = 0;
        while(InterlockedCompareExchange(&queue->lock, 1, 0))
            spin_wait(&spin);
        if(queue->tail)
            queue->tail->_Next = p;
        queue->tail = p;
        if(!queue->head)
            queue->head = p;
        WriteRelease(&queue->lock, 0);
    }
    else
    {
        p = queue->tail;
    }

    __TRY
    {
        if(copy)
            call__Concurrent_queue_base_v4__Copy_item(parent, p, id-page_id, e);
        else
            call__Concurrent_queue_base_v4__Move_item(parent, p, id-page_id, e);
        p->_Mask |= 1 << (id - page_id);
    }
    __FINALLY_CTX(queue_push_finally, queue);
}

static BOOL threadsafe_queue_pop(threadsafe_queue *queue, size_t id,
        void *e, _Concurrent_queue_base_v4 *parent)
{
    size_t page_id = id & ~(parent->alloc_count-1);
    int spin;
    _Page *p;
    BOOL ret = FALSE;

    spin = 0;
    while(queue->tail_pos <= id)
        spin_wait(&spin);

    spin = 0;
    while(queue->head_pos != id)
        spin_wait(&spin);

    p = queue->head;
    if(p->_Mask & (1 << (id-page_id)))
    {
        /* TODO: Add exception handling */
        call__Concurrent_queue_base_v4__Assign_and_destroy_item(parent, e, p, id-page_id);
        ret = TRUE;
    }

    if(id == page_id+parent->alloc_count-1)
    {
        spin = 0;
        while(InterlockedCompareExchange(&queue->lock, 1, 0))
            spin_wait(&spin);
        queue->head = p->_Next;
        if(!queue->head)
            queue->tail = NULL;
        WriteRelease(&queue->lock, 0);

        /* TODO: Add exception handling */
        call__Concurrent_queue_base_v4__Deallocate_page(parent, p);
    }

    InterlockedIncrementSizeT(&queue->head_pos);
    return ret;
}

/* ?_Internal_push@_Concurrent_queue_base_v4@details@Concurrency@@IAEXPBX@Z */
/* ?_Internal_push@_Concurrent_queue_base_v4@details@Concurrency@@IEAAXPEBX@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4__Internal_push, 8)
void __thiscall _Concurrent_queue_base_v4__Internal_push(
        _Concurrent_queue_base_v4 *this, void *e)
{
    size_t id;

    TRACE("(%p %p)\n", this, e);

    id = InterlockedIncrementSizeT(&this->data->tail_pos)-1;
    threadsafe_queue_push(this->data->queues + id % QUEUES_NO,
            id / QUEUES_NO, e, this, TRUE);
}

/* ?_Internal_move_push@_Concurrent_queue_base_v4@details@Concurrency@@IAEXPAX@Z */
/* ?_Internal_move_push@_Concurrent_queue_base_v4@details@Concurrency@@IEAAXPEAX@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4__Internal_move_push, 8)
void __thiscall _Concurrent_queue_base_v4__Internal_move_push(
        _Concurrent_queue_base_v4 *this, void *e)
{
    size_t id;

    TRACE("(%p %p)\n", this, e);

    id = InterlockedIncrementSizeT(&this->data->tail_pos)-1;
    threadsafe_queue_push(this->data->queues + id % QUEUES_NO,
            id / QUEUES_NO, e, this, FALSE);
}

/* ?_Internal_pop_if_present@_Concurrent_queue_base_v4@details@Concurrency@@IAE_NPAX@Z */
/* ?_Internal_pop_if_present@_Concurrent_queue_base_v4@details@Concurrency@@IEAA_NPEAX@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4__Internal_pop_if_present, 8)
bool __thiscall _Concurrent_queue_base_v4__Internal_pop_if_present(
        _Concurrent_queue_base_v4 *this, void *e)
{
    size_t id;

    TRACE("(%p %p)\n", this, e);

    do
    {
        do
        {
            id = this->data->head_pos;
            if(id == this->data->tail_pos) return FALSE;
        } while(InterlockedCompareExchangePointer((void**)&this->data->head_pos,
                    (void*)(id+1), (void*)id) != (void*)id);
    } while(!threadsafe_queue_pop(this->data->queues + id % QUEUES_NO,
                id / QUEUES_NO, e, this));
    return TRUE;
}

/* ?_Internal_swap@_Concurrent_queue_base_v4@details@Concurrency@@IAEXAAV123@@Z */
/* ?_Internal_swap@_Concurrent_queue_base_v4@details@Concurrency@@IEAAXAEAV123@@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4__Internal_swap, 8)
void __thiscall _Concurrent_queue_base_v4__Internal_swap(
        _Concurrent_queue_base_v4 *this, _Concurrent_queue_base_v4 *r)
{
    FIXME("(%p %p) stub\n", this, r);
}

DEFINE_THISCALL_WRAPPER(_Concurrent_queue_base_v4_dummy, 4)
void __thiscall _Concurrent_queue_base_v4_dummy(_Concurrent_queue_base_v4 *this)
{
    ERR("unexpected call\n");
}

DEFINE_RTTI_DATA0(_Concurrent_queue_base_v4, 0, ".?AV_Concurrent_queue_base_v4@details@Concurrency@@")

static LONG _Runtime_object_id;

typedef struct
{
    const vtable_ptr *vtable;
    int id;
} _Runtime_object;

extern const vtable_ptr _Runtime_object_vtable;

/* ??0_Runtime_object@details@Concurrency@@QAE@H@Z */
/* ??0_Runtime_object@details@Concurrency@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(_Runtime_object_ctor_id, 8)
_Runtime_object* __thiscall _Runtime_object_ctor_id(_Runtime_object *this, int id)
{
    TRACE("(%p %d)\n", this, id);
    this->vtable = &_Runtime_object_vtable;
    this->id = id;
    return this;
}

/* ??0_Runtime_object@details@Concurrency@@QAE@XZ */
/* ??0_Runtime_object@details@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Runtime_object_ctor, 4)
_Runtime_object* __thiscall _Runtime_object_ctor(_Runtime_object *this)
{
    TRACE("(%p)\n", this);
    this->vtable = &_Runtime_object_vtable;
    this->id = InterlockedExchangeAdd(&_Runtime_object_id, 2);
    return this;
}

DEFINE_THISCALL_WRAPPER(_Runtime_object__GetId, 4)
int __thiscall _Runtime_object__GetId(_Runtime_object *this)
{
    TRACE("(%p)\n", this);
    return this->id;
}

DEFINE_RTTI_DATA0(_Runtime_object, 0, ".?AV_Runtime_object@details@Concurrency@@")

typedef struct __Concurrent_vector_base_v4
{
    void* (__cdecl *allocator)(struct __Concurrent_vector_base_v4 *, size_t);
    void *storage[3];
    size_t first_block;
    size_t early_size;
    void **segment;
} _Concurrent_vector_base_v4;

#define STORAGE_SIZE ARRAY_SIZE(this->storage)
#define SEGMENT_SIZE (sizeof(void*) * 8)

typedef struct compact_block
{
    size_t first_block;
    void *blocks[SEGMENT_SIZE];
    int size_check;
}compact_block;

/* Return the integer base-2 logarithm of (x|1). Result is 0 for x == 0. */
static inline unsigned int log2i(unsigned int x)
{
    ULONG index;
    BitScanReverse(&index, x|1);
    return index;
}

/* ?_Segment_index_of@_Concurrent_vector_base_v4@details@Concurrency@@KAII@Z */
/* ?_Segment_index_of@_Concurrent_vector_base_v4@details@Concurrency@@KA_K_K@Z */
size_t __cdecl _vector_base_v4__Segment_index_of(size_t x)
{
    unsigned int half;

    TRACE("(%Iu)\n", x);

    if((sizeof(x) == 8) && (half = x >> 32))
        return log2i(half) + 32;

    return log2i(x);
}

/* ?_Internal_throw_exception@_Concurrent_vector_base_v4@details@Concurrency@@IBEXI@Z */
/* ?_Internal_throw_exception@_Concurrent_vector_base_v4@details@Concurrency@@IEBAX_K@Z */
DEFINE_THISCALL_WRAPPER(_vector_base_v4__Internal_throw_exception, 8)
void __thiscall _vector_base_v4__Internal_throw_exception(void/*_vector_base_v4*/ *this, size_t idx)
{
    TRACE("(%p %Iu)\n", this, idx);

    switch(idx) {
    case 0: _Xout_of_range("Index out of range");
    case 1: _Xout_of_range("Index out of segments table range");
    case 2: throw_range_error("Index is inside segment which failed to be allocated");
    }
}

#ifdef _WIN64
#define InterlockedCompareExchangeSizeT(dest, exchange, cmp) InterlockedCompareExchangeSize((size_t *)dest, (size_t)exchange, (size_t)cmp)
static size_t InterlockedCompareExchangeSize(size_t volatile *dest, size_t exchange, size_t cmp)
{
    size_t v;

    v = InterlockedCompareExchange64((LONGLONG*)dest, exchange, cmp);

    return v;
}
#else
#define InterlockedCompareExchangeSizeT(dest, exchange, cmp) InterlockedCompareExchange((LONG*)dest, (size_t)exchange, (size_t)cmp)
#endif

#define SEGMENT_ALLOC_MARKER ((void*)1)

static void concurrent_vector_alloc_segment(_Concurrent_vector_base_v4 *this,
        size_t seg, size_t element_size)
{
    int spin;

    while(!this->segment[seg] || this->segment[seg] == SEGMENT_ALLOC_MARKER)
    {
        spin = 0;
        while(this->segment[seg] == SEGMENT_ALLOC_MARKER)
            spin_wait(&spin);
        if(!InterlockedCompareExchangeSizeT((this->segment + seg),
                    SEGMENT_ALLOC_MARKER, 0))
        {
            __TRY
            {
                if(seg == 0)
                    this->segment[seg] = this->allocator(this, element_size * (1 << this->first_block));
                else if(seg < this->first_block)
                    this->segment[seg] = (BYTE**)this->segment[0]
                        + element_size * (1 << seg);
                else
                    this->segment[seg] = this->allocator(this, element_size * (1 << seg));
            }
            __EXCEPT_ALL
            {
                this->segment[seg] = NULL;
                _CxxThrowException(NULL, NULL);
            }
            __ENDTRY
            if(!this->segment[seg])
                _vector_base_v4__Internal_throw_exception(this, 2);
        }
    }
}

/* ??1_Concurrent_vector_base_v4@details@Concurrency@@IAE@XZ */
/* ??1_Concurrent_vector_base_v4@details@Concurrency@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4_dtor, 4)
void __thiscall _Concurrent_vector_base_v4_dtor(
        _Concurrent_vector_base_v4 *this)
{
    TRACE("(%p)\n", this);

    if(this->segment != this->storage)
        free(this->segment);
}

/* ?_Internal_capacity@_Concurrent_vector_base_v4@details@Concurrency@@IBEIXZ */
/* ?_Internal_capacity@_Concurrent_vector_base_v4@details@Concurrency@@IEBA_KXZ */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_capacity, 4)
size_t __thiscall _Concurrent_vector_base_v4__Internal_capacity(
        const _Concurrent_vector_base_v4 *this)
{
    size_t last_block;
    int i;

    TRACE("(%p)\n", this);

    last_block = (this->segment == this->storage ? STORAGE_SIZE : SEGMENT_SIZE);
    for(i = 0; i < last_block; i++)
    {
        if(!this->segment[i])
            return !i ? 0 : 1 << i;
    }
    return 1 << i;
}

/* ?_Internal_reserve@_Concurrent_vector_base_v4@details@Concurrency@@IAEXIII@Z */
/* ?_Internal_reserve@_Concurrent_vector_base_v4@details@Concurrency@@IEAAX_K00@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_reserve, 16)
void __thiscall _Concurrent_vector_base_v4__Internal_reserve(
        _Concurrent_vector_base_v4 *this, size_t size,
        size_t element_size, size_t max_size)
{
    size_t block_idx, capacity;
    int i;
    void **new_segment;

    TRACE("(%p %Iu %Iu %Iu)\n", this, size, element_size, max_size);

    if(size > max_size) _vector_base_v4__Internal_throw_exception(this, 0);
    capacity = _Concurrent_vector_base_v4__Internal_capacity(this);
    if(size <= capacity) return;
    block_idx = _vector_base_v4__Segment_index_of(size - 1);
    if(!this->first_block)
        InterlockedCompareExchangeSizeT(&this->first_block, block_idx + 1, 0);
    i = _vector_base_v4__Segment_index_of(capacity);
    if(this->storage == this->segment) {
        for(; i <= block_idx && i < STORAGE_SIZE; i++)
            concurrent_vector_alloc_segment(this, i, element_size);
        if(block_idx >= STORAGE_SIZE) {
            new_segment = malloc(SEGMENT_SIZE * sizeof(void*));
            if(new_segment == NULL) _vector_base_v4__Internal_throw_exception(this, 2);
            memset(new_segment, 0, SEGMENT_SIZE * sizeof(*new_segment));
            memcpy(new_segment, this->storage, STORAGE_SIZE * sizeof(*new_segment));
            if(InterlockedCompareExchangePointer((void*)&this->segment, new_segment,
                        this->storage) != this->storage)
                free(new_segment);
        }
    }
    for(; i <= block_idx; i++)
        concurrent_vector_alloc_segment(this, i, element_size);
}

/* ?_Internal_clear@_Concurrent_vector_base_v4@details@Concurrency@@IAEIP6AXPAXI@Z@Z */
/* ?_Internal_clear@_Concurrent_vector_base_v4@details@Concurrency@@IEAA_KP6AXPEAX_K@Z@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_clear, 8)
size_t __thiscall _Concurrent_vector_base_v4__Internal_clear(
        _Concurrent_vector_base_v4 *this, void (__cdecl *clear)(void*, size_t))
{
    size_t seg_no, elems;
    int i;

    TRACE("(%p %p)\n", this, clear);

    seg_no = this->early_size  ? _vector_base_v4__Segment_index_of(this->early_size) + 1 : 0;
    for(i = seg_no - 1; i >= 0; i--) {
        elems = this->early_size - (1 << i & ~1);
        clear(this->segment[i], elems);
        this->early_size -= elems;
    }
    while(seg_no < (this->segment == this->storage ? STORAGE_SIZE : SEGMENT_SIZE)) {
        if(!this->segment[seg_no]) break;
        seg_no++;
    }
    return seg_no;
}

/* ?_Internal_compact@_Concurrent_vector_base_v4@details@Concurrency@@IAEPAXIPAXP6AX0I@ZP6AX0PBXI@Z@Z */
/* ?_Internal_compact@_Concurrent_vector_base_v4@details@Concurrency@@IEAAPEAX_KPEAXP6AX10@ZP6AX1PEBX0@Z@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_compact, 20)
void * __thiscall _Concurrent_vector_base_v4__Internal_compact(
        _Concurrent_vector_base_v4 *this, size_t element_size, void *v,
        void (__cdecl *clear)(void*, size_t),
        void (__cdecl *copy)(void*, const void*, size_t))
{
    compact_block *b;
    size_t size, alloc_size, seg_no, alloc_seg, copy_element, clear_element;
    int i;

    TRACE("(%p %Iu %p %p %p)\n", this, element_size, v, clear, copy);

    size = this->early_size;
    alloc_size = _Concurrent_vector_base_v4__Internal_capacity(this);
    if(alloc_size == 0) return NULL;
    alloc_seg = _vector_base_v4__Segment_index_of(alloc_size - 1);
    if(!size) {
        this->first_block = 0;
        b = v;
        b->first_block = alloc_seg + 1;
        memset(b->blocks, 0, sizeof(b->blocks));
        memcpy(b->blocks, this->segment,
                (alloc_seg + 1) * sizeof(this->segment[0]));
        memset(this->segment, 0, sizeof(this->segment[0]) * (alloc_seg + 1));
        return v;
    }
    seg_no = _vector_base_v4__Segment_index_of(size - 1);
    if(this->first_block == (seg_no + 1) && seg_no == alloc_seg) return NULL;
    b = v;
    b->first_block = this->first_block;
    memset(b->blocks, 0, sizeof(b->blocks));
    memcpy(b->blocks, this->segment,
            (alloc_seg + 1) * sizeof(this->segment[0]));
    if(this->first_block == (seg_no + 1) && seg_no != alloc_seg) {
        memset(b->blocks, 0, sizeof(b->blocks[0]) * (seg_no + 1));
        memset(&this->segment[seg_no + 1], 0, sizeof(this->segment[0]) * (alloc_seg - seg_no));
        return v;
    }
    memset(this->segment, 0,
            (alloc_seg + 1) * sizeof(this->segment[0]));
    this->first_block = 0;
    _Concurrent_vector_base_v4__Internal_reserve(this, size, element_size,
            ~(size_t)0 / element_size);
    for(i = 0; i < seg_no; i++)
        copy(this->segment[i], b->blocks[i], i ? 1 << i : 2);
    copy_element = size - ((1 << seg_no) & ~1);
    if(copy_element > 0)
        copy(this->segment[seg_no], b->blocks[seg_no], copy_element);
    for(i = 0; i < seg_no; i++)
        clear(b->blocks[i], i ? 1 << i : 2);
    clear_element = size - ((1 << seg_no) & ~1);
    if(clear_element > 0)
        clear(b->blocks[seg_no], clear_element);
    return v;
}

/* ?_Internal_copy@_Concurrent_vector_base_v4@details@Concurrency@@IAEXABV123@IP6AXPAXPBXI@Z@Z */
/* ?_Internal_copy@_Concurrent_vector_base_v4@details@Concurrency@@IEAAXAEBV123@_KP6AXPEAXPEBX1@Z@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_copy, 16)
void __thiscall _Concurrent_vector_base_v4__Internal_copy(
        _Concurrent_vector_base_v4 *this, const _Concurrent_vector_base_v4 *v,
        size_t element_size, void (__cdecl *copy)(void*, const void*, size_t))
{
    size_t seg_no, v_size;
    int i;

    TRACE("(%p %p %Iu %p)\n", this, v, element_size, copy);

    v_size = v->early_size;
    if(!v_size) {
        this->early_size = 0;
       return;
    }
    _Concurrent_vector_base_v4__Internal_reserve(this, v_size,
            element_size, ~(size_t)0 / element_size);
    seg_no = _vector_base_v4__Segment_index_of(v_size - 1);
    for(i = 0; i < seg_no; i++)
        copy(this->segment[i], v->segment[i], i ? 1 << i : 2);
    copy(this->segment[i], v->segment[i], v_size - (1 << i & ~1));
    this->early_size = v_size;
}

/* ?_Internal_assign@_Concurrent_vector_base_v4@details@Concurrency@@IAEXABV123@IP6AXPAXI@ZP6AX1PBXI@Z4@Z */
/* ?_Internal_assign@_Concurrent_vector_base_v4@details@Concurrency@@IEAAXAEBV123@_KP6AXPEAX1@ZP6AX2PEBX1@Z5@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_assign, 24)
void __thiscall _Concurrent_vector_base_v4__Internal_assign(
        _Concurrent_vector_base_v4 *this, const _Concurrent_vector_base_v4 *v,
        size_t element_size, void (__cdecl *clear)(void*, size_t),
        void (__cdecl *assign)(void*, const void*, size_t),
        void (__cdecl *copy)(void*, const void*, size_t))
{
    size_t v_size, seg_no, v_seg_no, remain_element;
    int i;

    TRACE("(%p %p %Iu %p %p %p)\n", this, v, element_size, clear, assign, copy);

    v_size = v->early_size;
    if(!v_size) {
        _Concurrent_vector_base_v4__Internal_clear(this, clear);
        return;
    }
    if(!this->early_size) {
        _Concurrent_vector_base_v4__Internal_copy(this, v, element_size, copy);
        return;
    }
    seg_no = _vector_base_v4__Segment_index_of(this->early_size - 1);
    v_seg_no = _vector_base_v4__Segment_index_of(v_size - 1);

    for(i = 0; i < min(seg_no, v_seg_no); i++)
        assign(this->segment[i], v->segment[i], i ? 1 << i : 2);
    remain_element = min(this->early_size, v_size) - (1 << i & ~1);
    if(remain_element != 0)
        assign(this->segment[i], v->segment[i], remain_element);

    if(this->early_size > v_size)
    {
        if((i ? 1 << i : 2) - remain_element > 0)
            clear((BYTE**)this->segment[i] + element_size * remain_element,
                    (i ? 1 << i : 2) - remain_element);
        if(i < seg_no)
        {
            for(i++; i < seg_no; i++)
                clear(this->segment[i], 1 << i);
            clear(this->segment[i], this->early_size - (1 << i));
        }
    }
    else if(this->early_size < v_size)
    {
        if((i ? 1 << i : 2) - remain_element > 0)
            copy((BYTE**)this->segment[i] + element_size * remain_element,
                    (BYTE**)v->segment[i] + element_size * remain_element,
                    (i ? 1 << i : 2) - remain_element);
        if(i < v_seg_no)
        {
            _Concurrent_vector_base_v4__Internal_reserve(this, v_size,
                    element_size, ~(size_t)0 / element_size);
            for(i++; i < v_seg_no; i++)
                copy(this->segment[i], v->segment[i], 1 << i);
            copy(this->segment[i], v->segment[i], v->early_size - (1 << i));
        }
   }
    this->early_size = v_size;
}

/* ?_Internal_grow_by@_Concurrent_vector_base_v4@details@Concurrency@@IAEIIIP6AXPAXPBXI@Z1@Z */
/* ?_Internal_grow_by@_Concurrent_vector_base_v4@details@Concurrency@@IEAA_K_K0P6AXPEAXPEBX0@Z2@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_grow_by, 20)
size_t __thiscall _Concurrent_vector_base_v4__Internal_grow_by(
        _Concurrent_vector_base_v4 *this, size_t count, size_t element_size,
        void (__cdecl *copy)(void*, const void*, size_t), const void *v)
{
    size_t size, seg_no, last_seg_no, remain_size;

    TRACE("(%p %Iu %Iu %p %p)\n", this, count, element_size, copy, v);

    if(count == 0) return this->early_size;
    do {
        size = this->early_size;
        _Concurrent_vector_base_v4__Internal_reserve(this, size + count, element_size,
                ~(size_t)0 / element_size);
    } while(InterlockedCompareExchangeSizeT(&this->early_size, size + count, size) != size);

    seg_no = size ? _vector_base_v4__Segment_index_of(size - 1) : 0;
    last_seg_no = _vector_base_v4__Segment_index_of(size + count - 1);
    remain_size = min(size + count, 1 << (seg_no + 1)) - size;
    if(remain_size > 0)
        copy(((BYTE**)this->segment[seg_no] + element_size * (size - ((1 << seg_no) & ~1))), v,
            remain_size);
    if(seg_no != last_seg_no)
    {
        for(seg_no++; seg_no < last_seg_no; seg_no++)
            copy(this->segment[seg_no], v, 1 << seg_no);
        copy(this->segment[last_seg_no], v, size + count - (1 << last_seg_no));
    }
    return size;
}

/* ?_Internal_grow_to_at_least_with_result@_Concurrent_vector_base_v4@details@Concurrency@@IAEIIIP6AXPAXPBXI@Z1@Z */
/* ?_Internal_grow_to_at_least_with_result@_Concurrent_vector_base_v4@details@Concurrency@@IEAA_K_K0P6AXPEAXPEBX0@Z2@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_grow_to_at_least_with_result, 20)
size_t __thiscall _Concurrent_vector_base_v4__Internal_grow_to_at_least_with_result(
        _Concurrent_vector_base_v4 *this, size_t count, size_t element_size,
        void (__cdecl *copy)(void*, const void*, size_t), const void *v)
{
    size_t size, seg_no, last_seg_no, remain_size;

    TRACE("(%p %Iu %Iu %p %p)\n", this, count, element_size, copy, v);

    _Concurrent_vector_base_v4__Internal_reserve(this, count, element_size,
            ~(size_t)0 / element_size);
    do {
        size = this->early_size;
        if(size >= count) return size;
     } while(InterlockedCompareExchangeSizeT(&this->early_size, count, size) != size);

    seg_no = size ? _vector_base_v4__Segment_index_of(size - 1) : 0;
    last_seg_no = _vector_base_v4__Segment_index_of(count - 1);
    remain_size = min(count, 1 << (seg_no + 1)) - size;
    if(remain_size > 0)
        copy(((BYTE**)this->segment[seg_no] + element_size * (size - ((1 << seg_no) & ~1))), v,
            remain_size);
    if(seg_no != last_seg_no)
    {
        for(seg_no++; seg_no < last_seg_no; seg_no++)
            copy(this->segment[seg_no], v, 1 << seg_no);
        copy(this->segment[last_seg_no], v, count - (1 << last_seg_no));
    }
    return size;
}

/* ?_Internal_push_back@_Concurrent_vector_base_v4@details@Concurrency@@IAEPAXIAAI@Z */
/* ?_Internal_push_back@_Concurrent_vector_base_v4@details@Concurrency@@IEAAPEAX_KAEA_K@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_push_back, 12)
void * __thiscall _Concurrent_vector_base_v4__Internal_push_back(
       _Concurrent_vector_base_v4 *this, size_t element_size, size_t *idx)
{
    size_t index, seg, segment_base;
    void *data;

    TRACE("(%p %Iu %p)\n", this, element_size, idx);

    do {
        index = this->early_size;
        _Concurrent_vector_base_v4__Internal_reserve(this, index + 1,
                element_size, ~(size_t)0 / element_size);
    } while(InterlockedCompareExchangeSizeT(&this->early_size, index + 1, index) != index);
    seg = _vector_base_v4__Segment_index_of(index);
    segment_base = (seg == 0) ? 0 : (1 << seg);
    data = (BYTE*)this->segment[seg] + element_size * (index - segment_base);
    *idx = index;

    return data;
}

/* ?_Internal_resize@_Concurrent_vector_base_v4@details@Concurrency@@IAEXIIIP6AXPAXI@ZP6AX0PBXI@Z2@Z */
/* ?_Internal_resize@_Concurrent_vector_base_v4@details@Concurrency@@IEAAX_K00P6AXPEAX0@ZP6AX1PEBX0@Z3@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_resize, 28)
void __thiscall _Concurrent_vector_base_v4__Internal_resize(
        _Concurrent_vector_base_v4 *this, size_t resize, size_t element_size,
        size_t max_size, void (__cdecl *clear)(void*, size_t),
        void (__cdecl *copy)(void*, const void*, size_t), const void *v)
{
    size_t size, seg_no, end_seg_no, clear_element;

    TRACE("(%p %Iu %Iu %Iu %p %p %p)\n", this, resize, element_size, max_size, clear, copy, v);

    if(resize > max_size) _vector_base_v4__Internal_throw_exception(this, 0);
    size = this->early_size;
    if(resize > size)
        _Concurrent_vector_base_v4__Internal_grow_to_at_least_with_result(this,
                resize, element_size, copy, v);
    else if(resize == 0)
        _Concurrent_vector_base_v4__Internal_clear(this, clear);
    else if(resize < size)
    {
        seg_no = _vector_base_v4__Segment_index_of(size - 1);
        end_seg_no = _vector_base_v4__Segment_index_of(resize - 1);
        clear_element = size - (seg_no ? 1 << seg_no : 2);
        if(clear_element > 0)
            clear(this->segment[seg_no], clear_element);
        if(seg_no) seg_no--;
        for(; seg_no > end_seg_no; seg_no--)
            clear(this->segment[seg_no], 1 << seg_no);
        clear_element = (1 << (end_seg_no + 1)) - resize;
        if(clear_element > 0)
            clear((BYTE**)this->segment[end_seg_no] + element_size * (resize - ((1 << end_seg_no) & ~1)),
                    clear_element);
        this->early_size = resize;
    }
}

/* ?_Internal_swap@_Concurrent_vector_base_v4@details@Concurrency@@IAEXAAV123@@Z */
/* ?_Internal_swap@_Concurrent_vector_base_v4@details@Concurrency@@IEAAXAEAV123@@Z */
DEFINE_THISCALL_WRAPPER(_Concurrent_vector_base_v4__Internal_swap, 8)
void __thiscall _Concurrent_vector_base_v4__Internal_swap(
        _Concurrent_vector_base_v4 *this, _Concurrent_vector_base_v4 *v)
{
    _Concurrent_vector_base_v4 temp;

    TRACE("(%p %p)\n", this, v);

    temp = *this;
    *this = *v;
    *v = temp;
    if(v->segment == this->storage)
        v->segment = v->storage;
    if(this->segment == v->storage)
        this->segment = this->storage;
}

/* ?is_current_task_group_canceling@Concurrency@@YA_NXZ */
bool __cdecl is_current_task_group_canceling(void)
{
    return Context_IsCurrentTaskCollectionCanceling();
}

/* ?_GetCombinableSize@details@Concurrency@@YAIXZ */
/* ?_GetCombinableSize@details@Concurrency@@YA_KXZ */
size_t __cdecl _GetCombinableSize(void)
{
    FIXME("() stub\n");
    return 11;
}

#if _MSVCP_VER >= 140
typedef struct {
    void *unk0;
    BYTE unk1;
} task_continuation_context;

/* ??0task_continuation_context@Concurrency@@AAE@XZ */
/* ??0task_continuation_context@Concurrency@@AEAA@XZ */
DEFINE_THISCALL_WRAPPER(task_continuation_context_ctor, 4)
task_continuation_context* __thiscall task_continuation_context_ctor(task_continuation_context *this)
{
    TRACE("(%p)\n", this);
    memset(this, 0, sizeof(*this));
    return this;
}

typedef struct {
    const vtable_ptr *vtable;
    void (__cdecl *func)(void);
    int unk[4];
    void *unk2[3];
    void *this;
} function_void_cdecl_void;

/* ?_Assign@_ContextCallback@details@Concurrency@@AAEXPAX@Z */
/* ?_Assign@_ContextCallback@details@Concurrency@@AEAAXPEAX@Z */
DEFINE_THISCALL_WRAPPER(_ContextCallback__Assign, 8)
void __thiscall _ContextCallback__Assign(void *this, void *v)
{
    TRACE("(%p %p)\n", this, v);
}

#define call_function_do_call(this) CALL_VTBL_FUNC(this, 8, void, (function_void_cdecl_void*), (this))
#define call_function_do_clean(this,b) CALL_VTBL_FUNC(this, 16, void, (function_void_cdecl_void*,bool), (this, b))
/* ?_CallInContext@_ContextCallback@details@Concurrency@@QBEXV?$function@$$A6AXXZ@std@@_N@Z */
/* ?_CallInContext@_ContextCallback@details@Concurrency@@QEBAXV?$function@$$A6AXXZ@std@@_N@Z */
DEFINE_THISCALL_WRAPPER(_ContextCallback__CallInContext, 48)
void __thiscall _ContextCallback__CallInContext(const void *this, function_void_cdecl_void func, bool b)
{
    TRACE("(%p %p %x)\n", this, func.func, b);
    call_function_do_call(func.this);
    call_function_do_clean(func.this, func.this!=&func);
}

/* ?_Capture@_ContextCallback@details@Concurrency@@AAEXXZ */
/* ?_Capture@_ContextCallback@details@Concurrency@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(_ContextCallback__Capture, 4)
void __thiscall _ContextCallback__Capture(void *this)
{
    TRACE("(%p)\n", this);
}

/* ?_Reset@_ContextCallback@details@Concurrency@@AAEXXZ */
/* ?_Reset@_ContextCallback@details@Concurrency@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(_ContextCallback__Reset, 4)
void __thiscall _ContextCallback__Reset(void *this)
{
    TRACE("(%p)\n", this);
}

/* ?_IsCurrentOriginSTA@_ContextCallback@details@Concurrency@@CA_NXZ */
bool __cdecl _ContextCallback__IsCurrentOriginSTA(void *this)
{
    TRACE("(%p)\n", this);
    return FALSE;
}

typedef struct {
    /*_Task_impl_base*/void *task;
    bool scheduled;
    bool started;
} _TaskEventLogger;

/* ?_LogCancelTask@_TaskEventLogger@details@Concurrency@@QAEXXZ */
/* ?_LogCancelTask@_TaskEventLogger@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_TaskEventLogger__LogCancelTask, 4)
void __thiscall _TaskEventLogger__LogCancelTask(_TaskEventLogger *this)
{
    TRACE("(%p)\n", this);
}

/* ?_LogScheduleTask@_TaskEventLogger@details@Concurrency@@QAEX_N@Z */
/* ?_LogScheduleTask@_TaskEventLogger@details@Concurrency@@QEAAX_N@Z */
DEFINE_THISCALL_WRAPPER(_TaskEventLogger__LogScheduleTask, 8)
void __thiscall _TaskEventLogger__LogScheduleTask(_TaskEventLogger *this, bool continuation)
{
    TRACE("(%p %x)\n", this, continuation);
}

/* ?_LogTaskCompleted@_TaskEventLogger@details@Concurrency@@QAEXXZ */
/* ?_LogTaskCompleted@_TaskEventLogger@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_TaskEventLogger__LogTaskCompleted, 4)
void __thiscall _TaskEventLogger__LogTaskCompleted(_TaskEventLogger *this)
{
    TRACE("(%p)\n", this);
}

/* ?_LogTaskExecutionCompleted@_TaskEventLogger@details@Concurrency@@QAEXXZ */
/* ?_LogTaskExecutionCompleted@_TaskEventLogger@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_TaskEventLogger__LogTaskExecutionCompleted, 4)
void __thiscall _TaskEventLogger__LogTaskExecutionCompleted(_TaskEventLogger *this)
{
    TRACE("(%p)\n", this);
}

/* ?_LogWorkItemCompleted@_TaskEventLogger@details@Concurrency@@QAEXXZ */
/* ?_LogWorkItemCompleted@_TaskEventLogger@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_TaskEventLogger__LogWorkItemCompleted, 4)
void __thiscall _TaskEventLogger__LogWorkItemCompleted(_TaskEventLogger *this)
{
    TRACE("(%p)\n", this);
}

/* ?_LogWorkItemStarted@_TaskEventLogger@details@Concurrency@@QAEXXZ */
/* ?_LogWorkItemStarted@_TaskEventLogger@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_TaskEventLogger__LogWorkItemStarted, 4)
void __thiscall _TaskEventLogger__LogWorkItemStarted(_TaskEventLogger *this)
{
    TRACE("(%p)\n", this);
}

typedef struct {
    PTP_WORK work;
    void (__cdecl *callback)(void*);
    void *arg;
} _Threadpool_chore;

/* ?_Reschedule_chore@details@Concurrency@@YAHPBU_Threadpool_chore@12@@Z */
/* ?_Reschedule_chore@details@Concurrency@@YAHPEBU_Threadpool_chore@12@@Z */
int __cdecl _Reschedule_chore(const _Threadpool_chore *chore)
{
    TRACE("(%p)\n", chore);

    SubmitThreadpoolWork(chore->work);
    return 0;
}

static void WINAPI threadpool_callback(PTP_CALLBACK_INSTANCE instance, void *context, PTP_WORK work)
{
    _Threadpool_chore *chore = context;
    TRACE("calling chore callback: %p\n", chore);
    if (chore->callback)
        chore->callback(chore->arg);
}

/* ?_Schedule_chore@details@Concurrency@@YAHPAU_Threadpool_chore@12@@Z */
/* ?_Schedule_chore@details@Concurrency@@YAHPEAU_Threadpool_chore@12@@Z */
int __cdecl _Schedule_chore(_Threadpool_chore *chore)
{
    TRACE("(%p)\n", chore);

    chore->work = CreateThreadpoolWork(threadpool_callback, chore, NULL);
    /* FIXME: what should be returned in case of error */
    if(!chore->work)
        return -1;

    return _Reschedule_chore(chore);
}

/* ?_Release_chore@details@Concurrency@@YAXPAU_Threadpool_chore@12@@Z */
/* ?_Release_chore@details@Concurrency@@YAXPEAU_Threadpool_chore@12@@Z */
void __cdecl _Release_chore(_Threadpool_chore *chore)
{
    TRACE("(%p)\n", chore);

    if(!chore->work) return;
    CloseThreadpoolWork(chore->work);
    chore->work = NULL;
}

/* ?_IsNonBlockingThread@_Task_impl_base@details@Concurrency@@SA_NXZ */
bool __cdecl _Task_impl_base__IsNonBlockingThread(void)
{
    FIXME("() stub\n");
    return FALSE;
}

/* ?ReportUnhandledError@_ExceptionHolder@details@Concurrency@@AAAXXZ */
/* ?ReportUnhandledError@_ExceptionHolder@details@Concurrency@@AAEXXZ */
/* ?ReportUnhandledError@_ExceptionHolder@details@Concurrency@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(_ExceptionHolder__ReportUnhandledError, 4)
void __thiscall _ExceptionHolder__ReportUnhandledError(void *this)
{
    FIXME("(%p) stub\n", this);
}
#endif

__ASM_BLOCK_BEGIN(concurrency_details_vtables)
    __ASM_VTABLE(_Concurrent_queue_base_v4,
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(_Concurrent_queue_base_v4_dummy)
#endif
            VTABLE_ADD_FUNC(_Concurrent_queue_base_v4_dummy)
            VTABLE_ADD_FUNC(_Concurrent_queue_base_v4_dummy)
            VTABLE_ADD_FUNC(_Concurrent_queue_base_v4_vector_dtor)
            VTABLE_ADD_FUNC(_Concurrent_queue_base_v4_dummy)
            VTABLE_ADD_FUNC(_Concurrent_queue_base_v4_dummy));
    __ASM_VTABLE(_Runtime_object,
            VTABLE_ADD_FUNC(_Runtime_object__GetId));
__ASM_BLOCK_END

void init_concurrency_details(void *base)
{
#ifdef RTTI_USE_RVA
    init__Concurrent_queue_base_v4_rtti(base);
    init__Runtime_object_rtti(base);
#endif
}
#endif

#if _MSVCP_VER >= 110
/* ?_Byte_reverse_table@details@Concurrency@@3QBEB */
const BYTE byte_reverse_table[256] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};
#endif
