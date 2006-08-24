/**
 * etd.c - USB driver stack project for Windows NT 4.0
 *
 * Copyright (c) 2002-2004 Zhiming  mypublic99@yahoo.com
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (in the main directory of the distribution, the file
 * COPYING); if not, write to the Free Software Foundation,Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <usbdriver.h>
#include "ehci.h"

#define init_elem( ptr, type, ehci_elem_type ) \
{\
	PEHCI_ELEM_LINKS tmp;\
	ptr = ( type* )( plist->phys_bufs[ i ] + sizeof( type ) * j );\
	ptr->hw_next = ehci_elem_type << 1;\
	if( ehci_elem_type == INIT_LIST_FLAG_QTD )\
		ptr->hw_next = 0;\
	tmp = ptr->elem_head_link = &plist->elem_head_buf[ i * elms_per_page + j ];\
	InitializeListHead( &tmp->elem_link );\
	InitializeListHead( &tmp->sched_link );\
	InsertTailList( &plist->free_list, &tmp->elem_link );\
	tmp->list_link = plist;\
	tmp->pool_link = pinit_ctx->pool;\
	tmp->phys_part = ( PVOID )( ( ULONG )ptr | ( ehci_elem_type << 1 ) );\
	if( ehci_elem_type != INIT_LIST_FLAG_QTD )\
		ptr->phys_addr = ( plist->phys_addrs[ i ].LowPart + sizeof( type ) * j ) | ( ehci_elem_type << 1 );\
	else \
		ptr->phys_addr = plist->phys_addrs[ i ].LowPart + sizeof( type ) * j ;\
}

// get the actual max list count of the pool, two limit, max_elem_pool and max_lists_pool
#define get_max_lists_count( pOOL, max_liSTS ) \
{\
	LONG ii1=0;\
	switch( elem_pool_get_type( pOOL ) )\
	{\
	case INIT_LIST_FLAG_QTD:\
		ii1 = EHCI_MAX_QTDS_LIST;\
		break;\
	case INIT_LIST_FLAG_FSTN:\
		ii1 = EHCI_MAX_QHS_LIST;\
		break;\
	case INIT_LIST_FLAG_SITD:\
		ii1 = EHCI_MAX_ITDS_LIST;\
		break;\
	case INIT_LIST_FLAG_QH:  \
		ii1 = EHCI_MAX_SITDS_LIST;\
		break;\
	case INIT_LIST_FLAG_ITD: \
		ii1 = EHCI_MAX_FSTNS_LIST;\
		break;\
	}\
	max_liSTS = ( EHCI_MAX_ELEMS_POOL / ii1 ) > EHCI_MAX_LISTS_POOL ? EHCI_MAX_LISTS_POOL : ( EHCI_MAX_ELEMS_POOL / ii1 );\
}

VOID elem_list_destroy_elem_list(PEHCI_ELEM_LIST plist);

PLIST_ENTRY elem_list_get_list_head(PEHCI_ELEM_LIST plist);

LONG elem_list_get_total_count(PEHCI_ELEM_LIST plist);

LONG elem_list_get_elem_size(PEHCI_ELEM_LIST plist);

LONG elem_list_get_link_offset(PEHCI_ELEM_LIST plist);

LONG elem_list_add_ref(PEHCI_ELEM_LIST plist);

LONG elem_list_release_ref(PEHCI_ELEM_LIST plist);

LONG elem_list_get_ref(PEHCI_ELEM_LIST plist);

BOOLEAN
elem_pool_lock(PEHCI_ELEM_POOL pool, BOOLEAN at_dpc)
{
    UNREFERENCED_PARAMETER(pool);
    UNREFERENCED_PARAMETER(at_dpc);

    return TRUE;
}

BOOLEAN
elem_pool_unlock(PEHCI_ELEM_POOL pool, BOOLEAN at_dpc)
{
    UNREFERENCED_PARAMETER(pool);
    UNREFERENCED_PARAMETER(at_dpc);

    return TRUE;
}

LONG
get_elem_phys_part_size(ULONG type)
{
    // type is INIT_LIST_FLAG_XXX
    LONG size;

    size = 0;
    switch (type)
    {
        case INIT_LIST_FLAG_ITD:
            size = 64;
            break;
        case INIT_LIST_FLAG_SITD:
            size = 28;
            break;
        case INIT_LIST_FLAG_QTD:
            size = 32;
            break;
        case INIT_LIST_FLAG_QH:
            size = 48;
            break;
        case INIT_LIST_FLAG_FSTN:
            size = 8;
            break;
    }
    return size;
}

BOOLEAN
elem_list_init_elem_list(PEHCI_ELEM_LIST plist, LONG init_flags, PVOID context, LONG count)
{
    LONG pages, i, j, elms_per_page;
    PEHCI_QH pqh;
    PEHCI_ITD pitd;
    PEHCI_SITD psitd;
    PEHCI_QTD pqtd;
    PEHCI_FSTN pfstn;
    PINIT_ELEM_LIST_CONTEXT pinit_ctx;

    UNREFERENCED_PARAMETER(count);

    if (plist == NULL || context == NULL)
        return FALSE;

    RtlZeroMemory(plist, sizeof(EHCI_ELEM_LIST));

    pinit_ctx = context;

    plist->destroy_list = elem_list_destroy_elem_list;
    plist->get_list_head = elem_list_get_list_head;
    plist->get_total_count = elem_list_get_total_count;
    plist->get_elem_size = elem_list_get_elem_size;
    plist->get_link_offset = elem_list_get_link_offset;
    plist->add_ref = elem_list_add_ref;
    plist->release_ref = elem_list_release_ref;
    plist->get_ref = elem_list_get_ref;

    InitializeListHead(&plist->free_list);

    switch (init_flags & 0x0f)
    {
        case INIT_LIST_FLAG_ITD:
            plist->total_count = EHCI_MAX_ITDS_LIST;
            plist->elem_size = sizeof(EHCI_ITD);
            break;
        case INIT_LIST_FLAG_QH:
            plist->total_count = EHCI_MAX_QHS_LIST;
            plist->elem_size = sizeof(EHCI_QH);
            break;
        case INIT_LIST_FLAG_SITD:
            plist->total_count = EHCI_MAX_SITDS_LIST;
            plist->elem_size = sizeof(EHCI_SITD);
            break;
        case INIT_LIST_FLAG_FSTN:
            plist->total_count = EHCI_MAX_FSTNS_LIST;
            plist->elem_size = sizeof(EHCI_FSTN);
            break;
        case INIT_LIST_FLAG_QTD:
            plist->total_count = EHCI_MAX_QTDS_LIST;
            plist->elem_size = sizeof(EHCI_QTD);
            break;
        default:
            goto ERROR_OUT;
    }
    if (plist->elem_size & 0x1f)
    {
        plist->total_count = 0;
        goto ERROR_OUT;
    }

    plist->flags = init_flags;
    plist->parent_pool = pinit_ctx->pool;
    plist->padapter = pinit_ctx->padapter;
    pages = ((plist->elem_size * plist->total_count) + (PAGE_SIZE - 1)) / PAGE_SIZE;
    elms_per_page = PAGE_SIZE / plist->elem_size;

    plist->phys_addrs = usb_alloc_mem(NonPagedPool,
                                      (sizeof(PHYSICAL_ADDRESS) + sizeof(PBYTE)) * pages +
                                      sizeof(EHCI_ELEM_LINKS) * plist->total_count);

    if (plist->phys_addrs == NULL)
    {
        plist->total_count = 0;
        goto ERROR_OUT;
    }

    plist->phys_bufs = (PBYTE *) & plist->phys_addrs[pages];
    plist->elem_head_buf = (PEHCI_ELEM_LINKS) & plist->phys_bufs[pages];
    RtlZeroMemory(plist->phys_addrs,
                  (sizeof(PHYSICAL_ADDRESS) + sizeof(PBYTE)) * pages +
                  sizeof(EHCI_ELEM_LINKS) * plist->total_count);

    for(i = 0; i < pages; i++)
    {
        plist->phys_bufs[i] = HalAllocateCommonBuffer(plist->padapter,
                                                      PAGE_SIZE, &plist->phys_addrs[i], FALSE);

        if (plist->phys_bufs[i] == NULL)
        {
            // failed, roll back
            for(j = i - 1; j >= 0; j--)
                HalFreeCommonBuffer(plist->padapter,
                                    PAGE_SIZE, plist->phys_addrs[j], plist->phys_bufs[j], FALSE);
            goto ERROR_OUT;
        }
        RtlZeroMemory(plist->phys_bufs[i], PAGE_SIZE);
        for(j = 0; j < elms_per_page; j++)
        {
            switch (init_flags & 0xf)
            {
                case INIT_LIST_FLAG_QH:
                {
                    init_elem(pqh, EHCI_QH, INIT_LIST_FLAG_QH);
                    break;
                }
                case INIT_LIST_FLAG_ITD:
                {
                    init_elem(pitd, EHCI_ITD, INIT_LIST_FLAG_ITD);
                    break;
                }
                case INIT_LIST_FLAG_QTD:
                {
                    init_elem(pqtd, EHCI_QTD, INIT_LIST_FLAG_QTD);
                    break;
                }
                case INIT_LIST_FLAG_SITD:
                {
                    init_elem(psitd, EHCI_SITD, INIT_LIST_FLAG_SITD);
                    break;
                }
                case INIT_LIST_FLAG_FSTN:
                {
                    init_elem(pfstn, EHCI_FSTN, INIT_LIST_FLAG_FSTN);
                    break;
                }
                default:
                    TRAP();
            }
        }
    }
    return TRUE;

ERROR_OUT:
    if (plist->phys_addrs != NULL)
        usb_free_mem(plist->phys_addrs);

    RtlZeroMemory(plist, sizeof(EHCI_ELEM_LIST));
    return FALSE;
}

VOID
elem_list_destroy_elem_list(PEHCI_ELEM_LIST plist)
{
    LONG i, pages;

    if (plist == NULL)
        return;

    pages = (plist->total_count * plist->elem_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for(i = 0; i < pages; i++)
        HalFreeCommonBuffer(plist->padapter, PAGE_SIZE, plist->phys_addrs[i], plist->phys_bufs[i], FALSE);

    usb_free_mem(plist->phys_addrs);
    RtlZeroMemory(plist, sizeof(EHCI_ELEM_LIST));
}

PLIST_ENTRY
elem_list_get_list_head(PEHCI_ELEM_LIST plist)
{
    if (plist == NULL)
        return NULL;
    return &plist->free_list;
}

LONG
elem_list_get_total_count(PEHCI_ELEM_LIST plist)
{
    if (plist == NULL)
        return 0;
    return plist->total_count;;
}

LONG
elem_list_get_elem_size(PEHCI_ELEM_LIST plist)
{
    if (plist == NULL)
        return 0;
    return plist->elem_size;
}

LONG
elem_list_get_link_offset(PEHCI_ELEM_LIST plist)
{
    if (plist == NULL)
        return 0;

    return get_elem_phys_part_size(plist->flags & 0xf);
}

LONG
elem_list_add_ref(PEHCI_ELEM_LIST plist)
{
    plist->reference++;
    return plist->reference;
}

LONG
elem_list_release_ref(PEHCI_ELEM_LIST plist)
{
    plist->reference--;
    return plist->reference;
}

LONG
elem_list_get_ref(PEHCI_ELEM_LIST plist)
{
    return plist->reference;
}

//
// pool methods
//

BOOLEAN
elem_pool_init_pool(PEHCI_ELEM_POOL pool, LONG flags, PVOID context)
{
    INIT_ELEM_LIST_CONTEXT init_ctx;

    if (pool == NULL || context == NULL)
        return FALSE;

    RtlZeroMemory(pool, sizeof(EHCI_ELEM_POOL));

    init_ctx.pool = pool;
    init_ctx.padapter = context;

    pool->elem_lists[0] = usb_alloc_mem(NonPagedPool, sizeof(EHCI_ELEM_LIST));

    if (pool->elem_lists[0] == NULL)
        return FALSE;

    if (elem_list_init_elem_list(pool->elem_lists[0], flags, &init_ctx, 0) == FALSE)
    {
        usb_free_mem(pool->elem_lists[0]);
        return FALSE;
    }
    pool->link_offset = pool->elem_lists[0]->get_link_offset(pool->elem_lists[0]);
    pool->free_count = pool->elem_lists[0]->get_total_count(pool->elem_lists[0]);
    pool->list_count = 1;
    pool->flags = flags;

    return TRUE;
}

LONG
elem_pool_get_link_offset(PEHCI_ELEM_POOL elem_pool)
{
    return elem_pool->link_offset;
}

LONG
elem_pool_get_total_count(PEHCI_ELEM_POOL elem_pool)
{
    return elem_pool->elem_lists[0]->get_total_count(elem_pool->elem_lists[0]) * elem_pool->list_count;
}

VOID
elem_pool_destroy_pool(PEHCI_ELEM_POOL pool)
{
    LONG i;
    if (pool == NULL)
        return;
    for(i = pool->list_count - 1; i >= 0; i--)
    {
        pool->elem_lists[i]->destroy_list(pool->elem_lists[i]);
        usb_free_mem(pool->elem_lists[i]);
        pool->elem_lists[i] = NULL;
    }
    RtlZeroMemory(pool, sizeof(EHCI_ELEM_POOL));
    return;
}

PEHCI_ELEM_LINKS
elem_pool_alloc_elem(PEHCI_ELEM_POOL pool)
{
    LONG i;
    PEHCI_ELEM_LIST pel = NULL;
    PLIST_HEAD lh;
    PEHCI_ELEM_LINKS elnk;

    if (pool == NULL)
        return NULL;

    for(i = 0; i < pool->list_count; i++)
    {
        pel = pool->elem_lists[i];
        if (pel->get_ref(pel) == pel->get_total_count(pel))
            continue;
        break;
    }
    if (i == pool->list_count)
    {
        if (elem_pool_expand_pool(pool, pel->get_total_count(pel)) == FALSE)
            return NULL;
        pel = pool->elem_lists[i];
    }

    lh = pel->get_list_head(pel);
    elnk = (PEHCI_ELEM_LINKS) RemoveHeadList(lh);
    InitializeListHead(&elnk->elem_link);
    InitializeListHead(&elnk->sched_link);

    pel->add_ref(pel);
    pool->free_count--;

    return elnk;
}

VOID
elem_pool_free_elem(PEHCI_ELEM_LINKS elem_link)
{
    PLIST_HEAD lh;
    LONG ref;
    PEHCI_ELEM_POOL pool;
    if (elem_link == NULL)
        return;
    pool = elem_link->pool_link;
    lh = elem_link->list_link->get_list_head(elem_link->list_link);
    if (lh == NULL)
        return;
    InsertHeadList(lh, (PLIST_ENTRY) elem_link);
    ref = elem_link->list_link->release_ref(elem_link->list_link);
    pool->free_count++;
    if (ref == 0)
        elem_pool_collect_garbage(pool);
    return;
}

BOOLEAN
elem_pool_is_empty(PEHCI_ELEM_POOL pool)
{
    PEHCI_ELEM_LIST pel;

    if (pool == NULL)
        return TRUE;
    pel = pool->elem_lists[0];
    return (BOOLEAN) (pool->list_count == 1 && pool->free_count == pel->get_total_count(pel));
}

LONG
elem_pool_get_free_count(PEHCI_ELEM_POOL pool)
{
    if (pool == NULL)
        return 0;
    return pool->free_count;
}

PEHCI_ELEM_LINKS
elem_pool_alloc_elems(PEHCI_ELEM_POOL pool, LONG count)
{
    LIST_HEAD lh;
    PLIST_ENTRY pthis;
    LONG i, alloc_count, max_pool_lists;
    PEHCI_ELEM_LIST pel;
    PEHCI_ELEM_LINKS elnk;
    // calculate to see if the count is affordable

    if (pool == NULL || count <= 0)
        return NULL;

    get_max_lists_count(pool, max_pool_lists);
    InitializeListHead(&lh);
    pel = pool->elem_lists[0];
    if (count <= pool->free_count)
        alloc_count = 0;
    else
        alloc_count = count - pool->free_count;

    if (alloc_count > pel->get_total_count(pel) * (max_pool_lists - pool->list_count))
        return NULL;

    for(i = 0; i < count; i++)
    {
        if ((elnk = elem_pool_alloc_elem(pool)) == NULL)
        {
            // undo what we have done
            while (IsListEmpty(&lh) == FALSE)
            {
                pthis = RemoveHeadList(&lh);
                elnk = struct_ptr(pthis, EHCI_ELEM_LINKS, elem_link);
                elem_pool_free_elem(elnk);
            }
            return NULL;
        }
        InsertTailList(&lh, &elnk->elem_link);
    }
    ListFirst(&lh, pthis);
    elnk = struct_ptr(pthis, EHCI_ELEM_LINKS, elem_link);
    RemoveEntryList(&lh);
    return elnk;
}

BOOLEAN
elem_pool_free_elems(PEHCI_ELEM_LINKS elem_chains)
{
    // note: no list head exists.
    LIST_HEAD lh;
    PEHCI_ELEM_LINKS elnk;

    InsertTailList(&elem_chains->elem_link, &lh);
    while (IsListEmpty(&lh) == FALSE)
    {
        elnk = (PEHCI_ELEM_LINKS) RemoveHeadList(&lh);
        elem_pool_free_elem(elnk);
    }
    return TRUE;
}

LONG
elem_pool_get_type(PEHCI_ELEM_POOL pool)
{
    if (pool == NULL)
        return -1;
    return (pool->flags & 0xf);
}

BOOLEAN
elem_pool_expand_pool(PEHCI_ELEM_POOL pool, LONG elem_count)
{
    LONG elem_cnt_list, list_count, i, j;
    INIT_ELEM_LIST_CONTEXT init_ctx;

    if (pool == NULL || elem_count <= 0 || elem_count > EHCI_MAX_ELEMS_POOL)
        return FALSE;

    init_ctx.pool = pool;
    init_ctx.padapter = pool->elem_lists[0]->padapter;

    elem_cnt_list = pool->elem_lists[0]->get_total_count(pool->elem_lists[0]);
    list_count = (elem_count + elem_cnt_list - 1) / elem_cnt_list;
    get_max_lists_count(pool, i);

    if (list_count + pool->list_count > i)
        return FALSE;

    for(i = pool->list_count; i < list_count + pool->list_count; i++)
    {
        pool->elem_lists[i] = usb_alloc_mem(NonPagedPool, sizeof(EHCI_ELEM_LIST));
        if (elem_list_init_elem_list(pool->elem_lists[i], pool->flags, &init_ctx, 0) == FALSE)
            break;
    }

    if (i < list_count + pool->list_count)
    {
        // undo all we have done
        for(j = pool->list_count; j < pool->list_count + i; j++)
        {
            pool->elem_lists[j]->destroy_list(pool->elem_lists[j]);
            usb_free_mem(pool->elem_lists[j]);
            pool->elem_lists[j] = NULL;
        }
        return FALSE;
    }

    // update pool
    pool->free_count += elem_cnt_list * list_count;
    pool->list_count += list_count;
    return TRUE;
}

BOOLEAN
elem_pool_collect_garbage(PEHCI_ELEM_POOL pool)
{
    LONG i, j, k, fl;
    LONG free_elem_lists[EHCI_MAX_LISTS_POOL - 1];
    PEHCI_ELEM_LIST pel;

    if (pool == NULL)
        return FALSE;

    for(i = 1, fl = 0; i < pool->list_count; i++)
    {
        if (pool->elem_lists[i]->get_ref(pool->elem_lists[i]) == 0)
        {
            free_elem_lists[fl++] = i;
        }
    }
    for(j = fl - 1; j >= 0; j--)
    {
        pel = pool->elem_lists[free_elem_lists[j]];
        pel->destroy_list(pel);
        usb_free_mem(pel);

        for(k = free_elem_lists[j] + 1; k < pool->list_count; k++)
        {
            // shrink the elem_lists
            pool->elem_lists[k - 1] = pool->elem_lists[k];
        }
        pool->elem_lists[k] = NULL;
        pel = pool->elem_lists[0];
        pool->free_count -= pel->get_total_count(pel);
        pool->list_count--;
    }
    return TRUE;
}

BOOLEAN
elem_pool_can_transfer(PEHCI_ELEM_POOL pool, LONG td_count)
{
    LONG i;
    if (pool == NULL || td_count <= 0)
        return FALSE;
    get_max_lists_count(pool, i);
    if ((i - pool->list_count)
        * pool->elem_lists[0]->get_total_count(pool->elem_lists[0]) + pool->free_count < td_count)
        return FALSE;
    return TRUE;
}

//----------------------------------------------------------
